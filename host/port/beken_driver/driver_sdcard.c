/*************************************************************
 * @file        driver_sdcard.c
 * @brief       driver of PTN101
 * @author      Wang GuangMing
 * @version     V1.1
 * @date        2022-08-23
 * @par
 * @attention
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifndef CEVA_X
#include "types.h"
// #include "app_env.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#else
#include "ceva_includes.h"
#include "driver_types.h"
#endif
#include "bkreg.h"
#include "sys_irq.h"
#include "driver_dma.h"
#include "drv_system.h"
#include "driver_sdcard.h"
//#include "bautil.h"
#include "spr_defs.h"
#include "driver_gpio.h"
#include "timer.h"

// #define SDIO_CLK_CMD_DAT0_SEL8_9_10
// #define CONFIG_SDCARD_DETECT
#define SD_ATACH_DEBOUNCE_CNT       50 //unit:10ms
#define SD_DETACH_DEBOUNCE_CNT      2 //unit:10ms

#ifdef CONFIG_APP_SDCARD_4_LINE
    #define CONFIG_SDCARD_BUSWIDTH_4LINE
#endif

#define SDIO_TRANS_MIN_STOP //send stop in each read/write multi block

#ifndef CEVA_X


#define REG_GET(addr) (*(volatile uint32*)(addr))
#define REG_SET_EQU(addr, val) (*(volatile uint32*)(addr) = (val))
#define REG_SET_AND(addr, val) (*(volatile uint32*)(addr) &= (val))
#define REG_SET_OR(addr, val) (*(volatile uint32*)(addr) |= (val))
//bit set
#define REG_BITS_SET(addr,bit,msk,val) \
do\
{\
    (*(volatile uint32*)(addr)) &= ((uint32)~(msk));\
    (*(volatile uint32*)(addr)) |= (((val) << (bit)) & (msk));\
}while(0)

#else

#define REG_GET(addr)  (_in(((uint32)addr)))
#define REG_SET_EQU(addr,val)  (_out((uint32)(val), (uint32)(addr)))
#define REG_SET_AND(addr,val)  (_out(_in(((uint32)addr)) & (uint32)(val), (uint32)(addr)))
#define REG_SET_OR(addr,val)  (_out(  _in(((uint32)addr)) | (uint32)(val),(uint32)(addr)))
//bit set
#define REG_BITS_SET(addr,bit,msk,val) \
(_out(_in(((uint32)addr)) & ((uint32)~(msk)) | (((val)<<(bit))&(msk)), (uint32)(addr)))

#endif




#ifdef CEVA_X
#define timer_clear_watch_dog()
#endif
static int sd_check_timeout(int flg);
#define DRV_SD_READ_MAX 8192
#if (SD_LITTLE_EDIAN == 1)
#define DRV_SD_WRITE_MAX 1024
#else
#define DRV_SD_WRITE_MAX 8192
#endif

#ifdef SDIO_CLK_CMD_DAT0_SEL8_9_10
    #define REG_IO_SD_CLK   REG_GPIO_0x08//GPIO8
    #define REG_IO_SD_DAT   REG_GPIO_0x0A//GPIO10
#else
    #define REG_IO_SD_CLK   REG_GPIO_0x25//GPIO37
    #define REG_IO_SD_DAT   REG_GPIO_0x27//GPIO39
#endif

//sdio init config, clk&data should cfg input monitor func, lvl can be read directly
#define IO_FUNC             0xF0//0xF4:inout&(min drv), 0xF0:output&(min drv), 0x3F0:output&(max drv)
#define IO_FUNC_INPUT       0xBC//0xBC:input & pullup & in monitor

// #define SD_CLK_LVL          (REG_GET(&REG_IO_SD_CLK) & 0x1)//read input level bit
#define SD_DAT0_SET_INPUT   REG_SET_EQU(&REG_IO_SD_DAT, IO_FUNC_INPUT)//config gpio to gpio input mode
#define SD_DAT0_SET_SDIO    REG_SET_EQU(&REG_IO_SD_DAT, IO_FUNC)//config gpio to sdio data mode
#define SD_DAT0_GET_IDLE    (REG_GET(&REG_IO_SD_DAT) & 0x1)//read input level bit

static uint32_t sdcard_t_ref;
void sdcard_timeout_init(void)
{
    sdcard_t_ref = sys_time_get();
}           

int sdcard_timeout(int over_tim_ms)
{
    if((sys_time_get() - sdcard_t_ref) >= over_tim_ms) return 1;
    else return 0;
}

//ms: 0 ~ 165000, convert time(ms) to clock cycle num
int sdio_timer_ms_2_cycle(int ms)
{
    int clkdiv = (REG_GET(&REG_SYSTEM_0x01) & MSK_SYSTEM_0x01_SDIO_DIV) >> SFT_SYSTEM_0x01_SDIO_DIV;
    int clk_KHz = (clkdiv == CLK_203125Hz) ? (203125 / 1000) : ((26000000 / 1000) >> clkdiv);
    return ((int)clk_KHz * ms);
}

typedef enum _SD_HOST_ST_e{
    ST_SD_IDLE,

    ST_SD_WR_MBLK_START,
    ST_SD_WR_MBLK_TRANS_START,
    ST_SD_WR_MBLK_TRANS_CHK,
    ST_SD_WR_MBLK_SD_PROG,

    ST_SD_RD_MBLK_START,
    ST_SD_RD_MBLK_TRANS_START,
    ST_SD_RD_MBLK_TRANS_CHK,
    ST_SD_RD_MBLK_SD_PROG, //#ifdef SDIO_TRANS_MIN_STOP
}SD_HOST_ST_e;

#ifdef SDIO_TRANS_MIN_STOP
typedef enum _FG_SD_RW_OP_e{
    FG_SD_RW_OP_INIT,
    FG_SD_RW_OP_READ,
    FG_SD_RW_OP_WRITE,
}FG_SD_RW_OP_e;
#endif


static void *sdio_trans_dma = NULL;

typedef void (*CBK_SDIO_DMA)(int res);
typedef void (*CBK_SD_INSERT_EVT)(void);

#ifdef CONFIG_SDCARD_DETECT
static CBK_SD_INSERT_EVT s_cbk_sd_insert_in = NULL;
static CBK_SD_INSERT_EVT s_cbk_sd_pull_out = NULL;
#endif

#define CLK_ENABLE  REG_SET_AND(&REG_SYSTEM_0x05, ~MSK_SYSTEM_0x05_SDIO_PWD)
#define CLK_DISABLE REG_SET_OR(&REG_SYSTEM_0x05, MSK_SYSTEM_0x05_SDIO_PWD)
#define CLK_CHECK() (!(REG_GET(&REG_SYSTEM_0x05) & MSK_SYSTEM_0x05_SDIO_PWD))

#define REG_READ(addr)          REG_GET(addr)
#define REG_WRITE(addr, _data)  REG_SET_EQU(addr, _data)


static SDCARD_S sdcard;
static uint8_t SDOnline = SD_CARD_OFFLINE;
#ifdef CONFIG_SDCARD_DETECT
static uint8_t SD_det_gpio_flag=0;
static uint8_t SD_detect_pin=0;
static uint8_t SD_detect_level=0;
#endif
static uint16_t Sd_MMC_flag = 0;

static uint32_t op_start_block;
static SD_HOST_ST_e sd_host_state = ST_SD_IDLE;
#ifdef SDIO_TRANS_MIN_STOP
static uint32_t op_addr_pending = 0xFFFFFFFF;//last op block
static FG_SD_RW_OP_e fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;//last rw opration is read or write
#endif

#ifdef TEST_SDCARD_RW
static volatile uint8_t gs_sd_write_cnt = 0;
#endif
static uint64_t gs_sd_buf_addr=0;
uint64_t g_sd_capacity=0;
uint64_t g_sd_pic_addr=0;

// #define bk_bt_run_immediately(...)
extern void bk_bt_run_immediately(const char *func_name,uint8_t main_flag);
void drv_sd_info_update(int dir) {}

static int sdcard_read_data_dma(uint8_t* dma_buf, uint32_t dma_sz, void* cbk);
static int sdcard_write_data_dma(uint8_t* dma_buf, uint32_t dma_sz, void* cbk);
#ifdef CONFIG_SDCARD_DETECT
static void sd_detect_fun(void);
#endif

typedef struct
{
    uint8 *data_buf;
    uint32 cnt_max;
    uint32 cnt_add;
    uint8 *dma_buf;
    uint32 dma_size;
    CBK_SDIO_DMA cbk;
}SD_DMA_DATA_S;

static volatile SD_DMA_DATA_S gs_sd_dma_read_data = {0};
static volatile SD_DMA_DATA_S gs_sd_dma_write_data = {0};


uint8_t *gs_dma_memory = NULL;



void sdio_gpio_config(void)
{
#ifdef CONFIG_APP_SDCARD_4_LINE// data 1 2 3 enable
	gpio_enable_second_function(GPIO_FUNCTION_SDIO_DATA1_3_ENABLE);
	REG_SET_OR(&REG_SYSTEM_0x1A, ((0x03 <<((34-32)*2)) | (0x03 <<((35-32)*2)) | (0x03 <<((36-32)*2))));
#endif

#ifdef SDIO_CLK_CMD_DAT0_SEL8_9_10 //sdio sel gpio8~10
    //clk,cmd,dat gpio8~10 config enable extended functions
    // gpio_enable_second_function(GPIO_FUNCTION_SDIO);
    REG_SET_EQU(&REG_GPIO_0x08, IO_FUNC);
    REG_SET_EQU(&REG_GPIO_0x09, IO_FUNC);
    REG_SET_EQU(&REG_GPIO_0x0A, IO_FUNC);

	//gpio8~10 extend sdio function perial mode config(sdio -> func1 -> 0x0)
	REG_SET_AND(&REG_SYSTEM_0x18, (~((0x03 << 16) | (0x03 << 18) | (0x03 << 20))));

	//sdio io position choose gpio8~10
	REG_SET_AND(&REG_SYSTEM_0x1A, ~MSK_SYSTEM_0x1A_SDIO_POS);
#else //sdio sel gpio37~39
    //clk,cmd,dat gpio37~39 config enable extended functions
    REG_SET_EQU(&REG_GPIO_0x25, IO_FUNC);
    REG_SET_EQU(&REG_GPIO_0x26, IO_FUNC);
    REG_SET_EQU(&REG_GPIO_0x27, IO_FUNC);

	//gpio37~39 extend function perial mode config(sdio -> func4 -> 0x3)
	REG_SET_OR(&REG_SYSTEM_0x1A, ((0x03 << 10) | (0x03 << 12) | (0x03 << 14)));

	//sdio io position choose gpio37~39
	REG_SET_OR(&REG_SYSTEM_0x1A, MSK_SYSTEM_0x1A_SDIO_POS);
#endif

#ifndef CEVA_X
	system_peri_mcu_irq_disable(SYS_PERI_IRQ_SDIO);
	system_peri_mcu_irq_enable(SYS_PERI_IRQ_GENER_DMA);
#else
	system_peri_dsp_irq_disable(SYS_PERI_IRQ_SDIO);
	system_peri_dsp_irq_enable(SYS_PERI_IRQ_GENER_DMA);
#endif
}

void sdio_gpio_uninit(void)
{
#ifdef SDIO_CLK_CMD_DAT0_SEL8_9_10 //sdio sel gpio8~10
    REG_SET_EQU(&REG_GPIO_0x08, 0x328);
    REG_SET_EQU(&REG_GPIO_0x09, 0x328);
    REG_SET_EQU(&REG_GPIO_0x0A, 0x328);
#else
    REG_SET_EQU(&REG_GPIO_0x25, 0x328);
    REG_SET_EQU(&REG_GPIO_0x26, 0x328);
    REG_SET_EQU(&REG_GPIO_0x27, 0x328);
#endif

#ifdef CONFIG_APP_SDCARD_4_LINE // data 1 2 3 enable
    REG_SET_EQU(&REG_GPIO_0x22, 0x328);
    REG_SET_EQU(&REG_GPIO_0x23, 0x328);
    REG_SET_EQU(&REG_GPIO_0x24, 0x328);
#endif
}

void sdio_register_reset(void)
{
    uint32_t reg;

    /* Clear cmd rsp int bit */
    reg = REG_SDCARD_CMD_RSP_INT_SEL;

    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL, reg);

    /* Clear tx/rx fifo */
    reg = SDCARD_FIFO_RX_FIFO_RST | SDCARD_FIFO_TX_FIFO_RST;
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg);

    /* Disabe all sdio interrupt */
    reg = 0;
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_MASK, reg);

    /* Config tx/rx fifo threshold */
    reg = ((SDCARD_RX_FIFO_THRD & SDCARD_FIFO_RX_FIFO_THRESHOLD_MASK)
           << SDCARD_FIFO_RX_FIFO_THRESHOLD_POSI)
          | ((SDCARD_TX_FIFO_THRD & SDCARD_FIFO_TX_FIFO_THRESHOLD_MASK)
             << SDCARD_FIFO_TX_FIFO_THRESHOLD_POSI);
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg);
}

