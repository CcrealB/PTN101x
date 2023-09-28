#include "includes.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "app_beken_includes.h"
#include "app_drc.h"
#include "app_tlv.h"
#include "PTN101_calibration.h"
//#include "driver_usb.h"
#include "app_bt.h"
#ifdef CONFIG_PRODUCT_TEST_INF
#include "driver_icu.h"
#include "bt_spp.h"
#endif

#if CONFIG_BLUETOOTH_PBAP
#include "bt_pbap.h"
#endif

#if PTS_TESTING
#include "pts/pts.h"
#endif

#ifdef CONFIG_USE_BLE
	#include "app_fff0.h"
	#include "app.h"
#endif

#include "drv_mailbox.h"
#include "drv_msg.h"
// #include "u_com.h"
#include "u_include.h"

uint8_t g_dsp_log_on_fg = 1;
uint8_t g_mcu_log_on_fg = 1;	//yuan
uint8_t g_hid_log_on_fg = 0;	//yuan

// __attribute__((weak)) void com_cmd_send_proc(uint8_t *buf,uint8_t len) {}
// __attribute__((weak)) void com_cmd_recv_proc(uint8_t *buf, uint8_t len) {}

//**** UARTx : 0, 1, 2 ****************************************************
#define u_uart_send(UARTx, buf, len)     ptn_uart_write(UARTx, buf, len)

//************************************************
void com_cmd_send_proc(uint8_t *buf, uint8_t len)
{
	if(g_mcu_log_on_fg==0){	// 上位機連線
    	if(sys_log_port_get(SYS_LOG_PORT_UART0)){
    		u_uart_send(0, buf, len);	//yuan
    	}else if(sys_log_port_get(SYS_LOG_PORT_UART1)){
    		u_uart_send(1, buf, len);	//yuan
    	}
	}
//#ifdef PT40Effect
#ifdef USR_UART1_INIT_EN
    u_uart_send(1, buf, len);	//yuan
#endif
    //====================================================
#if (USB0_FUNC_SEL & USBD_CLS_HID)
    if(g_hid_log_on_fg){	//yuan
        extern void usbd_dbg_hid_tx(void* buf, int size);
        usbd_dbg_hid_tx(buf, 32);
        CLEAR_WDT; //清除看门狗，默约38.8sec
        os_delay_us(3200);	// delay 1ms
    }
#endif

#ifdef CONFIG_USE_BLE
	extern void app_fff1_send_lvl(uint8_t* buf, uint8_t len);
	if(appm_get_connection_num()){
		app_fff1_send_lvl((uint8_t*)buf, len);
//		DBG_LOG_INFO("=====BLE tx Len:%d ...\n",len);
	}else{
//		DBG_LOG_INFO("=====BLE NO connection:%d ...\n",appm_get_connection_num());
	}
#endif

}
//************************************************
void com1_cmd_send_proc(uint8_t *buf, uint8_t len)
{
	u_uart_send(1, buf, len);
}

//************************************************
void com2_cmd_send_proc(uint8_t *buf, uint8_t len)
{
	u_uart_send(2, buf, len);
}


extern uint8_t lf;
extern void UpComputerUart32ByeRx(uint8_t* ptr);

#if (USB0_FUNC_SEL & USBD_CLS_HID)
extern void UpComputerHid32ByeRx(void*ptr);
void usbd_dbg_hid_rx(void* ptr, int size)
{
    uint8_t* buf = (uint8_t*)ptr;
#if 0
    DBG_LOG_INFO("user dbg hid rcv %d data:\n", size);
    for(int i = 0; i < size; i++) { std_printf("%02X ", buf[i]); }
    std_printf("\n");
    extern void usbd_dbg_hid_tx(void* buf, int size);
    usbd_dbg_hid_tx(ptr, size);
#else
    if(buf[0] == 0xA5) {
        // DBG_LOG_INFO("hid rcv a PTN cmd, data[0]:%02X, sz:%d\n",buf[0], size);
    	UpComputerHid32ByeRx(ptr);
        extern uint8_t g_hid_log_on_fg;	//yuan
        g_hid_log_on_fg = 1;			//yuan
    }
#endif
}
#endif


