#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "tc_interface.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "app_debug.h"
#include "bt_at_types.h"
#include "diskio.h"
#if BT_DUALMODE_RW
#include "app_adv.h"
#endif
#ifdef A2DP_MPEG_AAC_DECODE
#include "bt_a2dp_mpeg_aac_decode.h"
#endif

extern uint32_t sys_loop_start_time_get(void);

#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
extern uint32_t XVR_reg_0x24_save;
static uint8_t RF_DUT_MODE = 0;
uint8_t app_get_power_level(uint8_t index)
{
    uint8_t level=0;
    if(RF_DUT_MODE == 2)
        level = (uint8_t)((uint32_t)(XVR_reg_0x24_save&(0x0f<<8))>>8);
    else
        level = 0x0F;
    switch(index)
    {
        case 0:
            level = level - 10;
            if(level<3)
                level = 3;
            break;
        case 1:
            level = level - 7;
            break;
        default:
            break;
    }
    return level;
}
extern void BK3000_RF_Dut_ReWrite(uint8_t option);
void app_bt_enable_dut_mode( int8_t enable)
{
    app_handle_t app_h = app_get_sys_handler();

    if(app_h->unit == NULL)
        return;

    app_sleep_func(0);
    if(enable)
    {
        app_set_crystal_calibration(1);
        
        if(app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE))
            return;

        #if BT_DUALMODE_RW
        appm_stop_advertising();
        #endif
        
        //app_env_rf_pwr_set(0);
        RF_DUT_MODE = enable;
        BK3000_RF_Dut_ReWrite(RF_DUT_MODE); // dut mode
        
        if(hci_get_acl_link_count(app_h->unit)
        &&( app_bt_flag1_get(APP_AUDIO_FLAG_SET )) )
        {
            app_button_match_action();
            app_bt_flag1_set(APP_FLAG_DUT_MODE_ENABLE,1);
        }
        else
        {
            result_t err;
            err = bt_unit_disable( app_h->unit );
            if(UWE_BUSY!=err)
                app_bt_flag1_set(APP_FLAG_DUT_MODE_ENABLE,1);
        }

    }
    else
    {
        BK3000_wdt_reset();
    }

    return;
}
#endif

static int8_t fcc_mode = 0;
extern uint32_t XVR_analog_reg_save[16];
extern void BKxvr_Set_Tx_Power(uint32_t pwr);
void app_bt_enable_fcc_mode(int8_t mode, uint8_t chnl, uint8_t pwr)
{
    uint32_t v;
    app_sleep_func(0);
    app_handle_t app_h = app_get_sys_handler();
    uint32_t reg=0;
    if(app_h->unit == NULL) return;

    if(mode > 0)
    {
        REG_XVR_0x22 = 0x0B082409;
        REG_XVR_0x3A = 0x20006000;
        app_h->flag_sm1 |= APP_FLAG_FCC_MODE_ENABLE;
        bt_unit_set_scan_enable( app_h->unit, 0);
        bt_unit_disable( app_h->unit );
        (*(volatile unsigned long*)(0x00F7A000)) = 0;  // JAL_LE_MODE_ADDR = 0x00F7A000
        SYSirq_Unmask_Interrupt(&v,1 << VIC_IDX_CEVA);
        
        v=REG_XVR_0x24;
        v=v&0xFFFFFF80;
        chnl=chnl+2;
        v=v|(chnl);
        REG_XVR_0x24 = v;
        BKxvr_Set_Tx_Power(pwr & 0x0f);
        REG_XVR_0x25 = 0;
        os_delay_ms(1);
        switch(mode&0x0f)
        {
            case 1: //PN9 GFSK
                //REG_XVR_0x01 = 0xf10401b0;
                //BK3000_XVR_REG_0x04 = 0x58e45844;
                //REG_XVR_0x09 = 0x03fff0aa;
                REG_XVR_0x25 = 0x00003800;
                break;
            case 2://PN9 EDR2
                //REG_XVR_0x01 = 0xf10401b0;
                //BK3000_XVR_REG_0x04 = 0x58e45844;
                //REG_XVR_0x09 = 0x03fff0aa;
                REG_XVR_0x25 = 0x00007800;
                break;
            case 3://PN9 EDR3
                //REG_XVR_0x01 = 0xf10401b0;
                //BK3000_XVR_REG_0x04 = 0x58e45844;
                //REG_XVR_0x09 = 0x03fff0aa;
                REG_XVR_0x25 = 0x0000B800;
                break;
            case 4://CW
                //REG_XVR_0x01 = 0xf10401b0;
                //BK3000_XVR_REG_0x04 = 0x58e45844;
                //REG_XVR_0x09 = 0x03fff0aa;
                REG_XVR_0x25 = 0x00003100;
                break;
            case 5:// recv PN9/GFSK;
                REG_SYSTEM_0x42 |= 1<<1;
                reg = REG_SYSTEM_0x46;
                reg &= ~(0x07 << 14);
                reg |= (6 << 14);
                REG_SYSTEM_0x46 = reg;
                REG_XVR_0x25 = 0x00002400;
                break;  
            case 6:// recv PN9/GFSK;
                REG_XVR_0x25 = 0x00002400;
                break;
            case 7:
                REG_XVR_0x04 = XVR_analog_reg_save[4] = 0x80000000;
                REG_XVR_0x25 = 0x00042400;
                break;
            case 8:
                REG_XVR_0x04 = XVR_analog_reg_save[4] = 0x80000000;
                REG_XVR_0x25 = 0x00043800;
                break;
            default:  // for lpbg calibration
                BK3000_set_clock(CPU_CLK_XTAL,0);
                break;
        }
#if 0
        os_delay_ms(1);
        if(enable > 1) /* PN9 test mode */
        REG_XVR_0x25 = (0x1<<11)|(0x1<<12) | (0x1<<13);
        else		   /* single carrior test mode */
        REG_XVR_0x25 = (0x1<<12) | (0x1<<13);
#endif
        os_printf("Enter FCC testmode !!!\r\n");
        fcc_mode = mode;
        /* add other code */
    }
    else
    {
        BK3000_wdt_reset();
    }
}

