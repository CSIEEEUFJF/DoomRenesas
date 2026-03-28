#include <stdint.h>
#include <string.h>
#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomkeys.h"
#include "doomgeneric/i_video.h"
#include "tx_api.h"
#include "common_data.h"
#include "main_thread.h"
#include "lcd.h"
#include "doom_port_status.h"

#define LCD_PANEL_WIDTH           240
#define LCD_PANEL_HEIGHT          320
#define LCD_FB_STRIDE             256
#define DOOM_BORDER_X             ((LCD_PANEL_WIDTH - DOOMGENERIC_RESY) / 2)
#define USER_SCREEN_WIDTH         DOOMGENERIC_RESX
#define USER_SCREEN_HEIGHT        LCD_PANEL_WIDTH

#define TOUCH_S5_PIN              IOPORT_PORT_00_PIN_05
#define TOUCH_S4_PIN              IOPORT_PORT_00_PIN_06
#define TOUCH_RESET_PIN           IOPORT_PORT_06_PIN_09

#define TOUCH_REG_TOUCH0          0x00U
#define TOUCH_REG_TOUCH1          0x01U
#define TOUCH_REG_TOUCH2          0x02U
#define TOUCH_REG_CHANMSK         0x04U
#define TOUCH_REG_PROX0           0x0BU
#define TOUCH_REG_IRQMSK          0x22U
#define TOUCH_REG_IRQSRC          0x23U
#define TOUCH_CMD_PENTRG          0xE0U
#define TOUCH_CMD_READ_REG        0x40U
#define TOUCH_MAX_ADC_VALUE       4095U

#define TOUCH_REG_TOUCH0_CFG      ((0x7U << 4) | (0x4U << 0))
#define TOUCH_REG_TOUCH1_CFG      ((0x1U << 5) | (0x1U << 2) | (0x3U << 0))
#define TOUCH_REG_TOUCH2_CFG      (0x4U << 0)
#define TOUCH_CHANMSK_XY          ((0x1U << 7) | (0x1U << 6))
#define TOUCH_IRQ_PENRELEASE      (0x1U << 2)
#define TOUCH_IRQ_PENDOWN         (0x1U << 3)

#define INPUT_MASK_LEFT           (1U << 0)
#define INPUT_MASK_RIGHT          (1U << 1)
#define INPUT_MASK_UP             (1U << 2)
#define INPUT_MASK_DOWN           (1U << 3)
#define INPUT_MASK_FIRE           (1U << 4)
#define INPUT_MASK_STRAFE_LEFT    (1U << 5)
#define INPUT_MASK_STRAFE_RIGHT   (1U << 6)

#define TOUCH_PAD_LEFT_X          (80)
#define TOUCH_PAD_RIGHT_X         (USER_SCREEN_WIDTH - 80)
#define TOUCH_PAD_CENTER_Y        (USER_SCREEN_HEIGHT - 64)
#define TOUCH_PAD_RADIUS          (56)
#define TOUCH_PAD_DEADZONE        (6)
#define TOUCH_CAL_MIN_DEFAULT     512U
#define TOUCH_CAL_MAX_DEFAULT     3072U
#define TOUCH_CAL_MIN_SPAN        256U

#define KEY_QUEUE_CAPACITY        16U

typedef struct st_touch_state
{
    boolean available;
    boolean active;
    int16_t x;
    int16_t y;
} touch_state_t;

typedef struct st_input_map
{
    uint8_t mask;
    unsigned char key;
} input_map_t;

typedef struct st_touch_calibration
{
    uint16_t min_x;
    uint16_t max_x;
    uint16_t min_y;
    uint16_t max_y;
} touch_calibration_t;

static uint8_t *framebuffer(void)
{
    return (uint8_t *) &g_display_fb_background[0][0];
}

static touch_state_t g_touch_state;
static uint8_t g_input_mask;
static uint16_t g_key_queue[KEY_QUEUE_CAPACITY];
static uint8_t g_key_queue_head;
static uint8_t g_key_queue_tail;
static boolean g_touch_fault;
static touch_calibration_t g_touch_calibration;
static uint8_t g_palette_boot_refresh;

