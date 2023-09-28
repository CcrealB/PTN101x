#ifndef _APP_PROMPT_WAV_H_
#define _APP_PROMPT_WAV_H_

#include "config.h"
//#define WAV_DEBUG

#ifdef WAV_DEBUG
    #define WAV_PRT      os_printf
#else
    #define WAV_PRT      os_null_printf
#endif

#ifdef USER_KARAOK_MODE
    // #define WAV_FILL_48K_EN     1 //wav fill with stereo 48k24bit

    #define wav_aud_mic_open(...)            //aud_mic_open()
    #ifdef AUD_WAV_TONE_SEPARATE
        #define wav_aud_dac_config(...)
        #define wav_aud_dac_open()
        #define wav_aud_dac_close()
    #else
        #define wav_aud_dac_config(...)          audio_aso_config(48000, 2, 32, ASO_TYPE_WAV)
        #define wav_aud_dac_open()               audio_aso_open(ASO_TYPE_WAV)
        #define wav_aud_dac_close()              audio_aso_close(ASO_TYPE_WAV)
    #endif
    #define wav_aud_dac_set_volume(...)           //yuan41 aud_dac_set_volume
    #define wav_aud_volume_mute(...)         //aud_volume_mute
    #define wav_aud_PAmute_operation(...)    //aud_PAmute_operation
    #define wav_extPA_open(...)              //extPA_open
    #define wav_extPA_set_req(...)           //extPA_set_req
    #ifdef AUD_WAV_TONE_SEPARATE
        #define WAV_FILL_48K_EN     0 //wav fill with stereo 48k24bit
        #define AUD_WAV_APP_AS_COEXIST  //audio stream coexist with bt/usb...[for wav in sbc mode, conflict with bt sbc, may need to modify wav fmt in future.]
        #define wav_aud_dac_get_free_buffer_size     aud_wav_ringbuf_free_get
        #define wav_aud_dac_fill_buffer              aud_wav_fill_buffer
        #define music_ch_busy()                     0
    #else
        #define WAV_FILL_48K_EN     1 //wav fill with stereo 48k24bit
        #define wav_aud_dac_get_free_buffer_size     aud_dac_get_free_buffer_size
        #define wav_aud_dac_fill_buffer              aud_dac_fill_buffer
        #define music_ch_busy()                     app_wave_playing()
    #endif
#else
    #define WAV_FILL_48K_EN     0 //wav fill with stereo 8k24bit
    #define wav_aud_mic_open            aud_mic_open
    #define wav_aud_dac_config          aud_dac_config
    #define wav_aud_dac_open            aud_dac_open
    #define wav_aud_dac_close           aud_dac_close
    #define wav_aud_dac_set_volume      aud_dac_set_volume
    #define wav_aud_volume_mute         aud_volume_mute
    #define wav_aud_PAmute_operation    aud_PAmute_operation
    #define wav_extPA_open              extPA_open
    #define wav_extPA_set_req           extPA_set_req
    #define wav_aud_dac_get_free_buffer_size     aud_dac_get_free_buffer_size
    #define wav_aud_dac_fill_buffer              aud_dac_fill_buffer
#endif

#define APP_WAVE_FILE_ID_NULL           (-1)
#define APP_WAVE_FILE_ID_START          0
#define APP_WAVE_FILE_ID_CONN           1
#define APP_WAVE_FILE_ID_DISCONN        2
#define APP_WAVE_FILE_ID_POWEROFF       3
#define APP_WAVE_FILE_ID_ENTER_PARING   4
#define APP_WAVE_FILE_ID_LOW_BATTERY    5
#define APP_WAVE_FILE_ID_HFP_RING       6
#define APP_WAVE_FILE_ID_HFP_ACK        7
#define APP_WAVE_FILE_ID_HFP_REJECT     8
#define APP_WAVE_FILE_ID_HFP_CANCEL     9
#define APP_WAVE_FILE_ID_HFP_VOICE_DIAL 10
#define APP_WAVE_FILE_ID_MUTE_MIC       11
#define APP_WAVE_FILE_ID_UNMUTE_MIC     12
#define APP_WAVE_FILE_ID_VOL_MAX        13
#define APP_WAVE_FILE_ID_VOL_MIN        14
#define APP_WAVE_FILE_ID_HFP_TRANSFER   15
#define APP_WAVE_FILE_ID_REDIAL         16
#define APP_WAVE_FILE_ID_CLEAR_MEMORY   17
#define APP_WAVE_FILE_ID_VOICE_NUM_0    18
#define APP_WAVE_FILE_ID_VOICE_NUM_1    19
#define APP_WAVE_FILE_ID_VOICE_NUM_2    20
#define APP_WAVE_FILE_ID_VOICE_NUM_3    21
#define APP_WAVE_FILE_ID_VOICE_NUM_4    22
#define APP_WAVE_FILE_ID_VOICE_NUM_5    23
#define APP_WAVE_FILE_ID_VOICE_NUM_6    24
#define APP_WAVE_FILE_ID_VOICE_NUM_7    25
#define APP_WAVE_FILE_ID_VOICE_NUM_8    26
#define APP_WAVE_FILE_ID_VOICE_NUM_9    27
#define APP_WAVE_FILE_ID_VOICE_CONTINUE 0x8000

