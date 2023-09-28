#ifndef _APP_BEKEN_INCLUDES_H_
#define _APP_BEKEN_INCLUDES_H_

#include <stddef.h>
#include <stdint.h>
#include <config/config.h>
#include <jos.h>
#include <bluetooth.h>
#include <bluetooth/core/hci_internal.h>
#include <bt_a2dp_sink.h>
#include <bluetooth/bt_sco_backend_utils.h>
#include "app_sbc.h"
#include "mp3dec.h"
#include "app_anc.h"
#include "app_work_mode.h"
#include "app_button.h"
#include "app_led.h"
#include "app_charge.h"
#include "app_prompt_wav.h"

#include "app_customer.h"
#include "app_env.h"
#include "app_debug.h"
#include "app_aec.h"
#include "app_hfp.h"
#include "app_equ.h"

#include "app_async_data_stream.h"
#include "playmode.h"
#include "app_player.h"
#include "driver_audio.h"
#include "app_linein.h"
#include "api_usb.h"
#include "app_dsp.h"
#include "app_spdif.h"
#include "../mod_irda/app_irda.h"
#include "app_exception.h"

#include "app_msg.h"

#include "audio_out_interface.h"
#include "app_bt.h"
#include "app_temperature_sensor.h"
#include "driver_flash.h"
#include "bt_app_internal.h"

#include "app_bt_connect.h"
#include "app_bt_management.h"
#include "lslc_irq.h"

#if (CONFIG_DRIVER_OTA == 1)
#include "driver_ota.h"
#endif
#if (BEKEN_OTA == 1)
#include "beken_ota.h"
#endif
#if (CONFIG_TEMPERATURE_NTC == 1)
#include "app_temperature_ntc.h"
#endif
#if (CONFIG_EAR_IN == 1)
#include "app_ear_in.h"
#endif
/* current transaction id. */
#define TID_A2DP_PROTOCOL_SSA_REQUEST   0x7000
#define TID_HFP_PROTOCOL_SSA_REQUEST    0x7001

#endif

//EOF
