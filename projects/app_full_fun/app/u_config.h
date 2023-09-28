/***************************************************************
 * 方案配置
 **************************************************************/
#ifndef _CONFIGH_
#define _CONFIGH_

//ic model(default model:IC_PTN1012)
#define IC_PTN1011		1
#define IC_PTN1012		2
//#undef IC_MODEL
#define IC_MODEL		IC_PTN1012

//types of usb class to select
#define USB_INVALID             0  //USB invalid
#define USBD_CLS_AUDIO          1
#define USBD_CLS_HID            2
#define USBD_CLS_AUDIO_HID      (USBD_CLS_HID | USBD_CLS_AUDIO)
#define USBH_CLS_DISK           8   //usb host msc, udisk

#define USER_UART_FUNCTION_DIRECT   // use recieve uart data directly

//***************************************************************************************************

#define USER_DBG_INFO(fmt,...)	os_printf("[USER]"fmt, ##__VA_ARGS__)

//**** 方案選擇 ********************************
//#include "PTN_DEMO.h"	//V3.3 ok
//#include "app_ui/ui_test.h"
//#include "DY_DT_K800.h"
//#include "ONS_01.h"
//#include "ONS_SK100.h"
//#include "ONS_TY332.h"
//#include "ZY_OA_001.h"	// 0806 ok
//#include "ZY_Spark_Pop.h"	// 0806 ok
//#include "ZY_BEEMIC2p4.h"
//#include "CPS_TL820.h"	// 0806 ok
//#include "APA320.h"
//#include "NW_TRX_01.h"
//#include "XLL_V48.h"
//#include "HAA9811.h"
//#include "SK_460.h"
//#include "SY_K59.h"

//#include "BZQ703.h"	//V3.3 ok
//#include "GY_P10.h"	    //0821 OK
//#include "YQ_DEMO.h"	//0806 ok
//#include "SG_P60.h"
//#include "PTH_X3_0602_V01.h"
//#include "MZ_200K.h"	//0821 ok
//#include "XFN_S930.h" //0821 OK
//#include "XWX_S6616.h"	//
//#include "A40_Effect.h"
//#include "YPT_Q66.h"
//#include "MJ_K04.h"   //0821 OK
//#include "SC_D3.h"
//#include "ZT_M184.h"	//0821 ok
//#include "COK_S8.h"
//#include "text1.h"
//#include "HZ_P1.h"
//********************************************************************************
#if IC_MODEL != IC_PTN1012
    #if (UART0_USB0_COEXIST == 1) /* 对于1011，如果设定串口和USB共存，报错*/
        #error "config error UART0_USB0_COEXIST"
    #endif
    #undef USB1_FUNC_SEL /* 如果是1011, 没有USB1，强制注销USB1的宏*/
    #define USB1_FUNC_SEL       USB_INVALID  //PTN1011 have no usb1
#endif

// 如果UART0和USB0引脚并联，并启用了USB0功能
#if (UART0_USB0_COEXIST == 0) && (USB0_FUNC_SEL != USB_INVALID) && (!defined(SYS_LOG_PORT_SEL))
	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_NO/* 关闭log功能 */
#endif

#ifndef FLASH_SIZE	// config flash size
    #define FLASH_SIZE  FLASH_INFO_16M //1=4M;2=8M;3=16M;4=32M
#endif
#ifndef CPU_CLK
	#define CPU_CLK  300000000	// 360000000 / 220000000	CPU_DPLL_CLK
#endif

#endif