/*
  0 -- 26M
  1 -- 13M
  2 -- 6.5M
  3 -- 203.125KHz
*/
static void beken_sdcard_set_clk_div(uint8_t clkdiv)
{

    os_delay_us(10);

    /*Config sdio clock*/
    uint32_t tmp =REG_GET(&REG_SYSTEM_0x01);
    tmp&= ~MSK_SYSTEM_0x01_SDIO_DIV;
    tmp|= clkdiv<<SFT_SYSTEM_0x01_SDIO_DIV;
    REG_SET_EQU(&REG_SYSTEM_0x01, tmp);
    /* test set invalid for clk and sdio timer, why? by Borg @230329.
    uint32_t reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg | ((SDCARD_FIFO_SD_RATE_SELECT_MASK & 1) << SDCARD_FIFO_SD_RATE_SELECT_POSI));//*/
}

void sdio_set_low_clk(void)
{
    beken_sdcard_set_clk_div(CLK_203125Hz);
}

void sdio_set_high_clk(void)
{
    beken_sdcard_set_clk_div(CLK_13M);
}

void sdio_clk_config(uint32_t enable)
{
    (enable) ? CLK_ENABLE : CLK_DISABLE;
}

void sdio_sendcmd_function(uint8_t cmd_index, uint32_t flag, uint32_t timeout, void *arg)
{
    uint32_t reg;
    flag &= CMD_FLAG_MASK;

    reg = (uint32_t)arg;
    REG_WRITE(REG_SDCARD_CMD_SEND_AGUMENT, reg);

    reg = timeout;
    REG_WRITE(REG_SDCARD_CMD_RSP_TIMER, reg);
    reg = ((((uint32_t)cmd_index)&SDCARD_CMD_SEND_CTRL_CMD_INDEX_MASK)<< SDCARD_CMD_SEND_CTRL_CMD_INDEX_POSI)
        | ((flag & SDCARD_CMD_SEND_CTRL_CMD_FLAGS_MASK)<< SDCARD_CMD_SEND_CTRL_CMD_FLAGS_POSI)
        | SDCARD_CMD_SEND_CTRL_CMD_START;
//  reg=BFD(cmd_index,SDCARD_CMD_SEND_CTRL_CMD_INDEX_POSI,6)
//      |BFD(flag,SDCARD_CMD_SEND_CTRL_CMD_FLAGS_POSI,3)
//      |SDCARD_CMD_SEND_CTRL_CMD_START;
//  reg = ((((uint32_t)cmd_index)&SDCARD_CMD_SEND_CTRL_CMD_INDEX_MASK)
//      << SDCARD_CMD_SEND_CTRL_CMD_INDEX_POSI)
//      | ((flag & SDCARD_CMD_SEND_CTRL_CMD_FLAGS_MASK)
//      << SDCARD_CMD_SEND_CTRL_CMD_FLAGS_POSI)
//      | SDCARD_CMD_SEND_CTRL_CMD_START;
    REG_WRITE(REG_SDCARD_CMD_SEND_CTRL, reg);
}

SDIO_Error sdio_wait_cmd_response(uint32_t cmd)
{
    uint32_t reg;
    while(1)
    {
        bk_bt_run_immediately(__func__,0);
        reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
        //wait until cmd response
        if(reg & (SDCARD_CMDRSP_NORSP_END_INT
                  | SDCARD_CMDRSP_RSP_END_INT
                  | SDCARD_CMDRSP_TIMEOUT_INT) )
        {
            break;
        }

    }

    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL, SD_CMD_RSP);//clear the int flag
    if((reg & SDCARD_CMDRSP_TIMEOUT_INT) /*||(reg&SDCARD_CMDRSP_NORSP_END_INT)*/ )
    {
        uint32_t reg_val = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
        REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg_val & MSK_SD_CMD_DAT_STUS);//sd state&fifo reset， add by Borg@230329 for V1.x SD nonrecognition issue(reason:not reset after CMD8 timeout)
        //if((cmd != 1))
        {
            SD_LOG_W("sd cmd%d timeout, rsp_int:0x%08X\n", cmd, reg);
        }

        return SD_CMD_RSP_TIMEOUT;
    }
    if(reg & SDCARD_CMDRSP_CMD_CRC_FAIL)
    {
        if((cmd != 41) && (cmd != 2) && (cmd != 9) && (cmd != 1))
        {
            SD_LOG_W("sdcard cmd %d crcfail, cmdresp_int_reg:0x%x\r\n", cmd , reg);
            return SD_CMD_CRC_FAIL;
        }
    }
    return SD_OK;
}

void sdio_get_cmdresponse_argument(uint8_t num, uint32_t *resp)
{
    switch(num)
    {
    case 0:
        *resp = REG_READ(REG_SDCARD_CMD_RSP_AGUMENT0);
        break;
    case 1:
        *resp = REG_READ(REG_SDCARD_CMD_RSP_AGUMENT1);
        break;
    case 2:
        *resp = REG_READ(REG_SDCARD_CMD_RSP_AGUMENT2);
        break;
    case 3:
        *resp = REG_READ(REG_SDCARD_CMD_RSP_AGUMENT3);
        break;
    default:
        break;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// This block is used for SDIO DMA
void sdio_dma_init(void)
{
    #define DMA_IDX(dma)      (((uint32_t)(dma) - MDU_GENER_DMA_BASE_ADDR) / 0x20)
    SD_LOG_I("sdio dma init\r\n");
    if(sdio_trans_dma == NULL) sdio_trans_dma = dma_channel_malloc();
    if(sdio_trans_dma == NULL)
    {
        SD_LOG_E("sdio dma malloc failed\r\n");
        while(1);
    }
    SD_LOG_I("sdio use DMA%d: 0x%p\n", DMA_IDX(sdio_trans_dma), sdio_trans_dma);
}

void sdio_dma_disable(void)
{
    dma_channel_disable_interrupt(sdio_trans_dma, DMA_INT_BOTH);
    dma_channel_set_int_callback(sdio_trans_dma, NULL, NULL);
    dma_channel_enable(sdio_trans_dma, false);
}

//shoule call in sd pull out callback
void sdio_dma_uninit(void)
{
    if(sdio_trans_dma) dma_channel_free(sdio_trans_dma);
    sdio_trans_dma = NULL;
    #if (SD_LITTLE_EDIAN == 1)
    if(gs_dma_memory) { jfree(gs_dma_memory); gs_dma_memory = NULL; }
    #endif
}

static int sd_check_timeout(int flg)
{
    static int s_t1 = 0;
    if(flg == 0)
    {
        s_t1 = 0;
    }
    else if(s_t1++ >= 6000000)//about 1 second
    {
        SDCARD_FATAL("  sd_timeout_err\r\n\r\n");
        return TRUE;
    }
    return FALSE;
}

////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void driver_sdcard_recv_data_start(int timeout )
{
    REG_WRITE(REG_SDCARD_DATA_REC_TIMER, timeout);
#ifdef CONFIG_SDCARD_BUSWIDTH_4LINE
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL, (0x1 | (1 << 2) | (512 << 4) | ((SD_LITTLE_EDIAN==1)?(1<<17):(0))));
#else
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL, (0x1 | (512 << 4) | ((SD_LITTLE_EDIAN==1)?(1<<17):(0))));
#endif
}


int wait_Receive_Data(void)
{
    uint32_t ret = SD_ERR_LONG_TIME_NO_RESPONS, status = 0;

    sd_check_timeout(0);
    while (1)
    {
        /*if(bk_rtos_get_time() > start_tm + 4000) // 4s
        {
            ret = SD_ERR_LONG_TIME_NO_RESPONS;
            break;
        }*/
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout\r\n",__func__);
            break;
        }

        status = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
        if(status & (SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ))
        {
            ret = SD_OK;
            break;
        }
        else if (status & SDCARD_CMDRSP_DATA_CRC_FAIL)
        {
            ret = SD_DATA_CRC_FAIL;
            break;
        }
        else if (status & SDCARD_CMDRSP_DATA_TIME_OUT_INT)
        {
            ret = SD_DATA_TIMEOUT;
            break;
        }
    }
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL, SD_DATA_RSP);/*< clear the int flag */
    return ret;
}

void sdio_set_data_timeout(uint32_t timeout)
{
    REG_WRITE(REG_SDCARD_DATA_REC_TIMER, timeout);
}

uint8_t sd_is_attached(void)
{
#ifdef CONFIG_APP_SDCARD
#ifdef CONFIG_SDCARD_DETECT
    return (SDOnline == SD_CARD_ONLINE);
#else
    return SD_CARD_ONLINE;
#endif
#else
    return SD_CARD_OFFLINE;
#endif
}

uint32_t sdcard_get_total_block(void)
{
    return sdcard.total_block;
}

static uint16_t NoneedInitflag = 0;

static void sdio_hw_init(void)
{
    //uint32_t tmp;

    //select sdio clk source :XTAL
    //tmp = REG_SYSTEM_0x00;
    //tmp &= ~ (MSK_SYSTEM_0x00_CORE_DIV | MSK_SYSTEM_0x00_CORE_SEL);
    //tmp |= (1 << 7) ;
    //REG_SYSTEM_0x00 = tmp;

    //close mem sd
    REG_SET_AND(&REG_SYSTEM_0x21, (~MSK_SYSTEM_0x21_SDIO_MEM_SD));
    //REG_SYSTEM_0x06 =  0xFFFFFFFF;
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SDIO);

    /* config sdcard gpio */
    sdio_gpio_config();

    /* reset sdcard moudle register */
    sdio_register_reset();

    /* set sdcard low clk */
    sdio_set_low_clk();

    /* set sdcard  clk enable*/
    sdio_clk_config(1);
}

static void sdio_hw_uninit(void)
{
    os_printf("sdio_hw_uninit\n");
    sdio_clk_config(0);
    sdio_register_reset();
    sdio_gpio_uninit();
	system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_SDIO);
    REG_SET_OR(&REG_SYSTEM_0x21, MSK_SYSTEM_0x21_SDIO_MEM_SD);
}

static void sdio_send_cmd(SDIO_CMD_PTR sdio_cmd_ptr)
{
    sdio_sendcmd_function(sdio_cmd_ptr->index,
                          sdio_cmd_ptr->flags,
                          sdio_cmd_ptr->timeout,
                          (void *)sdio_cmd_ptr->arg);
}


static void sdio_sw_init(void)
{
    os_memset((void *)&sdcard, 0, sizeof(SDCARD_S));
#ifdef CONFIG_SDCARD_DETECT
    sdcard.detect_func = sd_detect_fun;
#endif
}

/******************************************************************************/
/***************************** sdcard function ********************************/
/******************************************************************************/
/* GO_IDLE_STATE */
static SDIO_Error sdcard_cmd0_process(void)
{
    SDCARD_DEBUG("send cmd0 \r\n");

    SDIO_CMD_S cmd;

    cmd.index = GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = SD_CMD_NORESP;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);

    cmd.err = sdio_wait_cmd_response(cmd.index);
    return cmd.err;
}

/*GO_ACTIVATE*/
static SDIO_Error sdcard_cmd1_process(void)
{
    SDIO_CMD_S cmd;
    uint32_t response, reg;

    cmd.index = 1;
    cmd.arg = 0x40ff8000;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = sdio_timer_ms_2_cycle(5);;//DEF_CMD_TIME_OUT;
cmd1_loop:
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    if(cmd.err == SD_OK)
    {
        sdio_get_cmdresponse_argument(0, &response);
        if(!(response & OCR_MSK_VOLTAGE_ALL))
            cmd.err = SD_ERR_CMD41_CNT;
        if(!(response & OCR_MSK_BUSY))
            goto cmd1_loop;
        if(response & OCR_MSK_HC)
            sdcard.Addr_shift_bit = 0;
        else
            sdcard.Addr_shift_bit = 9;
    }
    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    reg |= 20;
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg);

    return cmd.err;
}

static SDIO_Error sdcard_mmc_cmd8_process(void)
{
    int i;
    SDIO_CMD_S cmd;

    uint32_t tmp;
    uint8_t *tmpptr = (uint8_t *)jmalloc(512,M_ZERO);
    if(tmpptr == NULL)
        return 1;
    os_memset(tmpptr, 0, 512);

    cmd.index = SEND_IF_COND;
    cmd.arg = 0;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT; // DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);

    if (cmd.err != SD_OK)
        goto freebuf;
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, (1 << 20));// reset first
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, 0x3ffff);// set fifo later
    driver_sdcard_recv_data_start(DEF_LOW_SPEED_CMD_TIMEOUT);

    tmp = 0;
    cmd.err = wait_Receive_Data();
    if(cmd.err == SD_OK)
    {
        for (i = 0; i < 128; i++)
        {
            while(!(REG_READ(REG_SDCARD_FIFO_THRESHOLD) & (0x1 << 18)))
            {
                tmp++;
                if(tmp > 0x20)
                    break;
            }

            *((uint32_t *)tmpptr + i) = REG_READ(REG_SDCARD_RD_DATA_ADDR);
        }
        sdcard.total_block = tmpptr[212] | (tmpptr[213] << 8) | (tmpptr[214] << 16) | (tmpptr[215] << 24);
    }

