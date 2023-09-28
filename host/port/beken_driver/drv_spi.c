/**
 **************************************************************************************
 * @file    driver_spi.c
 * @brief   Driver SPI
 *
 * @author  Jiankun.Liao
 * @version V1.0.2
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 * TODO 
   1.SPI Slave
 **************************************************************************************
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bkreg.h"
#include "drv_system.h"
#include "drv_spi.h"
#include "drv_mailbox.h"
#ifdef CEVAX2
#include "drv_gpio.h"
#include "drv_dma.h"
#include <vec-c.h>
#else
#include "driver_gpio.h"
#include "driver_dma.h"
#endif

#if (SPI_0_FUN1_ENABLE || SPI_1_FUN1_ENABLE || SPI_2_FUN1_ENABLE || SPI_2_FUN4_ENABLE)

#define GPIO_SPI0_1F_CSN_IO                     GPIO2
#define GPIO_SPI0_1F_SCK_IO                     GPIO3
#define GPIO_SPI0_1F_MOSI_IO                    GPIO4
#define GPIO_SPI0_1F_MISO_IO                    GPIO5
#define GPIO_SPI0_1F_CSN_CONFIG                 REG_GPIO_0x02
#define GPIO_SPI0_1F_SCK_CONFIG                 REG_GPIO_0x03
#define GPIO_SPI0_1F_MOSI_CONFIG                REG_GPIO_0x04
#define GPIO_SPI0_1F_MISO_CONFIG                REG_GPIO_0x05

#define GPIO_SPI1_1F_CSN_IO                     GPIO22
#define GPIO_SPI1_1F_SCK_IO                     GPIO23
#define GPIO_SPI1_1F_MOSI_IO                    GPIO24
#define GPIO_SPI1_1F_MISO_IO                    GPIO25
#define GPIO_SPI1_1F_CSN_CONFIG                 REG_GPIO_0x16
#define GPIO_SPI1_1F_SCK_CONFIG                 REG_GPIO_0x17
#define GPIO_SPI1_1F_MOSI_CONFIG                REG_GPIO_0x18
#define GPIO_SPI1_1F_MISO_CONFIG                REG_GPIO_0x19

#define GPIO_SPI2_1F_CSN_IO                     GPIO36
#define GPIO_SPI2_1F_SCK_IO                     GPIO37
#define GPIO_SPI2_1F_MOSI_IO                    GPIO38
#define GPIO_SPI2_1F_MISO_IO                    GPIO39
#define GPIO_SPI2_1F_CSN_CONFIG                 REG_GPIO_0x24
#define GPIO_SPI2_1F_SCK_CONFIG                 REG_GPIO_0x25
#define GPIO_SPI2_1F_MOSI_CONFIG                REG_GPIO_0x26
#define GPIO_SPI2_1F_MISO_CONFIG                REG_GPIO_0x27

#define GPIO_SPI2_4F_SCK_IO                     GPIO30
#define GPIO_SPI2_4F_CSN_IO                     GPIO31
#define GPIO_SPI2_4F_MISO_IO                    GPIO32
#define GPIO_SPI2_4F_MOSI_IO                    GPIO33
#define GPIO_SPI2_4F_SCK_CONFIG                 REG_GPIO_0x1E
#define GPIO_SPI2_4F_CSN_CONFIG                 REG_GPIO_0x1F
#define GPIO_SPI2_4F_MISO_CONFIG                REG_GPIO_0x20
#define GPIO_SPI2_4F_MOSI_CONFIG                REG_GPIO_0x21


#ifndef CEVAX2
    #define DSP_RAM_BASIC_ADDR          0

    #define REG_GET(addr)           (*(volatile unsigned int*)(addr))
    #define REG_SET(addr, val)      (*(volatile unsigned int*)(addr) = (val))
    /* #define REG_SET_AND(addr, val)  (*(volatile unsigned int*)(addr) &= (val))
     #define REG_SET_OR(addr, val)   (*(volatile unsigned int*)(addr) |= (val))
     #define REG_BITS_SET(addr,bit,msk,val)  do{\
         (*(volatile unsigned int*)(addr)) &= ((unsigned int)~(msk));\
         (*(volatile unsigned int*)(addr)) |= (((val) << (bit)) & (msk));}while(0)*/

    #define gpio_config_spi             gpio_config_new
    #define system_peri_xxx_irq_enable  system_peri_mcu_irq_enable
    #define gpio_output_set             gpio_output
#else
    #define DSP_RAM_BASIC_ADDR          (0x03000000)

    #define REG_GET(addr)           (_in(((unsigned int)addr)))
    #define REG_SET(addr,val)       (_out((unsigned int)(val), (unsigned int)(addr)))
    // #define REG_SET_AND(addr,val)   (_out(_in(((unsigned int)addr)) & (unsigned int)(val), (unsigned int)(addr)))
    // #define REG_SET_OR(addr,val)    (_out(_in(((unsigned int)addr)) | (unsigned int)(val),(unsigned int)(addr)))
    // #define REG_BITS_SET(addr,bit,msk,val)  (_out(_in(((unsigned int)addr)) & ((unsigned int)~(msk)) | (((val)<<(bit))&(msk)), (unsigned int)(addr)))

    #define gpio_config_spi             gpio_config
    #define system_peri_xxx_irq_enable  system_peri_dsp_irq_enable
#endif

#if     defined(__BA2__)
#define NOP()   __asm__("b.nop 5;")
#elif   defined(CEVAX2)
#define NOP()   __asm__("PCU.nop;")
#else
#define NOP()
#endif

#define SPI_REG_RW_DELAY        NOP();NOP();NOP();NOP();


#ifndef MIN
#define MIN(a,b)                                   ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a,b)                                   ((a) < (b) ? (b) : (a))
#endif

#define SPI_CLK_DIVID_SET(BaudRate)                 MAX((NUMBER_ROUND_UP(SPI_BAUD_26MHZ/2, BaudRate) - 1), 1)
#define NUMBER_ROUND_UP(a,b)                        ((a) / (b) + (((a) % (b)) ? 1 : 0))
#define NUMBER_ROUND_DOWN(a,b)                      ((a) / (b))

#define SPI_FIFO_DEPTH                              (64)
#define SPI_FIFO_DEPTH_BASE                         (16)

#define SPI_TRANS_DMA_MAX                           (4092)//(4095), set 4092 for 4 byte aligned



typedef enum{
    spi_idle            = 0,
    spi_writing         = 1,
    spi_reading         = 2,
    spi_write_read      = 3,
}spi_trans_state;

typedef union _spi_cfg
{
    struct{
        volatile uint32_t txfifo_int_level  :2;
        volatile uint32_t rxfifo_int_level  :2;
        volatile uint32_t txudf_en          :1;
        volatile uint32_t rxovf_en          :1;
        volatile uint32_t txfifo_int_en     :1;
        volatile uint32_t rxfifo_int_en     :1;
        volatile uint32_t spi_ckr           :8;
        volatile uint32_t slv_release_inten :1;
        volatile uint32_t wire3_en          :1;
        volatile uint32_t bit_wdth          :1;
        volatile uint32_t lsb_first         :1;
        volatile uint32_t ckpol             :1;
        volatile uint32_t ckpha             :1;
        volatile uint32_t msten             :1;
        volatile uint32_t spien             :1;
        volatile uint32_t byte_intlval      :6;
        volatile uint32_t reserved          :2;
    }reg;
    volatile uint32_t fill;
}spi_cfg;

