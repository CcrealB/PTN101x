/* (c) 2012 Beken Ltd. All Rights Reserved. Beken Confidential */

#ifndef __BT_GOEP_H__
#define __BT_GOEP_H__

/***************************************************************
 * Bluetooth Generic Object Exchange Profile (GOEP) API *
 ***************************************************************/

//#include "bt_at_types.h"
#include <bt_api.h>
//temp mask
//#include <bt_opps.h>
#include <bt_rfcomm_proto.h>
#if CONFIG_BLUETOOTH_PBAP
/* The supported OBEX version (1.0) */
#define GOEP_OBEX_VER		0x10
/* The maximum length of the target header */
#define MAX_TARGET_LEN		32
/* The maximum length of the OBEX packet */
#define MAX_PACKET_LEN		512

#define MAX_NAME_LEN	64
#define MAX_TEL_LEN		32

#define GOEP_SIZE_DIGEST 16
#define GOEP_SIZE_MD5_STRING 32

#define GOEP_AUTH_REQ_NONCE 0x00
#define GOEP_AUTH_REQ_OPTIONS 0x01
#define GOEP_AUTH_REQ_REALM 0x02

/* Find uint8_t Field */
#define goepHdrFindUint8Field(b, p) ((b)[(p)])

/* Find uint16_t Field */
#define goepHdrFindUint16Field(b, p) ( ((b)[(p)]<<8) + ((b)[(p)+1]) )

typedef enum
{
	goep_unknown,
	goep_initialising,
	goep_initialised,
	goep_connecting,
	goep_connected,
	goep_pushing,
	goep_pushing_last,
	goep_deleting,
	goep_pulling_first, //8
	goep_pulling, //9
	goep_remote_put,
	goep_remote_get,
	goep_aborting,
	goep_setpath,
	goep_disconnecting, //e
	goep_rem_disconnecting,
	goep_connect_abort,
	goep_connect_refused,
	goep_connect_auth,
	goep_connect_cancel,
    
	goep_st_end_of_list
} goep_states;

/*
 * GOEP session
 */

typedef struct bt_goep_session_s {
	struct mbuf		*g_mbuf;
	uint16_t		firstOffset;
	uint16_t		pktLen;
	rfcomm_dlc_h	rfcomm_so;

	goep_states		state;
	goep_states		abort_state;
		
	uint16_t		conLen;
	char*			conInfo; /* connection info. */
	uint32_t		conID;

	BOOL			useHeaders;
	BOOL			useConID;
} bt_goep_session_t;

/* Handle to a Bluetooth session object */
typedef struct bt_goep_session_s bt_goep_session_h;

enum 
{
	/*! Last operation was successful.*/
	goep_success,			   
	/*! Last operation failed.*/
	goep_failure,			   
	/*! Client is the wrong state for this command. */
    goep_invalid_command,      
	/*! Parameters sent in the command are invalid. */
    goep_invalid_parameters,   
	/*! Connection has failed due to being unauthorised. */
    goep_connect_unauthorised,           
	/*! Remote Host has aborted our current multi-packet command. */
    goep_host_abort,           
	/*! Local Client has aborted our current multi-packet command. */
    goep_local_abort,          
	/*! Remote Client has disconnected */
	goep_remote_disconnect,	   
	/*! Server doesn't support this type of GOEP. */
	goep_server_unsupported,   
	/*! Server has accepted the connection on a service that wasn't
	  requested. */
	goep_server_invalid_serv,  
	/*! Server has refused the connection */
    goep_connection_refused,   
    /*! Folder not found and GOEP_PATH_NOCREATE specified */
    goep_setpath_notfound,     
	/*! Not authorised to access the requested folder */
    goep_setpath_unauthorised, 
    /*! Server couldn't understand the request. */
    goep_get_badrequest,       
	/*! Server reported that the request was understood but forbidden. */
    goep_get_forbidden,        
	/*! Requested file was not found. */
    goep_get_notfound,         	
	goep_transport_failure,
	/*! Connect was cancelled by the client during authentication */
	goep_connect_cancelled,
    /*! Local Client has disconnected due to linkloss */
	goep_remote_disconnect_linkloss,
	goep_end_of_status_list
};

enum
{
	goep_Rsp_Continue		= 0x90,
	goep_Rsp_Success		= 0xA0,
	
	goep_Rsp_Unauthorised	= 0x41,
	
	goep_Rsp_BadRequest		= 0xC0,
	goep_Rsp_UnauthComp		= 0xC1,
	goep_Rsp_Forbidden		= 0xC3,
	goep_Rsp_NotFound		= 0xC4,
        goep_Rsp_NotAccept          = 0xC6,
	goep_Rsp_PreConFail		= 0xCC,
	goep_Rsp_NotImplemented = 0xD1,
	goep_Rsp_ServUnavail	= 0xD3,

