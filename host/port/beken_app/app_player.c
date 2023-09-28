#include <stdlib.h>
#include <jos.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "msg_pub.h"

#include "mp3dec.h"
#include "mp3common.h"
#include "coder.h"
#include "layer21.h"
#include "bkmp3_resample.h"
#include "string.h"
#include "app_fs.h"
#include "diskio.h"
#include "bt_mini_sched.h"
#include "port.h"
#if (CONFIG_APP_WATCH_DSP == 1)
#include "mcu_watch_config.h"
#include "app_watch_mcu.h"
#endif
#include "app_async_data_stream.h"
#include "utils_audio.h"

#if SD_ASO_SRC248_EN
#include "api_src.h"
#endif

#if (CONFIG_APP_MP3PLAYER == 1)

// --------------------------------- debug
#define std_printf               os_printf
#define PLY_LOG_I(fmt,...)       std_printf("[PLY|I]"fmt, ##__VA_ARGS__)
#define PLY_LOG_W(fmt,...)       std_printf("[PLY|W:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PLY_LOG_E(fmt,...)       std_printf("[PLY|E:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PLY_LOG_D(fmt,...)       std_printf("[PLY|D]"fmt, ##__VA_ARGS__)
#define PLY_LOG_DD(fmt,...)      //std_printf("[PLY|D]"fmt, ##__VA_ARGS__)

// #define DBG_PLAYER_ASO
#ifdef DBG_PLAYER_ASO
    #define SD_RD_DEC_PROC(en)      REG_GPIO_0x0C = ((en) ? 2 : 0);//sdcard read and decode
    #define SD_FILL_BUF_PROC(en)    REG_GPIO_0x0E = ((en) ? 2 : 0);//sdcard fill buf and src
#else
    #define SD_RD_DEC_PROC(en)
    #define SD_FILL_BUF_PROC(en)
#endif

// --------------------------------- config
#define CONFIG_AUD_FADE_IN_OUT_PLAY     0//CONFIG_AUD_FADE_IN_OUT
#define MEDIA_ERR_CNT_THRE              15

#define PLAYER_RB_16BIT_EN              0//0:32bit, 1:16bit, ringbuff audio bit width

#define FILE_READ_BUF_SIZE              2048
#define PLAY_RINGBUF_SIZE               AUDIO_DAC_BUFF_LEN//8192U
#define MP3_FRAME_BUF_SIZE              5000U

// -------- wav file
#define WAV_24BIT_FILE_SUPPORT          //define to support 24bit audio wav file
#define WAV24_TO_16BIT_EN               1//[0:32bit/1:16bit], file read then fill buff with 16bit


#define FILL_OUT_BUF_SMPS               256 //for src buff size limited, the pcm data must proccess in sub sect.
// #define FILE_RD_FIXD_T_MS               10 //(unit:ms), define to set file read size to fixd frame time, limited by audio rb size


// not implemented yet
#define APP_PLAYER_FLAG_RANDOM_ORDER            0x2000

#define APP_PLAYER_HW_STANDBY_TIME              100
//lianxue.liu
#define APP_PLAYER_WAVE_READ_BLOCK_NUM          4
// before setting audio corretly, we need wait for at least 4 corret frame decode. 
#define APP_PLAYER_AUDIO_INITION_BEGIN          4

#define PADDING_DATA_TIMES                      15
#define PADDING_TICK_BETWEEN_MUSIC              30


// ---------------------------------
enum
{
    APP_PLAYER_SCHEDULE_CMD_NONE            = 0,
    APP_PLAYER_SCHEDULE_CMD_NEXT_SONG = 1,
    APP_PLAYER_SCHEDULE_CMD_PREV_SONG = 2,
    APP_PLAYER_SCHEDULE_CMD_NEXT_DIR = 3,
    APP_PLAYER_SCHEDULE_CMD_MODE_CHANGE = 4,
    APP_PLAYER_SCHEDULE_CMD_PREV_DIR = 5,
    APP_PLAYER_SCHEDULE_CMD_FIX_SONG = 8,
    APP_PLAYER_SCHEDULE_CMD_SEL_SONG_INDEX = 9,
};

#define APP_PLAY_SCHEDULE_CMD_PLAY_FLAG      0x40000000

#define APP_PLAYER_FLAG_HW_ATTACH                0x1
#define APP_PLAYER_FLAG_PLAY_PAUSE               0x2
#define APP_PLAYER_FLAG_PLAY_CONTINOUS           0x4
#define APP_PLAYER_FLAG_PLAY_CYCLE               0x8
#define APP_PLAYER_FLAG_PLAY_END                 0x10
#define APP_PLAYER_FLAG_WAIT_DAC_EMPTY           0x20
#define APP_PLAYER_FLAG_HALT                     0x40


#ifdef USER_KARAOK_MODE

#define fat_aso_open()              audio_aso_open(ASO_TYPE_SD)
#define fat_aso_close()             audio_aso_close(ASO_TYPE_SD)
// #define fat_aso_config(fs,chs,bits) audio_aso_config(fs,chs,bits,ASO_TYPE_SD)
#define fat_aso_volume_set(vol)     //audio_aso_volume_set(vol, ASO_TYPE_SD)
#define fat_aso_rb_clear()          aud_dac_buffer_clear()
#define fat_aso_mute(en)            //audio_dac_ana_mute(en)
#define fat_aso_rb_write(buf, sz)   aud_dac_fill_buffer(buf, sz)
#define fat_aso_rb_free_size_get()  aud_dac_get_free_buffer_size()

#else

#define fat_aso_open()              aud_dac_open(ASO_TYPE_SD)
#define fat_aso_close()             aud_dac_close(ASO_TYPE_SD)
#define fat_aso_config(fs,chs,bits) aud_dac_config(fs,chs,bits,ASO_TYPE_SD)
#define fat_aso_volume_set(vol)     aud_dac_set_volume(vol)
#define fat_aso_rb_clear()          aud_dac_buffer_clear()
#define fat_aso_mute(en)            audio_dac_ana_mute(en)
#define fat_aso_rb_write(buf, sz)   aud_dac_fill_buffer(buf, sz)
#define fat_aso_rb_free_size_get()  aud_dac_get_free_buffer_size()

#endif



typedef struct
{
    union
    {
        struct mp3_decoder_t{
            MP3DecInfo mp3DecInfo;
            FrameHeader fh;
            SideInfo si;
            ScaleFactorInfo sfi;
            HuffmanInfo hi;
            DequantInfo di;
            IMDCTInfo mi;
            SubbandInfo sbi;
            L2DecodeContext l2i;
        }mp3;
        struct wav_decoder_t{
            uint8_t rbbuf[2048];//FILE_READ_BUF_SIZE
            short aulawsmpl[2048];
            short alawtbl[256];
            short ulawtbl[256];
        }wav;
    };
}music_decoder_t;

// extern result_t avrcp_ct_set_volume(uint8_t volume);

//#define MUSIC_PREV_SONG
#ifdef MUSIC_PREV_SONG
uint8 flag_prev_song = 0;
#endif

#ifdef APP_PLAYER_DEBUG_FOR_QUICK_PLAY
int quick_play_flag = 0;
#endif

typedef enum _PLAY_STATE_e{
    PLAY_IDLE,
    PLAY_MP3,
    PLAY_WAV,
}PLAY_STATE_e;

typedef struct _app_player_ctrl_s
{
    fat_file_info_t file_info;        // cur file info
    uint16          dir_index;
    int32           file_index;      // file handle
    uint32          block_played;

    uint32          player_flag;
    uint32          schedule_cmd; 
    uint32          schedule_param;
    uint8_t         has_init;
    uint8_t         fixed_dir_en; //config play fixed dir
    uint16_t        fixed_dir_idx;//dir idx for play fixed dir
    uint32_t        media_err_cnt;
    uint8           media_err_status; //lianxue.liu  
    uint8           wav_header[44];
    uint8           wav_header_index;
    uint8           wav_read_flag;    
    uint32          wav_freq;
    uint8_t         wav_channel;
    uint8_t         wav_bits;
    uint32          wav_bytePsecd;
    uint32_t        wav_data_pos;   //wav file audio data start position
    uint32_t        wav_tail_pos;   //wav file audio data end position + 1
    uint8_t         adjust_playtime;
    uint16_t        total_time;
}app_player_ctrl;

static app_player_ctrl app_player;
static wavInfo wav_info;

typedef struct _app_player_ctrl_backup_s
{
    uint32          file_index;      // file handle
    uint32          file_size_blks;
    uint32          block_played;
    uint16          dir_index;
    uint8_t         play_pause;
}__PACKED_POST__ app_player_ctrl_backup;

static app_player_status_callback_fxn app_player_status_cb = NULL;

app_player_ctrl_backup player_config_backup;

uint8 *readBuf;   
short *mp3_pcm_ptr; 
short *mp3_pcm_ptr_tmp; 
int pcm_size = 0;
volatile uint32 buf_free = 0;
         
short  *aulawsmpl;
short  *alawtbl;
short  *ulawtbl;
uint8  *rbbuf;

HMP3Decoder hMP3Decoder;    //error file struct
MP3FrameInfo mp3FrameInfo;     //mp3 file struct
MP3DecInfo *mp3decinfo;
   
uint8 init=0;
static uint8_t mp3_play_flag = PLAY_IDLE;
// short *last_half_data = NULL;
// uint16_t resample_size = 0;
uint16 frame_decode_delay_count=0;
uint8 player_vol_bt = 10;

FIL *newFatfs;
// uint32 play_derect = 0;   // 1:next, 0 :previous
static int s_rd_sz_set = FILE_READ_BUF_SIZE;    //file read size set
static int s_aud_buf_sz = PLAY_RINGBUF_SIZE / 2;//min free size of ringbuf before write ringbuf

uint16_t music_samplerate;
uint8_t music_channel;
uint8_t music_bit_width;
static uint8_t padding_data_times;
static uint64_t padding_tick;
int32_t remain_samples;
extern uint16_t hold_samples;
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
extern int32_t s_fade_scale;
#endif
extern uint8_t A2DP_SAMPLE_RATEX;


uint8 appPlayerPlayMode=0;

#if SD_ASO_SRC248_EN
aud_src_ctx_t sd_aso_src_s = {
    .fs_in = 48000,
    .fs_out = SRC_OUT_SAMPLERATE,
    .chs = 2,
    .bits_in = 16,
    .bits_out = PLAYER_RB_16BIT_EN ? 16 : 24,
    .src_buf = NULL,
    .max_fra_sz = 256,
};
#endif

static void app_player_fill_buffer(uint8_t *buff, int size, uint8_t chs, uint8_t bits);
static void app_player_fill_zero_in_dac_buf(uint16_t size);
static void app_player_play_pause( int play_pause  );
static void app_player_mp3_file_end( void );
static void app_player_wav_file_end( void );
static void app_player_process_file_end( void );
static int app_player_get_next_stream_file( void );
static int app_player_get_prev_stream_file( void );
static int app_player_to_first_file( void );
static void app_player_stop( void  );
static int app_player_wave_header_handler(uint8 *buff, uint16 len);
static void app_player_wave_init(void);
static void app_player_mp3_init(void);
static void app_player_file_info_init( void );
static void wav_mem_uninit(void);
static void wav_mem_init(void);
static void mp3_mem_uninit(void);
static void mp3_mem_init(void);
static int app_player_get_stream_file_from_index(int index);




