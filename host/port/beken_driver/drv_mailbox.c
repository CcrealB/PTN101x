/**
 **************************************************************************************
 * @file    drv_tmailbox.c
 * @brief   Driver for the mailbox between MCU and DSP
 *
 * @author  Aixing.Li
 * @version V2.0.0
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "bkreg.h"
#include "drv_mailbox.h"
#include "sys_irq.h"
// #include "config_debug.h"

#define MBX_LOG_E(fmt,...)  os_printf("[MBX|ERR:%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#if 0
static void mbx_critical_code_protect(uint8_t en)
{
    #if 1
    static uint32_t interrupts_info, mask;
    if(en) SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    else SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    #else
    static uint32_t cpu_flags;
    if(en) SYSirq_Disable_Interrupts_Except(&cpu_flags, ((1 << VIC_IDX_CEVA) | (1 << VIC_IDX_USB0)));
    else SYSirq_Enable_All_Interrupts(cpu_flags);
    #endif
    // REG_GPIO_0x0C = en ? 2 : 0; //debug
}
#endif

int mbx_mcu2dsp_transfer(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2, uint32_t *rsp)
{
    uint32_t interrupts_info, mask;
    if(mailbox_mcu2dsp_is_busy()) { 
        MBX_LOG_E("mailbox_mcu2dsp_is_busy:0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%p!\n", cmd, param0, param1, param2, rsp);
        return -1;
    }
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    mailbox_mcu2dsp_send_until_recv(cmd, param0, param1, param2);
    if(rsp != NULL)
    {//mailbox_dsp2mcu_ack_get(0~3)
        rsp[0] = REG_MBOX1_MAIL1;
        rsp[1] = REG_MBOX1_0x05;
        rsp[2] = REG_MBOX1_0x06;
        rsp[3] = REG_MBOX1_0x07;
    }
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    return 0;
}

void mailbox_mcu2dsp_send(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
    REG_MBOX0_MAIL0  = cmd;
    REG_MBOX0_0x01   = param0;
    REG_MBOX0_0x02   = param1;
    REG_MBOX0_0x03   = param2;
    REG_MBOX0_READY |= 1;
}

void mailbox_mcu2dsp_resp(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
    REG_MBOX0_MAIL1  = cmd;
    REG_MBOX0_0x05   = param0;
    REG_MBOX0_0x06   = param1;
    REG_MBOX0_0x07   = param2;
}

void mailbox_dsp2mcu_send(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
    REG_MBOX1_MAIL0  = cmd;
    REG_MBOX1_0x01   = param0;
    REG_MBOX1_0x02   = param1;
    REG_MBOX1_0x03   = param2;
    REG_MBOX1_READY |= 1;
}

void mailbox_dsp2mcu_resp(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
    REG_MBOX1_MAIL1  = cmd;
    REG_MBOX1_0x05   = param0;
    REG_MBOX1_0x06   = param1;
    REG_MBOX1_0x07   = param2;
}

void mailbox_mcu2dsp_send_until_recv(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
    uint32_t t = 100000;

    REG_MBOX0_MAIL0  = cmd;
    REG_MBOX0_0x01   = param0;
    REG_MBOX0_0x02   = param1;
    REG_MBOX0_0x03   = param2;
    REG_MBOX0_READY |= 1;
    __asm__("b.nop 5;");
    __asm__("b.nop 5;");
    __asm__("b.nop 5;");
    __asm__("b.nop 5;");
    __asm__("b.nop 5;");
    __asm__("b.nop 5;");
    while((t--) && (REG_MBOX0_READY & 1)) __asm__("b.nop 5;");
}

uint32_t mailbox_mcu2dsp_is_busy(void)
{
    return REG_MBOX0_READY & 1;
}

uint32_t mailbox_dsp2mcu_is_busy(void)
{
    return REG_MBOX1_READY & 1;
}

uint32_t mailbox_mcu2dsp_ack_get(uint32_t idx)
{
    return *((volatile uint32_t*)(&REG_MBOX0_MAIL1) + idx);
}

uint32_t mailbox_dsp2mcu_ack_get(uint32_t idx)
{
    return *((volatile uint32_t*)(&REG_MBOX1_MAIL1) + idx);
}

#if !defined(CEVAX2) && !defined(TEAKLITE4)

void mailbox_mcu_isr(void)
{
    if(REG_MBOX1_READY & 1)
    {
        mailbox_mcu_cmd_handler((MailBoxCmd*)&REG_MBOX1_MAIL0);

        REG_MBOX1_CLEAR |= 1;
    }

    if(REG_MBOX1_READY & 2)
    {
        mailbox_mcu_cmd_handler((MailBoxCmd*)&REG_MBOX1_MAIL1);

        REG_MBOX1_CLEAR |= 2;
    }
}

__attribute__((weak)) void mailbox_mcu_cmd_handler(MailBoxCmd* mbc)
{

}

#else

void mailbox_dsp_isr(void)
{
    if(REG_MBOX0_READY & 1)
    {
        mailbox_dsp_cmd_handler((MailBoxCmd*)&REG_MBOX0_MAIL0);

        REG_MBOX0_CLEAR |= 1;
    }

    if(REG_MBOX0_READY & 0x2)
    {
        mailbox_dsp_cmd_handler((MailBoxCmd*)&REG_MBOX0_MAIL1);

        REG_MBOX0_CLEAR |= 2;
    }
}

#endif
