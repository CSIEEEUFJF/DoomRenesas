# Display Path

This project drives the S7G2 Starter Kit's integrated TFT through the Renesas GLCDC and an ILI9341 panel controller over SPI.

## Hardware and format

- Panel size: 240x320.
- Runtime framebuffer: a single CLUT8 background framebuffer.
- Framebuffer storage: `g_display_fb_background[1][256 * 320]`, aligned in `.bss`.
- Display color path: Doom renders 8-bit indices, and the GLCDC CLUT converts those indices into RGB output.

The framebuffer stride is 256 bytes, not 240. That extra alignment simplifies the display buffer layout and matches the generated GLCDC configuration.

## Software path

1. `main_thread_entry()` brings up the board and starts the Doom thread.
2. `ILI9341V_Init()` resets and configures the panel over SPI.
3. `DG_UpdatePalette()` uploads the 256-color Doom palette into the GLCDC CLUT.
4. `DG_DrawFrame()` writes Doom's 8-bit frame directly into the display framebuffer.
5. A small overlay is drawn on top of the frame to visualize the touch controls.

The SPI LCD path uses the generated `g_spi_lcdc` instance and the `g_lcd_spi_callback()` completion callback. Transfers are synchronized through `g_main_semaphore_lcdc`.

## Initialization details

The LCD bring-up explicitly sends:

- `ILI9341_NORMAL_MODE_ON`
- `ILI9341_DISP_INV_OFF`
- `ILI9341_SLEEP_OUT`
- `ILI9341_DISP_ON`

That sequence is intended to keep the panel in normal polarity after reset and reboot.

## Practical notes

- The project does not use external SDRAM.
- The display runs in a single-buffer CLUT8 mode to reduce RAM pressure.
- If the image appears blank or inverted, the first places to inspect are `src/lcd_setup.c`, `src/lcd.h`, and `src/doomgeneric_renesas.c`.

