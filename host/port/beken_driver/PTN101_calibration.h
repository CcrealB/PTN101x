/**
 **************************************************************************************
 * @file    bk3266_calibration.h
 * @brief   Calibrations for BK3266, such as audio dac DC offset, charger, sar-adc, tx, etc
 * 
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BK3266_CALIBRATION_H__
#define __BK3266_CALIBRATION_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "types.h"

void cal_delay_us(uint32_t us);
void cal_delay_ms(uint32_t ms);
/**
 * @brief  SAR-ADC enable
 * @brief  channel 0:VBATTERY, others:FIXME
 * @param  enable  0:disable, 1:enable
 * @return NULL
 */
void sar_adc_enable(uint32_t channel, uint32_t enable);

/**
 * @brief  SAR-ADC read
 * @param  NULL
 * @return SAR-ADC value
 */
uint32_t sar_adc_read(void);

uint32_t power_on_sar_adc(void);
/**
 * @brief  TX calibration initialzation
 * @param  NULL
 * @return NULL
 */
void tx_calibration_init(void);

/**
 * @brief  TX calibration de-initialzation
 * @param  NULL
 * @return NULL
 */
void tx_calibration_deinit(void);

/**
 * @brief  TX DC calibration
 * @param  NULL
 * @return NULL
 */
void tx_dc_calibration(void);

/**
 * @brief  TX_Q_CONST_IQCAL_P calibration
 * @param  NULL
 * @return NULL
 */
void tx_q_const_iqcal_p_calibration(void);

/**
 * @brief  TX IQ gain imbalance calibration
 * @param  NULL
 * @return NULL
 */
void tx_iq_gain_imbalance_calibration(void);

/**
 * @brief  TX IQ phase imbalance calibration
 * @param  NULL
 * @return tx_ty2
 */
uint32_t tx_iq_phase_imbalance_calibration(void);

/**
 * @brief  TX output power calibration
 * @param  NULL
 * @return NULL
 */
void tx_output_power_calibration(void);

/**
 * @brief  RX RSSI calibration
 * @param  NULL
 * @return NULL
 */
void rf_IF_filter_cap_calibration(void);
void tx_output_power_calibration_simple(void);


void LPBG_calibration(void);
uint8 write_cali_result();

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__BK3266_CALIBRATION_H__
