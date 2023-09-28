
/**********************************************
 * @file     qn8035.c
   @brief    qn8035收音底层驱动
   @details
   @author
   @date   2011-11-24
   @note
***********************************************/
#include "USER_Config.h"

#ifdef __QN8035_H
uint16_t Fre_data;     //信道设置变量

//********************************************
void QND_DelayMs(uint16_t ms)
{
	for(uint16_t i=0; i<ms; i++){
		os_delay_us(1600);
	}
}
//********************************************
uchar QND_ReadReg(uchar regAddr)
{
	uint8_t  byte;
	SDAIN_MODE = GPIO_INPUT;
	I2cSpeed = 5;
	byte = I2C_ReadA8D8(0x10, regAddr);
//	SDA_MODE = GPIO_INOUT;
	I2cSpeed = 0;
    return  byte;
}

//****************************************************0
void QND_WriteReg(uchar regAddr,uchar val)
{
	SDAIN_MODE = GPIO_INPUT;
	I2cSpeed = 5;
	I2C_WriteA8D8(0x10, regAddr, val);
//	SDA_MODE = GPIO_INOUT;
	I2cSpeed = 0;
}
//************************************************************
void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val)
{
    UINT8 temp;
    temp = QND_ReadReg(reg);
    temp &= (UINT8)(~bitMask);
    temp |= data_val & bitMask;
    QND_WriteReg(reg, temp);
}




//#endif
//****************************************************************************************
#if(QnTest==2)

#define R_TXRX_MASK    0x30
UINT8   qnd_RSSIns = 255;
UINT8   qnd_Crystal = QND_CRYSTAL_DEFAULT;
UINT8   qnd_RSSIn[QND_BAND_NUM];
UINT8   qnd_PrevMode;
UINT8   qnd_Country  = COUNTRY_CHINA ;
UINT16  qnd_CH_START = 7600;
UINT16  qnd_CH_STOP  = 10800;
UINT8   qnd_CH_STEP  = 1;
UINT8  qnd_ClearScanFlag = 0;
UINT16 qnd_ClearChannel = 0;
UINT8  qnd_FirstScan = 1;

UINT8  qnd_AutoScanAll;
UINT8  qnd_IsStereo;
UINT8  qnd_ChCount;
UINT16  qnd_RSSInBB[QND_BAND_NUM+1];
UINT16 qnd_ChList[QN_CCA_MAX_CH];
UINT8   qnd_StepTbl[3]={5,10,20};
QND_SeekCallBack qnd_CallBackFunc = 0;



/**********************************************************************
void QNF_RXInit()
**********************************************************************
Description: set to SNR based MPX control. Call this function before
             tune to one specific channel

Parameters:
None
Return Value:
None
**********************************************************************/
void QNF_RXInit()
{
    QNF_SetRegBit(0x1B,0x08,0x00); //Let NFILT adjust freely
    QNF_SetRegBit(0x2C,0x3F,0x09); //When SNR<ccth31, ccfilt3 will work
    QNF_SetRegBit(0x1D,0x40,0x00);//Let ccfilter3 adjust freely
    QNF_SetRegBit(0x41,0x0F,0x0A);//Set a hcc index to trig ccfilter3's adjust
    QND_WriteReg(0x45,0x20);//Set aud_thrd will affect ccfilter3's tap number
    QNF_SetRegBit(0x40,0x70,0x70); //snc/hcc/sm snr_rssi_sel; snc_start=40; hcc_start=22; sm_start=20
    QNF_SetRegBit(0x19,0x80,0x80); //Use SNR for ccfilter selection criterion
    QNF_SetRegBit(0x3E,0x80,0x80); //it is decided by programming this register
    QNF_SetRegBit(0x41,0xE0,0xA0);//DC notching High pass filter bandwidth; remove low freqency dc signals
    QNF_SetRegBit(0x42,0x10,0x10);//disable the vtune monitor
    QNF_SetRegBit(0x34, 0x7F,SMSTART_VAL); //set SNCSTART
   // QNF_SetRegBit(0x35,0x7F,SNCSTART_VAL); //set SNCSTART
   // QNF_SetRegBit(0x36,0x7F,HCCSTART_VAL); //set HCCSTART
   QNF_SetRegBit(0x35,0x7F,60); //Change SNCSTART=60,the past SNCSTART=40
   QNF_SetRegBit(0x36,0x7F,36); //Change HCCSTART=36,the past HCCSTART=22
}


/**********************************************************************
void QNF_InitRSSInBB()
**********************************************************************
Description: init RSSI noise floor band

Parameters:
    None
Return Value:
    None
**********************************************************************/
void QNF_InitRSSInBB()
{
    UINT8 i,d2,d,step;
    UINT16 d1;

    // get frequency leap step
    step = qnd_StepTbl[qnd_CH_STEP];
    // total frequency span
    d1 = qnd_CH_STOP - qnd_CH_START;
    // make sure d2 value <= 255, normally QND_BAND_NUM should < 10
    d2 = step * QND_BAND_NUM;
    d = d1/d2;
    // first one should be CH_START
    qnd_RSSInBB[0] = qnd_CH_START;
    for(i=1; i<QND_BAND_NUM; i++) {
        qnd_RSSInBB[i] = qnd_RSSInBB[i-1] + d * step;
    }
    // last one set one more step higher for convenience
    qnd_RSSInBB[i] = qnd_CH_STOP+step;
}

/**********************************************************************
UINT8 QNF_GetRSSInBandIndex(UINT16 chfreq)
**********************************************************************
Description: get band index

Parameters:
    chfreq: channel frequency (10Khz unit, eg: for 101.10Mhz, input 10110)
Return Value: band index for input CH frequency
**********************************************************************/
UINT8 QNF_GetRSSInBandIndex(UINT16 chfreq)
{
    UINT8 i;
    for(i=0; i<QND_BAND_NUM; i++) {
        if(chfreq < qnd_RSSInBB[i+1]) {
            // finally come to here, if no wrong input
            return i;
        }
    }
    return 0;
}

/**********************************************************************
UINT8 QNF_GetRSSIn(UINT16 chFreq)
**********************************************************************
Description: get RSSI noise floor

Parameters:
    chfreq: channel frequency (10Khz unit, eg: for 101.10Mhz, input 10110)
Return Value:
    the RSSI noise floor
**********************************************************************/
UINT8 QNF_GetRSSIn(UINT16 chFreq)
{
    UINT8 idx;
    idx = QNF_GetRSSInBandIndex(chFreq);
    return qnd_RSSIn[idx];
}

/**********************************************************************
UINT16 QNF_GetCh()
**********************************************************************
Description: get current channel frequency

Parameters:
    None
Return Value:
    channel frequency
**********************************************************************/
UINT16 QNF_GetCh()
{
    UINT8 tCh;
    UINT8  tStep;
    UINT16 ch = 0;
    // set to reg: CH_STEP
    tStep = QND_ReadReg(CH_STEP);
    tStep &= CH_CH;
    ch  =  tStep ;
    tCh= QND_ReadReg(CH_NUM);
    ch = (ch<<8)+tCh;
    return CHREG2FREQ(ch);
}

/**********************************************************************
UINT8 QNF_SetCh(UINT16 freq)
**********************************************************************
Description: set channel frequency

Parameters:
    freq:  channel frequency to be set
Return Value:
    1: success
**********************************************************************/
UINT8 QNF_SetCh(UINT16 freq)
{
    // calculate ch parameter used for register setting
    UINT8 tStep;
    UINT8 tCh;
    UINT16 f;
        f = FREQ2CHREG(freq);
        // set to reg: CH
        tCh = (UINT8) f;
        QND_WriteReg(CH_NUM, tCh);
        // set to reg: CH_STEP
        tStep = QND_ReadReg(CH_STEP);
        tStep &= ~CH_CH;
        tStep |= ((UINT8) (f >> 8) & CH_CH);
        QND_WriteReg(CH_STEP, tStep);

    return 1;
}

/**********************************************************************
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step)
**********************************************************************
Description: config start, stop, step register for FM/AM CCA or CCS

Parameters:
    start
        Set the frequency (10kHz) where scan to be started,
        eg: 7600 for 76.00MHz.
    stop
        Set the frequency (10kHz) where scan to be stopped,
        eg: 10800 for 108.00MHz
    step
        1: set leap step to (FM)100kHz / 10kHz(AM)
        2: set leap step to (FM)200kHz / 1kHz(AM)
        0:  set leap step to (FM)50kHz / 9kHz(AM)
Return Value:
         None
**********************************************************************/
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step)
{
    // calculate ch para
    UINT8 tStep = 0;
    UINT8 tS;
    UINT16 fStart;
    UINT16 fStop;
        fStart = FREQ2CHREG(start);
        fStop = FREQ2CHREG(stop);
        // set to reg: CH_START
    tS = (UINT8) fStart;
    QND_WriteReg(CH_START, tS);
    tStep |= ((UINT8) (fStart >> 6) & CH_CH_START);
    // set to reg: CH_STOP
    tS = (UINT8) fStop;
    QND_WriteReg(CH_STOP, tS);
    tStep |= ((UINT8) (fStop >> 4) & CH_CH_STOP);
    // set to reg: CH_STEP
    tStep |= step << 6;
    QND_WriteReg(CH_STEP, tStep);
}

