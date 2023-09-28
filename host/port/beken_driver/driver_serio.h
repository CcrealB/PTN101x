/**
 * **************************************************************************************
 * @file    driver_serio.h
 * 
 * @author  Borg Xiao
 * @date    20230403
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#ifndef _DRIVER_SERIO_H_
#define _DRIVER_SERIO_H_

#define UART_TX_POLL_MODE
#define UART_DEBUG_TX_POLL_MODE
#define TX_FIFO_THRD                0x40
#define RX_FIFO_THRD                0x40
#define DEF_TX_EN                   0x1
#define DEF_RX_EN                   0x1
#define DEF_IRDA_MODE               0x0    // 0:uart mode  1:IRDA MODE
#define DEF_DATA_LEN                0x3  // 0=5bit, 1=6bit, 2=7bit, 3=8bit
#define DEF_PARITY_EN               0x0  // 0=no parity  1: enable parity
#define DEF_PARITY_MODE             0x0  // 0:odd  1: even
#define DEF_STOP_BIT                0x0  // 1bit

typedef struct _UART_HDL_t
{
    uint32_t baud_rate;
    uint8_t UARTx;      //0~2->UART0/1/2
    int8_t tx_io;
    int8_t rx_io;
    uint8_t data_bits   ;// data bit num: 5~8bit
    uint8_t stop_bits   ;// stop bit num: 1~2bit
    uint8_t parity_type ;// parity type: 0=no, 1=odd, 2=even
    uint8_t tx_fifo_thrd;// exp:0x40
    uint8_t rx_fifo_thrd;// exp:0x40
}UART_CTX_t;

void ptn_uart_init(UART_CTX_t *uart_h);
void ptn_uart_uninit(UART_CTX_t *uart_h);
void ptn_uart_enable(UART_CTX_t *uart_h, uint8_t tx_en, uint8_t rx_en);
void ptn_uart_send(UART_CTX_t *uart_h, uint8_t* buff, uint8_t size);

void ptn_uart_write(uint8_t UARTx, uint8_t* buff, uint8_t size);
uint32_t ptn_uart_rx_ready(uint8_t UARTx, uint32_t intr_stat);

uint8_t ptn_uart_tx_fifo_free_get(uint8_t UARTx);
uint8_t ptn_uart_rx_fifo_fill_get(uint8_t UARTx);

//isr
void ptn_uart_isr(uint8_t UARTx);
void ptn_uart_rx_byte(uint8_t UARTx, uint8_t data);
void ptn_uart_rx_time_out(uint8_t UARTx);
/* ----------------------------------------------- */ //hal api
void hal_uart0_init(uint32_t bps, uint8_t tx_en, uint8_t rx_en);
void hal_uart1_init(uint8_t tx_io, uint8_t rx_io, uint32_t bps, uint8_t tx_en, uint8_t rx_en);
void hal_uart2_init(uint32_t bps, uint8_t tx_en, uint8_t rx_en);

/* ----------------------------------------------- */ //example for app
void app_uart0_init_def(uint8_t en);
void app_uart1_init_def(uint8_t en, uint8_t io16_17_en);
void app_uart2_init_def(uint8_t en);

void controller_read_virtual_uart(uint8_t ch);
#define host_write_virtual_uart(ch)  (controller_read_virtual_uart(ch))
uint16_t uart_send_poll( uint8_t * buff, int len );

#endif // _DRIVER_SERIO_H_
