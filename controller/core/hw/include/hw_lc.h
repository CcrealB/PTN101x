#ifndef _PARTHUS_HW_LC_
#define _PARTHUS_HW_LC_

/*
 * MODULE NAME:    hw_lc.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Hardware Access Functions
 * MAINTAINER:     John Nelson
 * DATE:           1 Jun 1999
 *
 * SOURCE CONTROL: $Id: hw_lc.h,v 1.89 2016/03/29 12:44:27 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    1 June 1999 -   jn       - Initial Version V0.9
 *
 */


#include "sys_types.h"
#include "sys_config.h"
#include "hw_macro_defs.h"

#include "hw_jalapeno.h"                   /*Dedicated to hardware platform */

void HWlc_Initialise(void);

/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  common control registers
*
****************************************************************/

__INLINE__ void HW_set_bd_addr(const t_bd_addr *);
__INLINE__ void HW_set_bd_addr_via_uap_lap_nap(t_uap_lap uap_lap, t_nap nap);
__INLINE__ void HW_set_uap_lap(t_uap_lap);
__INLINE__ void HW_set_nap(t_nap);
__INLINE__ uint32_t HW_get_uap_lap(void);
__INLINE__ uint32_t HW_get_nap(void);
__INLINE__ void HW_get_bd_addr_Ex(t_bd_addr *);

__INLINE__ void HW_set_sync(const uint32_t *);
__INLINE__ void HW_set_sync_U32(const uint32_t low_word, const uint32_t high_word);
__INLINE__ void HW_get_sync_Ex(uint32_t *);

__INLINE__ void HW_set_intraslot_offset(const uint32_t);
__INLINE__ uint32_t HW_get_intraslot_offset(void);
__INLINE__ uint32_t HW_Get_Intraslot_Avoid_Race(void);

__INLINE__ void HW_set_enc_key(const uint32_t*);
__INLINE__ void HW_get_enc_key_Ex(uint32_t *);

__INLINE__ void HW_set_bt_clk(t_clock bt_clk);
__INLINE__ void HW_set_bt_clk_offset(t_clock bt_clk_offset);
__INLINE__ t_clock HW_get_bt_clk(void);
__INLINE__ t_clock HW_Get_Bt_Clk_Avoid_Race(void);

__INLINE__ void HW_set_native_clk(const t_clock);
__INLINE__ t_clock HW_get_native_clk(void);
DRAM_CODE t_clock HW_Get_Native_Clk_Avoid_Race(void);

__INLINE__ void HW_set_am_addr(const uint32_t);
__INLINE__ uint32_t HW_get_am_addr(void);

__INLINE__ void HW_set_encrypt(const uint32_t);
__INLINE__ uint32_t HW_get_encrypt(void);

__INLINE__ void HW_set_whiten(const uint32_t);
__INLINE__ uint32_t HW_get_whiten(void);

__INLINE__ void HW_set_use_lf(const uint32_t);
__INLINE__ uint32_t HW_get_use_lf(void);

__INLINE__ void HW_set_sleep_status(const uint32_t);
__INLINE__ uint32_t HW_get_sleep_status(void);

__INLINE__ void HW_set_slave(const uint32_t);
__INLINE__ uint32_t HW_get_slave(void);

__INLINE__ void HW_set_page(const uint32_t);
__INLINE__ uint32_t HW_get_page(void);

__INLINE__ void HW_set_sco_cfg0(const uint32_t);
__INLINE__ uint32_t HW_get_sco_cfg0(void);

__INLINE__ void HW_set_sco_cfg1(const uint32_t);
__INLINE__ uint32_t HW_get_sco_cfg1(void);

__INLINE__ void HW_set_sco_cfg2(const uint32_t);
__INLINE__ uint32_t HW_get_sco_cfg2(void);

__INLINE__ void HW_set_sco_fifo(const uint32_t);
__INLINE__ uint32_t HW_get_sco_fifo(void);

__INLINE__ void HW_set_vci_clk_sel( const uint32_t vci_clk );
__INLINE__ uint32_t HW_get_vci_clk_sel(void);

__INLINE__ void HW_set_loop(const uint32_t);
__INLINE__ uint32_t HW_get_loop(void);

__INLINE__ void HW_set_test_eco(const uint32_t);
__INLINE__ uint32_t HW_get_test_eco(void);

