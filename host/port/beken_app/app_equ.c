#include <math.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"

#if (CONFIG_AUD_EQS_SUPPORT==1)//by beken zb

//EQ 1
#define TAB1_TotalEQnum 5
#define TAB1_EQGAIN 3

#define TAB1_EQ0A0 -1972996
#define TAB1_EQ0A1 936999
#define TAB1_EQ0B0 1074196
#define TAB1_EQ0B1 -1966879
#define TAB1_EQ0B2 917495

#define TAB1_EQ1A0 -1675702
#define TAB1_EQ1A1 738322
#define TAB1_EQ1B0 1062469
#define TAB1_EQ1B1 -1668196
#define TAB1_EQ1B2 731935

#define TAB1_EQ2A0 -1216723
#define TAB1_EQ2A1 538819
#define TAB1_EQ2B0 983084
#define TAB1_EQ2B1 -1269332
#define TAB1_EQ2B2 551701

#define TAB1_EQ3A0 -861351
#define TAB1_EQ3A1 440959
#define TAB1_EQ3B0 1234504
#define TAB1_EQ3B1 -660147
#define TAB1_EQ3B2 456235

#define TAB1_EQ4A0 112655
#define TAB1_EQ4A1 -351039
#define TAB1_EQ4B0 850529
#define TAB1_EQ4B1 149624
#define TAB1_EQ4B2 -286804

//EQ2
#define TAB2_TotalEQnum 5
#define TAB2_EQGAIN -2

#define TAB2_EQ0A0 -2033449
#define TAB2_EQ0A1 988428
#define TAB2_EQ0B0 1045285
#define TAB2_EQ0B1 -2033798
#define TAB2_EQ0B2 991369

#define TAB2_EQ1A0 -1764286
#define TAB2_EQ1A1 789963
#define TAB2_EQ1B0 1110313
#define TAB2_EQ1B1 -1733071
#define TAB2_EQ1B2 759440

#define TAB2_EQ2A0 -1312606
#define TAB2_EQ2A1 572732
#define TAB2_EQ2B0 1266296
#define TAB2_EQ2B1 -1117406
#define TAB2_EQ2B2 550212

#define TAB2_EQ3A0 -611466
#define TAB2_EQ3A1 394830
#define TAB2_EQ3B0 1074937
#define TAB2_EQ3B1 -581716
#define TAB2_EQ3B2 398218

#define TAB2_EQ4A0 602306
#define TAB2_EQ4A1 393461
#define TAB2_EQ4B0 782275
#define TAB2_EQ4B1 210400
#define TAB2_EQ4B2 267856

//EQ3
#define TAB3_TotalEQnum 5
#define TAB3_EQGAIN -3

#define TAB3_EQ0A0 -2082978
#define TAB3_EQ0A1 1034453
#define TAB3_EQ0B0 1051646
#define TAB3_EQ0B1 -2082951
#define TAB3_EQ0B2 1031409

#define TAB3_EQ1A0 -2043965
#define TAB3_EQ1A1 996660
#define TAB3_EQ1B0 1044892
#define TAB3_EQ1B1 -2044129
#define TAB3_EQ1B2 1000180

#define TAB3_EQ2A0 -1670349
#define TAB3_EQ2A1 667006
#define TAB3_EQ2B0 1113044
#define TAB3_EQ2B1 -1654896
#define TAB3_EQ2B2 617992

#define TAB3_EQ3A0 494878
#define TAB3_EQ3A1 -65555
#define TAB3_EQ3B0 1036792
#define TAB3_EQ3B1 481467
#define TAB3_EQ3B2 -67182

#define TAB3_EQ4A0 602306
#define TAB3_EQ4A1 393461
#define TAB3_EQ4B0 782275
#define TAB3_EQ4B1 210400
#define TAB3_EQ4B2 267856

const static int32 eq1_tab_arr[TAB1_TotalEQnum][5]=
							   {{-TAB1_EQ0A0, -TAB1_EQ0A1,TAB1_EQ0B0,TAB1_EQ0B1,TAB1_EQ0B2},
							     {-TAB1_EQ1A0, -TAB1_EQ1A1,TAB1_EQ1B0,TAB1_EQ1B1,TAB1_EQ1B2},
							     {-TAB1_EQ2A0, -TAB1_EQ2A1,TAB1_EQ2B0,TAB1_EQ2B1,TAB1_EQ2B2},
							     {-TAB1_EQ3A0, -TAB1_EQ3A1,TAB1_EQ3B0,TAB1_EQ3B1,TAB1_EQ3B2},
							     {-TAB1_EQ4A0, -TAB1_EQ4A1,TAB1_EQ4B0,TAB1_EQ4B1,TAB1_EQ4B2}};
