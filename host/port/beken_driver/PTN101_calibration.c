/**
 **************************************************************************************
 * @file    bk3266_calibration.c
 * @brief   Calibrations for BK3266, such as audio dac DC offset, charger, sar-adc, tx, etc
 * 
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
//#include "types.h"
#include "sys_config.h"
#include "bautil.h"
#include "sys_irq.h"
#include "bkreg.h"
#include "drv_system.h"
#include "driver_efuse.h"
#include "compiler_arm.h"
#include "app_tlv.h"
#include "driver_flash.h"

#define CALIB_DBG_ENABLE                (0)
extern int32_t os_printf(const char *fmt, ...);
#if     CALIB_DBG_ENABLE
#define CALIB_DBG(fmt, ...)             os_printf("[CALIB]: "fmt, ##__VA_ARGS__)
#else
#define CALIB_DBG(fmt, ...)
#endif

#define TX_CALIB_ALGORITHM_ERGODIC      (0)
#define TX_CALIB_ALGORITHM_DICHOTOMY    (1)
#define CFG_TX_CALIB_ALGORITHM          (TX_CALIB_ALGORITHM_ERGODIC)

#define SAR_ADC_READ_AVG_COUNT          (2)
#define SAR_ADC_WAIT_BUSY_COUNT         (5000)

#define ADDR_SYSCTRL_BASE            (0x01000000)
#define ADDR_SYSCTRL_0X1            (ADDR_SYSCTRL_BASE + 0x1 * 4)
#define REG_SYSCTRL_0X1                       (*(volatile uint32_t*)ADDR_SYSCTRL_0X1)
#define ADDR_SYSCTRL_0X3            (ADDR_SYSCTRL_BASE + 0x3 * 4)
#define REG_SYSCTRL_0X3                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X3)
#define ADDR_SYSCTRL_0X5            (ADDR_SYSCTRL_BASE + 0x5 * 4)
#define REG_SYSCTRL_0X5                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X5)
#define ADDR_SYSCTRL_0X6            (ADDR_SYSCTRL_BASE + 0x6 * 4)
#define REG_SYSCTRL_0X6                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X6)
#define ADDR_SYSCTRL_0X31            (ADDR_SYSCTRL_BASE + 0x31 * 4)
#define REG_SYSCTRL_0X31                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X31)
#define ADDR_SYSCTRL_0X32           (ADDR_SYSCTRL_BASE + 0x32 * 4)
#define REG_SYSCTRL_0X32                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X32)
#define ADDR_SYSCTRL_0X40            (ADDR_SYSCTRL_BASE + 0x40 * 4)
#define REG_SYSCTRL_0X40                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X40)
#define ADDR_SYSCTRL_0X43            (ADDR_SYSCTRL_BASE + 0x43 * 4)
#define REG_SYSCTRL_0X43                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X43)
#define ADDR_SYSCTRL_0X44            (ADDR_SYSCTRL_BASE + 0x44 * 4)
#define REG_SYSCTRL_0X44                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X44)
#define ADDR_SYSCTRL_0X46            (ADDR_SYSCTRL_BASE + 0x46 * 4)
#define REG_SYSCTRL_0X46                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X46)
#define ADDR_SYSCTRL_0X59            (ADDR_SYSCTRL_BASE + 0x59 * 4)
#define REG_SYSCTRL_0X59                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X59)
#define ADDR_SYSCTRL_0X5B            (ADDR_SYSCTRL_BASE + 0x5B * 4)
#define REG_SYSCTRL_0X5B                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X5B)
#define ADDR_SYSCTRL_0X5C           (ADDR_SYSCTRL_BASE + 0x5c * 4)
#define REG_SYSCTRL_0X5C                        (*(volatile uint32_t*)ADDR_SYSCTRL_0X5C)


uint32_t  REG_SYSCTRL_0X3_BAK = 0;
uint32_t  REG_SYSCTRL_0X5_BAK = 0;
uint32_t  REG_SYSCTRL_0X6_BAK = 0;
uint32_t  REG_SYSCTRL_0X40_BAK = 0;
uint32_t  REG_SYSCTRL_0X46_BAK = 0;
uint32_t  REG_SYSCTRL_0X59_BAK = 0;
uint32_t  REG_SYSCTRL_0X5C_BAK = 0;


#define ADDR_GPIO_BASE                                    0x01000C00
#define ADDR_GPIO_0x0                  (ADDR_GPIO_BASE + 0x0 * 4)
#define ADDR_GPIO_0x1                  (ADDR_GPIO_BASE + 0x1 * 4)
#define ADDR_GPIO_0x2                  (ADDR_GPIO_BASE + 0x2 * 4)
#define ADDR_GPIO_0x3                  (ADDR_GPIO_BASE + 0x3 * 4)
#define ADDR_GPIO_0x4                  (ADDR_GPIO_BASE + 0x4 * 4)
#define ADDR_GPIO_0x5                  (ADDR_GPIO_BASE + 0x5 * 4)
#define ADDR_GPIO_0x6                  (ADDR_GPIO_BASE + 0x6 * 4)
#define ADDR_GPIO_0x7                  (ADDR_GPIO_BASE + 0x7 * 4)
#define ADDR_GPIO_0x8                  (ADDR_GPIO_BASE + 0x8 * 4)
#define ADDR_GPIO_0x9                  (ADDR_GPIO_BASE + 0x9 * 4)
#define ADDR_GPIO_0xa                  (ADDR_GPIO_BASE + 0xa * 4)
#define ADDR_GPIO_0xb                  (ADDR_GPIO_BASE + 0xb * 4)
#define ADDR_GPIO_0xc                  (ADDR_GPIO_BASE + 0xc * 4)
#define ADDR_GPIO_0xd                  (ADDR_GPIO_BASE + 0xd * 4)
#define ADDR_GPIO_0xe                  (ADDR_GPIO_BASE + 0xe * 4)
#define ADDR_GPIO_0xf                   (ADDR_GPIO_BASE + 0xf * 4)

#define ADDR_GPIO_0x30                  (ADDR_GPIO_BASE + 0x30 * 4)
#define ADDR_GPIO_0x31                  (ADDR_GPIO_BASE + 0x31 * 4)
#define ADDR_GPIO_0x32                  (ADDR_GPIO_BASE + 0x32 * 4)
#define ADDR_GPIO_0x33                  (ADDR_GPIO_BASE + 0x33 * 4)
#define ADDR_GPIO_0x34                  (ADDR_GPIO_BASE + 0x34 * 4)
#define ADDR_GPIO_0x35                  (ADDR_GPIO_BASE + 0x35 * 4)
#define ADDR_GPIO_0x36                  (ADDR_GPIO_BASE + 0x36 * 4)
#define ADDR_GPIO_0x37                  (ADDR_GPIO_BASE + 0x37 * 4)
#define ADDR_GPIO_0x3C                  (ADDR_GPIO_BASE + 0x3C * 4)

#define ADDR_XVR_BASE                   (0x01910000)
#define ADDR_XVR_0x00                   (ADDR_XVR_BASE + 0x00 * 4)
#define ADDR_XVR_0x01                   (ADDR_XVR_BASE + 0x01 * 4)
#define ADDR_XVR_0x02                   (ADDR_XVR_BASE + 0x02 * 4)
#define ADDR_XVR_0x03                   (ADDR_XVR_BASE + 0x03 * 4)
#define ADDR_XVR_0x04                   (ADDR_XVR_BASE + 0x04 * 4)
#define ADDR_XVR_0x05                   (ADDR_XVR_BASE + 0x05 * 4)
#define ADDR_XVR_0x06                   (ADDR_XVR_BASE + 0x06 * 4)
#define ADDR_XVR_0x07                   (ADDR_XVR_BASE + 0x07 * 4)
#define ADDR_XVR_0x08                   (ADDR_XVR_BASE + 0x08 * 4)
#define ADDR_XVR_0x09                   (ADDR_XVR_BASE + 0x09 * 4)
#define ADDR_XVR_0x0A                   (ADDR_XVR_BASE + 0x0A * 4)
#define ADDR_XVR_0x0B                   (ADDR_XVR_BASE + 0x0B * 4)
#define ADDR_XVR_0x0C                   (ADDR_XVR_BASE + 0x0C * 4)
#define ADDR_XVR_0x0D                   (ADDR_XVR_BASE + 0x0D * 4)
#define ADDR_XVR_0x0E                   (ADDR_XVR_BASE + 0x0E * 4)
#define ADDR_XVR_0x0F                   (ADDR_XVR_BASE + 0x0F * 4)
#define ADDR_XVR_0x10                   (ADDR_XVR_BASE + 0x10 * 4)
#define ADDR_XVR_0x11                   (ADDR_XVR_BASE + 0x11 * 4)
#define ADDR_XVR_0x12                   (ADDR_XVR_BASE + 0x12 * 4)
#define ADDR_XVR_0x13                   (ADDR_XVR_BASE + 0x13 * 4)
#define ADDR_XVR_0x14                   (ADDR_XVR_BASE + 0x14 * 4)
#define ADDR_XVR_0x15                   (ADDR_XVR_BASE + 0x15 * 4)
#define ADDR_XVR_0x16                   (ADDR_XVR_BASE + 0x16 * 4)
#define ADDR_XVR_0x17                   (ADDR_XVR_BASE + 0x17 * 4)
#define ADDR_XVR_0x18                   (ADDR_XVR_BASE + 0x18 * 4)
#define ADDR_XVR_0x19                   (ADDR_XVR_BASE + 0x19 * 4)
#define ADDR_XVR_0x1A                   (ADDR_XVR_BASE + 0x1A * 4)
#define ADDR_XVR_0x1B                   (ADDR_XVR_BASE + 0x1B * 4)
#define ADDR_XVR_0x1C                   (ADDR_XVR_BASE + 0x1C * 4)
#define ADDR_XVR_0x1D                   (ADDR_XVR_BASE + 0x1D * 4)
#define ADDR_XVR_0x1E                   (ADDR_XVR_BASE + 0x1E * 4)
#define ADDR_XVR_0x1F                   (ADDR_XVR_BASE + 0x1F * 4)
#define ADDR_XVR_0x20                   (ADDR_XVR_BASE + 0x20 * 4)
#define ADDR_XVR_0x21                   (ADDR_XVR_BASE + 0x21 * 4)
#define ADDR_XVR_0x22                   (ADDR_XVR_BASE + 0x22 * 4)
#define ADDR_XVR_0x23                   (ADDR_XVR_BASE + 0x23 * 4)
#define ADDR_XVR_0x24                   (ADDR_XVR_BASE + 0x24 * 4)
#define ADDR_XVR_0x25                   (ADDR_XVR_BASE + 0x25 * 4)
#define ADDR_XVR_0x26                   (ADDR_XVR_BASE + 0x26 * 4)
#define ADDR_XVR_0x27                   (ADDR_XVR_BASE + 0x27 * 4)
#define ADDR_XVR_0x28                   (ADDR_XVR_BASE + 0x28 * 4)
#define ADDR_XVR_0x29                   (ADDR_XVR_BASE + 0x29 * 4)
#define ADDR_XVR_0x2A                   (ADDR_XVR_BASE + 0x2A * 4)
#define ADDR_XVR_0x2B                   (ADDR_XVR_BASE + 0x2B * 4)
#define ADDR_XVR_0x2C                   (ADDR_XVR_BASE + 0x2C * 4)
#define ADDR_XVR_0x2D                   (ADDR_XVR_BASE + 0x2D * 4)
#define ADDR_XVR_0x2E                   (ADDR_XVR_BASE + 0x2E * 4)
#define ADDR_XVR_0x2F                   (ADDR_XVR_BASE + 0x2F * 4)
#define ADDR_XVR_0x30                   (ADDR_XVR_BASE + 0x30 * 4)
#define ADDR_XVR_0x31                   (ADDR_XVR_BASE + 0x31 * 4)
#define ADDR_XVR_0x32                   (ADDR_XVR_BASE + 0x32 * 4)
#define ADDR_XVR_0x33                   (ADDR_XVR_BASE + 0x33 * 4)
#define ADDR_XVR_0x34                   (ADDR_XVR_BASE + 0x34 * 4)
#define ADDR_XVR_0x35                   (ADDR_XVR_BASE + 0x35 * 4)
#define ADDR_XVR_0x36                   (ADDR_XVR_BASE + 0x36 * 4)
#define ADDR_XVR_0x37                   (ADDR_XVR_BASE + 0x37 * 4)
#define ADDR_XVR_0x38                   (ADDR_XVR_BASE + 0x38 * 4)
#define ADDR_XVR_0x39                   (ADDR_XVR_BASE + 0x39 * 4)
#define ADDR_XVR_0x3A                   (ADDR_XVR_BASE + 0x3A * 4)
#define ADDR_XVR_0x3B                   (ADDR_XVR_BASE + 0x3B * 4)
#define ADDR_XVR_0x3C                   (ADDR_XVR_BASE + 0x3C * 4)
#define ADDR_XVR_0x3D                   (ADDR_XVR_BASE + 0x3D * 4)
#define ADDR_XVR_0x3E                   (ADDR_XVR_BASE + 0x3E * 4)
#define ADDR_XVR_0x3F                   (ADDR_XVR_BASE + 0x3F * 4)
#define ADDR_XVR_0x40                   (ADDR_XVR_BASE + 0x40 * 4)
#define ADDR_XVR_0x41                   (ADDR_XVR_BASE + 0x41 * 4)
#define ADDR_XVR_0x42                   (ADDR_XVR_BASE + 0x42 * 4)
#define ADDR_XVR_0x43                   (ADDR_XVR_BASE + 0x43 * 4)
#define ADDR_XVR_0x44                   (ADDR_XVR_BASE + 0x44 * 4)
#define ADDR_XVR_0x45                   (ADDR_XVR_BASE + 0x45 * 4)
#define ADDR_XVR_0x46                   (ADDR_XVR_BASE + 0x46 * 4)
#define ADDR_XVR_0x47                   (ADDR_XVR_BASE + 0x47 * 4)
#define ADDR_XVR_0x48                   (ADDR_XVR_BASE + 0x48 * 4)
#define ADDR_XVR_0x49                   (ADDR_XVR_BASE + 0x49 * 4)
#define ADDR_XVR_0x4A                   (ADDR_XVR_BASE + 0x4A * 4)
#define ADDR_XVR_0x4B                   (ADDR_XVR_BASE + 0x4B * 4)
#define ADDR_XVR_0x4C                   (ADDR_XVR_BASE + 0x4C * 4)
#define ADDR_XVR_0x4D                   (ADDR_XVR_BASE + 0x4D * 4)
#define ADDR_XVR_0x4E                   (ADDR_XVR_BASE + 0x4E * 4)
#define ADDR_XVR_0x4F                   (ADDR_XVR_BASE + 0x4F * 4)


#define  ADDR_SADC_BASE                         0x01870000
#define  ADDR_SADC_CONFIG_0                (ADDR_SADC_BASE + 0x00 * 4)
#define  ADDR_SADC_RAW_DATA              (ADDR_SADC_BASE + 0x01 * 4)
#define  ADDR_SADC_CONFIG_1                (ADDR_SADC_BASE + 0x02 * 4)
#define  ADDR_SADC_CONFIG_2                (ADDR_SADC_BASE + 0x03 * 4)
#define  ADDR_SADC_DATA_16                  (ADDR_SADC_BASE + 0x04 * 4)

#define  SADC_CONFIG_0                             (*(volatile unsigned long*)(ADDR_SADC_BASE + 0x00 * 4))
#define  SADC_RAW_DATA                           (*(volatile unsigned long*)(ADDR_SADC_BASE + 0x01 * 4))
#define  SADC_CONFIG_1                            (*(volatile unsigned long*)(ADDR_SADC_BASE + 0x02 * 4))
#define  SADC_CONFIG_2                            (*(volatile unsigned long*)(ADDR_SADC_BASE + 0x03 * 4))
#define  SADC_DATA_16                              (*(volatile unsigned long*)(ADDR_SADC_BASE + 0x04 * 4))

//#define TX_Q_CONST_IQCAL_P              (0x50)
//#define TX_Q_CONST_IQCAL_P_4_PHASE      (57)

uint32_t TX_Q_CONST_IQCAL_P         = 0x50;
uint32_t TX_Q_CONST_IQCAL_P_4_PHASE = 57;

extern uint32_t XVR_analog_reg_save[16];
//static uint32_t XVR_ANALOG_REG_BAK[16];
#define XVR_ANALOG_REG_BAK  XVR_analog_reg_save
//


uint8_t  calib_charger_vlcf = 0;
uint8_t  calib_charger_icp  = 0;
uint8_t  calib_charger_vcv  = 0;

uint32_t calib_sar_adc_dc   = 0;
uint32_t calib_sar_adc_4p2  = 0;

uint8_t  calib_tx_i_dc_offset;   //@XER_0x32<8:15>
uint8_t  calib_tx_q_dc_offset;   //@XER_0x32<0:7>
uint8_t  calib_tx_i_gain_comp;   //@XER_0x32<24:31>
uint8_t  calib_tx_q_gain_comp;   //@XER_0x32<16:23>
uint8_t  calib_tx_phase_comp;    //@XER_0x33<8:15>,  <0:7> tx_ty2 (based on phase_comp)
uint8_t  calib_tx_output_power = 0xf;  //@XER_0xC<27:30> 
uint8_t  calib_tx_output_gain = 0xff;   //@XER_0x32<16:23> <24:31>
uint8_t  calib_tx_if_filter = 0;              // @XVR_5<14:19>


uint32_t xvr_reg_0x0c_bak;
uint32_t xvr_reg_0x25_bak;
uint32_t xvr_reg_0x31_bak;
uint32_t xvr_reg_0x33_bak;

__inline static int32_t abs(int32_t x)
{
    int32_t y = x - (x < 0);

    y ^= (y >> 31);

    return y;
}

static void reg_bit_set(uint32_t reg, uint32_t pos, uint32_t value)
{
    volatile uint32_t* r = (volatile uint32_t*)reg;

    if(value)
    {
        *r |= (1 << pos);
    }
    else
    {
        *r &= ~(1 << pos);
    }
}

__inline static uint32_t reg_bit_get(uint32_t reg, uint32_t pos)
{
    return ((*(volatile uint32_t*)reg) >> pos) & 1;
}

__inline static void reg_bits_set(uint32_t reg, uint32_t pos1, uint32_t pos2, uint32_t value)
{
    volatile uint32_t* r = (volatile uint32_t*)reg;
    uint32_t temp = *r;

    uint32_t mask = (1 << (pos2 - pos1 + 1)) - 1;

    temp &= ~(mask << pos1);
    temp |=  (value & mask) << pos1;
    *r = temp;
}

__inline static uint32_t reg_bits_get(uint32_t reg, uint32_t pos1, uint32_t pos2)
{
    uint32_t mask = (1 << (pos2 - pos1 + 1)) - 1;

    return ((*(volatile uint32_t*)reg) >> pos1) & mask;
}

__inline static void xvr_reg_set(uint32_t reg, uint32_t value)
{
    int idx = (reg - ADDR_XVR_BASE) / 4;

    if(idx < 16)
    {
        *(volatile uint32_t*)reg = XVR_ANALOG_REG_BAK[idx] = value;
    }
    else
    {
        *(volatile uint32_t*)reg = value;
    }
}

__inline static uint32_t xvr_reg_get(uint32_t reg)
{
    int idx = (reg - ADDR_XVR_BASE) / 4;

    return idx < 16 ? XVR_ANALOG_REG_BAK[idx] : *(volatile uint32_t*)reg;
}

__inline static void xvr_reg_bit_set(uint32_t reg, uint32_t pos, uint32_t value)
{
    uint32_t* r = (uint32_t*)&XVR_ANALOG_REG_BAK[(reg - ADDR_XVR_BASE) / 4];

    if(value)
    {
        *r |= (1 << pos);
    }
    else
    {
        *r &= ~(1 << pos);
    }

     *(volatile uint32_t*)reg = *r;
}

__inline static void xvr_reg_bits_set(uint32_t reg, uint32_t pos1, uint32_t pos2, uint32_t value)
{
    uint32_t* r = (uint32_t*)&XVR_ANALOG_REG_BAK[(reg - ADDR_XVR_BASE) / 4];

    uint32_t mask = (1 << (pos2 - pos1 + 1)) - 1;

    *r &= ~(mask << pos1);
    *r |=  (value & mask) << pos1;

    *(volatile uint32_t*)reg = *r;
}

__inline static uint32_t xvr_reg_bit_get(uint32_t reg, uint32_t pos)
{
    return (XVR_ANALOG_REG_BAK[(reg - ADDR_XVR_BASE) / 4] >> pos) & 1;
}

__inline static uint32_t xvr_reg_bits_get(uint32_t reg, uint32_t pos1, uint32_t pos2)
{
    uint32_t mask = (1 << (pos2 - pos1 + 1)) - 1;

    return (XVR_ANALOG_REG_BAK[(reg - ADDR_XVR_BASE) / 4] >> pos1) & mask;
}

void cal_delay_us(uint32_t us)
{
    sys_delay_cycle(6 * us);  // actually, sys_delay_cycle(17 * us) is really delay 1 us
}

void cal_delay_ms(uint32_t ms)
{
    while(ms--)
        cal_delay_us(1000);
}

void sar_adc_enable(uint32_t channel, uint32_t enable)
{
    CALIB_DBG("%s(%d,%d) \r\n", __func__, channel, enable);
    if(enable)
    {
        uint32_t  t;
        uint32_t saradc_config=0;
     
        REG_SYSCTRL_0X3_BAK = REG_SYSCTRL_0X3;
        REG_SYSCTRL_0X5_BAK = REG_SYSCTRL_0X5;
        REG_SYSCTRL_0X6_BAK = REG_SYSCTRL_0X6;
        REG_SYSCTRL_0X40_BAK = REG_SYSCTRL_0X40;
        REG_SYSCTRL_0X46_BAK = REG_SYSCTRL_0X46;
        REG_SYSCTRL_0X59_BAK = REG_SYSCTRL_0X59;
        //SAR-ADC configurations
        reg_bits_set(ADDR_SYSCTRL_0X46,14,16,6);  // LDO output voltage select:1.8V
        reg_bit_set(ADDR_SYSCTRL_0X5,13,0);  // Power ON SAR-ADC
        reg_bit_set(ADDR_SYSCTRL_0X6,14,1);  // saradc gate
        reg_bit_set(ADDR_SYSCTRL_0X3,0,1);  // sysclk gate disable   
        reg_bit_set(ADDR_SYSCTRL_0X40,20,1); // ABB CB
        sys_delay_cycle(6);
        reg_bits_set(ADDR_SYSCTRL_0X59,21,29,0x4B); // adc ctrl, adc vref:bandgap
        reg_bit_set(ADDR_SADC_CONFIG_1, 16, !!(channel & 0x10)); //SAR-ADC channel set bit[4]
        saradc_config = ((0x04&0x3f)<<9) | ((channel&0xf)<<3) | (1<<2);  // div=0x04,enable
        SADC_CONFIG_0 = saradc_config;
        #if 0
        uint32_t  j;
        while(!(SADC_CONFIG_0 & (1<<30)))
            t = SADC_DATA_16;
        cal_delay_us(2);
        for(j=0;j<10;j++)
        {
            reg_bits_set(ADDR_SADC_CONFIG_0, 0, 1,1);  // single mode
            t = SAR_ADC_WAIT_BUSY_COUNT;
            while((--t) && ((SADC_CONFIG_0 & (1<<30)))); // empty
            if(t<5) os_printf("saradc timeout error\r\n");
            t = SADC_DATA_16;
            cal_delay_us(2);
        }
        #endif
        (void)t;
    }
    else
    {

        SADC_CONFIG_0 |= MSK_SADC_0x00_ADC_INT_CLEAR; /*  clear saradc interrupt; */
        SADC_CONFIG_0 = 0;
        
        REG_SYSCTRL_0X3 = REG_SYSCTRL_0X3_BAK;
        REG_SYSCTRL_0X5 = REG_SYSCTRL_0X5_BAK;
        REG_SYSCTRL_0X6 = REG_SYSCTRL_0X6_BAK;
        REG_SYSCTRL_0X40 = REG_SYSCTRL_0X40_BAK;
        REG_SYSCTRL_0X46 = REG_SYSCTRL_0X46_BAK;
        REG_SYSCTRL_0X59 = REG_SYSCTRL_0X59_BAK;       
    }
}

