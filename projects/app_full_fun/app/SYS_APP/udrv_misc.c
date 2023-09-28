

#include <stdint.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "u_include.h"


#if LED_RGB_NUM

#include <stdio.h>
#include <string.h>
#include "bkreg.h"
#include "driver_dma.h"
#include "driver_gpio.h"
#include "drv_audio.h"
#include "drv_system.h"
#include "driver_ringbuff.h"
#include "driver_i2s.h"
#include "udrv_misc.h"

#ifndef LED_RGB_IO
    #error "LED_RGB_IO not defined!!!"
#endif

#define I2S_LOG_OUT     DBG_LOG_INFO// DBG_LOG_INFO or COM_LOG_INFO
#ifndef I2S_LOG_OUT
    #define I2S_LOG_OUT(...)
#endif
#define I2S_DMA_ASO_DEBUG           0

#if I2S_DMA_ASO_DEBUG
    #define I2S_DATA_WIDTH          24
    #define I2S_CLOCK_ENABLE
#else
    #define I2S_DATA_WIDTH          32
#endif

/*
- I2S:[BCK:3.072MHz(325.5ns), channel data width:32bit]
    - [Note]: I2S data tran format: R-L-R-L...
        - exp: uint32_t dat[10] -> data format: dat[1]--dat[0]--dat[3]--dat[2]--dat[5]--dat[4]--...

- RGB LED TX1812：
    - reset   : Low level >=80us    [80us/325.5ns = 245.8 ~ 246(unit:i2s_bck) ~ 30.625Byte -> 32Byte]
    - code0   : H-300ns-L-900ns     [1 led_bit -> 4 * i2s_bck -> 0.5 Byte]
    - code1   : H-900ns-L-300ns     [1 led_bit -> 4 * i2s_bck -> 0.5 Byte]
    - [Note]: code H&L time limit (300ns:220~380ns, 900ns:800ns~1000ns)
    - LED data format: 
        - reset(32Byte 0x00)
        - LED0 : Green: 8*led_bit(4Byte) -- Red: 8*led_bit(4Byte) -- Blue: 8*led_bit(4Byte)
        - LED1 : Green: 8*led_bit(4Byte) -- Red: 8*led_bit(4Byte) -- Blue: 8*led_bit(4Byte)
        - LED2 : Green: 8*led_bit(4Byte) -- Red: 8*led_bit(4Byte) -- Blue: 8*led_bit(4Byte)
        - ...
*/

#define RGB_LED_CODE0   0x8  //H-300ns-L-900ns
#define RGB_LED_CODE1   0xE  //H-900ns-L-300ns

typedef struct _RGB_LED_DAT_t{
    uint32_t G;
    uint32_t R;
    uint32_t B;
}RGB_LED_DAT_t;
uint8_t LED_RGB[LED_RGB_NUM][3];

typedef struct _RGB_LED_t{
	uint8_t reset1[32];
    RGB_LED_DAT_t led[LED_RGB_NUM];
    uint8_t reset2[32];
#if (LED_RGB_NUM % 2) //for 8byte aligned
    uint32_t rsvd;
#endif
}__attribute__((aligned(8))) RGB_LED_t;
//#define I2S_RING_BUFF_SIZE          (48 * 4)//1ms*4byte*2ch*2frame
#define I2S_RING_BUFF_SIZE	    sizeof(RGB_LED_t)//(LED_RGB_NUM*12+32)

RingBufferContext aud_i2s0_tx_rb;
uint8_t aud_i2s0_tx_ringbuf[I2S_RING_BUFF_SIZE] __attribute__((aligned(4)));

