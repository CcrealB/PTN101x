#include <string.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "app_vendor_decode.h" // for a2dp vendor decode
#include "beken_external.h"
#ifdef A2DP_MPEG_AAC_DECODE
#include "bt_a2dp_mpeg_aac_decode.h"
#endif
#ifdef AUD_WAV_TONE_SEPARATE
#include "sbc_decoder_soft.h"
#endif
static SbcEncoderContext sbc_encoder_context;
static SbcDecoderContext sbc_decoder_context;

static uint8_t s_aud_mode = 0;

SbcEncoderContext* sbc_encoder = &sbc_encoder_context;
SbcDecoderContext* sbc_decoder = &sbc_decoder_context;

#ifdef CONFIG_SBC_PROMPT
    #ifdef AUD_WAV_TONE_SEPARATE
    static SbcDecoderContext wav_sbc_decoder_context;
    SbcDecoderContext* wav_sbc_decoder = &wav_sbc_decoder_context;
    #else
    SbcDecoderContext* wav_sbc_decoder = sbc_decoder;
    #endif
#endif //CONFIG_SBC_PROMPT

volatile uint8_t flag_sbc_buffer_play = 0;
static sbc_mem_node_t  *sbc_mem_node=NULL;
//static app_sbc_t  app_sbc;
app_sbc_t  app_sbc;

int encode_buffer_full = 0;

inline void set_aud_mode_flag(uint32_t flag)
{
    s_aud_mode |= flag;
}

inline void unset_aud_mode_flag(uint32_t flag)
{
    s_aud_mode &= ~flag;
}

inline uint32_t get_aud_mode_flag(uint32_t flag)
{
    return s_aud_mode & flag;
}

inline uint8 get_flag_sbc_buffer_play(void)
{
    return flag_sbc_buffer_play;
}

inline void set_flag_sbc_buffer_play(uint8 flag)
{
    flag_sbc_buffer_play = flag;
}


#ifdef BEKEN_DEBUG
int encode_pkts = 0;
int decode_pkts = 0;
int encode_buffer_empty = 0;
#endif

#if A2DP_ROLE_SOURCE_CODE
int stereo_mode_slave_latency = 0;
#endif

#if (SBC_ERR_FRM_PROC == 1)
static uint8_t MidFilter_Flag[AUDIO_DAC_BUFF_LEN >> 9];
#define SBC_MUTE_THR	6
#endif

#if A2DP_ROLE_SOURCE_CODE
extern driver_ringbuff_t *getLineRingbuf(void);
uint8_t *getLineinBufferBase(void);
uint32_t getLineinBufferLength(void);
#endif

void sbc_init_adjust_param(void)
{
    app_sbc.sbc_ecout = 0;
    app_sbc.sbc_clk_mode = 0;
}

static RAM_CODE void sbc_mem_pool_freeNode( sbc_mem_list_t  *list,
                                                      sbc_mem_node_t *node )
{
    if( list->head == NULL )
    {
        list->head = list->tail = node;
        node->next = NULL;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
        node->next = NULL;
    }
}

static RAM_CODE sbc_mem_node_t * sbc_mem_pool_getNode( sbc_mem_list_t *list)
{
    sbc_mem_node_t *node;

    if( list->head == NULL )
        return NULL;

    node = list->head;
    list->head = list->head->next;
    if( list->head == NULL )
        list->tail = NULL;

    return node;
}

void sbc_mem_pool_init( int framelen )
{
    uint8_t* mem_base = app_sbc.sbc_encode_buffer;
    int   node_len = 120; //( (framelen >> 2) + 1 ) << 2; // alignment
    int   node_num = SBC_ENCODE_BUFFER_LEN / node_len;
    int  i;

    if( node_num > SBC_MEM_POOL_MAX_NODE )
        node_num = SBC_MEM_POOL_MAX_NODE;

    app_sbc.sbc_mem_pool.node_num = node_num;

    if (NULL != sbc_mem_node)
    {
        jfree(sbc_mem_node);
        sbc_mem_node = NULL;
    }
    sbc_mem_node = (sbc_mem_node_t *)jmalloc(sizeof(sbc_mem_node_t)*SBC_MEM_POOL_MAX_NODE, M_ZERO);

    //os_printf("[sbc buffer pool init: node num: %d, node_len : %d\n", node_num, node_len );
    for( i = 0; i < node_num; i++)
    {
        sbc_mem_node[i].data_ptr = mem_base + i*node_len;
#ifdef A2DP_VENDOR_DECODE	// for a2dp vendor decode
        sbc_mem_node[i].data_len = 0;
#endif
        sbc_mem_pool_freeNode( &app_sbc.sbc_mem_pool.freelist, &sbc_mem_node[i] );
    }
    app_sbc.sbc_ecout = 0;
}

void sbc_mem_pool_deinit( void )
{
    sbc_mem_node_t *node;
    
    do
    {
        node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
#ifdef BEKEN_DEBUG
        decode_pkts++;
#endif
    }while( node != NULL );
    app_sbc.sbc_ecout = 0;

    do
    {
        node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.freelist );
    }while( node != NULL );

    app_sbc.sbc_mem_pool.node_num = 0;
    app_sbc.sbc_target_initial = 0;