freebuf:
    jfree(tmpptr);
    return cmd.err;
}

static SDIO_Error sdcard_cmd8_process(void)
{
    SDIO_CMD_S cmd;
    uint8_t voltage_accpet, check_pattern;

    cmd.index = SEND_IF_COND;
    cmd.arg = 0x1AA;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = sdio_timer_ms_2_cycle(5);//DEF_LOW_SPEED_CMD_TIMEOUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    if(cmd.err == SD_CMD_RSP_TIMEOUT)
    {
        SD_LOG_W("cmd8 no rsp, voltage mismatch or Ver1.X SD or not SD\r\n");
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(cmd.err == SD_CMD_CRC_FAIL)
    {
        SD_LOG_W("cmd8 cmdcrc err\r\n");
        return SD_CMD_CRC_FAIL;
    }

    SD_LOG_I("found a Ver2.00 or later SDCard\r\n");

    // check Valid Response,
    // R7-[11:8]:voltage accepted, [7:0] echo-back of check pattern
    sdio_get_cmdresponse_argument(0, &cmd.resp[0]);

    check_pattern = cmd.resp[0] & 0xff;
    voltage_accpet = cmd.resp[0] >> 8 & 0xf;

    if(voltage_accpet == 0x1 && check_pattern == 0xaa)
    {
        SD_LOG_I("support 2.7~3.6V\r\n");
        return SD_OK;
    }
    else
    {
        SD_LOG_I("unsupport voltage\r\n");
        return SD_INVALID_VOLTRANGE;
    }
    return SD_OK;
}

/*Send host capacity support information(HCS) and  asks
  the card to send its OCR in the response on CMD line*/
static SDIO_Error sdcard_acmd41_process(uint32_t ocr)
{
    SDIO_CMD_S cmd;


    cmd.index = APP_CMD;
    cmd.arg = 0;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT; // DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    if(cmd.err != SD_OK)
    {
        SD_LOG_W("send cmd55 err:%d\r\n", cmd.err);
        return cmd.err;
    }


    cmd.index = SD_APP_OP_COND;
    cmd.arg = ocr;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    // why cmd41 always return crc fail?
    if(cmd.err != SD_OK && cmd.err != SD_CMD_CRC_FAIL)
    {
        SD_LOG_W("send cmd41 err:%d\r\n", cmd.err);
        return cmd.err;
    }

    return SD_OK;
}

/*ask the CID number on the CMD line*/
// Manufacturer ID          MID     8   [127:120]
// OEM/Application          ID  OID 16  [119:104]
// Product name             PNM     40  [103:64]
// Product revision         PRV     8   [63:56]
// Product serial number    PSN     32  [55:24]
// reserved                 --      4   [23:20]
// Manufacturing date       MDT     12  [19:8]
static SDIO_Error sdcard_cmd2_process(void)
{
    SDIO_CMD_S cmd;

    cmd.index = ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = SD_CMD_LONG;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT; // DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    // dismiss the CID info

    return cmd.err;
}

static SDIO_Error sdcard_mmc_cmd3_process(void)
{
    SDIO_CMD_S cmd;

    cmd.index = SEND_RELATIVE_ADDR;
#if 0
    cmd.arg = 0;
#else
    sdcard.card_rca = 1;
    cmd.arg = (sdcard.card_rca << 16);
#endif
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    if(cmd.err == SD_CMD_RSP_TIMEOUT)
    {
        SDCARD_WARN("mmc cmd3 noresp \r\n");
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(cmd.err == SD_CMD_CRC_FAIL)
    {
        SDCARD_WARN("mmc cmd3 cmdcrc err\r\n");
        return SD_CMD_CRC_FAIL;
    }

#if 0
    sdio_get_cmdresponse_argument(0, &cmd.resp[0]);
    sdcard.card_rca = (UINT16) (cmd.resp[0] >> 16);
#endif
    SDCARD_PRT("mmc cmd3 is ok, card rca:0x%x\r\n", sdcard.card_rca);
    return SD_OK;
}

/*ask the card to publish a new RCA*/
static SDIO_Error sdcard_cmd3_process(void)
{
    SDIO_CMD_S cmd;

    cmd.index = SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    if(cmd.err == SD_CMD_RSP_TIMEOUT)
    {
        SDCARD_WARN("cmd3 noresp \r\n");
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(cmd.err == SD_CMD_CRC_FAIL)
    {
        SDCARD_WARN("cmd3 cmdcrc err\r\n");
        return SD_CMD_CRC_FAIL;
    }

    sdio_get_cmdresponse_argument(0, &cmd.resp[0]);
    sdcard.card_rca = (uint16_t) (cmd.resp[0] >> 16);
    SDCARD_PRT("cmd3 is ok, card rca:0x%x\r\n", sdcard.card_rca);
    return SD_OK;
}

#define SD_CARD 0
#define MMC_CARD 1
/*get CSD Register content*/
static SDIO_Error sdcard_cmd9_process(uint8_t card_type)
{
    SDIO_CMD_S cmd;
    int mult, csize;

    cmd.index = SEND_CSD;
    cmd.arg = (uint32_t)(sdcard.card_rca << 16);
    cmd.flags = SD_CMD_LONG;
    cmd.timeout = DEF_HIGH_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    if(cmd.err != SD_OK)
    {
        return cmd.err;
    }

    sdio_get_cmdresponse_argument(0, &cmd.resp[0]);
    sdio_get_cmdresponse_argument(1, &cmd.resp[1]);
    sdio_get_cmdresponse_argument(2, &cmd.resp[2]);
    // sdio_get_cmdresponse_argument(3, &cmd.resp[3]);

    os_printf("CSD_code(128bit):0x%08X %08X %08X %08X\n", cmd.resp[0], cmd.resp[1], cmd.resp[2], cmd.resp[3]);
    sdcard.block_size = 1 << ((cmd.resp[1] >> 16) & 0xf);
    // int rd_block_sz = 1 << ((cmd.resp[1] >> 16) & 0xf);//[83:80] READ_BL_LEN, read block size(unit:byte)
    // int wr_block_sz = 1 << ((cmd.resp[3] >> 22) & 0xf);//[25:22] WRITE_BL_LEN, write block size(unit:byte).
    // int blocks_per_sector = (cmd.resp[2] >> 7 ) & 0x7F;//[45:39] blocks of one sector
    // os_printf("rd_block_sz:%d, wr_block_sz:%d, blocks_per_sector:%d\n", rd_block_sz, wr_block_sz, blocks_per_sector);

    if(card_type == SD_CARD)
    {

        if(((cmd.resp[0] >> 30) & 0x3) == 0)
        {
            csize = (((cmd.resp[1] & 0x3FF ) << 2) | ((cmd.resp[2] >> 30 ) & 0x3));//[73:62] C_SIZE, device size
            mult  = ( cmd.resp[2] >> 15 ) & 0x7;//[49:47] C_SIZE_MULT, device size multiplier

            sdcard.total_block = (csize + 1 ) * ( 1 << (mult + 2 ) );
            sdcard.total_block *= (sdcard.block_size >> 9);
        }
        else
        {
            csize = (((cmd.resp[1] & 0x3F ) << 16) | ((cmd.resp[2] >> 16 ) & 0xFFFF));
            sdcard.total_block = (csize + 1) * 1024;
        }

    }
    else
    {
        if(sdcard.Addr_shift_bit != 0)
        {
            csize = (((cmd.resp[1] & 0x3FF ) << 2) | ((cmd.resp[2] >> 30 ) & 0x3));//[73:62] C_SIZE, device size
            mult = (cmd.resp[2] >> 15 ) & 0x7;//[49:47] C_SIZE_MULT, device size multiplier

            sdcard.total_block = (csize + 1 ) * ( 1 << (mult + 2 ) );
            sdcard.total_block *= (sdcard.block_size >> 9);
        }
        else
        {
            sdcard.total_block = 0;
        }
    }
    sdcard.block_size = SD_DEFAULT_BLOCK_SIZE;
    
    g_sd_capacity = (uint64_t)sdcard.total_block*sdcard.block_size;
    g_sd_pic_addr = g_sd_capacity - 64*1024*1024;
    
    SDCARD_PRT("Bsize:%x;Total_block:%x\r\n", sdcard.block_size, sdcard.total_block);

    return SD_OK;
}


/*select/deselect card*/
static SDIO_Error sdcard_cmd7_process(void)
{
    SDIO_CMD_S cmd;

    cmd.index = SELECT_CARD;
    cmd.arg = (uint32_t)(sdcard.card_rca << 16);
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_HIGH_SPEED_CMD_TIMEOUT;// DEF_CMD_TIME_OUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    return cmd.err;
}

/*set bus width*/
static SDIO_Error sdcard_acmd6_process(void)
{
    SDIO_CMD_S cmd;
    cmd.index = APP_CMD;
    cmd.arg = (uint32_t)(sdcard.card_rca << 16);
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_HIGH_SPEED_CMD_TIMEOUT;// DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    if(cmd.err != SD_OK)
        return cmd.err;

    cmd.index = SWITCH_FUNC;
#ifdef CONFIG_SDCARD_BUSWIDTH_4LINE
    cmd.arg = 2;
#else
    cmd.arg = 0;
#endif
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout =  DEF_HIGH_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;

    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    return cmd.err;
}

/**************************************************
static SDIO_Error sdcard_cmd18_process(uint32_t addr)
{
    SDIO_CMD_S cmd;

    cmd.index = 18;
    cmd.arg = addr;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    return cmd.err;
}
*****************************************************/
static SDIO_Error sdcard_cmd12_process(uint32_t addr)
{
    SDIO_CMD_S cmd;

    cmd.index = 12;
    cmd.arg = addr;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_HIGH_SPEED_CMD_TIMEOUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    //after cmd12, should judge sd busy, avoid delay in rd/wr block(cmd18/cmd25)
    return cmd.err;
}

/***********************************************
static SDIO_Error sdcard_cmd17_process(uint32_t addr)
{
    SDIO_CMD_S cmd;

    cmd.index = 17;
    cmd.arg = addr;
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_LOW_SPEED_CMD_TIMEOUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);
    return cmd.err;
}
**************************************************/


void sdcard_uninitialize(void)
{
    sdio_hw_uninit();
    sdio_sw_init();
    NoneedInitflag = 0;
}

void sdcard_get_card_info(SDCARD_S *card_info)
{
    card_info->total_block = sdcard.total_block;
    card_info->block_size = sdcard.block_size;
    card_info->card_rca = sdcard.card_rca;
    card_info->init_flag = sdcard.init_flag;
    card_info->Addr_shift_bit = sdcard.Addr_shift_bit;
}

uint8_t sdcard_is_busy(void)
{
    uint8_t sd_is_busy = 0;
    SD_DAT0_SET_INPUT;
    sd_is_busy = !SD_DAT0_GET_IDLE;
    if(!sd_is_busy) SD_DAT0_SET_SDIO;
    return sd_is_busy;
}

static void sdcard_read_data_init(int blocknum)
{
    uint32_t reg;


    SD_DAT0_SET_INPUT;
    sd_check_timeout(0);
    while(!SD_DAT0_GET_IDLE)
    {
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout\r\n",__func__);
            break;
        }
    }
    SD_DAT0_SET_SDIO;

    //SDCARD_PRT("sdcard initial\r\n");
    sdio_clk_config(1);
    reg  = DEF_HIGH_SPEED_CMD_TIMEOUT * blocknum;
    REG_WRITE(REG_SDCARD_DATA_REC_TIMER,reg);
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,0xFFFFFFFF);
    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    reg &= ~(0xFFFF | (1 << 16) | (1 << 20));
    reg  |= (0x0101 | ((1 << 16) | (1 << 20)));
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);

#ifdef CONFIG_SDCARD_BUSWIDTH_4LINE
    reg = 0x1|(1 << 2)|(1 << 3)|(512 << 4)|((SD_LITTLE_EDIAN==1)?(1<<17):(0));
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL,reg);
#else
    reg = 0x1|(0 << 2)|(1 << 3)|(512 << 4)|((SD_LITTLE_EDIAN==1)?(1<<17):(0));
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL,reg);
#endif
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,(SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ));
    // last_read_op=1;
    // SDIO_WR_flag = SDIO_RD_DATA;
}
static SDIO_Error sdcard_cmd18_process(int block_addr)
{
    SDIO_CMD_S cmd;
    int Ret = SD_OK;

    cmd.index = 18;
    cmd.arg = (uint32_t)(block_addr << sdcard.Addr_shift_bit);
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout = DEF_HIGH_SPEED_CMD_TIMEOUT;// DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);
    cmd.err = sdio_wait_cmd_response(cmd.index);

    Ret = cmd.err;
    return Ret;
}

