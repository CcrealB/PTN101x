#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "karacmd.h"   //cjq++
#if WAV_FILL_48K_EN
#include "src.h"
#endif
#ifdef AUD_WAV_TONE_SEPARATE
#include "sbc_decoder_soft.h"
#endif

static app_wave_play_ctrl_t app_wave_ctrl;
#if (!defined(AUD_WAV_TONE_SEPARATE))
uint8_t mp3_need_pause = 0; //若正在播放mp3, 有提示音时要先暂停mp3
#endif

#if (CONFIG_PROMPT_WAVE_AS_ALAW == 1)
CONST short prompt_Alaw_table[256] =  {
      -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
      -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
      -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
      -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
     -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
     -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
     -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
     -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
       -43,    -41,    -47,    -45,    -35,    -33,    -39,    -37,
       -59,    -57,    -63,    -61,    -51,    -49,    -55,    -53,
       -11,     -9,    -15,    -13,     -3,     -1,     -7,     -5,
       -27,    -25,    -31,    -29,    -19,    -17,    -23,    -21,
      -172,   -164,   -188,   -180,   -140,   -132,   -156,   -148,
      -236,   -228,   -252,   -244,   -204,   -196,   -220,   -212,
       -86,    -82,    -94,    -90,    -70,    -66,    -78,    -74,
      -118,   -114,   -126,   -122,   -102,    -98,   -110,   -106,
       688,    656,    752,    720,    560,    528,    624,    592,
       944,    912,   1008,    976,    816,    784,    880,    848,
       344,    328,    376,    360,    280,    264,    312,    296,
       472,    456,    504,    488,    408,    392,    440,    424,
      2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
      3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
      1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
      1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
        43,     41,     47,     45,     35,     33,     39,     37,
        59,     57,     63,     61,     51,     49,     55,     53,
        11,      9,     15,     13,      3,      1,      7,      5,
        27,     25,     31,     29,     19,     17,     23,     21,
       172,    164,    188,    180,    140,    132,    156,    148,
       236,    228,    252,    244,    204,    196,    220,    212,
        86,     82,     94,     90,     70,     66,     78,     74,
       118,    114,    126,    122,    102,     98,    110,    106
};
#endif
void app_wave_file_play_init(void)
{
    memset( &app_wave_ctrl, 0, sizeof(app_wave_ctrl));
    memset( &app_wave_ctrl.voice_num, 0xFF, 32 );
    app_wave_ctrl.voice_index = 0xFF;
    app_wave_ctrl.file_id_pending_saved = APP_WAVE_FILE_ID_NULL;
    return;
}

static uint32_t s_sbc_fill_cnt = 0;
static __inline app_wave_handle_t app_get_wave_handle( void )
{
    return &app_wave_ctrl;
}

int app_get_wave_info(uint32_t *freq_ptr, uint32_t * channel_ptr, uint32_t *vol_s_ptr)
{
    if(freq_ptr)
    {
        *freq_ptr = app_wave_ctrl.freq;
    }

    if(channel_ptr)
    {
        *channel_ptr = app_wave_ctrl.channel;
    }

    if(vol_s_ptr)
    {
        *vol_s_ptr = app_wave_ctrl.vol_s;
    }
    return 0;
}

void app_wave_file_volume_init( uint8_t vol )
{
    app_wave_ctrl.vol = vol;
}

void app_wave_file_vol_s_init( uint8_t vol )
{
    app_wave_ctrl.vol_s = vol;
}

void app_wave_file_aud_notify( uint32_t freq, uint16_t channel, uint8_t vol )
{
    app_wave_ctrl.freq = freq;
    app_wave_ctrl.channel = channel;
    app_wave_ctrl.vol_s = vol;
}

void app_wave_file_voice_num_set( char * num )
{
    int len;
    int i;
    int count = 0;

    memset( &app_wave_ctrl.voice_num, 0xFF, 32 );

    if( num == NULL )
    {
        app_wave_ctrl.voice_index = 0xFF;

        return;
    }

    len = j_strlen(num);

    for( i = 0; i < len; i++ )
    {
        if( num[i] >= '0' && num[i] <= '9' )
        {
            app_wave_ctrl.voice_num[count] = num[i] - '0';
            count++;
        }
        else
            continue;
    }

    if( count > 0 )
        app_wave_ctrl.voice_index = 0;
}