/**********************************************************************
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode)
**********************************************************************
Description:    Set audio output to mono.

Parameters:
  modemask: mask register specified bit
  mode
        QND_RX_AUDIO_MONO:    RX audio to mono
        QND_RX_AUDIO_STEREO:  RX audio to stereo
        QND_TX_AUDIO_MONO:    TX audio to mono
        QND_TX_AUDIO_STEREO:  TX audio to stereo
Return Value:
  None
**********************************************************************/
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode)
{
    if (mode == QND_RX_AUDIO_MONO)
         QNF_SetRegBit(SYSTEM1,modemask, 0x04);
    else
        QNF_SetRegBit(SYSTEM1,modemask, mode);

}

/**********************************************************************
void QNF_UpdateRSSIn(UINT16 chFreq)
**********************************************************************
Description: update the qnd_RSSIns and qnd_RSSIn value
Parameters:
    None
Return Value:
    None
**********************************************************************/
void  QNF_UpdateRSSIn(UINT16 chFreq)
{
    UINT8 i,j;
    UINT8 r0;
    UINT16 ch;
    UINT8 minrssi;

    // backup SYSTEM1 register
    r0 = QND_ReadReg(SYSTEM1);
    UINT8 tmp1,tmp2;
    for( i = 0; i < QND_BAND_NUM; i++ )
    {
        minrssi = 255;
        for (j = 0; j < 4; j++)
        {
            tmp1 = QND_GetRSSI(qnd_RSSInBB[i] + j*qnd_StepTbl[qnd_CH_STEP]);
            if(i == QND_BAND_NUM-1 )
                tmp2 = QND_GetRSSI(qnd_RSSInBB[i+1] - qnd_StepTbl[qnd_CH_STEP] - j*qnd_StepTbl[qnd_CH_STEP]);
            else
                tmp2 = QND_GetRSSI(qnd_RSSInBB[i+1] - j*qnd_StepTbl[qnd_CH_STEP]);
            if (tmp1 <  minrssi)
            {
                minrssi = tmp1;
                ch = qnd_RSSInBB[i] + j*qnd_StepTbl[qnd_CH_STEP];
            }
            if (tmp2 <  minrssi)
            {
                minrssi = tmp2;
                ch = qnd_RSSInBB[i+1] - j*qnd_StepTbl[qnd_CH_STEP];
            }
        }
        qnd_RSSIn[i] = minrssi;
        if (qnd_RSSIns > minrssi)
        {
            qnd_RSSIns = minrssi;
            qnd_ClearChannel = ch;
        }
    }
    QND_WriteReg(SMSTART,qnd_RSSIns + 12);
    QND_WriteReg(HCCSTART,qnd_RSSIns + 12);
    QND_WriteReg(SNCSTART,qnd_RSSIns + 18);
    // restore SYSTEM1 register
    QND_WriteReg(SYSTEM1, r0);
}

/**********************************************************************
UINT8 QND_GetRSSI(UINT16 ch)
**********************************************************************
Description:    Get the RSSI value
Parameters:
Return Value:
RSSI value  of the channel setted
**********************************************************************/
UINT8 QND_GetRSSI(UINT16 ch)
{
    QND_SetSysMode(QND_MODE_RX|QND_MODE_FM);
    QNF_ConfigScan(ch, ch, qnd_CH_STEP);
    QNF_SetCh(ch);
    QNF_SetRegBit(0x00, 0x33, 0x13);  //Enter CCA mode. This speed up the channel locking.
    QND_DelayMs(100);
    if(QND_ReadReg(RDSCOSTAS) & 0x80){ //this is a software patch for reading RSSI
        return (QND_ReadReg(RSSISIG)+9);
    }else{
        return QND_ReadReg(RSSISIG);
    }
}

/**********************************************************************
void QN_ChipInitialization()
**********************************************************************
Description: chip first step initialization, called only by QND_Init()

Parameters:
None
Return Value:
None
**********************************************************************/
void QN_ChipInitialization()
{
    QND_WriteReg(0x00, 0x81);
    QND_DelayMs(10);
    QNF_SetMute(1);
    QND_WriteReg(0x54, 0x47);//mod pll setting
    QND_WriteReg(0x19, 0x40);//AGC setting
    QND_WriteReg(0x2d, 0xD6);//notch filter threshold adjusting
    QND_WriteReg(0x43, 0x10);//notch filter threshold enable
    QND_WriteReg(0x00, 0x51);
    QND_WriteReg(0x00, 0x21); // chip calibration
    QND_DelayMs(50);
}

/**********************************************************************
int QND_Init()
**********************************************************************
Description: Initialize device to make it ready to have all functionality ready for use.

Parameters:
    None
Return Value:
    1: Device is ready to use.
    0: Device is not ready to serve function.
**********************************************************************/
void QN8035_Init()
{
    QN_ChipInitialization();
    QNF_SetMute(1);
    QND_WriteReg(SYSTEM1,  0x01); //resume original status of chip /* 2008 06 13 */
    QNF_SetMute(0);


    Fre_data = 9970;
    QND_TuneToCH(Fre_data);

}

/**********************************************************************
void QND_SetSysMode(UINT16 mode)
***********************************************************************
Description: Set device system mode(like: sleep ,wakeup etc)
Parameters:
mode:  set the system mode , it will be set by  some macro define usually:

SLEEP (added prefix: QND_MODE_, same as below):  set chip to sleep mode
WAKEUP: wake up chip
TX:     set chip work on TX mode
RX:     set chip work on RX mode
FM:     set chip work on FM mode
AM:     set chip work on AM mode
TX|FM:  set chip work on FM,TX mode
RX|AM;  set chip work on AM,RX mode
RX|FM:    set chip work on FM,RX mode
Return Value:
None
**********************************************************************/
void QND_SetSysMode(UINT16 mode)
{
    UINT8 val;
    switch(mode)
    {
    case QND_MODE_SLEEP:                       //set sleep mode
        QNF_SetMute(1);
        //        QNF_SetRegBit(0x48, 0x80, 0x00); //neither the REG_LAN register nor HG2_EN bit in the QN8035
        qnd_PrevMode = QND_ReadReg(SYSTEM1);
        QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, STNBY);
        break;
    case QND_MODE_WAKEUP:                      //set wakeup mode
        QND_WriteReg(SYSTEM1, qnd_PrevMode);
        QNF_SetMute(0);
        break;
    case QND_MODE_DEFAULT:
        QNF_SetRegBit(SYSTEM1,0x30,0x10);
        break;
    default:
        val = (UINT8)(mode >> 8);
        if (val){
            // QNF_SetRegBit(0x48, 0x80, 0x80);//neither the REG_LAN register nor HG2_EN bit in the QN8035
            val = val >> 3;
            if(val&0x10)
                // set to new mode if it's not same as old
                if((QND_ReadReg(SYSTEM1) & R_TXRX_MASK) != val){
                    QNF_SetMute(1);
                    QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, val);
                    //   QNF_SetMute(0);
                }
                // make sure it's working on analog output
                // QNF_SetRegBit(SYSTEM1, 0x08, 0x00);//it has not IIS functionality in QN8035
        }
        break;
    }
}

/**********************************************************************
void QND_TuneToCH(UINT16 ch)
**********************************************************************
Description: Tune to the specific channel. call QND_SetSysMode() before
call this function
Parameters:
ch
Set the frequency (10kHz) to be tuned,
eg: 101.30MHz will be set to 10130.
Return Value:
None
**********************************************************************/
void QND_TuneToCH(UINT16 ch)
{
//    UINT8 rssi;
//    UINT8 minrssi;
	Show_Freq();

    UINT8 reg;

    QNF_RXInit();
    QNF_SetMute(1);
    if ((ch==8430)||(ch==7290)||(ch==6910))  //Peter has a list of channel to flip IMR. Please ask him for update
    {
        QNF_SetRegBit(CCA, IMR, IMR);           //this is a software patch
    }
    else
    {
        QNF_SetRegBit(CCA, IMR, 0x00);
    }
    QNF_ConfigScan(ch, ch, qnd_CH_STEP);
    QNF_SetCh(ch);
    QNF_SetRegBit(0x00, 0x33, 0x13);  //Enter CCA mode. This speed up the channel locking.
    //Auto tuning
    QND_WriteReg(0x4F, 0x80);
    reg = QND_ReadReg(0x4F);
    reg >>= 1;
    QND_WriteReg(0x4F, reg);
    QND_DelayMs(CH_SETUP_DELAY_TIME);
    QNF_SetMute(0);
}

