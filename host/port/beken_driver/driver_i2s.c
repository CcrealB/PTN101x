#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "bkreg.h"

#if(CONFIG_DRIVER_I2S)

#define I2S_DBG_ENABLE                      (0)
#if     I2S_DBG_ENABLE
#define I2S_DBG(fmt, ...)                   bk_printf("[I2S]: "fmt, ##__VA_ARGS__)
int                                         bk_printf(const char *fmt, ...);
#else
#define I2S_DBG(fmt, ...)
#endif

#define I2S_MCLK_11p2896M                   (11289600 * 8)
#define I2S_MCLK_12p2880M                   (12288000 * 8)

#define I2S_RX_FIFO_CAPACITY                (128)
#define I2S_TX_FIFO_CAPACITY                (128)

#define SFT_I2S_CTRL_BIT_RATIO              (0)
#define SFT_I2S_CTRL_SMP_RATIO              (8)
#define SFT_I2S_CTRL_DLEN                   (13)
#define SFT_I2S_CTRL_BIT_WIDTH              (16)
#define SFT_I2S_CTRL_SLEN                   (21)
#define SFT_I2S_CTRL_LSB_FIRST              (24)
#define SFT_I2S_CTRL_BIT_CLK_INV            (25)
#define SFT_I2S_CTRL_LR_CLK_INV             (26)
#define SFT_I2S_CTRL_MODE                   (27)
#define SFT_I2S_CTRL_MASTER_EN              (30)
#define SFT_I2S_CTRL_EN                     (31)
#define MSK_I2S_CTRL_BIT_RATIO              (0xFF << SFT_I2S_CTRL_BIT_RATIO  )
#define MSK_I2S_CTRL_SMP_RATIO              (0x1F << SFT_I2S_CTRL_SMP_RATIO  )
#define MSK_I2S_CTRL_DLEN                   (0x07 << SFT_I2S_CTRL_DLEN       )
#define MSK_I2S_CTRL_BIT_WIDTH              (0x1F << SFT_I2S_CTRL_BIT_WIDTH  )
#define MSK_I2S_CTRL_SLEN                   (0x07 << SFT_I2S_CTRL_SLEN       )
#define MSK_I2S_CTRL_LSB_FIRST              (0x01 << SFT_I2S_CTRL_LSB_FIRST  )
#define MSK_I2S_CTRL_BIT_CLK_INV            (0x01 << SFT_I2S_CTRL_BIT_CLK_INV)
#define MSK_I2S_CTRL_LR_CLK_INV             (0x01 << SFT_I2S_CTRL_LR_CLK_INV )
#define MSK_I2S_CTRL_MODE                   (0x07 << SFT_I2S_CTRL_MODE       )
#define MSK_I2S_CTRL_MASTER_EN              (0x01 << SFT_I2S_CTRL_MASTER_EN  )
#define MSK_I2S_CTRL_EN                     (0x01 << SFT_I2S_CTRL_EN         )

