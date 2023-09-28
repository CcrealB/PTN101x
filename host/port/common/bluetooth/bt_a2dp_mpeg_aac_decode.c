#include "bt_a2dp_mpeg_aac_decode.h"
#include "app_beken_includes.h"
#include "audio_sync.h"

#ifdef A2DP_MPEG_AAC_DECODE
#include "aac_decoder_api.h"

extern uint32   _ahbmem_begin;
//extern uint32   _sbcmem_begin;
//extern uint32   _sbcmem_end;

static void*    aac_decoder  = NULL;
static uint32_t aac_mute_cnt = 0;
static int32_t* aac_pcm_buffer;

RingBufferNodeContext aac_frame_nodes;

// uint32_t a2dp_get_freq(void);
uint32_t a2dp_get_codec_type(void);
//void   g_a2dp_stream_start_cb(void);

void ring_buffer_node_init(RingBufferNodeContext* rbn, uint8_t* address, uint32_t node_len, uint32_t nodes)
{
    rbn->address  = address;
    rbn->node_len = node_len;
    rbn->node_num = nodes;
    rbn->rp       = 0;
    rbn->wp       = 0;
}

void ring_buffer_node_clear(RingBufferNodeContext* rbn)
{
    rbn->rp = 0;
    rbn->wp = 0;
}

uint32_t ring_buffer_node_get_fill_nodes(RingBufferNodeContext* rbn)
{
    return rbn->wp >= rbn->rp ? rbn->wp - rbn->rp : rbn->node_num - rbn->rp + rbn->wp;
}

uint32_t ring_buffer_node_get_free_nodes(RingBufferNodeContext* rbn)
{
    uint32_t free_nodes = rbn->wp >= rbn->rp ? rbn->node_num - rbn->wp + rbn->rp : rbn->rp - rbn->wp;

    return free_nodes > 0 ? free_nodes - 1 : 0;
}

uint8_t* ring_buffer_node_get_read_node(RingBufferNodeContext* rbn)
{
    uint8_t* node = rbn->address + rbn->node_len * rbn->rp;

    if(++rbn->rp >= rbn->node_num)
    {
        rbn->rp = 0;
    }

    return node;
}

uint8_t* ring_buffer_node_get_write_node(RingBufferNodeContext* rbn)
{
    uint8_t* node = rbn->address + rbn->node_len * rbn->wp;

    if(++rbn->wp >= rbn->node_num)
    {
        rbn->wp = 0;
    }

    return node;
}

uint8_t* ring_buffer_node_peek_read_node(RingBufferNodeContext* rbn)
{
    return rbn->address + rbn->node_len * rbn->rp;
}

void ring_buffer_node_take_read_node(RingBufferNodeContext* rbn)
{
    if(++rbn->rp >= rbn->node_num) rbn->rp = 0;
}

void bt_a2dp_aac_node_buffer_init(void)
{
    uint32_t aac_size = ((aac_decoder_get_ram_size_without_in_buffer() + 1024 - 1) >> 10) << 10;
    uint8_t* address  = (uint8_t*)((uint32_t)aac_decoder + aac_size);
    uint32_t nodes    = AAC_FRAME_BUFFER_MAX_NODE;

    aac_pcm_buffer = (int32_t*)(address + nodes * AAC_FRAME_BUFFER_MAX_SIZE);

    //os_printf("[AAC]: aac size %d, nodes buffer set to %d (%d Bytes)\n", aac_size, nodes, nodes * AAC_FRAME_BUFFER_MAX_SIZE);

    ring_buffer_node_init(&aac_frame_nodes, address, AAC_FRAME_BUFFER_MAX_SIZE, nodes);
}

void bt_a2dp_set_configure_cb_aac_param(const bt_a2dp_codec_t *codec, uint32_t *bps, uint32_t *freq, uint32_t *channel)
{
    bt_a2dp_mpeg_aac_t* aac = (bt_a2dp_mpeg_aac_t*)&codec->u.mpeg_aac;

    if(aac->octet1 & MPEG_AAC_FREQ_44K) *freq = 44100;
    if(aac->octet2 & MPEG_AAC_FREQ_48K) *freq = 48000;

    if(aac->octet2 & MPEG_AAC_CHANNELS_1) *channel = 1;
    if(aac->octet2 & MPEG_AAC_CHANNELS_2) *channel = 2;

    *bps = MPEG_AAC_BIT_RATE(aac);

    LOG_I(AAC,"[AAC] freq = %d, channels = %d, bps = %d\n", *freq, *channel, *bps);
}

