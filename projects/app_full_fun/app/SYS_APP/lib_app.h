#ifndef _LIB_APP_H_
#define _LIB_APP_H_
//==== 20230908 ====

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>


char* lib_demo_version_get(void);
void lib_demo_init(void* send_func);
void lib_demo_exec(uint8_t *buf, int len);

int lib_effet_proc(int len);
int lib_effet_proc1(uint8_t *p_buf, int buf_size);

/* demo use:
#include "lib_app.h"
void main(void)
{
    lib_demo_init(com_cmd_send_proc);
    USER_DBG_INFO("====%s\n", lib_demo_version_get());
    while(1)
    {
        char buf[128];
        lib_demo_exec((uint8_t*)buf, 6);
    }
}
*/

void ByPass(uint16_t OnOff, uint16_t sid);
void IOGainSetDamp(uint8_t sid, float gain, uint8_t time);
void IOGainSet(uint8_t sid, float gain);
void MicDryVolSet(float gain, uint8_t ch);
void RevPreDelaySet(int32_t delay);
void RevLPassSet(int32_t freq);
void RevRepSet(float Decay);
void RevVolSet(float gain, uint8_t ch);
void RevDfusSet(uint8_t sid, float val);
void RevHFreqDampSet(float val);
void RevExcurSet(float val);
void K_EchoDelayTimeSet(int32_t DelayTime);
void K_EchoRepSet(float Decay);
void EchoVolSet(float gain, uint8_t ch);
void K_MicNoiseGateSet(float gain);
void K_MusNoiseGateSet(float gain);
void DuckingSet(uint8_t CH, float igain, float Htime, float FadeDB);
void FreqshiftSet(int32_t freq);
void VoiceCanSet(float SvsGain, float SvsFc0, float SvsFc1);
void VoiceCan(uint8_t OnOff);
void KeyShiftSet(int32_t Pitch);
void K_pshiftSet(int32_t Pitch);
void MusDrcSet(float compR, float compW, float compAtime, float compRtime);
void MusDrcCompTSet(float compT);
void MicDrcSet(float compR, float compW, float compAtime, float compRtime);
void MicDrcCompTSet(float compT);
void K_EqSet(uint8_t Id_type, uint8_t ch, float eq_G, float eq_Q, float eq_Wc, uint8_t eq_type);
void K_DreSet(uint8_t type, float Atime, float Rtime, float Low_lev, float High_lev);

//****　ＡＣＭ８５ｘｘ　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void ACM_EQ_SET(uint8_t type, uint8_t ch, uint16_t freq, uint16_t q, int gain);
void ACM_DRB_SET(char LowLevel, char HighLevel);
void K_OutEqMixSet(uint8_t type, uint8_t sw);
void ACM_VOL(uint8_t ch, int gain, uint8_t Inv);
void ACM86_Mute(uint8_t OnOff);
void ACM86_PEQ_ONOFF(uint8_t PEQEnable);
void ACM86_DRB_ONOFF(uint8_t DRB_Enable);
void ACM86_InputMixer(float L2L, float R2L, float L2R, float R2R);
void ACM86_SpkOutputMixer(float L2L, float R2L, float S2L, float L2R, float R2R, float S2R);
void ACM86_SdoOutputMixer(float L2L, float R2L, float S2L, float L2R, float R2R, float S2R);
void SetI2CAddr(uint8_t addr);
void ACM_ClassGaie(float Dgain, float Again);


//******************************************
#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t
//=======================================
typedef struct
{
	u8 LedChF;
	u8* RGB_AUTO;
	u8* W_Y;
	u8*	W_Y_R;		// 亮度 暫存
	u8* RGB_MODE;
	u8* RGB_MODE_R;
	u8* RGB_SP;	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
	u8*	RGB_SP_R;
	u8* RGB_EF;	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
	u8*	RGB_EF_R;
	u8*	RGB_AEF;
	u8*	RGB_AEF_R;
	u8*	LED_RR;		// RGB 顯示 暫存
	u8*	LED_GG;
	u8*	LED_BB;
	u8*	LED_FR;
	const u8*	RGB_EF_N;
	u16* LED_LEN;
	u32* RGB_TIME;
	u8* WFlash_Time;
	u8* ef_cont;
	u8* LED_RGB;
	u8* CH_Level;
	const u8* AudiOffEf;
	const u8* RGB_Color;
	u32(*Tempo_ms_cont)(void);
}RGB_EF_INF_TAB;

void AUDIO_TEMPO_FUN(u16 *Level, u32 time);
void RGB_LED_PROG_FUN(RGB_EF_INF_TAB *EF);

#endif /* _LIB_APP_H_ */