#if A2DP_ROLE_SOURCE_CODE
    app_sbc.decode_cnt = 0;
    app_sbc.src_pkt_num = 0;
#endif

    if (sbc_mem_node)
    {
        jfree(sbc_mem_node);
        sbc_mem_node = NULL;
    }
}

uint32_t sbc_buf_get_use_count(void)
{
    return app_sbc.use_count;
}

void sbc_buf_increase_use_count(void)
{
    app_sbc.use_count += 1;
}

void sbc_buf_decrease_use_count(void)
{
    if(app_sbc.use_count > 0)
        app_sbc.use_count --;
}

uint16_t sbc_buf_get_node_count(void)
{
    return app_sbc.sbc_ecout;
}

void sbc_target_init_malloc_buff(void)
{
#ifdef USER_KARAOK_MODE
    extern uint32_t _aud_enc_begin;
    if(NULL == app_sbc.sbc_encode_buffer)
        app_sbc.sbc_encode_buffer = (uint8_t *)((uint32_t)&_aud_enc_begin); 
#else
#ifdef CONFIG_APP_AEC 
    extern uint32_t _sbcmem_begin;
    if(NULL == app_sbc.sbc_encode_buffer)
        app_sbc.sbc_encode_buffer = (uint8_t *)((uint32_t)&_sbcmem_begin); 
        //app_sbc.sbc_encode_buffer = (uint8_t *)jmalloc(SBC_ENCODE_BUFFER_LEN * sizeof(uint8_t), M_ZERO);
#else
    #if A2DP_ROLE_SOURCE_CODE
    #else
        app_sbc.sbc_encode_buffer =(uint8_t *) &sbc_encode_buff[0];
    #endif
#endif

#if A2DP_ROLE_SOURCE_CODE
	if(app_sbc.sbc_encode_buffer)
		jfree(app_sbc.sbc_encode_buffer);

	if(app_bt_flag1_get(APP_FLAG_LINEIN))
	{
		app_sbc.sbc_encode_buffer = (uint8_t *)jmalloc(getLineinBufferLength() * sizeof(uint8_t), M_ZERO);
	}
	else
	{
		app_sbc.sbc_encode_buffer = (uint8_t *)jmalloc(SBC_ENCODE_BUFFER_LEN * sizeof(uint8_t), M_ZERO);
	}
#endif

#endif
    if(NULL == app_sbc.sbc_output)
    {
        app_sbc.sbc_output = (uint8_t *)jmalloc(1024 * sizeof(uint8_t), M_ZERO);
    }
}

void sbc_target_deinit_jfree_buff(void)
{

	if(app_sbc.sbc_output)
	{
		jfree(app_sbc.sbc_output);
		app_sbc.sbc_output = NULL;
	}
    
#if (!defined(CONFIG_APP_AEC))
    app_sbc.sbc_encode_buffer = NULL;
#else
	if(app_sbc.sbc_encode_buffer)
	{
		app_sbc.sbc_encode_buffer = NULL;
	}
#endif
}

void *sbc_get_sbc_ptr(void)
{
    return (void*)0;//FIXME@liaixing
}

void sbc_target_init(void)
{
    app_sbc.sbc_target_initial = 0;

#if A2DP_ROLE_SOURCE_CODE
	app_sbc.src_pkt_num = 0;
#endif

    return;
}

void sbc_target_deinit( void )
{
    sbc_mem_pool_deinit();
    sbc_target_deinit_jfree_buff();//modify by zjw for malloc space
}
void sbc_mem_free(void)
{
	sbc_mem_pool_deinit();
	sbc_target_deinit_jfree_buff();
}
void sbc_stream_start_init( uint32_t freq )
{
    app_sbc.sbc_first_try = 0;
    app_sbc.freq = freq;
    app_sbc.timer_cnt = 0;
}

/* sbc input node count monitor
input: 3/4-1/2 MAX_NODE

output:
000    0: 44.1 clk is slow;
001    1: 44.1 clk is norm;
010    2: 44.1clk is fast;
100    4: 48 clk is slow;
101    5: 48 clk is norm;
110    6: 48 clk is fast;

*/
uint8_t sbc_node_buff_monitor(void)
{
    #ifdef APLL_KEEP_98p304MHZ
    uint8_t freq_b = 4;// *(app_sbc.freq == 48000);
    #else
    uint8_t freq_b = 4 *(app_sbc.freq == 48000);
    #endif
    uint16_t sbc_nodes;
    uint16_t sbc_node_thr_L, sbc_node_thr_H;
    if(get_aud_mode_flag(AUD_FLAG_GAME_MODE)){
        sbc_node_thr_H = SBC_GAME_MODE_FIRST_TRY;
        sbc_node_thr_L = SBC_GAME_MODE_LEVEL_LOW;
    }else{
        sbc_node_thr_H = SBC_FIRST_TRY_TIME;
        sbc_node_thr_L = (SBC_MEM_POOL_MAX_NODE >> 1);
    }

    //dac buff max node = 16384 / 8 / 128 = 16.//audio_sync_get_residue_nodes();
    sbc_nodes = app_sbc.sbc_ecout + (aud_dac_get_fill_buffer_size() >> 3) / 128;//>>3 for 32bit2ch, 128 samples per sbc node
    if(sbc_nodes > sbc_node_thr_H)
    {
        return freq_b;          	  // Current local CLK is slow;
    }
    else if(sbc_nodes < sbc_node_thr_L)
    {
        return freq_b + 2;           // Current local CLK is fast;
    } 
    else
    {
        return freq_b + 1;           // Current local CLK is normal;
    }         
}