result_t bt_a2dp_sink_get_default_aac_function(bt_a2dp_mpeg_aac_t *aac)
{
    if(aac)
    {
        aac->object_type = MPEG2_AAC_LC | MPEG4_AAC_LC;
        aac->octet1 = MPEG_AAC_FREQ_44K;
        aac->octet2 = MPEG_AAC_FREQ_48K| MPEG_AAC_CHANNELS_1 | MPEG_AAC_CHANNELS_2;
        aac->octet3 = MPEG_AAC_VBR | 0x01;
        aac->octet4 = 0xF4;
        aac->octet5 = 0x00;

        return UWE_OK;
    }
    else
    {
        return UWE_PARAM;
    }
}

result_t bt_a2dp_aac_stream_start(uint32_t sample_rate, uint32_t channels)
{
    aac_decoder = (void*)((uint32_t)&_ahbmem_begin + 20 * 1024);

    bt_a2dp_aac_decoder_init(aac_decoder, sample_rate, channels);
    bt_a2dp_aac_node_buffer_init();

    return UWE_OK;
}

result_t bt_a2dp_aac_stream_sync(uint32_t type)
{
    if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING|/*APP_FLAG_CALL_ESTABLISHED|*/APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION))
    {
        if(audio_sync_get_flag() == 0)
        {
            os_printf("AAC SYNC <%x>\r\n", type);
            audio_sync_init();
            aud_dac_buffer_clear();
            aud_dac_dig_volume_fade_in();
            bt_a2dp_aac_node_buffer_init();
        }
    }

    return UWE_OK;
}

result_t bt_a2dp_aac_stream_suspend(void)
{
    aac_decoder = NULL;

    return UWE_OK;
}

result_t bt_a2dp_aac_stream_input(stream_softc_t *st, struct mbuf *m)
{
    int32_t err = UWE_OK;

    uint8_t* inbuf = &m->m_data[9];
    uint32_t inlen = 0;
    uint8_t  len   = 255;

    if(aac_decoder && a2dp_get_codec_type())
    {
        #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
        #endif

        do { inlen += len = *inbuf++; } while(len == 255);

        #if 0
        {
            #if 1
            static uint32_t max_len = 0;
            static uint32_t avg_len  = 0;
            avg_len = avg_len / 2 + inlen / 2;
            if(max_len < inlen)
            {
                max_len = inlen;
                os_printf("[M]:%d\n", inlen);
            }
            #endif

            #if 0
            static uint32_t count = 0;
            if(count++ == 44)
            {
                count = 0;
                os_printf("[A]:%d\n", avg_len);
            }
            #endif
        }
        #endif

        if(inlen <= (AAC_FRAME_BUFFER_MAX_SIZE - 8))
        {
            if(ring_buffer_node_get_free_nodes(&aac_frame_nodes))
            {
                uint8_t* node = ring_buffer_node_get_write_node(&aac_frame_nodes);
                *((uint32_t*)node) = inlen;
                m_copydata(m, inbuf - &m->m_data[0], inlen, node + 4);
            }
            else
            {
                audio_dac_ana_mute(1), aac_mute_cnt = 50;
                os_printf("[AAC]: aac frame node buffer is full\n");
                err = UWE_NOMEM;
            }
        }
        else
        {
           audio_dac_ana_mute(1), aac_mute_cnt = 50;
           os_printf("[AAC]: aac frame node buffer is overflow\n");
           err = UWE_NOMEM;
        }
    }

    m_freem(m);

    return err;
}