typedef union _spi_cn
{
    struct{
        volatile uint32_t tx_en             :1;
        volatile uint32_t rx_en             :1;
        volatile uint32_t tx_finish_int_en  :1;
        volatile uint32_t rx_finish_int_en  :1;
        volatile uint32_t reserved          :4;
        volatile uint32_t tx_trans_len      :12;
        volatile uint32_t rx_trans_len      :12;
    }reg;
    volatile uint32_t fill;
}spi_cn;

typedef union _spi_stat
{
    struct{
        volatile uint32_t reserved1         :1;
        volatile uint32_t txfifo_wr_ready   :1;
        volatile uint32_t rxfifo_rd_ready   :1;
        volatile uint32_t reserved2         :1;
        volatile uint32_t reserved3         :4;
        volatile uint32_t txfifo_int        :1;
        volatile uint32_t rxfifo_int        :1;
        volatile uint32_t slv_release_int   :1;
        volatile uint32_t txudf             :1;
        volatile uint32_t rxovf             :1;
        volatile uint32_t tx_finish_int     :1;
        volatile uint32_t rx_finish_int     :1;
        volatile uint32_t reserved4         :1;
        volatile uint32_t txfifo_clr        :1;
        volatile uint32_t rxfifo_clr        :1;
        volatile uint32_t reserved5         :14;
    }reg;
    volatile uint32_t fill;
}spi_stat;

typedef union _spi_dat
{
    struct{
        volatile uint32_t spi_dat           :16;
        volatile uint32_t reserved          :16;
    }reg;
    volatile uint32_t fill;
}spi_dat;


typedef struct _spi_reg
{
    volatile spi_cfg    cfg;
    volatile spi_cn     cn;
    volatile spi_stat   stat;
    volatile spi_dat    dat;
}spi_reg_t;



#define spi_reg(x)                ((volatile spi_reg_t*)(MDU_SPI0_BASE_ADDR + 0x8000 * x))

typedef void (*hw_config_fxn_t)(void *obj);

typedef struct _spi_internal_object
{
    spi_object              ext_obj;
    
    volatile spi_transfer_object trans_obj;

    DMA_REQ                 dma_tx_req;
    DMA_REQ                 dma_rx_req;
    
    volatile spi_reg_t *    reg;
    void *                  tx_dma_handle;
    void *                  rx_dma_handle;
    void *                  arg;


    hw_config_fxn_t         hw_config_fxn;
    spi_trans_cb_fxn        trans_cb_fxn;
}spi_internal_object,*spi_internal_handle;



#if (SPI_0_FUN1_ENABLE == 1)
static void spi0_fun1_hw_cfg(void *obj);
#endif
#if (SPI_1_FUN1_ENABLE == 1)
static void spi1_fun1_hw_cfg(void *obj);
#endif
#if (SPI_2_FUN1_ENABLE == 1)
static void spi2_fun1_hw_cfg(void *obj);
#endif
#if (SPI_2_FUN4_ENABLE == 1)
static void spi2_fun4_hw_cfg(void *obj);
#endif
static void spi_hw_trans(spi_internal_handle handle);



static volatile spi_internal_object spi_internal_obj[SPI_NUM] = {
#if (SPI_0_FUN1_ENABLE == 1)
    {
        .ext_obj.is_open = false,
        .ext_obj.is_transfer = spi_idle,
        .ext_obj.spi_name = SPI_0_FUN_1,
        .ext_obj.spi_mode = SPI_UNDEFINED,

        .trans_obj.tx_buffer = NULL,
        .trans_obj.rx_buffer = NULL,
        .trans_obj.tx_len = 0,
        .trans_obj.rx_len = 0,
        .trans_obj.tx_offset = 0,
        .trans_obj.rx_offset = 0,
        .trans_obj.trans_mode = SPI_TRANS_UNDEFINED,


        .dma_tx_req = DMA_REQ_SPI0_TX,
        .dma_rx_req = DMA_REQ_SPI0_RX,

        .reg = spi_reg(0),
        .tx_dma_handle = NULL,
        .rx_dma_handle = NULL,
        .arg = NULL,
        
        .hw_config_fxn = spi0_fun1_hw_cfg,
        .trans_cb_fxn = NULL,
    },
#endif
#if (SPI_1_FUN1_ENABLE == 1)
    {
        .ext_obj.is_open = false,
        .ext_obj.is_transfer = spi_idle,
        .ext_obj.spi_name = SPI_1_FUN_1,
        .ext_obj.spi_mode = SPI_UNDEFINED,
            
        .trans_obj.tx_buffer = NULL,
        .trans_obj.rx_buffer = NULL,
        .trans_obj.tx_len = 0,
        .trans_obj.rx_len = 0,
        .trans_obj.tx_offset = 0,
        .trans_obj.rx_offset = 0,
        .trans_obj.trans_mode = SPI_TRANS_UNDEFINED,


        .dma_tx_req = DMA_REQ_SPI1_TX,
        .dma_rx_req = DMA_REQ_SPI1_RX,

        .reg = spi_reg(1),
        .tx_dma_handle = NULL,
        .rx_dma_handle = NULL,
        .arg = NULL,
        
        .hw_config_fxn = spi1_fun1_hw_cfg,
        .trans_cb_fxn = NULL,
    },
#endif
#if (SPI_2_FUN1_ENABLE == 1)
    {
        .ext_obj.is_open = false,
        .ext_obj.is_transfer = spi_idle,
        .ext_obj.spi_name = SPI_2_FUN_1,
        .ext_obj.spi_mode = SPI_UNDEFINED,
            
        .trans_obj.tx_buffer = NULL,
        .trans_obj.rx_buffer = NULL,
        .trans_obj.tx_len = 0,
        .trans_obj.rx_len = 0,
        .trans_obj.tx_offset = 0,
        .trans_obj.rx_offset = 0,
        .trans_obj.trans_mode = SPI_TRANS_UNDEFINED,
        
        .dma_tx_req = DMA_REQ_SPI2_TX,
        .dma_rx_req = DMA_REQ_SPI2_RX,

        .reg = spi_reg(2),
        .tx_dma_handle = NULL,
        .rx_dma_handle = NULL,
        .arg = NULL,
        
        .hw_config_fxn = spi2_fun1_hw_cfg,
        .trans_cb_fxn = NULL,
    },
#endif
#if (SPI_2_FUN4_ENABLE == 1)
    {
        .ext_obj.is_open = false,
        .ext_obj.is_transfer = spi_idle,
        .ext_obj.spi_name = SPI_2_FUN_4,
        .ext_obj.spi_mode = SPI_UNDEFINED,

        .trans_obj.tx_buffer = NULL,
        .trans_obj.rx_buffer = NULL,
        .trans_obj.tx_len = 0,
        .trans_obj.rx_len = 0,
        .trans_obj.tx_offset = 0,
        .trans_obj.rx_offset = 0,
        .trans_obj.trans_mode = SPI_TRANS_UNDEFINED,
    
        .dma_tx_req = DMA_REQ_SPI2_TX,
        .dma_rx_req = DMA_REQ_SPI2_RX,

        .reg = spi_reg(2),
        .tx_dma_handle = NULL,
        .rx_dma_handle = NULL,
        .arg = NULL,
        
        .hw_config_fxn = spi2_fun4_hw_cfg,
        .trans_cb_fxn = NULL,
    },
#endif
};

// int spi_debug(const char *fmt, ...)
// {
//     extern volatile unsigned int g_debug_dsp_switch;
//     if(g_debug_dsp_switch & (0x1 << DEBUG_DSP_SPI))
//     {
//         os_printf(fmt);
//     }
//     return 0;
// }

