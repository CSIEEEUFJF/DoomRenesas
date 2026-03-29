# Touch, Buttons, and Input

This document describes the project's input path: resistive touch, physical buttons, and translation into Doom engine keyboard events.

## Overview

The project uses two input paths:

- integrated board touch, read directly through `SX8654` controller;
- physical buttons `S4` and `S5`.

Instead of depending on full GUIX input stack, the port accesses touch at lower level through `RIIC2` and converts gestures into keys already understood by `doomgeneric`.

## Main files

- `src/doomgeneric_renesas.c`  
  Touch init, controller polling, calibration, and key mapping.
- `src/synergy_gen/common_data.c`  
  `g_i2c` instance used to communicate with `SX8654`.
- `src/synergy_gen/pin_data.c`  
  Generated pin configuration.
- `src/doom_port_status.c`  
  Failure/state registration in `doom_error_msg`.

## Hardware and signals used

- touch controller: `SX8654`
- bus: `IIC2`
- configured address: `0x48`
- touch reset pin: `TOUCH_RESET_PIN`
- physical fire buttons: `S4` and `S5`

## Touch initialization sequence

Touch is initialized by `renesas_touch_init()` in `src/doomgeneric_renesas.c`.

Current flow:

1. open `g_i2c`;
2. reset touch controller;
3. configure operation registers;
4. enable `X/Y` channels;
5. configure `pendown` and `penrelease` interrupts;
6. put controller in pen-triggered acquisition mode.

If any stage fails, touch is disabled and failure is recorded in `doom_error_msg`.

## Input model used by game

Doom still receives events as if running on keyboard:

- `KEY_LEFTARROW`
- `KEY_RIGHTARROW`
- `KEY_UPARROW`
- `KEY_DOWNARROW`
- `KEY_STRAFE_L`
- `KEY_STRAFE_R`
- `KEY_FIRE`

Renesas backend keeps a small event queue (`g_key_queue`) and translates touch/button states into these keys.

## Current visual overlay

Project draws two circular pads over image:

- left pad: movement and strafe;
- right pad: camera turning;
- `S4` and `S5`: fire.

Important:

- overlay is only a visual representation;
- actual logic depends on coordinate mapping in `touch_pad_mask()`;
- panel is effectively treated as single-touch path during gameplay.

## Coordinate conversion

Coordinates read from `SX8654` are raw and must be transformed into the logical space used by game.

Current backend does:

1. read `raw_x` and `raw_y`;
2. gradually expand observed range;
3. scale to:
   - user logical width: `320`
   - user logical height: `240`

Initial calibration limits are conservative:

- default minimum: `512`
- default maximum: `3072`

These values live in `src/doomgeneric_renesas.c`.

## Current module state

Touch already responds on real hardware, but is still under fine tuning. In particular:

- pad geometry may still need refinement;
- forward/back and camera response may vary depending on perceived panel orientation;
- small calibration changes can strongly affect control feel.

So this module should be treated as functional, but still in stabilization.

## Practical limitations

- usage is effectively **single-touch** for gameplay.
- behavior depends on final visual display orientation.
- regenerating Synergy files may require rechecking `IIC2`, pins, and touch reset.
- overlay does not use GUIX, so all visual/geometry changes must be adjusted in backend port code.

## Physical buttons

Buttons `S4` and `S5` are read by GPIO and, when pressed, generate `KEY_FIRE`.

This path is simpler and more stable than touch, so it is a useful input-debug reference: if `S4/S5` work and touch does not, issue usually lies in `SX8654 -> I2C -> calibration -> mapping` path.

## Input debugging

If touch stops responding or starts responding incoherently:

1. check `doom_error_msg`;
2. confirm `g_i2c` opened correctly;
3. review `renesas_touch_init()`;
4. review `touch_poll_controller()`;
5. adjust `touch_pad_mask()` and pad parameters;
6. verify logical orientation used in `g_touch_state.x` and `g_touch_state.y`.

## Most important tuning points

To recalibrate or change pad behavior, best entry points are:

- `TOUCH_PAD_LEFT_X`
- `TOUCH_PAD_RIGHT_X`
- `TOUCH_PAD_CENTER_Y`
- `TOUCH_PAD_RADIUS`
- `TOUCH_PAD_DEADZONE`
- `TOUCH_CAL_MIN_DEFAULT`
- `TOUCH_CAL_MAX_DEFAULT`
- function `touch_pad_mask()`
- function `touch_poll_controller()`
