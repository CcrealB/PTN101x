#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "tra_hcit.h"
#include "driver_efuse.h"
#include <stdlib.h>
#include "math.h"

#define FLASH_DELAY_COUNT   100

static app_env_t app_env;
static uint8_t flag_auto_powerdown = 0;
static btaddr_t btaddr_def = {{0x88, 0x32, 0x88, 0x32, 0x12, 0x22}};

app_env_t * app_env_get_handle( void )
{
    return &app_env;
}

void app_env_dump(void)
{
    int i,j;
    char addr[14], linkkey[34];
    app_env_handle_t  env_h = app_env_get_handle();

    os_printf("env data:\r\n");
    for( i = 0; i < MAX_KEY_STORE; i++ )
    {
        for( j = 0; j < 6; j++ )
        {
            sprintf( &addr[j*2], "%02x", env_h->env_data.key_pair[i].btaddr.b[5-j] );
        }
        for( j=0; j < 16; j++)
            sprintf( &linkkey[j*2], "%02x", env_h->env_data.key_pair[i].linkkey[j] );
        addr[13] = 0;
        linkkey[33] = 0;
        //if( env_h->env_data.key_pair[i].used == 0x01 )
            os_printf("key_pair[%d] used: %02x, addr: 0x%s, linkkey: 0x%s\r\n",
                i, env_h->env_data.key_pair[i].used, addr, linkkey);
    }

    os_printf("key_index: %d, last btaddr: 0x%02x%02x%02x%02x%02x%02x\r\n",
        env_h->env_data.key_index, env_h->env_data.default_btaddr.b[5], env_h->env_data.default_btaddr.b[4],
        env_h->env_data.default_btaddr.b[3],env_h->env_data.default_btaddr.b[2],env_h->env_data.default_btaddr.b[1],
        env_h->env_data.default_btaddr.b[0] );
    os_printf("\r\nenv cfg:\r\n");
    os_printf("used: %d, bt flag: %x, system_flag: %x, disconn_action: %d,device_class, %d\r\n",
        env_h->env_cfg.used, env_h->env_cfg.bt_para.bt_flag, env_h->env_cfg.system_para.system_flag,
        env_h->env_cfg.bt_para.disconn_action, env_h->env_cfg.bt_para.device_class);
    os_printf("device_name: %s, device_pin: %s\r\n",
        env_h->env_cfg.bt_para.device_name, env_h->env_cfg.bt_para.device_pin);
    os_printf("device btaddr: 0x%02x%02x%02x%02x%02x%02x\r\n",
        env_h->env_cfg.bt_para.device_addr.b[5], env_h->env_cfg.bt_para.device_addr.b[4],
        env_h->env_cfg.bt_para.device_addr.b[3],env_h->env_cfg.bt_para.device_addr.b[2],
        env_h->env_cfg.bt_para.device_addr.b[1],env_h->env_cfg.bt_para.device_addr.b[0]);

    os_printf("auto connection interval: %d, auto connection count: %d\r\n",
        env_h->env_cfg.bt_para.auto_conn_interval,
        env_h->env_cfg.bt_para.auto_conn_count);
    os_printf("recovery start: %d, interval: %d, count: %d \r\n", env_h->env_cfg.bt_para.disconn_start,
        env_h->env_cfg.bt_para.disconn_retry_interval, env_h->env_cfg.bt_para.disconn_retry_count );
    os_printf("lowpower channel %d, threshold: %x, pd threshold: %x, interval: %d\r\n",
        env_h->env_cfg.system_para.lp_channel,
        env_h->env_cfg.system_para.lp_threshold,
        env_h->env_cfg.system_para.lp_pd_threshold,
        env_h->env_cfg.system_para.lp_interval);
//#if (CONFIG_CHARGE_EN == 1)
        os_printf("current: %d\r\n",
        env_h->env_cfg.system_para.charger_current);
//#endif

    os_printf("a2dp vol: %d, mic vol: %d, hfp vol: %d, wave vol: %d, mute pin: %d, line in pin: %d\r\n",
        env_h->env_cfg.system_para.vol_a2dp, env_h->env_cfg.system_para.vol_mic,
        env_h->env_cfg.system_para.vol_hfp, env_h->env_cfg.system_para.vol_wave,
        app_env_get_pin_num(PIN_paMute), app_env_get_pin_num(PIN_lineDet));
    os_printf("sleep time out: %d, powerdown time out: %d\r\n",
        env_h->env_cfg.system_para.sleep_timeout,
        env_h->env_cfg.system_para.powerdown_timeout);
    os_printf("pd condition: %02x, match timeout: %d, action of disconn: %d\r\n",
        env_h->env_cfg.bt_para.pd_cond,
        env_h->env_cfg.bt_para.match_timeout, env_h->env_cfg.bt_para.action_disconn );

    os_printf("wave sel: %d\r\n", env_h->env_cfg.wave_lang_sel );
    for( i = 0; i < WAVE_EVENT; i++ )
    {
		os_printf("wave audio info[%d]: used: %x, page_index:%d\r\n",
			i, env_h->env_cfg.wave_info[i].used,
			env_h->env_cfg.wave_info[i].page_index );
    }

    for( i = 0; i < WAVE_EVENT; i++ )
    {
		os_printf("wave audio info1[%d]: used: %x, page_index:%d\r\n",
			i, env_h->env_cfg.wave_info1[i].used,
			env_h->env_cfg.wave_info1[i].page_index );
    }

    for( i = 0; i < WAVE_EVENT; i++ )
    {
		os_printf("wave audio info2[%d]: used: %x, page_index:%d\r\n",
			i, env_h->env_cfg.wave_info2[i].used,
			env_h->env_cfg.wave_info2[i].page_index );
    }


    for( i = 0; i < WAVE_EVENT; i++ )
    {
		os_printf("wave audio info3[%d]: used: %x, page_index:%d\r\n",
			i, env_h->env_cfg.wave_info3[i].used,
			env_h->env_cfg.wave_info3[i].page_index );
    }

    for( i = 0; i < WAVE_EVENT; i++ )
    {
		os_printf("wave audio info4[%d]: used: %x, page_index:%d\r\n",
			i, env_h->env_cfg.wave_info4[i].used,
			env_h->env_cfg.wave_info4[i].page_index );
	}
    for( i = 0; i < LED_NUM; i++ )
		os_printf("led[%d]:%d\r\n",i,env_h->env_cfg.led_map[i]);

    for( i = 0; i < LED_EVENT_END; i++ )
        os_printf("led info[%d]: index: %d, ontime: %d, offtime: %d, repeat:%d, num flash: %d, timeout: %d\r\n",
            i, env_h->env_cfg.led_info[i].index, env_h->env_cfg.led_info[i].led_ontime,
            env_h->env_cfg.led_info[i].led_offtime, env_h->env_cfg.led_info[i].repeat,
            env_h->env_cfg.led_info[i].number_flash, env_h->env_cfg.led_info[i].timeout );

    os_printf("button threshold: \r\npress: %d, repeat: %d, long: %d, very long: %d, double; %d\r\n",
        env_h->env_cfg.button_para.press, env_h->env_cfg.button_para.repeat,
        env_h->env_cfg.button_para.longp, env_h->env_cfg.button_para.vlongp,
        env_h->env_cfg.button_para.doublep);

    for( i = 0; i < BUTTON_BT_END; i++ )
	{
		uint8_t *ptr_u8 = (uint8_t *)&env_h->env_cfg.button_code[i];
        os_printf("button code[%d]: %02x%02x%02x%02x%02x%02x%02x%02x \r\n", i, ptr_u8[7],ptr_u8[6],ptr_u8[5],ptr_u8[4],ptr_u8[3],ptr_u8[2],ptr_u8[1],ptr_u8[0]);
	}
    os_printf("pwrBtn_pin:%d\r\n",app_env_get_pin_num(PIN_pwrBtn));
    os_printf("mosCtrl_pin:%d\r\n",app_env_get_pin_num(PIN_mosCtrl));

    os_printf("fft_shift:%d decay_time:%d ear_gain:%d mic_gain:%d\r\n",
    			env_h->env_cfg.hfp_cfg.aec_fft_shift,
    			env_h->env_cfg.hfp_cfg.aec_decay_time,
    			env_h->env_cfg.hfp_cfg.aec_ear_gain,
    			env_h->env_cfg.hfp_cfg.aec_mic_gain);
    os_printf("feature_flag:0x%x BT_MODE: 0x%x,\r\n, vol_mic_dig: %d, pa_mute_delay_time:%d\r\n",
    env_h->env_cfg.feature.feature_flag,
    env_h->env_cfg.feature.bt_mode,
    env_h->env_cfg.feature.vol_mic_dig,
    env_h->env_cfg.feature.pa_mute_delay_time*10);

	os_printf("feature_flag:0x%x\r\n",env_h->env_cfg.feature.feature_flag);
		os_printf("a2dp:\r\n");
		for(i=0;i<17;i++)
    	os_printf("vol[%d].ang:%d\t,dig:%d\r\n",i,env_h->env_cfg.feature.a2dp_vol.vol[i].ana_dig_gain>>13 ,env_h->env_cfg.feature.a2dp_vol.vol[i].ana_dig_gain&0x1fff);
		os_printf("hfp_vol:\r\n");
		for(i=0;i<17;i++)
    	os_printf("vol[%d].ang:%d\t,dig:%d\r\n",i,env_h->env_cfg.feature.hfp_vol.vol[i].ana_dig_gain>>13 ,env_h->env_cfg.feature.hfp_vol.vol[i].ana_dig_gain&0x1fff);
		os_printf("linein_vol:\r\n");
		for(i=0;i<17;i++)
    	os_printf("vol[%d].ang:%d\t,dig:%d\r\n",i,env_h->env_cfg.feature.linein_vol.vol[i].ana_dig_gain>>13 ,env_h->env_cfg.feature.linein_vol.vol[i].ana_dig_gain&0x1fff);



}


void app_print_linkkey(void)
{
    int i,j;
    char addr[14]={0}, linkkey[34]={0};
    app_env_handle_t  env_h = app_env_get_handle();

    for (i = 0; i < MAX_KEY_STORE; i++)
    {
        for (j = 0; j < 6; j++)
            sprintf( &addr[j*2], "%02x", env_h->env_data.key_pair[i].btaddr.b[5-j] );
        for( j=0; j < 16; j++)
            sprintf( &linkkey[j*2], "%02x", env_h->env_data.key_pair[i].linkkey[j] );
        if( env_h->env_data.key_pair[i].used != 0xff)
            LOG_I(APP,"key_pair[%d] used: %02x,crystal:%d, audio_cali:%04x,addr: 0x%s, linkkey: 0x%s\r\n",
                i, env_h->env_data.key_pair[i].used,env_h->env_data.key_pair[i].crystal_cali_data,env_h->env_data.key_pair[i].aud_dc_cali_data, addr, linkkey);
    }
}

#if (CONFIG_CHARGE_EN == 1)
//extern uint16_t adc_list[][2];
//extern uint16_t th_low_trk,th_high_trk,th_lc,th_cv,th_cv_trk,th_end;
#endif