	goep_Rsp_EOL
}; /* Packet Response Codes */

enum
{
	goep_Pkt_Connect		= 0x80,
	goep_Pkt_Disconnect		= 0x81,
	
	goep_Pkt_Put			= 0x02,
	goep_Pkt_PutLast		= 0x82,
	
	goep_Pkt_Get			= 0x03,
	goep_Pkt_GetNext		= 0x83,
	
	goep_Pkt_SetPath		= 0x85,
	goep_Pkt_Abort			= 0xFF,

	goep_Pkt_EOL
}; /* Packet Types */

enum
{
	goep_Hdr_Name			= 0x01,
	goep_Hdr_Type			= 0x42,
	goep_Hdr_Who			= 0x4a,
	goep_Hdr_Length			= 0xc3,
	goep_Hdr_Target			= 0x46,
	goep_Hdr_Body			= 0x48,
	goep_Hdr_EndOfBody		= 0x49,
	goep_Hdr_ConnID			= 0xcb,
	goep_Hdr_AuthChallenge	= 0x4d,
	goep_Hdr_AuthResponse	= 0x4e,
	goep_Hdr_AppSpecific	= 0x4c,

	goep_Hdr_EOL
}; /* Header Types */

/*!
	@brief Response codes sent back to a remote client from a local server
*/
typedef enum
{
	goep_svr_resp_OK,
	goep_svr_resp_Continue,
			
	goep_svr_resp_BadRequest,
	goep_svr_resp_Forbidden,
	goep_svr_resp_Unauthorized,
	goep_svr_resp_NotFound,
	/*! Pre condition failed */
	goep_svr_resp_PreConFail,
	goep_svr_resp_ServUnavailable,
	
	goep_svr_resp_end_of_list
} goep_svr_resp_codes;

BOOL handleServerCommand(struct mbuf *m);

/* Setup Header Fields (Packet type and packet length) */
BOOL goepHdrSetUpHeader(bt_goep_session_h *state, struct mbuf *buf, const uint8_t pcktType, uint16_t *length);

/* Update Length Field */
void goepHdrSetPacketLen(bt_goep_session_h *state, struct mbuf *buf, const uint16_t length);
		
/* Add uint8_t field */
BOOL goepHdrAddUint8Field(bt_goep_session_h *state, struct mbuf *buf, const uint8_t val, uint16_t *length);

/* add uint16_t field */
BOOL goepHdrAddUint16Field(bt_goep_session_h *state, struct mbuf *buf, const uint16_t val, uint16_t *length);

/* Add uint32 header */
BOOL goepHdrAddUint32Header(bt_goep_session_h *state, struct mbuf *buf, const uint8_t type, const uint32_t val, uint16_t *length);

/* add Header */
BOOL goepHdrAddStringHeader(bt_goep_session_h *state, struct mbuf *buf, const uint8_t type, const uint16_t hdrLen, const char* data, uint16_t *length);

/* Add header info for application specific parameters
*/
void goepHdrAddAppSpecHeader(bt_goep_session_h *state, struct mbuf *buf, uint16_t *length);

/* Add Header without any body (just the header section) */
BOOL goepHdrAddEmptyHeader(bt_goep_session_h *state, struct mbuf *buf, const uint8_t type, const uint16_t hdrLen, uint16_t *length);

/* Update header length for application specific parameters
 	Offset is where the application started adding fields.  As returned by goepHdrAddAppSpecHeader.
	Returns the total length of the packet
*/
uint16_t goepHdrUpdateAppSpecLength(bt_goep_session_h *state, uint16_t length);

/* Find uint32 Header
    Returns an offset into the buffer [0 offset and length on error]
*/
uint16_t goepHdrFindUint32Header(const uint8_t* buffer, const uint16_t start, const uint16_t stop, const uint8_t type);

/* Find String Header
    Returns an offset into the buffer [0 offset and length on error]
*/
uint16_t goepHdrFindStringHeader(const uint8_t* buffer, const uint16_t start, const uint16_t stop, const uint8_t type, uint16_t *length);

/* Does the packet contain a body header
        Either a BODY or END_OF_BODY
*/
BOOL goepHdrContainBody(const uint8_t* buffer, const uint16_t start, const uint16_t stop);


/* Return BODY or end of body header if found
*/
BOOL checkifgoepHdrContainBody(const uint8_t* buffer, const uint16_t start, const uint16_t stop,uint16_t type);

result_t goepSendConnectResponse(bt_goep_session_h *state, uint16_t resp, const uint8_t *nonce, uint8_t options, uint16_t size_realm, const uint8_t *realm);
#endif
#endif