#if WAV_FILL_48K_EN
void *src_buff = NULL;
//buff : mono 8k16bit origin wav, convert 1ch8k16bit to 2ch48k32bit
void app_wave_file_fill_buffer(uint8_t *buff, uint16_t size )
{
    // os_printf("sz:%d ", size);
    uint16_t size_fill = size * 4 * 6;//2ch * 32bit/16bit * 48k/8k
    uint8_t *buff_st = jmalloc(size_fill, M_ZERO);
    if(buff_st == NULL) {
        LOG_E(WAV,"wav buff_st jmalloc fail!!!\r\n");
        return;
    }

    int i = 0;
    uint16_t samples = size >> 1;
    int16_t *pcmi = (int16_t*)buff_st;
    int32_t *pcmo = (int32_t*)buff_st;
    //big little cvt
    for(i = 0; i < samples; i++){
        pcmi[i] = (*(buff + 2*i)) | (*(buff + 2*i + 1) << 8);
    }

    //resample mono 8k16b to mono 48k32b
    uint16_t smps_out = 0;
    if(src_buff){
        smps_out = src_248_exec_16i24o(src_buff, (int16_t*)pcmi, (int32_t*)pcmo, samples);
    }

    //cvt 48k32b mono to stereo
    for(i = smps_out - 1; i >= 0; i--) {
        pcmo[2*i] = pcmo[i];
        pcmo[2*i+1] = pcmo[i];
    }

    wav_aud_dac_fill_buffer((uint8_t*)pcmo, size_fill);

    if (NULL != buff_st) {
        jfree(buff_st);
        buff_st = NULL;
    }
}
#else
void app_wave_file_fill_buffer(uint8_t *buff, uint16_t size )
{
    uint16_t i = 0;
#if (CONFIG_PROMPT_WAVE_AS_ALAW == 1)
    uint8_t *buff_st = jmalloc(size*8,M_ZERO);
    int16_t smpl = 0;
#else
    uint8_t *buff_st = jmalloc(size*4,M_ZERO);
#endif
    uint8_t* src = buff;

    if(buff_st == NULL)
    {
        LOG_E(WAV,"wavbuff_st jmalloc fail!!!\r\n");
        return;
    }
#ifdef AUD_WAV_TONE_SEPARATE
    int16_t* dst = (int16_t*)buff_st;

    if(app_get_wave_handle()->freq == 8000)
    {
        int16_t smpl = 0;
        while(i < size){
            smpl = (*(src + i)) | (*(src + i + 1) << 8);
            *dst++ = smpl;
            *dst++ = smpl;
            i += 2;
        }
        wav_aud_dac_fill_buffer( buff_st, size*2);
    }
    else //16k16bit default
    {
        while(i < size)
        {
            *dst++ = (*(src + i)) | (*(src + i + 1) << 8);
            i += 2;
        }
        wav_aud_dac_fill_buffer( buff_st, size);
    }


#else //8k32bit2ch
    int32* dst = (int32*)buff_st;

#if (CONFIG_PROMPT_WAVE_AS_ALAW == 1)
    while(i < size)
    {
        smpl = prompt_Alaw_table[(*(src + i)) ^ 0x80];
        *(buff_st + 4*i) = (smpl & 0x00ff);
        *(buff_st + 4*i + 1) = ((smpl&0xff00) >> 8);
        *(buff_st + 4*i + 2) = (smpl & 0x00ff);
        *(buff_st + 4*i + 3) = ((smpl&0xff00) >> 8);
        i++;
    }
#else
    while(i < size)
    {
        int32 t = (*(src + i) << 16) | (*(src + i + 1) << 24);
    
        t >>= 8;
        *dst++ = t;
        *dst++ = t;
        i += 2;
    }
#endif

#if (CONFIG_PROMPT_WAVE_AS_ALAW == 1)
    wav_aud_dac_fill_buffer( buff_st, size*8);
#else
    wav_aud_dac_fill_buffer( buff_st, size*4);
#endif
#endif//AUD_WAV_TONE_SEPARATE
    if (NULL != buff_st)
    {
        jfree(buff_st);
        buff_st = NULL;
    }
}
#endif
uint8_t app_wave_file_play_start( int file_id )
{
	extern void PlayWaveStart(uint16_t id);	//yuan++
	PlayWaveStart(file_id);

    static int last_file_id = APP_WAVE_FILE_ID_NULL,file_conn_flag = 0;
    app_wave_handle_t wave_h = app_get_wave_handle();
    int len,size,frms;
    int continue_flag = APP_WAVE_FILE_ID_VOICE_CONTINUE == (file_id & APP_WAVE_FILE_ID_VOICE_CONTINUE);
    uint16_t page_len = FLASH_PAGE_LEN;
    int i;

    file_id &= ~APP_WAVE_FILE_ID_VOICE_CONTINUE;

#if (CONFIG_CUSTOMER_BUTTON_HID_MODE == 1)
	app_customer_hid_set_cnt(file_id);
#endif

#if (CONFIG_CUSTOMER_1V2_CON_MODE == 1)
	app_customer_1v2_con_set_cnt(file_id);
#endif

    if((app_bt_flag1_get(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED)) && (file_id == APP_WAVE_FILE_ID_LOW_BATTERY)) return WAV_LOW_BATTERY_WHILE_SCO;

    if(file_id < 0 || file_id >= WAVE_EVENT) return WAV_FILE_ID_INVALID;

    if(-1 == app_env_get_wave_type(file_id)) return WAV_FILE_TYPE_INVALID;

    if(app_bt_flag1_get(APP_FLAG_POWERDOWN) && (file_id!=APP_WAVE_FILE_ID_POWEROFF)) return WAV_POWER_OFF_WHILE_NOT_PWD;

    if(get_Charge_state() && app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))  return WAV_POWER_OFF_WHILE_NOT_PWD;

	if(last_file_id == APP_WAVE_FILE_ID_POWEROFF) return WAV_POWER_OFF_WHILE_NOT_PWD;

	if(((app_linein_get_state() == LINEIN_W4_ATTACH) || app_bt_flag2_get(APP_FLAG2_LED_LINEIN_INIT))
		&& (file_id != APP_WAVE_FILE_ID_LINEIN_MODE)
		&& (file_id != APP_WAVE_FILE_ID_START))
	    return WAV_FILE_LINEIN_MODE;

    LOG_I(WAV,"wave_start, ID:%d, type:%d, playing:%d\r\n", file_id, app_env_get_wave_type(file_id), wave_h->flag_playing);
    
    if(1 == wave_h->flag_playing)
    {
        if((file_id == APP_WAVE_FILE_ID_POWEROFF) && (wave_h->file_id != APP_WAVE_FILE_ID_POWEROFF))
        {
            wave_h->file_id = INVALID_WAVE_EVENT;
            wave_h->func = NULL;
            app_wave_file_play_stop();
        }
        else
        {
            wave_h->file_id_pending_saved = file_id;
            return WAV_NO_ERROR;
        }
    }

	if(file_id == APP_WAVE_FILE_ID_CONN) file_conn_flag = 1;

    wave_h->mute_tick = 0;
    wave_h->type = app_env_get_wave_type(file_id);

    if(!continue_flag)
    {
        app_sleep_func(0);
    }

