#include "driver_beken_includes.h"
#include "app_beken_includes.h"

//static uint16_t saradc_chn = 0;
static int saradc_count = 0;
static uint16_t saradc_value = 0;
static uint16 saradc_avr_value = 0;  // the average voltage of battery
static uint16_t saradc_threshold = SARADC_BELOW_THRESHOLD + 100;
static uint16_t saradc_threshold_pd = SARADC_BELOW_THRESHOLD;
uint32_t saradc_below_count = SARADC_BELOW_COUNT;
static int8_t  saradc_pd_flag = 0;
static uint16_t saradc_charger_bigc_threshold = SARADC_BIGC_THRESHOLD;
static uint16_t saradc_charger_full_threshold = SARADC_FULL_THRESHOLD;
static uint8_t  saradc_charger_bigc_count = 0;
static uint8_t  saradc_charger_almost_full_count = 0;
static uint8_t  saradc_charger_full_count = 0;
static uint8_t  saradc_charger_flag = SARADC_CHARGER_MODE_NONE;
volatile uint8_t  s_saradc_1st_calibration = FALSE;
volatile uint16_t s_saradc_1st_read_times = 0;

volatile int16_t v_saradc_start_cnt = 0;
volatile uint8_t s_saradc_chnl_busy = 0;
#define BK3000_A0_SARADC_MASK  (1 << 13)

#define CNT_TEMPR  8
#define TEMPE_MED_FILT_WIN_SIZE         3
#ifdef DEBUG_IC_TEMPERATURE
uint16_t saradc_tempe_data = 0;
#endif
uint16_t saradc_Tbase = 0;
uint8_t saradc_T_first_flag = 0;
uint16_t saradc_T[CNT_TEMPR] = {0};  // temperature detect

static uint16_t saradc_tempe_med_data[TEMPE_MED_FILT_WIN_SIZE] = {0};
extern uint16_t button_adc_value;
__inline void saradc_set_chnl_busy(uint8_t busy)
{
    v_saradc_start_cnt = busy * 10;  // for timeout count;
    s_saradc_chnl_busy = busy;    
}
__inline uint8_t saradc_get_chnl_busy(void)
{
    return  s_saradc_chnl_busy;   
}
void saradc_calibration_first(void)
{
    // SARADC_CRITICAL_CODE(1);
    saradc_set_chnl_busy(1);
    saradc_init(SARADC_MODE_CONTINUE, SARADC_CH_VBAT, 4);
    // SARADC_CRITICAL_CODE(0);
}
void saradc_start_working(void)
{
    saradc_calibration_first();
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_SADC);
}
uint16_t saradc_get_value(void)
{
    return saradc_value;
}
/* the voltage of battery used the mid-filter value of saradc. The function is same as "saradc_get_filter_bat_avr_value()" */
uint16_t saradc_get_bat_value(void)
{	
	return saradc_avr_value;//saradc_transf_adc_to_vol(saradc_get_value());
}

void saradc_reset( void )
{
    saradc_count = 0;
    saradc_pd_flag = 0;

    saradc_charger_bigc_count = 0;
    saradc_charger_almost_full_count = 0;
    saradc_charger_full_count = 0;
    saradc_charger_flag = SARADC_CHARGER_MODE_NONE;
    SADC_PRT("saradc_reset\r\n");
}

void saradc_refer_select(uint8_t mode)
{
 // bit29: saradc reference, 0=vbg1p2; 1=vsys/2
    if(mode)
        REG_SYSTEM_0x59 |= (0x01 << 29);   //saradc reference select: vsys/2
    else
        REG_SYSTEM_0x59 &= ~(0x01 << 29);  //saradc reference select: vbg1p2

}

