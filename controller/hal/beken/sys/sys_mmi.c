/***********************************************************************
 *
 * MODULE NAME:    sys_mmi.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Hardware MMI Interface
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ***********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include "sys_mmi.h"
#include "hw_leds.h"
#include "hw_delay.h"

#define BTCLK_TICK_TOGGLE_MASK (256-1)
#define EVENT_TOGGLE_MASK (64-1)

//static uint8_t mmi_toggle_cnt[LEDS_DATA_WIDTH];

CONST uint8_t _sys_mmi_7seg[]= {
		SYS_MMI_7SEG_0, SYS_MMI_7SEG_1, SYS_MMI_7SEG_2, SYS_MMI_7SEG_3,
		SYS_MMI_7SEG_4, SYS_MMI_7SEG_5, SYS_MMI_7SEG_6, SYS_MMI_7SEG_7,
		SYS_MMI_7SEG_8, SYS_MMI_7SEG_9, SYS_MMI_7SEG_A, SYS_MMI_7SEG_B,
		SYS_MMI_7SEG_C, SYS_MMI_7SEG_D, SYS_MMI_7SEG_E, SYS_MMI_7SEG_F
};

void SYSmmi_Display_Numeric(uint16_t data)
{/*
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_0_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_1_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_2_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_3_BASE, _sys_mmi_7seg[data & 0xF]);*/
}

void SYSmmi_Display_7SEG_D(uint8_t data)
{
#if defined(SEG7_IF_0_BASE) && defined(SEG7_IF_1_BASE)
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_0_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_1_BASE, _sys_mmi_7seg[data & 0xF]);
#endif
}

void SYSmmi_Display_7SEG_C(uint8_t data)
{
#if defined(SEG7_IF_2_BASE) && defined(SEG7_IF_3_BASE)
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_2_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_3_BASE, _sys_mmi_7seg[data & 0xF]);
#endif
}

void SYSmmi_Display_7SEG_B(uint8_t data)
{
#if defined(SEG7_IF_4_BASE) && defined(SEG7_IF_5_BASE)
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_4_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_5_BASE, _sys_mmi_7seg[data & 0xF]);
#endif
}

void SYSmmi_Display_7SEG_A(uint8_t data)
{
#if defined(SEG7_IF_6_BASE) && defined(SEG7_IF_7_BASE)
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_6_BASE, _sys_mmi_7seg[data & 0xF]);
	data >>= 4;
	IOWR_ALTERA_AVALON_PIO_DATA(SEG7_IF_7_BASE, _sys_mmi_7seg[data & 0xF]);
#endif
}

//  LCD Module 16*2
#define lcd_write_cmd(base, data)                     IOWR(base, 0, data)
#define lcd_read_cmd(base)                            IORD(base, 1)
#define lcd_write_data(base, data)                    IOWR(base, 2, data)
#define lcd_read_data(base)                           IORD(base, 3)

void SYSmmi_Initialise(void)
{
    SYSmmi_Display_Event(eSYSmmi_HAL_Test_Event);

	SYSmmi_Display_7SEG_A(0x00);
	SYSmmi_Display_7SEG_B(0x00);
	SYSmmi_Display_7SEG_C(0x00);
	SYSmmi_Display_7SEG_D(0x00);

#if defined(LCD_0_BASE)
	lcd_write_cmd(LCD_0_BASE,0x38);
	HWdelay_Wait_For_ms(1, 0);
	lcd_write_cmd(LCD_0_BASE,0x0C);
	HWdelay_Wait_For_ms(1, 0);
	lcd_write_cmd(LCD_0_BASE,0x01);
	HWdelay_Wait_For_ms(1, 0);
	lcd_write_cmd(LCD_0_BASE,0x06);
	HWdelay_Wait_For_ms(1, 0);
	lcd_write_cmd(LCD_0_BASE,0x80);
	HWdelay_Wait_For_ms(1, 0);

	SYSmmi_Display_LCD("CEVA BT4.0 DM");
#endif

}

void SYSmmi_Display_LCD(char* text)
{
#if defined(LCD_0_BASE)
    int i;

    if (text)
    {
        for(i=0;text[i];i++)
	    {
		    lcd_write_data(LCD_0_BASE,text[i]);
		    HWdelay_Wait_For_ms(1, 0);
	    }
    }

	lcd_write_cmd(LCD_0_BASE,0xC0);
    HWdelay_Wait_For_ms(1, 0);
#endif
}

void SYSmmi_Display_eSYSmmi_BTCLK_Tick_Event(void)
{
/*    ++mmi_toggle_cnt[0];

    if (!(mmi_toggle_cnt[0] & BTCLK_TICK_TOGGLE_MASK))
    {
        HWled_Toggle(eSYSmmi_BTCLK_Tick_Event);
    }*/
}

void SYSmmi_Display_eSYSmmi_Periodic_Event(t_SYSmmi_Event event)
{
/*	if (event < LEDS_DATA_WIDTH)
	{
		++mmi_toggle_cnt[event];

		if (!(mmi_toggle_cnt[event] & EVENT_TOGGLE_MASK))
		{
			HWled_Toggle(event);
		}
	}*/
}

void SYSmmi_Display_Event(t_SYSmmi_Event event)
{
#if 0
	switch(event)
	{
	case eSYSmmi_BTCLK_Tick_Event:
		SYSmmi_Display_eSYSmmi_BTCLK_Tick_Event();
		break;

	case eSYSmmi_LC_Connection_Event:
		HWled_On(eSYSmmi_LC_Connection_Event);
		break;
	case eSYSmmi_LM_Connection_Event:
        HWled_On(eSYSmmi_LM_Connection_Event);
        break;
	case eSYSmmi_Scan_Active_Event:
		HWled_On(eSYSmmi_Scan_Active_Event);
		break;

	case eSYSmmi_HAL_Corruption_Event:
		SYSmmi_Display_Numeric(0xDEAD); /* 7-SEG */
		break;
	case eSYSmmi_HAL_Test_Event:
		SYSmmi_Display_Numeric(0xFFFF); /* 7-SEG */
		break;
	case eSYSmmi_HAL_Incompatible_Event:
		SYSmmi_Display_Numeric(0xC0DE); /* 7-SEG */
		break;
	case eSYSmmi_SPI_Conflict_Event:
		SYSmmi_Display_Numeric(0xC0FC); /* 7-SEG */
		break;

	case eSYSmmi_LC_Disconnection_Event:
		HWled_Off(eSYSmmi_LC_Connection_Event);
		break;
	case eSYSmmi_LM_Disconnection_Event:
        HWled_Off(eSYSmmi_LM_Connection_Event);
        break;
	case eSYSmmi_Scan_Inactive_Event:
		HWled_Off(eSYSmmi_Scan_Active_Event);
		break;

	default:
		SYSmmi_Display_eSYSmmi_Periodic_Event(event);
	}
#endif
}
