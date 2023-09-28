
//#include "USER_Config.h"
#include ".\WMIC_BK\BK9532.h"
#ifdef BK9532

 bit b1MS;                  	// 1ms定时时间 fg
 bit bSignalLost[2];           	// 信号丢失
 bit bReverbEnable[2];         	// 混响使能
 bit bPlcEnable[2];            	// PLC单元使能
 bit bRxMute[2];               	// 静音
 e_AntennaType rAntennaMode[2];	// 天线工作模式
 u16 wSignalLostTimeout[2]; 	// 信号丢失计时
 u16 wShowVolumeTimeout[2]; 	// 音量调整后自动返回频点显示时间计时
 u8	 rCurrentVolume[2];			// 当前输出增益
 u8 rDataAvailableDelay[2];		// 稳定收到信号200ms計時器

 u16 wAFC_OnDly[2];				// 500ms計時
 u32 dwReg7B[2];
 u8 xtal_adj[2];
 bit bAFC_Enable[2];

/*
2020-05-27 增加I2S左右声道输出选择
2020-05-26 增加迈威固定频点随机ID对频模式

2019-08-15 主要修改内容：
1. 收不到信号时关AFC，并以10ms间隔调整REG38[30:24]的值以调整晶振电容达到调整接收本振频率来拓宽信号捕捉的范围
2. 收到信号后仍延迟500ms待接收稳定后再打开AFC进入正常接收状态。
*/

/************** 2020-05-26迈威固定频点随机ID对频示例 **************/

#define     RX_PAIR_DATA_LENGTH	    4           //固定频点随机ID有效字节数为4字节
#define     LEAD_TsdIdle            0xff
#define     LEAD_TsdStart           0x00
#define     LEAD_TsdDataLo          0x60
#define     LEAD_TsdDataHi          0x90
#define     LEAD_TsdChkLo           0x50
#define     LEAD_TsdChkHi           0xa0

#define     PAIR_DONE               1           //对频结束
#define     PAIR_START              0           //对频开始

static uint8_t WorkCh=1;

u8 rREG38BK[4];
u8 const XTAL_TAB[] = {31,43,55,67,79};	// 频偏测试表格
//u8 const XTAL_TAB[] = {63,63,63,63,63};	// 频偏测试表格