void bt_a2dp_aac_stream_decode(void)
{
    if(aac_decoder && a2dp_get_codec_type())
    {
        if(app_bt_flag1_get(APP_FLAG_WAVE_PLAYING|APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION)||hfp_has_sco())
        {
            if(audio_sync_get_flag()) audio_sync_set_flag(0);
            ring_buffer_node_clear(&aac_frame_nodes);
            return;
        }

        if(audio_sync_get_flag())
        {
            audio_sync_prepare_for_dac_open(0);
            audio_sync_process_for_dac_open();
            return;
        }

        audio_aso_open(ASO_TYPE_BT);//aud_dac_open();
        int samples = AAC_SAMPLES_PER_FRAME;
        #if BT_A2DP_SRC248_EN
        samples = (AAC_SAMPLES_PER_FRAME * 480 / 441) + 16;//for src 44.1->48 need more buffer size and int div rsvd 1 samples
        #endif
        #if (BT_AUD_SYNC_ADJ_BY_SW == 1)
        samples += 8; // 1024 / 128 = 8, audio sync adj 8 samples per 1024
        #endif
        if(aud_dac_get_free_buffer_size() >= (samples * 8))//*8 for 32bit2ch
        {
            if(ring_buffer_node_get_fill_nodes(&aac_frame_nodes))
            {
                uint8_t* inbuf = ring_buffer_node_peek_read_node(&aac_frame_nodes);
                uint32_t inlen = *(uint32_t*)inbuf;
                uint8_t* outbuf;
                uint32_t outlen;

                #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
                BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
                #endif

                #if CONFIG_AUDIO_SYNC_ENABLE
                int32_t r = audio_sync_process(0);
                if(r)            if(  aac_mute_cnt == 0) audio_dac_ana_mute(1), aac_mute_cnt = 50;
                if(aac_mute_cnt) if(--aac_mute_cnt == 0) audio_dac_ana_mute(0);
                #endif

				CLEAR_PWDOWN_TICK;
                BT_DECODE_RUN(1);
                int dec_err = bt_a2dp_aac_decoder_decode(aac_decoder, inbuf + 4, inlen, &outbuf, &outlen);
                BT_DECODE_RUN(0);
                if(dec_err == 0)
                {
                #if 0
                    int32_t  i;
                    int16_t* src = (int16_t*)outbuf;
                    int32_t* dst = (int32_t*)aac_pcm_buffer;
                    for(i = 0; i < outlen / 2; i++) *dst++ = *src++ << 8;

                    #if CONFIG_AUDIO_SYNC_ENABLE
                    if(r > 0) memset(dst, 0, 128 * 8), outlen += 128 * 4;
                    if(r < 0) outlen -= 128 * 4;
                    #endif

					#if (CONFIG_PRE_EQ == 1)
                    pre_eq_proc((int32_t*)aac_pcm_buffer,outlen>>1);
                    //INFO_PRT("%d\n",outlen);
                	#endif
                    aud_dac_fill_buffer((uint8*)aac_pcm_buffer, outlen*2);
                #else
                    #define SUB_FRA_SMPS    128
                    int i, k;
                    int32_t frame_buff[SUB_FRA_SMPS * 2];
                    int16_t* src = (int16_t*)outbuf;
                    int samples = outlen >> 2;//16bit2ch
                    int sub_smps = SUB_FRA_SMPS;
                    int remain = samples;
                    for(k = 0; k < samples && remain > 0; k+=SUB_FRA_SMPS)
                    {
                        remain = samples - SUB_FRA_SMPS;
                        sub_smps = remain > SUB_FRA_SMPS ? SUB_FRA_SMPS : remain;
                        for(i = 0; i < sub_smps * 2; i++) frame_buff[i] = *src++ << 8;
                        #if (CONFIG_PRE_EQ == 1)
                        pre_eq_proc((int32_t*)frame_buff, sub_smps * 2);
                        #endif
                        aud_dac_fill_buffer((uint8_t*)frame_buff, sub_smps*8);
                    }
                #endif
                }
                else
                {
                    os_printf("AAC frame decode error\n");
                }

                ring_buffer_node_take_read_node(&aac_frame_nodes);

                if(r > 1000 || r < -1000) bt_a2dp_aac_stream_sync(0xD);
            }
            else
            {
                if(audio_sync_get_flag() == 0)
                {
                    aac_mute_cnt = 100;
                    audio_dac_ana_mute(1);
                    bt_a2dp_aac_stream_sync(0xE);
                }
            }
        }
    }
}
result_t bt_stream_start_suspend_callback(BOOL start)
{
    if(a2dp_get_codec_type() != 0/*SBC*/)
    {
        if(start)
        {
            audio_aso_open(ASO_TYPE_BT);//aud_dac_open();
        }
        else
        {
            bt_a2dp_aac_stream_suspend();
        }
    }

    return UWE_OK;
}

#endif
