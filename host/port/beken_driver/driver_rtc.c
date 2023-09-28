#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "driver_rtc.h"

#if(CONFIG_DRIVER_RTC==1)

static rtc_interrupt_cb rtc_time_cb = NULL;
static rtc_watch_time rtc_time = {0,0,0};

//#define addPMU_Reg0x8											*((volatile unsigned long *) (0x01000800+0x8*4))
//#define addPMU_Reg0x9											*((volatile unsigned long *) (0x01000800+0x9*4))
//#define addPMU_Reg0xa											*((volatile unsigned long *) (0x01000800+0xa*4))
//************************************************************//
//AON_RTC
//************************************************************//
#define BASEADDR_AON_RTC                                        0x01000e00
//addAON_RTC_Reg0x0
#define addAON_RTC_Reg0x0                                       *((volatile unsigned long *) (0x01000e00+0x0*4))
#define posAON_RTC_Reg0x0_rtc_clk_en                            6
#define bitAON_RTC_Reg0x0_rtc_clk_en                            0x40
#define set_AON_RTC_Reg0x0_rtc_clk_en(val)                      addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x40)) | ((val) << 6))
#define setf_AON_RTC_Reg0x0_rtc_clk_en                          addAON_RTC_Reg0x0 |= 0x40
#define clrf_AON_RTC_Reg0x0_rtc_clk_en                          addAON_RTC_Reg0x0 &= ~0x40
#define get_AON_RTC_Reg0x0_rtc_clk_en                           ((addAON_RTC_Reg0x0 & 0x40) >> 6)

#define posAON_RTC_Reg0x0_rtc_tick_int                          5
#define bitAON_RTC_Reg0x0_rtc_tick_int                          0x20
#define set_AON_RTC_Reg0x0_rtc_tick_int(val)                    addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x20)) | ((val) << 5))
#define setf_AON_RTC_Reg0x0_rtc_tick_int                        addAON_RTC_Reg0x0 |= 0x20
#define clrf_AON_RTC_Reg0x0_rtc_tick_int                        addAON_RTC_Reg0x0 &= ~0x20
#define get_AON_RTC_Reg0x0_rtc_tick_int                         ((addAON_RTC_Reg0x0 & 0x20) >> 5)

#define posAON_RTC_Reg0x0_rtc_aon_int                           4
#define bitAON_RTC_Reg0x0_rtc_aon_int                           0x10
#define set_AON_RTC_Reg0x0_rtc_aon_int(val)                     addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x10)) | ((val) << 4))
#define setf_AON_RTC_Reg0x0_rtc_aon_int                         addAON_RTC_Reg0x0 |= 0x10
#define clrf_AON_RTC_Reg0x0_rtc_aon_int                         addAON_RTC_Reg0x0 &= ~0x10
#define get_AON_RTC_Reg0x0_rtc_aon_int                          ((addAON_RTC_Reg0x0 & 0x10) >> 4)

#define posAON_RTC_Reg0x0_rtc_tick_int_en                       3
#define bitAON_RTC_Reg0x0_rtc_tick_int_en                       0x8
#define set_AON_RTC_Reg0x0_rtc_tick_int_en(val)                 addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x8)) | ((val) << 3))
#define setf_AON_RTC_Reg0x0_rtc_tick_int_en                     addAON_RTC_Reg0x0 |= 0x8
#define clrf_AON_RTC_Reg0x0_rtc_tick_int_en                     addAON_RTC_Reg0x0 &= ~0x8
#define get_AON_RTC_Reg0x0_rtc_tick_int_en                      ((addAON_RTC_Reg0x0 & 0x8) >> 3)

#define posAON_RTC_Reg0x0_rtc_aon_int_en                        2
#define bitAON_RTC_Reg0x0_rtc_aon_int_en                        0x4
#define set_AON_RTC_Reg0x0_rtc_aon_int_en(val)                  addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x4)) | ((val) << 2))
#define setf_AON_RTC_Reg0x0_rtc_aon_int_en                      addAON_RTC_Reg0x0 |= 0x4
#define clrf_AON_RTC_Reg0x0_rtc_aon_int_en                      addAON_RTC_Reg0x0 &= ~0x4
#define get_AON_RTC_Reg0x0_rtc_aon_int_en                       ((addAON_RTC_Reg0x0 & 0x4) >> 2)

