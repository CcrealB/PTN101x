/**
 **************************************************************************************
 * @file    drv_audio.h
 * @brief   Driver API for audio codec
 *
 * @author  Aixing.Li
 * @version V3.0.0
 *
 * &copy; 2018~2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_AUDIO_H__
#define __DRV_AUDIO_H__

#include <stdint.h>

#define AUDIO_STAUS_MSK_ADC_FIFO_EMPTY          (1 << 0 )
#define AUDIO_STAUS_MSK_ADC_FIFO_FULL           (1 << 1 )
#define AUDIO_STAUS_MSK_ANC_FIFO_EMPTY          (1 << 2 )
#define AUDIO_STAUS_MSK_ANC_FIFO_FULL           (1 << 3 )
#define AUDIO_STAUS_MSK_DAC_TX1_FIFO_EMPTY      (1 << 4 )
#define AUDIO_STAUS_MSK_DAC_TX1_FIFO_FULL       (1 << 5 )
#define AUDIO_STAUS_MSK_DAC_TX2_FIFO_EMPTY      (1 << 6 )
#define AUDIO_STAUS_MSK_DAC_TX2_FIFO_FULL       (1 << 7 )
#define AUDIO_STAUS_MSK_DAC_TX1_LPF_FIFO_EMPTY  (1 << 8 )
#define AUDIO_STAUS_MSK_DAC_TX1_LPF_FIFO_FULL   (1 << 9 )
#define AUDIO_STAUS_MSK_DAC_TX1_2FS_FIFO_EMPTY  (1 << 10)
#define AUDIO_STAUS_MSK_DAC_TX1_2FS_FIFO_FULL   (1 << 11)
#define AUDIO_STAUS_MSK_ADC_2FS_FIFO_EMPTY      (1 << 12)
#define AUDIO_STAUS_MSK_ADC_2FS_FIFO_FULL       (1 << 13)
#define AUDIO_STAUS_MSK_DAC_TX1_2FS_INT_FLAG    (1 << 25)
#define AUDIO_STAUS_MSK_ADC_2FS_INT_FLAG        (1 << 26)
#define AUDIO_STAUS_MSK_DAC_TX2_INT_FLAG        (1 << 27)
#define AUDIO_STAUS_MSK_DAC_TX1_INT_FLAG        (1 << 28)
#define AUDIO_STAUS_MSK_DAC_TX1_LPF_INT_FLAG    (1 << 29)
#define AUDIO_STAUS_MSK_ANC_INT_FLAG            (1 << 30)
#define AUDIO_STAUS_MSK_ADC_INT_FLAG            (1 << 31)

#define AUDIO_DIV_16K           0x0F1FAA4C//0x06590000
#define AUDIO_DIV_16K_1PPM      (1024)       //3117876   //106
#define AUDIO_DIV_8K_1PPM       (1024)      //6235752   //212

#define AUDIO_DIV_441K          0x0DE517A9//0x049B2369
#define AUDIO_DIV_441K_SLOW     0x0DE507A9//0x0CC6492B//0x049B2970
#define AUDIO_DIV_441K_FAST     0x0DE527A9//0x0F0DE627//0x049B1D5C
#define AUDIO_DIV_441K_1PPM     896       //956192    //77

#define AUDIO_DIV_48K           0x0F1FAA4C//0x043B5555
#define AUDIO_DIV_48K_SLOW      0x0F1F9A4C//0x0DE27F98//0x043B5AE0
#define AUDIO_DIV_48K_FAST      0x0F1FBA4C//0x105CD500//0x043B4FC8
#define AUDIO_DIV_48K_1PPM      1024       //1039292   //70

/**
 * @brief audio ADC & ANC channels definition
 */
typedef enum
{
    AUDIO_ADC_CHANNEL_0 = 1 << 0,
    AUDIO_ANC_CHANNEL_0 = 1 << 1,
    AUDIO_ANC_CHANNEL_1 = 1 << 2,
    AUDIO_ANC_CHANNEL_2 = 1 << 3,
    AUDIO_ANC_CHANNEL_3 = 1 << 4,
}AUDIO_ADC_ANC_CHANNEL;

/**
 * @brief audio ADC mode definition
 */
