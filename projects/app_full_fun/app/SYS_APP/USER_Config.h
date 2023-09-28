
/*************************************************************************
  USER 個別方案  外掛程序 include

**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "lib_app.h"
#include "sys_types.h"
#include "app_prompt_wav.h"
#include "driver_gpio.h"
#include "api_usb.h"

#include "driver_beken_includes.h"
#include "app_beken_includes.h"

#include "driver_flash.h"	// user flash Read/Write

void os_delay_us(uint32_t usec);

#include "sys_irq.h"
#include "u_debug.h"
#include "u_com.h"
#include "u_config.h"
#include "karacmd.h"
#include "UpComputer.h"
#include "udrv_misc.h"

//==========================================
#ifdef u_key
	#include "u_key.h"
#endif
#ifdef udrv_saradc
	#include "udrv_saradc.h"
#endif
#ifdef drv_spi
	#include "drv_spi.h"
#endif
#ifdef _Hi2c
	#include "_Hi2c.h"
#endif
#ifdef OLED128x64
	#include ".\OLED\OLED.h"
#endif
#ifdef ACM86xxId1
	#include ".\ACM86xx\ACM86xx.h"
#endif
#ifdef BK9532
	#include ".\WMIC_BK\BK9532.h"
#endif
#ifdef KT56
	#include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
#endif
#ifdef QN8035
	#include ".\FM_QNxx\QN8035.h"
#endif

#ifdef ST7789V
	#include ".\LCD_ST7789V\ST7789V.h"
#endif

#if defined(TM1668) || defined(TM1629)
	#include ".\TM16xx\TM16xx.h"
#endif

#ifdef AMG8802
	#include"AMG8802\app.h"
#endif

#ifdef	CONFIG_AUD_AMP_DET_FFT
	#include".\RGB_LED\LED_EF_FUN.h"
#endif

#ifdef EC11_Encoder
	void Encoder_EC11_Init(unsigned char Set_EC11_TYPE);
	char Encoder_EC11_Scan();
#endif

//********************************************************
extern uint8_t g_dsp_log_on_fg;
extern uint8_t g_mcu_log_on_fg;
extern uint8_t PowerDownFg;
extern const int8_t MusVol_TAB[33];
extern const int8_t MicVol_TAB[33];

void GroupRead(uint8_t Group, uint8_t GrType);
void GroupWrite_1(uint8_t *EF_RAM, uint8_t Group, uint8_t type);
void GroupWrite(uint8_t Group, uint8_t GrType);
void UserFlash_Init();
void ClearUserFlash();

//void Music_VolSet(float val);
void Mic_VolSet(float val);
void Out_VolSet(float val);

//void Audio_Def();
void AngInOutGainSet(int32_t Gain);
void EF_ClewToneGr(uint8_t Gr);
void EQ_ClewToneGr(uint8_t Gr);

void PlayWaveStop(uint16_t id);
void DisPlay_UpData(uint8_t Page, uint8_t Id2);

void UserFlash4K_read();
void UserFlash4K_Write(uint8_t *buff);
int system_work_mode_change_button(void);
int app_button_sw_action( uint8_t button_event );

#if	(KbonNUM)
	void Knob_function();
#endif
//======================================================================
#ifdef	RECED_EN
void recorder_start(uint8_t disk_type, uint8_t en);
uint8_t recorder_is_working(void);
uint8_t rec_disk_type_get(void);

uint16_t rec_file_total_num_get(void);  //获取总的录音文件数量（需要初始化文件系统）
uint8_t rec_file_name_get_by_idx(uint16_t rec_file_idx, char* fn);  //根据索引获取文件名
void rec_file_play_by_idx(uint16_t rec_file_idx);//根据索引播放录音文件

void user_RecPlayWork(void);
void user_RecWork(void);
#endif