#define posAON_RTC_Reg0x0_rtc_cnt_stop                          1
#define bitAON_RTC_Reg0x0_rtc_cnt_stop                          0x2
#define set_AON_RTC_Reg0x0_rtc_cnt_stop(val)                    addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x2)) | ((val) << 1))
#define setf_AON_RTC_Reg0x0_rtc_cnt_stop                        addAON_RTC_Reg0x0 |= 0x2
#define clrf_AON_RTC_Reg0x0_rtc_cnt_stop                        addAON_RTC_Reg0x0 &= ~0x2
#define get_AON_RTC_Reg0x0_rtc_cnt_stop                         ((addAON_RTC_Reg0x0 & 0x2) >> 1)

#define posAON_RTC_Reg0x0_rtc_cnt_reset                         0
#define bitAON_RTC_Reg0x0_rtc_cnt_reset                         0x1
#define set_AON_RTC_Reg0x0_rtc_cnt_reset(val)                   addAON_RTC_Reg0x0 = ((addAON_RTC_Reg0x0 & (~0x1)) | ((val) << 0))
#define setf_AON_RTC_Reg0x0_rtc_cnt_reset                       addAON_RTC_Reg0x0 |= 0x1
#define clrf_AON_RTC_Reg0x0_rtc_cnt_reset                       addAON_RTC_Reg0x0 &= ~0x1
#define get_AON_RTC_Reg0x0_rtc_cnt_reset                        (addAON_RTC_Reg0x0 & 0x1)

//addAON_RTC_Reg0x1/up_val
#define addAON_RTC_Reg0x1                                       *((volatile unsigned long *) (0x01000e00+0x1*4))
//addAON_RTC_Reg0x2/tick_val
#define addAON_RTC_Reg0x2                                       *((volatile unsigned long *) (0x01000e00+0x2*4))
//addAON_RTC_Reg0x3/cnt_val
#define addAON_RTC_Reg0x3                                       *((volatile unsigned long *) (0x01000e00+0x3*4))
//addAON_RTC_Reg0x4
#define addAON_RTC_Reg0x4                                       *((volatile unsigned long *) (0x01000e00+0x4*4))
//addAON_RTC_Reg0x5
#define addAON_RTC_Reg0x5                                       *((volatile unsigned long *) (0x01000e00+0x5*4))

#define addAON_PMU_Reg0x0                                       *((volatile unsigned long *) (0x01000800+0x0*4))
#define SYSTEM_INT_ENABLE_L										*((volatile unsigned long *) (0x01000000+0x9*4))

void rtc_band_calibration(void)
{
	//REG_SYSTEM_0x4D |= (1<<22);//RC32K band manual enable	
	//sys_delay_ms(50);//wait for rc calibration

	REG_SYSTEM_0x4D |= (0<<24);//0-->1trigger calibration	
	sys_delay_ms(50);//wait for rc calibration
	REG_SYSTEM_0x4D |= (1<<24);//0-->1trigger calibration		
	sys_delay_ms(50);//wait for rc calibration
}

void rtc_lpoclk_sel(uint32 sel)
{
	if(sel)
	{
		addAON_PMU_Reg0x0 |= (1<<6);//lpoclk=32kd
	}
	else
	{
		addAON_PMU_Reg0x0 |= (0x1<<7);//[7:6]=10 sel clk_ROSC
		addAON_PMU_Reg0x0 &=~(0x1<<6);//[7:6]=10 sel clk_ROSC
	}
}

void rtc_initial(uint32 value, uint32 tick)
{
	clrf_AON_RTC_Reg0x0_rtc_aon_int_en;
	setf_AON_RTC_Reg0x0_rtc_cnt_reset;
	while(addAON_RTC_Reg0x3 != 0x0){;}
	setf_AON_RTC_Reg0x0_rtc_clk_en;
	addAON_RTC_Reg0x1 = value;//up_val
	while(addAON_RTC_Reg0x4 != addAON_RTC_Reg0x1){;}
	addAON_RTC_Reg0x3 = 0x25;//tick_val
	while(addAON_RTC_Reg0x5 != addAON_RTC_Reg0x2){;}
    setf_AON_RTC_Reg0x0_rtc_aon_int;
    if(tick){setf_AON_RTC_Reg0x0_rtc_tick_int;}
    setf_AON_RTC_Reg0x0_rtc_aon_int_en;
    if(tick){setf_AON_RTC_Reg0x0_rtc_tick_int_en;}

    SYSTEM_INT_ENABLE_L |= (1<<1);//rtc_int enable
    clrf_AON_RTC_Reg0x0_rtc_cnt_stop;//rtc_cnt_stop=0
    clrf_AON_RTC_Reg0x0_rtc_cnt_reset;
	rtc_register_cb(rtc_get_time);

}