const static int32 eq2_tab_arr[TAB2_TotalEQnum][5]=
							   {{-TAB2_EQ0A0, -TAB2_EQ0A1,TAB2_EQ0B0,TAB2_EQ0B1,TAB2_EQ0B2},
							     {-TAB2_EQ1A0, -TAB2_EQ1A1,TAB2_EQ1B0,TAB2_EQ1B1,TAB2_EQ1B2},
							     {-TAB2_EQ2A0, -TAB2_EQ2A1,TAB2_EQ2B0,TAB2_EQ2B1,TAB2_EQ2B2},
							     {-TAB2_EQ3A0, -TAB2_EQ3A1,TAB2_EQ3B0,TAB2_EQ3B1,TAB2_EQ3B2},
							     {-TAB2_EQ4A0, -TAB2_EQ4A1,TAB2_EQ4B0,TAB2_EQ4B1,TAB2_EQ4B2}};
const static int32 eq3_tab_arr[TAB3_TotalEQnum][5]=
							   {{-TAB3_EQ0A0, -TAB3_EQ0A1,TAB3_EQ0B0,TAB3_EQ0B1,TAB3_EQ0B2},
							     {-TAB3_EQ1A0, -TAB3_EQ1A1,TAB3_EQ1B0,TAB3_EQ1B1,TAB3_EQ1B2},
							     {-TAB3_EQ2A0, -TAB3_EQ2A1,TAB3_EQ2B0,TAB3_EQ2B1,TAB3_EQ2B2},
							     {-TAB3_EQ3A0, -TAB3_EQ3A1,TAB3_EQ3B0,TAB3_EQ3B1,TAB3_EQ3B2},
							     {-TAB3_EQ4A0, -TAB3_EQ4A1,TAB3_EQ4B0,TAB3_EQ4B1,TAB3_EQ4B2}};
app_aud_eq_t app_aud_eq[APP_EQ_NUM];

void app_aud_eq_init(void)
{
    uint8_t i;
    uint8_t j;
    for( i = 0; i < APP_EQ_NUM; i++ )
    {
        app_aud_eq[i].eq_enable=0;
    }

    for(i=0;i<APP_EQ_NUM;i++)
    {
        if(i==0)	
        {
            app_aud_eq[0].eq_gain =TAB1_EQGAIN;
            
            for(j=0;j<TAB1_TotalEQnum;j++)
            {
                app_aud_eq[0].eq_enable |= 1<<j; 
                app_aud_eq[i].eq_para[j].a[0]=eq1_tab_arr[j][0];
                app_aud_eq[i].eq_para[j].a[1]=eq1_tab_arr[j][1];
                app_aud_eq[i].eq_para[j].b[0]=eq1_tab_arr[j][2];
                app_aud_eq[i].eq_para[j].b[1]=eq1_tab_arr[j][3];
                app_aud_eq[i].eq_para[j].b[2]=eq1_tab_arr[j][4];
            }
        }
        else if(i==1)
        {
            app_aud_eq[1].eq_gain =TAB2_EQGAIN;
            
            for(j=0;j<TAB2_TotalEQnum;j++)
            {
                app_aud_eq[1].eq_enable |= 1<<j; 
                app_aud_eq[i].eq_para[j].a[0]=eq2_tab_arr[j][0];
                app_aud_eq[i].eq_para[j].a[1]=eq2_tab_arr[j][1];
                app_aud_eq[i].eq_para[j].b[0]=eq2_tab_arr[j][2];
                app_aud_eq[i].eq_para[j].b[1]=eq2_tab_arr[j][3];
                app_aud_eq[i].eq_para[j].b[2]=eq2_tab_arr[j][4];
            }
        }
        else if(i==2)
        {
            app_aud_eq[2].eq_gain =TAB3_EQGAIN;
            for(j=0;j<TAB3_TotalEQnum;j++)
            {
                app_aud_eq[2].eq_enable |= 1<<j; 
                app_aud_eq[i].eq_para[j].a[0]=eq3_tab_arr[j][0];
                app_aud_eq[i].eq_para[j].a[1]=eq3_tab_arr[j][1];
                app_aud_eq[i].eq_para[j].b[0]=eq3_tab_arr[j][2];
                app_aud_eq[i].eq_para[j].b[1]=eq3_tab_arr[j][3];
                app_aud_eq[i].eq_para[j].b[2]=eq3_tab_arr[j][4];
            }
        }
    }
}

