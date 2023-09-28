
//#include "USER_Config.h"

#include ".\WMIC_BK\BK9532.h"

#ifdef BK9532

u8 reg_val[4];
u8 analog_reg_val[16][4];

u8 rFreqIndex[2];         		// 当前频点索引
e_ChannleDef  rWorkChannel;		// 当前工作频段

//***********************************************************
u8 const rx_reg_val[54][4] =
{
    {0xDF, 0xFF, 0xFF, 0xF8},	//00 REG0
    {0x04, 0xD2, 0x80, 0x57},	//02 REG1,0x52 to 0xD2,140515
    {0x89, 0x90, 0xE0, 0x28},	//04 REG2,
    {0x24, 0x52, 0x06, 0x9F},	//06 REG3,
    {0x52, 0x88, 0x00, 0x44},	//08 REG4,
    {0x00, 0x28, 0x03, 0x80},	//0A REG5,
    {0x5B, 0xED, 0xFB, 0x00},	//0C REG6,
    {0x1C, 0x2E, 0xC5, 0xAA},	//0E REG7, UBAND,150917;
    {0xEF, 0xF1, 0x19, 0x4C},	//10 REG8, UBAND,150917
    {0x88, 0x51, 0x13, 0xA2},	//12 REG9, update REG9[7]=0-->1,140414;
    {0x00, 0x6F, 0x00, 0x6F},	//14 REGA
    {0x1B, 0xD2, 0x58, 0x63},	//16 REGB
    {0x00, 0x00, 0x00, 0x08},	//18 REGC,
    {0x3A, 0x9B, 0x69, 0xD0},	//1A REGD,          13

    {0x00, 0x00, 0x00, 0x00}, //58 REG2C, 混响    14
    {0x00, 0x00, 0x00, 0xff}, //5A REG2D, 混响
    {0xf1, 0x28, 0xa0, 0x00}, //5C REG2E, 混响
    {0x00, 0x00, 0x2e, 0x91}, //5E REF2F, 混响    17

    {0x40, 0x40, 0x40, 0x40}, //60 REG30, 使能GPIO3,GPIO2,GPIO1,GPIO0第二功能
    
    //2019-04-19 增加GPIO4作为MUTE输出
#ifdef  ENABLE_GPIO4_MUTE_FUN
    {0xC1, 0x08, 0x00, (GPIO4_MUTE_STATUS<<1)},   //GPIO4为输出状态并保持静音状态下的电平。
#else
    {0xC1, 0x08, 0x00, 0x71}, //62 REG31, 开GPIO3帧错误功能 GPIO0,GPIO1,GPIO2为I2S数字接口
    //0xC1, 0x00, 0x00, 0x71, //62 REG31, GPIO3RSSI_MIC GPIO0,GPIO1,GPIO2为I2S数字接口
#endif
    //2019-04-19

    {0x20, 0xFF, 0x0F, 0x09}, //64 REG32, 2dB
    
    {0x00, 0x90, 0x00, 0x80}, //66 REG33, 音频RSSI门槛设定。
    {0xFF, 0xFF, 0x01, 0x0E}, //68 REG34, 被动啸叫
    {0x09, 0x00, 0x00, 0x00}, //6A REG35, 被动啸叫
    {0x0C, 0x60, 0x60, 0xDD}, //6C REG36, 关混响 I2S使能，MSB在前，主模式。
//
    {0x3e, 0x00, 0x98, 0x00}, //6E REG37,
    {0x40, 0xD7, 0xD5, 0xF7}, //70 REG38,
    {0x00, 0x00, 0x00, 0x00}, //72 REG39,
    {0x28, 0x02, 0x05, 0x64}, //74 REG3A
    {0x6D, 0x00, 0x08, 0x00}, //76 REG3B, 开PLC
    {0x00, 0x40, 0xFF, 0xDC}, //78 REG3C,
    {0x00, 0x00, 0x66, 0x29},	//7A REG3D,
    
    {0x1F, 0x55, 0x4F, 0xEE}, //7C REG3E,
    {0x8D, 0x7A, 0x00, 0x2F}, //7E REG3F,         33
//
	{0x43, 0x00, 0x00, 0x00}, //B2 REG59, 混响    34
	{0x00, 0x00, 0x00, 0x00}, //B4 REG5A,
	{0x00, 0x00, 0x00, 0x00}, //B6 REG5B,
	{0x2c, 0xd5, 0x00, 0x00}, //B8 REG5C, 混响
	{0x1F, 0xFF, 0x3F, 0xFF}, //BA REG5D，
	{0x00, 0x00, 0x0F, 0x00}, //BC REG5E,         39
//
    {0x00, 0x08, 0x95, 0x32}, //E0 REG70,         40
    {0x18, 0xA4, 0x08, 0x10}, //E2 REG71,
    {0x00, 0x00, 0x00, 0x00}, //E4 REG72,
    {0x00, 0x00, 0x00, 0x08}, //E6 REG73,
    {0x00, 0x00, 0x00, 0x00}, //E8 REG74,
    {0x00, 0x00, 0x06, 0x29}, //EA REG75,
    {0x00, 0x00, 0xFB, 0x06}, //EC REG76,
    {0x00, 0x00, 0x00, 0x00}, //EE REG77,
    {0x00, 0x00, 0x00, 0x00}, //F0 REG78,
    {0x00, 0x00, 0x00, 0x00}, //F2 REG79,
    {0x00, 0x01, 0x00, 0x01}, //F4 REG7A,
    {0x3B, 0xE4, 0x07, 0x96}, //F6 REG7B,
    {0x0F, 0x86, 0x00, 0x74}, //F8 REG7C,
    {0x00, 0x32, 0xA8, 0xFF}, //FA REG7D,         53
};

