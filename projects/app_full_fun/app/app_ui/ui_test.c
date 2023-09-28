/**********************************************************************
* 文件:	user_ui.c
* 作者:	
* 版本:	V1.0
* 日期： 20230423
* 描述： 测试工程
**********************************************************************/
#include "u_config.h"

#ifdef PTN101x_PRJ_TEST

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "u_include.h"
#include "u_key.h"
#include "udrv_saradc.h"
#ifdef CONFIG_AUD_AMP_DET_FFT
#include "app_aud_amp_det.h"
#endif
#if CONFIG_DSP_EFFECT_EN
#include "USER_Config.h"
#include "ui_test_def.h"
void dsp_effet_loop(void);
// uint8_t MusVolVal;
#endif

#define USER_DBG_INFO(fmt,...)       os_printf("[USER]"fmt, ##__VA_ARGS__)

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
////////////////////////////////////////////////// test
// #define CPU_ARITHMETIC_TEST //float int32 uint32
#ifdef CPU_ARITHMETIC_TEST
void cpu_arithmetic_test(void);
#endif

//////////////////////////////////////////////////
void key_function(KEY_CONTEXT_t *pKeyArray);
void saradc_debug_show(uint8_t soft_mode_en);

#ifdef CONFIG_CHARGE_INNER
//**** charge only process , before pre init ***********************
void app_charge_mini_sched(void)
{
    #define CHRG_CLK_DIV        (CPU_SLEEP_CLK_DIV * 8)
    #define chrg_delay_us(us)   sys_delay_cycle(us * 26 / CHRG_CLK_DIV)
    #define chrg_delay_ms(ms)   sys_delay_cycle(ms * (26000 / CHRG_CLK_DIV))

    uint16_t vbat = app_charge_check_vbat();
    USER_DBG_INFO("VBAT:%d, VbusPwr:%d\n", vbat, vusb_is_charging());
    if(!((vbat < POWERON_VBAT_THRESHOLD) && vusb_is_charging())) return;//if vbus no power in
    //TO DO, if power key pressed, return
    USER_DBG_INFO("charge only start...\r\n");

    app_env_cali_data_prt();
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CHRG_CLK_DIV);
    
    set_flash_clk(FLASH_CLK_26mHz);

    while(1) //charge only schedual
    {
        saradc_debug_show(1);
        watch_dog_clear();
        chrg_delay_ms(10);
    }
}
#endif

//**** pre init ***********************
void user_init_prv(void)
{
    // saradc_volt_direct_get(SARADC_CH_GPIO19);
}

//**** normal init ********************
void user_init(void)
{
    // user_key_init(key_function);
    USER_DBG_INFO("%s build @ %s %s\n", FILE_NAME(__FILE__), __DATE__,__TIME__);
#ifdef CONFIG_AUD_AMP_DET_FFT
    aud_mag_det_enable(1);
#endif
#if CONFIG_DSP_EFFECT_EN
	//===========================================
	UserFlash_Init(25);	// 設置FLASH 傳入版本參數

	// MusVolVal = SysInf.MusVol32;
	// if(MusVolVal<3)MusVolVal=3;

	USER_DBG_INFO("======== V12 user_init End, WorkMode:%d\n", SysInf.WorkMode);
    USER_DBG_INFO("APP %s Build Time @ %s %s\n", FILE_NAME(__FILE__), __DATE__, __TIME__);

    USER_DBG_INFO("=lib_demo_version:%s\n", lib_demo_version_get());
    USER_DBG_INFO("====lib_effet_proc:%d\n", lib_effet_proc(3));
    uint8_t test[5] = {0,0,0,0,0};
    USER_DBG_INFO("====lib_effet_proc1:%d  %d\n", lib_effet_proc1(test,5),test[0] );

    
    IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],32);	//打開提示音音頻鏈路
    IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],32);
    IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],32);
    IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);
    app_wave_file_play_start(APP_WAVE_FILE_ID_START);
#endif
}