static env_lpbg_cali_data_t s_env_lpbg_cali_data;
static env_saradc_cali_data_t  s_env_saradc_cali_data;
static env_charge_cali_data_t  s_env_charge_cali_data;
static env_aud_dc_offset_data_t s_env_audio_dc_offset_0;
static env_aud_dc_offset_data_t s_env_audio_dc_offset_1;
static uint16_t s_env_tempr_cali_data;
/* Env calibration data Tag:
Bit0:lpbg
Bit1:saradc   
Bit2:charge
Bit3:audio_dc0
Bit4:audio_dc1
Bit5:temperature
Bit6-7:reserved
Bit8:Cali data in eFuse;
Bit9:Cali data in Flash;
*/
static uint16 s_env_cali_data_tag = 0;
#define CALI_TAG_LPBG      (1<<0)
#define CALI_TAG_CHG       (1<<1)
#define CALI_TAG_ADC       (1<<2)
#define CALI_TAG_TMPR      (1<<3)
#define CALI_TAG_AUD_DIFF  (1<<4)
#define CALI_TAG_AUD_SNGL  (1<<5)
#define CALI_TAG_IN_EFUSE  (1<<8)
#define CALI_TAG_IN_FLASH  (1<<9)

#if EFUSE_EN
static env_cali_data_t  s_env_cali_data;
#endif

#define SARADC_CALI_DC_DECR      2016     //   2016+0x3f = 2079
#define SARADC_CALI_4P2_DECR     2544   // 2544 + 0x1ff = 3055
#define TEMPR_CALI_DECR          1490    // 1490 + 0x7f = 1617

__inline env_lpbg_cali_data_t *app_get_env_lpbg_cali_data(void)
{
    return &s_env_lpbg_cali_data;
}
__inline env_saradc_cali_data_t *  app_get_env_saradc_cali_data(void)
{
    return &s_env_saradc_cali_data;
}
__inline env_charge_cali_data_t * app_get_env_charge_cali_data(void)
{
    return &s_env_charge_cali_data;
}
__inline env_aud_dc_offset_data_t* app_get_env_dc_offset_cali(void)
{
	app_env_handle_t  env_h = app_env_get_handle();
	return (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER) ? &s_env_audio_dc_offset_0 : &s_env_audio_dc_offset_1;
}

__inline uint32_t app_get_env_aud_cali_valid(void)
{
	app_env_handle_t  env_h = app_env_get_handle();
    return (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER) ? (s_env_cali_data_tag&CALI_TAG_AUD_DIFF):(s_env_cali_data_tag&CALI_TAG_AUD_SNGL);
}

void app_env_cali_data_prt(void)
{
    if(s_env_cali_data_tag & (CALI_TAG_IN_EFUSE|CALI_TAG_IN_FLASH))
    {
        if(s_env_cali_data_tag & CALI_TAG_IN_EFUSE)
        {
            LOG_I(CALI,"Calibration Data in eFuse\r\n");
        }
        else
        {
            LOG_I(CALI,"Calibration Data in Flash\r\n");
        }
    }
    else
    {
        LOG_I(CALI,"NO Calibration Data(Default)\r\n");
    }
    
    LOG_I(CALI,"------------------------------------\r\n");
    LOG_I(CALI,"|    LPBG:%d,%x,%x,%x\r\n",(s_env_cali_data_tag & CALI_TAG_LPBG),s_env_lpbg_cali_data.cali_lpbg_bandgap,s_env_lpbg_cali_data.cali_lpbg_vddcore,s_env_lpbg_cali_data.cali_lpbg_vddio);
    LOG_I(CALI,"|  SARADC:%d,%d,%d\r\n",(s_env_cali_data_tag & CALI_TAG_ADC)>>2,s_env_saradc_cali_data.sar_adc_dc,s_env_saradc_cali_data.sar_adc_4p2);
    LOG_I(CALI,"|  Charge:%d,%x,%x,%x\r\n",(s_env_cali_data_tag & CALI_TAG_CHG)>>1,s_env_charge_cali_data.charger_vlcf,s_env_charge_cali_data.charger_icp,s_env_charge_cali_data.charger_vcv);
    LOG_I(CALI,"|Aud_diff:%d,%x,%x\r\n",(s_env_cali_data_tag & CALI_TAG_AUD_DIFF)>>4,s_env_audio_dc_offset_0.dac_l_dc_offset[0],s_env_audio_dc_offset_0.dac_r_dc_offset[0]);
    LOG_I(CALI,"------------------------------------\r\n");
}

BOOL odd_check(uint16_t val)
{
    BOOL odd_r = TRUE;
    while(val)
    {
        odd_r = !odd_r;
        val = val&(val-1);
    }
    return odd_r;
}

#define VDDIO_STEP    3
void change_vddio(uint8_t vddio)
{
    uint32_t reg_0x5C = REG_SYSTEM_0x5C;
    uint8_t old_vddio = reg_0x5C&0x3f;    
    
    if(vddio > old_vddio)
    {
        while(vddio-old_vddio>=VDDIO_STEP)
        {
            old_vddio += VDDIO_STEP;
            reg_0x5C &= ~(0x3f<<0);
            reg_0x5C |= old_vddio;
            REG_SYSTEM_0x5C = reg_0x5C;
            sys_delay_ms(1);
            //LOG_I(CHARGE,"%x->%x\r\n",old_vddio,vddio);
        }
    }
    else
    {
        while(old_vddio - vddio>=VDDIO_STEP)
        {
            old_vddio -= VDDIO_STEP;
            reg_0x5C &= ~(0x3f<<0);
            reg_0x5C |= old_vddio;
            REG_SYSTEM_0x5C = reg_0x5C;
            sys_delay_ms(1);
            //LOG_I(CHARGE,"%x->%x\r\n",old_vddio,vddio);
        }
    }
    if(vddio != old_vddio)
    {
        reg_0x5C &= ~(0x3f<<0);
        reg_0x5C |= vddio;
        REG_SYSTEM_0x5C = reg_0x5C;
        //LOG_I(CHARGE,"%x->%x\r\n",old_vddio,vddio);
    }
}

#if EFUSE_EN
uint8_t eFuse_get_cali_data(void)
{

    uint8_t result = 0;
    uint8_t cali_data[11];

    /* set default calibration data */
    s_env_saradc_cali_data.sar_adc_dc = 2040;
    s_env_saradc_cali_data.sar_adc_4p2 = 2700;    // 4.2v
    s_env_charge_cali_data.charger_icp = 0x10;
    s_env_charge_cali_data.charger_vcv = 0x10;
    s_env_charge_cali_data.charger_vlcf = 0x3f;
    s_env_lpbg_cali_data.cali_lpbg_bandgap = (REG_SYSTEM_0x46>>19)&0x3f;
    s_env_lpbg_cali_data.cali_lpbg_vddcore = (REG_SYSTEM_0x5C>>8)&0x3f;
    s_env_lpbg_cali_data.cali_lpbg_vddio= REG_SYSTEM_0x5C&0x3f; 
    s_env_tempr_cali_data = 1560;

    eFuse_enable(1); 
 
    result = eFuse_read(cali_data,16,11); 

    eFuse_enable(0);

    if(result)  // 
    {
        os_memcpy(&s_env_cali_data,cali_data,sizeof(s_env_cali_data));
        result = 0;
        if((s_env_cali_data.odd_bg == odd_check(s_env_cali_data.bandgap))
            &&(s_env_cali_data.odd_vcore == odd_check(s_env_cali_data.vddcore))
            &&(s_env_cali_data.odd_vio == odd_check(s_env_cali_data.vddio)))
        {            
            s_env_lpbg_cali_data.cali_lpbg_bandgap = s_env_cali_data.bandgap;
            s_env_lpbg_cali_data.cali_lpbg_vddcore= s_env_cali_data.vddcore;
            s_env_lpbg_cali_data.cali_lpbg_vddio= s_env_cali_data.vddio;
            
            REG_SYSTEM_0x46 &= ~(0x3f<<19);
            sys_delay_cycle(6);
            REG_SYSTEM_0x46 |= s_env_lpbg_cali_data.cali_lpbg_bandgap<<19;

            REG_SYSTEM_0x5C &= ~(0x3f<<8);
            sys_delay_cycle(6);
            REG_SYSTEM_0x5C |= s_env_lpbg_cali_data.cali_lpbg_vddcore<<8;
            
            change_vddio(s_env_lpbg_cali_data.cali_lpbg_vddio);
            s_env_cali_data_tag |= CALI_TAG_LPBG;
            result++;
        }
        if(s_env_cali_data.odd_chgi == odd_check(s_env_cali_data.icp^s_env_cali_data.vcv))
        {
            s_env_charge_cali_data.charger_icp = s_env_cali_data.icp;
            s_env_charge_cali_data.charger_vcv = s_env_cali_data.vcv;
            s_env_cali_data_tag |= CALI_TAG_CHG;
            result++;
        }
        if(s_env_cali_data.odd_chgv == odd_check(s_env_cali_data.vlcf))
        {
            s_env_charge_cali_data.charger_vlcf = s_env_cali_data.vlcf; 
            s_env_cali_data_tag |= CALI_TAG_CHG;
            result++;
        }
        if(s_env_cali_data.odd_adc == odd_check(s_env_cali_data.vdc^s_env_cali_data.v4p2))
        {
            s_env_saradc_cali_data.sar_adc_dc = (uint16_t)s_env_cali_data.vdc + SARADC_CALI_DC_DECR;
            if( (s_env_cali_data.v4p2_sprt) && ( s_env_cali_data.odd_v4p2_sprt == odd_check(s_env_cali_data.v4p2_sprt) ) )
            {
                s_env_saradc_cali_data.sar_adc_4p2 = ((uint16_t)s_env_cali_data.v4p2_sprt << 9) + (uint16_t)s_env_cali_data.v4p2;
            }
            else
            {
                s_env_saradc_cali_data.sar_adc_4p2 = (uint16_t)s_env_cali_data.v4p2 + SARADC_CALI_4P2_DECR;
            }
            s_env_cali_data_tag |= CALI_TAG_ADC;
            result++;
        }
        if(s_env_cali_data.odd_t == odd_check(s_env_cali_data.tmpr))
        {
            s_env_tempr_cali_data = (uint16_t)s_env_cali_data.tmpr + TEMPR_CALI_DECR;
            s_env_cali_data_tag |= CALI_TAG_TMPR;
            result++;
        }  
        if(s_env_cali_data.odd_aud == odd_check(s_env_cali_data.aud_l^s_env_cali_data.aud_r))
        {
            s_env_audio_dc_offset_0.dac_l_dc_offset[0] = s_env_cali_data.aud_l;
            s_env_audio_dc_offset_0.dac_r_dc_offset[0] = s_env_cali_data.aud_r;
            s_env_cali_data_tag |= CALI_TAG_AUD_DIFF;
            result++;
        }

        if(result) s_env_cali_data_tag |= CALI_TAG_IN_EFUSE;    
    }
    return result;

}
#endif