#define SFT_I2S_CFG_RX_INT_EN               (0)
#define SFT_I2S_CFG_TX_INT_EN               (1)
#define SFT_I2S_CFG_RX_OVERFLOW_INT_EN      (2)
#define SFT_I2S_CFG_TX_EMPTY_INT_EN         (3)
#define SFT_I2S_CFG_RX_THRESHOLD            (4)
#define SFT_I2S_CFG_TX_THRESHOLD            (6)
#define SFT_I2S_CFG_RX_FIFO_CLEAR           (8)
#define SFT_I2S_CFG_TX_FIFO_CLEAR           (9)
#define SFT_I2S_CFG_SMP_RATIO_H2B           (10)
#define SFT_I2S_CFG_BIT_RATIO_H4B           (12)
#define SFT_I2S_CFG_LR_COM_STORE			(16)
#define SFT_I2S_CFG_PARALLEL_EN 			(17)
#define SFT_I2S_CFG_RX2_INT_EN              (0 )
#define SFT_I2S_CFG_TX2_INT_EN              (1 )
#define SFT_I2S_CFG_RX2_OVERFLOW_INT_EN     (2 )
#define SFT_I2S_CFG_TX2_EMPTY_INT_EN        (3 )
#define SFT_I2S_CFG_RX3_INT_EN              (4 )
#define SFT_I2S_CFG_TX3_INT_EN              (5 )
#define SFT_I2S_CFG_RX3_OVERFLOW_INT_EN     (6 )
#define SFT_I2S_CFG_TX3_EMPTY_INT_EN        (7 )
#define SFT_I2S_CFG_RX4_INT_EN              (8 )
#define SFT_I2S_CFG_TX4_INT_EN              (9 )
#define SFT_I2S_CFG_RX4_OVERFLOW_INT_EN     (10)
#define SFT_I2S_CFG_TX4_EMPTY_INT_EN        (11)
#define MSK_I2S_CFG_RX_INT_EN               (1 << SFT_I2S_CFG_RX_INT_EN          )
#define MSK_I2S_CFG_TX_INT_EN               (1 << SFT_I2S_CFG_TX_INT_EN          )
#define MSK_I2S_CFG_RX_OVERFLOW_INT_EN      (1 << SFT_I2S_CFG_RX_OVERFLOW_INT_EN )
#define MSK_I2S_CFG_TX_EMPTY_INT_EN         (1 << SFT_I2S_CFG_TX_EMPTY_INT_EN    )
#define MSK_I2S_CFG_RX_THRESHOLD            (3 << SFT_I2S_CFG_RX_THRESHOLD       )
#define MSK_I2S_CFG_TX_THRESHOLD            (3 << SFT_I2S_CFG_TX_THRESHOLD       )
#define MSK_I2S_CFG_RX_FIFO_CLEAR           (1 << SFT_I2S_CFG_RX_FIFO_CLEAR      )
#define MSK_I2S_CFG_TX_FIFO_CLEAR           (1 << SFT_I2S_CFG_TX_FIFO_CLEAR      )
#define MSK_I2S_CFG_SMP_RATIO_H2B           (3 << SFT_I2S_CFG_SMP_RATIO_H2B      )
#define MSK_I2S_CFG_BIT_RATIO_H4B           (15 << SFT_I2S_CFG_BIT_RATIO_H4B     )
#define MSK_I2S_CFG_LR_COM_STORE			(1 << SFT_I2S_CFG_LR_COM_STORE       )
#define MSK_I2S_CFG_PARALLEL_EN			    (1 << SFT_I2S_CFG_PARALLEL_EN        )
#define MSK_I2S_CFG_RX1_INT_EN              (1 << SFT_I2S_CFG_RX1_INT_EN         )
#define MSK_I2S_CFG_TX1_INT_EN              (1 << SFT_I2S_CFG_TX1_INT_EN         )
#define MSK_I2S_CFG_RX1_OVERFLOW_INT_EN     (1 << SFT_I2S_CFG_RX1_OVERFLOW_INT_EN)
#define MSK_I2S_CFG_TX1_EMPTY_INT_EN        (1 << SFT_I2S_CFG_TX1_EMPTY_INT_EN   )
#define MSK_I2S_CFG_RX2_INT_EN              (1 << SFT_I2S_CFG_RX2_INT_EN         )
#define MSK_I2S_CFG_TX2_INT_EN              (1 << SFT_I2S_CFG_TX2_INT_EN         )
#define MSK_I2S_CFG_RX2_OVERFLOW_INT_EN     (1 << SFT_I2S_CFG_RX2_OVERFLOW_INT_EN)
#define MSK_I2S_CFG_TX2_EMPTY_INT_EN        (1 << SFT_I2S_CFG_TX2_EMPTY_INT_EN   )
#define MSK_I2S_CFG_RX3_INT_EN              (1 << SFT_I2S_CFG_RX3_INT_EN         )
#define MSK_I2S_CFG_TX3_INT_EN              (1 << SFT_I2S_CFG_TX3_INT_EN         )
#define MSK_I2S_CFG_RX3_OVERFLOW_INT_EN     (1 << SFT_I2S_CFG_RX3_OVERFLOW_INT_EN)
#define MSK_I2S_CFG_TX3_EMPTY_INT_EN        (1 << SFT_I2S_CFG_TX3_EMPTY_INT_EN   )

