
/****************************************************
 * ACM86xx driver
****************************************************/
#include "USER_Config.h"
#ifdef TAS5760M

#define u8	uint8_t
#define u16	uint16_t

#define TAS_DBG_INFO(fmt,...)      os_printf("[ACM]"fmt, ##__VA_ARGS__)


/*******************************************************
 * os_delay_ms... refer to your own os driver implement
 *******************************************************/
void Tas5760M_init()
{
#if 1
	u8 reg_data = 0;
	#define TWO_PBTL_OUT	// 两个功放都是做PBTL输出

#define TAS5760M_IIC_ADDR	0x6D//0xd8
#define TAS5760M_IIC_ADDR2	0x6C//0xd8


	#define AMP_STB  	GPIO14
	gpio_config_new(AMP_STB, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(AMP_STB, 0);
	os_delay_us(100);
#ifdef TWO_PBTL_OUT
	TAS_DBG_INFO("== TAS5760M_IIC_ADDR  START ...\n");
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x01,0xFD);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x02,0x14);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x03,0x80);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x04,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x05,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x06,0xD7);	// PBTL L_CH
	TAS_DBG_INFO("== TAS5760M_IIC_ADDR  END ...\n");

	TAS_DBG_INFO("== TAS5760M_IIC_ADDR2  START ...\n");
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x01,0xFD);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x02,0x14);	// 0x14
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x03,0x80);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x04,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x05,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x06,0xD5);		// PBTL R_CH
	TAS_DBG_INFO("== TAS5760M_IIC_ADDR2  END ...\n");
	TAS_DBG_INFO("== TAS5760M_IIC_ADDR2  reg_data =====================  %02X ...\n",reg_data);
	reg_data = I2C_ReadA8D8(TAS5760M_IIC_ADDR2, 0x06);
	TAS_DBG_INFO("== TAS5760M_IIC_ADDR2  REG06 ========================  %02X ...\n",reg_data);

#else
	ACM_DBG_INFO("== TAS5760M_IIC_ADDR  START ...\n");
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x01,0xFD);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x02,0x14);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x03,0x80);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x04,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x05,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR, 0x06,0x55);	// 0x51 // BTL
	ACM_DBG_INFO("== TAS5760M_IIC_ADDR  END ...\n");

	ACM_DBG_INFO("== TAS5760M_IIC_ADDR2  START ...\n");
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x01,0xFD);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x02,0x14);	// 0x14
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x03,0x80);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x04,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x05,0xCF);
	I2C_WriteA8D8(TAS5760M_IIC_ADDR2, 0x06,0xD5);		// 0xD1 // PBTL
	ACM_DBG_INFO("== TAS5760M_IIC_ADDR2  END ...\n");	
	ACM_DBG_INFO("== TAS5760M_IIC_ADDR2  reg_data =====================  %02X ...\n",reg_data);
	reg_data = I2C_ReadA8D8(TAS5760M_IIC_ADDR2, 0x06);
	ACM_DBG_INFO("== TAS5760M_IIC_ADDR2  REG06 ========================  %02X ...\n",reg_data);
#endif
	os_delay_us(100);
	gpio_output(AMP_STB, 1);

#endif

}

#endif


        
