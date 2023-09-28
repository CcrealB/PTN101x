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
#include "config.h"

#ifdef CONFIG_DRV_IR_RX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drv_system.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "driver_irda.h"
#include "irda_comm.h"
// #include "msg_pub.h"


/* ------------------------------------------------------------------------- */
#define REG_IRDA_BASE_ADDR                  (0x01868000UL)

#define REG_IRDA_CONFIG_ADDR                (REG_IRDA_BASE_ADDR + 0 * 4)
#define REG_IRDA_CONFIG_MASK                0x00FFFFFF
#define REG_IRDA_CONFIG                     (*((volatile uint32_t *) REG_IRDA_CONFIG_ADDR))

#define IRDA_CONFIG_ENABLE_POSI             0
#define IRDA_CONFIG_ENABLE_MASK             (0x01UL << IRDA_CONFIG_ENABLE_POSI)
#define IRDA_CONFIG_ENABLE_SET              (0x01UL << IRDA_CONFIG_ENABLE_POSI)

#define IRDA_CONFIG_POLARITY_POSI           1
#define IRDA_CONFIG_POLARITY_MASK           (0x01UL << IRDA_CONFIG_POLARITY_POSI)
#define IRDA_CONFIG_POLARITY_LOW_ACTIVE     (0x00UL << IRDA_CONFIG_POLARITY_POSI)
#define IRDA_CONFIG_POLARITY_HIGH_ACTIVE    (0x01UL << IRDA_CONFIG_POLARITY_POSI)

#define IRDA_CONFIG_UART_ENABLE_POSI        2
#define IRDA_CONFIG_UART_ENABLE_MASK        (0x01UL << IRDA_CONFIG_UART_ENABLE_POSI)
#define IRDA_CONFIG_UART_ENABLE_CLEAR       (0x00UL << IRDA_CONFIG_UART_ENABLE_POSI)
#define IRDA_CONFIG_UART_ENABLE_SET         (0x01UL << IRDA_CONFIG_UART_ENABLE_POSI)

#define IRDA_CONFIG_UART_LENGTH_POSI        3
#define IRDA_CONFIG_UART_LENGTH_MASK        (0x03UL << IRDA_CONFIG_UART_LENGTH_POSI)
#define IRDA_CONFIG_UART_LENGTH_5_BIT       (0x00UL << IRDA_CONFIG_UART_LENGTH_POSI)
#define IRDA_CONFIG_UART_LENGTH_6_BIT       (0x01UL << IRDA_CONFIG_UART_LENGTH_POSI)
#define IRDA_CONFIG_UART_LENGTH_7_BIT       (0x02UL << IRDA_CONFIG_UART_LENGTH_POSI)
#define IRDA_CONFIG_UART_LENGTH_8_BIT       (0x03UL << IRDA_CONFIG_UART_LENGTH_POSI)

#define IRDA_CONFIG_UART_PAR_EN_POSI        5
#define IRDA_CONFIG_UART_PAR_EN_MASK        (0x01UL << IRDA_CONFIG_UART_PAR_EN_POSI)
#define IRDA_CONFIG_UART_PAR_EN_CLEAR       (0x00UL << IRDA_CONFIG_UART_PAR_EN_POSI)
#define IRDA_CONFIG_UART_PAR_EN_SET         (0x01UL << IRDA_CONFIG_UART_PAR_EN_POSI)

#define IRDA_CONFIG_UART_PAR_MODE_POSI      6
#define IRDA_CONFIG_UART_PAR_MODE_MASK      (0x01UL << IRDA_CONFIG_UART_PAR_MODE_POSI)
#define IRDA_CONFIG_UART_PAR_MODE_EVEN      (0x00UL << IRDA_CONFIG_UART_PAR_MODE_POSI)
#define IRDA_CONFIG_UART_PAR_MODE_ODD       (0x01UL << IRDA_CONFIG_UART_PAR_MODE_POSI)

#define IRDA_CONFIG_UART_STOP_LEN_POSI      7
#define IRDA_CONFIG_UART_STOP_LEN_MASK      (0x01UL << IRDA_CONFIG_UART_STOP_LEN_POSI)
#define IRDA_CONFIG_UART_STOP_LEN_1_BIT     (0x00UL << IRDA_CONFIG_UART_STOP_LEN_POSI)
#define IRDA_CONFIG_UART_STOP_LEN_2_BIT     (0x01UL << IRDA_CONFIG_UART_STOP_LEN_POSI)

