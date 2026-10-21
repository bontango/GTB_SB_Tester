# Gottlieb System 80 Pinball Tester

Firmware for a PIC18F45K22 based tester for Gottlieb System 80 pinball boards.
It drives the lamp, sound and solenoid outputs so that boards can be checked
without a working MPU. Output is shown on a 20x4 HD44780 LCD and controlled
with five buttons (OK, up, down, left, right).

More test equipment: <https://lisy.dev/swrep/test_equipment>

## Tests

| Test      | Options                                   | Range (default) |
|-----------|-------------------------------------------|-----------------|
| Lamps     | all lamps, several aux lampboard variants | 2 ... 48        |
| Sound     | small SB, big SB 80/80A, big SB 80B       | 1 ... 15        |
| Solenoids | SOL1 ... SOL9                             | 1 ... 9         |

The sound number is output in binary on the lines Sound 1/2/4/8.
In the menu, line 3 sets the from/to range and line 4 the time between steps.
Lamp time 0 switches to manual mode (step with left/right).
Press and hold OK to cancel a running test.

## Build

The project is an MPLAB X project imported into VS Code with the
MPLAB extension (CMake/Ninja).

- Compiler: XC8 v3.10, device pack PIC18F-K_DFP 1.5.114
- Build: MPLAB sidebar, then *Build* (or `F7` with the CMake extension)
- Output: `out/GTB_SB_Tester/default.hex`

## Version history

- **0.3** SOL8/SOL9 fixed, sound test drives the sound lines instead of the
  solenoids, from/to range limited, time 0 no longer blocks cancel,
  switch debounce fixed
- **0.1** first version, based on the Bally tester

## Author

bontango - <https://lisy.dev>