void app_env_cali_data_init(void)
{
    TLV_TYPE tlv_cali_data;
    BOOL tlv_loop = TRUE;
    BOOL tlv_valid = TRUE;
    uint8_t *env_cali_ptr; /* ATE calibration data address */
    uint8_t chip_magic[8];

#if (EFUSE_EN == 0)
    /* set default calibration data */
    s_env_saradc_cali_data.sar_adc_dc = 2040;
    s_env_saradc_cali_data.sar_adc_4p2 = 2700;    // 4.2v
    s_env_charge_cali_data.charger_icp = 0x10;
    s_env_charge_cali_data.charger_vcv = 0x10;
    s_env_charge_cali_data.charger_vlcf = 0x3f;
    s_env_lpbg_cali_data.cali_lpbg_bandgap = (REG_SYSTEM_0x46>>19)&0x3f;
    s_env_lpbg_cali_data.cali_lpbg_vddcore = (REG_SYSTEM_0x5C>>8)&0x3f;
    s_env_lpbg_cali_data.cali_lpbg_vddio= REG_SYSTEM_0x5C&0x3f; 
    s_env_tempr_cali_data = 1560;
#endif

    {
        env_cali_ptr = (uint8 *)app_env_get_flash_addr(TLV_SECTOR_ENVCALI);
        flash_read_data(chip_magic, (uint32)env_cali_ptr, sizeof(env_chip_magic));
        if(os_memcmp(chip_magic,env_chip_magic,sizeof(env_chip_magic))) /* has no ATE calibration data */
        {
            //INFO_PRT("===set default calibration data\r\n");
        }
        else
        {
            //INFO_PRT("===get calibration data in flash\r\n");
            env_cali_ptr += sizeof(env_chip_magic);
            s_env_cali_data_tag |= CALI_TAG_IN_FLASH;
            while(tlv_loop)
            {
                flash_read_data((uint8 *)&tlv_cali_data, (uint32)env_cali_ptr, sizeof(TLV_TYPE));
                env_cali_ptr += sizeof(TLV_TYPE);
                switch(tlv_cali_data.type)
                {
                    case TLV_TYPE_CALI_END:
                        tlv_valid = FALSE;  // invalid tlv type,  continue
                        break;
                    case TLV_TYPE_CALI_DC_OFFSET_DIFF_DISPGA:
                        flash_read_data((uint8 *)&s_env_audio_dc_offset_0, (uint32)env_cali_ptr, tlv_cali_data.len);
                        s_env_cali_data_tag |= CALI_TAG_AUD_DIFF;
                        break;
                    case TLV_TYPE_CALI_DC_OFFSET_SNGL_DISPGA:
                        flash_read_data((uint8 *)&s_env_audio_dc_offset_1, (uint32)env_cali_ptr, tlv_cali_data.len);
                        s_env_cali_data_tag |= CALI_TAG_AUD_SNGL;
                        break;
                    case TLV_TYPE_CALI_CHARGE:
                        flash_read_data((uint8 *)&s_env_charge_cali_data, (uint32)env_cali_ptr, tlv_cali_data.len);
                        s_env_cali_data_tag |= CALI_TAG_CHG;
                        //INFO_PRT("charger cali:%d,%d,%d\r\n",s_env_charge_cali_data.charger_vlcf,s_env_charge_cali_data.charger_icp,s_env_charge_cali_data.charger_vcv);
                        break;
                    case TLV_TYPE_CALI_SARADC:
                        flash_read_data((uint8 *)&s_env_saradc_cali_data, (uint32)env_cali_ptr, tlv_cali_data.len);
                        s_env_cali_data_tag |= CALI_TAG_ADC;
                        //INFO_PRT("saradc:%d,%d\r\n",s_env_saradc_cali_data.sar_adc_dc,s_env_saradc_cali_data.sar_adc_4p2);
                        break;
                    case TLV_TYPE_CALI_VOLTAGE:
                        flash_read_data((uint8 *)&s_env_lpbg_cali_data, (uint32)env_cali_ptr, tlv_cali_data.len);
                        
                        REG_SYSTEM_0x46 &= ~(0x3f<<19);
                        sys_delay_cycle(6);
                        REG_SYSTEM_0x46 |= s_env_lpbg_cali_data.cali_lpbg_bandgap<<19;

                        REG_SYSTEM_0x5C &= ~(0x3f<<8);
                        sys_delay_cycle(6);
                        REG_SYSTEM_0x5C |= s_env_lpbg_cali_data.cali_lpbg_vddcore<<8;
                        
                        change_vddio(s_env_lpbg_cali_data.cali_lpbg_vddio);
                        s_env_cali_data_tag |= CALI_TAG_LPBG;
                        //INFO_PRT("LPBG, bandgap:%x,vddcore:%x,vddio:%x\r\n",s_env_lpbg_cali_data.cali_lpbg_bandgap,s_env_lpbg_cali_data.cali_lpbg_vddcore,s_env_lpbg_cali_data.cali_lpbg_vddio);
                        break;
                    case TLV_TYPE_CALI_TEMPR:
                        flash_read_data((uint8 *)&s_env_tempr_cali_data, (uint32)env_cali_ptr, tlv_cali_data.len);
                        s_env_cali_data_tag |= CALI_TAG_TMPR;
                        //INFO_PRT("temperature cal:%d\r\n",s_env_tempr_cali_data);
                        break;
                    #if 0//(CONFIG_ANC_ENABLE == 1)
                    case TLV_TYPE_CALI_ANC_PARAM:
                        app_anc_gain_params_read_from_flash((uint32)env_cali_ptr, tlv_cali_data.len);
                        break;
                    case TLV_TYPE_CALI_ANC_COEFS:
                        app_anc_filter_coefs_read_from_flash((uint32)env_cali_ptr, tlv_cali_data.len);
                        break;
                    #endif
                    case TLV_TYPE_CALI_END2:
                        tlv_loop = FALSE;
                        break;
                    default:break;

                }
                if(tlv_valid&tlv_loop)
                    env_cali_ptr += tlv_cali_data.len;
            }

        }
    }

 
    //if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE)
    //	&&app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_LOW_VOLT))
    //{
                 
    //}

}
#if (CONFIG_ANC_ENABLE == 1)
void app_anc_cali_data_init(void)
{
    TLV_TYPE tlv_cali_data;
    BOOL tlv_loop = TRUE;
    BOOL tlv_valid = TRUE;
    uint8_t *env_cali_ptr; /* ATE calibration data address */
    uint8_t chip_magic[8];
	
    {
        env_cali_ptr = (uint8 *)app_env_get_flash_addr(TLV_SECTOR_ENVCALI);
        flash_read_data(chip_magic, (uint32)env_cali_ptr, sizeof(env_chip_magic));
        if(os_memcmp(chip_magic,env_chip_magic,sizeof(env_chip_magic))) /* has no ATE calibration data */
        {
            //INFO_PRT("===set default calibration data\r\n");
        }
        else
        {
            //INFO_PRT("===get calibration data in flash\r\n");
            env_cali_ptr += sizeof(env_chip_magic);
            s_env_cali_data_tag |= CALI_TAG_IN_FLASH;
            while(tlv_loop)
            {
                flash_read_data((uint8 *)&tlv_cali_data, (uint32)env_cali_ptr, sizeof(TLV_TYPE));
                env_cali_ptr += sizeof(TLV_TYPE);
                switch(tlv_cali_data.type)
                {
                    case TLV_TYPE_CALI_END:
                        tlv_valid = FALSE;  // invalid tlv type,  continue
                        break;
                    
                    case TLV_TYPE_CALI_ANC_PARAM:
                        app_anc_gain_params_read_from_flash((uint32)env_cali_ptr, tlv_cali_data.len);
                        break;
                    case TLV_TYPE_CALI_ANC_COEFS:
                        app_anc_filter_coefs_read_from_flash((uint32)env_cali_ptr, tlv_cali_data.len);
                        break;
                    
                    case TLV_TYPE_CALI_END2:
                        tlv_loop = FALSE;
                        break;
                    default:break;

                }
                if(tlv_valid&tlv_loop)
                    env_cali_ptr += tlv_cali_data.len;
            }

        }
    }
}
#endif

void app_env_init( void )
{
	uint8_t i;
	app_env_handle_t  env_h = app_env_get_handle();
	memset( env_h, 0xff, sizeof( app_env_t ));
    INFO_PRT("Env.data size:%d\r\n",sizeof(app_env_data_t));
	app_flash_read_env_data();
	app_flash_read_env_cfg();


    if(env_h->env_cfg.used == 1)
    {
        if (!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_UARTDBG))
    	{
            uart_gpio_disable();
		}
    }		
    LOG_I(APP, "env cfg.used:%d\r\n", env_h->env_cfg.used);
    if( env_h->env_cfg.used != 0x01 )
    {
        memset((uint8 *)&env_h->env_cfg, 0, sizeof(app_env_cfg_t));
        env_h->env_cfg.bt_para.bt_flag = APP_BT_FLAG_DEFAULT;
        env_h->env_cfg.bt_para.disconn_action = ENV_ACTION_DISCONNECT_CONN;//ENV_ACTION_DISCONNECT_NONE;
        env_h->env_cfg.bt_para.disconn_retry_count = APP_AUTO_RECONN_RETRY_TIMES;
        env_h->env_cfg.bt_para.disconn_retry_interval = APP_DISCONN_ACTION_TIMEOUT;
        env_h->env_cfg.bt_para.disconn_start = APP_DISCONN_ACTION_FIRST_DELAY;
        env_h->env_cfg.bt_para.match_timeout = -1;    // always try
        env_h->env_cfg.bt_para.pd_cond = 0x3;
        env_h->env_cfg.bt_para.action_disconn = 0;

        env_h->env_cfg.system_para.system_flag = APP_SYSTEM_FLAG_DEFAULT;                       //default auto conn enable and a2dp enable
        env_h->env_cfg.bt_para.auto_conn_count = APP_AUTO_CONN_RETRY_TIMES;
        env_h->env_cfg.bt_para.auto_conn_interval = APP_DISCONN_ACTION_TIMEOUT;

		app_env_set_pin_value(PIN_paMute,PAMUTE_GPIO_PIN);
    #ifdef CONFIG_LINEIN_FUNCTION
        //env_h->env_cfg.system_para.linein_pin = LINEIN_GPIO_DETECT_ID; //env_h->env_cfg.system_para.linein_pin doesn't exist
    #endif
        env_h->env_cfg.system_para.sleep_timeout = SLEEP_TICK_CHECK;
        env_h->env_cfg.system_para.lp_channel = 0;
        env_h->env_cfg.system_para.lp_threshold = SARADC_BELOW_THRESHOLD;
        env_h->env_cfg.system_para.lp_pd_threshold = SARADC_BELOW_THRESHOLD - 100;
        env_h->env_cfg.system_para.lp_interval = APP_LOWPOWER_DETECT_INTERVAL;
//#if (CONFIG_CHARGE_EN == 1)
        env_h->env_cfg.system_para.charger_current = DEFAULT_CHARGE_CURRENT;
//#endif

        env_h->env_cfg.system_para.vol_a2dp  = 10;
        env_h->env_cfg.system_para.vol_hfp     = 10;
        env_h->env_cfg.system_para.vol_wave  = 10;
        env_h->env_cfg.system_para.vol_mic    = 12;
        env_h->env_cfg.system_para.powerdown_timeout = POWER_DOWN_CHECK;



        env_h->env_cfg.button_para.press = BUTTON_PRESS_COUNT_THRE;
        env_h->env_cfg.button_para.repeat = BUTTON_PRESS_REPEAT_THRE;
        env_h->env_cfg.button_para.longp = BUTTON_LONG_PRESS_COUNT_THRE;
        env_h->env_cfg.button_para.vlongp = BUTTON_LONG_PRESS_COUNT_THRE2;
        env_h->env_cfg.button_para.doublep = BUTTON_DOUBLE_CLICK_WAIT_COUNT_THRE;
        for(i=0;i<BUTTON_BT_END;i++) //BUTTON_END
            env_h->env_cfg.button_code[i] = 0;

        env_h->env_cfg.feature.bt_mode = BT_MODE_1V2;
        env_h->env_cfg.feature.vol_mic_dig = 48;
        env_h->env_cfg.feature.a2dp_rf_pwr.big_adj = 0xA;
        env_h->env_cfg.feature.hfp_rf_pwr.big_adj = 0xA;
        env_h->env_cfg.feature.charge_timeout = (3*60*60*100); // default:3 hours
        env_h->env_cfg.system_para.frq_offset = 0x1f;
        env_h->env_cfg.feature.feature_flag = 0x00;
        env_h->env_cfg.feature.feature_flag |= (APP_ENV_FEATURE_FLAG_INQUIRY_ALWAYS | APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE | APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE);
    }
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
    env_h->env_cfg.feature.feature_flag |= APP_ENV_FEATURE_FLAG_VOLUME_SYNC;