void sdcard_print_reg(void)
{
    int i;
    //uint32 reg;
    os_printf("\r\n");
    for(i=0; i<14; i++)
    {
        if((i!=11) && (i!=12))
        {
            os_printf(" r%d:%x ", i,REG_READ(SDCARD_BASE_ADDR+i*4));
        }
    }
    os_printf("\r\n");
}

static SDIO_Error sdcard_send_read_stop(void)
{
    SDCARD_PRT("%s\r\n",__func__);
    //send stop command
    int reg,Ret;
    Ret = 0;

    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    reg &= ~(0xFFFF | (1 << 16) | (1 << 20));
    reg |= (0x0101 | ((1 << 16) | (1 << 20)));
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);
    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    reg &= ~(0xFFFF | (1 << 16) | (1 << 20));
    reg |= (0x0101 | ((1 << 16) | (1 << 20)));
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);
    Ret = sdcard_cmd12_process(DEF_HIGH_SPEED_CMD_TIMEOUT);
    
    
    //close clock
    //sdio_clk_config(0);
    return Ret;
}

static SDIO_Error sdcard_cmd25_process(int block_addr)
{
    SDIO_CMD_S cmd;
    int ret = SD_OK;
    uint32_t reg;


    SD_DAT0_SET_INPUT;
    sd_check_timeout(0);
    while(!SD_DAT0_GET_IDLE)
    {
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout\r\n",__func__);
            break;
        }
    }
    SD_DAT0_SET_SDIO;

    sdio_clk_config(1);
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL, 0xffffffff);
    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);

    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD, reg|SDCARD_FIFO_SD_STA_RST);

    reg |= (0x0101 | SDCARD_FIFO_TX_FIFO_RST);
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);

    cmd.index = 25;//WRITE_MULTIPLE_BLOCK;
    cmd.arg = (uint32_t)(block_addr << sdcard.Addr_shift_bit);
    cmd.flags = SD_CMD_SHORT;
    cmd.timeout =  DEF_HIGH_SPEED_CMD_TIMEOUT;//DEF_CMD_TIME_OUT;
    sdio_send_cmd(&cmd);

    cmd.err = sdio_wait_cmd_response(cmd.index);

    ret = cmd.err;
    return ret;
}



static SDIO_Error sdcard_send_write_stop(int err)
{
    int reg,ret;
    SDCARD_PRT("%s\r\n",__func__);


// 3. after the last block,write zero
    sd_check_timeout(0);
    while(1)
    {
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout1\r\n",__func__);
            break;
        }

        reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
        if(reg & SDCARD_FIFO_TXFIFO_WR_READY)
        {
            REG_WRITE(REG_SDCARD_WR_DATA_ADDR, 0);
            break;
        }
    }
    // 4.wait and clear flag
    sd_check_timeout(0);
    do
    {
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout2\r\n",__func__);
            break;
        }
        reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
        if(reg &(SDCARD_CMDRSP_DATA_BUSY | SDCARD_CMDRSP_DATA_WR_END_INT))
            break;
    }while(1);  //BUSY

    if((reg&SDCARD_CMDRSP_DATA_BUSY))
    {
        reg = REG_READ(REG_SDCARD_CMD_RSP_INT_MASK);
        reg &= ~SDCARD_CMDRSP_TX_FIFO_EMPTY_MASK;
        REG_WRITE(REG_SDCARD_CMD_RSP_INT_MASK,reg);
    }
    if((reg&SDCARD_CMDRSP_DATA_WR_END_INT))
    {
        REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,SDCARD_CMDRSP_DATA_WR_END_INT);
    }

    reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
    if(2 != ((reg & SDCARD_CMDRSP_WR_STATU)>>20))
    {
        ret =  SD_ERROR;
    }
    else
    {
        ret =  SD_OK;
    }

    reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
    reg |= SDCARD_FIFO_TX_FIFO_RST;
    REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);
    reg = REG_READ(REG_SDCARD_CMD_RSP_INT_MASK);
    reg &= ~SDCARD_CMDRSP_TX_FIFO_EMPTY_MASK;
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_MASK,reg);

    SD_DAT0_SET_INPUT;
    sd_check_timeout(0);
    while(!SD_DAT0_GET_IDLE)
    {
        if(sd_check_timeout(1))
        {
            SDCARD_FATAL("%s,timeout3\r\n",__func__);
            break;
        }
    }
    SD_DAT0_SET_SDIO;

    ret += sdcard_cmd12_process(DEF_HIGH_SPEED_CMD_TIMEOUT);
    if(ret != SD_OK)
    {
        SDCARD_FATAL("===write err:%x,====\r\n",ret);
    }
    ret += err;
    return ret;
}




void dma_rx_cbk(void *dma,DMA_INTERRUPT_TYPE type,void *uarg)
{    
    //os_printf("%s\r\n",__func__);

}
void dma_tx_cbk(void *dma,DMA_INTERRUPT_TYPE type,void *uarg)
{   
    //os_printf("%s\r\n",__func__);
    
}


static int sdcard_read_data_dma(uint8_t* dma_buf, uint32_t dma_sz, void* cbk)
{
    if(sdio_trans_dma == NULL)
    {
        SDCARD_FATAL("dma:%p", sdio_trans_dma);
        return SD_ERROR;
    }

    cbk = NULL; //dma no need callback
    dma_channel_disable_interrupt(sdio_trans_dma, DMA_INT_BOTH);
    dma_channel_enable(sdio_trans_dma, false);
    dma_channel_set_int_callback(sdio_trans_dma, cbk, NULL);  
    dma_channel_config(sdio_trans_dma,
                        DMA_REQ_SDIO_RX,
                        DMA_MODE_SINGLE,
                        (uint32_t)&REG_SDIO_HOST_0x0C,
                        (uint32_t)&REG_SDIO_HOST_0x0C,
                        DMA_ADDR_NO_CHANGE,
                        DMA_DATA_TYPE_LONG,
                        (uint32_t)dma_buf,
                        (uint32_t)dma_buf + dma_sz,
                        DMA_ADDR_AUTO_INCREASE,
                        DMA_DATA_TYPE_LONG,
                        dma_sz);
    if(cbk) dma_channel_enable_interrupt(sdio_trans_dma, DMA_ALL_FINISH_INT);
    dma_channel_enable(sdio_trans_dma, true);

    return SD_DMA_RUNNING;
}

static int sdcard_read_data_dma_init(uint8_t* read_data, uint32_t block_num, void*cbk)
{   
    SD_DMA_DATA_S *p_dma_read_data = (SD_DMA_DATA_S *)&gs_sd_dma_read_data;
    
    p_dma_read_data->data_buf = read_data;
    p_dma_read_data->cnt_max = block_num<<9;
    p_dma_read_data->cnt_add = 0;
    p_dma_read_data->cbk = cbk;
    if((block_num<<9) > DRV_SD_READ_MAX)
    {
        p_dma_read_data->dma_size = DRV_SD_READ_MAX;
    }
    else
    {
        p_dma_read_data->dma_size = (block_num<<9);
    }

    return SD_OK;
}

extern uint32 dma_get_remain(void* _dma);
static void sdcard_wait_idle(void)
{
    sdcard_timeout_init();
    while(1)
    {
        if ((sdcard_read_dma_check_finish() == TRUE)
        && (sdcard_write_dma_check_finish() == TRUE))
        {
            //os_printf("sdcard pre opration has finish\r\n");
            break;
        }
        else
        {
            if(sdcard_timeout(1000)){
                SD_LOG_E("sd over_time\n");
                break;
            }
        }
    }
    
}

int sdcard_read_multi_block(void*buf,int first_block, int block_num)
{
    return sdcard_read_multi_block_dma((uint8_t *)buf,  first_block,  block_num, NULL);
}

SDIO_Error sdcard_read_multi_block_dma(uint8_t *read_buff, uint32_t first_block, uint32_t block_num,void* cbk)
{
    sdcard_wait_idle();

    drv_sd_info_update(1);
    #ifdef CEVA_X
    read_buff += DSP_RAM_BASIC_ADDR;
    #endif
    SDCARD_PRT("%s,read_buff:%x, first_block:%x, block_num:%d\r\n",__func__,read_buff, first_block, block_num);
    int ret = SD_OK;
    if(first_block >= sdcard_get_total_block())
    {
        SDCARD_FATAL("rd first_block:%x, max_block:%x\r\n", first_block, sdcard_get_total_block());
        return SD_OUT_RANGE;
    }
    if(block_num == 0)
    {
        SDCARD_FATAL("block_num=0\r\n");
        return SD_OK;
    }

    op_start_block = first_block;
    ret += sdcard_read_data_dma_init(read_buff, block_num, cbk);
#ifndef SDIO_TRANS_MIN_STOP
    sd_host_state = ST_SD_RD_MBLK_START;
#else
    if(fg_sd_rw_op_prv == FG_SD_RW_OP_WRITE){ //if last opration is read, stop write and wait sd program cmp.
        sdcard_send_write_stop(0);
        sd_host_state = ST_SD_RD_MBLK_SD_PROG;
    }else if(fg_sd_rw_op_prv == FG_SD_RW_OP_READ){//if last opration is read
        //read continue if address continuous, else need stop read trans and start new read trans.
        if(op_start_block == op_addr_pending){
            sd_host_state = ST_SD_RD_MBLK_TRANS_START;
        }else{
            sdcard_send_read_stop();
        }
    }
    if(sd_host_state == ST_SD_IDLE) sd_host_state = ST_SD_RD_MBLK_START;
    fg_sd_rw_op_prv = FG_SD_RW_OP_READ;
    op_addr_pending = op_start_block + block_num;//record the next pending address
#endif
    if(ret != SD_OK)
    {
        SDCARD_FATAL("err:sdcard_read_multi_block_dma, ret=%d\r\n",ret);
        sd_reinit();
        return ret;
    }
    if(cbk == NULL)
    {
        sdcard_timeout_init();
        while(1)
        {
            if(sdcard_read_dma_check_finish() == TRUE)
            {
                ret = SD_OK;
                break;
            }
            else
            {
                if(sdcard_timeout(1000)){
                    ret = SD_DATA_TIMEOUT;
                    SD_LOG_E("sd rd over_time\n");
                    break;
                }
            }
        } 
    }
    //app need check finish
    return ret;
}

static int sdcard_write_data_dma(uint8_t* dma_buf, uint32_t dma_sz,void*cbk)
{
    int reg;

    if(sdio_trans_dma == NULL)
    {
        SDCARD_FATAL("dma:%p", sdio_trans_dma);
        return SD_ERROR;
    }

    reg = REG_READ(REG_SDCARD_DATA_REC_CTRL);
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL, reg|SDCARD_DATA_REC_CTRL_DATA_WR_DATA_EN);

    cbk = NULL; //dma no need callback
    dma_channel_set_int_callback(sdio_trans_dma, cbk, NULL);
    dma_channel_disable_interrupt(sdio_trans_dma, DMA_ALL_FINISH_INT);
    dma_channel_enable(sdio_trans_dma, FALSE);
    dma_channel_config(sdio_trans_dma,
                        DMA_REQ_SDIO_TX,
                        DMA_MODE_SINGLE,
                        (uint32_t)dma_buf,
                        (uint32_t)dma_buf + dma_sz,
                        DMA_ADDR_AUTO_INCREASE,
                        DMA_DATA_TYPE_LONG,
                        (uint32_t)&REG_SDIO_HOST_0x0B,
                        (uint32_t)&REG_SDIO_HOST_0x0B,
                        DMA_ADDR_NO_CHANGE,
                        DMA_DATA_TYPE_LONG,
                        dma_sz);
    if(cbk) dma_channel_enable_interrupt(sdio_trans_dma, DMA_ALL_FINISH_INT);
    dma_channel_enable(sdio_trans_dma, true);

    return SD_DMA_RUNNING;
}

#if (SD_LITTLE_EDIAN == 1)
void sdcard_data_convert(uint8_t *in, uint8_t *out, uint32 len)
{
    int i=0;
    if((in == NULL) || (out == NULL)  || (len == 0) || (len > DRV_SD_WRITE_MAX))
    {
        return;
    }
    for(i=0; i<len; i+=4)
    {
        out[i+0] = in[i+3];
        out[i+1] = in[i+2];
        out[i+2] = in[i+1];
        out[i+3] = in[i+0];
    }
}
#endif

static int sdcard_write_dma_status(void)
{
    int reg;
    if(dma_get_remain(sdio_trans_dma))
    {
        SDCARD_PRT("remain_wr:%x\r\n",dma_get_remain(sdio_trans_dma));
        return FALSE;
    }
    reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
    if(reg  & (SDCARD_CMDRSP_TX_FIFO_NEED_WRITE|SDCARD_CMDRSP_TX_FIFO_EMPTY))
    {
        int i;
        for(i=0; i<1000; i++)
        {
            reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
            if((reg & (SDCARD_CMDRSP_TX_FIFO_NEED_WRITE|SDCARD_CMDRSP_TX_FIFO_EMPTY))
                == (SDCARD_CMDRSP_TX_FIFO_NEED_WRITE|SDCARD_CMDRSP_TX_FIFO_EMPTY))
            {
                break;
            }
        }
        if(i>=1000)
        {
            os_printf("err,write half finsh,reg9:%x\r\n",reg);
        }
        REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,(SDCARD_CMDRSP_DATA_WR_END_INT|SDCARD_CMDRSP_TX_FIFO_NEED_WRITE|SDCARD_CMDRSP_TX_FIFO_EMPTY));
        return TRUE;
    }
    return FALSE;
}

