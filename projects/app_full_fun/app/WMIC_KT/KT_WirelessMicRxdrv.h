///****************************************************************************
//  File Name: KT_WirelessMicRxdrv.h
//  Function:  KT Wireless Mic Receiver Products Driver For Customer
///****************************************************************************
//        Revision History
//  Version Date        Description
//  V0.1    2016-07-13  For KT0656M
//  V0.2    2017-02-10  规范化整理，修改了KT_WirelessMicRx_Init函数里面
//                      auto_mute control中移位的错误
//  V0.3    2017-02-17  添加KT_WirelessMicRx_CheckAUXCH(辅助信道状态变化监测程序)函数
//  V0.4    2017-03-27  在tune函数里面增加了一些操作解决温漂的问题
//  V1.3    2017-04-01  有天线分集的时候，显示现在是接收的主还是从，
//						根据当前是主路或者从路然后读取主或从的RSSI及SNR,pilot及BPSK
//						根据最新的命名规则进行了版本号修改
//  V1.4    2017-04-01  初始化中增加了修改0x1f和0x0d寄存器的值
//  V1.5    2017-05-24  初始化函数中，把comp_tc由原来的1改成了3，
//						把vtr_momitor_en配置为1使能，把ref_vtr_vth_sel配置为1
//  V1.6    2017-06-05  增加了KT_WirelessMicRx_Patch函数，用来修复一些bug
//						修改了一些宏定义的名称，增加了跟KT0616M一样的选择晶振程序
//  V1.7    2017-06-28  增加了AUTOMUTE_SNR_LOWTH和AUTOMUTE_SNR_HIGHTH宏定义
//						初始化里面把PLL_UNLOCK_EN=1注释掉，加了会有概率引起死机现象
//						在切换晶振的过程中，把ifadc也rst了
//						增加了测量电池电压函数KT_WirelessMicRx_BatteryMeter_Read
//  V1.8    2017-08-25  修改了AUTOMUTE_SNR_LOWTH和AUTOMUTE_SNR_HIGHTH的值
//						从原来的0x58和0x60改成了0x78和0x80,增加了搜台功能的宏定义
//						echo delay最多配置为23(197ms)
//  V1.9    2017-11-21  修改了KT_WirelessMicRx_SAIInit函数
//  V1.10   2017-12-25  在调用KT_WirelessMicRx_SAIInit函数的位置加入 #ifdef I2S_EN
//						配置I2S_Slave之前要先写i2s_slave_rst=1, 否则运行中修改I2S配置可能失败;
//						在AUTOMUTE_EN未定义时，把AutoMute关掉(AutoMute默认是开的);
//						I2S配置相关的宏定义加入 "KT_I2S_" 前缀, 防止"LEFT" "RIGHT"之类模糊的宏定义造成冲突.
//  V1.11   2018-03-22  根据芯片的版本来决定patch函数的调用与否
//  V1.12   2018-04-08  根据芯片的版本来决定是否在tune台的时候把rfamp_int_en写为0，a版本需要写为0，b版本不能写为0
//  V1.13   2018-05-03  根据芯片的版本来决定是否打开芯片内部的afc控制，a版本不打开内部的afc控制，b版本打开afc控制
//						在b版本中，把TUNE函数的i2s的初始化部分注释掉了，其实不应该注释。
//  V1.14   2018-06-27  KT_WirelessMicRx_GetFastRSSI读从机的FASTRSSI应该是0x0222寄存器，原来的写成了0x0221寄存器
//						把linein 输入输出的音量调一致，把KVCO coarse aim设置为120MHz/V，修复快速tune台可能出现的问题，见#12359
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 功能及参数定义
//-----------------------------------------------------------------------------
#ifndef __Kt_WirelessMicRxDrv_H
#define __Kt_WirelessMicRxDrv_H
#include "USER_Config.h"

#define KT0656M
//#define KT0655M

//#define XTAL_DUAL
#ifdef MZ_200K
	#define XTAL_24M_ONLY
#else
	#define XTAL_24P576M_ONLY
#endif

#ifdef	 XTAL_DUAL
	//#define OLD_SEL_XTAL_MODE 			  //KT0616M选择晶体的程序
	#define NEW_SEL_XTAL_MODE 				  //KT0656M选择晶体的程序
#endif

//#define AUTO_SEARCH_FREQ	 //自动搜台程序	 ??

#ifdef  XTAL_24P576M_ONLY
    #define XTAL_SEL1  1 //0:24MHz xtal 1:24.576MHz xtal
    #define XTAL_SEL2  0 //0:24MHz xtal 1:24.576MHz xtal