void sbc_dac_clk_tracking(uint32_t step)
{
    if(get_flag_sbc_buffer_play()) //a2dp stream started...
    {
        app_sbc.timer_cnt += step;
        if(app_sbc.timer_cnt > 500)   // unit:10ms
        {
            app_sbc.timer_cnt = 0;
            aud_dac_clk_process();
        }
    }
}

/*
 * status = sbc_encode_buffer_status(device_index),where:
 * <value> = 0: normal;
 * <value> = 1: host layer sbc node buffer is nearly full;
 * <value> = 2: host layer sbc node buffer is nearly empty;
 */
uint8_t RAM_CODE sbc_encode_buffer_status(void)
{
    uint8_t status = 0;
    if(app_bt_flag1_get(APP_FLAG_HFP_CALLSETUP) || hfp_has_sco())
        return 0;

    if (!app_bt_flag1_get(APP_FLAG_MUSIC_PLAY))
    	return 0;
#ifdef A2DP_MPEG_AAC_DECODE
        extern uint32_t a2dp_get_codec_type();
        uint32 aac_nodes = 0;
        if(A2DP_CODEC_MPEG_AAC == a2dp_get_codec_type()) // AAC
        {
            aac_nodes = ring_buffer_node_get_fill_nodes(&aac_frame_nodes);
            if(AAC_FRAME_BUFFER_MAX_NODE - aac_nodes < 4)
            {
                status = 1;  /* AAC node buffer nearly full !!! */
                //os_printf("AAC F:%d\r\n",aac_nodes);
            }
            else if(aac_nodes < 2)
            {
                status = 2; /* AAC node buffer nearly empty !!! */
                //os_printf("AAC N:%d\r\n",aac_nodes);
            }
        }
        else //SBC
#endif
        {
        if((SBC_MEM_POOL_MAX_NODE - app_sbc.sbc_ecout) < 10)
            status = 1;  /* SBC node buffer nearly full !!! */
        else if(app_sbc.sbc_ecout < 10)
            status = 2; /* SBC node buffer nearly empty !!! */
        }
    return status;
}

static inline uint32_t audio_get_max_nodes(void)
{
    if(a2dp_get_codec_type() == 0/*SBC*/)
    {
        return SBC_MEM_POOL_MAX_NODE;
    }
    else
    {
        #ifdef A2DP_MPEG_AAC_DECODE
        return AAC_FRAME_BUFFER_MAX_NODE;
        #else
        return 0;
        #endif
    }
}

static inline uint32_t audio_get_residue_nodes(void)
{
    if(a2dp_get_codec_type() == 0/*SBC*/)
    {
        extern app_sbc_t app_sbc;
        return app_sbc.sbc_ecout;
    }
    else
    {
        #ifdef A2DP_MPEG_AAC_DECODE
        return ring_buffer_node_get_fill_nodes(&aac_frame_nodes);
        #else
        return 0;
        #endif
    }
}

uint8_t RAM_CODE sbc_encode_and_audio_low(void)
{
    if(app_check_bt_mode(BT_MODE_1V2))
    {
        if (!app_bt_flag1_get(APP_FLAG_MUSIC_PLAY))
    	    return 0;
    
        if ((audio_get_residue_nodes() < audio_get_max_nodes()/5) || (aud_dac_get_free_buffer_size() > 512))
        {
            return 1;
        }
    }
    
    return 0;
}

void RAM_CODE sbc_fill_encode_buffer( struct mbuf *m, int len, int frames )
{
    int i;
    int sbc_frame_len = len/frames;
    sbc_mem_node_t *node;
    
    if( app_sbc.sbc_target_initial == 0 )
    {
        sbc_target_init_malloc_buff();
        sbc_mem_pool_init(  sbc_frame_len );
        encode_buffer_full = 0;
        app_sbc.sbc_target_initial = 1;
        app_sbc.sbc_ecout = 0;
        app_sbc.sbc_clk_mode = 0;
    }
    
#if (defined(CONFIG_SBC_PROMPT) && (!defined(AUD_WAV_TONE_SEPARATE)))
    if(!bt_audio_ch_busy())
#endif
    {
        for( i = 0; i < frames; i++)
        {
            node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.freelist );
            if( node == NULL )
            {
                //os_printf("f:%d\r\n", encode_buffer_full);
                encode_buffer_full++;
                break;
            }
#ifdef BEKEN_DEBUG
            encode_pkts++;
#endif
            m_copydata( m, i*sbc_frame_len, sbc_frame_len, (void *)node->data_ptr);
#ifdef A2DP_VENDOR_DECODE	// for a2dp vendor decode
            node->data_len = sbc_frame_len;
#endif
            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.uselist, node);
        }
        app_sbc.sbc_ecout += i;
    }
}

