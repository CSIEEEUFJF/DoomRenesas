# Touch and Input

This port uses the board's SX8654 touch controller directly over `IIC2`, rather than through the higher-level GUIX touch framework.

## Input sources

- Touch panel: SX8654.
- Physical fire buttons: `S4` and `S5`.
- Diagnostic path: touch faults are written into `doom_error_msg`.

## How touch is handled

The touch controller is reset and configured during startup. The port then polls the controller, reads raw coordinates, calibrates them on the fly, and maps the result to Doom keyboard events.

The current runtime mapping is visualized on screen with two translucent circular pads:

- Left pad: movement and strafe.
- Right pad: turning.
- `S4` and `S5`: fire.

The overlay is only a guide; the actual control path is implemented as Doom key events such as arrow keys, strafe, and fire.

## Current limitations

- The panel is effectively single-touch for gameplay purposes.
- Calibration is intentionally tolerant, but the touch geometry still depends on the panel orientation and the SX8654 raw coordinate range.
- If the pad centers or axes feel wrong, the adjustment point is `src/doomgeneric_renesas.c`.

## Troubleshooting

If touch stops responding, check `doom_error_msg` first. The port sets descriptive messages for controller errors, SPI/LCD faults, and CPU faults. A persistent error usually means the controller initialization failed or the touch path was disabled after an I2C error.

