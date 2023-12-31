/**
 ****************************************************************************************
 *
 * @file lb.h
 *
 * @brief Main API file for the Link Broadcaster
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 ****************************************************************************************
 */

#ifndef LB_H_
#define LB_H_

/**
 ****************************************************************************************
 * @defgroup LB Link Broadcaster
 * @ingroup ROOT
 * @brief BT Lower Layers
 *
 * The CONTROLLER contains the modules allowing the physical link establishment,
 * maintenance and management.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h"    // Task definitions
#include "ke_task.h"      // kernel task definitions


/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the LB task
/*@TRACE*/
enum lb_msg_id
{
    LB_MSG_ID_FIRST = TASK_FIRST_MSG(TASK_ID_LB),

    /*
     * ************** Msg LC -> LB****************
     */
    LB_MST_KEY_CFM,
    LB_ENC_RESTART_CFM,
    LB_ENC_START_IND,
    LB_ENC_STOP_IND,

    /*
     * ************** Msg LD -> LB****************
     */
    LB_ACL_TX_CFM,
    LB_CSB_TX_END_IND,
    LB_CSB_RX_IND,
    LB_CSB_RX_END_IND,
    LB_AFH_UPDATE_IND,
    LB_AFH_REPORT_TO,
    LB_SSCAN_END_IND,
    LB_STRAIN_END_IND,

    #if PCA_SUPPORT
    LB_LOCAL_PCA_REQ,
    LB_PCA_END_IND,
    LB_PCA_SSCAN_START_REQ,
    #if RW_BT_MWS_COEX
    LB_PCA_MONITOR_INTV_TO,
    #endif //RW_BT_MWS_COEX

    /*
     * ************** Msg LB->LB****************
     */
    LB_PCA_TX_INTV_TO,
    #endif //PCA_SUPPORT
};

/// LB master key confirmation
/*@TRACE*/
struct lb_mst_key_cfm
{
    ///Status
    uint8_t status;
    ///Encryption mode
    uint8_t enc_mode;
    ///Key size mask
    uint16_t key_sz_msk;
}__attribute__((packed));

/// LB encryption restart confirmation
/*@TRACE*/
struct lb_enc_restart_cfm
{
    ///Status
    uint8_t status;
}__attribute__((packed));

/// LB encryption start indication
/*@TRACE*/
struct lb_enc_start_ind
{
    ///Key flag
    uint8_t key_flag;
}__attribute__((packed));

/// LB ACL transmission confirmation structure
/*@TRACE*/
struct lb_acl_tx_cfm
{
    /// EM buffer containing PDU
    uint16_t em_buf;
}__attribute__((packed));

/// LB CSB TX end indication
/*@TRACE*/
struct lb_csb_tx_end_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
}__attribute__((packed));

/// LB CST RX indication
/*@TRACE*/
struct lb_csb_rx_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
    /// CLK when CSB data was received
    uint32_t clk;
    /// (CLKNslave - CLK) modulo 2^28
    uint32_t offset;
    /// receive status
    uint8_t rx_status;
    /// PDU length
    uint16_t length;
    /// PDU (packed)
    uint8_t pdu[__ARRAY_EMPTY];
}__attribute__((packed));

/// LB CSB RX end indication
/*@TRACE*/
struct lb_csb_rx_end_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
}__attribute__((packed));

/// LB CSB AFH update indication
/*@TRACE*/
struct lb_afh_update_ind
{
    /// AFH_Channel_Map
    struct chnl_map    afh_ch_map;
}__attribute__((packed));

/// LB CSB AFH report timeout
/*@TRACE*/
struct lb_afh_report_to
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
}__attribute__((packed));

#if PCA_SUPPORT
/// LB PCA local adjustment request structure
/*@TRACE*/
struct lb_local_pca_req
{
    /// Clock adjusted us
    int16_t clk_adj_us;
    /// Clock adjusted slots
    uint8_t clk_adj_slots;
    /// Clock adjusted period (slots)
    uint8_t clk_adj_period;
};

/// LM PCA adjustment complete indication structure
/*@TRACE*/
struct lb_pca_end_ind
{
    /// Status
    uint8_t status;
};

