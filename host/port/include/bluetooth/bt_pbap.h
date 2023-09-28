/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */


#ifndef __BT_PBAP_PCE_H__
#define __BT_PBAP_PCE_H__
#if CONFIG_BLUETOOTH_PBAP
/***************************************************************
 * Bluetooth Phonebook Access Profile Client Side API *
 ***************************************************************/

#include "bt_at_types.h"
#include <bt_api.h>

#define PBAP_DOWNLOAD                       (1 << 0)
#define PBAP_BROWSING                       (1 << 1)
#define PBAP_DATABASE_IDENFITY              (1 << 2)
#define PBAP_FOLDER_VERSION                 (1 << 3)
#define PBAP_VCARD_SELECTING                (1 << 4)
#define PBAP_ENHANCED_MISSED_CALLS          (1 << 5)
#define PBAP_X_BT_UCI_VCARD_PROPERTY        (1 << 6)
#define PBAP_X_BT_UID_VCARD_PROPERTY        (1 << 7)
#define PBAP_CONTACT_REFERENCING            (1 << 8)
#define PBAP_DEFAULT_CONTACT_IMAGE_FORMAT   (1 << 9)


#define PBAP_SUPPORTED_FEATURES (PBAP_BROWSING | PBAP_DOWNLOAD)



/* Handle to a Bluetooth session object */
typedef struct bt_pbap_session_s *bt_pbap_session_h;
/* Handle to a local Bluetooth server */
typedef struct bt_pbap_session_s *bt_pbap_server_h;

/*!
	@brief Valid phonebook repositories.
*/
typedef enum
{
	pbap_current,
	pbap_local,
	pbap_sim1,
	
	pbap_r_unknown
} pbap_phone_repository;

/*!
	@brief Valid phonebook folders.
*/
typedef enum
{
	pbap_telecom,
	pbap_pb,
	pbap_ich,
	pbap_och,
	pbap_mch,
	pbap_cch,
	pbap_spd,
	pbap_fav,
	
	pbap_b_unknown
} pbap_phone_book;

/*!
	@brief Order values for use with the PullvCardListing function.
*/
typedef enum 
{
	/*! Indexed. */
	pbap_order_idx = 0x00,
	/*! Alphanumeric */
	pbap_order_alpha = 0x01, 
	/*! Phonetic */
	pbap_order_phone = 0x02
} pbap_order_values;

/*!
	@brief Search Attributes to use with the PullvCardListing function.
*/
typedef enum 
{
	/*! Name. */
	pbap_search_name = 0x00,
	/*! Number */
	pbap_search_number = 0x01, 
	/*! Sound */
	pbap_search_sound = 0x02
} pbap_search_values;

/*!
	@brief vCard formats.
*/
typedef enum 
{
	/*! vCard 2.1. */
	pbap_format_21 = 0x00,
	/*! vCard 3.0 */
	pbap_format_30 = 0x01
} pbap_format_values;

/*!
	@brief Phonebook Access Profile Specific Parameters.
*/
typedef enum 
{
	/*! Order. */
	pbap_param_order = 0x01,
	/*! Search Value */
	pbap_param_srch_val = 0x02, 
	/*! Search Atribute */
	pbap_param_srch_attr = 0x03, 
	/*! Maximum List Count */
	pbap_param_max_list = 0x04, 
	/*! List Start Offset */
	pbap_param_strt_offset = 0x05, 
	/*! Filter */
	pbap_param_filter = 0x06, 
	/*! Format */
	pbap_param_format = 0x07, 
	/*! Phonebook Size */
	pbap_param_pbook_size = 0x08, 
	/*! New Missed Calls */
	pbap_param_missed_calls = 0x09
} pbap_goep_parameters;

/* Currently running PBAP Client Command */
typedef enum
{
    pbapc_com_None,
    pbapc_com_RegSdp,
    pbapc_com_Connect,
    pbapc_com_ObexPassword,
    pbapc_com_Disconnect,
    pbapc_com_SetPhonebook, //5
    pbapc_com_PullvCardList,//6
    pbapc_com_PullvCard, //7
    pbapc_com_PullPhonebook, //8
    pbapc_com_GetProperties,
    pbapc_com_EOL
} pbapc_running_command;

/**************************
 * Bluetooth Connectivity *
 **************************/