void app_bt_print_ber(void)
{
    if(app_bt_flag1_get(APP_FLAG_FCC_MODE_ENABLE))
    {  
        if(fcc_mode == 6 || fcc_mode == 7) // recv PN9,EDR2
        {
            static uint64_t t0 = 0;
            uint64_t t1;
            uint32_t nTotal=0,nErr=0,nTotal_1=0,nErr_1=0;
            static uint32_t nTotal_0=0,nErr_0=0;
            uint32_t nPer;
            if((t0 == 0) /*|| (REG_XVR_0x15==0)*/)
            {
            	t0 = os_get_tick_counter();
                return;
            }
            t1 = os_get_tick_counter();
            if(t1-t0>100)
            {
                REG_XVR_0x25 |= (1<<9);//hold
                nTotal_1 = REG_XVR_0x15;
                nErr_1 = REG_XVR_0x16;
                
                if(nTotal_1 < nTotal_0)
                {
                    nTotal = 0xffffffff - nTotal_0 + nTotal_1;
                }
                else
                {
                    nTotal = nTotal_1 - nTotal_0;
                }

                if(nErr_1 < nErr_0)
                {
                    nErr = 0xffffffff - nErr_0 + nErr_1;
                }
                else
                {
                    nErr = nErr_1 - nErr_0;
                }
                
                if(nTotal == 0)
                	nPer = 0;
                else
                    nPer = (uint64_t)nErr*100000/nTotal;  

                os_printf("BER total:0x%x,Err:0x%x,percent:%d.%d %%%%  \r\n",nTotal,nErr,nPer/10,nPer%10);
                REG_XVR_0x25 &= ~(1<<9);//not hold
                t0 = os_get_tick_counter();
                nTotal_0 = nTotal_1;
                nErr_0 = nErr_1;
            }
        }
        
    }
}

uint8_t app_check_bt_mode(uint8_t mode)
{
	app_env_handle_t  env_h = app_env_get_handle();
	return (env_h->env_cfg.feature.bt_mode)&mode;
}

uint8_t get_bt_dev_priv_work_flag(void)
{
    uint32_t hf_flag=0,a2dp_flag=0;
    uint8_t i;
    for(i=0; i<BT_MAX_AG_COUNT; i++)
    {
        hf_flag = get_hf_priv_flag(i,APP_FLAG2_HFP_INCOMING|APP_FLAG_HFP_OUTGOING);
        a2dp_flag = get_a2dp_priv_flag(i,APP_BUTTON_FLAG_PLAY_PAUSE|APP_FLAG_MUSIC_PLAY);
        if(hf_flag && a2dp_flag)
            return 1;
    }
    return 0;
}