#define IRDA_CONFIG_UART_CLK_DIVID_POSI     8
#define IRDA_CONFIG_UART_CLK_DIVID_MASK     (0x0000FFFFUL << IRDA_CONFIG_UART_CLK_DIVID_POSI)

#define SFT_IRDA_CONFIG_FIFO_FULL           24
#define MSK_IRDA_CONFIG_FIFO_FULL           (0x1UL << SFT_IRDA_CONFIG_FIFO_FULL)
#define SFT_IRDA_CONFIG_FIFO_EMPTY          25
#define MSK_IRDA_CONFIG_FIFO_EMPTY          (0x1UL << SFT_IRDA_CONFIG_FIFO_EMPTY)


#define REG_IRDA_INT_ENABLE_ADDR            (REG_IRDA_BASE_ADDR + 1 * 4)
#define REG_IRDA_INT_ENABLE_MASK            0x3FUL
#define REG_IRDA_INT_ENABLE                 (*((volatile uint32_t *) REG_IRDA_INT_ENABLE_ADDR))

#define IRDA_INT_ENABLE_END_POSI            0
#define IRDA_INT_ENABLE_END_MASK            (0x01UL << IRDA_INT_ENABLE_END_POSI)
#define IRDA_INT_ENABLE_END_CLEAR           (0x00UL << IRDA_INT_ENABLE_END_POSI)
#define IRDA_INT_ENABLE_END_SET             (0x01UL << IRDA_INT_ENABLE_END_POSI)

#define IRDA_INT_ENABLE_RIGHT_POSI          1
#define IRDA_INT_ENABLE_RIGHT_MASK          (0x01UL << IRDA_INT_ENABLE_RIGHT_POSI)
#define IRDA_INT_ENABLE_RIGHT_CLEAR         (0x00UL << IRDA_INT_ENABLE_RIGHT_POSI)
#define IRDA_INT_ENABLE_RIGHT_SET           (0x01UL << IRDA_INT_ENABLE_RIGHT_POSI)

#define IRDA_INT_ENABLE_REPEAT_POSI         2
#define IRDA_INT_ENABLE_REPEAT_MASK         (0x01UL << IRDA_INT_ENABLE_REPEAT_POSI)
#define IRDA_INT_ENABLE_REPEAT_CLEAR        (0x00UL << IRDA_INT_ENABLE_REPEAT_POSI)
#define IRDA_INT_ENABLE_REPEAT_SET          (0x01UL << IRDA_INT_ENABLE_REPEAT_POSI)

#define IRDA_INT_ENABLE_FIFO_WR_EN_POSI     3
#define IRDA_INT_ENABLE_FIFO_WR_EN_MASK     (0x01UL << IRDA_INT_ENABLE_FIFO_WR_EN_POSI)
#define IRDA_INT_ENABLE_FIFO_WR_EN_CLEAR    (0x00UL << IRDA_INT_ENABLE_FIFO_WR_EN_POSI)
#define IRDA_INT_ENABLE_FIFO_WR_EN_SET      (0x01UL << IRDA_INT_ENABLE_FIFO_WR_EN_POSI)

#define IRDA_INT_ENABLE_PARITY_ERR_POSI     4
#define IRDA_INT_ENABLE_PARITY_ERR_MASK     (0x01UL << IRDA_INT_ENABLE_PARITY_ERR_POSI)
#define IRDA_INT_ENABLE_PARITY_ERR_CLEAR    (0x00UL << IRDA_INT_ENABLE_PARITY_ERR_POSI)
#define IRDA_INT_ENABLE_PARITY_ERR_SET      (0x01UL << IRDA_INT_ENABLE_PARITY_ERR_POSI)

#define IRDA_INT_ENABLE_STOP_ERR_POSI       5
#define IRDA_INT_ENABLE_STOP_ERR_MASK       (0x01UL << IRDA_INT_ENABLE_STOP_ERR_POSI)
#define IRDA_INT_ENABLE_STOP_ERR_CLEAR      (0x00UL << IRDA_INT_ENABLE_STOP_ERR_POSI)
#define IRDA_INT_ENABLE_STOP_ERR_SET        (0x01UL << IRDA_INT_ENABLE_STOP_ERR_POSI)


