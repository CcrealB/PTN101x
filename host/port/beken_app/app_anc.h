/**
 **************************************************************************************
 * @file    app_anc.h
 * @brief   Application for Active Noise Cancellation
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2020 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __APP_ANC_H__
#define __APP_ANC_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

enum
{
    ANC_STATUS_IDLE = 0,
    ANC_STATUS_NOISE_CANCEL,
    ANC_STATUS_TRANSPARENCY,
};

enum
{
	ANC_STATE_INIT = 0,
	ANC_STATE_SPP_CMD,
	ANC_STATE_HFP_CMD,
	ANC_STATE_HFP_RSP,
};

/*spp test cmds between PTN101 and testbox*/
enum
{
	ANC_CMD_ENTER_TESTMODE = 0,
	ANC_CMD_MASTER_IS_LEFT = 1,
	ANC_CMD_ANC_OFF = 2,
	ANC_CMD_ANC_ON = 3,
	ANC_CMD_SET_GAIN = 4,
	ANC_CMD_WRITE_GAIN = 5,
	ANC_CMD_READ_GAIN = 6,
	ANC_CMD_ANC_WHITE_NOISE_OUT = 7,
	ANC_CMD_ANC_PULSE_OUT = 8,
	ANC_CMD_FILTER_UPDATE = 9,
	ANC_CMD_FILTER_WRITE = 0x0A,

	ANC_CMD_TRANSPARENCY,
	ANC_CMD_MASTER_PLUS,
	ANC_CMD_MASTER_MINUS,
	ANC_CMD_SLAVE_PLUS,
	ANC_CMD_SLAVE_MINUS,
	ANC_CMD_MASTER_SAVE,
	ANC_CMD_SLAVE_SAVE,
	
	ANC_CMD_SLAVE_SET_GAIN,
	ANC_CMD_MASTER_SET_GAIN,

	ANC_CMD_STATUS_SWITCH = 0x53,
	ANC_CMD_CTRL_LIMITER = 0x4C,
	ANC_CMD_CTRL_I2S = 0x7D,
	ANC_CMD_CTRL_ANA_GAIN_SET = 0xA4,
	ANC_CMD_CTRL_BYPASS = 0xBB,
	ANC_CMD_CTRL_CLEAR = 0xCC,
	ANC_CMD_GAN_FILTER_READ = 0x66,
	ANC_CMD_GAN_FILTER_UPDATE = 0x99,
	ANC_CMD_DBGAIN_READ    = 0xD6,
	ANC_CMD_DBGAIN_WRITE   = 0xD5,
	ANC_CMD_DBGAIN_WRITE2  = 0xDD,
	ANC_CMD_DBGAIN_UPDATE  = 0xD4,
	ANC_CMD_DBGAIN_INVERSE = 0xD1,
};
/*cmds && rsp between master and slave*/
enum
{
	ANC_HFP_CMD_ANC_OFF =0,
	ANC_HFP_CMD_ANC_ON,
	ANC_HFP_CMD_SET_GAIN,
	ANC_HFP_CMD_WRITE_GAIN,
	ANC_HFP_CMD_READ_GAIN,
	ANC_HFP_RSP,
	
	ANC_HFP_CMD_TRANSPARENCY,
	ANC_HFP_CMD_PLUS,
	ANC_HFP_CMD_MINUS,
	ANC_HFP_CMD_SAVE,
	ANC_HFP_CMD_SET_GAIN_TEST,

	ANC_HFP_CMD_ANC_WHITE_NOISE_OUT,
	ANC_HFP_CMD_ANC_PULSE_OUT,
	ANC_HFP_CMD_FILTER_WRITE,
	ANC_HFP_CMD_FILTER_UPDATE,
};

typedef union
{
    int8_t   i8[4];
    uint8_t  u8[4];
    int16_t  i16[2];
    uint16_t u16[2];
    int32_t  i32;
    uint32_t u32;
    float    f32;
}Variant32;

typedef struct _AncParams
{
    int16_t noise_cancel_ff_volume;
    int16_t noise_cancel_fb_volume;
    int16_t transparency_ff_volume;
    int16_t transparency_fb_volume;
    int16_t noise_cancel_ec_volume;
    uint8_t channel_id;
    uint8_t reserved;
    int16_t transparency_ec_volume;
}AncParam;

typedef struct _AncVolScaler
{
	float    noise_mic35_vol_scale;
	float 	 noise_mic24_vol_scale;
	float 	 trans_mic35_vol_scale;
	float 	 trans_mic24_vol_scale;
	float 	 noise_ec_vol_scale;
	uint32_t channel_id;
	float    trans_ec_vol_scale;
}AncVolScaler;

typedef struct anc_spp_cmd
{
	uint8_t cmd_sync;
	uint8_t cmd;
	int16_t param1;
	int16_t param2;
	int16_t param3;
	int16_t param4;
	uint8_t crc;
}ANC_SPP_CMD;

typedef struct anc_spp_filter_cmd
{
	uint8_t cmd_sync;
	uint8_t cmd;
	uint8_t trans_noise_mode;
	uint8_t filter_num;
	uint8_t filter1[20];
	uint8_t filter2[20];
	uint8_t filter3[20];
	uint8_t filter4[20];
	uint8_t filter5[20];
	uint8_t crc;
	uint8_t filters[20*5-1];
}ANC_SPP_FILETER_CMD;

enum ANC_NOISE_TRANS_MODE
{
    ANC_NOISE_MIC3 = 0,
    ANC_TRANS_MIC3 = 1,
    ANC_NOISE_MIC5 = 2,
    ANC_TRANS_MIC5 = 3,
    ANC_NOISE_MIC2 = 4,
    ANC_TRANS_MIC2 = 5,
    ANC_NOISE_MIC4 = 6,
    ANC_TRANS_MIC4 = 7,
    ANC_NOISE_EC0  = 8,
    ANC_TRANS_EC0  = 9,
    ANC_NOISE_EC1  = 10,
    ANC_TRANS_EC1  = 11,
    ANC_NOISE_TRANS_MODE_COUNT
};

void app_anc_init(void);
void app_anc_enable(uint32_t enable);

int32_t app_anc_status_get(void);

void app_anc_status_switch_to(int32_t status);
void app_anc_freq_changed(uint32_t freq);
void app_anc_volume_restore(int32_t flag);
void app_anc_mode_restore(int32_t flag);

void app_anc_dbg(uint8_t key);
//void app_anc_debug(HCI_COMMAND_PACKET*, HCI_EVENT_PACKET*);
void app_anc_gain_params_read_from_flash(uint32_t addr, uint32_t size);
void app_anc_filter_coefs_read_from_flash(uint32_t addr, uint32_t size);
void app_anc_vol_scaler_read_from_flash(uint32_t addr, uint32_t size);

#endif//__APP_ANC_H__