//yuan++*******************************
void delay_nms(uint16_t val)
{
	for(uint16_t i=0; i<val; i++){
		os_delay_us(1600);
	}
}

//*******************************************************************
bit Init_RX(void)
{
	u8 i;
	u8 addr;
// 	delay_nms(200);         //延迟跳过BK9532通过I2C总线读取外挂EEPROM的时间。
	for(i=0;i<=RX_INIT_RETRY_TIMES;i++){
		RX_I2C_Read(0x70,reg_val);
		if(reg_val[3] != RX_Chip_ID) continue;
		break;
	}	
	if(reg_val[3] != RX_Chip_ID){
		BK95xx_DBG_INFO("RX_Chip_ID:%02X %02X  %02X...Err\n",reg_val[2],reg_val[3],RX_Chip_ID);
		return FALSE;
	}

	BK95xx_DBG_INFO("RX_Chip_ID:%02X %02X  %02X...Ok\n",reg_val[2],reg_val[3],RX_Chip_ID);
    for(i=0;i<54;i++){
    	if(i <= 13)         //REG0x0~0x0D
            addr = i;
        else if( i <= 17)   //REG2C~REG2F
            addr = (0x2C - 14) + i;
        else if( i <= 33    )//0x30~0x3F
            addr = (0x30 - 18) + i;
        else if( i <= 39)   //REG59~REG5E
            addr = (0x59 - 34) + i;
        else if( i <= 53)   //REG0x70~0x7F
            addr = (0x70 - 40) + i;  

        RX_I2C_Write(addr, (u8*)rx_reg_val[i]); //rx_reg_val=rx_48k_uband values
    //    RX_I2C_Read(addr,reg_val);
    //    BK95xx_DBG_INFO("Read_Reg %02X: %02X %02X %02X %02X\n",addr,reg_val[0],reg_val[1],reg_val[2],reg_val[3]);
    }
    delay_nms(100);
    Reset_chip_rx();
    return TRUE;
}

//Reset chip state machine
void Reset_chip_rx(void)
{
    RX_I2C_Read(0x3F,reg_val);
    reg_val[3] &= ~0x20;//
    RX_I2C_Write(0x3F,reg_val);
    reg_val[3] |= 0x20;//
    RX_I2C_Write(0x3F,reg_val);
}


void RX_I2C_WriteReg(u8 reg,u32 dat)
{
	reg_val[3] = (dat&0xff);
	dat >>= 8;
	reg_val[2] = (dat&0xff);
	dat >>= 8;	
	reg_val[1] = (dat&0xff);
	dat >>= 8;	
	reg_val[0] = (dat&0xff);
	RX_I2C_Write(reg,reg_val);
}

//*****************************************************
u32 RX_I2C_ReadReg(u8 reg)
{
    u32 ret;
    RX_I2C_Read(reg,reg_val);
    ret  = reg_val[0];          
    ret <<= 8;
    ret |= reg_val[1];
    ret <<= 8;
    ret |= reg_val[2];
    ret <<= 8;
    ret |= reg_val[3];   
    return ret;
}