#else
    env_h->env_cfg.feature.feature_flag &= ~APP_ENV_FEATURE_FLAG_VOLUME_SYNC;
#endif
    env_h->env_cfg.bt_para.bt_flag |= APP_ENV_BT_FLAG_ADDR_POLL;

    if(env_h->env_cfg.bt_para.auto_conn_interval < APP_DISCONN_ACTION_TIMEOUT)
    	env_h->env_cfg.bt_para.auto_conn_interval = APP_DISCONN_ACTION_TIMEOUT;
    if(env_h->env_cfg.bt_para.disconn_retry_interval < APP_DISCONN_ACTION_TIMEOUT)
    	env_h->env_cfg.bt_para.disconn_retry_interval = APP_DISCONN_ACTION_TIMEOUT;

    LOG_I(APP,"env bt_flag:%08x\r\n",env_h->env_cfg.bt_para.bt_flag);

    if( env_h->env_data.key_index > MAX_KEY_STORE )
    {
        env_h->env_data.key_index = 0;
    }

#ifdef AUTO_CALIBRATE_FREQ_OFFSET
    LOG_I(CALI,"calib_freq:%d\r\n",env_h->env_data.calib_freq_offset);
    if((env_h->env_data.calib_freq_offset&0xC0) != 0xC0)
        env_h->env_cfg.system_para.frq_offset = env_h->env_data.calib_freq_offset&0x3f;
#endif

    app_env_cali_data_init();
    app_env_cali_data_prt();
}

static uint8 s_last_ana_voltage = 0,s_last_dig_voltage = 0;
void app_env_reconfig_ana_voltage(int8_t ana_offset)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 ana_dig_volt = env_h->env_cfg.feature.ana_dig_volt;
    uint8 ana_v = (ana_dig_volt & 0xf0) >> 4;
    ana_v += ana_offset;
    if(ana_v > 7) ana_v = 7;
    if(s_last_ana_voltage == ana_v) return;
    BK3000_set_ana_voltage(ana_v);  
    s_last_ana_voltage = ana_v;
}

void app_env_reconfig_dig_voltage(int8_t dig_offset)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 ana_dig_volt = env_h->env_cfg.feature.ana_dig_volt;
    uint8 dig_v = (ana_dig_volt & 0x0f);
    dig_v += dig_offset;
    if(dig_v > 7) dig_v = 7;
    if(s_last_dig_voltage == dig_v) return;
    BK3000_set_dig_voltage(dig_v);  
    s_last_dig_voltage = dig_v;
}

void app_env_reconfig_ana_reg(int8_t ana_offset,int8_t dig_offset)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 ana_dig_volt = env_h->env_cfg.feature.ana_dig_volt;
    uint8 ana_v = (ana_dig_volt & 0xf0) >> 4;
    uint8 dig_v = (ana_dig_volt & 0x0f);
    ana_v += ana_offset;
    dig_v += dig_offset;
    if(ana_v > 7) ana_v = 7;
    if(dig_v > 7) dig_v = 7;
    
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP || CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP || CONFIG_ANC_ENABLE
    if(CPU_DPLL_CLK>250000000)
    {
        ana_v = 7;
        dig_v = 7;
    }
    else if(CPU_DPLL_CLK>200000000)
    {
        if(ana_v < 6)
            ana_v = 6;
        if(dig_v < 6)
            dig_v = 6;
    }
    #endif
    
    if((s_last_ana_voltage == ana_v) && (s_last_dig_voltage == dig_v)) return;

    //INFO_PRT("Core.voltage:%02x,%02x\r\n",ana_v,dig_v);
    BK3000_set_ana_dig_voltage(ana_v,dig_v);  
    s_last_ana_voltage = ana_v;
    s_last_dig_voltage = dig_v;

}
#if (CONFIG_AUD_EQS_SUPPORT==1)
extern void app_aud_eq_init(void);
extern void app_eq_pram_sel(void);
#endif
void app_poweron_xtal_self_test(void)
{
    FATAL_PRT("XTAL self-test\r\n");
    BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);
    os_delay_ms(1);
    system_set_0x4d_reg(0x3f); // test 6 xtal capacitors
    os_delay_ms(1);
    uint32 PMU_0x0D_BACKUP = REG_PMU_0x0D;
    REG_PMU_0x0D = 0x1;
    REG_PMU_0x0D = PMU_0x0D_BACKUP;
    system_set_0x4d_reg(app_get_best_offset());
    system_apll_toggle();
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
}
void app_env_post_init( void )
{
    app_env_handle_t  env_h = app_env_get_handle();

#ifndef CONFIG_BT_FUNC_INVALID
#ifdef CONFIG_BLUETOOTH_A2DP
//yuan	a2dp_volume_init( env_h->env_cfg.system_para.vol_a2dp );
#endif

#ifdef CONFIG_BLUETOOTH_HFP
#ifdef CONFIG_APP_HALFDUPLEX
    app_hfp_agc_init( env_h->env_cfg.system_para.vol_mic );
    app_hfp_echo_cfg_ptr_set( &env_h->env_cfg.env_echo_cfg );
#endif
#endif
#endif
//yuan    app_wave_file_volume_init(env_h->env_cfg.system_para.vol_wave);	//
//yuan    aud_mute_init();
//yuan    aud_PAmute_operation(1);

    saradc_set_lowpower_para(env_h->env_cfg.system_para.lp_interval,
                                env_h->env_cfg.system_para.lp_threshold,
                                env_h->env_cfg.system_para.lp_pd_threshold );

//yuan    aud_mic_set_volume(env_h->env_cfg.system_para.vol_mic);
#if (CONFIG_PRE_EQ == 1)
	extern aud_pre_equ_t aud_pre_eqe;
#if (CONFIG_AUD_EQS_SUPPORT==1)
	app_aud_eq_init();
	app_eq_pram_sel();
#endif
	extern SAMPLE_ALIGN aud_pre_equ_para_t tbl_eq_coef[];
	
	float V0 = powf( 10, (float)env_h->env_cfg.aud_eq.eq_gain /20 );
	aud_pre_eqe.globle_gain = (uint32)((float)0x4000 * V0);
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
			aud_pre_eqe.online_flag = 1;
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
	LOG_I(APP,"AUD_PRE_EQ EN:0x%x,gain:%d,cnt:%d\r\n",env_h->env_cfg.aud_eq.eq_enable,env_h->env_cfg.aud_eq.eq_gain,aud_pre_eqe.totle_EQ);
#endif

#if (CONFIG_HFP_SPK_EQ== 1)
	extern hfp_spk_equ_t hfp_spk_eqe;
	extern SAMPLE_ALIGN hfp_spk_equ_para_t tbl_hfp_spk_eq_coef[];

	float V1 = powf( 10, (float)env_h->env_cfg.hfp_cfg.hfp_spk_eq_gain /20 );
	hfp_spk_eqe.globle_gain = (uint32)((float)0x4000 * V1);
	hfp_spk_eqe.online_flag = 0;
	hfp_spk_eqe.totle_EQ = 0;
	uint16 j;
	for(j=0;j<CON_HFP_SPK_EQ_BANDS;j++)
	{
		if((1<<j)&env_h->env_cfg.hfp_cfg.hfp_spk_eq_enable)
		{
			tbl_hfp_spk_eq_coef[j].a[0] = env_h->env_cfg.hfp_cfg.hfp_spk_eq_para[j].a[0];
			tbl_hfp_spk_eq_coef[j].a[1] = env_h->env_cfg.hfp_cfg.hfp_spk_eq_para[j].a[1];
			tbl_hfp_spk_eq_coef[j].b[0] = env_h->env_cfg.hfp_cfg.hfp_spk_eq_para[j].b[0];
			tbl_hfp_spk_eq_coef[j].b[1] = env_h->env_cfg.hfp_cfg.hfp_spk_eq_para[j].b[1];
			tbl_hfp_spk_eq_coef[j].b[2] = env_h->env_cfg.hfp_cfg.hfp_spk_eq_para[j].b[2];
			hfp_spk_eqe.totle_EQ++;
			hfp_spk_eqe.online_flag = 1;
		}
		else
		{
			tbl_hfp_spk_eq_coef[j].a[0] = 0;
			tbl_hfp_spk_eq_coef[j].a[1] = 0;
			tbl_hfp_spk_eq_coef[j].b[0] = 0x100000;
			tbl_hfp_spk_eq_coef[j].b[1] = 0;
			tbl_hfp_spk_eq_coef[j].b[2] = 0;
		}
	
	}
	LOG_I(APP,"HFP_SPK_EQ EN:0x%x,gain:%d,cnt:%d\r\n",env_h->env_cfg.hfp_cfg.hfp_spk_eq_enable,env_h->env_cfg.hfp_cfg.hfp_spk_eq_gain,hfp_spk_eqe.totle_EQ);
#endif

#if (CONIFG_HFP_MIC_EQ== 1)
		extern hfp_mic_equ_t hfp_mic_eqe;
		extern SAMPLE_ALIGN hfp_mic_equ_para_t tbl_hfp_mic_eq_coef[];
	
		float V2 = powf( 10, (float)env_h->env_cfg.hfp_cfg.hfp_mic_eq_gain /20 );
		hfp_mic_eqe.globle_gain = (uint32)((float)0x4000 * V2);
		hfp_mic_eqe.online_flag = 0;
		hfp_mic_eqe.totle_EQ = 0;
		uint16 k;
		for(k=0;k<CON_HFP_MIC_EQ_BANDS;k++)
		{
			if((1<<k)&env_h->env_cfg.hfp_cfg.hfp_mic_eq_enable)
			{
				tbl_hfp_mic_eq_coef[k].a[0] = env_h->env_cfg.hfp_cfg.hfp_mic_eq_para[k].a[0];
				tbl_hfp_mic_eq_coef[k].a[1] = env_h->env_cfg.hfp_cfg.hfp_mic_eq_para[k].a[1];
				tbl_hfp_mic_eq_coef[k].b[0] = env_h->env_cfg.hfp_cfg.hfp_mic_eq_para[k].b[0];
				tbl_hfp_mic_eq_coef[k].b[1] = env_h->env_cfg.hfp_cfg.hfp_mic_eq_para[k].b[1];
				tbl_hfp_mic_eq_coef[k].b[2] = env_h->env_cfg.hfp_cfg.hfp_mic_eq_para[k].b[2];
				hfp_mic_eqe.totle_EQ++;
				hfp_mic_eqe.online_flag = 1;
			}
			else
			{
				tbl_hfp_mic_eq_coef[k].a[0] = 0;
				tbl_hfp_mic_eq_coef[k].a[1] = 0;
				tbl_hfp_mic_eq_coef[k].b[0] = 0x100000;
				tbl_hfp_mic_eq_coef[k].b[1] = 0;
				tbl_hfp_mic_eq_coef[k].b[2] = 0;
			}
		
		}
		LOG_I(APP,"HFP_MIC_EQ EN:0x%x,gain:%d,cnt:%d\r\n",env_h->env_cfg.hfp_cfg.hfp_mic_eq_enable,env_h->env_cfg.hfp_cfg.hfp_mic_eq_gain,hfp_mic_eqe.totle_EQ);
#endif

#ifndef CONFIG_BT_FUNC_INVALID
    app_env_rf_pwr_set(0);
    bt_app_get_conn_addr_from_env();
#endif
    //system_set_0x4d_reg(app_get_best_offset());
}
void app_env_core_pm_init(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint32 t_vusb_stat_rd = VUSB_REG_STAT_RD;
    t_vusb_stat_rd &= ~(0x07 << SFT_VUSB_MORETHAN_1V); // High level trig;
    //gpio_output(GPIO16,1);
    //app_bt_ldo_init();
    //gpio_output(GPIO16,0);

    vusb_int_level(t_vusb_stat_rd);
    vusb_int_enable(MSK_PMU_0x00_VUSB_1V_INTR | MSK_PMU_0x00_VUSB_2V_INTR | MSK_PMU_0x00_VUSB_4V_INTR);

    if (0x01 == env_h->env_cfg.used)
    {
        if( env_h->env_data.lang_sel >= 0 && env_h->env_data.lang_sel <= 4 )
             env_h->env_cfg.wave_lang_sel = env_h->env_data.lang_sel;
        else
            env_h->env_data.lang_sel = env_h->env_cfg.wave_lang_sel;

        system_dbuck_enable(!!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE));
        system_abuck_enable(!!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE));
    }
    else
    {
        system_dbuck_enable(SYS_CFG_BUCK_ON);
        system_abuck_enable(SYS_CFG_BUCK_ON);
    }
    //app_env_reconfig_ana_reg(0,0);
}