//***********************************************
#ifdef USER_UART_FUNCTION_DIRECT
void com_cmd_recv_proc_direct(uint8_t value)
{
	static uint8_t TempBuf[33];
	TempBuf[lf++] = value;
	if(lf==1 && TempBuf[0] != 0xA5){
		lf = 0;
		return;
	}
	if(lf==2 && TempBuf[1] != 0xA1){
		lf = 0;
		return;
	}
	if(lf <32) return;
	lf = 0;
	UpComputerUart32ByeRx(TempBuf);
//	DBG_LOG_INFO("com_cmd_recv_proc_direct: %02X\n", value);
}
#endif

//***********************************************
#ifdef USER_UART1_FUNCTION_DIRECT
void com1_cmd_recv_proc_direct(uint8_t value)
{

#if defined(CDX_9522_1011)
	extern void CDX_9522_CmdRx(uint8_t rdata);
	CDX_9522_CmdRx(value);
#elif defined(A40_Effect) || defined(KT16TRX)
	extern void Uart1_CmdRx(uint8_t rdata);
	Uart1_CmdRx(value);
#elif defined(MJ_K04)
	extern void Uart1_CmdRx(uint8_t rdata);
	Uart1_CmdRx(value);



#else
	static uint8_t TempBuf[33];
	TempBuf[lf++] = value;
	if(lf==1 && TempBuf[0] != 0xA5){
		lf = 0;
		return;
	}
	if(lf==2 && TempBuf[1] != 0xA1){
		lf = 0;
		return;
	}
	if(lf <32) return;
	lf = 0;
	UpComputerUart32ByeRx(TempBuf);
//	u_uart_send(0, TempBuf, 32);	//yuan

#endif
//	DBG_LOG_INFO("com1_cmd_recv_proc_direct: %02X\n", value);
}
#endif

//***********************************************
#ifdef USER_UART2_FUNCTION_DIRECT

void com2_cmd_recv_proc_direct(uint8_t value)
{
	extern void Uart2_CmdRx(uint8_t rdata);
	Uart2_CmdRx(value);
//	DBG_LOG_INFO("com2_cmd_recv_proc_direct: %02X\n", value);
}
#endif


/**************************************************************************/
//**** ble ********************************************
#ifdef CONFIG_USE_BLE
void com_ble_send(uint8_t *buf, uint8_t size)
{
	extern void app_fff1_send_lvl(uint8_t* buf, uint8_t len);
	if(appm_get_connection_num()){
		app_fff1_send_lvl((uint8_t*)buf, size);
	}else{
//		DBG_LOG_ERR("com_ble_send error!\n");
	}
}
//*********************************************
void com_ble_recv(uint8_t *buf, uint8_t size)
{
#ifdef TEST_USER_BLE_RX
	DBG_LOG_INFO("FFF2 recv %d data, HEX display:\n", size);
	for(int i = 0; i < size; i++) std_printf("%02x ",buf[i]);
	std_printf("\n\n");
#endif
	extern void UpComputer32ByeRx(void*ptr);
	UpComputer32ByeRx(buf);
//	extern void UpComputerRx(uint8_t rdata);
//	for(int i = 0; i < size; i++){
//		if(lf<32) UpComputerRx(buf[i]);
//	}
}

//#ifdef TEST_USER_BLE_TX
void test_ble_fff0s_send(void)
{
    uint8_t buff[32];
    int i; for(i=0; i< 32; i++){ buff[i] = i; }
    // app_ota_ble_send((uint8_t*)buff, 32);
    com_ble_send((uint8_t*)buff, 32);
}
//#endif //CONFIG_USE_BLE

#endif