//**** RX demodulation and baseband process reset
void BK_Rx_BB_Reset(void)	//0x32[4] for 9522,0x3F[31] for 9526
{
    RX_I2C_Read(0x3f,reg_val);
    reg_val[0] &= ~0x80;			//ex val RX_REG32[4]=rx_bb_en = 0
    RX_I2C_Write(0x3f,reg_val);
    reg_val[0] |= 0x80;			    //ex val RX_REG32[4]=rx_bb_en = 1
    RX_I2C_Write(0x3f,reg_val);
}

//****************************************
void BK_Rx_PLC_Reset(void)
{
   RX_I2C_Read(0x3f,reg_val);
   reg_val[0] |= 0x10;
   RX_I2C_Write(0x3f,reg_val);
   reg_val[0] &= ~0x10;
   RX_I2C_Write(0x3f,reg_val);
}

//**** RX mode,trigger chip to calibrate
void Rx_Trigger(void)
{
    //Tune the RF loop LDO voltage to 0x0
    analog_reg_val[6][0] &= ~0xE0;//REG6[31:29]=0
    RX_I2C_Write(0x06,analog_reg_val[6]);
	delay_nms(1);  //Delay 1ms
    //Enable calibration clock
    analog_reg_val[7][0] |= 0x02;//REG7[25]=1
    RX_I2C_Write(0x07,analog_reg_val[7]);

    //Calibrate RF VCO
    analog_reg_val[3][1] &= ~0x40;//REG3[22]=0
    RX_I2C_Write(0x03,analog_reg_val[3]);
    analog_reg_val[3][1] |= 0x40;//REG3[22]=1
    RX_I2C_Write(0x03,analog_reg_val[3]);
    delay_nms(2);  //At least delay 2ms	20230626 5->2

    //Calibrate Digital VCO
    analog_reg_val[4][0] &= ~0x02; //REG4[25]=0
    RX_I2C_Write(0x04,analog_reg_val[4]);
    analog_reg_val[4][0] |= 0x02;  //REG4[25]=1
    RX_I2C_Write(0x04,analog_reg_val[4]);

    //Disable calibration clock
    analog_reg_val[7][0] &= ~0x02;//REG7[25]=0
    RX_I2C_Write(0x07,analog_reg_val[7]);
    delay_nms(1);  //Delay 1ms

    //load the default RF loop LDO voltage
    analog_reg_val[6][0] |= 0x40;//REG6[31:29]=0x2
    RX_I2C_Write(0x06,analog_reg_val[6]);
}
//**** 設置接收工作頻率 *****************************************************
void RX_TuneFreq(void)
{
#if 1
	if(FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].dis<710000)	analog_reg_val[3][2] |= (1<<5);	// <710Mhz
		else															analog_reg_val[3][2] = (analog_reg_val[3][2]&0x1f);     //REG0x03[15:13] = 000b
    analog_reg_val[3][1] &= ~0x20;                          //REG3[21]=0
    analog_reg_val[3][1] |= 0x10;                           //REG3[20]=1
    RX_I2C_Write(0x03,analog_reg_val[3]);
#else
	if(CHA==rWorkChannel)	freq = FreqTableA[rFreqIndex].dis;
		else				freq = FreqTableB[rFreqIndex].dis;
	if( freq < 710000){
		analog_reg_val[3][2] &=~0xE0;							//REG3[15:13]=0
		analog_reg_val[3][2] |= 0x20;							//REG3[13]=1
	}else{
		analog_reg_val[3][2] &=~0xE0;							//REG3[15:13]=0
	}
	RX_I2C_Write(0x03,analog_reg_val[3]);
#endif
	RX_I2C_WriteReg(0x0D,FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].reg0xD);
	RX_I2C_WriteReg(0x39,FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].reg0x39);	// 设定工作ID 20221021
#if 0
	BK95xx_DBG_INFO("RX%d TuneFreq Ch:%d  %ld  %08x  id:%08X\n",rWorkChannel, rFreqIndex[rWorkChannel], FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].dis,\
			FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].reg0xD, FreqTable[rWorkChannel][rFreqIndex[rWorkChannel]].reg0x39);
#endif
	Rx_Trigger();
    BK_Rx_BB_Reset();
	BK_Rx_PLC_Reset();
}