#if (SPI_0_FUN1_ENABLE == 1)
static void spi0_fun1_hw_cfg(void *obj)
{
    spi_internal_handle inter_handle = obj;
    spi_object *eobj = &inter_handle->ext_obj;

    if(eobj->io_cs_en)   gpio_config_spi(GPIO_SPI0_1F_CSN_IO , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_clk_en)  gpio_config_spi(GPIO_SPI0_1F_SCK_IO , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_miso_en) gpio_config_spi(GPIO_SPI0_1F_MISO_IO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_mosi_en) gpio_config_spi(GPIO_SPI0_1F_MOSI_IO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);

    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SPI0);
    system_mem_clk_enable(SYS_MEM_CLK_SPI0);
    system_peri_clk_enable(SYS_PERI_CLK_SPI0);
    system_peri_xxx_irq_enable(SYS_PERI_IRQ_SPI0);

}
#endif
#if (SPI_1_FUN1_ENABLE == 1)
static void spi1_fun1_hw_cfg(void *obj)
{
    spi_internal_handle inter_handle = obj;
    spi_object *eobj = &inter_handle->ext_obj;

    if(eobj->io_cs_en)   gpio_config_spi(GPIO_SPI1_1F_CSN_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_clk_en)  gpio_config_spi(GPIO_SPI1_1F_SCK_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_miso_en) gpio_config_spi(GPIO_SPI1_1F_MISO_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_mosi_en) gpio_config_spi(GPIO_SPI1_1F_MOSI_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);

    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SPI1);
    system_mem_clk_enable(SYS_MEM_CLK_SPI1);
    system_peri_clk_enable(SYS_PERI_CLK_SPI1);
    system_peri_xxx_irq_enable(SYS_PERI_IRQ_SPI1);
}
#endif
#if (SPI_2_FUN1_ENABLE == 1)
static void spi2_fun1_hw_cfg(void *obj)
{
    spi_internal_handle inter_handle = obj;
    spi_object *eobj = &inter_handle->ext_obj;

    if(eobj->io_cs_en)   gpio_config_spi(GPIO_SPI2_1F_CSN_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_clk_en)  gpio_config_spi(GPIO_SPI2_1F_SCK_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_miso_en) gpio_config_spi(GPIO_SPI2_1F_MISO_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    if(eobj->io_mosi_en) gpio_config_spi(GPIO_SPI2_1F_MOSI_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC1);
    
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SPI2);
    system_mem_clk_enable(SYS_MEM_CLK_SPI2);
    system_peri_clk_enable(SYS_PERI_CLK_SPI2);
    system_peri_xxx_irq_enable(SYS_PERI_IRQ_SPI2);
}
#endif
#if (SPI_2_FUN4_ENABLE == 1)
static void spi2_fun4_hw_cfg(void *obj)
{
    spi_internal_handle inter_handle = obj;
    spi_object *eobj = &inter_handle->ext_obj;

    if(eobj->io_cs_en)   gpio_config_spi(GPIO_SPI2_4F_CSN_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC4);
    if(eobj->io_clk_en)  gpio_config_spi(GPIO_SPI2_4F_SCK_PIN , GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC4);
    if(eobj->io_miso_en) gpio_config_spi(GPIO_SPI2_4F_MISO_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC4);
    if(eobj->io_mosi_en) gpio_config_spi(GPIO_SPI2_4F_MOSI_PIN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_FUNC4);
    
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SPI2);
    system_mem_clk_enable(SYS_MEM_CLK_SPI2);
    system_peri_clk_enable(SYS_PERI_CLK_SPI2);
    system_peri_xxx_irq_enable(SYS_PERI_IRQ_SPI2);
}
#endif


#if 0
#define GPIO_RX     GPIO37
#define GPIO_TX     GPIO38
#define DBG_GPIO_INIT           do{\
    gpio_config_spi(GPIO_RX, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);\
    gpio_config_spi(GPIO_TX, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);\
    gpio_output_set(GPIO_RX, 1);\
    gpio_output_set(GPIO_TX, 1);}while(0);
#define DBG_GPIO_ACTION_RX      do{gpio_output_set(GPIO_RX, 0);gpio_output_set(GPIO_RX, 1);}while(0);
#define DBG_GPIO_ACTION_TX      do{gpio_output_set(GPIO_TX, 0);gpio_output_set(GPIO_TX, 1);}while(0);
#else
#define DBG_GPIO_INIT
#define DBG_GPIO_ACTION_RX
#define DBG_GPIO_ACTION_TX
#endif


spi_handle spi_init(SPI_Name spi_name,spi_param_t *param)
{
    volatile spi_internal_handle inter_handle = NULL;
    DBG_GPIO_INIT
    if(spi_name >= SPI_NUM || param == NULL)
    {
        return NULL;
    }
    uint8_t i =0;
    for( i=0;i<SPI_NUM;i++)
    {
        if(spi_internal_obj[i].ext_obj.spi_name == spi_name)
        {
            inter_handle = (spi_internal_handle)&spi_internal_obj[i];
            break;
        }
    }
    if(inter_handle != NULL)
    {
        if(inter_handle->ext_obj.is_open == false)
        {
            uint32_t ul_freq_div;
            if (param->baud_rate == 0)
            {
                ul_freq_div = SPI_CLK_DIVID_SET(SPI_BAUD_1MHZ);
            }
            else
            {
                // ul_freq_div = NUMBER_ROUND_UP(SPI_BAUD_26MHZ/2, param->baud_rate) - 1;
                ul_freq_div = SPI_BAUD_26MHZ / 2 / param->baud_rate;
            }    
           
            if (ul_freq_div >= 255)
            {
                ul_freq_div = 255;
            }
            volatile spi_reg_t *reg = inter_handle->reg;
            spi_cfg cfg;
            cfg.fill = REG_GET(&reg->cfg);

            DEBUG_SPI_PRINTF("reg = %08x sizeof(spi_reg_t) = %d\r\n",reg,sizeof(spi_reg_t));
            
            cfg.reg.spi_ckr = (uint8_t)ul_freq_div;
            DEBUG_SPI_PRINTF("reg->cfg.fill = 0x%08X , ul_freq_div = %d\r\n",cfg.fill,ul_freq_div);
            
            cfg.reg.bit_wdth = param->bit_wdth;
            cfg.reg.txudf_en = 0;
            cfg.reg.rxovf_en = 0;
            
            cfg.reg.wire3_en = param->wire;
            inter_handle->ext_obj.io_cs_en   = cfg.reg.wire3_en ? 0 : param->io_cs_en;
            inter_handle->ext_obj.io_clk_en  = param->io_clk_en ;
            inter_handle->ext_obj.io_miso_en = param->io_miso_en;
            inter_handle->ext_obj.io_mosi_en = param->io_mosi_en;
            if(inter_handle->hw_config_fxn != NULL)
            {
                inter_handle->hw_config_fxn((void *)inter_handle);
            }
            
            cfg.reg.lsb_first = param->lsb_first;
            cfg.reg.ckpol = param->clk_polarity;
            cfg.reg.ckpha = param->clk_pha;
            if(param->mode == SPI_Slave && cfg.reg.wire3_en == false)
            {
                cfg.reg.slv_release_inten = true;
            }
            else
            {
                cfg.reg.slv_release_inten = false;
            }
            cfg.reg.msten = param->mode;
            cfg.reg.spien = false;
            if(cfg.reg.wire3_en == 0)
            {
                cfg.reg.byte_intlval = 1;   // cs pull high early if set 0
            }
            else
            {
                cfg.reg.byte_intlval = 0;  
            }
            REG_SET(&reg->cfg, cfg.fill);

            
            inter_handle->ext_obj.spi_mode = param->mode;
            inter_handle->ext_obj.is_open = true;
            inter_handle->ext_obj.is_transfer = spi_idle;

            DEBUG_SPI_PRINTF("reg->cfg.fill = 0x%08X , ul_freq_div = %d\r\n",reg->cfg.fill,ul_freq_div);
        }
    }
    return (spi_handle)inter_handle;
}