//********************************
void user_init_post(void)
{
	//=========================
    // extern void dsp_audio_init(void);
    // dsp_audio_init();
#ifdef USR_UART1_INIT_EN
    hal_uart1_init(GPIO16, GPIO17, 115200, 1, 1);
#endif
#ifdef USR_UART2_INIT_EN
    hal_uart2_init(115200, 1, 1);
#endif
#ifdef CONFIG_USER_SPI_FUNC
    user_spi_init(0);
#endif

#ifdef LED_DRIVER_BY_I2S0_TX
    extern void user_app_i2s0_init(uint8_t en);
	user_app_i2s0_init(1);
#endif
#ifdef DEBUG_IC_TEMPERATURE
    debug_show_temp_senser();
#endif

#ifdef IR_TX
    ir_tx_init(1);
    ir_tx_init(0);
#endif

    #ifdef CONFIG_DISK_SDCARD
    if(gpio_input(SDCARD_DETECT_IO) == SDCARD_DETECT_LVL)//如果有插SD卡，切SD卡禁止跑蓝牙流程
    {
        if(sd_is_attached()) system_work_mode_set_button(SYS_WM_SDCARD_MODE);
        app_bt_flag3_set(PWR_ON_BT_SCAN_DIS, 1);
    }
    #endif
#if 0//def CONFIG_DISK_UDISK
    udisk_mode_auto_sw_set(0);
    udisk_mode_auto_play_set(0);
#endif
#ifdef SPDIF_GPIO
    spdif_mode_auto_sw_set(1);
    spdif_mode_auto_exit_set(1);
    spdif_mode_auto_play_set(1);
#endif
}

//**********************************************
#ifdef CONFIG_USER_SPI_FUNC
short spi_tx_buf[5] = {1,2,3,4,5};
short spi_rx_buf[5] = {1,2,3,4,5};
#endif
void user_loop(void)
{
    static uint32_t main_loop_cnt;
    main_loop_cnt++;
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10))//10ms task
    {
        t_mark = sys_time_get();
        extern void user_loop_10ms(void);
        user_loop_10ms();
        static uint32_t t_mark_1sec = 0;
        if(sys_timeout(t_mark_1sec, 1000))
        {
            t_mark_1sec = sys_time_get();
            // DBG_LOG_INFO("main_loop_cnt = %ld\n", main_loop_cnt);
            DBG_REG_SET(main_loop_cnt);
            main_loop_cnt = 0;
            #ifdef CPU_ARITHMETIC_TEST
            cpu_arithmetic_test();
            #endif
            #if 0//debug show bt state
            for(int idx=0; idx<BT_MAX_AG_COUNT; idx++){         
                LOG_I(BT,"%d bt_state:%d, event:%d\r\n", idx, bt_app_entity_get_state(idx), bt_app_entity_get_event(idx));
            }
            #endif
            #ifdef DEBUG_IC_TEMPERATURE
            debug_show_temp_senser_proc();
            #endif

        #ifdef FFT_RUN_TEST
            extern void fft_test(void);
            fft_test();
        #endif
            // app_player_debug_show();
        }
        #ifdef CONFIG_USER_SPI_FUNC
        while(user_spi_is_busy());
        // user_spi_read((uint8_t*)spi_rx_buf, sizeof(spi_rx_buf));
        user_spi_write((uint8_t*)spi_tx_buf, sizeof(spi_tx_buf), 0);
        // user_spi_transfer((uint8_t*)spi_tx_buf, sizeof(spi_tx_buf), (uint8_t*)spi_rx_buf, sizeof(spi_rx_buf), 0);
        #endif
    }
}

//*******************************************
#ifdef CONFIG_SYS_TICK_INT_1MS
void tick_task_1ms(void) //timer0 1ms isr
{
    static uint8_t task_T4ms = 0;
    if((task_T4ms++ & 3) == 0)
    {
        #ifdef SarAdcChVal
		user_saradc_update_trig();
        #endif
    }
}
#endif