#ifdef CONFIG_SBC_PROMPT
    //同一时间来两提示音，若两个提示音不同类型，要重新初始化audio
    if(app_env_get_wave_type(file_id) != app_env_get_wave_type(last_file_id))
    {
        app_wave_file_play_stop();
    }
#endif

#ifdef CONFIG_SBC_PROMPT
    if((INTER_WAV == wave_h->type) || (INTER_SBC == wave_h->type))
#else
    if(wave_h->type == 0)
#endif
    {
        wave_h->page_index = app_env_get_wave_page_index(file_id);

        if(wave_h->page_index == -1) return WAV_FILE_PAGE_INDEX_INVALID;

		if(file_id == APP_WAVE_FILE_ID_DISCONN)
		{
			if((file_conn_flag == 0) && app_check_bt_mode(BT_MODE_1V1)) return WAV_NO_ERROR;
            
			file_conn_flag = 0;
		}

#if(CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif
        enable_audio_ldo();

        //memcpy(wave_h->page_buff, (void*)(wave_h->page_index << 8), page_len);
        flash_read_data(wave_h->page_buff, (uint32_t)(wave_h->page_index << 8), page_len);

        memcpy(&len, &wave_h->page_buff[0], 4);

        if(len == 0xFFFFFFFF) return WAV_FILE_LEN_INVALID;

        last_file_id = file_id;

        wave_h->page_end = wave_h->page_index + (len/page_len);

#ifdef CONFIG_SBC_PROMPT
        if(INTER_SBC == wave_h->type)
        {
            if(wave_h->page_buff[4] != 0x9c) return SBC_FILE_SYNCWORD_INVALID;

            wave_h->sbc_code_ptr = (uint8_t *)(wave_h->page_index << 8) + 4;
        }
#endif

        if( app_bt_flag1_get( APP_FLAG_LINEIN ))
        {
            BK3000_Ana_Line_enable(0);
            wav_aud_volume_mute(1);
        }
#if (CONFIG_APP_MP3PLAYER == 1) && (!defined(AUD_WAV_TONE_SEPARATE))
        else if(app_is_mp3_mode() && player_get_play_status())
        {
            mp3_need_pause = 1;
            app_player_button_play_pause();
        }
#endif

        wav_aud_mic_open(0);

        if(wave_h->flag_playing == 0)
        {
            #ifdef AUD_WAV_APP_AS_COEXIST
            app_bt_flag1_set(APP_FLAG_WAVE_PLAYING, 0);
            #else
            app_bt_flag1_set(APP_FLAG_WAVE_PLAYING, 1);
            #endif

            if(!continue_flag)
            {
                wav_aud_dac_close();
            }
            
            // frms = ((AUDIO_DAC_BUFF_LEN >> 1)/1024 - 1);
            
#ifdef CONFIG_SBC_PROMPT
            if((INTER_WAV == wave_h->type)
               || (EXT_WAV == wave_h->type))
            {
                if(!continue_flag)
                {
                #ifdef AUD_WAV_TONE_SEPARATE
                    wav_aud_dac_config(16000, 1, 16);
                    wave_h->freq = 16000;
                    LOG_I(WAV,"wav default sample rate:%d\n", wave_h->freq);
                #else
                    wav_aud_dac_config(8000, 1, 16);
                #endif
                }
            }
            else if(INTER_SBC == wave_h->type)
            {
                int frequency;
                
                s_sbc_fill_cnt = 0;
                switch(((wave_h->page_buff[5] >> 6) & 0x03))
                {
                    case 1:
                        frequency = 8000;//32000;
                    break;
                    
                    case 2:
                        frequency = 44100;
                    break;
                    
                    case 3:
                        frequency = 48000;
                    break;
                    
                    case 0:
                    default:
                        frequency = 16000;
                    break;
                }
                LOG_I(WAV,"sbc wave sample rate:%d\n", frequency);

                if(!continue_flag) wav_aud_dac_config(frequency, 1, 16);

                wave_h->block_size = wave_h->page_buff[6] * 2 + 8;
                wave_h->page_end   = (uint32_t)(wave_h->sbc_code_ptr) + len;
                wave_h->freq = frequency;
            }
            #if WAV_FILL_48K_EN
            if(!continue_flag)
            {
                int32_t max_frame_size = 192;//accroding 1ms*48*4byte*1ch
                int32_t channels = 1;
                int32_t srcu6_size = 0;
                int32_t ini_ret = 0;
                int src_id = src_248_id_get(8000);
                srcu6_size = src_248_size(src_id, max_frame_size, channels);
			    if(src_buff) jfree(src_buff);
                src_buff = jmalloc(srcu6_size, M_ZERO);
                if(src_buff)
                {
                    ini_ret = src_248_init(src_buff, src_id, max_frame_size, channels);
                    if(ini_ret != 0)
                    {
                        LOG_E(WAV, "src_init fail:%d\n", ini_ret);
                        jfree(src_buff);
                        src_buff = NULL;
                    }
                }
                LOG_I(WAV,"src size:%u, src_id:0x%X, ini_ret:%d\n", srcu6_size, src_id, ini_ret);
            }
            #endif
#else
            if(!continue_flag)
            {
                wav_aud_dac_config(8000, 1, 16);
            }
#endif
            wav_aud_dac_set_volume(wave_h->vol);

            wav_aud_dac_open();
        }
        else
        {
            size = wav_aud_dac_get_free_buffer_size()-2;

        #ifdef AUD_WAV_TONE_SEPARATE
            frms = size / page_len;//for dsp is 16k16bit1ch, it can put 1:1 size data.
            // if(app_get_wave_handle()->freq == 8000) frms >>= 1;//wav dont support 8k
        #elif WAV_FILL_48K_EN
            frms = size / 24 / page_len;//for 8k16bit1ch->48k32bit2ch, Only 1/24 of the data can be placed
        #else
#if(CONFIG_PROMPT_WAVE_AS_ALAW == 1)
            frms = (size >> 4)/page_len;
#else
            frms = (size >> 2)/page_len;
#endif
        #endif
        }

#ifdef CONFIG_SBC_PROMPT
        if( INTER_WAV == wave_h->type)
        {
            LOG_I(WAV,"wave_start: wav, frms:%d\n", frms);
            app_wave_file_fill_buffer(	&wave_h->page_buff[8], page_len-8 );

            wave_h->page_index++;

            for( i = 0; i < MIN(frms, wave_h->page_end - wave_h->page_index - 1); i++ )
            {
                //memcpy(wave_h->page_buff, (void*)(wave_h->page_index << 8), page_len);
                flash_read_data(wave_h->page_buff, (uint32_t)(wave_h->page_index << 8), page_len);

                app_wave_file_fill_buffer(	wave_h->page_buff, page_len );

                wave_h->page_index++;
            }
        }
        else if(INTER_SBC == wave_h->type)
        {
            wave_h->status = PROMPT_START;
            LOG_I(WAV,"wave_start: sbc\r\n");
#ifdef AUD_WAV_TONE_SEPARATE
            sbc_decoder_soft_init(wav_sbc_decoder);
#else
            sbc_decoder_init(wav_sbc_decoder);
#endif
            sbc_decoder_ctrl(wav_sbc_decoder, SBC_DECODER_CTRL_CMD_SET_OUTPUT_STEREO_FLAG, 0);
            sbc_decoder_ctrl(wav_sbc_decoder, SBC_DECODER_CTRL_CMD_SET_OUTPUT_PCM_WIDTH, 16);
        }
#else
        app_wave_file_fill_buffer(	&wave_h->page_buff[8], page_len-8 );

        wave_h->page_index++;

        for(i = 0; i < MIN(frms, wave_h->page_end - wave_h->page_index - 1); i++)
        {
            memcpy(wave_h->page_buff, (void *)(wave_h->page_index << 8 ), page_len);
            //flash_memcpy(wave_h->page_buff,  (uint8_t *)(wave_h->page_index * FLASH_PAGE_LEN_CRC ), FLASH_PAGE_LEN_CRC,0);
            app_wave_file_fill_buffer(wave_h->page_buff, page_len);
            wave_h->page_index++;
        }
#endif

        wave_h->flag_playing = 1;
        wave_h->file_id = file_id;

        wav_extPA_open(1);
        wav_extPA_set_req(1);
                
        return WAV_NO_ERROR;
    }

    return WAV_NO_ERROR;
}

