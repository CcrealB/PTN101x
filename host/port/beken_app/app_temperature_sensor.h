#ifndef __TEMPERATURE_SENSOR__
#define __TEMPERATURE_SENSOR__

typedef struct _temperature_cali_s
{
    uint16 Temperature_base;
    int8   Delta_Temperature_idx;
    int8   Calibration_base;
}__PACKED_POST__  _Temperature_Cali_T;

void app_bt_ldo_init(void);
__inline uint8 app_need_modify_bt_ldo(void);
uint16 app_temperature_median_filter(uint16 *data_ptr,uint16 win_size);
void app_set_temperature_base(uint16 *data_ptr);
void app_update_temperature_base(uint16 *data_ptr);
uint16 app_calibration_bt_ldo(void);
void app_update_bt_ldo(uint16 cali_x);
#endif
