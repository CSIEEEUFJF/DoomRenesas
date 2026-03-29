# Memory and Embedded Constraints

This project was shaped by the real constraints of the `SK-S7G2`. This module documentation explains why several seemingly unusual decisions are actually required for the port to fit and boot.

## Constraint summary

- available internal RAM: approximately `640 KiB`
- external SDRAM: **not available**
- asset storage: board QSPI flash
- operating system: `ThreadX`
- display: requires framebuffer resident in internal RAM

In other words: memory budget is small and shared among RTOS, stacks, BSP heap, Doom zone, framebuffer, renderer structures, and globals.

## General project strategy

To fit on this board, the port adopts these decisions:

- use a single `CLUT8` framebuffer;
- avoid 16-bit or 32-bit framebuffers;
- keep `doom1.wad` out of boot RAM in `.qspi_flash`;
- boot game with minimal command line;
- keep sound and music disabled;
- reduce desktop-environment dependencies;
- use `doom_error_msg` as a low-cost observability tool.

## Important project numbers

Some currently relevant values in the repository:

- BSP heap: `0x52000`
- BSP main stack: `0x800`
- default Doom zone on Renesas target: `240 KiB`
- minimum fallback zone: `176 KiB`
- main display framebuffer: `256 x 320 x 1 byte = 81,920 bytes`

These numbers are not independent. If one grows too much, another subsystem may fail to initialize.

## BSP heap and Doom zone

Doom uses its own zone allocator, but this zone still must come from memory allocated by the system.

In current port:

- BSP reserves a larger heap to make initial allocation viable;
- `I_ZoneBase()` attempts to allocate zone with Renesas profile in KiB;
- default value is `240 KiB`;
- if allocation fails, code tries reducing size down to minimum acceptable value.

This logic is in `src/doomgeneric/i_system.c`.

## Embedded WAD in QSPI

`doom1.wad` converted to C array is in:

- `src/doom1_wad.c`

It is placed in section:

- `.qspi_flash`

This is crucial because it prevents full WAD contents from being copied to RAM at boot. File access is adapted in `src/doomgeneric/w_file_stdc.c`, which recognizes `doom1.wad` and redirects open calls to embedded implementation.

## Why display needs extra care

On a low-RAM board, a “normal” framebuffer is expensive. So the project uses:

- a single buffer;
- `CLUT8`;
- Doom palette loaded into `GLCDC` CLUT.

This choice lowers video-layer cost and leaves more room for Doom zone and renderer.

## Where memory pressure usually appears

Historically most sensitive points were:

- initial Doom zone allocation;
- renderer tables and caches;
- structures related to textures and lumps;
- video buffers and transition effects;
- unnecessary WAD copies into RAM.

That is why many decisions in this port prioritize footprint over architectural luxury.

## Role of `doom_error_msg`

`doom_error_msg` exists to make memory failures less opaque.

When allocation fails, the project may record:

- requested size;
- observed free memory;
- zone size;
- in many cases, failure file and line.

This greatly reduces diagnosis time versus a silent crash.

## Classic signs of memory issues

If things break again after changes, watch for messages such as:

- `Unable to allocate ... for zone`
- `Couldn't realloc lumpinfo`
- `Z_Malloc failed on allocation of ...`

These usually point directly to the subsystem that became too expensive.

## Practical rules for future changes

Before adding a new feature, go through this list:

1. Does it require an extra framebuffer?
2. Does it increase resident table count?
3. Does it copy large data to RAM?
4. Does it increase Doom zone?
5. Does it depend on peripherals or libraries that bring extra buffers?

If answer is “yes” to any item, check `.map` impact before considering the change stable.

## Where to review when budget goes out of control

- `synergy_cfg/ssp_cfg/bsp/bsp_cfg.h`  
  BSP heap and stack.
- `src/doomgeneric/i_system.c`  
  Zone allocation policy.
- `src/doom1_wad.c`  
  Embedded asset.
- `src/doomgeneric/w_file_stdc.c`  
  Embedded WAD access.
- `src/synergy_gen/main_thread.h`  
  Framebuffer definition.
- `Debug/DoomRenesas.map`  
  Final evidence of memory usage after link.