#if (SBC_ERR_FRM_PROC == 1)
uint8_t RAM_CODE mid_filter(uint8_t *output,uint8_t *filt_flag)
{
	uint16_t *smplr = (uint16_t *)output;
	int16_t k;
	uint32_t smpl_amp_sum = 0;
	uint16_t flag_sum = 0;

	for(k=0; k<256; k++)
	{
		if((*smplr)!=0)
		{
			smpl_amp_sum = 1;
			break;
		}
		smplr ++;
	}
	for(k=1; k<(AUDIO_DAC_BUFF_LEN>>9); k++)
	{
		flag_sum += filt_flag[k];
		filt_flag[k-1] = filt_flag[k];

	}
 	filt_flag[(AUDIO_DAC_BUFF_LEN>>9)-1] = (smpl_amp_sum > 0);
 	flag_sum += filt_flag[(AUDIO_DAC_BUFF_LEN>>9)-1];
 	if(flag_sum > SBC_MUTE_THR)
 		return 1;
 	else
 		return 0;
}
#endif

#ifdef A2DP_SBC_DUMP_SHOW
void sbc_encode_frame_info(void)
{

}
#endif

#if (CONFIG_DRC == 1)
static __inline int16_t f_sat(int32_t din)
{
    if (din>32767)
        return 32767;
    if (din<-32768)
        return -32768;
    else
        return(din);
}
#endif

extern BOOL is_voice_recog_status_on(void);
void RAM_CODE sbc_do_decode( void )
{
    int decode_frms, i, frame_len;
    sbc_mem_node_t *node;
    uint16_t sbc_node_thr = get_aud_mode_flag(AUD_FLAG_GAME_MODE)?SBC_GAME_MODE_FIRST_TRY:SBC_FIRST_TRY_TIME;
    if((sbc_decoder == NULL) || (app_sbc.sbc_target_initial == 0))
    {
        return;
    }

    if(hfp_has_sco() || is_voice_recog_status_on())
    {
        do
        {
            node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
            if(node != NULL)
            {
                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                app_sbc.sbc_ecout --;
            }
        }while(node != NULL);
        return;
    }

	//  sbc buffer cache, 	avoid "POP"
    if(!get_flag_sbc_buffer_play())
    {
#ifndef AUD_WAV_TONE_SEPARATE
        if(app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
        {
            do
            {
                node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
                if(node != NULL)
                {
                    sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                    app_sbc.sbc_ecout --;
                }
            }while(node != NULL);
            return;
        }
#endif

        if(app_sbc.sbc_ecout < sbc_node_thr) 
        {
            return ;
        }

        set_flag_sbc_buffer_play(1);
        LOG_I(SBC,"sbc cache:%d,%d\r\n",app_sbc.sbc_ecout,app_sbc.sbc_target_initial);

        audio_dac_ana_mute(0);
        aud_dac_dig_volume_fade_in();
		sbc_decoder_init(sbc_decoder);

#if (SBC_ERR_FRM_PROC == 1)
        memset(MidFilter_Flag,0,sizeof(MidFilter_Flag));
#endif
    }

    int samples = 128;//SAMPLES_PER_SBC_FRAME = 128
    #if BT_A2DP_SRC248_EN
    samples = (128 * 480 / 441) + 2;//for src 44.1->48 need more buffer size and int div rsvd 1 samples
    #endif
    #if (BT_AUD_SYNC_ADJ_BY_SW == 1)
    samples += 1; //audio sync adj 1 sample per 128
    #endif

    decode_frms = aud_dac_get_free_buffer_size() / (samples * 8);//*8 for 32bit2ch

#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    if(decode_frms > 0)
    {
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif
        for( i = 0; i < decode_frms; i++ )
        {
            node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
#if (SBC_ERR_FRM_PROC == 1)
            if(node == NULL)
            {
                memset(app_sbc.sbc_output,0,1024);
                encode_buffer_empty++;
            }
#else
            if( node == NULL )
            {
                #ifdef BEKEN_DEBUG
                encode_buffer_empty++;
                #endif
                break;
            }
#endif

            CLEAR_PWDOWN_TICK;

#if (SBC_ERR_FRM_PROC == 1)
            if(node != NULL)
            {
#ifdef A2DP_VENDOR_DECODE // for a2dp vendor decode
                frame_len = sbc_decoder_decode(sbc_decoder, node->data_ptr, node->data_len);
#else
                frame_len = sbc_decoder_decode(sbc_decoder, node->data_ptr, SBC_ENCODE_BUFFER_LEN);
#endif
                if(app_bt_flag2_get(APP_FLAG2_SW_MUTE))
                {
                    memset((uint8_t*)sbc_decoder->pcm_sample,0,1024);
                }
                
                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
#ifdef BEKEN_DEBUG
                decode_pkts++;
#endif
                app_sbc.sbc_ecout --;

                if( frame_len < 0 )
                {
                    os_printf("ERR: frame_len(=%d) < 0\r\n",frame_len);
                    continue;
                }
            }
#else
            {
#ifdef A2DP_VENDOR_DECODE // for a2dp vendor decode
                frame_len = sbc_decoder_decode(sbc_decoder, node->data_ptr, node->data_len);
#else 
                BT_DECODE_RUN(1);
                frame_len = sbc_decoder_decode(sbc_decoder, node->data_ptr, SBC_ENCODE_BUFFER_LEN);
                BT_DECODE_RUN(0);
#endif
                if(app_bt_flag2_get(APP_FLAG2_SW_MUTE)) 
                {
                    memset((uint8_t*)sbc_decoder->pcm_sample,0,1024);
                }

#ifdef BEKEN_DEBUG
                decode_pkts++;
#endif

                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                if( frame_len < 0 )
                {
                    continue;
                }
            }
#endif
#ifndef AUD_WAV_TONE_SEPARATE
            if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
#endif
    	    {
#if (SBC_ERR_FRM_PROC == 1)
                if(!mid_filter(app_sbc.sbc_output,MidFilter_Flag)) memset(app_sbc.sbc_output,0,1024);
#endif

                if(get_bt_dev_priv_work_flag())
                {
#if(CONFIG_AUD_FADE_IN_OUT == 1)
                    set_aud_fade_in_out_state(AUD_FADE_OUT);
#endif
                }
                else if(has_hfp_flag_1toN(APP_FLAG_HFP_OUTGOING|APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED|APP_FLAG2_HFP_INCOMING))
                {
                    memset((uint8_t*)sbc_decoder->pcm_sample,0,1024);
                }

                int16_t *sample = (int16_t *)sbc_decoder->pcm_sample;
                int a;
                for( a = 0; a < 256; a+= 16 ) aud_mute_update( sample[a], sample[a+1] );
#ifndef AUD_WAV_TONE_SEPARATE
                if(!bt_audio_ch_busy ())
#endif
                {
                    #if (CONFIG_PRE_EQ == 1)
                    #if (CONFIG_EAR_IN == 1)
                    if(get_earin_status()||!app_env_get_pin_enable(PIN_earInDet))
                    #endif
                    pre_eq_proc((int32_t*)sbc_decoder->pcm_sample,1024>>2);
                    #endif
                    aud_dac_fill_buffer((uint8_t*)sbc_decoder->pcm_sample, 1024);
                }
			}
		}

#if (SBC_ERR_FRM_PROC != 1)
        app_sbc.sbc_ecout -= i;
#endif

#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV); 
    }
    else
    {
        BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV); 
    }
#endif
}