//**** RX antenna mode selection
void BK_Rx_Ant_mode(e_AntennaType mode)
{
    RX_I2C_Read(0x3f,reg_val);
    if(DA_AUTO==mode){
        reg_val[1] |= 0x02;             //自动天线使能
    }else if(SA_ANT2_PIN5==mode){
        reg_val[1] &= ~0x02;            //自动天线禁止
        reg_val[1] |= 0x01;             //选择PIN5作为天线输入端口
    }else{
        reg_val[1] &= ~0x02;            //自动天线禁止
        reg_val[1] &= ~0x01;            //选择PIN6作为天线输入端口
    }
    RX_I2C_Write(0x3f,reg_val);
}

//*******************************************
void BK_Rx_ADPCM_ModeSel(e_Adpcm_ErrDef mode)
{
    RX_I2C_Read(0x3d,reg_val);
    reg_val[2] &= ~(1<<6);
    reg_val[2] |= (mode<<6);
    RX_I2C_Write(0x3d,reg_val);
}

//******************************************
void BK_Rx_PLC_En(u8 status)
{
    RX_I2C_Read(0x3b,reg_val);
    if(status){
        reg_val[2] |= 0x08;	//开PLC
    }else{
        reg_val[2] &= ~0x08;	//关PLC
    }
    RX_I2C_Write(0x3b,reg_val);
    BK_Rx_PLC_Reset(); 
}

//******************************************
void BK_Rx_Reverb_En(u8 status)
{
    RX_I2C_Read(0x36,reg_val);
    if(status){
        reg_val[3] &= ~0x40;            //开混响
        reg_val[0] |= 0x02;             //清混响缓存,避免开机出现啪啪啪的回声
        RX_I2C_Write(0x36,reg_val);
        reg_val[0] &= ~0x02;            //使能混响缓存
    }else{
        reg_val[3] |= 0x40;            //关混响
    }
    RX_I2C_Write(0x36,reg_val);
}

//******************************************
char BK_Rx_GetVolume(void)
{
    char eq_gain = -24;
    char duf_gain = -7;
    u32 tmp;
    tmp = RX_I2C_ReadReg(0x32);
    duf_gain += (tmp&0x1f);
    eq_gain += ((tmp>>5)&0x1f);
    return (eq_gain+duf_gain);
}
/*
 * 改REG32的音频增益即可。默认是2dB。
#ifdef RX_GAIN_USER
;PDW 0x20, 0xFF, 0x0F, 0xFF ;REG32, EqGain+DufGain=7+24=31dB  ;2019-9-12
;PDW 0x20, 0xFF, 0x0F, 0xF9 ;REG32, EqGain+DufGain=7+18=25dB  ;2020-6-16
PDW 0x20, 0xFF, 0x0F, 0xe7 ;REG32, EqGain+DufGain=7+0=7dB  ;2019-9-10
;PDW 0x20, 0xFF, 0x0F, 0xA7 ;REG32, EqGain+DufGain=5+0=5dB
;PDW 0x20, 0xFF, 0x0E, 0xE7 ;REG32,EqGain+DufGain=-1+0=-1dB  ;2022-12-2
#else
;PDW 0x20, 0xFF, 0x0F, 0x09 ;REG32, EqGain+DufGain=0+2dB ;2019-9-10
PDW 0x20, 0xFF, 0x0F, 0x47 ;REG32, 音频增益(标准):EqGain+DufGain=2+0dB  ;2019-9-10
#endif
 */
//******************************************
void BK_Rx_SetVolume(u8 vol)
{
    u32 tmp;
    if(vol>7) vol = 7;
    tmp = RX_I2C_ReadReg(0x32);
//	BK95xx_DBG_INFO("== SetVolume1: %08X\n",tmp);
	tmp &= ~0xfful;	//yuan
	tmp |= 7;
//yuan    tmp &= ~(0x1ful<<3);
	tmp |= ((u32)vol<<5);
//	BK95xx_DBG_INFO("== SetVolume2: %08X\n",tmp);
    RX_I2C_WriteReg(0x32,tmp);
}


