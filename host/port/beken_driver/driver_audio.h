#ifndef _DRIVER_AUDIO_H_
#define _DRIVER_AUDIO_H_

#include "config.h"
#include "sys_types.h"
#include "compiler_arm.h"
#ifdef CONFIG_APP_EQUANLIZER
#include "app_equ.h"
#endif
#include "driver_dma.h"
#include "driver_ringbuff.h"

#ifdef USER_KARAOK_MODE
    #define AUDIO_FRAMEms           4
    #define SAMPLES_PER_FRAME       192//for 48000
    #define AUDIO_USBO_BUF_SIZE     2304//192*4(16bit2ch)*3frame
    #define AUDIO_USBI_BUF_SIZE     2304//192*4(16bit2ch)*3frame
#endif

#define AUDIO_ADC_BUFF_LEN      2048
#define AUDIO_DAC_BUFF_LEN      16384

#ifdef USER_KARAOK_MODE

#define AUD_M2D_MCU_RINGBUF_EN      //audio framework, don't modify
#define AUD_D2M_MCU_RINGBUF_EN

#ifdef AUD_D2M_MCU_RINGBUF_EN
    #define def_asi_rb_addr    (((uint32_t)&_aud_rb_begin) & ~0x3)
    #define def_asi_rb_size    AUDIO_ADC_BUFF_LEN
#endif

#ifdef AUD_M2D_MCU_RINGBUF_EN
    #define def_aso_rb_addr_declare     extern uint32_t _aud_rb_begin
    #define def_aso_rb_addr    ((((uint32_t)&_aud_rb_begin) & ~0x3) + AUDIO_ADC_BUFF_LEN)
    #define def_aso_rb_size    AUDIO_DAC_BUFF_LEN
#endif

#define def_aso_bit_width       24
#define def_aso_Byte2Smp(size)  ((size) >> (2 + ((def_aso_bit_width) != 16)))
#define def_aso_Smp2Byte(smps)  ((smps) << (2 + ((def_aso_bit_width) != 16)))

#define def_asi_bit_width       16
#define def_asi_Byte2Smp(size)  ((size) >> (2 + ((def_asi_bit_width) != 16)))
#define def_asi_Smp2Byte(smps)  ((smps) << (2 + ((def_asi_bit_width) != 16)))
#endif

#if A2DP_ROLE_SOURCE_CODE
#define AUDIO_SYNC_INTVAL       3000
#endif

#define AUDIO_MAX_FILT_NUM      5
#define AUDIO_VOLUME_MIN        0
#define AUDIO_VOLUME_MAX        32	//yuan++ 16->32

#define PAMUTE_GPIO_PIN         15
#define SDADC_VOLUME_MAX        124 //0.5db per step

typedef struct _aud_mute_cfg_s
{
    uint8_t mute_pin;
    uint8_t mute_high_flag;
    uint8_t mute_status;
    uint8_t shade_flag;
    uint8_t mute_outside;
	uint8_t auto_mute_flag; //�Զ�(����)������ʶ
    uint32_t mute_mask;
}aud_mute_cfg_t;

typedef struct aud_volome_s
{
	uint16_t ana_dig_gain; //bit15~bit13 = ana vol; bit12~bit0 = dig vol(18dB~-60dB)
}__PACKED_POST__ aud_volume_t;

typedef struct
{
    uint8_t* data_buff/*[AUDIO_DAC_BUFF_LEN]*/;

    void* dma_handle;

    driver_ringbuff_t aud_rb;
    int   empty_count;
    int   channels;
}AUDIO_CTRL_BLK;

typedef enum
{
    AUDIO_DAC_NORM = 0,
    AUDIO_DAC_SLOW = 0x01,
    AUDIO_DAC_SLOWER = 0x02,
    AUDIO_DAC_FAST = 0x04,
    AUDIO_DAC_FASTER = 0x08,
}t_dac_clk_state;

typedef enum _AUD_AS_TYPE_e{
    ASO_TYPE_NONE       = 0,
    ASO_TYPE_BT         = 1,
    ASO_TYPE_USB        = 2,
    ASO_TYPE_SD         = 4,
    ASO_TYPE_UDISK      = 8,
    ASO_TYPE_WAV        = 0x10,
    ASO_TYPE_SPDIF      = 0x20,
    ASO_TYPE_DEF        = ASO_TYPE_BT | ASO_TYPE_SD | ASO_TYPE_UDISK | ASO_TYPE_SPDIF,

    ASI_TYPE_NONE       = 0,
    ASI_TYPE_USB        = 1, //usb mic
    ASI_TYPE_SD         = 2, //sd record
    ASI_TYPE_UDISK      = 4, //udisk record
    ASI_TYPE_DEF        = ASI_TYPE_SD | ASI_TYPE_UDISK,
}AUD_AS_TYPE_e;