spi_dirver_err_code spi_uninit(spi_handle handle)
{
    if(handle == NULL){
        return spi_drv_err_param;
    }
    spi_internal_handle inter_handle = (spi_internal_handle)handle;
    if(inter_handle->ext_obj.is_transfer != spi_idle){
        return spi_drv_err_busy;
    }
    if(inter_handle->tx_dma_handle != NULL){
        dma_channel_free(inter_handle->tx_dma_handle);
    }
    if(inter_handle->rx_dma_handle != NULL){
        dma_channel_free(inter_handle->rx_dma_handle);
    }
    volatile spi_reg_t *reg = inter_handle->reg;
    REG_SET(&reg->cfg, 0);

    inter_handle->ext_obj.is_open = false;
    inter_handle->ext_obj.is_transfer = spi_idle;
    inter_handle->ext_obj.spi_mode = SPI_UNDEFINED;
    
    inter_handle->trans_obj.trans_mode = SPI_TRANS_UNDEFINED;    
    inter_handle->trans_obj.tx_buffer = NULL;
    inter_handle->trans_obj.rx_buffer = NULL;
    inter_handle->trans_obj.tx_len = 0;
    inter_handle->trans_obj.tx_offset = 0;
    inter_handle->trans_obj.rx_len = 0;
    inter_handle->trans_obj.rx_offset = 0;
    
    inter_handle->trans_cb_fxn = NULL;
    handle = NULL;
    return spi_drv_err_none;
}

spi_dirver_err_code spi_transfer(spi_handle handle,spi_transfer_object *trans_obj,void *arg,spi_trans_cb_fxn cb)
{
    spi_internal_handle internal_handle = (spi_internal_handle)handle;
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
    if(handle == NULL || trans_obj == NULL){
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        return spi_drv_err_param;
    }
    if(handle->is_open == false){
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        return spi_drv_err_not_init;
    }
    if(handle->is_transfer != spi_idle){
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        return spi_drv_err_busy;
    }
    if( !(trans_obj->trans_mode  & (SPI_TRANS_BLOCKING | SPI_TRANS_CALLBACK)) || \
        (trans_obj->tx_len == 0 && trans_obj->rx_len == 0))
    {
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        return spi_drv_err_param;
    }
    
    if(trans_obj->trans_mode & SPI_TRANS_CALLBACK)
    {
        if(cb != NULL)
        {
            internal_handle->trans_cb_fxn = cb;
        }
        else
        {
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            return spi_drv_err_param;
        }
    }
    
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
    internal_handle->arg = arg;
    internal_handle->trans_obj.rx_buffer = trans_obj->rx_buffer;
    internal_handle->trans_obj.tx_buffer = trans_obj->tx_buffer;
    internal_handle->trans_obj.tx_len = trans_obj->tx_len;
    internal_handle->trans_obj.rx_len = trans_obj->rx_len;
    internal_handle->trans_obj.trans_mode = trans_obj->trans_mode;
    internal_handle->trans_obj.tx_dma_addr_fixed = trans_obj->tx_dma_addr_fixed;
    if(trans_obj->trans_mode & SPI_TRANS_USE_DMA_FLAG)
    {
        if(internal_handle->tx_dma_handle == NULL && internal_handle->trans_obj.tx_buffer)
        {
            internal_handle->tx_dma_handle = dma_channel_malloc();
            if(internal_handle->tx_dma_handle == NULL){
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
                return spi_drv_err_dma_alloc_failed;
            }
        }
        if(internal_handle->rx_dma_handle == NULL && internal_handle->trans_obj.rx_buffer)
        {
            internal_handle->rx_dma_handle = dma_channel_malloc();
            if(internal_handle->rx_dma_handle == NULL){
    DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
                return spi_drv_err_dma_alloc_failed;
            }
        }
    }
    spi_hw_trans((spi_internal_handle)handle);
    return spi_drv_err_none;
}





