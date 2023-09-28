#include <string.h>
#include "aec.h"
#include "app_beken_includes.h"
#ifdef CONFIG_APP_AEC
#include "driver_beken_includes.h"
#include "app_bt.h"
#include "app_aec.h"


#define ECHO_RX_BUFFER_SIZE     (320 * 2 * 2+8)
#define ECHO_TX_BUFFER_SIZE     (320 * 2 * 2+8)


static uint8_t  aec_init_flag    = 0;

static driver_ringbuff_t aec_rin_rb;
static driver_ringbuff_t aec_out_rb;

static uint8_t* aec_rin_buf   = NULL;
static uint8_t* aec_out_buf   = NULL;

static AECContext* aec = NULL;

static uint8_t *get_aec_ram_buff(void)
{
    extern uint32 _sbcmem_begin;
    return (uint8_t *)((uint32)&_sbcmem_begin+4);
}

static void app_aec_para_set(AECContext * aec, app_hfp_cfg_t* hfp_tx_cfg)
{
#if 0
    aec->flags        = hfp_tx_cfg->module_flag;

#if AEC_EC_ENABLE
    aec->ec_depth     = hfp_tx_cfg->ec_depth;
    aec->delay_offset = hfp_tx_cfg->delay_offset;
    aec->TxRxThr      = (hfp_tx_cfg->ec_sm_thr)<<(16-4);
    memcpy(aec->TxRxSm, hfp_tx_cfg->ec_sm, sizeof(int8_t)*4);
#endif

#if AEC_NS_ENABLE
   aec->ns_depth      = hfp_tx_cfg->ns_depth;
   aec->minG          = (hfp_tx_cfg->ns_minG)<<(16-8);
   {
     int32_t idx      = hfp_tx_cfg->ns_para;
     extern int32_t NS_PARA[2][3][4];
     memcpy((int32_t*)(&aec->VADInfo),(int32_t*)NS_PARA[idx],sizeof(int32_t)*4);
     memcpy(aec->nspara,(int32_t*)NS_PARA[idx]+4,sizeof(int32_t)*8);
   }
#endif

#if AEC_HPF_ENABLE
    aec->cutoff       = hfp_tx_cfg->hpf_cutoff;
    aec->cutbin       = aec->cutoff/31;
    if ((aec->fs==16000)&&(aec->FFT_ptr.N==128))
    aec->cutbin>>=1;
#endif

#if AEC_DRC_ENABLE
    aec->drc_mode     = hfp_tx_cfg->drc_mode;
#endif

#if AEC_CNI_ENABLE
    aec->cni_floor    = (hfp_tx_cfg->cni_floor)<<13;
    aec->cni_fade     = hfp_tx_cfg->cni_fade;
#endif

#if AEC_EQ_ENABLE
    if (aec->fs==8000)
        aec->EQF      = hfp_tx_cfg->eqf_tab_nb;
    else
        aec->EQF      = hfp_tx_cfg->eqf_tab_wb;
#endif
#endif
}

void app_aec_init(int32_t sample_rate)
{
    if(aec_init_flag == 0)
    {
        uint32_t aec_mem_size  = 0;
        app_env_handle_t env_h = app_env_get_handle();

        INFO_PRT("AEC.init:%d\r\n", sample_rate);

        INFO_PRT("AEC size %d @sbcmem  Date is %d \r\n",aec_size(AEC_MAX_MIC_DELAY),aec_ver());

        aec_rin_buf   = get_aec_ram_buff();
        aec_mem_size += ECHO_RX_BUFFER_SIZE;
        aec_out_buf   = get_aec_ram_buff() + aec_mem_size;
        aec_mem_size += ECHO_TX_BUFFER_SIZE;
        rb_init(&aec_rin_rb, (uint8 *)aec_rin_buf, ECHO_RX_BUFFER_SIZE, 0, NULL);
        rb_init(&aec_out_rb, (uint8 *)aec_out_buf, ECHO_TX_BUFFER_SIZE, 0, NULL);

        aec = (AECContext*)(get_aec_ram_buff() + aec_mem_size);
        aec_mem_size += aec_size(AEC_MAX_MIC_DELAY);

        aec_init(aec,(int16_t)sample_rate);
        app_aec_para_set(aec,&(env_h->env_cfg.hfp_cfg));
        aec->ec_depth=10;
        aec->drc_mode=0x18;
        aec->ns_depth=3;
        aec->delay_offset=0x80;
        os_printf("aec mem start %x end %x\n",aec,&aec->div_tab[16]);
        aec_init_flag = 1;
    }
}


