/**
 * **************************************************************************************
 * @file    driver_irda.c
 * 
 * @author  Borg Xiao
 * @date    20230904 create this file @ref7231
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 * */

#ifndef _DRIVER_IRDA_H_
#define _DRIVER_IRDA_H_


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int irda_rx_code_get(uint8_t *buff, int size);
void irda_rx_init(void);
void irda_rx_isr(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* _DRIVER_IRDA_H_ */