static const input_map_t g_input_map[] =
{
    { INPUT_MASK_LEFT,         KEY_LEFTARROW  },
    { INPUT_MASK_RIGHT,        KEY_RIGHTARROW },
    { INPUT_MASK_UP,           KEY_UPARROW    },
    { INPUT_MASK_DOWN,         KEY_DOWNARROW  },
    { INPUT_MASK_FIRE,         KEY_FIRE       },
    { INPUT_MASK_STRAFE_LEFT,  KEY_STRAFE_L   },
    { INPUT_MASK_STRAFE_RIGHT, KEY_STRAFE_R   }
};

static void show_boot_pattern(void)
{
    uint8_t *dst;
    int x;
    int y;

    dst = framebuffer();

    for (y = 0; y < LCD_PANEL_HEIGHT; ++y)
    {
        for (x = 0; x < LCD_FB_STRIDE; ++x)
        {
            uint8_t color_index;

            if (x >= LCD_PANEL_WIDTH)
            {
                color_index = 0U;
            }
            else if (y < (LCD_PANEL_HEIGHT / 4))
            {
                color_index = (uint8_t) (32 + (x >> 1));
            }
            else if (y < (LCD_PANEL_HEIGHT / 2))
            {
                color_index = (uint8_t) (96 + (x >> 1));
            }
            else if (y < ((LCD_PANEL_HEIGHT * 3) / 4))
            {
                color_index = (uint8_t) (160 + (x >> 1));
            }
            else
            {
                color_index = (uint8_t) (224 + (x >> 2));
            }

            dst[(y * LCD_FB_STRIDE) + x] = color_index;
        }
    }
}

static void renesas_trap(ssp_err_t err)
{
    doom_set_error_msg("Renesas display error: %d", (int) err);

    while (1)
    {
        tx_thread_sleep(1);
    }
}

static void touch_disable(ssp_err_t err, const char *context)
{
    g_touch_state.available = false;
    g_touch_state.active = false;
    g_touch_fault = true;
    doom_set_error_msg("Touch %s error: %d", context, (int) err);
}

void g_lcd_spi_callback(spi_callback_args_t *p_args)
{
    if ((p_args != NULL) && (p_args->event == SPI_EVENT_TRANSFER_COMPLETE))
    {
        tx_semaphore_put(&g_main_semaphore_lcdc);
    }
}

static uint32_t clut_color(struct color color)
{
    return (0xFFUL << 24)
            | ((uint32_t) color.r << 16)
            | ((uint32_t) color.g << 8)
            | (uint32_t) color.b;
}

static boolean pin_is_low(ioport_port_pin_t pin)
{
    ioport_level_t level = IOPORT_LEVEL_HIGH;
    ssp_err_t err;

    err = g_ioport.p_api->pinRead(pin, &level);
    if (SSP_SUCCESS != err)
    {
        return false;
    }

    return (level == IOPORT_LEVEL_LOW);
}

static ssp_err_t touch_write(uint8_t *src, uint32_t bytes, boolean restart)
{
    return g_i2c.p_api->write(g_i2c.p_ctrl, src, bytes, restart);
}

static ssp_err_t touch_read(uint8_t *dest, uint32_t bytes, boolean restart)
{
    return g_i2c.p_api->read(g_i2c.p_ctrl, dest, bytes, restart);
}

static ssp_err_t touch_write_then_read(uint8_t *data, uint32_t bytes)
{
    ssp_err_t err;

    err = touch_write(data, bytes, true);
    if (SSP_SUCCESS != err)
    {
        return err;
    }

    return touch_read(data, bytes, false);
}

static void touch_queue_event(boolean pressed, unsigned char key)
{
    uint8_t next;

    next = (uint8_t) ((g_key_queue_head + 1U) % KEY_QUEUE_CAPACITY);
    if (next == g_key_queue_tail)
    {
        return;
    }

    g_key_queue[g_key_queue_head] = (uint16_t) (((pressed ? 1U : 0U) << 8) | key);
    g_key_queue_head = next;
}

