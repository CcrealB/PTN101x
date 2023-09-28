/* How to use:
1. Config hardware and key struct.
2. Call user_key_init(pKeyFunc) to initialization the hardware and registe pKeyFunc().
3. Poll the user_key_scan() at 50ms period.
4. Realize your function in pKeyFunc().*/

#ifndef _USER_KEY_H_
#define _USER_KEY_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "u_config.h"

#ifndef GPIO_KEY_NUM
	#define GPIO_KEY_NUM	0
#endif
#ifndef ADC_KEY_NUM
	#define ADC_KEY_NUM		0
#endif
#ifndef ADC_KEY2_NUM
	#define ADC_KEY2_NUM	0
#endif
#ifndef ADC_KEY3_NUM
	#define ADC_KEY3_NUM	0
#endif

#define KEY_NUMBER	(GPIO_KEY_NUM + ADC_KEY_NUM + ADC_KEY2_NUM + ADC_KEY3_NUM)

#define ADC_D2V		((float)4096/1870)		//cvt adc value to voltage(1800mV) 1800->1870 20230629

/*key state*/
typedef enum keyState_e{
    KEY_STAT_INIT, 	//init state(must be release state)
    KEY_STAT_PRES,	//press
    KEY_STAT_REL, 	//release
    // KEY_STAT_PRES_L,	//long press confirm
    KEY_STAT_PRES_L_KEEP,	//long press keep
    // KEY_STAT_REL_L, 	//long press release
    // KEY_STAT_EXCEPT,//exception state
}keyState_e;

/*key event or result*/
typedef enum keyEvent_e{
    KEY_S_PRES,			//short or single press
    KEY_S_REL,			//short or single release
    KEY_D_CLICK,		//double click
    KEY_T_CLICK,		//triple click
    KEY_Q_CLICK,		//triple click
    KEY_5_CLICK,		//5 time click
    KEY_L_PRES,			//long press
    KEY_L_PRESSING,		//long press keep
    KEY_L_REL,			//long press release
    KEY_DL_PRES,		//double long press
    KEY_DL_PRESSING,	//double long press keep
    KEY_DL_REL,			//double long press release
    KEY_TL_PRES,		//triple long press
    KEY_TL_PRESSING,	//triple long press keep
    KEY_TL_REL,			//triple long press release
    KEY_NO_ACTION,		//no key pressed or key pressed but no need to deal with
}keyEvent_e;

typedef struct _KEY_CONTEXT_t
{
    uint8_t 	index;
    uint8_t 	type; // 0 = GpioKey, 1= AdcKey
    uint16_t 	volt_L;
    uint16_t 	volt_H;
    uint32_t 	gpio;
    uint8_t 	mulClickEn; //multiple click support, 1:support, 0:not support
    uint8_t 	clickCnt;//for judge single/double/triple/...
    uint16_t 	keepTime;
    keyState_e 	state;
    keyEvent_e 	event;
}KEY_CONTEXT_t;


/*----key scan parameter macro*/
#define KEY_SCAN_TIME 			10
#define SHORT_PRES_TIME_MIN		KEY_SCAN_TIME //short press min time threshold
#define LONG_PRES_TIME_MIN		(1000U) //long press min time threshold

#define PRES_OVER_TIME		(200U)  //key press over time
#define REL_OVER_TIME		(200U) //key release over time


/* function prototypes -------------------------------------------------------*/
void user_key_init(void* pKeyFunc);
void user_key_scan(void);

#endif /* _USER_KEY_H_ */