void saradc_init( int mode, int channel, int div )
{
#if 1
    uint32_t t=0;
    //SAR-ADC configurations
    //REG_SYSTEM_0x46 &= ~(7<<14);
    //REG_SYSTEM_0x46 |= 6<<14;  // ldo output voltage:1.8V
    REG_SYSTEM_0x05 &= ~MSK_SYSTEM_0x05_SADC_PWD;// Power ON SAR-ADC
    REG_SYSTEM_0x06 |= MSK_SYSTEM_0x06_SADC_CLKGAT_DIS;// saradc clk gate disable
    REG_SYSTEM_0x03 |= MSK_SYSTEM_0x03_SYS_CLKGAT_DIS; // sysclk gate disable 
    REG_SYSTEM_0x40 |= 1<<20;  // ABB CB enable 
    //REG_SYSTEM_0x59 = 0x09600000;
    REG_SYSTEM_0x59 &= ~((uint32_t)0x1ff<<21);  // bit29: saradc reference, 0=vbg1p2; 1=vsys/2
    sys_delay_cycle(6);
    REG_SYSTEM_0x59 |= ((uint32_t)0x4B<<21); 
    #if 0
    while(!(REG_SADC_0x00 & MSK_SADC_0x00_FIFO_EMPTY))
        t = REG_SADC_0x04;

    REG_SADC_0x02 &= ~MSK_SADC_0x02_CHANNEL_EXPAND;
    REG_SADC_0x02 &= ~MSK_SADC_0x02_ALMOST_CFG;
    // REG_SADC_0x02 |= (0x3 << SFT_SADC_0x02_ALMOST_CFG);
    REG_SADC_0x02 |= ((SarADC_AVG_NUM - 1) << SFT_SADC_0x02_ALMOST_CFG);

    REG_SADC_0x02 |= (!!(channel & 0x10))<<SFT_SADC_0x02_CHANNEL_EXPAND;//SAR-ADC channel set bit[4] at reg02[16]
    REG_SADC_0x00 = ((mode&0x03) | ((div&0x3f)<<9) | ((channel&0xf)<<3));//SAR-ADC channel set bit[3:0]

    REG_SADC_0x00 |= MSK_SADC_0x00_ADC_INT_CLEAR; /*  clear interrupt; *///++ by borg @230201, avoid no data at intr
    #else//++ by borg @230201
    uint32_t reg_0x00;
    uint32_t reg_0x02 = REG_SADC_0x02;
    //SAR-ADC channel set bit[4] at reg02[16], cvt times set reg02[1:0](3+1=4)
    reg_0x02 &= ~(MSK_SADC_0x02_CHANNEL_EXPAND | MSK_SADC_0x02_ALMOST_CFG);
    reg_0x02 |= (((!!(channel & 0x10)) << SFT_SADC_0x02_CHANNEL_EXPAND) | ((SarADC_AVG_NUM - 1) << SFT_SADC_0x02_ALMOST_CFG));
    REG_SADC_0x02 = reg_0x02;
    //SAR-ADC channel set bit[3:0], adc_clk= clk/[2*(div+1)]
    reg_0x00 = (mode&0x03) | ((div&0x3f)<<9) | ((channel&0xf)<<3);
    REG_SADC_0x00 = reg_0x00;

    while(!(REG_SADC_0x00 & MSK_SADC_0x00_FIFO_EMPTY)) t = REG_SADC_0x04;
    REG_SADC_0x00 = reg_0x00 | MSK_SADC_0x00_ADC_INT_CLEAR; /*  clear interrupt; */
    #endif
    REG_SADC_0x00 |= MSK_SADC_0x00_ADC_EN;         //Digital SAR-ADC enable
    (void)t;
#endif
}

int saradc_set_lowpower_para( int interval, int threshold_lowpower, int threshold_powerdown )
{
    //SADC_PRT("set_lowpower_para:%d:%d:%d\r\n", interval, threshold_lowpower, threshold_powerdown);
    LOG_I(ADC, "lowpwr_para:%d:%d:%d\r\n", interval, threshold_lowpower, threshold_powerdown);
    saradc_threshold = threshold_lowpower;
    saradc_threshold_pd = threshold_powerdown;
//	saradc_below_count = 2;
    if( interval >= 200*4 )  // adc detect interval = interval/4
        saradc_below_count = 4;
    else 					 // adc detect interval = 200
		saradc_below_count = (interval+199)/200;

    return 0;
}

int saradc_set_charger_para( int threshold_bcur, int threshold_full )
{
    saradc_charger_bigc_threshold = threshold_bcur;
    saradc_charger_full_threshold = threshold_full;
    SADC_PRT("set_charger_para:%d:%d\r\n", threshold_bcur, threshold_full);
    return 0;
}

int saradc_charger_status(void)
{
    return saradc_charger_flag;
}

