#ifndef _APP_EQU_H_
#define _APP_EQU_H_

#define CON_AUD_EQ_BANDS    16
#define CON_HFP_SPK_EQ_BANDS 4
#define CON_HFP_MIC_EQ_BANDS 4

//audio equalizer
typedef struct
{
    int16_t x[2][3];
    int16_t y[2][2];
    int    a[2];
    int    b[3];
    uint8_t flag_enable;
}__PACKED_POST__ aud_equ_t;

typedef   struct _app_eq_para_t
{
    int32_t	a[2];
    int32_t	b[3];
}__PACKED_POST__ app_eq_para_t;

typedef struct __app_aud_eq_s
{
    uint16_t eq_enable;//bit0->eq_para[0]
    int16_t	 eq_gain;
    app_eq_para_t eq_para[16];
} __PACKED_POST__ app_aud_eq_t;

#if (CONFIG_PRE_EQ == 1)
void pre_eq_proc(int32_t *buff,uint16_t size);
#endif
#if (CONFIG_HFP_SPK_EQ == 1)
void hfp_spk_eq_proc(uint8_t *input, uint16_t size);
#endif
#if (CONIFG_HFP_MIC_EQ == 1)
void hfp_mic_eq_proc(uint8_t *input, uint16_t size);
#endif

#endif
