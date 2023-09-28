/*
 * ST7789V.c
 *  Created on: 2022年11月12日
 *      Author: yuanf
 */
#include "USER_Config.h"

#ifdef LCD_ST7789_EN
#include 	".\LCD_ST7789V\ST7789V.h"
#include 	"LCD_ST7789V/font1206.h"
#include	".\AMG8802\app.h"
#include	".\AMG8802\amg8802.h"

//#define u8	uint8_t
//#define u16	uint16_t
//#define u32	uint32_t

//#include "font24x24.h"
//#include "font48x48.h"
//#include "font32x32.h"

#include "font12x24.h"	//NG
#include "font16x32.h"
#include "font24x48.h"
#include "font32x64.h"
#include "font40x80.h"
#include "font48x96.h"
#include "font64x128.h"
#include "picture.h"
#include "font8x16.h"

#define LCD_DBG_INFO(fmt,...)       os_printf("[USER]"fmt, ##__VA_ARGS__)

//**** 定义控制端口 *********
#define LCD_CS		GPIO2
#define LCD_CD		GPIO10
#define LCD_RESET	GPIO13
#define LCD_MOSI	GPIO4
#define LCD_MISO	GPIO5
#define LCD_SCK		GPIO3
#if defined(TL8035) || defined(TL8025v2)
	#define	LCD_LED_EN	GPIO31
#else
	#define	LCD_LED_EN	GPIO22
#endif
//#define	LCD_LED_EN	GPIO14

#define LCD_CS_L	gpio_output(LCD_CS, 0)
#define LCD_CS_H	gpio_output(LCD_CS, 1)
#define LCD_CD_L	gpio_output(LCD_CD, 0)
#define LCD_CD_H	gpio_output(LCD_CD, 1)
#define LCD_RESET_L	gpio_output(LCD_RESET, 0)
#define LCD_RESET_H	gpio_output(LCD_RESET, 1)
#define LCD_MOSI_L	gpio_output(LCD_MOSI, 0)
#define LCD_MOSI_H	gpio_output(LCD_MOSI, 1)
#define LCD_MISO_R	gpio_input(LCD_MISO)
#define LCD_SCK_L	gpio_output(LCD_SCK, 0)
#define LCD_SCK_H	gpio_output(LCD_SCK, 1)
#define add 10


uint8_t DFT_SCAN_DIR;

//LCD的画笔颜色和背景色
uint16_t POINT_COLOR = WHITE;	//画笔颜色
uint16_t BACK_COLOR = BLACK;  	//背景色

_lcd_dev lcddev;

#ifdef CONFIG_USER_SPI_FUNC
	extern void user_spi_init(uint8_t spi_16bit_en);
	extern int user_spi_is_busy(void);
	extern int user_spi_transfer(uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz, uint8_t tx_addr_fixed);
	extern int user_spi1_transfer(uint16_t* tx_buf, int tx_sz, uint16_t* rx_buf, int rx_sz, uint16_t tx_addr_fixed);
	uint8_t SpiTxBuff[5];
#endif

void _nop(){}

//**** 写寄存器 ***************
void LCD_WR_REG(u8 REG)
{
//	LCD_CS_L;
	LCD_CD_L;
#ifndef CONFIG_USER_SPI_FUNC
	for(u8 i=0; i<8; i++){
		if (REG & 0x80)	LCD_MOSI_H;
			else		LCD_MOSI_L;
		REG <<= 1;
		LCD_SCK_L;
		LCD_SCK_H;
	}//送低8位
#else
	SpiTxBuff[0] = REG;
	user_spi_transfer((u8*) SpiTxBuff, 1, NULL, 0, 0);
	while(user_spi_is_busy());
#endif
//	LCD_CS_H;
}
//**** 写数据 ***************
void LCD_WR_DATA(u8 DATA)
{
//	LCD_CS_L;
	LCD_CD_H;
#ifndef CONFIG_USER_SPI_FUNC
	for(u8 i=0; i<8; i++){
		if (DATA & 0x80)	LCD_MOSI_H;
			else			LCD_MOSI_L;
		DATA <<= 1;
		LCD_SCK_L;
		LCD_SCK_H;
	}//送低8位
#else
	SpiTxBuff[0] = DATA;
	user_spi_transfer((u8*) SpiTxBuff, 1, NULL, 0, 0);
	while(user_spi_is_busy());
#endif
//	LCD_CS_H;
}

//**** LCD写GRAM //RGB_Code:颜色值
void LCD_WriteRAM(u16 DAT)
{
//	LCD_CS_L;
	LCD_CD_H;
#ifndef CONFIG_USER_SPI_FUNC
	for(u8 i=0; i<16; i++){
		if (DAT & 0x8000)	LCD_MOSI_H;
			else			LCD_MOSI_L;
		DAT <<= 1;
		LCD_SCK_L;
		LCD_SCK_H;
	}
#else
	SpiTxBuff[0] = ((DAT>>8)&0xFF);
	SpiTxBuff[1] = (DAT&0xFF);
	user_spi_transfer((u8*) SpiTxBuff, 2, NULL, 0, 0);
	while(user_spi_is_busy());
#endif
//	LCD_CS_H;
}

