
#include "udrv_saradc.h"

#ifdef _UDRV_SARADC_H_
#include <stdint.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "u_debug.h"

#ifdef SarAdcChVal

#ifdef	Kbon4051NUM
	uint8_t AdcSelCh = 0;
	uint8_t Kbon4051Sw[3] = Kbon4051SwGpio;
#endif
#ifdef	Kbon4052NUM
	uint8_t AdcSelCh = 0;
	uint8_t Kbon4052Sw[2] = Kbon4052SwGpio;
#endif

#ifdef	KbonNUMBER
	uint16_t KnobVal[KbonNUMBER];
	uint16_t KnobValR[KbonNUMBER];
	uint8_t KbonGpio[KbonNUM] = KbonGpioVal;
#endif
	uint8_t SarAdcCh[] =  SarAdcChVal;

    uint16_t SarAdcVal[SarAdc_NUM];
	uint16_t SarAdcValR[SarAdc_NUM];

//****************************************************
GPIO_PIN get_gpio_by_saradc_ch(SARADC_CH_e sadc_chn)
{
    GPIO_PIN gpio = GPIO_NUM;
    switch (sadc_chn){
    	case SARADC_CH_GPIO11: gpio = GPIO11; break;
    	case SARADC_CH_GPIO18: gpio = GPIO18; break;
    	case SARADC_CH_GPIO19: gpio = GPIO19; break;
    	case SARADC_CH_GPIO20: gpio = GPIO20; break;
    	case SARADC_CH_GPIO21: gpio = GPIO21; break;
    	case SARADC_CH_GPIO22: gpio = GPIO22; break;
    	case SARADC_CH_GPIO23: gpio = GPIO23; break;
    	case SARADC_CH_GPIO31: gpio = GPIO31; break;
    	case SARADC_CH_GPIO32: gpio = GPIO32; break;
    	case SARADC_CH_GPIO34: gpio = GPIO34; break;
    	case SARADC_CH_GPIO36: gpio = GPIO36; break;
    	case SARADC_CH_GPIO37: gpio = GPIO37; break;
    	case SARADC_CH_GPIO38: gpio = GPIO38; break;
    	case SARADC_CH_GPIO39: gpio = GPIO39; break;
    	default: break;
    }
    return gpio;
}

/**
 * @brief convert adc val to voltage.
 * @param adc_val (0 ~ 4095)
 * @return voltage(unit:mV)
 * @note about error
 * the err = (2048 - saradc_cali->sar_adc_dc)
 * you should read adc_rd_val(0~4095), then adc_real_val = adc_rd_val + err
 * refer to saradc_transf_adc_to_vol()
 * @note about reference voltage refer to 
 * sar_adc_enable(uint32_t channel, uint32_t enable)//saradc reference, 0=vbg1p2; 1=vsys/2
 * */
uint16_t saradc_cvt_to_voltage(uint16_t adc_val)
{
    env_saradc_cali_data_t *saradc_cali = app_get_env_saradc_cali_data();
    adc_val = adc_val + 2048 - saradc_cali->sar_adc_dc;
    return (uint16_t)((float)(1870 * adc_val)/4096);	//yuan++ 1800->1870 20230629
}

//soft mode, get adc value
uint16_t sar_adc_voltage_get(SARADC_CH_e sadc_chn)
{
//  uint16_t voltage = 0;
    uint16_t adc_val = 0;
    extern void sar_adc_enable(uint32_t channel, uint32_t enable);
    extern uint32_t sar_adc_read(void);
    uint8_t intr_en = SARADC_INTR_IS_ON;
    if(intr_en) SARADC_CRITICAL_CODE(1);
    sar_adc_enable(sadc_chn, 1);
    adc_val = sar_adc_read();
    sar_adc_enable(sadc_chn, 0);
    if(intr_en) SARADC_CRITICAL_CODE(0);
//  voltage = saradc_cvt_to_voltage(adc_val);
//  DBG_LOG_INFO("sadc_chn:%d, voltage:%d, adc_val:%d\n", sadc_chn, voltage, adc_val);
//    DBG_LOG_INFO("==== CHG BATT ADC:%d  %1.2fv\n",adc_val,(float)adc_val*0.0024);
    return adc_val;
//	return voltage;
}