typedef enum
{
    AUDIO_ADC_MODE_DIFFERENCE,
    AUDIO_ADC_MODE_SINGLE_END,
    AUDIO_ADC_MODE_AUTO_DC_CALIB,
}AUDIO_ADC_MODE;

/**
 * @brief audio ADC bit width definition
 */
typedef enum
{
    AUDIO_ADC_WIDTH_24,
    AUDIO_ADC_WIDTH_20,
    AUDIO_ADC_WIDTH_16,
}AUDIO_ADC_WIDTH;

/**
 * @brief audio DAC mode definition
 */
typedef enum
{
    AUDIO_DAC_MODE_DIFFERENCE,
    AUDIO_DAC_MODE_SINGLE_END,
    AUDIO_DAC_MODE_CLASSD,
    AUDIO_DAC_MODE_VCOM,
}AUDIO_DAC_MODE;

/**
 * @brief audio DAC calibration process
 */
typedef enum
{
    AUDIO_DAC_CALIB_START,
    AUDIO_DAC_CALIB_WAITING,
    AUDIO_DAC_CALIB_FINISH,
    AUDIO_DAC_CALIB_START_TO_FINISH,
}AUDIO_DAC_CALIB;

/**
 * @brief audio fifo data mode definition
 */
typedef enum
{
    AUDIO_FIFO_DATA_MODE_INTERLEAVE,
    AUDIO_FIFO_DATA_MODE_SEPARATION,
}AUDIO_FIFO_DATA_MODE;

/**
 * @brief audio control command definition
 */