static void spi_hw_trans(spi_internal_handle handle)
{
    if(handle == NULL)
    {
        return;
    }   
    volatile spi_transfer_object * trans_obj = &handle->trans_obj;
    volatile spi_reg_t *reg = handle->reg;
    
    trans_obj->tx_offset = 0;
    trans_obj->rx_offset = 0;
    
    spi_cfg     cfg;
    spi_cn      cn;
    spi_stat    stat;

    cfg.fill = REG_GET(&reg->cfg);
    cn.fill = REG_GET(&reg->cn);
    stat.fill = REG_GET(&reg->stat);

    stat.reg.txfifo_clr = 1;
    stat.reg.rxfifo_clr = 1;


    if(trans_obj->tx_buffer == NULL)
    {
        cn.reg.tx_en =0;
    }
    
    if(trans_obj->rx_buffer == NULL)
    {
        cn.reg.rx_en =0;
    }
    cn.reg.rx_en =0;
    cn.reg.tx_en =0;
    cfg.reg.spien = 1;
    handle->ext_obj.is_transfer = spi_idle;
    
    DEBUG_SPI_PRINTF("%d handle->ext_obj.transfer_mode=%04x\r\n", handle->ext_obj.spi_name,trans_obj->trans_mode);

    REG_SET(&reg->cfg, cfg.fill);
    REG_SET(&reg->cn, cn.fill);
    REG_SET(&reg->stat, stat.fill);
    
    DEBUG_SPI_PRINTF("trans_obj->rx_buf=%p,trans_obj->tx_buf=%p\r\n",trans_obj->rx_buffer,trans_obj->tx_buffer);


    if(trans_obj->rx_buffer)
    {
        DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        handle->ext_obj.is_transfer |= spi_reading;
        if( trans_obj->trans_mode & SPI_TRANS_USE_DMA_FLAG )
        {
            cfg.reg.rxfifo_int_en = 0;
            cn.reg.rx_finish_int_en = 1;
            uint32_t rx_len = trans_obj->rx_len;
            if(rx_len > SPI_TRANS_DMA_MAX)
            {
                rx_len = SPI_TRANS_DMA_MAX;
            }
            cn.reg.rx_trans_len = rx_len;

            uint32_t dest_end_addr = (uint32_t)((uint8_t*)trans_obj->rx_buffer + (rx_len << cfg.reg.bit_wdth));
            if(dest_end_addr & 0x3) {
                DEBUG_SPI_PRINTF("ERROR:dma addr is not 4byte aligned, src:0x%X, dest:0x%X\n", trans_obj->rx_buffer, dest_end_addr);
                dest_end_addr = (dest_end_addr & ~0x3) + 4;//for addr need word align when src_req_mux isn't equal 0
            }
            DEBUG_SPI_PRINTF("rx dma addr 0x%08X ~ 0x%08X\n",(uint32_t)(trans_obj->rx_buffer), dest_end_addr);

            dma_channel_config(handle->rx_dma_handle,
                               handle->dma_rx_req,
                               DMA_MODE_SINGLE,
                               (uint32_t)&reg->dat,
                               (uint32_t)&reg->dat,
                               DMA_ADDR_NO_CHANGE,
                               ((cfg.reg.bit_wdth == SPI_BIT_8) ? DMA_DATA_TYPE_CHAR : DMA_DATA_TYPE_SHORT),
                               (uint32_t)((uint32_t)trans_obj->rx_buffer + DSP_RAM_BASIC_ADDR),
                               (uint32_t)(dest_end_addr + DSP_RAM_BASIC_ADDR),
                               DMA_ADDR_AUTO_INCREASE,
                               DMA_DATA_TYPE_LONG,
                               rx_len << cfg.reg.bit_wdth);

        }
        else
        {
            cfg.reg.rxfifo_int_level = SPI_FIFO_48_BYTE; 
            cfg.reg.rxfifo_int_en = 1;
            cn.reg.rx_finish_int_en = 1;
            uint32_t rx_len = trans_obj->rx_len;
            if(rx_len > SPI_TRANS_DMA_MAX)
            {
                rx_len = SPI_TRANS_DMA_MAX;
            }
            cn.reg.rx_trans_len = rx_len;
        }
    }

    if(trans_obj->tx_buffer)
    {
        DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
        handle->ext_obj.is_transfer |= spi_writing;
        if( trans_obj->trans_mode & SPI_TRANS_USE_DMA_FLAG )
        {
            cfg.reg.txfifo_int_en = 0;
            cn.reg.tx_finish_int_en = 1;
            uint32_t tx_len = trans_obj->tx_len;
            if(tx_len > SPI_TRANS_DMA_MAX)
            {
                tx_len = SPI_TRANS_DMA_MAX;
            }
            cn.reg.tx_trans_len = tx_len;

            uint32_t src_end_addr = (uint32_t)((uint8_t*)trans_obj->tx_buffer + (tx_len << cfg.reg.bit_wdth));
            if(src_end_addr & 0x3) {
                DEBUG_SPI_PRINTF("ERROR: dma addr is not 4byte aligned, 0x%X ~ 0x%X\n", trans_obj->tx_buffer, src_end_addr);
                src_end_addr = (src_end_addr & ~0x3) + 4;//for addr need word align when src_req_mux isn't equal 0
            }
            DEBUG_SPI_PRINTF("tx dma addr 0x%08X ~ 0x%08X\n",(uint32_t)(trans_obj->tx_buffer), src_end_addr);
            DEBUG_SPI_PRINTF("tx dma tx_dma_addr_fixed:%d", trans_obj->tx_dma_addr_fixed);

            dma_channel_config(handle->tx_dma_handle,
                               handle->dma_tx_req,
                               DMA_MODE_SINGLE,
                               (uint32_t)((uint32_t)trans_obj->tx_buffer + DSP_RAM_BASIC_ADDR),
                               (uint32_t)(src_end_addr + DSP_RAM_BASIC_ADDR),
                               (trans_obj->tx_dma_addr_fixed ? DMA_ADDR_NO_CHANGE : DMA_ADDR_AUTO_INCREASE),
                               DMA_DATA_TYPE_LONG,
                               (uint32_t)&reg->dat,
                               (uint32_t)&reg->dat,
                               DMA_ADDR_NO_CHANGE,
                               ((cfg.reg.bit_wdth == SPI_BIT_8) ? DMA_DATA_TYPE_CHAR : DMA_DATA_TYPE_SHORT),
                               tx_len << cfg.reg.bit_wdth);

        }
        else
        {
            cfg.reg.txfifo_int_level = SPI_FIFO_16_BYTE; 
            cfg.reg.txfifo_int_en = 1;
            cn.reg.tx_finish_int_en = 1;
            uint32_t tx_len = trans_obj->tx_len;
            if(tx_len > SPI_TRANS_DMA_MAX)
            {
                tx_len = SPI_TRANS_DMA_MAX;
            }
            cn.reg.tx_trans_len = tx_len;

            while(!stat.reg.txfifo_wr_ready)
            {
                stat.fill = REG_GET(&reg->stat);
                SPI_REG_RW_DELAY
            }

            while( (trans_obj->tx_offset < trans_obj->tx_len) )
            {
                if(stat.reg.txfifo_wr_ready)
                {
                    if(cfg.reg.bit_wdth == SPI_BIT_8)
                    {
                        REG_SET(&reg->dat, *((uint8_t*)trans_obj->tx_buffer + trans_obj->tx_offset));
                    }
                    else
                    {
                        REG_SET(&reg->dat, *((uint16_t*)trans_obj->tx_buffer + trans_obj->tx_offset));
                    }
                    trans_obj->tx_offset++;
                }
                else
                {
                    break;
                }
                stat.fill = REG_GET(&reg->stat);
                SPI_REG_RW_DELAY
            }

            DEBUG_SPI_PRINTF("%d trans_obj->tx_offset=%d,trans_obj->tx_len=%d\r\n", handle->ext_obj.spi_name,trans_obj->tx_offset,trans_obj->tx_len);
        }
    }

    REG_SET(&reg->cfg, cfg.fill);

    if(trans_obj->tx_buffer)
    {
        if( trans_obj->trans_mode & SPI_TRANS_USE_DMA_FLAG)
        {
            dma_channel_enable(handle->tx_dma_handle, true);
        }
        cn.fill |= MSK_SPI0_CN_TX_EN;
    }
    if(trans_obj->rx_buffer)
    {
        if( trans_obj->trans_mode & SPI_TRANS_USE_DMA_FLAG)
        {
            dma_channel_enable(handle->rx_dma_handle, true);
        }
        cn.fill |= MSK_SPI0_CN_RX_EN;
    }
    
    REG_SET(&reg->cn, cn.fill);
    
    DEBUG_SPI_PRINTF("%d, handle->ext_obj.is_transfer:%d\r\n", __LINE__, handle->ext_obj.is_transfer);
    DEBUG_SPI_PRINTF("cn=%08x,stat=%08x,cfg=%08x\r\n", REG_GET(&reg->cn), REG_GET(&reg->stat), REG_GET(&reg->cfg));


    if( trans_obj->trans_mode & SPI_TRANS_BLOCKING)
    {
        DEBUG_SPI_PRINTF("%d SPI SPI_TRANS_BLOCKING \r\n", handle->ext_obj.spi_name);
        while(handle->ext_obj.is_transfer != spi_idle)
        {
            sys_delay_us(2);
        }
        DEBUG_SPI_PRINTF("%d SPI SPI_TRANS_BLOCKING Finish\r\n", handle->ext_obj.spi_name);
        cfg.reg.spien = 0;
        REG_SET(&reg->cfg, cfg.fill);
    }
}