int sdcard_write_dma_check_finish(void)
{
    static uint32_t s_timeout = 0;
    SD_DMA_DATA_S *p_dma_write_data = (SD_DMA_DATA_S *)&gs_sd_dma_write_data;
    int ret = FALSE;

    if(sd_host_state == ST_SD_IDLE)
    {
        ret = TRUE;
    }
    else if(sd_host_state == ST_SD_WR_MBLK_START)
    {
        if(!sdcard_is_busy())
        {
            SDIO_Error err = sdcard_cmd25_process(op_start_block);
            if(err == SD_OK){
                sdcard_dma_write_reg_init(p_dma_write_data->cnt_max >> 9);
                sd_host_state = ST_SD_WR_MBLK_TRANS_START;
            }else{
                //sd_rd_exception_proc()
                if(++s_timeout >= 20000)
                {
                    SDCARD_FATAL("wr sd_host_state:%d, err:%d\n", sd_host_state, err);
                    s_timeout = 0;
                #ifdef SDIO_TRANS_MIN_STOP
                    op_addr_pending = 0xFFFFFFFF;//last op block
                    fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;
                #endif
                    p_dma_write_data->cnt_add = 0;
                    p_dma_write_data->cnt_max = 0;
                    drv_sd_info_update(0);
                    sd_host_state = ST_SD_IDLE;
                    if(p_dma_write_data->cbk) p_dma_write_data->cbk(0);
                    ret = TRUE;
                }
            }
        }
    }
    else if(sd_host_state == ST_SD_WR_MBLK_TRANS_START)
    {
        #if (SD_LITTLE_EDIAN == 1)
        //about 50us
        sdcard_data_convert(&p_dma_write_data->data_buf[p_dma_write_data->cnt_add], p_dma_write_data->dma_buf, p_dma_write_data->dma_size);
        sdcard_write_data_dma(p_dma_write_data->dma_buf, p_dma_write_data->dma_size,p_dma_write_data->cbk);
        #else
        sdcard_write_data_dma(&p_dma_write_data->data_buf[p_dma_write_data->cnt_add], p_dma_write_data->dma_size,p_dma_write_data->cbk);
        #endif
        sd_host_state = ST_SD_WR_MBLK_TRANS_CHK;
        s_timeout = 0;
    }
    else if(sd_host_state == ST_SD_WR_MBLK_TRANS_CHK)
    {
        if(sdcard_write_dma_status() == FALSE)
        {
            if((s_timeout++) >(20000 + (p_dma_write_data->dma_size*1000)))
            {
                SDCARD_FATAL("wr sd_host_state:%d, timeout:%d\r\n", sd_host_state, s_timeout);
                s_timeout = 0;
                sdio_dma_disable();
                sdcard_send_write_stop(0);
            #ifdef SDIO_TRANS_MIN_STOP
                op_addr_pending = 0xFFFFFFFF;//last op block
                fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;
            #endif
                p_dma_write_data->cnt_add = 0;
                p_dma_write_data->cnt_max = 0;
                drv_sd_info_update(0);
                sd_host_state = ST_SD_IDLE;
                if(p_dma_write_data->cbk)
                {
                    p_dma_write_data->cbk(0);
                }
                ret = TRUE;
            }
        }
        else
        {
            s_timeout = 0;
            p_dma_write_data->cnt_add += p_dma_write_data->dma_size;
            SDCARD_PRT("check_wr_finish,cnt_add=%d,max=%d, data_buf=%p\r\n", 
                p_dma_write_data->cnt_add,p_dma_write_data->cnt_max,&p_dma_write_data->data_buf[p_dma_write_data->cnt_add]);

            if(p_dma_write_data->cnt_add < p_dma_write_data->cnt_max)
            {
                if((p_dma_write_data->cnt_add + p_dma_write_data->dma_size) >= p_dma_write_data->cnt_max)
                {
                    p_dma_write_data->dma_size = p_dma_write_data->cnt_max - p_dma_write_data->cnt_add;
                }
                sd_host_state = ST_SD_WR_MBLK_TRANS_START;//continue write
            }
            else
            {
                sdio_dma_disable();
                p_dma_write_data->cnt_add = 0;
                p_dma_write_data->cnt_max = 0;
                drv_sd_info_update(0);
            #ifndef SDIO_TRANS_MIN_STOP
                sdcard_send_write_stop(0);
                sd_host_state = ST_SD_WR_MBLK_SD_PROG;//sdcard program
            #else
                sd_host_state = ST_SD_IDLE;//place before cbk
                if(p_dma_write_data->cbk) {
                    p_dma_write_data->cbk(0);
                }
                ret = TRUE;
            #endif
            }
        }
    }
    else if(sd_host_state == ST_SD_WR_MBLK_SD_PROG)
    {
    #ifndef SDIO_TRANS_MIN_STOP
        if(!sdcard_is_busy())
        {
            sdio_clk_config(0);
            sd_host_state = ST_SD_IDLE;//place before cbk
            if(p_dma_write_data->cbk)
            {
                p_dma_write_data->cbk(0);
            }
            ret = TRUE;
        }
    #else
        if(!sdcard_is_busy())  sd_host_state = ST_SD_WR_MBLK_START;
    #endif
    }
    return ret;
}

static int sdcard_read_dma_status(void)
{
    int reg;
    if(dma_get_remain(sdio_trans_dma))
    {
        SDCARD_PRT("remain_rd:%x\r\n",dma_get_remain(sdio_trans_dma));
        return FALSE;
    }
    reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
    if(reg & (SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ))
    {
        int i;
        for(i=0; i<1000; i++)
        {
            reg = REG_READ(REG_SDCARD_CMD_RSP_INT_SEL);
            if((reg & (SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ))
                == (SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ))
            {
                break;
            }
        }
        if(i>=1000)
        {
            //os_printf("err,read half finsh,reg9:%x\r\n",reg);
        }
        REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,(SDCARD_CMDRSP_DATA_REC_END_INT|SDCARD_CMDRSP_RX_FIFO_NEED_READ));
        return  TRUE;
    }
    return FALSE;
}

int sdcard_read_dma_check_finish(void)
{
    static uint32_t s_timeout = 0;
    SD_DMA_DATA_S *p_dma_read_data = (SD_DMA_DATA_S *)&gs_sd_dma_read_data;
    int ret = FALSE;

    if(sd_host_state == ST_SD_IDLE)
    {
        ret = TRUE;
    }
    else if(sd_host_state == ST_SD_RD_MBLK_START)
    {
        if(!sdcard_is_busy())
        {
            sdcard_read_data_init(p_dma_read_data->cnt_max >> 9);
            SDIO_Error err = sdcard_cmd18_process(op_start_block);
            if(err == SD_OK){
                sd_host_state = ST_SD_RD_MBLK_TRANS_START;
            }else{
                //sd_rd_exception_proc()
                if(++s_timeout >= 20000)
                {
                    SDCARD_FATAL("rd sd_host_state:%d, err:%d\n", sd_host_state, err);
                    s_timeout = 0;
                #ifdef SDIO_TRANS_MIN_STOP
                    op_addr_pending = 0xFFFFFFFF;//last op block
                    fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;
                #endif
                    p_dma_read_data->cnt_add = 0;
                    p_dma_read_data->cnt_max = 0;
                    drv_sd_info_update(0);
                    sd_host_state = ST_SD_IDLE;
                    if(p_dma_read_data->cbk) p_dma_read_data->cbk(0);
                    ret = TRUE;
                }
            }
        }
    }
    else if(sd_host_state == ST_SD_RD_MBLK_TRANS_START)
    {
        sdcard_read_data_dma(&p_dma_read_data->data_buf[p_dma_read_data->cnt_add], p_dma_read_data->dma_size,p_dma_read_data->cbk);
        sd_host_state = ST_SD_RD_MBLK_TRANS_CHK;
        s_timeout = 0;
    }
    else if(sd_host_state == ST_SD_RD_MBLK_TRANS_CHK)
    {
        if(sdcard_read_dma_status() == FALSE)
        {
            if((s_timeout++) >(20000 + (p_dma_read_data->dma_size*1000)))
            {
                SDCARD_FATAL("err,%s,timeout:%d\r\n", __func__, s_timeout);
                SDCARD_FATAL("dma_max:%d, cnt_add:%d, dma_sz:%d, dma_remain:%d\n", p_dma_read_data->cnt_max, p_dma_read_data->cnt_add, p_dma_read_data->dma_size, dma_get_remain(sdio_trans_dma));
                s_timeout = 0;
                sdio_dma_disable();
                sdcard_send_read_stop();
            #ifdef SDIO_TRANS_MIN_STOP
                op_addr_pending = 0xFFFFFFFF;//last op block
                fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;
            #endif
                p_dma_read_data->cnt_add = 0;
                p_dma_read_data->cnt_max = 0;
                drv_sd_info_update(0);
                sd_host_state = ST_SD_IDLE;
                if(p_dma_read_data->cbk)
                {
                    p_dma_read_data->cbk(0);
                }
                ret = TRUE;
            }
        }
        else
        {
            s_timeout = 0;
            
            p_dma_read_data->cnt_add += p_dma_read_data->dma_size;
            SDCARD_PRT("check_rd_finish,cnt_add=%d,max=%d, data_buf=%p\r\n", 
                p_dma_read_data->cnt_add,p_dma_read_data->cnt_max,&p_dma_read_data->data_buf[p_dma_read_data->cnt_add]);

            if(p_dma_read_data->cnt_add < p_dma_read_data->cnt_max)
            {
                if((p_dma_read_data->cnt_add + p_dma_read_data->dma_size) >= p_dma_read_data->cnt_max)
                {
                    p_dma_read_data->dma_size = p_dma_read_data->cnt_max - p_dma_read_data->cnt_add;
                }
                sd_host_state = ST_SD_RD_MBLK_TRANS_START;
            }
            else
            {
                sdio_dma_disable();
            #ifndef SDIO_TRANS_MIN_STOP
                sdcard_send_read_stop();
                sdio_clk_config(0);
            #endif
                p_dma_read_data->cnt_add = 0;
                p_dma_read_data->cnt_max = 0;
                drv_sd_info_update(0);
                sd_host_state = ST_SD_IDLE;//place before cbk
                if(p_dma_read_data->cbk)
                {
                    p_dma_read_data->cbk(0);
                }
                ret = TRUE;
            }
        }
    }
    #ifdef SDIO_TRANS_MIN_STOP
    else if(sd_host_state == ST_SD_RD_MBLK_SD_PROG)
    {
        if(!sdcard_is_busy()) sd_host_state = ST_SD_RD_MBLK_START;
    }
    #endif
    return ret;
}

void sdcard_dma_read_reg_init(uint32_t block_num)
{
    return;
}

void sdcard_dma_write_reg_init(uint32_t block_num)
{
    uint32 reg;
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_MASK,(SDCARD_CMDRSP_DATA_WR_END_INT_MASK|SDCARD_CMDRSP_TX_FIFO_NEED_WRITE_MASK|SDCARD_CMDRSP_TX_FIFO_EMPTY_MASK));
    REG_WRITE(REG_SDCARD_CMD_RSP_INT_SEL,(SDCARD_CMDRSP_DATA_WR_END_INT|SDCARD_CMDRSP_TX_FIFO_NEED_WRITE|SDCARD_CMDRSP_TX_FIFO_EMPTY));
    REG_WRITE(REG_SDCARD_DATA_REC_TIMER,DEF_HIGH_SPEED_DATA_TIMEOUT * block_num);
    reg = (SD_DEFAULT_BLOCK_SIZE << SDCARD_DATA_REC_CTRL_BLK_SIZE_POSI)
        |SDCARD_DATA_REC_CTRL_DATA_MUL_BLK
        |((SD_LITTLE_EDIAN == 1) ? SDCARD_DATA_REC_CTRL_DATA_BYTE_SEL : 0)
#ifdef CONFIG_SDCARD_BUSWIDTH_4LINE
        |SDCARD_DATA_REC_CTRL_DATA_BUS