//*******************************************************
void sd_start_task()
{
#if (CONFIG_APP_MP3PLAYER == 1)
    os_printf("%s(), cur mode:%d\n", __func__, get_app_mode());
#if 0//ndef BT_SD_MUSIC_COEXIST
    if((get_app_mode() != SYS_WM_SDCARD_MODE) || app_bt_flag1_get(APP_AUDIO_FLAG_SET))
    {
        os_printf("BT is still active\r\n");
        return;
    }
#endif
    if(!sd_is_attached()) {
        LOG_W(SD, " !!!!!!!! sd is not exist\n");
        return;
    }

#ifdef CONFIG_APP_AEC
    app_aec_uninit();
#endif

    sbc_mem_free();// sbc_target_deinit_jfree_buff();
#ifdef A2DP_MPEG_AAC_DECODE
    bt_a2dp_aac_stream_suspend();
#endif
    if((get_fat_ok_flag() == 1) && (get_cur_media_type() != DISK_TYPE_SD))
    {
        Media_Fs_Uninit(get_cur_media_type());
    }
    if((get_fat_ok_flag() == 0) && 1)//if fat_ok and sdcard is mount ok
    {
        if(Media_Fs_Init(DISK_TYPE_SD) != FR_OK)
        {
            os_printf("SD Init Err!!!\r\n");
            msg_put(MSG_SD_READ_ERR);
            return;
        }
    }
    app_player_init();

    app_player_button_setting();//set button to player
//yuan++	app_player_set_vol(AUDIO_VOLUME_MAX);//set volume to max
#ifndef CONFIG_SD_AUTO_PLAY_DIS
    app_player_play_pause_caller(1);//auto play
#endif
#endif /* CONFIG_APP_MP3PLAYER */
}


// #ifdef CONFIG_APP_UDISK

/*
for mode3: user need call last_is_udisk_mode_set(1) in user_init() if last mode is udisk mode.
*/
#ifndef UDISK_UI_MODE
    #define UDISK_UI_MODE       1//1:auto enable, 2:auto disable, 3:according last mode
#endif
#if UDISK_UI_MODE == 1
#elif UDISK_UI_MODE == 2
    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
    #define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
#elif UDISK_UI_MODE == 3
    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
    #define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
#endif

static uint8_t s_udisk_mod_cfg_fg = 0x83;
//auto switch to udisk mode when udisk inserted and init complete.
void udisk_mode_auto_sw_set(uint8_t en) { en ? (s_udisk_mod_cfg_fg |= 1) : (s_udisk_mod_cfg_fg &= ~1); }
uint8_t udisk_mode_auto_sw_get(void) { return (s_udisk_mod_cfg_fg & 1); }
//auto play when switch to udisk mode
void udisk_mode_auto_play_set(uint8_t en) { en ? (s_udisk_mod_cfg_fg |= 2) : (s_udisk_mod_cfg_fg &= ~2); }
uint8_t udisk_mode_auto_play_get(void) { return (s_udisk_mod_cfg_fg & 2); }

void last_is_udisk_mode_set(uint8_t en) { en ? (s_udisk_mod_cfg_fg |= 4) : (s_udisk_mod_cfg_fg &= ~4); }
uint8_t last_is_udisk_mode_get(void) { return (s_udisk_mod_cfg_fg & 4); }

void usbh_udisk_auto_op_init(void)
{
#ifdef CONFIG_UDISK_AUTO_PLAY_DIS //disable auto switch to udisk mode when udisk inserted
    udisk_mode_auto_play_set(0);
#endif
#ifdef CONFIG_UDISK_AUTO_MOD_SW_DIS //disable auto play when switch to udisk mode
    udisk_mode_auto_sw_set(0);
#endif
}

#ifdef CONFIG_APP_UDISK
extern uint32_t udisk_enumed_time_get(void);

__attribute__((weak)) void user_udisk_ready_callback(void)
{
	// LOG_I(WM, "user_udisk_ready\n");
    uint32_t tim_start = sys_loop_start_time_get();//system start time
    uint32_t tim_enumed = udisk_enumed_time_get();
	LOG_I(WM, "%s, auto sw:%d, ply:%d\n", __FUNCTION__, udisk_mode_auto_sw_get(), udisk_mode_auto_play_get());
    LOG_I(WM, "%ums -- %ums -- %ums\n", tim_start, tim_enumed, sys_time_get());
#if ((UDISK_UI_MODE == 1) || (UDISK_UI_MODE == 2))
    if(udisk_mode_auto_sw_get()){
        extern void system_work_mode_set_button(uint8_t mode);
        system_work_mode_set_button(SYS_WM_UDISK_MODE);
    }
#elif UDISK_UI_MODE == 3//not test
    if(last_is_udisk_mode_get()){//如果上次是U盘模式，则自动切换
        udisk_mode_auto_sw_set(1);
    }else{//如果上次不是U盘模式，则系统启动后5s不切换，等5s后插U盘可自动切模式
        if(s_udisk_mod_cfg_fg & 0x80)
        { 
            if((tim_enumed - tim_start) > 5000){
                s_udisk_mod_cfg_fg &= ~0x80;
                udisk_mode_auto_sw_set(1);
            }
        }
    }
    if(udisk_mode_auto_sw_get()){
        extern void system_work_mode_set_button(uint8_t mode);
        system_work_mode_set_button(SYS_WM_UDISK_MODE);
    }
    udisk_mode_auto_sw_set(1);
#endif
}

