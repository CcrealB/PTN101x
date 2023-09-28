/*****************************************************************************Enable_ICU_Intr*
 * MODULE NAME:    sys_main.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Entry point to the Bluetooth stack
 * MAINTAINER:     Tom Kerwick
 * DATE:           10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
******************************************************************************/
#include "sys_config.h"
#include "bt_ctrl_init.h"
#include "bt_mini_sched.h"
#include "sys_irq.h"
#include "sys_mmi.h"
#include "hw_lc.h"
#include "hw_delay.h"
#include "hw_leds.h"
#include "tra_hcit.h"
#include "hc_event_gen.h"
#include "lslc_stat.h"
#include "lmp_config.h"
#include "sys_init.h"

/******************************************************************************
 *
 * FUNCTION:  SYS_Main_Initialise
 * PURPOSE:   Controller Initialisations
 *
 ******************************************************************************/
void controller_init(void)
{
    Bk3000_Initialize();
    SYSconfig_Initialise();
#ifndef CONFIG_BT_FUNC_INVALID
    SYSmmi_Initialise();
    BT_Initialise();
#endif
}