void sbc_discard_uselist_node(void)
{
    sbc_mem_node_t *node;
    uint32_t interrupts_info, mask;

    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    do
    {
        node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.uselist);
        if(node)
        {
            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
        }
    }while(node);
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
}

void sbc_first_play_init(uint8_t enable,uint8_t cnt)
{
    os_printf("sbc_first_play_init\r\n");
    set_flag_sbc_buffer_play(0);
}

#ifdef CONFIG_OPTIMIZING_SBC
app_detect_aud_rssi_t opt_sbc;
const int8_t t_opt_sbc_volume[opt_sbc_volume_steps] = {16,20,25,32,40,51,64,81,102,128};
void optimize_sbc_init(void)
{
	memset(&opt_sbc,0,sizeof(app_detect_aud_rssi_t));
}

int32_t optimize_sbc_adj_amp(int32_t sample)
{
	return ((sample>>12)*t_opt_sbc_volume[opt_sbc.sbc_gain_index])>>7;
}

void optimize_sbc_calc_rssi(int16_t sample)
{
	if(((sample==0x7fff)||(sample==(-0x8000)))
		&&(opt_sbc.sbc_gain_index>0))
	{

		uint32_t a7_tmp;
		uint8_t ang_val,tmp;
		a7_tmp = BK3000_A7_CONFIG;
		ang_val = (a7_tmp>>17)&0xf;
		a7_tmp &= ~(0xf<<17);
		tmp = MIN(opt_sbc.sbc_gain_index,ang_val);
		opt_sbc.ang_adj -= tmp;
		opt_sbc.sbc_gain_index -= tmp;
		ang_val -= tmp;
		a7_tmp |= ang_val<<17;
		BK3000_A7_CONFIG = a7_tmp;
		opt_sbc.k = 0;
		opt_sbc.sum = 0;
		return;
	}
    
	if(opt_sbc.k>=aud_rssi_sum_num)
	{
		uint32_t a7_tmp;
		uint8_t ang_val;
		opt_sbc.k = 0;
		opt_sbc.avg = opt_sbc.sum/aud_rssi_sum_num;
		a7_tmp = BK3000_A7_CONFIG;
		ang_val = (a7_tmp>>17)&0xf;
		a7_tmp &= ~(0xf<<17);

		if(opt_sbc.avg>aud_rssi_scale_max)
		{
			if(opt_sbc.sbc_gain_index>0)
			{
				if(ang_val>0)
				{
					ang_val -= 1;
					a7_tmp |= ang_val<<17;
					BK3000_A7_CONFIG = a7_tmp;
					opt_sbc.ang_adj --;
					opt_sbc.sbc_gain_index--;
				}
			}
		}
		else if(opt_sbc.avg<aud_rssi_scale_min)
		{
			if(opt_sbc.sbc_gain_index<opt_sbc_volume_index_max)
			{
				if(ang_val<15)
				{
					ang_val += 1;
					a7_tmp |= ang_val<<17;
					BK3000_A7_CONFIG = a7_tmp;
					opt_sbc.ang_adj ++;
					opt_sbc.sbc_gain_index++;
				}
			}

		}

		opt_sbc.sum = 0;
	}
	opt_sbc.k++;
	opt_sbc.sum +=
	ABS((int16_t)(sample));
}