uint8_t start_wave_and_action(uint32_t file_id, wav_cb_func cb)
{
    uint8_t ret;
    WAV_PRT("start_wave_and_action ... \n");

    //file uncorrect, needn't play
    if(check_wave_file_correct(file_id))
    {
        WAV_PRT("start_mode_change_wave no file!!!\n");
        if(cb)
        {
            cb();
        }
        return WAV_FILE_ID_INVALID;
    }

    //error status,needn't play
    ret = app_wave_file_play_start(file_id);
    //if(ret && (3 != ret)) /* ??????????????? */
    if(ret)
    {
        WAV_PRT("start_wave ERROR STATUS!!!\n");
        if(cb)
        {
            cb();
        }

        return ret;
    }
    else
    {
        // if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING) && (cb))//Modification without shutdown
        if(!app_wave_playing() && (cb))//Modification without shutdown
			cb();		
		else
        	app_get_wave_handle()->func = cb;
    }
    return WAV_NO_ERROR;
}

int app_wave_playing( void )
{
#if 0//def AUD_WAV_TONE_SEPARATE
    return 0;
#else
    app_wave_handle_t wave_h = app_get_wave_handle();

    return wave_h->flag_playing;
#endif
}

#ifdef CONFIG_SBC_PROMPT
int app_wave_is_sbc_playing(void)
{
    app_wave_handle_t wave_h = app_get_wave_handle();
    return ((wave_h->type == INTER_SBC) || (wave_h->type == EXT_SBC));
}