__INLINE__ void HW_set_test_crc(const uint32_t);
__INLINE__ uint32_t HW_get_test_crc(void);

__INLINE__ void HW_set_test_hec(const uint32_t);
__INLINE__ uint32_t HW_get_test_hec(void);

__INLINE__ void HW_set_test_radio(const uint32_t);
__INLINE__ uint32_t HW_get_test_radio(void);

__INLINE__ void HW_set_vci_clk_sel_map(const uint32_t);
__INLINE__ uint32_t HW_get_vci_clk_sel_map(void);

__INLINE__ void HW_set_test_msg(const uint32_t);
__INLINE__ uint32_t HW_get_test_msg(void);

__INLINE__ void HW_set_dwh_ini(const uint32_t);
__INLINE__ uint32_t HW_get_dwh_ini(void);

__INLINE__ void HW_set_dwh2_ini(const uint32_t);
__INLINE__ uint32_t HW_get_dwh2_ini(void);

__INLINE__ void HW_set_crc_ini(const uint32_t ini);
__INLINE__ uint32_t HW_get_crc_ini(void);     


/*
 * Interrupt Macros
 */

__INLINE__ void HW_set_pkd_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_pkd_intr_mask(void);

__INLINE__ void  HW_set_pkd_rx_hdr_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_pkd_rx_hdr_intr_mask(void);

__INLINE__ void HW_set_pka_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_pka_intr_mask(void);

__INLINE__ void HW_set_no_pkt_rcvd_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_no_pkt_rcvd_intr_mask(void);

__INLINE__ void HW_set_sync_det_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_sync_det_intr_mask(void);

__INLINE__ void HW_set_tim_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_tim_intr_mask(void);

__INLINE__ void HW_set_aux_tim_intr_mask(const uint32_t);
__INLINE__ uint32_t HW_get_aux_tim_intr_mask(void);

__INLINE__ void HW_set_pkd_intr_clr(const uint32_t);
__INLINE__ void HW_set_pkd_rx_hdr_intr_clr(const uint32_t);
__INLINE__ void HW_set_pka_intr_clr(const uint32_t);
__INLINE__ void HW_set_no_pkt_rcvd_intr_clr(const uint32_t);
__INLINE__ void HW_set_sync_det_intr_clr(const uint32_t);
__INLINE__ void HW_set_tim_intr_clr(const uint32_t);
__INLINE__ void HW_set_aux_tim_intr_clr(const uint32_t);

/*
 * Declare functions for software-only testing
 */
__INLINE__ uint32_t HW_get_pkd_intr_clr(void);
__INLINE__ uint32_t HW_get_pkd_rx_hdr_intr_clr(void);
__INLINE__ uint32_t HW_get_pka_intr_clr(void);
__INLINE__ uint32_t HW_get_no_pkt_rcvd_intr_clr(void);
__INLINE__ uint32_t HW_get_sync_det_intr_clr(void);
__INLINE__ uint32_t HW_get_tim_intr_clr(void);
__INLINE__ uint32_t HW_get_aux_tim_intr_clr(void);


/***************************************************************
 * 
 * Delare the functions used in accessing the Jalapeno
 * Common Status Registers 
 *
 **************************************************************/
__INLINE__ uint32_t HW_get_pkd_intr(void);
__INLINE__ uint32_t HW_get_pkd_rx_hdr_intr(void);
__INLINE__ uint32_t HW_get_pka_intr(void);
__INLINE__ uint32_t HW_get_no_pkt_rcvd_intr(void);
__INLINE__ uint32_t HW_get_sync_det_intr(void);
__INLINE__ uint32_t HW_get_tim_intr(void);
__INLINE__ uint32_t HW_get_aux_tim_intr(void);

/*
 * Declare functions used to set the Common status registers
 * in software-only testing
 *
 */
__INLINE__ void HW_set_pkd_intr(const uint32_t pkd_intr);
__INLINE__ void HW_set_pkd_rx_hdr_intr(const uint32_t pkd_rx_hdr_intr);
__INLINE__ void HW_set_pka_intr(const uint32_t pka_intr);
__INLINE__ void HW_set_no_pkt_rcvd_intr(const uint32_t pka_intr);
__INLINE__ void HW_set_sync_det_intr(const uint32_t pka_intr);
__INLINE__ void HW_set_tim_intr(const uint32_t tim_intr);
__INLINE__ void HW_set_aux_tim_intr(const uint32_t aux_tim_intr);