#endif
        ;
    REG_WRITE(REG_SDCARD_DATA_REC_CTRL,reg);
}
static int sdcard_write_data_dma_init(uint8_t* write_data, uint32_t block_num, void*cbk)
{   
    SD_DMA_DATA_S *p_dma_write_data = (SD_DMA_DATA_S *)&gs_sd_dma_write_data;
    
    p_dma_write_data->data_buf = write_data;
    p_dma_write_data->cnt_max = block_num << 9;//* 512;
    p_dma_write_data->cnt_add = 0;
    p_dma_write_data->cbk = cbk;   
    p_dma_write_data->dma_size = p_dma_write_data->cnt_max;

    #if (SD_LITTLE_EDIAN == 1)
    if(p_dma_write_data->dma_size > DRV_SD_WRITE_MAX) p_dma_write_data->dma_size = DRV_SD_WRITE_MAX;
    if(gs_dma_memory == NULL)
    {
        gs_dma_memory = jmalloc(DRV_SD_WRITE_MAX, 0);//(uint8_t *)0x01219800;//
        if(gs_dma_memory == NULL)
        {
            SDCARD_FATAL("%s,err,memory not enough!\r\n",__func__);
            return SD_ERROR;
        }
        else
        {            
#ifdef CEVA_X
            if(gs_dma_memory < 0x800000)
            {
                gs_dma_memory += DSP_RAM_BASIC_ADDR;
            }
#endif
            SDCARD_PRT("jmalloc gs_dma_memory ok,%x\r\n",gs_dma_memory);
        }
    }
    p_dma_write_data->dma_buf = gs_dma_memory;
    #endif

    return SD_OK;
}

int sdcard_write_multi_block(void*buf, int first_block, int block_num)
{
    return sdcard_write_multi_block_dma((uint8_t *)buf, first_block, block_num,NULL);
}

SDIO_Error sdcard_write_multi_block_dma(uint8_t *write_buff, uint32_t first_block, uint32_t block_num,void*cbk)
{
    sdcard_wait_idle();

    drv_sd_info_update(1);
    
#ifdef CEVA_X
    write_buff += DSP_RAM_BASIC_ADDR;
#endif
    SDCARD_PRT("%s,write_buff:%x, first_block:%x, block_num:%d,cbk:%x\r\n",__func__,write_buff, first_block, block_num, cbk);
    int ret = SD_OK;
#ifdef TEST_SDCARD_RW
    gs_sd_write_cnt++;
#endif
    if(first_block >= sdcard_get_total_block())
    {
        SDCARD_FATAL("wr first_block:%x, max_block:%x\r\n", first_block, sdcard_get_total_block());
        return SD_OUT_RANGE;
    }
    if(block_num == 0)
    {
        SDCARD_FATAL("block_num=0\r\n");
        return SD_OK;
    }

    op_start_block = first_block;
    ret += sdcard_write_data_dma_init(write_buff, block_num,cbk);
#ifndef SDIO_TRANS_MIN_STOP
    sd_host_state = ST_SD_WR_MBLK_START;
#else
    if(fg_sd_rw_op_prv == FG_SD_RW_OP_READ){ //if last opration is read, stop read
        sdcard_send_read_stop();
    }else if(fg_sd_rw_op_prv == FG_SD_RW_OP_WRITE){ //if last opration is write
        // write continue if address continuous, else need stop write trans and program than start new read trans.
        if(op_start_block == op_addr_pending){
            sd_host_state = ST_SD_WR_MBLK_TRANS_START;
        }else{
            sdcard_send_write_stop(0);
        }
    }
    if(sd_host_state == ST_SD_IDLE) sd_host_state = ST_SD_WR_MBLK_START;
    fg_sd_rw_op_prv = FG_SD_RW_OP_WRITE;
    op_addr_pending = op_start_block + block_num;//record the next pending address
#endif

    if(ret != SD_OK)
    {
        SDCARD_FATAL("err:sdcard_write_multi_block_dma, ret=%d\r\n",ret);
        sd_reinit();
        return ret;
    }
    if(cbk == NULL)
    {
        sdcard_timeout_init();
        while(1)
        {
            if(sdcard_write_dma_check_finish() == TRUE){
                ret = 0;
                break;
            }
            else
            {
                if(sdcard_timeout(1000)){
                    ret = SD_DATA_TIMEOUT;
                    SD_LOG_E("sd wr over_time\n");
                    break;
                }
            }
        }
    }
    //app need check finish

    return ret;
}


uint32_t sdcard_get_block_size(void)
{
    return sdcard.block_size;
}

void sdcard_idle(char enable)
{
}


void clr_sd_noinitial_flag(void)
{
    NoneedInitflag = 0;
}

int sdio_host_state_get(void)
{
    return sd_host_state;
}

SDIO_Error SD_init(void)
{
    if(NoneedInitflag ==1)return(SD_OK);
    SD_LOG_I("%s()\r\n", __func__);
    bk_bt_run_immediately(__func__,1);

    drv_sd_info_update(1);
    gs_sd_buf_addr = 0;
    //REG_SYSTEM_0x5C &= (~(0x3F << 0));    //io voltage

    SDIO_Error err = SD_OK;
    sdio_sw_init();
    //sys_delay_ms(20);
    sdio_hw_init();
    // sys_delay_ms(30);
    // bk_bt_run_immediately(__func__,0);

    SD_LOG_I("%s %d CMD0\r\n",__func__,__LINE__);
    err = sdcard_cmd0_process();// reset card
    if(err != SD_OK)
    {
        SD_LOG_E("cmd0 err: %d\r\n", err);
        goto err_return;
    }
    //sys_delay_ms(5);

    SD_LOG_I("%s %d CMD1\r\n",__func__,__LINE__);
    err = sdcard_cmd1_process();
    if(err == SD_OK){
        goto MMC_init;
    }else if(err != SD_CMD_RSP_TIMEOUT){
        SD_LOG_E("cmd1 err:%d\n", err);
    }
    //sys_delay_ms(5);

    // check support voltage
    SD_LOG_I("%s %d CMD8\r\n", __func__, __LINE__);
    err = sdcard_cmd8_process();
    if(err != SD_OK && err != SD_CMD_RSP_TIMEOUT )
    {
        SD_LOG_E("cmd8 err:%d\r\n", err);
        goto err_return;
    }

    SD_LOG_I("%s %d CMD55 & CMD41\n", __func__, __LINE__);
    if(err == SD_OK)
    {
        int retry_time = SD_MAX_VOLT_TRIAL;
        uint32_t resp0;
        while(retry_time)
        {
            err = sdcard_acmd41_process(SD_DEFAULT_OCR);
            sdio_get_cmdresponse_argument(0, &resp0);
            if(err != SD_OK)
            {
                SD_LOG_E("cmd55&cmd41 err:%d, rsp:0x%X\r\n", err, resp0);
                goto err_return;
            }
            if(resp0 & OCR_MSK_BUSY)
            {
                sdcard.Addr_shift_bit = (resp0 & OCR_MSK_HC) ? 0 : 9;
                break;
            }
            retry_time--;
            sys_delay_ms(8);
            bk_bt_run_immediately(__func__,0);
            sys_delay_ms(8);
            bk_bt_run_immediately(__func__,0);
        }
        if(!retry_time)
        {
            SD_LOG_I("send cmd55&cmd41 retry time out\r\n");
            err = SD_INVALID_VOLTRANGE;
            goto err_return;
        }

        SD_LOG_I("cmd55&cmd41 complete, card is ready\r\n");

        if(resp0 & OCR_MSK_HC){
            SD_LOG_I("High Capacity SD Memory Card\r\n");
        }else{
            SD_LOG_I("Standard Capacity SD Memory Card\r\n");
        }
    }
    else if(err == SD_CMD_RSP_TIMEOUT)
    {
        int retry_time = SD_MAX_VOLT_TRIAL;
        uint32_t resp0;
        while(retry_time)
        {
            err = sdcard_acmd41_process(OCR_MSK_VOLTAGE_ALL);
            sdio_get_cmdresponse_argument(0, &resp0);
            if(err != SD_OK)
            {
                SD_LOG_E("cmd55&cmd41 err:%d, rsp:0x%X\r\n", err, resp0);
                goto err_return;
            }
            if(resp0 & OCR_MSK_BUSY)
            {
                sdcard.Addr_shift_bit = (resp0 & OCR_MSK_HC) ? 0 : 9;
                break;
            }
            retry_time--;
            sys_delay_ms(8);
            bk_bt_run_immediately(__func__,0);
            sys_delay_ms(8);
            bk_bt_run_immediately(__func__,0);
        }
        if(!retry_time)
        {
            SD_LOG_I("send cmd55&cmd41 retry time out, maybe a MMC card\r\n");
            err = SD_ERROR;
            goto err_return;
        }
        SD_LOG_I("cmd55&cmd41 complete, SD V1.X card is ready\r\n");
    }
    //sys_delay_ms(2);
    // get CID, return R2
    SD_LOG_I("%s %d CMD2\r\n",__func__,__LINE__);
    err = sdcard_cmd2_process();
    if(err != SD_OK)
    {
        SDCARD_FATAL("send cmd2 err:%d\r\n", err);
        goto err_return;
    }
    //sys_delay_ms(2);
    // get RCA,
    SD_LOG_I("%s %d CMD3\r\n",__func__,__LINE__);
    err = sdcard_cmd3_process();
    if(err != SD_OK)
    {
        SDCARD_FATAL("send cmd3 err:%d\r\n", err);
        goto err_return;
    }

    // change to high speed clk
    sdio_set_high_clk();
    //sys_delay_ms(2);
    // get CSD
    SD_LOG_I("%s %d CMD9\r\n",__func__,__LINE__);
    err = sdcard_cmd9_process(SD_CARD);
    if(err != SD_OK)
    {
        SDCARD_FATAL("send cmd9 err:%d\r\n", err);
        goto err_return;
    }
    //sys_delay_ms(2);
    // select card
    SD_LOG_I("%s %d CMD7\r\n",__func__,__LINE__);
    err = sdcard_cmd7_process();
    if(err != SD_OK)
    {
        SDCARD_FATAL("send cmd7 err:%d\r\n", err);
        goto err_return;
    }
    //sys_delay_ms(2);
    // change bus width, for high speed
    SD_LOG_I("%s %d ACMD6\r\n",__func__,__LINE__);
    err = sdcard_acmd6_process();
    if(err != SD_OK)
    {
        SDCARD_FATAL("send acmd6 err:%d\r\n", err);
        goto err_return;
    }

    // Sd_MMC_flag = SD_CARD;
    err = SD_OK;
    SD_LOG_I("sdcard initialize is done\r\n");
    
    goto right_return;

MMC_init:
    SD_LOG_I("MMC_init:%x \r\n", err);
    err = sdcard_cmd2_process();
    SD_LOG_I("cmd 2 :%x\r\n", err);
    if(err != SD_OK)
        goto err_return;
    err = sdcard_mmc_cmd3_process();
    SD_LOG_I("cmd 3 :%x\r\n", err);
    sdio_set_high_clk();
    err = sdcard_cmd9_process(MMC_CARD);
    SD_LOG_I("cmd 9 :%x\r\n", err);
    if(sdcard.Addr_shift_bit == 0)
    {
        err = sdcard_mmc_cmd8_process();
        SD_LOG_I("cmd 8 :%x\r\n", err);
    }
    if(err != SD_OK)
        goto err_return;
    err = sdcard_cmd7_process();
    if(err != SD_OK)
        goto err_return;
    // Sd_MMC_flag = MMC_CARD;
    goto right_return;

right_return:
    SD_LOG_I("SD_init_ok:\n\n");
    Sd_MMC_flag = SD_CARD;
    NoneedInitflag = 1;
    sd_host_state = ST_SD_IDLE;
    #ifdef SDIO_TRANS_MIN_STOP
    op_addr_pending = 0xFFFFFFFF;//last op block
    fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;//last rw opration is read or write
    #endif
    
    sdio_dma_init();
err_return:
    // print64bits("capacity:",&g_sd_capacity);
    // print64bits("pic_addr:",&g_sd_pic_addr);
    SD_LOG_I("total_blocks:%x, block_size:%x, pic block(64M):0x%x, pic addr:0x%x\r\n",
        sdcard_get_total_block(), sdcard_get_block_size(),
        (sdcard_get_total_block() - (64*1024*1024) / sdcard_get_block_size()),
        (sdcard_get_total_block() * sdcard_get_block_size() - (64*1024*1024)));
    SD_LOG_I("Addr_shift_bit:%x, card_rca:%x, init_flag:%x\n\n\n\n",sdcard.Addr_shift_bit, sdcard.card_rca, sdcard.init_flag);
    drv_sd_info_update(0);
    return err;
}

void sd_close(void)
{
    //SDIO_Error res;
    int reg;

    sd_host_state = ST_SD_IDLE;
    #ifdef SDIO_TRANS_MIN_STOP
    op_addr_pending = 0xFFFFFFFF;//last op block
    fg_sd_rw_op_prv = FG_SD_RW_OP_INIT;//last rw opration is read or write
    #endif

    if(CLK_CHECK())
    {
        os_printf("%s,1\r\n",__func__);
        reg = REG_READ(REG_SDCARD_FIFO_THRESHOLD);
        reg &= ~(0xFFFF | (3 << 16) | (1 << 20));
        reg |= (0x0101 | ((3 << 16) | (1 << 20)));
        REG_WRITE(REG_SDCARD_FIFO_THRESHOLD,reg);
        
        reg = REG_READ(REG_SDCARD_DATA_REC_CTRL);
        reg |= SDCARD_DATA_REC_CTRL_DATA_STOP_EN;
        reg &= ~(SDCARD_DATA_REC_CTRL_DATA_WR_DATA_EN);
        REG_WRITE(REG_SDCARD_DATA_REC_CTRL,reg);


        SD_DAT0_SET_INPUT;
        sd_check_timeout(0);
        while(!SD_DAT0_GET_IDLE)
        {
            if(sd_check_timeout(1))
            {
                SDCARD_FATAL("%s,timeout\r\n",__func__);
                break;
            }
        }
        sdcard_uninitialize();
    }
}