/**
 * Function type:  bt_pbap_connected_cb_t
 * Description:    Notifies that a Bluetooth connection with a remote device
 *                 has been established.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *     @app_ctx_h: (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_pbap_rfcomm_connected_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h);

typedef void (*bt_pbap_obex_connected_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, result_t result);

typedef void (*bt_pbap_set_path_cfm_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, result_t result);

typedef void (*bt_pbap_vcard_list_start_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_vcard_list_data_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_vcard_list_complete_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, result_t result);

typedef void (*bt_pbap_vcard_entry_start_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_vcard_entry_data_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_vcard_entry_complete_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, result_t result);

typedef void (*bt_pbap_pb_start_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_pb_data_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);

typedef void (*bt_pbap_pb_complete_cb_t)(
    bt_pbap_session_h session_h, bt_app_ctx_h app_ctx_h, result_t result);

/**
 * Function type:  bt_pbap_disconnected_cb_t
 * Description:    Notifies that a Bluetooth connection with a remote device
 *                 has been disconnected, or that a previous connection attempt
 *                 has failed.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *     @status:    (IN) Disconnect status
 *     @app_ctx_h: (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_pbap_disconnected_cb_t)(
    bt_pbap_session_h session_h, result_t status, bt_app_ctx_h app_ctx_h);

/* Bluetooth connection-status application notification callback functions */
typedef struct {
    bt_pbap_rfcomm_connected_cb_t		rfcomm_connected_cb;
    bt_pbap_obex_connected_cb_t			obex_connected_cb;
	bt_pbap_set_path_cfm_cb_t			set_path_cfm_cb;
	bt_pbap_vcard_list_start_cb_t		vcard_list_start_cb;
	bt_pbap_vcard_list_data_cb_t		vcard_list_data_cb;
	bt_pbap_vcard_list_complete_cb_t	vcard_list_complete_cb;
	bt_pbap_vcard_entry_start_cb_t		vcard_entry_start_cb;
	bt_pbap_vcard_entry_data_cb_t		vcard_entry_data_cb;
	bt_pbap_vcard_entry_complete_cb_t	vcard_entry_complete_cb;
	bt_pbap_pb_start_cb_t				phonebook_start_cb;
	bt_pbap_pb_data_cb_t				phonebook_data_cb;
	bt_pbap_pb_complete_cb_t			phonebook_complete_cb;
    bt_pbap_disconnected_cb_t			disconnected_cb;
} bt_pbap_conn_cbs_t;

/**
 * Function name:  bt_pbap_conn_create
 * Description:    Creates a new Bluetooth session object, and registers the
 *                 application's Bluetooth connectivity callback functions for
 *                 the related session with the driver.
 *                 NOTE: The application must call this function before calling
 *                 bt_pbap_conn_connect() to initiate a new connection, or
 *                 bt_pbap_conn_accept() to accept a connection request.
 *                 The application is responsible for destroying the session
 *                 object, by calling bt_pbap_conn_destroy().
 * Parameters:
 *     @session_h: (OUT) Pointer to a handle to a Bluetooth session object, to
 *                       bet set by the function
 *     @conn_cbs:  (IN)  Pointer to a structure of Bluetooth connection-status
 *                       application notification callback functions
 *     @app_ctx_h: (IN)  Handle to a Bluetooth application context
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_conn_create(bt_pbap_session_h *session_h,
    bt_pbap_conn_cbs_t *conn_cbs, bt_app_ctx_h app_ctx_h);

/**
 * Function name:  bt_pbap_conn_destroy
 * Description:    Destroys a Bluetooth session object.
 *                 NOTE: The application must call this function to destroy the
 *                 session handle for a disconnected or unestablished
 *                 connection.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *
 * Return value:   None
 * Scope:          Global
 **/
void bt_pbap_conn_destroy(bt_pbap_session_h *session_h);

/**
 * Function name:  bt_pbap_conn_disconnect
 * Description:    Initiates a disconnect of a Bluetooth connection with a
 *                 remote device.
 *                 NOTE: The driver will respond by calling the application's
 *                 bt_pbap_disconnected_cb_t callback.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_conn_disconnect(bt_pbap_session_h session_h);

/*--------------------------------
   Client-Specific Connection API
  --------------------------------*/