int saradc_lowpower_status(void)
{
    if(saradc_pd_flag >= 2/*saradc_below_count*/)
    {
        saradc_pd_flag = 0;
        return 2;
    }
    else if(saradc_count >= saradc_below_count)
        return 1;
    else
        return 0;
}

int saradc_normalpower_status(void)
{
    return (saradc_count == 0);
}
#if 0 //(CONFIG_CHARGE_EN == 1)
static void saradc_charge_status_updata( uint16_t data )
{
//	#ifdef BEKEN_DEBUG
//	os_printf("saradc3:%d\r\n",data);
//	#endif
    saradc_start = 0;
    saradc3_value += data;
    ++saradc3_det_cnt;
    if (saradc3_det_cnt > 3)
    {
        saradc3_value >>= 2;
        if (data >770) // 4.2v:820   
        {
            det_charge_flag = 1;
        }
        else if (data < 730)
        {
            det_charge_flag = 0;
        }
        saradc3_det_cnt = 0;
        saradc3_value = 0;
    }
}
#endif
static uint16_t s_vbat_saradc_buf[5] ={0};
static uint16_t adc_filter(uint16 data)
{
    unsigned char i = 0;
    uint16_t sum = 0;
    uint16_t value_avr_adc =0;
    uint16_t value_adc_max = 0;
    uint16_t value_adc_min = 0;
    static uint8_t adc_cnt = 0;
    s_vbat_saradc_buf[4] = data;
    for(i = 0; i < 4; i++)
        s_vbat_saradc_buf[i] = s_vbat_saradc_buf[i+1];
    value_adc_max = value_adc_min = s_vbat_saradc_buf[0];
    for(i = 0; i < 4; i++)
    {    
        value_adc_max =MAX(s_vbat_saradc_buf[i],value_adc_max);
        value_adc_min = MIN(s_vbat_saradc_buf[i],value_adc_min);
	    sum += s_vbat_saradc_buf[i];
    }
    //os_printf("max:%d,min:%d\r\n",value_adc_max,value_adc_min);
    value_avr_adc = (sum-(value_adc_max + value_adc_min))>> 1;
	
    //��һ���ϵ������ģʽ
    //�Ƚ����ѹģʽCV���г��
    //�ȴ�ADCֵ�ȶ����ٽ����л�
    if((++adc_cnt) > 10)
    {
        adc_cnt = 10;
	#if (CONFIG_CHARGE_EN == 1)
        //first_charge_flag = 1;
	#endif
    }
	
    if(value_avr_adc < 3000)
    {	
        LOG_I(ADC,"vbat-P/P:%d,%d\r\n",value_adc_max,value_adc_min);
        return value_adc_max;
    }
    //LOG_I(VBAT,"+++adc:%d(mV),adc_avr:%d(mV)\r\n",s_vbat_saradc_buf[4],value_avr_adc);
    return value_avr_adc;
}
uint16_t saradc_transf_adc_to_vol(uint16_t data)
{
    uint16_t v_data;
    env_saradc_cali_data_t *saradc_cali = app_get_env_saradc_cali_data();
    v_data = 4200*(data + 2048 - saradc_cali->sar_adc_dc)/(saradc_cali->sar_adc_4p2 + 2048 - saradc_cali->sar_adc_dc);
  // v_data = 2530*3*data/4096;     // vbg = 2.53
    return v_data;
}

/** @param adc_val the origin data from adc hardware, not the value calibrated
 * @return: adc value after calibration. [2700 : refer app_env_cali_data_init -> sar_adc_4p2 = 2700] */
uint16_t SarADC_val_cali_get(uint16_t adc_val)
{
    env_saradc_cali_data_t *saradc_cali = app_get_env_saradc_cali_data();
    uint16_t adc_val_cali = (adc_val + 2048 - saradc_cali->sar_adc_dc) * 2700 / (saradc_cali->sar_adc_4p2 + 2048 - saradc_cali->sar_adc_dc);
    return adc_val_cali;
}

/** @param adc_val the origin data from adc hardware, not the value calibrated
 * @return: voltage(unit:mv), [1200 : refer saradc_init -> vbg1p2] */
uint16_t SarADC_val_to_voltage(uint16_t adc_val)
{
    env_saradc_cali_data_t *saradc_cali = app_get_env_saradc_cali_data();
    uint16_t voltage_mv = 1200 * (adc_val + 2048 - saradc_cali->sar_adc_dc) / (saradc_cali->sar_adc_4p2 + 2048 - saradc_cali->sar_adc_dc);
    return voltage_mv;
}