#if SD_ASO_SRC248_EN
int fat_aso_config(int fs, uint8_t chs, uint8_t bits)
{
    int ret = 0;

    sd_aso_src_s.fs_in = fs;
    sd_aso_src_s.chs = chs;
    sd_aso_src_s.bits_in = WAV24_TO_16BIT_EN ? 16 : bits;
    sd_aso_src_s.max_smps_in = FILL_OUT_BUF_SMPS * fs / SRC_OUT_SAMPLERATE; //for audio frame buf size limit, must limit audio sample num input
    ret = audio_src_init(&sd_aso_src_s);
    if(ret != 0)
    {
        LOG_E(FAT, "audio_src_init fail:%d\n", ret);
        goto RET;
    }
    ret = audio_aso_config(sd_aso_src_s.fs_out, 2, sd_aso_src_s.bits_out, ASO_TYPE_SD);
    if(ret != 0)
    {
        LOG_E(FAT, "audio_aso_config fail:%d\n", ret);
        goto RET;
    }
RET:
    return ret;
}
#else
#define fat_aso_config(fs,chs,bits) audio_aso_config(fs,chs,bits,ASO_TYPE_SD)
#endif
#if 0
uint32 app_get_mp3_filenumber( void )
{
	return app_player.file_index;
}
uint32 app_get_mp3_state( void )
{
    if(app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE)
        return 1;
    else
        return 0;
}
#endif

//get breakpoint data and size, used to save to no-volatile memry.
int app_player_breakpoint_get(uint32_t* p_addr, int* p_size)
{
    *p_addr = (uint32_t)&player_config_backup;
    *p_size = sizeof(player_config_backup);
    return 0;
}

//set breakpoint data.
int app_player_breakpoint_set(void* addr, int size)
{
    app_player_ctrl_backup *bak = (app_player_ctrl_backup*)addr;
    if(size < sizeof(player_config_backup))
    {
        os_printf("save play fail!\n");
        return -1;
    }
    // memcpy(&player_config_backup, addr, sizeof(player_config_backup));
    player_config_backup.file_index     = bak->file_index;
    player_config_backup.dir_index      = bak->dir_index;
    player_config_backup.file_size_blks = bak->file_size_blks;
    player_config_backup.block_played   = bak->block_played;
    return 0;
}

static void app_restore_breakpoint(void)
{
#if 0
    uint8 item;

    if(get_cur_media_type()== 0/*DISK_TYPE_SD*/)
        item = ENV_SD_INFO;
    else
        item = ENV_UDISK_INFO;

    //app_env_restore((void*)&player_config_backup,item);
    //os_printf("back info:diridex=%x,fileidx=%x,filesize=%x,blockplayed=%x\r\n",
    //player_config_backup.dir_index,player_config_backup.file_index,player_config_backup.file_size_blks,player_config_backup.block_played);
#endif    
    app_player.file_index = player_config_backup.file_index;
    app_player.dir_index = player_config_backup.dir_index;
    app_player.file_info.file_blks = player_config_backup.file_size_blks;
    app_player.block_played = player_config_backup.block_played;
}

static void app_player_save_breakpoint( void )
{   
    player_config_backup.file_index = app_player.file_index;
    player_config_backup.dir_index = app_player.dir_index;
    player_config_backup.file_size_blks = app_player.file_info.file_blks;
    player_config_backup.block_played = app_player.block_played;

    #if 0
    uint8 item;
    if(get_cur_media_type()== 0/*DISK_TYPE_SD*/)
        item = ENV_SD_INFO;
    else
        item = ENV_UDISK_INFO;
    //os_printf("save breakpoint:index:%x,blockplayed:%x,filesize:%x\r\n",
    //player_config_backup.file_index,player_config_backup.block_played,player_config_backup.file_size_blks);

    //app_env_save_task((void*)&player_config_backup,item);
    #endif
	
}

void app_player_save_play_status( void )
{   
    player_config_backup.play_pause = (app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE);
    os_printf("save play:%d\r\n",player_config_backup.play_pause);
}

uint8_t app_player_get_save_play_status( void )
{   
    return player_config_backup.play_pause;
}

static void app_player_shedule_cmd(int cmd )
{
    cmd |= APP_PLAY_SCHEDULE_CMD_PLAY_FLAG;
    app_player_play_pause(0);

#ifdef APP_PLAYER_DEBUG_FOR_QUICK_PLAY
    quick_play_flag = 0;
#endif
    app_player.schedule_cmd = cmd;
}

uint32 media_playing_flag = 0;//current meida playing status

static void resume_mp3_from_wav(void)
{
    if(media_playing_flag)
    {
        app_player_play_pause(1); //contiue playing after playing wav.
        media_playing_flag = 0;
    }
    else
    {
        aud_PAmute_operation(1);
    }
}

static void playwav_resumeMp3(uint32 fieldId)
{
    if(0 == check_wave_file_correct(fieldId) && 
        !app_bt_flag1_get( APP_FLAG_WAVE_PLAYING ))
    {
        if(app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE)// bak up player_flag to contiue playing after playing wav.
        {
            media_playing_flag = 1;
            app_player_play_pause(0);// pause the current play
        }
        //the start of wav play
        fieldId = APP_WAVE_FILE_ID_NULL;//++ yuan
        start_wave_and_action(fieldId, resume_mp3_from_wav);
    }
}

void app_playwav_resumeMp3(uint32 fieldId)
{
    playwav_resumeMp3(fieldId);
}
#if 0
uint32 app_player_automatic_play_at_first_time(void)
{
    if(get_fat_ok_flag())
    {
        app_player_file_info_init();
        app_player_button_play_pause();
    }
    return 0;
}
#endif

void app_player_set_vol( uint8_t vol )
{
    if(vol < AUDIO_VOLUME_MIN ||vol > AUDIO_VOLUME_MAX) return;

    player_vol_bt = vol;
    if(player_vol_bt == AUDIO_VOLUME_MIN)
    {
        if(app_player.player_flag&APP_PLAYER_FLAG_PLAY_PAUSE)
            aud_PAmute_operation(0);        
    }
    
    if(!music_ch_busy())
        fat_aso_volume_set(player_vol_bt);
    if(AUDIO_VOLUME_MAX == player_vol_bt)
    {
        playwav_resumeMp3(APP_WAVE_FILE_ID_VOL_MAX);
    }

    if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
    {
        //avrcp_ct_set_volume(player_vol_bt?(player_vol_bt-1)*8+7:0);
    }
#if (CONFIG_APP_WATCH_DSP == 1)
    if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
    {
        app_player_report_status_to_dsp(APP_PLAYER_VOL_CHANGE,&player_vol_bt);
    }
#endif
#if (SYS_VOL_SAVE == 1)
    app_save_volume(ENV_MUSIC_VOL_INFO);
#endif
}

int app_player_button_setvol_up( void )
{
    if(player_vol_bt == AUDIO_VOLUME_MIN)
    {
        if(app_player.player_flag&APP_PLAYER_FLAG_PLAY_PAUSE)
            aud_PAmute_operation(0);        
    }
	
    if(++player_vol_bt >= AUDIO_VOLUME_MAX)
    {
        player_vol_bt=AUDIO_VOLUME_MAX; 
    }
  //  os_printf("vol_up:%d\r\n",player_vol_bt);
    
    if(!music_ch_busy())
        fat_aso_volume_set(player_vol_bt);
    if(AUDIO_VOLUME_MAX == player_vol_bt)
    {
        playwav_resumeMp3(APP_WAVE_FILE_ID_VOL_MAX);
    }

    if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
    {
        //avrcp_ct_set_volume(player_vol_bt?(player_vol_bt-1)*8+7:0);
    }
#if (CONFIG_APP_WATCH_DSP == 1)
    if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
    {
        app_player_report_status_to_dsp(APP_PLAYER_VOL_CHANGE,&player_vol_bt);
    }
#endif
#if (SYS_VOL_SAVE == 1)
    app_save_volume(ENV_MUSIC_VOL_INFO);
#endif
    return 0;
}

int app_player_button_setvol_down( void )
{
    if(player_vol_bt != AUDIO_VOLUME_MIN)
    {
        player_vol_bt --;
    }
   // os_printf("vol_down:%d\r\n",player_vol_bt);

    if(!music_ch_busy())
        fat_aso_volume_set(player_vol_bt);
    if(AUDIO_VOLUME_MIN == player_vol_bt)
    {
        playwav_resumeMp3(APP_WAVE_FILE_ID_VOL_MIN);
    }

    if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
    {
        //avrcp_ct_set_volume(player_vol_bt?(player_vol_bt-1)*8+7:0);
    }
#if (CONFIG_APP_WATCH_DSP == 1)
    if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
    {
        app_player_report_status_to_dsp(APP_PLAYER_VOL_CHANGE,&player_vol_bt);
    }
#endif
#if (SYS_VOL_SAVE == 1)
    app_save_volume(ENV_MUSIC_VOL_INFO);
#endif
    return 0;
}

int app_player_button_play_pause( void )	// yuan++ 沒動作
{
    app_player_play_pause( !(app_player.player_flag &  APP_PLAYER_FLAG_PLAY_PAUSE ) );
    return 0;
}

int app_player_button_next( void )
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_NEXT_SONG );
    return 0;
}
int app_player_button_nextDir( void )
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_NEXT_DIR );
    return 0;
}

int app_player_button_prev( void )
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_PREV_SONG );
    return 0;
}

int app_player_button_prevDir(void)
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_PREV_DIR );
    return 0;
}

/*static int app_player_button_fix( void )
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_FIX_SONG );
    return 0;
}*/

int app_player_select_music(int index)  //int app_player_select_music(uint32_t index)
{
    app_player_shedule_cmd( APP_PLAYER_SCHEDULE_CMD_SEL_SONG_INDEX );
    app_player.schedule_param = index;
    return 0;
}   

#if CALC_PLAY_TIME 
static void app_player_wave_get_bytePersecd(void)
{
    if( app_player.wav_header_index != 44 ) return;

#if 0
    uint8 *buff = &app_player.wav_header[0];
    uint32 bytePsecd = buff[28] + (buff[29]<<8) + (buff[30]<<16) + (buff[31]<<24);
#else
    uint32 bytePsecd =  wav_info.fs * wav_info.ch * wav_info.bits / 8;
#endif
    app_player.wav_bytePsecd = bytePsecd;  
}

static void app_player_wave_calc_total_playtime(void)
{
    if( app_player.wav_header_index != 45 ) return;
    
    // app_player.total_time = (newFatfs->obj.objsize - 44) / app_player.wav_bytePsecd;
    app_player.total_time = (app_player.wav_tail_pos - app_player.wav_data_pos) / app_player.wav_bytePsecd;
    // MP3_Show_Play_Time(total_time);
}

uint32_t app_player_wave_calc_cur_playtime(void)
{
    if( app_player.wav_header_index != 45 ) return 0;

    // uint32_t cur_time = (newFatfs->fptr - 44) / app_player.wav_bytePsecd;
    uint32_t cur_time = (newFatfs->fptr - app_player.wav_data_pos) / app_player.wav_bytePsecd;
    // MP3_Show_Play_Time(cur_time);
    return cur_time;
}
#endif

static int app_player_wave_header_handler( uint8 *buff, uint16 len )
{
    if( app_player.wav_header_index >= 44 )
        return 0;

    if(!(((buff[0x00] == 'R') && (buff[0x01] == 'I')&& (buff[0x02] == 'F') && (buff[0x03] == 'F')) &&
         ((buff[0x08] == 'W') && (buff[0x09] == 'A')&& (buff[0x0A] == 'V') && (buff[0x0B] == 'E'))))
    {
        return 0;
    }

    memcpy(&app_player.wav_header[0], buff, 12);

    int32_t  s = len  - 12; //buf remain size
    uint8_t* p = buff + 12;
    uint32_t f = 1; //wav_chunk_cnt

    while(s >= 8)
    {
        uint32_t id = p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);
        uint32_t sz = p[4] + (p[5] << 8) + (p[6] << 16) + (p[7] << 24);

        switch(id)
        {
        case 0x20746d66://"fmt "
            memcpy(&app_player.wav_header[12], p, 24);
            f++;
            break;
        case 0x61746164://"data"
            memcpy(&app_player.wav_header[36], p, 8);
            f++;
            break;
        default:
            break;
        }

        if(f == 3) break; //find the chunk "fmt" and  "data"

        s -= (8 + sz);
        p += (8 + sz);
    }

    if(f != 3) return 0; //not find the chunk "fmt" and  "data"

    app_player.wav_header_index = 44;
    app_player.wav_data_pos = (uint32_t)(p + 8 - buff);
    app_player_wave_init();
    s -= 8;
    p += 8;

    if(s>0)
    {
        uint32_t m = s % wav_info.block_align;
        if((24 ==wav_info.bits) && m )
        {
            f_SeekfromCurPos(newFatfs, 1, s);
        }
        else
        {
            //wav_fill_buffer(p + 8, s - 8);
        }
    }
    return 1;
}