void app_eq_pram_sel(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    if(env_h->env_data.eq_lang_sel<APP_EQ_NUM)
    {
        memcpy( &env_h->env_cfg.aud_eq.eq_enable,&app_aud_eq[env_h->env_data.eq_lang_sel].eq_enable,sizeof(app_aud_eq_t));
    }
    else
    {
        env_h->env_data.eq_lang_sel=0;
        memcpy( &env_h->env_cfg.aud_eq.eq_enable,&app_aud_eq[env_h->env_data.eq_lang_sel].eq_enable,sizeof(app_aud_eq_t));
    }
}
void app_online_eq_gain_enable(uint16_t enable,int16_t gain)
{	
    extern aud_pre_equ_t aud_pre_eqe;
    app_env_handle_t env_h = app_env_get_handle();
    env_h->env_cfg.aud_eq.eq_enable = enable;
    env_h->env_cfg.aud_eq.eq_gain = gain;
    float V0 = powf( 10, (float)env_h->env_cfg.aud_eq.eq_gain /20 );
    aud_pre_eqe.globle_gain = (uint32)((float)0x4000 * V0);
}
void eq_onlin_set(void){
#if (CONFIG_PRE_EQ == 1)
    app_env_handle_t  env_h = app_env_get_handle();
    extern aud_pre_equ_t aud_pre_eqe;
    extern SAMPLE_ALIGN aud_pre_equ_para_t tbl_eq_coef[];
    app_eq_pram_sel();
    aud_pre_eqe.online_flag = 0;
    aud_pre_eqe.totle_EQ = 0;
    uint16 i;
    for(i=0;i<CON_AUD_EQ_BANDS;i++)
    {
        if((1<<i)&env_h->env_cfg.aud_eq.eq_enable)
        {
            tbl_eq_coef[i].a[0] = env_h->env_cfg.aud_eq.eq_para[i].a[0];
            tbl_eq_coef[i].a[1] = env_h->env_cfg.aud_eq.eq_para[i].a[1];
            tbl_eq_coef[i].b[0] = env_h->env_cfg.aud_eq.eq_para[i].b[0];
            tbl_eq_coef[i].b[1] = env_h->env_cfg.aud_eq.eq_para[i].b[1];
            tbl_eq_coef[i].b[2] = env_h->env_cfg.aud_eq.eq_para[i].b[2];
            aud_pre_eqe.totle_EQ++;
            aud_pre_eqe.online_flag = 0x5a;
        }
        else
        {
            tbl_eq_coef[i].a[0] = 0;
            tbl_eq_coef[i].a[1] = 0;
            tbl_eq_coef[i].b[0] = 0x100000;
            tbl_eq_coef[i].b[1] = 0;
            tbl_eq_coef[i].b[2] = 0;
        }
    }
#endif
}

#endif

#ifdef CONFIG_APP_EQUANLIZER

#if (CONFIG_PRE_EQ == 1)
aud_pre_equ_t aud_pre_eqe;
extern SAMPLE_ALIGN aud_pre_equ_para_t tbl_eq_coef[];
extern void func_sw_eq(int *input,int *output,int len,int eq_cnt,uint32_t index);//len: 1=L+R=4bytes
#endif
#if (CONFIG_HFP_SPK_EQ == 1)
aud_pre_equ_t hfp_spk_eqe;
extern SAMPLE_ALIGN hfp_spk_equ_para_t tbl_hfp_spk_eq_coef[];
extern void func_sw_spk_eq(int *input,int *output,int len,int eq_cnt,uint32_t index);//len: 1=L+R=4bytes
#endif
#if (CONIFG_HFP_MIC_EQ == 1)
aud_pre_equ_t hfp_mic_eqe;
extern SAMPLE_ALIGN hfp_mic_equ_para_t tbl_hfp_mic_eq_coef[];
extern void func_sw_mic_eq(int *input,int *output,int len,int eq_cnt,uint32_t index);//len: 1=L+R=4bytes
#endif