////Software AFC:Calibrate clock frequency
///*-----------------------------------------------------------------------------------------------------------
//!!!24.576M crystal requirement:Load capacitor=CL=10pF;Frequency tolerance=F0=+-10ppm.The bits_per_ppm should be not bigger than 2.
//
// bits_per_ppm(bits/ppm):frequency tuning coefficient, which is tested by used 24.576M crystal.
//
//  Test method:
//  a) This function(BK_Cali_Clk(void)) is not loaded to disable software AFC. Auto antenna control is off(REG32[29:28]=0).
//
//  b) Connect the PIN6 of BK9522 to spectrum analyzer.The analyzer settings:central freq=2*(fwork+0.16384)MHz,span=100KHz,REF=-20dBm.
//
//  c) The default ctune value(REG1A[6:0] ) is 0x40. Tune the external capacitors' values(demo board's C1,C2) to make the 2*flo
//  within 2*(fwork+0.16384)MHz+-3ppm.For example,the work frequency is 790MHz,and thus the 2*flo is 1580.32768MHz.Tune the C1,C2 to
//  make the 2*flo  frequency within 1580.32768MHz+-5KHz.Choose at least 3 PCB boards to verify the C1,C2 values.
//   The C1 and C2 are about 10pF if the crystal's parameter CL is 10pF.
//
//    (The same method is applied to TX BK9521:load BK_TX_ContinuousWave_Start() to be TX ContinuousWave,
//    The analyzer settings:central freq=fwork MHz,span=100KHz,REF=15dBm.Tune the C1,C2 to make the frequency within
//    fwork +-3ppm(such as 790MHz+-2kHz).Choose at least 3 PCB boards to verify the C1,C2 values.)
//
//  d) Using the above C1,C2 values to BK9521 and BK9522 boards.Change the BK9522 ctune value(REG1A[6:0] ) to 0x0 and 0x7f,
//  and record the 2*flo frequency values f1 and f2.
//
//  e) Calculate the bits_per_ppm=127*f1/((f1-f2)*10^6).  The bits_per_ppm should be not bigger than 2.
//-----------------------------------------------------------------------------------------------------------*/
//void BK_Cali_Clk(void)
//{
//    u8 FIFO_num, phase_lock, ctune_old_value;
//    u8  xdata CtuneBuf[4];
//    unsigned int xdata temp_val;
//    float xdata bits_per_ppm;
//    int xdata deltaf, ctune_delta_bits, ctune_new_value;
//
//    ctune_delta_bits = 0;
//    bits_per_ppm = 1.8; //The coefficient is tested by demo board's crystal and should not bigger than 2.Customer need change the values according to used crystal.
//
//    RX_I2C_Read(0x19,reg_val);
//    phase_lock = reg_val[3] & 0x20;//REG19[5]=phase lock
//    if(phase_lock)
//    {
//        RX_I2C_Read(0x16,reg_val);
//        FIFO_num = reg_val[1] & 0x3f;//read REG16[21:16]=audio FIFO count
//        temp_val = reg_val[2];
//        temp_val = temp_val << 8;
//        temp_val += reg_val[3];
//        temp_val = (temp_val + 16384) & 0x7fff;//remove bit15
//        deltaf = temp_val - 16384;
//        deltaf = deltaf / 128;//read REG16[14:0]=delta freq,unit is ppm,equal to (ftx-frx)/frx
//        //debug_info=deltaf;
//
//        if((deltaf >= 4) || (deltaf <= -4)) //if big deltaf,coarse tuning,at most +-20ppm
//        {
//            ctune_delta_bits = (int) (deltaf * bits_per_ppm);
//            if(deltaf >= 20) 	ctune_delta_bits = (int) (20 * bits_per_ppm); //Limited to 20ppm
//            else if(deltaf <= -20) 	ctune_delta_bits = (int)(-20 * bits_per_ppm); //Limited to -20ppm
//            AfcCnt = 1;//default AfcCnt=1
//        }
//        else if(AfcCnt == 4) //if small deltaf, count 3 times for fine tuning,at most tuning 1bit
//        {
//            if(FIFO_num > 8) ctune_delta_bits = 1;	//fifo_num limited to 8 for smallest freq dirft
//            else if(FIFO_num < 8 && deltaf <= 0) ctune_delta_bits = -1;
//            else ctune_delta_bits = 0;
//            AfcCnt = 1;//default AfcCnt=1
//        }
//        else
//        {
//            ctune_delta_bits = 0;
//            AfcCnt ++;
//        }
//
//        RX_I2C_Read(0x1a,CtuneBuf);
//        ctune_old_value = CtuneBuf[3] & 0x7f;
//        ctune_new_value = ctune_old_value - ctune_delta_bits;
//        if(ctune_new_value > 0x7f)
//            ctune_new_value = 0x7f;
//        else if(ctune_new_value < 0)
//            ctune_new_value = 0x0;
//
//        CtuneBuf[3] = (CtuneBuf[3] & 0x80) | (ctune_new_value & 0x7f); //update internal crystal capacitors to tune freq
//        RX_I2C_Write(0x1a,CtuneBuf);
//    }
//}
//
//void BK_Rx_BER_Start(void)
//{
//    RX_I2C_Read(0x38,bak_reg_val);          //读同步字到bak_reg_val暂存
//    RX_I2C_WriteReg(0x38,0x000295e5);       //修改同步字
//
//    RX_I2C_Read(0x3f,reg_val);
//    reg_val[0] &= ~0x60;
//    RX_I2C_Write(0x3f,reg_val);
//
//    reg_val[0] |= 0x40;
//    RX_I2C_Write(0x3f,reg_val);
//}
//
//void BK_Rx_BER_Stop(void)
//{
//    RX_I2C_Read(0x3f,reg_val);
//    reg_val[0] &= ~0x60;                    //REG3f[30]=0 for BER test stop
//    RX_I2C_Write(0x3f,reg_val);
//
//    RX_I2C_Write(0x38,bak_reg_val);         //恢复同步字
//}