uint32_t sar_adc_read(void)
{
    uint32_t adc_data=0,t = SAR_ADC_WAIT_BUSY_COUNT;
    uint32_t j;
    
#if 1   // software mode    
    #define AVER_TIMES   3
    uint32_t temp;
    SADC_CONFIG_0 |= (1<<SFT_SADC_0x00_ADC_EN) | (2<<SFT_SADC_0x00_ADC_MODE); // mode  & enable
 
    adc_data = 0;
    while((--t)&& ((SADC_CONFIG_0 & (1<<SFT_SADC_0x00_FIFO_EMPTY))));   
    if(t<5) CALIB_DBG("saradc timeout\r\n");
    temp = SADC_DATA_16; 

    for(j=0;j<AVER_TIMES;j++)
    {  
        t = SAR_ADC_WAIT_BUSY_COUNT;
        while((--t)&& ((SADC_CONFIG_0 & (1<<SFT_SADC_0x00_FIFO_EMPTY))));   
        if(t<5) CALIB_DBG("saradc timeout\r\n");
        adc_data += SADC_DATA_16; 
    }
    adc_data /= AVER_TIMES;
    SADC_CONFIG_0 &= ~(MSK_SADC_0x00_ADC_MODE |MSK_SADC_0x00_ADC_EN);  //  diable & mode = 0
    (void)temp;
#else  // singlestep mode
    adc_data = 0;
    for(j=0;j<2;j++)
    {
        reg_bits_set(ADDR_SADC_CONFIG_0, 0, 1,1);
        t = SAR_ADC_WAIT_BUSY_COUNT;
        while((--t) && ((SADC_CONFIG_0 & (1<<30)))); // empty
        if(t<5) os_printf("saradc timeout error\r\n");
        adc_data += SADC_DATA_16;
        cal_delay_us(2);
    }
    adc_data /= j;
#endif
    
    //CALIB_DBG("%s() = 0x%X (%d)\r\n", __func__, adc_data, adc_data);
    
    return adc_data;
}