/// @brief calc relationship of (half of play ringbuff size) and (file read size)
/// @param p_wav 
/// @param file_buf_sz in/out, input read buf size defined, output file read size that should be configured
/// @param aud_buf_sz in/out, input half ring buf size, output audio buff size needed.
/// @return 0:ok, -1:fail
int file_read_size_get(wavInfo *p_wav, int *file_buf_sz, int* aud_buf_sz)
{
    #define SMPS_PER_MS_GET(fs)     ((fs) / 1000 + (((fs) % 1000) ? 1 : 0))//get samples per ms by sample rate
    int rd_sz_set = *file_buf_sz;
    int aud_fra_sz = *aud_buf_sz;

    PLY_LOG_I("%s() file_buf_sz:%d, aud_buf_sz:%d\n", __FUNCTION__, *file_buf_sz, *aud_buf_sz);
    int aso_block_align = PLAYER_RB_16BIT_EN ? 4 : 8;//16bit2ch/32bit2ch

    int asi_smps_per_ms = SMPS_PER_MS_GET(p_wav->fs);
#if SD_ASO_SRC248_EN
    int aso_smps_per_ms = SMPS_PER_MS_GET(SRC_OUT_SAMPLERATE);
#else
    int aso_smps_per_ms = asi_smps_per_ms;
#endif

#ifdef FILE_RD_FIXD_T_MS
    int fra_t_min_ms = FILE_RD_FIXD_T_MS;
#else
    int asi_bytes_per_smp = (p_wav->block_align % 3) ? p_wav->block_align : (p_wav->block_align / 3 * 4);//for 24bit according to 32bit capacity
    int asi_fra_t_ms = (rd_sz_set / asi_bytes_per_smp) / asi_smps_per_ms;
    int aso_fra_t_ms = (aud_fra_sz / aso_block_align) / aso_smps_per_ms;
    int fra_t_min_ms = (aso_fra_t_ms < asi_fra_t_ms) ? aso_fra_t_ms : asi_fra_t_ms;
    // PLY_LOG_I("audio frame max[rd:%dms, wr:%dms]\n", asi_fra_t_ms, aso_fra_t_ms);
#endif
    rd_sz_set = fra_t_min_ms * asi_smps_per_ms * p_wav->block_align;
    aud_fra_sz = fra_t_min_ms * aso_smps_per_ms * aso_block_align;

    *file_buf_sz = rd_sz_set;
    *aud_buf_sz = aud_fra_sz;
    PLY_LOG_I("calc: rd_sz_set:%d, aud_fra_sz:%d, op: %dms\n", rd_sz_set, aud_fra_sz, fra_t_min_ms);
    return 0;
}

static void app_player_wave_init(void)
{
    app_player.wav_read_flag = 1;
    uint32 wave_actual_len=0;
    uint32_t wave_tail_size = 0;
    uint32_t wave_time_sec = 0;
    uint32_t bytes_per_sec = 0;
    int set_audio_return = 10;
    if( app_player.wav_header_index != 45 )
    {
        app_player.media_err_cnt = 0;
        
        memcpy( (uint8 *)&wav_info.fmtTag, &app_player.wav_header[0x14], 2 );
        memcpy( (uint8 *)&wav_info.fs, &app_player.wav_header[0x18], 4 );
        memcpy( (uint8 *)&wav_info.ch, &app_player.wav_header[0x16], 2 );
        memcpy( (uint8 *)&wav_info.block_align, &app_player.wav_header[0x20], 2 );
        memcpy( (uint8 *)&wav_info.bits, &app_player.wav_header[0x22], 2 );
        memcpy((uint8_t*)&bytes_per_sec, &app_player.wav_header[0x1C], 4);
        memcpy((uint8_t*)&wave_actual_len, &app_player.wav_header[0x28], 4);

        app_player.wav_freq = wav_info.fs;
        app_player.wav_channel = wav_info.ch;
        app_player.wav_bits = wav_info.bits;

        app_player.block_played = app_player.wav_data_pos;
        app_player.wav_tail_pos = app_player.wav_data_pos + wave_actual_len;
        wave_tail_size = app_player.file_info.file_blks - app_player.wav_tail_pos;
        wave_time_sec = wave_actual_len / bytes_per_sec;

        PLY_LOG_I("wave_type: %d, freq: %d, channel: %d, bits: %d\r\n", wav_info.fmtTag,wav_info.fs, wav_info.ch, wav_info.bits );
        PLY_LOG_I("wave file_size:%d Byte, wave_actual_len:%d Byte\n", app_player.file_info.file_blks, wave_actual_len);
        PLY_LOG_I("wave head_sz:%d, tail_sz:%d; time: %dmin %ds\n", app_player.wav_data_pos, wave_tail_size, wave_time_sec / 60, wave_time_sec % 60);
        if( (wav_info.fmtTag != 0x01)  		// linear 8bits/16bits/32bits
            && (wav_info.fmtTag != 0x06)    // a-law  8bits
            &&(wav_info.fmtTag != 0x07))    // u-law  8bits
        {
            app_player.block_played = app_player.file_info.file_blks;
            return;
        }
        if (wav_info.fs > 48000){
            app_player.block_played = app_player.file_info.file_blks;
            return;
        }
        #if 0
        if(wav_info.bits == 24)				// Not Support 24 bits linear
        {
            app_player.block_played = app_player.file_info.file_blks;
            return;
        }
        #ifdef MUSIC_PREV_SONG
        flag_prev_song = 0;
        #endif
        #endif
    #if CALC_PLAY_TIME 
        app_player_wave_get_bytePersecd();
    #endif
        
        //calc relationship of (half of play ringbuff size) and (file read size)
        #if 1
        s_rd_sz_set = FILE_READ_BUF_SIZE;
        s_aud_buf_sz = PLAY_RINGBUF_SIZE >> 1;
        file_read_size_get(&wav_info, &s_rd_sz_set, &s_aud_buf_sz);
        #else
        s_rd_sz_set = FILE_READ_BUF_SIZE;
        if(8 == wav_info.bits) s_rd_sz_set >>= 1;
        do
        {
            s_rd_sz_set >>= 1;
            s_aud_buf_sz = (wav_info.ch == 1) ? (s_rd_sz_set << 1) : s_rd_sz_set;
            #if SD_ASO_SRC248_EN
            s_aud_buf_sz = s_aud_buf_sz * SRC_OUT_SAMPLERATE / wav_info.fs + 1;
            #endif
        }while(s_aud_buf_sz > (PLAY_RINGBUF_SIZE / 2));
        if(8 == wav_info.bits) s_aud_buf_sz <<= 1;
        s_rd_sz_set /= wav_info.block_align;
        s_rd_sz_set *= wav_info.block_align;
        if(!PLAYER_RB_16BIT_EN) s_aud_buf_sz <<= 1;
        os_printf("rd_sz_set:%d, s_aud_buf_sz:%d\n", s_rd_sz_set, s_aud_buf_sz);
        #endif
        
        if(s_rd_sz_set < 512)  // Not support!!!!
        {
            PLY_LOG_W("Not support!!!, read size less than 512:%d\n", s_rd_sz_set);
            app_player.block_played = app_player.file_info.file_blks;
            return;
        }
        init=0;
        fat_aso_close();
	    set_audio_return = fat_aso_config(wav_info.fs, wav_info.ch, wav_info.bits);
		music_samplerate = wav_info.fs;
		music_channel = wav_info.ch;
        music_bit_width = mp3FrameInfo.bitsPerSample;
		if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
        {
         	if(music_samplerate == 48000) remain_samples = 0;
        }
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        hold_samples = MUSIC_END_FADE_OUT*music_samplerate/1000*4;
        aud_fade_in_out_init(wav_info.fs,AUD_FADE_DURING_TIMR);
#endif
        //set_audio_return = fat_aso_config( wav_info.fs, wav_info.ch, 16 );
        //os_printf("set_audio_return=%d\r\n",set_audio_return);
        //os_printf("app_player.schedule_cmd=%x\r\n",app_player.schedule_cmd);
        if((-1) == set_audio_return)
        {
            /*if( app_player.schedule_cmd  & APP_PLAYER_SCHEDULE_CMD_PREV_SONG )
              app_player_button_prev();
              else
              app_player_button_next();*/
            os_printf("not over!!! need change!\r\n");
            app_player.block_played = app_player.file_info.file_blks;
            return ;
        }

        fat_aso_volume_set(player_vol_bt);
        app_player.wav_header_index = 45;
        mp3_play_flag = PLAY_WAV;
        // if(wave_actual_len < app_player.file_info.file_blks)
        //     app_player.block_played = app_player.file_info.file_blks - wave_actual_len;
        
    }
}

void app_player_halt(uint8_t savebp)
{
    app_player_ctrl* ply = &app_player;
    if(!app_is_mp3_mode() || app_player.player_flag & APP_PLAYER_FLAG_HALT) return;
    
    app_player.player_flag |= APP_PLAYER_FLAG_HALT;
    app_player_play_pause(0);

    if(savebp)
    {
        app_player_save_breakpoint();
    }
    else
    {
        player_config_backup.dir_index = ply->fixed_dir_en ? ply->fixed_dir_idx : 0;
        player_config_backup.file_index = 0;
        player_config_backup.block_played = 0;
    }
    player_config_backup.play_pause = 0;

    if(app_player.has_init)
    {
        if(mp3_play_flag == PLAY_MP3)
        {
            app_player_mp3_file_end();
        }
        else if (mp3_play_flag == PLAY_WAV)
        {
            app_player_wav_file_end();
        }   
        //app_watch_pm_clear_flag(APP_WATCH_PM_OBJ_SD,APP_WATCH_PM_SD_FLAG_PLAYER);
        app_player.has_init = 0;
    }
    fat_aso_mute(1);
    fat_aso_close();
    
}

void app_player_restore_from_halt(void)
{
    if(!app_is_mp3_mode() || !(app_player.player_flag & APP_PLAYER_FLAG_HALT)) return;
    
    app_player.player_flag &= ~APP_PLAYER_FLAG_HALT;

    if(!app_player.has_init)
    {
        //app_watch_pm_set_flag(APP_WATCH_PM_OBJ_SD,APP_WATCH_PM_SD_FLAG_PLAYER);
        app_player_file_info_init();
        app_player.has_init = 1;
    }
}

static void app_player_mp3_init(void)
{
    mp3_mem_init();

    //fat_aso_config(48000, 2, 16 );  //temp setting
    init=0;
    frame_decode_delay_count = 0;
#ifdef MUSIC_PREV_SONG    
    flag_prev_song = 0;
 #endif   
    fat_aso_volume_set(player_vol_bt);

#ifdef JMALLOC_STATISTICAL
    os_printf("JMALLOC_STATISTICAL: MP3\r\n");
    memory_usage_show();
#endif
}

void app_player_init( void)
{
    //app_env_handle_t  env_h = app_env_get_handle();
    PLY_LOG_I("app_player_init\r\n");
//    if(!(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH))//borg 230130
//    {
//        PLY_LOG_W(" !!!!!!!! disk is detached\n");
//        return;
//    }
    PLY_LOG_I("sizeof(music_decoder_t):%d Byte\n", sizeof(music_decoder_t));
    PLY_LOG_I("types:0x%X, 0x%X\n",newFatfs->obj.fs->fs_type, newFatfs);
    app_player_file_info_init();       
    //player_vol_bt=env_h->env_cfg.system_para.vol_a2dp;
    if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
    {
        //avrcp_ct_set_volume(player_vol_bt?(player_vol_bt-1)*8+7:0);
    }
    //app_player.player_flag = 0;
}