static void touch_apply_mask(uint8_t previous_mask, uint8_t next_mask)
{
    size_t i;

    for (i = 0; i < (sizeof(g_input_map) / sizeof(g_input_map[0])); ++i)
    {
        uint8_t mask = g_input_map[i].mask;

        if (((previous_mask & mask) != 0U) && ((next_mask & mask) == 0U))
        {
            touch_queue_event(false, g_input_map[i].key);
        }
    }

    for (i = 0; i < (sizeof(g_input_map) / sizeof(g_input_map[0])); ++i)
    {
        uint8_t mask = g_input_map[i].mask;

        if (((previous_mask & mask) == 0U) && ((next_mask & mask) != 0U))
        {
            touch_queue_event(true, g_input_map[i].key);
        }
    }
}

static void touch_reset_calibration(void)
{
    g_touch_calibration.min_x = TOUCH_CAL_MIN_DEFAULT;
    g_touch_calibration.max_x = TOUCH_CAL_MAX_DEFAULT;
    g_touch_calibration.min_y = TOUCH_CAL_MIN_DEFAULT;
    g_touch_calibration.max_y = TOUCH_CAL_MAX_DEFAULT;
}

static void touch_expand_calibration(uint32_t raw_x, uint32_t raw_y)
{
    if (raw_x < g_touch_calibration.min_x)
    {
        g_touch_calibration.min_x = (uint16_t) raw_x;
    }

    if (raw_x > g_touch_calibration.max_x)
    {
        g_touch_calibration.max_x = (uint16_t) raw_x;
    }

    if (raw_y < g_touch_calibration.min_y)
    {
        g_touch_calibration.min_y = (uint16_t) raw_y;
    }

    if (raw_y > g_touch_calibration.max_y)
    {
        g_touch_calibration.max_y = (uint16_t) raw_y;
    }
}

static int16_t touch_scale_axis(uint32_t raw_value, uint16_t min_raw, uint16_t max_raw, int16_t pixel_count)
{
    int32_t scaled;
    uint32_t clamped_value;
    uint32_t span;

    if (max_raw <= (uint16_t) (min_raw + TOUCH_CAL_MIN_SPAN))
    {
        min_raw = 0U;
        max_raw = (uint16_t) TOUCH_MAX_ADC_VALUE;
    }

    clamped_value = raw_value;
    if (clamped_value < min_raw)
    {
        clamped_value = min_raw;
    }
    else if (clamped_value > max_raw)
    {
        clamped_value = max_raw;
    }

    span = (uint32_t) (max_raw - min_raw);
    if (span == 0U)
    {
        return 0;
    }

    scaled = (int32_t) ((((int32_t) (pixel_count - 1)) * (int32_t) (clamped_value - min_raw)) / (int32_t) span);
    if (scaled < 0)
    {
        scaled = 0;
    }
    else if (scaled >= pixel_count)
    {
        scaled = (int32_t) pixel_count - 1;
    }

    return (int16_t) scaled;
}

static uint8_t touch_pad_mask(void)
{
    int dx;
    int dy;
    int distance_squared;
    uint8_t mask = 0U;

    if (!g_touch_state.active)
    {
        return 0U;
    }

    dx = (int) g_touch_state.x - TOUCH_PAD_LEFT_X;
    dy = (int) g_touch_state.y - TOUCH_PAD_CENTER_Y;
    distance_squared = (dx * dx) + (dy * dy);
    if (distance_squared <= (TOUCH_PAD_RADIUS * TOUCH_PAD_RADIUS))
    {
        if (dy <= -TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_UP;
        }
        else if (dy >= TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_DOWN;
        }

        if (dx <= -TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_STRAFE_LEFT;
        }
        else if (dx >= TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_STRAFE_RIGHT;
        }

        return mask;
    }

    dx = (int) g_touch_state.x - TOUCH_PAD_RIGHT_X;
    dy = (int) g_touch_state.y - TOUCH_PAD_CENTER_Y;
    distance_squared = (dx * dx) + (dy * dy);
    if (distance_squared <= (TOUCH_PAD_RADIUS * TOUCH_PAD_RADIUS))
    {
        if (dx <= -TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_LEFT;
        }
        else if (dx >= TOUCH_PAD_DEADZONE)
        {
            mask |= INPUT_MASK_RIGHT;
        }
    }

    return mask;
}

