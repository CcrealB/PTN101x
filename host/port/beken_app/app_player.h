#ifndef _APP_PLAYER_H_
#define _APP_PLAYER_H_

#include "ff.h"
#include "mp3common.h"
#include "app_debug.h"
#if (CONFIG_APP_MP3PLAYER == 1)

#if MCU_DBG_APP_PLAYER_ENABLE
#define DEBUG_APP_PLAYER_PRINTF(fmt,...)              do{if(mcu_dbg_ctrl & MCU_DBG_APP_PLAYER_ENABLE_MASK)MCU_DBG("BLE","_WATCH",fmt,##__VA_ARGS__);}while(0)
#else
#define DEBUG_APP_PLAYER_PRINTF(fmt,...)
#endif
enum
    {
        APP_PLAYER_MODE_PLAY_ALL_CYCLE = 0, // 
        APP_PLAYER_MODE_PLAY_ONE = 2,        //
        APP_PLAYER_MODE_PLAY_RANDOM = 3,     //
        APP_PLAYER_MODE_END
    };

enum
    {
        BUTTON_PLAYER_NONE = 0,
        BUTTON_PLAYER_PLAY_PAUSE,
        BUTTON_PLAYER_NEXT,
        BUTTON_PLAYER_PREV,
        BUTTON_PLAYER_VOL_P,
        BUTTON_PLAYER_VOL_M,
        BUTTON_PLAYER_VOL_MUTE,

#if (defined(CONFIG_BLUETOOTH_HFP))
        BUTTON_MP3_HFP_ACK,
        BUTTON_MP3_HFP_REJECT,
#endif

        BUTTON_PLAYER_PLAY_MODE_SET,
        BUTTON_PLAYER_MODE_CHANGE,
        BUTTON_PLAYER_END
    };


typedef enum{
    APP_PLAYER_PLAY,
    APP_PLAYER_PAUSE,
    APP_PLAYER_MUSIC_SWITCH,
    APP_PLAYER_VOL_CHANGE,
    APP_PLAYER_MODE_CHANGE,
    APP_PLAYER_MUSIC_NAME,
}APP_PLAYER_STATUS;
    
/**
 * @brief app_player_status_callback_fxn
 * @param status    : app player status
 * @param arg       : status = APP_PLAYER_PLAY          -> arg is NULL
 *                    status = APP_PLAYER_PAUSE        -> arg is NULL
 *                    status = APP_PLAYER_MUSIC_SWITCH -> arg is NULL
 *                    status = APP_PLAYER_VOL_CHANGE   -> arg is volume value
 *                    status = APP_PLAYER_MODE_CHANGE  -> arg is APP_PLAYER_MODE_PLAY_ALL_CYCLE / APP_PLAYER_MODE_PLAY_ONE / APP_PLAYER_MODE_PLAY_RANDOM
  *                   status = APP_PLAYER_MODE_CHANGE  -> arg is filename
 */
typedef void (*app_player_status_callback_fxn)(APP_PLAYER_STATUS status,void *arg);


#define MAX_DIR_DEPTH             6  // no large than 7, otherwise the break point can not restroe correctly.(see driver_flash.c for detail)
#define MUSIC_END_FADE_OUT 10 //ms

void app_player_button_setting(void);
void app_player_init( void);
//void app_player_uninit( int mode );
//void app_player_file_info_init( void );
//int app_player_state_pause( void );
//void player_start_first_running(void);
void MP3_CALL app_player_play_func( void );

int app_player_button_play_pause( void );
int app_player_button_next( void );
int app_player_button_prev( void );
int app_player_button_nextDir( void );
int app_player_button_prevDir(void);
int app_player_button_setvol_up( void );
int app_player_button_setvol_down( void );
uint32 app_check_mp3_music_type(void);
void app_player_play_pause_caller( int play_pause);
int player_get_play_status( void );

void app_playwav_resumeMp3(uint32 fieldId);
void app_player_restore_current_music(void);
void app_player_save_play_status( void );
uint8_t app_player_get_save_play_status( void );
void app_player_halt(uint8_t savebp);
void app_player_restore_from_halt(void);
void app_player_report_status_to_dsp(APP_PLAYER_STATUS sta,void*argv);
void app_player_set_vol( uint8_t vol );
uint8_t app_player_is_active(void);
void app_player_set_fixed_dir(uint16_t dirIdx, uint8_t en);//设置播放固定的目录，退出播放器会自动清除此设置

int get_music_DirNumTotal(void); //获取文件系统上的所有含音乐文件的目录数量
int get_music_dFileNumTotal(int DirIdx); //获取文件系统上指定音乐目录的音乐文件数量
int get_music_gFileNumTotal(void);//获取文件系统上的所有音乐文件数量
int get_cur_music_DirIdx(void);//获取当前播放的目录索引
int get_cur_music_dFileIdx(void);//获取当前播放的目录中的文件索引
int get_cur_music_gFileIdx(void);//获取当前播放的全局文件索引
char* get_cur_music_FileName(void);//获取当前播放的音乐文件名

uint8 get_fat_ok_flag(void);
uint8 get_disk_type(void);
void *Get_File_From_Number(int number);
void *Get_Cur_Dir_file(uint16 dir_num,uint16 song_idx);
void app_player_wait_async_idle(const char *call_func_name);


void MP3_CALL Convert_Mono(short *buffer, int outputSamps);
void ClearMP3(MP3DecInfo *mp3DecInfo);
int app_player_unload(void);
int check_async_status(void);

uint8_t get_app_player_volume(void);

void app_player_callback_register(app_player_status_callback_fxn cb);
void set_app_player_play_mode(uint8_t mode);
uint8_t get_app_player_play_mode(void);
int app_player_select_music(int index);





void set_app_player_flag_hw_attach(void);
void set_app_player_flag_hw_detach(void);
#if CALC_PLAY_TIME
uint32_t app_player_get_playtime(void);
uint32_t app_player_get_totaltime(void);
#endif


int app_player_breakpoint_get(uint32_t* p_addr, int* p_size);
int app_player_breakpoint_set(void* addr, int size);

extern uint8 appPlayerPlayMode;	//yuan++

/* ------------------------------------------------- *///debug show
void app_player_debug_show();

#endif

#endif