#define SFT_I2S_STATUS_RX_INT               (0)
#define SFT_I2S_STATUS_TX_INT               (1)
#define SFT_I2S_STATUS_RX_OVERFLOW_INT      (2)
#define SFT_I2S_STATUS_TX_EMPTY_INT         (3)
#define SFT_I2S_STATUS_RX_FIFO_READY        (4)
#define SFT_I2S_STATUS_TX_FIFO_READY        (5)
#define SFT_I2S_STATUS_RX2_INT              (0 )
#define SFT_I2S_STATUS_TX2_INT              (1 )
#define SFT_I2S_STATUS_RX2_OVERFLOW_INT     (2 )
#define SFT_I2S_STATUS_TX2_EMPTY_INT        (3 )
#define SFT_I2S_STATUS_RX3_INT              (4 )
#define SFT_I2S_STATUS_TX3_INT              (5 )
#define SFT_I2S_STATUS_RX3_OVERFLOW_INT     (6 )
#define SFT_I2S_STATUS_TX3_EMPTY_INT        (7 )
#define SFT_I2S_STATUS_RX4_INT              (8 )
#define SFT_I2S_STATUS_TX4_INT              (9 )
#define SFT_I2S_STATUS_RX4_OVERFLOW_INT     (10)
#define SFT_I2S_STATUS_TX4_EMPTY_INT        (11)
#define MSK_I2S_STATUS_RX_INT               (1 << SFT_I2S_STATUS_RX_INT          )
#define MSK_I2S_STATUS_TX_INT               (1 << SFT_I2S_STATUS_TX_INT          )
#define MSK_I2S_STATUS_RX_OVERFLOW_INT      (1 << SFT_I2S_STATUS_RX_OVERFLOW_INT )
#define MSK_I2S_STATUS_TX_EMPTY_INT         (1 << SFT_I2S_STATUS_TX_EMPTY_INT    )
#define MSK_I2S_STATUS_RX_FIFO_READY        (1 << SFT_I2S_STATUS_RX_FIFO_READY   )
#define MSK_I2S_STATUS_TX_FIFO_READY        (1 << SFT_I2S_STATUS_TX_FIFO_READY   )
#define MSK_I2S_STATUS_RX1_INT              (1 << SFT_I2S_STATUS_RX1_INT         )
#define MSK_I2S_STATUS_TX1_INT              (1 << SFT_I2S_STATUS_TX1_INT         )
#define MSK_I2S_STATUS_RX1_OVERFLOW_INT     (1 << SFT_I2S_STATUS_RX1_OVERFLOW_INT)
#define MSK_I2S_STATUS_TX1_EMPTY_INT        (1 << SFT_I2S_STATUS_TX1_EMPTY_INT   )
#define MSK_I2S_STATUS_RX2_INT              (1 << SFT_I2S_STATUS_RX2_INT         )
#define MSK_I2S_STATUS_TX2_INT              (1 << SFT_I2S_STATUS_TX2_INT         )
#define MSK_I2S_STATUS_RX2_OVERFLOW_INT     (1 << SFT_I2S_STATUS_RX2_OVERFLOW_INT)
#define MSK_I2S_STATUS_TX2_EMPTY_INT        (1 << SFT_I2S_STATUS_TX2_EMPTY_INT   )
#define MSK_I2S_STATUS_RX3_INT              (1 << SFT_I2S_STATUS_RX3_INT         )
#define MSK_I2S_STATUS_TX3_INT              (1 << SFT_I2S_STATUS_TX3_INT         )
#define MSK_I2S_STATUS_RX3_OVERFLOW_INT     (1 << SFT_I2S_STATUS_RX3_OVERFLOW_INT)
#define MSK_I2S_STATUS_TX3_EMPTY_INT        (1 << SFT_I2S_STATUS_TX3_EMPTY_INT   )


//io
#define IO_I2S_MCLK     GPIO24

#define IO_I2S0_BCLK    GPIO18
#define IO_I2S0_SCLK    GPIO19
#define IO_I2S0_DIN     GPIO20
#define IO_I2S0_DOUT    GPIO21
#define IO_I2S0_DOUT2    GPIO22
#define IO_I2S0_DOUT3    GPIO23

#define IO_I2S1_BCLK    GPIO26
#define IO_I2S1_SCLK    GPIO27
#define IO_I2S1_DIN     GPIO28
#define IO_I2S1_DOUT    GPIO29

#define IO_I2S2_BCLK    GPIO30
#define IO_I2S2_SCLK    GPIO31
#define IO_I2S2_DIN     GPIO32
#define IO_I2S2_DOUT    GPIO33

#define I2S_TX_INT_LEVEL        8 //(1, 8, 16, 24), tx int occur when fifo data num less than 8
#define I2S_RX_INT_LEVEL        16 //(1, 8, 16, 24), rx int occur when fifo data num more than 16

#ifndef u_gpio_config
#if   defined(__BA2__)
#define u_gpio_config(io, dir, pull, mode)      gpio_config_new(io, dir, pull, mode)
#elif defined(CEVAX2)
#define u_gpio_config(io, dir, pull, mode)      gpio_config(io, dir, pull, mode)
#endif
#endif

typedef struct _I2SContext
{
    volatile uint32_t ctrl;
    volatile uint32_t config;
    volatile uint32_t status;
    volatile uint32_t data;
    volatile uint32_t cfg2;
    volatile uint32_t stat2;
    volatile uint32_t dat1;
    volatile uint32_t dat2;
    volatile uint32_t dat3;
}I2SContext;


uint8 i2s_flag = 0;

