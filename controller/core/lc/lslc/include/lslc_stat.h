#ifndef _PARTHUS_LSLC_STAT_
#define _PARTHUS_LSLC_STAT_

/******************************************************************************
 * MODULE NAME:    lslc_stat.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    LSLC_Status
 * MAINTAINER:     John Nelson
 * DATE:           27 May 1999
 *
 * SOURCE CONTROL: $Id: lslc_stat.h,v 1.35 2013/10/25 16:46:01 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    27 May 1999 -   jn       - Initial Version V0.9
 *    11 Nov 1999 -   jn       - Packet Log Added
 *
 ******************************************************************************/

#include "lc_types.h"

/*
 * Monitors ADT: Monitors for number of Rx/Tx Packets
 */
typedef struct _LLCstatusMonitors
   {
   uint32_t RxValidPackets;
   uint32_t RxInvalidPackets;

   uint32_t RxNoPacketCntr;
   uint32_t RxHECErrorCntr;
   uint32_t RxCRCErrorCntr;
   uint32_t RxDuplicateCntr;

#if (PRH_BS_DBG_LC_LSLC_STAT==1)
   uint32_t RxRestrictedCntr;
   uint32_t RxAMADDRErrorCntr;

   uint16_t TxPacketCntr[MAX_PACKET_TYPES];
   uint16_t RxPacketCntr[MAX_PACKET_TYPES];

   uint32_t TxBroadcastCntr;
   uint32_t RxBroadcastCntr;
   uint32_t RxBroadcastDuplicateCntr;

   uint32_t TxAckSentCntr;               /* Counts ARQN=ACK sent             */
   uint32_t RxAckProcessedCntr;          /* Counts ACKs processed            */ 
#if ((DEBUG_BASEBAND_MONITORS == 1) && (DEBUG_ESCO_LINK_PKT_LOST == 1))
   uint32_t RxEscoPktCrcErrCntr;
   uint32_t RxEscoPktLostCntr;
#endif
#endif

} t_LLCmonitors;

/*
 * Expose for Macros Below
 */
extern t_LLCmonitors monitors;

/*
 * Monitors: Public Modifier Methods
 */
void LSLCstat_Reset_Monitors(void);
t_LLCmonitors *LSLCstat_Read_and_Reset_Monitors(t_LLCmonitors *pMonitors);

#define LSLCstat_Inc_Valid_Packet_Cntr()        (monitors.RxValidPackets++)
#define LSLCstat_Inc_Invalid_Packet_Cntr()      (monitors.RxInvalidPackets++)
#define LSLCstat_Inc_Rx_No_Packet_Cntr()        (monitors.RxNoPacketCntr++)
#define LSLCstat_Inc_HEC_Error_Cntr()           (monitors.RxHECErrorCntr++)
#define LSLCstat_Inc_CRC_Error_Cntr()           (monitors.RxCRCErrorCntr++)
#define LSLCstat_Inc_Rx_Duplicate_Cntr()        (monitors.RxDuplicateCntr++)

#if (PRH_BS_DBG_LC_LSLC_STAT==1)
#define LSLCstat_Inc_Rx_Restricted_Packet_Cntr()(monitors.RxRestrictedCntr++)
#define LSLCstat_Inc_AMADDR_Error_Cntr()        (monitors.RxAMADDRErrorCntr++)

#define LSLCstat_Inc_Tx_Packet_Cntr(pkt)        (monitors.TxPacketCntr[pkt]++)
#define LSLCstat_Dec_Tx_Packet_Cntr(pkt)        (monitors.TxPacketCntr[pkt]--)
#define LSLCstat_Inc_Rx_Packet_Cntr(pkt)        (monitors.RxPacketCntr[pkt]++)

#define LSLCstat_Inc_Tx_Broadcast_Packet_Cntr() (monitors.TxBroadcastCntr++)
#define LSLCstat_Inc_Rx_Broadcast_Packet_Cntr() (monitors.RxBroadcastCntr++)
#define LSLCstat_Inc_Broadcast_Duplicate_Cntr() (monitors.RxBroadcastDuplicateCntr++)

#define LSLCstat_Inc_Tx_Ack_Sent_Cntr()         (monitors.TxAckSentCntr++)
#define LSLCstat_Inc_Rx_Ack_Processed_Cntr()    (monitors.RxAckProcessedCntr++)
#if ((DEBUG_BASEBAND_MONITORS == 1) && (DEBUG_ESCO_LINK_PKT_LOST == 1))
#define LSLCstat_Inc_Rx_ESCO_CrcErr_Cntr()      (monitors.RxEscoPktCrcErrCntr++)    
#define LSLCstat_Inc_Rx_ESCO_Lost_Cntr()        (monitors.RxEscoPktLostCntr++)    
#endif