//**** 写寄存器 ****************************************
void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue)
{
	LCD_WR_REG(LCD_Reg);
	LCD_WR_DATA(LCD_RegValue);
}

//**** 开始写GRAM ******************
void LCD_WriteRAM_Prepare(void)
{
 	LCD_WR_REG(lcddev.wramcmd);
}


//**** 设置光标位置 	Xpos:横坐标	Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(Xpos>>8);
	LCD_WR_DATA(Xpos&0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(Ypos>>8);
	LCD_WR_DATA(Ypos&0XFF);
}
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/8989/5408/9341等IC已经实际测试
void LCD_Scan_Dir(u8 dir)
{
	u16 regval=0;
	u8 dirreg=0;
//	u16 temp;
	switch(dir){//方向转换
		case 0:	dir=6;	break;
		case 1:	dir=7;	break;
		case 2:	dir=4;	break;
		case 3:	dir=5;	break;
		case 4:	dir=1;	break;
		case 5:	dir=0;	break;
		case 6:	dir=3;	break;
		case 7:	dir=2;	break;
	}
	switch(dir){
		case L2R_U2D://从左到右,从上到下
			regval|=(0<<7)|(0<<6)|(0<<5);
			break;
		case L2R_D2U://从左到右,从下到上
			regval|=(1<<7)|(0<<6)|(0<<5);
			break;
		case R2L_U2D://从右到左,从上到下
			regval|=(0<<7)|(1<<6)|(0<<5);
			break;
		case R2L_D2U://从右到左,从下到上
			regval|=(1<<7)|(1<<6)|(0<<5);
			break;
		case U2D_L2R://从上到下,从左到右
			regval|=(0<<7)|(0<<6)|(1<<5);
			break;
		case U2D_R2L://从上到下,从右到左
			regval|=(0<<7)|(1<<6)|(1<<5);
			break;
		case D2U_L2R://从下到上,从左到右
			regval|=(1<<7)|(0<<6)|(1<<5);
			break;
		case D2U_R2L://从下到上,从右到左
			regval|=(1<<7)|(1<<6)|(1<<5);
			break;
	}
	dirreg=0X36;
	regval|=0x00;
	LCD_WriteReg(dirreg,regval);

	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(0);LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.width-1)>>8);
	LCD_WR_DATA((lcddev.width-1)&0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(0);LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.height-1)>>8);
	LCD_WR_DATA((lcddev.height-1)&0XFF);
}
//**** 画点/	x,y:坐标	/POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y)
{
	LCD_SetCursor(x,y);		//设置光标位置
	LCD_WriteRAM_Prepare();	//开始写入GRAM
	LCD_WriteRAM(POINT_COLOR);
}

//**** 设置LCD显示方向（6804不支持横屏显示）dir:0,竖屏；1,横屏
void LCD_Display_Dir(u8 dir)
{
	if(dir==0){			//竖屏
		lcddev.dir=0;
		lcddev.width=240;
		lcddev.height=320;
		lcddev.wramcmd=0X2C;
		lcddev.setxcmd=0X2A;
		lcddev.setycmd=0X2B;
		DFT_SCAN_DIR=U2D_R2L;
	}else{ 				//横屏
		lcddev.dir=1;
		lcddev.width=320;
		lcddev.height=240;
		lcddev.wramcmd=0X2C;
		lcddev.setxcmd=0X2A;
		lcddev.setycmd=0X2B;
	//	DFT_SCAN_DIR=L2R_U2D;
		DFT_SCAN_DIR=R2L_D2U;

	}
	LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
}