//********************************
void user_loop_10ms(void)
{
    saradc_debug_show(DBG_SarADC_MOD_SOFT);
    user_key_scan();

#ifdef CONFIG_AUD_REC_AMP_DET
    void user_aud_amp_task(void);
    user_aud_amp_task();
#endif
#if CONFIG_DSP_EFFECT_EN
    dsp_effet_loop();
#endif
}



void key_function(KEY_CONTEXT_t *pKeyArray)
{
    if(pKeyArray->event != KEY_L_PRESSING) USER_DBG_INFO("key%d: %d\n", pKeyArray->index, pKeyArray->event);
    if(pKeyArray->index == 0){
        switch (pKeyArray->event){
        case KEY_S_PRES:
#ifdef IR_TX
        	ir_tx_init(1);
#endif
            // app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
        	break;
        case KEY_S_REL:
            // system_work_mode_change_button();
            // app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
            // csm_hid_key_send(CSM_PLAY);
        break;
        case KEY_D_CLICK:
        {
            #ifdef TEST_USER_BLE_TX
            extern void test_ble_fff0s_send(void);
            test_ble_fff0s_send();
            #endif
            // app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0);
        }break;
        case KEY_T_CLICK:
            // app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
        break;
        case KEY_Q_CLICK:
            // csm_hid_key_send(CSM_STOP);
        break;
        case KEY_5_CLICK:
            app_env_clear_all_key_info();
        break;
        case KEY_L_PRES:break;
        case KEY_L_PRESSING:break;
        case KEY_L_REL:break;
        default:break;
        }
    }else if(pKeyArray->index == 1){
    }else if(pKeyArray->index == 4){
    }
}




/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#ifdef CPU_ARITHMETIC_TEST

#define TEST_MARK_BEGIN     REG_GPIO_0x0E = 2;//gpio output 2, no need to init
#define TEST_MARK_END       REG_GPIO_0x0E = 0;//gpio output 0, no need to init

#define TMP_PI      ((float)3.1415926)
#define TMP_NUM     10
#define arith_run_test(buff, len, val)   do\
{\
    int i = 0;\
    TEST_MARK_BEGIN\
    for(i = 0; i < len; i++) buff[i] = val;\
    TEST_MARK_END\
    TEST_MARK_BEGIN\
    for(i = 0; i < len; i++) buff[i] += val;\
    TEST_MARK_END\
    TEST_MARK_BEGIN\
    for(i = 0; i < len; i++) buff[i] -= val;\
    TEST_MARK_END\
    TEST_MARK_BEGIN\
    for(i = 0; i < len; i++) buff[i] *= val;\
    TEST_MARK_END\
    TEST_MARK_BEGIN\
    for(i = 0; i < len; i++) buff[i] /= val;\
    TEST_MARK_END\
}while(0)

void float_test(float* buff, int len) { arith_run_test(buff, len, TMP_PI); }
void int_test(int* buff, int len) { arith_run_test(buff, len, TMP_NUM); }
void uint_test(unsigned int* buff, int len) { arith_run_test(buff, len, TMP_NUM); }
void int16_test(short* buff, int len) { arith_run_test(buff, len, TMP_NUM); }
void uint16_test(unsigned short* buff, int len) { arith_run_test(buff, len, TMP_NUM); }

//place in 1sec loop
void cpu_arithmetic_test(void)
{
    int len = 1000;
    int buff[1000] = {0};
    //single float
    float *p_f = (float*)buff;
    float_test(p_f, len);
    os_printf("p_f:%f, %f\n", p_f[0], p_f[1]);
    //int
    int *p_int = (int*)buff;
    int_test(p_int, len);
    os_printf("p_int:%d, %d\n", p_int[0], p_int[1]);
    //uint
    unsigned int *p_uint = (unsigned int *)buff;
    uint_test((unsigned int*)p_uint, len);
    os_printf("p_uint:%d, %d\n", p_uint[0], p_uint[1]);
    //int16
    short *p_int16 = (short*)buff;
    int16_test(p_int16, len);
    os_printf("p_int16:%d, %d\n", p_int16[0], p_int16[1]);
    //uint16
    unsigned short *p_uint16 = (unsigned short *)buff;
    uint16_test((unsigned short*)p_uint16, len);
    os_printf("p_uint16:%d, %d\n", p_uint16[0], p_uint16[1]);
}
#endif