static inline void spi_hw_interrupt_dma(SPI_Name spi_name)
{
    volatile spi_internal_handle inter_handle = (volatile spi_internal_handle)&spi_internal_obj[spi_name];
    volatile spi_reg_t *reg = inter_handle->reg;
    volatile spi_transfer_object * trans_obj = &inter_handle->trans_obj;
   
    spi_cfg     cfg;
    spi_cn      cn;
    spi_stat    stat;

    cfg.fill = REG_GET(&reg->cfg);
    cn.fill = REG_GET(&reg->cn);
    stat.fill = REG_GET(&reg->stat);

    DEBUG_SPI_PRINTF("%d %d CFG=0x%08X, CN=0x%08X, STAT=0x%08X\r\n", __LINE__, spi_name, cfg.fill, cn.fill, stat.fill);

    if(stat.fill & 
       (MSK_SPI0_STAT_RXOVF | \
        MSK_SPI0_STAT_TXUDF | \
        MSK_SPI0_STAT_TX_FINISH_INT | \
        MSK_SPI0_STAT_RX_FINISH_INT | \
        MSK_SPI0_STAT_RXFIFO_INT |\
        MSK_SPI0_STAT_TXFIFO_INT))
    {
        DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);

        REG_SET(&reg->stat, stat.fill);

        uint32_t continue_trans_len = 0;

        //last read transfer complete process.
        if(inter_handle->ext_obj.is_transfer & spi_reading)
        {
            if( (stat.reg.rx_finish_int == 1) )
            {
                trans_obj->rx_offset += cn.reg.rx_trans_len;
            }
            DEBUG_SPI_PRINTF("%d %d CFG=0x%08X, CN=0x%08X, STAT=0x%08X\r\n", __LINE__, spi_name, cfg.fill, cn.fill, stat.fill);
            DEBUG_SPI_PRINTF("trans_obj->rx_offset:%d, trans_obj->rx_len:%d\r\n", trans_obj->rx_offset, trans_obj->rx_len);
            if(trans_obj->rx_offset == trans_obj->rx_len)
            {
                inter_handle->ext_obj.is_transfer &= ~spi_reading;
                dma_channel_enable(inter_handle->rx_dma_handle, false);
                DBG_GPIO_ACTION_RX
                cn.reg.rx_en = 0;
                // cfg.reg.rxfifo_int_en = 0;
                // cn.reg.rx_finish_int_en = 0;
                DEBUG_SPI_PRINTF("%d [INT]SPI RX Finish\r\n",spi_name);
            }
        }
        //last write transfer complete process.
        if(inter_handle->ext_obj.is_transfer & spi_writing)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            if( (stat.reg.tx_finish_int == 1) )
            {
                trans_obj->tx_offset += cn.reg.tx_trans_len;
            }
            if(trans_obj->tx_offset == trans_obj->tx_len)
            {
                inter_handle->ext_obj.is_transfer &= ~spi_writing;
                dma_channel_enable(inter_handle->tx_dma_handle, false);
                DBG_GPIO_ACTION_TX
                cn.reg.tx_en = 0;
                // cfg.reg.txfifo_int_en = 0;
                // cn.reg.tx_finish_int_en = 0;
                DEBUG_SPI_PRINTF("%d [INT]SPI TX Finish\r\n",spi_name);
            }
        }

        // REG_SET(&reg->cfg, cfg.fill);
        REG_SET(&reg->cn, cn.fill);

        //read continune
        if(inter_handle->ext_obj.is_transfer & spi_reading)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            if(stat.reg.rx_finish_int == 1)
            {
                cn.reg.rx_en = 0;
                REG_SET(&reg->cn, cn.fill);
                continue_trans_len = trans_obj->rx_len - trans_obj->rx_offset;
                if(continue_trans_len > SPI_TRANS_DMA_MAX)
                {
                    continue_trans_len = SPI_TRANS_DMA_MAX;
                }
                cn.reg.rx_trans_len = continue_trans_len;

                uint32_t dest_end_addr = (uint32_t)((uint8_t*)trans_obj->rx_buffer + ((trans_obj->rx_offset + continue_trans_len) << cfg.reg.bit_wdth));
                if(dest_end_addr & 0x3) dest_end_addr = (dest_end_addr & ~0x3) + 4;//for addr need word align when src_req_mux isn't equal 0

                dma_channel_config(inter_handle->rx_dma_handle,
                                    inter_handle->dma_rx_req,
                                    DMA_MODE_SINGLE,
                                    (uint32_t)&reg->dat,
                                    (uint32_t)&reg->dat,
                                    DMA_ADDR_NO_CHANGE,
                                    ((cfg.reg.bit_wdth == SPI_BIT_8) ? DMA_DATA_TYPE_CHAR : DMA_DATA_TYPE_SHORT),
                                    (uint32_t)((uint8_t*)trans_obj->rx_buffer + DSP_RAM_BASIC_ADDR + (trans_obj->rx_offset << cfg.reg.bit_wdth)),
                                    (uint32_t)(dest_end_addr + DSP_RAM_BASIC_ADDR),
                                    DMA_ADDR_AUTO_INCREASE,
                                    DMA_DATA_TYPE_LONG,
                                    continue_trans_len << cfg.reg.bit_wdth);

                dma_channel_enable(inter_handle->rx_dma_handle, true);
                cn.reg.rx_en = 1;
            }
        }

        //write continune
        if(inter_handle->ext_obj.is_transfer & spi_writing)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            if(stat.reg.tx_finish_int == 1)
            {
                cn.reg.tx_en = 0;
                REG_SET(&reg->cn, cn.fill);
                continue_trans_len = trans_obj->tx_len - trans_obj->tx_offset;
                if(continue_trans_len > SPI_TRANS_DMA_MAX)
                {
                    continue_trans_len = SPI_TRANS_DMA_MAX;
                }
                cn.reg.tx_trans_len = continue_trans_len;

                uint32_t src_start_addr = (uint32_t)((uint8_t*)trans_obj->tx_buffer);
                if(!trans_obj->tx_dma_addr_fixed) src_start_addr += (trans_obj->tx_offset << cfg.reg.bit_wdth);
                uint32_t src_end_addr = (uint32_t)((uint8_t*)trans_obj->tx_buffer + ((trans_obj->tx_offset + continue_trans_len) << cfg.reg.bit_wdth));
                if(src_end_addr & 0x3) src_end_addr = (src_end_addr & ~0x3) + 4;//for addr need word align when src_req_mux isn't equal 0

                dma_channel_config(inter_handle->tx_dma_handle,
                                    inter_handle->dma_tx_req,
                                    DMA_MODE_SINGLE,
                                    (uint32_t)(src_start_addr + DSP_RAM_BASIC_ADDR),
                                    (uint32_t)(src_end_addr + DSP_RAM_BASIC_ADDR),
                                    (trans_obj->tx_dma_addr_fixed ? DMA_ADDR_NO_CHANGE : DMA_ADDR_AUTO_INCREASE),
                                    DMA_DATA_TYPE_LONG,
                                    (uint32_t)&reg->dat,
                                    (uint32_t)&reg->dat,
                                    DMA_ADDR_NO_CHANGE,
                                    ((cfg.reg.bit_wdth == SPI_BIT_8) ? DMA_DATA_TYPE_CHAR : DMA_DATA_TYPE_SHORT),
                                    continue_trans_len << cfg.reg.bit_wdth);

                dma_channel_enable(inter_handle->tx_dma_handle, true);
                cn.reg.tx_en = 1;
            }
        }

        if( (trans_obj->trans_mode & SPI_TRANS_CALLBACK) && 
            (inter_handle->ext_obj.is_transfer == spi_idle) )
        {
            if(inter_handle->trans_cb_fxn != NULL)
            {
                cfg.reg.spien = 0;
                inter_handle->trans_cb_fxn((spi_handle)inter_handle,(spi_transfer_object*)trans_obj,inter_handle->arg);
            }
        }
    }

    REG_SET(&reg->cfg, cfg.fill);
    REG_SET(&reg->cn, cn.fill);
        
    DEBUG_SPI_PRINTF("%d CFG=0x%08X,CN=0x%08X,STAT=0x%08X\r\n",spi_name,cfg.fill,cn.fill,stat.fill);
    
    if(stat.reg.slv_release_int)
    {
        DEBUG_SPI_PRINTF("reg->stat.slv_release_int\r\n");
        stat.reg.slv_release_int = 1;
        // REG_SET(&reg->stat, stat.fill);
        //TODO
    }
}