static uint8_t touch_overlay_color(uint8_t background_index)
{
    uint32_t luminance;
    struct color color;

    color = colors[background_index];
    luminance = ((30U * color.r) + (59U * color.g) + (11U * color.b)) / 100U;

    return (uint8_t) ((luminance > 127U) ? 0U : 255U);
}

static void touch_overlay_pixel(uint8_t *dst, int x, int y, boolean stipple)
{
    uint8_t *pixel;

    if ((x < 0) || (x >= LCD_PANEL_WIDTH) || (y < 0) || (y >= LCD_PANEL_HEIGHT))
    {
        return;
    }

    if (stipple && (((x ^ y) & 0x01) != 0))
    {
        return;
    }

    pixel = &dst[(y * LCD_FB_STRIDE) + x];
    *pixel = touch_overlay_color(*pixel);
}

static void touch_overlay_user_pixel(uint8_t *dst, int user_x, int user_y, boolean stipple)
{
    int fb_x;
    int fb_y;

    if ((user_x < 0) || (user_x >= USER_SCREEN_WIDTH) || (user_y < 0) || (user_y >= USER_SCREEN_HEIGHT))
    {
        return;
    }

    fb_x = user_y;
    fb_y = (USER_SCREEN_WIDTH - 1) - user_x;
    touch_overlay_pixel(dst, fb_x, fb_y, stipple);
}

static void touch_overlay_circle(uint8_t *dst, int center_x, int center_y, int radius, boolean filled)
{
    int x;
    int y;
    int radius_squared;
    int inner_radius;
    int inner_squared;

    radius_squared = radius * radius;
    inner_radius = radius - 2;
    inner_squared = inner_radius * inner_radius;

    for (y = -radius; y <= radius; ++y)
    {
        for (x = -radius; x <= radius; ++x)
        {
            int distance_squared = (x * x) + (y * y);

            if (distance_squared > radius_squared)
            {
                continue;
            }

            if (filled)
            {
                touch_overlay_user_pixel(dst, center_x + x, center_y + y, true);
            }
            else if (distance_squared >= inner_squared)
            {
                touch_overlay_user_pixel(dst, center_x + x, center_y + y, false);
            }
        }
    }
}

static void touch_overlay_crosshair(uint8_t *dst, int center_x, int center_y)
{
    int offset;

    for (offset = -8; offset <= 8; ++offset)
    {
        touch_overlay_user_pixel(dst, center_x + offset, center_y, false);
        touch_overlay_user_pixel(dst, center_x, center_y + offset, false);
    }
}

static void touch_overlay_draw(uint8_t *dst)
{
    touch_overlay_circle(dst, TOUCH_PAD_LEFT_X, TOUCH_PAD_CENTER_Y, TOUCH_PAD_RADIUS, false);
    touch_overlay_circle(dst, TOUCH_PAD_LEFT_X, TOUCH_PAD_CENTER_Y, TOUCH_PAD_RADIUS - 6, true);
    touch_overlay_crosshair(dst, TOUCH_PAD_LEFT_X, TOUCH_PAD_CENTER_Y);

    touch_overlay_circle(dst, TOUCH_PAD_RIGHT_X, TOUCH_PAD_CENTER_Y, TOUCH_PAD_RADIUS, false);
    touch_overlay_circle(dst, TOUCH_PAD_RIGHT_X, TOUCH_PAD_CENTER_Y, TOUCH_PAD_RADIUS - 6, true);
    touch_overlay_crosshair(dst, TOUCH_PAD_RIGHT_X, TOUCH_PAD_CENTER_Y);
}