#define REG_IRDA_INT_STATUS_ADDR            (REG_IRDA_BASE_ADDR + 2 * 4)
#define REG_IRDA_INT_STATUS_MASK            0x3FUL
#define REG_IRDA_INT_STATUS                 (*((volatile uint32_t *) REG_IRDA_INT_STATUS_ADDR))


#define REG_IRDA_INT_FIFO_DATA_OUT_ADDR     (REG_IRDA_BASE_ADDR + 3 * 4)
#define REG_IRDA_INT_FIFO_DATA_OUT_MASK     0xFFFFFFFF
#define REG_IRDA_INT_FIFO_DATA_OUT          (*((volatile uint32_t *) REG_IRDA_INT_FIFO_DATA_OUT_ADDR))

/* ------------------------------------------------------------------------- */
void irda_rx_init(void)
{
    system_peri_clk_enable(SYS_PERI_CLK_IRDA);
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_IRDA);

    gpio_config_new(GPIO2, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);
    REG_IRDA_CONFIG = 0;

    REG_IRDA_INT_STATUS = REG_IRDA_INT_STATUS_MASK;
    REG_IRDA_INT_ENABLE = IRDA_INT_ENABLE_END_SET
                        | IRDA_INT_ENABLE_RIGHT_SET
                        | IRDA_INT_ENABLE_REPEAT_SET;
    REG_IRDA_CONFIG = (0x01UL   << IRDA_CONFIG_ENABLE_POSI)
                    | (0x00UL   << IRDA_CONFIG_POLARITY_POSI)
                    | (0x00UL   << IRDA_CONFIG_UART_ENABLE_POSI)
                    | (0x03UL   << IRDA_CONFIG_UART_LENGTH_POSI)
                    | (0x00UL   << IRDA_CONFIG_UART_PAR_EN_POSI)
                    | (0x00UL   << IRDA_CONFIG_UART_PAR_MODE_POSI)
                    | (0x00UL   << IRDA_CONFIG_UART_STOP_LEN_POSI)
                    | (0x3921UL << IRDA_CONFIG_UART_CLK_DIVID_POSI);

    system_peri_mcu_irq_enable(SYS_PERI_IRQ_IRDA);
}

static uint32_t s_ir_rx_code = 0xFFFFFFFF;

/** @return rx data (litte endien) */
int irda_rx_code_get(uint8_t *buff, int size)
{
    if(size < 4) return -1;
    uint32_t code = s_ir_rx_code;
    buff[0] = code & 0xFF;
    buff[1] = (code >> 8) & 0xFF;
    buff[2] = (code >> 16) & 0xFF;
    buff[3] = (code >> 24) & 0xFF;
    return 0;
}

void irda_rx_isr(void)
{
    uint32_t status;
    static uint8_t valid_flag = 0;

    status = REG_IRDA_INT_STATUS;

    IRDA_LOG_D("irda_rx_isr\n");
    if (status & IRDA_INT_ENABLE_END_MASK)
    {
        uint32_t code = -1;
        if (valid_flag)
        {
            valid_flag = 0;
            int num = 0;
            while(!(REG_IRDA_CONFIG & MSK_IRDA_CONFIG_FIFO_EMPTY)){
                code = REG_IRDA_INT_FIFO_DATA_OUT;
                num++;
            }
            if(num > 1) { IRDA_LOG_E("ir_rx data 0x%x, num:%d\n", code, num); }
            IRDA_LOG_D("ir_rx data 0x%x, num:%d\n", code, num);
            s_ir_rx_code = code;
            irda_rx_evt_set_rx_cmp();
            // msg_put(MSG_IRDA_RX);
        }
        else
        {
            IRDA_LOG_E("ir_rx err\n");
        }
    }

    if (status & IRDA_INT_ENABLE_RIGHT_MASK)
    {
        valid_flag = 0x01;
    }

    if (status & IRDA_INT_ENABLE_REPEAT_MASK)
    {
        IRDA_LOG_D("ir_rx repeat\n");
        irda_rx_evt_set_repete();
        // msg_put(MSG_IRDA_RX);
    }

    REG_IRDA_INT_STATUS = status;
}

#endif /* CONFIG_DRV_IR_RX */