static SDIO_Error sd_reinit_handle(void)
{
    SDIO_Error res;
    
    sd_close();

    
    res = SD_init();
    if(res != SD_OK)
    {
        SDCARD_FATAL("  sd_reinit_err!\r\n\r\n\r\n");
    }
    else
    {
        SDCARD_FATAL("  sd_reinit_ok!\r\n\r\n\r\n");
    }
    return res;
}

SDIO_Error sd_reinit(void)
{
    int i = 1;
    while(i--)
    {
        timer_clear_watch_dog();
        if(sd_reinit_handle() == SD_OK)
        {
            return SD_OK;
        }
    }
    SDCARD_FATAL("  sd maybe err\r\n\r\n\r\n");
#ifdef CONFIG_SDCARD_DETECT
    if(s_cbk_sd_pull_out)
    {
        SDCARD_FATAL("pull out process!\n");
        SDOnline = SD_CARD_OFFLINE;
        s_cbk_sd_pull_out();
    }
#endif
    return SD_ERROR;
}

#ifdef CONFIG_SDCARD_DETECT
/** 10ms loop applyed
 * return   0 -- NO CHANGE
            1 -- SD INSERT
            2 -- SD PULLOUT
*/
static void sd_detect_fun(void)
{
    static int cnt=0;
    int s;
    if(!SD_det_gpio_flag)
    {
        //if sdcard detect with out independent gpio
    }
    else
    {
        s=gpio_input(SD_detect_pin)?1:0;
        if((s^SD_detect_level)==0)
        {
            if(SDOnline==SD_CARD_OFFLINE)
            {
                if(cnt > SD_ATACH_DEBOUNCE_CNT)
                {
                    SDOnline=SD_CARD_ONLINE;
                    os_printf("%s, SDOnline:%d\n", __func__, SDOnline);
                    //mount FS
                    if(s_cbk_sd_insert_in) s_cbk_sd_insert_in();
                    // Media_Fs_Init(0);
                }
                else
                    cnt++;
            }
            else
            {
                cnt=0;
            }
        }
        else
        {
            if(SDOnline==SD_CARD_ONLINE)
            {
                if(cnt > SD_DETACH_DEBOUNCE_CNT)
                {
                    SDOnline=SD_CARD_OFFLINE;
                    os_printf("%s, SDOnline:%d\n", __func__, SDOnline);
                    //unmount FS
                    if(s_cbk_sd_pull_out) s_cbk_sd_pull_out();
                }else
                cnt++;
            }
            else
            {
                cnt=0;
            }
        }
    }
}
#endif

#ifdef CONFIG_SDCARD_DETECT
void app_sd_init(void)
{
#ifdef SDCARD_DETECT_IO
	SD_det_gpio_flag = 1;
	SD_detect_pin = SDCARD_DETECT_IO;
	SD_detect_level = SDCARD_DETECT_LVL;
    gpio_config(SD_detect_pin, 3);
	sdcard.detect_func = sd_detect_fun;
#else
    app_env_handle_t env_h = app_env_get_handle();
//    os_memset( &driver_sdcard, 0, sizeof( driver_sdcard ) ) ;
	if(env_h->env_cfg.system_para.pins[PIN_sdDet]==0)return;
	if((APP_ENV_SYS_FLAG_SD_DETECT_ENA&env_h->env_cfg.system_para.system_flag)==0)return;
    if(env_h->env_cfg.system_para.pins[PIN_sdDet]&(1<<7))   {
		SD_det_gpio_flag = 1;
		SD_detect_pin=env_h->env_cfg.system_para.pins[PIN_sdDet]&0x1f;
		SD_detect_level=(env_h->env_cfg.system_para.pins[PIN_sdDet]&(1<<6))?1:0;
        gpio_config(SD_detect_pin, 3);
        SDOnline    = SD_CARD_OFFLINE;
		sdcard.detect_func=sd_detect_fun;
    }
#endif
    os_printf("SD_detect_pin:%d, valid level:%d, cur:%d\n",SD_detect_pin, SD_detect_level, gpio_input(SD_detect_pin));
}
#endif

void app_sdcard_init(void* cbk_insert_in, void* cbk_pull_out)
{
#ifdef CONFIG_SDCARD_DETECT
    s_cbk_sd_insert_in = cbk_insert_in;
    s_cbk_sd_pull_out = cbk_pull_out;

    app_sd_init();
#endif
}

void app_sd_scanning(void)
{
#ifdef CONFIG_SDCARD_DETECT
    if(sdcard.detect_func)
    {
        (*sdcard.detect_func)();
    }
    else
    {
        SDCARD_FATAL("detect_func is NULL!");
    }
#endif
}
void app_sd_set_online(int on_off)
{
    os_printf("app_sd_set_online: %d\n",on_off);
	SDOnline=on_off;
}









#ifdef TEST_SDCARD_RW

////////////////////////////////////////////////////////////////////////////////////////////////////
#define SD_TEST_ADDRESS (0x0)

typedef struct
{
    uint64_t addr;
    uint64_t len;
    uint64_t offset;
}watch_rd_ack_t;
watch_rd_ack_t g_watch_rd_ack[3];
static uint8_t *gs_sd_buf = NULL;
static volatile uint32_t gs_pre_read_flg_write_cnt=0;

void drv_init_sd_offset(uint32_t offset,uint8_t *buffer,uint32_t op_len)
{
    uint64_t base_addr = 0;
    uint64_t addr = 0;
    uint64_t len = 0;
    uint64_t addr_offset = 0;
    if(sdcard.total_block == 0)
    {
        drv_sd_info_update(1);
        if(sdcard.total_block == 0)
        {
            SD_init();
            drv_sd_info_update(0);
        }
        uint32_t pic_offset = 64*1024*1024;
        g_sd_capacity = (uint64_t)sdcard.total_block*sdcard.block_size;
        print64bits("capacity_new:",&g_sd_capacity);
        g_sd_capacity = g_sd_capacity - pic_offset;
        g_sd_pic_addr = g_sd_capacity; 
        print64bits("pic_addr_new:",&g_sd_pic_addr);
    }
    memset((uint8_t *)g_watch_rd_ack, 0x00, sizeof(g_watch_rd_ack));
    
    //base_addr = sdcard.pic_addr;  
    base_addr = g_sd_pic_addr;
    addr_offset = offset&0x1FF;
    addr = base_addr + ((offset>>9)<<9);
    len = op_len;

    if(addr_offset)
    {
        g_watch_rd_ack[0].addr = addr>>9;
        g_watch_rd_ack[0].offset = addr_offset;
        g_watch_rd_ack[0].len = 512 - addr_offset;
        if(g_watch_rd_ack[0].len > len)
        {
            g_watch_rd_ack[0].len = len;
        }
        len -= g_watch_rd_ack[0].len;
        if(len)
        {
            g_watch_rd_ack[1].addr = (addr+512)>>9;
            g_watch_rd_ack[1].len = len & 0xFFFFFE00;
            if(len & 0x1FF)
            {
                g_watch_rd_ack[2].addr = (addr+512+g_watch_rd_ack[1].len)>>9;
                g_watch_rd_ack[2].len = len & 0x1FF;
            }
        }
    }
    else
    {
        if(len)
        {
            g_watch_rd_ack[1].addr = (addr)>>9;
            g_watch_rd_ack[1].len = len & 0xFFFFFE00;
            if(len & 0x1FF)
            {
                g_watch_rd_ack[2].addr = (addr+g_watch_rd_ack[1].len)>>9;
                g_watch_rd_ack[2].len = len & 0x1FF;
            }
        }
    }
    
    
    for(int i=0; i<3; i++)
    {
        //print64bits("addr", &g_watch_rd_ack[i].addr);
        //print64bits("offset", &g_watch_rd_ack[i].offset);
        //print64bits("len", &g_watch_rd_ack[i].len);
    }
    
}
void drv_save_pre_read_flg(uint64_t pree_addr)
{
    //return;
    gs_sd_buf_addr = pree_addr;
    gs_pre_read_flg_write_cnt = gs_sd_write_cnt;
}

int drv_sd_check_sd_pre_buf(void)
{
    if(gs_sd_buf == NULL)
    {
        gs_sd_buf = jmalloc(512, 0);
        if(gs_sd_buf == NULL)
        {
            os_printf("err,buffer is NULL\r\n");
            return -1;
        }
    }
    return 0;
}
int drv_sd_read(uint32_t offset,uint8_t *buffer,uint32_t rd_len)
{
    uint32_t len_sum=0;
    uint32_t len_cur=0;
    if(rd_len == 0)
    {
        os_printf("err,read len is 0\r\n");
        return -1;
    }
    if(drv_sd_check_sd_pre_buf() == -1)
    {
        return -1;
    }
    SDCARD_PRT("offset:%x,buffer:%x,rd_len:%d\r\n",offset,buffer,rd_len);
    if((offset != 0) && (((offset+g_sd_pic_addr) &  0xFFFFFFFFFFFFFE00) == (gs_sd_buf_addr)))
    {
        drv_sd_info_update(1);
        //os_printf("gs_pre_read_flg_write_cnt:%d,gs_sd_write_cnt:%d,\r\n",gs_pre_read_flg_write_cnt,gs_sd_write_cnt);
        if(gs_pre_read_flg_write_cnt == gs_sd_write_cnt)
        {
            uint32_t pre_len = 0;
            uint32_t addr_offset = 0;
            
            addr_offset = offset&0x1FF;
            pre_len = 512 - addr_offset;
            if(pre_len > rd_len)
            {
                pre_len = rd_len;
            }
            //print64bits("pre_addr", &gs_sd_buf_addr);
            //os_printf("pre_len:%d\r\n",pre_len);
            memcpy(buffer, &gs_sd_buf[addr_offset], pre_len);
            buffer += pre_len;
            offset += pre_len;
            rd_len -= pre_len;
        }
        drv_sd_info_update(0);
        if(rd_len == 0)
        {
            return 0;
        }
    }
    //memset(gs_sd_buf, 0x00, 512);
    drv_init_sd_offset(offset,buffer,rd_len);
    if(g_watch_rd_ack[0].len)
    {
        sdcard_read_multi_block_dma(gs_sd_buf,g_watch_rd_ack[0].addr,1,NULL);
        memcpy(buffer, &gs_sd_buf[g_watch_rd_ack[0].offset],g_watch_rd_ack[0].len);
        SDCARD_PRT("buffer0:%x,len:%d\r\n",buffer,(uint32_t)g_watch_rd_ack[0].len);
        buffer += g_watch_rd_ack[0].len;
        drv_save_pre_read_flg(g_watch_rd_ack[0].addr<<9);
    }
    if(g_watch_rd_ack[1].len)
    {
        //os_printf("large read!\r\n");
        while(len_sum < g_watch_rd_ack[1].len)
        {
            if((g_watch_rd_ack[1].len - len_sum)>=(DRV_SD_READ_MAX))
            {
                len_cur = (DRV_SD_READ_MAX);
            }
            else
            {
                len_cur = (g_watch_rd_ack[1].len - len_sum);
            }
            sdcard_read_multi_block_dma(buffer+len_sum,g_watch_rd_ack[1].addr+(len_sum/512),len_cur/512,NULL);
            len_sum += len_cur;
        }
        SDCARD_PRT("buffer1:%x,len:%d\r\n",buffer,(uint32_t)g_watch_rd_ack[1].len);
        buffer += g_watch_rd_ack[1].len;
    }
    if(g_watch_rd_ack[2].len)
    {
        sdcard_read_multi_block_dma(gs_sd_buf,g_watch_rd_ack[2].addr,1,NULL);
        memcpy(buffer, gs_sd_buf,g_watch_rd_ack[2].len);
        SDCARD_PRT("buffer2:%x,len:%d\r\n",buffer,(uint32_t)g_watch_rd_ack[2].len);
        buffer += g_watch_rd_ack[2].len;
        drv_save_pre_read_flg(g_watch_rd_ack[2].addr<<9);
    }
    return 0;
}

