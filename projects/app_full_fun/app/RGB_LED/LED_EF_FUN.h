
#ifndef	__LED_EF_FUN_H
#define	__LED_EF_FUN_H
#include	"USER_Config.h"

//typedef unsigned char	u8
#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t

	extern	u8 W_Y[9];
	extern	u8 RGB_MODE[9];	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
	extern	u8 RGB_AUTO[9];	//聲控自動轉漸變(0,1)
	extern	u8 RGB_SP[9];	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
	extern	u8 RGB_EF[9];	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
	extern	u8 RGB_AEF[9];	//

#endif