void tx_calibration_init(void)
{
    uint32_t saradc_value;

    CALIB_DBG("%s()\r\n", __func__);

    set_spr(SPR_VICMR(0), get_spr(SPR_VICMR(0)) & ~(1 << VIC_IDX_SADC));
    sar_adc_enable(23, 1);

    xvr_reg_0x0c_bak = xvr_reg_get(ADDR_XVR_0x0C);
    xvr_reg_0x25_bak = *(volatile uint32_t*)ADDR_XVR_0x25;
    xvr_reg_0x31_bak = *(volatile uint32_t*)ADDR_XVR_0x31;
    xvr_reg_0x33_bak = *(volatile uint32_t*)ADDR_XVR_0x33;
    
    *(volatile uint32_t*)ADDR_XVR_0x25 = 0x7800;
    saradc_value = sar_adc_read();
    os_printf("tx_init sadc data = %d\r\n", saradc_value);
    (void)saradc_value;	
    
    CALIB_DBG("%s(),0xc:0x%x,0x25:0x%x\r\n", __func__,xvr_reg_0x0c_bak,xvr_reg_0x25_bak);
}

void tx_calibration_deinit(void)
{
    CALIB_DBG("%s()\r\n", __func__);

    xvr_reg_set(ADDR_XVR_0x0C, xvr_reg_0x0c_bak);
    xvr_reg_bits_set(ADDR_XVR_0x0C,27,30,calib_tx_output_power);

    *(volatile uint32_t*)ADDR_XVR_0x25 = xvr_reg_0x25_bak;    
    *(volatile uint32_t*)ADDR_XVR_0x31 = xvr_reg_0x31_bak;    
    *(volatile uint32_t*)ADDR_XVR_0x33 = ((*(volatile uint32_t*)ADDR_XVR_0x33) &0xffff)|(xvr_reg_0x33_bak&0xffff0000);

    sar_adc_enable(0, 0);
    set_spr(SPR_VICMR(0), get_spr(SPR_VICMR(0)) | (1 << VIC_IDX_SADC));

    CALIB_DBG("%s(),0xc:0x%x,0x25:0x%x\r\n", __func__,xvr_reg_0x0c_bak,xvr_reg_0x25_bak);

}