__attribute__((weak)) void user_udisk_lost_callback(void)
{
	LOG_I(WM, "user_udisk_lost\n");
    system_work_mode_change_button();
}

void usbh_udisk_init_cmp_callback(void)
{
    user_udisk_ready_callback();
}

//udisk is lost/pulled out (in isr)
void usbh_udisk_lost_callback(void)
{
	LOG_I(WM, "%s\n", __FUNCTION__);
    if(!app_is_udisk_mode()) return;
#if (CONFIG_APP_MP3PLAYER == 1)
    set_app_player_flag_hw_detach();
#endif
    user_udisk_lost_callback();
}

void udisk_start_task()
{
#if (CONFIG_APP_MP3PLAYER == 1)
    os_printf("%s(), cur mode:%d\n", __func__, get_app_mode());
    extern int udisk_is_attached();
    if(!udisk_is_attached()) {
        LOG_W(SD, " !!!!!!!! sd is not exist\n");
        goto RET_ERR;
    }

#ifdef CONFIG_APP_AEC
    app_aec_uninit();
#endif

    sbc_mem_free();// sbc_target_deinit_jfree_buff();
#ifdef A2DP_MPEG_AAC_DECODE
    bt_a2dp_aac_stream_suspend();
#endif
    if((get_fat_ok_flag() == 1) && (get_cur_media_type() != DISK_TYPE_UDISK))
    {
        Media_Fs_Uninit(get_cur_media_type());
    }

    if(get_fat_ok_flag() == 0)
    {
        if(Media_Fs_Init(DISK_TYPE_UDISK) != FR_OK)
        {
            os_printf("UDisk Init Err!!!\r\n");
            // msg_put(MSG_SD_READ_ERR);
            goto RET_ERR;
        }
    }
    app_player_init();

    app_player_button_setting();//set button to player
    // app_player_set_vol(AUDIO_VOLUME_MAX);//set volume to max
    app_player_play_pause_caller(!!udisk_mode_auto_play_get());//play default
    return;// 0;
RET_ERR:
    return;// -1;
#endif /* CONFIG_APP_MP3PLAYER */
}

void app_udisk_mode_exit(void)
{
    // REG_GPIO_0x0D = 2;//borg: ~28ms @230714
    app_player_play_pause_caller(0);
    Delay(1000);
    app_player_unload();
    Media_Fs_Uninit(DISK_TYPE_UDISK);
    fat_free_files_buffer();
    // REG_GPIO_0x0D = 0;
}

#endif

#ifdef CONFIG_APP_SPDIF
static uint8_t s_spdif_mod_cfg_fg = 0;
//auto switch to spdif mode when spdif inserted and init complete.
void spdif_mode_auto_sw_set(uint8_t en) { en ? (s_spdif_mod_cfg_fg |= 1) : (s_spdif_mod_cfg_fg &= ~1); }
uint8_t spdif_mode_auto_sw_get(void) { return (s_spdif_mod_cfg_fg & 1); }
//auto switch to spdif mode when spdif no signel.
void spdif_mode_auto_exit_set(uint8_t en) { en ? (s_spdif_mod_cfg_fg |= 2) : (s_spdif_mod_cfg_fg &= ~2); }
uint8_t spdif_mode_auto_exit_get(void) { return (s_spdif_mod_cfg_fg & 2); }
//auto play when switch to spdif mode
void spdif_mode_auto_play_set(uint8_t en) { en ? (s_spdif_mod_cfg_fg |= 4) : (s_spdif_mod_cfg_fg &= ~4); }
uint8_t spdif_mode_auto_play_get(void) { return (s_spdif_mod_cfg_fg & 4); }
#endif

