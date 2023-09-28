/**
 * **************************************************************************************
 * @file    hal_dsp_audio.c
 * 
 * @author  Borg Xiao
 * @date    20230508
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include "config.h"

#if SYS_DSP_ENABLE == 1

#include <string.h>
#include "bkreg.h"
#include "drv_system.h"
#include "dsp_include.h"
#include "app_dsp.h"
#include "hal_dsp_audio.h"
#include "sys_irq.h"



/* ******************************************************************************* */
/* ******************************************************************************* */
/* ******************************************************************************* */ //dsp effect cmd

// ------------------------------------------ single cmd set/get
/**
 * @brief set a dsp paramater one time
 * @param p_cmd uint32_t array[3], dsp effect cmd that send to dsp
 *      format: FF FF FF FF Id(uint32_t) param(uint32_t), 
 *      exp:    FF FF FF FF CD CC 8C 3F 00 00 80 3F
 * @param fast_en 1:process cmd immediately mcu need wait more time. 0:send cmd to msg pool
 * @return:     for fast_en=1: -1:err/0:ok
 *              for fast_en=0: -1:err/0~63:ok and the value is fifo free num
 * @note demo:  send cmd with head: 0xFFFFFFFF, cmd 0x12345678, param:0x3F80000
 * uint8_t cmd[12] -> FF FF FF FF 78 56 34 12 00 00 80 3F
 * mbx_cmd_mcu2dsp_set_single((uint32_t*)&cmd[0], 12, 0);
 * */
int mbx_cmd_mcu2dsp_set_single(uint32_t *p_cmd, uint8_t cmd_sz, uint8_t fast_en)
{
    if(p_cmd == NULL || cmd_sz > 12) {
        MDSP_LOG_E("cmd:0x%p, size:%d\n", p_cmd, cmd_sz);
        return -1;
    }

    if(fast_en) 
    {
        uint32_t dsp2mcu_ack_data[4] = {0};
        uint32_t mbx_cmd = USR_MBX_CMD_AUDIO_EFT_SET_SINGLE | MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_NEED_RSP_FLAG;
        mbx_mcu2dsp_transfer(mbx_cmd, (uint32_t)p_cmd[0], (uint32_t)p_cmd[1], (uint32_t)p_cmd[2], &dsp2mcu_ack_data[0]);
        uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
        uint32_t ack_p0 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        uint32_t ack_p1 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
        uint32_t ack_p2 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
        return (ack_cmd == USR_MBX_CMD_AUDIO_EFT_SET_SINGLE && ack_p0 == p_cmd[0] && ack_p1 == p_cmd[1] && ack_p2 == p_cmd[2]) ? 0 : -1;
    }
    else
    {
        #if 1 //push to share msg pool, dsp no intr
        MailBoxCmd mbc;
        mbc.cmd = USR_MBX_CMD_AUDIO_EFT_SET_SINGLE;
        memcpy(&mbc.param0, p_cmd, cmd_sz);
        return app_dsp_cmd_msg_push(&mbc);//msg pool free num(0~63)
        #else //trig dsp intr, and dsp push cmd in dsp internal msg pool 
        uint32_t dsp2mcu_ack_data[4] = {0};
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_EFT_SET_SINGLE, (uint32_t)p_cmd[0], (uint32_t)p_cmd[1], (uint32_t)p_cmd[2], &dsp2mcu_ack_data[0]);
        uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
        uint32_t free = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        // uint32_t fill = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
        return (ack_cmd == USR_MBX_CMD_AUDIO_EFT_SET_SINGLE) ? free : -1;
        #endif
    }
}

/**
 * @brief get a dsp paramater one time
 * @param p_cmd uint32_t array[3], dsp effect cmd that send to dsp
 *      format: FF FF FF FF Id(uint32_t) param(uint32_t), 
 *      exp:    FF FF FF FF CD CC 8C 3F 00 00 80 3F
 * @return -1:err/0:ok, for ok, get data from p_cmd
 * */
int mbx_cmd_mcu2dsp_get_single(uint32_t *p_cmd, uint8_t cmd_sz)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_EFT_GET_SINGLE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)p_cmd[0], (uint32_t)p_cmd[1], (uint32_t)p_cmd[2], &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    p_cmd[0] = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    p_cmd[1] = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    p_cmd[2] = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
    return ((ack_cmd == USR_MBX_CMD_AUDIO_EFT_GET_SINGLE) ? 0 : -1);
}

// ------------------------------------------ multiple cmd set/get
/**
 * @brief set dsp effect with one or more cmd num
 * @param cmd_buf cmd buff, size = cmd_size * cmd_num;
 * @param cmd_size every cmd size should be fixed in cmd buff
 * @param cmd_num number of command
 * @return: cmd total size ack, 0:ERR, >=12:OK
 * @note demo: send 2 cmd at a time
 * uint8_t cmd_buff[12*2] = FF FF FF FF 00 00 00 00 11 11 11 11 FF FF FF FF 00 00 00 00 11 11 11 11 ;
 * mbx_cmd_mcu2dsp_set((uint8_t*)cmd_buf, 12, 2);
 * */
