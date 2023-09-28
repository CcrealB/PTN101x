#include "driver_vusb.h"
//#include "bk3000_reg.h"
#include "bkreg.h"
#include "msg_pub.h"
#include "driver_vusb_dlp.h"
#include "drv_system.h"
#include "app_beken_includes.h"
static t_VUSB_mode s_vusb_state = VUSB_IDLE_MODE;
static t_VUSB_mode s_current_vusb_state = VUSB_IDLE_MODE;
static uint64_t s_w4_comm_mode_tick = 0;
extern uint64_t os_get_tick_counter(void);
uint8_t vusb_is_plug_in(void)
{
    return ((VUSB_REG_STAT_RD & (MSK_VUSB_MORETHAN_1V | MSK_VUSB_MORETHAN_2V | MSK_VUSB_MORETHAN_4V)) ? 1 : 0);
}
uint8_t vusb_is_charging(void)
{
    return ((VUSB_REG_STAT_RD & MSK_VUSB_MORETHAN_4V) ? 1 : 0);
}

uint8_t vusb_4v_ready(void) { return ((VUSB_REG_STAT_RD & MSK_VUSB_MORETHAN_4V) ? 1 : 0); }
uint8_t vusb_2v_ready(void) { return ((VUSB_REG_STAT_RD & MSK_VUSB_MORETHAN_2V) ? 1 : 0); }
uint8_t vusb_1v_ready(void) { return ((VUSB_REG_STAT_RD & MSK_VUSB_MORETHAN_1V) ? 1 : 0); }

void vusb_isr(void)
{
    t_VUSB_mode mode = VUSB_IDLE_MODE;
    //INFO_PRT("VUSB.isr\r\n");
    delay_us(100);
    uint32 t_vusb_stat_rd = VUSB_REG_STAT_RD;
    vusb_int_level(t_vusb_stat_rd);
    if(t_vusb_stat_rd & MSK_VUSB_MORETHAN_4V)
    {
        //INFO_PRT("VUSB.voltage:>4V\r\n");
        mode = VUSB_CHARGE_MODE;    
    }
    else if(t_vusb_stat_rd & MSK_VUSB_MORETHAN_2V)
    {
    	sniffmode_wakeup_dly = 400;
    	app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
        //INFO_PRT("VUSB.voltage:>2V\r\n");
        s_w4_comm_mode_tick = os_get_tick_counter();
        mode = VUSB_W4_COMM_MODE;//VUSB_COMM_MODE;    
    }
    else if(t_vusb_stat_rd & MSK_VUSB_MORETHAN_1V)
    {
        //INFO_PRT("VUSB.voltage:>1V\r\n");
        mode = VUSB_STANDBY_MODE;    
    }    
    else
    {
        //INFO_PRT("VUSB.voltage:<1V\r\n");
        mode = VUSB_IDLE_MODE;    
    }
    vusb_set_mode(mode);
    vusb_mode_stm();
}
t_VUSB_mode vusb_get_mode(void)
{
    return s_vusb_state;
}
uint8 vusb_set_mode(t_VUSB_mode mode)
{
	s_vusb_state = mode;
	return 1;
}
void vusb_set_sdio_enable(void)
{
     VUSB_REG_DLP_ENABLE |= MSK_VUSB_DLP_ENABLE;
}
void vusb_set_sdio_disable(void)
{
    VUSB_REG_DLP_ENABLE &= ~MSK_VUSB_DLP_ENABLE;
}
void vusb_int_enable(uint32 mask)
{
    REG_PMU_0x00 &= ~(MSK_PMU_0x00_VUSB_1V_INTR | MSK_PMU_0x00_VUSB_2V_INTR | MSK_PMU_0x00_VUSB_4V_INTR);
    REG_PMU_0x00 |= mask;
}
void vusb_int_level(uint32 t_vusb_stat_rd)
{
    uint32 vusb_int_lvl = REG_PMU_0x00;
    //uint32 vusb_int_sta = t_vusb_stat_rd & (0x07 << SFT_VUSB_INTSTA_1V);
    
    vusb_int_lvl &= ~(0x07 << SFT_PMU_0x00_VUSB_4V_LVL);
    vusb_int_lvl |= ((((t_vusb_stat_rd & MSK_VUSB_MORETHAN_1V) >> SFT_VUSB_MORETHAN_1V) << SFT_PMU_0x00_VUSB_1V_LVL) \
                    |(((t_vusb_stat_rd & MSK_VUSB_MORETHAN_2V) >> SFT_VUSB_MORETHAN_2V) << SFT_PMU_0x00_VUSB_2V_LVL) \
                    |(((t_vusb_stat_rd & MSK_VUSB_MORETHAN_4V) >> SFT_VUSB_MORETHAN_4V) << SFT_PMU_0x00_VUSB_4V_LVL));
    REG_PMU_0x00 = vusb_int_lvl;
}
void vusb_mode_stm(void)
{
    switch(s_vusb_state)
    {
        case VUSB_IDLE_MODE: 
            //INFO_PRT("VUSB.idle\r\n");
            vusb_set_sdio_disable();
            vusb_dlp_disable();
            break;
        case VUSB_STANDBY_MODE:
            //INFO_PRT("VUSB.standby\r\n");
            vusb_set_sdio_disable();
            vusb_dlp_disable();
            break;
        case VUSB_W4_COMM_MODE:
            if((os_get_tick_counter() - s_w4_comm_mode_tick) > VUSB_W4_COMM_MODE_TIMEOUT)
            {
                vusb_set_mode(VUSB_COMM_MODE);
                msg_put(MSG_VUSB_DETECT);
            }
            else
            {
                vusb_set_mode(VUSB_W4_COMM_MODE);  
                msg_put(MSG_VUSB_DETECT_JITTER);
            }
            break;
        case VUSB_COMM_MODE:
            vusb_set_sdio_enable();
            vusb_dlp_enable();
			if (app_bt_flag2_get(APP_FLAG2_VUSB_DLP_PRINTF))
				vusb_dlp_init(DLP_MEDIUM_BAUDRATE);
			else
            	vusb_dlp_init(DLP_LOWER_MEDIUM_BAUDRATE);
			//INFO_PRT("VUSB.comm\r\n");
            break;
        case VUSB_CHARGE_MODE:
            if(s_current_vusb_state != VUSB_CHARGE_MODE)
            {
                INFO_PRT("VUSB.charge\r\n");
                vusb_set_sdio_disable();
                vusb_dlp_disable();
            }
            break;
        default:break;
    }    
    s_current_vusb_state = s_vusb_state;
}