void tx_dc_calibration(void)
{
    
    CALIB_DBG("%s()\r\n", __func__);

    xvr_reg_bit_set(ADDR_XVR_0x0C, 26, 1);

    xvr_reg_set(ADDR_XVR_0x25,0x7800);

    xvr_reg_bit_set(ADDR_XVR_0x0C, 2, 0);            // enIQcal=0, disable TSSI for IQ calibration path
    xvr_reg_bit_set(ADDR_XVR_0x0C, 1, 1);           // enDCcal=1, enable TSSI for DC calibration
    xvr_reg_bit_set(ADDR_XVR_0x0C, 0, 0);           // enPcal=0, disable TSSI for output power calibration

    xvr_reg_set(ADDR_XVR_0x31,0x4000000);            //enable TX LO leakage power output
    xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, 3);   //  calib_tx_output_power, need reconfig

#if 0

    int8_t start;
    int8_t stop;
    //
    start = 0xF0;
    stop  = 0x10;
    reg_bits_set(ADDR_XVR_0x32, 0, 7, 0xff);

    do
    {
        uint32_t tssi_start, tssi_stop;

        reg_bits_set(ADDR_XVR_0x32, 8, 15, start);
        cal_delay_us(100);
        tssi_start = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x32, 8, 15, stop);
        cal_delay_us(100);
        tssi_stop = sar_adc_read();

        CALIB_DBG("start = %d, stop = %d, tssi_start = %d, high = %d\n", start, stop, tssi_start, tssi_stop);

        if(start + 1 == stop) break;

        if(tssi_start < tssi_stop)
        {
            stop = (start + stop) / 2;
            calib_tx_i_dc_offset = stop;
        }
        else
        {
            start = (start + stop) / 2;
            calib_tx_i_dc_offset = start;
        }

    }while(start < stop);

    reg_bits_set(ADDR_XVR_0x32, 8, 15, calib_tx_i_dc_offset);

    CALIB_DBG("calib_tx_i_dc_offset = 0x%02X\r\n", calib_tx_i_dc_offset);

    //
    start = 0xF0;
    stop  = 0x10;

    do
    {
        uint32_t tssi_start, tssi_stop;

        reg_bits_set(ADDR_XVR_0x32, 0, 7, start);
        cal_delay_us(100);
        tssi_start = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x32, 0, 7, stop);
        cal_delay_us(100);
        tssi_stop = sar_adc_read();

        CALIB_DBG("start = %d, stop = %d, tssi_start = %d, tssi_stop = %d\n", start, stop, tssi_start, tssi_stop);

        if(start + 1 == stop) break;

        if(tssi_start < tssi_stop)
        {
            stop = (start + stop) / 2;
            calib_tx_q_dc_offset = stop;
        }
        else
        {
            start = (start + stop) / 2;
            calib_tx_q_dc_offset = start;
        }

    }while(start < stop);

    reg_bits_set(ADDR_XVR_0x32, 0, 7, calib_tx_q_dc_offset);

    CALIB_DBG("calib_tx_q_dc_offset = 0x%02X\r\n", calib_tx_q_dc_offset);

#else

    int8_t i_dc_offset, q_dc_offset;
    uint32_t tssi_read = 0, tssi_min = 0xFFFF;

    reg_bits_set(ADDR_XVR_0x32, 0, 7, 0xff);
	
    for(i_dc_offset = 0xF0; i_dc_offset <= 0x10; i_dc_offset++)
    {
        reg_bits_set(ADDR_XVR_0x32, 8, 15, i_dc_offset);
        cal_delay_us(100);
        tssi_read = sar_adc_read();

        CALIB_DBG("i_dc_offset = %d, tssi_read = %d\n", i_dc_offset, tssi_read);

	 if(tssi_read < tssi_min)
	 {
	     tssi_min = tssi_read;
	     calib_tx_i_dc_offset = i_dc_offset;
	 }
	 
    }

    /*In case that Saradc didnt work*/
    if(calib_tx_i_dc_offset == 0xF0)
    {
        tssi_min = 0xFFFF;

        for(i_dc_offset = 0xF0; i_dc_offset <= 0x10; i_dc_offset++)
        {
            reg_bits_set(ADDR_XVR_0x32, 8, 15, i_dc_offset);
            cal_delay_us(100);
            tssi_read = sar_adc_read();

            CALIB_DBG("i_dc_offset = %d, tssi_read = %d\n", i_dc_offset, tssi_read);

        if(tssi_read < tssi_min)
        {
            tssi_min = tssi_read;
            calib_tx_i_dc_offset = i_dc_offset;
        }

        }
    }

    reg_bits_set(ADDR_XVR_0x32, 8, 15, calib_tx_i_dc_offset);

    CALIB_DBG("calib_tx_i_dc_offset = 0x%02X, tssi_min = %d\r\n", calib_tx_i_dc_offset, tssi_min);

    tssi_min = 0xFFFF;
	
    for(q_dc_offset = 0xF0; q_dc_offset <= 0x10; q_dc_offset++)
    {
        reg_bits_set(ADDR_XVR_0x32, 0, 7, q_dc_offset);
        cal_delay_us(100);
        tssi_read = sar_adc_read();

        CALIB_DBG("q_dc_offset = %d, tssi_read = %d\n", q_dc_offset, tssi_read);

	 if(tssi_read < tssi_min)
	 {
	     tssi_min = tssi_read;
	     calib_tx_q_dc_offset = q_dc_offset;
	 }
	 
    }

    reg_bits_set(ADDR_XVR_0x32, 0, 7, calib_tx_q_dc_offset);

    CALIB_DBG("calib_tx_q_dc_offset = 0x%02X, tssi_min = %d\r\n", calib_tx_q_dc_offset, tssi_min);

#endif

}

void tx_q_const_iqcal_p_calibration(void)
{
#if 1
    int32_t /* gain0,*/ gain;

    CALIB_DBG("%s()\r\n", __func__);

    //#define Q_CONST_GAIN_THRESHOLD    (390)  
    int32_t Q_CONST_GAIN_THRESHOLD = 0,g_min,g_max;;  //min: power=0x0,gain=0x0 ;max:power=0xf,gain=0x7f , ( (min) + (max))/2  
   xvr_reg_set(ADDR_XVR_0x31,0x4000000);
    xvr_reg_bit_set(ADDR_XVR_0x0C, 2, 1);            // enIQcal=0, disable TSSI for IQ calibration path
    xvr_reg_bit_set(ADDR_XVR_0x0C, 1, 0);           // enDCcal=1, enable TSSI for DC calibration
    xvr_reg_bit_set(ADDR_XVR_0x0C, 0, 0);           // enPcal=0, disable TSSI for output power calibration

    xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, 0);   // min: power=0x0,gain=0x0 
    reg_bits_set(ADDR_XVR_0x31,  8, 15, 0);
    reg_bits_set(ADDR_XVR_0x31, 16, 23, 0);
    cal_delay_us(100);
    g_min = sar_adc_read();
