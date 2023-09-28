/**
 * **************************************************************************************
 * @file    driver_serio.c
 * 
 * @author  Borg Xiao
 * @date    20230403
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "tra_hcit.h"
#include "drv_system.h"

#ifndef __IO
#define __IO volatile
#endif

static const uint32_t UART_ADDR[] = {MDU_UART0_BASE_ADDR, MDU_UART1_BASE_ADDR, MDU_UART2_BASE_ADDR};
#define UART_REG(UARTx)    (__IO REG_UART_t*)UART_ADDR[UARTx]

extern void uart_send (unsigned char *buff, unsigned int len);
uint16_t uart_send_poll( uint8_t * buff, int len )
{
    int i;

#ifdef HCI_DEBUG
    /* os_printf("uart_send_poll: "); */
	/* for( i = 0; i < len; i++) */
    /*     os_printf("0x%02x ", buff[i]); */
    /* os_printf("\r\n"); */
#endif
    if (BT_HOST_MODE == NONE_CONTROLLER)
        uart_send(buff, len);
    else if(BT_HOST_MODE == JUNGO_HOST){
        for( i = 0; i < len; i++)
            host_write_virtual_uart(buff[i]);
	}
    return len;
}

typedef union _UART_CFG_e
{
    struct{
        __IO uint32_t tx_en          :1;
        __IO uint32_t rx_en          :1;
        __IO uint32_t irda_fmt_en    :1; // data format: 0=uart, 1=irda.
        __IO uint32_t data_bit_len   :2; // 0=5bit, 1=6bit, 2=7bit, 3=8bit
        __IO uint32_t parity_en      :1; // 0=no parity  1: enable parity
        __IO uint32_t parity_type    :1; // 0:odd  1: even
        __IO uint32_t stop_bit_len   :1; // 0:1bit, 1:2bit
        __IO uint32_t clk_div        :13;// bit[20:8]
        __IO uint32_t rsvd           :11;
    };
    __IO uint32_t val;
}UART_CFG_e;

typedef struct _REG_UART_t{
    __IO UART_CFG_e base_cfg;
    __IO uint32_t fifo_cfg;
    __IO uint32_t fifo_stat;
    __IO uint32_t fifo_port;
    __IO uint32_t intr_cfg;
    __IO uint32_t intr_stat;
    __IO uint32_t flow_cfg;
    __IO uint32_t wake_cfg;
}REG_UART_t;

static void uart_config(UART_CTX_t *uart_h, uint8_t en)
{
    __IO REG_UART_t* reg = UART_REG(uart_h->UARTx);
    SYS_MEM_CLK mem_clk_idx[] = {SYS_MEM_CLK_UART0, SYS_MEM_CLK_UART1, SYS_MEM_CLK_UART2};
    SYS_PERI_CLK peri_clk_idx[] = {SYS_PERI_CLK_UART0, SYS_PERI_CLK_UART1, SYS_PERI_CLK_UART2};
    SYS_PERI_IRQ peri_irq_idx[] = {SYS_PERI_IRQ_UART0, SYS_PERI_IRQ_UART1, SYS_PERI_IRQ_UART2};
    SYS_PERI_CLK_GATING clk_gating[] = {SYS_PERI_CLK_GATING_UART0, SYS_PERI_CLK_GATING_UART1, SYS_PERI_CLK_GATING_UART2};
    UART_CFG_e base_cfg = {
        .clk_div = UART_CLOCK_FREQ_26M / uart_h->baud_rate - 1,
        .stop_bit_len = (uart_h->stop_bits == 2) ? 1 : 0,
        .parity_type = (uart_h->parity_type == 2) ? 1 : 0,
        .parity_en = !!uart_h->parity_type,
        .data_bit_len = ((uart_h->data_bits >= 5) && (uart_h->data_bits <= 8)) ? (uart_h->data_bits - 5) : 3,
        .irda_fmt_en = 0,
        .rx_en = 0,
        .tx_en = 0,
    };
    if(en)
    {
        // REG_SYSTEM_0x01 &= ~(1 << 22);
        if(uart_h->UARTx == 1) system_ctrl(SYS_CTRL_CMD_UART2_GPIO_MAPPING, (uart_h->tx_io == 16) ? 1 : 0);
        system_mem_clk_enable(mem_clk_idx[uart_h->UARTx]);
        system_peri_clk_enable(peri_clk_idx[uart_h->UARTx]);
        system_peri_clk_gating_disable(clk_gating[uart_h->UARTx]);
        reg->base_cfg.val = base_cfg.val;
        reg->fifo_cfg = ((0 << SFT_UART0_FIFO_CONF_RX_STOP_DETECT_TIME)/* 0：32  1：64  2：128  3：256 */
                        | (uart_h->rx_fifo_thrd << SFT_UART0_FIFO_CONF_RX_FIFO_THRESHOLD)
                        | (uart_h->tx_fifo_thrd << SFT_UART0_FIFO_CONF_TX_FIFO_THRESHOLD));
        reg->intr_cfg = MSK_UART0_INT_ENABLE_RX_STOP_END_MASK | MSK_UART0_INT_ENABLE_RX_FIFO_NEED_READ_MASK;
        system_peri_mcu_irq_enable(peri_irq_idx[uart_h->UARTx]);
    }
    else
    {
        reg->base_cfg.val = base_cfg.val;
        system_peri_mcu_irq_disable(peri_irq_idx[uart_h->UARTx]);
        system_peri_clk_gating_enable(clk_gating[uart_h->UARTx]);
        system_peri_clk_disable(peri_clk_idx[uart_h->UARTx]);
        system_mem_clk_disable(mem_clk_idx[uart_h->UARTx]);
        // REG_SYSTEM_0x01 |= (1 << 22);
    }
}