#define APP_WAVE_FILE_ID_BT_MODE        28
#define APP_WAVE_FILE_ID_MP3_MODE       29
#define APP_WAVE_FILE_ID_FM_MODE        30
#define APP_WAVE_FILE_ID_LINEIN_MODE    31

#define APP_WAVE_FILE_ID_RESERVED0			32
#define APP_WAVE_FILE_ID_RESERVED1			33
#define APP_WAVE_FILE_ID_RESERVED2			34
//#define APP_WAVE_FILE_ID_BLE_CONN			APP_WAVE_FILE_ID_RESERVED0
//#define APP_WAVE_FILE_ID_BLE_DISCONN		APP_WAVE_FILE_ID_RESERVED1
#define APP_WAVE_FILE_ID_RESERVED3			35//lang change
#define APP_WAVE_FILE_ID_RESERVED4			36//eq change
#define APP_WAVE_FILE_ID_RESERVED5			37
#define APP_WAVE_FILE_ID_RESERVED6			38
#define APP_WAVE_FILE_ID_RESERVED7			39
#define APP_WAVE_FILE_ID_RESERVED8			40
#define APP_WAVE_FILE_ID_RESERVED9			41

//#define APP_WAVE_FILE_ID_ENTER_GAMING_MODE	APP_WAVE_FILE_ID_RESERVED2

#define WAVE_EVENT                          42

#define INVALID_WAVE_EVENT                  WAVE_EVENT

//#define APP_WAVE_FILE_ID_EARIN_DET			APP_WAVE_FILE_ID_RESERVED5

//#define IS_MODE_CHANGE_FILE_ID(id)      (((id >= APP_WAVE_FILE_ID_BT_MODE) && (id <= APP_WAVE_FILE_ID_LINEIN_MODE))? 1:0)

typedef void (*wav_cb_func)(void) ;

enum wav_error_codes
{
    WAV_NO_ERROR                = 0x00, 
    WAV_LOW_BATTERY_WHILE_SCO   = 0x01,
    WAV_FILE_ID_INVALID         = 0x02,
    WAV_FILE_TYPE_INVALID       = 0x03,
    WAV_POWER_OFF_WHILE_NOT_PWD = 0x04,
    WAV_FILE_PAGE_INDEX_INVALID = 0x05,
    WAV_FILE_LEN_INVALID        = 0x06,
    WAV_FILE_LINEIN_MODE        = 0x07,
    SBC_FILE_SYNCWORD_INVALID   = 0x08,
};
typedef struct _app_wave_play_ctrl_s
{
    uint8_t  flag_playing;
    uint8_t  type;
    uint8_t   page_buff[264];

    uint32_t  page_index;
    uint32_t  page_end;

    uint8_t   mute_tick;
    uint32_t  freq;
    uint16_t   channel;
    uint8_t   vol_s;
    uint8_t   vol;

    uint8_t   voice_num[32];
    uint8_t   voice_index;
    int       file_id;
    int       file_id_pending_saved;
    wav_cb_func func;
	#ifdef CONFIG_SBC_PROMPT
    uint8_t   status;
    uint8_t   block_size;
    uint8_t *sbc_code_ptr;
	#endif
}app_wave_play_ctrl_t;

typedef app_wave_play_ctrl_t * app_wave_handle_t;

void app_wave_file_volume_init( uint8_t vol );
void app_wave_file_vol_s_init( uint8_t vol );
void app_wave_file_aud_notify( uint32_t freq, uint16_t channel, uint8_t vol );
void app_wave_file_voice_num_set( char * num );
uint8_t app_wave_file_play_start( int file_id );
uint8_t check_wave_file_correct(int file_id);
uint8_t enter_mode_wave_and_action(uint32_t new_mode, wav_cb_func cb);
void app_wave_file_play_stop( void );
void app_wave_file_play( void );
int app_wave_playing( void );
int app_get_wave_info(uint32_t *freq_ptr, uint32_t * channel_ptr, uint32_t *vol_s_ptr);
void app_wave_file_play_init(void);
void app_wave_file_fill_buffer(uint8_t *buff, uint16_t size );
#ifdef CONFIG_SBC_PROMPT
int app_wave_is_sbc_playing(void);
int app_wave_check_type( uint8_t type);
void app_wave_set_status( uint8_t status);
int app_wave_check_status( uint8_t status);
uint8_t app_wave_get_blocksize(void);
int app_wave_fill_sbc_node(uint8_t *obj_ptr);
#endif

uint8_t start_wave_and_action(uint32_t ,wav_cb_func );

#endif
