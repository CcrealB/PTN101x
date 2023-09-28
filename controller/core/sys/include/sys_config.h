#ifndef _PARTHUS_SYS_CONFIG_
#define _PARTHUS_SYS_CONFIG_

/***********************************************************************
 *
 * MODULE NAME:    sys_config.h
 * PROJECT CODE:   BlueStream
 * MAINTAINER:     Ivan Griffin
 * DATE:           Wed Jul 14 11:39:10 BST 1999
 *
 * SOURCE CONTROL: $Id: sys_config.h,v 1.105 2013/05/30 16:07:52 tomk Exp $
 *
 * LICENCE:
 *    This source code is copyright (c) 1999-2004 Ceva Inc.
 *    All rights reserved.
 *
 * REVISION HISTORY:
 *    14 Jul 1999 -         IG      - Macro defines for complete system.
 *     8 Mar 2001 -         JS      - Seperated hal and core defines (each HAL
 *                                    has its own sys_hal_config.h
 *    Jul/Aug 2001 -       JN       - Cleanup of sys_config for configuration
 *
 * Configuration is based on 5 major files 
 *
 * 1. Core configurable Features (including LMP) sys_features.h
 * 2. HAL configurable                           sys_hal_config.h
 * 3. Debugging options for components/modules   sys_debug.h
 * 4. Development options (new code selectiion)  sys_develop.h
 * 5. General configuration options              sys_config.h
 *
 * This file abstracts all others and is the only file that should be
 * included in the component modules within the system.
 *
 ***********************************************************************/
/*
 * System wide types
 */
#include "sys_types.h"

/*
 * System wide constants
 */
#include "sys_const.h"

#include <assert.h>


/*
 * Generated Features file from tool prh_bs_config
 */
#if defined(SHOGA)
#include "prh_bs_features.h"
#endif
/*
 * Generic Features
 */
#include "sys_features.h"


/*
 * Platform specific features 
 */
#include "sys_hal_features.h"

#include "sys_debug.h"

/*
 * Include development header file at end so that all can be overridden
 */

/*
 * Include HAL specific config file
 */
#include "sys_hal_config.h"

/*
 * The following constants are platform specific
 * and will normally be defined in sys_hal_config.h in the 
 * individual hal subdirectories.
 */

#ifndef __INLINE__
    #define __INLINE__ __inline
    #undef __USE_INLINES__
#endif   

#ifndef __COMPILER_VERSION__
    #define __COMPILER_VERSION__ "unknown compiler"
#endif

#ifndef __IRQ__   
    #define __IRQ__  
#endif

#include "doubly_list.h"

/*
 * For some compilers, inlines are handled via pragmas, ignore if not defined
 */
#ifndef PRAGMA_INLINE
#define PRAGMA_INLINE 0
#endif


/*
 * Macro to print version & date and time of compilation
 */
#define VERSION "(\"Ceva XpertBlue\" - Bluetooth Host Controller Stack - "__DATE__" - "__TIME__")"


/*
 * Include development overrides at end so that all can be overridden
 */
#include "sys_develop.h"

/***********************************************************************
 * t_SYS_Config
 *
 * The system configuration data structure
 *
 * Contains the global wide system configuration variables.
 ***********************************************************************/

typedef struct
{
    t_bd_addr local_device_address;
    t_syncword local_device_syncword;

/*
 * Basic configuration
 */
    uint32_t data_whitening_enable;
    uint32_t hopping_mode;
    uint32_t tx_freq;
    uint32_t rx_freq;
    uint32_t win_ext;

    uint8_t  feature_set[16];
	
	uint8_t hci_command_set[64];			/* HCI Supported Command Set */
    t_versionInfo version_info;         /* HC & LMP Version Information */
    t_bufferSize  hc_buffer_size;       /* HCI ( Queue ) Buffer size    */
#if (PRH_BS_CFG_SYS_FLEXIBLE_DATA_BUFFER_SIZES_SUPPORTED==1)
    t_bufferSize  hc_buffer_size_in;    /* Buffer size on incoming data */
#endif
    t_classDevice device_class;         /* Class of the Device          */
    uint8_t  clock_jitter_us;            /* Clock jitter in us           */
    uint8_t  clock_drift_ppm;            /* Clock drift in ppm           */
    uint16_t  max_active_devices;
    uint16_t  max_active_devices_in_piconet;
    void* device_links_location;   
    void* parked_device_links_location;
    uint16_t local_device_index;
    uint32_t rand_seed;                  /* Seed for random number       */
#if (PRH_BS_CFG_SYS_DEPRECATED_BT12_FEATURES_SUPPORTED==1)
    uint8_t  unit_key[16];               /* Local device's unit key      */
#endif
	uint8_t  erroneous_data_reporting;   /* Indicates if Erroneous Data Reporting is supported */
#if (AGC_MODE_CHANNEL_ASSESSMENT == 1)
    uint8_t  agc_mode;
#endif
} t_SYS_Config;


void SYSconfig_Initialise(void);

