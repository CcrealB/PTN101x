/**
 * **************************************************************************************
 * @file    app_irda.c
 * 
 * @author  Borg Xiao
 * @date    20230904 create this file
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 
## 说明：本文件未NEC红外解码驱动模块，通过给用户提供回调函数来实现不同应用。
 
## 使用步骤：
    - 1、用户在方案头文件中启用红外接收配置（2选1）：
        - 红外硬件解码方式定义：
            #define IRDA_RX_IO          GPIO2  //when use GPIO2, hardware mode enable, IRDA_RX_PWM_TIMER could be undefine.
        - 红外软件IO模拟解码方式定义：（需要定义两个宏，其中一个是定义一路PWM作为定时器用)
            #define IRDA_RX_IO          GPIO13 //when use GPIO2, hardware mode enable, IRDA_RX_PWM_TIMER could be undefine.
            #define IRDA_RX_PWM_TIMER   GPIO19 //use as timer for recieve parse counter
    - 2、用户在方案文件中定义并实现函数 “void user_ir_rx_callback(uint8_t *buff, int size)”
        - size固定为4， buff则为红外地址码和命令码，格式参考本文件结构体类型 NEC_IRDA_t，根据需要实现相关应用。
 * */

#ifndef _APP_IRDA_H_
#define _APP_IRDA_H_


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#define IR_RX_FLAG_HEAD_OK       (1 << 0)
#define IR_RX_FLAG_RX_CMP        (1 << 1)
#define IR_RX_FLAG_REPETE        (1 << 2)


typedef struct _NEC_IRDA_t{
    uint8_t addr;
    uint8_t addr_n;
    uint8_t cmd;
    uint8_t cmd_n;
}NEC_IRDA_t;

void app_ir_rx_init(void);
void app_ir_rx_proc(void);

uint32_t irda_rx_event_get(void);
void user_ir_rx_callback(uint8_t *buff, int size);

/* ------------------------------------------------------------------------- */

extern void irda_rx_sw_init(void);
extern int irda_rx_code_get_sw(uint8_t *buff, int size);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* _APP_IRDA_H_ */
