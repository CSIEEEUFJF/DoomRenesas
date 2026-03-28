#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "bsp_api.h"
#include "doom_port_status.h"

char doom_error_msg[256] BSP_PLACE_IN_SECTION_V2(".noinit") BSP_ALIGN_VARIABLE_V2(4);

static void doom_fault_trap(const char *msg)
{
    doom_set_error_literal(msg);

    while (1)
    {
    }
}

void doom_set_error_literal(const char *msg)
{
    size_t i;

    memset(doom_error_msg, 0, sizeof(doom_error_msg));

    if (msg == NULL)
    {
        return;
    }

    for (i = 0; (i + 1U) < sizeof(doom_error_msg) && msg[i] != '\0'; ++i)
    {
        doom_error_msg[i] = msg[i];
    }

    doom_error_msg[i] = '\0';
}

void doom_clear_error_msg(void)
{
    memset(doom_error_msg, 0, sizeof(doom_error_msg));
}

void doom_set_error_msg(const char *fmt, ...)
{
    va_list args;

    doom_set_error_literal("");
    va_start(args, fmt);
    vsnprintf(doom_error_msg, sizeof(doom_error_msg), fmt, args);
    va_end(args);
    doom_error_msg[sizeof(doom_error_msg) - 1] = '\0';
}

void HardFault_Handler(void)
{
    doom_fault_trap("HardFault");
}

void MemManage_Handler(void)
{
    doom_fault_trap("MemManage");
}

void BusFault_Handler(void)
{
    doom_fault_trap("BusFault");
}

void UsageFault_Handler(void)
{
    doom_fault_trap("UsageFault");
}

void tx_startup_err_callback(void *p_instance, void *p_data)
{
    (void) p_instance;
    (void) p_data;
    doom_fault_trap("tx_startup_err");
}