void app_aec_uninit(void)
{
    if(aec_init_flag == 1)
    {        
        if(NULL != aec_rin_buf)
        {
            aec_rin_buf = NULL;
        }

        if(NULL != aec_out_buf)
        {
            aec_out_buf = NULL;
        }
        #if MEM_ISOLATE
        aec_close(aec);
        #endif
        if(NULL != aec)
        {  
          aec = NULL;
        }
        aec_init_flag = 0;
    }
}

void app_aec_fill_rin_buf(uint8_t *buff, uint8_t fid, uint32_t len)
{
    if(app_wave_playing()) return;

    if(aec_init_flag) rb_write_buffer(&aec_rin_rb, buff, len);
}

int32_t app_aec_read_out_buf(uint8_t* buf, uint32_t len)
{
    if(aec_init_flag)
    {
        uint32_t fill   = rb_get_buffer_fill_size(&aec_out_rb);
        int8_t   insert = aec->insert;
        insert = (fill<=((120+40)*sizeof(int16_t))) ? insert : 0 ;
        return (rb_read_buffer(&aec_out_rb, buf, len-insert*sizeof(int16_t)));
    }
    else
    {
        return 0;
    }
}

void RAM_CODE app_aec_swi(void)
{
    if(aec_init_flag)
    {
        #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
        #endif

        int rx_size = rb_get_buffer_fill_size(&aec_rin_rb) / 2;
        int tx_size = aud_mic_get_fill_buffer_size() / 2;
        int AEC_FRAME_SAMPLES=aec->frame_samples;

        if((rx_size >= AEC_FRAME_SAMPLES) && (tx_size >= AEC_FRAME_SAMPLES))
        {
            int16_t* rin;
            int16_t* sin;
            int16_t* out;
            uint32_t flags;

            flags = aec->flags;
            rin   = aec->rin;
            sin   = aec->sin;
            out   = aec->out;

            rb_read_buffer(&aec_rin_rb,(uint8*)rin, AEC_FRAME_SAMPLES * 2);
            aud_mic_read_buffer((uint8*)sin, AEC_FRAME_SAMPLES * 2);

            if(flags & 0x80)
            {
                int i;
                char* prin = (char*)rin;
                char* psin = (char*)sin;
                //char* pout = (char*)out;
                for(i = 0; i < AEC_FRAME_SAMPLES; i++)
                {
                    {
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*prin++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*prin++);
                    }
                    {
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*psin++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*psin++);
                    }
                    /*{
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*pout++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*pout++);
                    }*/
                }
            }

            {
                int32_t mic_delay = (int32_t)aud_dac_get_fill_buffer_size() / 8 - (int32_t)rb_get_buffer_fill_size(&aec_rin_rb) / 2 + (int32_t)aud_mic_get_fill_buffer_size() / 2;

                mic_delay+=(aec->delay_offset)<<1;
                aec->vol=get_current_hfp_volume();
                //if (flags&0x40) os_printf("delay is %d vol %d\n",mic_delay,aec->vol);      
				if(mic_delay < 0)
                {
                    mic_delay = 0;
                }
                else if(mic_delay > AEC_MAX_MIC_DELAY)
                {
                    mic_delay = AEC_MAX_MIC_DELAY;
                }
                aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, mic_delay);

                aec_proc(aec, rin, sin, out);

                if (aec->frame_cnt<6)
                    memset(out,0,AEC_FRAME_SAMPLES * 2);
            }
            rb_write_buffer(&aec_out_rb, (uint8*)out, AEC_FRAME_SAMPLES * 2);
        }
    }
}


void app_aec_set_params(uint8_t* para)
{
    INFO_PRT("set aec para(%08X, %d, %02X, %d, %d, %d, %d)\r\n", aec, (int8_t)para[0], para[1], para[2], para[3], para[4], para[5]);

    if(aec) aec_ctrl(aec, AEC_CTRL_CMD_SET_PARAMS, (uint32_t)para);

}
#else
int32_t app_aec_read_out_buf(uint8_t* buf, uint32_t len) { return 0; }
void app_aec_fill_rin_buf(uint8_t *buff, uint8_t fid, uint32_t len) {}
#endif