//****************************************
void u_SaradcKbon_Init()
{
#if (KbonNUM)
	for(uint8_t i=0; i<KbonNUM; i++){
		gpio_config_new(get_gpio_by_saradc_ch(KbonGpio[i]), GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
	}
	for(uint8_t i=0; i<KbonNUMBER; i++){
		KnobVal[i]=0;
		KnobValR[i]=0xFFFF;
	}
#endif
#ifdef Kbon4051NUM
	for(uint8_t i=0; i<3; i++){
		gpio_config_new(Kbon4051Sw[i], GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		gpio_output(Kbon4051Sw[i],0);
	}
#endif
#ifdef Kbon4052NUM
	for(uint8_t i=0; i<2; i++){
		gpio_config_new(Kbon4052Sw[i], GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		gpio_output(Kbon4052Sw[i],0);
	}
#endif
#if (SarAdc_NUM)
	for(uint8_t i=0; i<SarAdc_NUM; i++){
		SarAdcVal[i]=0;
		SarAdcValR[i]=0xFFFF;
		gpio_config_new(get_gpio_by_saradc_ch(SarAdcCh[i]), GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
	}
#endif
}

//*********************************************************
uint16_t u_saradc_value_get(SARADC_CH_e sadc_chn)
{
    for(uint8_t i=0; i<SarAdc_NUM; i++){
    	if(sadc_chn == SarAdcCh[i]){
    	//	return SarAdcVal[i];
    		return	SarADC_val_cali_get(SarAdcVal[i]);	// SarADC_val_cali_get 校正 ADC 偏差值
    	}
    }
    DBG_LOG_WARN("adc channel %d not exist!!!", sadc_chn);
    return	0;
}

//*********************************************************
//get adc value by user define channel number
uint16_t user_saradc_val_get(uint8_t usr_chn)
{
    uint16_t val = 0;
    if(usr_chn >= SarAdc_NUM) goto RET;

    uint8_t intr_en = SARADC_INTR_IS_ON;
    if(intr_en) SARADC_CRITICAL_CODE(1);
    // uint32_t interrupts_info, mask;
    // SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    val = SarAdcVal[usr_chn];
    // SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    if(intr_en) SARADC_CRITICAL_CODE(0);
    return (val);
    // return SarADC_val_cali_get(val);
RET:
    DBG_LOG_WARN("adc channel %d not exist!!!", usr_chn);
    return	0;
}

//***** 1. 發送 ADC 採樣觸發信息 ******************
void user_saradc_update_trig(void)
{
	if(saradc_get_chnl_busy())	return;//if adc is converting, return.
    msg_put(MSG_USER_SarADC_UPDT);
}

//**** 設置  ADC 通道 並觸發轉換 **********************
/*
int u_saradc_val_update_req(SARADC_CH_e sadc_chn)
{
    if(saradc_get_chnl_busy())  return 0;//if adc is converting, return.
    saradc_set_chnl_busy(1);//set busy state
    saradc_init(SARADC_MODE_CONTINUE, sadc_chn, 4);//config adc convert mode SARADC_MODE_CONTINUE, set channel, adc clk div num
    // saradc_refer_select(1);//select adc ref voltage(0=vbg1p2; 1=vsys/2)
    return 1;
}
*/
//***** 2. 設置  ADC 採樣通道並觸發轉換**********************
void user_saradc_update_req(void)
{
    static int cnt = 0;
//	u_saradc_val_update_req(SarAdcCh[cnt]);
    if(saradc_get_chnl_busy())  return;//if adc is converting, return.
    SARADC_CRITICAL_CODE(1);
    saradc_set_chnl_busy(1);	//set busy state
    saradc_init(SARADC_MODE_CONTINUE, SarAdcCh[cnt], SarADC_CLK_DIV(SarADC_CLK_Hz));//4, config saradc mode, channel, div (adc_clk= clk/[2*(div+1)])
//  saradc_refer_select(1);//select adc ref voltage(0=vbg1p2; 1=vsys/2)
    if(++cnt >= SarAdc_NUM) cnt = 0;
    SARADC_CRITICAL_CODE(0);
}

//**** 3. update in saradc isr **************************************
int usr_saradc_val_update_isr(SARADC_CH_e chn, uint16_t adc_val)
{
    for(uint8_t i=0; i<SarAdc_NUM; i++){
       	if(chn == SarAdcCh[i]){
//     		if(adc_val==0){
//     			SarAdcVal[i] = SarAdcValR[i];	// GPIO32 有時會誤讀 0 防止
//     			DBG_LOG_ERR("!!!!!!!!!!!! ADC ERR! chn:%d\n", chn);
//     		}else{
   				SarAdcVal[i] = adc_val;
//     		}
   			//==========================================
		#ifdef MJ_K04
				if(i==0){
					KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)>>1;
					if(++AdcSelCh >7) AdcSelCh = 0;
					gpio_output(Kbon4051Sw[0],AdcSelCh&1);
					gpio_output(Kbon4051Sw[1],(AdcSelCh>>1)&1);
					gpio_output(Kbon4051Sw[2],(AdcSelCh>>2)&1);
				}
		#endif
		#ifdef XFN_S930
   			if(i==0){
   				KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)>>1;
   				if(++AdcSelCh >7) AdcSelCh = 0;
   				gpio_output(Kbon4051Sw[0],AdcSelCh&1);
   				gpio_output(Kbon4051Sw[1],(AdcSelCh>>1)&1);
   				gpio_output(Kbon4051Sw[2],(AdcSelCh>>2)&1);
   			}
		#endif
		#if defined(SK_460) || defined(XWX_S6616)
   			if(i==1){
   				KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)>>1;
   				if(++AdcSelCh >7) AdcSelCh = 0;
   			//	AdcSelCh = 3;	// 只讀 CHn
    			gpio_output(Kbon4051Sw[0],AdcSelCh&1);
       			gpio_output(Kbon4051Sw[1],(AdcSelCh>>1)&1);
       			gpio_output(Kbon4051Sw[2],(AdcSelCh>>2)&1);
       		}
		#endif
		#if defined(SG_P60) || defined(SC_D3)
   			if(i==0){
   				KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)/2;
   			}else if(i==1){
   				KnobVal[AdcSelCh+8] = (KnobVal[AdcSelCh+8]+adc_val)/2;
   				if(++AdcSelCh >7) AdcSelCh = 0;
   				//	AdcSelCh = 1;
   				gpio_output(Kbon4051Sw[0],AdcSelCh&1);
   				gpio_output(Kbon4051Sw[1],(AdcSelCh>>1)&1);
   				gpio_output(Kbon4051Sw[2],(AdcSelCh>>2)&1);
   			}
		#endif
		#if defined(ONS_01) || defined(ONS_TY332)
   			if(i==2){
   				KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)/2;
   			}else if(i==3){
   				KnobVal[AdcSelCh+8] = (KnobVal[AdcSelCh+8]+adc_val)/2;
   				if(++AdcSelCh >7) AdcSelCh = 0;
   				gpio_output(Kbon4051Sw[0],AdcSelCh&1);
   				gpio_output(Kbon4051Sw[1],(AdcSelCh>>1)&1);
   				gpio_output(Kbon4051Sw[2],(AdcSelCh>>2)&1);
   			}
		#endif
		#ifdef SY_K59
   			if(i==3){
   				KnobVal[AdcSelCh] = (KnobVal[AdcSelCh]+adc_val)/2;
   			}else if(i==4){
   				KnobVal[AdcSelCh+4] = (KnobVal[AdcSelCh+4]+adc_val)/2;
   				if(++AdcSelCh >3) AdcSelCh = 0;
   				gpio_output(Kbon4052Sw[0],AdcSelCh&1);
   				gpio_output(Kbon4052Sw[1],(AdcSelCh>>1)&1);
   			}
		#endif
//       	DBG_LOG_WARN("adc channel %d   %d   %d\n", i, chn, adc_val);
       		return 1;
       	}
   	}
    if(chn > SARADC_CH_TOUCHIN)	DBG_LOG_WARN("!!!! adc channel %d   %d  NotExist\n", chn, adc_val);	// 待查 ADC CH 30 ???
    return 0;
}

