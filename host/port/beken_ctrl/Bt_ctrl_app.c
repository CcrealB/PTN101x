#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_ctrl_app.h"
extern uint8_t RAM_CODE sbc_encode_buffer_status(void);
extern void Enable_Bk3000_GPIO_EINT_wakeup(void);
extern RAM_CODE void Disable_Bk3000_GPIO_EINT_wakeup(void);
extern void disable_timer0_pt0(void);
extern int8_t app_is_available_sleep_system(void);
extern void app_modify_bt_ldo(void);
CONST static t_bt_ctrl_app_cbs ctrl_app_cbs =
{
    jmalloc_s,
    jfree_s,
    juart_receive,
    os_printf,
    app_check_bt_mode,
    system_set_0x4d_reg,
    system_get_0x4d_reg,
    app_modify_bt_ldo,
};

CONST static t_bt_ctrl_irq_cbs ctrl_irq_cbs =
{
    SYSirq_Disable_Interrupts_Save_Flags,
    SYSirq_Interrupts_Restore_Flags,
    SYSirq_Interrupts_Clear_Trig_Flags,
    Enable_Bk3000_GPIO_EINT_wakeup,
    Disable_Bk3000_GPIO_EINT_wakeup
};

CONST static t_bt_ctrl_sleep_cbs ctrl_sleep_cbs =
{
    disable_timer0_pt0,
    sniff_enable_timer0_pt0,
    app_is_available_sleep_system,
    SYSpwr_Prepare_For_Sleep_Mode,
    SYSpwr_Wakeup_From_Sleep_Mode,
    gpio_button_wakeup_enable,
    BK3000_set_clock
};
CONST static t_bt_ctrl_profiles_flag_cbs ctr_pf_flag_cbs =
{
    get_2hfps_sco_and_incoming_flag,
    sbc_encode_buffer_status
};
void bt_controller_cb_register(void)
{
    sys_controller_app_attach(&ctrl_app_cbs);
    sys_controller_irq_attach(&ctrl_irq_cbs);
    sys_controller_sleep_attach(&ctrl_sleep_cbs);
    sys_controller_profile_flag_attach(&ctr_pf_flag_cbs);
}

