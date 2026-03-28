#include <stdint.h>
#include "main_thread.h"
#include "common_data.h"
#include "lcd.h"
#include "doom_port_status.h"

static void lcd_trap(ssp_err_t err)
{
    doom_set_error_msg("LCD SPI error: %d", (int) err);

    while (1)
    {
        tx_thread_sleep(1);
    }
}

static void lcd_write(uint8_t cmd, const uint8_t *data, uint32_t len)
{
    ssp_err_t err;

    err = g_ioport.p_api->pinWrite(LCD_CMD, IOPORT_LEVEL_LOW);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_ioport.p_api->pinWrite(LCD_CS, IOPORT_LEVEL_LOW);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc, TX_WAIT_FOREVER);

    if (len > 0)
    {
        err = g_ioport.p_api->pinWrite(LCD_CMD, IOPORT_LEVEL_HIGH);
        if (SSP_SUCCESS != err)
        {
            lcd_trap(err);
        }

        err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, data, len, SPI_BIT_WIDTH_8_BITS);
        if (SSP_SUCCESS != err)
        {
            lcd_trap(err);
        }

        tx_semaphore_get(&g_main_semaphore_lcdc, TX_WAIT_FOREVER);
    }

    err = g_ioport.p_api->pinWrite(LCD_CS, IOPORT_LEVEL_HIGH);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }
}

static void lcd_read(uint8_t cmd, uint8_t *data, uint32_t len)
{
    static uint8_t dummy_write[20];
    ssp_err_t err;

    err = g_ioport.p_api->pinWrite(LCD_CMD, IOPORT_LEVEL_LOW);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_ioport.p_api->pinWrite(LCD_CS, IOPORT_LEVEL_LOW);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc, TX_WAIT_FOREVER);

    err = g_ioport.p_api->pinWrite(LCD_CMD, IOPORT_LEVEL_HIGH);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_ioport.p_api->pinCfg(IOPORT_PORT_01_PIN_02,
                                 (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT
                                 | (uint32_t) ((g_spi_lcdc.p_cfg->clk_polarity == SPI_CLK_POLARITY_HIGH)
                                         ? IOPORT_LEVEL_LOW : IOPORT_LEVEL_HIGH));
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MICROSECONDS);

    err = g_ioport.p_api->pinCfg(IOPORT_PORT_01_PIN_02,
                                 (uint32_t) IOPORT_CFG_PERIPHERAL_PIN
                                 | (uint32_t) IOPORT_PERIPHERAL_SCI0_2_4_6_8);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    err = g_spi_lcdc.p_api->writeRead(g_spi_lcdc.p_ctrl, dummy_write, data, len, SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc, TX_WAIT_FOREVER);

    err = g_ioport.p_api->pinWrite(LCD_CS, IOPORT_LEVEL_HIGH);
    if (SSP_SUCCESS != err)
    {
        lcd_trap(err);
    }
}

void ILI9341V_Init(void)
{
    uint8_t data[8];
    int i;
    static const uint8_t powerb[] = {0x00, 0xC1, 0x30};
    static const uint8_t dtca[] = {0x85, 0x00, 0x78};
    static const uint8_t dtcb[] = {0x00, 0x00};
    static const uint8_t powera[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    static const uint8_t power_seq[] = {0x64, 0x03, 0x12, 0x81};
    static const uint8_t prc[] = {0x20};
    static const uint8_t power1[] = {0x23};
    static const uint8_t power2[] = {0x10};
    static const uint8_t vcom1[] = {0x3E, 0x28};
    static const uint8_t vcom2[] = {0x86};
    static const uint8_t mac[] = {0x48};
    static const uint8_t pixel_format[] = {0x55};
    static const uint8_t frame_ctrl1[] = {0x00, 0x18};
    static const uint8_t dfc[] = {0x08, 0x82, 0x27};
    static const uint8_t gamma3[] = {0x00};
    static const uint8_t rgb_interface[] = {0xC2};
    static const uint8_t interface_mode[] = {0x01, 0x00, 0x06};
    static const uint8_t column_addr[] = {0x00, 0x00, 0x00, 0xEF};
    static const uint8_t page_addr[] = {0x00, 0x00, 0x01, 0x3F};
    static const uint8_t gamma[] = {0x01};
    static const uint8_t pgamma[] =
    {
        0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07,
        0x10, 0x03, 0x0E, 0x09, 0x00
    };
    static const uint8_t ngamma[] =
    {
        0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08,
        0x0F, 0x0C, 0x31, 0x36, 0x0F
    };

    g_ioport.p_api->pinWrite(LCD_CS, IOPORT_LEVEL_HIGH);
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_HIGH);
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_LOW);
    tx_thread_sleep(1);
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_HIGH);

    tx_thread_sleep(12);

    lcd_write(ILI9341_SW_RESET, NULL, 0);
    tx_thread_sleep(5);

    for (i = 0; i < 4; ++i)
    {
        data[0] = (uint8_t) (0x10 + i);
        lcd_write(0xD9, data, 1);
        lcd_read(0xD3, data, 1);
    }

    lcd_write(ILI9341_POWERB, powerb, sizeof(powerb));
    lcd_write(ILI9341_DTCA, dtca, sizeof(dtca));
    lcd_write(ILI9341_DTCB, dtcb, sizeof(dtcb));
    lcd_write(ILI9341_POWERA, powera, sizeof(powera));
    lcd_write(ILI9341_POWER_SEQ, power_seq, sizeof(power_seq));
    lcd_write(ILI9341_PRC, prc, sizeof(prc));
    lcd_write(ILI9341_POWER1, power1, sizeof(power1));
    lcd_write(ILI9341_POWER2, power2, sizeof(power2));
    lcd_write(ILI9341_VCOM1, vcom1, sizeof(vcom1));
    lcd_write(ILI9341_VCOM2, vcom2, sizeof(vcom2));
    lcd_write(ILI9341_MAC, mac, sizeof(mac));
    lcd_write(ILI9341_PIXEL_FORMAT, pixel_format, sizeof(pixel_format));
    lcd_write(ILI9341_FRM_CTRL1, frame_ctrl1, sizeof(frame_ctrl1));
    lcd_write(ILI9341_DFC, dfc, sizeof(dfc));
    lcd_write(ILI9341_3GAMMA_EN, gamma3, sizeof(gamma3));
    lcd_write(ILI9341_RGB_INTERFACE, rgb_interface, sizeof(rgb_interface));
    lcd_write(ILI9341_INTERFACE, interface_mode, sizeof(interface_mode));
    lcd_write(ILI9341_COLUMN_ADDR, column_addr, sizeof(column_addr));
    lcd_write(ILI9341_PAGE_ADDR, page_addr, sizeof(page_addr));
    lcd_write(ILI9341_GAMMA, gamma, sizeof(gamma));
    lcd_write(ILI9341_PGAMMA, pgamma, sizeof(pgamma));
    lcd_write(ILI9341_NGAMMA, ngamma, sizeof(ngamma));
    lcd_write(ILI9341_NORMAL_MODE_ON, NULL, 0);
    lcd_write(ILI9341_DISP_INV_OFF, NULL, 0);
    lcd_write(ILI9341_SLEEP_OUT, NULL, 0);
    tx_thread_sleep(2);
    lcd_write(ILI9341_DISP_ON, NULL, 0);
    lcd_read(ILI9341_READ_DISP_PIXEL, data, 1);
}