/**********************************************************************
void QND_SetCountry(UINT8 country)
***********************************************************************
Description: Set start, stop, step for RX and TX based on different
             country
Parameters:
country:
Set the chip used in specified country:
    CHINA:
    USA:
    JAPAN:
Return Value:
    None
**********************************************************************/
void QND_SetCountry(UINT8 country)
{
    qnd_Country = country;
    switch(country)
    {
    case COUNTRY_CHINA:
        qnd_CH_START = 8750;
        qnd_CH_STOP = 10800;
        qnd_CH_STEP = 1;
        break;
    case COUNTRY_USA:
        qnd_CH_START = 8810;
        qnd_CH_STOP = 10790;
        qnd_CH_STEP = 2;
        break;
    case COUNTRY_JAPAN:
        qnd_CH_START = 7600;
        qnd_CH_STOP = 9000;
        qnd_CH_STEP = 1;
        break;
    default:
        break;
    }
}

/**********************************************************************
void QND_UpdateRSSIn(UINT16 ch)
**********************************************************************
Description: in case of environment changed, we need to update RSSI noise floor
Parameters:
    None
Return Value:
    None
**********************************************************************/
void QND_UpdateRSSIn(UINT16 ch)
{
    UINT8 temp;
    UINT8 v_abs;
    if (qnd_FirstScan == 0 )
    {
        temp = QND_GetRSSI(qnd_ClearChannel);

        if(temp > qnd_RSSIns)
        {
            v_abs = temp - qnd_RSSIns;
        }
        else
        {
            v_abs = qnd_RSSIns - temp;
        }
        if (v_abs< RSSINTHRESHOLD)
        {
            qnd_ClearScanFlag = 0;
        }
        else
        {
            qnd_ClearScanFlag = 1;
        }
    }
    if (qnd_ClearScanFlag||qnd_FirstScan||ch)
    {
        QNF_UpdateRSSIn(ch);
        qnd_FirstScan = 0;
    }
    return;
}

/***********************************************************************
Description: set call back function which can be called between seeking
channel
Parameters:
func : the function will be called between seeking
Return Value:
None
**********************************************************************/
void QND_SetSeekCallBack(QND_SeekCallBack func)
{
    qnd_CallBackFunc = func;
}

/***********************************************************************
UINT16 QND_RXValidCH(UINT16 freq, UINT8 db);
***********************************************************************
Description: to validate a ch (frequency)(if it's a valid channel)
Freq: specific channel frequency, unit: 10Khz
  eg: 108.00MHz will be set to 10800.
Step:
  FM:
  QND_FMSTEP_100KHZ: set leap step to 100kHz
  QND_FMSTEP_200KHZ: set leap step to 200kHz
  QND_FMSTEP_50KHZ:  set leap step to 50kHz
Return Value:
  0: not a valid channel
  other: a valid channel at this frequency
***********************************************************************/
UINT16 QND_RXValidCH(UINT16 freq, UINT8 step)
{
    UINT8 regValue;
    UINT8 timeOut;
    QNF_ConfigScan(freq, freq, step);
    QNF_SetRegBit(SYSTEM1,0x03,0x02);//channel scan mode,channel frequency is decided by internal CCA
    timeOut = 0;
    do
    {
        regValue = QND_ReadReg(SYSTEM1);
        timeOut ++;
    } while((regValue & CHSC) && timeOut<100);//when seeking a channel or time out,be quited the loop
    regValue = QND_ReadReg(0x04)& 0x08;//reading the rxcca_fail flag of RXCCA status
    if(regValue & 0x08)
    {
        return 0;
    }
    else
    {
        if(qnd_CallBackFunc)
            qnd_CallBackFunc(freq, BAND_FM);
        return freq;
    }
}

/***********************************************************************
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up);
***********************************************************************
Description: Automatically scans the frequency range, and detects the
first channel(AM or FM, it will be determine by the system mode which set
by QND_SetSysMode).
A threshold value needs to be passed in for channel detection.
Parameters:
start
Set the frequency (10kHz) where scan will be started,
eg: 76.00MHz will be set to 7600.
stop
Set the frequency (10kHz) where scan will be stopped,
eg: 108.00MHz will be set to 10800.
step
FM:
QND_FMSTEP_100KHZ: set leap step to 100kHz
QND_FMSTEP_200KHZ: set leap step to 200kHz
QND_FMSTEP_50KHZ:  set leap step to 50kHz
AM:
QND_AMSTEP_***:
db:
Set threshold for quality of channel to be searched.
up:
Set the seach direction :
Up;0,seach from stop to start
Up:1 seach from start to stop
Return Value:
The channel frequency (unit: 10kHz)
***********************************************************************/
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up)
{
    UINT8 stepValue;
    UINT16 freq = start;
    UINT16 validCH;

    up=(start <= stop) ? 1 : 0;
    QNF_SetMute(1);
    stepValue = qnd_StepTbl[step];
    QNF_SetRegBit(GAIN_SEL,0x08,0x08);//NFILT program is enabled
    QNF_SetRegBit(CCA1,0x30,0x30); //use threshold extension filter
    QNF_SetRegBit(CCA, 0x3F, ((40+db)>63)? 63:(40+db));//setting the threshold for CCA
	QNF_SetRegBit(0x39 , 0x3f, 11); //set SNR threshold to be 5
	QNF_SetRegBit(0x3A, 0xc0, 0xC0); //set CCA_NAGC to be 60ms
    do
    {
        validCH = QND_RXValidCH(freq, step);
        if (validCH == 0)
        {
             if ((!up && (freq <= stop)) || (up && (freq >= stop)))
             {
               break;
             }
             else
             {
                freq = freq + (up ? stepValue : -stepValue);
             }
        }
    } while (validCH == 0);
    QND_TuneToCH(freq);
    return freq;
}

/**********************************************************************
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up)
**********************************************************************
Description:    Automatically scans the complete FM or AM band and detects
            all the available  channels(AM or FM, it will be determine by
            the workmode which set by QND_SetSysmode). A threshold value
            needs to be passed in for the channel detection.
Parameters:
    start
        Set the frequency (10kHz) where scan will be started,
        eg: 76.00MHz will be set to 7600.
    stop
        Set the frequency (10kHz) where scan will be stopped,
        eg: 108.00MHz will be set to 10800.
    Step
        FM:
            QND_FMSTEP_100KHZ: set leap step to 100kHz
            QND_FMSTEP_200KHZ: set leap step to 200kHz
            QND_FMSTEP_50KHZ:  set leap step to 50kHz
        AM:
        QND_AMSTEP_***:
    db
        Set signal noise ratio for channel to be searched.
    up:
        Set the seach direction :
        Up;0,seach from stop to start
        Up:1 seach from start to stop

Return Value:
  The channel count found by this function
  -1: no channel found
**********************************************************************/
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up)
{
    UINT16 freq;
    UINT16 temp;
    UINT8  stepvalue;

    stop = stop > qnd_CH_STOP ? qnd_CH_STOP : stop;
    QNF_SetMute(1);
    qnd_ChCount = 0;
    up=(start<stop) ? 1 : 0;
    qnd_AutoScanAll = 1;
    stepvalue = qnd_StepTbl[step];
    QNF_SetRegBit(GAIN_SEL,0x08,0x08);//NFILT program is enabled
    QNF_SetRegBit(CCA1,0x30,0x30); //use threshold extension filter
    QNF_SetRegBit(CCA, 0x3F, ((40+db)>63)? 63:(40+db));//setting the threshold for CCA
    QNF_SetRegBit(0x39 , 0x3F, 11); //set SNR threshold to be 11
    //set CCA_NAGC to be 60ms,
    //0x00:10ms;0x40:20ms
    //0x80:40ms;0xC0:60ms
    QNF_SetRegBit(0x3A, 0xC0, 0xC0);
    for(freq=start; (up ? (freq<=stop):(freq>=stop));) //add support for both direction scan
    {
        temp = QND_RXValidCH(freq, step);
        if (temp)
        {
            qnd_ChList[qnd_ChCount++] =temp;
        }
        freq += (up ? stepvalue : -stepvalue);
    }
    QND_TuneToCH((qnd_ChCount >= 1)? qnd_ChList[0] : stop);
    qnd_AutoScanAll = 0;
    QNF_SetMute(0);
    return qnd_ChCount;
}

