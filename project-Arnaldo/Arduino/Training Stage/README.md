# 8-Arm Radial Maze: Training Stages (Arduino)

Arduino Mega sketches for the three training stages of a mouse task in an automated 8-arm radial maze (Zutshi Lab). Each stage is a separate sketch; upload the one for the stage you are running.

Only four arms are used: **arm 1 is the homeport**, and **arms 4, 5 and 6** sit opposite it. Arms 2, 3, 7 and 8 are not touched by any of the code.

## Folder layout

```
maze_training/
├── maze_stage1/maze_stage1.ino   Stage 1: water at every arm, never twice in a row
├── maze_stage2/maze_stage2.ino   Stage 2: one fixed reward arm for the whole session
└── maze_stage3/maze_stage3.ino   Stage 3: a new random reward arm every cycle
```

Each sketch sits in a folder of the same name, as the Arduino IDE requires.

## Hardware

| Arm | Role | Door servo pin | Pump pin (active LOW) | Lick sensor pin (HIGH = beam broken) |
|---|---|---|---|---|
| 1 | Homeport | 2 | 30 | 38 |
| 4 | Arm | not used (door kept open) | 33 | 41 |
| 5 | Arm | not used (door kept open) | 34 | 42 |
| 6 | Arm | not used (door kept open) | 35 | 43 |

- **Board:** Arduino Mega 2560.
- **Pumps:** active LOW. One reward is a 100 ms pulse (`pumpTime`).
- **Lick sensors:** IR beam-break, read as plain digital inputs; HIGH means the beam is broken.
- **Homeport door:** a servo on pin 2. Position 175 is closed, 120 is open.
- **Doors 4, 5, 6:** not controlled by the code; set them open by hand before a session.

## What is common to all three stages

1. **Power-on:** the homeport door moves to closed, and all sensors are ignored for the first 2 seconds (`startupWait`) while the door and sensors settle.
2. **Trial start:** the first lick at the homeport gives water and opens the homeport door. The door then stays open for the rest of the session and is never moved again.
3. **Lick detection:** a lick counts once per beam break and only if the beam stays broken for at least 10 ms (`lickConfirmTime`). Holding the tongue in the beam does not trigger repeated rewards.
4. **Pumps:** every pump starts off at power-on, so no water is released before the session starts.

## Stage rules

### Stage 1: water at every arm, never twice in a row
- Water is available at arms 1, 4, 5 and 6.
- After the mouse gets water at an arm, that arm is blocked.
- The blocked arm is unblocked as soon as the mouse gets water at a different arm, so only the most recently rewarded arm is ever blocked.
- Example: water at 4 → arm 4 blocked → water at 5 → arm 4 available again, arm 5 blocked.

### Stage 2: one fixed reward arm for the whole session
- At the start only the homeport gives water.
- At the first homeport lick, one of arms 4, 5 or 6 is picked at random as the reward arm, and it stays the same until the board is reset.
- The task alternates between two states:
  - **Water at the homeport:** after the mouse drinks there, water moves to the reward arm and the homeport gives nothing.
  - **Water at the reward arm:** after the mouse drinks there, water moves back to the homeport.
- Licks anywhere else do nothing.

### Stage 3: a new random reward arm every cycle
- Same as stage 2, except a new reward arm is picked at random (from 4, 5, 6) at **every** rewarded homeport lick.
- The picks are independent, so the same arm can come up twice in a row.

## Running a session

1. Turn on the pump power supply. The Mega's USB power alone cannot drive the pumps.
2. Set doors 4, 5 and 6 open by hand. The homeport door closes itself at power-on.
3. Open the sketch for the stage in the Arduino IDE, select **Arduino Mega or Mega 2560**, and upload.
4. Open the Serial Monitor at **57600 baud**. Opening it resets the board, which restarts the session.
5. Wait for the `# time_ms,event,arm,detail` header line before putting the mouse in the homeport.
6. At the end, copy the Serial Monitor text into a file. The monitor does not save anything itself.

## Serial log format

Each event is one comma-separated line: `time_ms,event,arm,detail`. `time_ms` counts from the last reset.

| Event | Detail | Meaning |
|---|---|---|
| `lick` | `water` | Lick at an arm that had water; one pump pulse was given |
| `lick` | `no_water` | Lick at an arm with no water at that moment |
| `door` | `opened` | Homeport door opened (first homeport lick only) |
| `reward_arm` | `same_all_session` | Stage 2: the reward arm for this cycle (always the same arm) |
| `reward_arm` | `new_this_cycle` | Stage 3: the newly picked reward arm for this cycle |

Lines starting with `#` are comments. If you see `# WARNING: sensor already triggered at start, arm N`, that sensor read "beam broken" at the end of the startup wait. Check that nothing is in the beam and that the sensor is aligned; that arm will not register licks until its beam clears.

Example (stage 2):

```
# Stage 2: one fixed reward arm (4, 5 or 6) for the whole session
# time_ms,event,arm,detail
4019,lick,1,water
4019,reward_arm,5,same_all_session
4019,door,1,opened
6019,lick,4,no_water
7019,lick,5,water
9019,lick,1,water
9019,reward_arm,5,same_all_session
```

## Settings you may need to change

All settings are at the top of each sketch.

| Setting | Default | What it does |
|---|---|---|
| `doorOpenPos` / `doorClosedPos` | 120 / 175 | Homeport door positions. If the door does the opposite of what you expect, swap them. |
| `pumpTime` | 100 ms | Length of one water pulse; sets the drop size. |
| `startupWait` | 2000 ms | Time after power-on during which sensors are ignored. |
| `lickConfirmTime` | 10 ms | How long the beam must stay broken to count as a lick. |

Change the same setting in all three sketches if you want the stages to stay consistent.

## Troubleshooting

- **Serial Monitor shows licks but no water comes out:** the pump power supply is off, or the line needs priming (a 100 ms pulse cannot pull water through an empty tube).
- **Door does the opposite of what it should:** swap `doorOpenPos` and `doorClosedPos`.
- **The behaviour looks like an old version of the code:** close the sketch in the Arduino IDE *without saving*, reopen it from this folder, and upload again. The IDE keeps its own copy of an open sketch and can save it back over a newer file.

## Not included yet

- **Open Ephys TTL output** (pins 22–26): left out until the lab decides how lick events will reach the recording system.
- **The final task with the overhead light:** not part of these training stages.