/**
 * Function name:  bt_pbap_conn_connect
 * Description:    Issues a Bluetooth connection request to a remote device.
 *                 NOTE: The driver will respond by calling the application's
 *                 bt_pbap_connected_cb_t callback if a connection is
 *                 established, or the bt_pbap_disconnected_cb_t callback
 *                 otherwise.
 * Parameters:
 *     @session_h:      (OUT) Pointer to a handle to a Bluetooth session object,
 *                            to be set by the function
 *     @laddr:          (IN)  Pointer to a local Bluetooth address
 *     @raddr:          (IN)  Pointer to a remote Bluetooth address
 *     @rfcomm_channel: (IN)  RFCOMM channel
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_conn_connect(bt_pbap_session_h session_h,
    const btaddr_t *laddr, const btaddr_t *raddr, uint8_t channel, uint8_t action);

/*--------------------------------
   Server-Specific Connection API
  --------------------------------*/

/**
 * Function type:  bt_pbap_newconn_cb_t
 * Description:    Notifies of a request from a remote Bluetooth device to
 *                 connect to the local Bluetooth server.
 *                 NOTE: The application must either accept the request by
 *                 calling bt_pbap_conn_accept(), or reject it by calling
 *                 bt_pbap_conn_reject().
 * Parameters:
 *     @conn_req_h: (IN) Handle to a Bluetooth connection request
 *     @laddr:      (IN) Pointer to a local Bluetooth address
 *     @raddr:      (IN) Pointer to a remote Bluetooth address
 *     @mode:       (IN) Bluetooth link modes mask -- see bt_link_modes_mask_t
 *     @app_ctx_h:  (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_pbap_newconn_cb_t)(bt_conn_req_h conn_req_h,
    const btaddr_t *laddr, const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_ctx_h);

/**
 * Function name:  bt_pbap_server_start
 * Description:    Starts a local Bluetooth server.
 * Parameters:
 *     @server_h:       (OUT) Pointer to a handle to a local Bluetooth server,
 *                            to be set by the function
 *     @laddr:          (IN)  Pointer to a local Bluetooth address
 *     @rfcomm_channel: (IN)  RFCOMM channel
 *     @newconn_cb:     (IN)  Remote Bluetooth connection request notification
 *                            callback -- see bt_pbap_newconn_cb_t
 *     @app_ctx_h:      (IN)  Handle to a Bluetooth application context
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_server_start(bt_pbap_server_h *server_h,
    const btaddr_t *laddr, uint8_t rfcomm_channel,
    bt_pbap_newconn_cb_t newconn_cb, bt_app_ctx_h app_ctx_h);

/**
 * Function name:  bt_pbap_server_stop
 * Description:    Stops a local Bluetooth server.
 * Parameters:
 *     @server_h: (IN) Handle to a local Bluetooth server
 *
 * Return value:   None
 * Scope:          Global
 **/
void bt_pbap_server_stop(bt_pbap_server_h *server_h);

/**
 * Function name:  bt_pbap_conn_accept
 * Description:    Accepts a Bluetooth connection request from a remote device.
 *                 NOTE: Before calling this function, the application must
 *                 call bt_pbap_conn_create().
 *                 The driver will respond by calling the application's
 *                 bt_pbap_connected_cb_t callback -- if a connection
 *                 was established -- or its bt_pbap_disconnected_cb_t
 *                 callback otherwise.
 * Parameters:
 *     @session_h:  (OUT) Pointer to a handle to a Bluetooth session object, to
 *                        bet set by the function
 *     @conn_req_h: (IN) Handle to a Bluetooth connection request
 *     @mode:       (IN) Bluetooth link modes mask -- see bt_link_modes_mask_t
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_conn_accept(bt_pbap_session_h session_h,
    bt_conn_req_h conn_req_h, bt_link_modes_mask_t mode);

/**
 * Function name:  bt_pbap_conn_reject
 * Description:    Rejects a Bluetooth connection request from a remote device.
 * Parameters:
 *     @conn_req_h: (IN) Handle to a Bluetooth connection request
 *
 * Return value:   None
 * Scope:          Global
 **/
void bt_pbap_conn_reject(bt_conn_req_h conn_req_h);

/*********************************************
 * PBAP Client Side Role-Specific Functionality
 *********************************************/