#define FILTER_COEFS_FRA_BITS   (20)
#define FILTER_PREGAIN_FRA_BITS (14)

static __inline int32_t MUL32x32_ASR(int32_t a, int32_t b, uint32_t bits)
{
    #if   defined(__BA2__)
    int32_t result;
    __asm
    (
        "b.mulras %0,%1,%2,%3;"
        : "=r" (result)
        : "r" (a), "r" (b), "i" (bits)
        :
    );
    return result;
    #elif defined(CEVAX2)
    //TODO
    #else
    return (int64_t)a * b >> bits;
    #endif
}

static __inline int32_t SSAT16(int32_t value)
{
    #if   defined(__BA2__)
    int32_t result;
    __asm("b.lim %0,%1,%2;" : "=r" (result) : "r" (value), "r" (32767) : );
    return result;
    #elif defined(CEVAX2)
    return _lim(sat16, (int)value);
    #else
    int32_t max = 32767;
    int32_t min = ~max;
    if(value > max)
    {
        value = max;
    }
    else if(value < min)
    {
        value = min;
    }
    return value;
    #endif
}

static __inline int32_t SSAT24(int32_t value)
{
    #if   defined(__BA2__)
    int32_t result;
    __asm("b.lim %0,%1,%2;" : "=r" (result) : "r" (value), "r" (8388607) : );
    return result;
    #elif defined(CEVAX2)
    return _lim(sat24, (int)value);
    #else
    int32_t max = 8388607;
    int32_t min = ~max;
    if(value > max)
    {
        value = max;
    }
    else if(value < min)
    {
        value = min;
    }
    return value;
    #endif
}

void func_sw_eq2(int32_t* eq, int64_t* delays, int32_t* pcm, uint32_t samples, uint32_t filters)
{
    int32_t  i, f;
    int32_t* buf;
    int32_t* coefs = eq;
    int64_t* delay = delays;

    for(f = 0; f < filters; f++)
    {
        int32_t  a1, a2, b0, b1, b2;
        int64_t  s0, s1, s2, s3, s4, dd;

        a1 = *coefs++;
        a2 = *coefs++;
        b0 = *coefs++;
        b1 = *coefs++;
        b2 = *coefs++;

        s1 = *delay++;
        s2 = *delay++;
        s3 = *delay++;
        s4 = *delay++;

        buf = pcm;

        for(i = 0; i < samples; i++)
        {
            int32_t sout = *buf;

            s0 = (int64_t)b0 * sout + s1;
            s1 = (int64_t)b1 * sout + s2;
            s2 = (int64_t)b2 * sout;

            sout = s0 >> FILTER_COEFS_FRA_BITS;
            dd   = s0 - ((int64_t)sout << FILTER_COEFS_FRA_BITS);

            s1 += (int64_t)a1 * sout + dd * 2;
            s2 += (int64_t)a2 * sout - dd;

            *buf++ = sout;
            sout   = *buf;

            s0 = (int64_t)b0 * sout + s3;
            s3 = (int64_t)b1 * sout + s4;
            s4 = (int64_t)b2 * sout;

            sout = s0 >> FILTER_COEFS_FRA_BITS;
            dd   = s0 - ((int64_t)sout << FILTER_COEFS_FRA_BITS);

            s3 += (int64_t)a1 * sout + dd * 2;
            s4 += (int64_t)a2 * sout - dd;

            *buf++ = sout;
        }

         delay  -= 4;
        *delay++ = s1;
        *delay++ = s2;
        *delay++ = s3;
        *delay++ = s4;
    }
}