typedef enum
{
    AUDIO_CTRL_CMD_NULL = 0,
    AUDIO_CTRL_CMD_FIR_MEM_CLEAR,
    AUDIO_CTRL_CMD_STATUS_GET,
    AUDIO_CTRL_CMD_SWITCH_TO_MIC,   //0~4 means channel0 ~ channel4
    AUDIO_CTRL_CMD_SWITCH_TO_DACL,  //0~4 means channel0 ~ channel4
    AUDIO_CTRL_CMD_SWITCH_TO_DACR,  //0~4 means channel0 ~ channel4
    AUDIO_CTRL_CMD_SWITCH_TO_DACLR, //0~4 means channel0 ~ channel4
    AUDIO_CTRL_CMD_SWITCH_TO_LINEIN,//0:LINEIN0, 1:LINEIN1
    AUDIO_CTRL_CMD_ADC_ANC_INIT,
    AUDIO_CTRL_CMD_ADC_ANC_ENABLE,
    AUDIO_CTRL_CMD_ADC_ANC_SAMPLE_RATE_SET,
    AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET,
    AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_GET,

    AUDIO_CTRL_CMD_ADC_ENABLE = 0x40,//0:disable, 1:ADC0
    AUDIO_CTRL_CMD_ADC_INT_ENABLE,
    AUDIO_CTRL_CMD_ADC_2FS_INT_ENABLE,
    AUDIO_CTRL_CMD_ADC_CIC1_D1_SET,
    AUDIO_CTRL_CMD_ADC_LPF12_TYPE_SET,
    AUDIO_CTRL_CMD_ADC_LPF1_BYPASS,
    AUDIO_CTRL_CMD_ADC_LPF2_BYPASS,
    AUDIO_CTRL_CMD_ADC_LPF1_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_ADC_LPF2_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_ADC_LPF1_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_ADC_LPF2_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_ADC_SAMPLE_EDGE_SET,
    AUDIO_CTRL_CMD_ADC_DIG_MIC0_ENABLE,
    AUDIO_CTRL_CMD_ADC_DIG_MIC1_ENABLE,
    AUDIO_CTRL_CMD_ADC_SAMPLE_WIDTH_SET,
    AUDIO_CTRL_CMD_ADC_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_ADC_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_ADC_2FS_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_ADC_2FS_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_ADC_GAIN_SET,
    AUDIO_CTRL_CMD_ADC_FIFO_READ,

    AUDIO_CTRL_CMD_ANC_ENABLE = 0x80,//0:disable, 1:ANC0, 2:ANC1, 4:ANC2, 8:ANC3
    AUDIO_CTRL_CMD_ANC_INT_ENABLE,
    AUDIO_CTRL_CMD_ANC_CLK_ENABLE,
    AUDIO_CTRL_CMD_ANC_CIC1_D1_SET,
    AUDIO_CTRL_CMD_ANC_LPF12_TYPE_SET,
    AUDIO_CTRL_CMD_ANC_LPF1_BYPASS,
    AUDIO_CTRL_CMD_ANC_LPF2_BYPASS,
    AUDIO_CTRL_CMD_ANC_LPF1_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_ANC_LPF2_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_ANC_LPF1_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_ANC_LPF2_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_ANC_SAMPLE_WIDTH_SET,
    AUDIO_CTRL_CMD_ANC_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_ANC_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_ANC_GAIN_SET,
    AUDIO_CTRL_CMD_ANC_FIFO_DATA_MODE_SET,//@see AUDIO_FIFO_DATA_MODE
    AUDIO_CTRL_CMD_ANC_FIFO_READ,//for interleave mode
    AUDIO_CTRL_CMD_ANC_FIFO_READ4x16,
    AUDIO_CTRL_CMD_ANC_FIFO_READ4x32,
    AUDIO_CTRL_CMD_ANC_FIFO_CH0_READ,
    AUDIO_CTRL_CMD_ANC_FIFO_CH1_READ,
    AUDIO_CTRL_CMD_ANC_FIFO_CH2_READ,
    AUDIO_CTRL_CMD_ANC_FIFO_CH3_READ,
    AUDIO_CTRL_CMD_ANC_DMIC_ENABLE,//0:disable, 1:DMIC0, 2:DMIC1

    AUDIO_CTRL_CMD_DAC_ENABLE = 0xC0,//0:disable, 1:enable
    AUDIO_CTRL_CMD_DAC_TX1_INT_ENABLE,
    AUDIO_CTRL_CMD_DAC_TX1_2FS_INT_ENABLE,
    AUDIO_CTRL_CMD_DAC_TX1_LPF_INT_ENABLE,
    AUDIO_CTRL_CMD_DAC_TX2_INT_ENABLE,
    AUDIO_CTRL_CMD_DAC_ADDER_MODE_SET,
    AUDIO_CTRL_CMD_DAC_SAMPLE_RATE_SET,
    AUDIO_CTRL_CMD_DAC_CIC1_D2_SET,
    AUDIO_CTRL_CMD_DAC_CIC2_D3_SET,
    AUDIO_CTRL_CMD_DAC_CIC2_BYPASS,
    AUDIO_CTRL_CMD_DAC_LPF34_TYPE_SET,
    AUDIO_CTRL_CMD_DAC_LPF3_BYPASS,
    AUDIO_CTRL_CMD_DAC_LPF4_BYPASS,
    AUDIO_CTRL_CMD_DAC_LPF3_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_DAC_LPF4_FIR_ORDER_SET,
    AUDIO_CTRL_CMD_DAC_LPF3_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_DAC_LPF4_IIR_TYPE_SET,
    AUDIO_CTRL_CMD_DAC_PN_CONFIG,
    AUDIO_CTRL_CMD_DAC_NOTCH_ENABLE,
    AUDIO_CTRL_CMD_DAC_TX1_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_DAC_TX1_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_DAC_TX1_2FS_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_DAC_TX1_2FS_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_DAC_TX1_LPF_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_DAC_TX1_LPF_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_DAC_TX2_FIFO_RDTH_SET,
    AUDIO_CTRL_CMD_DAC_TX2_FIFO_WRTH_SET,
    AUDIO_CTRL_CMD_DAC_CH0_LOOP_SRC_SEL,
    AUDIO_CTRL_CMD_DAC_CH1_LOOP_SRC_SEL,
    AUDIO_CTRL_CMD_DAC_FIFO_DATA_MODE_SET,//@see AUDIO_FIFO_DATA_MODE
    AUDIO_CTRL_CMD_DAC_FIFO_CH0_WRITE,
    AUDIO_CTRL_CMD_DAC_FIFO_CH1_WRITE,
    AUDIO_CTRL_CMD_DAC_TX1_FIFO_CH0_WRITE,
    AUDIO_CTRL_CMD_DAC_TX1_FIFO_CH1_WRITE,
    AUDIO_CTRL_CMD_DAC_TX2_FIFO_CH0_WRITE,
    AUDIO_CTRL_CMD_DAC_TX2_FIFO_CH1_WRITE,

}AUDIO_CTRL_CMD;