/** @return: adc_val(0~4095), [1200 : refer saradc_init -> vbg1p2] */
uint16_t SarADC_voltage_to_val(uint16_t volt_mv)
{
#if 1
    return volt_mv * 4096 / 1800;
#else
    env_saradc_cali_data_t *saradc_cali = app_get_env_saradc_cali_data();
    uint16_t adc_val = (volt_mv * (saradc_cali->sar_adc_4p2 + 2048 - saradc_cali->sar_adc_dc) / 1200) + saradc_cali->sar_adc_dc - 2048;
    return (adc_val < 4095) ? adc_val : 4095;
#endif
}

/*Warning: this 'data' is battery volgate,unit is mV;Ex: 4.2V = 4200 mV*/
void saradc_lowpower_status_update( uint16_t data )
{
//    uint16 v_data;
    saradc_value = data;

    /* Vbat = (d-(DC-511)) * 4200 / (Norm - (DC - 511) */
    data = adc_filter(saradc_transf_adc_to_vol(data));
    //data = adc_filter(data);
    saradc_avr_value = data;

    if( data < saradc_threshold_pd )
    {
        saradc_pd_flag++;
    }
    else if( (data < saradc_threshold) && (data >= saradc_threshold_pd) )
    {
        saradc_count++;
        if( saradc_count >= (saradc_below_count << 1) )
            saradc_count = (saradc_below_count << 1);

        saradc_pd_flag--;
        if( saradc_pd_flag <= 0 )
            saradc_pd_flag = 0;
    }
    else
    {
        saradc_count--;
        if( saradc_count <= 0 )
            saradc_count = 0;

        saradc_pd_flag--;
        if( saradc_pd_flag <= 0 )
            saradc_pd_flag = 0;
    }

}


#ifdef POLL_READ_SARADC_VALUE
uint32_t saradc_scan(void)
{
    uint32_t data;

    if(BK3000_SARADC_ADC_CONF & SARADC_FIFO_EMPTY)
    {
        SADC_PRT("s");
        return 1;
    }
    else
    {
        SADC_PRT("saradc_config:%p\r\n", BK3000_SARADC_ADC_CONF);
        data = BK3000_SARADC_ADC_DATA & 0x3ff;
        SADC_PRT("saradc_scan:%p\r\n", data);

        BK3000_SARADC_ADC_CONF = SARADC_MODE_SOFTWARE
                                | (0 << sft_SARADC_CHANNEL)
                                | (SARADC_START_DELAY)
                                | (SARADC_CLK_DIV512 << bit_SARADC_CLKDIV);

        /* if(GPIO_CHARGER_FLAG) */
        /*     saradc_charger_status_update( data ); */
        /* else */
        /*     saradc_lowpower_status_update( data ); */

        return 0;
    }
}
#endif
uint8_t saradc_calibration_end(void)
{
    return s_saradc_1st_calibration;
}

void temperature_saradc_enable(void)
{
    if(saradc_get_chnl_busy())  return;
    SARADC_CRITICAL_CODE(1);
    REG_SYSTEM_0x40 |= (1<<20); // ABB CB;
    sys_delay_cycle(6);
    REG_SYSTEM_0x40 |= (1<<19); // enable temperature detect;
    sys_delay_cycle(6);
    REG_SYSTEM_0x59 |= (1<<12);  // choose mode for temperature;
    saradc_set_chnl_busy(1);
    saradc_init(SARADC_MODE_CONTINUE,SARADC_CH_TEMPERATURE,4);
    SARADC_CRITICAL_CODE(0);
#if 0
    //static uint8_t n=0;
    //n++;
    //if(n>=CNT_TEMPR)
    //{
        //tmpr_delta = ((int16_t)saradc_Tbase - (int16_t)saradc_T[0])*100/334;
        //INFO_PRT("temperature:%d,%d,%d,%d,%d,%d,%d,%d\r\n",saradc_T[0],saradc_T[1],saradc_T[2],saradc_T[3],saradc_T[4],saradc_T[5],saradc_T[6],saradc_T[7]);
        //INFO_PRT("temperature delt:%d\r\n",tmpr_delta);
        //n=0;
    //}
#endif
}