void hal_i2s_init(I2S_CFG_t *i2sCfg, uint8_t enable)
{
    // GPIO_DIR clk_io_dir = (i2sCfg->role == I2S_ROLE_MASTER) GPIO_OUTPUT : GPIO_INPUT;
    switch(i2sCfg->I2Sx)
    {
    case I2S0:
        if(enable)
        {
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S0_BCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S0_SCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S0_DIN,  GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S0_DOUT, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->dout2_valid) u_gpio_config(IO_I2S0_DOUT2, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC2);
            if(i2sCfg->dout3_valid) u_gpio_config(IO_I2S0_DOUT3, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC2);
            system_mem_clk_enable(SYS_MEM_CLK_I2S0);
            system_peri_clk_enable(SYS_PERI_CLK_I2S0);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S0);
            if(i2sCfg->clk_gate_dis)  system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_I2S0);
        }
        else
        {
            system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_I2S0);
            system_peri_mcu_irq_disable(SYS_PERI_IRQ_I2S0);
            system_peri_clk_disable(SYS_PERI_CLK_I2S0);
            system_mem_clk_disable(SYS_MEM_CLK_I2S0);
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S0_BCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S0_SCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S0_DIN,  GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S0_DOUT, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->dout2_valid) u_gpio_config(IO_I2S0_DOUT2, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->dout3_valid) u_gpio_config(IO_I2S0_DOUT3, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
        }
        break;

    case I2S1:
        if(enable)
        {
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S1_BCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S1_SCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S1_DIN,  GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S1_DOUT, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            system_mem_clk_enable(SYS_MEM_CLK_I2S1);
            system_peri_clk_enable(SYS_PERI_CLK_I2S1);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S1);
            if(i2sCfg->clk_gate_dis)  system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_I2S1);
        }
        else
        {
            system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_I2S1);
            system_peri_mcu_irq_disable(SYS_PERI_IRQ_I2S1);
            system_peri_clk_disable(SYS_PERI_CLK_I2S1);
            system_mem_clk_disable(SYS_MEM_CLK_I2S1);
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S1_BCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S1_SCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S1_DIN,  GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S1_DOUT, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
        }
        break;
            
    case I2S2:
        if(enable)
        {
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S2_BCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S2_SCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S2_DIN,  GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S2_DOUT, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            system_mem_clk_enable(SYS_MEM_CLK_I2S2);
            system_peri_clk_enable(SYS_PERI_CLK_I2S2);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S2);
            if(i2sCfg->clk_gate_dis)  system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_I2S2);
        }
        else
        {
            system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_I2S2);
            system_peri_mcu_irq_disable(SYS_PERI_IRQ_I2S2);
            system_peri_clk_disable(SYS_PERI_CLK_I2S2);
            system_mem_clk_disable(SYS_MEM_CLK_I2S2);
            if(i2sCfg->bck_valid)   u_gpio_config(IO_I2S2_BCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->lrck_valid)  u_gpio_config(IO_I2S2_SCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->din_valid)   u_gpio_config(IO_I2S2_DIN,  GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            if(i2sCfg->dout_valid)  u_gpio_config(IO_I2S2_DOUT, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
        }
        break;
        default: break;
    }

    if(enable)
    {
        uint8_t ret = I2S_ERROR_CODE_OK;

        ret = i2s_init(i2sCfg->I2Sx, i2sCfg->role, i2sCfg->sample_rate, i2sCfg->datawith, 32);//datawidth = 16/24bit, bus width=32bit
        if(ret != I2S_ERROR_CODE_OK){
            LOG_E(I2S, "I2S%d init error %s():%d\n", i2sCfg->I2Sx, __FUNCTION__, ret);
        }

        i2s_ctrl(i2sCfg->I2Sx, I2S_CTRL_CMD_LR_COM_STORE, 0);
        //LRCK INV->(def 0：high_R--low_L--high_R--low_L...)/(1：low_R--high_L--low_R--high_L...)
        i2s_ctrl(i2sCfg->I2Sx, I2S_CTRL_CMD_SET_LR_CLK_INV, i2sCfg->lrck_inv_en);
        // i2s_ctrl(i2sCfg->I2Sx, I2S_CTRL_CMD_SET_RX_THRESHOLD, I2S_RX_INT_LEVEL >> 3);//0:1, 1:8, 2:16, 3:24
        // i2s_ctrl(i2sCfg->I2Sx, I2S_CTRL_CMD_SET_TX_THRESHOLD, I2S_TX_INT_LEVEL >> 3);//0:1, 1:8, 2:16, 3:24
    }
}

int32_t i2s_init(I2S i, uint32_t role, uint32_t sample_rate, uint32_t sample_width, uint32_t sample_ratio)
{
    uint32_t bit_ratio = 0;
    volatile I2SContext* i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + i * 0x8000);

    //ASSERT(sample_ratio >= sample_width);
    
    REG_SYSTEM_0x01 &= ~(1 << 22);
