/**
 **************************************************************************************
 * @file    drv_audio.c
 * @brief   Driver API for audio codec
 *
 * @author  Aixing.Li
 * @version V3.0.0
 *
 * &copy; 2018~2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "bkreg.h"
#include "drv_audio.h"
#include "drv_system.h"

#define AUDIO_LPF_FILTER_TYPE_FIR    (0)
#define AUDIO_LPF_FILTER_TYPE_IIR    (1)
#define AUDIO_ANC_LPF_FILTER_TYPE    AUDIO_LPF_FILTER_TYPE_FIR
#define AUDIO_ADC_LPF_FILTER_TYPE    AUDIO_LPF_FILTER_TYPE_FIR
#define AUDIO_DAC_LPF_FILTER_TYPE    AUDIO_LPF_FILTER_TYPE_FIR

#define audio_dig_ctxt  ((AudioDigContext*)MDU_ANC_BASE_ADDR)

#if     defined(__BA2__)
#define ATTRIBUTE_AT_CONST_DATA_SECTION	__attribute__((section(".rodata")))
#elif   defined(CEVAX2) || defined(TEAKLITE4)
#define ATTRIBUTE_AT_CONST_DATA_SECTION	__attribute__((section("const_data")))
#else
#define ATTRIBUTE_AT_CONST_DATA_SECTION
#endif

typedef enum
{
    AUDIO_ADC_CIC1_D1_128 = 0,
    AUDIO_ADC_CIC1_D1_64,
    AUDIO_ADC_CIC1_D1_32,
    AUDIO_ADC_CIC1_D1_16,
    AUDIO_ADC_CIC1_D1_8,
    AUDIO_ADC_CIC1_D1_4,
    AUDIO_ADC_CIC1_D1_192 = 8,
    AUDIO_ADC_CIC1_D1_96,
    AUDIO_ADC_CIC1_D1_48,
    AUDIO_ADC_CIC1_D1_24,
    AUDIO_ADC_CIC1_D1_12,
    AUDIO_ADC_CIC1_D1_6,
}AUDIO_ADC_CIC1_D1;

typedef enum
{
    AUDIO_DAC_CIC1_D2_8 = 0,
    AUDIO_DAC_CIC1_D2_16,
    AUDIO_DAC_CIC1_D2_32,
    AUDIO_DAC_CIC1_D2_64,
}AUDIO_DAC_CIC1_D2;

typedef enum
{
    AUDIO_DAC_CIC2_D3_1 = 0,
    AUDIO_DAC_CIC2_D3_2,
    AUDIO_DAC_CIC2_D3_4,
    AUDIO_DAC_CIC2_D3_8,
    AUDIO_DAC_CIC2_D3_16,
    AUDIO_DAC_CIC2_D3_32,
    AUDIO_DAC_CIC2_D3_3 = 8,
    AUDIO_DAC_CIC2_D3_6,
    AUDIO_DAC_CIC2_D3_12,
    AUDIO_DAC_CIC2_D3_24,
}AUDIO_DAC_CIC2_D3;

typedef enum
{
    AUDIO_DAC_SRD_16 = 0,
    AUDIO_DAC_SRD_32,
    AUDIO_DAC_SRD_64,
    AUDIO_DAC_SRD_128,
    AUDIO_DAC_SRD_256,
    AUDIO_DAC_SRD_512,
    AUDIO_DAC_SRD_24 = 8,
    AUDIO_DAC_SRD_48,
    AUDIO_DAC_SRD_96,
    AUDIO_DAC_SRD_192,
    AUDIO_DAC_SRD_384,
    AUDIO_DAC_SRD_768,
}AUDIO_DAC_SAMPLE_RATE_DIV;

typedef struct _AudioDigContext
{
    volatile uint32_t reg0; //adc config
    volatile uint32_t reg1; //dac config
    volatile uint32_t reg2; //adc fifo config
    volatile uint32_t reg3; //dac fifo config
    volatile uint32_t reg4; //adc & dac fifo config
    volatile uint32_t reg5; //int enable & dac lpf config
    volatile uint32_t reg6; //filters settings
    volatile uint32_t reg7; //dac fifo config & adc gain setting

    volatile uint32_t reserved[56];

    volatile uint32_t status;

    volatile uint32_t anc_ch0_din;
    volatile uint32_t anc_ch1_din;
    volatile uint32_t anc_ch2_din;
    volatile uint32_t anc_ch3_din;

    volatile uint32_t dac_tx2_dout0;
    volatile uint32_t dac_tx2_dout1;

    volatile uint32_t adc_din;
    volatile uint32_t anc_din;//interleave
    volatile uint32_t adc_2fs_din;

    volatile uint32_t dac_tx1_2fs_din0;
    volatile uint32_t dac_tx1_2fs_din1;
    volatile uint32_t dac_tx1_lpf_din0;
    volatile uint32_t dac_tx1_lpf_din1;

    volatile uint32_t dac_tx1_dout0;
    volatile uint32_t dac_tx1_dout1;

    volatile uint32_t adc_lpf1_coefs[16];
    volatile uint32_t anc_lpf1_coefs[16];
    volatile uint32_t dac_lpf4_coefs[16];
    volatile uint32_t adc_lpf2_coefs[128];
    volatile uint32_t anc_lpf2_coefs[128];
    volatile uint32_t dac_lpf3_coefs[128];

}AudioDigContext;

#if ((AUDIO_ANC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR)\
    || (AUDIO_ADC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR)\
    || (AUDIO_DAC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR))
ATTRIBUTE_AT_CONST_DATA_SECTION const static unsigned long AUDIO_LPF_IIR_COEF[] =
{
    0x00002F1B,
    0x00005048,
    0x00002F1B,
    0x001B4880,
    0x0034068E,
    0x0,
    0x0,
    0x0,
    0x00002F1B,
    0x00005048,
    0x00002F1B,
    0x001B4880,
    0x0034068E,
    0x0,
    0x0,
    0x0,
    0x0000F9DB,
    0x000070A4,
    0x0000F9DB,
    0x0016B296,
    0x0036E1B1,
    0x0,
    0x0,
    0x0,
    0x0000F9DB,
    0x000070A4,
    0x0000F9DB,
    0x0016B296,
    0x0036E1B1,
    0x0,
    0x0,
    0x0
};
#endif

ATTRIBUTE_AT_CONST_DATA_SECTION const static unsigned long AUDIO_LPF_FIR_COEF[] =
{
    0x9d    ,
    0x99    ,
    0x3fff06,
    0x3ffd62,
    0x3ffea6,
    0x1dd   ,
    0x1d4   ,
    0x3ffd98,
    0x3ffc3d,
    0x1c5   ,
    0x5b5   ,
    0x3fff96,
    0x3ff828,
    0x3ffdd0,
    0x993   ,
    0x60f   ,
    0x3ff59a,
    0x3ff4ce,
    0x9a7   ,
    0x114a  ,
    0x3ff950,
    0x3fe83c,
    0xec    ,
    0x1dbf  ,
    0x800   ,
    0x3fddea,
    0x3febe1,
    0x2370  ,
    0x230a  ,
    0x3fdfa2,
    0x3fcc2c,
    0x1789  ,
    0x4500  ,
    0x3ff829,
    0x3fab7f,
    0x3ff0aa,
    0x5fd0  ,
    0x2df8  ,
    0x3f9bf6,
    0x3facc6,
    0x5e22  ,
    0x7d57  ,
    0x3fb4f0,
    0x3f5671,
    0x2806  ,
    0xd421  ,
    0xd70   ,
    0x3f07b4,
    0x3fa896,
    0x11047 ,
    0xb7e0  ,
    0x3eeb05,
    0x3ece94,
    0xfd26  ,
    0x1c924 ,
    0x3f452e,
    0x3d748f,
    0x3373  ,
    0x39b57 ,
    0xddcb  ,
    0x3a8abd,
    0x3c6a85,
    0xb45e7 ,
    0x1adb6d,
};

void audio_init(void)
{
    uint32_t reg;

    reg  = REG_SYSTEM_0x4F;
    reg &= ~(0x1F << 23);
    reg |= (1 << 5) | (1 << 16) | (1 << 21) | (0x1F << 23) | (1 << 28);
    REG_SYSTEM_0x4F = reg;

    reg  = REG_SYSTEM_0x57;
    reg &= ~((3 << 6) | (3 << 14) | (1 << 21) | (1 << 23));
    reg |= (1 << 4) | (2 << 6) | (2 << 14);
    REG_SYSTEM_0x57 = reg;

    audio_dig_ctxt->reg6 &= ~MSK_ANC_0x06_ENABLE;
    audio_dig_ctxt->adc_lpf1_coefs[0 ] = 0x00002ef2;
    audio_dig_ctxt->adc_lpf1_coefs[1 ] = 0x00007466;
    audio_dig_ctxt->adc_lpf1_coefs[2 ] = 0x003f6299;
    audio_dig_ctxt->adc_lpf1_coefs[3 ] = 0x003ce8a9;
    audio_dig_ctxt->adc_lpf1_coefs[4 ] = 0x003efc27;
    audio_dig_ctxt->adc_lpf1_coefs[5 ] = 0x000b1366;
    audio_dig_ctxt->adc_lpf1_coefs[6 ] = 0x0018d494;
    audio_dig_ctxt->adc_lpf1_coefs[7 ] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[8 ] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[9 ] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[10] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[11] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[12] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[13] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[14] = 0x0;
    audio_dig_ctxt->adc_lpf1_coefs[15] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[0 ] = 0x00002ef2;
    audio_dig_ctxt->anc_lpf1_coefs[1 ] = 0x00007466;
    audio_dig_ctxt->anc_lpf1_coefs[2 ] = 0x003f6299;
    audio_dig_ctxt->anc_lpf1_coefs[3 ] = 0x003ce8a9;
    audio_dig_ctxt->anc_lpf1_coefs[4 ] = 0x003efc27;
    audio_dig_ctxt->anc_lpf1_coefs[5 ] = 0x000b1366;
    audio_dig_ctxt->anc_lpf1_coefs[6 ] = 0x0018d494;
    audio_dig_ctxt->anc_lpf1_coefs[7 ] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[8 ] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[9 ] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[10] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[11] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[12] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[13] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[14] = 0x0;
    audio_dig_ctxt->anc_lpf1_coefs[15] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[0 ] = 0x002EF2;
    audio_dig_ctxt->dac_lpf4_coefs[1 ] = 0x3F6299;
    audio_dig_ctxt->dac_lpf4_coefs[2 ] = 0x3EFC27;
    audio_dig_ctxt->dac_lpf4_coefs[3 ] = 0x18D494;
    audio_dig_ctxt->dac_lpf4_coefs[4 ] = 0x0B1366;
    audio_dig_ctxt->dac_lpf4_coefs[5 ] = 0x3CE8A9;
    audio_dig_ctxt->dac_lpf4_coefs[6 ] = 0x007466;
    audio_dig_ctxt->dac_lpf4_coefs[7 ] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[8 ] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[9 ] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[10] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[11] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[12] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[13] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[14] = 0x0;
    audio_dig_ctxt->dac_lpf4_coefs[15] = 0x0;

    audio_dig_ctxt->reg0 = 0x00000000;
    audio_dig_ctxt->reg1 = 0x00000000 | (1 << 15);

    #if AUDIO_ANC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->anc_lpf2_coefs;
        for(i = 0; i < 32; i++) *coefs++ = AUDIO_LPF_IIR_COEF[i];
        for(; i < 128; i++)     *coefs++ = 0;

        audio_dig_ctxt->reg0 |= 0x00020000;
    }
    #else
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->anc_lpf2_coefs;
        for(i = 0; i < 64; i++) *coefs++ = AUDIO_LPF_FIR_COEF[i];
        for(; i < 128; i++)     *coefs++ = 0;
    }
    #endif

    #if AUDIO_ADC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->adc_lpf2_coefs;
        for(i = 0; i < 32; i++) *coefs++ = AUDIO_LPF_IIR_COEF[i];
        for(; i < 128; i++)     *coefs++ = 0;

        audio_dig_ctxt->reg0 |= 0x00000200;
    }
    #else
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->adc_lpf2_coefs;
        for(i = 0; i < 64; i++) *coefs++ = AUDIO_LPF_FIR_COEF[i];
        for(; i < 128; i++)     *coefs++ = 0;
    }
    #endif

    #if AUDIO_DAC_LPF_FILTER_TYPE == AUDIO_LPF_FILTER_TYPE_IIR
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->dac_lpf3_coefs;
        for(i = 0; i < 32; i++) *coefs++ = AUDIO_LPF_IIR_COEF[i];

        audio_dig_ctxt->reg1 |= 0x001000;
    }
    #else
    {
        uint32_t  i;
        uint32_t* coefs;

        coefs = (uint32_t*)audio_dig_ctxt->dac_lpf3_coefs;
        for(i = 0; i < 32; i++) *coefs++ = AUDIO_LPF_FIR_COEF[2 * i];
        for(i = 0; i < 32; i++) *coefs++ = AUDIO_LPF_FIR_COEF[63 - 2 * i];
        for(i = 0; i < 64; i++) *coefs++ = 0;
    }
    #endif

    audio_dig_ctxt->reg2 = 0x3f7f3f7f;
    #if defined(CEVAX2)
    sys_delay_ms(1);
    #endif
    audio_dig_ctxt->reg3 = 0x7f3f7f3f;
    audio_dig_ctxt->reg4 = 0x3f7f7f3f;
    audio_dig_ctxt->reg5 = 0x0bf64300;
    audio_dig_ctxt->reg6 = 0x13f3f661;
    audio_dig_ctxt->reg7 = 0x2d2d7f3f;
}

static void audio_adc_anc_init(uint32_t channels, uint32_t bits, uint32_t mode)
{
    uint32_t ch, channel, width;

    if(bits == 16 || bits == 20 || bits == 24) bits = 6 - bits / 4;
    if(bits > 2) return;

    width = bits;
    bits  = (6 - bits) * 4;

    for(ch = 0; ch < 5; ch++)
    {
        channel = channels & (1 << ch);

        if(channel)
        {
            uint32_t* reg = (uint32_t*)&REG_SYSTEM_0x50 + ch;
            uint32_t  val = *reg | (1 << 30);

            if(channel == AUDIO_ADC_CHANNEL_0)
            {
                audio_dig_ctxt->reg0 &= ~MSK_ANC_0x00_ADC_CHN_CUT;
                audio_dig_ctxt->reg0 |= (width << SFT_ANC_0x00_ADC_CHN_CUT);
            }
            else
            {
                audio_dig_ctxt->reg0 &= ~MSK_ANC_0x00_ADC_ANC_CUT;
                audio_dig_ctxt->reg0 |= (width << SFT_ANC_0x00_ADC_ANC_CUT);
            }

            switch(mode)
            {
            case AUDIO_ADC_MODE_DIFFERENCE:
                val &= ~((3 << 0) | (1 << 13) | (1 << 19) | (1 << 20));
                val |= (2 << 0);
                *reg = val | (1 << 29);
                *reg = val;
                break;
            case AUDIO_ADC_MODE_SINGLE_END:
                val &= ~((3 << 0) | (1 << 14) | (1 << 19) | (1 << 20));
                val |= (2 << 0) | (1 << 13);
                *reg = val | (1 << 29);
                *reg = val;
                break;
            case AUDIO_ADC_MODE_AUTO_DC_CALIB:
                {
                    #define AUDIO_ADC_CALIB_USE_HARDWARE    (0)
                    #define AUDIO_ADC_CALIB_DISCARD_SAMPLES (1024)
                    #define AUDIO_ADC_CALIB_DC_PCM_SAMPLES  (256)

                    int32_t  i, s, sum;
                    uint32_t absd, calib;
                    uint32_t optc, optd, gain;
                    uint32_t mask  = channel == AUDIO_ADC_CHANNEL_0 ? MSK_ANC_0x40_RX_CHN_NEMPT : MSK_ANC_0x40_RX_ANC_NEMPT;
                    int32_t* port  = channel == AUDIO_ADC_CHANNEL_0 ? (int32_t*)&audio_dig_ctxt->adc_din : (int32_t*)&audio_dig_ctxt->anc_ch0_din + ch - 1;

                    //Digital ADC enable
                    audio_dig_ctxt->reg0 |= MSK_ANC_0x00_ADC_ENABLE | (1 << (ch + 1));

                    sys_delay_ms(10);

                    //Audio LDO & MIC bias enable
                    REG_SYSTEM_0x4F |= (1 << 5);
                    REG_SYSTEM_0x57 |= (1 << 21) | (1 << 23);

                    gain = val & (0xF << 15);
                    val &= ~((3 << 0) | (3 << 4) | (0x7F << 6) | (1 << 14) | (0xF << 15) | (3 << 20));
                    val |= (2 << 0) | (2 << 4) | (0 << 13) | (1 << 19) | (1 << 20) | (AUDIO_ADC_CALIB_USE_HARDWARE << 21) | (1 << 28);
                    *reg = val;

                    calib = 0;
                    optc  = 0;
                    optd  = 0xFFFFFFFF;

                    for(i = 6; i >= 0; i--)
                    {
                        calib |= (1 << i);

                        *reg = val | (calib << 6) | (1 << 29);
                        sys_delay_ms(1);
                        *reg = val | (calib << 6);
                        sys_delay_ms(10 * (AUDIO_ADC_CALIB_USE_HARDWARE ? 1 : 3));

                        s   = 0;
                        sum = 0;
                        do
                        {
                            if(!(REG_ANC_0x40 & mask))
                            {
                                int32_t t = *port;
                                t <<= (32 - bits);
                                t >>= 16;
                                sum += t;
                                if(++s == AUDIO_ADC_CALIB_DISCARD_SAMPLES) sum = 0;
                            }
                        }while(s < AUDIO_ADC_CALIB_DISCARD_SAMPLES + AUDIO_ADC_CALIB_DC_PCM_SAMPLES);

                        sum = (sum + AUDIO_ADC_CALIB_DC_PCM_SAMPLES / 2) / AUDIO_ADC_CALIB_DC_PCM_SAMPLES;

                        absd = sum > 0 ? sum : -sum;

                        if(optd > absd)
                        {
                            optd = absd;
                            optc = calib;
                        }

                        //bk_printf(">>>1: CALIB = %02X, DC = %d\r\n", calib, sum);

                        if(sum > 0) calib &= ~(1 << i);
                    }

                    calib = optc;

                    val |= (calib << 6);
                    *reg = val | (1 << 29);
                    sys_delay_ms(1);
                    *reg = val;
                    sys_delay_ms(10);

                    #if AUDIO_ADC_CALIB_USE_HARDWARE
                    val &= ~(1 << 21);
                    *reg = val;
                    sys_delay_ms(100);
                    #endif

                    #if AUDIO_ADC_CALIB_USE_HARDWARE

                    val &= ~(0x7F << 6);

                    calib = optc;
                    optd  = 0xFFFFFFFF;

                    do
                    {
                        s   = 0;
                        sum = 0;
                        do
                        {
                            if(!(REG_ANC_0x40 & mask))
                            {
                                int32_t t = *port;
                                t <<= (32 - bits);
                                t >>= 16;
                                sum += t;
                                if(++s == AUDIO_ADC_CALIB_DISCARD_SAMPLES) sum = 0;
                            }
                        }while(s < AUDIO_ADC_CALIB_DISCARD_SAMPLES + AUDIO_ADC_CALIB_DC_PCM_SAMPLES);

                        sum = (sum + AUDIO_ADC_CALIB_DC_PCM_SAMPLES / 2) / AUDIO_ADC_CALIB_DC_PCM_SAMPLES;

                        absd = sum > 0 ? sum : -sum;

                        if(optd > absd)
                        {
                            optd = absd;
                            optc = calib;
                        }

                        //bk_printf(">>>2: CALIB = %02X, DC = %d\r\n", calib, sum);

                        if(sum > 0 || absd < 250) break;

                        if(++calib >= 0x80)
                        {
                            calib--;
                            break;
                        }

                        *reg = val | (calib << 6);
                        *reg = val | (calib << 6) | (1 << 29);
                        sys_delay_ms(1);
                        *reg = val | (calib << 6);
                        sys_delay_ms(10);
                    }while(1);

                    calib = optc;

                    val |= (calib << 6);

                    #endif

                    mask = !!((audio_dig_ctxt->reg0 & (0x1F << 1)) & ~(1 << (ch + 1)));

                    audio_dig_ctxt->reg0 &= ~((!mask) | (1 << (ch + 1)));

                    val &= ~(1 << 28);
                    *reg = val | gain;

                    if(!mask)
                    {
                        REG_SYSTEM_0x4F &= ~(1 << 5);
                        REG_SYSTEM_0x57 &= ~((1 << 21) | (1 << 23));
                    }

                    //bk_printf(">>>3: CALIB = %02X, DC = %d\n", optc, optd);
                }
                break;
            }
        }
    }
}

#if CONFIG_AUDIO_DAC_ALWAYSOPEN
static void audio_dac_keep_open(uint8_t enb_ana, const char func_name[])
{
    audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
	#if 1 //Fill zeros pcm data
	{
		uint32_t cnt1 = 63;
		uint32_t cnt2 = 63;

		while(cnt1 && cnt2)
		{
			if(cnt1 && !(REG_ANC_0x40 & MSK_ANC_0x40_TX1_NFULL))
			{
				audio_dig_ctxt->dac_tx1_dout0 = 1;
				audio_dig_ctxt->dac_tx1_dout0 = 1;
				cnt1--;
			}

			if(cnt2 && !(REG_ANC_0x40 & MSK_ANC_0x40_TX2_NFULL))
			{
				audio_dig_ctxt->dac_tx2_dout0 = 1;
				audio_dig_ctxt->dac_tx2_dout0 = 1;
				cnt2--;
			}
		}

		audio_dig_ctxt->dac_tx1_dout0 = 0;
		audio_dig_ctxt->dac_tx1_dout0 = 0;
		audio_dig_ctxt->dac_tx2_dout0 = 0;
		audio_dig_ctxt->dac_tx2_dout0 = 0;
	}
	#endif

	if(enb_ana)
	{
		//sys_delay_us(100);
		REG_SYSTEM_0x57 |= (1 << 21) | (1 << 23); //audio ldo1.0 & ldo1.5 enable
		REG_SYSTEM_0x55 |= (3 << 15) | (1 << 19); //audio DCOC & PA enable
		REG_SYSTEM_0x56 |= (1 << 23); //audio DAC bias enable
		sys_delay_ms(1);
	        if(!app_check_bt_mode(BT_MODE_TWS|BT_MODE_BLE|BT_MODE_DM_TWS))
	        {
	        	REG_SYSTEM_0x55 |= (3 << 20); //for differ mode dac left & right channel enable
	        }
	        else
	        {
			 if(app_check_bt_mode(BT_MODE_BLE))
			 	REG_SYSTEM_0x55 &=~ (3 << 20); //not need open dac
			else
				REG_SYSTEM_0x55 |= (2 << 20); //dac left channel enable
	        }
			// REG_SYSTEM_0x55 |= (2 << 20); //dac left channel enable
	}
    audio_dig_ctxt->reg1 |= (MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);

	LOG_I(DRV, "DAC keep open @%s\r\n", func_name);
}
#endif

static void audio_adc_anc_enable(uint32_t channels, uint32_t enable)
{
    uint32_t ch;

    for(ch = 0; ch < 5; ch++)
    {
        if(channels & (1 << ch))
        {
            uint32_t* reg = (uint32_t*)&REG_SYSTEM_0x50 + ch;
            uint32_t  val = *reg;

            if(enable)
            {
                REG_SYSTEM_0x4F |= (1 << 5);
                REG_SYSTEM_0x57 |= (1 << 21) | (1 << 23);

                if(!(val & (3 << 19))) REG_SYSTEM_0x57 |= (1 << 24);

                val |= (1 << 28);
                *reg = val | (1 << 29);
                sys_delay_ms(1);
                *reg = val;
				//sel mic_bias
				app_env_handle_t  env_h = app_env_get_handle();
				REG_SYSTEM_0x57 = (REG_SYSTEM_0x57 & ~(0x03<<25))
					|((( env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_MIC_BIAS ) >> 13) << 25);

                audio_dig_ctxt->reg0 |= MSK_ANC_0x00_ADC_ENABLE | (1 << (ch + 1));
            }
            else
            {
                uint32_t en = !!((audio_dig_ctxt->reg0 & (0x1F << 1)) & ~(1 << (ch + 1)));

                audio_dig_ctxt->reg0 &= ~((!en) | (1 << (ch + 1)));

                val &= ~(1 << 28);
                *reg = val;

                if(!en)
                {
                    REG_SYSTEM_0x4F &= ~(1 << 5);
                    REG_SYSTEM_0x57 &= ~(1 << 24);
                    if(!(audio_dig_ctxt->reg1 & MSK_ANC_0x01_DAC_ENABLE)) REG_SYSTEM_0x57 &= ~((1 << 21) | (1 << 23));
                }
            }
        }
    }
}

static void audio_adc_anc_sample_rate_set(uint32_t channel, uint32_t samplerate)
{
    int32_t  d1;
    uint32_t lpf1_bypass = 0;
    uint32_t lpf2_bypass = 0;

    switch(samplerate)
    {
    case 8000:
        d1 = AUDIO_ADC_CIC1_D1_192;
        break;
    case 16000:
        d1 = AUDIO_ADC_CIC1_D1_96;
        break;
    case 32000:
        d1 = AUDIO_ADC_CIC1_D1_48;
        break;
    case 64000:
        d1 = AUDIO_ADC_CIC1_D1_24;
        break;
    case 128000:
        d1 = AUDIO_ADC_CIC1_D1_12;
        break;
    case 256000:
        d1 = AUDIO_ADC_CIC1_D1_6;
        break;
    case 11025:
    case 12000:
        d1 = AUDIO_ADC_CIC1_D1_128;
        break;
    case 22050:
    case 24000:
        d1 = AUDIO_ADC_CIC1_D1_64;
        break;
    case 44100:
    case 48000:
        d1 = AUDIO_ADC_CIC1_D1_32;
        break;
    case 88200:
    case 96000:
        #if 0
        d1 = AUDIO_ADC_CIC1_D1_16;
        #else
        d1 = AUDIO_ADC_CIC1_D1_64;
        lpf1_bypass = 1;
        lpf2_bypass = 1;
        #endif
        break;
    case 176400:
    case 192000:
        #if 0
        d1 = AUDIO_ADC_CIC1_D1_8;
        #else
        d1 = AUDIO_ADC_CIC1_D1_32;
        lpf1_bypass = 1;
        lpf2_bypass = 1;
        #endif
        break;
    case 352800:
    case 384000:
        d1 = AUDIO_ADC_CIC1_D1_16;
        lpf1_bypass = 1;
        lpf2_bypass = 1;
        break;
    default:
        d1 = -1;
        break;
    }

    if(d1 >= 0)
    {
        if(channel == 0)
        {
            audio_dig_ctxt->reg0 &= ~(MSK_ANC_0x00_RX_CHN_D1_BIT20 | MSK_ANC_0x00_RX_CHN_D1_BIT3 | MSK_ANC_0x00_LPF1_CHN_BPS | MSK_ANC_0x00_LPF2_CHN_BPS);
            audio_dig_ctxt->reg0 |= (((d1 & 7) << SFT_ANC_0x00_RX_CHN_D1_BIT20) | ((d1 >> 3) << SFT_ANC_0x00_RX_CHN_D1_BIT3)) | (lpf1_bypass << SFT_ANC_0x00_LPF1_CHN_BPS) | (lpf2_bypass << SFT_ANC_0x00_LPF2_CHN_BPS);
        }
        else
        {
            audio_dig_ctxt->reg0 &= ~(MSK_ANC_0x00_RX_ANC_D1_BIT20 | MSK_ANC_0x00_RX_ANC_D1_BIT3 | MSK_ANC_0x00_LPF1_ANC_BPS | MSK_ANC_0x00_LPF2_ANC_BPS);
            audio_dig_ctxt->reg0 |= (((d1 & 7) << SFT_ANC_0x00_RX_ANC_D1_BIT20) | ((d1 >> 3) << SFT_ANC_0x00_RX_ANC_D1_BIT3)) | (lpf1_bypass << SFT_ANC_0x00_LPF1_ANC_BPS) | (lpf2_bypass << SFT_ANC_0x00_LPF2_ANC_BPS);
        }
    }
}

static uint32_t audio_adc_anc_ana_gain_get(uint32_t channels)
{
    uint32_t ch;

    for(ch = 0; ch < 5; ch++)
    {
        if(channels & (1 << ch))
        {
            return ((*((uint32_t*)&REG_SYSTEM_0x50 + ch)) & (0xF << 15)) >> 15;
        }
    }

    return 0;
}

static void audio_adc_anc_ana_gain_set(uint32_t channels, uint32_t gain)
{
    uint32_t ch;

    for(ch = 0; ch < 5; ch++)
    {
        if(channels & (1 << ch))
        {
            uint32_t* reg = (uint32_t*)&REG_SYSTEM_0x50 + ch;
            uint32_t  val = *reg;

            val &= ~(0xF << 15);
            val |= (gain & 0xF) << 15;

            *reg = val;
        }
    }
}

void audio_adc_init(uint32_t bits, uint32_t mode)
{
    audio_adc_anc_init(AUDIO_ADC_CHANNEL_0, bits, mode);
}

void audio_adc_sample_rate_set(uint32_t samplerate)
{
    audio_adc_anc_sample_rate_set(0, samplerate);
}

void audio_adc_ana_gain_set(uint32_t gain)
{
    audio_adc_anc_ana_gain_set(AUDIO_ADC_CHANNEL_0, gain);
}

void audio_adc_dig_gain_set(uint32_t gain)
{
    uint32_t reg = audio_dig_ctxt->reg7;

    reg &= ~(0x3F << 16);
    reg |= (gain & 0x3F) << 16;

    audio_dig_ctxt->reg7 = reg;
}

void audio_adc_enable(uint32_t enable)
{
    audio_adc_anc_enable(AUDIO_ADC_CHANNEL_0, enable);
}

void set_audio_dc_calib_data(uint16_t cali_data)
{
    audio_dig_ctxt->reg1 |= (MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
    REG_SYSTEM_0x56 &= 0xFFFF0000;
    sys_delay_cycle(6);
    REG_SYSTEM_0x56 |= cali_data;
    sys_delay_ms(10);
}

uint16_t get_audio_dc_calib_data(void)
{
    return REG_SYSTEM_0x56&0xffff;
}

void audio_dac_dc_calib_start(void)
{
    audio_dig_ctxt->reg1 |= (MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);

    REG_SYSTEM_0x57 |=  (1 << 21); //audio ldo1.0v enable
    sys_delay_cycle(6);
    REG_SYSTEM_0x57 |=  (1 << 23); //audio ldo1.5v enable
    REG_SYSTEM_0x56 |=  (1 << 23); //dac bias enable
    sys_delay_cycle(6);
    REG_SYSTEM_0x56 &= ~(1 << 24); //select low clock
    sys_delay_cycle(6);
    REG_SYSTEM_0x56 |=  (0 << 24); //0:low, 1:high
    REG_SYSTEM_0x55 &= ~(1 << 12); //select low gain
    sys_delay_cycle(6);
    REG_SYSTEM_0x55 |=  (1 << 12); //0:low, 1:high
    sys_delay_cycle(6);
    REG_SYSTEM_0x55 &= ~(1 << 19); //audio PA disable
    sys_delay_cycle(6);
    REG_SYSTEM_0x55 |=  (3 << 20); //dac L/R enable
    sys_delay_cycle(6);
    REG_SYSTEM_0x55 |=  (1 << 26); //dac mute
    sys_delay_cycle(6);
    REG_SYSTEM_0x55 |=  (3 << 15); //dac L/R dc offset calibration enable
    sys_delay_us(100);
    REG_SYSTEM_0x55 |=  (1 << 14); //calibration process enable
}

void audio_dac_dc_calib_finish(void)
{
	app_env_handle_t  env_h = app_env_get_handle();
    if(app_get_env_key_num_total() == 0)
    {
        uint32 dc_offet = (REG_SYSTEM_0x36 >> 5) & 0xFFFF;
        REG_SYSTEM_0x56 &= 0xFFFF0000;
        sys_delay_cycle(6);
        REG_SYSTEM_0x56 |= dc_offet;

        REG_SYSTEM_0x55 &= ~(1 << 14); //calibration process disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(3 << 15); //dac L/R dc offset calibration disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(3 << 20); //dac L/R disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(1 << 26);  // clear mute
        REG_SYSTEM_0x56 &= ~(1 << 23); //dac bias enable
        REG_SYSTEM_0x57 &= ~(1 << 21); //audio ldo1.0v disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x57 &= ~(1 << 23); //audio ldo1.5v disable
        LOG_I(APP,"  Audio.DC:%02x,%02x\r\n",dc_offet & 0xff,(dc_offet >> 8) & 0xff);
    }

    if(!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER))
    {
    	uint32_t reg;
        reg  = REG_SYSTEM_0x56;
        reg &= ~(0x03<<30);
        REG_SYSTEM_0x56 = reg | (0x02<<30);
        REG_SYSTEM_0x55 &= ~(3 << 18);
    }

    REG_SYSTEM_0x55 |= (1 << 26); 

    #if !CONFIG_AUDIO_DAC_ALWAYSOPEN
    audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
	#else
    audio_dac_keep_open(1, __func__);
	#endif    
}

void audio_dac_dc_calib_end(uint32 dc_offet)
{
	app_env_handle_t  env_h = app_env_get_handle();
    if(app_get_env_key_num_total() == 0)
    {
        //uint32 dc_offet = (REG_SYSTEM_0x36 >> 5) & 0xFFFF;
        REG_SYSTEM_0x56 &= 0xFFFF0000;
        sys_delay_cycle(6);
        REG_SYSTEM_0x56 |= dc_offet;

        REG_SYSTEM_0x55 &= ~(1 << 14); //calibration process disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(3 << 15); //dac L/R dc offset calibration disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(3 << 20); //dac L/R disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~(1 << 26);  // clear mute
        REG_SYSTEM_0x56 &= ~(1 << 23); //dac bias enable
        REG_SYSTEM_0x57 &= ~(1 << 21); //audio ldo1.0v disable
        sys_delay_cycle(6);
        REG_SYSTEM_0x57 &= ~(1 << 23); //audio ldo1.5v disable
        LOG_I(APP,"  Audio.DC:%02x,%02x\r\n",dc_offet & 0xff,(dc_offet >> 8) & 0xff);
    }

    if(!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER))
    {
    	uint32_t reg;
        reg  = REG_SYSTEM_0x56;
        reg &= ~(0x03<<30);
        REG_SYSTEM_0x56 = reg | (0x02<<30);
        REG_SYSTEM_0x55 &= ~(3 << 18);
    }

    REG_SYSTEM_0x55 |= (1 << 26); 

    #if !CONFIG_AUDIO_DAC_ALWAYSOPEN
    audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
	#else
    audio_dac_keep_open(1, __func__);
	#endif    
}

void audio_dac_dc_calib(uint32_t type)
{
    switch(type)
    {
    case AUDIO_DAC_CALIB_START:
        audio_dac_dc_calib_start();
        break;
    case AUDIO_DAC_CALIB_WAITING:
        sys_delay_ms(20);
        break;
    case AUDIO_DAC_CALIB_FINISH:
        audio_dac_dc_calib_finish();
        break;
    case AUDIO_DAC_CALIB_START_TO_FINISH:
    {
        uint32 dc_offset_L_result=0,dc_offset_R_result=0,dc_offset_L,dc_offset_R,i;
        uint32 dc_offset_Lmin=0xff,dc_offset_Lmax=0,dc_offset_Rmin=0xff,dc_offset_Rmax=0;
        audio_dac_dc_calib_start();
        for(i=0;i<5;i++)
        {
            sys_delay_ms(6);//3
            dc_offset_L = (REG_SYSTEM_0x36 >> 5) & 0xFF;
            dc_offset_R = (REG_SYSTEM_0x36 >> 13) & 0xFF;
            //INFO_PRT("dc_offset[%d]: L=%x,R=%x\r\n",i,dc_offset_L,dc_offset_R);
            REG_SYSTEM_0x55 &=  ~(1 << 14); //calibration disable
            sys_delay_cycle(100);
            REG_SYSTEM_0x55 |=  (1 << 14); //calibration enable

            if(dc_offset_L < dc_offset_Lmin)
                dc_offset_Lmin = dc_offset_L;
            if(dc_offset_L > dc_offset_Lmax)
                dc_offset_Lmax = dc_offset_L;

            if(dc_offset_R < dc_offset_Rmin)
                dc_offset_Rmin = dc_offset_R;
            if(dc_offset_R > dc_offset_Rmax)
                dc_offset_Rmax = dc_offset_R;

            dc_offset_L_result += dc_offset_L;
	     dc_offset_R_result += dc_offset_R;
        }

        dc_offset_L_result -= (dc_offset_Lmin + dc_offset_Lmax);
        dc_offset_R_result -= (dc_offset_Rmin + dc_offset_Rmax);

        dc_offset_L_result /= 3;
        dc_offset_R_result /= 3;

        INFO_PRT("dc_offset_cali_aver: L=%x, R=%x, %x\r\n",dc_offset_L_result,dc_offset_R_result,dc_offset_L_result|(dc_offset_R_result<<8));
        audio_dac_dc_calib_end(dc_offset_L_result|(dc_offset_R_result<<8));
        break;
    }
        break;
    default:
        break;
    }
}

void audio_dac_init(uint32_t mode)
{
    uint32_t reg;

    reg  = REG_SYSTEM_0x55;
    reg |= (1 << 0) | ((!mode) << 13);//13 means diff
    REG_SYSTEM_0x55 = reg;
    reg  = REG_SYSTEM_0x56;
    reg |= (1 << 20) | (1 << 21);
    REG_SYSTEM_0x56 = reg;

    if(mode==AUDIO_DAC_MODE_VCOM) 
    {
        //reg  = REG_SYSTEM_0x56;
        //reg &= ~(0x03<<30);
        //REG_SYSTEM_0x56 = reg | (0x02<<30);// will cause calibration unstable
        reg  = REG_SYSTEM_0x55;
        reg &= ~(0x1f<<6);
        REG_SYSTEM_0x55 = reg |(0x10<<6);
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 |= ((1<<0)|(1<<18));
        sys_delay_cycle(6);
        REG_SYSTEM_0x55 &= ~((1<<17)|(1<<13));	
    }

    if((app_get_env_key_num_total() == 0)&&(!app_get_env_aud_cali_valid()))    
    {        
        LOG_D(AUD,"Calibrate audio dc, %d,%x\r\n",app_get_env_key_num_total(),app_get_env_aud_cali_valid());  
        audio_dac_dc_calib(AUDIO_DAC_CALIB_START );  // AUDIO_DAC_CALIB_START_TO_FINISH
    }    
    else    
    {   
        if(app_get_env_aud_cali_valid())
        {
            env_aud_dc_offset_data_t* aud_cal = app_get_env_dc_offset_cali();
            uint16_t aud_offset=0;
            aud_offset = aud_cal->dac_l_dc_offset[0] | (aud_cal->dac_r_dc_offset[0]<<8);
            set_audio_dc_calib_data(aud_offset);
            LOG_I(AUD,"GET dc_offset from cp, cali:%04x\r\n",aud_offset);  
        }
        else
        {      
            app_env_handle_t  env_h = app_env_get_handle();  
            int key_index = app_get_active_linkey(1);
            if(key_index>0)  
            {     
                set_audio_dc_calib_data(env_h->env_data.key_pair[key_index-1].aud_dc_cali_data);        
            }   
            else   
            {       
                set_audio_dc_calib_data(env_h->env_data.key_pair[0].aud_dc_cali_data);            
                key_index = 1;
            }           
            LOG_I(AUD,"GET dc_offset from key index:%d, cali:%04x\r\n",key_index,env_h->env_data.key_pair[key_index-1].aud_dc_cali_data);              
        }
    }
    
}

void audio_dac_sample_rate_set(uint32_t samplerate)
{
    uint32_t d2, d3, sr;
    uint32_t lpf3_bypass = 0;
    uint32_t lpf4_bypass = 0;

    switch(samplerate)
    {
    case 8000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_12;
        sr = AUDIO_DAC_SRD_768;
        break;
    case 16000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_6;
        sr = AUDIO_DAC_SRD_384;
        break;
    case 32000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_3;
        sr = AUDIO_DAC_SRD_192;
        break;
    case 64000:
        d2 = AUDIO_DAC_CIC1_D2_8;
        d3 = AUDIO_DAC_CIC2_D3_3;
        sr = AUDIO_DAC_SRD_96;
        break;
    case 128000:
        d2 = AUDIO_DAC_CIC1_D2_8;
        d3 = AUDIO_DAC_CIC2_D3_3;
        sr = AUDIO_DAC_SRD_48;
        lpf3_bypass = 1;
        break;
    case 256000:
        d2 = AUDIO_DAC_CIC1_D2_8;
        d3 = AUDIO_DAC_CIC2_D3_3;
        sr = AUDIO_DAC_SRD_24;
        lpf3_bypass = 1;
        lpf4_bypass = 1;
        break;
    case 11025:
    case 12000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_8;
        sr = AUDIO_DAC_SRD_512;
        break;
    case 22050:
    case 24000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_4;
        sr = AUDIO_DAC_SRD_256;
        break;
    case 44100:
    case 48000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_2;
        sr = AUDIO_DAC_SRD_128;
        break;
    case 88200:
    case 96000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_1;
        sr = AUDIO_DAC_SRD_64;
        break;
    case 176400:
    case 192000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_1;
        sr = AUDIO_DAC_SRD_32;
        lpf3_bypass = 1;
        break;
    case 352800:
    case 384000:
        d2 = AUDIO_DAC_CIC1_D2_16;
        d3 = AUDIO_DAC_CIC2_D3_1;
        sr = AUDIO_DAC_SRD_16;
        lpf3_bypass = 1;
        lpf4_bypass = 1;
        break;
    default:
        return;
    }

    audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_TX_ANC_D2 |
                              MSK_ANC_0x01_TX_ANC_D3_BIT20 | MSK_ANC_0x01_RX_ANC_D3_BIT3 |
                              MSK_ANC_0x01_SAMP_RATE_BIT20 | MSK_ANC_0x01_SAMP_RATE_BIT3 |
                              MSK_ANC_0x01_LPF3_ANC_BPS | MSK_ANC_0x01_LPF4_ANC_BPS);
    audio_dig_ctxt->reg1 |= (d2 << SFT_ANC_0x01_TX_ANC_D2) |
                            ((d3 & 7) << SFT_ANC_0x01_TX_ANC_D3_BIT20) | ((d3 >> 3) << SFT_ANC_0x01_RX_ANC_D3_BIT3) |
                            ((sr & 7) << SFT_ANC_0x01_SAMP_RATE_BIT20) | ((sr >> 3) << SFT_ANC_0x01_SAMP_RATE_BIT3) |
                            (lpf3_bypass << SFT_ANC_0x01_LPF3_ANC_BPS) | (lpf4_bypass << SFT_ANC_0x01_LPF4_ANC_BPS);
}

void audio_dac_ana_gain_set(uint32_t gain)
{
    uint32_t reg = REG_SYSTEM_0x55;

    reg &= ~(7 << 22);
    reg |= (gain & 7) << 22;

    REG_SYSTEM_0x55 = reg;
}
void audio_dac_ana_mute(uint32_t enable)
{
    os_printf("Mute:%d\r\n",enable);
    if(enable)
    {
        REG_SYSTEM_0x55 |= (1 << 26);// dac mute
    }
    else
    {
        REG_SYSTEM_0x55 &= ~(1 << 26);// dac don't mute    
    }
}
static uint8_t s_dac_ana_opened = 0;
#if 0
void audio_dac_ana_enable(uint32_t enable)
{
    uint8_t is_changed = enable ^ s_dac_ana_opened;
    if(!is_changed) return;
    if(enable)
    {
        s_dac_ana_opened = 1;
        sys_delay_us(100);
        REG_SYSTEM_0x56 &= ~(1 << 26);// reset
        REG_SYSTEM_0x56 |= (1 << 25);  // bypass DWA;03/05/2020,@xiasiqing
        REG_SYSTEM_0x57 |= (1 << 23);  // audio ldo1.5v
        REG_SYSTEM_0x55 |= (1 << 19);  // dac            
        REG_SYSTEM_0x55 |= (1 << 20); // dac R        
        sys_delay_ms(4);        
        REG_SYSTEM_0x55 |= (1 << 21);// dac L  : only L don't have POP, R still have
    }
    else
    {
        s_dac_ana_opened = 0;
        REG_SYSTEM_0x55 &= ~(1 << 19);
        sys_delay_us(100);
        REG_SYSTEM_0x55 &= ~(3 << 20);
        REG_SYSTEM_0x57 &= ~(1 << 23);
        REG_SYSTEM_0x56 |= (1 << 26);
    }    
}
#endif
void audio_dac_dig_enable(uint32_t enable)
{
    if(enable)
    {
        audio_dig_ctxt->reg1 |= (MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
    }
    else
    {
        #if !CONFIG_AUDIO_DAC_ALWAYSOPEN
        audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
		#else
		audio_dac_keep_open(0, __func__);
		#endif
    }    
}
void audio_dac_enable(uint32_t enable)
{
    uint8_t is_changed = enable ^ s_dac_ana_opened;
    app_env_handle_t  env_h = app_env_get_handle();

    if(!is_changed) return;

    LOG_I(AUD,"DAC.en:%d\r\n",enable);
    if(enable)
    {
        s_dac_ana_opened = 1;

        #if 1 //Fill zeros pcm data
        {
            uint32_t cnt1 = 63;
            uint32_t cnt2 = 63;

            while(cnt1 && cnt2)
            {
                if(cnt1 && !(REG_ANC_0x40 & MSK_ANC_0x40_TX1_NFULL))
                {
                    audio_dig_ctxt->dac_tx1_dout0 = 1;
                    audio_dig_ctxt->dac_tx1_dout0 = 1;
                    cnt1--;
                }

                if(cnt2 && !(REG_ANC_0x40 & MSK_ANC_0x40_TX2_NFULL))
                {
                    audio_dig_ctxt->dac_tx2_dout0 = 1;
                    audio_dig_ctxt->dac_tx2_dout0 = 1;
                    cnt2--;
                }
            }

            audio_dig_ctxt->dac_tx1_dout0 = 0;
            audio_dig_ctxt->dac_tx1_dout0 = 0;
            audio_dig_ctxt->dac_tx2_dout0 = 0;
            audio_dig_ctxt->dac_tx2_dout0 = 0;
        }
        #endif

        //sys_delay_us(100);
        REG_SYSTEM_0x57 |= (1 << 21) | (1 << 23); //audio ldo1.0 & ldo1.5 enable
        REG_SYSTEM_0x55 |= (3 << 15) | (1 << 19); //audio DCOC & PA enable
        REG_SYSTEM_0x55 &= ~(1 << 26);
        REG_SYSTEM_0x56 |= (1 << 23); //audio DAC bias enable
        sys_delay_ms(1);

        //REG_SYSTEM_0x55 |= (1 << 26);

        if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER)
        {
            if(!app_check_bt_mode(BT_MODE_TWS|BT_MODE_BLE|BT_MODE_DM_TWS))
            {
                REG_SYSTEM_0x55 |= (3 << 20); //for differ mode dac left & right channel enable
            }
            else
            {
                if(app_check_bt_mode(BT_MODE_BLE))
                    REG_SYSTEM_0x55 &=~ (3 << 20); //not need open dac
                else
                    REG_SYSTEM_0x55 |= (2 << 20); //dac left channel enable
            }
        }
        else
        {
            //REG_SYSTEM_0x57 |= (1 << 23);
            //sys_delay_cycle(6);
            //REG_SYSTEM_0x57 |= (1 << 21);
            //sys_delay_us(100);
            REG_SYSTEM_0x56 |= (1 << 23);
            REG_SYSTEM_0x55 |= (1 << 19);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 |= (1 << 18);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 |= (1 << 20);
            sys_delay_ms(1);
            REG_SYSTEM_0x55 &= ~(1 << 18);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 &= ~(1 << 20);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 |= (1 << 17);
            sys_delay_us(100);
            REG_SYSTEM_0x55 |= (1 << 21);
            sys_delay_cycle(100);
            REG_SYSTEM_0x55 &= ~(1 << 17);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 |= (1 << 20);
            sys_delay_cycle(100);
            REG_SYSTEM_0x55 |= (1 << 18);            
        }

        audio_dig_ctxt->reg1 |= (MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
    }
    else
    {
        s_dac_ana_opened = 0;

        audio_dac_fadeout(3);

        os_delay_ms(10);

		#if !CONFIG_AUDIO_DAC_ALWAYSOPEN
        if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER)
        {
            REG_SYSTEM_0x55 &= ~(1 << 19);
            sys_delay_us(100);
            REG_SYSTEM_0x55 &= ~((3 << 15) | (3 << 20)); //audio DCOC & channels disable
            if(!(audio_dig_ctxt->reg0 & MSK_ANC_0x00_ADC_ENABLE)) REG_SYSTEM_0x57 &= ~((1 << 21) | (1 << 23));
            REG_SYSTEM_0x56 &= ~(1 << 23);//DAC bias disable
        }
        else
        {
            REG_SYSTEM_0x55 &= ~(3 << 20);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 &= ~(1 << 19);
            sys_delay_cycle(6);
            REG_SYSTEM_0x55 &= ~(1 << 18);
            if(!(audio_dig_ctxt->reg0 & MSK_ANC_0x00_ADC_ENABLE)) REG_SYSTEM_0x57 &= ~((1 << 21) | (1 << 23));
            REG_SYSTEM_0x56 &= ~(1 << 23);
        }
        audio_dig_ctxt->reg1 &= ~(MSK_ANC_0x01_DAC_ENABLE | MSK_ANC_0x01_TX_CHN_EN);
		#else
		audio_dac_keep_open(0, __func__);
		#endif
    }
}

void audio_dac_fadeout(uint32_t channels)
{
    #define FADE_OUT_CNT    1000

    int32_t cnt0, cnt1, cnt2, cnt3;
    int32_t pcm0, pcm1, pcm2, pcm3;
    int32_t delta0, delta1, delta2, delta3;

    #if 0
    uint32_t curr = dma_channel_src_curr_pointer_get(audio_dac_ctrl_blk.aud_rb.dma) - (uint32_t)audio_dac_ctrl_blk.aud_rb.address;
    pcm0 = *(int32_t*)(&audio_dac_ctrl_blk.aud_rb.address[(curr - 8 + audio_dac_ctrl_blk.aud_rb.capacity) % audio_dac_ctrl_blk.aud_rb.capacity]);
    pcm1 = *(int32_t*)(&audio_dac_ctrl_blk.aud_rb.address[(curr - 4 + audio_dac_ctrl_blk.aud_rb.capacity) % audio_dac_ctrl_blk.aud_rb.capacity]);
    pcm2 = ((int32_t)REG_ANC_0x45 << 8) >> 8;
    pcm3 = ((int32_t)REG_ANC_0x46 << 8) >> 8;
    #else
    pcm0 = ((int32_t)REG_ANC_0x4E << 8) >> 8;
    pcm1 = ((int32_t)REG_ANC_0x4F << 8) >> 8;
    pcm2 = ((int32_t)REG_ANC_0x45 << 8) >> 8;
    pcm3 = ((int32_t)REG_ANC_0x46 << 8) >> 8;
    #endif

    delta0 = pcm0 / FADE_OUT_CNT;
    delta1 = pcm1 / FADE_OUT_CNT;
    delta2 = pcm2 / FADE_OUT_CNT;
    delta3 = pcm3 / FADE_OUT_CNT;

    if(delta0 == 0) delta0 = pcm0 > 0 ? 1 : -1;
    if(delta1 == 0) delta1 = pcm1 > 0 ? 1 : -1;
    if(delta2 == 0) delta2 = pcm2 > 0 ? 1 : -1;
    if(delta3 == 0) delta3 = pcm3 > 0 ? 1 : -1;

    cnt0 = (channels & 0x1) ? pcm0 / delta0 : 0;
    cnt1 = (channels & 0x1) ? pcm1 / delta1 : 0;
    cnt2 = (channels & 0x2) ? pcm2 / delta2 : 0;
    cnt3 = (channels & 0x2) ? pcm3 / delta3 : 0;

    while(cnt0 || cnt1 || cnt2 || cnt3)
    {
        if((channels & 0x1) && !(REG_ANC_0x40 & MSK_ANC_0x40_TX1_NFULL))
        {
            if(cnt0) { cnt0--; pcm0 -= delta0; };
            if(cnt1) { cnt1--; pcm1 -= delta1; };

            REG_ANC_0x4E = pcm0;
            REG_ANC_0x4E = pcm1;
        }

        if((channels & 0x2) && !(REG_ANC_0x40 & MSK_ANC_0x40_TX2_NFULL))
        {
            if(cnt2) { cnt2--; pcm2 -= delta2; };
            if(cnt3) { cnt3--; pcm3 -= delta3; };

            REG_ANC_0x45 = pcm2;
            REG_ANC_0x45 = pcm3;
        }
    }
}

void audio_anc_init(uint32_t bits, uint32_t mode)
{
    audio_adc_anc_init(AUDIO_ANC_CHANNEL_0, bits, mode);
    audio_adc_anc_init(AUDIO_ANC_CHANNEL_1, bits, mode);
    audio_adc_anc_init(AUDIO_ANC_CHANNEL_2, bits, mode);
    audio_adc_anc_init(AUDIO_ANC_CHANNEL_3, bits, mode);
}

void audio_anc_sample_rate_set(uint32_t samplerate)
{
    audio_adc_anc_sample_rate_set(1, samplerate);
}

void audio_anc_ana_gain_set(uint32_t gain)
{
    audio_adc_anc_ana_gain_set(AUDIO_ANC_CHANNEL_0, gain);
    audio_adc_anc_ana_gain_set(AUDIO_ANC_CHANNEL_1, gain);
    audio_adc_anc_ana_gain_set(AUDIO_ANC_CHANNEL_2, gain);
    audio_adc_anc_ana_gain_set(AUDIO_ANC_CHANNEL_3, gain);
}

void audio_anc_dig_gain_set(uint32_t gain)
{
    uint32_t reg = audio_dig_ctxt->reg7;

    reg &= ~(0x3F << 24);
    reg |= (gain & 0x3F) << 24;

    audio_dig_ctxt->reg7 = reg;
}

void audio_anc_enable_ext(uint32_t channels, uint32_t enable)
{
    uint32_t ch;

    for(ch = 1; ch < 5; ch++)
    {
        if(channels & (1 << ch))
        {
            uint32_t* reg = (uint32_t*)&REG_SYSTEM_0x50 + ch;
            uint32_t  val = *reg;

            val  = enable ? val | (1 << 28) : val & ~(1 << 28);
            *reg = val | (1 << 29);
            sys_delay_ms(1);
            *reg = val;
        }
    }

    if(enable)
    {
        REG_SYSTEM_0x4F |= (1 << 5);
        REG_SYSTEM_0x57 |= (1 << 21) | (1 << 23);

        if(!(REG_SYSTEM_0x51 & (3 << 19))) REG_SYSTEM_0x57 |= (1 << 24);

        audio_dig_ctxt->reg0 |= (MSK_ANC_0x00_ADC_ENABLE | (0xF << 2));
    }
    else
    {
        uint32_t en = !!(audio_dig_ctxt->reg0 & MSK_ANC_0x00_RX_CHN_EN);

        if(!en)
        {
            REG_SYSTEM_0x4F &= ~(1 << 5);
            REG_SYSTEM_0x57 &= ~(1 << 24);
            if(!(audio_dig_ctxt->reg1 & MSK_ANC_0x01_DAC_ENABLE)) REG_SYSTEM_0x57 &= ~((1 << 21) | (1 << 23));
        }

        audio_dig_ctxt->reg0 &= ~((!en) | (0xF << 2));
    }
}

void audio_anc_enable(uint32_t enable)
{
    audio_anc_enable_ext(AUDIO_ANC_CHANNEL_0 | AUDIO_ANC_CHANNEL_1 | AUDIO_ANC_CHANNEL_2 | AUDIO_ANC_CHANNEL_3, enable);
}

int32_t audio_ctrl(uint32_t cmd, uint32_t arg)
{
    uint32_t reg;

    switch(cmd)
    {
    case AUDIO_CTRL_CMD_FIR_MEM_CLEAR:
        return -1;
    case AUDIO_CTRL_CMD_STATUS_GET:
        *(uint32_t*)arg = REG_ANC_0x40;
        break;
    case AUDIO_CTRL_CMD_ADC_ENABLE:
        reg  = REG_ANC_0x00;
        reg &= ~(MSK_ANC_0x00_ADC_ENABLE | MSK_ANC_0x00_RX_CHN_EN);
        reg |= (!!arg) << SFT_ANC_0x00_RX_CHN_EN;
        reg |= (!!(reg & (0x1F << SFT_ANC_0x00_RX_CHN_EN))) << SFT_ANC_0x00_ADC_ENABLE;
        REG_ANC_0x00 = reg;
        break;
    case AUDIO_CTRL_CMD_SWITCH_TO_MIC:
        if(arg < 5)
        {
            uint32_t* ptr = (uint32_t*)&REG_SYSTEM_0x50 + arg;

            reg  = *ptr;
            reg &= ~(3 << 26);
            reg |=  (1 << 28);
            *ptr = reg;
        }
        break;
    case AUDIO_CTRL_CMD_SWITCH_TO_DACL:
        if(arg < 5)
        {
            uint32_t* ptr = (uint32_t*)&REG_SYSTEM_0x50 + arg;

            reg  = *ptr;
            reg &= ~(7 << 26);
            reg |=  (1 << 27);
            *ptr = reg;
        }
        break;
    case AUDIO_CTRL_CMD_SWITCH_TO_DACR:
        if(arg < 5)
        {
            uint32_t* ptr = (uint32_t*)&REG_SYSTEM_0x50 + arg;

            reg  = *ptr;
            reg &= ~(7 << 26);
            reg |=  (1 << 26);
            *ptr = reg;
        }
        break;
    case AUDIO_CTRL_CMD_SWITCH_TO_DACLR:
        if(arg < 5)
        {
            uint32_t* ptr = (uint32_t*)&REG_SYSTEM_0x50 + arg;

            reg  = *ptr;
            reg &= ~(7 << 26);
            reg |=  (3 << 26);
            *ptr = reg;
        }
        break;
    case AUDIO_CTRL_CMD_SWITCH_TO_LINEIN:
        if(arg)
        {
            gpio_config_new(GPIO4, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);// REG_GPIO_0x04 = 8;
            gpio_config_new(GPIO5, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);// REG_GPIO_0x05 = 8;
            gpio_config_new(GPIO2, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);// REG_GPIO_0x02 = 8;	// yuan++
            gpio_config_new(GPIO3, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);// REG_GPIO_0x03 = 8;	// 未使用不能配置高阻, 否則干擾LineIn會有底噪
        }
        else
        {
            gpio_config_new(GPIO2, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);// REG_GPIO_0x02 = 8;
            gpio_config_new(GPIO3, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);// REG_GPIO_0x03 = 8;
            gpio_config_new(GPIO4, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);// REG_GPIO_0x02 = 8;
            gpio_config_new(GPIO5, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);// REG_GPIO_0x03 = 8;
        }
        reg  = REG_SYSTEM_0x4F;
        reg &= ~3;
        reg |= 1 << (!!arg);
        REG_SYSTEM_0x4F  = reg;
        REG_SYSTEM_0x51 &= ~(7 << 28);
        REG_SYSTEM_0x52 &= ~(7 << 28);
        // REG_SYSTEM_0x53 &= ~(7 << 28);
        // REG_SYSTEM_0x54 &= ~(7 << 28);
        break;
    case AUDIO_CTRL_CMD_ADC_ANC_INIT:
        audio_adc_anc_init(arg & 0xFF, (arg >> 8) & 0xFF, (arg >> 16) & 0xFF);
        break;
    case AUDIO_CTRL_CMD_ADC_ANC_ENABLE:
        audio_adc_anc_enable(arg & 0xFF, (arg >> 8) & 0xFF);
        break;
    case AUDIO_CTRL_CMD_ADC_ANC_SAMPLE_RATE_SET:
        audio_adc_anc_sample_rate_set(arg & 0xFF, (arg >> 8) & 0xFFFFFF);
        break;
    case AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET:
        audio_adc_anc_ana_gain_set(arg & 0xFF, (arg >> 8) & 0xFF);
        break;
    case AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_GET:
        *(uint32_t*)arg = audio_adc_anc_ana_gain_get(*(uint32_t*)arg);
        break;
    case AUDIO_CTRL_CMD_ADC_INT_ENABLE:
        reg  = REG_ANC_0x05;
        reg &= ~MSK_ANC_0x05_RXCHN_INT_EN;
        reg |= (!!arg) << SFT_ANC_0x05_RXCHN_INT_EN;
        REG_ANC_0x05 = reg;
        break;
    case AUDIO_CTRL_CMD_ADC_2FS_INT_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_ADC_CIC1_D1_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF12_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF1_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF2_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF1_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF2_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF1_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_LPF2_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_SAMPLE_EDGE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_DIG_MIC0_ENABLE:
        reg  = REG_ANC_0x00;
        reg &= ~MSK_ANC_0x00_DIGMIC1_EN;
        reg |= (!!arg << SFT_ANC_0x00_DIGMIC1_EN);
        REG_ANC_0x00 = reg;
        break;
    case AUDIO_CTRL_CMD_ADC_DIG_MIC1_ENABLE:
        reg  = REG_ANC_0x00;
        reg &= ~MSK_ANC_0x00_DIGMIC2_EN;
        reg |= (!!arg << SFT_ANC_0x00_DIGMIC2_EN);
        REG_ANC_0x00 = reg;
        break;
    case AUDIO_CTRL_CMD_ADC_SAMPLE_WIDTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_FIFO_RDTH_SET:
        reg  = REG_ANC_0x02;
        reg &= ~MSK_ANC_0x02_RX_CHN_RTHRD;
        reg |= (arg & MAX_ANC_0x02_RX_CHN_RTHRD) << SFT_ANC_0x02_RX_CHN_RTHRD;
        REG_ANC_0x02 = reg;
        break;
    case AUDIO_CTRL_CMD_ADC_FIFO_WRTH_SET:
        reg  = REG_ANC_0x02;
        reg &= ~MSK_ANC_0x02_RX_CHN_WTHRD;
        reg |= (arg & MAX_ANC_0x02_RX_CHN_WTHRD) << SFT_ANC_0x02_RX_CHN_WTHRD;
        REG_ANC_0x02 = reg;
        break;
    case AUDIO_CTRL_CMD_ADC_2FS_FIFO_RDTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_2FS_FIFO_WRTH_SET:
        break;
    case AUDIO_CTRL_CMD_ADC_GAIN_SET:
        return -1;
    case AUDIO_CTRL_CMD_ADC_FIFO_READ:
        *(uint32_t*)arg = REG_ANC_0x47;
        break;
    case AUDIO_CTRL_CMD_ANC_ENABLE:
        reg  = REG_ANC_0x00;
        reg &= ~(MSK_ANC_0x00_ADC_ENABLE | (0xF << SFT_ANC_0x00_RX_ANC0_EN));
        reg |= (arg & 0xF) << SFT_ANC_0x00_RX_ANC0_EN;
        reg |= (!!(reg & (0x1F << SFT_ANC_0x00_RX_CHN_EN))) << SFT_ANC_0x00_ADC_ENABLE;
        REG_ANC_0x00 = reg;
        break;
    case AUDIO_CTRL_CMD_ANC_INT_ENABLE:
        reg  = REG_ANC_0x05;
        reg &= ~MSK_ANC_0x05_RXANC_INT_EN;
        reg |= (!!arg) << SFT_ANC_0x05_RXANC_INT_EN;
        REG_ANC_0x05 = reg;
        break;
    case AUDIO_CTRL_CMD_ANC_CLK_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_ANC_CIC1_D1_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF12_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF1_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF2_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF1_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF2_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF1_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_LPF2_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_SAMPLE_WIDTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_FIFO_RDTH_SET:
        reg  = REG_ANC_0x02;
        reg &= ~MSK_ANC_0x02_RX_ANC_RTHRD;
        reg |= (arg & MAX_ANC_0x02_RX_ANC_RTHRD) << SFT_ANC_0x02_RX_ANC_RTHRD;
        REG_ANC_0x02 = reg;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_WRTH_SET:
        reg  = REG_ANC_0x02;
        reg &= ~MSK_ANC_0x02_RX_ANC_WTHRD;
        reg |= (arg & MAX_ANC_0x02_RX_ANC_WTHRD) << SFT_ANC_0x02_RX_ANC_WTHRD;
        REG_ANC_0x02 = reg;
        break;
    case AUDIO_CTRL_CMD_ANC_GAIN_SET:
        return -1;
    case AUDIO_CTRL_CMD_ANC_FIFO_DATA_MODE_SET:
        reg  = REG_ANC_0x06;
        reg &= ~MSK_ANC_0x06_DIS_ADC_INTEAV;
        reg |= (!!arg) << SFT_ANC_0x06_DIS_ADC_INTEAV;
        REG_ANC_0x06 = reg;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_READ:
        *(uint32_t*)arg = REG_ANC_0x48;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_READ4x16:
        ((uint16_t*)arg)[0] = REG_ANC_0x48;
        ((uint16_t*)arg)[1] = REG_ANC_0x48;
        ((uint16_t*)arg)[2] = REG_ANC_0x48;
        ((uint16_t*)arg)[3] = REG_ANC_0x48;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_READ4x32:
        ((uint32_t*)arg)[0] = REG_ANC_0x48;
        ((uint32_t*)arg)[1] = REG_ANC_0x48;
        ((uint32_t*)arg)[2] = REG_ANC_0x48;
        ((uint32_t*)arg)[3] = REG_ANC_0x48;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_CH0_READ:
        *(uint32_t*)arg = REG_ANC_0x41;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_CH1_READ:
        *(uint32_t*)arg = REG_ANC_0x42;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_CH2_READ:
        *(uint32_t*)arg = REG_ANC_0x43;
        break;
    case AUDIO_CTRL_CMD_ANC_FIFO_CH3_READ:
        *(uint32_t*)arg = REG_ANC_0x44;
        break;
    case AUDIO_CTRL_CMD_ANC_DMIC_ENABLE:
        reg  = REG_ANC_0x00;
        reg &= ~(MSK_ANC_0x00_DIGMIC1_EN | MSK_ANC_0x00_DIGMIC2_EN);
        reg |= (arg & 0x3) << SFT_ANC_0x00_DIGMIC1_EN;
        REG_ANC_0x00 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_ENABLE:
        reg  = REG_ANC_0x01;
        reg &= ~MSK_ANC_0x01_DAC_ENABLE;
        reg |= (!!arg) << SFT_ANC_0x01_DAC_ENABLE;
        REG_ANC_0x01 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX1_INT_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_2FS_INT_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_LPF_INT_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX2_INT_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_DAC_ADDER_MODE_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_SAMPLE_RATE_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_CIC1_D2_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_CIC2_D3_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_CIC2_BYPASS:
        reg  = REG_ANC_0x01;
        reg &= ~MSK_ANC_0x01_TX_CIC2_BPS;
        reg |= (!!arg) << SFT_ANC_0x01_TX_CIC2_BPS;
        REG_ANC_0x01 = reg;
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF34_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF3_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF4_BYPASS:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF3_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF4_FIR_ORDER_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF3_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_LPF4_IIR_TYPE_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_PN_CONFIG:
        return -1;
    case AUDIO_CTRL_CMD_DAC_NOTCH_ENABLE:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_FIFO_RDTH_SET:
        reg  = REG_ANC_0x03;
        reg &= ~MSK_ANC_0x03_TX1_ANC_RTHRD;
        reg |= (arg & MAX_ANC_0x03_TX1_ANC_RTHRD) << SFT_ANC_0x03_TX1_ANC_RTHRD;
        REG_ANC_0x03 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX1_FIFO_WRTH_SET:
        reg  = REG_ANC_0x03;
        reg &= ~MSK_ANC_0x03_TX1_ANC_WTHRD;
        reg |= (arg & MAX_ANC_0x03_TX1_ANC_WTHRD) << SFT_ANC_0x03_TX1_ANC_WTHRD;
        REG_ANC_0x03 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX1_2FS_FIFO_RDTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_2FS_FIFO_WRTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_LPF_FIFO_RDTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX1_LPF_FIFO_WRTH_SET:
        return -1;
    case AUDIO_CTRL_CMD_DAC_TX2_FIFO_RDTH_SET:
        reg  = REG_ANC_0x04;
        reg &= ~MSK_ANC_0x04_TX2_RTHRD;
        reg |= (arg & MAX_ANC_0x04_TX2_RTHRD) << SFT_ANC_0x04_TX2_RTHRD;
        REG_ANC_0x04 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX2_FIFO_WRTH_SET:
        reg  = REG_ANC_0x04;
        reg &= ~MSK_ANC_0x04_TX2_WTHRD;
        reg |= (arg & MAX_ANC_0x04_TX2_WTHRD) << SFT_ANC_0x04_TX2_WTHRD;
        REG_ANC_0x04 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_CH0_LOOP_SRC_SEL:
        return -1;
    case AUDIO_CTRL_CMD_DAC_CH1_LOOP_SRC_SEL:
        return -1;
    case AUDIO_CTRL_CMD_DAC_FIFO_DATA_MODE_SET:
        reg  = REG_ANC_0x06;
        reg &= ~MSK_ANC_0x06_DIS_DAC_INTEAV;
        reg |= (!!arg) << SFT_ANC_0x06_DIS_DAC_INTEAV;
        REG_ANC_0x06 = reg;
        break;
    case AUDIO_CTRL_CMD_DAC_FIFO_CH0_WRITE:
    case AUDIO_CTRL_CMD_DAC_TX1_FIFO_CH0_WRITE:
        REG_ANC_0x4E = arg;
        break;
    case AUDIO_CTRL_CMD_DAC_FIFO_CH1_WRITE:
    case AUDIO_CTRL_CMD_DAC_TX1_FIFO_CH1_WRITE:
        REG_ANC_0x4F = arg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX2_FIFO_CH0_WRITE:
        REG_ANC_0x45 = arg;
        break;
    case AUDIO_CTRL_CMD_DAC_TX2_FIFO_CH1_WRITE:
        REG_ANC_0x46 = arg;
        break;
    default:
        return -1;
    }

    return 0;
}