//*****************************************************
void BK_I2C_Init()
{
	SDAIN_MODE = GPIO_INPUT;
	gpio_config_new(SCL_IO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(SDA1, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
}

//***************************************************************
void SetWorkAB(uint8_t ab)
{
	rWorkChannel = ab;                             //选用A通道频点
	SET_SDA_IO(ab);
}

u8 PairCh[2]={PAIR_AFn,PAIR_BFn};	//0809 edit
#ifdef 	PAIR_ID
//***************************************************************
bit RX_Pair(u32 *id)
{
	BK_I2C_Init();
    u8 buf[RX_PAIR_DATA_LENGTH] = {0};
    u16  timeout;
    u8 checksum;
    u8 tmp;
    u8 index;
    u8 bytes;
    u8 biterr;
    u8 i;
    bit bPairDone;

    rFreqIndex[rWorkChannel] = PairCh[rWorkChannel];   	//設置对频频点索引
    RX_TuneFreq();                  					//設置对频频点
    BK_Rx_AFC_En(bAFC_Enable[rWorkChannel]=FALSE);		// 没有收到信号关AFC(只有在关AFC的时候调晶振电容才有效)
    bPairDone = PAIR_DONE;
    for(i=0;i<5;i++){
        rREG38BK[0] = XTAL_TAB[i];
        RX_I2C_Write(0x38,rREG38BK);
        delay_nms(10);
        BK95xx_DBG_INFO("ShowRFLevel: %d\n",RX_I2C_ReadReg(0x75)&0xff);	//显示RF RSSI格数
        if((RX_I2C_ReadReg(0x7c)&(1ul<<27))==0){
        	bPairDone = PAIR_START;             //检测到有对频信号，开始对频。
            delay_nms(50);
            BK_Rx_AFC_En(bAFC_Enable[rWorkChannel]=TRUE);     //延迟开AFC
            BK95xx_DBG_INFO("=== PAIR_START...\n");
            break;
        }                
    }
    //=====================================================
    if(bPairDone){
    	BK95xx_DBG_INFO("NO PAIR !!!!\n");
    	return FALSE;	//在对频频点没有收到信号，强制退出。
    }else{
    	BK95xx_DBG_INFO("PAIR ING....\n");
    }
    //=====================================================
    timeout = 1000;		// 对频超时时间1秒。
    while(timeout){
        tmp = RX_I2C_ReadReg(0x7c);     // 读取TX发过来的数据。
        if((biterr>30)||(LEAD_TsdStart==tmp)){
            bytes = 0;
            index = 1;
            biterr = 0;
        }      
        //按次序接收TX发过来的数据。
        switch(index){
            case 1:
            	if(LEAD_TsdDataLo==(tmp&0xf0)){
                    buf[bytes] = tmp&0x0f;
                    index = 2;
                    biterr = 0;
                }
                break;
            case 2:
                if(LEAD_TsdDataHi==(tmp&0xf0)){
                    tmp <<= 4;
                    buf[bytes] |= (tmp&0xf0);
                    if(++bytes<RX_PAIR_DATA_LENGTH){     //判断是否收完对频数据
                        index = 1;
                    }else{
                        index = 3;
                    }
                    biterr = 0;
                }                        
                break;
            case 3:  //接收校验码低4位
               if(LEAD_TsdChkLo==(tmp&0xf0)){
                   checksum = tmp&0x0f;
                   index = 4;
                   biterr = 0;
               }
                break;
            case 4:  //接收校验码高4位
                if(LEAD_TsdChkHi==(tmp&0xf0)){
                    tmp <<= 4;
                    checksum |= (tmp&0xf0);
                    index = 5;
                    biterr = 0;
                }     
                break;
            case 5:  //
                tmp = 0;                  
                for(i=0;i<RX_PAIR_DATA_LENGTH;i++)	tmp += buf[i];	// 计算校验和
                if(tmp==checksum){   //校验成功
                    *id  = buf[3];          //
                    *id <<= 8;              
                    *id |= buf[2];          
                    *id <<= 8;              
                    *id |= buf[1];          
                    *id <<= 8;              
                    *id |= buf[0];          
//                  bPairDone = PAIR_DONE;  //对频成功
                    BK95xx_DBG_INFO("PAIR OK....ID: %08X\n",*id);
                    return TRUE;                    
                }
                index = 0; bytes = 0;
                break;
            default : index = 0; bytes = 0; break;
        }   
        //============================
        if(b1MS){
        	b1MS = FALSE;
            if(timeout>0) timeout--;
            if(biterr<100) biterr++;
        }
    }
    return FALSE;
}
#endif

//**********************************************************************
bit CheckSignalLost(void)
{
	BK_I2C_Init();
    static u8 cnt[2]={3,3};
    static u8 afc_index[2] = {3,3};

    if(RX_I2C_ReadReg(0x7C)&(u32)(1ul<<27) ){	// 没有收到TX信号
    	if(rDataAvailableDelay[rWorkChannel]){
    		BK95xx_DBG_INFO("bSignalLost CH:%d\n",rWorkChannel);
    		rDataAvailableDelay[rWorkChannel] = 0;
    	}
        if(cnt[rWorkChannel]<3){	// 延长丢信号后静音时间
            cnt[rWorkChannel]++;
            if(cnt[rWorkChannel]==3){
                bRxMute[rWorkChannel]=TRUE;
                BK_Rx_Mute_En(TRUE);
            }
            return 1;
        }else{
            bSignalLost[rWorkChannel] = TRUE;
            bRxMute[rWorkChannel]=TRUE;
            wAFC_OnDly[rWorkChannel] = 0;
            if(bAFC_Enable[rWorkChannel]){
                BK_Rx_AFC_En(bAFC_Enable[rWorkChannel]=FALSE);	// 没有收到信号关AFC(只有在关AFC的时候调晶振电容才有效)
                afc_index[rWorkChannel] = 0;
            }else{
            	rREG38BK[0] = XTAL_TAB[afc_index[rWorkChannel]];      //调整晶振电容，以测试是否能收到发射的信号。
            	RX_I2C_Write(0x38,rREG38BK);
            //	BK95xx_DBG_INFO("== Rx%d XtalTab[%d]: %d  Rssi:%d\n",rWorkChannel, afc_index[rWorkChannel], XTAL_TAB[afc_index[rWorkChannel]],(RX_I2C_ReadReg(0x75)&0xff) );
            	if(++afc_index[rWorkChannel]>=5){
            		afc_index[rWorkChannel] = 0;
            		// RX频点自动搜索跟踪
            		if(++rFreqIndex[rWorkChannel]>FreqNum) rFreqIndex[rWorkChannel] = 0;	// 切換頻點
            		RX_TuneFreq();
            	}
            }
        }
    }else{   // 收到TX信号
    	if(bRxMute[rWorkChannel]){
    		bRxMute[rWorkChannel]=FALSE;
    		BK_Rx_Mute_En(FALSE);	// 收到信号解除静音
    		cnt[rWorkChannel] = 0;
    		afc_index[rWorkChannel] = 0;
    		wSignalLostTimeout[rWorkChannel] = 0;
    		afc_index[rWorkChannel] = 0;
    		bSignalLost[rWorkChannel] = FALSE;	// 收到信号
    	}
       	if(wAFC_OnDly[rWorkChannel]==50){	// 收到发射信号后延迟500ms打开AFC
       		if(!bAFC_Enable[rWorkChannel]){
       			BK_Rx_AFC_En(bAFC_Enable[rWorkChannel]=TRUE);    // 开AFC
       			BK95xx_DBG_INFO("CheckSignalLost Yes, AFC ON...\n");
       		}
       		return 1;
       	}else{
       		wAFC_OnDly[rWorkChannel]++;	// 500ms計時
       	}
    }
    return 0;
}

//*****************************************************
void ShowRFLevel()
{
	BK_I2C_Init();
	u8 val = RX_I2C_ReadReg(0x75)&0xff;
	static u8 buf[4];
    static u8 bi = 0;
    //RF RSSI 消抖动
    buf[bi] = val;
    if(++bi>=4) bi = 0;
    val = 0;
    for(u8 i=0;i<4;i++){
        if(buf[i]>val) val = buf[i];
    }

    static uint8_t valB;
    if(abs(valB-val) > 1){
    	valB = val;
    	BK95xx_DBG_INFO("Ch:%d  RFLevel:%d  FreqIndex:%d\n",WorkCh, val, rFreqIndex[WorkCh]);
    }
}

//************************************************************
void ShowAFLevel()
{
	BK_I2C_Init();
	u8 val = (RX_I2C_ReadReg(0x79)&0xffff)>>6;
	static uint8_t valB;
	if(abs(valB-val) > 2){
		valB = val;
		BK95xx_DBG_INFO("ShowAFLevel: %d\n",val);
	}
}

static uint8_t FUN_NUM=0;
static uint8_t NoPlay=0;
//*************************************
void BK9532_COMM(void)
{
#if defined(ZY_OA_001)||defined(MZ_200K)
	BK_I2C_Init();
	switch(FUN_NUM){
	case 0:	// 流行
		UserEf.Pitch = 0;
		UserEf.RevVol=0.3;
		UserEf.RevVol2=0.4;
		UserEf.RevRep=0.4;
		UserEf.EchoVol=0.3;
		UserEf.EchoVol2=0.4;
		UserEf.EchoDeyT=200;
		UserEf.EchoRep=0.4;
		if(NoPlay){
			if(SysInf.Lang){
				app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
			}else{
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);
			}
		}else{
			NoPlay=1;
		}
		break;
	case 1:	// 歌神
		UserEf.Pitch = 0;
		UserEf.RevVol=0.3;
		UserEf.RevVol2=0.3;
		UserEf.RevRep=0.3;
		UserEf.EchoVol=0.3;
		UserEf.EchoVol2=0.3;
		UserEf.EchoDeyT=150;
		UserEf.EchoRep=0.3;
		if(SysInf.Lang){
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
		}else{
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0);
		}
		break;
	case 2:	// 主持
		UserEf.Pitch = 0;
		UserEf.RevVol=0.1;
		UserEf.RevVol2=0.1;
		UserEf.RevRep=0.1;
		UserEf.EchoVol=0;
		UserEf.EchoVol2=0;
		UserEf.EchoDeyT=0;
		UserEf.EchoRep=0;
		if(SysInf.Lang){
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);
		}else{
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);
		}
		break;
	case 3:	// 錄音棚
		UserEf.Pitch = 0;
		UserEf.RevVol=0.2;
		UserEf.RevVol2=0.2;
		UserEf.RevRep=0.2;
		UserEf.EchoVol=0.2;
		UserEf.EchoVol2=0.2;
		UserEf.EchoDeyT=150;
		UserEf.EchoRep=0.4;
		if(SysInf.Lang){
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
		}else{
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);
		}
		break;
	case 4:	// 機器人
		UserEf.Pitch = -4;
		UserEf.RevVol=0.1;
		UserEf.RevVol2=0.1;
		UserEf.RevRep=0.1;
		UserEf.EchoVol=0;
		UserEf.EchoVol2=0;
		UserEf.EchoDeyT=0;
		UserEf.EchoRep=0;
		if(SysInf.Lang){
			app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC);
		}else{
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);
		}
		break;
	case 5:  // 娃娃音
		UserEf.Pitch = 12;
		UserEf.RevVol=0.1;
		UserEf.RevVol2=0.1;
		UserEf.RevRep=0.2;
		UserEf.EchoVol=0;
		UserEf.EchoVol2=0;
		UserEf.EchoDeyT=0;
		UserEf.EchoRep=0;
		if(SysInf.Lang){
			app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		}else{
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_6);
		}
		break;
	}
	if(++FUN_NUM>5) FUN_NUM=0;