int app_wave_check_type( uint8_t type)
{
    app_wave_handle_t wave_h = app_get_wave_handle();
	if(wave_h->type==type){
		return 1;
	}else{
		return 0;
	}
}

int app_wave_check_status( uint8_t status)
{
    app_wave_handle_t wave_h = app_get_wave_handle();
	if(wave_h->status==status){
		return 1;
	}else{
		return 0;
	}
}

void app_wave_set_status( uint8_t status)
{
    app_wave_handle_t wave_h = app_get_wave_handle();
	wave_h->status=status;
}

uint8_t app_wave_get_blocksize(void)
{
    app_wave_handle_t wave_h = app_get_wave_handle();
	return wave_h->block_size;
}

int app_wave_fill_sbc_node(uint8_t *obj_ptr)
{
	int ret = 0;
    app_wave_handle_t wave_h = app_get_wave_handle();
	uint8_t* wave_end = (uint8_t*)(wave_h->page_end);

	if((wave_end-wave_h->sbc_code_ptr)>=wave_h->block_size)
	{
		//memcpy(obj_ptr,wave_h->sbc_code_ptr,wave_h->block_size);
		flash_read_data(obj_ptr, (uint32_t)(wave_h->sbc_code_ptr), wave_h->block_size);

		ret = 1;
		wave_h->sbc_code_ptr += wave_h->block_size;
        wave_h->status = (wave_end-wave_h->sbc_code_ptr)>=wave_h->block_size ? PROMPT_WORKING : PROMPT_FILL_ZERO;
	}
	else if(s_sbc_fill_cnt < 32)  // 32 * 16ms = 512ms
    {
        s_sbc_fill_cnt++;
        wave_h->status = PROMPT_FILL_ZERO;
    }
    else
		wave_h->status = PROMPT_EMPTY;

	return ret;
}
#endif

