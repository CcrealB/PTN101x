#ifndef _PARTHUS_SYS_INIT_
#define _PARTHUS_SYS_INIT_

/***********************************************************************
 *
 * HEADER NAME:    sys_init.h
 * PROJECT CODE:   BlueStream
 * DESCRIPTION:    hal specific init code
 * MAINTAINER:     Raymond Burke
 * CREATION DATE:  21 September 2001
 *
 * SOURCE CONTROL: $Id: 
 *
 * LICENSE:
 *     This source code is copyright (c) 2001-2004 Ceva Inc.
 *     All rights reserved.
 *
 *    
 ***********************************************************************/

/* void SYSinit_Initialise(void); */

typedef struct _struct_beken_config {
	uint8_t g_enable_32k_sleep;
	uint8_t g_CPU_Halt_mode;    //0:normal mode; 1:only stop CPU when acl number = 0; 2:not stop CPU
	uint8_t g_enable_gpio_eint_wakeup;
	uint8_t g_Disable_ACL_active_check_when_Sleep;
	uint8_t g_Enable_TX_Power_Control;
	uint8_t cur_cpu_clk;
    uint8_t Enable_Role_Switch;
    uint8_t Enable_Qos;
} t_bt_beken_config;


typedef void *(*jmalloc_t)(uint32_t size, uint16_t flags);
typedef void (*jfree_t)(void *ptr);
typedef void (*juart_receive_t)(uint8_t *buf, uint16_t size);
typedef int32_t (*os_printf_t)(const char *fmt, ...);
typedef uint8_t (*get_app_env_bt_mode_t)(uint8_t);
typedef void (*set_sys0x4d_reg_t)(uint32_t reg_val);
typedef uint8_t (*get_sys0x4d_reg_t)(void);
typedef void (*app_modify_bt_ldo_t)(void);


typedef struct bt_ctrl_app_cbs_s {
    jmalloc_t                           jmalloc_c;
    jfree_t                             jfree_c;
    juart_receive_t                     juart_receive_c;
    os_printf_t                         os_printf_c;
    get_app_env_bt_mode_t               get_app_env_bt_mode_c;
    set_sys0x4d_reg_t              set_sys0x4d_reg_c;
    get_sys0x4d_reg_t              get_sys0x4d_reg_c;
    app_modify_bt_ldo_t           app_modify_bt_ldo_c;
}t_bt_ctrl_app_cbs;

typedef void (*SYSirq_Disable_Interrupts_Save_Flags_T)(uint32_t *flags, uint32_t *mask);
typedef void (*SYSirq_Interrupts_Restore_Flags_T)(uint32_t flags, uint32_t mask);
typedef void (*SYSirq_Disable_GPIO_INT_Wakeup_T)(void);
typedef void (*SYSirq_Enable_GPIO_INT_Wakeup_T)(void);
typedef void (*SYSirq_Interrupts_Clear_Trig_Flags_T)(void);

typedef struct bt_ctrl_irq_cbs_s {
    SYSirq_Disable_Interrupts_Save_Flags_T          SYSirq_Disable_Interrupts_Save_Flags_C;
    SYSirq_Interrupts_Restore_Flags_T               SYSirq_Interrupts_Restore_Flags_C;
    SYSirq_Interrupts_Clear_Trig_Flags_T            SYSirq_Interrupts_Clear_Trig_Flags_C;
    SYSirq_Enable_GPIO_INT_Wakeup_T                 SYSirq_Enable_GPIO_INT_Wakeup_C;
    SYSirq_Disable_GPIO_INT_Wakeup_T                SYSirq_Disable_GPIO_INT_Wakeup_C;                                 
}t_bt_ctrl_irq_cbs;

typedef void (*bt_disable_host_timer_t)(void);
typedef void (*bt_enable_host_timer_t)(void);
typedef int8_t (*bt_host_sleep_allowed_t)(void);
typedef void (*bt_charge_set_sniff_mode_t)(void);
typedef void (*bt_set_cpu_sleep_mode_t)(void); /* = close 26M */
typedef void (*bt_set_cpu_active_mode_t)(void);/* = open 26M */
typedef void (*sys_gpio_btn_wkup_enable_t)(void);
typedef void (*bt_set_cpu_clk_t)(int clock_sel, int div);

typedef struct bt_ctrl_sleep_cbs_s {
    bt_disable_host_timer_t         bt_disable_host_timer;
    bt_enable_host_timer_t          bt_enable_host_timer;
    bt_host_sleep_allowed_t         bt_host_sleep_allowed;
    bt_set_cpu_sleep_mode_t         bt_set_cpu_sleep_mode;  // close 26M
    bt_set_cpu_active_mode_t        bt_set_cpu_active_mode; // open 26M
    sys_gpio_btn_wkup_enable_t      sys_gpio_btn_wkup_enable;   
    bt_set_cpu_clk_t                 bt_set_cpu_clk;
}t_bt_ctrl_sleep_cbs;

typedef uint8_t (*get_hfps_sco_and_incoming_flag_t)(void);
typedef uint8_t (*get_sbc_encode_buffer_status_t)(void);

typedef struct bt_ctrl_profiles_flag_cbs_s {
    get_hfps_sco_and_incoming_flag_t    get_hfps_sco_and_incoming_flag_c;  
    get_sbc_encode_buffer_status_t      get_sbc_encode_buffer_status_c;
}t_bt_ctrl_profiles_flag_cbs;

void Bk3000_Initialize(void);
void BK3000_CFG_Initial(void);

void sys_controller_sleep_attach(const t_bt_ctrl_sleep_cbs *ctrl_sleep);
void sys_controller_app_attach(const t_bt_ctrl_app_cbs *ctrl_app);
void sys_controller_irq_attach(const t_bt_ctrl_irq_cbs *ctrl_irq);
void sys_controller_profile_flag_attach(const t_bt_ctrl_profiles_flag_cbs *ctrl_pf_flag);

#endif