//*********************************
void ST7789V_BL_EN(uint8_t OnOff)
{
	if(OnOff)	gpio_output(LCD_LED_EN, 1);
		else	gpio_output(LCD_LED_EN, 0);
}
//**** 初始化lcd **********************
void ST7789V_GpioInit()
{
	gpio_config_new(LCD_LED_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);	ST7789V_BL_EN(0);
	gpio_config_new(LCD_RESET, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);	LCD_RESET_L;
	gpio_config_new(LCD_CS, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);		LCD_CS_H;
	gpio_config_new(LCD_CD, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);		LCD_CD_H;

#ifndef CONFIG_USER_SPI_FUNC
    gpio_config_new(LCD_MOSI, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_config_new(LCD_MISO, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(LCD_SCK, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LCD_SCK, 1);
#else
    user_spi_init(1);
#endif
    LCD_RESET_H;
    LCD_CS_L;
}
//**** 初始化lcd **********************
void ST7789V_ExitSleep()
{
	LCD_WR_REG(0x11);	//Exit Sleep
	//delay_ms(120); 	//Delay 120ms
}
//**** 初始化lcd **********************
void ST7789V_Init(void)
{
#if 0
	//----------------------------------------------------
	LCD_RESET_L;
	delay_ms(10);
	LCD_RESET_H;
	delay_ms(120);

	LCD_WR_REG(0x11);	//Exit Sleep
	delay_ms(120); 		//Delay 120ms
#endif
	//---- Display and color format setting
	LCD_WR_REG(0x36);	LCD_WR_DATA(0x00);
	LCD_WR_REG(0x3A);	LCD_WR_DATA(0x05);
	//----  ST7789V Frame rate setting ------
	LCD_WR_REG(0xb2);	LCD_WR_DATA(0x0c);	LCD_WR_DATA(0x0c);	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x33);	LCD_WR_DATA(0x33);
	LCD_WR_REG(0xb7);	LCD_WR_DATA(0x35);
	//---- ST7789V Power setting ------
	LCD_WR_REG(0xbb);	LCD_WR_DATA(0x28);
	LCD_WR_REG(0xc0);	LCD_WR_DATA(0x2c);
	LCD_WR_REG(0xc2);	LCD_WR_DATA(0x01);	LCD_WR_REG(0xc3);	LCD_WR_DATA(0x0b);
	LCD_WR_REG(0xc4);	LCD_WR_DATA(0x20);	LCD_WR_REG(0xc6);	LCD_WR_DATA(0x0f);
	LCD_WR_REG(0xd0);	LCD_WR_DATA(0xa4);	LCD_WR_DATA(0xa1);
	//---- ST7789V gamma setting ------
	LCD_WR_REG(0xe0);	LCD_WR_DATA(0xd0);	LCD_WR_DATA(0x01);	LCD_WR_DATA(0x08);
	LCD_WR_DATA(0x0f);	LCD_WR_DATA(0x11);	LCD_WR_DATA(0x2a);	LCD_WR_DATA(0x36);
	LCD_WR_DATA(0x55);	LCD_WR_DATA(0x44);	LCD_WR_DATA(0x3a);	LCD_WR_DATA(0x0b);
	LCD_WR_DATA(0x06);	LCD_WR_DATA(0x11);	LCD_WR_DATA(0x20);	LCD_WR_REG(0xe1);
	LCD_WR_DATA(0xd0);	LCD_WR_DATA(0x02);	LCD_WR_DATA(0x07);	LCD_WR_DATA(0x0a);
	LCD_WR_DATA(0x0b);	LCD_WR_DATA(0x18);	LCD_WR_DATA(0x34);	LCD_WR_DATA(0x43);
	LCD_WR_DATA(0x4a);	LCD_WR_DATA(0x2b);	LCD_WR_DATA(0x1b);	LCD_WR_DATA(0x1c);
	LCD_WR_DATA(0x22);	LCD_WR_DATA(0x1f);
	//---------------------------------------
	LCD_Display_Dir(1);	//0=竖屏	1=横屏
	LCD_Clear(BLACK);
	LCD_WR_REG(0x29);	//Display on
}

//**** 已指定區域清屏函数  color:要清屏的填充色
void LCD_Clear(u16 color)
{
	u32 totalpoint=lcddev.width;
	totalpoint*=lcddev.height; 	//得到总点数
	LCD_SetCursor(0x00,0x0000);	//设置光标位置
	LCD_WriteRAM_Prepare();     //开始写入GRAM

#if CONFIG_USER_SPI_FUNC
	uint16_t i;
	uint16_t L1=totalpoint/20;
	uint16_t L2=totalpoint/1920;
	LCD_CD_H;
	uint8_t buff[3840];
	for(uint16_t i=0; i<L1; i+=2){
		buff[i] = (color>>8)&0xFF;
		buff[i+1] = color&0xFF;
	}
	for(i=0; i<L2; i++){
		user_spi_transfer((u8*)buff, L1, NULL, 0, 0);
		while(user_spi_is_busy());
	}
#else
	for(u32 index=0;index<totalpoint;index++){
		LCD_WriteRAM(color);
	}
#endif
}

//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{
	u16 i,j;
	u16 xlen=0;
	xlen=ex-sx+1;
	for(i=sy;i<=ey;i++){
	 	LCD_SetCursor(sx,i);      				//设置光标位置
		LCD_WriteRAM_Prepare();     			//开始写入GRAM
		for(j=0;j<xlen;j++)LCD_WriteRAM(color);	//设置光标位置
	}
}


//画线	/x1,y1:起点坐标	/x2,y2:终点坐标
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if(delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;//水平线
	else{incy=-1;delta_y=-delta_y;}
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<=distance+1;t++){//画线输出
		LCD_DrawPoint(uRow,uCol);//画点
		xerr+=delta_x ;
		yerr+=delta_y ;
		if(xerr>distance){
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance){
			yerr-=distance;
			uCol+=incy;
		}
	}
}

//在指定位置画一个指定大小的圆	/(x,y):中心点	/r    :半径
void Draw_Circle(u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		LCD_DrawPoint(x0+a,y0-b);             //5
 		LCD_DrawPoint(x0+b,y0-a);             //0
		LCD_DrawPoint(x0+b,y0+a);             //4
		LCD_DrawPoint(x0+a,y0+b);             //6
		LCD_DrawPoint(x0-a,y0+b);             //1
 		LCD_DrawPoint(x0-b,y0+a);
		LCD_DrawPoint(x0-a,y0-b);             //2
  		LCD_DrawPoint(x0-b,y0-a);             //7
		a++;
		//使用Bresenham算法画圆
		if(di<0)di +=4*a+6;
		else
		{
			di+=10+4*(a-b);
			b--;
		}
	}
}