void func_sw_eq_mono(int32_t* eq, int64_t* delays, int32_t* pcm, uint32_t samples, uint32_t filters)
{
    int32_t  i, f;
    int32_t* buf;
    int32_t* coefs = eq;
    int64_t* delay = delays;

    for(f = 0; f < filters; f++)
    {
        int32_t  a1, a2, b0, b1, b2;
        int64_t  s0, s1, s2, s3, s4, dd;

        a1 = *coefs++; a2 = *coefs++; b0 = *coefs++; b1 = *coefs++; b2 = *coefs++;
        s1 = *delay++; s2 = *delay++; s3 = *delay++; s4 = *delay++;
        buf = pcm;

        for(i = 0; i < samples; i++)
        {
            int32_t sout = *buf;

            s0 = (int64_t)b0 * sout + s1;
            s1 = (int64_t)b1 * sout + s2;
            s2 = (int64_t)b2 * sout;

            sout = s0 >> FILTER_COEFS_FRA_BITS;
            dd   = s0 - ((int64_t)sout << FILTER_COEFS_FRA_BITS);

            s1 += (int64_t)a1 * sout + dd * 2;
            s2 += (int64_t)a2 * sout - dd;

            *buf++ = sout;
        }

         delay  -= 4;
        *delay++ = s1; *delay++ = s2; *delay++ = s3; *delay++ = s4;
    }
}

#if (CONFIG_PRE_EQ == 1)
void pre_eq_proc(int32_t *buff, uint16_t size)
{
	int32_t  p;
	int32_t* ptr = buff;

	if((aud_pre_eqe.online_flag == 0x5a)||(aud_pre_eqe.online_flag == 1))
	{
		for(p = 0; p < size; p++) ptr[p] = MUL32x32_ASR(ptr[p], aud_pre_eqe.globle_gain, FILTER_PREGAIN_FRA_BITS);

        #if 0
		func_sw_eq((int *)ptr,(int *)ptr,size>>1,aud_pre_eqe.totle_EQ,0);
        #else
        extern SAMPLE_ALIGN uint32_t s_eq_last_XY[];
        extern SAMPLE_ALIGN aud_pre_equ_para_t tbl_eq_coef[];
        func_sw_eq2((int32_t*)tbl_eq_coef, (int64_t*)s_eq_last_XY, (int32_t*)ptr, size >> 1, aud_pre_eqe.totle_EQ);
        #endif

		for(p = 0; p < size; p++) ptr[p] = SSAT24(ptr[p]);
	}
}
#endif

#if (CONFIG_HFP_SPK_EQ == 1)
void hfp_spk_eq_proc(uint8_t *input, uint16_t size)
{
	int32_t data_tmp[128],p;
	int16_t *ptr;

	if(size<=0||size>256)
		return;
	ptr = (int16_t *)input;
	if((hfp_spk_eqe.online_flag == 0x5a)||(hfp_spk_eqe.online_flag == 1))
	{
		for(p=0; p<size/2; p++)
		{
			data_tmp[p]=ptr[p]*hfp_spk_eqe.globle_gain;
		}
		func_sw_spk_eq((int *)data_tmp,(int *)data_tmp,size/2,hfp_spk_eqe.totle_EQ,0);
		for(p=0; p<size/2; p++)
		{
			data_tmp[p]>>=14;
			if(data_tmp[p] > 32767)	
				ptr[p] = 32767;
			else if(data_tmp[p] < -32767) 
				ptr[p] = -32767;
			else	
				ptr[p] = data_tmp[p];
		}
	}
}
#endif

#if (CONIFG_HFP_MIC_EQ == 1)
void hfp_mic_eq_proc(uint8_t *input, uint16_t size)
{
	int32_t data_tmp[128],p;
	int16_t *ptr;
	
	if(size<=0||size>256)
		return;
	ptr = (int16_t *)input;
	if((hfp_mic_eqe.online_flag == 0x5a)||(hfp_mic_eqe.online_flag == 1))
	{
		for(p=0; p<size/2; p++)
		{
			data_tmp[p]=ptr[p]*hfp_mic_eqe.globle_gain;
		}
		func_sw_mic_eq((int *)data_tmp,(int *)data_tmp,size/2,hfp_mic_eqe.totle_EQ,0);
		for(p=0; p<size/2; p++)
		{
			data_tmp[p]>>=14;
			if(data_tmp[p] > 32767)	
				ptr[p] = 32767;
			else if(data_tmp[p] < -32767) 
				ptr[p] = -32767;
			else	
				ptr[p] = data_tmp[p];
		}
	}
}
#endif

#if (CONFIG_PRE_EQ == 1)

void app_set_pre_eq_gain(uint8_t *para)
{
	uint16_t tmp_gain;
	
	tmp_gain = para[0]|para[1]<<8|para[2]<<16|para[3]<<24;
	os_printf("%02x, %02x %x\r\n",para[0],para[1],tmp_gain);
	aud_pre_eqe.globle_gain = (uint32_t)tmp_gain;
}