/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  transmit control registers
*
****************************************************************/
__INLINE__ void HW_set_tx_len(const uint32_t);
__INLINE__ uint32_t HW_get_tx_len(void);

__INLINE__ void HW_set_tx_type(const uint32_t);
__INLINE__ uint32_t HW_get_tx_type(void);

__INLINE__ void HW_set_tx_flow(const uint32_t);
__INLINE__ uint32_t HW_get_tx_flow(void);

__INLINE__ void HW_set_tx_arqn(const uint32_t);
__INLINE__ uint32_t HW_get_tx_arqn(void);

__INLINE__ void HW_set_tx_seqn(const uint32_t);
__INLINE__ uint32_t HW_get_tx_seqn(void);

__INLINE__ void HW_set_tx_buf(const uint32_t);
__INLINE__ uint32_t HW_get_tx_buf(void);

__INLINE__ void HW_set_transmit(const uint32_t);
__INLINE__ uint32_t HW_get_transmit(void);

__INLINE__ void HW_set_tx_half(const uint32_t);
__INLINE__ uint32_t HW_get_tx_half(void);

__INLINE__ void HW_set_tx_p_flow(const uint32_t);
__INLINE__ uint32_t HW_get_tx_p_flow(void);

__INLINE__ void HW_set_tx_l_ch(const uint32_t);
__INLINE__ uint32_t HW_get_tx_l_ch(void);


/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  transmit status registers
*
****************************************************************/
__INLINE__ uint32_t HW_get_tx0_over(void);
__INLINE__ uint32_t HW_get_tx0_under(void);
__INLINE__ uint32_t HW_get_tx1_over(void);
__INLINE__ uint32_t HW_get_tx1_under(void);
__INLINE__ uint32_t HW_get_tx2_over(void);
__INLINE__ uint32_t HW_get_tx2_under(void);


/*
 * Declare functions used for testing the trasmit status
 * registers when testing with software only
 */
__INLINE__ void HW_set_tx0_over(uint32_t  tx0_over);
__INLINE__ void HW_set_tx0_under(uint32_t tx0_under);
__INLINE__ void HW_set_tx1_over(uint32_t  tx1_over);
__INLINE__ void HW_set_tx1_under(uint32_t tx1_under);
__INLINE__ void HW_set_tx2_over(uint32_t  tx2_over);
__INLINE__ void HW_set_tx2_under(uint32_t tx2_under);


/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  receive control registers
*
****************************************************************/
__INLINE__ void HW_set_rx_mode(const uint32_t);
__INLINE__ uint32_t HW_get_rx_mode(void);

__INLINE__ void HW_set_tx_mode(const uint32_t);
__INLINE__ uint32_t HW_get_tx_mode(void);

__INLINE__ void HW_set_abort_on_wrong_am_addr(const uint32_t);
__INLINE__ uint32_t HW_get_abort_on_wrong_am_addr(void);

__INLINE__ void HW_set_rx_buf(const uint32_t);
__INLINE__ uint32_t HW_get_rx_buf(void);

__INLINE__ void HW_set_err_sel(const uint32_t);
__INLINE__ uint32_t HW_get_err_sel(void);

__INLINE__ void HW_set_win_ext(const uint32_t);
__INLINE__ uint32_t HW_get_win_ext(void);

__INLINE__ void   HW_set_freeze_bit_cnt(const uint32_t);
__INLINE__ uint32_t HW_get_win_ext(void);

__INLINE__ void HW_set_freeze_bt_clk(const uint32_t value);
__INLINE__ uint32_t HW_get_freeze_bt_clk(void);


/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  receive status registers
*
****************************************************************/
__INLINE__ uint32_t HW_get_rx_len(void);
__INLINE__ uint32_t HW_get_rx_type(void);
__INLINE__ uint32_t HW_get_rx_flow(void);
__INLINE__ uint32_t HW_get_rx_arqn(void);
__INLINE__ uint32_t HW_get_rx_seqn(void);
__INLINE__ uint32_t HW_get_rx_am_addr(void);
__INLINE__ uint32_t HW_get_rx_l_ch(void);
__INLINE__ uint32_t HW_get_rx_p_flow(void);
__INLINE__ uint32_t HW_get_rx_pkt(void);
__INLINE__ uint32_t HW_get_hec_err(void);
__INLINE__ uint32_t HW_get_crc_err(void);
__INLINE__ uint32_t HW_get_rx_hdr(void);
__INLINE__ uint32_t HW_get_rx0_over(void);
__INLINE__ uint32_t HW_get_rx0_under(void);
__INLINE__ uint32_t HW_get_rx1_over(void);
__INLINE__ uint32_t HW_get_rx1_under(void);
__INLINE__ uint32_t HW_get_rx2_over(void);
__INLINE__ uint32_t HW_get_rx2_under(void);
__INLINE__ uint32_t HW_get_rst_code(void);
__INLINE__ uint32_t HW_get_err_cnt(void);
__INLINE__ uint32_t HW_get_am_addr_abort(void);