int app_player_unload(void)
{
    os_printf("app_player_unload\r\n");
    fat_aso_close();

    app_player_save_breakpoint();
    app_player_set_fixed_dir(0, 0);//exit clear play fixed dir flag
    os_printf("END 1,mp3_play_flag=%x\r\n",mp3_play_flag);
    if(mp3_play_flag == PLAY_MP3)
    {
        app_player_mp3_file_end();
    }
    else if (mp3_play_flag == PLAY_WAV)
    {
        app_player_wav_file_end();
    }
    fat_aso_rb_clear();

    j_memset( &app_player, 0, sizeof(app_player_ctrl) );
    app_player.has_init = 0;

    dbg_show_dynamic_mem_info(1);
    os_printf("app_player_unload ok\r\n");
    return 0;
}

static int app_player_to_first_file( void )
{
    FILE_INFO *pFile;
    app_player_ctrl* ply = &app_player;

    uint32 player_flag = app_player.player_flag;
    uint32 schedule_cmd = app_player.schedule_cmd;
    memset( (uint8 *)&app_player, 0, sizeof( app_player_ctrl ) );
    os_printf("app_player_to_first_file\r\n");
    app_player.player_flag = player_flag;
    app_player.schedule_cmd = schedule_cmd;

    if( 0 == get_musicfile_count())
    {
        return -1;
    }

    app_player.dir_index = ply->fixed_dir_en ? ply->fixed_dir_idx : 0;
    app_player.file_index = 0;
    newFatfs = Get_Cur_Dir_file(app_player.dir_index,app_player.file_index);
    pFile = get_file_info();
	
    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
    app_player.file_info.file_blks = newFatfs->obj.objsize;
    app_player.file_info.file_start_cluster = newFatfs->obj.sclust;

    app_player.block_played = 0;
    memcpy(app_player.file_info.ext_name,pFile->extname,3);
    memcpy(app_player.file_info.filename,pFile->fname,64);
    os_printf("filename:%s, ext:%2s\r\n", app_player.file_info.filename, app_player.file_info.ext_name);
    return 0;
}

#if 0
static int app_player_change_play_mode()
{
    if(++appPlayerPlayMode==APP_PLAYER_MODE_END)
        appPlayerPlayMode=0;
    //os_printf("appPlayerPlayMode=%d\r\n",appPlayerPlayMode);
    return 0;
}
#endif
static int app_player_get_stream_file(int dir_idx, int file_idx)
{
    FILE_INFO *pFile;
    app_player_ctrl* ply = &app_player;
    PLY_LOG_I("totalDir:%d, dir_idx:%d, file_idx:%d\n", get_musicdir_num(), ply->dir_index, ply->file_index);
    newFatfs = Get_Cur_Dir_file(ply->dir_index, ply->file_index);
    pFile = get_file_info();
	
    ply->player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
	ply->file_info.file_blks = newFatfs->obj.objsize;
    ply->file_info.file_start_cluster = newFatfs->obj.sclust;
    ply->block_played = 0;
    
    memcpy(ply->file_info.ext_name, pFile->extname, 3);
    memcpy(ply->file_info.filename, pFile->fname, 64);
    //PLY_LOG_I("filename+++++++++++++++++++++:%s\r\n",ply->file_info.filename);
    return 0;
}

static int app_player_get_next_dir(void)
{
    app_player_ctrl* ply = &app_player;
    ply->fixed_dir_en = 0;
    ply->dir_index = (ply->dir_index < (get_musicdir_num() - 1)) ? (ply->dir_index + 1) : 0;
    ply->file_index = 0;
    return app_player_get_stream_file(ply->dir_index, ply->file_index);
}

static int app_player_get_prev_dir(void)
{
    app_player_ctrl* ply = &app_player;
    ply->fixed_dir_en = 0;
    ply->dir_index = (ply->dir_index > 0) ? (ply->dir_index - 1) : (get_musicdir_num() - 1);
    ply->file_index = 0;
    return app_player_get_stream_file(ply->dir_index, ply->file_index);
}

static int app_player_get_stream_file_from_index(int index)
{
    FILE_INFO *pFile ;

    os_printf("\r\n%s,index=%x\n",__func__,index);
    newFatfs = Get_File_From_Number(index);
    if(newFatfs != NULL)
    {
	Get_DirFileIdx_By_gFileIndex(index, &app_player.dir_index, (uint16_t*)&app_player.file_index);
        //uint16_t file_idx;
        //Get_DirFileIdx_By_gFileIndex(index, &app_player.dir_index, &file_idx);
        //app_player.file_index = file_idx;   //app_player.file_index = index;
        os_printf("app_player.dir_index=%d,app_player.file_index=%d\r\n",app_player.dir_index,app_player.file_index);
        pFile = get_file_info();
        app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
        app_player.file_info.file_blks = newFatfs->obj.objsize;
        app_player.file_info.file_start_cluster = newFatfs->obj.sclust;
        app_player.block_played = 0;
        memcpy(app_player.file_info.ext_name,pFile->extname,3);
        memcpy(app_player.file_info.filename,pFile->fname,64);
    }
    else
    {
        os_printf("Get File Index Error %d\r\n",index);
        return -1;
    }
    return 0;
}

static int app_player_get_next_stream_file( void )
{
    app_player_ctrl* ply = &app_player;
    FILE_INFO *pFile ;
    uint16 musicfilecount;
    static unsigned long next=1;
    os_printf("app_player.dir_index=%x\n",app_player.dir_index);
    if(ply->fixed_dir_en == 1){
        ply->dir_index = ply->fixed_dir_idx;
    }
    musicfilecount =get_curDirMusicfile_count(app_player.dir_index);
    if( 0 == musicfilecount)
    {
        app_player.file_index=0;
        if(ply->fixed_dir_en == 0){
            app_player.dir_index++;
            if(app_player.dir_index >= get_musicdir_num())
                app_player.dir_index = 0;
        }
    }
    if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_RANDOM)
    {
        next = next*1103515245+12345;
        if(ply->fixed_dir_en == 0){
            app_player.dir_index = next%get_musicdir_num();
        }
        app_player.file_index = next%get_curDirMusicfile_count(app_player.dir_index);
    }
    else
    {
        app_player.file_index++;
        if(app_player.file_index >=musicfilecount)
        {
            app_player.file_index = 0;
            if(ply->fixed_dir_en == 0){
                app_player.dir_index++;
                if(app_player.dir_index>=get_musicdir_num())
                    app_player.dir_index = 0;
            }
        }
    }
    newFatfs = Get_Cur_Dir_file(app_player.dir_index,app_player.file_index);
    os_printf("next_file.... app_player.dir_index=%d,app_player.file_index=%d\r\n",app_player.dir_index,app_player.file_index);
    pFile = get_file_info();
    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
    app_player.file_info.file_blks = newFatfs->obj.objsize;
    app_player.file_info.file_start_cluster = newFatfs->obj.sclust;
    app_player.block_played = 0;
    memcpy(app_player.file_info.ext_name,pFile->extname,3);
    memcpy(app_player.file_info.filename,pFile->fname,64);
    //os_printf("filename+++++++++++++++++++++:%s\r\n",app_player.file_info.filename);
    return 0;
}


static int app_player_get_prev_stream_file( void )
{
    app_player_ctrl* ply = &app_player;
    FILE_INFO *pFile;
    uint16 musicfilecount;
    static unsigned long next=1;
    if(ply->fixed_dir_en == 1){
        ply->dir_index = ply->fixed_dir_idx;
    }
    musicfilecount =get_curDirMusicfile_count(app_player.dir_index);
	
    if( 0 == musicfilecount)
    {
        app_player.file_index=0;
        if(ply->fixed_dir_en == 0){
            if(app_player.dir_index>0)
                app_player.dir_index--;
            else
            {
                if(get_musicdir_num()>=1)
                    app_player.dir_index = get_musicdir_num()-1;
                else
                    return -1;
            }
        }
    }
    if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_RANDOM)
    {
        next = next*1103515245+12345;
        if(ply->fixed_dir_en == 0){
            app_player.dir_index = next%get_musicdir_num();
        }
        app_player.file_index = next%get_curDirMusicfile_count(app_player.dir_index);
    }
    else
    {
        if(app_player.file_index>0)
        {
            app_player.file_index--;
        }
        else if(ply->fixed_dir_en == 0)
        {
            app_player.file_index = get_curDirMusicfile_count(app_player.dir_index)-1;
        }
        else
        {
            if(app_player.dir_index>0)
                app_player.dir_index--;
            else 
                app_player.dir_index = get_musicdir_num()-1;
            app_player.file_index = get_curDirMusicfile_count(app_player.dir_index)-1;
        }
    }
    newFatfs = Get_Cur_Dir_file(app_player.dir_index,app_player.file_index);
    os_printf("prev_file.... app_player.dir_index=%d,app_player.file_index=%d\r\n",app_player.dir_index,app_player.file_index);
    pFile = get_file_info();
	
    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
    app_player.file_info.file_blks = newFatfs->obj.objsize;
    app_player.file_info.file_start_cluster = newFatfs->obj.sclust;
    app_player.block_played = 0;
    
    memcpy(app_player.file_info.ext_name,pFile->extname,3);
    memcpy(app_player.file_info.filename,pFile->fname,64);
    //os_printf("filename+++++++++++++++++++++:%s\r\n",app_player.file_info.filename);
    return 0;
}
static int app_player_get_fix_stream_file( uint16  fileIndex )
{
    FILE_INFO *pFile ;
    uint16 musicfilecount =get_curDirMusicfile_count(app_player.dir_index);
	
    if( 0 == musicfilecount)
    {
        return -1;
    }
    
    app_player.file_index = fileIndex;
    if(app_player.file_index > (musicfilecount-1))
        app_player.file_index = 0;
        
    newFatfs = Get_Cur_Dir_file(app_player.dir_index,app_player.file_index);
    os_printf("fix_file  app_player.file_index=%d\r\n",app_player.file_index);
    pFile = get_file_info();
	
    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
    app_player.file_info.file_blks = newFatfs->obj.objsize;
    app_player.file_info.file_start_cluster = newFatfs->obj.sclust;
    app_player.block_played = 0;
    
    memcpy(app_player.file_info.ext_name,pFile->extname,3);
    memcpy(app_player.file_info.filename,pFile->fname,64);
    
    return 0;
}