/************************************************************************
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option )
*************************************************************************
Description: config audio
Parameters:
  optiontype: option
    QND_CONFIG_MUTE; ‘option’control muteable, 0:mutedisable,1:mute enable
    QND_CONFIG_MONO; ‘option’control mono, 0: QND_AUDIO_STEREO,1: QND_AUDIO_STEREO
    QND_CONFIG_EQUALIZER: 'option' control the EQUALIZER,0:disable  EQUALIZER; 1: enable EQUALIZER;
    QND_CONFIG_VOLUME: 'option' control the volume gain,range : 0~83(0: -65db, 65: 0db, 83: +18db
    QND_CONFIG_BASS_QUALITY: 'option' set BASS quality factor,0: 1, 1: 1.25, 2: 1.5, 3: 2
    QND_CONFIG_BASS_FREQ: 'option' set BASS central frequency,0: 60Hz, 1: 70Hz, 2: 80Hz, 3: 100Hz
    QND_CONFIG_BASS_GAIN: 'option' set BASS control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db
    QND_CONFIG_MID_QUALITY: 'option' set MID quality factor,0 :1, 1 :2
    QND_CONFIG_MID_FREQ: 'option' set MID central frequency,0: 0.5KHz, 1: 1KHz, 2: 1.5KHz, 3: 2KHz
    QND_CONFIG_MID_GAIN: 'option' set MID control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db)
    QND_CONFIG_TREBLE_FREQ: 'option' set TREBLE central frequency,0: 10KHz, 1: 12.5KHz, 2: 15KHz, 3: 17.5KHz
    QND_CONFIG_TREBLE_GAIN: 'option' set TREBLE control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db

Return Value:
    none
**********************************************************************/
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option )
{
    UINT8 regVal;
    switch(optiontype)
    {
    case QND_CONFIG_MONO:
        if (option)
            QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_MONO);
        else
            QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_STEREO);
        break;
    case QND_CONFIG_MUTE:
        if (option)
            QNF_SetMute(1);
        else
            QNF_SetMute(0);
        break;
    case QND_CONFIG_VOLUME:             //set volume control gain
        if (option > 47)
            option = 47;
        regVal = (UINT8)(option/6);  //volume: [-42db, 0db]
        QNF_SetRegBit(0x14, 0x07, regVal);   //set analog gain
        regVal = (UINT8)(option%6);
        QNF_SetRegBit(0x14, 0x38, (UINT8)((5-regVal)<<3));   //set digital gain
        break;

    default:
        break;
    }
}

/**********************************************************************
UINT8 QND_RDSEnable(UINT8 mode)
**********************************************************************
Description: Enable or disable chip to work with RDS related functions.
Parameters:
          on: QND_RDS_ON:  Enable chip to receive/transmit RDS data.
                QND_RDS_OFF: Disable chip to receive/transmit RDS data.
Return Value:
           QND_SUCCESS: function executed
**********************************************************************/
UINT8 QND_RDSEnable(UINT8 on)
{
    UINT8 val;
    UINT8 tmp;
    UINT8 timeout = 0;

    QND_LOG("=== QND_SetRDSMode === ");
    // get last setting
    val = QND_ReadReg(SYSTEM1);
    if (on == QND_RDS_ON){
        val |= RDSEN;
    }else if (on == QND_RDS_OFF){
        val &= ~RDSEN;
    }else{
        return 0;
    }
    if (on == QND_RDS_OFF){ //To speed up the reset of the datapath
        QNF_SetRegBit(SMP_HLD_THRD,0x20,0x20);
//        QNF_SetRegBit(0x19,0x80,0x80);//neither the SCRAM1 nor priv_en bit in the QN8035
        QND_DelayMs(400);
        do{
            tmp = QND_ReadReg(STATUS2);
            QND_DelayMs(100);
            timeout++;
        }while((timeout <40)&&(tmp & 0x10));
//        QNF_SetRegBit(0x19,0x80,0);//neither the SCRAM1 nor priv_en bit in the QN8035
        QNF_SetRegBit(SMP_HLD_THRD,0x20,0);
    }
    QND_WriteReg(SYSTEM1, val);
    return 1;
}

/**********************************************************************
void QND_RDSHighSpeedEnable(UINT8 on)
**********************************************************************
Description: Enable or disable chip to work with RDS related functions.
Parameters:
  on:
    1: enable 4x rds to receive/transmit RDS data.
    0: disable 4x rds, enter normal speed.
Return Value:
  none
**********************************************************************/
void QND_RDSHighSpeedEnable(UINT8 on)
{
    QNF_SetRegBit(0x18, 0x08, on?0x08:0x00);
}

/***********************************
char QND_RDSModeDetect(void)
************************************
Description: Check the RDS mode for

Parameters:
None
Return Value:
1: 4kb/s RDS signal is detected
0: 1kb/s RDS signal is detected
-1: No RDS signal is detected
************************************/
INT8 QND_RDSModeDetect(void)
{
    //UINT8 i;
    UINT8 val;
    QND_RDSEnable(1);
    for (int i=1; i>=0; i--){
        QND_RDSHighSpeedEnable(i);
        QND_DelayMs(1500);
        val=QND_RDSDetectSignal();
        if (val & 0x10) return i;
    }
    QND_RDSEnable(0);
    return -1;
}

/**********************************************************************
UINT8 QND_DetectRDSSignal(void)
**********************************************************************
Description: detect the RDSS signal .

Parameters:
    None
Return Value:
    the value of STATUS3
**********************************************************************/
UINT8 QND_RDSDetectSignal(void)
{
    UINT8 val = QND_ReadReg(STATUS2);
    return val;
}

/**********************************************************************
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
**********************************************************************
Description: Load (TX) or unload (RX) RDS data to on-chip RDS buffer.
             Before calling this function, always make sure to call the
             QND_RDSBufferReady function to check that the RDS is capable
             to load/unload RDS data.
Parameters:
  rdsRawData :
    8 bytes data buffer to load (on TX mode) or unload (on RXmode)
    to chip RDS buffer.
  Upload:
    1-upload
    0--download
Return Value:
    QND_SUCCESS: rds data loaded to/from chip
**********************************************************************/
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
{
    UINT8 i;
    UINT8 temp;
    {
        //RX MODE
        for (i = 0; i <= 7; i++)
        {
            temp = QND_ReadReg(RDSD0 + i);
            rdsRawData[i] = temp;
        }
    }
}

/**********************************************************************
UINT8 QND_RDSCheckBufferReady(void)
**********************************************************************
Description: Check chip RDS register buffer status before doing load/unload of
RDS data.

Parameters:
    None
Return Value:
    QND_RDS_BUFFER_NOT_READY: RDS register buffer is not ready to use.
    QND_RDS_BUFFER_READY: RDS register buffer is ready to use. You can now
    load (for TX) or unload (for RX) data from/to RDS buffer
**********************************************************************/
UINT8 QND_RDSCheckBufferReady(void)
{
    UINT8 val;
    UINT8 rdsUpdated;
    rdsUpdated = QND_ReadReg(STATUS2);
    do{
        val = QND_ReadReg(STATUS2)^rdsUpdated;
    }
    while(!(val&RDS_RXUPD)) ;
    return QND_RDS_BUFFER_READY;
}


#endif

//*****************************************************************************************
#if(QnTest==1)

//UINT8   qnd_PrevMode;
//UINT8   qnd_Country  = COUNTRY_CHINA ;
//UINT16  qnd_CH_START = 7600;
//UINT16  qnd_CH_STOP  = 10800;
//UINT8   qnd_CH_STEP  = 1;
UINT8 qnd_ChipID;
UINT8 qnd_IsQN8035B;

UINT8  qnd_AutoScanAll = 0;
//UINT8  qnd_ChCount;

UINT8  qnd_div1;
UINT8  qnd_div2;
UINT8  qnd_nco;

//UINT16 qnd_ChList[QN_CCA_MAX_CH];
UINT8   qnd_StepTbl[3]={5,10,20};
QND_SeekCallBack qnd_CallBackFunc = 0;

/**********************************************************************
Description: set register specified bit
Parameters:	On: 1: mute, 0: unmute
Return Value: None
**********************************************************************/
void QNF_SetMute(UINT8 On_)
{
	if(On_){
		QNF_SetRegBit(0x4a, 0x20, 0x20);
	}else{
		QNF_SetRegBit(0x4a, 0x20, 0x00);
	}
}

/**********************************************************************
Description: get current channel frequency
Parameters: None
Return Value: channel frequency
**********************************************************************/
UINT16 QNF_GetCh()
{
    UINT8 tCh;
    UINT8  tStep;
    UINT16 ch = 0;
    // set to reg: CH_STEP
    tStep = QND_ReadReg(CH_STEP);
    tStep &= CH_CH;
    ch  =  tStep ;
    tCh= QND_ReadReg(CH_NUM);
    ch = ((UINT16)ch<<8)+tCh;
    return CHREG2FREQ(ch);
}

/**********************************************************************
Description: config start, stop, step register for FM/AM CCA or CCS
Parameters:
    start
        Set the frequency (10kHz) where scan to be started,
        e.g. 7600 for 76.00MHz.
    stop
        Set the frequency (10kHz) where scan to be stopped,
        e.g. 10800 for 108.00MHz
    step
        1: set leap step to (FM)100kHz / 10kHz(AM)
        2: set leap step to (FM)200kHz / 1kHz(AM)
        0:  set leap step to (FM)50kHz / 9kHz(AM)
Return Value:
         None
**********************************************************************/
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step)
{
    // calculate ch para
    UINT8 tStep = 0;
    UINT8 tS;
    UINT16 fStart;
    UINT16 fStop;

    fStart = FREQ2CHREG(start);
    fStop = FREQ2CHREG(stop);
    // set to reg: CH_START
    tS = (UINT8) fStart;
    QND_WriteReg(CH_START, tS);
    tStep |= ((UINT8) (fStart >> 6) & CH_CH_START);
    // set to reg: CH_STOP
    tS = (UINT8) fStop;
    QND_WriteReg(CH_STOP, tS);
    tStep |= ((UINT8) (fStop >> 4) & CH_CH_STOP);
    // set to reg: CH_STEP
    tStep |= step << 6;

    QND_WriteReg(CH_STEP, tStep);
}