#else
#define LSLCstat_Inc_Rx_Restricted_Packet_Cntr()
#define LSLCstat_Inc_AMADDR_Error_Cntr() 

#define LSLCstat_Inc_Tx_Packet_Cntr(pkt) 
#define LSLCstat_Dec_Tx_Packet_Cntr(pkt)        
#define LSLCstat_Inc_Rx_Packet_Cntr(pkt)        

#define LSLCstat_Inc_Tx_Broadcast_Packet_Cntr() 
#define LSLCstat_Inc_Rx_Broadcast_Packet_Cntr() 
#define LSLCstat_Inc_Broadcast_Duplicate_Cntr()        

#define LSLCstat_Inc_Tx_Ack_Sent_Cntr()         
#define LSLCstat_Inc_Rx_Ack_Processed_Cntr()  

#endif

/*
 * Monitors: Public Accessor Methods
 */
t_LLCmonitors* LSLCstat_Read_Monitors(t_LLCmonitors *pMon);
#define LSLCstat_Get_Monitors()     (&monitors)

/*
 * Packet Log ADT: Packet Log for Rx/Tx Packets
 */

/*
 * Different Log of Packets Types
 */
#define LOG_ALL    1
#define LOG_VALID  2
#define LOG_WRAP   4
#define LOG_NOWRAP 8
#define LOG_PRINT  16


#define PACKETLOG_NONE 0 
#define PACKETLOG_ALL_WRAP (LOG_ALL   | LOG_WRAP)
#define PACKETLOG_VALID_WRAP (LOG_VALID | LOG_WRAP) 
#define PACKETLOG_ALL_NOWRAP (LOG_ALL   | LOG_NOWRAP) 
#define PACKETLOG_VALID_NOWRAP (LOG_VALID | LOG_NOWRAP)
#define PACKETLOG_ALL_WRAP_PRINT ( LOG_ALL   | LOG_WRAP   | LOG_PRINT)
#define PACKETLOG_VALID_WRAP_PRINT ( LOG_VALID | LOG_WRAP   | LOG_PRINT) 
#define PACKETLOG_ALL_NOWRAP_PRINT ( LOG_ALL   | LOG_NOWRAP | LOG_PRINT) 
#define PACKETLOG_VALID_NOWRAP_PRINT ( LOG_VALID | LOG_NOWRAP | LOG_PRINT)

typedef uint32_t t_LogPacketTypes;

typedef enum { TX_PKT=0, RX_PKT=1 } _t_txrx;

typedef struct _LLCstatusPacket
   {
/*
 * Display problems with uint8_t in Armulator etc hence use uint32_t
 */
   t_packet  packet_type;           /* t_packet                         */

   uint32_t  frequency;              /* Hop Channel(s) (Frequency)       */
   uint32_t  frequency_clock;        /* Time used for Hop generation     */

   t_TXRXstatus status;             /* t_TXRXstatus                     */
   _t_txrx  tx0_rx1;                /* Indicate if Tx or Rx Packet      */
   t_state  context;                /* Bluetooth state context          */
 
   uint32_t  syncword[2];            /* Syncword stored in hardware      */

   uint32_t  com_ctrl_GP1_reg;       /* General Purpose Register 1 @20H  */
   uint32_t  com_ctrl_GP2_reg;       /* General Purpose Register 1 @24H  */
   uint32_t  tx_rx_ctrl_reg;         /* Transmit/Receive Control Register*/
   uint32_t  tx_rx_stat_reg;         /* Transmit/Receive Status Register */
 
   uint32_t  start_of_payload;       /* First 4 bytes of ACL payload     */

   uint32_t  native_clock_when_log;  /* Native Clock at log instant      */
   uint32_t  bt_clock_when_log;      /* BT Clock at log instant          */ 
   uint32_t  esco_ctrl_reg;          /* eSCO control register            */
   uint32_t  edr_ctrl_reg;           /* EDR control register             */

  } t_LLCpacket;

/*
 * Packet Log: Public Modifier Methods
 */
void LSLCstat_Setup_Packet_Log(t_LogPacketTypes logtype);
RAM_CODE void LSLCstat_Log_Rx_Packet(t_TXRXstatus status);
void LSLCstat_Log_Rx_Final_Status(t_TXRXstatus status);
void LSLCstat_Log_Tx_Packet(t_TXRXstatus status);
void LSLCstat_Log_TxRx_Frequency(uint16_t frequency, t_clock freqClock,
    t_state context);
/*
 * Packet Log: Public Accessor Methods
 */
const t_LLCpacket* LSLCstat_Get_Packet_Log(void);
uint32_t LSLCstat_Get_Next_Packet_Index(void);
t_LogPacketTypes LSLCstat_Get_Packet_Log_Type(void);

#endif