void ptn_uart_init(UART_CTX_t *uart_h) { uart_config(uart_h, 1); }
void ptn_uart_uninit(UART_CTX_t *uart_h) { uart_config(uart_h, 0); }

void ptn_uart_enable(UART_CTX_t *uart_h, uint8_t tx_en, uint8_t rx_en)
{
    __IO REG_UART_t* reg = UART_REG(uart_h->UARTx);
    reg->base_cfg.tx_en = tx_en;
    reg->base_cfg.rx_en = rx_en;
    *((volatile unsigned long *)(MDU_GPIO_BASE_ADDR + ((uart_h->tx_io) << 2))) = tx_en ? 0x70 : 0x08;//Tx&PullUp : HRES&NoPull;
    *((volatile unsigned long *)(MDU_GPIO_BASE_ADDR + ((uart_h->rx_io) << 2))) = rx_en ? 0x7C : 0x08;//Rx&PullUp : HRES&NoPull;
    if(uart_h->UARTx == 1){
        if(tx_en) system_gpio_peri_config(uart_h->tx_io, (uart_h->tx_io == 16) ? 1 : 0);	//GPIO16: Perial Mode 2 function; GPIO6: Perial Mode 1 function
        if(rx_en) system_gpio_peri_config(uart_h->rx_io, (uart_h->rx_io == 17) ? 1 : 0);	//GPIO17: Perial Mode 2 function; GPIO7: Perial Mode 1 function
    }
}

uint32_t ptn_uart_rx_ready(uint8_t UARTx, uint32_t intr_stat)
{
    // __IO REG_UART_t* reg = UART_REG(UARTx);
    // uint32_t intr_stat = reg->intr_stat;
    // reg->intr_stat = intr_stat;
    if(intr_stat & (MSK_UART0_INT_STATUS_RX_STOP_ERROR | MSK_UART0_INT_STATUS_RX_PARITY_ERROR | MSK_UART0_INT_STATUS_RX_FIFO_OVER_FLOW))
    {
        LOG_W(UART, "[UART%d] rx ov/err:0x%X\n", UARTx, intr_stat);
    }

    return (intr_stat & (MSK_UART0_INT_STATUS_RX_FIFO_NEED_READ | MSK_UART0_INT_STATUS_RX_STOP_END));
}


void ptn_uart_send(UART_CTX_t *uart_h, uint8_t* buff, uint8_t size)
{
    __IO REG_UART_t* reg = UART_REG(uart_h->UARTx);
    while(size){ if(reg->fifo_stat & MSK_UART0_FIFO_STATUS_FIFO_WR_READY){ reg->fifo_port = *buff++; size--; } }
}

//UARTx:0~2
void ptn_uart_write(uint8_t UARTx, uint8_t* buff, uint8_t size)
{
    __IO REG_UART_t* reg = UART_REG(UARTx);
    while(size){ if(reg->fifo_stat & MSK_UART0_FIFO_STATUS_FIFO_WR_READY){ reg->fifo_port = *buff++; size--; } }
}

uint8_t ptn_uart_tx_fifo_free_get(uint8_t UARTx)
{
    __IO REG_UART_t* reg = UART_REG(UARTx);
    return (256 - (reg->fifo_stat & MAX_UART0_FIFO_STATUS_TX_FIFO_COUNT));
}

uint8_t ptn_uart_rx_fifo_fill_get(uint8_t UARTx)
{
    __IO REG_UART_t* reg = UART_REG(UARTx);
    return ((reg->fifo_stat >> 8) & MAX_UART0_FIFO_STATUS_RX_FIFO_COUNT);
}

static void hal_uart_init(
    uint8_t UARTx,                  //uart hardware number: 0/1/2
    uint32_t bps,                   //baudrate, exp:115200/2000000
    uint8_t tx_io, uint8_t rx_io,   //(GPIO0, GPIO1) / (GPIO6/GPIO16, GPIO7/GPIO17) / (GPIO34, GPIO35)
    uint8_t tx_en, uint8_t rx_en    //enable config: 0/1
)
{
    UART_CTX_t uart = {
        .UARTx = UARTx,
        .baud_rate = bps,
        .tx_io = tx_io,
        .rx_io = rx_io,
        .data_bits = 8,
        .stop_bits = 1,
        .parity_type = 0,
        .tx_fifo_thrd = 0x40,
        .rx_fifo_thrd = 0x40,
    };
    (tx_en || rx_en) ? ptn_uart_init(&uart) : ptn_uart_uninit(&uart);
    ptn_uart_enable(&uart, tx_en, rx_en);
}