//    gain0 = g_min * 2;

    xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, 0xf);   // min: power=0x0,gain=0x0 
    reg_bits_set(ADDR_XVR_0x31,  8, 15, 0x7f);
    reg_bits_set(ADDR_XVR_0x31, 16, 23, 0x7f);
    cal_delay_us(100);
    g_max = sar_adc_read();

    Q_CONST_GAIN_THRESHOLD = g_min+g_max;

    CALIB_DBG("Q_CONST_GAIN_THRESHOLD = %d = (%d + %d)\r\n", Q_CONST_GAIN_THRESHOLD,g_min,g_max);

    xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, 1);   //  calib_tx_output_power, need reconfig

    #if (CFG_TX_CALIB_ALGORITHM ==  TX_CALIB_ALGORITHM_ERGODIC) //

    uint32_t q_const;
    int32_t  diff, diff_min = 0x7FFFFFFF;

    for(q_const = 0; q_const <= 0x64; q_const += 4)
    {
        reg_bits_set(ADDR_XVR_0x31, 8, 15, 0);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, q_const);
        cal_delay_us(100);
        gain = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256-q_const);
        cal_delay_us(100);
        gain += sar_adc_read();

        diff = abs(gain/* - gain0*/ - Q_CONST_GAIN_THRESHOLD);

        if(diff < diff_min)
        {
            diff_min = diff;
            TX_Q_CONST_IQCAL_P = q_const;
        }

        CALIB_DBG("q_const = %d, diff = %d, diff_min = %d\r\n", q_const, diff,diff_min);
    }

    #elif  (CFG_TX_CALIB_ALGORITHM == TX_CALIB_ALGORITHM_DICHOTOMY) //

    uint32_t start = 36;
    uint32_t stop  = 144;

    do
    {
        int32_t diff_start;
        int32_t diff_stop;

        reg_bits_set(ADDR_XVR_0x31, 16, 23, start);
        cal_delay_us(100);
        gain = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - start);
        cal_delay_us(100);
        gain += sar_adc_read();

        diff_start = abs(gain - gain0 - Q_CONST_GAIN_THRESHOLD);

        reg_bits_set(ADDR_XVR_0x31, 16, 23, stop);
        cal_delay_us(100);
        gain = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - stop);
        cal_delay_us(100);
        gain += sar_adc_read();

        diff_stop = abs(gain - gain0 - Q_CONST_GAIN_THRESHOLD);

        CALIB_DBG("start = %d, stop = %d, diff_start = %d, diff_stop = %d\r\n", start, stop, diff_start, diff_stop);

        if(start + 1 == stop) break;

        if(diff_start < diff_stop)
        {
            stop = (start + stop) / 2;
            TX_Q_CONST_IQCAL_P = start;
        }
        else
        {
            start = (start + stop) / 2;
            TX_Q_CONST_IQCAL_P = stop;
        }

    }while(start < stop);

    #endif

    TX_Q_CONST_IQCAL_P_4_PHASE = (uint32_t)(TX_Q_CONST_IQCAL_P / 1.4142 + 0.5);

    CALIB_DBG("TX_Q_CONST_IQCAL_P         = %d\r\n", TX_Q_CONST_IQCAL_P);
    CALIB_DBG("TX_Q_CONST_IQCAL_P_4_PHASE = %d\r\n", TX_Q_CONST_IQCAL_P_4_PHASE);
#endif
}

void tx_iq_gain_imbalance_calibration(void)
{
#if 1
    int32_t i;
    int32_t gain_i;
    int32_t gain_q;

    CALIB_DBG("%s()\r\n", __func__);

    calib_tx_i_gain_comp = calib_tx_output_gain;
    calib_tx_q_gain_comp = calib_tx_output_gain;

    xvr_reg_set(ADDR_XVR_0x25,0x7800);

    xvr_reg_bit_set(ADDR_XVR_0x0C, 2, 1);            // enIQcal=0, disable TSSI for IQ calibration path
    xvr_reg_bit_set(ADDR_XVR_0x0C, 1, 0);           // enDCcal=1, enable TSSI for DC calibration
    xvr_reg_bit_set(ADDR_XVR_0x0C, 0, 0);           // enPcal=0, disable TSSI for output power calibration

    xvr_reg_set(ADDR_XVR_0x31,0x4000000);            //enable TX LO leakage power output
    
    xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, 1); 
    reg_bits_set(ADDR_XVR_0x32,  24, 31, calib_tx_output_gain); 
    reg_bits_set(ADDR_XVR_0x32, 16, 23, calib_tx_output_gain); 

    reg_bits_set(ADDR_XVR_0x31,  8, 15, 0);
    reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P);
    cal_delay_us(100);
    gain_i = sar_adc_read();

    reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P);
    cal_delay_us(100);
    gain_i += sar_adc_read();

    reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P);
    reg_bits_set(ADDR_XVR_0x31, 16, 23, 0);
    cal_delay_us(100);
    gain_q = sar_adc_read();

    reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P);
    cal_delay_us(100);
    gain_q += sar_adc_read();

    if(abs(gain_i - gain_q) > 1)
    {
        if(gain_i > gain_q)
        {
            #if (CFG_TX_CALIB_ALGORITHM ==  TX_CALIB_ALGORITHM_ERGODIC) //

            int32_t diff;
            int32_t diff_min = abs(gain_i - gain_q);

            for(i = calib_tx_output_gain-40; i < calib_tx_output_gain; i++)
            {
                reg_bits_set(ADDR_XVR_0x32, 24, 31, i);
                reg_bits_set(ADDR_XVR_0x31,  8, 15, 0);
                reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_i = sar_adc_read();

                reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_i += sar_adc_read();

                diff = abs(gain_i - gain_q);
                if(diff < diff_min)
                {
                    diff_min = diff;
                    calib_tx_i_gain_comp = i;
                }

                CALIB_DBG("i_gain_comp = %d, q_gain_comp = %d, gain_i = %d, gain_q = %d, diff = %d\r\n", i, calib_tx_output_gain,gain_i, gain_q, diff);
            }

            #elif  (CFG_TX_CALIB_ALGORITHM == TX_CALIB_ALGORITHM_DICHOTOMY) //

            calib_tx_i_gain_comp = 0;

            for(i = 7; i >= 0; i--)
            {
                calib_tx_i_gain_comp |= 1 << i;
                reg_bits_set(ADDR_XVR_0x32, 24, 31, calib_tx_i_gain_comp);

                reg_bits_set(ADDR_XVR_0x31,  8, 15, 0);
                reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_i = sar_adc_read();

                reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_i += sar_adc_read();

                if(abs(gain_i - gain_q) < 1) break;
                if(gain_i > gain_q) calib_tx_i_gain_comp &= ~(1 << i);
            }

            #endif
        }
        else
        {
            #if (CFG_TX_CALIB_ALGORITHM ==  TX_CALIB_ALGORITHM_ERGODIC) //

            int32_t diff;
            int32_t diff_min = abs(gain_i - gain_q);

            for(i = calib_tx_output_gain-40; i < calib_tx_output_gain; i++)
            {
                reg_bits_set(ADDR_XVR_0x32, 16, 23, i);

                reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P);
                reg_bits_set(ADDR_XVR_0x31, 16, 23, 0);
                cal_delay_us(100);
                gain_q = sar_adc_read();

                reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_q += sar_adc_read();

                diff = abs(gain_i - gain_q);
                if(diff < diff_min)
                {
                    diff_min = diff;
                    calib_tx_q_gain_comp = i;
                }

                CALIB_DBG("i_gain_comp = %d, q_gain_comp = %d, gain_i = %d, gain_q = %d, diff = %d\r\n",calib_tx_output_gain, i, gain_i, gain_q, diff);
            }

            #elif  (CFG_TX_CALIB_ALGORITHM == TX_CALIB_ALGORITHM_DICHOTOMY) //

            calib_tx_q_gain_comp = 0;

            for(i = 7; i >= 0; i--)
            {
                calib_tx_q_gain_comp |= 1 << i;
                reg_bits_set(ADDR_XVR_0x32, 16, 23, calib_tx_q_gain_comp);

                reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P);
                reg_bits_set(ADDR_XVR_0x31, 16, 23, 0);
                cal_delay_us(100);
                gain_q = sar_adc_read();

                reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P);
                cal_delay_us(100);
                gain_q += sar_adc_read();

                if(abs(gain_i - gain_q) < 1) break;
                if(gain_q > gain_i) calib_tx_q_gain_comp &= ~(1 << i);
            }

            #endif
        }
    }

    reg_bits_set(ADDR_XVR_0x32, 24, 31, calib_tx_i_gain_comp);
    reg_bits_set(ADDR_XVR_0x32, 16, 23, calib_tx_q_gain_comp);

    CALIB_DBG("calib_tx_i_gain_comp = 0x%02X\r\n", calib_tx_i_gain_comp);
    CALIB_DBG("calib_tx_q_gain_comp = 0x%02X\r\n", calib_tx_q_gain_comp);