static inline void spi_hw_interrupt_reg(SPI_Name spi_name)
{
    volatile spi_internal_handle inter_handle = (volatile spi_internal_handle)&spi_internal_obj[spi_name];
    volatile spi_reg_t *reg = inter_handle->reg;
    volatile spi_transfer_object * trans_obj = &inter_handle->trans_obj;
   
    spi_cfg     cfg;
    spi_cn      cn;
    spi_stat    stat;

    cfg.fill = REG_GET(&reg->cfg);
    cn.fill = REG_GET(&reg->cn);
    stat.fill = REG_GET(&reg->stat);

    DEBUG_SPI_PRINTF("%d %d CFG=0x%08X, CN=0x%08X, STAT=0x%08X\r\n", __LINE__, spi_name, cfg.fill, cn.fill, stat.fill);

    if(stat.fill & 
       (MSK_SPI0_STAT_RXOVF | \
        MSK_SPI0_STAT_TXUDF | \
        MSK_SPI0_STAT_TX_FINISH_INT | \
        MSK_SPI0_STAT_RX_FINISH_INT | \
        MSK_SPI0_STAT_RXFIFO_INT |\
        MSK_SPI0_STAT_TXFIFO_INT))
    {
        DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);

        REG_SET(&reg->stat, stat.fill);

        uint32_t continue_trans_len = 0;

        //last read transfer complete process.
        if(inter_handle->ext_obj.is_transfer & spi_reading)
        {
            uint8_t len = 48;
            //FIFO process
            while(  (trans_obj->rx_offset < trans_obj->rx_len) && (len--) )
            {
                if(stat.reg.rxfifo_rd_ready)
                {
                    if(cfg.reg.bit_wdth == SPI_BIT_8)
                    {
                        *((uint8_t*)trans_obj->rx_buffer + trans_obj->rx_offset) = (uint8_t)REG_GET(&reg->dat);
                    }
                    else
                    {
                        *((uint16_t*)trans_obj->rx_buffer + trans_obj->rx_offset) = (uint16_t)REG_GET(&reg->dat);
                    }
                    trans_obj->rx_offset++;
                }
                else
                {
                    break;
                }
                stat.fill = REG_GET(&reg->stat);
                SPI_REG_RW_DELAY
            }
            DEBUG_SPI_PRINTF("%d [INT]rx_offset=%d\r\n",spi_name,trans_obj->rx_offset);
            DEBUG_SPI_PRINTF("%d %d CFG=0x%08X, CN=0x%08X, STAT=0x%08X\r\n", __LINE__, spi_name, cfg.fill, cn.fill, stat.fill);
            DEBUG_SPI_PRINTF("trans_obj->rx_offset:%d, trans_obj->rx_len:%d\r\n", trans_obj->rx_offset, trans_obj->rx_len);
            if(trans_obj->rx_offset == trans_obj->rx_len)
            {
                inter_handle->ext_obj.is_transfer &= ~spi_reading;
                DBG_GPIO_ACTION_RX
                cn.reg.rx_en = 0;
                // cfg.reg.rxfifo_int_en = 0;
                // cn.reg.rx_finish_int_en = 0;
                DEBUG_SPI_PRINTF("%d [INT]SPI RX Finish\r\n",spi_name);
            }
        }
        //last write transfer complete process.
        if(inter_handle->ext_obj.is_transfer & spi_writing)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            if(trans_obj->tx_offset == trans_obj->tx_len)
            {
                inter_handle->ext_obj.is_transfer &= ~spi_writing;
                DBG_GPIO_ACTION_TX
                cn.reg.tx_en = 0;
                // cfg.reg.txfifo_int_en = 0;
                // cn.reg.tx_finish_int_en = 0;
                DEBUG_SPI_PRINTF("%d [INT]SPI TX Finish\r\n",spi_name);
            }
        }

        // REG_SET(&reg->cfg, cfg.fill);
        REG_SET(&reg->cn, cn.fill);

        //read continune
        if(inter_handle->ext_obj.is_transfer & spi_reading)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            // cfg.reg.rxfifo_int_en = 1;
            // cn.reg.rx_finish_int_en = 1;
            if(stat.reg.rx_finish_int == 1)
            {
                continue_trans_len = trans_obj->rx_len - trans_obj->rx_offset;
                if(continue_trans_len > SPI_TRANS_DMA_MAX)
                {
                    continue_trans_len = SPI_TRANS_DMA_MAX;
                }
                cn.reg.rx_trans_len = continue_trans_len;
                // cn.reg.rx_en = 1;
            }
        }
        //write continune
        if(inter_handle->ext_obj.is_transfer & spi_writing)
        {
            DEBUG_SPI_PRINTF("%s %d\r\n",__func__,__LINE__);
            if(stat.reg.tx_finish_int == 1)
            {
                continue_trans_len = trans_obj->tx_len - trans_obj->tx_offset;
                if(continue_trans_len > SPI_TRANS_DMA_MAX)
                {
                    continue_trans_len = SPI_TRANS_DMA_MAX;
                }
                cn.reg.tx_trans_len = continue_trans_len;
            }

            //FIFO process
            while( (trans_obj->tx_offset < trans_obj->tx_len) )
            {
                if(stat.reg.txfifo_wr_ready)
                {
                    if(cfg.reg.bit_wdth == SPI_BIT_8)
                    {
                        REG_SET(&reg->dat, *((uint8_t*)trans_obj->tx_buffer + trans_obj->tx_offset));
                    }
                    else
                    {
                        REG_SET(&reg->dat, *((uint16_t*)trans_obj->tx_buffer + trans_obj->tx_offset));
                    }
                    trans_obj->tx_offset++;
                }
                else
                {
                    break;
                }
                stat.fill = REG_GET(&reg->stat);
                SPI_REG_RW_DELAY
            }
        }

        if( (trans_obj->trans_mode & SPI_TRANS_CALLBACK) && 
            (inter_handle->ext_obj.is_transfer == spi_idle) )
        {
            if(inter_handle->trans_cb_fxn != NULL)
            {
                cfg.reg.spien = 0;
                inter_handle->trans_cb_fxn((spi_handle)inter_handle,(spi_transfer_object*)trans_obj,inter_handle->arg);
            }
        }
    }

    REG_SET(&reg->cfg, cfg.fill);
    REG_SET(&reg->cn, cn.fill);
        
    DEBUG_SPI_PRINTF("%d CFG=0x%08X,CN=0x%08X,STAT=0x%08X\r\n",spi_name,cfg.fill,cn.fill,stat.fill);
    
    if(stat.reg.slv_release_int)
    {
        DEBUG_SPI_PRINTF("reg->stat.slv_release_int\r\n");
        stat.reg.slv_release_int = 1;
        // REG_SET(&reg->stat, stat.fill);
        //TODO
    }
}

void spi_hw_interrupt(SPI_Name spi_name)
{
    if(spi_name >= SPI_NUM)
    {
        return;
    }

    DEBUG_SPI_PRINTF(" >>> %s %d enter\r\n",__func__,__LINE__);
    if(spi_internal_obj[spi_name].trans_obj.trans_mode & SPI_TRANS_USE_DMA_FLAG)
    {
        spi_hw_interrupt_dma(spi_name);
    }
    else
    {
        spi_hw_interrupt_reg(spi_name);
    }
    DEBUG_SPI_PRINTF(" <<< %s %d exit\r\n",__func__,__LINE__);
}

#ifdef SPI_SIMPLE_DRV


/**
 * @param SPIx : 0, 1, 2;
 * @param master_en 1:master, 0:slave
 * @param spi_16bit_en  0:8bit, 1:16bit
 * @param wire3_en 0:cs pin invalid, 1: cs pin valid
 * */