/**********************************************************************
Description:    Get the RSSI value
Parameters:	Return Value:
RSSI value  of the channel settled
**********************************************************************/
/*
UINT8 QND_GetRSSI(UINT16 ch)
{
    QNF_SetRegBit(REG_REF,ICPREF,0x0a);
    QNF_ConfigScan(ch, ch, qnd_CH_STEP);
    QNF_SetCh(ch);
    if(qnd_ChipID == CHIPSUBID_QN8035A0)
    {
        //Enter CCA mode, speed up the PLL locking.
		QNF_SetRegBit(SYSTEM1, CHSC, CHSC);
        __delay_ms(QND_READ_RSSI_DELAY+90);
    }
    else if(qnd_ChipID == CHIPSUBID_QN8035A1)
    {
        //if this delay time effects the CCA time,it may be shorted to 20ms
        __delay_ms(50);
    }
	else
    {
        QNF_SetRegBit(0x55, 0x80, 0x80);
        __delay_ms(ENABLE_2K_SPEED_PLL_DELAY);
        QNF_SetRegBit(0x55, 0x80, 0x00);
    }
    QNF_SetRegBit(REG_REF,ICPREF,0x00);
    if(QND_ReadReg(RDSCOSTAS) & 0x80)
    {
        return (QND_ReadReg(RSSISIG)+9);
    }
    else
    {
        return QND_ReadReg(RSSISIG);
    }
}
*/



/**********************************************************************
Description: Set device system mode(like: sleep ,wakeup etc)
Parameters:
mode:  set the system mode , it will be set by  some macro define usually:

SLEEP : set chip to sleep mode
WAKEUP: wake up chip
RX:     set chip work on RX mode
Return Value:
None
**********************************************************************/
/*
void QND_SetSysMode(UINT16 mode)
{
    UINT8 val;
    switch(mode)
    {
        case QND_MODE_SLEEP:                       //set sleep mode
            QNF_SetRegBit(REG_DAC, 0x08, 0x00);    //make sure Power down control by FSM control
            QNF_SetRegBit(SYSTEM1, STNBY_RX_MASK, STNBY_MODE);
            break;
        case QND_MODE_WAKEUP:                      //set wakeup mode
            QNF_SetRegBit(REG_DAC, 0x08, 0x00);    //make sure Power down control by FSM control
            QNF_SetRegBit(SYSTEM1, STNBY_RX_MASK, RX_MODE);
            QNF_SetMute(1);
            __delay_ms(SLEEP_TO_WAKEUP_DELAY_TIME); //avoid noise from sleep to wakeup mode during.
            QNF_SetMute(0);
            break;
        default:
            val = (UINT8)(mode >> 8);
            if (val)
            {
                val = val >> 3;
                if(val&0x10)
                    // set to new mode if it's not same as old
                    if((QND_ReadReg(SYSTEM1) & STNBY_RX_MASK) != val)
                    {
                        QNF_SetMute(1);
                        QNF_SetRegBit(SYSTEM1, STNBY_RX_MASK, val);
                    }
            }
            break;
    }
}
*/

/**********************************************************************
void QND_SetCountry(UINT8 country)
***********************************************************************
Description: Set start, stop, step for RX and TX based on different
             country
Parameters:
country:
Set the chip used in specified country:
    CHINA:
    USA:
    JAPAN:
Return Value:
    None
**********************************************************************/
/*
void QND_SetCountry(UINT8 country)
{
    qnd_Country = country;
    switch(country)
    {
        case COUNTRY_CHINA:
            qnd_CH_START = 8750;
            qnd_CH_STOP = 10800;
            qnd_CH_STEP = 1;
            break;
        case COUNTRY_USA:
            qnd_CH_START = 8810;
            qnd_CH_STOP = 10790;
            qnd_CH_STEP = 2;
            break;
        case COUNTRY_JAPAN:
            qnd_CH_START = 7600;
            qnd_CH_STOP = 9000;
            qnd_CH_STEP = 1;
            break;
        default:
            break;
    }
}
*/
/***********************************************************************
Description: set call back function which can be called between seeking
channel
Parameters:
func : the function will be called between seeking
Return Value:
None
**********************************************************************/

void QND_SetSeekCallBack(QND_SeekCallBack func)
{
    qnd_CallBackFunc = func;
}



/***********************************************************************
Description: to validate a ch (frequency)(if it's a valid channel)
Freq: specific channel frequency, unit: 10Khz
  e.g. 108.00MHz will be set to 10800.
Step:
  FM:
  QND_FMSTEP_100KHZ: set leap step to 100kHz
  QND_FMSTEP_200KHZ: set leap step to 200kHz
  QND_FMSTEP_50KHZ:  set leap step to 50kHz
Return Value:
  0: not a valid channel
  1: a valid channel at this frequency
 -1:chip does not normally work.
***********************************************************************/
INT8 QND_RXValidCH(UINT16 freq, UINT8 step)
{
    UINT8 regValue;
    UINT8 timeOut = 40; //time out is 200ms
    UINT8 isValidChannelFlag = 0;
    QNF_ConfigScan(freq, freq, step);
    QNF_SetCh(freq);
	QND_WriteReg(0x4f, 0x80);
    regValue = QND_ReadReg(0x4f);
    regValue = (regValue >> 1);
    QND_WriteReg(0x4f, regValue);
//	TRACE("freq:%d  cap:%d\n",freq,regValue&0x3F);
	//enter CCA mode,channel index is decided by internal CCA
    QNF_SetRegBit(SYSTEM1,RXCCA_MASK,RX_CCA);
    while(1){
        regValue = QND_ReadReg(SYSTEM1);
        //if it seeks a potential channel, the loop will be quited
        if((regValue & CHSC) == 0) break;
        QND_DelayMs(5);   //delay 5ms
        //if it was time out,chip would not normally work.
        if((timeOut--) == 0) return -1;
    }
    //TRACE("CHSC:%d,timeOut:%d \n",regValue&CHSC,timeOut);
    //reading out the rxcca_fail flag of RXCCA status
    isValidChannelFlag = ((QND_ReadReg(STATUS1) & RXCCA_FAIL) ? 0:1);
    if(isValidChannelFlag){
        if(qnd_CallBackFunc)
        qnd_CallBackFunc(freq, BAND_FM);
        return 1;
    }else{
        return 0;
    }
}

#if 1
//调好了
char qn8035_rx_sch(void)
{
	QND_WriteReg(0x00,0x11);
	Fre_data+=4;
	if(Fre_data>1110){
		Fre_data = 1080;
	}
	QND_TuneToCH(Fre_data);
	if(QND_ReadReg(0x03)>65){	//接收到正确的通道
		return 1;
	}else{
		return 0;
	}
}
#else
//**********************************************************************
char qn8035_rx_sch(void)
{
	static uint8_t sch_num = 0;
	static uint8_t	RSSI_buff;					//信号强度缓存
	static uint8_t	RSSI_Result = 0xff;
	static uint16_t  Fre_buff;

	Fre_data+=1;
	if(Fre_data>=1110){
		Fre_data = 1080;
	}
	QND_TuneToCH(Fre_data);
	RSSI_buff = QND_ReadReg(0x03);
	if(RSSI_buff < RSSI_Result){ //寻找信号强度最弱的通道,即最优通道
		RSSI_Result = RSSI_buff;//记录上次强度，并与下一次比较
		Fre_buff = Fre_data;	//记录最优通道的号数
	}

	if(++sch_num>20){
		sch_num = 0;
		Fre_data = Fre_buff;
		QND_TuneToCH(Fre_data);//设置为最优通道
		RSSI_Result = 0xff;
		return 1;//搜索完毕
	}
	return 0;
}
#endif