#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
extern AUDIO_CTRL_BLK audio_dac_ctrl_blk;
#endif
#if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
extern AUDIO_CTRL_BLK audio_adc_ctrl_blk;
#endif

void aud_init(void);

#ifdef USER_KARAOK_MODE

#ifdef AUD_WAV_TONE_SEPARATE
void aud_wav_fill_buffer(uint8_t *buff, uint16_t size);
int aud_wav_ringbuf_free_get(void);
#define bt_audio_ch_busy()      0
#endif

void audio_aso_type_set(AUD_AS_TYPE_e as_type, uint8_t en);
uint32_t audio_aso_type_get(AUD_AS_TYPE_e as_type);
void audio_asi_type_set(AUD_AS_TYPE_e as_type, uint8_t en);
uint32_t audio_asi_type_get(AUD_AS_TYPE_e as_type);


int audio_aso_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample, AUD_AS_TYPE_e as_type);
void audio_aso_open(AUD_AS_TYPE_e as_type);
void audio_aso_close(AUD_AS_TYPE_e as_type);
uint32_t aud_dac_sample_rate_cur_get(void);

int aud_aso_ringbuf_write(unsigned char *buff, int size, AUD_AS_TYPE_e as_type);
int aud_aso_ringbuf_fill_get(AUD_AS_TYPE_e as_type);
int aud_aso_ringbuf_free_get(AUD_AS_TYPE_e as_type);
int aud_asi_ringbuf_read(unsigned char *buff, int size, AUD_AS_TYPE_e as_type);
int aud_asi_ringbuf_fill_get(AUD_AS_TYPE_e as_type);

#ifdef AUD_M2D_MCU_RINGBUF_EN
/****************************** def audio out ********************************/
//def_aso_ringbuf
#endif

/******************************usb audio out********************************/
#ifdef AUD_M2D_MCU_RINGBUF_EN
int usb_out_ringbuff_read(uint8_t *buff, int size);
int usb_out_ringbuff_write(uint8_t *buff, int size);
int usb_out_ringbuff_free_get(void);
int usb_out_ringbuff_fill_get(void);
void usb_out_ringbuff_clear(void);
#endif

#ifdef AUD_M2D_MCU_RINGBUF_EN
//move audio data from usb ringbuff to dsp
void audio_mcu2dsp_play(void);
#endif
/******************************usb audio in********************************/
#ifdef AUD_D2M_MCU_RINGBUF_EN
int usb_in_ringbuff_read(uint8_t *buff, int size);
int usb_in_ringbuff_write(uint8_t *buff, int size);
int usb_in_ringbuff_free_get(void);
int usb_in_ringbuff_fill_get(void);
void usb_in_ringbuff_clear(void);
#endif
//move audio data from usb ringbuff to dsp
void audio_dsp2mcu_record(void);

#else //USER_KARAOK_MODE

#ifndef bt_audio_ch_busy()
    #define bt_audio_ch_busy()      app_wave_playing()
#endif

#endif //USER_KARAOK_MODE

int  aud_dac_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample);
void aud_dac_open(void);
void aud_dac_close(void);
void aud_dac_dig_volume_fade_in(void);
#if(CONFIG_DAC_CLOSE_IN_IDLE == 1)
void aud_dac_close_in_idle(void);
#endif
void aud_dac_clk_set_coef(uint32_t clk_val);
void aud_dac_clk_set_default(void);
void aud_dac_clk_process(void);
void aud_dac_set_volume(int8_t volume);
void aud_dac_fill_buffer(uint8_t *buff, uint16_t size);
uint16_t aud_dac_get_free_buffer_size(void);
uint16_t aud_dac_get_fill_buffer_size(void);
void DRAM_CODE adio_sco_fill_buffer(uint8_t *buff, uint8_t fid,uint16_t size );
void aud_dac_buffer_clear(void);


int audio_asi_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample, AUD_AS_TYPE_e as_type);
void audio_asi_open(uint8_t enable, AUD_AS_TYPE_e as_type);

int  aud_mic_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample);
void aud_mic_open(int enable);
void aud_mic_set_volume( uint8_t volume );
void aud_mic_mute(uint8_t enable);
uint16_t aud_mic_get_fill_buffer_size(void);
uint32_t aud_mic_read_buffer(uint8_t* buf, uint16_t len);

void aud_mute_init(void);
void aud_mute_func_init(int8_t fast_shift, int8_t slow_shift);
void aud_mute_update(int16_t samplel, int16_t sampler);