void temperature_saradc_update(uint16_t data)
{
    uint8_t i=0;
    uint16 med_tempe = 0;
    if(data == 0){
        LOG_W(TEMP,"[TEMPE WARNNING] 000 temperature data:%d\r\n",data);
        return;
    }
    if(!saradc_T_first_flag)
    {
        saradc_Tbase = data;
        LOG_I(TEMP,"temperature base:%d\r\n",saradc_Tbase);
        saradc_T_first_flag = 1;
        for(i=0;i<CNT_TEMPR;i++)
        {
            saradc_T[i] = saradc_Tbase;
        }
        for(i=0;i<TEMPE_MED_FILT_WIN_SIZE;i++)
        {
            saradc_tempe_med_data[i] = saradc_Tbase;    
        }
        app_set_temperature_base(saradc_T);
        return;
    }
    for(i=0;i<TEMPE_MED_FILT_WIN_SIZE-1;i++)
        saradc_tempe_med_data[i] = saradc_tempe_med_data[i+1];
    saradc_tempe_med_data[TEMPE_MED_FILT_WIN_SIZE-1] = data;
    #ifdef DEBUG_IC_TEMPERATURE
    saradc_tempe_data = data;
//    LOG_I(TEMP,"temperature data:%d\r\n",data);
    #endif
    med_tempe = app_temperature_median_filter(saradc_tempe_med_data,TEMPE_MED_FILT_WIN_SIZE);
    for(i=0;i<CNT_TEMPR-1;i++)
        saradc_T[i] = saradc_T[i+1];
    saradc_T[CNT_TEMPR-1] = med_tempe;
    app_update_temperature_base(saradc_T);
    
}

//get recent tempe data, data num must <= TEMPE_MED_FILT_WIN_SIZE
int temperature_saradc_data_get(uint16_t *data, int num)
{
    if(num > TEMPE_MED_FILT_WIN_SIZE) num = TEMPE_MED_FILT_WIN_SIZE;
    memcpy(data, saradc_tempe_med_data, num * 2);
    return num;
}