int drv_sd_write(uint32_t offset,uint8_t *buffer,uint32_t wr_len)
{
    uint32_t len_sum=0;
    uint32_t len_cur=0;
    if(wr_len == 0)
    {
        os_printf("err,write len is 0\r\n");
        return -1;
    }
    if(drv_sd_check_sd_pre_buf() == -1)
    {
        return -1;
    }
    SDCARD_PRT("offset:%x,buffer:%x,rd_len:%d\r\n",offset,buffer,wr_len);
    drv_init_sd_offset(offset,buffer,wr_len);
    if(g_watch_rd_ack[0].len)
    {
        sdcard_read_multi_block_dma(gs_sd_buf,g_watch_rd_ack[0].addr,1,NULL);
        memcpy(&gs_sd_buf[g_watch_rd_ack[0].offset], buffer, g_watch_rd_ack[0].len);
        sdcard_write_multi_block_dma(gs_sd_buf,g_watch_rd_ack[0].addr,1,NULL);
        SDCARD_PRT("buffer0:%x,len:%d\r\n",buffer,g_watch_rd_ack[0].len);
        buffer += g_watch_rd_ack[0].len;
    }
    if(g_watch_rd_ack[1].len)
    {
        while(len_sum < g_watch_rd_ack[1].len)
        {
            if((g_watch_rd_ack[1].len - len_sum)>=(DRV_SD_WRITE_MAX))
            {
                len_cur = (DRV_SD_WRITE_MAX);
            }
            else
            {
                len_cur = (g_watch_rd_ack[1].len - len_sum);
            }
            sdcard_write_multi_block_dma(buffer+len_sum,g_watch_rd_ack[1].addr+(len_sum/512),len_cur/512,NULL);
            len_sum += len_cur;
        }
        SDCARD_PRT("buffer1:%x,len:%d\r\n",buffer,g_watch_rd_ack[1].len);
        buffer += g_watch_rd_ack[1].len;
    }
    
    if(g_watch_rd_ack[2].len)
    {
        sdcard_read_multi_block_dma(gs_sd_buf, g_watch_rd_ack[2].addr, 1,NULL);
        memcpy(gs_sd_buf, buffer, g_watch_rd_ack[2].len);
        sdcard_write_multi_block_dma(gs_sd_buf, g_watch_rd_ack[2].addr, 1,NULL);
        SDCARD_PRT("buffer2:%x,len:%d\r\n",buffer,g_watch_rd_ack[2].len);
        buffer += g_watch_rd_ack[2].len;
    }
    return 0;
}




void sd_test_rw3(uint8 *data_wr, uint8 *data_rd)
{
    os_printf("%s\r\n\r\n", __func__);
    uint32 data_len;
    uint32 offset = 0;
    int cnt;
    int i;
    
    for(cnt = 1; cnt < (63*1024*1024); cnt+=4000)
    {
        timer_clear_watch_dog();
        data_len=4000;
        for(i=0; i<data_len; i++)
        {
            data_wr[i] = i+cnt;
        }
        //data_wr[0] = data_len;    
        drv_sd_write(offset+cnt,data_wr,data_len);
        drv_sd_read(offset+cnt,data_rd,data_len);
        for(i=0; i<data_len; i++)
        {
            if(data_wr[i] != data_rd[i])
            {
                break;
            }
        }
        if(i<data_len)
        {
            os_printf("\r\nwrite:data_len %d,err_pos %d,wr_val %x,rd_val %x\r\n",
                data_len,i,data_wr[i],data_rd[i]);
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_wr[i]);
            }
            os_printf("\r\nread:\r\n");
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_rd[i]);
            }
            os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset+cnt,data_len,data_len);
            os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset+cnt,data_len,data_len);
            while(1);
        }
        else
        {
            os_printf("\r\n\r\n compare ok!:addr %x,len %x %d\r\n",offset+cnt,data_len,data_len);
        }
    }
    os_printf("\r\n\r\n test end\r\n\r\n");
    //while(1)
    {
        //timer_clear_watch_dog();
    }
}


void sd_test_rw2(uint8 *data_wr, uint8 *data_rd)
{
    os_printf("%s\r\n\r\n", __func__);
    uint32 data_len;
    uint32 offset = SD_TEST_ADDRESS;
    int cnt;
    int i;
    
    for(cnt = 1; cnt < 8192; cnt+=11)
    {
        timer_clear_watch_dog();
        data_len=cnt;
        for(i=0; i<data_len; i++)
        {
            data_wr[i] = i+data_len;
        }
        data_wr[0] = data_len;  
        drv_sd_write(offset,data_wr,data_len);
        drv_sd_read(offset,data_rd,data_len);
        
        for(i=0; i<data_len; i++)
        {
            if(data_wr[i] != data_rd[i])
            {
                break;
            }
        }
        if(i<data_len)
        {
            os_printf("\r\nwrite:data_len %d,err_pos %d,wr_val %x,rd_val %x\r\n",
                data_len,i,data_wr[i],data_rd[i]);
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_wr[i]);
            }
            os_printf("\r\nread:\r\n");
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_rd[i]);
            }
            os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset,data_len,data_len);
            os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset,data_len,data_len);
            while(1);
        }
        else
        {
            os_printf("\r\n\r\n compare ok!:addr %x,len %x %d\r\n",offset,data_len,data_len);
        }
    }
    os_printf("\r\n\r\n test end\r\n\r\n");
    //while(1)
    {
        //timer_clear_watch_dog();
    }
}

void sd_test_rw(uint8 *data_wr, uint8 *data_rd)
{
    os_printf("%s\r\n\r\n", __func__);
    uint32 data_len;
    #if (WATCH_STORAGE_TYPE == WATCH_STORAGE_TYPE_SD)
    uint32 offset=0x2800000;
    #else
    uint32 offset=0x6CA200;
    #endif
    int cnt;
    int i;
    typedef struct
    {
        uint32 addr;
        uint32 len;
    }test_data_t;
    test_data_t  test_data[] =
    {   
        {0xCBE00,3596},
        {0xCBE00,3597},
        {0xCBE00,3598},
        {0xCBE00,3599},
        {0xCBE1A,3596},
        {0xCA200,3597},
        {0xCB00D,3597},        
        {0xC0E00,1097},
        {0xC1E00,1297},
        {0xC2E00,1497},
        {0xC3E00,1697},
        {0xC4E00,1897},
        {0xC5E00,2097},
        {0xCB600,2297},
        {0xCB700,2497},
        {0xCB800,2697},
        {0xCB900,2897},
        {0xCBA00,3097},
        {0xCBB00,3297},
        {0xCBC00,3397},
        {0xCBD00,3497},
        {0xCBEF0,3590},
        {0xCBEFF,3597},
        
        {0xCBE1A,3596},
        {0xCCC27,3596},
        {0xCDA34,3596},
        {0xCE841,3596},
        {0xCF64E,3596},
        {0xD045B,3596},
        {0xD1268,3596},
        {0xD2075,2943},
        {0x0D3E4,147},
        {0x0D3E4,147},
        {0x0E16C,168},
        {0x0D3E4,147},
        {0x0D3E4,147},
        {0x0E428,231},
        {0x76800,494},
        {0x76800,3360},
        {0x76800+3360*1,3360},
        {0x76800+3360*2,3360},
        {0x76800+3360*3,3360},
        {0x76800+3360*4,3360},
        {0x76800+3360*5,3360},
        {0x76801,3360},
    };
    
    for(int nnd=0; nnd<15; nnd++)
    {
        for(cnt = 0; cnt <(sizeof(test_data)/sizeof(test_data[0])); cnt++)
        {
            offset = test_data[cnt].addr + (nnd<<20);
            data_len=test_data[cnt].len;
            for(i=0; i<data_len; i++)
            {
                data_wr[i] = i+data_len;
            }
            data_wr[0] = data_len;  
            drv_sd_write(offset,data_wr,data_len);
            drv_sd_read(offset,data_rd,data_len);
            
            for(i=0; i<data_len; i++)
            {
                if(data_wr[i] != data_rd[i])
                {
                    break;
                }
            }
            if(i<data_len)
            {
                os_printf("\r\nwrite:data_len %d,err_pos %d,wr_val %x,rd_val %x\r\n",
                data_len,i,data_wr[i],data_rd[i]);
                for(i=0; i<data_len; i++)
                {
                    if(i%16 == 0)
                    {
                        os_printf("\r\n");
                    }
                    os_printf("%02x ",data_wr[i]);
                }
                os_printf("\r\nread:\r\n");
                for(i=0; i<data_len; i++)
                {
                    if(i%16 == 0)
                    {
                        os_printf("\r\n");
                    }
                    os_printf("%02x ",data_rd[i]);
                }
                os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset,data_len,data_len);
                os_printf("\r\n\r\n compare err!:addr %x,len %x %d\r\n",offset,data_len,data_len);
                while(1);
            }
            else
            {
                os_printf("\r\n\r\n compare ok!:addr %x,len %x %d\r\n",offset,data_len,data_len);
            }
        }
    }
     

    os_printf("\r\n\r\n test end\r\n\r\n");
    //while(1)
    {
        timer_clear_watch_dog();
    }
}

void sd_test(void)
{
    uint8 *data_wr=NULL;
    uint8 *data_rd=NULL;
    uint32 data_len;
    int i,j,len;
    int index = 0;
    sd_reinit();
    data_wr = jmalloc(8192, 1);
    if(data_wr == NULL)
    {
        os_printf("wr buf NULL \r\n");
        while(1);
    }
    os_printf("data_wr:%x\r\n",data_wr);
    data_rd = jmalloc(8192, 1);
    if(data_rd == NULL)
    {
        os_printf("rd buf NULL \r\n");
        while(1);
    }
    os_printf("data_wr:%x,data_rd:%x\r\n",data_wr,data_rd);

    for(index=9; index<4096; index+=9)
    {
        timer_clear_watch_dog();
        data_len = 8192;
        for(i=0; i<data_len; i++)
        {
            data_wr[i] = i+index;
        }
        drv_sd_write(SD_TEST_ADDRESS,data_wr,data_len);
        for(j=0; j<8192; j+=index)
        {
            if((j+index) < 8192)
            {
                len = index;
            }
            else
            {
                len = 8192-j;
            }
            drv_sd_read(SD_TEST_ADDRESS+j,data_rd+j,len);//drv_rd_cbk
            
        }
        
        for(i=0; i<data_len; i++)
        {
            if(data_wr[i] != data_rd[i])
            {
                break;
            }
        }
        if(i<data_len)
        {
            os_printf("\r\nwrite:data_len %d,err_pos %d,wr_val %x,rd_val %x\r\n",
                data_len,i,data_wr[i],data_rd[i]);
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_wr[i]);
            }
            os_printf("read:\r\n");
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_rd[i]);
            }
            
            os_printf("compare err,index:%d\r\n", index);
            while(1);
        }
        else
        {
            os_printf("compare ok,index:%d\r\n", index);
        }
    }
    

    data_len = 1;
    while(1)
    {
        //timer_clear_watch_dog();
           
        for(i=0; i<data_len; i++)
        {
            data_wr[i] = i;
        }
        data_wr[0] = data_len;

        drv_sd_write(SD_TEST_ADDRESS,data_wr,data_len);

        drv_sd_read(SD_TEST_ADDRESS,data_rd,data_len);
        
        for(i=0; i<data_len; i++)
        {
            if(data_wr[i] != data_rd[i])
            {
                break;
            }
        }
        if(i<data_len)
        {
            os_printf("\r\nwrite:data_len %d,err_pos %d,wr_val %x,rd_val %x\r\n",
                data_len,i,data_wr[i],data_rd[i]);
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_wr[i]);
            }
            os_printf("read:\r\n");
            for(i=0; i<data_len; i++)
            {
                if(i%16 == 0)
                {
                    os_printf("\r\n");
                }
                os_printf("%02x ",data_rd[i]);
            }
            os_printf("\r\n\r\n compare err!:%d\r\n\r\n",data_len);
        }
        else
        {
            os_printf("\r\n\r\n compare ok!:%d\r\n\r\n",data_len);
        }
        if(data_len == 1)
        {
            data_len = 2;
        }
        else if(data_len == 2)
        {
            data_len = 3;
        }
        else if(data_len == 3)
        {
            data_len = 4;
        }
        else if(data_len == 4)
        {
            data_len = 127;
        }
        else if(data_len == 127)
        {
            data_len = 128;
        }
        else if(data_len == 128)
        {
            data_len = 256;
        }
        else if(data_len == 256)
        {
            data_len = 257;
        }
        else if(data_len == 257)
        {
            data_len = 512;
        }
        else if(data_len == 512)
        {
            data_len = 1024;
        }
        else if(data_len == 1024)
        {
            data_len = 2048;
        }
        else if(data_len == 2048)
        {
            data_len = 3000;
        }
        else if(data_len == 3000)
        {
            data_len = 4090;
        }
        else if(data_len == 4090)
        {
            data_len = 4096;
        }
        else if(data_len == 4096)
        {
            data_len = 8192;
        }
        else if(data_len == 8192)
        {
            break;
        }
    }
    
    
    sd_test_rw(data_wr,data_rd);
    sd_test_rw2(data_wr,data_rd);
    sd_test_rw3(data_wr,data_rd);
    jfree(data_wr);    
    jfree(data_rd);
    os_printf("\r\n\r\n test end\r\n\r\n");
    while(1)
    {
        timer_clear_watch_dog();
    }
}


#endif

// EOF