#define CMD_AUD_EQ_OPEN 0xF0
#define CMD_AUD_EQ_CLOSE 0xF1
#define CMD_AUD_EQ_RST_PARA 0xF2

void app_set_pre_eq(uint8_t *para)
{
	int i;
	#if 1
	if(para[0]==0) //first bands
	{
		aud_pre_eqe.totle_EQ = 0;
	}
	else if(para[0]==CMD_AUD_EQ_OPEN)
		aud_pre_eqe.online_flag = 0x5a;
	else if (para[0]==CMD_AUD_EQ_CLOSE) //close EQ
		aud_pre_eqe.online_flag = 0;
	else if(para[0]==CMD_AUD_EQ_RST_PARA)
	{
		aud_pre_eqe.totle_EQ = 0;
		for(i=0;i<CON_AUD_EQ_BANDS;i++)
		{
			tbl_eq_coef[i].a[0] = 0;
			tbl_eq_coef[i].a[1] = 0;
			tbl_eq_coef[i].b[0] = 0x100000;
			tbl_eq_coef[i].b[1] = 0;
			tbl_eq_coef[i].b[2] = 0;
		}
	}
	if(para[0]>=CON_AUD_EQ_BANDS)
		return;

//	os_printf("bf:%d:%x,%x,%x,%x,%x\r\n",para[0],tbl_eq_coef[para[0]].a[0],
//		tbl_eq_coef[para[0]].a[1],
//		tbl_eq_coef[para[0]].b[0],
//		tbl_eq_coef[para[0]].b[1],
//		tbl_eq_coef[para[0]].b[2]);
	if(para[1]==0) //enable flag
	{
		tbl_eq_coef[para[0]].a[0] = 0;
		tbl_eq_coef[para[0]].a[1] = 0;
		tbl_eq_coef[para[0]].b[0] = 0x100000;
		tbl_eq_coef[para[0]].b[1] = 0;
		tbl_eq_coef[para[0]].b[2] = 0;
		return;//enable flag == 0
	}

	if(para[0]+1>aud_pre_eqe.totle_EQ)
		aud_pre_eqe.totle_EQ = para[0]+1;

	memcpy(&tbl_eq_coef[para[0]].a[0], &para[2], 4*5);
	aud_pre_eqe.online_flag = 0x5a;
	#endif
}

void app_show_pre_eq(void)
{
	int i;
	os_printf("AUD_PRE_EQ flag:0x%x,cnt:%d,gain:%d\r\n",aud_pre_eqe.online_flag,aud_pre_eqe.totle_EQ,aud_pre_eqe.globle_gain);
	for(i=0;i<CON_AUD_EQ_BANDS;i++)
		os_printf("i:%d\r\n,%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n",i,
		tbl_eq_coef[i].a[0],
		tbl_eq_coef[i].a[1],
		tbl_eq_coef[i].b[0],
		tbl_eq_coef[i].b[1],
		tbl_eq_coef[i].b[2]);
}

void app_set_eq_gain_enable(uint8 *para)
{
	app_env_handle_t env_h = app_env_get_handle();
	env_h->env_cfg.aud_eq.eq_enable = (para[1]<<8)|para[0];
	env_h->env_cfg.aud_eq.eq_gain = (para[3]<<8)|para[2];
	float V0 = powf( 10, (float)env_h->env_cfg.aud_eq.eq_gain /20 );
	aud_pre_eqe.globle_gain = (uint32)((float)0x4000 * V0);
	//app_button_sw_action(BUTTON_BT_VOL_P);
}
#endif

#if (CONFIG_HFP_SPK_EQ == 1)

void app_set_hfp_spk_eq_gain(uint8_t *para)
{
	uint16_t tmp_gain;
	
	tmp_gain = para[0]|para[1]<<8|para[2]<<16|para[3]<<24;
	os_printf("HFP_SPK_EQ %02x, %02x %x\r\n",para[0],para[1],tmp_gain);
	hfp_spk_eqe.globle_gain = (uint32_t)tmp_gain;
}

#define CMD_AUD_EQ_OPEN 0xF0
#define CMD_AUD_EQ_CLOSE 0xF1
#define CMD_AUD_EQ_RST_PARA 0xF2