/**
 * Function type:  bt_pbap_get_features_cb_t
 * Description:    Retrieves PBAP supported features.
 *                 The driver calls this function as part of the process of
 *                 establishing a Bluetooth connection.
 * Parameters:
 *     @session:            (IN)  Handle to a Bluetooth session object
 *     @supported_features: (OUT) Pointer to a bitmask of supported features
 *                                available in the PBAP
 *     @app_ctx_h:          (IN)  Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_pbap_get_features_cb_t)(bt_pbap_session_h session_h,
    uint16_t *supported_features, bt_app_ctx_h app_ctx);

/**
 * Function type:  bt_pbap_at_result_cb_t
 * Description:    Notifies that a Bluetooth connection with a remote device
 *                 has been established.
 * Parameters:
 *     @session:     (IN) Handle to a Bluetooth session object
 *     @at_result_t: (IN) Pointer to an AT result structure
 *     @app_ctx_h:   (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_pbap_at_result_cb_t)(bt_pbap_session_h session_h,
    at_result_t *at_result, bt_app_ctx_h app_ctx);

/* PBAP PCE role-specific application callback functions */
typedef struct {
    bt_pbap_at_result_cb_t            at_result_cb;
    bt_pbap_get_features_cb_t         get_features_cb;
} bt_pbap_cbs_t;

/**
 * Function name:  bt_pbap_register
 * Description:    Registers the application to use the PBAP PCE driver's
 *                 role-specific APIs.
 * Parameters:
 *     @cbs: (IN)  Pointer to a structure of PBAP PCE role-specific application
 *                 callback functions
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t bt_pbap_register(const bt_pbap_cbs_t *cbs);

/**
 * Function name:  bt_pbap_unregister
 * Description:    Unregisters the application from using the PBAP PCE driver's
 *                 role-specific APIs.
 * Parameters:     None
 *
 * Return value:   None
 * Scope:          Global
 **/
void bt_pbap_unregister(void);

result_t pbap_cmd_disconn(btaddr_t *raddr);



result_t bt_pbap_obex_abort(bt_pbap_session_h session_h);
result_t bt_pbap_obex_disconnect(bt_pbap_session_h session_h);
result_t bt_pbap_go_to_root(bt_pbap_session_h session_h, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath);
result_t bt_pbap_set_path(bt_pbap_session_h session_h, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath);
result_t bt_pbap_pull_phonebook_start(bt_pbap_session_h session_h, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath, uint16_t start, uint16_t end, pbap_format_values pbFormat);
result_t bt_pbap_pull_vcard_listing_start(bt_pbap_session_h session_h, uint16_t maxList, uint16_t listStart, uint8_t orderValue, uint8_t search_att);
result_t bt_pbap_pull_target_vcard(bt_pbap_session_h session_h, uint8_t phone_num_len, uint8_t* phone_num_str);
result_t bt_pbap_pull_vcard_entry_start(bt_pbap_session_h session_h, uint32_t entry, uint32_t filter_lo, uint32_t filter_hi, pbap_format_values pbFormat);
result_t bt_pbap_obex_get(bt_pbap_session_h session, struct mbuf *m);

//result_t pbap_slc_disconnect(void);
//result_t pbap_slc_connect(btaddr_t *raddr, uint8_t action);
result_t pbap_connect_rfcomm(btaddr_t raddr, uint8_t channel);
uint8_t is_PBAP_ready();
uint8_t go_back_to_root(bt_pbap_session_h session);

result_t pbap_pull_phonebook(btaddr_t *raddr, uint16_t start, 
    uint16_t end, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath);
result_t pbap_pull_vcard_list(btaddr_t *raddr, uint16_t start_index, uint16_t list_length, uint8_t orderValue, uint8_t search_att);
result_t pbap_pull_vcard_entry(btaddr_t *raddr, uint32_t index);
result_t pbap_set_path(btaddr_t *raddr, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath);
result_t pbap_pull_target_vcard(btaddr_t *raddr, uint8_t phone_num_len, uint8_t* phone_num_str);




void set_remote_supported_features(btaddr_t* raddr, uint32_t features);
void set_remote_supported_repositories(btaddr_t* raddr, uint8_t repositories);
#endif
#endif