void app_player_file_info_init( void )
{
    app_player_ctrl* ply = &app_player;
    int file_index;
    uint16 dir_index;
    FILE_INFO *pFile;
    if( 0 == get_musicfile_count())
    {
        os_printf("file_info_init return\n");
        msg_put(MSG_CHANGE_MODE);
        return;
    }
		
    memset( (uint8 *)&app_player, 0, sizeof( app_player ) );
    app_player.player_flag |= (APP_PLAYER_FLAG_PLAY_CONTINOUS|APP_PLAYER_FLAG_PLAY_CYCLE|APP_PLAYER_FLAG_HW_ATTACH);
    //lianxue.liu
    app_restore_breakpoint();

    //if the previous player_vol_music is 0, need mute the system vol
    //if(player_vol_music == 0)
    //    fat_aso_volume_set(player_vol_music);

    dir_index = player_config_backup.dir_index;
    file_index = player_config_backup.file_index;
    os_printf("dir_index:%d,file index :%d\r\n",dir_index,file_index);
    //check the file is exist or not
    if((dir_index>=get_musicdir_num())||(file_index >=get_curDirMusicfile_count(dir_index)) || (file_index < 0))
    {
        app_player_to_first_file();
    }
    else
    {
        newFatfs = Get_Cur_Dir_file(dir_index,file_index);
        pFile = get_file_info();
        app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
        app_player.file_info.file_blks = newFatfs->obj.objsize;
        app_player.file_info.file_start_cluster = newFatfs->obj.sclust;
        app_player.block_played = 0;
        memcpy(app_player.file_info.ext_name,pFile->extname,3);
        memcpy(app_player.file_info.filename,pFile->fname,64);        
        if(app_player.file_info.file_blks == player_config_backup.file_size_blks)
        {
            app_player.dir_index = player_config_backup.dir_index;
            app_player.file_index = player_config_backup.file_index;
            app_player.block_played = player_config_backup.block_played;
            if( ptn_strnicmp((char*)app_player.file_info.ext_name, "WAV", 3) == 0 )
            {
                //app_player.block_played = 0;
                f_lseek(newFatfs,0);
            }
            else if( ptn_strnicmp((char*)app_player.file_info.ext_name, "MP3", 3) == 0 )
            {
            #if CALC_PLAY_TIME 
                app_player.adjust_playtime = 1;
            #endif
                f_lseek(newFatfs, app_player.block_played);
            }
        }
        else
        {
            app_player.dir_index = ply->fixed_dir_en ? ply->fixed_dir_idx : 0;
            app_player.file_index = 0;
            os_printf("types=%x,%x\n",newFatfs->obj.fs->fs_type,newFatfs);
            app_player_to_first_file();
        }
    }
    if(player_config_backup.play_pause)
    {
        app_player_play_pause( 1 );
    }
}

#if 0
void app_player_print_file_info( void )
{
    os_printf("File: %s, start index : %d, file size: %d\r\n",FileInfo.fname,app_player.block_played,
              app_player.file_info.file_blks );

}
#endif

void app_player_play_pause_caller( int play_pause )
{
    app_player_play_pause( play_pause );
}

static void app_player_play_pause( int play_pause  )
{
    uint32 status;// = BK3000_SDIO_CMD_RSP_INT_SEL;
    uint32 readbytes = 0;
#if defined(CONFIG_APP_SDCARD)	//yuan++
    app_handle_t app_h = app_get_sys_handler();
#endif
    if(get_musicfile_count() == 0 ){
    	return;
    }
    os_printf("\n\napp_player_play_pause:%d, app_player.player_flag:0x%x, player_vol_bt:%d \n", play_pause,app_player.player_flag, player_vol_bt);
    
    if((play_pause == 0) &&  (app_player.player_flag &  APP_PLAYER_FLAG_PLAY_PAUSE ))  // pause
    {
        os_printf("MP3 PAUSE \r\n");
    #if 0//defined(CONFIG_APP_SDCARD) //&& defined(CONFIG_LINE_SD_SNIFF)
        if(SYS_WM_SDCARD_MODE == app_h->sys_work_mode)
            sdcard_idle(1);
    #endif
        //app_set_led_event_action(LED_EVENT_MUSIC_PAUSE);
        app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_PAUSE;
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        set_aud_fade_in_out_state(AUD_FADE_OUT);
        app_player.player_flag |= APP_PLAYER_FLAG_WAIT_DAC_EMPTY;
        padding_data_times = 0;
        padding_tick = AUD_FADE_DURING_TIMR/10 + 10 + os_get_tick_counter();
#else
        aud_PAmute_operation(1);
        fat_aso_close();
#endif
#if (CONFIG_APP_WATCH_DSP == 1)
        if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
        {
            app_player_report_status_to_dsp(APP_PLAYER_PAUSE,NULL);
            app_player_report_status_to_dsp(APP_PLAYER_MUSIC_NAME,app_player.file_info.filename);
        }
#endif
    }
    else if( play_pause && !( app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE))
    {
        if(app_bt_flag1_get( APP_FLAG_WAVE_PLAYING ))
            return;
    #if defined(CONFIG_APP_SDCARD) //&& defined(CONFIG_LINE_SD_SNIFF)
        if(SYS_WM_SDCARD_MODE == app_h->sys_work_mode)
            sdcard_idle(0);
    #endif
        //app_set_led_event_action(LED_EVENT_MUSIC_PLAY);
        if( ptn_strnicmp((char*)app_player.file_info.ext_name, "WAV", 3) == 0 )
        {
            os_printf("wav\n");
            app_player.wav_read_flag = 0;

            if( app_player.wav_header_index == 45 )
            {
                init=0;
                os_printf("player_vol_bt = %d\r\n", player_vol_bt);
                fat_aso_close();
                fat_aso_config( app_player.wav_freq, app_player.wav_channel, app_player.wav_bits);
                fat_aso_volume_set(player_vol_bt);
                fat_aso_open();                            
                app_player.wav_read_flag = 1;
            }
            else
            {
                wav_mem_init();
                os_printf("fs_type=%x,%x\n",newFatfs->obj.fs->fs_type,newFatfs);
                status = f_read(newFatfs, rbbuf, 2048, &readbytes);
                
                if(status > 0)
                {
                    os_printf("read wav: status=%x\r\n",status);
                    app_player.block_played = app_player.file_info.file_blks;
                }
                else
                {  
                    // sbc_mem_free();
                    // linein_sbc_alloc_free();
                    // linein_sbc_encode_init();
                    if(!app_player.block_played)
                    {
                        app_player.block_played += readbytes;
                    }
                    else
                    {
                        f_lseek(newFatfs, app_player.block_played);
                    }
                    if(!app_player_wave_header_handler(rbbuf,readbytes))
                    {
                        os_printf("Err wav file!\r\n");
                        app_player_wav_file_end();
                        app_player_process_file_end();
                        return;
                    }
                    fat_aso_open();
                    
                    #if CALC_PLAY_TIME 
                    app_player_wave_calc_total_playtime();
                    #endif
                }
            }
            if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING) && fat_aso_rb_free_size_get() >= s_aud_buf_sz)
            {
                app_player_fill_zero_in_dac_buf(fat_aso_rb_free_size_get() - s_aud_buf_sz);
            }

        }
        else if( ptn_strnicmp((char*)app_player.file_info.ext_name, "MP3", 3) == 0 )
        {
            os_printf("mp3\n");
            if(mp3_play_flag != PLAY_MP3)
            {
                //os_printf("MP3 start vol:%d \r\n",player_vol_bt);
                app_player_mp3_init();
                if(mp3decinfo) 
                {
                    mp3decinfo->decode_state = MP3_DECODE_FIND_ID3_INFO;

                #if CALC_PLAY_TIME
                    mp3decinfo->totalframes = newFatfs->obj.objsize;
                    mp3decinfo->curFramecnt = 0;
                #endif
                }
                else	
                {
                    os_printf("mp3 memery initial unsucssed\r\n");
                }

                PLY_LOG_I("mp3 initial is ok \r\n");
                fat_aso_rb_clear();
                mp3_play_flag = PLAY_MP3;
            }
            else
            {
                mp3decinfo->decode_state = MP3_DECODE_FIND_SYNC_WORD;
                init = 0;
                frame_decode_delay_count = 0;
                PLY_LOG_I("continue playing mp3\r\n");
                fat_aso_rb_clear();
                // fat_aso_open();
            }
        }
        else
        {
            os_printf("other\n");
        }
        app_player.player_flag |= APP_PLAYER_FLAG_PLAY_PAUSE;
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        set_aud_fade_in_out_state(AUD_FADE_IN);
#endif
#if (CONFIG_APP_WATCH_DSP == 1)
        if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
        {
            app_player_report_status_to_dsp(APP_PLAYER_PLAY,NULL);
            app_player_report_status_to_dsp(APP_PLAYER_MUSIC_NAME,app_player.file_info.filename);
        }
#endif

        //aud_PAmute_operation(0);
    }
}

static void app_player_stop( void  )
{
    if( app_player.file_index < 0 )
        return;

    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_PAUSE;
    if( ptn_strnicmp((char*)app_player.file_info.ext_name, "WAV", 3) == 0 )
    {
 //       app_player.cur_block = app_player.start_block;
        app_player.block_played = 0;
        app_player.wav_header_index = 0;
        app_player.wav_read_flag = 0;
    }
    else if( ptn_strnicmp((char*)app_player.file_info.ext_name, "MP3", 3) == 0 )
    {
//        app_player.cur_block = app_player.start_block;
        app_player.block_played = 0;
    }

}
static void app_player_process_file_end( void )
{
    if(app_player.media_err_status == FR_DISK_ERR)
    {
        app_player.media_err_status = FR_OK;
        //msg_put(MSG_MEDIA_READ_ERR);
        return;
    }
    app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_PAUSE;
    if(app_player.wav_header_index != 0)//for player bug
        app_player.wav_header_index = 0;
    if( app_player.player_flag & APP_PLAYER_FLAG_PLAY_CONTINOUS )
    {		
        if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ONE)
        {
             app_player_get_fix_stream_file(app_player.file_index); // get current file
        }
        else
        {
            app_player_get_next_stream_file(); // get next file
        }
	
        //when switch to next/prev file, need to save the break point
        //app_player_save_breakpoint();
        app_player_play_pause(1);                                       // not the file list end, just play next file
    }
    else // not continous play
    {
        if( app_player.player_flag & APP_PLAYER_FLAG_PLAY_CYCLE )   // cycle , play last index
        {
            f_lseek(newFatfs,0);
            app_player.block_played = 0;
            app_player_play_pause(1);
        }
        else                                                                                // just close
        {
            app_player_stop();
            fat_aso_close();
        }
    }
}

static void app_player_wav_file_end( void )
{
    app_player.wav_header_index = 0;
    mp3_play_flag = PLAY_IDLE;
    app_player_wait_async_idle(__func__);
    wav_mem_uninit();
    // f_close(newFatfs);
    if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH) { f_close(newFatfs); }
}

static void app_player_mp3_file_end( void )
{
    frame_decode_delay_count = 0;
    init=0;
    app_player.media_err_cnt = 0;
    mp3_play_flag = PLAY_IDLE;
    app_player_wait_async_idle(__func__);
    mp3_mem_uninit();
    rbbuf = NULL;
    // f_close(newFatfs);
    if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH) { f_close(newFatfs); }
}
#if (APP_ASYNC_SDCARD_ENABLE == 1)
volatile APP_ASYNC_STATE gs_async_wav_state = APP_ASYNC_IDLE;
static void async_read_wav_cbk(uint32_t arg1,uint32_t arg2,void *obj)
{
	gs_async_wav_state = APP_ASYNC_OK;
}
#endif

static void read_error_max_process(void)
{
    if(Media_is_online())
    {
        msg_put(MSG_CHANGE_MODE);
        set_app_player_flag_hw_detach();
        //if read sd error, switch mode no initinal 
        clr_sd_noinitial_flag();
    }
}