#ifdef IR_TX
uint8_t IrTxData[8]={0x6C,0x57,0x00,0x00,0x32,0x1C,0xE8,0x00};
//*********************************
void UserIrTxFun()
{
	static uint8_t TxCont=0;
	static uint8_t DataFg=0;
	static uint8_t TxBitCont=0;
	static uint8_t TxBitSt=1;
	switch (TxCont){
		case 0:	pwm_set(9000, 1);	TxCont++;	break;
        case 1:	pwm_set(4500, 0);	TxCont++;	break;
        case 2:
        	if(TxBitSt){
        		pwm_set(560, 1);
        		TxBitSt=0;
        	}else{
        		if((IrTxData[DataFg]>>TxBitCont)&1)	pwm_set(2250, 0);
        			else							pwm_set(560, 0);
        		TxBitSt = 1;
        		if(++TxBitCont==8){
        			TxBitCont=0;
        			if(++DataFg==8)	TxCont++;
        		}
        	}
        	break;
        case 3:
        	if(TxBitSt){
        		pwm_set(560, 1);
        		TxBitSt=0;
        	}else{
        		ir_tx_init(0);
        		TxCont=0;
        		TxBitCont=0;
        		TxBitSt=1;
        		DataFg=0;
        	}
        	break;
	}
}
#endif