#if 0

// #define U_GLOBAL_INT_STOP()         cpu_set_interrupts_enabled(0)
// #define U_GLOBAL_INT_START()        cpu_set_interrupts_enabled(1)
#define U_GLOBAL_INT_STOP()         system_peri_mcu_irq_disable(SYS_PERI_IRQ_SADC)
#define U_GLOBAL_INT_START()        system_peri_mcu_irq_enable(SYS_PERI_IRQ_SADC)

//cant interrupt by sarADC inter
uint16_t test_sar_adc_val_get(void)
{
    uint16_t sadc_val = 0;
    uint16_t voltage = 0;
    U_GLOBAL_INT_STOP();
    sar_adc_enable(SARADC_CH_GPIO11, 1);
    sadc_val = sar_adc_read();
    voltage = saradc_cvt_to_voltage(sadc_val);
    uint16_t vol1 = saradc_cvt_to_voltage1(sadc_val);
    sar_adc_enable(SARADC_CH_GPIO11, 0);
    U_GLOBAL_INT_START();
    DBG_LOG_INFO("%s() - %u, %u\n", __FUNCTION__, sadc_val, voltage);
    DBG_LOG_INFO("vol1 - %u\n", vol1);
    return voltage;
}
#endif



#ifdef DEBUG_IC_TEMPERATURE
//打印温度传感器值，每10s更新一次，每60s写一次flash， 更新最近30次的值写flash。