#endif
}

uint8_t ChipEn[2];
#define OffContDef	1200
uint16_t WMicOffCont;

t_FreqDef FreqTable[2][FreqNum];
/***************************************************************************
  500~710Mhz  	（freq+0.16384）*6*2^23(8388608)/24.576	600MHz：0x49431EB8
  freqdat = (khz+163.84*6*341)+((khz*6+2)/3);

  710~980Mhz	（freq+0.16384）*4*2^23/24.576	790MHz：0x404DBF25
  freqdat = (khz+163.84*4*341)+((khz*4+2)/3);
****************************************************/
void FreqTabInit(uint8_t wChip)
{
	u32	wfreq;
	for(uint8_t i=0; i<FreqNum;i++){
		if(wChip){	// B CH
			if(i!=PAIR_BFn)	FreqTable[wChip][i].reg0x39 = WORK_ID;
				else		FreqTable[wChip][i].reg0x39 = PAIR_ID;
			wfreq = Bfreq+(i*freqStep);
	#ifdef PAIR_BFreq
			if(i==(FreqNum-1))	wfreq = PAIR_BFreq;
	#endif
		}else{		// A CH
			if(i!=PAIR_AFn)	FreqTable[wChip][i].reg0x39 = WORK_ID;
				else		FreqTable[wChip][i].reg0x39 = PAIR_ID;
			wfreq = Afreq+(i*freqStep);
	#ifdef PAIR_AFreq
			if(i==(FreqNum-1))	wfreq = PAIR_AFreq;
	#endif
		}
		FreqTable[wChip][i].dis = wfreq;
		if(FreqTable[wChip][i].dis<710000)	FreqTable[wChip][i].reg0xD = (float)((float)((float)wfreq+163.84)*6*341)+((float)((float)((float)wfreq+163.84)*6+2)/3);
			else							FreqTable[wChip][i].reg0xD = (float)((float)((float)wfreq+163.84)*4*341)+((float)((float)((float)wfreq+163.84)*4+2)/3);
		BK95xx_DBG_INFO("%d.%02d.dFreqTable: %d  %08X  %08X\n",i,wChip,FreqTable[wChip][i].dis,FreqTable[wChip][i].reg0xD,FreqTable[wChip][i].reg0x39);
	};
}
//******************************************************************
void BK9532_Init(void)
{
	BK_I2C_Init();
	BK95xx_DBG_INFO("====== BK9532_Init 1 ====\n");

	for(uint_t CHn=0;CHn<2;CHn++){
		FreqTabInit(CHn);					// 配置頻率表
		rFreqIndex[CHn] = 0;                // 默认从第1个频点开始
		rCurrentVolume[CHn] = INIT_VOLUME;	// 默认开机音量
		rAntennaMode[CHn] = INIT_ANTMODE;	// 默认天线模式
		bReverbEnable[CHn] = FALSE;        	// 默认开混响
		xtal_adj[CHn] = 0x63;				// 设晶振振荡电容调整初值 2019-08-15

		SetWorkAB(CHn);
		//===========================================
		SET_SDA_IO(CHn);
		ChipEn[CHn] = Init_RX();	// RX初始化
		if(ChipEn[CHn]){
			RX_I2C_Read(0x38,rREG38BK);	// 将REG38寄存器的值备份到rREG38BK
			rREG38BK[0] = xtal_adj[CHn];		// 调整晶振电容
			RX_I2C_Write(0x38,rREG38BK);
#ifdef 	PAIR_ID
			if(CHn){
				if(RX_Pair(&SysInf.BchPair)){	// RX上电对频,如果对频成功，工作ID的值会被改为新的ID。否则不会被改动。
					GroupWrite(0, 2); // 对频成功, 保存 Pair ID
					//==== 闪灯，提示对频成功 ======================
					BK95xx_DBG_INFO("Bch Pair Ok... %08X\n",SysInf.BchPair);
				}
				for(u8 i=0; i<FreqNum; i++){
					if(i!=PAIR_BFn)	FreqTable[rWorkChannel][i].reg0x39 = SysInf.BchPair;
				}
			}else{
    			if(RX_Pair(&SysInf.AchPair)){	// RX上电对频,如果对频成功，工作ID的值会被改为新的ID。否则不会被改动。
    				GroupWrite(0, 2); // 对频成功,保存 Pair ID
    				//==== 闪灯，提示对频成功 ================================
    				BK95xx_DBG_INFO("Ach Pair Ok... %08X\n",SysInf.AchPair);
    			}
    			for(u8 i=0; i<FreqNum; i++){
    				if(i!=PAIR_AFn)	FreqTable[rWorkChannel][i].reg0x39 = SysInf.AchPair;
    			}
    		}
#endif
			//==== 增加I2S设置调用演示 ========================
			t_PCMCfg i2s_cfg;       //2020-05-26 增加I2S设置调用
			i2s_cfg.mode = PCM_SLAVE;	//I2S模式
			i2s_cfg.dat  = PCM_SDA_O;	//SDA为输出引脚
//			i2s_cfg.bclk = PCM_SCK_I;	//SCK引脚
//			i2s_cfg.lrck = PCM_LRCK_I;	//LRCK引脚
			if(CHn){
				i2s_cfg.ch   = PCM_RIGHT;	//音声道
			}else{
				i2s_cfg.ch   = PCM_LEFT;	//音声道
			}
			BK_Rx_I2SOpen(i2s_cfg);		//开启I2S接口
			//================================================
			BK_Rx_SetVolume(rCurrentVolume[CHn]);	// 设定音量
			BK_Rx_Ant_mode(rAntennaMode[CHn]);		// 天线模式。
//			RX_WriteID(SysInf.AchPair);				// 设定工作ID 20221021
//			BK95xx_DBG_INFO("RX1 WorkID:%08X\n",SysInf.AchPair);
//			rFreqIndex[CHn] = 1;					// 2020-05-26
//			RX_TuneFreq();                    		// 设定频点

			BK_Rx_ADPCM_ModeSel(ADPCM_NORMAL);		// ADPCM常规处理方式
			BK_Rx_PLC_En(bPlcEnable[CHA]=TRUE);		// 开 PLC
			BK_Rx_Reverb_En(bReverbEnable[CHn]);	// 开/关混响

			bSignalLost[CHn] = TRUE;				// 刚上电，默认没有收到TX信号
			bRxMute[CHn] = TRUE;					// 2019-05-21 上电静音
			BK_Rx_Mute_En(bRxMute[CHn]);        	// 解除静音  2019-04-19 为避免开机GPIO4电平乱跳屏蔽此条语句

			BK_Rx_AFC_En(bAFC_Enable[CHn]=FALSE);   //关掉AFC
		}else{
			BK95xx_DBG_INFO("Init_RX %d !!! Err\n",CHn);
		}
	}
	SetWorkAB(CHA);
	NoPlay=0;
	FUN_NUM=0;
	BK9532_COMM();
	WMicOffCont=OffContDef;
}