__INLINE__ t_syncword        SYSconfig_Get_Local_Syncword(void);
__INLINE__ const t_syncword* SYSconfig_Get_Local_Syncword_Ref(void);
__INLINE__ void SYSconfig_Get_Local_Syncword_Ex(t_syncword* p_syncword);

__INLINE__ void SYSconfig_Set_Local_Syncword(t_syncword syncword);

__INLINE__ t_bd_addr SYSconfig_Get_Local_BD_Addr(void);
__INLINE__ const t_bd_addr *SYSconfig_Get_Local_BD_Addr_Ref(void);
__INLINE__ void SYSconfig_Get_Local_BD_Addr_Ex(t_bd_addr *addr);
__INLINE__ void SYSconfig_Get_Local_BD_Addr_U32(uint32_t* addr);

__INLINE__ void SYSconfig_Set_Local_BD_Addr(t_bd_addr bd_addr);

__INLINE__ uint8_t* SYSconfig_Get_HC_Commands_Ref(void);

__INLINE__ uint8_t* SYSconfig_Get_LMP_Features_Ref(void);
__INLINE__ void    SYSconfig_Set_LMP_Features(const uint8_t lmp_features_array[]);

__INLINE__ uint8_t* SYSconfig_Get_LMP_Extended_Features_Ref(uint8_t page);

__INLINE__ BOOL SYSconfig_Is_Feature_Supported(uint32_t feature);
__INLINE__ BOOL SYSconfig_Is_Command_Supported(uint32_t command);
__INLINE__ void    SYSconfig_Disable_Feature(uint32_t feature);
__INLINE__ void    SYSconfig_Enable_Feature(uint32_t feature);

__INLINE__ uint8_t* SYSconfig_Get_Ext_Features_Ref(void);
__INLINE__ void    SYSconfig_Set_Ext_Features(const uint8_t ext_features_array[]);

__INLINE__ const t_versionInfo* SYSconfig_Get_Version_Info_Ref(void);
__INLINE__ const t_bufferSize* SYSconfig_Get_HC_Buffer_Size_Ref(void);

__INLINE__ t_classDevice SYSconfig_Get_Device_Class(void);
__INLINE__ void SYSconfig_Set_Device_Class(t_classDevice device_class);

__INLINE__ uint32_t SYSconfig_Get_Data_Whitening_Enable(void);
__INLINE__ void SYSconfig_Set_Data_Whitening_Enable(BOOL dwh_enable);

__INLINE__ uint32_t SYSconfig_Get_Hopping_Mode(void);
__INLINE__ uint32_t SYSconfig_Get_Tx_Freq(void);
__INLINE__ uint32_t SYSconfig_Get_Rx_Freq(void);
__INLINE__ void    SYSconfig_Set_Hopping_Mode(t_rfSelection hopping_mode,
               t_freq txMasterFixedFreq, t_freq rxMasterFixedFreq );
__INLINE__ uint32_t SYSconfig_Get_Win_Ext(void);

__INLINE__ uint8_t SYSconfig_Get_Clock_Jitter(void);
__INLINE__ void   SYSconfig_Set_Clock_Jitter(uint8_t jitter);
__INLINE__ uint8_t SYSconfig_Get_Clock_Drift(void);
__INLINE__ void   SYSconfig_Set_Clock_Drift(uint8_t drift);

__INLINE__ uint16_t SYSconfig_Get_Local_Device_Index(void);
__INLINE__ void SYSconfig_Set_Local_Device_Index(uint16_t local_device_index);

__INLINE__ uint16_t SYSconfig_Get_Max_Active_Devices_In_Piconet(void);
__INLINE__ void SYSconfig_Set_Max_Active_Devices_In_Piconet(uint16_t max_active_devices_in_piconet);

__INLINE__ uint16_t SYSconfig_Get_Max_Active_Devices(void);
__INLINE__ void SYSconfig_Set_Max_Active_Devices(uint16_t max_active_devices);

__INLINE__ void* SYSconfig_Get_Device_Links_Location(void);
__INLINE__ void SYSconfig_Set_Device_Links_Location(void* pDL);
#if (PRH_BS_CFG_SYS_DEPRECATED_BT12_FEATURES_SUPPORTED==1)
__INLINE__ void SYSconfig_Set_Unit_Key(uint8_t* new_unit_key);
__INLINE__ uint8_t* SYSconfig_Get_Unit_Key_Ref(void);
#endif
__INLINE__ uint32_t SYSconfig_Get_SYS_Rand_Seed(void);

__INLINE__ uint8_t SYSconfig_Get_Erroneous_Data_Reporting(void);
__INLINE__ void SYSconfig_Set_Erroneous_Data_Reporting(uint8_t);


#if (AGC_MODE_CHANNEL_ASSESSMENT == 1)
__INLINE__ uint8_t SYSconfig_Get_AGC_Mode(void);
__INLINE__ void SYSconfig_Set_AGC_Mode(uint8_t);
#endif

#ifdef __USE_INLINES__
#include "sys_config_impl.h"
#endif

#ifdef CONFIG_EN_AFH_FUNC
#define DBG_AHF_PRINTF(fmt, ...)
#endif

#endif