//==============================================================
void RgbLedShow(void)
{
    RGB_LED_t *p_rgb_led = (RGB_LED_t*)aud_i2s0_tx_ringbuf;
    app_i2s0_open(0);
    memset(p_rgb_led, 0, sizeof(RGB_LED_t));
    // memset(p_rgb_led, 0, I2S_RING_BUFF_SIZE);
    for(int i = 0; i < LED_RGB_NUM; i++){
        //get color type
        uint8_t color_R = LED_RGB[i][0];
        uint8_t color_G = LED_RGB[i][1];
        uint8_t color_B = LED_RGB[i][2];
        //update color type to dma buffer
        for(int bit = 0; bit < 8; bit++){
            int color_bit = bit << 2;
            p_rgb_led->led[i].R |= ((uint32_t)((color_R & 0x1) ? RGB_LED_CODE1 : RGB_LED_CODE0) << color_bit);
            p_rgb_led->led[i].G |= ((uint32_t)((color_G & 0x1) ? RGB_LED_CODE1 : RGB_LED_CODE0) << color_bit);
            p_rgb_led->led[i].B |= ((uint32_t)((color_B & 0x1) ? RGB_LED_CODE1 : RGB_LED_CODE0) << color_bit);
            color_R >>= 1;
            color_G >>= 1;
            color_B >>= 1;
        }
    }
    //for I2S data tran format: R-L-R-L..., below convert to L-R-L-R...
    uint32_t tmp;
    uint32_t *p = (uint32_t*)p_rgb_led;
    for(int i = 0; i < (I2S_RING_BUFF_SIZE / 4 / 2); i++){
        tmp = p[2*i];
        p[2*i] = p[2*i+1];
        p[2*i] = tmp;
    }
    app_i2s0_open(1);
}

//==============================================================
void RgbLedOneColour(uint8_t n, uint8_t c)
{
	uint8_t LED_RGB1[5][3]={{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255}};
	memcpy(&LED_RGB[n][0], &LED_RGB1[c][0], 3);
	RgbLedShow();
}
//==============================================================
void RgbLedAllColour(uint8_t val)
{
	uint8_t LED_RGB1[5][3]={{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255}};
	I2S_LOG_OUT("RgbShowNum:%d\n",val);
	for(uint8_t i=0; i<LED_RGB_NUM; i++){
	    memcpy(&LED_RGB[i][0], &LED_RGB1[val][0], 3);
	}
	RgbLedShow();
}

//**********************************************************************
void RgbLedOutShift(void)
{
    uint8_t LED_RGB1[1][3];
    memcpy(&LED_RGB1[0][0], &LED_RGB[0][0], 3);
    for(uint8_t i=0; i<19; i++){
    	memcpy(&LED_RGB[i][0], &LED_RGB[i+1][0], 3);
    }
    memcpy(&LED_RGB[19][0], &LED_RGB1[0][0], 3);
    RgbLedShow();
}

//**********************************************************************
void user_i2s0_dma_init(uint8_t en, uint8_t dma_loop_en)
{
    RingBufferContext* p_rb = &aud_i2s0_tx_rb;
    uint8_t* p_rb_buf = (uint8_t*)((uint32_t)&aud_i2s0_tx_ringbuf);
    I2S_LOG_OUT("%s(%u)\n", __FUNCTION__, en);
    if(en)
    {
        void* dma = dma_channel_malloc();
        if(dma){
            DBG_LOG_INFO("%s: DMA%d:0x%08X\n", __FUNCTION__, (((uint32_t)(dma) - MDU_GENER_DMA_BASE_ADDR) / 0x20), (uint32_t)dma);
        }else{
            DBG_LOG_ERR("dma malloc fail!!!\n");
        }
        DMA_REQ dma_req = DMA_REQ_I2S0_TX1;//GPIO21
        volatile uint32_t *dst_addr = &REG_I2S0_PCM_DAT;
        if(LED_RGB_IO == GPIO22){
            dma_req = DMA_REQ_I2S0_TX2;
            dst_addr = &REG_I2S0_PCM_DAT2;
        }else if(LED_RGB_IO == GPIO23){
            dma_req = DMA_REQ_I2S0_TX3;
            dst_addr = &REG_I2S0_PCM_DAT3;
        }else if(LED_RGB_IO != GPIO21){	//yuan edit
            DBG_LOG_ERR("\n\n\n LED_RGB_IO config err!!!\n");
        }

        dma_channel_config(dma,
                        dma_req,
                        dma_loop_en ? DMA_MODE_REPEAT : DMA_MODE_SINGLE,
                        (uint32_t)p_rb_buf,
                        (uint32_t)p_rb_buf + I2S_RING_BUFF_SIZE,
                        DMA_ADDR_AUTO_INCREASE,
                        DMA_DATA_TYPE_LONG,
                        (uint32_t)dst_addr,
                        (uint32_t)dst_addr,
						DMA_ADDR_NO_CHANGE,
                        DMA_DATA_TYPE_LONG,
                        I2S_RING_BUFF_SIZE
                        );

        memset(p_rb_buf, 0, I2S_RING_BUFF_SIZE);
        ring_buffer_init(p_rb, (uint8_t *)p_rb_buf, I2S_RING_BUFF_SIZE, dma, RB_DMA_TYPE_READ);
        // dma_channel_enable(dma, 1);
        // dma_channel_enable(dma, 0);
    }
    else
    {
        dma_channel_free(p_rb->dma);
        p_rb->dma = NULL;
    }
}