void app_wave_file_play( void )
{
    app_wave_handle_t wave_h = app_get_wave_handle();
    uint16_t size = 0, i, frms;
    uint16_t page_len = FLASH_PAGE_LEN;

    if( wave_h->flag_playing )
    {
#if(CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    #ifdef USER_KARAOK_MODE
        if(mailbox_mcu2dsp_is_busy()) return;
    #endif
        CLEAR_SLEEP_TICK;
	#ifdef CONFIG_SBC_PROMPT
		if(INTER_SBC==wave_h->type)
		{
			app_wave_file_sbc_fill();
		}
        else if(INTER_WAV==wave_h->type)
	#else
        if( wave_h->type == 0 )
	#endif
        {
            size = wav_aud_dac_get_free_buffer_size()-2;
            if( wave_h->page_index < wave_h->page_end )
            {
            #ifdef AUD_WAV_TONE_SEPARATE
                frms = size / page_len;//for dsp is 16k16bit1ch, it can put 1:1 size data.
                if(frms > 4) frms = 4;//dsp 16k wav ringbuff size is 16pt/ms*2byte/pt*4ms/fra*10fra=1280, page_len = 256 -> 256*1=256 -> 1280/256=4.
                // if(app_get_wave_handle()->freq == 8000) frms >>= 1;//wav dont support 8k //div 2 for 8k to 16k need 2 times of buff space
            #elif WAV_FILL_48K_EN
                frms = size / 24 / page_len;//for 8k16bit1ch->48k32bit2ch, Only 1/24 of the data can be placed
                if(frms > 2) frms = 2;//dac ringbuff size is 16384, page_len = 256 -> 256*24=6144. 
            #else
				#if(CONFIG_PROMPT_WAVE_AS_ALAW == 1)
		  			frms = (size >> 4)/page_len;
				#else
					frms = (size >> 2)/page_len;
				#endif

                frms = (frms > 8)?8:frms;
            #endif

                for( i = 0; i < frms; i++)
                {
                    flash_read_data(wave_h->page_buff, (uint32_t)(wave_h->page_index << 8), page_len);
					app_wave_file_fill_buffer(	wave_h->page_buff, page_len );

                    wave_h->page_index++;
                    if( wave_h->page_index >= wave_h->page_end )
                        break;
                }
                #ifndef USER_KARAOK_MODE
                size = wav_aud_dac_get_free_buffer_size();
                if( size >= AUDIO_DAC_BUFF_LEN - 8 )
                {
                    WAV_PRT("app_wave_file_play, app_wave_file_play_stop 1\r\n");
                    app_wave_file_play_stop();
                    return;
                }
                #endif
            }
            else
            {
                #ifdef AUD_WAV_TONE_SEPARATE
                if( size >= (1280 - 320))//stop when wav remain 10ms [dsp wav ringbuff size is 16pt/ms*2byte/pt*4ms/fra*10fra=1280, 10ms*16*2*1]
                #elif WAV_FILL_48K_EN
                if( size >= (AUDIO_DAC_BUFF_LEN - 3840))//10ms*48*4*2 max frame in dsp
                #else
                if( size >= AUDIO_DAC_BUFF_LEN - 8 )
                #endif
                {
                    WAV_PRT("app_wave_file_play,_stop2:%d,:%d,%d\r\n", size, wave_h->page_index, wave_h->page_end);
                    app_wave_file_play_stop();
                    return;
                }
            }
        }
        else
        {
            wave_h->page_index++;
            if( wave_h->page_index >= wave_h->page_end )
            {
                WAV_PRT("_stop3\r\n");
                app_wave_file_play_stop();
                return;
            }
        }

        extPA_set_req(1);
        if( wave_h->mute_tick <= 2 )
        {
            wave_h->mute_tick++;
        }
        else
        {
            wav_aud_PAmute_operation(0);
        }
    }
    return;
}