static void app_set_chip_btaddr(btaddr_t *btaddr )
{
    uint8_t buffer[HCI_COMMAND_HEAD_LENGTH+BLUETOOTH_BTADDR_SIZE];
    HCI_PACKET *pkt = (HCI_PACKET *)(&buffer[0]);

    pkt->code       = TRA_HCIT_COMMAND;
    pkt->opcode.ogf = VENDOR_SPECIFIC_DEBUG_OGF;
    pkt->opcode.ocf = 0X1A; // TCI_SET_LOCAL_BD_ADDR&0X3FF;
    pkt->total      = sizeof(btaddr_t);
    memcpy(pkt->param, btaddr->b, sizeof(btaddr_t));

    uart_send_poll(buffer, sizeof(buffer));
}

void app_init(void)
{
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    memset(app_h, 0, sizeof(APP_SYSTEM_T));
#ifndef CONFIG_BT_FUNC_INVALID
    /* bt_app_management */
    bt_app_entity_init();
#endif
    app_wave_file_play_init();


#ifndef CONFIG_BT_FUNC_INVALID
    if( env_h->env_cfg.used == 0x01 )
    {
        memcpy( (char *)&btaddr_def, (char *)&env_h->env_cfg.bt_para.device_addr, 6 );
    }
    else
    {
        memcpy((char *)&env_h->env_cfg.bt_para.device_addr,(char *)&btaddr_def, 6 );
    }
#endif
    app_h->led_event          = LED_EVENT_END;
    app_h->press_thre         = env_h->env_cfg.button_para.press;
    app_h->repeat_thre        = env_h->env_cfg.button_para.repeat;
    app_h->long_thre          = env_h->env_cfg.button_para.longp;
    app_h->vlong_thre         = env_h->env_cfg.button_para.vlongp;
    app_h->double_thre        = env_h->env_cfg.button_para.doublep;
    app_h->low_detect_count   = env_h->env_cfg.system_para.lp_interval;
    app_h->powerdown_count    = env_h->env_cfg.system_para.powerdown_timeout;
    app_h->low_detect_channel = env_h->env_cfg.system_para.lp_channel;
    app_h->volume_store       = 0;
    app_h->mic_volume_store   = env_h->env_cfg.system_para.vol_mic;
    app_h->linein_vol         = env_h->env_cfg.system_para.vol_a2dp;
    app_h->pause_powerdown_count = env_h->env_cfg.system_para.pause_powerdown_timeout;
    LOG_I(APP,"system_flag: %x,%x\r\n",env_h->env_cfg.system_para.system_flag,env_h->env_cfg.used);
    //app_set_chip_btaddr(&btaddr_def);

    app_button_type_set( BUTTON_TYPE_NON_HFP );

#ifndef CONFIG_BT_FUNC_INVALID
    app_h->sys_work_mode = SYS_WM_BT_MODE;
    app_h->bt_mode       = BT_DISCONNECTED;

    if( env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_AUTO_CONN)
    {
        app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION,1);
    }
#endif
    app_h->btn_init_map = button_get_gpio_map(NULL);

//    app_bt_flag2_set(APP_FLAG2_DAC_OPEN, 0);
}

void app_post_init( void )
{
    app_handle_t app_h = app_get_sys_handler();

    LOG_I(APP,"app_post_init\r\n");
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint8_t idx = 0;
    for(idx = 0; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
        bt_sniff_free_st(idx);
#endif
#if (CONFIG_DRIVER_OTA == 1)
    jtask_init( &app_h->ota_reboot_task, J_TASK_TIMEOUT );
#endif
    jtask_init( &app_h->app_auto_con_task, J_TASK_TIMEOUT );
    jtask_init( &app_h->app_reset_task, J_TASK_TIMEOUT );
    jtask_init( &app_h->app_match_task, J_TASK_TIMEOUT );
    jtask_init( &app_h->app_audio_task, J_TASK_TIMEOUT );
    jtask_init(&app_h->app_a2dp_task,J_TASK_TIMEOUT);
    jtask_init( &app_h->app_common_task, J_TASK_TIMEOUT );
    jtask_init( &app_h->app_save_volume_task, J_TASK_TIMEOUT );
    jtask_init( &app_h->app_only_a2dp_hfp_link_to_scan_task, J_TASK_TIMEOUT );
#if (defined(BT_SD_MUSIC_COEXIST))
    jtask_init(&app_h->app_coexist_task, J_TASK_TIMEOUT );
#endif

#ifndef CONFIG_BT_FUNC_INVALID
    app_set_chip_btaddr(&btaddr_def);
    app_bt_button_setting();
#endif
    app_led_init();
    //app_set_led_event_action( LED_EVENT_POWER_ON );
}

void app_reset(void)
{
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    bt_unit_enable( app_h->unit );

    if( env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_AUTO_CONN)
        app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION,1);

    app_set_chip_btaddr(&btaddr_def);

    os_printf("app_reset\r\n");
    
    if(app_is_bt_mode())
    {
        //extPA_close(1);
    }
    
    app_bt_button_setting();
    app_led_init();

    if(app_env_get_pin_enable(PIN_paMute))
    {
        gpio_config(app_env_get_pin_num(PIN_paMute), app_env_get_pin_valid_level(PIN_paMute)?0:3 );
    }

    if(app_env_get_pin_enable(PIN_lineDet))
    {
        gpio_config(app_env_get_pin_num(PIN_lineDet), app_env_get_pin_valid_level(PIN_lineDet)?0:3 );
    }
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_XTAL, 0);
#endif
}

int env_get_auto_conn_info_max_id(void)
{
    int i;
    uint32_t id;
    app_env_handle_t env_h = app_env_get_handle();

    id = 0;
    for(i = 0; i < MAX_KEY_STORE; i++)
    {
        if((env_h->env_data.key_pair[i].used == 0x01)||(env_h->env_data.key_pair[i].used == 0x02))
        {
            id = i;
        }
    }

    return id;
}

int env_get_auto_conn_info(int start_index, int *id)
{
    int i;
    app_env_handle_t env_h = app_env_get_handle();

    if(start_index > MAX_KEY_STORE)
    {
        goto seek_over;
    }

    for(i = start_index; i < MAX_KEY_STORE; i++)
    {
        if((env_h->env_data.key_pair[i].used == 0x01) || (env_h->env_data.key_pair[i].used == 0x02))
        {
            *id = i;

            return 0;
        }
    }

seek_over:
    return -1;
}

int app_env_key_stored( btaddr_t *addr )
{
    int i;
    app_env_handle_t  env_h = app_env_get_handle();

    for( i = 0; i < MAX_KEY_STORE; i++ )
    {
        if((env_h->env_data.key_pair[i].used != 0xFF)
        && (btaddr_same( addr, &(env_h->env_data.key_pair[i].btaddr))) )
        return i+1;
    }
    return 0;
}

int app_env_key_delete( btaddr_t *addr )
{
    int i;
    app_env_handle_t  env_h = app_env_get_handle();

    for( i = 0; i < MAX_KEY_STORE; i++ )
    {
        if((env_h->env_data.key_pair[i].used != 0xFF)
        && (btaddr_same( addr, &(env_h->env_data.key_pair[i].btaddr))) )
        {
            memset(env_h->env_data.key_pair[i].linkkey,0xff,HCI_KEY_SIZE);
            return 1;
        }
    }
    return 0;
}

int app_env_keypair_used_delete( btaddr_t *addr )
{
    int i;
    app_env_handle_t  env_h = app_env_get_handle();

    for( i = 0; i < MAX_KEY_STORE; i++ )
    {
        if((env_h->env_data.key_pair[i].used != 0xFF)
        && (btaddr_same( addr, &(env_h->env_data.key_pair[i].btaddr))) )
        {
            env_h->env_data.key_pair[i].used = 0;
            return 1;
        }
    }
    return 0;
}

int8_t app_get_env_conn_retry_count(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    return env_h->env_cfg.bt_para.disconn_retry_count;    
}