int mbx_cmd_mcu2dsp_set(uint8_t *cmd_buf, int cmd_size, int cmd_num)
{
    if(((uint32_t)cmd_buf & 0x3) != 0) {
        MDSP_LOG_E("cmd buff must 4byte aligned:%d\n", (uint32_t)cmd_buf & 0x3);
        return -1;
    }
    if(cmd_size == 0 || cmd_num == 0) {
        MDSP_LOG_E("size:%d, num:%d\n", cmd_size, cmd_num);
        return -1;
    }
    uint32_t dsp2mcu_ack_data[4] = {0};
    #if 1
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_EFT_SET | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)cmd_buf, (uint32_t)cmd_size, (uint32_t)cmd_num, &dsp2mcu_ack_data[0]);
    // uint32_t addr = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    int size = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    int num = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
    return (size * num);//(addr == (uint32_t)cmd_buf && size == cmd_size && num == cmd_num);
    #else //return dsp cmd recieve fifo free num(63 max), if > 0, you can continue to send commands
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_EFT_SET, (uint32_t)cmd_buf, (uint32_t)cmd_size, (uint32_t)cmd_num, NULL);
    int msgs_free = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    // int msgs_fill = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    return msgs_free;
    #endif
}

/**
 * @brief set dsp effect with one or more cmd num
 * @param cmd_buf cmd buff, size = cmd_size * cmd_num;
 * @param cmd_size every cmd size should be fixed in cmd buff
 * @param cmd_num number of command
 * @return: cmd total size ack, 0:ERR, >=12:OK
 * @note demo: get 2 mod param at a time
 * uint8_t cmd_buff[12*2];//used to load the returned value
 * mbx_cmd_mcu2dsp_get((uint8_t*)cmd_buf, 12, 2);
 * */
int mbx_cmd_mcu2dsp_get(uint8_t *cmd_buf, int cmd_size, int cmd_num)
{
    if(((uint32_t)cmd_buf & 0x3) != 0) {
        MDSP_LOG_E("cmd buff must 4byte aligned:%d\n", (uint32_t)cmd_buf & 0x3);
        return -1;
    }
    if(cmd_size == 0 || cmd_num == 0) {
        MDSP_LOG_E("size:%d, num:%d\n", cmd_size, cmd_num);
        return -1;
    }
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_EFT_GET | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)cmd_buf, (uint32_t)cmd_size, (uint32_t)cmd_num, &dsp2mcu_ack_data[0]);
    // uint32_t addr = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    int size = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    int num = (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
    return (size * num);//(addr == (uint32_t)cmd_buf && size == cmd_size && num == cmd_num);
}


#ifdef CONFIG_SOFT_FIFO_DCOM
/**
 * @brief send cmd to dsp
 * @param buff mcu to dsp
 * @param size should fixed to 14(no more than 62)
 * @return mcu2dsp fifo free num
 * @note cmd demo
 * pc->mcu: 01 E0 FC 24 2B 80 10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F
 * param:                     10 10                                      (10 10 for dfifo trans sub cmd)
 * buff:                      10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F
 * */
uint32_t com_cmd_mcu2dsp_proc(uint8_t *buff, uint8_t size)
{
    uint32_t ret = 0;
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_MCU2DSP | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, (uint32_t)size, 0, &dsp2mcu_ack_data[0]);
    // uint32_t ok_size = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    uint32_t dfifo_mcu2dsp_free_num = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    // uint32_t dfifo_mcu2dsp_fill_num = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
    ret = dfifo_mcu2dsp_free_num;
    return ret;
}

/**
 * @brief get cmd fifo free num, if > 0, cmd can be send to dsp 
 * get share mem var addr to get dfifo free num, (the var is updated in dsp side)[max is COM_RX_FIFO_FRA_NUM defined in dsp]
 * */
int mcu2dsp_dfifo_free_num_get(void)
{
    #if 1
    static int *p = NULL;
    if(p == NULL)
    {
        uint32_t dsp2mcu_ack_data[4] = {0};
        mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_MCU2DSP_FREE_GET | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
        p = (int*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        DBG_LOG_INFO("p:%p, *p:%d, value:%d\n", p, *p, (int)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1]);
    } 
    return *p;
    #else
    return (int)com_cmd_mcu2dsp_proc((uint8_t*)NULL, (uint32_t)0);
    #endif
}

void mailbox_mcu_cmd_handler(MailBoxCmd* mbc)
{
    uint32_t cmd = mbc->cmd & MAILBOX_CMD_SET_MASK;
    switch (cmd)
    {
    case USR_MBX_CMD_COM_DSP2MCU:
    {
        uint8_t buf[64] = {0};//max recv size is 64
        uint32_t dsp2mcu_ack_data[4] = {0};
        mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_DSP2MCU | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buf, (uint32_t)64, 0, &dsp2mcu_ack_data[0]);
        uint8_t rsp_len = (uint8_t)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        if(g_dsp_log_on_fg && rsp_len < 64) com_cmd_send_proc(buf, rsp_len);
    }break;
    
    default:
        break;
    }
}
#endif


#endif // SYS_DSP_ENABLE