//place in 10ms loop, note: soft_mode_en must be 0 when in mini sched
void saradc_debug_show(uint8_t soft_mode_en)
{
#if DEBUG_SarADC_NUM
    #define PRT_THRD                80//50//max 4096, 50mV~114, 25mV~57, 36mV->82, 18mV->41
    #define SARADC_DBG(fmt,...)     os_printf("[SarADC] "fmt, ##__VA_ARGS__)

    int i;
    SARADC_CH_e saradc[] = SarAdcChVal;
    // int adc_ch_num = sizeof(saradc)/sizeof(saradc[0]);
    #define adc_ch_num  (sizeof(saradc)/sizeof(saradc[0]))
    static uint16_t val_pre[adc_ch_num] = { 0 };
    static int init = 0;
//// init
    if(init == 0)
    {
        init = 1;
        for(i = 0; i < adc_ch_num; i++){
            int io = get_gpio_by_saradc_ch(saradc[i]);
            gpio_config_new(io, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
            // if(io == GPIO32){ gpio_config_new(io, GPIO_HRES, GPIO_PULL_DOWN, GPIO_PERI_NONE); }
        }
        SARADC_DBG("init ok\n");
        // sys_delay_ms(10);
    }

//// 5s period show
    static uint32_t cnt = 0;
    if(++cnt % 500 == 0)
    {
        for(i = 0; i < adc_ch_num; i++)
        {
            int gpio = get_gpio_by_saradc_ch(saradc[i]);
            if(soft_mode_en){
                SARADC_DBG("IO%d = %d\n", gpio, sar_adc_voltage_get(saradc[i]));
            }else{
                SARADC_DBG("IO%d = %d\n", gpio, user_saradc_val_get(i));
            }
        }
    }

//// update adc value
    for(i = 0; i < adc_ch_num; i++)
    {
        uint16_t val;
        if(soft_mode_en){
            val = sar_adc_voltage_get(saradc[i]);
        }else{
            val = user_saradc_val_get(i);
        }
        int dealt = val - val_pre[i];
        int thrd = PRT_THRD;
        if(dealt > thrd || dealt < -thrd){
            val_pre[i] = val;
            SARADC_DBG("\t IO%d = %d, %d\n", get_gpio_by_saradc_ch(saradc[i]), val, dealt);
        }
    }
#endif 
}

/* **************************************************************************************** */
/* **************************************************************************************** */
/* **************************************************************************************** */ // dsp effect

#if CONFIG_DSP_EFFECT_EN
//***************************************************
void Audio_Def()
{
	SysInf.MusicVol = -10;
	SysInf.MicVol = 0;

	ioGain[Tone_I2s2LR] = -30;
	ioGain[Tone_DacLR] = -30;
//	ioGain[Out2L_I2s2L] = 0;
//	ioGain[Out2R_I2s2R] = 0;
	ioGain[Play_MusL] = 0;
	ioGain[Play_MusR] = 0;
#if (USB0_FUNC_SEL & USBD_CLS_AUDIO)
	ioGain[Usb_MusL] = 0;
	ioGain[Usb_MusR] = 0;
#endif

	ioGain[Out2L_RecL] = -10;
	ioGain[Out2R_RecR] = -10;

}
//***************************************************
void Music_VolSet(float val)
{
	ioGain[MusL_Out1L] = SysInf.MusicVol;
	ioGain[MusR_Out1R] = SysInf.MusicVol;
	ioGain[MusL_Out2L] = SysInf.MusicVol;
	ioGain[MusR_Out2R] = SysInf.MusicVol;
	IOGainSet(MusL_Out1L, SysInf.MusicVol);
	IOGainSet(MusR_Out1R, SysInf.MusicVol);
	IOGainSet(MusL_Out2L, SysInf.MusicVol);
	IOGainSet(MusR_Out2R, SysInf.MusicVol);
}
//***************************************************
void Mic_VolSet(float val)
{
#if ADC0_IN_En
	ioGain[Adc1_Mic] = val;
	IOGainSet(Adc1_Mic, val);
#endif
#if ANC_IN_En & 0x4
	ioGain[Adc4_Mic] = val;
	IOGainSet(Adc4_Mic, val);
#endif
#if ANC_IN_En & 0x8
	ioGain[Adc5_Mic] = val;
	IOGainSet(Adc5_Mic, val);
#endif
#if I2S2_OUT_En == 1
	ioGain[I2s2L_Mic] = val;
	ioGain[I2s2R_Mic] = val;
	IOGainSet(I2s2L_Mic, val);
	IOGainSet(I2s2R_Mic, val);
#endif
}
//***************************************************
void Out_VolSet(float val)
{
	ioGain[Out1L_DacL] = val;
	ioGain[Out1R_DacR] = val;
	IOGainSet(Out1L_DacL, val);
	IOGainSet(Out1R_DacR, val);
#if I2S2_OUT_En == 1
	ioGain[Out2L_I2s2L] = val;
	ioGain[Out2R_I2s2R] = val;
	IOGainSet(Out2L_I2s2L, val);
	IOGainSet(Out2R_I2s2R, val);
#endif
#if I2S3_OUT_En == 1
	IOGainSet(Out2L_I2s3L, val);
	IOGainSet(Out2R_I2s3R, val);
#endif
}

void dsp_effet_loop(void)
{
    static uint16_t Init_Cont = 0;

    //=============================================
    if(Init_Cont >=150){
        if(EF_Maim())	return;
    }else{
        Init_Cont++;
        if(Init_Cont == 100){
            EF_EQ_ReSend();
        //----------------------------
        }else if(Init_Cont == 102){
            g_dsp_log_on_fg = 0;
            g_mcu_log_on_fg = 0;
        }else if(Init_Cont == 103){
            Send_Id1 = CmdPid;
            Send_Id2 = 0x80;
            SendUpComp(SendBuff, 32);	// 請上位機連線
        }else if(Init_Cont == 104){
            g_dsp_log_on_fg = 1;
            g_mcu_log_on_fg = 1;
        }
	}
}
//***************************************************
void EF_ClewToneGr(uint8_t Gr) {}
void EQ_ClewToneGr(uint8_t Gr) {}
void PlayWaveStart(uint16_t id){}
void PlayWaveStop(uint16_t id){}
void DisPlay_UpData(uint8_t Page, uint8_t Id2) {}
#endif

#endif