void bt_mode_exit(void)
{//refer to app_button_conn_disconn()
    app_handle_t app_h = app_get_sys_handler();

    // if(app_bt_flag1_get(APP_AUDIO_FLAG_SET))  // bt connected
    if(app_bt_flag1_get(APP_FLAG_ACL_CONNECTION))
    {
        uint8_t idx = 0;
        for(idx=0; idx<BT_MAX_AG_COUNT; idx++)
            bt_unit_acl_disconnect(app_h->unit, &g_bt_app_entity_container[idx].bt_remote_addr);
        // bt_all_acl_disconnect(app_h->unit);
        // app_bt_flag1_set((APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
        //jtask_schedule( app_h->app_reset_task, 800, (jthread_func)bt_unit_disable,( void *)app_h->unit);
    }
    else // have no bt connection
    {
        //app_bt_shedule_task((jthread_func)bt_unit_disable, (void *)app_h->unit, 0);
        app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
        //bt_auto_connect_stop();
        jtask_stop(app_h->app_auto_con_task);
        jtask_stop(app_h->app_reset_task);
    }
    bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);
}

void bt_mode_enter(void)
{//refer to app_button_conn_disconn()
	app_handle_t app_h = app_get_sys_handler();

    jtask_stop(app_h->app_auto_con_task);
    jtask_stop(app_h->app_reset_task);
    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #if (BUTTON_DETECT_IN_SNIFF == 1)
    SYSpwr_Wakeup_From_Sleep_Mode();
    sniffmode_wakeup_dly = 1000;
    sniff_enable_timer0_pt0();
    #endif
#endif

    if(app_get_env_key_num())
    {
        t_bt_app_entity last_entites[2];
        uint8_t idx=0;
        t_sys_bt_state bt_state = SYS_BT_UNUSED_STATE;

        for(idx=0; idx<BT_MAX_AG_COUNT; idx++)
        {
            bt_state  = bt_app_entity_get_state(idx);
            LOG_I(BTN,"bt_state=%x\r\n",bt_state);
            if(bt_state<SYS_BT_WORKING_STATE)
            {
                app_handle_t sys_hdl = app_get_sys_handler();
                bt_unit_set_scan_enable(sys_hdl->unit, 0);
                bt_app_entity_set_state(idx,SYS_BT_STANDBY_STATE);
                last_entites[idx] = g_bt_app_entity_container[idx];
            }
        }
        bt_app_get_conn_addr_from_env();
        for (idx = 0; idx < BT_MAX_AG_COUNT; ++idx)
        {
            int result_id = bt_app_entity_find_id_from_raddr(&(last_entites[idx].bt_remote_addr));
            if(result_id != MNG_ERROR_NO_ENTITY)
            {
                g_bt_app_entity_container[result_id].mac_win_book = last_entites[idx].mac_win_book;
            }
        }
    }else{
        app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION | APP_FLAG_RECONNCTION, 0);
        bt_app_set_device_scanning(HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE);		//  yuan++
    }

//  bt_app_set_device_scanning(HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE);		//  yuan++

    app_bt_button_setting();
}



//**************************************************
static void exit_work_mode(uint32_t mode)
{
    os_printf("%s(), mode:%d\n", __func__, mode);
//	bt_app_entity_print_state();
    switch(mode)
    {
#if (CONFIG_APP_MP3PLAYER == 1)
    case SYS_WM_SDCARD_MODE:
        // set_app_player_flag_hw_detach();
        app_player_play_pause_caller(0);
        Delay(1000);
        app_player_unload();
        Media_Fs_Uninit(0);
        fat_free_files_buffer();
        break;
#endif
    case SYS_WM_BT_MODE:
#if 1
    	bt_mode_exit();
#else
		{
			app_handle_t app_h = app_get_sys_handler();
			if(app_bt_flag1_get(APP_AUDIO_FLAG_SET))  // bt connected
			{
				bt_all_acl_disconnect(app_h->unit);
				app_bt_flag1_set((APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
				//jtask_schedule( app_h->app_reset_task, 800, (jthread_func)bt_unit_disable,( void *)app_h->unit);
			}
			else // have no bt connection
			{
				//app_bt_shedule_task((jthread_func)bt_unit_disable, (void *)app_h->unit, 0);
				app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
				//bt_auto_connect_stop();
				bt_unit_set_scan_enable( app_h->unit, HCI_NO_SCAN_ENABLE);//yuan++
				jtask_stop(app_h->app_auto_con_task);
				jtask_stop(app_h->app_reset_task);
			}
		}
#endif
        break;

#ifdef CONFIG_APP_UDISK
    case SYS_WM_UDISK_MODE:
        app_udisk_mode_exit();
        break;
#endif

#ifdef CONFIG_APP_SPDIF
    case SYS_WM_SPDIF_MODE:
        app_spdif_exit();
        break;
#endif
    default:
        break;
    }
}

//*******************************************************
static void enter_work_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();

    os_printf("%s(), mode:%d\n", __func__, app_h->sys_work_mode);
//	bt_app_entity_print_state();

    switch(app_h->sys_work_mode){
    case SYS_WM_BT_MODE:
#if 1
    	bt_mode_enter();
#else
    {
    	app_env_handle_t env_h = app_env_get_handle();
    #if A2DP_ROLE_SOURCE_CODE
        if(isA2dpSrcConnected())
        {
            a2dpSrcCmdStreamSuspend();
        }
    #else
        app_bt_flag1_set(APP_AUDIO_FLAG_SET, 0);
    #endif
        jtask_stop(app_h->app_auto_con_task);
        jtask_stop(app_h->app_reset_task);
        CLEAR_PWDOWN_TICK;
        CLEAR_SLEEP_TICK;
    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        #if (BUTTON_DETECT_IN_SNIFF == 1)
        SYSpwr_Wakeup_From_Sleep_Mode();
        sniffmode_wakeup_dly = 1000;
        sniff_enable_timer0_pt0();
        #endif
    #endif
        if(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_AUTO_CONN){
            app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION,1);
        }
        bt_connection_req(&app_h->remote_btaddr);	// if paired device list is not empty
        bt_app_set_device_scanning(HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE);		//  yuan++

        app_bt_button_setting();
    }
#endif
        break;

    case SYS_WM_SDCARD_MODE:
        jtask_schedule( app_common_get_task_handle(), 100, sd_start_task,NULL);
        break;

#ifdef CONFIG_APP_UDISK
    case SYS_WM_UDISK_MODE:
        jtask_schedule(app_common_get_task_handle(), 200, udisk_start_task, NULL);
        break;
#endif

#ifdef CONFIG_APP_SPDIF
    case SYS_WM_SPDIF_MODE:
        app_spdif_enter();
        break;
#endif
    default:
        break;
    }
}

