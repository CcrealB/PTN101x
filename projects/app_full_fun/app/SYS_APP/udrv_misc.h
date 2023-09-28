
//#ifndef _UDRV_MISC_H_
#define _UDRV_MISC_H_

#include <stdint.h>

#if LED_RGB_NUM
	extern uint8_t LED_RGB[LED_RGB_NUM][3];
#endif
//void RgbLedOut(uint8_t cf);
void RgbLedShow(void);
void RgbLedOutShift(void);
void RgbLedAllColour(uint8_t val);
void RgbLedOneColour(uint8_t n, uint8_t c);

void app_i2s0_init(uint8_t en);
void app_i2s0_open(uint8_t en);
void i2s0_tx_dma_loop_start(void);
void i2s0_tx_dma_loop_stop(void);


#if 1//def IR_TX
void ir_tx_init(uint8_t en);
void pwm_set(uint32_t us, uint8_t OnOff);
#endif

//#endif //_UDRV_MISC_H_