int8_t optimize_sbc_get_volume_adj(void)
{
		return opt_sbc.ang_adj;
}
#else
int32_t optimize_sbc_adj_amp(int32_t sample)
{
	return (sample>>15);
}

void optimize_sbc_calc_rssi(int16_t sample)
{
	return;
}

int8_t optimize_sbc_get_volume_adj(void)
{
	return 0;
}
#endif

#ifdef CONFIG_SBC_PROMPT
void app_wave_file_sbc_fill(void)
{
    uint8_t  in_ptr[64];
    uint8_t* out_ptr   = (uint8_t*)wav_sbc_decoder->pcm_sample;
    uint8_t  input_len = app_wave_get_blocksize();
    int i,decode_frms;
    int16_t ret;

#if 0//def AUD_WAV_TONE_SEPARATE
    if(app_wave_check_type(INTER_WAV) || app_wave_check_type(EXT_WAV)) return;
#else
    if(!app_wave_playing() ||app_wave_check_type(INTER_WAV) || app_wave_check_type(EXT_WAV)) return;
#endif

    #ifdef AUD_WAV_TONE_SEPARATE
    decode_frms = (wav_aud_dac_get_free_buffer_size()+1)/256;
    uint32_t freq = 16000;//16k wav default
    app_get_wave_info(&freq, NULL, NULL);
    if(freq == 8000) decode_frms >>= 1;//div 2 for 8k to 16k need 2 times of buff space
    #elif WAV_FILL_48K_EN
    decode_frms = (wav_aud_dac_get_free_buffer_size()+1)/6144;
    #else
    decode_frms = (wav_aud_dac_get_free_buffer_size()+1)/1024;
    #endif

    for(i = 0; i < decode_frms; i++)
    {
        if(app_wave_check_status(PROMPT_EMPTY))
        {
            app_wave_set_status(PROMPT_CLOSED);
            app_wave_file_play_stop();
            break;
        }
        else if(app_wave_check_status(PROMPT_FILL_ZERO))
        {
            app_wave_fill_sbc_node(in_ptr);
            memset(out_ptr, 0, 256);
            app_wave_file_fill_buffer(out_ptr, 256);
        }
        else
        {
            app_wave_fill_sbc_node(in_ptr);
            #ifdef AUD_WAV_TONE_SEPARATE
            ret = sbc_decoder_soft_decode(wav_sbc_decoder, in_ptr, input_len);
            #else
            ret = sbc_decoder_decode(wav_sbc_decoder, in_ptr, input_len);
            #endif
            if(ret < 0)
            {
                memset(out_ptr,0,256);    
            }
            app_wave_file_fill_buffer(out_ptr, 256);
        }
    }
}

#endif

#if A2DP_ROLE_SOURCE_CODE

#define LTX_PRT os_printf

extern int8_t debugShowTrace(uint8_t val);
extern void debugSetTraceVal(uint8_t val);
extern driver_ringbuff_t *getLineRingbuf(void);
extern int32_t rbGetSampleNumber(driver_ringbuff_t *rb);
extern result_t a2dpSrcCmdStreamSend( struct mbuf *m );
extern void RAM_CODE dacClkAdjust( int mode );
extern driver_ringbuff_t *getLineRingbuf(void);

static void sbcDacClkAdjust( void )
{
	if( app_sbc.sbc_ecout >= 55 )
	{
		dacClkAdjust(3);
		app_sbc.sbc_clk_mode = 3;
	}
	else if( app_sbc.sbc_ecout <= 15 )
	{
		dacClkAdjust(4);
		app_sbc.sbc_clk_mode = 4;
	}
	else if(app_sbc.sbc_ecout >= 40)
	{
		dacClkAdjust(1);
		app_sbc.sbc_clk_mode = 1;
	}
	else if(app_sbc.sbc_ecout <= 30)
	{
		dacClkAdjust(2);
		app_sbc.sbc_clk_mode = 2;
	}
	else if(app_sbc.sbc_clk_mode != 0 )
	{
		dacClkAdjust(0);
		app_sbc.sbc_clk_mode = 0;
	}
}

void appSbcDacClkAdjust(void)
{
	sbcDacClkAdjust();
}