#if 0
    switch(i)
    {
        case I2S0:
            gpio_config_new(GPIO18, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO19, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO20, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO21, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);      //I2S1_DOUT1
            //gpio_config_new(GPIO22, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC2);    //I2S1_DOUT2
            //gpio_config_new(GPIO23, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC2);    //I2S1_DOUT3
            system_mem_clk_enable(SYS_MEM_CLK_I2S0);
            system_peri_clk_enable(SYS_PERI_CLK_I2S0);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S0);
            break;
            
        case I2S1:
            gpio_config_new(GPIO26, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO27, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO28, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO29, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            system_mem_clk_enable(SYS_MEM_CLK_I2S1);
            system_peri_clk_enable(SYS_PERI_CLK_I2S1);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S1);
            break;
            
        case I2S2:
            gpio_config_new(GPIO30, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO31, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO32, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            gpio_config_new(GPIO33, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
            system_mem_clk_enable(SYS_MEM_CLK_I2S2);
            system_peri_clk_enable(SYS_PERI_CLK_I2S2);
            system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S2);
            break;
            
        default:
            return I2S_ERROR_CODE_INVALID_PARAMETER;
    }
#endif
    switch(sample_rate)
    {
        case 8000:
        case 16000:
        case 32000:
        case 64000:
        case 128000:
        case 256000:
        case 12000:
        case 24000:
        case 48000:
        case 96000:
        case 192000:
        case 384000:
        	bit_ratio = I2S_MCLK_12p2880M / (4 * sample_rate * sample_ratio);
            break;
            
        case 11025:
        case 22050:
        case 44100:
        case 88200:
        case 176400:
        case 352800:
        	bit_ratio = I2S_MCLK_11p2896M / (4 * sample_rate * sample_ratio);
            break;
            
        default:
            return I2S_ERROR_CODE_INVALID_PARAMETER;
    }

    i2s->ctrl = ((bit_ratio << SFT_I2S_CTRL_BIT_RATIO) & MSK_I2S_CTRL_BIT_RATIO) |
                (((sample_ratio - 1) << SFT_I2S_CTRL_SMP_RATIO) & MSK_I2S_CTRL_SMP_RATIO) |
                ((sample_width - 1) << SFT_I2S_CTRL_BIT_WIDTH) |
                //(0x01 << SFT_I2S_CTRL_LR_CLK_INV ) |
                (I2S_MODE_PHILIP << SFT_I2S_CTRL_MODE) |
                ((I2S_ROLE_MASTER == role) << SFT_I2S_CTRL_MASTER_EN);

    i2s->config = (2 << SFT_I2S_CFG_RX_THRESHOLD) |
                  (2 << SFT_I2S_CFG_TX_THRESHOLD) |
                  (1 << SFT_I2S_CFG_RX_FIFO_CLEAR)|
                  (1 << SFT_I2S_CFG_TX_FIFO_CLEAR)|
                  (1 << SFT_I2S_CFG_PARALLEL_EN)  |
                  ((bit_ratio >> 8) << SFT_I2S_CFG_BIT_RATIO_H4B) |
                  (((sample_ratio - 1) >> 5) << SFT_I2S_CFG_SMP_RATIO_H2B);

    i2s->cfg2   = 0x00000000;
    i2s->status = 0x0000000C;
    i2s->stat2  = 0x00000CCC;

    return I2S_ERROR_CODE_OK;
}