//***********************************************************
void system_work_mode_set_button(uint8_t new_mode)
{
    uint32_t last_mode;
    app_handle_t app_h = app_get_sys_handler();
    last_mode = app_h->sys_work_mode;
    //-------------------------------------------------
    if(new_mode == SYS_WM_SDCARD_MODE){
#ifdef CONFIG_APP_SDCARD
    	if(!sd_is_attached()) new_mode++;
#else
    	new_mode++;
#endif
    }
    //-------------------------------------------------
    if(new_mode == SYS_WM_UDISK_MODE){
#ifdef CONFIG_APP_UDISK
    	if(!udisk_is_attached()) new_mode++;
#else
    	new_mode++;
#endif
    }
    //-------------------------------------------------
    if(new_mode == SYS_WM_LINEIN1_MODE){
#if defined(LINE_DETECT_IO) || defined(LINE_EN)
    	extern uint8_t	LineInR;
    	if(LineInR)	new_mode++;
#else
        new_mode++;
#endif
    }
    //-------------------------------------------------
    if(new_mode == SYS_WM_LINEIN2_MODE){
#if defined(LINE2_DETECT_IO) || defined(LINE2_EN)
    	extern uint8_t	LineIn2R;
    	if(LineIn2R)	new_mode++;
#else
    	new_mode++;
#endif
    }
    if(new_mode == SYS_WM_SPDIF_MODE){
    #ifdef CONFIG_APP_SPDIF
        if(spdif_mode_auto_sw_get()){
            if(!spdif_is_online()) new_mode++;
        }
    #else
    	new_mode++;
    #endif
    }
    if(new_mode >= SYS_WM_NULL){
    	new_mode = SYS_WM_BT_MODE;
    }

    //=======================================================
    app_h->sys_work_mode = new_mode;
    if(last_mode != new_mode){
    	if(last_mode == SYS_WM_BT_MODE
        || last_mode == SYS_WM_SDCARD_MODE
        || last_mode == SYS_WM_UDISK_MODE
        || last_mode == SYS_WM_SPDIF_MODE){
    		jtask_stop(app_h->app_common_task);
    		exit_work_mode(last_mode);
    	}
    	if(new_mode == SYS_WM_BT_MODE
        || new_mode == SYS_WM_SDCARD_MODE
        || new_mode == SYS_WM_UDISK_MODE
        || new_mode == SYS_WM_SPDIF_MODE){
    	    enter_work_mode();
    	}
    }
    enter_mode_wave_and_action(new_mode, NULL);
    //	enter_mode_wave_and_action(new_mode, enter_work_mode);
    os_printf("%s() mode:%d -> %d\n", __func__, last_mode, new_mode);
}

//******************************************************
int system_work_mode_change_button(void)
{
	app_handle_t app_h = app_get_sys_handler();
	os_printf("chg mode: %d\n",app_h->sys_work_mode);
    uint32_t new_mode = app_h->sys_work_mode+1;
    system_work_mode_set_button(new_mode);

    return new_mode;
}