static void app_player_play_wave_file( void )
{

    uint32 status = FR_BUSY;
    static uint32 readbytes;

    // if( app_player.block_played >= app_player.file_info.file_blks )   // play to end
    if( app_player.block_played >= app_player.wav_tail_pos )   // play to end
    {
        app_player.wav_header_index = 0;
        LOG_I(WAVE,"wave play end\r\n");
        app_player_wav_file_end();
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        //os_printf("size:%d\r\n",rb_get_buffer_fill_size(&pcm_rb));
        aud_fade_in_out_init(music_samplerate,MUSIC_END_FADE_OUT);
        if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
        {
            rb_fade_from_buffer_tail(get_sd_ringbuf(),0,music_channel);
        }
        else
        {
            rb_fade_from_buffer_tail(get_dac_ringbuf(),1,music_channel);
        }
        //rb_fade_from_buffer_tail(&pcm_rb,0,music_channel);
#endif
        app_player.player_flag |= APP_PLAYER_FLAG_PLAY_END;
        app_player.player_flag |= APP_PLAYER_FLAG_WAIT_DAC_EMPTY;
        padding_data_times = 0;
        padding_tick = PADDING_TICK_BETWEEN_MUSIC + os_get_tick_counter();
    }
    //else if((aud_get_buffer_size() >= buffsize) && app_player.wav_read_flag && (app_player.wav_header_index == 45) )        // playing , refresh the audio data
    else if(app_player.wav_read_flag && (app_player.wav_header_index == 45) )
    {
        if(fat_aso_rb_free_size_get() >= s_aud_buf_sz)
        {
            SD_RD_DEC_PROC(1)
        	#if (APP_ASYNC_SDCARD_ENABLE == 1)
            //APP_PLAYER_WAVE_READ_BLOCK_NUM
            if(APP_ASYNC_IDLE == gs_async_wav_state)
            {
            	gs_async_wav_state = APP_ASYNC_INIT;
            	//os_printf("		wav init,&=%d,btr=%d\r\n",&readbytes,s_rd_sz_set);
            	status = f_read_async_init(newFatfs, rbbuf, s_rd_sz_set, &readbytes, async_read_wav_cbk, NULL);
            	if(status != FR_OK)
            	{
            	    os_printf("		wav_async_init_err,status=%d\r\n",status);
            	    gs_async_wav_state = APP_ASYNC_IDLE;
            	}
                return;
			}
			else if(APP_ASYNC_OK == gs_async_wav_state)
			{
				gs_async_wav_state = APP_ASYNC_IDLE;
				status = FR_OK;
				if(s_rd_sz_set != readbytes)
				{
					LOG_I(WAV," wav read data err, set size=%d, readbytes=%d\r\n", s_rd_sz_set, readbytes);
				}
                #ifdef DBG_PLAYER_ASO
				else
				{
                    static uint32_t s_tick = 0;
                    uint32_t tick = os_timer_get_current_tick();
					if(s_tick == 0) s_tick = os_timer_get_current_tick();
					if((tick-s_tick)>=(1000)){
						s_tick = tick;
						PLY_LOG_D("wav ok\r\n");
					}
				}
                #endif
			}
            SD_RD_DEC_PROC(0)
            if(FR_BUSY == status)
            {
            	return;
            }
            SD_RD_DEC_PROC(1)
            #else
			status = f_read(newFatfs, rbbuf, s_rd_sz_set, &readbytes);
            #endif
            SD_RD_DEC_PROC(0)
			
            if(status != FR_OK)
            {
                app_player.media_err_cnt++;
                if(app_player.media_err_cnt > MEDIA_ERR_CNT_THRE)
                {
                    app_player.media_err_cnt = 0;
                    LOG_I(WAV,"app_player.media_err_cnt:%d\r\n",app_player.media_err_cnt);
                    app_player.block_played = app_player.file_info.file_blks;//end process
                    app_player.media_err_status = FR_DISK_ERR;
                    if(status == FR_DISK_ERR)
                    {
                        read_error_max_process();
                    }
                }
                return;
            }
            else
            {
                wavInfo *p_wav = &wav_info;
                uint8_t wav_bits = p_wav->bits;
                #ifdef WAV_24BIT_FILE_SUPPORT
                if(p_wav->bits == 24)
                {
                    int block_align = p_wav->block_align;
                    int total_samples = readbytes / 3;
                    if((block_align == 3) || (block_align == 6))
                    {
                        #if WAV24_TO_16BIT_EN
                        aud_cvt_wav24_to_pcm16(rbbuf, (int16_t*)rbbuf, total_samples);
                        readbytes = total_samples << 1;//16bit
                        wav_bits = 16;
                        #else
                        if((total_samples * 4) > FILE_READ_BUF_SIZE) {
                            PLY_LOG_E("buf overflow:%d > %d\n", (total_samples * 4), FILE_READ_BUF_SIZE);
                            read_error_max_process();
                            return;
                        }
                        aud_cvt_wav24_to_pcm24(rbbuf, (int32_t*)rbbuf, total_samples);
                        readbytes = total_samples << 2;//32bit
                        #endif
                    }
                }
                #endif
                app_player_fill_buffer(rbbuf, readbytes, p_wav->ch, wav_bits);
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)  
                if(get_aud_fade_in_out_state() & AUD_FADE_FINISHED)
                {
                    set_aud_fade_in_out_state(AUD_FADE_NONE);
                }
#endif
            }
			
            if(readbytes == 0)
                app_player.block_played =  app_player.file_info.file_blks;
            
            app_player.media_err_cnt = 0;
            app_player.block_played += readbytes;
        }
    }
}

#if CALC_PLAY_TIME
//return cur play time(unit:sec)
uint32_t app_player_get_playtime(void)
{
	uint32_t time = 0;
    if(mp3_play_flag == PLAY_MP3){
        time = MP3_Calc_Current_Play_Time(hMP3Decoder); 
    }else if(mp3_play_flag == PLAY_WAV) {
        time = app_player_wave_calc_cur_playtime();
    }
	return time;
}

//return cur music total time(unit:sec)
uint32_t app_player_get_totaltime(void)
{
    return app_player.total_time;
}
#endif

static void MP3_CALL app_player_play_mp3_file( void )
{	
    int err; 
    static uint16 error_cnt = 0; 
    // extern AUDIO_CTRL_BLK audio_dac_ctrl_blk;

    if(!hMP3Decoder && mp3_play_flag)
    {
        return ;
    }
   
    buf_free = fat_aso_rb_free_size_get();
    // if( (buf_free<MP3_FRAME_BUF_SIZE)&&(dma_channel_enable_get(audio_dac_ctrl_blk.dma_handle)) )
    if(buf_free < MP3_FRAME_BUF_SIZE)
    {
        #ifdef DBG_PLAYER_ASO
        static uint32 temp_cnt = 0;
        if(++temp_cnt % 100000 == 0) PLY_LOG_D("MP3:buf_free=%x\r\n",buf_free);
        #endif
        return;
    }
    SD_RD_DEC_PROC(1)
	#if (APP_ASYNC_SDCARD_ENABLE == 1)
    err = MP3Decode_async(hMP3Decoder, mp3_pcm_ptr, &pcm_size);
    SD_RD_DEC_PROC(0)
    #else    
    err = MP3Decode(hMP3Decoder, mp3_pcm_ptr, &pcm_size);
    #endif
	if(FR_BUSY == err)
	{
        PLY_LOG_DD("MP3:decode busy\r\n");
		return;
	}
    SD_RD_DEC_PROC(1)
    SD_RD_DEC_PROC(0)
	app_player.block_played = newFatfs->fptr;
    if(err != ERR_MP3_NONE)
    {	
        switch(err)
        {
            case ERR_MP3_INDATA_UNDERFLOW:
            {
                if(f_EOF(newFatfs) == FR_FILE_END)
                {
                    app_player_mp3_file_end();
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
                    aud_fade_in_out_init(music_samplerate,MUSIC_END_FADE_OUT);
                    //rb_fade_from_buffer_tail(&pcm_rb,0,music_channel);
                    if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
                    {
                        rb_fade_from_buffer_tail(get_sd_ringbuf(),0,music_channel);
                    }
                    else
                    {
                        rb_fade_from_buffer_tail(get_dac_ringbuf(),1,music_channel);
                    }
#endif
                    app_player.player_flag |= APP_PLAYER_FLAG_PLAY_END;
                    app_player.player_flag |= APP_PLAYER_FLAG_WAIT_DAC_EMPTY;
                    padding_data_times = 0;
                    padding_tick = PADDING_TICK_BETWEEN_MUSIC + os_get_tick_counter();
                }
                else
                {
                    mp3decinfo->decode_state = MP3_DECODE_FIND_ID3_INFO;
                    LOG_I(MP3,"not the end of the file\r\n");
                }
                break;
            }	
            case ERR_MP3_READ_DATA_FAILED:
            {
                LOG_I(MP3,"read error:%d\r\n",error_cnt);
                if(error_cnt++ > MEDIA_ERR_CNT_THRE)
                {
                    error_cnt = 0;
                    read_error_max_process();
                }
                break;
            }
            case ERR_MP3_FILE_END:
            {
                app_player_mp3_file_end();     
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
                aud_fade_in_out_init(music_samplerate,MUSIC_END_FADE_OUT);
                //rb_fade_from_buffer_tail(&pcm_rb,0,music_channel);
                if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
                {
                    rb_fade_from_buffer_tail(get_sd_ringbuf(),0,music_channel);
                }
                else
                {
                    rb_fade_from_buffer_tail(get_dac_ringbuf(),1,music_channel);
                }
#endif
                app_player.player_flag |= APP_PLAYER_FLAG_PLAY_END;
                app_player.player_flag |= APP_PLAYER_FLAG_WAIT_DAC_EMPTY;
                padding_data_times = 0;
                padding_tick = PADDING_TICK_BETWEEN_MUSIC + os_get_tick_counter();
                if(error_cnt++ > 30)
                {
                    error_cnt = 0;
                    read_error_max_process();
                }
                break;
            }
            case ERR_MP3_MAINDATA_UNDERFLOW:
            {
                mp3decinfo->decode_state = MP3_DECODE_FIND_SYNC_WORD;
                break;
            }
            case ERR_MP3_DECODE_MAX_ERR:
                LOG_I(MP3,"--max error---\r\n");
                app_player_mp3_file_end();
                app_player_process_file_end();
                break;
            default:
                ClearMP3((MP3DecInfo *)hMP3Decoder);
                mp3decinfo->decode_state = MP3_DECODE_FIND_SYNC_WORD;
                break;

        }
        LOG_I(MP3,"retrun err:%x,%x,%x\r\n",err,app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE,app_player.schedule_cmd & 0xFFFF );
        return;
    }  
    error_cnt = 0;

    // wait for at least 4 corret frame decode. 
    if((frame_decode_delay_count > APP_PLAYER_AUDIO_INITION_BEGIN) && init == 0)
    {
        uint32_t  interrupts_info,mask;
    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #endif
        MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo); ////get decode
        LOG_I(MP3,"bitrate=%dkb/s, samprate=%dHZ\r\n",(mp3FrameInfo.bitrate)/1000,mp3FrameInfo.samprate);
        LOG_I(MP3,"Info.nChans=%d, MPAG=%d,Info.layer=%d\r\n", mp3FrameInfo.nChans,mp3FrameInfo.version,mp3FrameInfo.layer);
        LOG_I(MP3,"outputsamps:%d\r\n",mp3FrameInfo.outputSamps);
        LOG_I(MP3,"framesize:%d\r\n",mp3decinfo->framesize);

    #if CALC_PLAY_TIME
        if(app_player.adjust_playtime) {
            app_player.adjust_playtime = 0;
            mp3decinfo->curFramecnt = newFatfs->fptr / mp3decinfo->framesize;
        }
        app_player.total_time = MP3_Calc_Play_Time(hMP3Decoder, &mp3FrameInfo);
    #endif
	
        SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
        aud_PAmute_operation(1);

        fat_aso_close();
        fat_aso_config(mp3FrameInfo.samprate, mp3FrameInfo.nChans, mp3FrameInfo.bitsPerSample);
  
        music_samplerate = mp3FrameInfo.samprate;
        music_channel = mp3FrameInfo.nChans;
        music_bit_width = mp3FrameInfo.bitsPerSample;
        if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
        {
            if(music_samplerate == 48000) 
                remain_samples = 0;
        }
        // sbc_mem_free();
        // linein_sbc_alloc_free();
        // linein_sbc_encode_init();
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        aud_fade_in_out_init(mp3FrameInfo.samprate,AUD_FADE_DURING_TIMR);
        hold_samples = MUSIC_END_FADE_OUT*music_samplerate/1000*4;
#endif
        fat_aso_open();
        fat_aso_volume_set(player_vol_bt);
        aud_PAmute_operation(0);

        if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING) && fat_aso_rb_free_size_get() > MP3_FRAME_BUF_SIZE){
            app_player_fill_zero_in_dac_buf(fat_aso_rb_free_size_get() - MP3_FRAME_BUF_SIZE);
        }
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)
        set_aud_fade_in_out_state(AUD_FADE_IN);