/**********************************************************************
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up)
**********************************************************************
Description:    Automatically scans the complete FM or AM band and detects
            all the available  channels(AM or FM, it will be determine by
            the workmode which set by QND_SetSysmode). A threshold value
            needs to be passed in for the channel detection.
Parameters:
    start
        Set the frequency (10kHz) where scan will be started,
        e.g. 76.00MHz will be set to 7600.
    stop
        Set the frequency (10kHz) where scan will be stopped,
        e.g. 108.00MHz will be set to 10800.
    Step
        FM:
            QND_FMSTEP_100KHZ: set leap step to 100kHz
            QND_FMSTEP_200KHZ: set leap step to 200kHz
            QND_FMSTEP_50KHZ:  set leap step to 50kHz
    db:
    Set threshold for quality of channel to be searched,
    the range of db value:CCA_SENSITIVITY_LEVEL_0 ~ CCA_SENSITIVITY_LEVEL_19
    up:
        Set the seach direction :
        Up;0,seach from stop to start
        Up:1 seach from start to stop

Return Value:
  The channel count found by this function
  0: no channel found
**********************************************************************/
/*
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up)
{
    UINT16 freq;
    INT16 temp;
    UINT8  stepValue;

    stop = stop > qnd_CH_STOP ? qnd_CH_STOP : stop;
    QNF_SetMute(1);
    qnd_AutoScanAll = 1;
    qnd_ChCount = 0;
    up=(start<stop) ? 1 : 0;
    stepValue = qnd_StepTbl[step];
    QND_RXSetTH(db);
	freq=start;
    do
    {
        if(qnd_ChipID == CHIPSUBID_QN8035A0)
        {
            temp = QND_RXValidCH(freq, step);
            if(temp == -1)
            {
                break;
            }
            else if(temp == 1)
            {
                qnd_ChList[qnd_ChCount++] = freq;
            }
            freq += (up ? stepValue : -stepValue);
        }
        else
        {
            temp = QND_RXSeekCH(freq, stop, step, db, up);
            if(temp == -1)
            {
                break;
            }
            else if(temp)
            {
                qnd_ChList[qnd_ChCount++] = temp;
            }
            else
            {
                temp = stop;
            }
            freq = temp + (up ? stepValue : -stepValue);
        }
    }
	while((up ? (freq<=stop):(freq>=stop)) && (qnd_ChCount < QN_CCA_MAX_CH));
    QND_TuneToCH((qnd_ChCount >= 1)? qnd_ChList[0] : stop);
    qnd_AutoScanAll = 0;
    return qnd_ChCount;
}
*/
/************************************************************************
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option )
*************************************************************************
Description: config audio
Parameters:
  optiontype: option
    QND_CONFIG_MONO; ‘option’control mono, 0: stereo receive mode ,1: mono receiver mode
    QND_CONFIG_MUTE; ‘option’control mute, 0:mute disable,1:mute enable
    QND_CONFIG_VOLUME: 'option' control the volume gain,range : 0~47(-47db~0db)

Return Value:
    none
**********************************************************************/
/*
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option )
{
    UINT8 regVal;
    switch(optiontype)
    {
        case QND_CONFIG_MONO:
            if (option)
                QNF_SetRegBit(SYSTEM1,RX_MONO,RX_MONO);
            else
                QNF_SetRegBit(SYSTEM1,RX_MONO,0x00);
            break;
        case QND_CONFIG_MUTE:
            if (option)
                QNF_SetMute(1); //mute audio
            else
                QNF_SetMute(0); // disable mute audio
            break;
        case QND_CONFIG_VOLUME:
            if (option > 47)
            {
                option = 47;
            }
            if (option == 0)    //audio is muted when the volume is adjusted to minimum
            {
                QNF_SetRegBit(VOL_CTL, 0x80, 0x80); //mute audio
            }
            else
            {
                QNF_SetRegBit(VOL_CTL, 0x80, 0x00); // disable mute audio
            }
            regVal = (UINT8)(option/6);
            QNF_SetRegBit(VOL_CTL, 0x07, regVal);   //set analog gain
            regVal = (UINT8)(option%6);
            QNF_SetRegBit(VOL_CTL, 0x38, (UINT8)((5-regVal)<<3));   //set digital gain
            break;
        default:
            break;
    }
}
*/
/**********************************************************************
UINT8 QND_RDSEnable(UINT8 on)
**********************************************************************
Description: Enable or disable chip to work with RDS related functions.
Parameters:
    on:
        QND_RDS_ON:  Enable chip to receive/transmit RDS data.
        QND_RDS_OFF: Disable chip to receive/transmit RDS data.
Return Value:
           1: success
           0: failure
**********************************************************************/
UINT8 QND_RDSEnable(UINT8 on)
{
    if (on == QND_RDS_ON)
    {
        QNF_SetRegBit(SYSTEM1,0x08,0x08);//enable RDS
    }
    else if (on == QND_RDS_OFF)
    {
        QNF_SetRegBit(SYSTEM1,0x08,0x00);// disable RDS
    }
    else
    {
        return 0;
    }
    return 1;
}

/**********************************************************************
void QND_RDSHighSpeedEnable(UINT8 on)
**********************************************************************
Description: Enable or disable chip to work 4x spend RDS mode.
             notices: the FM radio stations aren't this 4x signal, it's just used
             for our chips pair working.

Parameters:
  on:
    1: enable 4x rds to receive/transmit RDS data.
    0: disable 4x rds, enter normal speed.
Return Value:
  none
**********************************************************************/
void QND_RDSHighSpeedEnable(UINT8 on)
{
    QNF_SetRegBit(INT_CTRL, 0x08, on?0x08:0x00);
}

/**********************************************************************
Description: Automatic detects the RDS signal is normal speed or 4x speed.
Parameters:
None
Return Value:
1: 4x RDS signal is detected
0: normal RDS signal is detected
-1: No RDS signal is detected
**********************************************************************/
INT8 QND_RDSModeDetect(void)
{
    INT8 i;
    UINT8 val;
    QND_RDSEnable(1);
    for (i=1; i>=0; i--)
    {
        QND_RDSHighSpeedEnable(i);
        QND_DelayMs(1500);
        val=QND_RDSDetectSignal();
        if (val & 0x10) return i;
    }
    QND_RDSEnable(0);
    return -1;
}

/**********************************************************************
UINT8 QND_DetectRDSSignal(void)
**********************************************************************
Description: detect the RDSS signal .

Parameters:
    None
Return Value:
    the value of STATUS2
    &0x10: Status of synchronization
    &0x0f: Status of block errors
    &0x40: Status of ‘E’ block (MMBS block)
**********************************************************************/
UINT8 QND_RDSDetectSignal(void)
{
    UINT8 val = QND_ReadReg(STATUS2);
    return val;
}

/**********************************************************************
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
**********************************************************************
Description: Load (TX) or unload (RX) RDS data to on-chip RDS buffer.
             Before calling this function, always make sure to call the
             QND_RDSBufferReady function to check that the RDS is capable
             to load/unload RDS data.
Parameters:
  rdsRawData :
    8 bytes data buffer to load (on TX mode) or unload (on RXmode)
    to chip RDS buffer.
  Upload:
    1-upload
    0--download
Return Value:
    None
**********************************************************************/
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
{
    UINT8 i;
    UINT8 temp;
    {
        for (i = 0; i <= 7; i++)
        {
            temp = QND_ReadReg(RDSD0 + i);
            rdsRawData[i] = temp;
        }
    }
}

/**********************************************************************
UINT8 QND_RDSCheckBufferReady(void)
**********************************************************************
Description: Check chip RDS register buffer status before doing unload of
RDS data.

Parameters:
    None
Return Value:
    QND_RDS_BUFFER_NOT_READY: RDS register buffer is not ready to use.
    QND_RDS_BUFFER_READY: RDS register buffer is ready to use. You can now
    unload (for RX) data from RDS buffer
**********************************************************************/
UINT8 QND_RDSCheckBufferReady(void)
{

    UINT8 val;
    UINT8 rdsUpdated;
    UINT8 timeOut = 100;

    rdsUpdated = QND_ReadReg(STATUS2);
    while(1)
    {
        val = QND_ReadReg(STATUS2)^rdsUpdated;
        if(val&RDS_RXUPD) return QND_RDS_BUFFER_READY;
        QND_DelayMs(1);
    	if((timeOut--) == 0) return QND_RDS_BUFFER_NOT_READY;
    }
}

/**********************************************************************
Description: set channel frequency
Parameters:	freq:  channel frequency to be set
Return Value:	None
**********************************************************************/
void QNF_SetCh(UINT16 freq)
{
    UINT8 tStep;
    UINT8 tCh;
    UINT16 f;
//    UINT16 pll_dlt;

    if(freq == 8550){
        QND_WriteReg(XTAL_DIV1, QND_XTAL_DIV1_855);
		QND_WriteReg(XTAL_DIV2, QND_XTAL_DIV2_855);
		QND_WriteReg(NCO_COMP_VAL, 0x69);
		freq = 8570;
    }else{
        QND_WriteReg(XTAL_DIV1, qnd_div1);
        QND_WriteReg(XTAL_DIV2, qnd_div2);
        QND_WriteReg(NCO_COMP_VAL, qnd_nco);
    }
	//Manually set RX Channel index
    QNF_SetRegBit(SYSTEM1, CCA_CH_DIS, CCA_CH_DIS);
    f = FREQ2CHREG(freq);
    // set to reg: CH
    tCh = (UINT8) f;
    QND_WriteReg(CH_NUM, tCh);
    // set to reg: CH_STEP
    tStep = QND_ReadReg(CH_STEP);
    tStep &= ~CH_CH;
    tStep |= ((UINT8) (f >> 8) & CH_CH);
    QND_WriteReg(CH_STEP, tStep);
}