#endif
}

uint32_t tx_iq_phase_imbalance_calibration(void)
{

    uint32_t tx_ty2 = 0;
    int32_t  phase_i;
    int32_t  phase_q;

    CALIB_DBG("%s()\r\n", __func__);


    #if (CFG_TX_CALIB_ALGORITHM ==  TX_CALIB_ALGORITHM_ERGODIC) //

    int8_t  tx_phase_comp;
    int32_t diff, diff_min = 0x7FFFFFFF;

    for(tx_phase_comp = 0xE0; tx_phase_comp <= 0x20; tx_phase_comp++)
    {
        int32_t t = tx_phase_comp;

        t = ((t * t << 18) + (t * t * t * t) * 3 + (1 << 26)) >> 27;

        reg_bits_set(ADDR_XVR_0x33, 8, 15, tx_phase_comp);
        reg_bits_set(ADDR_XVR_0x33, 0,  7, t);

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i += sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q += sar_adc_read();

        diff = abs(phase_i - phase_q);
        if(diff_min > diff)
        {
            diff_min = diff;
            tx_ty2   = t;
            calib_tx_phase_comp = tx_phase_comp;
        }

        CALIB_DBG("@tx_phase_comp = %d (0x%0X), tx_ty2 = %d, diff = %d\r\n", tx_phase_comp, tx_phase_comp & 0xFF, t, diff);
    }

    #elif  (CFG_TX_CALIB_ALGORITHM == TX_CALIB_ALGORITHM_DICHOTOMY) //

    int8_t   start = 0xE0;
    int8_t   stop  = 0x20;
    uint32_t tx_ty2_start;
    uint32_t tx_ty2_stop;

    do
    {
        uint32_t diff_start;
        uint32_t diff_stop;

        int32_t t = start;

        tx_ty2_start = ((t * t << 18) + (t * t * t * t) * 3 + (1 << 26)) >> 27;

        reg_bits_set(ADDR_XVR_0x33, 8, 15, start);
        reg_bits_set(ADDR_XVR_0x33, 0,  7, tx_ty2_start);

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i += sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q += sar_adc_read();

        diff_start = abs(phase_i - phase_q);

        t = stop;

        tx_ty2_stop = ((t * t << 18) + (t * t * t * t) * 3 + (1 << 26)) >> 27;

        reg_bits_set(ADDR_XVR_0x33, 8, 15, stop);
        reg_bits_set(ADDR_XVR_0x33, 0,  7, tx_ty2_stop);

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_i += sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q = sar_adc_read();

        reg_bits_set(ADDR_XVR_0x31,  8, 15, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        reg_bits_set(ADDR_XVR_0x31, 16, 23, 256 - TX_Q_CONST_IQCAL_P_4_PHASE);
        cal_delay_us(100);
        phase_q += sar_adc_read();

        diff_stop = abs(phase_i - phase_q);

        CALIB_DBG("start = %d, stop = %d, diff_start = %d, diff_stop = %d\r\n", start, stop, diff_start, diff_stop);

        if(start + 1 == stop) break;

        if(diff_start < diff_stop)
        {
            stop   = (start + stop) / 2;
            tx_ty2 = tx_ty2_start;
            calib_tx_phase_comp = start;
        }
        else
        {
            start  = (start + stop) / 2;
            tx_ty2 = tx_ty2_stop;
            calib_tx_phase_comp = stop;
        }

    }while(start < stop);

    #endif

    reg_bits_set(ADDR_XVR_0x33, 8, 15, calib_tx_phase_comp);
    reg_bits_set(ADDR_XVR_0x33, 0,  7, tx_ty2);


    CALIB_DBG("calib_tx_phase_comp = 0x%02X,tx_ty2:0x%02X\r\n", calib_tx_phase_comp,tx_ty2);

    return tx_ty2;
}

void tx_output_power_calibration_simple(void)
{
    uint32_t tx_output_power;
    int32_t  txd_tssi = 0;
   // uint32_t cnt = 0,retest_1=0,retest_2=0 ;
    CALIB_DBG("%s()\r\n", __func__);

    xvr_reg_set(ADDR_XVR_0x25,0x3100);

    xvr_reg_bit_set(ADDR_XVR_0x0C, 2, 0);            // enIQcal=0, disable TSSI for IQ calibration path
    xvr_reg_bit_set(ADDR_XVR_0x0C, 1, 0);           // enDCcal=1, enable TSSI for DC calibration
    xvr_reg_bit_set(ADDR_XVR_0x0C, 0, 1);           // enPcal=0, disable TSSI for output power calibration
    //xvr_reg_bits_set(ADDR_XVR_0x0C, 5,8, 0x8);   // tssi_att=F, set power calibration threshold
    reg_bits_set(ADDR_XVR_0x32,  24, 31, 0xff); 
    reg_bits_set(ADDR_XVR_0x32, 16, 23, 0xff); 


    //for(retest_2 = 0;retest_2<5;retest_2++)
    {
        for(tx_output_power = 0; tx_output_power <= 0xf; tx_output_power++)
        {
            xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, tx_output_power); 
            cal_delay_us(1);
            txd_tssi =  (REG_SYSCTRL_0X31>>10)&3 ;

            CALIB_DBG("calib_tx_pa_power = 0x%X, txd_tssi = 0x%X\n", tx_output_power, txd_tssi);

            if(txd_tssi == 3)
            {
                break;
            }
        }

        if(tx_output_power > 0xf)
            calib_tx_output_power = 0xf;
        else
            calib_tx_output_power = tx_output_power;    
        
       // for(retest_1=0;retest_1<5;retest_1++)
        {
            //cnt = 0;
            for(tx_output_power = 0; tx_output_power <= 0xff; tx_output_power++)
            {
                reg_bits_set(ADDR_XVR_0x32,  24, 31, tx_output_power); 
                reg_bits_set(ADDR_XVR_0x32, 16, 23, tx_output_power); 
                cal_delay_us(1);
                txd_tssi =  (REG_SYSCTRL_0X31>>10)&3 ;

                //if(txd_tssi == 1)
                //    cnt++;

                CALIB_DBG("calib_tx_iq_gain = 0x%X, txd_tssi = 0x%X\n", tx_output_power, txd_tssi);
                
                if(txd_tssi == 3)
                {
                    break;
                    #if 0
                    if(cnt>5)
                    {
                        retest_1 = 5;  // end gain recal
                        retest_2 = 5;  // end power recal
                        break;
                    }
                    else
                    {
                        cnt = 0;
                        tx_output_power = 0xff;
                    }
                    #endif
                }
            }            
        }
        if(tx_output_power > 0xff)
        {
            calib_tx_output_gain = 0xff;
        }
        else
            calib_tx_output_gain = tx_output_power;
    }
    

    CALIB_DBG("calib_tx_output_power = 0x%x,tx_output_gain= 0x%x\r\n", calib_tx_output_power,calib_tx_output_gain);

}

