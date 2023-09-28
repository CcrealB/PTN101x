#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "app_temperature_sensor.h"
static volatile uint8 s_need_update_bt_ldo = 0;
#define SARADC_TEMP_DATA_NUM       8
#define TEMPERATURE_THRESHOLD     33  // 3.34 * 20;
#define BT_LDO_THRESHOLD_MIN       2
#define BT_LDO_THRESHOLD_MAX       0x1d
CONST int8 C_Delta_Cali_Table[16]  = {
    -10,  -9, -7, -6, -4, -3, -1,  0, 2, 4,  5,  7,  8,  10,12, 13
//  -1,   -2, -1, -2, -1, -2, -1,  0, 2, 2,  1,  2,  1,  2, 2,  1// calibration value;
//  -70,-60,-50,-40,-30,-20,-10, 0, 10, 20, 30, 40, 50, 60, 70, 80,// Temperature
};

static _Temperature_Cali_T s_Temperature_Cali;
 
void app_bt_ldo_init(void)
{
    s_need_update_bt_ldo = app_calibration_bt_ldo();
    s_Temperature_Cali.Calibration_base = s_need_update_bt_ldo;
    s_Temperature_Cali.Delta_Temperature_idx = 7;
    app_update_bt_ldo(s_need_update_bt_ldo);
    LOG_I(LDO,"BT.LDO:%02x\r\n",s_need_update_bt_ldo);
}
/* CEVA INTR:PKD or NO_PKT_RCVD */
void app_modify_bt_ldo(void)
{
    if(s_need_update_bt_ldo & 0x80)
    {
        app_update_bt_ldo(s_need_update_bt_ldo & 0x1f);
		app_set_crystal_calibration(1);
		msg_put(MSG_TEMPRATURE_APLL_TOGGLE);
#if (CPU_CLK_SEL == CPU_CLK_DPLL)
        msg_put(MSG_TEMPRATURE_DPLL_TOGGLE);
#endif
        s_need_update_bt_ldo &= ~0x80;
    }
}
static void bubble_sort(uint16 *data,uint16 len)
{
    uint16 temp_x;
    uint16 i,j;
    for(i=0;i<len-1;i++)
    {
        for(j=0;j<len-i-1;j++)
        {
            if(data[j] > data[j+1])
            {
                temp_x = data[j];
                data[j] = data[j+1];
                data[j+1] = temp_x;
            }
        }
    }
}
/* Warning: Win size = 3 or 5 */
uint16 app_temperature_median_filter(uint16 *data_ptr,uint16 win_size)
{
    uint16 data[5];
    if(win_size == 3)
    {
        memcpy(data,data_ptr,6);
        bubble_sort(data,3);
        return(data[1]);
    }
    else   // must be 5,
    {
        memcpy(data,data_ptr,10);
        bubble_sort(data,5);
        return(data[2]);
    }
}
void app_set_temperature_base(uint16 *data_ptr)
{
    uint16 i;
    uint16 avg_temp = 0;
    for(i=0; i<SARADC_TEMP_DATA_NUM; i++)
    {
        avg_temp +=  (*(data_ptr+i)>>(SARADC_TEMP_DATA_NUM - i));
    }
    s_Temperature_Cali.Temperature_base = avg_temp;
}
void app_update_temperature_base(uint16 *data_ptr)
{
    uint16 i;
    uint16 avg_temp = 0;
    uint16 tempe_diff = 0;
    int8   sign_diff = 0;
    int8   curr_cali_base = 0;
    for(i=0; i<SARADC_TEMP_DATA_NUM; i++)
    {
        avg_temp +=  (*(data_ptr+i)>>(SARADC_TEMP_DATA_NUM - i));
    }
    sign_diff = 2*(avg_temp > s_Temperature_Cali.Temperature_base) - 1;
    tempe_diff = (sign_diff > 0) ? (avg_temp - s_Temperature_Cali.Temperature_base) : (s_Temperature_Cali.Temperature_base - avg_temp);

    if(tempe_diff > TEMPERATURE_THRESHOLD)
    {
        s_Temperature_Cali.Temperature_base = avg_temp;
#if 0   // while BT RX/TX enable,this calibration is not accurate.It will be replaced by searching table;
        s_need_update_bt_ldo = app_calibration_bt_ldo();
        s_need_update_bt_ldo |= 0x80;
#endif
        s_Temperature_Cali.Delta_Temperature_idx -= sign_diff;   // Invert;
		if(s_Temperature_Cali.Delta_Temperature_idx < 0 ) s_Temperature_Cali.Delta_Temperature_idx = 0;
		if(s_Temperature_Cali.Delta_Temperature_idx > 0x0f ) s_Temperature_Cali.Delta_Temperature_idx = 0x0f;

		curr_cali_base = s_Temperature_Cali.Calibration_base + C_Delta_Cali_Table[(s_Temperature_Cali.Delta_Temperature_idx) & 0x0f];
		if(curr_cali_base < BT_LDO_THRESHOLD_MIN) curr_cali_base = BT_LDO_THRESHOLD_MIN;
		if(curr_cali_base > BT_LDO_THRESHOLD_MAX) curr_cali_base = BT_LDO_THRESHOLD_MAX;


        s_need_update_bt_ldo = curr_cali_base | 0x80;
        FATAL_PRT("BT.LDO:%02x,%d\r\n",s_need_update_bt_ldo,tempe_diff*sign_diff*(-1));
    }
}
uint16 app_calibration_bt_ldo(void)
{
/*
SYSTEM.REG4C<30>=1   //enable 32k clock to BTLDO calibration.
SYSTEM.REG40<12>=0    //power on btldo calibration block.
SYSTEM.REG40<20>=1   // enable ABBCB��provide 1u bias to calibration block.  

SYSTEM.REG51<31>=0       
SYSTEM.REG51<31>=1
SYSTEM.REG51<31>=0

waiting 5ms
SYSTEM.REG36<4:0>=X;
*/
    uint16 cali_x = 0;
    REG_SYSTEM_0x4C |= (1<<30); 
    REG_SYSTEM_0x40 &= ~(1<<12);
    sys_delay_cycle(6); 
    REG_SYSTEM_0x40 |= (1<<20);
    REG_SYSTEM_0x5B &= ~(1<<31);
    sys_delay_cycle(600); 
    REG_SYSTEM_0x5B |= (1<<31);
    sys_delay_cycle(600); 
    REG_SYSTEM_0x5B &= ~(1<<31);
    // wait for 5ms;
    os_delay_ms(5);
    cali_x = (REG_SYSTEM_0x36 & 0x1f);
    sys_delay_cycle(6);
    REG_SYSTEM_0x40 |= (1<<12);
    cali_x += 4;
    if(cali_x > 0x1f) cali_x = 0x1f;
    return cali_x;
 
}
void app_update_bt_ldo(uint16 cali_x)
{
    uint32 reg = REG_SYSTEM_0x40;
    reg &= ~(0x1f << 7);// bt ldo;
    reg |= (cali_x << 7);
    REG_SYSTEM_0x40 = reg;
#if 0
    reg = REG_SYSTEM_0x4F;
    reg &= ~(0x1f << 11);
    reg |= (cali_x << 11);
    REG_SYSTEM_0x4F = reg;
    
    reg = REG_SYSTEM_0x55;
    reg &= ~(0x1f << 6);
    reg |= (cali_x << 6);
    REG_SYSTEM_0x55 = reg;

    reg = REG_SYSTEM_0x57;
    reg &= ~(0x1f << 27);
    reg |= (cali_x << 27);
    REG_SYSTEM_0x57 = reg;
    sys_delay_cycle(6); 

    reg = REG_SYSTEM_0x57;
    reg &= ~(0x1f << 9);
    reg |= (cali_x << 9);
    REG_SYSTEM_0x57 = reg;
#endif
}

