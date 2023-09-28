#ifndef __DRIVER_VUSB_H__
#define __DRIVER_VUSB_H__
#include "types.h"
#define VUSB_W4_COMM_MODE_TIMEOUT     4       // 4*10ms
typedef enum
{
    VUSB_IDLE_MODE,
    VUSB_STANDBY_MODE,
    VUSB_W4_COMM_MODE,
    VUSB_COMM_MODE,
    VUSB_CHARGE_MODE    
}t_VUSB_mode;
uint8 vusb_set_mode(t_VUSB_mode mode);
t_VUSB_mode vusb_get_mode(void);
void vusb_mode_stm(void);
void vusb_isr(void);
void vusb_int_enable(uint32 mask);
void vusb_int_level(uint32 mask);
uint8_t vusb_is_plug_in(void);
uint8_t vusb_is_charging(void);
uint8_t vusb_4v_ready(void);
uint8_t vusb_2v_ready(void);
uint8_t vusb_1v_ready(void);
#endif