int32_t i2s_ctrl(I2S i, uint32_t cmd, uint32_t arg)
{
    uint32_t reg;
    volatile I2SContext* i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + i * 0x8000);

    switch(cmd)
    {
        case I2S_CTRL_CMD_SET_SMP_RATIO:
            {
                uint32_t br, sr, mclk;
                reg  = i2s->ctrl;
                br   = (reg & MSK_I2S_CTRL_BIT_RATIO) >> SFT_I2S_CTRL_BIT_RATIO;
                sr   = (reg & MSK_I2S_CTRL_SMP_RATIO) >> SFT_I2S_CTRL_SMP_RATIO;
                mclk = (br + 1) * (sr + 1);
                reg &= ~(MSK_I2S_CTRL_BIT_RATIO | MSK_I2S_CTRL_SMP_RATIO);
                sr   = arg - 1;
                br   = mclk / (sr + 1) - 1;
                reg |= (br << SFT_I2S_CTRL_BIT_RATIO) | (sr << SFT_I2S_CTRL_SMP_RATIO);
                i2s->ctrl = reg;
                reg  = i2s->config;
                reg &= ~(MSK_I2S_CFG_BIT_RATIO_H4B | MSK_I2S_CFG_SMP_RATIO_H2B);
                reg |= ((br >> 8) << SFT_I2S_CFG_BIT_RATIO_H4B) | ((sr >> 5) << SFT_I2S_CFG_SMP_RATIO_H2B);
                i2s->config = reg;
            }
            break;
            
        case I2S_CTRL_CMD_SET_DLEN:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_DLEN;
            reg |= (arg << SFT_I2S_CTRL_DLEN) & MSK_I2S_CTRL_DLEN;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_BIT_WIDTH:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_BIT_WIDTH;
            reg |= (arg << SFT_I2S_CTRL_BIT_WIDTH) & MSK_I2S_CTRL_BIT_WIDTH;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_SLEN:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_SLEN;
            reg |= (arg << SFT_I2S_CTRL_SLEN) & MSK_I2S_CTRL_SLEN;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_LSB_FIRST:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_LSB_FIRST;
            reg |= (arg << SFT_I2S_CTRL_LSB_FIRST) & MSK_I2S_CTRL_LSB_FIRST;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_BIT_CLK_INV:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_BIT_CLK_INV;
            reg |= (arg << SFT_I2S_CTRL_BIT_CLK_INV) & MSK_I2S_CTRL_BIT_CLK_INV;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_LR_CLK_INV:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_LR_CLK_INV;
            reg |= (arg << SFT_I2S_CTRL_LR_CLK_INV) & MSK_I2S_CTRL_LR_CLK_INV;
            i2s->ctrl = reg;
            break;
            
        case I2S_CTRL_CMD_SET_MODE:
            reg  = i2s->ctrl;
            reg &= ~MSK_I2S_CTRL_MODE;
            reg |= (arg << SFT_I2S_CTRL_MODE) & MSK_I2S_CTRL_MODE;
            i2s->ctrl = reg;
            break;

        case I2S_CTRL_CMD_CLR_RX_FIFO:
            i2s->config |= MSK_I2S_CFG_RX_FIFO_CLEAR;
            break;

        case I2S_CTRL_CMD_CLR_TX_FIFO:
            i2s->config |= MSK_I2S_CFG_TX_FIFO_CLEAR;
            break;

        case I2S_CTRL_CMD_SET_RX_THRESHOLD:
            reg  = i2s->config;
            reg &= ~MSK_I2S_CFG_RX_THRESHOLD;
            reg |= (arg << SFT_I2S_CFG_RX_THRESHOLD) & MSK_I2S_CFG_RX_THRESHOLD;
            i2s->config = reg;
            break;
            
        case I2S_CTRL_CMD_SET_TX_THRESHOLD:
            reg  = i2s->config;
            reg &= ~MSK_I2S_CFG_TX_THRESHOLD;
            reg |= (arg << SFT_I2S_CFG_TX_THRESHOLD) & MSK_I2S_CFG_TX_THRESHOLD;
            i2s->config = reg;
            break;
            
        case I2S_CTRL_CMD_RX_TRIG:
            i2s->data = 0;
            i2s->data = 0;
            i2s->dat1 = 0;
            i2s->dat1 = 0;
        	i2s->dat2 = 0;
        	i2s->dat2 = 0;
        	i2s->dat3 = 0;
        	i2s->dat3 = 0;
        	break;
            
        case I2S_CTRL_CMD_LR_COM_STORE:
        	reg  = i2s->config;
        	reg &= ~MSK_I2S_CFG_LR_COM_STORE;
    		reg |= (!!arg) << SFT_I2S_CFG_LR_COM_STORE;
        	i2s->config = reg;
        	break;
            
        case I2S_CTRL_CMD_PARALLEL_EN:
            reg  = i2s->config;
            reg &= ~MSK_I2S_CFG_PARALLEL_EN;
            reg |= (!!arg) << SFT_I2S_CFG_PARALLEL_EN;
            i2s->config = reg;
            break;
            
        default:
            return I2S_ERROR_CODE_INVALID_PARAMETER;
    }

    return I2S_ERROR_CODE_OK;
}

void i2s_enable(I2S i, uint32_t enable)
{
    volatile I2SContext* i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + i * 0x8000);
    uint32_t reg = i2s->ctrl;

    reg &= ~MSK_I2S_CTRL_EN;
    reg |= (!!enable) << SFT_I2S_CTRL_EN;

    i2s->ctrl = reg;
    
    i2s_flag = (enable ? 1 : 0);
    
    // LOG_I(I2S, "I2S%x enable:%x\r\n", i, enable);
}

uint8_t i2s_is_enable(void)
{
    return i2s_flag;
}

