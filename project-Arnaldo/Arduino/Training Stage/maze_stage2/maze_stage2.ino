/*
  8-arm maze training, STAGE 2: one fixed reward arm for the whole session

  Arms used: 1 (homeport), 4, 5, 6.

  - Power-on: the homeport door moves to CLOSED. Sensors are ignored for
    the first 2 seconds while the door and sensors settle.
  - Then water is available at the homeport only.
  - First lick at the homeport: water, and the door moves to OPEN and
    stays there. The door is never moved again.
  - After water at the homeport, water moves to the reward arm
    (one of 4, 5, 6). The homeport gives nothing.
  - After water at the reward arm, water moves back to the homeport.
  - Licks anywhere else do nothing.
  - The reward arm is picked at random at the first homeport lick
    and stays the same until the board is reset.

  Board: Arduino Mega. Serial Monitor: 57600 baud.
*/

#include <Servo.h>

/* ==================== PINS ============================================*/

// Only arms 1, 4, 5, 6 are used. Arms 2, 3, 7, 8 are never touched.
const int NUM_PORTS = 4;
const int HOME = 0;   // position of the homeport (arm 1) in the lists below

const int armNumber[NUM_PORTS] = {1, 4, 5, 6};
const int pumpPins[NUM_PORTS]  = {30, 33, 34, 35};   // active LOW
const int lickPins[NUM_PORTS]  = {38, 41, 42, 43};   // HIGH = beam broken

// Only the homeport door is controlled. Doors 4, 5, 6 stay open by hand.
const int homeDoorPin = 2;


/* ==================== SETTINGS ========================================*/

const int sensorTriggeredState = HIGH;

// Door servo positions, as seen on the rig with the hardware test sketch.
// If the door ever does the opposite, swap these two numbers.
const int doorOpenPos   = 120;
const int doorClosedPos = 175;

const unsigned long pumpTime = 100;         // ms of water per reward
const unsigned long startupWait = 2000;     // ms after power-on with sensors ignored
const unsigned long lickConfirmTime = 10;   // ms the beam must stay broken to count as a lick


/* ==================== STATE ===========================================*/

Servo homeDoor;

bool doorOpened = false;     // true after the first homeport lick

bool pumpOn[NUM_PORTS] = {false, false, false, false};
unsigned long pumpStartTime[NUM_PORTS];

bool lastReading[NUM_PORTS];   // raw sensor reading on the previous check
bool beamBroken[NUM_PORTS];    // confirmed sensor state
unsigned long lastChangeTime[NUM_PORTS];

// The one place where a lick gives water right now
int waterPort = HOME;

// Reward arm for the whole session (position 1, 2 or 3 = arm 4, 5 or 6)
int targetPort = -1;   // -1 = not picked yet


/* ==================== HELPERS =========================================*/

// One line per event: time_ms,event,arm,detail
void logEvent(const char* event, int arm, const char* detail) {
  Serial.print(millis());
  Serial.print(',');
  Serial.print(event);
  Serial.print(',');
  Serial.print(arm);
  Serial.print(',');
  Serial.println(detail);
}

// Start one pulse of water. updatePumps() switches it off again.
void giveWater(int p) {
  digitalWrite(pumpPins[p], LOW);
  pumpOn[p] = true;
  pumpStartTime[p] = millis();
}

void updatePumps() {
  for (int p = 0; p < NUM_PORTS; p++) {
    if (pumpOn[p] && millis() - pumpStartTime[p] >= pumpTime) {
      digitalWrite(pumpPins[p], HIGH);
      pumpOn[p] = false;
    }
  }
}

// Move the homeport door to OPEN. It is never moved again by this sketch.
void openHomeDoor() {
  homeDoor.write(doorOpenPos);
  doorOpened = true;
  logEvent("door", armNumber[HOME], "opened");
}

// True only at the moment a new lick is confirmed: the beam went from
// clear to broken and stayed broken for lickConfirmTime.
// Very short blips (electrical noise) are ignored.
bool lickStarted(int p) {
  bool reading = (digitalRead(lickPins[p]) == sensorTriggeredState);

  if (reading != lastReading[p]) {
    lastReading[p] = reading;
    lastChangeTime[p] = millis();
  }

  if (reading != beamBroken[p] && millis() - lastChangeTime[p] >= lickConfirmTime) {
    beamBroken[p] = reading;
    return reading;
  }
  return false;
}


/* ==================== TASK LOGIC ======================================*/

void handleLick(int p) {

  // Wrong place (wrong arm, or a place that already gave its water)
  if (p != waterPort) {
    logEvent("lick", armNumber[p], "no_water");
    return;
  }

  giveWater(p);
  logEvent("lick", armNumber[p], "water");

  if (p == HOME) {

    // First homeport lick starts the session
    bool firstLick = !doorOpened;

    if (firstLick) {
      randomSeed(micros());                 // lick timing makes each session different
      targetPort = random(1, NUM_PORTS);    // 1, 2 or 3 -> arm 4, 5 or 6
    }

    // Water moves to the reward arm (the same arm all session)
    waterPort = targetPort;
    logEvent("reward_arm", armNumber[waterPort], "same_all_session");

    if (firstLick) {
      openHomeDoor();
    }
  }
  else {
    // Water moves back to the homeport
    waterPort = HOME;
  }
}


/* ==================== SETUP ===========================================*/

void setup() {

  Serial.begin(57600);

  for (int p = 0; p < NUM_PORTS; p++) {
    pinMode(lickPins[p], INPUT);

    // Set HIGH before switching to OUTPUT so the pump never fires at power-on
    digitalWrite(pumpPins[p], HIGH);
    pinMode(pumpPins[p], OUTPUT);

    pumpStartTime[p] = 0;
  }

  // Close the homeport door
  homeDoor.attach(homeDoorPin);
  homeDoor.write(doorClosedPos);

  Serial.println("# Stage 2: one fixed reward arm (4, 5 or 6) for the whole session");

  // Let the door finish moving and the sensors settle. Nothing counts as a lick yet.
  delay(startupWait);

  // Start from whatever each sensor reads now, so only a NEW beam break counts
  for (int p = 0; p < NUM_PORTS; p++) {
    bool reading = (digitalRead(lickPins[p]) == sensorTriggeredState);
    lastReading[p] = reading;
    beamBroken[p] = reading;
    lastChangeTime[p] = millis();

    if (reading) {
      Serial.print("# WARNING: sensor already triggered at start, arm ");
      Serial.println(armNumber[p]);
    }
  }

  Serial.println("# time_ms,event,arm,detail");
}


/* ==================== LOOP ============================================*/

void loop() {

  updatePumps();

  for (int p = 0; p < NUM_PORTS; p++) {
    if (lickStarted(p)) {
      handleLick(p);
    }
  }
}
