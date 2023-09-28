
#include "u_include.h"

#if TICK_TIMER_EXTEND_EN /* Tick Timer */
#include "bautil.h"
#include "spr_defs.h"

#define TT_PRINTF       DBG_LOG_INFO

//set Tick Timer mode
#define SET_TICK_TIM_MOD_DIS        set_spr(SPR_TTMR, get_spr(SPR_TTMR) & ~SPR_TTMR_M)//disable Tick Timer
#define SET_TICK_TIM_MOD(mode)      set_spr(SPR_TTMR, get_spr(SPR_TTMR) | mode)//set tick mode
#define SET_TICK_TIM_MOD_REST       do{SET_TICK_TIM_MOD_DIS; SET_TICK_TIM_MOD(SPR_TTMR_RT);}while(0) //Restart tick mode, 28bit timer
#define SET_TICK_TIM_MOD_SRUN       do{SET_TICK_TIM_MOD_DIS; SET_TICK_TIM_MOD(SPR_TTMR_SR);}while(0) //Single run mode, 28bit timer
#define SET_TICK_TIM_MOD_CONT       do{SET_TICK_TIM_MOD_DIS; SET_TICK_TIM_MOD(SPR_TTMR_CR);}while(0) //Continuous run mode, 32bit timer

//set Tick/Instruction conting mode
#define SET_TT_CNT_MOD_TICK    set_spr(SPR_TTUR, get_spr(SPR_TTUR) & ~SPR_TTUR_ICTM)//set Tick Counting Timer Mode
#define SET_TT_CNT_MOD_INS     set_spr(SPR_TTUR, get_spr(SPR_TTUR) | SPR_TTUR_ICTM)//set Instruction Counting Timer Mode

void hw_tick_timer_init(void)
{
    TT_PRINTF("tick_timer_exist:%d\n", !!(get_spr(SPR_UPR) & SPR_UPR_TTP));

    SET_TICK_TIM_MOD_CONT;

    SET_TT_CNT_MOD_TICK;
    // SET_TT_CNT_MOD_INS;//invalid?

    TT_PRINTF("SPR_TTMR = 0x%08X\n",get_spr(SPR_TTMR));
    TT_PRINTF("SPR_TTCR = 0x%08X\n",get_spr(SPR_TTCR));
    TT_PRINTF("SPR_TTRS = 0x%08X\n",get_spr(SPR_TTRS));
    TT_PRINTF("SPR_TTMTS = 0x%08X 0x%08X\n", get_spr(SPR_TTMTS_HI), get_spr(SPR_TTMTS_LO));
    TT_PRINTF("SPR_TTUR = 0x%08X\n",get_spr(SPR_TTUR));
    // cpu_set_timer_enabled(1);
}
#endif /* Tick Timer */

#define TIMER_VAL_GET       timer_cnt_val_get(TIMER0, TIMER_CH2)

void delay_test_timer0(char *fname, void (*cb_func)(uint32_t), uint32_t n)
{
    uint32_t tick_start = 0;
    uint32_t tick_stop = 0;
    uint32_t tick_diff = 0;

    DBG_LOG_INFO("\n\n====%s(%s, %d)\n", __FUNCTION__, fname, n);
    tick_start = TIMER_VAL_GET;
    cb_func(n);
    tick_stop = TIMER_VAL_GET;
    tick_diff = tick_stop - tick_start;
    DBG_LOG_INFO("start H:0x%08X O:%10lu\n", tick_start, tick_start);
    DBG_LOG_INFO("stop  H:0x%08X O:%10lu\n", tick_stop, tick_stop);
    DBG_LOG_INFO("diff  H:0x%08X O:%10lu\n", tick_diff, tick_diff);
    // DBG_LOG_INFO("start:0x%lu, stop:0x%lu dif:0x%luus\n", tick_start, tick_stop, tick_diff);
    DBG_LOG_INFO("TIMER_CH2:0x%08X\n", TIMER_VAL_GET);
}

#if 0

#if TICK_TIMER_EXTEND_EN
void delay_test_tt_extend(uint8_t ms)
{
    uint32_t tick_start = 0;
    uint32_t tick_stop = 0;
    uint32_t tick_diff = 0;

    DBG_LOG_INFO("\n\n====%s()\n", __FUNCTION__);
    tick_start = HW_SYS_TICK_GET();
    os_delay_ms(ms);
    tick_stop = HW_SYS_TICK_GET();
    tick_diff = (tick_stop - tick_start) / 110;//us
    DBG_LOG_INFO("start:0x%08X, stop:0x%08X dif:0x%08X\n", tick_start, tick_stop, tick_diff);
    DBG_LOG_INFO("HW_SYS_TICK_GET:0x%08X\n", HW_SYS_TICK_GET());
    // DBG_LOG_INFO("tick H:0x%08X L:0x%08X\n", HW_SYS_TICK_GETH(), HW_SYS_TICK_GETL());
}
#endif

void delay_test_tt_timestamp(uint8_t ms)
{
    uint64_t tick_start = 0;
    uint64_t tick_stop = 0;
    uint64_t tick_diff = 0;

    DBG_LOG_INFO("\n\n====%s()\n", __FUNCTION__);
    tick_start = HW_SYS_TICK_GET64b();
    os_delay_ms(ms);
    tick_stop = HW_SYS_TICK_GET64b();
    tick_diff = (tick_stop - tick_start) / 110;//us
    DBG_LOG_INFO("start H:0x%08X, L:0x%08X\n", (tick_start >> 32), tick_start);
    DBG_LOG_INFO("stop H:0x%08X, L:0x%08X\n", (tick_stop >> 32), tick_stop);
    DBG_LOG_INFO("diff H:0x%08X, L:0x%08X\n", (tick_diff >> 32), tick_diff);
    // DBG_LOG_INFO("HW_SYS_TICK_GET:0x%08X\n", HW_SYS_TICK_GET());
    DBG_LOG_INFO("tick H:0x%08X L:0x%08X\n", HW_SYS_TICK_GETH(), HW_SYS_TICK_GETL());
}
#endif