#else
    #define XTAL_SEL1  0 //0:24MHz xtal 1:24.576MHz xtal
    #define XTAL_SEL2  1 //0:24MHz xtal 1:24.576MHz xtal
#endif

#define XTAL_24M_FREQ     0
#define XTAL_24P576M_FREQ 1

#define HLSI_INV          0 //0: (HLSI); 1: inv(HLSI)

#define DCDC_EN           0 //DCDC使能位：0：关闭；1：使能。 yuan 1->0

#define AUTOMUTE_EN //undefine to disable automute
//#define SQUEAL_EN //undefine to disable squeal 啸叫

#define I2S_EN


#define MUTE              1
#define UNMUTE            0

#define FAST_RSSI_MUTE_EN       1     //1:enable 0:disable		??
#define FAST_RSSI_PD_MUTE_EN    1     //1:enable 0:disable
#define SUPER_RSSI_MUTE_EN      1     //1:enable 0:disable

#define AUTOMUTE_SNR_LOWTH	   0x68		//0x78	//??
#define AUTOMUTE_SNR_HIGHTH	   0x70		//0x80


//I2C地址的bit4根据芯片ADDR决定，高为1，低为0. I2C地址的bit3根据主还是从决定：主为1，从为0
#define KTWirelessMicRx_addressAM	0x31 	// addr pin = 0   //yuan 20220321 EDIT
#define KTWirelessMicRx_addressAS	0x35

#define KTWirelessMicRx_addressBM	0x39	// addr pin = 1
#define KTWirelessMicRx_addressBS	0x3D

//#define KTWirelessMicRxw_addressOTP 0x62
//#define KTWirelessMicRxr_addressOTP 0x63


#define INIT_FAIL_TH	3	//判断芯片是否正常上电次数


/*
//频点范围及步进

#define    BAND_TOP_CHA            510000	//727550//754850//662500
#define    BAND_BOTTOM_CHA         520000	//710450//740150//

#define    BAND_TOP_CHB            520000	//743750//769850//
#define    BAND_BOTTOM_CHB         530000	//728150//755150//

#define    BAND_STEP               500
*/

//电池电压检测
#define    BATTERY_MAX             0x7FF
#define    BATTERY_HIGHTH          0x500
#define    BATTERY_MIDDLETH        0x4C0
#define    BATTERY_LOWTH           0x4A0


#ifdef KT0655M
	#define LINEIN_AGC_DIS			0		//0：使用自动调整增益的功能 1：不使用自动调整功能
	#define LINEIN_LOCAL_VOL_CTRL	30
	#define COMPEN_GAIN				2		//0：补偿后总增益为0dB 1：补偿后总体增益为6dB 2：补偿后总体增益为12dB 3：补偿后总体增益为18dB
	#define PGA_GAIN_SEL			3		// 2'b00：-6dB 2'b01：0dB 2'b10：6dB	2'b11：12dB
	#define	SLNC_MUTE_TIME			0x13	//
	#define SLNC_MUTE_DIS			1		//0：使能Silence Mute功能 1：关闭Silence Mute功能。
	#define	SLNC_MUTE_LOW_LEVEL		0x04
	#define SLNC_MUTE_HIGH_LEVEL	0x06
	
	#define ALC_DIS					1		//0：开启 1：不开启。
	#define ALC_VMAX				0x7f	//
    #define ALC_SOFTKNEE			1		//
#endif

//REG0x0200
#define ADJUST_GAIN			4	//0:75KHz 1:50KHz 2:37.5KHz 3:25KHz 4:20KHz 5:120KHz 6:100KHz	 ??? //DCC303=1

//REG0x0241
#define BPSK_NEW_MODE		1	//0//1        //1:new mode  0:old mode
#define AUXDATA_EN			1	//

#define CARRY_NUM_TH		4	// 3'b000:10 3'b001:12  3'b010:14 3'b011:16 3'b100:18 3'b101:20  3'b110:22 3'b111:24
#define AUX_CARRY_NUM		3	//3'b000 32 3'b001 28	3'b010 24  3'b011 22 3'b100 20 3'b101 18  3'b110 16	 3'b111 12

//REG0x026F
#define SQUEAL_ELIM_EN		1	//0: disable;    1: ensable		bit5
#define SQEUAL_DET_EN		3	//0:关闭 1:保留 2:保留 3:开启	bit4:3
#define FNOTCH_NOTRESP_TH	1	//0: 1/4;    1: 1/2
#define N_OCTAVE			3	//0    //0: 1/5;    1: 1/10;    2: 1/20;    3:1/80

//REG0x0270
#define FFT_R_TH			15	//8// 0: 0;        15: 30
#define FRAME_NUM_TH		0	//4    //