#if I2S_INT_CODE_EN
void i2s_rx_int_enbale(I2S i, uint32_t enable)
{
    volatile I2SContext* i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + i * 0x8000);
    uint32_t reg = i2s->config;

    reg &= ~MSK_I2S_CFG_RX_INT_EN;
    reg |= (!!enable) << SFT_I2S_CFG_RX_INT_EN;
    reg |= (!!enable) << SFT_I2S_CFG_RX_OVERFLOW_INT_EN;

    i2s->config = reg;
}

void i2s_tx_int_enbale(I2S i, uint32_t enable)
{
    volatile I2SContext* i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + i * 0x8000);
    uint32_t reg = i2s->config;

    reg &= ~MSK_I2S_CFG_TX_INT_EN;
    reg |= (!!enable) << SFT_I2S_CFG_TX_INT_EN;
    reg |= (!!enable) << SFT_I2S_CFG_TX_EMPTY_INT_EN;

    i2s->config = reg;
}

static const unsigned char test_sin_data[176] =
{
    0x00, 0x00, 0x00, 0x00, 0x2D, 0x12, 0x2D, 0x12, 0xFC, 0x23, 0xFB, 0x23, 0x0E, 0x35, 0x10, 0x35, 0x0F, 0x45, 0x10, 0x45, 0xAA, 0x53, 0xA9, 0x53, 0x91, 0x60, 0x92, 0x60, 0x86, 0x6B, 0x84, 0x6B, 
    0x4C, 0x74, 0x4A, 0x74, 0xB4, 0x7A, 0xB5, 0x7A, 0xA1, 0x7E, 0xA1, 0x7E, 0xFE, 0x7F, 0xFE, 0x7F, 0xC3, 0x7E, 0xC3, 0x7E, 0xF6, 0x7A, 0xF6, 0x7A, 0xAC, 0x74, 0xAB, 0x74, 0x03, 0x6C, 0x03, 0x6C, 
    0x2B, 0x61, 0x2B, 0x61, 0x5A, 0x54, 0x5A, 0x54, 0xD3, 0x45, 0xD4, 0x45, 0xE4, 0x35, 0xE4, 0x35, 0xDB, 0x24, 0xDB, 0x24, 0x14, 0x13, 0x13, 0x13, 0xE9, 0x00, 0xE9, 0x00, 0xBA, 0xEE, 0xBA, 0xEE, 
    0xE6, 0xDC, 0xE4, 0xDC, 0xC7, 0xCB, 0xC6, 0xCB, 0xB6, 0xBB, 0xB6, 0xBB, 0x08, 0xAD, 0x07, 0xAD, 0x07, 0xA0, 0x09, 0xA0, 0xFA, 0x94, 0xFB, 0x94, 0x17, 0x8C, 0x18, 0x8C, 0x8E, 0x85, 0x8E, 0x85, 
    0x80, 0x81, 0x80, 0x81, 0x03, 0x80, 0x03, 0x80, 0x1E, 0x81, 0x1D, 0x81, 0xCA, 0x84, 0xC9, 0x84, 0xF5, 0x8A, 0xF5, 0x8A, 0x80, 0x93, 0x80, 0x93, 0x3F, 0x9E, 0x3E, 0x9E, 0xF8, 0xAA, 0xF8, 0xAA, 
    0x69, 0xB9, 0x6A, 0xB9, 0x49, 0xC9, 0x4B, 0xC9, 0x46, 0xDA, 0x45, 0xDA, 0x06, 0xEC, 0x05, 0xEC, 
};

void i2s0_isr(void)
{
	unsigned int		i;
	volatile unsigned int data_num;
    volatile I2SContext* i2s;
    static uint8 data_cnt = 0;
    uint32_t txint_level;
    uint32_t rxint_level;
    
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + I2S0 * 0x8000);
    
    //LOG_I(I2S, "I2S0 status:%x\r\n", i2s->status);
    
	if (i2s->status & MSK_I2S_STATUS_TX_INT)  //Tx interrupt
	{
	    txint_level = ((i2s->config & MSK_I2S_CFG_TX_THRESHOLD) >> SFT_I2S_CFG_TX_THRESHOLD);
		switch (txint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num; i++)
		{
			i2s->data = *((uint16*)&test_sin_data[data_cnt]);
            data_cnt += 2;
            if(data_cnt >= (sizeof(test_sin_data)/sizeof(test_sin_data[0])))
                data_cnt = 0;
		}
		i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}

	if (i2s->status & MSK_I2S_STATUS_RX_INT)  //Rx interrupt
	{
	    uint16 rx_data[32];

        rxint_level = ((i2s->config & MSK_I2S_CFG_RX_THRESHOLD) >> SFT_I2S_CFG_RX_THRESHOLD);
		switch (rxint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num/2; i++)
		{
			rx_data[i] = i2s->data;
            rx_data[i] = i2s->data;
		}
        adio_sco_fill_buffer((uint8 *)&rx_data[0], 0, data_num);
	}
    
	if (i2s->status & MSK_I2S_STATUS_TX_EMPTY_INT)     //Tx empty error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}
    
	if (i2s->status & MSK_I2S_STATUS_RX_OVERFLOW_INT)  //Rx overflow error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_RX_OVERFLOW_INT);
	}
}