static void touch_poll_controller(void)
{
    ssp_err_t err;
    uint8_t irqsrc;
    uint8_t raw_touch[4];
    uint32_t raw_x;
    uint32_t raw_y;

    if (!g_touch_state.available)
    {
        return;
    }

    irqsrc = (uint8_t) (TOUCH_CMD_READ_REG | TOUCH_REG_IRQSRC);
    err = touch_write_then_read(&irqsrc, sizeof(irqsrc));
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "read");
        return;
    }

    err = touch_read(raw_touch, sizeof(raw_touch), false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "sample");
        return;
    }

    if ((irqsrc & TOUCH_IRQ_PENRELEASE) != 0U)
    {
        g_touch_state.active = false;
        return;
    }

    if (((irqsrc & TOUCH_IRQ_PENDOWN) == 0U) && !g_touch_state.active)
    {
        return;
    }

    raw_x = (((uint32_t) raw_touch[0] & 0x0FU) << 8) | raw_touch[1];
    raw_y = (((uint32_t) raw_touch[2] & 0x0FU) << 8) | raw_touch[3];

    touch_expand_calibration(raw_x, raw_y);

    g_touch_state.x = touch_scale_axis(raw_y,
                                       g_touch_calibration.min_y,
                                       g_touch_calibration.max_y,
                                       USER_SCREEN_WIDTH);
    g_touch_state.y = touch_scale_axis(raw_x,
                                       g_touch_calibration.min_x,
                                       g_touch_calibration.max_x,
                                       USER_SCREEN_HEIGHT);

    g_touch_state.active = true;
}

static void renesas_touch_init(void)
{
    ssp_err_t err;
    uint8_t command[4];

    memset(&g_touch_state, 0, sizeof(g_touch_state));
    g_touch_fault = false;
    touch_reset_calibration();

    err = g_i2c.p_api->open(g_i2c.p_ctrl, g_i2c.p_cfg);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "open");
        return;
    }

    g_ioport.p_api->pinWrite(TOUCH_RESET_PIN, IOPORT_LEVEL_LOW);
    tx_thread_sleep(2);
    g_ioport.p_api->pinWrite(TOUCH_RESET_PIN, IOPORT_LEVEL_HIGH);
    tx_thread_sleep(2);

    command[0] = TOUCH_REG_TOUCH0;
    command[1] = TOUCH_REG_TOUCH0_CFG;
    command[2] = TOUCH_REG_TOUCH1_CFG;
    command[3] = TOUCH_REG_TOUCH2_CFG;
    err = touch_write(command, 4, false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "cfg0");
        return;
    }

    command[0] = TOUCH_REG_CHANMSK;
    command[1] = TOUCH_CHANMSK_XY;
    err = touch_write(command, 2, false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "chan");
        return;
    }

    command[0] = TOUCH_REG_IRQMSK;
    command[1] = (uint8_t) (TOUCH_IRQ_PENDOWN | TOUCH_IRQ_PENRELEASE);
    err = touch_write(command, 2, false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "irq");
        return;
    }

    command[0] = TOUCH_REG_PROX0;
    command[1] = 0U;
    err = touch_write(command, 2, false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "prox");
        return;
    }

    command[0] = TOUCH_CMD_PENTRG;
    err = touch_write(command, 1, false);
    if (SSP_SUCCESS != err)
    {
        touch_disable(err, "mode");
        return;
    }

    g_touch_state.available = true;
}

static void renesas_input_poll(void)
{
    uint8_t next_mask;

    touch_poll_controller();

    next_mask = touch_pad_mask();

    if (pin_is_low(TOUCH_S4_PIN) || pin_is_low(TOUCH_S5_PIN))
    {
        next_mask |= INPUT_MASK_FIRE;
    }

    if (next_mask != g_input_mask)
    {
        touch_apply_mask(g_input_mask, next_mask);
        g_input_mask = next_mask;
    }
}