//*****************************************************
unsigned int get_app_mode()
{
    app_handle_t app_h = app_get_sys_handler();
    return (app_h->sys_work_mode);
}

unsigned int get_bt_mode()
{
    app_handle_t app_h = app_get_sys_handler();
    return (app_h->bt_mode);
}

void set_bt_mode(unsigned int mode)
{
    app_handle_t app_h = app_get_sys_handler();
    app_h->bt_mode = mode;

    LOG_D(APP,"set bt_mode to %d\r\n", mode);
}

uint32_t app_is_mp3_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return (SYS_WM_SDCARD_MODE == app_h->sys_work_mode);
}

uint32_t app_is_bt_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return (SYS_WM_BT_MODE == app_h->sys_work_mode);
}

uint8_t app_is_sdcard_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return (app_h->sys_work_mode == SYS_WM_SDCARD_MODE);
}

uint8_t app_is_udisk_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return (app_h->sys_work_mode == SYS_WM_UDISK_MODE);
}

uint32_t app_is_line_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return (SYS_WM_LINEIN1_MODE == app_h->sys_work_mode);
}

#ifdef BT_SD_MUSIC_COEXIST
uint32_t player_runing_status = 0;
#endif

void app_incoming_call_enter(void)
{
#if (defined(BT_SD_MUSIC_COEXIST))
    app_handle_t app_h = app_get_sys_handler();
#endif

#ifdef BT_SD_MUSIC_COEXIST
    if(SYS_WM_SDCARD_MODE == app_h->sys_work_mode)
    {
        os_printf("exit_mp3_\r\n");

        if(player_get_play_status())
        {
            player_runing_status = 1;
            app_player_play_pause_caller(0);
        }
        else
        {
            player_runing_status = 0;
        }

        Delay(1000);
        app_player_unload_for_incoming_call();

        /* sbc_target_init_malloc_buff(); */
    }
#endif
}

void incoming_call_exit(void *arg)
{
#if (defined(BT_SD_MUSIC_COEXIST))
    app_handle_t app_h = app_get_sys_handler();
#endif

#ifdef BT_SD_MUSIC_COEXIST
    if(SYS_WM_SDCARD_MODE == app_h->sys_work_mode)
    {
        aud_dac_close();

#ifdef CONFIG_APP_AEC
        app_aec_uninit();
#endif

        os_printf("enter-mp3-\r\n");

        sbc_target_deinit_jfree_buff();

        app_player_load_for_incoming_call();

        /*play mp3_sd, after hang up*/
        if(player_runing_status)
        {
            app_player_play_pause_caller(1);
        }
    }
#endif
}

void app_incoming_call_exit(void)
{
#if (defined(BT_SD_MUSIC_COEXIST))
    app_handle_t app_h = app_get_sys_handler();
    jtask_stop(app_h->app_coexist_task);

    jtask_schedule(app_h->app_coexist_task, 3000, incoming_call_exit, NULL);
#endif
}

#if A2DP_ROLE_SOURCE_CODE
extern uint8_t get_a2dp_role(void);
void app_bt_sdp_connect(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    btaddr_t *remote_btaddr = &sys_hdl->remote_btaddr;
    btaddr_t *local_btaddr = &env_h->env_cfg.bt_para.device_addr;

    os_printf("app_bt_sdp_connect, local = " BTADDR_FORMAT ", remote = " BTADDR_FORMAT "\r\n", BTADDR(local_btaddr), BTADDR(remote_btaddr));

    sdp_connect(local_btaddr, remote_btaddr);
}

void app_bt_sdp_service_connect(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    btaddr_t *remote_btaddr = &sys_hdl->remote_btaddr;
    btaddr_t *local_btaddr = &env_h->env_cfg.bt_para.device_addr;

    os_printf("app_bt_sdp_service_connect, local = " BTADDR_FORMAT ", remote = " BTADDR_FORMAT "\r\n", BTADDR(local_btaddr), BTADDR(remote_btaddr));

    sdp_service_connect(local_btaddr, remote_btaddr, TID_A2DP_PROTOCOL_SSA_REQUEST);
}