//设置窗口
void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{
	width=sx+width-1;
	height=sy+height-1;

	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(sx>>8);
	LCD_WR_DATA(sx&0XFF);
	LCD_WR_DATA(width>>8);
	LCD_WR_DATA(width&0XFF);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(sy>>8);
	LCD_WR_DATA(sy&0XFF);
	LCD_WR_DATA(height>>8);
	LCD_WR_DATA(height&0XFF);

	LCD_WR_REG(0x2c);	//yuan++
}

/***********************************************************
  在指定区域内填充指定颜色块
 (sx,sy),(ex,ey):填充矩形对角坐标
 fColor:要填充的颜色
***********************************************************/
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 fColor)
{
	u16	len = ex*ey*2;
	u8 buff[len];
	u16 k=0;
	u16 i;
	LCD_Set_Window(sx,sy,ex,ey);
	for(i=0; i<(len/2);i++) {
		buff[k] = (fColor>>8)&0xFF;
		buff[k+1] = fColor&0xFF;
		k +=2;
	}
//	LCD_DBG_INFO("==== %d\n",k);
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, len, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(i=0; i<len; i++) LCD_WR_DATA(buff[i]);
#endif
}

//画矩形	/(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 lineW, u16 fColor)
{
	LCD_Color_Fill(x1, 			y1, 			x2, 	lineW, 	fColor);
	LCD_Color_Fill(x1, 			y1+y2-lineW,	x2, 	lineW, 	fColor);
	LCD_Color_Fill(x1, 			y1, 			lineW, 	y2,		fColor);
	LCD_Color_Fill(x1+x2-lineW, y1, 			lineW,	y2,		fColor);
}


//*****************************************************************************
//*****************************************************************************