/**
 * @brief audio initialization
 */
void audio_init(void);

/**
 * @brief audio ADC initialization
 * @param bits ADC data bits width, @see AUDIO_ADC_WIDTH
 * @param mode ADC mode, @see AUDIO_ADC_MODE
 */
void audio_adc_init(uint32_t bits, uint32_t mode);

/**
 * @brief audio ADC sample rate configuration
 * @param samplerate ADC sample rate to be set, can be from 8KHz ~ 384KHz
 */
void audio_adc_sample_rate_set(uint32_t samplerate);

/**
 * @brief audio ADC analog gain set
 * @param gain analog gain to be set, value range from 0 ~ 12 which means 0dB ~ 24dB, step is 2dB
 */
void audio_adc_ana_gain_set(uint32_t gain);

/**
 * @brief audio ADC digital gain set
 * @param gain digital gain to be set, value range from 0 ~ 0x3F which means -45dB ~ 18dB, 0x2d means 0dB, step is 1dB
 */
void audio_adc_dig_gain_set(uint32_t gain);

/**
 * @brief audio ADC enable
 * @param enable 0:disable, 1:enable
 */
void audio_adc_enable(uint32_t enable);

/**
 * @brief audio DAC initialization
 * @param mode DAC mode, @see AUDIO_DAC_MODE
 */
void audio_dac_init(uint32_t mode);

/**
 * @brief audio DAC calibration
 * @param type calibration type, @see AUDIO_DAC_CALIB
 */
void audio_dac_dc_calib(uint32_t type);

/**
 * @brief audio DAC sample rate configuration
 * @param samplerate DAC sample rate to be set, can be from 8KHz ~ 384KHz
 */
void audio_dac_sample_rate_set(uint32_t samplerate);

/**
 * @brief audio DAC analog gain set
 * @param gain analog gain to be set, value range from 0 ~ 7 which means 0dB ~ 7dB
 */
void audio_dac_ana_gain_set(uint32_t gain);

/**
 * @brief audio DAC enable
 * @param enable 0:disable, 1:enable
 */
void audio_dac_enable(uint32_t enable);

/**
 * @brief audio DAC fade out
 * @param channels DAC TX channel, 1:TX1, 2:TX2, 3:TX1+TX2
 */
void audio_dac_fadeout(uint32_t channels);

/**
 * @brief audio ANC initialization
 * @param bits ANC data bits width, @see AUDIO_ADC_WIDTH
 * @param mode ANC mode, @see AUDIO_ADC_MODE
 */
void audio_anc_init(uint32_t bits, uint32_t mode);

/**
 * @brief audio ANC sample rate configuration
 * @param samplerate ANC sample rate to be set, can be from 8KHz ~ 384KHz
 */
void audio_anc_sample_rate_set(uint32_t samplerate);

/**
 * @brief audio ANC analog gain set
 * @param gain analog gain to be set, value range from 0 ~ 12 which means 0dB ~ 24dB, step is 2dB
 */
void audio_anc_ana_gain_set(uint32_t gain);

/**
 * @brief audio ANC digital gain set
 * @param gain digital gain to be set, value range from 0 ~ 0x3F which means -45dB ~ 18dB, 0x2d means 0dB, step is 1dB
 */
void audio_anc_dig_gain_set(uint32_t gain);

/**
 * @brief audio ANC enable
 * @param enable 0:disable, 1:enable
 */
void audio_anc_enable(uint32_t enable);

/**
 * @brief audio ANC enable
 * @param channels ANC channels, @see AUDIO_ADC_ANC_CHANNEL
 * @param enable   0:disable, 1:enable
 */
void audio_anc_enable_ext(uint32_t channels, uint32_t enable);

/**
 * @brief audio control
 * @param cmd control command, @see AUDIO_CTRL_CMD
 * @param arg control argument
 * @return error code
 */
int32_t audio_ctrl(uint32_t cmd, uint32_t arg);

void audio_dac_ana_mute(uint32_t enable);
void audio_dac_ana_enable(uint32_t enable);
void audio_dac_dig_enable(uint32_t enable);
void set_audio_dc_calib_data(uint16_t cali_data);
uint16_t get_audio_dc_calib_data(void);

#endif//__DRV_AUDIO_H__
