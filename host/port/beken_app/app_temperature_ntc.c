#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"

#if CONFIG_TEMPERATURE_NTC

static app_temperature_ntc_t s_temperature_ntc;
extern uint16 app_temperature_median_filter(uint16 *data_ptr,uint16 win_size);

CONST uint32 temp_protect_ntc_level[5]=//Rm*10
{
#if 0
    983800,//743538,//-20c
    427030,//282671,//0c
    212000,//148550,//15c
    63500,//48520,//45c
    34170,//29704//60c
#else
    923800,//689150,//-20c
    543014,//274450,//0c
    262000,//147200,//15c
    56455,//49110,//45c
    30240,//30240//60c
#endif
};

void app_temperature_ntc_set_interval(uint16 interval)
{
    s_temperature_ntc.adc_interval= interval; 
}

void app_temperature_ntc_set_status(uint8 status) 
{
    s_temperature_ntc.status = status;  
}

static uint32 app_temperature_adc_to_ntc(uint32 temp_adc)
{
    return ((temp_adc*TEMPERATURE_NTC_RV*10)/(TEMPERATURE_NTC_FULL_SCALE-temp_adc)); 
}
void app_temprature_ntc_saradc_enable(void)
{
    if(saradc_get_chnl_busy())  return;
    SARADC_CRITICAL_CODE(1);
    saradc_set_chnl_busy(1);
    saradc_init(SARADC_MODE_CONTINUE,TEMPERATURE_NTC_CH_CHANNEL,4); 
    saradc_refer_select(1);
    SARADC_CRITICAL_CODE(0);
}

void app_temprature_ntc_scanning(void)
{
    static uint64_t s_temperature_timer_cnt = 0;
    uint64_t t1;

    t1 = os_get_tick_counter();
    if((t1-s_temperature_timer_cnt) >= s_temperature_ntc.adc_interval)
    {
        s_temperature_timer_cnt = t1;
        msg_put(MSG_TEMPRATURE_NTC_DETECT);
    }
}

static void app_temperature_set_charge_current(uint8 current)
{
    if (get_Charge_state())
    {
        if (current == TEMPERATURE_NTC_0)
        {
            s_temperature_ntc.charge_close=1;
            mSET_USB_CHARGE_ENABLE(0);
        }
        else
        {
            mSET_USB_CHARGE_CURRENT(current);
            if (s_temperature_ntc.charge_close)
            {
                s_temperature_ntc.charge_close = 0;
                sys_delay_cycle(6);
                mSET_USB_CHARGE_ENABLE(1);
            }
        }
    }
}

static void app_temperature_ntc_calulate(uint16 *data_ptr)
{
    uint32 avg_temp=0;
    uint8 temp_status,i;
    uint32 avg_test=0;
    uint8 cali_data;
    app_env_handle_t env_h = app_env_get_handle();

    for(i=0;i<TEMPERATURE_NTC_BUFF_SIZE;i++)
        avg_temp += *(data_ptr+i);
    avg_temp /= TEMPERATURE_NTC_BUFF_SIZE;
    avg_test = avg_temp;
    avg_temp = app_temperature_adc_to_ntc(avg_temp);
    INFO_PRT("ntc=%d,%d,mv:%d\n",avg_temp,avg_test,(1700*avg_test)>>12);
    
    if (avg_temp >= temp_protect_ntc_level[0])
        temp_status = TEMPERATURE_NTC_NE_20;
    else if (avg_temp > temp_protect_ntc_level[1])
        temp_status = TEMPERATURE_NTC_0;
    else if (avg_temp > temp_protect_ntc_level[2])
        temp_status = TEMPERATURE_NTC_15;	
    else if (avg_temp > temp_protect_ntc_level[3])	
        temp_status = TEMPERATURE_NTC_45;
    else if (avg_temp > temp_protect_ntc_level[4])	
        temp_status = TEMPERATURE_NTC_60;			
    else
        temp_status = TEMPERATURE_NTC_MAX_60;

    if (temp_status != s_temperature_ntc.status)
    {
        app_temperature_ntc_set_status(temp_status);
        INFO_PRT("ntc_status:%d\r\n",s_temperature_ntc.status);
        switch (s_temperature_ntc.status)
        {
            case TEMPERATURE_NTC_NE_20://powerdown
            case TEMPERATURE_NTC_MAX_60:	
                //gpio_config(TEMPERATURE_NTC_PWR_CTRL_PIN, 1);
                //gpio_output(TEMPERATURE_NTC_PWR_CTRL_PIN, 0);
                //msg_put(MSG_POWER_DOWN);
                break;
            case TEMPERATURE_NTC_0://stop charge
            case TEMPERATURE_NTC_60:
                //app_temperature_set_charge_current(TEMPERATURE_NTC_0);
                break;
            case TEMPERATURE_NTC_15://charge current 20ma	
                app_temperature_set_charge_current(TEMPERATURE_NTC_CHARGE_CURRENT_20MA);
                break;
            case TEMPERATURE_NTC_45://charge current 50ma
                cali_data = env_h->env_cfg.system_para.charger_current & 0x1f;
                app_temperature_set_charge_current(cali_data);	//TEMPERATURE_NTC_CHARGE_CURRENT_50MA
                break;		
            default:
                break;
        }
    }	
}

void app_temperature_ntc_status_updata(uint16 data)
{
	volatile uint8 i = 0;
	uint32 med_tempe;
	static uint8 temp_first_cnt=0;
	
	data &= 0xfff;
	if (temp_first_cnt < TEMPERATURE_NTC_BUFF_SIZE)
	{
		s_temperature_ntc.buff[temp_first_cnt] = data; 
		s_temperature_ntc.med_buff[temp_first_cnt&0x02] = data;
		temp_first_cnt ++;
	}
	else
	{
		for(i=0;i<TEMPERATURE_NTC_FILT_WIN_SIZE-1;i++)
        	s_temperature_ntc.med_buff[i] = s_temperature_ntc.med_buff[i+1];
		s_temperature_ntc.med_buff[TEMPERATURE_NTC_FILT_WIN_SIZE-1] = data;
		med_tempe = app_temperature_median_filter(s_temperature_ntc.med_buff,TEMPERATURE_NTC_FILT_WIN_SIZE);
		for(i=0;i<TEMPERATURE_NTC_BUFF_SIZE-1;i++)
        	    s_temperature_ntc.buff[i] = s_temperature_ntc.buff[i+1];
		s_temperature_ntc.buff[TEMPERATURE_NTC_BUFF_SIZE-1] = med_tempe;
		app_temperature_ntc_set_interval(TEMPERATURE_NTC_INTERVAL_ACTION);
		app_temperature_ntc_calulate(s_temperature_ntc.buff);
	}
}
#endif