void appBtStereoProfileConnWrap(void)
{
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();
    MSG_T msg;

	os_printf("app_bt_stereo_profile_conn_wrap\r\n");

	jtask_stop( app_h->app_auto_con_task );

    app_bt_flag2_set( APP_FLAG2_STEREO_WORK_MODE, 1 );
    app_bt_flag2_set( APP_FLAG2_STEREO_BUTTON_PRESS|APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_STEREO_AUTOCONN, 0 );

    app_bt_active_conn_profile(PROFILE_ID_AVRCP_CT, (void *)&app_h->stereo_btaddr );

    if(!a2dp_has_connection())
        bt_unit_set_scan_enable( app_h->unit, HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE );

    memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)&app_h->remote_btaddr, sizeof(btaddr_t));

    msg.id = MSG_ENV_WRITE_ACTION;
    msg.arg = 0x01;
    msg.hdl = 0;
    msg_lush_put(&msg);

    //if(!a2dp_has_connection() && !hfp_has_connection())
    	//app_set_led_event_action(LED_EVENT_MATCH_STATE);

    app_bt_flag2_set( APP_FLAG2_MATCH_BUTTON|APP_FLAG2_STEREO_MATCH_BUTTON, 0);
}


#define con_sync_start_flag 7
#define con_audio_interval_xms(val) (val*32/10)

extern void lineinSbcEncodeInit(void);
extern void lineinSbcEncodeDestroy(void);

extern void sbcSrcNumSet(uint8_t num);
extern void open_linein2aux_loop(void);
extern result_t a2dpSrcCmdStreamStart(void);
extern void aud_mic_mute( uint8_t enable );
extern BOOL isA2dpSrcConnected(void);

static BOOL s_is_startAuxTx = FALSE; /* FALSE: not start, TRUE: already started.  */
void setAuxTxStarted(void)
{
    s_is_startAuxTx = TRUE;
}

void unsetAuxTxStarted(void)
{
    s_is_startAuxTx = FALSE;
}

BOOL isAuxTxStarted(void)
{
    return s_is_startAuxTx;
}


uint8_t debug_trace;

void debugSetTraceVal(uint8_t val)
{
	debug_trace = val;
}

int8_t debugShowTrace(uint8_t val)
{
    static uint8_t show_cnt=0,last_val;

    if(debug_trace==val)
    {
    	if(last_val!=val)
    	{
    		last_val = val;
    		show_cnt = 0;
    	}

    	if(show_cnt<=10)
    	{
    		show_cnt++;
    		os_printf("%d",val);
    	}
    	return 1;
    }
    else
    {
    	show_cnt = 0;
    	return 0;
    }
    //os_printf("trace:%d\r\n",val);
}

void appStartLinein2SbcEncode(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    os_printf("appStartLinein2SbcEncode\r\n");

    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);  

    // s1. init sbc encode.
    lineinSbcEncodeInit();

    // s2. start stream.
    jtask_schedule(sys_hdl->app_auto_con_task, 500, (jthread_func)a2dpSrcCmdStreamStart, NULL);
}

void appExitLinein2SbcEncode(void)
{
    os_printf("appExitLinein2SbcEncode\r\n");

    // s1. Destroy sbc encode.
    lineinSbcEncodeDestroy();
}

void appStartAuxTx(void *arg)
{
    os_printf("appStartAuxTx, enter..., %d\r\n", isA2dpSrcConnected());

	if(isA2dpSrcConnected() && (get_a2dp_role() == A2DP_ROLE_AS_SRC))
	{
        os_printf("appStartAuxTx, a2dp_role = SRC\r\n");

        setAuxTxStarted();

        // 1. open linein
        BK3000_Ana_Line_enable(1);

        // 2. open adc.
        aud_mic_mute(1);
		aud_mic_open(1);

        // 3. start encode.
		appStartLinein2SbcEncode();

        app_bt_flag2_set(APP_FLAG_LINEIN, 1 );
        app_bt_flag2_set(APP_FLAG2_STEREO_PLAYING,1);

	}
	else
	{
	    os_printf("appStartAuxTx, a2dp_role = %d \n", get_a2dp_role());
	}
}

void appExitAuxTx(void)
{
    os_printf("appExitAuxTx, enter, a2dp_role = %d, %d \r\n", get_a2dp_role(), isA2dpSrcConnected());

	if(isA2dpSrcConnected() && (get_a2dp_role() == A2DP_ROLE_AS_SRC))
	{
        os_printf("appExitAuxTx, EXEC, a2dp_role = SRC\r\n");

        unsetAuxTxStarted();

        app_bt_flag2_set(APP_FLAG2_STEREO_WORK_MODE, 0);
        app_bt_flag2_set(APP_FLAG2_STEREO_PLAYING, 0);

        // 1. close adc.
        //aud_mic_mute(1);
		//aud_mic_open(0);

        // 2. close encode.
		appExitLinein2SbcEncode();
	}
	else
	{
	    os_printf("appExitAuxTx, a2dp_role = %d, %d \n", get_a2dp_role(), isA2dpSrcConnected());
	}
}


#endif




// EOF