/// LB Sync Scan start request for PCA recovery mode
/*@TRACE*/
struct lb_pca_sscan_start_req
{
    /// Link identifier
    uint8_t link_id;
    /// Status
    uint8_t status;
};
#endif //PCA_SUPPORT

/// LB Sync Scan end indication
/*@TRACE*/
struct lb_sscan_end_ind
{
    /// Status
    uint8_t            status;
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// Clock_Offset (28 bits) - (CLKNslave - CLK) modulo 2^28
    uint32_t           clock_offset;
    #if PCA_SUPPORT
	/// Bit offset (CLKNslave - CLK) modulo 2^28
    int16_t            bit_offset;
    #endif // PCA_SUPPORT
    /// AFH_Channel_Map
    struct chnl_map    afh_ch_map;
    /// LT_ADDR
    uint8_t            lt_addr;
    /// Next_Broadcast_Instant (28 bits)
    uint32_t next_bcst_instant;
    /// Connectionless_Slave_Broadcast_Interval (in slots)
    uint16_t csb_int;
    /// Service_Data
    uint8_t service_data;
    ///Coarse clock adjust use
    bool coarse_clk_adj;
}__attribute__((packed));

/// LB Sync Train end indication
/*@TRACE*/
struct lb_strain_end_ind
{
    ///Status
    uint8_t status;
    ///Coarse clock adjust use
    bool coarse_clk_adj;
}__attribute__((packed));

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BT LB task
 *
 * This function initializes the the LB task, as well as the environment of the LB
 ****************************************************************************************
 */
void lb_init(void);

/**
 ****************************************************************************************
 * @brief Reset the BT LB task
 ****************************************************************************************
 */
void lb_reset(void);

/**
 ****************************************************************************************
 * @brief Get num broadcast retransmissions
 ****************************************************************************************
 */
uint8_t lb_util_get_nb_broadcast(void);

/**
 ****************************************************************************************
 * @brief Set num broadcast retransmissions
 ****************************************************************************************
 */
void lb_util_set_nb_broadcast(uint8_t num_bcst_ret);

/**
 ****************************************************************************************
 * @brief Get reserved lt_addr
 ****************************************************************************************
 */
uint8_t lb_util_get_res_lt_addr(void);

/**
 ****************************************************************************************
 * @brief Get CSB mode (active/inactive)
 ****************************************************************************************
 */
bool lb_util_get_csb_mode(void);

#if PCA_SUPPORT
/**
 ****************************************************************************************
 * @brief Generic handler for a PCA request on a Master device
 *
 * @param[in] clk_adj_us. Request intraslot adjustment
 * @param[in] clk_adj_slots. Request interslot adjustment
 * @param[in] clk_adj_period. Period associated with interslot adjustment
 * @param[in] tr_id. Transaction ID of the request.
 * @return status as a result of processing the request.
 ****************************************************************************************
 */
uint8_t lb_master_clk_adj_req_handler(int16_t clk_adj_us, uint8_t clk_adj_slots, uint8_t clk_adj_period, uint8_t tr_id);

/**
 ****************************************************************************************
 * @brief This function is used to clear a PCA clk_adj_ack pending flag/count.
 *
 ****************************************************************************************
 */
void lb_clk_adj_ack(uint8_t clk_adj_id, uint8_t link_id);

#endif //PCA_SUPPORT

/**
 ****************************************************************************************
 * @brief This function read the current master key.
 *
 * @param[out] MasterKey       Master key with LTK size
 ****************************************************************************************
 */
void LM_GetMasterKey(struct ltk *MasterKey);

/**
 ****************************************************************************************
 * @brief This function read the current master key random.
 *
 * @param[out] MasterRand      Master key random
 ****************************************************************************************
 */
void LM_GetMasterKeyRand(struct ltk *MasterRand);

/**
 ****************************************************************************************
 * @brief This function read the current Random vector for broadcast encryption.
 *
 * @param[out] MasterRand          Random Vector
 ****************************************************************************************
 */
void LM_GetMasterEncRand(struct ltk *MasterRand);

/**
 ****************************************************************************************
 * @brief This function read Broadcast Encryption Key Size.
 *
 * @return Encryption Key Size
 ****************************************************************************************
 */
uint8_t LM_GetMasterEncKeySize(void);

/// @} LB

#endif // LB_H_
