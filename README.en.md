# DoomRenesas

DoomRenesas is a port of `doomgeneric` for the Renesas Synergy S7G2 Starter Kit. The project targets the board's integrated 240x320 TFT display and runs the game without external SDRAM by keeping the display path, WAD storage, and runtime memory usage as small as possible.

The current boot flow starts Doom directly into `E1M1` with a minimal command line, no sound, and no music. The embedded `doom1.wad` is stored in QSPI flash and loaded through the port's custom WAD layer.

## What is included

- `doomgeneric` as the Doom runtime and software renderer.
- A Renesas display backend for the onboard TFT panel.
- A direct SX8654 touch path over `IIC2`.
- Physical fire buttons mapped to `S4` and `S5`.
- A shared `doom_error_msg` diagnostic buffer for debugger inspection.

## Documentation

- [Display path](./README_display.en.md)
- [Touch and input](./README_touch.en.md)
- [Build, flash, and debug](./README_build.en.md)
- [Memory and embedded constraints](./README_memory.en.md)

## Current status

The game boots and renders on the integrated screen, with on-screen touch controls still being tuned for the board's touch panel and orientation. If something stops during startup, inspect `doom_error_msg` in the debugger first; the port records the stage or fault there.