#define TEMP_SENSER_VAL_FLASH_ADDR      0x1FD000 //ota info addr, refer to bootloader.c
#define TEMPE_UPDATE_INT_TIMEsec        10  //unit:1sec, updata and debug show interval time
#define TEMPE_SAVE_FLASH_INT_TIMEsec    60  //unit:1sec, save flash interval time
#define TEMP_VAL_FILTER_AVG_NUM         30  //average times
#define TEMPE_LOG_I                     os_printf
//place in 1sec loop func
void debug_show_temp_senser_proc(void)
{
    int i;
    static uint32_t tempe_val_cnt = 0;
    static uint32_t tempe_save_cnt = 0;
    static uint32_t loop_updt_cnt = 0;
    static float tempe_val_array[TEMP_VAL_FILTER_AVG_NUM+1] = {0};
    extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
    float tempe_val_avg = 0;
    float tempe_val_acc = 0;

    if(++loop_updt_cnt >= TEMPE_UPDATE_INT_TIMEsec)//unit:sec
    {
        loop_updt_cnt = 0;

        //// update array value	//1690=0度 step 4=0.1度 yuan++
        tempe_val_array[tempe_val_cnt++] = (float)(1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值 1690->1640 20230629
        if(tempe_val_cnt >= TEMP_VAL_FILTER_AVG_NUM) tempe_val_cnt = 0;

        //// calc average value
        for(i = 0; tempe_val_array[i]; i++) tempe_val_acc += tempe_val_array[i];
        tempe_val_avg = (float)tempe_val_acc/i;

        extern uint8_t free_num;
        tempe_val_array[TEMP_VAL_FILTER_AVG_NUM] = (float)free_num;

        //// print result
        TEMPE_LOG_I("[TEMP SENSER] ======== (%2.1f data):\n", TEMP_VAL_FILTER_AVG_NUM);
        for(i = 0; i < TEMP_VAL_FILTER_AVG_NUM; i++) TEMPE_LOG_I("%2.1f ", tempe_val_array[i]);
        TEMPE_LOG_I(", cur:%d, avg: %2.1f free_num:%1.0f\n", saradc_tempe_data, tempe_val_avg, tempe_val_array[TEMP_VAL_FILTER_AVG_NUM]);

        //// save falsh
        tempe_save_cnt += TEMPE_UPDATE_INT_TIMEsec;
        if(tempe_save_cnt >= TEMPE_SAVE_FLASH_INT_TIMEsec)
        {
            tempe_save_cnt = 0;
            TEMPE_LOG_I("[TEMP SENSER] temp value save to flash.\n");
            flash_erase_sector(TEMP_SENSER_VAL_FLASH_ADDR, FLASH_ERASE_4K);
            flash_write_data((uint8_t*)&tempe_val_array, TEMP_SENSER_VAL_FLASH_ADDR, (sizeof(tempe_val_array)+1));
            flash_write_data((uint8_t*)&tempe_val_avg, TEMP_SENSER_VAL_FLASH_ADDR + sizeof(tempe_val_array)+1, 4);
        }
        saradc_tempe_data = 0;//clear
        // msg_put(MSG_TEMPERATURE_DETECT);//// trig temp update
    }
}

// place in init func
void debug_show_temp_senser(void)
{
    int i = 0;
    float tempe_val_array[TEMP_VAL_FILTER_AVG_NUM+1];
    float tempe_val_avg = 0;
    TEMPE_LOG_I("[TEMP SENSER] temp value load from flash.\n");
    flash_read_data((uint8_t*)&tempe_val_array, TEMP_SENSER_VAL_FLASH_ADDR, sizeof(tempe_val_array)+1);
    flash_read_data((uint8_t*)&tempe_val_avg, TEMP_SENSER_VAL_FLASH_ADDR + sizeof(tempe_val_array)+1, 4);

    //show
    TEMPE_LOG_I("[TEMP SENSER] ======== (%d data):\n", TEMP_VAL_FILTER_AVG_NUM);
    for(i = 0; i < TEMP_VAL_FILTER_AVG_NUM; i++) TEMPE_LOG_I("%2.1f ", tempe_val_array[i]);
    TEMPE_LOG_I("tempe_val_avg: %2.1f   free_num: %1.0f\n", tempe_val_avg, tempe_val_array[TEMP_VAL_FILTER_AVG_NUM]);
    msg_put(MSG_TEMPERATURE_DETECT);//// trig temp update
}
#endif


#endif
#endif