/****************************************************************
*
*  Declare the functions used in accessing the Jalapeno
*  eSCO related registers
*
****************************************************************/
__INLINE__ uint32_t HW_get_esco_lt_addr(void);
__INLINE__ uint32_t HW_get_esco_tx_len(void);
__INLINE__ uint32_t HW_get_esco_rx_len(void);
__INLINE__ uint32_t HW_get_sco_route(void);


/*
 * Declare functions used to test receive status accesses
 * in software only conditions
 */
__INLINE__ void HW_set_rx_len(uint32_t rx_len );
__INLINE__ void HW_set_rx_type(uint32_t rx_type );
__INLINE__ void HW_set_rx_flow(uint32_t rx_flow );
__INLINE__ void HW_set_rx_arqn(uint32_t rx_arqn );
__INLINE__ void HW_set_rx_seqn(uint32_t rx_seqn );
__INLINE__ void HW_set_rx_am_addr(uint32_t rx_am_addr );
__INLINE__ void HW_set_rx_l_ch(uint32_t rx_l_ch );
__INLINE__ void HW_set_rx_p_flow(uint32_t rx_p_flow );
__INLINE__ void HW_set_rx_pkt(uint32_t rx_pkt );
__INLINE__ void HW_set_hec_err(uint32_t hec_err );
__INLINE__ void HW_set_crc_err(uint32_t crc_err );
__INLINE__ void HW_set_rx_hdr(uint32_t rx_hdr );
__INLINE__ void HW_set_rx0_over(uint32_t rx0_over );
__INLINE__ void HW_set_rx0_under(uint32_t rx0_under );
__INLINE__ void HW_set_rx1_over(uint32_t rx1_over );
__INLINE__ void HW_set_rx1_under(uint32_t rx1_under );
__INLINE__ void HW_set_rx2_over(uint32_t rx2_over );
__INLINE__ void HW_set_rx2_under(uint32_t rx_2under);
__INLINE__ void HW_set_rst_code(uint32_t rst_code );
__INLINE__ void HW_set_err_cnt(uint32_t err_cnt );

/*
 * eSCO related.
 */
__INLINE__ void HW_set_esco_lt_addr(uint32_t esco_lt_addr);
__INLINE__ void HW_set_esco_rx_len(uint32_t esco_rx_len);


/****************************************************************
 *
 *   Declare the functions used in accessing the Serial Interface
 *   registers (Typically used for RF Control)
 *
 ****************************************************************/
__INLINE__ void HW_set_ser_cfg(const uint32_t);
__INLINE__ uint32_t HW_get_ser_cfg(void);

__INLINE__ void HW_set_ser_data(const uint32_t);
__INLINE__ uint32_t HW_get_ser_data(void);


/****************************************************************
 *
 *   Declare the functions used in accessing the auxilliary timer
 *
 ****************************************************************/
__INLINE__ void HW_set_aux_timer(const uint32_t);
__INLINE__ uint32_t HW_get_aux_timer(void);


/****************************************************************
 *
 *   Declare the functions used in accessing the tx and rx delays
 *
 ****************************************************************/
__INLINE__ void HW_set_tx_delay(const uint32_t);
__INLINE__ uint32_t HW_get_tx_delay(void);

__INLINE__ void HW_set_rx_delay(const uint32_t);
__INLINE__ uint32_t HW_get_rx_delay(void);

/****************************************************************
*
*    Declare the functions used for BT clock control
*
*****************************************************************/
__INLINE__ void HW_set_add_bt_clk_relative(const uint32_t);
__INLINE__ uint32_t HW_get_add_bt_clk_relative(void);

/*
 * Get Hardware Major and Minor revisions
 */