uint8_t getSbcClkMode(void)
{
	return app_sbc.sbc_clk_mode;
}

void sbcShowAllStatus(void)
{
	os_printf("aud_num:%d,buf:%d,eCnt:%d,clk_m:%d,0x%x",sync_data.u.sync_send.aud_num,aud_dac_get_free_buffer_size(),app_sbc.sbc_ecout,app_sbc.sbc_clk_mode,BK3000_AUD_DAC_FRACCOEF);
}

void sbc_stream_start_clear(void)
{
	app_sbc.src_pkt_num = 0;
}

void sbc_src_num_oper( uint8_t oper, uint8_t num )
{
    if( oper == 0 ) // plus
    {
        app_sbc.src_pkt_num += num;
    }
    else if( oper == 1 )
    {
        app_sbc.src_pkt_num -= num;
        if( app_sbc.src_pkt_num <= 0 )
            app_sbc.src_pkt_num = 0;
    }
    return;
}

void sbc_src_num_set(uint8_t num )
{
	app_sbc.src_pkt_num = num;
}

int sbc_src_num( void )
{
    return (int8_t)app_sbc.src_pkt_num;
}

int sbcCheckSrcNum(void)
{
	if((int8_t)app_sbc.src_pkt_num<16)
		return 1;
	else
	{
	    os_printf("sn:%d\r\n",app_sbc.src_pkt_num);
		return 0;
	}
}

uint8_t *getLineinBufferBase(void)
{
	return app_sbc.sbc_encode_buffer;
}

uint32_t getLineinBufferLength(void)
{
	return 512*10;
}

void sbcSrcNumSet(uint8_t num )
{
	app_sbc.src_pkt_num = num;
}

void sbcSrcNumOper(uint8_t oper, uint8_t num)
{
    if(0 == oper) // plus
    {
        app_sbc.src_pkt_num += num;
    }
    else if(1 == oper)
    {
        app_sbc.src_pkt_num -= num;
        if(app_sbc.src_pkt_num <= 0)
            app_sbc.src_pkt_num = 0;
    }

    return;
}

int8_t getSrcPktNum(void)
{
    return app_sbc.src_pkt_num;
}

void lineinSbcEncodeInit(void)
{
    media_packet_sbc_t *pkt;
    sbc_t *sbc;
    sbc = (sbc_t *)jmalloc(sizeof(sbc_t), 0);
    os_printf("lineinSbcEncodeInit: 0x%x \r\n",sbc);
    if (!sbc)
        return ;

    sbc_init(sbc, 0,SBC_MODE);
    sbc_target_init(sbc);
	sbc_target_init_malloc_buff();
	rb_init(getLineRingbuf(), getLineinBufferBase(), getLineinBufferLength(), 0, 0);

    app_sbc.sequence    = 0;
    app_sbc.samples     = 0;
    sbcSrcNumSet(0);

    app_sbc.in_frame_len  = sbc_get_codesize(app_sbc.sbc_ptr,SBC_MODE);
    app_sbc.out_frame_len = sbc_get_frame_length(app_sbc.sbc_ptr,SBC_MODE);
    app_sbc.payload_len   = L2CAP_MTU_DEFAULT - sizeof(rtp_header_t);//default mtu
    app_sbc.frames_per_packet = 8; //app_sbc.payload_len/ app_sbc.out_frame_len;
    app_sbc.sbc_output_buffer = (uint8_t *)jmalloc(L2CAP_MTU_DEFAULT, 0);//		outputbuffer;

    os_printf("in_len:%d,out_len:%d\r\n",app_sbc.in_frame_len,app_sbc.out_frame_len);
    if(app_sbc.sbc_output_buffer)
    {   
        os_printf("encode_buf ok\r\n");

        pkt = (media_packet_sbc_t*)app_sbc.sbc_output_buffer;
		memset((uint8_t *)(&pkt->hdr), 0, sizeof(rtp_header_t));

		pkt->hdr.ssrc = uw_htobe32(1);
        pkt->hdr.pt   = 0x60;
        pkt->hdr.v    = 2;
    }
	else
	{
		os_printf("encode_buf err\r\n");
	}
}

void lineinSbcEncodeDestroy(void)
{
    os_printf("lineinSbcEncodeDestroy...\r\n");
    if(app_sbc.sbc_ptr) 
    {

        if(app_sbc.sbc_ptr->priv) 
        {
            jfree(app_sbc.sbc_ptr->priv);
            app_sbc.sbc_ptr->priv = NULL;
        }

        jfree(app_sbc.sbc_ptr);
        app_sbc.sbc_ptr = NULL;
    }

    if(app_sbc.sbc_encode_buffer) 
    {
        jfree(app_sbc.sbc_encode_buffer);
        app_sbc.sbc_encode_buffer = NULL;
    }

    if(app_sbc.sbc_output) 
    {
        jfree(app_sbc.sbc_output);
        app_sbc.sbc_output = NULL;
    }

    if(app_sbc.sbc_output_buffer) 
    {
        jfree(app_sbc.sbc_output_buffer);
        app_sbc.sbc_output_buffer = NULL;
    }

    app_sbc.sbc_target_initial = 0;
    app_sbc.sequence    = 0;
    app_sbc.samples     = 0;
	app_sbc.src_pkt_num = 0;
}