void app_set_hfp_spk_eq(uint8_t *para)
{
	int i;
	#if 1
	if(para[0]==0) //first bands
	{
		hfp_spk_eqe.totle_EQ = 0;
	}
	else if(para[0]==CMD_AUD_EQ_OPEN)
		hfp_spk_eqe.online_flag = 0x5a;
	else if (para[0]==CMD_AUD_EQ_CLOSE) //close EQ
		hfp_spk_eqe.online_flag = 0;
	else if(para[0]==CMD_AUD_EQ_RST_PARA)
	{
		hfp_spk_eqe.totle_EQ = 0;
		for(i=0;i<CON_HFP_SPK_EQ_BANDS;i++)
		{
			tbl_hfp_spk_eq_coef[i].a[0] = 0;
			tbl_hfp_spk_eq_coef[i].a[1] = 0;
			tbl_hfp_spk_eq_coef[i].b[0] = 0x100000;
			tbl_hfp_spk_eq_coef[i].b[1] = 0;
			tbl_hfp_spk_eq_coef[i].b[2] = 0;
		}
	}
	if(para[0]>=CON_HFP_SPK_EQ_BANDS)
		return;

//	os_printf("bf:%d:%x,%x,%x,%x,%x\r\n",para[0],tbl_eq_coef[para[0]].a[0],
//		tbl_eq_coef[para[0]].a[1],
//		tbl_eq_coef[para[0]].b[0],
//		tbl_eq_coef[para[0]].b[1],
//		tbl_eq_coef[para[0]].b[2]);
	if(para[1]==0) //enable flag
	{
		tbl_hfp_spk_eq_coef[para[0]].a[0] = 0;
		tbl_hfp_spk_eq_coef[para[0]].a[1] = 0;
		tbl_hfp_spk_eq_coef[para[0]].b[0] = 0x100000;
		tbl_hfp_spk_eq_coef[para[0]].b[1] = 0;
		tbl_hfp_spk_eq_coef[para[0]].b[2] = 0;
		return;//enable flag == 0
	}

	if(para[0]+1>hfp_spk_eqe.totle_EQ)
		hfp_spk_eqe.totle_EQ = para[0]+1;

	memcpy(&tbl_hfp_spk_eq_coef[para[0]].a[0], &para[2], 4*5);
	hfp_spk_eqe.online_flag = 0x5a;
	#endif

}

void app_show_hfp_spk_eq(void)
{
	int i;
	os_printf("HFP_SPK_EQ flag:0x%x,cnt:%d,gain:%d\r\n",hfp_spk_eqe.online_flag,hfp_spk_eqe.totle_EQ,hfp_spk_eqe.globle_gain);
	for(i=0;i<CON_HFP_SPK_EQ_BANDS;i++)
		os_printf("i:%d\r\n,%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n",i,
		tbl_hfp_spk_eq_coef[i].a[0],
		tbl_hfp_spk_eq_coef[i].a[1],
		tbl_hfp_spk_eq_coef[i].b[0],
		tbl_hfp_spk_eq_coef[i].b[1],
		tbl_hfp_spk_eq_coef[i].b[2]);
}

void app_set_spk_eq_gain_enable(uint8 *para)
{
	app_env_handle_t env_h = app_env_get_handle();
	env_h->env_cfg.hfp_cfg.hfp_spk_eq_enable = (para[1]<<8)|para[0];
	env_h->env_cfg.hfp_cfg.hfp_spk_eq_gain = (para[3]<<8)|para[2];
	float V0 = powf( 10, (float)env_h->env_cfg.hfp_cfg.hfp_spk_eq_gain /20 );
	hfp_spk_eqe.globle_gain = (uint32)((float)0x4000 * V0);
	//app_button_sw_action(BUTTON_BT_VOL_P);
}
#endif

#if (CONIFG_HFP_MIC_EQ == 1)

void app_set_hfp_mic_eq_gain(uint8_t *para)
{
	uint16_t tmp_gain;
	
	tmp_gain = para[0]|para[1]<<8|para[2]<<16|para[3]<<24;
	os_printf("%02x, %02x %x\r\n",para[0],para[1],tmp_gain);
	hfp_mic_eqe.globle_gain = (uint32_t)tmp_gain;
}