void DG_UpdatePalette(void)
{
    static int boot_pattern_drawn = 0;
    ssp_err_t err;
    uint32_t *clut;
    int i;

    clut = g_display_clut_cfg_glcd.p_base;

    for (i = 0; i < 256; ++i)
    {
        clut[i] = clut_color(colors[i]);
    }

    err = g_display.p_api->clut(g_display.p_ctrl, &g_display_clut_cfg_glcd, DISPLAY_FRAME_LAYER_1);
    if (SSP_SUCCESS != err)
    {
        renesas_trap(err);
    }

    if (boot_pattern_drawn == 0)
    {
        show_boot_pattern();
        boot_pattern_drawn = 1;
    }
}

static void renesas_display_init(void)
{
    ssp_err_t err;

    doom_set_error_literal("stage: spi open");
    err = g_spi_lcdc.p_api->open(g_spi_lcdc.p_ctrl, g_spi_lcdc.p_cfg);
    if (SSP_SUCCESS != err)
    {
        renesas_trap(err);
    }

    doom_set_error_literal("stage: panel init");
    ILI9341V_Init();

    doom_set_error_literal("stage: glcd open");
    err = g_display.p_api->open(g_display.p_ctrl, g_display.p_cfg);
    if (SSP_SUCCESS != err)
    {
        renesas_trap(err);
    }

    doom_set_error_literal("stage: glcd start");
    err = g_display.p_api->start(g_display.p_ctrl);
    if (SSP_SUCCESS != err)
    {
        renesas_trap(err);
    }

    memset(framebuffer(), 0, LCD_FB_STRIDE * LCD_PANEL_HEIGHT);
    doom_set_error_literal("stage: display ready");
}

void DG_Init(void)
{
    renesas_display_init();
    renesas_touch_init();
    g_palette_boot_refresh = 8U;
}

void DG_DrawFrame(void)
{
    static int framebuffer_prepared = 0;
    const uint8_t *src;
    uint8_t *dst;
    int x;
    int y;

    src = (const uint8_t *) I_VideoBuffer;
    dst = framebuffer();

    if (framebuffer_prepared == 0)
    {
        memset(dst, 0, LCD_FB_STRIDE * LCD_PANEL_HEIGHT);
        framebuffer_prepared = 1;
    }

    for (y = 0; y < DOOMGENERIC_RESY; ++y)
    {
        for (x = 0; x < DOOMGENERIC_RESX; ++x)
        {
            int dst_x = DOOM_BORDER_X + y;
            int dst_y = (LCD_PANEL_HEIGHT - 1) - x;

            dst[(dst_y * LCD_FB_STRIDE) + dst_x] = src[(y * DOOMGENERIC_RESX) + x];
        }
    }

    touch_overlay_draw(dst);

    if (g_palette_boot_refresh > 0U)
    {
        DG_UpdatePalette();
        --g_palette_boot_refresh;
    }

    if (!g_touch_fault)
    {
        doom_clear_error_msg();
    }
}

void DG_SleepMs(uint32_t ms)
{
    ULONG ticks;

    ticks = (ULONG) (((uint64_t) ms * TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL);
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    tx_thread_sleep(ticks);
}

uint32_t DG_GetTicksMs(void)
{
    ULONG ticks;

    ticks = tx_time_get();
    return (uint32_t) ((((uint64_t) ticks) * 1000ULL) / TX_TIMER_TICKS_PER_SECOND);
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    uint16_t event;

    renesas_input_poll();

    if (g_key_queue_head == g_key_queue_tail)
    {
        return 0;
    }

    event = g_key_queue[g_key_queue_tail];
    g_key_queue_tail = (uint8_t) ((g_key_queue_tail + 1U) % KEY_QUEUE_CAPACITY);

    *pressed = (int) ((event >> 8) & 0x01U);
    *key = (unsigned char) (event & 0xFFU);
    return 1;
}

void DG_SetWindowTitle(const char *title)
{
    SSP_PARAMETER_NOT_USED(title);
}