void tx_output_power_calibration(void)
{
    uint32_t tx_output_power=0;
    int32_t  txd_tssi = 0;
    uint32_t cnt = 0,retest_1=0,retest_2=0 ;
    uint32_t xvr_c = 0;
    CALIB_DBG("%s()\r\n", __func__);

    xvr_reg_bit_set(ADDR_XVR_0x0C, 26, 0);
    xvr_reg_set(ADDR_XVR_0x25,0x3100);
    xvr_reg_set(ADDR_XVR_0x31,0x0000000);
    xvr_reg_bit_set(ADDR_XVR_0x0C, 2, 0);            // enIQcal=0, disable TSSI for IQ calibration path
    xvr_reg_bit_set(ADDR_XVR_0x0C, 1, 0);           // enDCcal=1, enable TSSI for DC calibration
    xvr_reg_bit_set(ADDR_XVR_0x0C, 0, 1);           // enPcal=0, disable TSSI for output power calibration
    //xvr_reg_bits_set(ADDR_XVR_0x0C, 5,8, 0x8);   // tssi_att=F, set power calibration threshold
    reg_bits_set(ADDR_XVR_0x32,  24, 31, 0xff); 
    reg_bits_set(ADDR_XVR_0x32, 16, 23, 0xff); 

    xvr_c = xvr_reg_get(ADDR_XVR_0x0C);
    xvr_c = (xvr_c>>5)&0xf;
    /*
    if(xvr_c>0xc)
        xvr_reg_bits_set(ADDR_XVR_0x0B, 12, 15, 5);
    else
        xvr_reg_bits_set(ADDR_XVR_0x0B, 12, 15, 1);*/

    for(retest_2 = 0;retest_2<2;retest_2++)
    {
        CLEAR_WDT;
        CALIB_DBG("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~retest_2:%d,tx_power:%x,cnt:%d\r\n", retest_2,tx_output_power,cnt);
#if 0
        for(tx_output_power = 0; tx_output_power <= 0xf; tx_output_power++)
        {
            xvr_reg_bits_set(ADDR_XVR_0x0C,  27, 30, tx_output_power); 
            cal_delay_us(1);
            txd_tssi =  (REG_SYSCTRL_0X31>>10)&3 ;

            CALIB_DBG("calib_tx_pa_power = 0x%X, txd_tssi = 0x%X\n", tx_output_power, txd_tssi);

            if(txd_tssi == 3)
            {
                break;
            }
        }

        if(tx_output_power > 0xf)
            calib_tx_output_power = 0xf;
        else
            calib_tx_output_power = tx_output_power;    
#endif
        for(retest_1=0;retest_1<2;retest_1++)
        {
            CALIB_DBG("~~~~~~~~~~~~~~~~~~~retest_1:%d,tx_power:%x,cnt:%d~~~~~~~~~~~~~~~~~~~~\r\n", retest_1,tx_output_power,cnt);
            cnt = 0;
            for(tx_output_power = 0; tx_output_power <= 0xff; tx_output_power++)
            {
                reg_bits_set(ADDR_XVR_0x32,  24, 31, tx_output_power); 
                reg_bits_set(ADDR_XVR_0x32, 16, 23, tx_output_power); 
                cal_delay_us(1);
                txd_tssi =  (REG_SYSCTRL_0X31>>10)&3 ;

                if(txd_tssi == 1)
                    cnt++;

                CALIB_DBG("calib_tx_iq_gain = 0x%X, txd_tssi = 0x%X\n", tx_output_power, txd_tssi);
                
                if(txd_tssi == 3)
                {
                    if(cnt>5)
                    {
                        retest_1 = 5;  // end gain recal
                        retest_2 = 5;  // end power recal
                        break;
                    }
                    else
                    {
                        cnt = 0;
                        tx_output_power = 0xff;
                    }
                }
            }            
        }
        if(tx_output_power > 0xff)
        {
            calib_tx_output_gain = 0xff;
        }
        else
            calib_tx_output_gain = tx_output_power;
    }
    
    xvr_reg_bit_set(ADDR_XVR_0x0C, 26, 1);
    CALIB_DBG("calib_tx_output_power = 0x%x,tx_output_gain= 0x%x\r\n", calib_tx_output_power,calib_tx_output_gain);

}


void rf_IF_filter_cap_calibration(void)
{
    uint32_t xvr_6 = 0;
    xvr_reg_0x25_bak = *(volatile uint32_t*)ADDR_XVR_0x25;
    *(volatile uint32_t*)ADDR_XVR_0x25 = 0;
    cal_delay_us(30);
    *(volatile uint32_t*)ADDR_XVR_0x25 = 0x2000;
    cal_delay_us(60);
    xvr_6 = xvr_reg_get(ADDR_XVR_0x06);
    xvr_reg_bit_set(ADDR_XVR_0x06, 2, 0);
    cal_delay_us(30);
    xvr_reg_bit_set(ADDR_XVR_0x06, 1, 1);
    cal_delay_us(30);
    xvr_reg_bit_set(ADDR_XVR_0x06, 0, 0);
    cal_delay_us(30);
    xvr_reg_bit_set(ADDR_XVR_0x06, 0, 1);
    cal_delay_us(30);
    xvr_reg_bit_set(ADDR_XVR_0x06, 0, 0);
    cal_delay_us(300);
    calib_tx_if_filter  = (REG_SYSCTRL_0X31>>12)&0x3f ;
    if(calib_tx_if_filter>53)
        calib_tx_if_filter = 53;
    //xvr_reg_bits_set(ADDR_XVR_0x05, 14, 19, calib_tx_if_filter);
    xvr_reg_bits_set(ADDR_XVR_0x0D, 0, 5, calib_tx_if_filter+10);
    CALIB_DBG("calib_tx_IF_filter_cab = 0x%x\r\n",calib_tx_if_filter);

    xvr_reg_set(ADDR_XVR_0x06, xvr_6);
    *(volatile uint32_t*)ADDR_XVR_0x25 = xvr_reg_0x25_bak;
}


static uint32_t cali_is_done = 0;
#define LPBG_CALI_DONE          0x0001
#define CHARGE_CALI_DONE        0x0002
#define SARADC_CALI_DONE        0x0004
#define TEMPR_CALI_DONE         0x0008

#define SARADC_CALI_DC_DECR     2016     // 2016+0x3f = 2079
#define SARADC_CALI_4P2_DECR    2544     // 2544 + 0x1ff = 3055
#define TEMPR_CALI_DECR         1490     // 1490 + 0x7f = 1617
#define CALI_EFUSE_ADDR         16

typedef struct _ate_charge_cali_data_s
{
    uint8_t charger_vlcf;
    uint8_t charger_icp;
    uint8_t charger_vcv;
}__PACKED_POST__  ate_charge_cali_data_t;

typedef struct ate_saradc_cali_data_s
{
    uint16_t sar_adc_dc;
    uint16_t sar_adc_4p2;
}__PACKED_POST__  ate_saradc_cali_data_t;


typedef struct ate_lpbg_cali_data_s
{
    uint8_t   cali_lpbg_bandgap;
    uint8_t   cali_lpbg_vddcore;
    uint8_t   cali_lpbg_vddio;
}__PACKED_POST__  ate_lpbg_cali_data_t;

static ate_lpbg_cali_data_t s_ate_lpbg_cali_data;
static ate_saradc_cali_data_t  s_ate_saradc_cali_data;
static ate_charge_cali_data_t  s_ate_charge_cali_data;
static uint16_t s_ate_tempr_cali_data;

// vbat = 4.2V, this calibration need be done at XTAL/LOW DPLL
void LPBG_calibration(void)
{
    uint32_t cali_bandgap=0, cali_vddcore=0,cali_vddio=0;
    ate_lpbg_cali_data_t *lpbg_cali = &s_ate_lpbg_cali_data;
    os_printf("%s\r\n",__func__);
    // vddalo LDO(bandgap) calibration
    reg_bits_set(ADDR_SYSCTRL_0X44,0,7,0xff);
    sys_delay_ms(1);
    reg_bit_set(ADDR_SYSCTRL_0X46,27,0);  //bandgap calibration powerdown off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X46,25,0);  //bandgap calibration mode on
    reg_bit_set(ADDR_SYSCTRL_0X5C,15,1); //vddcore(1.2V) calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,14,1);//vddcore(1.2V) calibration mode off   
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,7,1); //vddio(1.2V) calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,6,1);  //vddio(1.2V) calibration mode off
    reg_bit_set(ADDR_SYSCTRL_0X46,26,0); 
    sys_delay_ms(1);
    reg_bit_set(ADDR_SYSCTRL_0X46,26,1); //calibration start
    sys_delay_ms(5);
    reg_bit_set(ADDR_SYSCTRL_0X46,26,0);
    sys_delay_ms(20);
    cali_bandgap = reg_bits_get(ADDR_SYSCTRL_0X32,13,18);
    //os_printf("1:%x ",cali_bandgap);
    //reg0x46<24��o19>
    reg_bits_set(ADDR_SYSCTRL_0X46,19,24,cali_bandgap);
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X46,27,1);
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X46,25,1);  
    sys_delay_cycle(6);

    // vddcore/vddana 
    reg_bit_set(ADDR_SYSCTRL_0X46,27,1);    //bandgap calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X46,25,1);    //bandgap calibration mode off
    reg_bit_set(ADDR_SYSCTRL_0X5C,15,0);    //vddcore(1.2V) calibration powerdown off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,14,0);    //vddcore(1.2V) calibration mode on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,7,1);    //vddio(1.2V) calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,6,1);    //vddio(1.2V) calibration mode off    
    reg_bit_set(ADDR_SYSCTRL_0X46,26,1); //calibration start
    sys_delay_ms(5);
    reg_bit_set(ADDR_SYSCTRL_0X46,26,0);
    sys_delay_ms(20);
    cali_vddcore = reg_bits_get(ADDR_SYSCTRL_0X32,13,18);
    //os_printf("2:%x,",cali_vddcore);
    //reg0x5c<13��o8>
    reg_bits_set(ADDR_SYSCTRL_0X5C,8,13,cali_vddcore);
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,15,1); //vddcore(1.2V) calibration powerdown off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,14,1);//vddcore(1.2V) calibration mode off disable
    
    // vddio calibration
    reg_bit_set(ADDR_SYSCTRL_0X46,27,1);    //bandgap calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X46,25,1);    //bandgap calibration mode off
    reg_bit_set(ADDR_SYSCTRL_0X5C,15,1);    //vddcore(1.2V) calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,14,1);    //vddcore(1.2V) calibration mode off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,7,0);    //vddio(1.2V) calibration powerdown off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,6,0);    //vddio(1.2V) calibration mode on
    reg_bit_set(ADDR_SYSCTRL_0X46,26,1); //calibration start
    sys_delay_ms(5);
    reg_bit_set(ADDR_SYSCTRL_0X46,26,0);
    sys_delay_ms(20);
    cali_vddio = reg_bits_get(ADDR_SYSCTRL_0X32,13,18);
    //os_printf("3:%x",cali_vddio);
    //reg0x5c<5��o0>
    reg_bits_set(ADDR_SYSCTRL_0X5C,0,5,cali_vddio);
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,7,1);    //vddio(1.2V) calibration powerdown on
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,6,1);    //vddio(1.2V) calibration mode off
    sys_delay_cycle(6);
    reg_bit_set(ADDR_SYSCTRL_0X5C,15,1);
    reg_bit_set(ADDR_SYSCTRL_0X46,27,1);
    lpbg_cali->cali_lpbg_bandgap = cali_bandgap;
    lpbg_cali->cali_lpbg_vddcore = cali_vddcore;
    lpbg_cali->cali_lpbg_vddio = cali_vddio;
    cali_is_done |= LPBG_CALI_DONE;
    os_printf("cali_bandgap:%x,cali_vddcore:%x,cali_vddio:%x",cali_bandgap,cali_vddcore,cali_vddio);
}

