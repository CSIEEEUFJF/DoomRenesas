/* generated common source file - do not edit */
#include "common_data.h"
#if !defined(SSP_SUPPRESS_ISR_g_i2c) && !defined(SSP_SUPPRESS_ISR_IIC2)
SSP_VECTOR_DEFINE_CHAN(iic_rxi_isr, IIC, RXI, 2);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_i2c) && !defined(SSP_SUPPRESS_ISR_IIC2)
SSP_VECTOR_DEFINE_CHAN(iic_txi_isr, IIC, TXI, 2);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_i2c) && !defined(SSP_SUPPRESS_ISR_IIC2)
SSP_VECTOR_DEFINE_CHAN(iic_tei_isr, IIC, TEI, 2);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_i2c) && !defined(SSP_SUPPRESS_ISR_IIC2)
SSP_VECTOR_DEFINE_CHAN(iic_eri_isr, IIC, ERI, 2);
#endif
/* Instance structure to use this module. */
const fmi_instance_t g_fmi =
{ .p_api = &g_fmi_on_fmi };
const elc_instance_t g_elc =
{ .p_api = &g_elc_on_elc, .p_cfg = NULL };
const cgc_instance_t g_cgc =
{ .p_api = &g_cgc_on_cgc, .p_cfg = NULL };
riic_instance_ctrl_t g_i2c_ctrl;
const riic_extended_cfg g_i2c_extend =
{ .timeout_mode = RIIC_TIMEOUT_MODE_SHORT };
const i2c_cfg_t g_i2c_cfg =
{ .channel = 2,
  .rate = I2C_RATE_FAST,
  .slave = 0x48,
  .addr_mode = I2C_ADDR_MODE_7BIT,
  .sda_delay = 0,
  .rxi_ipl = (3),
  .txi_ipl = (3),
  .tei_ipl = (3),
  .eri_ipl = (3),
  .p_transfer_tx = NULL,
  .p_transfer_rx = NULL,
  .p_callback = NULL,
  .p_context = NULL,
  .p_extend = &g_i2c_extend };
const i2c_master_instance_t g_i2c =
{ .p_ctrl = &g_i2c_ctrl, .p_cfg = &g_i2c_cfg, .p_api = &g_i2c_master_on_riic };
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_cfg = NULL };
void g_common_init(void)
{
}