int8_t app_get_env_conn_auto_count(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    return env_h->env_cfg.bt_para.auto_conn_count;    
}

uint32_t app_get_env_profile_attrs(void)
{
    return BT_ALL_PROFILE_CONNECTED_SET;
}

int app_get_env_key_num_total( void )
{
    int i = 0, n = 0;
    app_env_handle_t  env_h = app_env_get_handle();

    for(i = 0; i < MAX_KEY_STORE; i++)
    {
        if(env_h->env_data.key_pair[i].used != 0xFF)
            n++;
    }
    return n;
}

int app_get_env_key_num( void )
{
    int i = 0, n = 0;
    app_env_handle_t  env_h = app_env_get_handle();

    for(i = 0; i < MAX_KEY_STORE; i++)
    {
        if((env_h->env_data.key_pair[i].used == 0x01)
        || (env_h->env_data.key_pair[i].used == 0x02))
            n++;
    }

    return n;
}

int app_get_active_linkey( int Seq )
{
    int i,n=0;
    app_env_handle_t  env_h = app_env_get_handle();
    if(Seq == 0)  // First active linkey
    {
        for( i = 0; i < MAX_KEY_STORE; i++ )
        {
	        if(env_h->env_data.key_pair[i].used == 0x01)
	        {
	            n = i+1;
		        break;
	        }
        }
    }
    else  // Second active linkey
    {
        for( i = 0; i < MAX_KEY_STORE; i++ )
        {
            if(env_h->env_data.key_pair[i].used == 0x02)
	        {
                n = i+1;
                break;
	        }
        }
    }
    return n;
}

btaddr_t *app_get_active_remote_addr(uint8_t seq )
{
    int i;
    app_env_handle_t  env_h = app_env_get_handle();
    if(seq == 0)  // First active linkey
    {
        for( i = 0; i < MAX_KEY_STORE; i++ )
        {
            if(env_h->env_data.key_pair[i].used == 0x02)
            {
                return  &(env_h->env_data.key_pair[i].btaddr);
            }
        }
    }
    else  // Second active linkey
    {
        for( i = 0; i < MAX_KEY_STORE; i++ )
        {
            if(env_h->env_data.key_pair[i].used == 0x01)
            {
                return  &(env_h->env_data.key_pair[i].btaddr);            
            }
        }
    }
    return NULL;
}

/* 0x02 or 0x01 keys will be auto-connected. */
/* For connection */
/* The latest connection key->used is always set to 0x02. If 0x02 or 0x01 already exsits, decrement them by 1.*/
/* For disconnection, if key->used is 0x01, no change; if key->used is 0x02, and key->used 0x01 exists,swap. */
int app_env_write_action( btaddr_t *raddr,uint8_t connected)
{
    app_env_handle_t  env_h = app_env_get_handle();
    int index = 0;
    app_env_key_pair_t *keytmp;
    int8_t id=bt_app_entity_find_id_from_raddr(raddr);

#ifdef CONFIG_PRODUCT_TEST_INF
    if(env_h->env_cfg.feature.feature_flag & APP_ENV_FEATURE_FLAG_AUTCONN_TESTER)
    {
        if(btaddr_same(raddr, &env_h->env_cfg.feature.tester_bt_addr))
            return 1;
    }
#endif

	if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
    {
        if(0x01 == connected)
        {
            index = app_env_key_stored(raddr);

            if(index > 0)
            {
                int i = 0;
	            for(i=0;((i < MAX_KEY_STORE) && (env_h->env_data.key_pair[i].used != 0xFF));i++)
                {
                   keytmp = &env_h->env_data.key_pair[i];
                    keytmp->used = 0x00;
                }
                keytmp = &env_h->env_data.key_pair[index-1];
                keytmp->used = 0x02; // for 1V1 auto connenction when powerup
                keytmp->aud_dc_cali_data = get_audio_dc_calib_data();
            }
        }
        memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)raddr, 6);
        app_env_write_flash(connected, id);
        return 0;
    }   
     else if(app_check_bt_mode(BT_MODE_1V2))
    {
        int index1, index2;
        index1= app_get_active_linkey(0);
        index2 = app_get_active_linkey(1);
        index = app_env_key_stored(raddr);
        LOG_I(CONN,"+++app_env_write_action(%d),%d,%d,%d\r\n",connected,index,index1,index2);
        memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)raddr, 6);
        //app_env_write_flash(connected);
        //memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)raddr, 6);
        if (0x01 == connected)
        {
            // No need to update env data
            if (index == index2)
                return 0;

            if (index2)
            {
                env_h->env_data.key_pair[index2-1].used = 0x01;

                if (index1)
                {
                    env_h->env_data.key_pair[index1-1].used = 0x00;
                }
            }

            if(index > 0)
            {
    	        keytmp = &env_h->env_data.key_pair[index-1];
    	        keytmp->used = 0x02;//connected;
    	        keytmp->aud_dc_cali_data = get_audio_dc_calib_data();
            }
            else if(index == 0)
            {
                 LOG_I(CONN,"++app_env_write_action,can't find the addr in key list\r\n");
            }
        }
        else if(0 == connected)
        {
            if ((index == index1) || !index1)
                return 0;

            if(index == index2)
            {
                if(hci_check_acl_link(&env_h->env_data.key_pair[index1-1].btaddr))
                {
                    env_h->env_data.key_pair[index2-1].used = 0x01;
                    env_h->env_data.key_pair[index1-1].used = 0x02;
                }
                else
                {
                    return 0;
                }
            }
            /*
            // one situation: new phone connect BK, get linkey,but disconnected. need store linkey
            else
            {
                return 0;
            }*/
        }
        else
            return 1;
    }
    return 1;
}
int app_env_write_flash(uint8_t connected, uint8_t id)
{
    //app_env_handle_t  env_h = app_env_get_handle();
    MSG_T msg;
    // No Flash Write if POWER DOWN is happening, otherwise the Flash Content may get corrupted
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN)|| app_charge_is_powerdown())
        return 0;

    if(app_bt_rmt_device_type_get(id) >= BT_DEVICE_TV)
    {
        if(app_bt_flag1_get(APP_TV_WORKING_FLAG_SET))
        {
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = connected;
            msg.arg2 = id;
            msg.hdl = 0;
            msg_lush_put(&msg);
            return 0;        
        }
    }
    else
    {
        if(app_bt_flag1_get(APP_AUDIO_WORKING_FLAG_SET))
        {
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = connected;
            msg.arg2 = id;
            msg.hdl = 0;
            msg_lush_put(&msg);
            return 0;
        }
    }
    LOG_I(ENV,"app_env_write_flash(%d): %d\r\n",connected, id);
    //flash_erase_sector(FLASH_ENVDATA_DEF_ADDR, FLASH_ERASE_4K);
    //flash_write_data( (uint8_t *)&(env_h->env_data), FLASH_ENVDATA_DEF_ADDR, sizeof(app_env_data_t));
    app_flash_write_env_data();
    return 1;
}
/*
 * env data sector len: 4096 = 16 * ENV_BLOCK_LEN,block len = 256 > sizeof(env_data)
 * Tag:
 *    0x5a: this block is valid env data;
 *    0x00: the block is invalid env data;
 *    0xff: the block has not been used;
 */
void app_flash_read_env_data(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint8_t i;
    /* searching valid env_data and read out */
    for(i=0;i<16;i++)
    {
        flash_read_data((uint8_t *)&env_h->env_data, (uint32_t)FLASH_ENVDATA_DEF_ADDR + i*ENV_BLOCK_LEN, sizeof(app_env_data_t));
        if(env_h->env_data.env_tag == 0x5a || env_h->env_data.env_tag == 0xff)  // valid env data or have not stored evv_data
        {
            break;
        }
        else
        {
            memset((uint8_t *)&env_h->env_data,0xff,sizeof(app_env_data_t));
        }
    }
}

void app_flash_write_env_data(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    uint8_t buff[ENV_BLOCK_LEN] = {0};
    uint8_t i = 0;

    /* searching valid space for save this env data */
    for(i = 0; i < 16; i++)
    {
        flash_read_data(buff, (uint32_t)FLASH_ENVDATA_DEF_ADDR + i*ENV_BLOCK_LEN, ENV_BLOCK_LEN);
        if(buff[0] == 0x5a)  // this block has been used
        {
            buff[0] = 0;
            flash_write_data(buff,(uint32_t)FLASH_ENVDATA_DEF_ADDR + i*ENV_BLOCK_LEN,1); // write 1st byte '0' to override this tag
        }
        else if(buff[0] == 0xff) // this block has been erased and not been used;
        {
            break;
        }
    }

    if(i == 16) /* have not searched valid space,then erase this sector and write env data to 1st block; */
    {

        flash_erase_sector(FLASH_ENVDATA_DEF_ADDR, FLASH_ERASE_4K);
        env_h->env_data.env_tag = 0x5a;
        flash_write_data( (uint8_t *)&(env_h->env_data), FLASH_ENVDATA_DEF_ADDR, sizeof(app_env_data_t));
    }
    else /* write env data to the i block */
    {
        env_h->env_data.env_tag = 0x5a;
        flash_write_data( (uint8_t *)&(env_h->env_data), FLASH_ENVDATA_DEF_ADDR + i*ENV_BLOCK_LEN, sizeof(app_env_data_t));
    }
}

/*
	function_name:app_env_get_flash_addr
	input:0=env_cfg_flash_addr
		1=env_data_flash_addr
		2=env_calibration_flash_addr
	output:flash_addr
*/
uint32_t app_env_get_flash_addr(t_TYPE_SECTOR type)
{
    uint32_t flash_def_addr = 0;

    switch(type)
    {
        case TLV_SECTOR_ENVCFG:
            flash_def_addr = FLASH_ENVCFG_DEF_ADDR_ABS;
            break;		
        case TLV_SECTOR_ENVDATA:
            flash_def_addr = FLASH_ENVDATA_DEF_ADDR_ABS;
            break;
        case TLV_SECTOR_ENVCALI:
            flash_def_addr = FLASH_ENVCALI_DEF_ADDR_ABS;
            break;
        case TLV_SECTOR_ENVEND:
            flash_def_addr = FLASH_ENVEND_DEF_ADDR_ABS;
            break;
        default:
            flash_def_addr = FLASH_ENVDATA_DEF_ADDR_ABS;
            break;
    }
    return flash_def_addr;
}

void app_flash_read_env_cfg(void)
{
	uint32_t flash_def_addr = app_env_get_flash_addr(TLV_SECTOR_ENVCFG);

	if(app_check_tlv_data_chip_magic((uint8_t *)flash_def_addr ,TLV_SECTOR_ENVCFG) == TLV_RST_TRUE)
	{
		flash_def_addr += sizeof(env_chip_magic);
		app_get_tlv_data((uint8_t *)flash_def_addr ,0);
	}
}