void saradc_isr( void )
{
    uint8_t cnt = 0;
    uint32_t reg = 0;
    uint16 data = 0;
#if SarADC_CALC_MODE == 1
    uint16_t adc_val[32];//refer to 0x2[4:0] Almost_cfg
#endif
    uint16 saradc_chn = 0;
    app_handle_t app_h = app_get_sys_handler();
    //uint32_t old_mask = get_spr(SPR_VICMR(0));

    //set_spr( SPR_VICMR(0), old_mask & (1 << VIC_AUD_ISR_INDEX));
    //cpu_set_interrupts_enabled(1);

#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    SYSpwr_Wakeup_From_Sleep_Mode();
#endif

    reg = REG_SADC_0x00;
    saradc_chn = (reg >> SFT_SADC_0x00_ADC_CHNL) & 0xf;
    saradc_chn |=(!! (REG_SADC_0x02&MSK_SADC_0x02_CHANNEL_EXPAND))<<4;
    REG_SADC_0x00 = reg & ~MSK_SADC_0x00_ADC_MODE;
    while(!(REG_SADC_0x00 & MSK_SADC_0x00_FIFO_EMPTY))
    {
    #if SarADC_CALC_MODE == 1
        adc_val[cnt++] = REG_SADC_0x04 & 0xffff;
    #else
        data = REG_SADC_0x04 & 0xffff;
        cnt++;
    #endif
    }
    REG_SADC_0x00 = MSK_SADC_0x00_ADC_INT_CLEAR; /* disable & clear interrupt; */
    REG_SYSTEM_0x05 |= MSK_SYSTEM_0x05_SADC_PWD;/* power down SARADC */
#if SarADC_CALC_MODE == 1
    if(cnt >= SarADC_AVG_NUM)
    {
        int i = 0, min = 0, max = 0;
        for(i = 0; i < cnt; i++) {
            if(adc_val[max] < adc_val[i]) { max = i; }
            if(adc_val[min] > adc_val[i]) { min = i; }
        }
    #ifdef SADC_DEBUG
        int delta = adc_val[max] - adc_val[min];
        // SADC_PRT("[%d] saradc_isr, cnt:%d, ch:%d\t", __LINE__, cnt, saradc_chn);
        if ((delta > 200) //usually 70~150
            || (adc_val[min] == 0)
            || ((adc_val[min] < 1000) || (adc_val[max] > 3000))//supply 1v voltage at adc input
            || (cnt > SarADC_AVG_NUM)// for sometimes, cnt may > SarADC_AVG_NUM, test @ SarADC_AVG_NUM == 4 @230707 by Borg
        ){
            extern GPIO_PIN get_gpio_by_saradc_ch(SARADC_CH_e sadc_chn);
            SADC_PRT("saradc_isr ch:IO%d, cnt:%d, delta:%d\t{ ", get_gpio_by_saradc_ch(saradc_chn), cnt, delta);
            for(i = 0; i < cnt; i++){ SADC_PRT("%u\t", adc_val[i]); } SADC_PRT("}\n");
        }
    #endif
        if(max == min){
            data = adc_val[max];
        }else{
            uint32_t adc_val_acc = 0;
            adc_val[max] = adc_val[min] = 0;
            for(i = 0; i < cnt; i++) { adc_val_acc += adc_val[i]; }
            data = adc_val_acc / (cnt - 2);
        }
    }
    else { LOG_E(ADC,"[%d] saradc_isr, cnt:%d, ch:%d data:%d\n", __LINE__, cnt, saradc_chn, data); goto RET; }
#else
    if(cnt == 0) { LOG_E(ADC,"saradc_isr !!!!!!!! cnt=0, ch:%d data:%d\n\n\n\n", saradc_chn, data); goto RET; }
#ifdef SADC_DEBUG
    else if(data == 0) { SADC_PRT("[%d] saradc_isr, cnt:%d, ch:%d data:%d\n", __LINE__, cnt, saradc_chn, data); }
#endif
#endif

    if(!s_saradc_1st_calibration)
    {
    	s_saradc_1st_read_times++;
        if(s_saradc_1st_read_times > 1)
        {
            s_saradc_1st_calibration = TRUE;
            saradc_value = data;
            saradc_avr_value = saradc_transf_adc_to_vol(data);
            LOG_I(ADC,"sadc cali:%d\r\n",data);
            saradc_set_chnl_busy(0);
            s_vbat_saradc_buf[0] = s_vbat_saradc_buf[1] = s_vbat_saradc_buf[2] = s_vbat_saradc_buf[3] = s_vbat_saradc_buf[4] = saradc_transf_adc_to_vol(data);
            
        }
        else
        {
           saradc_set_chnl_busy(0);
           saradc_calibration_first();
        }
    }
    else
    {
        /* 
        the following equation Vbat should be > 0
        Vbat = [d-(DC-511)] * 4200 / [Norm - (DC - 511)] 
        */
        #ifdef SarAdcChVal
        extern int usr_saradc_val_update_isr(SARADC_CH_e chn, uint16_t adc_val);
        if(usr_saradc_val_update_isr(saradc_chn, data))
        {
            saradc_set_chnl_busy(0);
        }
        #endif

        if((app_h->button_mode_adc_enable)&&(saradc_chn == app_h->button_mode_adc_chan))
        {
            button_adc_value = data;
            saradc_set_chnl_busy(0);
        }
        else
        {
        //data = (uint16)(data *933/env_chg_adc.adc_val);
            
            if(app_h->low_detect_channel == saradc_chn)
            {
                saradc_set_chnl_busy(0);
                if(data > 512)
                {
                    saradc_lowpower_status_update( data );
                }
            }
            else if(SARADC_CH_TEMPERATURE == saradc_chn)  // detect temperature
            {
                saradc_set_chnl_busy(0);
                temperature_saradc_update(data);                
                
                //REG_SYSTEM_0x40 &= ~(1<<19); // disable temperature detect;
            }
		#if CONFIG_TEMPERATURE_NTC
			else if (TEMPERATURE_NTC_CH_CHANNEL == saradc_chn)
			{
				saradc_set_chnl_busy(0);
				app_temperature_ntc_status_updata(data);
			}
		#endif
        }
    }    
RET:
    REG_SADC_0x00 |= MSK_SADC_0x00_ADC_INT_CLEAR; /*  clear interrupt; */
    //cpu_set_interrupts_enabled(0);
    //set_spr( SPR_VICMR(0), old_mask );
    (void)data;
}
