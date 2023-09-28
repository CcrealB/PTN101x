
//#ifndef _UDRV_SARADC_H_
#define _UDRV_SARADC_H_

#include "u_config.h"

#include "sys_types.h"
#include "driver_saradc.h"
#include "driver_gpio.h"

/*how to use?
1, config the macro definition SarADC_USER_CHx.
2, call u_saradc_init() to set gpio to appropriate state.
3, call u_saradc_value_get() to get adc sample code.
4, call saradc_cvt_to_voltage() to convert adc sample code to voltage(unit:mv)
*/
////////////////// user SarADC config

#ifndef SarADC_CLK_Hz
    #define SarADC_CLK_Hz       SarADC_CLK_2p6MHz
#endif

GPIO_PIN get_gpio_by_saradc_ch(SARADC_CH_e sadc_chn);
uint16_t saradc_cvt_to_voltage(uint16_t adc_val);
int u_saradc_val_update_req(SARADC_CH_e ssadc_chn);

//void u_saradc_init(SARADC_CH_e sadc_chn);
uint16_t u_saradc_value_get(SARADC_CH_e sadc_chn);
uint16_t user_saradc_val_get(uint8_t usr_chn);//get adc value by user define channel number
void user_saradc_update_trig(void);
void user_saradc_update_req(void);
int usr_saradc_val_update_isr(SARADC_CH_e chn, uint16_t adc_val);

uint16_t sar_adc_voltage_get(SARADC_CH_e sadc_chn);

void u_SaradcKbon_Init();

#ifdef SarAdcChVal
	extern uint16_t SarAdcVal[SarAdc_NUM];
	extern uint16_t SarAdcValR[SarAdc_NUM];
#endif

#ifdef	KbonNUMBER
	uint16_t KnobVal[KbonNUMBER];
	uint16_t KnobValR[KbonNUMBER];
#endif

#ifdef DEBUG_IC_TEMPERATURE
	void debug_show_temp_senser_proc(void);
	void debug_show_temp_senser(void);
#endif
//#endif /* _UDRV_SARADC_H_ */