void rtc_initial_deepsleep(uint32 value, uint32 tick)
{

    //uint32 value1;
     
	clrf_AON_RTC_Reg0x0_rtc_aon_int_en;
	setf_AON_RTC_Reg0x0_rtc_cnt_reset;
	while(addAON_RTC_Reg0x3 != 0x0){;}

	setf_AON_RTC_Reg0x0_rtc_clk_en;
	addAON_RTC_Reg0x1 = value;//up_val
	
	
	
	while(addAON_RTC_Reg0x4 != addAON_RTC_Reg0x1){;}
	addAON_RTC_Reg0x3 = 0x25;//tick_val
	while(addAON_RTC_Reg0x5 != addAON_RTC_Reg0x2){;}
    setf_AON_RTC_Reg0x0_rtc_aon_int;
    if(tick){setf_AON_RTC_Reg0x0_rtc_tick_int;}
    setf_AON_RTC_Reg0x0_rtc_aon_int_en;
    if(tick){setf_AON_RTC_Reg0x0_rtc_tick_int_en;}
    SYSTEM_INT_ENABLE_L |= (1<<1);//rtc_int enable

    clrf_AON_RTC_Reg0x0_rtc_cnt_stop;//rtc_cnt_stop=0
    clrf_AON_RTC_Reg0x0_rtc_cnt_reset;

    //if after waked up set the sleep cnt, add the following or set next time wake up value(continue counting and not affected by system reset)
    while(addAON_RTC_Reg0x3 == 0x0){;}
    addAON_RTC_Reg0x1 = 0x0;
}

void clr_rtc_initial_norm(void)
{
	//clrf_AON_RTC_Reg0x0_rtc_tick_int_en;
	//clrf_AON_RTC_Reg0x0_rtc_aon_int_en;
	//setf_AON_RTC_Reg0x0_rtc_clk_en;
	addAON_RTC_Reg0x1 = 0;//up_val
	setf_AON_RTC_Reg0x0_rtc_cnt_reset;
	while(addAON_RTC_Reg0x4 != addAON_RTC_Reg0x1){;}
	while(addAON_RTC_Reg0x3!=0x0){;}
}

void rtc_stop(void)
{
	setf_AON_RTC_Reg0x0_rtc_cnt_stop;
}

void rtc_rtc_cnt_reset(void)
{
	clrf_AON_RTC_Reg0x0_rtc_cnt_reset;
}


extern uint8_t syspwr_cpu_halt;
void rtc_isr(void)
{

     rtc_time_cb(&rtc_time);
	#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
	//gpio_output_reverse(38);
    SYSpwr_Wakeup_From_Sleep_Mode();
	#endif
	
    if(get_AON_RTC_Reg0x0_rtc_aon_int ) 
	{
        setf_AON_RTC_Reg0x0_rtc_aon_int ;//rtc_aon_int=1
        //gpio_output_reverse(38);
    }
    if(get_AON_RTC_Reg0x0_rtc_tick_int) 
	{
        setf_AON_RTC_Reg0x0_rtc_tick_int;//rtc_tick_int=1
    }
	#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
	syspwr_cpu_halt = 0;
	#endif
	
	

	
}



uint8_t rtc_sleep(void)
{
	uint32 rtc_time=0;
	
	rtc_time = addAON_RTC_Reg0x1-addAON_RTC_Reg0x3;
	if(rtc_time>30)//100us
	{
		return 0;
	}
	else
	{
		//gpio_output_reverse(16);
		return 1;
	}
}

uint32 rtc_get_value(void)
{
	return(addAON_RTC_Reg0x3);
}	
uint32 rtc_get_value2(void)
{
	return(addAON_RTC_Reg0x2);
}


uint32 rtc_get_value1(void)
{
	return(addAON_RTC_Reg0x1);
}



void rtc_register_cb(rtc_interrupt_cb cb)
{
    if(cb == NULL)
	{
		os_printf("cb para null\r\n");
		return ;
	}
	
	 	rtc_time_cb = cb;
}

void rtc_get_time(rtc_watch_time* time)
{
	 
	if(time == NULL)
	{
		os_printf("time para null\r\n");
		return ;
	}
	
	 time->second ++;
	if(time->second >= 60)
	{
		time->second = 0;
		time->minute ++;
		
		if(time->minute >= 60)
		{
			time->minute = 0;
			time->hour ++;
			if(time->hour >= 24)
			time->hour = 0;
		}
	}
	//os_printf("h=%d,m=%d,s=%d\r\n",time->hour,time->minute,time->second);
}


#endif