/*
Header + n* SBC frame
input:
		buffer: input buffer
		len:PCM DATA length
		mp: output buffer
		mp_data_len:packet length
		written:encoded data length
output:
		written: a2dp packet length
		return value: consumed PCM data length

added by beken
*/
static int	sbcEncodeMediapacket(uint32_t len, void *mp,
					                      uint32_t mp_data_len, uint32_t *written)
{
    media_packet_sbc_t *mp_sbc = (media_packet_sbc_t *) mp;
    uint32_t consumed   = 0;
    uint32_t encoded    = 0;
    uint8_t frame_count = 0;
    int len_t;
    uint8_t *tmpbuffer = &(mp_sbc->frame_count);
    mp_data_len       -= 1; //sizeof(mp_sbc->payload);
    tmpbuffer         += 1; //skip

    while (len - consumed >= app_sbc.in_frame_len 
           && mp_data_len - encoded >= app_sbc.out_frame_len 
           && frame_count < app_sbc.frames_per_packet)
    {
        uint32_t read;
        uint32_t written_s;

        written_s = 0;
        len_t     = rb_read_buffer(getLineRingbuf(), app_sbc.sbc_output, app_sbc.in_frame_len);

        if(len_t == 0)
            break;

#ifndef CONFIG_LINE_IN_TX_LOOP
    	if(debugShowTrace(2))
    	{
    		uint16_t *ptr;
    		int m;
    		debugSetTraceVal(0);
    		ptr = (uint16_t *)app_sbc.sbc_output;
    		for(m=0; m<512/4; m++)
              LTX_PRT("%x\r\n",ptr[2*m+1]);
    	}

        aud_dac_fill_buffer(app_sbc.sbc_output,app_sbc.in_frame_len);

        if(app_bt_flag2_get(APP_FLAG2_STEREO_PLAYING)
    	   && aud_dac_get_free_buffer_size() < 1024)
    	{
    	    aud_dac_open();
    	}
#endif

        /* in while(): mp_data_len - encoded >= app_sbc.out_frame_len */
        written_s = sbc_encoder_encode(sbc_encoder, (int16_t*)app_sbc.sbc_output);
        if(written_s < 0) break;
        memcpy(tmpbuffer+encoded, sbc_encoder->stream, written_s);

        frame_count++;          //encoded SBC frame count
        consumed += read;       //consumed PCM data len
        encoded  += written_s;  //encoded SBC data len
    }

    *written            = encoded + 1;     //sizeof(mp_sbc->payload);
    mp_sbc->frame_count = frame_count;

    return consumed;
}

result_t RAM_CODE sbcDoEncode( void )
{
    result_t err      = UWE_BUSY;
    uint8_t *outbuf     = app_sbc.sbc_output_buffer;
    uint32_t free_space = app_sbc.payload_len;
    uint32_t read, samples;

    int32_t input_len = rbGetSampleNumber(getLineRingbuf());

    if(input_len < 5*app_sbc.in_frame_len)
        return err;

    media_packet_sbc_t *pkt  = (media_packet_sbc_t*)outbuf;
    pkt->hdr.sequence_number = uw_htobe16(app_sbc.sequence);
    pkt->hdr.timestamp       = uw_htobe32(app_sbc.samples);
    app_sbc.sequence++;

    uint32_t written = 0;
	struct mbuf *m;

    read = sbcEncodeMediapacket(input_len, (void *)outbuf, free_space, &written);
    //os_printf("<<>b, read=%d, written=%d\r\n",read,written);
    if(read <=0)
        return err;

    if(written > 0)
    {
        written += (12);//sizeof(rtp_header_t);
		m        = m_get_flags(MT_DATA, M_PKTHDR, written);

		if(!m)
			return UWE_NOMEM;

		m_copyback(m, 0, written, outbuf);
		err = a2dpSrcCmdStreamSend(m);

        if(app_sbc.sbc_ptr->mode == 0x03/*SBC_MODE_JOINT_STEREO*/)
            samples = read >> 2;// /4
        else
            samples = read >> 1;// /2

        app_sbc.samples += samples;
    }

    return err;
}

#endif

void sco_callback_sbc_init(void)
{
    sbc_decoder_init(sbc_decoder);
    sbc_decoder_ctrl(sbc_decoder, SBC_DECODER_CTRL_CMD_SET_OUTPUT_STEREO_FLAG, 0);
    sbc_decoder_ctrl(sbc_decoder, SBC_DECODER_CTRL_CMD_SET_OUTPUT_PCM_WIDTH, 16);
    sbc_encoder_init(sbc_encoder, 16000, 1);
    sbc_encoder_ctrl(sbc_encoder, SBC_ENCODER_CTRL_CMD_SET_MSBC_ENCODE_MODE, (uint32_t)NULL);
}

void sco_callback_sbc_deinit(void)
{
}

// EOF