//REG0x0271
#define    PMAX_HITH		14  //8
#define    PMAX_LOWTH		12  //6
#define    AFC_RNG			2	//3	   //2'b00: +/-20kHz;2'b01: +/-40kHz;2'b10: +/-60kHz;2'b11: +/-90kHz;

//REG0x0272
#define    FDIFF_HITH		7	//15
#define    FDIFF_LOWTH		1	//7

#define ECHO_EN		1
#define ECHO_DIS	0

#define ECHO_STRU		1    //0: 全通; 1: 梳状
#define ECHO_GAIN_DOWN	0    //0: -13dB; 1: -10dB; 2: -7dB;
#define ECHO_GAIN_UP	7    //0: 0dB; 1: 1.9dB; 2: 3.5dB; 3: 5.5dB; 4: 7dB; 5: 9.4dB; 6: 10.9dB; 7: 13.1dB;
#define ECHO_RATIO		10    //0~25: 0~25/32 ECHO反馈比例
#define ECHO_DELAY		23    //0~23:    22ms~197ms ECHO信号延时

#define EXCITER_EN		1
#define EXCITER_DIS		0

#define EXCITER_TUNE	2    //0: 600Hz; 1: 1KHz; 2: 2KHz; 3: 3.8KHz; 激励起始频率
#define EXCITER_DRIVE	0    //0: 0dB; 1: 3.5dB; 2: 6dB; 3: 9dB; 4: 12dB; 5: 15dB; 激励剩余增益
#define EXCITER_ODD		1    //0~6:    负无穷~0dB 奇次激励衰减量
#define EXCITER_EVEN	1    //0~6:    负无穷~0dB 偶次激励衰减量

#define EQ_EN           1
#define EQ_DIS          0

#define EQ_25Hz         0
#define EQ_40Hz         1
#define EQ_63Hz         2
#define EQ_100Hz        3
#define EQ_160Hz        4
#define EQ_250Hz        5
#define EQ_400Hz        6
#define EQ_630Hz        7
#define EQ_1KHz         8
#define EQ_1K6Hz        9
#define EQ_2K5Hz        10
#define EQ_4KHz         11
#define EQ_6K3Hz        12
#define EQ_10KHz        13
#define EQ_16KHz        14

#define EQ_Neg12dB       0
#define EQ_Neg11dB       1
#define EQ_Neg10dB       2
#define EQ_Neg9dB        3
#define EQ_Neg8dB        4
#define EQ_Neg7dB        5
#define EQ_Neg6dB        6
#define EQ_Neg5dB        7
#define EQ_Neg4dB        8
#define EQ_Neg3dB        9
#define EQ_Neg2dB        10
#define EQ_Neg1dB        11
#define EQ_Pos0dB        12
#define EQ_Pos1dB        13
#define EQ_Pos2dB        14
#define EQ_Pos3dB        15
#define EQ_Pos4dB        16
#define EQ_Pos5dB        17
#define EQ_Pos6dB        18
#define EQ_Pos7dB        19
#define EQ_Pos8dB        20
#define EQ_Pos9dB        21
#define EQ_Pos10dB       22
#define EQ_Pos11dB       23
#define EQ_Pos12dB       24

#define PRESET_VOL       30 //0: mute; 31: maximum volume 设定输出音量 //DCC303=30

#define  PKGSYNC         1
#define  NON_PKGSYNC     0

typedef struct
{
    int32_t Memery_Frequency;
    uint8_t echoFlag; 		//1:ON 0:OFF
    uint8_t equalierFlag;	//1:ON 0:OFF
    uint8_t exciterFlag;		//1:ON 0:OFF
    uint8_t diversityFlag;	//1:ON 0:OFF
    uint8_t pilotFlag; 		//1:ON 0:OFF
    uint8_t echoDepth;
    uint8_t echoDelay;
    uint8_t equalier16Sel[15];
    uint8_t exciterOdd;
    uint8_t exciterEven;
}soundEffect,*pSoundEffect;

#define KT_I2S_LRCLK48K		0
#define KT_I2S_LRCLK96K		1
#define KT_I2S_LRCLK192K	2
#define KT_I2S_MCLK24P576M	1
#define KT_I2S_MCLK12P288M	0
#define KT_I2S_MASTER		1
#define KT_I2S_SLAVE		0
#define KT_I2S_STRREO		1
#define KT_I2S_MONO			0
#define KT_I2S_LEFT			1
#define KT_I2S_RIGHT		0
#define KT_I2S_NOMODE  		0
#define KT_I2S_I2SMODE 		1
#define KT_I2S_LEFTMODE		2
#define KT_I2S_RIGHTMODE	3
#define KT_I2S_LENGHT16BIT	0
#define KT_I2S_LENGHT20BIT	1
#define KT_I2S_LENGHT24BIT	2
#define KT_I2S_LENGHT8BIT	3