#endif
        
        init=2;
        SYSirq_Interrupts_Restore_Flags(interrupts_info,mask);
        init=1;
    }
    else if(init == 0)
    {
        goto decode_exit;
    }
	
    if(err == ERR_MP3_NONE )
    {
        mp3_pcm_ptr_tmp = mp3_pcm_ptr;
        MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
        
        app_player_fill_buffer((uint8_t*)mp3_pcm_ptr_tmp, pcm_size*2, mp3FrameInfo.nChans, 16);
        pcm_size = 0;

#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)  
        if(get_aud_fade_in_out_state() & AUD_FADE_FINISHED)
        {
            set_aud_fade_in_out_state(AUD_FADE_NONE);
        }
#endif
    }

decode_exit:
    if(mp3decinfo->decode_state == MP3_DECODE_FIND_SYNC_WORD)
    {
        frame_decode_delay_count++;
    }
}

static void app_player_process_schedule_cmd( void )
{
    if(!app_player.schedule_cmd) return;
    fat_aso_rb_clear();
    switch( app_player.schedule_cmd & 0xFFFF )
    {
        case APP_PLAYER_SCHEDULE_CMD_MODE_CHANGE:
            //os_printf("app_player_process_schedule_cmd\r\n");
            fat_aso_close();
            app_player.schedule_cmd = 0;
            break;
            
        case APP_PLAYER_SCHEDULE_CMD_FIX_SONG:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    //os_printf("END 3\r\n");
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    //os_printf("END 3\r\n");
                    app_player_wav_file_end();
                }

                //os_printf("next_s %d\r\n", os_get_tick_counter());
                //get_next_return = app_player_get_fix_stream_file(input_number-1); // get next file
                //input_number = 0;
                //os_printf("next_e %d\r\n", os_get_tick_counter());

                //when switch to next file, need to save the break point
                app_player_save_breakpoint();

                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                    app_player_play_pause(1);
                //app_player_print_file_info();

            }
            app_player.schedule_cmd = 0;
            break;
            
        case APP_PLAYER_SCHEDULE_CMD_NEXT_DIR:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    app_player_wav_file_end();
                }
                app_player_get_next_dir();                    // get next dir
                //os_printf("next_e %d\r\n", os_get_tick_counter());
                app_player_save_breakpoint();
                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                    app_player_play_pause(1);
                
            }
            app_player.schedule_cmd = 0;
            break;
        case APP_PLAYER_SCHEDULE_CMD_PREV_DIR:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    app_player_wav_file_end();
                }
                app_player_get_prev_dir();                    // get next dir
                //os_printf("next_e %d\r\n", os_get_tick_counter());
                app_player_save_breakpoint();
                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                    app_player_play_pause(1);
                
            }
            app_player.schedule_cmd = 0;
            break;
        case APP_PLAYER_SCHEDULE_CMD_SEL_SONG_INDEX:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                //record the schedule cmd derection here.
                // play_derect = 1;
                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    //os_printf("END 3\r\n");
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    //os_printf("END 3\r\n");
                    app_player_wav_file_end();
                }

                if(app_player_get_stream_file_from_index(app_player.schedule_param) == 0)
                {
                    //when switch to next file, need to save the break point
                    app_player_save_breakpoint();             
                }
#if (CONFIG_APP_WATCH_DSP == 1)
                if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
                {
                    app_player_report_status_to_dsp(APP_PLAYER_MUSIC_NAME,app_player.file_info.filename);
                }
#endif
                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                {
                    app_player_play_pause(1);
                }
            }
            app_player.schedule_cmd = 0;
            app_player.schedule_param = 0;
            break;
            
        case APP_PLAYER_SCHEDULE_CMD_NEXT_SONG:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    //os_printf("END 3\r\n");
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    //os_printf("END 3\r\n");
                    app_player_wav_file_end();
                }

                //os_printf("next_s %d\r\n", os_get_tick_counter());
                app_player_get_next_stream_file();                    // get next file
                //os_printf("next_e %d\r\n", os_get_tick_counter());

                //when switch to next file, need to save the break point
                app_player_save_breakpoint();
#if (CONFIG_APP_WATCH_DSP == 1)
                if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
                {
                    app_player_report_status_to_dsp(APP_PLAYER_MUSIC_NAME,app_player.file_info.filename);
                }
#endif
                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                {
                    app_player_play_pause(1);
                }
                //app_player_print_file_info();
            }
            app_player.schedule_cmd = 0;
            break;

        case APP_PLAYER_SCHEDULE_CMD_PREV_SONG:
            if(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH )
            {
                int get_pre_return=0;

                app_player.player_flag &= ~APP_PLAYER_FLAG_PLAY_END;
                if(app_player.wav_header_index != 0) //for player bug
                    app_player.wav_header_index = 0;
                if(mp3_play_flag == PLAY_MP3)
                {
                    //os_printf("END 4\r\n");
                    app_player_mp3_file_end();
                }
                else if(mp3_play_flag == PLAY_WAV)
                {
                    //os_printf("END 4\r\n");
                    app_player_wav_file_end();
                }
                //os_printf("pre_s %d\r\n", os_get_tick_counter());
                get_pre_return = app_player_get_prev_stream_file();
                //os_printf("pre_e %d\r\n", os_get_tick_counter());
                //when switch to prev file, need to save the break point
                app_player_save_breakpoint();
#if (CONFIG_APP_WATCH_DSP == 1)
                if(app_watch_get_music_mode() != MUSIC_MODE_PHONE)
                {
                    app_player_report_status_to_dsp(APP_PLAYER_MUSIC_NAME,app_player.file_info.filename);
                }
#endif
                if( app_player.schedule_cmd  & APP_PLAY_SCHEDULE_CMD_PLAY_FLAG )
                    app_player_play_pause(1);
                (void)get_pre_return;
            }
            app_player.schedule_cmd = 0;
            break;

        default:
            break;
    }

    return;
}

static void app_player_wait_dac_empty(void)
{
    if(fat_aso_rb_free_size_get() >= (PLAY_RINGBUF_SIZE >> 1))//for ringbuf
    {
        if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING) && padding_data_times<PADDING_DATA_TIMES){
            uint8_t zerobuf[1024];
            j_memset(zerobuf,0x00,sizeof(zerobuf));
            app_player_fill_buffer(zerobuf, sizeof(zerobuf), 2, 16);
            padding_data_times++;
        }
        else if(TickTimer_Is_Expired(padding_tick))
        {
            app_player.player_flag &= ~(APP_PLAYER_FLAG_WAIT_DAC_EMPTY|APP_PLAYER_FLAG_PLAY_END);
            if(app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE)
            {
                fat_aso_rb_clear();
                app_player_process_file_end();
            }
            else
            {
                aud_PAmute_operation(1);
                fat_aso_close();
            }
        }
    }
    else
    {
        if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING) 
        && (fat_aso_rb_free_size_get() > MP3_FRAME_BUF_SIZE)
        && (padding_data_times < PADDING_DATA_TIMES))
        {
            uint8_t zerobuf[1024];
            j_memset(zerobuf,0x00,sizeof(zerobuf));
            app_player_fill_buffer(zerobuf,sizeof(zerobuf),2, 16);
            padding_data_times++;
        }
    }
}

//16bit in default
static void app_player_fill_buffer(uint8_t *buff, int size, uint8_t chs, uint8_t bits)
{
    uint32_t i;
    int32_t temp_buf[FILL_OUT_BUF_SMPS * 2];//2ch
    int16_t* pcmi = (int16_t*)buff;
    int32_t* pcmo = (int32_t*)temp_buf;
    int samples = (bits == 16) ? size >> chs : (bits == 24) ? (size >> (chs + 1)) : (size / chs);
#if SD_ASO_SRC248_EN
    int SMPS_SUBSECT = sd_aso_src_s.max_smps_in;
#else
    #define SMPS_SUBSECT    FILL_OUT_BUF_SMPS
#endif
    int samples_sub = 0;
    int smps_out;
#ifdef DBG_PLAYER_ASO
    int smps_out_prv = 0;
#endif

    SD_FILL_BUF_PROC(1)
    for(i = 0; i < samples; i += samples_sub)
    {
        int remain = samples - i;
    #ifdef DBG_PLAYER_ASO
        smps_out_prv = smps_out;
    #endif
        samples_sub = (remain > SMPS_SUBSECT) ? SMPS_SUBSECT : remain;
    #if SD_ASO_SRC248_EN
        smps_out = audio_src_apply(&sd_aso_src_s, (void*)&pcmi[(chs * i) << (bits == 24)], (void*)pcmo, samples_sub);
    #else
        if(bits == 24/* && mp3_play_flag == PLAY_WAV*/)
            memcpy(pcmo, &pcmi[chs * i * 2], samples_sub * chs * 4);
        else
            aud_cvt_pcm16_to_pcm24((int16_t*)&pcmi[chs * i], pcmo, samples_sub * chs);
        smps_out = samples_sub;
    #endif
        if(chs == 1) aud_cvt_mono_to_stereo_pcm32((int32_t*)pcmo, (int32_t*)pcmo, smps_out);
    #if PLAYER_RB_16BIT_EN
        aud_cvt_pcm24_to_pcm16((int32_t*)pcmo, (int16_t*)pcmo, smps_out);
        fat_aso_rb_write((uint8_t*)pcmo, smps_out * 4);
    #else
        fat_aso_rb_write((uint8_t*)pcmo, smps_out * 8);
    #endif
    }
    SD_FILL_BUF_PROC(0)

#ifdef DBG_PLAYER_ASO
    static int cnt = 0;
    cnt += samples;
    if(cnt >= 240000){//5sec
        os_printf("smps acc:%d, cur:%d, ch:%d, free:%d\n", cnt, samples, chs, fat_aso_rb_free_size_get());
        os_printf("smps_out_prv:%d, smps_out:%d\n", smps_out_prv, smps_out);
        cnt = 0;
    }
#endif
}

static void app_player_fill_zero_in_dac_buf(uint16_t size)
{
    uint8_t zbuf[1024];
    j_memset(zbuf,0x00,sizeof(zbuf));
    while(size)
    {
        fat_aso_rb_write(zbuf,sizeof(zbuf));
        if(size>sizeof(zbuf)) size -= sizeof(zbuf);
        else break;
    }
}

void MP3_CALL app_player_play_func( void )
{
    if(app_player.player_flag & APP_PLAYER_FLAG_HALT) return;

    if( !(app_player.player_flag & APP_PLAYER_FLAG_HW_ATTACH ))
    {
        app_player_process_schedule_cmd();
        return;
    }
     
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)     
    if( (app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE) || (get_aud_fade_in_out_state() & (AUD_FADE_OUT|AUD_FADE_FINISHED)) )
#else
    if( app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE )
#endif
    {
        if((app_player.player_flag & APP_PLAYER_FLAG_PLAY_END) && (app_player.player_flag & APP_PLAYER_FLAG_WAIT_DAC_EMPTY))
        {
            app_player_wait_dac_empty();
        }
        else
        {
            if( ptn_strnicmp((char*)app_player.file_info.ext_name, "WAV", 3) == 0 )
            {
                app_player_play_wave_file();
            }
            else if( ptn_strnicmp((char*)app_player.file_info.ext_name, "MP3", 3) == 0 )
            {
                app_player_play_mp3_file();   
            }
            app_player_save_breakpoint();
        }
        CLEAR_SLEEP_TICK;
        CLEAR_PWDOWN_TICK;
    }
#if(CONFIG_AUD_FADE_IN_OUT_PLAY == 1)  
    else if( app_player.player_flag & APP_PLAYER_FLAG_WAIT_DAC_EMPTY )
    {
        app_player_wait_dac_empty();
        CLEAR_SLEEP_TICK;
        CLEAR_PWDOWN_TICK;
    }
    
    if(get_aud_fade_in_out_state() == AUD_FADE_NONE && !( app_player.player_flag & APP_PLAYER_FLAG_WAIT_DAC_EMPTY))