////RX RFinput as RX sensitivity when BER=0.01%
//void BK_Rx_BER_Read(void)
//{
//    u8 temp_buf[8];
//
//    delay_nms(250);   //Delay at least 250ms to collect PN9 data
//
//    RX_I2C_Read(0x3f, temp_buf, 4);
//    temp_buf[0] |= 0x20;//REG3f[29]=1,BER counts hold to read
//    RX_I2C_Write(0x3f, temp_buf, 4);
//
//    RX_I2C_Read(0x77, temp_buf, 4); //Read total bit counts
//    RX_I2C_Read(0x78, &temp_buf[4], 4); //Read error bit counts
//
//    //BER=error bit counts/total bit counts,return results to PC
//   // USBPrintStr(BK_CMD_BER_READ, temp_buf, sizeof(temp_buf));
//}

//******************************************
void BK_Rx_Mute_En(u8 status)
{
    RX_I2C_Read(0x36,reg_val);
    if(status){
        reg_val[2] &= ~0x20;
    }else{
        reg_val[2] |= 0x20;
    }
    RX_I2C_Write(0x36,reg_val);
    
    //2019-04-19 设置GPIO4静音输出状态
#ifdef ENABLE_GPIO4_MUTE_FUN
    RX_I2C_Read(0x31,reg_val);
    reg_val[3] &= ~(1<<1);
    if(status){
        reg_val[3] |= (GPIO4_MUTE_STATUS<<1);
    }else{
        reg_val[3] |= ((!GPIO4_MUTE_STATUS)<<1);
    }
    RX_I2C_Write(0x31,reg_val);
#endif
}

//************************************************
void BK_Rx_AFC_En(u8 status)
{
    RX_I2C_Read(0x3f,reg_val);
    if(status){
        reg_val[0] |= 0x04;
    }else{
        reg_val[0] &= ~0x04;
    }
    RX_I2C_Write(0x3f,reg_val);
}


////LNA增益设定
//void BK_Rx_Low_Sens(void)
//{
//    RX_I2C_Read(0x3e,reg_val);
//    reg_val[0] &= 0xf0;                 //For example:REG37[23:20]=lna_max=0x4=10dB,  2.5dB/bit
//    reg_val[0] |= 0x04;
//    RX_I2C_Write(0x3e,reg_val);
//}
//

//**********************************************************
bit RX_I2C_Write(u8 reg, u8 *buf)
{
    u8 i;
    if(reg<=0x0f){
        for(i=0;i<4;i++){
            analog_reg_val[reg][i] = buf[i];
        }
    }
    return I2C_BkWriteA8Nbyte(CHIP_DEV_RX, reg, buf, 4);
}

//***********************************************************
bit RX_I2C_Read(u8 reg,u8 *buf)
{
    u8 i;
    if(reg<=0x0b){
        for(i=0;i<4;i++){
            buf[i] = analog_reg_val[reg][i];
        }
        return TRUE;
    }else{
        I2C_BkReadA8Nbyte(CHIP_DEV_RX, reg, buf, 4);
    }
    return (~Ack_Flag)&1;
}