/***********************************************************************
Description: Setting the threshold value of automatic scan channel
th: Setting threshold for quality of channel to be searched,
    the range of th value:CCA_SENSITIVITY_LEVEL_0 ~ CCA_SENSITIVITY_LEVEL_9
Return Value: None
***********************************************************************/
void QND_RXSetTH(UINT8 th)
{
    UINT8 rssiTH;
    UINT8 snrTH;
    UINT16 rssi_snr_TH;
    UINT16 rssi_snr_TH_tbl [10] = { CCA_SENSITIVITY_LEVEL_0,CCA_SENSITIVITY_LEVEL_1,
                                    CCA_SENSITIVITY_LEVEL_2,CCA_SENSITIVITY_LEVEL_3,
                                    CCA_SENSITIVITY_LEVEL_4,CCA_SENSITIVITY_LEVEL_5,
                                    CCA_SENSITIVITY_LEVEL_6,CCA_SENSITIVITY_LEVEL_7,
                                    CCA_SENSITIVITY_LEVEL_8,CCA_SENSITIVITY_LEVEL_9
                                  };

    rssi_snr_TH = rssi_snr_TH_tbl[th];
    rssiTH = (UINT8) (rssi_snr_TH >> 8);
    snrTH = (UINT8) (rssi_snr_TH & 0xff);
    QND_WriteReg(0x4f, 0x00);//enable auto tunning in CCA mode
    QNF_SetRegBit(REG_REF,ICPREF,0x0a);
    QNF_SetRegBit(GAIN_SEL,0x08,0x08);//NFILT program is enabled
	//selection filter:filter3
    QNF_SetRegBit(CCA1,0x30,0x30);
    //Enable the channel condition filter3 adaptation,Let ccfilter3 adjust freely
    QNF_SetRegBit(SYSTEM_CTL2,0x40,0x00);
    QND_WRITE(CCA_CNT1,0x00);
    QNF_SetRegBit(CCA_CNT2,0x3f,0x03);
	//selection the time of CCA FSM wait SNR calculator to settle:20ms
	//0x00:	    20ms(default)
	//0x40:	    40ms
	//0x80:	    60ms
	//0xC0:	    100m
    QNF_SetRegBit(CCA_SNR_TH_1 , 0xc0, 0x00);
    //selection the time of CCA FSM wait RF front end and AGC to settle:20ms
    //0x00:     10ms
	//0x40:     20ms(default)
    //0x80:     40ms
	//0xC0:     60ms
    QNF_SetRegBit(CCA_SNR_TH_2, 0xc0, 0x40);
    QNF_SetRegBit(CCA, 0x3f, rssiTH);  //setting RSSI threshold for CCA
	QNF_SetRegBit(CCA_SNR_TH_1 , 0x3f, snrTH); //setting SNR threshold for CCA
}

/***********************************************************************
Description: Automatically scans the frequency range, and detects the
first channel(FM, it will be determine by the system mode which set
by QND_SetSysMode).
A threshold value needs to be passed in for channel detection.
Parameters: start
Set the frequency (10kHz) where scan will be started,
e.g. 76.00MHz will be set to 7600.
stop: Set the frequency (10kHz) where scan will be stopped,
e.g. 108.00MHz will be set to 10800.
step: FM:
QND_FMSTEP_100KHZ: set leap step to 100kHz
QND_FMSTEP_200KHZ: set leap step to 200kHz
QND_FMSTEP_50KHZ:  set leap step to 50kHz
db: Set threshold for quality of channel to be searched,
the range of db value:CCA_SENSITIVITY_LEVEL_0 ~ CCA_SENSITIVITY_LEVEL_19
up: Set the seach direction :
Up;0,seach from stop to start
Up:1 seach from start to stop
Return Value:
    The channel frequency (unit: 10kHz)
   -1:Chip does not normally work.
***********************************************************************/
INT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up)
{

    INT16 freq = start;
    INT8 validCH;
    UINT8 stepValue;
    UINT16 pStart = start;
    UINT16 pStop = stop;
    UINT8 regValue;
    UINT16 timeOut;
    UINT8 isValidChannelFlag = 0;

    up=(start <= stop) ? 1 : 0;
    stepValue = qnd_StepTbl[step];
    if(qnd_ChipID == CHIPSUBID_QN8035A0){
        QNF_SetMute(1);
        QND_RXSetTH(db);
        do{
            validCH = QND_RXValidCH(freq, step);
            if (validCH == 0){
                 if ((!up && (freq <= stop)) || (up && (freq >= stop))){
                   break;
                 }else{
                    freq = freq + (up ? stepValue : -stepValue);
                 }
            }else if(validCH == -1){
                return -1;
            }
        } while (validCH == 0);
        QND_TuneToCH(freq);
    }else{
        if(qnd_AutoScanAll == 0){
            QNF_SetMute(1);
            QND_RXSetTH(db);
        }
        do{
            QNF_ConfigScan(pStart, pStop, step);
        	//enter CCA mode,channel index is decided by internal CCA
            QNF_SetRegBit(SYSTEM1,RXCCA_MASK,RX_CCA);
            timeOut = 400;  //time out is 2S
           	while(1){
                regValue = QND_ReadReg(SYSTEM1);
                //if it seeks a potential channel, the loop will be quited
                if((regValue & CHSC) == 0) break;
                QND_DelayMs(10);   //delay 5ms
                //if it was time out,chip would not normally work.
                if((timeOut--) == 0) return 8750;
            }
            //TRACE("CHSC:%d,timeOut:%d \n",regValue&CHSC,timeOut);
            //reading out the rxcca_fail flag of RXCCA status
            isValidChannelFlag = (QND_ReadReg(STATUS1) & RXCCA_FAIL ? 0:1);
            freq = QNF_GetCh();
			if(isValidChannelFlag == 0){
                pStart = freq + (up ? stepValue : -stepValue);
            }
        }while ((isValidChannelFlag == 0) && (up ? (pStart<=pStop):(pStart>=pStop)));
        if(isValidChannelFlag){
            if(qnd_CallBackFunc)
                qnd_CallBackFunc(freq, BAND_FM);
        }else{
            freq = 8750;
        }
        if(qnd_AutoScanAll == 0){
            QND_TuneToCH(freq ? freq: stop);
        }
    }
    return freq;

}

//***************************************向上搜
void QN8035_RXSeekFreq(UINT8 UpDn)
{
	INT16 Freq = QND_RXSeekCH(8750, 10800, QND_FSTEP_100KHZ, 4, UpDn);
	USER_DBG_INFO("QND FREQ:%d\n",Freq);
	Fre_data = Freq;
    Show_Freq();
}

//***************************************向下搜
void QN8035_RXSeekFreqdown(UINT8 UpDn)
{
	INT16 Freq = QND_RXSeekCH(9300, 8750, QND_FSTEP_100KHZ, 4, UpDn);
	USER_DBG_INFO("QND FREQ:%d\n",Freq);
	Fre_data = Freq;
    Show_Freq();
}

/**********************************************************************
Description: set to SNR based MPX control. Call this function before
             tune to one specific channel
Parameters:	None
Return Value:	None
**********************************************************************/
void QNF_RXInit()
{
    QNF_SetRegBit(0x1b,0x08,0x00);  //Let NFILT adjust freely
    QNF_SetRegBit(0x2c,0x3f,0x12);
    QNF_SetRegBit(0x1d,0x40,0x00);
    QNF_SetRegBit(0x41,0x0f,0x0a);
    QND_WriteReg(0x45,0x50);
    QNF_SetRegBit(0x3e,0x80,0x80);
    QNF_SetRegBit(0x41,0xe0,0xc0);
}