//**********************************************************************
void app_i2s0_init(uint8_t en)
{
	for(uint8_t j=0; j<LED_RGB_NUM; j++){
		LED_RGB[j][0] = 0;
		LED_RGB[j][1] = 0;
		LED_RGB[j][2] = 0;
	}
//    #define IO_I2S0_BCLK    GPIO18
//    #define IO_I2S0_SCLK    GPIO19
//    #define IO_I2S0_DIN     GPIO20
//    #define IO_I2S0_DOUT    GPIO21
//    #define IO_I2S0_DOUT2    GPIO22
//    #define IO_I2S0_DOUT3    GPIO23

    //i2s_init
    if(en)
    {
    #ifdef I2S_CLOCK_ENABLE
    	gpio_config_new(IO_I2S0_BCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
    	gpio_config_new(IO_I2S0_SCLK, GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
    #endif
        // gpio_config_new(IO_I2S0_DIN,  GPIO_INPUT,  GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
#if (LED_RGB_IO==21)	//GPIO21
        gpio_config_new(LED_RGB_IO, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC1);
#else
        gpio_config_new(LED_RGB_IO, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_FUNC2);
#endif
        gpio_config_capacity(LED_RGB_IO, GPIO_DRV_20mA);
        system_mem_clk_enable(SYS_MEM_CLK_I2S0);
        system_peri_clk_enable(SYS_PERI_CLK_I2S0);
        system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2S0);
        system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_I2S0);
    }
    else
    {
        system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_I2S0);
        system_peri_mcu_irq_disable(SYS_PERI_IRQ_I2S0);
        system_peri_clk_disable(SYS_PERI_CLK_I2S0);
        system_mem_clk_disable(SYS_MEM_CLK_I2S0);
    #ifdef I2S_CLOCK_ENABLE
        gpio_config_new(IO_I2S0_BCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
        gpio_config_new(IO_I2S0_SCLK, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
    #endif
        // u_gpio_config(IO_I2S0_DIN,  GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
        gpio_config_new(LED_RGB_IO, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
    }
    if(en)
    {
        uint8_t ret = I2S_ERROR_CODE_OK;
        ret = i2s_init(I2S0, I2S_ROLE_MASTER, 48000, I2S_DATA_WIDTH, 32);//datawidth = 16/24bit, bus width=32bit
        if(ret != I2S_ERROR_CODE_OK){
            DBG_LOG_ERR("I2S0 init error %s():%d\n", __FUNCTION__, ret);
        }

        i2s_ctrl(I2S0, I2S_CTRL_CMD_LR_COM_STORE, 0);
        //LRCK INV->(def 0：H_data1 -- L_data0 -- H_data3 -- L_data2...)/(1：L_data1 -- H_data0 -- L_data3 -- H_data2...)
        // i2s_ctrl(I2S0, I2S_CTRL_CMD_SET_LR_CLK_INV, 1);
        // i2s_ctrl(I2S0, I2S_CTRL_CMD_SET_RX_THRESHOLD, I2S_RX_INT_LEVEL >> 3);//0:1, 1:8, 2:16, 3:24
        i2s_ctrl(I2S0, I2S_CTRL_CMD_SET_TX_THRESHOLD, 8 >> 3);//0:1, 1:8, 2:16, 3:24
    }
    user_i2s0_dma_init(en, 0);
}

//**********************************************************************
void app_i2s0_open(uint8_t en)
{
    RingBufferContext* p_rb = &aud_i2s0_tx_rb;
    ring_buffer_reset(p_rb);
    if(en){
        i2s_ctrl(I2S0, I2S_CTRL_CMD_CLR_TX_FIFO, 1);
        dma_channel_enable(p_rb->dma, en); //i2s out
        i2s_enable(I2S0, en);
        i2s_ctrl(I2S0, I2S_CTRL_CMD_RX_TRIG, 0);
        // i2s0_tx_dma_loop_start();
    }else{
        dma_channel_enable(p_rb->dma, en); //i2s out
        // i2s0_tx_dma_loop_stop();
        i2s_enable(I2S0, en);
    }
}

#if 0//for test
void user_app_i2s0_init(uint8_t en)
{
    I2S_LOG_OUT("%s(%d)\n", __FUNCTION__, en);
    app_i2s0_init(en);
    // dma_channel_ctrl(aud_i2s0_tx_rb.dma, DMA_CTRL_CMD_FINISH_INT_EN, 1);
    app_i2s0_open(en);
#if 0//I2S_DMA_ASO_DEBUG
    // Audio sin data 1000Hz@(48000Hz,24bit, -3.0dBFS), sample_num: 48 point@(1 period), peak_val = 5938679.
    int32_t sin_data_24bit[48] = {
    0x00000000, 0x000BD3F1, 0x00177413, 0x0022AD7A, 0x002D4EFB, 0x00372A06, 0x00401370, 0x0047E42F, 0x004E7A06, 0x0053B81F, 0x00578783, 0x0059D780,
    0x005A9DF6, 0x0059D780, 0x00578783, 0x0053B820, 0x004E7A06, 0x0047E42E, 0x00401370, 0x00372A06, 0x002D4EFA, 0x0022AD79, 0x00177412, 0x000BD3F1,
    0x00000000, 0xFFF42C0F, 0xFFE88BED, 0xFFDD5285, 0xFFD2B104, 0xFFC8D5FA, 0xFFBFEC91, 0xFFB81BD2, 0xFFB185F9, 0xFFAC47E0, 0xFFA8787D, 0xFFA62880,
    0xFFA5620A, 0xFFA62880, 0xFFA8787D, 0xFFAC47E1, 0xFFB185FB, 0xFFB81BD2, 0xFFBFEC92, 0xFFC8D5FA, 0xFFD2B106, 0xFFDD5287, 0xFFE88BEC, 0xFFF42C12};
    // int i = 48; while(i--) sin_data_24bit[i] = i;
    memcpy(&aud_i2s0_tx_ringbuf[0], sin_data_24bit, sizeof(sin_data_24bit));
#else
    for(uint8_t i=0; i<(I2S_RING_BUFF_SIZE/4); i++){
    	if(i<8)		sin_data_24bit[i] = 0;
    		else	sin_data_24bit[i] = 0x88888888;
    }
    memcpy(&aud_i2s0_tx_ringbuf[0], sin_data_24bit, sizeof(sin_data_24bit));

#endif

    i2s0_tx_dma_loop_start();
}
#endif

//**********************************************************************
void i2s0_tx_dma_loop_start(void)
{
    dma_channel_src_curr_address_set(aud_i2s0_tx_rb.dma, (uint32_t)&aud_i2s0_tx_ringbuf[I2S_RING_BUFF_SIZE]);
}

//**********************************************************************
void i2s0_tx_dma_loop_stop(void)
{
    dma_channel_src_curr_address_set(aud_i2s0_tx_rb.dma, (uint32_t)&aud_i2s0_tx_ringbuf[0]);
}

#endif /* AUDIO_I2S_EN */







/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
#ifdef CONFIG_USER_SPI_FUNC

#if (CONFIG_USER_SPI_FUNC & 1)
    #define USER_SPIx           0//0,1,2
    #define USER_SPI_NAME       SPI_0_FUN_1
#elif (CONFIG_USER_SPI_FUNC & 2)
    #define USER_SPIx           1
    #define USER_SPI_NAME       SPI_1_FUN_1
#elif (CONFIG_USER_SPI_FUNC & 8)
    #define USER_SPIx           2
    #define USER_SPI_NAME       SPI_2_FUN_4//SPI_2_FUN_1
#endif

#define USER_SPI_MASTER_EN      1
#define USER_SPI_3wire_EN       1//no cs pin
#define USER_SPI_PULL_EN        0//(defined(SPI_SIMPLE_DRV))//1:no dma & intrrupt, bug not fixed, do not use


#if (!USER_SPI_PULL_EN)
static spi_handle user_spi_handle;
static volatile uint8_t s_usr_spi_busy_flag;
static spi_param_t user_spi_param = {
    .mode = USER_SPI_MASTER_EN ? SPI_Master : SPI_Slave,
    .bit_wdth = SPI_BIT_8,//SPI_BIT_16
    .wire = USER_SPI_3wire_EN ? SPI_WIRE_3 : SPI_WIRE_4,//SPI_WIRE_3 -> no cs pin
    .lsb_first = SPI_MSB_FIRST_SEND,
    .clk_polarity = SPI_POL_HIGH,
    .clk_pha = SPI_PHA_SECOND,
    .baud_rate = SPI_BAUD_6M5HZ,//SPI_DEFAULT_BAUD,
    .io_cs_en = USER_SPI_3wire_EN ? 0 : 1,
    .io_clk_en = 1,
    .io_miso_en = 1,
    .io_mosi_en = 1,
};

static void user_spi_trans_cb(spi_handle handle, spi_transfer_object *trans_obj, void *arg)
{
    // if(handle == user_spi_handle)
        s_usr_spi_busy_flag = 0;
}

int user_spi_is_busy(void)
{
    return s_usr_spi_busy_flag;
}
#endif

void user_spi_init(uint8_t spi_16bit_en)
{
//	DBG_LOG_FUNC(" %s, spi_16bit_en:%d\n", __func__, spi_16bit_en);
#if USER_SPI_PULL_EN
    hal_spi_init(USER_SPIx, USER_SPI_MASTER_EN, spi_16bit_en, USER_SPI_3wire_EN);
#else
    if(spi_16bit_en) user_spi_param.bit_wdth = SPI_BIT_8;
    user_spi_handle = spi_init(USER_SPI_NAME, (spi_param_t *)&user_spi_param);
#endif
}

void user_spi_uninit(void)
{
#if (!USER_SPI_PULL_EN)
    spi_dirver_err_code err = spi_uninit(user_spi_handle);
    if(err != spi_drv_err_none){
        DBG_LOG_ERR(" %d\n", err);
    }
    s_usr_spi_busy_flag = 0;
#endif
}

/** @brief write and read
 * @param tx_sz size, unit:byte
 * @param rx_sz size, unit:byte
 * @param tx_addr_fixed 1:fixwd tx dma src addr, used for lcd pure color fill
 * @note suggest ensure (user_spi_is_busy() == 0) before call
 * for spi slave tx mode, the first data is fixed 0x72(8bit mode)/0x7232(16bit mode), it's should be a hardware bug. the rx mode is normal.
 * so for the bug, the actualy tansfer tx size is (tx_sz - 1) byte at 8bit mode, or (tx_sz - 2) in 16bit mode.
 * @example if in slave device, your tx data is uint8_t/uint16_t slave_tx_buf[5] = {1,2,3,4,5}, the code shoule refer below:
 * in mastrer device:
 *  uint8_t/uint16_t master_rx_buf[6];
 *  user_spi_transfer(NULL, 0, master_rx_buf, sizeof(master_rx_buf))
 * in slave device:
 *  uint8_t/uint16_t slave_tx_buf[5] = {1,2,3,4,5};
 *  user_spi_transfer(slave_tx_buf, sizeof(slave_tx_buf), NULL, 0)
 * */
int user_spi_transfer(uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz, uint8_t tx_addr_fixed)
{
#if USER_SPI_PULL_EN
    hal_spi_read_write(USER_SPIx, tx_buf, tx_sz, rx_buf, rx_sz);
#else
    spi_dirver_err_code err = spi_drv_err_none;
    spi_transfer_object trans = {
        .trans_mode = SPI_TRANS_CALLBACK | SPI_TRANS_USE_DMA_FLAG,
        // .trans_mode = SPI_TRANS_BLOCKING | SPI_TRANS_USE_DMA_FLAG,
        .tx_buffer = tx_buf,
        .tx_len = tx_sz >> (user_spi_param.bit_wdth == SPI_BIT_16),
        .rx_buffer = rx_buf,
        .rx_len = rx_sz >> (user_spi_param.bit_wdth == SPI_BIT_16),
        .tx_dma_addr_fixed = tx_addr_fixed,
    };

    // while(s_usr_spi_busy_flag) { sys_delay_us(2); }
    if(s_usr_spi_busy_flag && (trans.trans_mode & SPI_TRANS_CALLBACK)){
        DBG_LOG_ERR("last busy flag exist:%d\n", s_usr_spi_busy_flag);
    }
    s_usr_spi_busy_flag = 1;
    err = spi_transfer(user_spi_handle, &trans, NULL, user_spi_trans_cb);
    if(err != spi_drv_err_none){
        DBG_LOG_ERR("spi_transfer:%d\n", err);
        return -1;
    }
#endif
    return 0;
}

//*************************************************************************************************
int user_spi1_transfer(uint16_t* tx_buf, int tx_sz, uint16_t* rx_buf, int rx_sz, uint16_t tx_addr_fixed)
{
#if USER_SPI_PULL_EN
    hal_spi_read_write(USER_SPIx, tx_buf, tx_sz, rx_buf, rx_sz);
#else
    spi_dirver_err_code err = spi_drv_err_none;
    spi_transfer_object trans = {
        .trans_mode = SPI_TRANS_CALLBACK | SPI_TRANS_USE_DMA_FLAG,
        // .trans_mode = SPI_TRANS_BLOCKING | SPI_TRANS_USE_DMA_FLAG,
        .tx_buffer = tx_buf,
        .tx_len = tx_sz >> (user_spi_param.bit_wdth == SPI_BIT_16),
        .rx_buffer = rx_buf,
        .rx_len = rx_sz >> (user_spi_param.bit_wdth == SPI_BIT_16),
        .tx_dma_addr_fixed = tx_addr_fixed,
    };

    // while(s_usr_spi_busy_flag) { sys_delay_us(2); }
    if(s_usr_spi_busy_flag && (trans.trans_mode & SPI_TRANS_CALLBACK)){
        DBG_LOG_ERR("last busy flag exist:%d\n", s_usr_spi_busy_flag);
    }
    s_usr_spi_busy_flag = 1;
    err = spi_transfer(user_spi_handle, &trans, NULL, user_spi_trans_cb);
    if(err != spi_drv_err_none){
        DBG_LOG_ERR("spi_transfer:%d\n", err);
        return -1;
    }
#endif
    return 0;
}


#endif //CONFIG_USER_SPI_FUNC



/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/

#ifdef IR_TX//使用一路PWM作为定时器，另一路PWM产生38KHz载波，在定时器中断中更新载波。

// #define PWM_DEBUG

//=========================================================================================
#if defined(IR_TX_PWM_CARRI)//38KHz的 50% 占空比的 PWM 方波

#define USER_PWM_IO             IR_TX_PWM_CARRI
#define USER_PWM_FREQ_Hz        38000   //Hz
#define USER_PWM_DUTY_ON        50      //percent

#define PWM_REG_VAL_CYCLE       ((uint32_t)(26000000 / USER_PWM_FREQ_Hz))
#define PWM_REG_VAL_DUTY        ((uint32_t)(PWM_REG_VAL_CYCLE * USER_PWM_DUTY_ON / 100))

void ir_tx_carrier_init(void) { hal_pwm_wave_init(USER_PWM_IO, 1, PWM_REG_VAL_DUTY, PWM_REG_VAL_CYCLE, NULL); }
void ir_tx_carrier_out_en(uint8_t en) { hal_pwm_enable(USER_PWM_IO, en); }
#endif

//=========================================================================================
#if defined(IR_TX_PWM_TIMER)//使用PWM作为定时器，在定时器中断中控制PWM载波

#define USER_TIM_PWMIO          IR_TX_PWM_TIMER

void user_timer_isr(void)
{
#ifdef PWM_DEBUG
    static uint32_t duty = PWM_REG_VAL_DUTY;
    if(++duty > PWM_REG_VAL_CYCLE) duty = 0;
    hal_pwm_duty_set(USER_PWM_IO, duty);
    REG_GPIO_0x20 = 2;
    REG_GPIO_0x20 = 0;
#else
    extern void UserIrTxFun();
    UserIrTxFun();
#endif
}
void user_timer_init(uint32_t us, uint8_t intr_en) { hal_pwm_timer_init(USER_TIM_PWMIO, 26 * us, user_timer_isr); }
void user_timer_update(uint32_t us) { hal_pwm_timer_cycle_updt(USER_TIM_PWMIO, 26 * us); }
void user_timer_enable(uint8_t en) { hal_pwm_enable(USER_TIM_PWMIO, en); }
#endif

//=========================================================================================

void ir_tx_init(uint8_t en)
{
#ifdef PWM_DEBUG
    ir_tx_carrier_init();
    ir_tx_carrier_out_en(1);
    user_timer_init(1000, 1);
    user_timer_enable(1);
#else
    if(en)
    {
        ir_tx_carrier_init();
#ifndef ZT_M184
        user_timer_init(1000, 1);
        user_timer_enable(1);
#endif
    }
    else
    {
#ifndef ZT_M184
        user_timer_enable(0);
#endif
        ir_tx_carrier_out_en(0);
    }
#endif
}

void pwm_set(uint32_t us, uint8_t OnOff)
{
#ifndef ZT_M184
	user_timer_update(us);	//reinit timer intr time(unit:us)
#endif
	ir_tx_carrier_out_en(OnOff);
}


#endif /* IR_TX */