typedef struct
{
	uint8_t LRCLK;
	uint8_t MCLK;
	uint8_t masterOrSlave;//1:master 0:slave
	uint8_t stereoOrMono;	//1:stereo 0:Mono
	uint8_t leftOrRight;	//1:left 0:right
	uint8_t mode;			//0:不能选 1:I2S 2:左对齐 3:右对齐
	uint8_t dataLength;	//0:16 1:20 2:24 3:8
}strI2s,*pStrI2s;

#if 0	//yuan20221230++
	#define BIT0 0x01
	#define BIT1 0x02
	#define BIT2 0x04
	#define BIT3 0x08
	#define BIT4 0x10
	#define BIT5 0x20
	#define BIT6 0x40
	#define BIT7 0x80
#endif

#define channelA 0
#define channelB 1

#define chipAM 0
#define chipAS 1
#define chipBM 2
#define chipBS 3


extern uint8_t chipSel;			//芯片选择指示，表示当前选择的哪颗芯片
extern uint8_t Flag_AUTOMUTE[4];	//MUTE状态标识
extern uint8_t Flag_PKGSYNC[4];	//包同步状态标识
extern uint8_t Flag_PILOT[4];	//导频状态标识


extern pSoundEffect pChangeSound;
extern soundEffect soundA;
extern strI2s chipAI2sConfig;

extern uint8_t MorSSelect;
extern uint8_t Chip_E[4];

extern soundEffect soundB;
extern strI2s chipBI2sConfig;

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void KT_Bus_Write(uint16_t Register_Address, uint8_t Byte_Data,uint8_t chipNum);
uint8_t KT_Bus_Read(uint16_t Register_Address,uint8_t chipNum);

BOOL KT_WirelessMicRx_PreInit(void);
void KT_WirelessMicRx_Init(void);
void KT_WirelessMicRx_Tune(long Freq); //in KHz
BOOL KT_WirelessMicRx_Volume(uint8_t cVolume);
void KT_WirelessMicRx_SW_Echo(uint8_t cEcho_En);
void KT_WirelessMicRx_SetEcho(uint8_t cEcho_Ratio, uint8_t cEcho_Delay);
void KT_WirelessMicRx_SW_Exciter(uint8_t cExciter_En);
void KT_WirelessMicRx_SetExciter(uint8_t cExciter_Odd, uint8_t cExciter_Even);
void KT_WirelessMicRx_SW_Equalizer(uint8_t cEqualizer_En);
void KT_WirelessMicRx_SetEqualizer(uint8_t cEqualizer_Frq, uint8_t cEqualizer_Gain);
void KT_WirelessMicRx_SW_Diversity(uint8_t diversity_En);
uint8_t KT_WirelessMicRx_GetAF(void);

uint8_t KT_WirelessMicRx_GetRSSI(void);

uint8_t KT_WirelessMicRx_GetFastRSSI(void);

uint8_t KT_WirelessMicRx_Automute(void);
//void KT_WirelessMicRx_CheckAUXCH(void);
//uint8_t KT_WirelessMicRx_CheckPilot(void);
uint8_t KT_WirelessMicRx_GetSNR(void);
void KT_WirelessMicRx_SelectMS(void);
void KT_WirelessMicRx_SAIInit(pStrI2s i2sConfigTemp);

void KT_WirelessMicRx_FastTune(long Freq);

void KT_WirelessMicRx_Patch(void);

uint16_t KT_WirelessMicRx_BatteryMeter_Read(void);


void Pll_Band_Cali(uint8_t CLl, uint8_t CLh);
void PLL_Reset(void);
//static void oldCaclXtal(int32 Freq);
//static void caclXtal(int Freq);
//static void  pilotMuteRefresh(void);
//void rfIntCtl(void);
//static void selXtal(bit xtalSel);
//static void  snrMuteRefresh(void);

extern uint8_t RX_COMM_DATA[5][2];
extern uint8_t RX_COMM_T,RX_COMM_B;

void KT56_Init (void);
void KT56_main(void);
void Set_CH(uint8_t val);
uint32_t freqSearch(uint32_t startFreq,uint32_t stopFreq, uint8_t chip);
#if 0
	#define    	Freq_Top	510000
	#define    	Freq_Step	500
	#define    	S_CHA		1
	#define    	S_CHB		20
#else
	#define		Freq_Top	510000
	#define    	Freq_Step	500
	#define    	S_CHA		1
	#define    	S_CHB		40
#endif
#endif

