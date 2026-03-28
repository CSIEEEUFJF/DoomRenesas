# Memory and Embedded Constraints

This project is built for the S7G2 Starter Kit's embedded memory limits. The board has 640 KiB of RAM and no external SDRAM, so the port is tuned to avoid large runtime allocations.

## Main constraints

- No SDRAM is available.
- The display must fit into internal RAM.
- The WAD must not consume startup RAM.
- Doom's zone allocator must stay small enough to boot and render a frame.

## What the port does

- Stores `doom1.wad` in QSPI flash with the `BSP_PLACE_IN_SECTION_V2(".qspi_flash")` attribute.
- Uses a single CLUT8 framebuffer in internal RAM.
- Avoids desktop-style file system assumptions.
- Boots with a reduced command line: `-skill 1`, `-warp 1 1`, `-nomonsters`, `-nosound`, and `-nomusic`.
- Keeps `doom_error_msg` in a `.noinit` section so the debugger can inspect the last failure reason.

## Why this matters

Most Doom ports assume more RAM, a larger heap, or a richer host environment. On this board, the expensive parts are:

- zone allocations during map and texture setup,
- renderer lookup tables,
- display buffers,
- and any code that accidentally copies the WAD into RAM.

The current port avoids those traps as much as possible, but it is still a tight fit. If you add new graphics, sound, or caching features, check the memory budget first.

## Debugging memory issues

If the game stops during startup, inspect `doom_error_msg` for allocation failures. The port records the allocation size, free space, zone size, and in many cases the source file and line that triggered the failure. That makes it easier to see whether the pressure came from the WAD loader, the renderer, or the video path.

