#ifndef __BK9532_H__
#define __BK9532_H__

#include "USER_Config.h"
#ifdef BK9532
#define BK95xx_DBG_INFO(fmt,...)     os_printf("[BK95Rx]"fmt, ##__VA_ARGS__)

#define	u8 		uint8_t
#define	u16		uint16_t
#define	u32 	uint32_t
#define	bit 	uint8_t

#if defined(BZQ703)
	#define PAIR_ID		0x7FFFFFFF		// 巴金0050
	#define PAIR_AFn	4
	#define PAIR_BFn	21
	#define WORK_ID		0x7FFFFFFF
	#define Afreq		670000
	#define Bfreq		682500
	#define freqStep	500
	#define	FreqNum	25
#endif

#if defined(ZY_OA_001)||defined(XLL_V48)||defined(MZ_200K)
	#define PAIR_ID		0x00000700
	#define PAIR_AFn	6
	#define PAIR_BFn	6
	#define WORK_ID		0x00000701		// ZY_OA
//	#define WORKID2		0x00000211		//
//	#define WORKID4		0x000025B2		// S8900
#ifdef ZY004
	#define Afreq	661800
	#define Bfreq	667800
#else
#if ZY_OA001_B
	#define Afreq	651300
	#define Bfreq	641300
#else
	#define Afreq	651800	//A通道啟始频点
	#define Bfreq	641800	//B通道啟始频点
#endif
#endif
	#define freqStep	1000
	#define	FreqNum	7
#endif

#if defined(GY_P10)
	#define PAIR_ID		0x00000310
	#define PAIR_AFn	1
	#define PAIR_BFn	10
	#define WORK_ID		0x00000311
	#define Afreq		638300
	#define Bfreq		650300
	#define freqStep	1000
	#define	FreqNum	12
#endif

#if defined(text1)||defined(HZ_P2)
	#define PAIR_ID		0x00000310
	#define PAIR_AFn	1
	#define PAIR_BFn	10
	#define WORK_ID		0x00000311
	#define Afreq		638300
	#define Bfreq		650300
	#define freqStep	1000
	#define	FreqNum	12
#endif
#if defined(XFN_S930)
	#define PAIR_ID		0x00000510
	#define PAIR_AFn	0
	#define PAIR_BFn	20
	#define WORK_ID		0x00000511
	#define Afreq		639000
	#define Bfreq		650000
	#define freqStep	500
	#define	FreqNum	22
//	#define PAIR_ID		0x00002200
//	#define WORK_ID		0x00002201
//	#define PAIR_AFn	47
//	#define PAIR_BFn	47
//
//	#define PAIR_AFreq	646000
//	#define PAIR_BFreq	676000
//
//	#define Afreq		635100
//	#define Bfreq		659100
//	#define freqStep	500
//	#define	FreqNum	48
#endif

#if defined(SG_P60)
	#define PAIR_ID		0x00000210		// 圣格P60
	#define PAIR_AFn	47
	#define PAIR_BFn	0
	#define WORK_ID		0x00000211
	#define Afreq		660500
	#define Bfreq		685000
	#define freqStep	500
	#define	FreqNum	49
#endif

//*******************************************************************
#define INIT_VOLUME     7				// 默认音量
#define INIT_ANTMODE    SA_ANT2_PIN5	// 默认ANT2	//双天线模式

#define CHIP_DEV_RX		0x26
#define RX_Chip_ID		0x32
#define RX_INIT_RETRY_TIMES		10	// 錯誤重做次數
#define RX_I2C_RETRY_TIMES		5	// 錯誤重做次數

typedef struct{
    u32 dis;	// 仅方便显示频点用
    u32 reg0xD;	// 0x0D寄存器设定值
    u32 reg0x39;// 0x39寄存器设定值
}t_FreqDef;

typedef enum{
    CHA = 0,
    CHB,
}e_ChannleDef;

extern e_ChannleDef  rWorkChannel;	// 当前工作频段
extern u8 rFreqIndex[2];         	// 当前频点索引
//extern t_FreqDef FreqTable[2][FreqNum];

//**** interrupt mask define ******************
#define RX_INTM_SYM_INT0                0x01
#define RX_INTM_SYM_INT1                0x02
#define RX_INTM_TRANS_PHASE_LOCK        0x04
#define RX_INTM_RESERVE                 0x08
#define RX_INTM_VOL_TRANS               0x10
#define TX_INTM_PLL_UNLOCK              0x40
#define TX_INTM_SYM_INT                 0x40

