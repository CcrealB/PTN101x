#include "config.h"

#ifdef CONFIG_APP_IR_RX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver_gpio.h"
#include "driver_irda.h"
#include "app_irda.h"
#include "msg_pub.h"
#include "irda_comm.h"


#define IR_RX_FLAG_HEAD_OK       (1 << 0)
#define IR_RX_FLAG_RX_CMP        (1 << 1)
#define IR_RX_FLAG_REPETE        (1 << 2)

#ifdef CONFIG_IRDA
static uint16_t IR_Code[] = {         \
    0x0,    /* KEY_play */                  \
    0x5,    /* KEY_VOL- */                  \
    0x6,    /* KEY_VOL+ */                  \
    0x9,    /* KEY_PREV */                  \
    0xa,    /* KEY_NEXT */                  \
    0x4,    /* KEY_MODE */                  \
    0x1,    /* KEY_CH-  */                  \
    0x2,    /* KEY_CH+  */                  \
    0x8,    /* KEY_0    */                  \
    0xc,    /* KEY_1    */                  \
    0xd,    /* KEY_2    */                  \
    0xe,    /* KEY_3    */                  \
    0x10,   /* KEY_4    */                  \
    0x11,   /* KEY_5    */                  \
    0x12,   /* KEY_6    */                  \
    0x14,   /* KEY_7    */                  \
    0x15,   /* KEY_8    */                  \
    0x16    /* KEY_9    */                  \
};
#define IR_CODE_SIZE        (sizeof(IR_Code)/sizeof(uint16_t))

__attribute__((weak)) void app_ir_rx_ctrl(uint8_t addr, uint8_t cmd)
{
    uint32_t temp, index;
    for (index=0; index<IR_CODE_SIZE; index++)
    {
        if (temp == IR_Code[index])
            break;
    }

    if((0xFF != index) && (IRKey_handler[index]))
    {
        IRKey_handler[index]();
    }
}
#else
__attribute__((weak)) void app_ir_rx_ctrl(uint8_t addr, uint8_t cmd) {}
#endif


void app_ir_rx_init(void)
{
    IRDA_LOG_I("RX_IO:%d\n", IRDA_RX_IO);
    if(IRDA_RX_IO == GPIO2)
    {
        irda_rx_init();
    }
    else
    {
        irda_rx_sw_init();
    }
}

static uint8_t s_ir_rx_flag = 0;
void irda_rx_evt_set_rx_cmp(void)
{ 
    s_ir_rx_flag |= IR_RX_FLAG_RX_CMP;
    msg_put(MSG_IRDA_RX);
}
void irda_rx_evt_set_repete(void)
{
    s_ir_rx_flag |= IR_RX_FLAG_REPETE;
    msg_put(MSG_IRDA_RX);
}

uint32_t irda_rx_event_get(void)
{
    uint32_t event = s_ir_rx_flag;
    if(s_ir_rx_flag & (IR_RX_FLAG_REPETE | IR_RX_FLAG_RX_CMP)){
        s_ir_rx_flag &= ~(IR_RX_FLAG_REPETE | IR_RX_FLAG_RX_CMP);
        return event;
    }
    return 0;
}

void app_ir_rx_proc(void)
{
    if(irda_rx_event_get())
    {
        NEC_IRDA_t ir_rx;
        if(IRDA_RX_IO == GPIO2){
            irda_rx_code_get((uint8_t*)&ir_rx, sizeof(NEC_IRDA_t));
        }else{
            irda_rx_code_get_sw((uint8_t*)&ir_rx, sizeof(NEC_IRDA_t));
        }
        user_ir_rx_callback((uint8_t*)&ir_rx, sizeof(NEC_IRDA_t));
    }
}


/** @brief pelase reimplement this functions in your project/app */
__attribute__((weak)) void user_ir_rx_callback(uint8_t *buff, int size)
{
#ifdef CONFIG_IRDA
    int err = 0;
    NEC_IRDA_t *p_ir_rx = (NEC_IRDA_t*)buff;
    if (p_ir_rx->addr & p_ir_rx->addr_n & 0xFF) { err = -1; } /* addr check */
    if (p_ir_rx->cmd & p_ir_rx->cmd_n & 0xFF) { err = -1; } /* cmd data check */
    if(err == 0)
    {
        app_ir_rx_ctrl(p_ir_rx->addr, p_ir_rx->cmd);
    }
    IRDA_LOG_D("ir rx, err:%d, addr: 0x%02X, cmd:0x%02X\n", err, p_ir_rx->addr, p_ir_rx->cmd);
#else
    IRDA_LOG_I("ir rx buff[%d](HEX): %02X %02X %02X %02X\n", size, buff[0], buff[1], buff[2], buff[3]);
#endif
}


#endif /* CONFIG_APP_IR_RX */