void hal_spi_init(uint8_t SPIx, SPI_Mode master_en, SPI_Bit_Wdth spi_16bit_en, SPI_Wire wire3_en)
{
    volatile spi_reg_t *reg = spi_reg(SPIx);
   
    spi_cfg     cfg;
    // spi_cn      cn;
    // spi_stat    stat;

    DEBUG_SPI_PRINTF("%s()\n",__func__);
    cfg.fill = REG_GET(&reg->cfg);
    // cn.fill = REG_GET(&reg->cn);
    // stat.fill = REG_GET(&reg->stat);

    cfg.reg.txfifo_int_level = 1;//0~3 ->1,16,32,48, fifo depth=64
    cfg.reg.rxfifo_int_level = 3;//0~3 ->1,16,32,48, fifo depth=64
    cfg.reg.txudf_en = 0;
    cfg.reg.rxovf_en = 0;
    cfg.reg.txfifo_int_en = 0;
    cfg.reg.rxfifo_int_en = 0;
    cfg.reg.spi_ckr = SPI_BAUD_26MHZ / 2 / SPI_DEFAULT_BAUD;//clk div, 0:26MHz
    cfg.reg.slv_release_inten = 0;//cs rise interrupt
    cfg.reg.wire3_en = wire3_en;
    cfg.reg.bit_wdth = spi_16bit_en;//0->8bit, 1->16bit
    cfg.reg.lsb_first = 0;
    //spi ckpol:ckpha ->mode0~0:0; mode3~1:1
    cfg.reg.ckpol = 0;
    cfg.reg.ckpha = 0;
    cfg.reg.msten = master_en;
    cfg.reg.spien = 0;
    cfg.reg.byte_intlval = 1;//interval between data and data(UNIT: spi clk)

    REG_SET(&reg->cfg, cfg.fill);

//////////////////////////// spi gpio map select
    switch (SPIx)
    {
    #if (SPI_0_FUN1_ENABLE == 1)
        case 0: spi0_fun1_hw_cfg((void*)&spi_internal_obj[SPI_0_FUN_1]); break;
    #endif
    #if (SPI_1_FUN1_ENABLE == 1)
        case 1: spi1_fun1_hw_cfg((void*)&spi_internal_obj[SPI_1_FUN_1]); break;
    #endif
    #if (SPI_2_FUN1_ENABLE == 1)
        case 2: spi2_fun1_hw_cfg((void*)&spi_internal_obj[SPI_2_FUN_1]); break;
    #elif (SPI_2_FUN4_ENABLE == 1)
        case 2: spi2_fun4_hw_cfg((void*)&spi_internal_obj[SPI_2_FUN_4]); break;
    #endif
    default: break;
    }
}

/**
 * @param SPIx : 0, 1, 2
 * @param tx_buf data buffer to send out.   [if no data send, use param "NULL"]
 * @param tx_sz send size(unit:Byte)        [if no data send, use param "0"]
 * @param rx_buf data buffer to recieve in. [if no data recv, use param "NULL"]
 * @param rx_sz send size(unit:Byte)        [if no data recv, use param "0"]
 * */
void hal_spi_read_write(uint8_t SPIx, uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz)
{
    int tx_len;
    int rx_len;

    volatile spi_reg_t *reg = spi_reg(SPIx);
    spi_cfg     cfg;
    spi_cn      cn;
    spi_stat    stat;

    cfg.fill = REG_GET(&reg->cfg);
    cn.fill = REG_GET(&reg->cn);
    stat.fill = REG_GET(&reg->stat);

    tx_len = tx_sz >> cfg.reg.bit_wdth;
    rx_len = rx_sz >> cfg.reg.bit_wdth;

    DEBUG_SPI_PRINTF(" >>> enter %s()\n", __func__);
    if((tx_len > SPI_TRANS_DMA_MAX) || (rx_len > SPI_TRANS_DMA_MAX)){
        DEBUG_SPI_PRINTF("ERROR:  trans over size! tx_len:%d, rx_len:%d\n", tx_len, rx_len);
        goto RET;
    }

    if(((tx_buf == NULL) && (rx_buf == NULL)) || ((tx_sz == 0) && (rx_sz == 0))){
        DEBUG_SPI_PRINTF("ERROR:  param error! tx_sz:%d, rx_sz:%d\n", tx_sz, rx_sz);
        goto RET;
    }

    // cfg.reg.txudf_en = 1;
    // cfg.reg.rxovf_en = 1;
    // cfg.reg.txfifo_int_en = 1;
    // cfg.reg.rxfifo_int_en = 1;
    // cfg.reg.slv_release_inten = 1;//cs rise interrupt
    cfg.reg.spien = 1;
    REG_SET(&reg->cfg, cfg.fill);

    cn.reg.tx_trans_len = tx_len;
    cn.reg.rx_trans_len = rx_len;
    // cn.reg.tx_finish_int_en = 1;
    // cn.reg.rx_finish_int_en = 1;
    cn.reg.tx_en = (tx_len) ? 1 : 0;
    cn.reg.rx_en = (rx_len) ? 1 : 0;
    REG_SET(&reg->cn, cn.fill);

    stat.reg.txfifo_clr = 1;
    stat.reg.rxfifo_clr = 1;
    REG_SET(&reg->stat, stat.fill);

    int tx_idx = 0;
    int rx_idx = 0;

    if(cfg.reg.bit_wdth == SPI_BIT_8)
    {
        // while((stat.reg.tx_finish_int == 0) || (stat.reg.rx_finish_int == 0))
        while(1)
        {
            NOP();
            if((tx_idx < tx_len) && stat.reg.txfifo_wr_ready)
            {
                REG_SET(&reg->dat, *((uint8_t*)tx_buf + tx_idx));
                tx_idx++;
            }
            if((rx_idx < rx_len) && stat.reg.rxfifo_rd_ready)
            {
                *((uint8_t*)rx_buf + rx_idx) = (uint8_t)REG_GET(&reg->dat);
                rx_idx++;
            }
            stat.fill = REG_GET(&reg->stat);
            if((tx_idx >= tx_len) && (rx_idx >= rx_len)) break;
        }
    }
    else
    {
        // while((stat.reg.tx_finish_int == 0) || (stat.reg.rx_finish_int == 0))
        while(1)
        {
            NOP();
            if((tx_idx < tx_len) && stat.reg.txfifo_wr_ready)
            {
                REG_SET(&reg->dat, *((uint16_t*)tx_buf + tx_idx));
                tx_idx++;
            }
            if((rx_idx < rx_len) && stat.reg.rxfifo_rd_ready)
            {
                *((uint16_t*)rx_buf + rx_idx) = (uint16_t)REG_GET(&reg->dat);
                rx_idx++;
            }
            stat.fill = REG_GET(&reg->stat);
            if((tx_idx >= tx_len) && (rx_idx >= rx_len)) break;
        }
    }

    DEBUG_SPI_PRINTF("CFG=0x%08X,CN=0x%08X,STAT=0x%08X\n",cfg.fill,cn.fill,stat.fill);
    REG_SET(&reg->stat, stat.fill);
    DEBUG_SPI_PRINTF("%s %d\n",__func__,__LINE__);
    DEBUG_SPI_PRINTF("idx : %d %d\n", tx_idx, rx_idx);

    cn.reg.tx_trans_len = 0;
    cn.reg.rx_trans_len = 0;
    // cn.reg.tx_finish_int_en = 0;
    // cn.reg.rx_finish_int_en = 0;
    cn.reg.tx_en = 0;
    cn.reg.rx_en = 0;
    REG_SET(&reg->cn, cn.fill);
    
    // cfg.reg.txudf_en = 0;
    // cfg.reg.rxovf_en = 0;
    // cfg.reg.txfifo_int_en = 0;
    // cfg.reg.rxfifo_int_en = 0;
    // cfg.reg.slv_release_inten = 0;//cs rise interrupt
    cfg.reg.spien = 0;
    REG_SET(&reg->cfg, cfg.fill);
    DEBUG_SPI_PRINTF(" <<< exit %s()\n", __func__);
RET:
    return;
}


#endif //SPI_SIMPLE_DRV


#endif // SPI ENABLE