//correct--0, others--uncorrect
uint8_t check_wave_file_correct(int file_id)
{
    uint32_t page_index = 0;
    int len = 0;
    app_env_handle_t env_h = app_env_get_handle();

    if(env_h->env_cfg.used != 1) //no wave file have been configured
    {
        return 1;
    }

    page_index = app_env_get_wave_page_index(file_id);

    if(page_index==(-1))       //file not exist, needn't play mode switch wave
    {
        return 1;
    }

    flash_read_data((uint8_t *)&len,  (uint32_t)(page_index << 8), (uint32_t)4 );

    if(( len == 0xFFFFFFFF) || (len==0)) // not correct wav file
    {
        return 1;
    }

    return 0;
}

void app_wave_file_play_stop( void )
{
    app_wave_handle_t wave_h = app_get_wave_handle();
    app_env_handle_t env_h = app_env_get_handle();

    extern void PlayWaveStop(uint16_t id);	//yuan++
    PlayWaveStop(wave_h->file_id);


    #if WAV_FILL_48K_EN
    int continue_flag = APP_WAVE_FILE_ID_VOICE_CONTINUE == (wave_h->file_id & APP_WAVE_FILE_ID_VOICE_CONTINUE);
    if ((NULL != src_buff) && (!continue_flag)) {
        jfree(src_buff);
        src_buff = NULL;
    }
    #endif
    LOG_I(WAV,"wav stop, ID:%d, playing:%d,%d\r\n", wave_h->file_id, wave_h->flag_playing, !!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING));
    //===============================================
    if(wave_h->flag_playing == 0)
    {
		if (wave_h->file_id == APP_WAVE_FILE_ID_POWEROFF)
		{
	        wav_aud_dac_close();
	        wav_aud_volume_mute(1);
		}
#ifndef AUD_WAV_APP_AS_COEXIST
		else
        	app_audio_restore();
#endif
        return;
    }

    wave_h->flag_playing = 0;
    wave_h->mute_tick = 0;

    if( wave_h->type == 0 )
    {
        wave_h->page_index = 0;
        wave_h->page_end = 0;
    }

    if(!hfp_has_sco() && !app_bt_flag1_get(APP_FLAG_MUSIC_PLAY|APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
    {
   	    wav_aud_PAmute_operation(1);
    }

	if(app_bt_flag1_get(APP_FLAG_LINEIN))
		adcmute_cnt = 50;

#if(CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV); 
#endif

#if A2DP_ROLE_SOURCE_CODE
    if(app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE)) BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    WAV_PRT("app_wave_file_play_stop1\n");
    // voice num
    if(( env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_ADDR_AUDIO_DIAL )
	&&(app_bt_flag1_get(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
    &&(wave_h->file_id != APP_WAVE_FILE_ID_POWEROFF))
    {
        int i = 0,need_play_incoming_number = 0;
        while(!check_wave_file_correct(APP_WAVE_FILE_ID_VOICE_NUM_0+i))//check 0-9 wave file
        {
            i++;
            if(i > 9)
            {
                need_play_incoming_number = 1;
                break;
            }
        }
        if(( wave_h->voice_index != 0xFF  )&&(need_play_incoming_number))
        {
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_CONTINUE + APP_WAVE_FILE_ID_VOICE_NUM_0 + wave_h->voice_num[wave_h->voice_index]);
            wave_h->voice_index++;
            if( wave_h->voice_num[wave_h->voice_index] == 0xFF )
            {
                wave_h->voice_index = 0xFF;
            }

            return;
        }
    }

	clear_wave_playing();

    WAV_PRT("app_wave_file_play_stop2\n");

	if (wave_h->file_id == APP_WAVE_FILE_ID_POWEROFF)
	{
        wav_aud_dac_close();
        wav_aud_volume_mute(1);
	}
#ifndef AUD_WAV_APP_AS_COEXIST
    else
        app_audio_restore();
#endif

    if(wave_h->func)
    {
        (wave_h->func)();
    }
    wave_h->func = NULL;
    wave_h->file_id = WAVE_EVENT;

	if(wave_h->file_id_pending_saved != APP_WAVE_FILE_ID_NULL)
    {
        app_wave_file_play_start( wave_h->file_id_pending_saved );
		wave_h->file_id_pending_saved = APP_WAVE_FILE_ID_NULL;
    }
    return;
}

//0--check ok,play mode switch wave, others--check fail, needn't play mode switch wave
uint8_t enter_mode_wave_and_action(uint32_t new_mode, wav_cb_func cb)
{
    uint32_t file_id = APP_WAVE_FILE_ID_NULL;
//   	app_handle_t sys_hdl = app_get_sys_handler();
    switch(new_mode){
    case SYS_WM_BT_MODE:
//    	if(hci_get_acl_link_count(sys_hdl->unit)==0){	//连接状态
#if defined MZ_200K                 //cjq++
       if(SysInf.Lang)
    	   	file_id = APP_WAVE_FILE_ID_VOICE_NUM_6;
       else
#endif
    		file_id = APP_WAVE_FILE_ID_BT_MODE;
//    	}
        break;
    case SYS_WM_SDCARD_MODE:
    	file_id = APP_WAVE_FILE_ID_RESERVED0;
        break;

#ifdef CONFIG_APP_UDISK
    case SYS_WM_UDISK_MODE: //not a requirement
        file_id = APP_WAVE_FILE_ID_MP3_MODE;
        break;
#endif
    case SYS_WM_LINEIN1_MODE:
    	file_id = APP_WAVE_FILE_ID_LINEIN_MODE;
    	break;
    case SYS_WM_LINEIN2_MODE:
        file_id = APP_WAVE_FILE_ID_FM_MODE;
        break;
    case SYS_WM_SPDIF_MODE:
        file_id = APP_WAVE_FILE_ID_MUTE_MIC;
        break;
    default:
        break;
    }
    LOG_I(MODE,"change to new mode:%d,wave id:%d\r\n",new_mode,file_id);
    return start_wave_and_action(file_id, cb);
}
// EOF