int app_env_store_autoconn_info( btaddr_t * remote_addr,  uint8_t link_key[16] )
{
    int ret = 0,i;
    uint8_t cmd[24];
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();
    app_env_key_pair_t *key;

    if(env_h->env_data.key_index>MAX_KEY_STORE)  // if function app_env_clear_all_key_info is called
        env_h->env_data.key_index = 0;

    key =  &env_h->env_data.key_pair[env_h->env_data.key_index];

    app_env_key_pair_t *keytmp;
    i = app_env_key_stored( remote_addr );
    if( i > 0 )
    {
        key = &env_h->env_data.key_pair[i-1];
        if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
        {
            while(i--)
            {
                keytmp = &env_h->env_data.key_pair[i];
                keytmp->used = 0;
            }
        }
    }
    else
    {
        btaddr_t addr;
        memset((uint8_t *)&addr,0xff,6);

        if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
        {
            i = env_h->env_data.key_index;
            while(i--)
            {
                keytmp = &env_h->env_data.key_pair[i];
                keytmp->used = 0;
            }
        }

        i = env_h->env_data.key_index;
        if(!btaddr_same( &addr, &(env_h->env_data.key_pair[i].btaddr)))
        {
            LOG_I(ENV, "send delete cmd,index:%d\r\n",i);
            memcpy( &cmd[0], (uint8_t *)&(env_h->env_data.key_pair[i].btaddr), sizeof(btaddr_t));
            cmd[6] = 0x00;
            hci_send_cmd( sys_hdl->unit, HCI_CMD_DELETE_STORED_LINK_KEY,cmd, 7 );
        }
        env_h->env_data.key_index ++;
        if( env_h->env_data.key_index >= MAX_KEY_STORE )
        {
            env_h->env_data.key_index = 0;
        }

    }
    memcpy( (uint8_t *)&key->linkkey, link_key, LINK_KEY_SIZE);
    memcpy( (uint8_t *)&key->btaddr, (uint8_t *)remote_addr, BYTE_BD_ADDR_SIZE );
    key->crystal_cali_data = system_get_0x4d_reg();
	key->a2dp_src_uclass = bt_acl_con_get_specific_uclass();
	
    cmd[0] = 1;
    memcpy(&cmd[1], (uint8_t *)remote_addr, sizeof(btaddr_t));
    memcpy(&cmd[7], (uint8_t *)link_key, 16);

    hci_send_cmd(sys_hdl->unit,HCI_CMD_WRITE_STORED_LINK_KEY,cmd,23);

    if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
        key->used = 0x01;
    else
        key->used = 0;
     LOG_I(ENV, "key stored,env key index:%d\r\n",env_h->env_data.key_index);

#ifdef CONFIG_ACTIVE_SSP
    MSG_T msg;
    msg.id = MSG_ENV_WRITE_ACTION;
    msg.arg = 0x01;
    msg.hdl = 0;
    msg_lush_put(&msg);
#endif

    return ret;
}


btaddr_t* app_env_get_key_free( void )
{
    app_env_handle_t  env_h = app_env_get_handle();
    app_env_key_pair_t *key = &env_h->env_data.key_pair[env_h->env_data.key_index];

    if( key->used == 0x01 || key->used == 0x02)
        return &(key->btaddr);
    else
        return NULL;
}

void app_env_unit_info_init( char *name, uint8_t *dev_class, char *pin )
{
    app_env_handle_t env_h = app_env_get_handle();

    if( env_h->env_cfg.used == 0x01 )
    {
		memcpy( name, (char *)env_h->env_cfg.bt_para.device_name, 32);
        memcpy( pin, (char *)env_h->env_cfg.bt_para.device_pin, 16 );
        *dev_class = env_h->env_cfg.bt_para.device_class;
    }

    return;

}

btkey_t * app_env_get_linkkey( btaddr_t *addr )
{
    app_env_handle_t env_h = app_env_get_handle();
    int i;
    int j;
    char  linkkey[34]={0};
    btkey_t key;

    memset(key,0xff,HCI_KEY_SIZE);

    i = app_env_key_stored(addr);

    if( i != 0 )
    {
        if(memcmp(env_h->env_data.key_pair[i-1].linkkey,key,HCI_KEY_SIZE))
        { 
            for( j=0; j < 16; j++)
                sprintf( &linkkey[j*2], "%02x", env_h->env_data.key_pair[i-1].linkkey[j] );
           LOG_I(APP," linkkey: 0x%s\r\n", linkkey);

            return (btkey_t *)env_h->env_data.key_pair[i-1].linkkey;
        }
    }
    LOG_I(APP," linkkey lost\r\n");
    return NULL;
}

void app_env_clear_key_info( btaddr_t *addr )
{
    app_env_handle_t env_h = app_env_get_handle();
    int i;

    i = app_env_key_stored(addr);

    if( i != 0 )
    {
        env_h->env_data.key_pair[i-1].used = 0;
    }

    return;
}

void app_env_clear_all_key_info( void )
{
    app_env_handle_t env_h = app_env_get_handle();

    memset( (uint8_t *)&env_h->env_data, 0xff, sizeof( env_h->env_data ));
    /*
    flash_erase_sector(FLASH_ENVDATA_DEF_ADDR, FLASH_ERASE_4K);
    flash_write_data( (uint8_t *)&(env_h->env_data), FLASH_ENVDATA_DEF_ADDR, sizeof(app_env_data_t));
    */

    app_env_write_flash(0, 0xff);
    return;
}

void app_env_restore_new_product(void)
{
    flash_erase_sector(FLASH_ENVDATA_DEF_ADDR, FLASH_ERASE_4K);
    BK3000_setting_w4_reset();
    BK3000_wdt_reset();
    while(1);
}

static app_wave_info_t *app_env_get_wave_lang_handle( int lang, int wave_id )
{
    app_wave_info_t *wave_h;
    app_env_handle_t env_h = app_env_get_handle();

    switch( lang )
    {
        case 0:
            wave_h = &env_h->env_cfg.wave_info[wave_id];
            break;
        case 1:
            wave_h = &env_h->env_cfg.wave_info1[wave_id];
            break;
        case 2:
            wave_h = &env_h->env_cfg.wave_info2[wave_id];
            break;
        case 3:
            wave_h = &env_h->env_cfg.wave_info3[wave_id];
            break;
        default:
            wave_h = NULL;
            break;
    }

    return wave_h;
}

int app_env_get_wave_page_index( int wave_id )
{
    app_env_handle_t env_h = app_env_get_handle();
    app_wave_info_t *wave_h;

    wave_h = app_env_get_wave_lang_handle( env_h->env_cfg.wave_lang_sel, wave_id );

    if( wave_h == NULL )
        return -1;
#ifdef	CONFIG_SBC_PROMPT
	if((wave_h->used < INTER_WAV)
		||(wave_h->used > EXT_SBC))
		return -1;
#else
	if(wave_h->used != 0x01)
        return -1;
#endif
    return wave_h->page_index;
}

int app_env_get_wave_type( int wave_id )
{
    app_env_handle_t env_h = app_env_get_handle();
    app_wave_info_t *wave_h;
//wave_h->type = wave_h->used + 1;
    wave_h = app_env_get_wave_lang_handle( env_h->env_cfg.wave_lang_sel, wave_id );

    if( wave_h == NULL )
        return -1;
#ifdef	CONFIG_SBC_PROMPT
	if((wave_h->used < INTER_WAV+1)
		||(wave_h->used > EXT_SBC+1))
        return -1;
	return wave_h->used - 1;
#else
	if( (wave_h->used != 0x01) )
		return -1;

    return 0;
#endif

}

uint8_t app_env_get_wave_max_lang_num(void)
{
	uint8_t i,j,lang,num;
	static uint8_t get_lang=0;
	static uint8_t max_lang=0;
	app_env_handle_t env_h = app_env_get_handle();

	if (get_lang == 0)
	{
		get_lang = 1;
		lang = env_h->env_cfg.wave_lang_sel; 
		for (i=0; i<4; i++)
		{
			num = 0;
			for (j=0; j<INVALID_WAVE_EVENT; j++)
			{
				env_h->env_cfg.wave_lang_sel  = i;
				if ((app_env_get_wave_page_index(j)>0)
					&& (app_env_get_wave_type(j) != -1)) 
					num ++;	
			}
			if (num)
				max_lang ++;
		}
		env_h->env_cfg.wave_lang_sel = lang;
	}

	return max_lang;
}

void app_set_auto_powerdown(void)
{
	flag_auto_powerdown = 1;
}

uint8_t app_get_auto_powerdown(void)
{
	return flag_auto_powerdown;
}