//**** interrupt status define *****************
#define RX_INTS_SYM_INT0                0x01
#define RX_INTS_SYM_INT1                0x02
#define RX_INTS_TRANS_PHASE_LOCK        0x04
#define RX_INTS_RESERVE                 0x08
#define RX_INTS_TRANS_RSSI              0x10
#define TX_INTS_PLL_UNLOCK              0x02
#define TX_INTS_SYM_INT                 0x01

//2019-04-19    Ôö¼ÓGPIO4×÷MUTEÊä³öÒý½Å
#define ENABLE_GPIO4_MUTE_FUN                       //GPIO4ÓÃÀ´×÷MUTEÊä³öÒý½Å
#define OUTPUT_HIG                      1           //Êä³ö¸ßµçÆ½
#define OUTPUT_LOW                      0           //Êä³öµÍµçÆ½
#define GPIO4_MUTE_STATUS               OUTPUT_LOW  //¾²ÒôÊ±£¬GPIO4µÄ×´Ì¬¡£
//2019-04-19


typedef enum{
    SA_ANT1_PIN6 = 0,                   //使用PIN6作为天线输入口    2020-05-26
    SA_ANT2_PIN5,                       //使用PIN5作为天线输入口    2020-05-26
    DA_AUTO,                            //自动天线模式
}e_AntennaType;

typedef enum{
    ADPCM_NORMAL = 0,                   //PLC常规处理
    ADPCM_ZERO,                         //PLC补0处理
}e_Adpcm_ErrDef;

//extern bit b1MS;                  // 1ms定时时间到
//extern t_FreqDef const FreqTableA[11];

bit Init_RX(void);
void Reset_chip_rx(void);
void RX_I2C_WriteReg(u8 reg,u32 dat);
u32 RX_I2C_ReadReg(u8 reg);
//void BK_Rx_BB_Reset(void);
//void BK_Rx_PLC_Reset(void);
//void Rx_Trigger(void);
void RX_TuneFreq(void);
void BK_Rx_Ant_mode(e_AntennaType mode);
void BK_Rx_ADPCM_ModeSel(e_Adpcm_ErrDef mode);
void BK_Rx_PLC_En(u8 status);
void BK_Rx_Reverb_En(u8 status);
//char BK_Rx_GetVolume(void);
void BK_Rx_SetVolume(u8 vol);
void BK_Rx_Mute_En(u8 status);
void BK_Rx_AFC_En(u8 status);
bit RX_I2C_Write(u8 reg,u8 *buf);
bit RX_I2C_Read(u8 reg,u8 *buf);

//bit RX_Pair(u32 *id);
void BK9532_Init(void);
void BK9532_Main(void);
void delay_nms(uint16_t val);
extern uint8_t ChipEn[2];
extern t_FreqDef FreqTable[2][FreqNum];

//***************************************
typedef enum{
    PCM_SLAVE = 0,
    PCM_MASTER,
}e_PcmMode;

typedef enum{
    PCM_SDA_O = 0,
    PCM_SDA_I = 4,
}e_PcmDataCfg;

typedef enum{
    PCM_SCK_O = 0,
    PCM_SCK_I = 4,
}e_PcmBclkCfg;

typedef enum{
    PCM_LRCK_O = 0,
    PCM_LRCK_I = 4,
}e_PcmLrckCfg;

typedef enum{           
    PCM_LEFT = 0,       //左声道（LRCK=0时输出音频数据）
    PCM_RIGHT,          //右声道（LRCK=1时输出音频数据）
    PCM_MONO,           //单声道（在LRCK高电平和低电平期间均输出音频数据）
}e_PcmChannelCfg;       //2020-05-27

typedef struct{
    e_PcmMode mode;
    e_PcmDataCfg dat;
    e_PcmBclkCfg bclk;
    e_PcmLrckCfg lrck;
    e_PcmChannelCfg ch; //2020-05-27
}t_PCMCfg;
void BK_Rx_I2SOpen(t_PCMCfg cfg);

#define RF_RSSI_TH_1	25
#define RF_RSSI_TH_2	30
#define RF_RSSI_TH_3	35
#define RF_RSSI_TH_4	40
#define RF_RSSI_TH_5	45
#define RF_RSSI_TH_6	50
#define RF_RSSI_TH_7	55
#define RF_RSSI_TH_8	60

#define AF_RSSI_TH_1	4  //10
#define AF_RSSI_TH_2	8  //17
#define AF_RSSI_TH_3	12 //24
#define AF_RSSI_TH_4	16 //31
#define AF_RSSI_TH_5	20 //38
#define AF_RSSI_TH_6	24 //45
#define AF_RSSI_TH_7	28 //52
#define AF_RSSI_TH_8	32 //59

#endif
#endif
