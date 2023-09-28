#ifndef __TEMPERATURE_NTC__
#define __TEMPERATURE_NTC__

#if CONFIG_TEMPERATURE_NTC

#include "driver_saradc.h"

//0-15度充电电流为20MA，15-45度充电电流为50mA,小于0度或者大于45度停止充电
//小于-20度或则大于60度时，蓝牙需要自动关机，并不可开机

#define TEMPERATURE_NTC_CH_CHANNEL		SARADC_CH_GPIO32

#define TEMPERATURE_NTC_INTERVAL_INIT	5
#define TEMPERATURE_NTC_INTERVAL_ACTION	300 

#define TEMPERATURE_NTC_FILT_WIN_SIZE	3
#define TEMPERATURE_NTC_BUFF_SIZE		6

#define TEMPERATURE_NTC_FULL_SCALE		(4096)
#define TEMPERATURE_NTC_RV				(10000)//9100

#define TEMPERATURE_NTC_PWR_CTRL_PIN				33

typedef  struct _app_temperature_ntc_s
{
    uint8  status;
	uint8 charge_close;
    uint16 adc_interval;
	uint16 buff[TEMPERATURE_NTC_BUFF_SIZE];
	uint16 med_buff[TEMPERATURE_NTC_FILT_WIN_SIZE];
}__PACKED_POST__ app_temperature_ntc_t;

enum
{
	TEMPERATURE_NTC_INIT	= 0,
    TEMPERATURE_NTC_NE_20	= 1,
    TEMPERATURE_NTC_0       = 2,
    TEMPERATURE_NTC_15      = 3,
    TEMPERATURE_NTC_45 		= 4,
    TEMPERATURE_NTC_60   	= 5,
    TEMPERATURE_NTC_MAX_60  = 6
};

enum
{
    TEMPERATURE_NTC_CHARGE_CURRENT_20MA	= 0,
    TEMPERATURE_NTC_CHARGE_CURRENT_30MA  = 1,
    TEMPERATURE_NTC_CHARGE_CURRENT_40MA  = 2,
    TEMPERATURE_NTC_CHARGE_CURRENT_50MA 	= 3,
    TEMPERATURE_NTC_CHARGE_CURRENT_60MA  = 4,
    TEMPERATURE_NTC_CHARGE_CURRENT_70MA  = 5,
    TEMPERATURE_NTC_CHARGE_CURRENT_80MA  = 6,
    TEMPERATURE_RM_CHARGE_CURRENT_90MA  = 7
};

void app_temprature_ntc_saradc_enable(void);
void app_temperature_ntc_set_interval(uint16 interval);
void app_temperature_ntc_set_status(uint8 status);
void app_temprature_ntc_scanning(void);
void app_temperature_ntc_status_updata(uint16 data);
#endif
#endif