#define CMD_AUD_EQ_OPEN 0xF0
#define CMD_AUD_EQ_CLOSE 0xF1
#define CMD_AUD_EQ_RST_PARA 0xF2

void app_set_hfp_mic_eq(uint8_t *para)
{
	int i;
	#if 1
	if(para[0]==0) //first bands
	{
		hfp_mic_eqe.totle_EQ = 0;
	}
	else if(para[0]==CMD_AUD_EQ_OPEN)
		hfp_mic_eqe.online_flag = 0x5a;
	else if (para[0]==CMD_AUD_EQ_CLOSE) //close EQ
		hfp_mic_eqe.online_flag = 0;
	else if(para[0]==CMD_AUD_EQ_RST_PARA)
	{
		hfp_mic_eqe.totle_EQ = 0;
		for(i=0;i<CON_HFP_MIC_EQ_BANDS;i++)
		{
			tbl_hfp_mic_eq_coef[i].a[0] = 0;
			tbl_hfp_mic_eq_coef[i].a[1] = 0;
			tbl_hfp_mic_eq_coef[i].b[0] = 0x100000;
			tbl_hfp_mic_eq_coef[i].b[1] = 0;
			tbl_hfp_mic_eq_coef[i].b[2] = 0;
		}
	}
	if(para[0]>=CON_HFP_MIC_EQ_BANDS)
		return;

//	os_printf("bf:%d:%x,%x,%x,%x,%x\r\n",para[0],tbl_eq_coef[para[0]].a[0],
//		tbl_eq_coef[para[0]].a[1],
//		tbl_eq_coef[para[0]].b[0],
//		tbl_eq_coef[para[0]].b[1],
//		tbl_eq_coef[para[0]].b[2]);
	if(para[1]==0) //enable flag
	{
		tbl_hfp_mic_eq_coef[para[0]].a[0] = 0;
		tbl_hfp_mic_eq_coef[para[0]].a[1] = 0;
		tbl_hfp_mic_eq_coef[para[0]].b[0] = 0x100000;
		tbl_hfp_mic_eq_coef[para[0]].b[1] = 0;
		tbl_hfp_mic_eq_coef[para[0]].b[2] = 0;
		return;//enable flag == 0
	}

	if(para[0]+1>hfp_mic_eqe.totle_EQ)
		hfp_mic_eqe.totle_EQ = para[0]+1;

	memcpy(&tbl_hfp_mic_eq_coef[para[0]].a[0], &para[2], 4*5);
	hfp_mic_eqe.online_flag = 0x5a;
	#endif

}

void app_show_hfp_mic_eq(void)
{
	int i;
	os_printf("HFP_MIC_EQ flag:0x%x,cnt:%d,gain:%d\r\n",hfp_mic_eqe.online_flag,hfp_mic_eqe.totle_EQ,hfp_mic_eqe.globle_gain);
	for(i=0;i<CON_HFP_MIC_EQ_BANDS;i++)
		os_printf("i:%d\r\n,%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n",i,
		tbl_hfp_mic_eq_coef[i].a[0],
		tbl_hfp_mic_eq_coef[i].a[1],
		tbl_hfp_mic_eq_coef[i].b[0],
		tbl_hfp_mic_eq_coef[i].b[1],
		tbl_hfp_mic_eq_coef[i].b[2]);
}

void app_set_mic_eq_gain_enable(uint8 *para)
{
	app_env_handle_t env_h = app_env_get_handle();
	env_h->env_cfg.hfp_cfg.hfp_mic_eq_enable = (para[1]<<8)|para[0];
	env_h->env_cfg.hfp_cfg.hfp_mic_eq_gain = (para[3]<<8)|para[2];
	float V0 = powf( 10, (float)env_h->env_cfg.hfp_cfg.hfp_mic_eq_gain /20 );
	hfp_mic_eqe.globle_gain = (uint32)((float)0x4000 * V0);
	//app_button_sw_action(BUTTON_BT_VOL_P);
}
#endif //CONIFG_HFP_MIC_EQ
#endif //CONFIG_APP_EQUANLIZER

#if (CONIFG_HFP_MIC_EQ == 0)
void hfp_mic_eq_proc(uint8_t *input, uint16_t size) {}//avoid compile error, ++by Borg@230214
#endif