extern uint32_t app_env_get_flash_addr(t_TYPE_SECTOR type);

uint8_t write_cali_result()
{
    uint8_t cali_result[200],cali_result_r[200];
    uint8_t i,result=0;
    
    ate_lpbg_cali_data_t *lpbg_cali = &s_ate_lpbg_cali_data;
    ate_charge_cali_data_t *charge_cali = &s_ate_charge_cali_data;
    ate_saradc_cali_data_t *saradc_cali = &s_ate_saradc_cali_data;
    memset(cali_result,0,200);
    
    if(!cali_is_done)
        return result;
    
// enable efuse
#if 0

    if(cali_is_done&LPBG_CALI_DONE)
    {        
        cali_result[0] =( lpbg_cali->cali_lpbg_bandgap &0x3f)
                               |(( lpbg_cali->cali_lpbg_vddio&0x3)<<6);
        cali_result[1] =( lpbg_cali->cali_lpbg_vddcore&0x3f)
                               | (( lpbg_cali->cali_lpbg_vddio&0xc)<<4);
        cali_result[2] = (lpbg_cali->cali_lpbg_vddio&0x30)<<2;
    }
    if(cali_is_done&CHARGE_CALI_DONE)
    {
        cali_result[2] |= charge_cali->charger_icp&0x1f ;
        cali_result[3] = charge_cali->charger_vcv;
        cali_result[4] = charge_cali->charger_vlcf;
    }
    if(cali_is_done&SARADC_CALI_DONE)  
    {
        cali_result[5] = (( (saradc_cali->sar_adc_4p2 -SARADC_CALI_4P2_DECR)&0x100)>>1)
                                | ((saradc_cali->sar_adc_dc - SARADC_CALI_DC_DECR)&0x3f) ;
        cali_result[6] = (saradc_cali->sar_adc_4p2 -SARADC_CALI_4P2_DECR)&0x0ff;
    }
    if(cali_is_done&TEMPR_CALI_DONE)
    {
        cali_result[7] =(s_ate_tempr_cali_data -TEMPR_CALI_DECR)&0x7f;
    }
    
    eFuse_enable(1);
    result = eFuse_write(cali_result,CALI_EFUSE_ADDR,8);
    if(result)
    {
        result = eFuse_read(cali_result_r,CALI_EFUSE_ADDR,8);
    }
    else
    {
        CALIB_DBG("Write Cali Data in eFuse Failed!\r\n");
        result = 2;
    }

    if(result)
    {
        for(i=0;i<8;i++)
        {            
            if(cali_result[i] != cali_result_r[i])
            {
                CALIB_DBG("Data read/wrote in eFuse is not equal!w[%d] %x!= %x\r\n",i,cali_result[i],cali_result_r[i]);
                result = 3;
            }                
        }
    }
    else
    {
        CALIB_DBG("Data read error\r\n");
        result = 4;// read error
    }
    if(result == 1)
        CALIB_DBG("eFuse write finished:ok\r\n");
    
    cali_result[0] = 0x04;  // byte16:23  write forbidden
    result = eFuse_write(cali_result,31,1); 
    CALIB_DBG("write[16:23] Forbidden set %d\r\n",result);

    eFuse_enable(0);
    
#else
    uint8_t cali_tlv[4];
    uint16_t cali_result_index = 0;
    uint32_t flash_cali_addr = 0;

    memset(cali_result,0,200);
    memcpy(cali_result,env_chip_magic,sizeof(env_chip_magic));
    cali_result_index = sizeof(env_chip_magic);
   
    if(cali_is_done&CHARGE_CALI_DONE)
    {
        cali_tlv[0] = TLV_TYPE_CALI_CHARGE;
        cali_tlv[1] = 0;
        cali_tlv[2] = sizeof(ate_charge_cali_data_t);
        cali_tlv[3] = 0;
        memcpy(&cali_result[cali_result_index],(void*)&cali_tlv,sizeof(cali_tlv));
        cali_result_index += sizeof(cali_tlv);        
        memcpy(&cali_result[cali_result_index],(void*)charge_cali,sizeof(ate_charge_cali_data_t));
        cali_result_index += sizeof(ate_charge_cali_data_t);
    }

    if(cali_is_done&SARADC_CALI_DONE)
    {
        cali_tlv[0] = TLV_TYPE_CALI_SARADC;
        cali_tlv[1] = 0;
        cali_tlv[2] = sizeof(ate_saradc_cali_data_t);
        cali_tlv[3] = 0;
        memcpy(&cali_result[cali_result_index],(void*)&cali_tlv,sizeof(cali_tlv));
        cali_result_index += sizeof(cali_tlv);  
        cali_result[cali_result_index++] = saradc_cali->sar_adc_dc%0x100;
        cali_result[cali_result_index++] = saradc_cali->sar_adc_dc>>8;
        cali_result[cali_result_index++] = saradc_cali->sar_adc_4p2%0x100;
        cali_result[cali_result_index++] = saradc_cali->sar_adc_4p2>>8;
//        memcpy(&ate_cali_result[cali_result_index],(void*)&cali_saradc,sizeof(cali_saradc));
//        cali_result_index += sizeof(cali_saradc);
    }

    if(cali_is_done&LPBG_CALI_DONE)
    {
        cali_tlv[0] = TLV_TYPE_CALI_VOLTAGE;
        cali_tlv[1] = 0;
        cali_tlv[2] = sizeof(ate_lpbg_cali_data_t);
        cali_tlv[3] = 0;
        memcpy(&cali_result[cali_result_index],(void*)&cali_tlv,sizeof(cali_tlv));
        cali_result_index += sizeof(cali_tlv);   
        memcpy(&cali_result[cali_result_index],(void*)lpbg_cali,sizeof(ate_lpbg_cali_data_t));
        cali_result_index += sizeof(ate_lpbg_cali_data_t);
    }
    
    if(cali_is_done&TEMPR_CALI_DONE)
    {
        cali_tlv[0] = TLV_TYPE_CALI_TEMPR;
        cali_tlv[1] = 0;
        cali_tlv[2] = sizeof(uint16_t);
        cali_tlv[3] = 0;
        memcpy(&cali_result[cali_result_index],(void*)&cali_tlv,sizeof(cali_tlv));
        cali_result_index += sizeof(cali_tlv);   
        memcpy(&cali_result[cali_result_index],(void*)&s_ate_tempr_cali_data,sizeof(uint16_t));
        cali_result_index += sizeof(uint16_t);
    }

    flash_cali_addr = app_env_get_flash_addr(TLV_SECTOR_ENVCALI);
    //flash_erase_sector(flash_cali_addr, FLASH_ERASE_4K); //  @lvjinxia: low DPLL can't run flash erase
    flash_write_data(cali_result,flash_cali_addr,cali_result_index);
    flash_read_data(cali_result_r,flash_cali_addr,cali_result_index);
    
    for(i=0;i<cali_result_index;i++)
    {            
        if(cali_result[i] != cali_result_r[i])
        {
            os_printf("Data read/wrote in eFuse is not equal!w[%d] %x!= %x\r\n",i,cali_result[i],cali_result_r[i]);
            result = 0;
            break;
        }                
    }
    
    if(i>=cali_result_index)
    {
        result = 1;
        os_printf("write cali data ok:%d\r\n",cali_result_index);
    }
#endif

    return result;    
}


