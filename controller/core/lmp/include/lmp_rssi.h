#ifndef __LMP_RSSI_H__
#define __LMP_RSSI_H__
/**/
#include "lc_log.h"

#include "bt_timer.h"
#include "bt_fhs.h"

#include "hw_jal_defs.h"
#include "hw_lc.h"
#include "hw_memcpy.h"

#include "sys_mmi.h"

#include "dl_dev.h"

#include "lslc_pkt.h"
#include "lslc_access.h"
#include "lslc_clk.h"
#include "lslc_freq.h"
#include "lslc_hop.h"

#include "uslc_pagescan.h"
#include "uslc_chan_ctrl.h"

#include "lmp_acl_container.h"
#include "lmp_sco_container.h"
#include "lmp_config.h"
#include "types.h"
#if  LMP_RSSI_DEVICE_ASSESSMENT
#define LMP_RSSI_DEVICE_UPDATE_SLOTS           (160)
#define DEVICE_LONG_DISTANCE_THRESHOLD         (-80)
#define DEVICE_SHORT_DISTANCE_THRESHOLD        (-65)

#define DEVICE_DISTANCE_STATE_GOLDEN             0
#define DEVICE_DISTANCE_STATE_SHORT             (-1)
#define DEVICE_DISTANCE_STATE_LONG              (1)


#define DEVICE_TX_QUALITY_GOOD_THRESHOLD        (200)
#define DEVICE_TX_QUALITY_BAD_THRESHOLD         (100)

#define DEVICE_TX_QUALITY_STATE_GOOD              5
#define DEVICE_TX_QUALITY_STATE_BAD               3
#define DEVICE_TX_QUALITY_STATE_POOR              1

#define DEVICE_RX_QUALITY_GOOD_THRESHOLD        (150)
#define DEVICE_RX_QUALITY_BAD_THRESHOLD         (75)

#define DEVICE_RX_QUALITY_STATE_GOOD              5
#define DEVICE_RX_QUALITY_STATE_BAD               3
#define DEVICE_RX_QUALITY_STATE_POOR              1


void LMPrssi_Active_RSSI_Update(t_lmp_link *p_link);
void LMPrssi_Update_Device_RSSI(t_lmp_link *p_link);
void LMPrssi_Set_Device_Init_RSSI(t_deviceIndex device_index,char rssi);
char LMPrssi_Get_Device_RSSI(t_deviceIndex device_index);
char LMPrssi_Get_Device_Distance(t_deviceIndex device_index);
u_int8 LMPrssi_Emergency_Poll(t_deviceIndex device_index);
void LMPrssi_Update_Power_of_TWS(t_deviceIndex device_index);
void LMPrssi_Clear_Timer(t_lmp_link *p_link);
char LMPrssi_Get_Device_Tx_Quality(t_deviceIndex device_index);
char LMPrssi_Get_Device_Rx_Quality(t_deviceIndex device_index);
void LMPrssi_Dump_Quality_Info(void);
uint8_t LMPrssi_Get_Device_LinkTx_Quality(t_deviceIndex device_index);
uint8_t LMPrssi_Get_Device_Link_Quality(t_deviceIndex device_index);
#endif
#endif
