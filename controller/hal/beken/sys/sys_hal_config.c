/***********************************************************************
 *
 * MODULE NAME:    sys_hal_config.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Customer Specific Configuration
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ***********************************************************************/

#include "sys_config.h"
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
#include "bt_test.h"
#endif

/*****************************************************************************
 * SYShal_config_Initialise
 *
 * Setup/override the major system configuration structure g_sys_config.
 * Customer modifiable.
 *
 *****************************************************************************/
void SYShal_config_Initialise(void)
{
    t_bd_addr volatile bd_addr = {{0x3d, 0x01, 0x91, 0x19, 0x92, 0x38}};
    SYSconfig_Set_Local_BD_Addr(bd_addr);
}
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
void SYShal_config_set_EDR3_feature(void)
{
    extern t_SYS_Config g_sys_config;
    if(BTtst_Get_DUT_Mode() == DUT_DISABLED)
    {
        g_sys_config.feature_set[3] &= ~0x04;
    }
    else
    {
        g_sys_config.feature_set[3] |= 0x04;
    }
}
#endif

void SYShal_config_Set_Role_Switch_feature(void)
{
    extern t_SYS_Config g_sys_config;
    extern uint8_t app_check_bt_mode(uint8_t mode);

#if 1
    if(app_check_bt_mode(BT_MODE_1V1))
    {
        g_sys_config.feature_set[0] |= 0x20;
    }
    else
    {
        if(1 == g_LM_config_info.num_acl_links)
        {
            g_sys_config.feature_set[0] |= 0x20;
        }
        else
        {
            g_sys_config.feature_set[0] &= ~0x20;
        }
    }
#else
    g_sys_config.feature_set[0] &= ~0x20;
#endif
}

void SYShalconfig_BD_ADDR_Was_Set(void)
{
}

void SYShalconfig_Set_System_Hardware_Configuration(uint32_t info)
{
}

void SYShalconfig_Device_Class_Was_Set(void)
{
}

void SYShalconfig_Unit_Key_Was_Set(void)
{
}