__INLINE__ uint32_t HW_get_minor_revision(void);
__INLINE__ uint32_t HW_get_major_revision(void);

/****************************************************************
*
*    Declare the functions used in resetting jalapeno
*
*****************************************************************/
__INLINE__ void HWjal_Set_Rst_Code(const uint32_t);

/*
 * Declaret he functions used to test jalapeno resetting
 * in software simulations
 */
__INLINE__ uint32_t HWjal_Get_Rst_Code(void);


/****************************************************************
 *
 *   Declare the functions used in accessing ACL and SCO data
 *
 ****************************************************************/
#define HW_get_tx_acl_buf_addr()     (JAL_BASE_ADDR + JAL_TX_ACL_BUF_OFFSET)
#define HW_get_rx_acl_buf_addr()     (JAL_BASE_ADDR + JAL_RX_ACL_BUF_OFFSET)

/*
 * JAL_TX_SCO_BUF_OFFSET and JAL_RX_SCO_BUF_OFFSET are the same as
 * the ACL buffers
 */ 

#define HW_get_tx_sco_buf_addr()     (JAL_BASE_ADDR + JAL_TX_ACL_BUF_OFFSET)
#define HW_get_rx_sco_buf_addr()     (JAL_BASE_ADDR + JAL_RX_ACL_BUF_OFFSET)


/*****************************************************************
 *
 *    Declare the functions used in initialising jalapeno
 *
 *****************************************************************/
void HWlc_Initialise(void);
void HWlc_Reset(void);
__INLINE__ void HW_set_use_hab_crl1(const uint32_t value);

/*****************************************************************
 *
 *    Declare any auxilliary functions
 *
 ******************************************************************/
__INLINE__ void HW_toggle_tx_buf(void);
__INLINE__ void HW_toggle_rx_buf(void);

__INLINE__ uint32_t HW_get_spi_now_conflict(void);
__INLINE__ void HW_set_spi_now_conflict_clr(const uint32_t value);
__INLINE__ void HW_set_ser0_wr_clr(const uint32_t value);

__INLINE__ void HW_set_pta_mode_enable(const uint32_t mode);
__INLINE__ void HW_set_pta_grant_test_enable(const uint32_t mode);

/*****************************************************************
 *
 *    Declare any EDR functions
 *
 ******************************************************************/
#if (PRH_BS_CFG_SYS_LMP_EDR_SUPPORTED==1)
__INLINE__ void HW_Disable_EDR(void);

/****************************************************************
 *   Declare the functions used in accessing the tx and rx delays
 ****************************************************************/
__INLINE__ void HW_set_tx_edr_delay(const uint32_t);
__INLINE__ uint32_t HW_get_tx_edr_delay(void);

__INLINE__ void HW_set_rx_edr_delay(const uint32_t);
__INLINE__ uint32_t HW_get_rx_edr_delay(void);

/*
 *  EDR_Sync_Err
 */
__INLINE__ void HW_set_edr_sync_err(const uint32_t);
__INLINE__ uint32_t HW_get_edr_sync_err(void);

__INLINE__ uint32_t HW_get_native_bit_count(void);

#endif

#if (PRH_BS_CFG_SYS_ESCO_VIA_VCI_SUPPORTED==1)

/****************************************************************
* Register definitions for FIFO interface to CODEC (eSCO via VCI)
****************************************************************/

__INLINE__ uint32_t HW_get_vci_tx_fifo_fill_level(void);
__INLINE__ uint32_t HW_get_vci_rx_fifo_fill_level(void);

__INLINE__ void HW_set_vci_tx_fifo_threshold(const uint32_t value);
__INLINE__ void HW_set_vci_rx_fifo_threshold(const uint32_t value);

__INLINE__ uint32_t HW_get_vci_tx_fill_status(void);
__INLINE__ uint32_t HW_get_vci_rx_fill_status(void);

__INLINE__ void HW_set_vci_rgf_fifo_reset(const uint32_t enable);
__INLINE__ void HW_set_vci_rgf_fifo_16bit_mode(const uint32_t enable);
__INLINE__ void HW_set_vci_rgf_mode_enable(const uint32_t enable);

__INLINE__ void HW_write_vci_rx_fifo_data(const uint32_t data);
__INLINE__ uint32_t HW_read_vci_tx_fifo_data(void);

#endif

#ifdef __USE_INLINES__
#include "hw_lc_impl.h"
#endif

#endif