uint8_t aud_discard_sco_data(void);
void    aud_volume_mute(uint8_t enable);
void    aud_dac_volume_control( uint8_t *buff, uint32_t size);

#if(CONFIG_AUD_FADE_IN_OUT == 1)
typedef enum
{
    AUD_FADE_NONE       = 0,
    AUD_FADE_IN         = 1,
    AUD_FADE_OUT        = 2,
    AUD_FADE_FINISHED   = 4
}t_aud_fade_state;
__INLINE__ void  set_aud_fade_in_out_state(t_aud_fade_state state);
__INLINE__ t_aud_fade_state get_aud_fade_in_out_state(void);
void aud_fade_in_out_process(void);
#endif

void aud_PAmute_operation(int enable);
void aud_PAmute_delay_operation(int enable);

void extPA_open(uint8_t delay_enable);
void extPA_close(uint8_t delay_enable);
void extPA_set_req(uint8_t open_req);

#if A2DP_ROLE_SOURCE_CODE
typedef struct{
	union {
	    struct sync_start_s{
			uint32_t start_time : 28;
			uint32_t flag : 4;
			uint32_t clk_val :28;
			uint32_t vol :4;
	    } __PACKED_POST__ sync_start;

	    struct sync_send_s{
			uint32_t bt_clk : 28;
			uint32_t clk_mode : 4;
			uint32_t aud_num;
	    } __PACKED_POST__ sync_send;
	}u;
	int16_t aud_num_tmp;
}__PACKED_POST__ sync_data_TypeDef;
extern sync_data_TypeDef sync_data;
#endif

#if (CONFIG_PRE_EQ == 1)
void app_set_pre_eq_gain(uint8_t *para);
void app_set_pre_eq(uint8_t *para);
void app_show_pre_eq(void);
void app_set_eq_gain_enable(uint8_t *para);

typedef struct _aud_pre_equ_para_s
{
    int    a[2];
    int    b[3];
}__PACKED_POST__ aud_pre_equ_para_t;

typedef struct _aud_pre_equ_s
{
	int online_flag;
	uint16_t totle_EQ;
	uint32_t globle_gain;
}__PACKED_POST__ aud_pre_equ_t;
#endif

#if (CONFIG_HFP_SPK_EQ == 1)
void app_set_hfp_spk_eq_gain(uint8_t *para);
void app_set_hfp_spk_eq(uint8_t *para);
void app_show_hfp_spk_eq(void);
void app_set_spk_eq_gain_enable(uint8_t *para);

typedef struct _hfp_spk_equ_para_s
{
    int    a[2];
    int    b[3];
}__PACKED_POST__ hfp_spk_equ_para_t;

typedef struct _hfp_spk_equ_s
{
	int online_flag;
	uint16_t totle_EQ;
	uint32_t globle_gain;
}__PACKED_POST__ hfp_spk_equ_t;
#endif

#if (CONIFG_HFP_MIC_EQ == 1)
void app_set_hfp_mic_eq_gain(uint8_t *para);
void app_set_hfp_mic_eq(uint8_t *para);
void app_show_hfp_mic_eq(void);
void app_set_mic_eq_gain_enable(uint8_t *para);

typedef struct _hfp_mic_equ_para_s
{
    int    a[2];
    int    b[3];
}__PACKED_POST__ hfp_mic_equ_para_t;

typedef struct _hfp_mic_equ_s
{
	int online_flag;
	uint16_t totle_EQ;
	uint32_t globle_gain;
}__PACKED_POST__ hfp_mic_equ_t;
#endif

#if (CONFIG_APP_MSBC_RESAMPLE == 1)
#define NFIR		65
#define N_IIR	    13

typedef struct fir_state {
	int16_t ratio;
	int16_t taps;
	int16_t curr_pos;
	int16_t *coef;
	int16_t laststate[NFIR];
}t_FIR_STATE,*pFIR_STATE;
typedef struct
{
	int16_t *b;
	int16_t *a;
	int16_t y[N_IIR];
	int16_t x[N_IIR];
}t_IIR_State;

int16_t fir_hardcore_init(void);
int16_t fir_hardcore_close(void);
int16_t fir_hardcore_filter(int16_t *smpl,uint16_t nSmpls);
int16_t FIR_sw_filter_init(pFIR_STATE p_fir_st);
int16_t FIR_sw_filter_exe(pFIR_STATE p_fir_st,int16_t *in,int16_t *out,int16_t len);
void IIR_filter_init(t_IIR_State *st);
void IIR_filter_exe(t_IIR_State *st,int16_t *x,int16_t *y,int16_t len);
#endif

#endif