/**********************************************************************
Description: Tune to the specific channel. call QND_SetSysMode() before
call this function
Parameters: ch
Set the frequency (10kHz) to be tuned,
e.g. 101.30MHz will be set to 10130.
None
**********************************************************************/
void QND_TuneToCH(UINT16 ch)
{
	UINT8 reg;
	//Show_Freq();   //显示当前信道

	UINT8 imrFlag = 0;

	QNF_SetRegBit(REG_REF,ICPREF,0x0a);
	QNF_RXInit();  //调到特定频道需要设置SNR

//	QNF_SetMute(1);
	if(qnd_IsQN8035B == 0x13){
		if((ch==7630)||(ch==8580)||(ch==9340)||(ch==9390)||(ch==9530)||(ch==9980)||(ch==10480))	{
			imrFlag = 1;
		}
	}else if((qnd_ChipID == CHIPSUBID_QN8035A0)||(qnd_ChipID == CHIPSUBID_QN8035A1)){
		if((ch==6910)||(ch==7290)||(ch==8430)){
			imrFlag = 1;
		}else if(qnd_ChipID == CHIPSUBID_QN8035A1){
			if((ch==7860) || (ch==10710)){
				imrFlag = 1;
			}
		}
	}
	if(imrFlag){
		QNF_SetRegBit(CCA, IMR, IMR);
	}else{
		QNF_SetRegBit(CCA, IMR, 0x00);
	}

    //QNF_ConfigScan(ch, ch, qnd_CH_STEP);
    QNF_SetCh(ch);      //设置特定频道

//	if((qnd_ChipID == CHIPSUBID_QN8035A0)||(qnd_ChipID == CHIPSUBID_QN8035A1)){
        QNF_SetRegBit(SYSTEM1, CHSC, CHSC);       //扫描已占用信道并接收
//	}else{
//		QNF_SetRegBit(0x55, 0x80, 0x80);
//		QND_DelayMs(ENABLE_2K_SPEED_PLL_DELAY);	//delay 20ms
//		QNF_SetRegBit(0x55, 0x80, 0x00);
//	}

    //Auto tuning
    QND_WriteReg(0x4f, 0x80);
    reg = QND_ReadReg(0x4f);
    reg >>= 1;
    QND_WriteReg(0x4f, reg);

//    if(qnd_ChipID == CHIPSUBID_QN8035A0){
//    	QND_DelayMs(CH_SETUP_DELAY_TIME+300);	//delay 200ms
//    }else{
    	QND_DelayMs(CH_SETUP_DELAY_TIME);		//delay 200ms
//    }

    QNF_SetRegBit(REG_REF,ICPREF,0x00);
    //QNF_SetMute(0);
}

#endif


//***********************************************************
void Show_Freq(void)
{
	extern void LCD_ShowNum(uint8_t Xx,uint8_t Nx);
	extern void LCD_ShowDot(uint8_t n, uint8_t OnOff);
	uint16_t ShowFreq = Fre_data/10;     //Fre_data->8750,10800，数码管四位，缩小10倍处理
	if(ShowFreq>1000)
		LCD_ShowNum(0,ShowFreq%10000/1000);
	else
		LCD_ShowNum(0,17);
	LCD_ShowNum(1,ShowFreq%1000/100);
	LCD_ShowNum(2,ShowFreq%100/10);
	LCD_ShowNum(3,ShowFreq%10);
	LCD_ShowDot(7, 1);   //打开小数点
}



//************************************************************************************
void QN8035_Init(void)
{
	UINT8 aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,qq,ll,mm;
    QND_WriteReg(0x00, 0x81);        //复位所有寄存器
    QND_DelayMs(10);                 //等待复位完成

    qnd_ChipID = QND_ReadReg(CID1)& 0x03;
    qnd_IsQN8035B = QND_ReadReg(0x58) & 0x1f;
    /*********User sets chip working clock **********/
    QND_WriteReg(0x58,0x13);
    aa = QND_ReadReg(0x58);//++test
    //Following is where change the input clock type.as crystal or oscillator.
    QNF_SetRegBit(0x58,0x80,QND_CRYSTAL_CLOCK);
    bb = QND_ReadReg(0x58);//++test
    //Following is where change the input clock wave type,as sine-wave or square-wave.
//    QNF_SetRegBit(0x01,0x80,QND_SINE_WAVE_CLOCK);//QND_DIGITAL_CLOCK
    //Following is where change the input clock frequency.
//    QND_WriteReg(XTAL_DIV0, QND_XTAL_DIV0);
//    QND_WriteReg(XTAL_DIV1, QND_XTAL_DIV1);
//    QND_WriteReg(XTAL_DIV2, QND_XTAL_DIV2);
    QND_DelayMs(10);
    /********User sets chip working clock end ********/
    QND_WriteReg(0x54, 0x47);//mod PLL setting
    cc = QND_ReadReg(0x54);//++test
    QND_WriteReg(SMP_HLD_THRD, 0xc4);//select SNR as filter3,SM step is 2db
    dd = QND_ReadReg(SMP_HLD_THRD);//++test
    QNF_SetRegBit(0x40,0x70,0x70);
    ee = QND_ReadReg(0x40);//++test
    QND_WriteReg(0x33, 0x9c);//set HCC Hystersis to 5db
    ff = QND_ReadReg(0x33);//++test
    QND_WriteReg(0x2d, 0xd6);//notch filter threshold adjusting
    gg = QND_ReadReg(0x2d);//++test
    QND_WriteReg(0x43, 0x10);//notch filter threshold enable
    hh = QND_ReadReg(0x43);//++test
    QNF_SetRegBit(SMSTART,0x7f,SMSTART_VAL);
    ii = QND_ReadReg(SMSTART);//++test
    QNF_SetRegBit(SNCSTART,0x7f,SNCSTART_VAL);
    jj = QND_ReadReg(SNCSTART);//++test
    QNF_SetRegBit(HCCSTART,0x7f,HCCSTART_VAL);
    qq = QND_ReadReg(HCCSTART);//++test

	QNF_SetRegBit(0x47,0x0c,0x08);
	ll = QND_ReadReg(0x47);//++test
    //these variables are used in QNF_SetCh() function.
    qnd_div1 = QND_ReadReg(XTAL_DIV1);
    qnd_div2 = QND_ReadReg(XTAL_DIV2);
    qnd_nco = QND_ReadReg(NCO_COMP_VAL);

    USER_DBG_INFO("==== QND_ChipID:%02X  Is8035B:%02X\n",qnd_ChipID, qnd_IsQN8035B);

    //test
    USER_DBG_INFO("==== reg58H:%02X  reg58H:%02X\n",aa, bb);
    USER_DBG_INFO("==== reg54H:%02X  regSMP_HLD_THRD:%02X\n",cc, dd);
    USER_DBG_INFO("==== reg40H:%02X  reg33H:%02X\n",ee, ff);
    USER_DBG_INFO("==== reg2dH:%02X  reg43H:%02X\n",gg, hh);
    USER_DBG_INFO("==== regSNCSTART:%02X  regSNCSTART:%02X\n",ii, jj);
    USER_DBG_INFO("==== regHCCSTART:%02X  reg47H:%02X\n",qq, ll);
    USER_DBG_INFO("==== qnd_div1:%02X  qnd_div2:%02X  qnd_nco:%02X\n" ,qnd_div1, qnd_div2,qnd_nco);
   	QND_WriteReg(0x00,0x11);
   	mm =QND_ReadReg(0x00);//++test
   	USER_DBG_INFO("==== reg00H:%02X\n",mm);
   	Fre_data = 8750;          //初始化信道为87.5 *100
    QND_TuneToCH(Fre_data);   //设置FM接收频点

}


//**********************************************
UINT8 qn8035_read_stereo_status(void)
{
	return QND_ReadReg(0x04)&0x01;	//读取stereo;	//return 1:mono 0:stereo
}

//***********************************************************
void QN8035_Main(void)
{
	static uint8_t rssiR=0;
	static uint8_t rssi=0;
	static uint8_t rssi_num;
	static uint16_t rssi_val;

	uint8_t rssiW = QND_ReadReg(0x03);
	if(rssiW==0) rssiW = rssiR;

	rssi_val += rssiW;
	rssi_num++;
	if(rssi_num==8){
		rssi = (rssi_val>>3);
		rssi_val = 0;
		rssi_num=0;
	}

//if(rssi<85)//
	//信号强度不足和没有立体声都静音
	//if((rssi<85)||(qn8035_read_stereo_status()==1))
	if((rssi<70)||(qn8035_read_stereo_status()==1)){
//	if(rssi<65){
//		mute_control(1);//静音
		QNF_SetMute(1);
	}else{
//		mute_control(0);
		QNF_SetMute(0);
	}

	if(abs(rssiR-rssi)>2){      //abs函数：取绝对值
		rssiR=rssi;
		USER_DBG_INFO("==== qnd_rssi:%d   %02X\n",rssi, QND_ReadReg(0x4A));
	}


#if 0
	if(Flag_key){
		Flag_key = 0;
		key_read();
		if(key_an){
			key_an = 0;
			Flag_sch_status = 1;
		}
	}

	if((Flag_fre_sch)&&(Flag_sch_status==1)){
		mute_control(1);//
		Flag_fre_sch = 0;
		GIE = 0;
		if(qn8035_rx_sch()){//找到最好的通道
			Flag_sch_status = 2;
			//
		}
		GIE = 1;
	}
	if((Flag_ir_send_fre)&&(Flag_sch_status==2)){
		static uint8_t ir_send_num = 0;
		Flag_ir_send_fre = 0;
		GIE = 0;
		ir_send_data(Fre_data-1000,12);//12是测试
		if(QND_ReadReg(0x03)>90){
			//接收成功
			Flag_sch_status = 0;
			Write_8bit_Data_Eeprom_Pro(0x00,Fre_data-1000);
			mute_control(0);//
		}

		if(++ir_send_num>8)//超过8次不成功
		{
			ir_send_num = 0;
			Flag_sch_status = 0;//退出搜台模式
			//Fre_data = Read_8bit_Data_Eeprom_Pro(0x00);
			mute_control(0);//
		}
#endif

}
#endif