//***************************************************************
void LCD_PutChar8x16(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+8>lcddev.width)	return;
	uint8_t buff[256];
	u16 k=0;
	u8 i,j,m;
	LCD_Set_Window(x,y,8,16);
	c=c-' ';
	for(i=0; i<16;i++) {
		m=font8x16[c][i];

		for(j=0;j<8;j++) {
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 256, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(k=0; k<256; k++) LCD_WR_DATA(buff[k]);
#endif
}

#ifdef __FONT12x24_H
//*************************************************
void LCD_PutChar12x24(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+12>lcddev.width)	return;
	uint8_t buff[576];
	u16 k=0;
	u8 i,j,n,m;
	LCD_Set_Window(x,y,12,24);
	c=c-' ';
	for(i=0; i<48;i++) {
		m=font12x24[c][i];
		if(i%2==0)	n = 8;
			else	n = 4;
		for(j=0;j<n;j++) {
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
//	LCD_DBG_INFO("==== %d\n",k);
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 576, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(k=0; k<576; k++) LCD_WR_DATA(buff[k]);
#endif
}
#endif
#ifdef __FONT16x32_H
//***************************************************************
void LCD_PutChar16x32(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+16>lcddev.width)	return;
	uint8_t buff[1024];
	u16 k=0;
	u8 i,j,m;
	LCD_Set_Window(x,y,16,32);
	c=c-' ';
	for(i=0; i<64;i++) {
		m=font16x32[c][i];
		for(j=0;j<8;j++){
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 1024, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(k=0; k<1024; k++) LCD_WR_DATA(buff[k]);
#endif
}
#endif
#ifdef __FONT24x48_H
//*************************************************
void LCD_PutCharNum16x32(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+16>lcddev.width)	return;
	uint8_t buff[1024];
	u16 k=0;
	u8 i,j,m;
	LCD_Set_Window(x,y,16,32);
	for(i=0; i<64;i++) {
		m=num16x32[c][i];
		for(j=0;j<8;j++){
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 1024, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(k=0; k<1024; k++) LCD_WR_DATA(buff[k]);
#endif
}
//*************************************************
void LCD_PutChar24x48(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+24>lcddev.width)	return;
	uint8_t buff[2304];
	u8 i,j,m;
	u16 k=0;
	LCD_Set_Window(x,y,24,48);
	c -= ' ';
	for(i=0; i<144;i++) {
		m=font24x48[c][i];
		for(j=0;j<8;j++) {
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 2304, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(k=0; k<2304; k++) LCD_WR_DATA(buff[k]);
#endif
}
#endif
#ifdef __FONT32x64_H
//*************************************************
void LCD_PutChar32x64(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+32>lcddev.width)	return;
	uint8_t buff[4096];
	u16 i,k=0;
	u8 j,m;
	c -= '0';	//设定asc内码偏移量
	LCD_Set_Window(x,y,32,64);
	for(i=0; i<256;i++) {
		if(c>9)	m=0;
		 else	m=font32x64[c][i];
		for(j=0;j<8;j++) {
			if((m&0x01)){
				buff[k] = (fColor>>8)&0xFF;
				buff[k+1] = fColor&0xFF;
			}else{
				buff[k] = (bColor>>8)&0xFF;
				buff[k+1] = bColor&0xFF;
			}
			k +=2;
			m>>=1;
		}
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;
	user_spi_transfer((u8*)buff, 2048, NULL, 0, 0);
	while(user_spi_is_busy());
	user_spi_transfer((u8*)&buff[2048], 2048, NULL, 0, 0);
	while(user_spi_is_busy());
#else
	for(i=0; i<4096; i++) LCD_WR_DATA(buff[i]);
#endif
}
#endif
#ifdef __FONT40x80_H
//*************************************************
void LCD_PutChar40x80(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+40>lcddev.width)	return;
	u16 i,j;
	c=c-'0';	//设定asc内码偏移量
	LCD_Set_Window(x,y,40,80);
	for(i=0; i<400;i++) {
		u8 m=font40x80[c][i];
		for(j=0;j<8;j++) {
			if((m&0x01))LCD_WriteRAM(fColor);
				else 	LCD_WriteRAM(bColor);
			m>>=1;
		}
	}
}
#endif
#ifdef __FONT48x96_H
//*************************************************
void LCD_PutChar48x96(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+48>lcddev.width)	return;
	u16 i,j;
	c -= '0';	//设定asc内码偏移量
	LCD_Set_Window(x,y,48,96);
	for(i=0; i<576;i++) {
		u8 m=font48x96[c][i];
		for(j=0;j<8;j++) {
			if((m&0x01))LCD_WriteRAM(fColor);
				else 	LCD_WriteRAM(bColor);
			m>>=1;
		}
	}
}
#endif
#ifdef __FONT64x128_H
//*************************************************
void LCD_PutChar64x128(u16 x, u16 y, u8 c, u16 fColor, u16 bColor)
{
	if(x+64>lcddev.width)	return;
	u16 i,j;
	c -= '0';	//设定asc内码偏移量
	LCD_Set_Window(x,y,64,128);
	for(i=0; i<1024;i++) {
		u8 m=font64x128[c][i];
		for(j=0;j<8;j++) {
			if((m&0x01))LCD_WriteRAM(fColor);
				else 	LCD_WriteRAM(bColor);
			m>>=1;
		}
	}
}
#endif


#ifdef __FONT24x24_H
//***********************************************************
void Put24x24(u16 x, u16 y, u8 c[2], u16 fColor, u16 bColor)
{
	u8 i,j;
	u16 k;
	u8 HZnum=sizeof(font24x24Index)/2;	//自动统计汉字数目

	LCD_SetPos(x,  x+24-1,y, y+24-1);

	for (k=0;k<HZnum;k++) { //自建汉字库中的个数，循环查询内码
	//	if ((font24x24[k].Index[0]==c[0])&&(font24x24[k].Index[1]==c[1])){
		if ((font24x24Index[(k*2)]==c[0])&&(font24x24Index[(k*2+1)]==c[1])){
    		for(i=0;i<72;i++){
	//	  		u8 m=font24x24[k].Msk[i];
				u8 m=font24x24[k][i];
		  		for(j=0;j<8;j++) {
					if((m&0x80)==0x80) {
						write_data16(fColor);
					}else{
						write_data16(bColor);
					}
					m<<=1;
				}
		  	}
		}
	}
}
#endif

#ifdef __FONT32x32_H
//*************************************************
void Put32x32(u16 x, u16 y, u8 c[2], u16 fColor,u16 bColor)
{
	u8 i,j;
	u16 k;
	u8 HZnum=sizeof(font32x32)/2;	//自动统计汉字数目

	LCD_SetPos(x,  x+32-1,y, y+32-1);

	for (k=0;k<HZnum;k++) { //15标示自建汉字库中的个数，循环查询内码
		if ((font32x32Index[k*2]==c[0])&&(font32x32Index[k*2+1]==c[1])){
    		for(i=0;i<128;i++){
		  		u8 m=font32x32[k][i];
		  		for(j=0;j<8;j++) {
					if((m&0x80)==0x80) {
						write_data16(fColor);
					}else{
						write_data16(bColor);
					}
					m<<=1;
				}
		  	}
		}
	}
}
#endif

#ifdef __FONT48x48_H
//*************************************************
void Put48x48(u16 x, u16 y, u8 c[2], u16 fColor,u16 bColor)
{
	u8 j;
	u16 i,k;
	u8 HZnum=sizeof(font48x48)/2;	//自动统计汉字数目

	LCD_SetPos(x,  x+48-1,y, y+48-1);

	for (k=0;k<HZnum;k++) { //15标示自建汉字库中的个数，循环查询内码
		if ((font48x48Index[k*2]==c[0])&&(font48x48Index[k*2+1]==c[1])){
    		for(i=0;i<288;i++){
		  		u8 m=font48x48[k][i];
		  		for(j=0;j<8;j++) {
					if((m&0x80)==0x80) {
						write_data16(fColor);
					}else{
						write_data16(bColor);
					}
					m<<=1;
				}
		  	}
		}
	}
}
#endif

//*****************************************************************************
void LCD_PutString(u16 x, u16 y, u8 *s, u8 size, u16 fColor, u16 bColor)
{
	u8 l=0;
	while(*s){
		if( *s < 0x80){

			if(size==16){LCD_PutChar8x16(x+l*8,y,*s,fColor,bColor);	s++;l++;}
#ifdef __FONT12x24_H
			if(size==24){LCD_PutChar12x24(x+l*12,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT16x32_H
			if(size==32){LCD_PutChar16x32(x+l*16,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT24x48_H
			if(size==48){LCD_PutChar24x48(x+l*24,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT32x64_H
			if(size==64){LCD_PutChar32x64(x+l*32,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT40x80_H
			if(size==80){LCD_PutChar40x80(x+l*40,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT48x96_H
			if(size==96){LCD_PutChar48x96(x+l*48,y,*s,fColor,bColor);	s++;l++;}
#endif
#ifdef __FONT64x128_H
			if(size==128){LCD_PutChar64x128(x+l*64,y,*s,fColor,bColor);	s++;l++;}
#endif
		}else{
#ifdef __FONT24x24_H
			if(size==24){Put24x24(x+l*8, y, (u8*)s, fColor, bColor);	s+=2;l+=3;}
#endif
#ifdef __FONT32x32_H
			if(size==32){Put32x32(x+l*8, y, (u8*)s, fColor, bColor);	s+=2;l+=4;}
#endif
#ifdef __FONT48x48_H
			if(size==48){Put48x48(x+l*8, y, (u8*)s, fColor, bColor);	s+=2;l+=6;}
#endif
		}
	}
}


//m^n函数		/返回值:m^n次方.
u32 LCD_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;
	return result;
}
/******************************************************************************
 显示数字,高位为0, 显示 mode:0=' ' ,1='0'
 x,y:起点坐标	num:数值(0~999999999)		len:位数	size:字体大小
 mode: [7]:0,不填充;1,填充0. [6:1]:保留  [0]:0,非叠加显示;1,叠加显示.
*******************************************************************************/
void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{
	u8 t,temp;
	u8 enshow=0;
	char str[10];
	u8 f=0;
	for(t=0;t<len;t++){
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1)){
			if(temp==0){
				if(mode&0X80)		str[f]='0';	//"LCD_PutChar32x64(x+(size/2)*t,y,'0',0xFFFF,0);
				else 				str[f]=' ';	//LCD_PutChar32x64(x+(size/2)*t,y,' ',0xFFFF,0);
				f++;
 				continue;
			}else{
				enshow=1;
			}
		}
		str[f]=temp+'0';	//LCD_PutChar32x64(x+(size/2)*t,y,temp+'0',0xFFFF,0);
		f++;
	}
	str[f]=0;
	LCD_PutString(x, y, (u8*)str, size, POINT_COLOR, BACK_COLOR);
}

void LCD_ShowxNum_red(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{
	u8 t,temp;
	u8 enshow=0;
	char str[10];
	u8 f=0;
	for(t=0;t<len;t++){
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1)){
			if(temp==0){
				if(mode&0X80)		str[f]='0';	//"LCD_PutChar32x64(x+(size/2)*t,y,'0',0xFFFF,0);
				else 				str[f]=' ';	//LCD_PutChar32x64(x+(size/2)*t,y,' ',0xFFFF,0);
				f++;
 				continue;
			}else{
				enshow=1;
			}
		}
		str[f]=temp+'0';	//LCD_PutChar32x64(x+(size/2)*t,y,temp+'0',0xFFFF,0);
		f++;
	}
	str[f]=0;
	LCD_PutString(x, y, (u8*)str, size, RED, BACK_COLOR);
}

void LCD_ShowxNum_gre(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{
	u8 t,temp;
	u8 enshow=0;
	char str[10];
	u8 f=0;
	for(t=0;t<len;t++){
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1)){
			if(temp==0){
				if(mode&0X80)		str[f]='0';	//"LCD_PutChar32x64(x+(size/2)*t,y,'0',0xFFFF,0);
				else 				str[f]=' ';	//LCD_PutChar32x64(x+(size/2)*t,y,' ',0xFFFF,0);
				f++;
 				continue;
			}else{
				enshow=1;
			}
		}
		str[f]=temp+'0';	//LCD_PutChar32x64(x+(size/2)*t,y,temp+'0',0xFFFF,0);
		f++;
	}
	str[f]=0;
	LCD_PutString(x, y, (u8*)str, size, GREEN, BACK_COLOR);
}
/******************************************************************************
 x,y:起点坐标	num:数值(0~999999999)		len:位数	  fColor:前景色        bColor:背景色
*******************************************************************************/
void ShowNum(u16 x, u16 y, u32 num,u8 len,u16 fColor, u16 bColor)
{
	u8 t;
//	LCD_PutCharNum16x32(x,y,num/LCD_Pow(10,len-1)%10, fColor, bColor);
//	if(num>10)
		for(t=len;t>0;t--)
		{
			LCD_PutCharNum16x32(x+((t-1)*20),y,num%10, fColor, bColor);
			num=num/10;
		}

}

//****************************************
void ST7789V_TEST(void)
{
 	static u8 x=0;
// 	uint8_t TestStr[] = "12345678901234567890 TFTLCD TEST";
	POINT_COLOR = WHITE;
	BACK_COLOR = BLUE;
	switch(x){
		case 0:
			LCD_Clear(GREEN);
		//	LCD_ShowString(10,10,200,32,16,TestStr);
		//	LCD_PutString(40, 10, TestStr, 80, WHITE, BLUE);
			break;
		case 1:
			LCD_Clear(BLUE);
		//	LCD_PutString(48, 100, TestStr, 96, WHITE, BLUE);
			break;
		case 2:
			LCD_Clear(WHITE);
		//	LCD_PutString(64, 110, TestStr, 128, WHITE, BLUE);
			break;
	}
	os_printf("==== init_cont:%d\n", x);
	if(++x==3)	x=0;
}

//=====================================
//从8位数据获得16位颜色
//mode:0,低位在前,高位在后.
//     1,高位在前,低位在后.
//str:数据
u16 image_getcolor(u8 mode,u8 *str)
{
	u16 color;
	if(mode)
	{
		color=((u16)*str++)<<8;
		color|=*str;
	}else
	{
		color=*str++;
		color|=((u16)*str)<<8;
	}
	return color;
}
//在液晶上画图（仅支持：从左到右，从上到下 or 从上到下，从左到右 的扫描方式！）
//xsta,ysta,width,height:画图区域
//scan:见image2lcd V2.9的说明.
// *p:图像数据
void image_show(u16 xsta,u16 ysta,u16 width,u16 height,u8 scan,u8 *p)
{
	u32 i;
	u32 len=0;

	LCD_Scan_Dir(R2L_D2U);//从上到下,从左到右
	LCD_Set_Window(xsta,ysta,width,height);
	LCD_SetCursor(xsta,ysta);//设置光标位置

	LCD_WriteRAM_Prepare();   	//开始写入GRAM
	len=width*height;			//写入的数据长度
	for(i=0;i<len;i++)
	{
		LCD_WriteRAM(image_getcolor(scan&(1<<4),p));//Bit4: 0:WORD类型高低位字节顺序与PC相同，1:WORD类型高低位字节顺序与PC相反。p:imgx+ifosize数组加上结构体大小
		p+=2;
	}
	LCD_Set_Window(0,0,lcddev.width,lcddev.height);
}

//在指定的位置显示一个图片
//此函数可以显示image2lcd软件生成的任意16位真彩色图片.
//限制:1,尺寸不能超过屏幕的区域.
//     2,生成数据时不能勾选:高位在前(MSB First)
//     3,必须包含图片信息头数据
//x,y:指定位置
//imgx:图片数据(必须包含图片信息头,"4096色/16位真彩色/18位真彩色/24位真彩色/32位真彩色”的图像数据头)
//注意:针对STM32,不能选择image2lcd的"高位在前(MSB First)"选项,否则imginfo的数据将不正确!!
void image_display(u16 x,u16 y,u8 * imgx)
{
	HEADCOLOR *imginfo;
 	u8 ifosize=sizeof(HEADCOLOR);//得到HEADCOLOR结构体的大小
	imginfo=(HEADCOLOR*)imgx;
//	USER_DBG_INFO("    imgx=%d",imgx);
	image_show(x,y,imginfo->w,imginfo->h,imginfo->scan,imgx+ifosize);
}

//=================================================
u16 DMAimage_getcolor(u8 mode,u8 *str)
{
	u16 color;
	if(mode)
	{
		color=*str++;
		color|=((u16)*str)<<8;
	}else
	{
		color=((u16)*str++)<<8;
		color|=*str;
	}
	return color;
}

void DMAimage_show(u16 xsta,u16 ysta,u16 width,u16 height,u8 scan,u8 *p)
{

	u32 i;
	u32 len=0;
	u16 color[2304];

	LCD_Scan_Dir(R2L_D2U);//从上到下,从左到右
	LCD_Set_Window(xsta,ysta,width,height);
	LCD_SetCursor(xsta,ysta);//设置光标位置

	LCD_WriteRAM_Prepare();   	//开始写入GRAM
	len=width*height;			//写入的数据长度
	for(i=0;i<len;i++)
	{
		color[i]=DMAimage_getcolor(scan&(1<<4),p);//Bit4: 0:WORD类型高低位字节顺序与PC相同，1:WORD类型高低位字节顺序与PC相反。p:imgx+ifosize数组加上结构体大小

		p+=2;
	}
#if CONFIG_USER_SPI_FUNC
	LCD_CD_H;

	user_spi1_transfer((u16*)color, (len*2), NULL, 0, 0);

	while(user_spi_is_busy());
#endif
	LCD_Set_Window(0,0,lcddev.width,lcddev.height);
}

void DMAimage_display(u16 x,u16 y,u8 * imgx)
{
	HEADCOLOR *imginfo;
 	u8 ifosize=sizeof(HEADCOLOR);//得到HEADCOLOR结构体的大小
	imginfo=(HEADCOLOR*)imgx;

	DMAimage_show(x,y,imginfo->w,imginfo->h,imginfo->scan,imgx+ifosize);
}

//*********************************************
void ShowBattAch(uint16_t a)//麦克风电池状态
{
	if(a<7)	a=7;
	if(a>100)	a=100;
	LCD_Color_Fill(93,53,14,6,BLACK);//电量
	LCD_Color_Fill(93,53,a/7,6,GREEN);//电量
	LCD_ShowxNum(115, 48, a, 3, 16, 0);
}
//******************************************
void ShowBattBch(uint16_t a)//麦克风电池状态
{
	if(a<7)	a=7;
	if(a>100)	a=100;
	LCD_Color_Fill(257,53,14,6,BLACK);	//电量
	LCD_Color_Fill(257,53,a/7,6,GREEN);	//电量
	LCD_ShowxNum(279, 48, a, 3, 16, 0);
}

//*****************************************
void ShowRssiAch(int a)
{
	static uint8_t aR=2;
	for(int i=0;i<5;i++){
		LCD_Color_Fill(82-i*4,50+i*2,2,12-i*2,GRAY);//信号
		if(a>60-i*15)	LCD_Color_Fill(82-i*4,50+i*2,2,12-i*2,GREEN);//信号
	}
	if(a){
		if(aR!=1){
			aR =1;
			LCD_PutString(10, 48, (u8*)"MicA:", 16, GREEN, BLACK);
		}
	}else{
		if(aR!=0){
			aR =0;
			LCD_PutString(10, 48, (u8*)"MicA:", 16, GRAY, BLACK);
			LCD_Color_Fill(93,53,14,6,BLACK);	//
			LCD_Color_Fill(115,48,24,32, BLACK);

		}
	}
}
//****************************************
void ShowRssiBch(int a)
{
	static uint8_t aR=2;
	for(int i=0;i<5;i++){
		LCD_Color_Fill(246-i*4,50+i*2,2,12-i*2,GRAY);//信号
		if(a>60-i*15)	LCD_Color_Fill(246-i*4,50+i*2,2,12-i*2,GREEN);//信号
	}
	if(a){
		if(aR!=1){
			aR =1;
			LCD_PutString(172, 48, (u8*)"MicB:", 16, GREEN, BLACK);
		}
	}else{
		if(aR!=0){
			aR =0;
			LCD_PutString(172, 48, (u8*)"MicB:", 16, GRAY, BLACK);
			LCD_Color_Fill(257,53,14,6,BLACK);
			LCD_Color_Fill(279,48,24,32, BLACK);
		}
	}
}

extern uint8_t MenuF;
//=============================================
uint8_t ST7789V_main(void)
{
 	static uint8_t LooCont=0;
 	if(LooCont==150){

 		return 0;
 	}else{
 		LooCont++;
 		//==========================
 		if(LooCont==1){
 			ST7789V_GpioInit();

 			//==========================
 		}else if(LooCont==2){
 			ST7789V_ExitSleep();

 			//=============================
 		}else if(LooCont==12){
 			//------------------------
 			ST7789V_Init();

 			DMAimage_display(15,5,(u8*)gImage_logo);//logo

 			DMAimage_display(10,42,(u8*)gImage_num10);//彩虹色
 			DMAimage_display(10,200,(u8*)gImage_num10);//彩虹色

 			//======================================================================
 			LCD_ShowxNum(47, 48, SysInf.MicCh_A+1, 2, 16, 0);	// A频道CH
 			LCD_DrawRectangle(90, 50,20,12, 2, GREEN);//电量横
 			LCD_DrawRectangle(110, 53,1,6, 2, GREEN);//电量横
 			LCD_PutString(139, 48, (u8*)"%", 16, POINT_COLOR, BACK_COLOR);
 			ShowRssiAch(0);

 			LCD_ShowxNum(211,  48, SysInf.MicCh_B+1, 2, 16, 0);	// B频道CH
 			LCD_DrawRectangle(254, 50, 20, 12,2, GREEN);//电量横
 			LCD_DrawRectangle(274, 53,1,6, 2, GREEN);//电量
 			LCD_PutString(305, 48, (u8*)"%", 16, POINT_COLOR, BACK_COLOR);
 			ShowRssiBch(0);

 			//======================================================================
 			LCD_PutString(225, 214, (u8*)"M0", 16, POINT_COLOR, BACK_COLOR);
 			DMAimage_display(250,214,(u8*)gImage_Bluto_icon);//蓝牙
 			LCD_PutString(270, 214, (u8*)"BT   ", 16, POINT_COLOR, BACK_COLOR);

 			LCD_PutString(216, 5, (u8*)"ALL IN ONE", 16, POINT_COLOR, BACK_COLOR);
 			LCD_PutString(200, 20, (u8*)"KARAOKE SYSTEM", 16, POINT_COLOR, BACK_COLOR);
 			//LCD_PutString(180, 50, (u8*)"MIC2", 14, POINT_COLOR, BACK_COLOR);
 			LCD_DrawLine(52, 180, 269, 180);//线
 			LCD_DrawLine(52, 180, 52, 175);//线
 			LCD_DrawLine(123, 180, 123, 175);//线
 			LCD_DrawLine(196, 180, 196, 175);//线
 			LCD_DrawLine(269, 180, 269, 175);//线

 			MenuF = 9;	DisplayMenuChg();
 			MenuF = 23;	DisplayMenuChg();
 			MenuF = 13;	DisplayMenuChg();
 			MenuF = 3;	DisplayMenuChg();

		#if defined(AMG8802)
 			for(int i=0;i<12;i++){
 			 	DMAimage_display(11+i*13,210,(u8*)gImage_pow_frame);//电量外框
 			}
 			LCD_DrawRectangle(10, 210,157,24, 1, GREEN);//总电量大框
 			LCD_PutString(187, 214, (u8*)"%", 16, POINT_COLOR, BACK_COLOR);
		#elif defined(TL8025)||defined(TL8035)||defined(TL820) || defined(TL8025v2)
 			LCD_PutString(60, 214, (u8*)"Batt:  . v    %", 16, POINT_COLOR, BACK_COLOR);
		#endif

 			ST7789V_BL_EN(1);
 		}
 		return 1;
 	}
}
#endif
