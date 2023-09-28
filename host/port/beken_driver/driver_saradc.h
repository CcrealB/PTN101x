#ifndef _DRIVER_SARADC_H_
#define _DRIVER_SARADC_H_

#define SARADC_CRITICAL_CODE_PROTECT
// #define SADC_DEBUG

#ifdef SADC_DEBUG
    #define SADC_PRT      os_printf
#else
    #define SADC_PRT      os_null_printf
#endif


#define SARADC_CHARGER_THRESHOLD_COUNT        3
#define SARADC_MODE_SLEEP       0
#define SARADC_MODE_SINGLESTEP  1
#define SARADC_MODE_SOFTWARE    2
#define SARADC_MODE_CONTINUE    3


#define SARADC_FIFO_EMPTY    (1 << 6 )
#define SARADC_ADC_BUSY      (1 << 7 )

#define SARADC_ADC_SAMPLE_DIV36   (0 << 8)
#define SARADC_ADC_SAMPLE_DIV18   (1 << 8)

#define SARADC_START_DELAY      (1 << 10)

#define SARADC_CLK_DIV4         0
#define SARADC_CLK_DIV8         1
#define SARADC_CLK_DIV16        2
#define SARADC_CLK_DIV32        3
#define SARADC_CLK_DIV64        4
#define SARADC_CLK_DIV128       5
#define SARADC_CLK_DIV256       6
#define SARADC_CLK_DIV512       7

#define sft_SARADC_CHANNEL             3
#define bit_SARADC_DELAY              10
#define bit_SARADC_CLKDIV           16


#define SARADC_BELOW_COUNT      10
#define SARADC_BELOW_THRESHOLD  3000//0x1d0  // about 2.6V
 
#define SARADC_BIGC_THRESHOLD   4000//0x210 4.0V
#define SARADC_FULL_THRESHOLD   4200//0x2D0 4.2V
#define SARADC_VBAT_CHARGE_FULL 4150

#define SARADC_CALI_THRESHOLD   10


#define SARADC_TRK_THRESHOLD   30
#define SARADC_LC_THRESHOLD    38
#define SARADC_CV_THRESHOLD    41
#define SARADC_DELT            2
#define SARADC_END_THRESHOLD   43

#define SarADC_CALC_MODE        1 //0:get last value, 1:get avg value(remove max&min), ++by Borg@230707
#define SarADC_AVG_NUM          4 //1~32(4 or 8 is may good, don't less than 3 if SarADC_CALC_MODE == 1)
//adc_clk= clk/[2*(pre_div+1)] -> div=2*(pre_div+1)
#define SarADC_CLK_13MHz        13000000//div2, pre_div=0
#define SarADC_CLK_6M5Hz        6500000 //div4, pre_div=1
#define SarADC_CLK_4p33MHz      4333333 //div6, pre_div=2
#define SarADC_CLK_3p25MHz      3250000 //div8, pre_div=3
#define SarADC_CLK_2p6MHz       2600000 //div10, pre_div=4
#define SarADC_CLK_1MHz         1000000 //div26, pre_div=12
#define SarADC_CLK_DIV(Hz)      ((uint8_t)((26000000 / 2) / Hz - 1) & 0x3F)

#define SARADC_INTR_IS_ON     (!!(REG_SYSTEM_0x09 | SYS_PERI_IRQ_SADC))

#ifdef SARADC_CRITICAL_CODE_PROTECT
    #define SARADC_CRITICAL_CODE(en)    do{ REG_SYSTEM_0x09 = en ?  (REG_SYSTEM_0x09 & ~SYS_PERI_IRQ_SADC) : (REG_SYSTEM_0x09 | SYS_PERI_IRQ_SADC);}while(0)
#else
    #define SARADC_CRITICAL_CODE(en)
#endif

enum
{
    SARADC_CHARGER_MODE_LC = 0,
    SARADC_CHARGER_MODE_BC = 1,
    SARADC_CHARGER_MODE_ALMOST_FULL = 2,
    SARADC_CHARGER_MODE_NONE = 3
};

typedef enum _SARADC_CH_e
{
    SARADC_CH_VREF_GND = 0,
    SARADC_CH_VREF_VBG = 1,
    SARADC_CH_VBAT = 2,
    SARADC_CH_TEMPERATURE = 3,
    SARADC_CH_VUSB = 4,
    SARADC_CH_NC1 = 5,
    SARADC_CH_NC2 = 6,
    SARADC_CH_NC3 = 7,
    SARADC_CH_GPIO11 = 8,
    SARADC_CH_GPIO18 = 9,
    SARADC_CH_GPIO19 = 10,
    SARADC_CH_GPIO20 = 11,
    SARADC_CH_GPIO21 = 12,
    SARADC_CH_GPIO22 = 13,
    SARADC_CH_GPIO23 = 14,
    SARADC_CH_GPIO31 = 15,
    SARADC_CH_GPIO32 = 16,
    SARADC_CH_GPIO34 = 17,
    SARADC_CH_GPIO36 = 18,
    SARADC_CH_GPIO37 = 19,
    SARADC_CH_GPIO38 = 20,
    SARADC_CH_GPIO39 = 21,    
    SARADC_CH_TXLOAMP = 22,
    SARADC_CH_TXMODRSSI = 23,
    SARADC_CH_TXRSSI = 24,
    SARADC_CH_TOUCHIN = 25
}SARADC_CH_e;

void saradc_refer_select(uint8_t mode);
void saradc_init( int mode, int channel, int div );
int saradc_lowpower_status( void );
int saradc_normalpower_status( void );
int saradc_set_lowpower_para( int interval, int threshold_lowpower, int threshold_powerdown );
void saradc_isr( void );
void saradc_reset( void );
uint16_t saradc_get_value(void);//int saradc_get_value(void);
uint16_t saradc_get_bat_value(void);
uint16_t SarADC_val_cali_get(uint16_t adc_val);
uint16_t SarADC_val_to_voltage(uint16_t adc_val);
uint16_t SarADC_voltage_to_val(uint16_t volt_mv);
void saradc_calibration_first(void);
void saradc_start_working(void);
int saradc_set_charger_para( int threshold_bcur, int threshold_full );
int saradc_charger_status( void );
uint16_t saradc_transf_adc_to_vol(uint16_t data);
uint8_t saradc_calibration_end(void);
void temperature_saradc_enable(void);
void temperature_saradc_update(uint16_t data);
int temperature_saradc_data_get(uint16_t *data, int num);
__inline void saradc_set_chnl_busy(uint8_t busy);
__inline uint8_t saradc_get_chnl_busy(void);
#endif