void i2s1_isr(void)
{
	unsigned int		i;
	volatile unsigned int data_num;
    volatile I2SContext* i2s;
    static uint8 data_cnt = 0;
    uint32_t txint_level;
    uint32_t rxint_level;
    
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + I2S1 * 0x8000);
    
    //LOG_I(I2S, "I2S1 status:%x\r\n", i2s->status);
    
	if (i2s->status & MSK_I2S_STATUS_TX_INT)  //Tx interrupt
	{
	    txint_level = ((i2s->config & MSK_I2S_CFG_TX_THRESHOLD) >> SFT_I2S_CFG_TX_THRESHOLD);
		switch (txint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num; i++)
		{
			i2s->data = *((uint16*)&test_sin_data[data_cnt]);
            data_cnt += 2;
            if(data_cnt >= (sizeof(test_sin_data)/sizeof(test_sin_data[0])))
                data_cnt = 0;
		}
		i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}

	if (i2s->status & MSK_I2S_STATUS_RX_INT)  //Rx interrupt
	{
	    uint16 rx_data[32];

        rxint_level = ((i2s->config & MSK_I2S_CFG_RX_THRESHOLD) >> SFT_I2S_CFG_RX_THRESHOLD);
		switch (rxint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num/2; i++)
		{
			rx_data[i] = i2s->data;
            rx_data[i] = i2s->data;
		}
        adio_sco_fill_buffer((uint8 *)&rx_data[0], 0, data_num);
	}
    
	if (i2s->status & MSK_I2S_STATUS_TX_EMPTY_INT)     //Tx empty error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}
    
	if (i2s->status & MSK_I2S_STATUS_RX_OVERFLOW_INT)  //Rx overflow error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_RX_OVERFLOW_INT);
	}
}

void i2s2_isr(void)
{
	unsigned int		i;
	volatile unsigned int data_num;
    volatile I2SContext* i2s;
    static uint8 data_cnt = 0;
    uint32_t txint_level;
    uint32_t rxint_level;
    
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    i2s = (volatile I2SContext*)(MDU_I2S0_BASE_ADDR + I2S2 * 0x8000);
    
    //LOG_I(I2S, "I2S2 status:%x\r\n", i2s->status);
    
	if (i2s->status & MSK_I2S_STATUS_TX_INT)  //Tx interrupt
	{
	    txint_level = ((i2s->config & MSK_I2S_CFG_TX_THRESHOLD) >> SFT_I2S_CFG_TX_THRESHOLD);
		switch (txint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num; i++)
		{
			i2s->data = *((uint16*)&test_sin_data[data_cnt]);
            data_cnt += 2;
            if(data_cnt >= (sizeof(test_sin_data)/sizeof(test_sin_data[0])))
                data_cnt = 0;
		}
		i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}

	if (i2s->status & MSK_I2S_STATUS_RX_INT)  //Rx interrupt
	{
	    uint16 rx_data[32];

        rxint_level = ((i2s->config & MSK_I2S_CFG_RX_THRESHOLD) >> SFT_I2S_CFG_RX_THRESHOLD);
		switch (rxint_level)
		{
		    case 0	: 	data_num = 1;    break;
		    case 1	: 	data_num = 8;    break;
		    case 2	: 	data_num = 16;   break;
		    case 3	: 	data_num = 24;   break;
		    default	:  	data_num = 32;   break;
		}
        
		for (i = 0; i < data_num/2; i++)
		{
			rx_data[i] = i2s->data;
            rx_data[i] = i2s->data;
		}
        adio_sco_fill_buffer((uint8 *)&rx_data[0], 0, data_num);
	}
    
	if (i2s->status & MSK_I2S_STATUS_TX_EMPTY_INT)     //Tx empty error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_TX_EMPTY_INT);
	}
    
	if (i2s->status & MSK_I2S_STATUS_RX_OVERFLOW_INT)  //Rx overflow error
	{
        i2s->status |= (1 << SFT_I2S_STATUS_RX_OVERFLOW_INT);
	}
}
#endif //I2S_INT_CODE_EN

#endif