/*
寄存器定义如下：
	PDW 0x40, 0x40, 0x7d, 0x7d ;REG30, 使能GPIO3,GPIO2,GPIO1,GPIO0第二功能    2019-9-12	RX_PCM_SLAVE
	PDW 0x40, 0x40, 0x40, 0x40 ;REG30, 使能GPIO3,GPIO2,GPIO1,GPIO0第二功能

	PDW 0x04, 0x60, 0x60, 0xDD ;REG36, 关混响 I2S使能，MSB在前，从模式。2019-9-9	RX_PCM_SLAVE
	PDW 0x0C, 0x60, 0x60, 0xDD ;REG36, 关混响 I2S使能，MSB在前，主模式。

	PDW 0x89, 0x79, 0x00, 0x3F ;REG3F,  REG3F[17]=0,单天线，REG3F[16]=1,ANT在PIN5,REG3F[26]=0:AFC关,REG3F[4:3]]=11b:I2S左声道输出。  2022-11-28	RX_PCM_SLAVE
	PDW 0x89, 0x79, 0x00, 0x2F ;REG3F,  REG3F[17]=0,单天线，REG3F[16]=1,ANT在PIN5,REG3F[26]=0:AFC关,REG3F[4]]=0b:I2S双声道输出。  2022-11-28

说明：RX_PCM_SLAVE 定义了I2S从模式。否则就是主模式。」
另外，若要右声道输出，则执行如下代码，默认是左声道输出。
;2022-4-28 配合外部I2S右声道接口设置PCM LRCK信号。
#ifdef RX_PCM_SLAVE
 RX_REG_READ 3fh
 OR_VAR_NUM dat1,0001b  ;REG3F[4:3]=10b,I2S右声道输出.
 AND_VAR_NUM dat0,0111b
 RX_REG_WRITE 3fh
#endif
*/

//******************************************
void BK_Rx_I2SOpen(t_PCMCfg cfg)
{
    u32 tmp;
    tmp = RX_I2C_ReadReg(0x30);
    tmp &= 0xff000000;
    if(cfg.mode==PCM_MASTER){
        cfg.bclk = PCM_SCK_O;       //在主模式下BCLK和LRCK被强制作为输出
        cfg.lrck = PCM_SCK_O;
        tmp |= 0x00004040;
    }else{
        cfg.bclk = PCM_SCK_I;       //在从模式下BCLK和LRCK被强制作为输入
        cfg.lrck = PCM_SCK_I;
        tmp |= 0x00007D7D;
    }
    if(cfg.dat==PCM_SDA_I) 	tmp |= 0x007D0000;
    	else 				tmp |= 0x00400000;   //设置DATA的输入输出模式
    RX_I2C_WriteReg(0x30,tmp);      //设定GPIO模式
    tmp = RX_I2C_ReadReg(0x31);
    tmp &= 0xfffe00ff;
    tmp |= (((u32)cfg.dat<<14)|((u32)cfg.bclk<<11)|((u32)cfg.lrck<<8));
    RX_I2C_WriteReg(0x31,tmp);      //GPIO第二功能选择


    tmp = RX_I2C_ReadReg(0x36);
    tmp &= 0x07ffffff;              //选择I2S工作协议
    tmp |= (((u32)cfg.mode<<27)|(1ul<<26));   //使能I2S，并选择工作模式。
    RX_I2C_WriteReg(0x36,tmp);

//	RX_I2C_WriteReg(0x37,0x3E009800);   //默认24.576MHz晶振，48K采样率，24bits数据长度。
    RX_I2C_WriteReg(0x37,0x3E009800);   //默认24.576MHz晶振，48K采样率，24bits数据长度。
    
    //2020-05-27 增加左右声道输出选择
    tmp = RX_I2C_ReadReg(0x3F);
    if(cfg.ch==PCM_LEFT){
        tmp |= (1ul<<4);
        tmp |= (1ul<<3);
    }else if(cfg.ch==PCM_RIGHT){
        tmp |= (1ul<<4);
        tmp &= ~(1ul<<3);
    }else{
        tmp &= ~(1ul<<4);
    }
    RX_I2C_WriteReg(0x3F,tmp);



}
#endif