/* ----------------------------------------------- */ //hal api
void hal_uart0_init(uint32_t bps, uint8_t tx_en, uint8_t rx_en)
{
    hal_uart_init(0, bps, GPIO0, GPIO1, tx_en, rx_en);
}

void hal_uart1_init(uint8_t tx_io, uint8_t rx_io, uint32_t bps, uint8_t tx_en, uint8_t rx_en)
{
    hal_uart_init(1, bps, tx_io, rx_io, tx_en, rx_en);
}

void hal_uart2_init(uint32_t bps, uint8_t tx_en, uint8_t rx_en)
{
    hal_uart_init(2, bps, GPIO34, GPIO35, tx_en, rx_en);
}

/* ----------------------------------------------- */ //example for app
void app_uart0_init_def(uint8_t en)
{
    hal_uart0_init(UART_BAUDRATE_115200, en, en);
}

void app_uart1_init_def(uint8_t en, uint8_t io16_17_en)
{
    if(io16_17_en){
        hal_uart1_init(GPIO16, GPIO17, UART_BAUDRATE_115200, en, en);
    }else{
        hal_uart1_init(GPIO6, GPIO7, UART_BAUDRATE_115200, en, en);
    }
}

void app_uart2_init_def(uint8_t en)
{
    hal_uart2_init(UART_BAUDRATE_115200, en, en);
}


#if 0
// #define UART_RX_TEST

__attribute__((weak)) void ptn_uart_rx_byte(uint8_t UARTx, uint8_t data) {}
__attribute__((weak)) void ptn_uart_rx_time_out(uint8_t UARTx) {}

void ptn_uart_isr(uint8_t UARTx)
{
    __IO REG_UART_t* reg = UART_REG(UARTx);
    uint32_t status = reg->intr_stat;
    if(status & (MSK_UART0_INT_STATUS_RX_STOP_ERROR | MSK_UART0_INT_STATUS_RX_PARITY_ERROR | MSK_UART0_INT_STATUS_RX_FIFO_OVER_FLOW))
    {
        LOG_W(UART, "[UART%d] rx ov/err:0x%X\n", UARTx, status);
    }

    if(status & (MSK_UART0_INT_STATUS_RX_FIFO_NEED_READ | MSK_UART0_INT_STATUS_RX_STOP_END))
    {
        uint8_t data;
        while(reg->fifo_stat & MSK_UART0_FIFO_STATUS_FIFO_RD_READY)
        {
            data = (reg->fifo_port & MSK_UART0_FIFO_PORT_RX_FIFO_DOUT) >> 8;
            ptn_uart_rx_byte(UARTx, data);
        #ifdef UART_RX_TEST
            void uart_rx_byte_test(uint8_t data);
            uart_rx_byte_test(data);
        #endif
        }
        if(status & MSK_UART0_INT_STATUS_RX_STOP_END){
            ptn_uart_rx_time_out(UARTx);
        }
    }
    reg->intr_stat = status;
}
// __attribute__((weak)) void uart_handler(void) { ptn_uart_isr(0); }
// __attribute__((weak)) void uart1_handler(void) { ptn_uart_isr(1); }
// __attribute__((weak)) void uart2_handler(void) { ptn_uart_isr(2); }

#ifdef UART_RX_TEST
//上位机循环发： 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F .
#define UART_TEST_DBG(fmt,...)      os_printf(fmt, ##__VA_ARGS__)
void uart_rx_byte_test(uint8_t data)
{
    static uint32_t cnt = 0;//每收1byte计数一次
    static uint32_t frame_cnt = 0;//每收32byte计数一次
    uint32_t byte_cnt; 
    uint32_t lim_cnt;
    if(data == 0xFF){
        UART_TEST_DBG("fra:%d, cnt:%d(lim_cnt:%d), dat:%d\n", frame_cnt, byte_cnt, lim_cnt, data);
        cnt = 0;
        frame_cnt = 0;
        UART_TEST_DBG(" -- clear\n");
        return;
    }
    
    byte_cnt = cnt++;
    lim_cnt = byte_cnt & 0x1F;
    if(lim_cnt == 0x1F){ frame_cnt++; }
    if(lim_cnt != data)
    {
        UART_TEST_DBG("\t byte_cnt err!!!");
        UART_TEST_DBG("fra:%d, cnt:%d(lim_cnt:%d), dat:%d\n", frame_cnt, byte_cnt, lim_cnt, data);
    }
//monitor
    static uint32_t tloop = 0;
    if((data == 31) && sys_timeout(tloop, 5000))
    {
        tloop = sys_time_get();
        UART_TEST_DBG("\t ok...");
        UART_TEST_DBG("fra:%d, cnt:%d(lim_cnt:%d), dat:%d\n", frame_cnt, byte_cnt, lim_cnt, data);
    }
}
#endif

#endif

// EOF