extern uint8  is_charge_repeat;
void app_env_power_on_check(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    if ( env_h->env_cfg.used == 0x01)
    {
        #if (REMOTE_RANGE_PROMPT == 1)
        app_handle_t sys_hdl = app_get_sys_handler();
        if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE))
        {
            //UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);
            sys_hdl->flag_soft_reset = 1;
            LOG_I(ENV,"Remote Soft_Reset:0x%x\r\n",GET_PWR_DOWN_FLAG(0xff));
            return;
        }
        #endif
        if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET))
        {
            //UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET);
            LOG_I(ENV,"Soft_Reset:0x%x\r\n",GET_PWR_DOWN_FLAG(0xff));
            return;
        }
        uint16 volt = saradc_transf_adc_to_vol(saradc_get_value());
        uint16 pd_volt = env_h->env_cfg.system_para.lp_pd_threshold + 100;//Inc:pd_volt+0.1v, to prevent the POR reset of the battery
        LOG_I(VBAT,"VBAT:%d,%d,%d\r\n",saradc_calibration_end(),volt,pd_volt);
        if(!app_charge_is_usb_plug_in() && (volt < pd_volt))
        {
            app_powerdown_action();
        }
        if(app_charge_is_usb_plug_in() 
			&& GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP))
        {
            env_charge_cali_data_t *charge_cali = app_get_env_charge_cali_data();

            if(app_charge_is_wakeup_for_charge_full()&&(volt > SARADC_VBAT_CHARGE_FULL))   
                app_powerdown_action();    
            else if(app_charge_is_wakeup_for_charge_full()&&(volt < 4200))
            {
                if(REG_PMU_0x0D!=0)  // REG_PMU_0x0D backup vlcf value;
                {
                    if(CONFIG_RECHARGER_VLCF_OFFSET + REG_PMU_0x0D>=0x7f)
                        charge_cali->charger_vlcf = 0x7f;
                    else
                        charge_cali->charger_vlcf = CONFIG_RECHARGER_VLCF_OFFSET + REG_PMU_0x0D;
                    is_charge_repeat = 1;
                }
                else
                    WNG_PRT("CHARGE ERR!!!!\r\n"); 
            }
        }
        #if 0
        if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP))
        {
            if((app_charge_is_usb_plug_in())
            /*&&(IS_USB_CHARGE_FULL)*/)
            {//wakeup for charge full ,  shutdown quickly.
                UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP);
                app_powerdown_action();
            }
        }
        #endif
        if(HW_PWR_KEY!=app_env_check_pwrCtrl_mode())
        {
            //config pin
            if(app_env_get_pin_enable(PIN_pwrBtn))
            {
                if (app_env_get_pin_valid_level(PIN_pwrBtn))
                    gpio_config(app_env_get_pin_num(PIN_pwrBtn), 0 );
                else
                    gpio_config(app_env_get_pin_num(PIN_pwrBtn), 3 );
            }
            
        #if POWER_ON_OUT_CHARGE_ENABLE
            if(app_env_get_pin_enable(PIN_ioDet))
            {
                gpio_config(app_env_get_pin_num(PIN_ioDet), 4);
            }
        #endif
            
            Delay(100);
            //---------------------
            if(SW_PWR_KEY_SWITCH ==app_env_check_pwrCtrl_mode())
            {
                if(app_env_get_pin_valid_level(PIN_ioDet))
                {
                    gpio_config( app_env_get_pin_num(PIN_ioDet), 0 );
                }
                else
                    gpio_config( app_env_get_pin_num(PIN_ioDet), 3 );
                //os_printf("SW_PWR_KEY_SWITCH:%d,%d,%d\r\n", app_env_get_pin_num(PIN_ioDet),app_env_get_pin_valid_level(PIN_ioDet),gpio_input(app_env_get_pin_num(PIN_ioDet)));               
            }
            else
            {
                if( 0==gpio_input(app_env_get_pin_num(PIN_pwrBtn)))
                {
                    app_bt_flag2_set(APP_FLAG2_5S_PRESS,1);
                }
            }
                
            uint32 t0,t1;
            t0 = os_get_tick_counter();
            
            while (1)
            {
            #if POWER_ON_OUT_CHARGE_ENABLE
                //-------------------------
                //AON_flag bypass pwr on delay
                if(GET_PWR_DOWN_FLAG(AUTO_PWR_ON_FLAG|PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO))
                {
                    app_bt_flag2_set(APP_FLAG2_5S_PRESS, 0);
                    if(app_charge_is_usb_plug_in() && !app_env_check_Charge_Mode_HighEfficiency())
                        app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN, 1);
                    os_printf("gpio power on:0x%x\r\n", GET_PWR_DOWN_FLAG(0xff));
                    //UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO);
                    break;
                }
                //----------------------------
                //usb plug in,bypass pwr on delay
                if (app_env_get_pin_enable(PIN_ioDet))
                {
                    if (gpio_input(app_env_get_pin_num(PIN_ioDet))==app_env_get_pin_valid_level(PIN_ioDet))
                    {
                        app_bt_flag2_set(APP_FLAG2_5S_PRESS, 0);
                        if(app_charge_is_usb_plug_in() && !app_env_check_Charge_Mode_HighEfficiency())
                            app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN, 1);
                        break;
                    }
                }
            #endif
                
                t1 = os_get_tick_counter();
                if ( (t1 - t0) <=  env_h->env_cfg.feature.sw_pwr_on_key_time)// 1s power on
                {
                    if((app_env_get_pin_enable(PIN_pwrBtn)
                            &&gpio_input(app_env_get_pin_num(PIN_pwrBtn))!=app_env_get_pin_valid_level(PIN_pwrBtn))
                        ||((SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())
                            &&(gpio_input(app_env_get_pin_num(PIN_ioDet))!= app_env_get_pin_valid_level(PIN_ioDet)))
                        ||app_charge_is_powerdown())
                    {//key release or charge mode
                        app_bt_flag2_set(APP_FLAG2_5S_PRESS, 0);
                    #if ((CONFIG_CHARGE_EN == 1))
                        if(app_charge_is_usb_plug_in())
                        {
                        #if 0
                            if (!app_env_check_Charge_Mode_PwrDown())
                            {    
                                app_bt_flag2_set(APP_FLAG2_CHARGE_NOT_POWERDOWN,1);
                            }
                            else
                        #endif
                            {
                                app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN, 1);
                            }
                            break;
                        }
                        else
                    #endif
                        {
                            LOG_I(PWR,"ENTER_DEEP_SLEEP 1:%d\r\n",gpio_input(app_env_get_pin_num(PIN_pwrBtn)));
                            app_powerdown_action();
                        }
                    }
                    else
                    {
                        //key pressing
                        Delay(1000);
                        BK3000_start_wdt(0xFFFF);
                    }
                }
                else if ( (t1 - t0) >  env_h->env_cfg.feature.sw_pwr_on_key_time)
                {//key press enough,power on
                    static uint8 s_already_play_poweron = 0;
                    if (0 == s_already_play_poweron){
                        app_wave_file_play_start(APP_WAVE_FILE_ID_START);
                        aud_dac_open();
                        app_wave_file_play();
                        s_already_play_poweron = 1;
                        //SET_PWR_DOWN_FLAG(AUTO_PWR_ON_FLAG);
                        break;
                    }
                }
            }
            
            //power on........
            if ((SW_PWR_KEY_MOS_CTRL==app_env_check_pwrCtrl_mode())
                &&app_env_get_pin_enable(PIN_mosCtrl))
            {
                gpio_config(app_env_get_pin_num(PIN_mosCtrl),1);
                gpio_output(app_env_get_pin_num(PIN_mosCtrl),app_env_get_pin_valid_level(PIN_mosCtrl));
            }
            
        }
    }
    else
    {
        uart_initialise(UART_BAUDRATE_115200);
    }
}

int app_env_check_feature_flag(uint32_t flag)
{
    app_env_handle_t env_h = app_env_get_handle();
    return (env_h->env_cfg.feature.feature_flag & flag);
}
int app_env_check_inquiry_always(void)
{
	return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_INQUIRY_ALWAYS);
}
int app_env_check_SPP_profile_enable(void)
{
	return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_SPP_PROFILE);
}
int app_env_check_HID_profile_enable(void)
{
	return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_HID_PROFILE);
}
int app_env_check_AVRCP_TG_profile_enable(void)
{
	return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC);
}
int app_env_check_DAC_POP_handle_enable(void)
{
    return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DAC_DC_COMPENSATION);
}
int app_env_check_Charge_Mode_PwrDown(void)
{
    return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_CHARGE_MODE_PWR_DOWN);
}
int app_env_check_Charge_Det_use_gpio(void)
{
	return app_env_get_pin_enable(PIN_ioDet);
}
int app_env_check_Charge_Time_Out_Enable(void)
{
    return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_CHARGE_TIMEOUT_ENABLE);
}
int app_env_check_Charge_Mode_HighEfficiency(void)
{ //charging mode will enter deepsleep, not fake powerdown.
#if (CONFIG_TEMPERATURE_NTC || CONFIG_BOX_UI_RELATE)
	return 0;
#else
    return 1; //airpods3 case, always return 1;
#endif
    //return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_CHARGEING_LOW_I_Vusb);
}
int app_env_system_uart_dbg_enable(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    if(env_h->env_cfg.used == 1)
        return (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_UARTDBG);
    else
        return 0;
}

int app_env_check_Use_ext_PA(void)
{
	return app_env_get_pin_enable(PIN_paMute);
}
int app_env_check_bat_display(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    return (env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_APP_BAT_DISPLAY);
}
uint32_t app_env_check_bt_para_flag(uint32_t flag)
{
    app_env_handle_t env_h = app_env_get_handle();
    return (env_h->env_cfg.bt_para.bt_flag & flag);
}
int8_t app_env_get_autoconn_max(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    return(env_h->env_cfg.bt_para.auto_conn_count);
}
int8_t app_env_get_reconn_max(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    return(env_h->env_cfg.bt_para.disconn_retry_count);
}
int app_env_check_pwrCtrl_mode(void)
{
//return: 0=���뿪��; 1=����������;
// 2=����������; 3=���������ؼ�gpio����mos
//
    app_env_handle_t env_h = app_env_get_handle();
    if(env_h->env_cfg.used == 1)
        return env_h->env_cfg.system_para.mos_soft_power_flag;
    else
        return 0;
}
int app_env_check_sniff_mode_Enable(void)
{
    return app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_SNIFF_ENABLE);
}

int app_env_get_pin_enable(uint8_t pin_index)
{
    app_env_handle_t env_h = app_env_get_handle();
    if((env_h->env_cfg.used == 1)&&(0xFF!=app_env_get_handle()->env_cfg.system_para.pins[pin_index]))
	 	return (app_env_get_handle()->env_cfg.system_para.pins[pin_index]>>7)&0x01;
	else
		return 0;
}

int app_env_get_pin_valid_level(uint8_t pin_index)
{
 	return (app_env_get_handle()->env_cfg.system_para.pins[pin_index]>>6)&0x01;
}

int app_env_get_pin_num(uint8_t pin_index)
{
 	return (app_env_get_handle()->env_cfg.system_para.pins[pin_index])&0x3f;
}

void app_env_set_pin_value(uint8_t pin_index,uint8_t val)
{
 	app_env_get_handle()->env_cfg.system_para.pins[pin_index] = val;
}

#if POWER_ON_OUT_CHARGE_ENABLE
int app_env_check_auto_pwr_on_enable(void)
{
#if 1
	return 1;
#else
	app_env_handle_t env_h = app_env_get_handle();
    if((env_h->env_cfg.used == 1)
		&&app_env_get_pin_enable(PIN_ioDet))
		//&& (app_env_check_pwrCtrl_mode()<SW_PWR_KEY_SWITCH))
		return 1;
	else
		return 0;
#endif	
}

#endif
void app_env_pwrctrl_by_switch(uint32_t step)
{
    if(SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode()
		&&app_env_get_pin_enable(PIN_ioDet))
    {
        static uint8 sw_io_cnt = 0,sw_pwr_flag=0;
        if((app_env_get_pin_valid_level(PIN_ioDet) == gpio_input(app_env_get_pin_num(PIN_ioDet)))
            || app_charge_is_powerdown())
        {//power on...
            sw_io_cnt = 0;
        }
        else
        {//power down...
            if (sw_io_cnt < 200)
                sw_io_cnt += step;

            if ((sw_io_cnt >= 20) 
				&& !app_bt_flag1_get(APP_FLAG_POWERDOWN)
				&& (sw_pwr_flag == 0))
            {
                sw_io_cnt = 0;
				sw_pwr_flag = 1;
                LOG_I(PWR,"pwrctrl_by_switch\r\n");
                app_set_auto_powerdown();
                //app_button_sw_action(BUTTON_BT_POWERDOWN);
                CLEAR_PWDOWN_TICK;
                msg_put(MSG_POWER_DOWN);
            }
        }
    }
}

extern uint32_t XVR_analog_reg_save[];
extern uint32_t XVR_reg_0x24_save;
void app_env_rf_pwr_set(uint8_t type)
{
    //type: 0=a2dp mode;  1=hfp mode;
    app_env_handle_t  env_h = app_env_get_handle();
    app_rfpwr_t *rf_pwr;
    
    if(type==0)
    {
        rf_pwr = &env_h->env_cfg.feature.a2dp_rf_pwr;
    }
    else if(type == 1)
    {
        rf_pwr = &env_h->env_cfg.feature.hfp_rf_pwr;
    }
    else
        return;
    
    XVR_reg_0x24_save = (XVR_reg_0x24_save&~(0x0f<<8))|(((rf_pwr->big_adj)&0x0f)<<8);
}


btaddr_t* app_env_local_bd_addr(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    return(&(env_h->env_cfg.bt_para.device_addr));
}

int8_t *app_env_local_bd_name(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    return(env_h->env_cfg.bt_para.device_name);
}
