/*****************************************************************************
*文 件 名：TM1629-V1.0
*当前版本：V1.0
*完成日期：2022-11-11 Chen Yuan
*程序功能：1.LED 驱动;
          2.按键读程序驱动


********************************************************************************/
#include "USER_Config.h"

#if defined(XFN_S930) || defined(SC_D3)

#define TM_DBG_INFO(fmt,...)       os_printf("[TM16]"fmt, ##__VA_ARGS__)

//**** 定义控制端口 *********
#define TmDIO	GPIO17
#define TmCLK	GPIO16
uint8_t TmSTB =	GPIO3;//GPIO12

#define TmDIO_L	gpio_output(TmDIO, 0)
#define TmDIO_H	gpio_output(TmDIO, 1)
#define TmDIO_R	gpio_input(TmDIO)
#define TmCLK_L	gpio_output(TmCLK, 0)
#define TmCLK_H	gpio_output(TmCLK, 1)
#define TmSTB_L	gpio_output(TmSTB, 0)
#define TmSTB_H	gpio_output(TmSTB, 1)

//**** 定义数据 *********************
uint8_t TestN=0;

uint8_t DispBuff[16];
uint8_t* StbFg = DispBuff;

//**** 延时函数 *****
void NOP1()
{
}

/***************发送8bit数据，从低位开始**************/
void send_8bit(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++){
		TmCLK_L;
		if(dat&0x01)	TmDIO_H;
			else		TmDIO_L;
		NOP1();
		TmCLK_H;
		dat>>=1;
	}
	TmCLK_L;
	TmDIO_L;
}

/******************发送控制命令***********************/
void send_command(unsigned char word)
{
	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	TmSTB_H;
	NOP1();
	TmSTB_L;
	send_8bit(word);
}

//****  更新显示REG *****************************
void UpDisplayReg(uint8_t Addr, uint8_t OnOff)
{
	uint8_t Radd = (Addr>>4)&0xF;
	uint8_t Badd = Addr&0xF;
	Badd &= 0x07;
	if(OnOff)	StbFg[Radd] |= (1<<Badd);
		else	StbFg[Radd] &= (~(1<<Badd));
//	os_printf("= %d  %02X \n",Radd, StbFg[Radd]);
}
//**** 更新显示 ****************************
void UpDisplayAll()
{
//	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	TM16xx_Init(1);
//	for(uint8_t c=0; c<2; c++){
		send_command(0x40);		//设置数据命令:普通模式、地址自增1，写数据到显存
		send_command(0xc0);		//设置显示地址命令：从00H开始
		for(uint8_t i=0;i<16;i++)	send_8bit(StbFg[i]);	//发送16字节的显存数据

		send_command(0x8F);		//设置显示控制命令：打开显示，并设置为11/16. 亮度可以通过改变低三位调节
		TmSTB_H;
//	}
	gpio_config_new(TmDIO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);     //8.30 xfn——iic测试
//	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
}
//************************************
void UpDisplayOnOff(uint8_t WData)
{
//	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	TM16xx_Init(1);
//	for(uint8_t c=0; c<2; c++){
		send_command(0x40);		//设置数据命令:普通模式、地址自增1，写数据到显存
		send_command(0xc0);		//设置显示地址命令：从00H开始
		for(uint8_t i=0;i<16;i++)	send_8bit(WData);	//发送16字节的显存数据
		send_command(0x8F);		//设置显示控制命令：打开显示，并设置为11/16. 亮度可以通过改变低三位调节
		TmSTB_H;
//	}
	gpio_config_new(TmDIO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE); //8.30 xfn——iic测试
//	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
}

//**************************
void LED_TEST(uint8_t UpDn)
{
	uint8_t Reg_N = TestN/8;
	if(UpDn){
		StbFg[Reg_N] = 0;
		if(++TestN >= 128) TestN = 0;
	}else{
		StbFg[Reg_N] = 0;
		if(--TestN == 0) TestN = 127;
	}
	Reg_N = TestN/8;
	uint8_t Reg_Bit = TestN%8;

	StbFg[Reg_N] = 0;
	StbFg[Reg_N] |= (1<<Reg_Bit);

	TM_DBG_INFO("====ADD: 0x%02X   %02X\n",(Reg_N<<4)|Reg_Bit);
}
//**** 读取按键值 ****************
uint32_t read_key()
{
	uint32_t KeyN = 0;
//	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	TM16xx_Init(1);
	send_command(0x42);
	gpio_config_new(TmDIO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	for(uint8_t i=0; i<32; i++){
		TmCLK_L;
		if((i%8)<8) KeyN >>=1;
		TmCLK_H;
		if(TmDIO_R)	KeyN |= 0x80000000;
		NOP1();
	}
	gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	TmCLK_L;
	TmDIO_H;
	TmSTB_H;
	return KeyN;
}
//**************************************
void TM16xx_Init(uint8_t Step)
{
	if(Step==1){
		gpio_config_new(TmDIO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		TmDIO_L;
		gpio_config_new(TmCLK, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		TmCLK_L;
		gpio_config_new(TmSTB, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
		TmSTB_H;
//		UpDisplayOnOff(0xFF);
//		UpDisplayOnOff(0);
	}
 }

#endif



