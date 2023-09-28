/* Includes ------------------------------------------------------------------*/
#include "u_key.h"

#if (KEY_NUMBER)

#include <stdlib.h>
#include "driver_gpio.h"
#include "udrv_saradc.h"
#include "u_debug.h"

#ifdef KeyDbg
	#define KEY_DBG_INFO(fmt,...)	os_printf("[KEY]"fmt, ##__VA_ARGS__)
	#define KEY_DBG(fmt,...)		os_printf("[KEY]=="fmt, ##__VA_ARGS__)
#else
	#define KEY_DBG_INFO(fmt,...)
	#define KEY_DBG(fmt,...)
#endif
/* define --------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/
/* variables -----------------------------------------------------------------*/
// key struct
#define	KEY_TYPE_GPIO	0
#define	KEY_TYPE_ADC	1

KEY_CONTEXT_t keyArray[KEY_NUMBER];

static inline void key_driver(KEY_CONTEXT_t *pKeyCtx);
static void (*pKeyFunction)(KEY_CONTEXT_t *pKeyInfo_s);
/* function prototypes -------------------------------------------------------*/
/* functions -----------------------------------------------------------------*/
void user_key_init(void* pKeyFuncCbk)
{
	uint8_t i = 0;

#if (GPIO_KEY_NUM)
	const uint8_t keyGpio[GPIO_KEY_NUM] = KeyGpioVal;
	for(i = 0; i < GPIO_KEY_NUM; i++) {
		keyArray[i].index = i;
		keyArray[i].type = KEY_TYPE_GPIO;
		keyArray[i].gpio = keyGpio[i];
		keyArray[i].mulClickEn = 1;
		keyArray[i].keepTime = 0;
		keyArray[i].state = KEY_STAT_INIT;
		keyArray[i].event = KEY_NO_ACTION;
		gpio_config_new(keyArray[i].gpio, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
		KEY_DBG_INFO("key%d GPIO: %d \n",keyArray[i].index, keyGpio[i]);
    }
#endif

#if (ADC_KEY_NUM)
	uint8_t fi;
	const float keyDnR[ADC_KEY_NUM] = KeyDnRVal;	////設置 拉低電阻
	gpio_config_new(get_gpio_by_saradc_ch(SarADC_CH0), GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
	for(i = 0; i < ADC_KEY_NUM; i++) {
		fi = i+GPIO_KEY_NUM;
		keyArray[fi].index = fi;
		keyArray[fi].type = KEY_TYPE_ADC;
		keyArray[fi].gpio = SarADC_CH0;
		uint16_t temp = (((float)(AdcV*keyDnR[i]) / (keyDnR[i]+KeyUpR)* ADC_D2V));
		keyArray[fi].volt_L = temp - VRang;
		if(keyArray[fi].volt_L > 4096) keyArray[fi].volt_L = 0;	//
		keyArray[fi].volt_H = temp + VRang;
		keyArray[fi].mulClickEn = 1;
		keyArray[fi].keepTime = 0;
		keyArray[fi].state = KEY_STAT_INIT;
		keyArray[fi].event = KEY_NO_ACTION;
		KEY_DBG("key%d AdcVal: %d ~ %d  mv:%d\n", keyArray[fi].index, keyArray[fi].volt_L, keyArray[fi].volt_H, saradc_cvt_to_voltage(temp));
	}
#endif
#if (ADC_KEY2_NUM)
	const float key2DnR[ADC_KEY2_NUM] = Key2DnRVal;	////設置 拉低電阻
	gpio_config_new(get_gpio_by_saradc_ch(SarADC_CH1), GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
	for(i = 0; i < ADC_KEY2_NUM; i++) {
		fi = i+GPIO_KEY_NUM+ADC_KEY_NUM;
		keyArray[fi].index = fi;
		keyArray[fi].type = KEY_TYPE_ADC;
		keyArray[fi].gpio = SarADC_CH1;
		uint16_t temp = (((float)(AdcV2*key2DnR[i]) / (key2DnR[i]+Key2UpR)* ADC_D2V));
		keyArray[fi].volt_L = temp - VRang2;
		if(keyArray[fi].volt_L > 4096) keyArray[fi].volt_L = 0;	//
		keyArray[fi].volt_H = temp + VRang2;
		keyArray[fi].mulClickEn = 1;
		keyArray[fi].keepTime = 0;
		keyArray[fi].state = KEY_STAT_INIT;
		keyArray[fi].event = KEY_NO_ACTION;
		KEY_DBG("key%d AdcVal: %d ~ %d\n", keyArray[fi].index, keyArray[fi].volt_L, keyArray[fi].volt_H);
	}
#endif
#if (ADC_KEY3_NUM)
	const float key3DnR[ADC_KEY3_NUM] = Key3DnRVal;	////設置 拉低電阻
	gpio_config_new(get_gpio_by_saradc_ch(SarADC_CH2), GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
	for(i = 0; i < ADC_KEY3_NUM; i++) {
		fi = i+GPIO_KEY_NUM+ADC_KEY_NUM+ADC_KEY2_NUM;
		keyArray[fi].index = fi;
		keyArray[fi].type = KEY_TYPE_ADC;
		keyArray[fi].gpio = SarADC_CH2;
		uint16_t temp = (((float)(AdcV3*key3DnR[i]) / (key3DnR[i]+Key3UpR)* ADC_D2V));
		keyArray[fi].volt_L = temp - VRang3;
		if(keyArray[fi].volt_L > 4096) keyArray[fi].volt_L = 0;	//
		keyArray[fi].volt_H = temp + VRang3;
		keyArray[fi].mulClickEn = 1;
		keyArray[fi].keepTime = 0;
		keyArray[fi].state = KEY_STAT_INIT;
		keyArray[fi].event = KEY_NO_ACTION;
		KEY_DBG("key%d AdcVal: %d ~ %d\n", keyArray[fi].index, keyArray[fi].volt_L, keyArray[fi].volt_H);
	}
#endif
    pKeyFunction = pKeyFuncCbk;
}


//*********************************************************************
uint8_t key_pressing_get(KEY_CONTEXT_t* pKeyCtx)
{
    uint8_t keyPressing = 0;
    if(pKeyCtx->type == KEY_TYPE_ADC){
#if (ADC_KEY_NUM)
        uint16_t code = u_saradc_value_get(pKeyCtx->gpio);
 //       KEY_DBG_INFO("==== SARADC_ADC_GPIO: %d   %d\n", KEY_SarADC_CHN, pKeyCtx->gpio);

	#if 0
        static uint16_t codeR;
        if(abs(codeR-code)>50){
        	codeR=code;
        	KEY_DBG_INFO("==== SARADC_CVT_KEY:%d\n", code);
        }
	#endif
        keyPressing = ((code >= pKeyCtx->volt_L) && (code <= pKeyCtx->volt_H));
        static uint16_t keyPressingR, IndexR;
        if(keyPressing) IndexR = pKeyCtx->index;
        if(keyPressingR != keyPressing && IndexR == pKeyCtx->index){
        	keyPressingR = keyPressing;
           	if(keyPressing) KEY_DBG_INFO("==== SarAdc Key:%d [%d] %d\n", pKeyCtx->volt_L, code, pKeyCtx->volt_H);
        }
#endif
    }else{
#if (GPIO_KEY_NUM)
    //	keyPressing = gpio_input(pKeyCtx->gpio);
    	keyPressing = (gpio_input(pKeyCtx->gpio) ? 0 : 1); // low level valid
#endif
    }
    return keyPressing;
}

//*********************************************************************
static inline void key_driver(KEY_CONTEXT_t *pKeyCtx)
{
    uint8_t keyPressing = key_pressing_get(pKeyCtx);

    pKeyCtx->event = KEY_NO_ACTION;
    switch (pKeyCtx->state)
    {
    case KEY_STAT_INIT:
        pKeyCtx->keepTime = 0;
        pKeyCtx->clickCnt = 0;
        if(keyPressing){
//            KEY_DBG_INFO("\nkey %d detection start >>>>>>>>\n", pKeyCtx->index);
            pKeyCtx->state = KEY_STAT_PRES;
            pKeyCtx->clickCnt = 1;
        }
        break;
    case KEY_STAT_PRES:
        if(keyPressing)
        {
            pKeyCtx->keepTime += KEY_SCAN_TIME;
            //当按下超过一定时间，赋值长按事件，下次扫描切换到长按循环事件
            if(pKeyCtx->keepTime >= LONG_PRES_TIME_MIN){ 
//                KEY_DBG_INFO("num %d long press confirmed! @%d ms\n", pKeyCtx->clickCnt, pKeyCtx->keepTime);
                pKeyCtx->state = KEY_STAT_PRES_L_KEEP;
                pKeyCtx->event = KEY_L_PRES;
            }else if(pKeyCtx->keepTime == KEY_SCAN_TIME){
                // KEY_DBG_INFO("num %d short press down!\n", pKeyCtx->clickCnt);
                pKeyCtx->event = KEY_S_PRES;
            }
        }
        else
        {
//            KEY_DBG_INFO("num %d rel\t PRES:%d ms\n", pKeyCtx->clickCnt, pKeyCtx->keepTime);
            #if 1
            if(pKeyCtx->keepTime >= SHORT_PRES_TIME_MIN){
                if(pKeyCtx->mulClickEn)
                    pKeyCtx->state = KEY_STAT_REL;
                else{
//                    KEY_DBG_INFO("num %d short click confirmed!\n", pKeyCtx->clickCnt);
                    pKeyCtx->event = KEY_S_REL;
                    pKeyCtx->state = KEY_STAT_INIT;
//                    KEY_DBG_INFO(">>>>>>>> key %d detection complete!\n", pKeyCtx->index);
                }
            }else{//debounse, avoid unexpected pulse.
                pKeyCtx->state = KEY_STAT_INIT;
//                KEY_DBG_INFO("EEROR: press time too short!\n");
            }
            #else
            pKeyCtx->state = KEY_STAT_REL;
            #endif
            pKeyCtx->keepTime = 0;	
        }
        break;
    case KEY_STAT_REL:
        if(keyPressing)
        {
            pKeyCtx->state = KEY_STAT_PRES;
            pKeyCtx->clickCnt++;
//            KEY_DBG_INFO("num %d pres\t REL :%d ms\n", pKeyCtx->clickCnt, pKeyCtx->keepTime);
            pKeyCtx->keepTime = 0;
        }
        else
        {
            pKeyCtx->keepTime += KEY_SCAN_TIME;
            //当按下时间没超时，但释放时间超时，则确认单击事件
            if(pKeyCtx->keepTime > REL_OVER_TIME)
            {
//                KEY_DBG_INFO("num %d single click confirmed!\n", pKeyCtx->clickCnt);
                pKeyCtx->event = KEY_S_REL;
                pKeyCtx->state = KEY_STAT_INIT;
                pKeyCtx->keepTime = 0;
//                KEY_DBG_INFO(">>>>>>>> key %d detection complete!\n", pKeyCtx->index);
            }
        }
        break;
    case KEY_STAT_PRES_L_KEEP:
        if(keyPressing){
            pKeyCtx->keepTime += KEY_SCAN_TIME;
            pKeyCtx->event = KEY_L_PRESSING;
        }else{
//            KEY_DBG_INFO("num %d REL, press time:%d ms\n", pKeyCtx->clickCnt, pKeyCtx->keepTime);
            pKeyCtx->event = KEY_L_REL;
            pKeyCtx->state = KEY_STAT_INIT;
            pKeyCtx->keepTime = 0;
//            KEY_DBG_INFO(">>>>>>>> key %d detection complete!\n", pKeyCtx->index);
        }
        break;
    default:
        break;
    }
}

//*********************************************************************
void user_key_scan(void)
{
	uint8_t i;

	for(i = 0; i < KEY_NUMBER; i++){
        //key driver
        key_driver(&keyArray[i]);

        //process multiple click
		if((keyArray[i].event != KEY_NO_ACTION) && (keyArray[i].mulClickEn)){
    //        KEY_DBG_INFO("idx:%d, num:%d, evt:%d\n", keyArray[i].index, keyArray[i].clickCnt, keyArray[i].event);
            switch (keyArray[i].clickCnt){
            case 1: break;
            case 2:
                switch (keyArray[i].event) {
                // case KEY_S_PRES: /*impossible*/ break;
                // case KEY_L_PRES: keyArray[i].event = KEY_DL_PRES; break;
                // case KEY_L_PRESSING: keyArray[i].event = KEY_DL_PRESSING; break;
                case KEY_L_REL: /*第二次按下保持长按后释放也报双击事件*/ //keyArray[i].event = KEY_DL_REL; break;
                case KEY_S_REL: keyArray[i].event = KEY_D_CLICK; break;
                default:
                    keyArray[i].event = KEY_NO_ACTION;
                    break;
                }
                break;
            case 3:
                switch (keyArray[i].event) {
                // case KEY_S_PRES: /*impossible*/ break;
                // case KEY_L_PRES: keyArray[i].event = KEY_TL_PRES; break;
                // case KEY_L_PRESSING: keyArray[i].event = KEY_TL_PRESSING; break;
                case KEY_L_REL: //keyArray[i].event = KEY_TL_REL; break;
                case KEY_S_REL: keyArray[i].event = KEY_T_CLICK; break;
                default: keyArray[i].event = KEY_NO_ACTION; break;
                }
                break;
            case 4:
                switch (keyArray[i].event) {
                case KEY_S_REL: keyArray[i].event = KEY_Q_CLICK; break;
                default: keyArray[i].event = KEY_NO_ACTION; break;
                }
            	break;
            case 5:
                switch (keyArray[i].event) {
                case KEY_S_REL: keyArray[i].event = KEY_5_CLICK; break;
                default: keyArray[i].event = KEY_NO_ACTION; break;
                }
            	break;
            default:
                keyArray[i].event = KEY_NO_ACTION;
                KEY_DBG_INFO("WARN: %d click times.\n", keyArray[i].clickCnt);
                break;
            }
        }

        //key function
        if(keyArray[i].event != KEY_NO_ACTION){
            KEY_DBG_INFO("KEY RESULT -> IDX:%d, num:%d, EVT:%d\n", keyArray[i].index, keyArray[i].clickCnt, keyArray[i].event);
            if(pKeyFunction)
            	pKeyFunction(&keyArray[i]);
        }
	}
}

#endif /* USER_KEY_SUPPORT */