#endif
    {
        app_player_process_schedule_cmd();
    }
}

#if (APP_ASYNC_SDCARD_ENABLE == 1)
extern volatile APP_ASYNC_STATE gs_async_mp3_state;
int check_async_status(void)
{
    if((gs_async_wav_state == APP_ASYNC_IDLE) && (gs_async_mp3_state == APP_ASYNC_IDLE))
    {
        return 0;
    }
    return -1;
}

void app_player_wait_async_idle(const char *call_func_name)//add by borg 230109
{
    #if 0//add by borg 230110
    uint32_t time_mark = sys_time_get();
    PLY_LOG_I("wait async @ %s()\n", call_func_name);
    f_wait_async_idle(__func__);
    while(check_async_status() != 0)//wait async file opration cmp
    {
        extern int fat_delay_flag;
        if(!fat_delay_flag) app_player_play_func();
        f_async_timming_call();
        disk_async_timming_call();
        if(sys_timeout(time_mark, 1000))
        {
            PLY_LOG_E("async sate, wav:%d, mp3:%d\n", gs_async_wav_state, gs_async_mp3_state);
            PLY_LOG_I("is_mp3:%d, player_flag:%d, has_init:%d\n", mp3_play_flag, app_player.player_flag, app_player.has_init);
            break;
        }
    }
#endif
}
#else
void app_player_wait_async_idle(const char *call_func_name) { return; }
#endif

int player_get_play_status(void)
{
    return (app_player.player_flag & APP_PLAYER_FLAG_PLAY_PAUSE);
}

//get cur play dir index, range：0 ~ (get_musicdir_num() - 1)
int get_cur_music_DirIdx(void)
{
    return app_player.dir_index;
}

//get cur play file index of cur dir，range：0 ~ (get_curDirMusicfile_count(get_cur_music_DirIdx()) - 1)
int get_cur_music_dFileIdx(void)
{
    return app_player.file_index;
}

//get cur play global file index, range：0 ~ (get_musicfile_count() - 1)
int get_cur_music_gFileIdx(void)
{
    return Get_gFileIndex_By_DirFileIdx(app_player.dir_index, app_player.file_index);
}

char* get_cur_music_FileName(void)
{
    return (char*)&app_player.file_info.filename;
}

static void wav_mem_uninit(void)
{
#if 0
    memory_usage_show();
    if(rbbuf != NULL)
    {
        jfree(rbbuf);
        rbbuf = NULL;
        //os_printf("f 1\r\n");
    }
    if(aulawsmpl != NULL)
    {
        jfree(aulawsmpl);
        aulawsmpl = NULL;
        //os_printf("f 2\r\n");
    }

    if(alawtbl != NULL)
    {
        jfree(alawtbl);
        alawtbl = NULL;
        //os_printf("f 3\r\n");
    }

    if(ulawtbl != NULL)
    {
        jfree(ulawtbl);
        ulawtbl = NULL;
        //os_printf("f 4\r\n");
    }
    memory_usage_show();
#endif
    rbbuf = NULL;
    aulawsmpl = NULL;
    alawtbl = NULL;
    ulawtbl = NULL;
}

static void wav_mem_init(void)
{
    memory_usage_show();
#if 0
    rbbuf = (uint8 *) jmalloc(2048,0);
    aulawsmpl = (short *)jmalloc(2048*2,0);
    alawtbl = (short *)jmalloc(256*2,0);
    ulawtbl = (short *)jmalloc(256*2,0);
#endif
    extern uint32_t _aud_dec_begin;

    music_decoder_t *decoder = (music_decoder_t *)(&_aud_dec_begin);
    rbbuf = decoder->wav.rbbuf;
    aulawsmpl = decoder->wav.aulawsmpl;
    alawtbl = decoder->wav.alawtbl;
    ulawtbl = decoder->wav.ulawtbl;

    memcpy(alawtbl,pcm_Alaw_table,256*2);
    memcpy(ulawtbl,pcm_Ulaw_table,256*2);
    // memory_usage_show();
    dbg_show_dynamic_mem_info(1);
}

static void mp3_mem_uninit(void)
{
    os_printf("mp3_mem_uninit()\r\n");
    memory_usage_show();
    if(readBuf)
    {
        os_printf("mp3 mem free	1\r\n");
        //jfree(readBuf);
        readBuf = NULL;
        mp3decinfo->mainBuf = NULL;
    }

    if(hMP3Decoder)
    {
        os_printf("mp3 mem free	3\r\n");
        ClearMP3((MP3DecInfo *)hMP3Decoder);
        
        //MP3FreeDecoder(hMP3Decoder);
	    //os_printf("mp3 mem free	4\r\n");
        
        hMP3Decoder = NULL;
        mp3decinfo = NULL;
    }
    memory_usage_show();
}

extern SAMPLE_ALIGN u_int8 g_sbc_priv[]; // 2644+4
static void mp3_mem_init(void)
{
    L2DecodeContext *l2i;
    os_printf("mp3_mem_init\r\n");
    memory_usage_show();
#if 0
    if(NULL == hMP3Decoder)
    {
        hMP3Decoder = MP3InitDecoder();
    }
    if(NULL == hMP3Decoder)
    {
        os_printf("hMP3Decoder_malloc_fail\r\n");
        memory_usage_show();
        goto exit;
    }
#endif
    extern uint32_t _aud_dec_begin;

    music_decoder_t *decoder = (music_decoder_t *)(&_aud_dec_begin);
    j_memset(decoder,0x00,sizeof(music_decoder_t));
    mp3decinfo = &(decoder->mp3.mp3DecInfo);

    mp3decinfo->FrameHeaderPS =     (void *)&(decoder->mp3.fh);
    mp3decinfo->SideInfoPS =        (void *)&(decoder->mp3.si);
    mp3decinfo->ScaleFactorInfoPS = (void *)&(decoder->mp3.sfi);
    mp3decinfo->HuffmanInfoPS =     (void *)&(decoder->mp3.hi);
    mp3decinfo->DequantInfoPS =     (void *)&(decoder->mp3.di);
    mp3decinfo->IMDCTInfoPS =       (void *)&(decoder->mp3.mi);
    mp3decinfo->SubbandInfoPS =     (void *)&(decoder->mp3.sbi);
    mp3decinfo->L2DecInfo =         (void *)&(decoder->mp3.l2i);

    hMP3Decoder = (HMP3Decoder)&(decoder->mp3.mp3DecInfo);
    //mp3decinfo = (MP3DecInfo*)hMP3Decoder;
#if 1
    readBuf = (uint8*)&g_sbc_priv;
    memset(readBuf, 0, 2644+4);
#else
    readBuf= (unsigned char*)jmalloc(READBUF_SIZE, M_ZERO);   //FOR READ PLY_LOG_I
#endif
    mp3decinfo->mainBuf = readBuf; 
    mp3decinfo->mainBuf_ptr = mp3decinfo->mainBuf + BIT_RESVOR_SIZE;
    mp3decinfo->mainBuf_len = 0;
    mp3decinfo->err_cnt = 0;
    mp3_pcm_ptr = (short*)mp3decinfo->HuffmanInfoPS;
	//mp3_pcm_ptr = (short*)jmalloc(4608, M_ZERO); 
	
    l2i = (L2DecodeContext *)(mp3decinfo->L2DecInfo);
    l2i->sb_samples = (int32_t *)mp3decinfo->SubbandInfoPS;
    l2i->synth_buf = (int16_t *)mp3decinfo->IMDCTInfoPS;


    PLY_LOG_I("mp3_mem_init %p,%p,%p\r\n", readBuf, mp3_pcm_ptr, hMP3Decoder);
    // memory_usage_show();
    dbg_show_dynamic_mem_info(1);
    ClearMP3((MP3DecInfo *)hMP3Decoder);
    return;

 //exit:
    while(3254)
    {
        os_printf("F");
    }
    return;
}

void app_player_restore_current_music(void)
{
	fat_aso_close();
    fat_aso_config(music_samplerate, music_channel, music_bit_width);
    fat_aso_volume_set(player_vol_bt);
	if(app_bt_flag2_get( APP_FLAG2_STEREO_STREAMING))
    {
    	if(A2DP_SAMPLE_RATEX==48)
        {
            sbc_encoder_init(sbc_encoder,48000,2);
        }
        else if(A2DP_SAMPLE_RATEX==44)
        {
            sbc_encoder_init(sbc_encoder,44100,2);
        }
        else if(A2DP_SAMPLE_RATEX==32)
        {
            sbc_encoder_init(sbc_encoder,32000,2);
        }
        else if(A2DP_SAMPLE_RATEX==16)
        {
            sbc_encoder_init(sbc_encoder,16000,2);
        }
	}
    fat_aso_open();
}

uint8_t app_player_is_active(void)
{
    if((SYS_WM_SDCARD_MODE == get_app_mode() || SYS_WM_UDISK_MODE == get_app_mode())
    && (app_player.player_flag & (APP_PLAYER_FLAG_PLAY_PAUSE | APP_PLAYER_FLAG_WAIT_DAC_EMPTY)))
    {
        return 1;
    }
    return 0;
}

void app_player_set_fixed_dir(uint16_t dirIdx, uint8_t en)
{
    app_player.fixed_dir_en = en;
    app_player.fixed_dir_idx = dirIdx;
}

void app_player_report_status_to_dsp(APP_PLAYER_STATUS sta,void*argv)
{
    if(app_player_status_cb)
    {
        app_player_status_cb(sta,argv);
    }
}

uint8_t get_app_player_volume(void)
{
    return player_vol_bt;
}

void app_player_callback_register(app_player_status_callback_fxn cb)
{
    app_player_status_cb = cb;
}

void set_app_player_play_mode(uint8_t mode)
{
    if(mode == APP_PLAYER_MODE_PLAY_ALL_CYCLE 
        ||mode == APP_PLAYER_MODE_PLAY_ONE 
        ||mode == APP_PLAYER_MODE_PLAY_RANDOM
        ||mode == APP_PLAYER_MODE_END)
    {
            appPlayerPlayMode = mode;
    }
    PLY_LOG_I("==== SdCarePlayMode %d\n", mode);
}

uint8_t get_app_player_play_mode(void)
{
    return appPlayerPlayMode;
}

void set_app_player_flag_hw_attach(void)
{
    app_player.player_flag |= APP_PLAYER_FLAG_HW_ATTACH;
}

void set_app_player_flag_hw_detach(void)
{
    app_player.player_flag &= ~APP_PLAYER_FLAG_HW_ATTACH;
}


/* ------------------------------------------------- *///debug show
void app_player_debug_show()
{
#if 0//app_player debug
    if(!app_player_is_active())
    {
        PLY_LOG_D("app_player is inactive\n");
        goto RET;
    }
    PLY_LOG_D("TotalNum Dir:%d, dFile:%d, gFile:%d\n", 
        get_music_DirNumTotal(), get_music_dFileNumTotal(0), get_music_gFileNumTotal());
    PLY_LOG_D("PlayIdx  Dir:%d, dFile:%d, gFile:%d\n", 
        get_cur_music_DirIdx(), get_cur_music_dFileIdx(), get_cur_music_gFileIdx());
    PLY_LOG_D("cur file:%s\n", get_cur_music_FileName());
    PLY_LOG_D("[%02d:%02d]/[%02d:%02d]\n",
        app_player_get_playtime() / 60, app_player_get_playtime() % 60,
        app_player_get_totaltime() / 60, app_player_get_totaltime() % 60);
    #if 0
    char fn_buf[256];
    Get_File_Name_From_Number(0, &fn_buf[0]);
    PLY_LOG_D("0:%s\n", fn_buf);
    Get_File_Name_From_Number(1, &fn_buf[0]);
    PLY_LOG_D("1:%s\n", fn_buf);
    #endif
RET:
    return;
#endif
}
#endif