//**** 10ms 週期執行 ********************************************************
void BK9532_Main(void)
{
	WorkCh ^=1;
	BK_I2C_Init();
	if(WorkCh && ChipEn[1]){
		static uint32_t Btmp, BtmpR;
		//====================================================
		if(ChipEn[1]){
			SetWorkAB(CHB);
			if(CheckSignalLost()){	// 检查TX信号是否丢失
				// tmp = RX_I2C_ReadReg(0x76);
				// if(tmp&(1ul<<15)){	// 当前使用天线1
				// }else{				// 当前使用天线2
				// }
				// 显示频点. 通道A
				// 显示音量
				// PLC开启时，电池图标被点亮
				// 混响开启时，钥匙图标被点亮
				// 在收不到TX信号后 RF RSSI会有乱跳的情况。这里是为了在收不到信号后及时关掉RF RSSI显示。
//    			ShowRFLevel(); 	//显示RF RSSI格数
//    			ShowAFLevel();	//显示AF RSSI格数

				if(rDataAvailableDelay[CHB]>=20){	// 稳定收到信号200ms后才去读TX发的键值。在临界状态下，用户数据值不能确定。
					Btmp = RX_I2C_ReadReg(0x7c);    // 读取用户数据（接收TX发来的按键值）
					if(BtmpR != Btmp){
						BtmpR = Btmp;
						if((Btmp&0xFF)==0x5A)	BK9532_COMM();
				//		BK95xx_DBG_INFO("==== Btmp : %08X\n",Btmp);
					}
					WMicOffCont=OffContDef;
				}else{
					rDataAvailableDelay[CHB]++;	// 200ms計時
				}
			}
			SetWorkAB(CHA);
		}
	}else if(WorkCh==0 && ChipEn[0]){
		static uint32_t Atmp, AtmpR;
		//====================================================
		if(ChipEn[0]){
			SetWorkAB(CHA);
			if(CheckSignalLost()){	// 检查TX信号是否丢失
//   			ShowRFLevel();	//显示RF RSSI格数
//    			ShowAFLevel();	//显示AF RSSI格数

				if(rDataAvailableDelay[CHA]>=20){	// 稳定收到信号200ms后才去读TX发的键值。在临界状态下，用户数据值不能确定。
					Atmp = RX_I2C_ReadReg(0x7c);            //读取用户数据（接收TX发来的按键值）
					if(AtmpR != Atmp){
						AtmpR = Atmp;
						if((Atmp&0xFF)==0x5A)	BK9532_COMM();
				//		BK95xx_DBG_INFO("==== Atmp : %08X\n",Atmp);
					}
					WMicOffCont=OffContDef;
				}else{
					rDataAvailableDelay[CHA]++;	// 200ms計時
				}
			}
		}
	}
}

#endif

