# Display and Video Pipeline

This document describes how the project brings Doom's image to the integrated TFT on the `SK-S7G2`.

## Overview

The video path uses three main blocks:

- `doomgeneric` produces an 8-bit-per-pixel framebuffer;
- the backend in `src/doomgeneric_renesas.c` copies this framebuffer to display memory;
- `GLCDC` reads this buffer in `CLUT8`, while `ILI9341` acts as panel controller.

The result is a lean pipeline suited to the board's RAM limits.

## Hardware involved

- MCU: `Renesas Synergy S7G2`
- MCU display controller: `GLCDC`
- Panel controller: `ILI9341`
- Panel command interface: `SPI` over instance `g_spi_lcdc`
- Integrated panel: `240 x 320`

## Framebuffer format

The project uses a single background framebuffer in `CLUT8`, defined in generated configuration:

- buffer: `g_display_fb_background`
- count: `1`
- allocated resolution: `256 x 320`
- depth: `8 bits per pixel`

Important points:

- allocated width is `256`, not `240`;
- visible panel remains `240 x 320`;
- larger stride simplifies `GLCDC` layout and keeps memory alignment.

In practice, main framebuffer usage is:

- `256 x 320 x 1 byte = 81,920 bytes`

## Main module files

- `src/lcd_setup.c`  
  Low-level `ILI9341` initialization.
- `src/lcd.h`  
  LCD pins and commands.
- `src/doomgeneric_renesas.c`  
  Doom framebuffer conversion, palette upload, and control overlay.
- `src/synergy_gen/main_thread.h`  
  Framebuffer and display instance declarations.
- `src/synergy_gen/main_thread.c`  
  Generated `GLCDC` configuration.

## LCD initialization sequence

`ILI9341` is initialized by `ILI9341V_Init()` in `src/lcd_setup.c`.

Main flow:

1. physical panel reset;
2. `SW_RESET`;
3. programming of power, timing, pixel format, and gamma registers;
4. normal mode command and inversion disable;
5. `SLEEP_OUT`;
6. `DISP_ON`.

Most relevant commands explicitly sent at boot are:

- `ILI9341_NORMAL_MODE_ON`
- `ILI9341_DISP_INV_OFF`
- `ILI9341_SLEEP_OUT`
- `ILI9341_DISP_ON`

This sequence exists because the project already showed cases where palette or apparent panel polarity became incorrect after restarts.

## Display-path initialization

In the Renesas backend:

1. `DG_Init()` calls `renesas_display_init()`;
2. SSP-generated display instance is opened;
3. LCD is initialized;
4. `GLCDC` is started;
5. framebuffer is cleared;
6. `DG_UpdatePalette()` can then upload Doom palette to CLUT.

Callback `g_lcd_spi_callback()` signals end of SPI transfers using `g_main_semaphore_lcdc`.

## How the Doom frame is drawn

Doom internally works at:

- logical width: `320`
- logical height: `200`

Display framebuffer is treated as:

- physical panel: `240 x 320`
- logical orientation in this port: landscape for player

In `DG_DrawFrame()`, each Doom framebuffer pixel is copied into display framebuffer with rotation. Current math is:

- `dst_x = DOOM_BORDER_X + y`
- `dst_y = (LCD_PANEL_HEIGHT - 1) - x`

This means:

- Doom image is rotated to fit panel;
- Doom useful height (`200`) becomes visible gameplay width (`200` columns);
- a side border computed by `DOOM_BORDER_X` centers image inside physical width `240`.

## Palette and CLUT

Doom outputs color indices, not direct RGB pixels. Backend resolves this through `GLCDC` CLUT:

1. `DG_UpdatePalette()` reads current palette from `colors[]`;
2. converts each entry to `ARGB8888`;
3. writes all `256` entries to `g_display_clut_cfg_glcd.p_base` table.

This drastically reduces RAM usage compared with an `RGB565` or `RGB888` framebuffer.

## Control overlay

The video module also draws a simple overlay over framebuffer:

- translucent circular pads;
- visual marking of touch areas;
- no GUIX usage.

This overlay is produced directly in software over Doom's final image inside `src/doomgeneric_renesas.c`.

## Additional robustness measures

The port includes practical mechanisms to ease bring-up and debug:

- colored boot pattern to validate `GLCDC + CLUT` path;
- extra palette refresh in early frames;
- stage messages in `doom_error_msg`;
- explicit lockups on display error.

## Known issues

In recent project history, the most common display symptoms were:

- black screen with incomplete boot;
- stripes during framebuffer updates;
- inverted colors after restart;
- incorrect palette until first relevant visual event.

Not all symptoms are permanent every run, but they are documented because they are typical during embedded hardware bring-up.

## Where to adjust when something breaks

- `src/lcd_setup.c`  
  `ILI9341` command sequence.
- `src/lcd.h`  
  LCD command and pin definitions.
- `src/doomgeneric_renesas.c`  
  Frame rotation, overlay, palette, and `GLCDC` integration.
- `src/synergy_gen/main_thread.h`  
  Framebuffer format and size.
- `configuration.xml`  
  Main source of display configuration in Synergy project.
