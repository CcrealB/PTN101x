
#include "USER_Config.h"

#ifdef PCM1865

/*******************************************************
 *  I2C Address 0x4B,
	13.1寄存器映射描述
	寄存器映射是配置PCM186x软件控制设备的主要方式。寄存器映射为分为四页：0、1、3和253。
	页面0处理所有设备配置。							0x01~0x78
	第1页用于将系数间接编程到PCM186x上的两个固定功能DSP中。	0x01~0x0B
	第3页和第253页包含用于较低功耗的附加寄存器。			Page3:0x12,0x15 RSV; Page253:0x14
	所有未记录的登记簿都被视为保留；不要写入。
	通过用所需页面写入寄存器0x00来更改页面。
	通过将0xFE写入寄存器0x00来重置寄存器。
***********************************************************/
//*******************************
void PCM186x_delay_ms(uint16_t val)
{
	for(uint16_t i=0; i<val; i++){
		os_delay_us(1600);
	}
}

void PCM1865_Init()
{
#if 0
	I2C_WriteA8D8(0x4B, 0, 0);
	I2C_WriteA8D8(0x4B,0x01,0x00);
	I2C_WriteA8D8(0x4B,0x02,0x00);
	I2C_WriteA8D8(0x4B,0x03,0x00);
	I2C_WriteA8D8(0x4B,0x04,0x00);
	I2C_WriteA8D8(0x4B,0x05,0x86);
	I2C_WriteA8D8(0x4B,0x06,0x41);
	I2C_WriteA8D8(0x4B,0x07,0x41);
	I2C_WriteA8D8(0x4B,0x08,0x42);
	I2C_WriteA8D8(0x4B,0x09,0x42);
	I2C_WriteA8D8(0x4B,0x0A,0x00);
	I2C_WriteA8D8(0x4B,0x0B,0x44);
	I2C_WriteA8D8(0x4B,0x0C,0x00);
	I2C_WriteA8D8(0x4B,0x0D,0x00);
	I2C_WriteA8D8(0x4B,0x0E,0x00);
	I2C_WriteA8D8(0x4B,0x0F,0x00);
	I2C_WriteA8D8(0x4B,0x10,0x01);
	I2C_WriteA8D8(0x4B,0x11,0x20);
	I2C_WriteA8D8(0x4B,0x12,0x00);
	I2C_WriteA8D8(0x4B,0x13,0x00);
	I2C_WriteA8D8(0x4B,0x14,0x00);
	I2C_WriteA8D8(0x4B,0x15,0x00);
	I2C_WriteA8D8(0x4B,0x16,0x00);
	I2C_WriteA8D8(0x4B,0x17,0x00);
	I2C_WriteA8D8(0x4B,0x18,0x00);
	I2C_WriteA8D8(0x4B,0x19,0x00);
	I2C_WriteA8D8(0x4B,0x1A,0x00);
	I2C_WriteA8D8(0x4B,0x1B,0x00);
	I2C_WriteA8D8(0x4B,0x1C,0x00);
	I2C_WriteA8D8(0x4B,0x1D,0x00);
	I2C_WriteA8D8(0x4B,0x1E,0x00);
	I2C_WriteA8D8(0x4B,0x1F,0x00);
	I2C_WriteA8D8(0x4B,0x20,0x01);
	I2C_WriteA8D8(0x4B,0x21,0x00);
	I2C_WriteA8D8(0x4B,0x22,0x00);
	I2C_WriteA8D8(0x4B,0x23,0x01);
	I2C_WriteA8D8(0x4B,0x24,0x50);
	I2C_WriteA8D8(0x4B,0x25,0x07);
	I2C_WriteA8D8(0x4B,0x26,0x03);
	I2C_WriteA8D8(0x4B,0x27,0x3F);
	I2C_WriteA8D8(0x4B,0x28,0x00);
	I2C_WriteA8D8(0x4B,0x29,0x00);
	I2C_WriteA8D8(0x4B,0x2A,0x01);
	I2C_WriteA8D8(0x4B,0x2B,0x08);
	I2C_WriteA8D8(0x4B,0x2C,0x00);
	I2C_WriteA8D8(0x4B,0x2D,0x00);
	I2C_WriteA8D8(0x4B,0x2E,0x00);
	I2C_WriteA8D8(0x4B,0x2F,0x00);
	I2C_WriteA8D8(0x4B,0x30,0x00);
	I2C_WriteA8D8(0x4B,0x31,0x00);
	I2C_WriteA8D8(0x4B,0x32,0x00);
	I2C_WriteA8D8(0x4B,0x33,0x01);
	I2C_WriteA8D8(0x4B,0x34,0x00);
	I2C_WriteA8D8(0x4B,0x35,0x00);
	I2C_WriteA8D8(0x4B,0x36,0x01);
	I2C_WriteA8D8(0x4B,0x37,0x00);
	I2C_WriteA8D8(0x4B,0x38,0x00);
	I2C_WriteA8D8(0x4B,0x39,0x00);
	I2C_WriteA8D8(0x4B,0x3A,0x00);
	I2C_WriteA8D8(0x4B,0x3B,0x00);
	I2C_WriteA8D8(0x4B,0x3C,0x00);
	I2C_WriteA8D8(0x4B,0x3D,0x00);
	I2C_WriteA8D8(0x4B,0x3E,0x00);
	I2C_WriteA8D8(0x4B,0x3F,0x00);
	I2C_WriteA8D8(0x4B,0x40,0x80);
	I2C_WriteA8D8(0x4B,0x41,0x7F);
	I2C_WriteA8D8(0x4B,0x42,0x00);
	I2C_WriteA8D8(0x4B,0x43,0x80);
	I2C_WriteA8D8(0x4B,0x44,0x7F);
	I2C_WriteA8D8(0x4B,0x45,0x00);
	I2C_WriteA8D8(0x4B,0x46,0x80);
	I2C_WriteA8D8(0x4B,0x47,0x7F);
	I2C_WriteA8D8(0x4B,0x48,0x00);
	I2C_WriteA8D8(0x4B,0x49,0x80);
	I2C_WriteA8D8(0x4B,0x4A,0x7F);
	I2C_WriteA8D8(0x4B,0x4B,0x00);
	I2C_WriteA8D8(0x4B,0x4C,0x80);
	I2C_WriteA8D8(0x4B,0x4D,0x7F);
	I2C_WriteA8D8(0x4B,0x4E,0x00);
	I2C_WriteA8D8(0x4B,0x4F,0x80);
	I2C_WriteA8D8(0x4B,0x50,0x7F);
	I2C_WriteA8D8(0x4B,0x51,0x00);
	I2C_WriteA8D8(0x4B,0x52,0x80);
	I2C_WriteA8D8(0x4B,0x53,0x7F);
	I2C_WriteA8D8(0x4B,0x54,0x00);
	I2C_WriteA8D8(0x4B,0x55,0x80);
	I2C_WriteA8D8(0x4B,0x56,0x7F);
	I2C_WriteA8D8(0x4B,0x57,0x00);
	I2C_WriteA8D8(0x4B,0x58,0x00);
	I2C_WriteA8D8(0x4B,0x59,0x00);
	I2C_WriteA8D8(0x4B,0x5A,0x00);
	I2C_WriteA8D8(0x4B,0x5B,0x00);
	I2C_WriteA8D8(0x4B,0x5C,0x00);
	I2C_WriteA8D8(0x4B,0x5D,0x00);
	I2C_WriteA8D8(0x4B,0x5E,0x00);
	I2C_WriteA8D8(0x4B,0x5F,0x00);
	I2C_WriteA8D8(0x4B,0x60,0x01);
	I2C_WriteA8D8(0x4B,0x61,0x00);
	I2C_WriteA8D8(0x4B,0x62,0x10);
	I2C_WriteA8D8(0x4B,0x63,0x00);
	I2C_WriteA8D8(0x4B,0x64,0x00);
	I2C_WriteA8D8(0x4B,0x65,0x00);
	I2C_WriteA8D8(0x4B,0x66,0x00);
	I2C_WriteA8D8(0x4B,0x67,0x00);
	I2C_WriteA8D8(0x4B,0x68,0x00);
	I2C_WriteA8D8(0x4B,0x69,0x00);
	I2C_WriteA8D8(0x4B,0x6A,0x00);
	I2C_WriteA8D8(0x4B,0x6B,0x00);
	I2C_WriteA8D8(0x4B,0x6C,0x00);
	I2C_WriteA8D8(0x4B,0x6D,0x00);
	I2C_WriteA8D8(0x4B,0x6E,0x00);
	I2C_WriteA8D8(0x4B,0x6F,0x00);
	I2C_WriteA8D8(0x4B,0x70,0x70);
	I2C_WriteA8D8(0x4B,0x71,0x10);
	I2C_WriteA8D8(0x4B,0x72,0x00);
	I2C_WriteA8D8(0x4B,0x73,0x07);
	I2C_WriteA8D8(0x4B,0x74,0x07);
	I2C_WriteA8D8(0x4B,0x75,0x67);
	I2C_WriteA8D8(0x4B,0x76,0x11);
	I2C_WriteA8D8(0x4B,0x77,0xC4);
	I2C_WriteA8D8(0x4B,0x78,0x07);

	I2C_WriteA8D8(0x4B, 0, 1);
	I2C_WriteA8D8(0x4B,0x01,0x00);
	I2C_WriteA8D8(0x4B,0x02,0x00);
	I2C_WriteA8D8(0x4B,0x03,0x00);
	I2C_WriteA8D8(0x4B,0x04,0x00);
	I2C_WriteA8D8(0x4B,0x05,0x00);
	I2C_WriteA8D8(0x4B,0x06,0x00);
	I2C_WriteA8D8(0x4B,0x07,0x00);
	I2C_WriteA8D8(0x4B,0x08,0x00);
	I2C_WriteA8D8(0x4B,0x09,0x00);
	I2C_WriteA8D8(0x4B,0x0A,0x00);
	I2C_WriteA8D8(0x4B,0x0B,0x00);

	I2C_WriteA8D8(0x4B, 0, 3);
	I2C_WriteA8D8(0x4B,0x12,0x40);
	I2C_WriteA8D8(0x4B,0x15,0x01);

	I2C_WriteA8D8(0x4B, 0, 253);
	I2C_WriteA8D8(0x4B,0x14,0x00);
#endif

//	I2C_WriteA8D8(0x4B,0x0B,0x03);	//接收 PCM 字长- 32位；TDM_LRCK_MODE - LRCK 的占空比为1/50；立体声 PCM 字长- 32位；串行音频接口格式- TDM/DSP (需要256F BCK)
//	I2C_WriteA8D8(0x4B,0x0C,0x01);	//选择 TDM 传输数据。 01：4通道 TDM
//	I2C_WriteA8D8(0x4B,0x0D,0x80);	// TX_TDM_OFFSET[7：0]- 0x80 bck -只适用于从芯片
//	I2C_WriteA8D8(0x4B,0x0E,0x80);	// RX_TDM_OFFSET[7：0]- 0x80 bck  -只适用于从芯片
//	I2C_WriteA8D8(0x4B,0x20,0x42);	// PLL (与 bck PLL 模式一样)
//	I2C_WriteA8D8(0x4B,0x21,0x0B);	// 0x03 DSP1时钟分频器值3：1/4
//	I2C_WriteA8D8(0x4B,0x22,0x17);	// DSP2时钟分频器值7：1/24 -
//	I2C_WriteA8D8(0x4B,0x23,0x2F);	// ADC 时钟分频器值15：1/48 -
//	I2C_WriteA8D8(0x4B,0x28,0x03);	//启用 PLL、PLL 参考时钟选择- SCK -
//	I2C_WriteA8D8(0x4B,0x29,0x01);	// PLL P-Divider 值-
//	I2C_WriteA8D8(0x4B,0x2A,0x01);	// PLL R-Divider 值-
//	I2C_WriteA8D8(0x4B,0x2B,0x17);	// PLL J.D-Divider 值的整数部分-
//	I2C_WriteA8D8(0x4B,0x2C,0x00);	// PLL J.D-Divider 值的分数部分。 (最低有效位)
//	I2C_WriteA8D8(0x4B,0x2D,0x00);	// PLL J.D-Divider 值的分数部分。 (最高有效位、[13：8])

	I2C_WriteA8D8(0x4B,0x00,0xff);	//Reset
	PCM186x_delay_ms(100);

	I2C_WriteA8D8(0x4B,0x00,0x00);	//Page 0
	PCM186x_delay_ms(20);

	I2C_WriteA8D8(0x4B,0x01,36); //话筒1输入增益,18dB, 0.5dB/Step
	I2C_WriteA8D8(0x4B,0x02,36); //话筒2输入增益
	I2C_WriteA8D8(0x4B,0x03,36); //话筒3输入增益
	I2C_WriteA8D8(0x4B,0x04,36); //话筒4输入增益

	I2C_WriteA8D8(0x4B,0x06,0x50); //ADC1_L = {VIN1P,VIN1M}[DIFF]
	I2C_WriteA8D8(0x4B,0x07,0x50); //ADC1_R = {VIN2P,VIN2M}[DIFF]
	I2C_WriteA8D8(0x4B,0x08,0x60); //ADC2_L = {VIN4P,VIN4M}[DIFF] //ADC2的L通道只能选4
	I2C_WriteA8D8(0x4B,0x09,0x60); //ADC2_R = {VIN3P,VIN3M}[DIFF] //ADC2的R通道只能选3

	I2C_WriteA8D8(0x4B,0x10,0x00); //GPIO0=GPIO0; GPIO1=GPIO1
	I2C_WriteA8D8(0x4B,0x11,0x50); //GPIO2=GPIO2; GPIO3=DOUT2
	I2C_WriteA8D8(0x4B,0x13,0x40); //GPIO3=Output

	I2C_WriteA8D8(0x4B,0x19,0x00);	//PGA Control Mapping 疑问

	I2C_WriteA8D8(0x4B,0x20,0x41);	//这里如果选晶振做时钟，高频会很抖动


	I2C_WriteA8D8(0x4B, 0, 0);
	for(uint8_t i=0x01; i<=0x78; i++){
		USER_DBG_INFO("====PCM1865 P0 REG:0x%02X   %02X\n",i, I2C_ReadA8D8(0x4B, i));
	}
	I2C_WriteA8D8(0x4B, 0, 1);
	for(uint8_t i=0x01; i<=0x0B; i++){
		USER_DBG_INFO("====PCM1865 P1 REG:0x%02X   %02X\n",i, I2C_ReadA8D8(0x4B, i));
	}
	I2C_WriteA8D8(0x4B, 0, 3);
	USER_DBG_INFO("====PCM1865 P3 REG:0x12   %02X\n", I2C_ReadA8D8(0x4B, 0x12));
	USER_DBG_INFO("====PCM1865 P3 REG:0x15   %02X\n", I2C_ReadA8D8(0x4B, 0x15));

	I2C_WriteA8D8(0x4B, 0, 253);
	USER_DBG_INFO("====PCM1865 P253 REG:0x14   %02X\n", I2C_ReadA8D8(0x4B, 0x14));

	I2C_WriteA8D8(0x4B, 0, 0);

}



#endif
