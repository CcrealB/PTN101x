/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "includes.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "app_beken_includes.h"
#include "app_drc.h"
#include "app_tlv.h"
#include "PTN101_calibration.h"
//#include "driver_usb.h"
#include "app_bt.h"
#include "driver_serio.h"
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
#if (BT_DUALMODE_RW == 1)
#include "rwip_config.h"
#include "uart.h"
#endif

#if SYS_DSP_ENABLE == 1
#include "app_dsp.h"
#include "hal_dsp_audio.h"
#endif
//#include "bk3266_calibration.h"

#ifndef REG_READ
#define REG_READ(addr)          *((volatile uint32_t *)(addr))
#endif
#ifndef REG_WRITE
#define REG_WRITE(addr, _data)  (*((volatile uint32_t *)(addr)) = (_data))
#endif

volatile unsigned int g_debug_mcu_switch = 0;
#if (DEBUG_BASEBAND_MONITORS == 1)
extern void TC_Read_Local_Baseband_Monitors_Via_App(void);
extern void TC_Read_Non_Signal_Test_Monitors(void);
extern void LSLCstat_Reset_Monitors();
#endif
#if (DEBUG_SCATTERNET_MONITORS == 1)
extern void TC_Read_Local_LMPconfig_Monitors_Via_App(void);
#endif

extern void TRAhcit_Rx_Char(uint8_t ch);
extern void juart_receive(uint8_t *buf, uint16_t size);

extern void app_env_dump(void);
extern result_t a2dp_cmd_disconnect(void);
extern result_t hf_cmd_disconnect(void);
extern result_t spp_slc_disconnect(void);
extern void app_print_linkkey(void);
extern uint32_t XVR_analog_reg_save[16];

extern void print_page_scan_status(void);

unsigned char uart_rx_buf[0x100/*RX_FIFO_THRD*/]={0};
unsigned char uart_tx_buf[TX_FIFO_THRD]={0};
volatile BOOL uart_rx_done = FALSE;
volatile unsigned int uart_rx_index = 0;


#if (BT_DUALMODE_RW == 0)
void Delay_us(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}
#endif


#if ITEST_ENABLE
unsigned char uart_rx_buf_itest[RX_FIFO_THRD];
volatile unsigned int uart_rx_index_itest = 0;
volatile BOOL uart_rx_done_itest = FALSE;

static const char hex[] = "0123456789ABCDEF";
void xHexToStr(uint8_t *pStr, uint8_t *pHex, uint16_t pLen)
{
	uint8_t i; 
	for (i = 3; i < pLen; i++)
	{
		*pStr++ = hex[((pHex[i]) >> 4) &0x0F];
		*pStr++ = hex[(pHex[i]) & 0x0F];
	}
}

uint8_t get_mac_addr_str []= "txevm -g 1\r\n";
uint8_t get_mac_res[25];
uint8_t set_mac_addr[12];
uint8_t set_mac_addr_flag = 0;
uint8_t dut_test_start_str[] = "ble dut\r\n";
uint8_t dut_test_res_str[] = "enter ble dut\r\n";
uint8_t dut_quit_str[] = "exit ble dut\r\n";
uint8_t dut_tx_test_str[] = "set pwr:9 - c:";
uint8_t dut_tx_test_res[20];
uint8_t dut_reset_str[] = "reboot\r\n";
uint8_t dut_reset2_str[] = "uartdwld\r\n";
uint8_t itest_dut_flag = 0;

#endif

HCI_COMMAND_PACKET *pHCIrxBuf = (HCI_COMMAND_PACKET *)(&uart_rx_buf[0]);
HCI_EVENT_PACKET   *pHCItxBuf = (HCI_EVENT_PACKET *)(&uart_tx_buf[0]);
#ifdef	CONFIG_BLUETOOTH_AVDTP_SCMS_T
extern void security_control_cp_support_print(void);  //print the array's param for check
#endif

#if A2DP_ROLE_SOURCE_CODE

extern void a2dpSrcConnectRemoteDevice(btaddr_t *remote_btaddr_p);
extern void a2dpSnkConnectRemoteDevice(btaddr_t *remote_btaddr);
extern uint8_t get_a2dp_role(void);

#endif

enum
{
    DBG_HCI_STATE_RX_TYPE,
    DBG_HCI_STATE_RX_COMMAND_OPCODE1,
    DBG_HCI_STATE_RX_COMMAND_OPCODE2,
    DBG_HCI_STATE_RX_COMMAND_LENGTH,
    DBG_HCI_STATE_RX_DATA_START,
    DBG_HCI_STATE_RX_DATA_CONTINUE,
    DBG_HCI_STATE_RX_DATA_COMMIT
};
#if(CONFIG_PRE_EQ==1||CONFIG_HFP_SPK_EQ==1||CONIFG_HFP_MIC_EQ==1)
static uint8 sw_eq_flag = 0;
#endif
//uint8_t dbg_hci_rx_buf[255] = {0};
static uint8_t s_dbg_hci_rx_state;
static volatile uint8_t* s_dbg_hci_rx_pdu_buf = (uint8_t *)&uart_rx_buf[4];
static volatile uint16_t s_dbg_hci_rx_pdu_length;
static uint8_t s_dbg_hci_rx_length;
uint8_t* s_dbg_hci_rx_head_buf = (uint8_t *) &uart_rx_buf[0];

void show_bt_stack_status(void);
void app_bt_status_show( void );
#ifdef BEKEN_DEBUG
void app_bt_debug_show( void );
#endif
/*
 * uart_initialise
 *
 * This function initialises the UART registers & UART driver paramaters.
 */
void uart_initialise(uint32_t baud_rate)
{
    uint32_t baud_divisor = baud_divisor = 26000000 / baud_rate - 1;

    REG_UART0_CONF = (DEF_STOP_BIT << SFT_UART0_CONF_STOP_LEN)
             | ((DEF_PARITY_MODE & 0x1) << SFT_UART0_CONF_PAR_MODE)
             | (((DEF_PARITY_EN >> 1) & 0x1) << SFT_UART0_CONF_PAR_EN)
             | (DEF_DATA_LEN << SFT_UART0_CONF_LEN)
             | (baud_divisor << SFT_UART0_CONF_CLK_DIVID)
             | (DEF_RX_EN << SFT_UART0_CONF_RX_ENABLE)
             | (DEF_TX_EN << SFT_UART0_CONF_TX_ENABLE);
    REG_UART0_FIFO_CONF  = ((RX_FIFO_THRD << SFT_UART0_FIFO_CONF_RX_FIFO_THRESHOLD) | (TX_FIFO_THRD << SFT_UART0_FIFO_CONF_TX_FIFO_THRESHOLD));
    REG_UART0_INT_ENABLE = MSK_UART0_INT_ENABLE_RX_STOP_END_MASK | MSK_UART0_INT_ENABLE_RX_FIFO_NEED_READ_MASK;

    REG_GPIO_0x00 = 0x70;
    REG_GPIO_0x01 = 0x7C;
}
//**********************************************************************
void uart1_initialise(uint32_t baud_rate)
{
   uint32_t baud_divisor = baud_divisor = 26000000 / baud_rate - 1;
   
    REG_SYSTEM_0x01 &= ~(1 << 22);
    
	REG_UART1_CONF = (DEF_STOP_BIT << SFT_UART1_CONF_STOP_LEN)
			 | ((DEF_PARITY_MODE & 0x1) << SFT_UART1_CONF_PAR_MODE)
			 | (((DEF_PARITY_EN >> 1) & 0x1) << SFT_UART1_CONF_PAR_EN)
			 | (DEF_DATA_LEN << SFT_UART1_CONF_LEN)
			 | (baud_divisor << SFT_UART1_CONF_CLK_DIVID)
			 | (DEF_RX_EN << SFT_UART1_CONF_RX_ENABLE)
			 | (DEF_TX_EN << SFT_UART1_CONF_TX_ENABLE);
	REG_UART1_FIFO_CONF  = ((RX_FIFO_THRD << SFT_UART1_FIFO_CONF_RX_FIFO_THRESHOLD) | (TX_FIFO_THRD << SFT_UART1_FIFO_CONF_TX_FIFO_THRESHOLD));
	REG_UART1_INT_ENABLE = MSK_UART1_INT_ENABLE_RX_STOP_END_MASK | MSK_UART1_INT_ENABLE_RX_FIFO_NEED_READ_MASK;

	REG_GPIO_0x10 = 0x70;	//output enable GPIO16 2nd Function Enable TX
	REG_GPIO_0x11 = 0x7C;	//input enable  GPIO17 2nd Function Enable  RX
	system_gpio_peri_config(16,1);	//GPIO16,mode 0: Perial Mode 2 function
	system_gpio_peri_config(17,1);	//GPIO17,mode 0: Perial Mode 2 function

}

void uart_gpio_disable(void)
{
    REG_GPIO_0x00 = 0x3c;    // input and pull up
    REG_GPIO_0x01 = 0x3c;    // input and pull up
}
void uart_gpio_enable(void)
{
    REG_GPIO_0x00 = 0x70;    // Tx and pull up;
    REG_GPIO_0x01 = 0x7C;    // Rx and pull up;
}

void uart1_gpio_enable(void)
{
    REG_GPIO_0x10 = 0x70;//output enable GPIO16 2nd Function Enable TX
	REG_GPIO_0x11 = 0x7C;
}

void sys_log_uart_init(uint8_t UARTx, uint8_t en)
{
    if(UARTx == 0){
        app_uart0_init_def(en);
    }else if(UARTx == 1){
        app_uart1_init_def(en, 1);
    }    
}

void uart_send(unsigned char* buf, unsigned int len)
{
    while(len)
    {
        if(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_FIFO_WR_READY)
        {
            REG_UART0_FIFO_PORT = *buf++;
            len--;
        }
    }
}

int uart_putchar_1(char * st)
{
	uint8_t num = 0;
    while (*st) 
    {
		if(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_FIFO_WR_READY)
		{
			REG_UART0_FIFO_PORT = *st;
	    	st++;
			num++;
	    }
	} 
    return num;
}


void uart1_send(unsigned char* buf, unsigned int len)
{
    while(len)
    {
        if(REG_UART1_FIFO_STATUS & MSK_UART1_FIFO_STATUS_FIFO_WR_READY)
        {
            REG_UART1_FIFO_PORT = *buf++;
            len--;
        }
    }
}


// static uint8_t g_hid_log_on_fg = 0;
void hid_log_open(uint8_t en) { g_hid_log_on_fg = !!en; }
uint8_t hid_log_is_open(void) { return g_hid_log_on_fg; }

// static uint8_t g_mcu_log_on_fg = 1;
void sys_log_open(uint8_t en) { g_mcu_log_on_fg = !!en; }
uint8_t sys_log_is_open(void) { return g_mcu_log_on_fg; }

// static uint8_t g_dsp_log_on_fg = 1;
void dsp_log_open(uint8_t en) { g_dsp_log_on_fg = !!en; }
uint8_t dsp_log_is_open(void) { return g_dsp_log_on_fg; }

#ifndef SYS_LOG_PORT_SEL
    #define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART0
#endif
static uint32_t g_mcu_log_port = SYS_LOG_PORT_SEL;

uint32_t sys_log_port_get(uint32_t flag) { return g_mcu_log_port & flag; }
void sys_log_port_set(uint32_t flag, int en)
{
    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    if(!en) { g_mcu_log_port &= ~flag; }
    if(flag & SYS_LOG_PORT_UART0) { sys_log_uart_init(0, en); }
    if(flag & SYS_LOG_PORT_UART1) { sys_log_uart_init(1, en); }
    if(flag & SYS_LOG_PORT_USB_HID) { g_hid_log_on_fg = en; }
    if(en) { g_mcu_log_port |= flag; }
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    // en ? (g_mcu_log_port |= flag) : (g_mcu_log_port &= ~flag);
}

void app_sys_log_init(uint8_t en)
{
    sys_log_port_set(SYS_LOG_PORT_SEL, en);
}

int app_sys_log_write(uint8_t *buf, int size)
{
    if(!g_mcu_log_on_fg) return 0;

    if (app_bt_flag2_get(APP_FLAG2_VUSB_DLP_PRINTF)&&(vusb_get_mode()==VUSB_COMM_MODE)){
        vusb_dlp_send((unsigned char *)&buf[0], size);
    }
    if(sys_log_port_get(SYS_LOG_PORT_UART0)){ ptn_uart_write(0, (unsigned char *)&buf[0], size); }
    if(sys_log_port_get(SYS_LOG_PORT_UART1)){ ptn_uart_write(1, (unsigned char *)&buf[0], size); }
#if (USB0_FUNC_SEL & USBD_CLS_HID)
    extern void usbd_dbg_hid_tx(void* buf, int size);
    if(sys_log_port_get(SYS_LOG_PORT_USB_HID)){ usbd_dbg_hid_tx(buf, size > 32 ? 32 : size); }
#endif
#ifdef CONFIG_DBG_LOG_FLASH_OP
    if(sys_log_port_get(SYS_LOG_PORT_DBG)){
        extern void dbg_log_write_buff(uint8_t* buf, int size);
        dbg_log_write_buff((uint8_t *)&buf[0], rc);
    }
#endif
    return size;
}

#define PRINT_BUF_PREPARE(rc, buf, fmt)             \
    int rc;                                         \
    va_list args;                                   \
    va_start(args, fmt);                            \
    rc = vsnprintf(buf, sizeof(buf), fmt, args);    \
    va_end(args);                                   \
    buf[sizeof(buf) - 1] = '\0';
#define PRINT_BUF_SIZE 0X100

int32_t os_printf(const char *fmt, ...) {
    if(g_mcu_log_on_fg)
    {
    #if (BT_HOST_MODE == JUNGO_HOST)
        char buf[PRINT_BUF_SIZE];

        //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
        //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.

        PRINT_BUF_PREPARE(rc, buf, fmt);
        app_sys_log_write((unsigned char *)&buf[0], rc);
        
        //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
        return rc;
    #else
        //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
        //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
        os_delay_us(10);
        //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
        return 0;
    #endif
    }
    else
    {
        return 0;
    }
}

int32_t os_printf_itest(const char *fmt, ...) {
#if (BT_HOST_MODE == JUNGO_HOST)
    char buf[PRINT_BUF_SIZE];

    //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.

    PRINT_BUF_PREPARE(rc, buf, fmt);

    app_sys_log_write((unsigned char *)&buf[0], rc);

    //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return rc;
 #else
    //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    os_delay_us(10);
    //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return 0;
 #endif
}


int32_t os_null_printf(const char *fmt, ...) {
    return 0;
}

void clear_uart_buffer(void) {
    uart_rx_index = 0;
    uart_rx_done = FALSE;
    memset(uart_rx_buf, 0, sizeof(uart_rx_buf)); /**< Clear the RX buffer */
    memset(uart_tx_buf, 0, sizeof(uart_tx_buf)); /**< Clear the TX buffer */
}

void Beken_Uart_Tx(void) {
    unsigned int tx_len       = HCI_EVENT_HEAD_LENGTH+pHCItxBuf->total;
    pHCItxBuf->code  = TRA_HCIT_EVENT;
    pHCItxBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    uart_send(uart_tx_buf, tx_len);
}

static void app_debug_showstack(void) {
    extern uint32_t _sbss_end;
    extern uint32_t _stack;
    uint32_t count;
    uint32_t *ptr;
    uint32_t i;

    count = (((uint32_t)&_stack - (uint32_t)&_sbss_end) >> 2) - 2;
    ptr = (uint32_t *)((uint32_t)&_sbss_end  & (~3));

    os_printf("ShowStack:%p:%p\r\n",  &_sbss_end, &_stack);
    for(i = 0; i < count; i ++)
        os_printf("0x%x:%p\r\n", &ptr[i], ptr[i]);
}

#if (DEBUG_AGC_MODE_CHANNEL_ASSESSMENT == 1)
extern void _LSLCacc_Read_AGC_Param(void);
#endif

#define HF_SCO_CONN 0
#define HF_SCO_DISCONN 1

#if A2DP_ROLE_SOURCE_CODE
extern void app_bt_sdp_connect(void);
#endif

extern void app_led_dump(void);

#if A2DP_ROLE_SOURCE_CODE
extern void set_sdp_browse_on(void);
extern result_t sdp_send_serviceSearchRequest(void);
extern void app_bt_sdp_send_serviceAttributeRequest(void);
#endif

//******************************************************
void sub_cmd_proc(uint8_t *buf, uint8_t len)
{
    SUB_CMD_PKG_e *subCmd = (SUB_CMD_PKG_e*)buf;
    switch (subCmd->cmd)
    {
#ifdef CONFIG_SOFT_FIFO_DCOM
    case SUB_CMD_MAILBOX:
        /* cmd fromat: 01 E0 FC xx 2B 80 xx xx xx ...(head[01 E0 FC] cmd_len[xx] mcuCmd[0x2B] comCmd[0x80] data0[xx] data1[xx] ...)
        dspCmd format: 01 E0 FC LEN 2B 80 dspCmd dspParam ...(LEN = sizeof(dspParam)+3)
        test log:(PC->MCU and MCU->PC):
        * pc->mcu: 01 E0 FC 10 2B 80 10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F
        * buff:                      10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F   (dsp recieved)
        * cmd:                             FF FF FF FF CD CC 8C 3F 00 00 80 3F   (dsp cmd)
        * pc->mcu: (demo debug)
        *          01 E0 FC 10 2B 80 10 10 FF FF FF FF CD CC 8C 3F CD CC 8C 3F
        */
        com_cmd_mcu2dsp_proc((uint8_t*)&subCmd->param[0], len-1);
        // INFO_PRT("0x80 cmd:0x%X, len:%d\n", *((uint32_t*)&subCmd->param[2]), len);
        break;
#endif
    case SUB_CMD_DSP_SET://82
        /* cmd fromat: 01 E0 FC xx 2B 80 xx xx xx ...(head[01 E0 FC] cmd_len[xx] mcuCmd[0x2B] comCmd[0x80] data0[xx] data1[xx] ...)
        dspCmd format: 01 E0 FC LEN 2B 80 dspCmd dspParam ...(LEN = sizeof(dspParam)+3)
        test log:(PC->MCU and MCU->PC):
        * pc->mcu: 01 E0 FC 10 2B 82 10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F
        * buff:                      10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F   (10 10 No meaning,, only for 4byte aligned)
        * cmd :                            FF FF FF FF CD CC 8C 3F 00 00 80 3F   (dsp recieved & cmd)
        * pc->mcu: (demo debug)
        *          01 E0 FC 10 2B 82 10 10 FF FF FF FF CD CC 8C 3F CD CC 8C 3F
        */
        mbx_cmd_mcu2dsp_set((uint8_t*)&subCmd->param[2], len - 3, 1);//ensure param 4byte aligned
        // INFO_PRT("0x82 cmd:0x%X, len:%d\n", *((uint32_t*)&subCmd->param[2]), len);
        break;
    case SUB_CMD_DSP_SET_SINGLE://83
    {
       /* pc->mcu: 01 E0 FC 10 2B 83 10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F
        * param:                     10 10 FF FF FF FF CD CC 8C 3F 00 00 80 3F   (10 10 No meaning,, only for 4byte aligned)
        * cmd :                            FF FF FF FF CD CC 8C 3F 00 00 80 3F   (dsp recieved & cmd)*/
        #if 1
        int free = mbx_cmd_mcu2dsp_set_single((uint32_t*)&subCmd->param[2], len - 3, 0);
        INFO_PRT("single share msg cmd:0x%X, cmd len:%d, free:%d\n", *((uint32_t*)&subCmd->param[2]), len - 3, free);
        #else
        int ret = mbx_cmd_mcu2dsp_set_single((uint32_t*)&subCmd->param[2], len - 3, 1);
        INFO_PRT("single fast cmd:0x%X, cmd len:%d, ret:%d\n", *((uint32_t*)&subCmd->param[2]), len - 3, ret);
        #endif
    }
    break;
    case SUB_CMD_DSP_EFT_SET://0x85
    {
       // pc->mcu: 01 E0 FC 03 2B 85 01/00      on/off
        mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_DSP_PERFORMENCE | MAILBOX_CMD_FAST_RSP_FLAG, 1, (uint32_t)subCmd->param[0], 0, NULL);
    }
    break;
        
    case SUB_CMD_MAILBOX_DBG:
    {
        // cmd fromat: 01 E0 FC 12 2B 81 xx xx xx ...(head[01 E0 FC] cmd_len[xx] mcuCmd[0x2B] comCmd[0x80] data0[xx] data1[xx] ...)
        //dspCmd format: 01 E0 FC 12 2B 81 u32 u32 u32 u32
        uint32_t param[4];
        int param_size = len - 1;
        if(param_size == 16)
        {
            memcpy(&param[0], &subCmd->param[0], 16);
            uint32_t dsp2mcu_ack_data[4] = {0};
            mbx_mcu2dsp_transfer(param[0], param[1], param[2], param[3], &dsp2mcu_ack_data[0]);
            uint32_t p0 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
            uint32_t p1 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
            uint32_t p2 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
            uint32_t p3 = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
            INFO_PRT("tx 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", param[0], param[1], param[2], param[3]);
            INFO_PRT("rx 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", p0, p1, p2, p3);
        }
        else
        {
            INFO_PRT("SUB_CMD_MAILBOX_ALL size error: %d\n", param_size);
        }
    }break;
    case SUB_CMD_LOOP_TEST:
        /* cmd fromat: 01 E0 FC xx 2B 00 xx xx xx ...(head[01 E0 FC] cmd_len[xx] Cmd[2B] subCmd[00] data0[xx] data1[xx] ...)
        cmd_len = (data bytes) + (1byte Cmd) + (1byte subCmd)
        len = cmd_len -1
        test log(PC->MCU and MCU->PC):
            [2022-04-26 12:55:34.534]# SEND HEX>
            01 E0 FC 06 2B 00 AA BB CC DD 
            [2022-04-26 12:55:34.596]# RECV HEX>
            00 AA BB CC DD 
        */
        app_sys_log_write(buf, len);
        break;
    case SUB_CMD_DEBUG:
    {
        #if 0
        /* cmd fromat: 01 E0 FC xx 2B 01 xx xx xx ...(head[01 E0 FC] cmd_len[xx] mcuCmd[0x2B] comCmd[0x02] data0[xx] data1[xx] ...)
        len = (bytes of data len) + (1byte mcuCmd) + (1byte comCmd)
        test log(PC->MCU and MCU->PC):
            PC->MCU : 01 E0 FC 06 2B 01 AA BB CC DD
            MCU->PC : 01 AA BB CC DD 
        */
        app_sys_log_write(buf, len);
        #else
        SUB_CMD_PKG_e *pSysCmd = (SUB_CMD_PKG_e*)(subCmd->param);
        switch (pSysCmd->cmd)
        {
        case 0x00://reg read
        {//exp : read reg addr 0xAABBCCDD -> 01 E0 FC 07(len) 2B 01 00(rd cmd) AA BB CC DD
         //template :  01 E0 FC 07 2B 01 00 xx xx xx xx
         //demo: 01 E0 FC 07 2B 01 00 01 00 00 70 -- rd debug reg0x01000000[1C].
            uint32_t reg = 0;
            reg |= pSysCmd->param[0] << 24;
            reg |= pSysCmd->param[1] << 16;
            reg |= pSysCmd->param[2] << 8;
            reg |= pSysCmd->param[3];
            INFO_PRT("rd reg 0x%08X: 0x%08X\n", reg, REG_READ(reg));
        }break;
        case 0x01://reg write
        {//exp : write reg val 0x12345678 to reg addr 0xAABBCCDD -> 01 E0 FC 0B(len) 2B 01 01(wr cmd) AA BB CC DD 12 34 56 78
         //template : 01 E0 FC 0B 2B 01 01 xx xx xx xx vv vv vv vv
         //demo: 01 E0 FC 0B 2B 01 01 01 00 00 70 11 22 33 44 -- wr 0x11223344 to debug reg0x01000000[1C].
            uint32_t reg = 0;
            uint32_t val = 0;
            reg |= pSysCmd->param[0] << 24;
            reg |= pSysCmd->param[1] << 16;
            reg |= pSysCmd->param[2] << 8;
            reg |= pSysCmd->param[3];
            val |= pSysCmd->param[4] << 24;
            val |= pSysCmd->param[5] << 16;
            val |= pSysCmd->param[6] << 8;
            val |= pSysCmd->param[7];
            uint32_t val_prv = REG_READ(reg);
            REG_WRITE(reg, val);
            INFO_PRT("wr reg 0x%08X: 0x%08X -> 0x%08X\n", reg, val_prv, REG_READ(reg));
        }break;
        
        default:
            break;
        }
        #endif
    }break;
    case SUB_CMD_SYS:
    {
    #ifdef CONFIG_DBG_LOG_FLASH_OP
        // PC->MCU : 01 E0 FC 03 2B 02 00/01/02    //cmd:00:show log, 01:save, 02:read
        uint32_t cmd = subCmd->param[0];
        INFO_PRT("CONFIG_DBG_LOG_FLASH_OP:%d\n", cmd);
        if(cmd <= 1)
        {
            //save log to flash
            extern void sys_debug_show(uint8_t redirect);
            sys_debug_show(!!cmd);

        }
        //read log from flash and output to uart
        else if(cmd == 2) {
            extern void dbg_log_flash_load(void);
            dbg_log_flash_load();
        }
    #else //show log
        extern void sys_debug_show(uint8_t redirect);
        sys_debug_show(0);
    #endif
    }break;
    case SUB_CMD_SYS_DPLL://0x08
    {
        // PC->MCU : 01 E0 FC 03 2B 08 01      DPLL
        uint8_t *p = &subCmd->param[0];
        if(p[0] == 0x01)
        {
            msg_put(MSG_TEMPRATURE_DPLL_TOGGLE);
            INFO_PRT("msg_put MSG_TEMPRATURE_DPLL_TOGGLE\n");
        }
    }
    break;
    case SUB_CMD_LOG_CTRL://0x03
    {
        uint8_t *p = (uint8_t*)subCmd->param;
        switch (p[0])
        {
        case 0x00://mcu log
        {// PC->MCU : 01 E0 FC 04 2B 03 00 01/00. //mcu log on/off
            uint8_t value = p[1];
            INFO_PRT("g_mcu_log_on_fg:%d\n", value);
            extern uint8_t g_mcu_log_on_fg;
            g_mcu_log_on_fg = value;
        }break;
        case 0x01://dsp log ctrl in mcu side
        {// PC->MCU :  01 E0 FC 04 2B 03 01 01/00. //dsp log on/off
            uint8_t value = p[1];
            INFO_PRT("g_dsp_log_on_fg:%d\n", value);
            extern uint8_t g_dsp_log_on_fg;
            g_dsp_log_on_fg = value;
        }
        break;
        case 0x02://dsp log ctrl in dsp side
        {// PC->MCU : 01 E0 FC 04 2B 03 02 00/05  00/01. //dsp (log on/off and sel) (dsp debug loop log en)
            // void dsp_log_ctrl(int log_sel, int loop_log_en);
            dsp_log_ctrl(p[1], p[2]);
        }
        break;
        case 0x03://bt audio sync
        {// PC->MCU : 01 E0 FC 04 2B 03 03 00/01      sync log on/off
            aud_sync_log_on_set(p[1]);
            INFO_PRT("aud_sync_log_is_on:0x%X\n", p[1]);
        }
        break;
        case 0x04://usb audio
        {// PC->MCU : 01 E0 FC 04 2B 03 04 00/XX  [off/on flag]
            extern void usbd_log_flag_set(uint32_t flag, uint8_t en);
            usbd_log_flag_set(0xFFFFFFFF, 0);
            if(p[1]) usbd_log_flag_set(p[1], 1);
            INFO_PRT("usbd_log_flag_set, en:%d\n", p[1]);
        }
        break;
    #ifdef CONFIG_APP_SPDIF
        case 0x05://spdif
        {// PC->MCU : 01 E0 FC 04 2B 03 05 00/XX  [off/on flag]
            extern void spdif_log_flag_set(uint32_t flag, uint8_t en);
            spdif_log_flag_set(0xFFFFFFFF, 0);
            if(p[1]) spdif_log_flag_set(p[1], 1);
            INFO_PRT("spdif_log_flag_set, en:%d\n", p[1]);
        }
        break;
    #endif
        case 0x80://sys log port ctrl
        {// PC->MCU : 01 E0 FC 04 2B 03 80 XX  [off/on flag]
            extern void sys_log_port_set(uint32_t flag, int en);
            extern uint32_t sys_log_port_get(uint32_t flag);
            INFO_PRT("sys_log_port:0x%X->0x%X\n", sys_log_port_get(SYS_LOG_PORT_ALL), p[1]);
            // sys_log_port_set(SYS_LOG_PORT_ALL, 0);
            if(p[1]) { sys_log_port_set(~p[1], 0); sys_log_port_set(p[1], 1); }
        }
        break;
        default:
            INFO_PRT("SUB_CMD_LOG_CTRL NULL:0x%X\n", p[0]);
            break;
        }
        // com_cmd_send_proc(buf, len);
    }break;

    case SUB_CMD_MODE_SW://0x04
    {
        // PC->MCU : 01 E0 FC 04 2B 04 00 00/xx     mode switch
        // PC->MCU : 01 E0 FC 04 2B 04 01 01/00     bt on/off
        uint8_t *p = &subCmd->param[0];
        if(p[0] == 0x00)
        {
            int system_work_mode_change_button(void);
            system_work_mode_change_button();
        }
        #ifndef CONFIG_BT_FUNC_INVALID
        else if(p[0] == 0x01)
        {
            extern void bt_mode_enter(void);
            extern void bt_mode_exit(void);
            if(p[1] == 0x01){
                bt_mode_enter();
            }else{
                bt_mode_exit();
            }
        }
        #endif
        INFO_PRT("SUB_CMD_MODE_SW, len:%d -> 0x%02X 0x%02X\n", len, p[0], p[1]);
    }
    break;
    case SUB_CMD_USB_SW://05
    {
        // PC->MCU : 01 E0 FC 03 2B 05 00      usb off
        // PC->MCU : 01 E0 FC 03 2B 05 01      usb on
        // extern void usb_join(void);
        // extern void usb_exit(void);
        // INFO_PRT("param[0]:0x%X\n", subCmd->param[0]);
        // if(subCmd->param[0] == 1){
        //     usb_join();
        // }else{
        //     usb_exit();
        // }
    }
    break;
#ifdef CONFIG_APP_PLAYER
    case SUB_CMD_PLAYER://06
    {
        // PC->MCU : 01 E0 FC 03 2B 06 00      play pause
        // PC->MCU : 01 E0 FC 03 2B 06 01      next music
        // PC->MCU : 01 E0 FC 03 2B 06 02      prev music
        // PC->MCU : 01 E0 FC 03 2B 06 03      next dir
        // PC->MCU : 01 E0 FC 03 2B 06 04      prev dir
        extern int app_player_button_play_pause(void);
        extern int app_player_button_next(void);
        extern int app_player_button_prev(void);
        extern int app_player_button_nextDir(void);
        extern int app_player_button_prevDir(void);
        uint8_t param = subCmd->param[0];
        INFO_PRT("app_player param[0]:0x%X\n", param);
        if(param == 0){
            app_player_button_play_pause();
        }else if(param == 1){
            app_player_button_next();
        }else if(param == 2){
            app_player_button_prev();
        }else if(param == 3){
            app_player_button_nextDir();
        }else if(param == 4){
            app_player_button_prevDir();
        }
    }
    break;
#endif
#ifdef CONFIG_APP_RECORDER
    case SUB_CMD_RECORDER://07
    {
        uint8_t *p = &subCmd->param[0];
        if(p[0] == 0 || p[0] == 1)
        {
            // PC->MCU : 01 E0 FC 04 2B 07 00 00/01      rec sdcard stop/start
            // PC->MCU : 01 E0 FC 04 2B 07 01 00/01      rec udisk stop/start
            extern void recorder_start(uint8_t disk_type, uint8_t en);
            extern uint8_t recorder_is_working(void);
            extern uint8_t rec_disk_type_get(void);
            uint8_t disk_type = p[0];
            uint8_t enable = p[1];
            if(len < 3) { INFO_PRT("rec param size error\n"); return; }
            INFO_PRT("recoder working:%d, disk_type:%d, enable:%d\n", recorder_is_working(), disk_type, enable);
            recorder_start(disk_type, enable);
        }
        else if(p[0] == 3)
        {
            // PC->MCU : 01 E0 FC 04 2B 07 03 xx      rec play index
            extern void rec_file_play_by_idx(uint16_t rec_file_idx);
            uint8_t rec_file_index = p[1];
            rec_file_play_by_idx(rec_file_index);
            INFO_PRT("rec play index:%d\n", rec_file_index);
        }
        else if(p[0] == 4)
        {
            // PC->MCU : 01 E0 FC 04 2B 07 03 xx      rec show name by index xx
            extern uint8_t rec_file_name_get_by_idx(uint16_t rec_file_idx, char* fn);
            uint8_t rec_file_index = p[1];
            char fn[64];
            rec_file_name_get_by_idx(rec_file_index, fn);
            INFO_PRT("rec [%d] name:%s\n", rec_file_index, fn);
        }
    }
    break;
#endif
#if 1
    case SUB_CMD_USB_AUDIO://0x09
    {
        uint8_t *p = &subCmd->param[0];
        INFO_PRT("SUB_CMD_USB_AUDIO:0x%X\n", p[0]);
        if(p[0] == 0)
        {
        }
        else if(p[0] == 1)
        {
            // PC->MCU : 01 E0 FC 05 2B 09 01 00 00~02         usb audio 0:mute, 1:vol+, 2:vol-
            // PC->MCU : 01 E0 FC 05 2B 09 01 01 00~05         usb audio 0:play, 1:stop, ...5:rec
            // PC->MCU : 01 E0 FC 05 2B 09 01 02 01            usb audio 
            uint8_t idx = p[1];
            uint8_t CSM_CMD = 1 << p[2]; //refer CSM_PLAY , CSM_STOP
            if(idx >= 2) CSM_CMD = p[2];
            csm_hid_key_send(idx, CSM_CMD);
            // csm_hid_key_send(0, CSM_HID_END); 
            INFO_PRT("CSM_CMD %d:0x%X\n", idx, CSM_CMD);
        }
        else if(p[0] == 2)
        {
            // PC->MCU : 01 E0 FC 04 2B 09 02 00        usb audio out L vol get
            INFO_PRT("usb in dB LR: %.1f %.1f\n", usb_in_vol_dB_L_get(), usb_in_vol_dB_R_get());
            INFO_PRT("usb out dB LR: %.1f %.1f\n", usb_out_vol_dB_L_get(), usb_out_vol_dB_R_get());
            INFO_PRT("usb in max:%d LR: %d %d\n", usb_in_vol_idx_max_get(), usb_in_vol_idx_L_get(), usb_in_vol_idx_R_get());
            INFO_PRT("usb out max:%d LR: %d %d\n", usb_in_vol_idx_max_get(), usb_out_vol_idx_L_get(), usb_out_vol_idx_R_get());
        }
        else if(p[0] == 3)//vol adj test
        {
            // PC->MCU : 01 E0 FC 04 2B 09 03 00~02        usb audio vol, 0:mute, 1:vol+, 2:vol-
            uint8_t cmd = p[1];
            if(cmd == 0) usb_out_vol_mute();
            else if(cmd == 1) usb_out_vol_inc();
            else if(cmd == 2) usb_out_vol_dec();
            INFO_PRT("vol cmd:%d\n", cmd);
        }
    }
    break;
#endif
    default: break;
    }
}

extern uint32_t XVR_reg_0x24_save;
extern uint8_t edr_tx_edr_delay, edr_rx_edr_delay;


uint32_t bkreg_process(uint8_t* buffer, uint32_t length)
{
    HCI_COMMAND_PACKET* hci_cmd = (HCI_COMMAND_PACKET*)buffer;

    #if PTS_TESTING
    if ( (hci_cmd->code       == TRA_HCIT_COMMAND)
      && (hci_cmd->opcode.ogf == PTS_TESTING_OGF)
      && (hci_cmd->opcode.ocf == BEKEN_OCF)
      && (length              == (HCI_COMMAND_HEAD_LENGTH+hci_cmd->total))) {
        /*******************************************
         * uart cmd: 0x01 e0 40 01 10
         *                         10~4F for hfp
         *                         50~8F for a2dp
         *******************************************/
        pts_entry(hci_cmd->cmd);
        goto ret;
    }
    #endif

    if (   (hci_cmd->code       != TRA_HCIT_COMMAND)
        || (hci_cmd->opcode.ogf != VENDOR_SPECIFIC_DEBUG_OGF)
        || (hci_cmd->opcode.ocf != BEKEN_OCF)
        || (length              != (HCI_COMMAND_HEAD_LENGTH+hci_cmd->total)))
        goto ret;

    switch (hci_cmd->cmd)
    {
    case SUB_CMD_RCV_ENTRY:
    {
        //01 E0 FC xx 2B xx xx ...(code[0x01] opcode[0xE0FC] len[xx] user_cmd[0x2B] data0[xx] data1[xx] ...)
        sub_cmd_proc((uint8_t*)hci_cmd->param, hci_cmd->total - 1);//rcv data
        goto ret;
    }
    case BEKEN_UART_LINK_CHECK:
        pHCItxBuf->total = length;
        memcpy(pHCItxBuf->param, uart_rx_buf, pHCItxBuf->total);
        break;

    case BEKEN_UART_REGISTER_WRITE_CMD:
        {
            int reg_index;
            REGISTER_PARAM *rx_param        = (REGISTER_PARAM *)hci_cmd->param;
            REGISTER_PARAM *tx_param        = (REGISTER_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];

            pHCItxBuf->total                = length-1;
            memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
            pHCItxBuf->param[3]             = hci_cmd->cmd;
            tx_param->addr                  = rx_param->addr;
            tx_param->value                 = rx_param->value;
            *(volatile unsigned int *)rx_param->addr = rx_param->value;
            reg_index                       = (rx_param->addr-MDU_XVR_BASE_ADDR)/4;
            if ((reg_index>=0) && (reg_index<=0x0f))
                XVR_analog_reg_save[reg_index] = rx_param->value;
            if(reg_index == 0x24)
                XVR_reg_0x24_save = rx_param->value;
            if(reg_index == 0x27)
            {
                edr_tx_edr_delay = (rx_param->value >> 12) & 0x0f;
                //edr_rx_edr_delay = 1;
                edr_rx_edr_delay = (rx_param->value >> 8) & 0x0f;;
            }
        }
        break;

    case BEKEN_UART_REGISTER_READ_CMD:
        {
            int reg_index;
            REGISTER_PARAM *rx_param = (REGISTER_PARAM *)hci_cmd->param;
            REGISTER_PARAM *tx_param = (REGISTER_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];

            pHCItxBuf->total         = HCI_EVENT_HEAD_LENGTH+length;
            memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
            pHCItxBuf->param[3]      = hci_cmd->cmd;
            tx_param->addr           = rx_param->addr;
            reg_index                = (rx_param->addr-MDU_XVR_BASE_ADDR)/4;
            if ((reg_index>=0) && (reg_index<=0x0f))
                tx_param->value        = XVR_analog_reg_save[reg_index];
            else
                tx_param->value        = *(volatile unsigned int *)rx_param->addr;
        }
        break;

    case BEKEN_FLASH_READ_CMD:
        {
            FLASH_PARAM *rx_param = (FLASH_PARAM *)hci_cmd->param;
            FLASH_PARAM *tx_param = (FLASH_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];
            pHCItxBuf->total         = HCI_COMMAND_HEAD_LENGTH+sizeof(FLASH_PARAM)+rx_param->len;
            memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
            pHCItxBuf->param[3]      = hci_cmd->cmd;
            memcpy((unsigned char *)tx_param, (unsigned char *)rx_param, sizeof(FLASH_PARAM));
            flash_read_data(tx_param->data, tx_param->addr, tx_param->len);
        }
        break;

    case BEKEN_FLASH_WRITE_CMD:
        {
            FLASH_PARAM *rx_param = (FLASH_PARAM *)hci_cmd->param;
            FLASH_PARAM *tx_param = (FLASH_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];
            pHCItxBuf->total                = HCI_COMMAND_HEAD_LENGTH+sizeof(FLASH_PARAM);
            memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
            pHCItxBuf->param[3]             = hci_cmd->cmd;
            memcpy((unsigned char *)tx_param, (unsigned char *)rx_param, sizeof(FLASH_PARAM));
            flash_write_data(rx_param->data, rx_param->addr, rx_param->len);
        }
        break;

    case BEKEN_FLASH_ERASE_CMD:
        {
            FLASH_PARAM *rx_param = (FLASH_PARAM *)hci_cmd->param;
            FLASH_PARAM *tx_param = (FLASH_PARAM *)&pHCItxBuf->param[HCI_COMMAND_HEAD_LENGTH];
            pHCItxBuf->total                = HCI_COMMAND_HEAD_LENGTH+sizeof(tx_param->addr);
            memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
            pHCItxBuf->param[3]             = hci_cmd->cmd;
            memcpy((unsigned char *)tx_param, (unsigned char *)rx_param, sizeof(FLASH_PARAM));
            flash_erase_sector(rx_param->addr, FLASH_ERASE_4K);
        }
        break;

    case BEKEN_SHOW_STACK_CMD:
        app_debug_showstack();
        goto ret;

    case BEKEN_DUMP_ENV_CMD:        
        app_env_dump();
        goto ret;

    case BEKEN_CLEAR_LINKKEY_CMD:
        flash_erase_sector(FLASH_ENVDATA_DEF_ADDR, FLASH_ERASE_4K);
        break;

    case BEKEN_SHOW_BT_STATUS:
        app_bt_status_show();
        #ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
        security_control_cp_support_print();
        #endif
        goto ret;

    #ifdef BEKEN_DEBUG

    case BEKEN_SHOW_BT_DEBUG:
        app_bt_debug_show();
        goto ret;
    case BEKEN_LED_EQ_BUTTON:
		if(pHCIrxBuf->param[0]==0xff)
			app_led_dump();
#ifdef CONFIG_APP_EQUANLIZER
	#if(CONFIG_PRE_EQ==1||CONFIG_HFP_SPK_EQ==1||CONIFG_HFP_MIC_EQ==1)
		else if(pHCIrxBuf->param[0]==0xfe)
		{
			if(pHCIrxBuf->total>=7)
			{
				if(pHCIrxBuf->param[5]==0x00)//AUD
				{
					sw_eq_flag = 1;
				#if (CONFIG_PRE_EQ== 1) 
					app_set_eq_gain_enable(&(pHCIrxBuf->param[1]));
				#endif
				}
				else if(pHCIrxBuf->param[5]==0x01)//SPK
				{
					sw_eq_flag = 2;
				#if (CONFIG_HFP_SPK_EQ== 1) 
					app_set_spk_eq_gain_enable(&(pHCIrxBuf->param[1]));
				#endif
				}
				else if(pHCIrxBuf->param[5]==0x02)//MIC
				{
					sw_eq_flag = 3;
				#if (CONIFG_HFP_MIC_EQ== 1) 
					app_set_mic_eq_gain_enable(&(pHCIrxBuf->param[1]));
			#endif
				}
				else
					sw_eq_flag = 0;
			}
		}
		else if(pHCIrxBuf->param[0]==0xfa)
		{
			if(sw_eq_flag==1)
			{
			#if (CONFIG_PRE_EQ== 1) 
				app_set_pre_eq(&pHCIrxBuf->param[1]);
			#endif
			}
			else if(sw_eq_flag==2)
			{
			#if (CONFIG_HFP_SPK_EQ== 1) 
				app_set_hfp_spk_eq(&pHCIrxBuf->param[1]);
			#endif	
			}
			else if(sw_eq_flag==3)
			{
			#if (CONIFG_HFP_MIC_EQ== 1) 
				app_set_hfp_mic_eq(&pHCIrxBuf->param[1]);
			#endif	
			}
		}
		else if(pHCIrxBuf->param[0]==0xfb)
		{
		#if (CONFIG_PRE_EQ== 1) 
			app_show_pre_eq();
		#endif
		#if (CONFIG_HFP_SPK_EQ== 1) 	
			app_show_hfp_spk_eq();
		#endif
		#if (CONIFG_HFP_MIC_EQ== 1) 	
			app_show_hfp_mic_eq();
		#endif	
		}
		else if(pHCIrxBuf->param[0]==0xfc)
		{
			if(sw_eq_flag==1)
			{
			#if (CONFIG_PRE_EQ== 1) 
				app_set_pre_eq_gain(&pHCIrxBuf->param[1]);
			#endif
			}
			else if(sw_eq_flag==2)
			{
			#if (CONFIG_HFP_SPK_EQ== 1) 
				app_set_hfp_spk_eq_gain(&pHCIrxBuf->param[1]);
			#endif	
			}
			else if(sw_eq_flag==3)
			{
			#if (CONIFG_HFP_MIC_EQ== 1) 
				app_set_hfp_mic_eq_gain(&pHCIrxBuf->param[1]);
			#endif	
			}
		}
		else if(pHCIrxBuf->param[0]==0xfd)
			app_bt_flag2_set(APP_FLAG2_SW_MUTE,pHCIrxBuf->param[1]);
	#endif
#endif
		else
			app_button_sw_action(pHCIrxBuf->param[0]);
        goto ret;

    #ifdef CONFIG_APP_AEC
    case BEKEN_SET_AEC_PARA:
        app_aec_set_params(&hci_cmd->param[0]);
        goto ret;
    #endif

    #endif

    case BEKEN_PRINT_LINK_KEY:
        app_print_linkkey();
        goto ret;

    case BEKEN_ENTRY_DUT_MODE:
        #if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
        //01E0FC05AF00000000
        //app_bt_enable_dut_mode(hci_cmd->param[0]);
        bt_app_entity_set_event(0, SYS_BT_DUT_MODE_EVENT);
        #endif
        goto ret;
    case BEKEN_ENTRY_BLE_DUT_MODE:
	    //01E0FC02b001
        #if BLE_DUT_TEST
        app_bt_flag1_set(APP_FLAG_DUT_MODE_ENABLE, hci_cmd->param[0]);  // 00:Exit BLE dut mode; 01:Enter BLE dut mode
        rw_uart_ble_dut_mode_set(hci_cmd->param[0]);
        #endif
        goto ret;
    case BEKEN_ENTRY_FCC_TESTMODE:
        // 01 E0 FC 03 FC XX YY
        app_bt_enable_fcc_mode(hci_cmd->param[0],hci_cmd->param[1],hci_cmd->param[2]);
        pHCItxBuf->total = uart_rx_index;
        memcpy(pHCItxBuf->param, uart_rx_buf, pHCItxBuf->total);
        goto ret;
    case BEKEN_CMD_LPBG_CALI: {
        LPBG_calibration();
        goto ret;
        }
    case BEKEN_CMD_WRITE_CALI:
        if(pHCIrxBuf->total == 1)
        {
            write_cali_result();
            goto ret;
        }
        else if(pHCIrxBuf->total == 2)
        {
            if(pHCIrxBuf->param[0] == 0xF0/*Frequency Offset*/)
            {
                uint8_t fo = REG_SYSTEM_0x4D;//system_get_0x4d_reg();
                app_env_t* env_h = app_env_get_handle();
			    env_h->env_data.offset_bak_tester = fo | 0x80;
                app_env_write_action(&(env_h->env_data.default_btaddr), 0);

                pHCItxBuf->total = HCI_COMMAND_HEAD_LENGTH + 2;
                memcpy(pHCItxBuf->param, uart_rx_buf, HCI_EVENT_HEAD_LENGTH);
                pHCItxBuf->param[3] = pHCIrxBuf->cmd;
                pHCItxBuf->param[4] = pHCIrxBuf->param[0];
                pHCItxBuf->param[5] = fo;
                break;
            }
            else
            {
                goto ret;
            }
        }
        else
        {
            goto ret;
        }

    case BEKEN_ENTRY_NON_SIG_TESTMODE:
    {
        t_TestControl tc_contents;
        LSLCstat_Reset_Monitors();
        memcpy((uint8 *)&tc_contents,(uint8 *)&pHCIrxBuf->param[0],sizeof(t_TestControl));
        app_bt_enable_non_signal_test(&tc_contents);
        pHCItxBuf->total = uart_rx_index;
        memcpy(pHCItxBuf->param, uart_rx_buf, pHCItxBuf->total);
        break;
    }	
    #if (DEBUG_BASEBAND_MONITORS == 1)
    case BEKEN_READ_BASEBAND_MONITORS:{
        if(app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE))
            TC_Read_Non_Signal_Test_Monitors();
        else
            TC_Read_Local_Baseband_Monitors_Via_App();
        goto ret;
    }
    case BEKEN_RESET_BASEBAND_MONITORS:{
        LSLCstat_Reset_Monitors();
        goto ret;
    }
    #endif

    #if (DEBUG_SCATTERNET_MONITORS == 1)
    case BEKEN_READ_SCATTERNET_MONITORS:
        TC_Read_Local_LMPconfig_Monitors_Via_App();
        goto ret;
    #endif

    #if (DEBUG_AGC_MODE_CHANNEL_ASSESSMENT == 1)
    case BEKEN_AGC_MODE_PARAM:
        _LSLCacc_Read_AGC_Param();
        goto ret;
    #endif

    case BEKEN_TEMP1_CMD:
        a2dp_cmd_disconnect();
        goto ret;
    case BEKEN_TEMP2_CMD:
        hf_cmd_disconnect();
        goto ret;
    case BEKEN_TEMP3_CMD:
        spp_slc_disconnect();
        goto ret;
    #ifdef CONFIG_PRODUCT_TEST_INF
    case BEKEN_RSSI_CMD:
        os_printf("rssi:0x%d,freqoffset:0x%x\r\n",aver_rssi,aver_offset);
        goto ret;
    case BEKEN_TEST_SPP_AUDIOCONN:
        spp_send("AT+SPPPTT=1\r\n",13);
        goto ret;
    case BEKEN_TEST_SPP_AUDIODISCON:
        spp_send("AT+SPPPTT=0\r\n",13);
        goto ret;
    #else
    case BEKEN_RSSI_CMD:
        goto ret;
    #endif

    case BEKEN_SHOW_SYSTEM_INFO:
        os_printf("BEKEN_SHOW_SYSTEM_INFO.\r\n");
        show_bt_stack_status();
        goto ret;

    #if A2DP_ROLE_SOURCE_CODE
    case BEKEN_CMD_SDP_CONNECT:
        set_sdp_browse_on();
        app_bt_sdp_connect();
        goto ret;

    case BEKEN_CMD_SERVICE_SEARCH_REQUEST:
        sdp_send_serviceSearchRequest();
        goto ret;

    case BEKEN_CMD_SERVICE_ATTRIBUTE_REQUEST:
        app_bt_sdp_send_serviceAttributeRequest();
        goto ret;

    case BEKEN_CMD_A2DP_SSA_REQUEST:
        {
            /* for a2dp sdp_query request. */
            extern result_t send_a2dp_ssa_request(void);
            send_a2dp_ssa_request();
        }
        goto ret;

    case BEKEN_CMD_HFP_SSA_REQUEST:
        {
            /* for hfp sdp_query request. */
            extern result_t send_hfp_ssa_request(void);
            send_hfp_ssa_request();
        }
        goto ret;
    #endif

    #if CHARGER_VERIFY
    case BEKEN_CMD_CHARGE_VERIFY_V4P25:
        {
            uint8_t result = 0;
            charger_vlcf_vcv_set(0);  // vusb = 5v, vbat= 4.25v
            result = charger_vlcf_vcv_result(0);
            if(result < 16)
            {
                charger_vlcf_vcv_set(1);
                result = charger_vlcf_vcv_result(1);
                if(result>16)
                    os_printf("charger calibration check is ok\r\n");
            }
            if(result<=16)
                os_printf("charger calibration check is NOT ok\r\n");
            goto ret;
        }
    case BEKEN_CMD_CHARGE_VERIFY_V4P15:
        {
            uint8_t result = 0;
            charger_vlcf_vcv_set(0);  // vusb = 5v, vbat= 4.15v
            result = charger_vlcf_vcv_result(0);
            if(result < 16)
            {
                charger_vlcf_vcv_set(1);
                result = charger_vlcf_vcv_result(1);
                if(result<16)
                    os_printf("charger calibration check is ok\r\n");
            }
            if(result>=16)
                os_printf("charger calibration check is NOT ok\r\n");
            goto ret;
        }
    case BEKEN_CMD_CHARGE_CALIBRATION:
        calib_dbg_enable(1);
        charger_vlcf_calibration(0);
        charger_vcv_calibration(0); 
        calib_dbg_enable(0);
        goto ret;

    case BEKEN_CMD_CHARGE_WRITE_RERSULT:
        goto ret;
    #endif

    case BEKEN_CMD_SYS_RESET:
    	//===============================================
    	if(g_mcu_log_on_fg==0) goto ret;	//yuan++
        /* watchdog reset for uart download */
        if(hci_cmd->total >= 5)
        {
            uint32_t param = hci_cmd->param[0]<<24 | hci_cmd->param[1]<<16 | hci_cmd->param[2]<<8 | hci_cmd->param[3];
            if(param == 0x95279527)
            {
                /* watchdog reset */
                //REG_WDT_0x00 = 0x5A0001;
                //REG_WDT_0x00 = 0xA50001;
                dsp_shutdown();	//yuan++
                REG_SYSTEM_0x1D &= ~0x2;
                BK3000_wdt_reset();
            }
        }
        goto ret;

    case BEKEN_TRANSPARENT_HCI_CMD:
        if((hci_cmd->total > 4) &&(hci_cmd->total - hci_cmd->param[3] == 0x05))
        {
            uart_send_poll((uint8_t *)hci_cmd->param, hci_cmd->total-1);
        }
        goto ret;

    #if A2DP_ROLE_SOURCE_CODE
    case BEKEN_CMD_A2DP_SRC_CONN_REMOTE_DEVICE:
        {
            /* for a2dpSrc connect remote device. */
            os_printf("BEKEN_CMD_A2DP_SRC_CONN_REMOTE_DEVICE, %d\r\n",hci_cmd->total);

            btaddr_t remote_btaddr_def2 = {{0x86, 0x19, 0x36, 0x33, 0x22, 0x11}};

            if(hci_cmd->total >= 7) {
                uint8_t *input = hci_cmd->param;
                os_printf("A2DP_SRC_CONN_REMOTE:%02x:%02x:%02x:%02x:%02x:%02x\r\n",input[5],input[4],input[3],input[2],input[1],input[0]);
                memcpy((uint8_t *)&remote_btaddr_def2,(uint8_t *)input,sizeof(btaddr_t));
            }

            if(get_a2dp_role() == A2DP_ROLE_AS_SRC)
                a2dpSrcConnectRemoteDevice(&remote_btaddr_def2);
            else if(get_a2dp_role() == A2DP_ROLE_AS_SNK)
                a2dpSnkConnectRemoteDevice(&remote_btaddr_def2);
        }
        goto ret;
    #endif
	
    case BEKEN_SDIO_TEST:			//01 E0 FC 01 C2
        #ifdef TEST_SDCARD_RW
        sd_test_write();
        #endif
        goto ret;

	#if	CONFIG_USE_USB
    case BEKEN_USB_DEVICE_TEST:
    {   // 01 E0 FC 02 C3 02 DEVICE
        // 01 E0 FC 02 C3 01 HOST
        goto ret;
    }
	#endif
	
    case BEKEN_CMD_APPEND_TLV: {
        uint16 type = (uint16)(pHCIrxBuf->param[2]<<8) | pHCIrxBuf->param[1];
        uint16 len = (uint16)(pHCIrxBuf->param[4]<<8) | pHCIrxBuf->param[3];
        app_append_tlv_data(pHCIrxBuf->param[0],type,&pHCIrxBuf->param[5],len);
        goto ret;
        }
    #if (CONFIG_ANC_ENABLE == 1)
    case BEKEN_ANC_DBG_CMD:
        {
            int app_anc_debug(HCI_COMMAND_PACKET*, HCI_EVENT_PACKET*);
            if(0 == app_anc_debug((HCI_COMMAND_PACKET*)buffer, pHCItxBuf)) goto ret;
        }
        break;
    #endif
    #if (CONFIG_BLUETOOTH_PBAP)
    case PHONE_BOOK_PULL_BOOK:
        {
            LOG_I(PBAP, "pull book %d %d %d %d\r\n"
                , pHCIrxBuf->param[0], pHCIrxBuf->param[1], pHCIrxBuf->param[2], pHCIrxBuf->param[3]);
            app_handle_t sys_hdl = app_get_sys_handler();
            pbap_pull_phonebook(&sys_hdl->remote_btaddr, pHCIrxBuf->param[0], pHCIrxBuf->param[1], pHCIrxBuf->param[2], pHCIrxBuf->param[3]);
        }
        goto ret;
    case PHONE_BOOK_PULL_SetPath:
        {
            LOG_I(PBAP, "set path %d %d\r\n", pHCIrxBuf->param[0], pHCIrxBuf->param[1]);
            app_handle_t sys_hdl = app_get_sys_handler();
            pbap_set_path(&sys_hdl->remote_btaddr, pHCIrxBuf->param[0], pHCIrxBuf->param[1]);
        }
        goto ret;
    case PHONE_BOOK_PULL_vCARD:
        {
            LOG_I(PBAP, "pull vCard %d\r\n", pHCIrxBuf->param[0]);
            app_handle_t sys_hdl = app_get_sys_handler();
            pbap_pull_vcard_entry(&sys_hdl->remote_btaddr, pHCIrxBuf->param[0]); 
        }
        goto ret;
    case PHONE_BOOK_PULL_vCARDList:
        {
            LOG_I(PBAP, "pbap vCardList %d %d %d %d\r\n", pHCIrxBuf->param[0], pHCIrxBuf->param[1], pHCIrxBuf->param[2], pHCIrxBuf->param[3]);
            app_handle_t sys_hdl = app_get_sys_handler();
            pbap_pull_vcard_list(&sys_hdl->remote_btaddr, pHCIrxBuf->param[0], pHCIrxBuf->param[1], pHCIrxBuf->param[2], pHCIrxBuf->param[3]);
        }
        goto ret;
    case PHONE_BOOK_CALL_LAST_vCard_Number:
        {
            #ifdef EXTRACT_VCARD
            extern result_t hf_cmd_place_call(char number[32]);
            extern char last_vCardTel[32];
            hf_cmd_place_call(last_vCardTel);
            #endif
        }
        goto ret;
    case PHONE_BOOK_PULL_PHONE_NUM_vCard:
    {
        LOG_I(PBAP, "pbap target phone num %d\r\n", length);
        app_handle_t sys_hdl = app_get_sys_handler();
        pbap_pull_target_vcard(&sys_hdl->remote_btaddr, length-5, pHCIrxBuf->param);
    }goto ret;
    #else
    case PHONE_BOOK_PULL_BOOK:
    case PHONE_BOOK_PULL_SetPath:
    case PHONE_BOOK_PULL_vCARD:
    case PHONE_BOOK_PULL_vCARDList:
    case PHONE_BOOK_CALL_LAST_vCard_Number:
    case PHONE_BOOK_PULL_PHONE_NUM_vCard:
        goto ret;
    #endif
    default:
        goto ret;
    }

    return 1;
ret:
    return 0;
}

void Beken_Uart_Rx(void)
{
    if((uart_rx_done != TRUE) || (uart_rx_index == 0)) return;

    if(bkreg_process((uint8_t*)uart_rx_buf, (uint32_t)uart_rx_index)) Beken_Uart_Tx();

    clear_uart_buffer();
}

void Beken_BTSPP_Rx(uint8_t* buffer, uint32_t length)
{
    if(bkreg_process(buffer, length))
    {
        result_t spp_send(char *buff, uint32_t len);
        uint32_t tx_len  = HCI_EVENT_HEAD_LENGTH+pHCItxBuf->total;
        pHCItxBuf->code  = TRA_HCIT_EVENT;
        pHCItxBuf->event = HCI_COMMAND_COMPLETE_EVENT;
        spp_send((char*)uart_tx_buf, tx_len);
    }
}

void dbg_hci_rx_char(uint8_t ch)
{
    switch(s_dbg_hci_rx_state)
    {
        case DBG_HCI_STATE_RX_DATA_CONTINUE:
__DBG_HCI_STATE_RX_DATA_CONTINUE:
            *s_dbg_hci_rx_pdu_buf = ch;
            s_dbg_hci_rx_pdu_buf++;
            s_dbg_hci_rx_pdu_length--;
            if(!s_dbg_hci_rx_pdu_length)
            {
                s_dbg_hci_rx_state = DBG_HCI_STATE_RX_DATA_COMMIT;
                goto __DBG_HCI_STATE_RX_DATA_COMMIT;
            }
            return;

        case  DBG_HCI_STATE_RX_TYPE:
            if(ch == 0x01)   // HCI_COMMAND
            {
                s_dbg_hci_rx_head_buf[0] = ch;
                s_dbg_hci_rx_state = DBG_HCI_STATE_RX_COMMAND_OPCODE1;
            }
            else
            {
            	#if ITEST_ENABLE
            	//add itest other cmd //220317
            	if(uart_rx_index > 16)
            	{
                	s_dbg_hci_rx_state = DBG_HCI_STATE_RX_TYPE;
                	uart_rx_index = 0;
                	uart_rx_done = FALSE;
            	}
				#else
				s_dbg_hci_rx_state = DBG_HCI_STATE_RX_TYPE;
                uart_rx_index = 0;
                uart_rx_done = FALSE;
				#endif
            }
            return;

        case DBG_HCI_STATE_RX_COMMAND_OPCODE1:
            s_dbg_hci_rx_head_buf[1] = ch;
            s_dbg_hci_rx_state = DBG_HCI_STATE_RX_COMMAND_OPCODE2;
            return;

        case DBG_HCI_STATE_RX_COMMAND_OPCODE2:
            s_dbg_hci_rx_head_buf[2] = ch;
            s_dbg_hci_rx_state = DBG_HCI_STATE_RX_COMMAND_LENGTH;
            return;

        case DBG_HCI_STATE_RX_COMMAND_LENGTH:
            s_dbg_hci_rx_head_buf[3] = ch;
            s_dbg_hci_rx_pdu_length = ch;
            s_dbg_hci_rx_length = 4 + ch;
            s_dbg_hci_rx_state = DBG_HCI_STATE_RX_DATA_START;

            if (s_dbg_hci_rx_pdu_length==0)
            {
                goto __DBG_HCI_STATE_RX_DATA_START;
            }
            return;

        case DBG_HCI_STATE_RX_DATA_START:
__DBG_HCI_STATE_RX_DATA_START:
            s_dbg_hci_rx_pdu_buf = (volatile uint8_t*) &uart_rx_buf[4];
            if((s_dbg_hci_rx_pdu_length == 0) || (s_dbg_hci_rx_pdu_length > 250))
            {
                s_dbg_hci_rx_state = DBG_HCI_STATE_RX_DATA_COMMIT;
                goto __DBG_HCI_STATE_RX_DATA_COMMIT;
            }
            s_dbg_hci_rx_state = DBG_HCI_STATE_RX_DATA_CONTINUE;
            goto __DBG_HCI_STATE_RX_DATA_CONTINUE;

        case DBG_HCI_STATE_RX_DATA_COMMIT:
__DBG_HCI_STATE_RX_DATA_COMMIT:
            uart_rx_done = TRUE;
            uart_rx_index = s_dbg_hci_rx_length;
            #if 0
            {
                uint8_t i;
                os_printf("===hci_dbg:\r\n");
                for(i=0;i<s_dbg_hci_rx_length;i++)
                {
                    os_printf("%02x,",uart_rx_buf[i]);
                }
                os_printf("===\r\n");
            }
            #endif
            
            #if BLE_DUT_TEST
            rw_uart_tl_rx_packet(uart_rx_buf, uart_rx_index);
            #endif
            
            Beken_Uart_Rx();
            s_dbg_hci_rx_state = DBG_HCI_STATE_RX_TYPE;
            return;
        default:
            return;
    }
    return;
}

#if ITEST_ENABLE
void hci_itest_done(uint8_t *buff, uint16_t len)
{
	if(itest_dut_flag == 1)
	{
		//要把这个极致汇仪产测的代码放在最前面因为在后面uart_rx_index会被清掉
		if((buff[0] == 0x01) && ((buff[3] + 4) == len))
		{
			if((buff[1] != 0xE0) && (buff[2] != 0xFC))
			{
				if(buff[1] == 0x1E)
				{
					dut_tx_test_res[0] = 's';
					dut_tx_test_res[1] = 'e';
					dut_tx_test_res[2] = 't';
					dut_tx_test_res[3] = ' ';
					dut_tx_test_res[4] = 'p';
					dut_tx_test_res[5] = 'w';
					dut_tx_test_res[6] = 'r';
					dut_tx_test_res[7] = ':';
					dut_tx_test_res[8] = '9';
					dut_tx_test_res[9] = ' ';
					dut_tx_test_res[10] = '-';
					dut_tx_test_res[11] = ' ';
					dut_tx_test_res[12] = 'c';
					dut_tx_test_res[13] = ':';
					xHexToStr( &dut_tx_test_res[14], &uart_rx_buf[4], 1 );
					dut_tx_test_res[16] = '\r';
					dut_tx_test_res[17] = '\n';
					uart_send(dut_tx_test_res, 18);
				}
			}
		}

		if((buff[0] == 0x01) &&(buff[1] == 0xe0)&& (buff[2] == 0xfc) &&(buff[3] == 0x02)&& (buff[4] == 0x0E) && (buff[5] == 0xA0))
		{
			itest_dut_flag = 0;
			uart_send(dut_quit_str, strlen((char *)dut_quit_str));
		}
	}
}
#endif

void dbg_log_rx_proc(uint8_t UARTx, uint8_t value)
{
    if(!g_mcu_log_on_fg) goto RET;

    if (BT_HOST_MODE == NONE_CONTROLLER)
        juart_receive(&value, 1);
    else if ((BT_HOST_MODE == JUNGO_HOST)) {
        #if ITEST_ENABLE
        uart_rx_buf_itest[uart_rx_index_itest++] = value;
        if (uart_rx_index_itest == RX_FIFO_THRD)
            uart_rx_index_itest = 0;
        uart_rx_done_itest = TRUE;;
        #endif
        dbg_hci_rx_char(value);
    }else{
        TRAhcit_Rx_Char(value);
    }
RET:
    if(UARTx == 0){
    #ifdef USER_UART_FUNCTION_DIRECT
        extern void com_cmd_recv_proc_direct(uint8_t value);
        com_cmd_recv_proc_direct(value);
    #endif
    }else if(UARTx == 1){
    #ifdef USER_UART1_FUNCTION_DIRECT	//yuan++
        extern void com1_cmd_recv_proc_direct(uint8_t value);
        com1_cmd_recv_proc_direct(value);
    #endif
    }else if(UARTx == 2){
    #ifdef USER_UART2_FUNCTION_DIRECT
        void com2_cmd_recv_proc_direct(uint8_t value);
        com1_cmd_recv_proc_direct(value);
    #endif
    }
    return;
}
void uart_handler(void)
{
    uint32_t status;
    uint8_t value;
    status = REG_UART0_INT_STATUS;
    // if(status & (MSK_UART0_INT_STATUS_RX_FIFO_NEED_READ|MSK_UART0_INT_STATUS_RX_STOP_END))
    if(ptn_uart_rx_ready(0, status))
    {
        while (REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_FIFO_RD_READY)
        {
            value = (REG_UART0_FIFO_PORT & MSK_UART0_FIFO_PORT_RX_FIFO_DOUT) >> 8;
            dbg_log_rx_proc(0, value);
        }
		
		#if ITEST_ENABLE
		extern struct uart_env_tag uart_env;
		if(uart_env.ble_dut_enable && g_mcu_log_on_fg)
		{
			if( status & MSK_UART0_INT_STATUS_RX_STOP_END)
			{
				//连接心跳
				if((uart_rx_buf_itest[0] == 0x0d) &&(uart_rx_buf_itest[1] == 0x0a)) ///keep con state rsp
				{
					//uart_printf_dut("\r\n# \r\n# ");
					os_printf_itest("\r\n# \r\n# ");
					itest_dut_flag = 1;
				}
				//读取mac
				else if(!memcmp(uart_rx_buf_itest,get_mac_addr_str, strlen((char *)get_mac_addr_str))) ///itest get mac 
				{
					#if 0
					//os_printf_itest("sys MAC:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
					//co_default_bdaddr.addr[0],co_default_bdaddr.addr[1],
					//co_default_bdaddr.addr[2],co_default_bdaddr.addr[3],
					//co_default_bdaddr.addr[4],co_default_bdaddr.addr[5]);
					#else
					os_printf_itest("sys MAC:11:22:33:44:55:66\r\n");
					#endif					
					os_printf_itest("\r\n# \r\n# ");
					//uart_rx_index_itest = 0;
				}
				//使能极致汇仪 dut测试
				else if(!memcmp(uart_rx_buf_itest,dut_test_start_str, strlen((char *)dut_test_start_str)))
				{
					//os_printf("rx blt dut\r\n");
					uart_send(dut_test_res_str, strlen((char *)dut_test_res_str)); ///rsp"enter ble dut"
					//uart_rx_index_itest = 0;
					itest_dut_flag = 1;  ///add 210810
				}
				//芯片重启
				else if(!memcmp(uart_rx_buf_itest,dut_reset_str, strlen((char *)dut_reset_str))) ///reset for write lic(midea)
				{
					//wdt_reset(10);		//write lic
					//wdt_enable(0x10);
					BK3000_wdt_reset();
					while(1);
				}
				//芯片重启
				else if(!memcmp(uart_rx_buf_itest,dut_reset2_str, strlen((char *)dut_reset2_str)))  ///reset for read lic(midea)
				{
					//wdt_reset(10);		//read lic
					//wdt_enable(0x10);
					BK3000_wdt_reset();
					while(1);
				}
				#if 1
				else if((uart_rx_buf_itest[0] == 'm') 
					     &&(uart_rx_buf_itest[1] == 'a') 
					     &&(uart_rx_buf_itest[2] == 'c') 
					     &&(uart_rx_buf_itest[3] == ' ')) ///set mac
				{
					//收到写mac的指令，要按如下回数据
					os_printf("set mac cmd\r\n");
					memcpy(set_mac_addr, &uart_rx_buf_itest[4], 12);
					set_mac_addr_flag = 1;
					uart_rx_buf_itest[16] = '\r';
					uart_rx_buf_itest[17] = '\n';
					uart_rx_buf_itest[18] = 0x00;
					uart_putchar_1((char * )uart_rx_buf_itest);
					//save mac	
					//ke_timer_set(APP_SET_MAC_TIMER, TASK_APP, 1);
				}
				#endif
			}
			uart_rx_index_itest = 0;	
		}
		#endif
	}
		
    REG_UART0_INT_STATUS = status;
}

//**********************************************************************************
void uart1_handler(void)
{
    uint32_t status;
    uint8_t value;
    status = REG_UART1_INT_STATUS;
    // if(status & (MSK_UART1_INT_STATUS_RX_FIFO_NEED_READ|MSK_UART1_INT_STATUS_RX_STOP_END))
    if(ptn_uart_rx_ready(1, status))
    {
        while (REG_UART1_FIFO_STATUS & MSK_UART1_FIFO_STATUS_FIFO_RD_READY)
        {
            value = (REG_UART1_FIFO_PORT & MSK_UART1_FIFO_PORT_RX_FIFO_DOUT) >> 8;
            dbg_log_rx_proc(1, value);
        }
    }
    REG_UART1_INT_STATUS = status;
}

//**********************************************************************************
void uart2_handler(void)
{
    uint32_t status = REG_UART1_INT_STATUS;
    if(ptn_uart_rx_ready(2, status))
    {
        uint8_t value;
        while (REG_UART2_FIFO_STATUS & MSK_UART2_FIFO_STATUS_FIFO_RD_READY)
        {
            value = (REG_UART2_FIFO_PORT & MSK_UART2_FIFO_PORT_RX_FIFO_DOUT) >> 8;
            dbg_log_rx_proc(2, value);
        }
    }
    REG_UART1_INT_STATUS = status;
}

//****************************************************************************
void show_bt_stack_status(void)
{
    os_printf("%s\r\n", __func__);

    os_printf("sizeof(struct m_hdr):%d, sizeof(struct mbuf):%d\r\n", sizeof(struct m_hdr), sizeof(struct mbuf));

    os_printf("BEKEN SW Compliled at %s, %s\r\n", __TIME__, __DATE__);
}

#ifdef BEKEN_DEBUG
#ifdef A2DP_SBC_DUMP_SHOW
extern void a2dp_sbc_info_show(void);
extern void sbc_encode_frame_info(void);
#endif

void app_bt_debug_show( void )
{
    extern int encode_pkts;
    extern int decode_pkts;
    extern int encode_buffer_full;
    extern int encode_buffer_empty;

#ifdef A2DP_SBC_DUMP_SHOW
    a2dp_sbc_info_show();
    sbc_encode_frame_info();
#endif

    os_printf("--------sbc decode statistic--------------\r\n");

    os_printf("| encode pkts: %d\r\n",encode_pkts);
    os_printf("| decode pkts: %d\r\n",decode_pkts);
    os_printf("| encode full: %d\r\n",encode_buffer_full);
    os_printf("|encode empty: %d\r\n",encode_buffer_empty);
    os_printf("| encode node: %d\r\n",sbc_buf_get_node_count());
    os_printf("--------sbc decode statistic--------------\r\n");

    os_printf("memory status: \r\n");
    memory_usage_show();
    os_printf("--------tick status----------------------\r\n");
    os_printf("|           sleep tick: %d\r\n", sleep_tick);
    os_printf("|       powerdown tick: %d\r\n", pwdown_tick);
    os_printf("| sniffmode_wakeup_dly: %d\r\n",sniffmode_wakeup_dly);
    os_printf("--------tick status----------------------\r\n");


    return;
}

void app_bt_debug_info_clear(void)
{
    extern int encode_pkts;
    extern int decode_pkts;
    extern int encode_buffer_full;
    extern int encode_buffer_empty;

    encode_pkts = 0;
    decode_pkts = 0;
    encode_buffer_full = 0;
    encode_buffer_empty = 0;
}
#endif

void app_bt_status_show( void )
{
    app_handle_t app_h = app_get_sys_handler();

    os_printf("%s()\n", __FUNCTION__);
    print_page_scan_status();
    /* bt_app_management */
    bt_app_entity_print_state();
    
    {
        uint32_t flag;
        uint8_t i;
        os_printf("------------------------------------------\r\n");
        for(i=0;i<BT_MAX_AG_COUNT;i++)
        {
        #ifdef CONFIG_BLUETOOTH_HFP
            flag = get_hf_priv_flag(i,0xffffffff);
            os_printf("|    hfp-private flag %d:%08x\r\n",i,flag);
        #endif
            flag = get_a2dp_priv_flag(i,0xffffffff);
            os_printf("|   a2dp-private flag %d:%08x\r\n",i,flag);
        }
        os_printf("------------------------------------------\r\n");
    }

    if( app_h->unit == NULL )
    {
        os_printf("Bluetooth device not initialized yet.\r\n");
        return;
    }
    #ifdef CONFIG_BLUETOOTH_HFP
    os_printf("     HFP flag:%08x\r\n",get_hf_priv_flag(0,0xffffffff));
    #endif
    os_printf("    a2dp flag:%08x\r\n",get_a2dp_priv_flag(0,0xffffffff));
    os_printf("Global Flag-1:%08x\r\n",app_h->flag_sm1);
    os_printf("Global Flag-2:%08x\r\n",app_h->flag_sm2);
#if(CONFIG_AUD_FADE_IN_OUT == 1)
    // extern void aud_fade_status_debug(void);
    // aud_fade_status_debug();
#endif
    extern uint8_t syspwr_cpu_halt;
    extern int SYSpwr_Is_Available_Sleep_System(void);
    extern BOOL USLCsleep_Is_Tabasco_Asleep(void);
    os_printf("Sleep is allowed:%d,%d,%d,%d\r\n",app_is_available_sleep_system(),syspwr_cpu_halt,SYSpwr_Is_Available_Sleep_System(),USLCsleep_Is_Tabasco_Asleep());
    os_printf("CPU irq mask:%08x\r\n",get_spr(SPR_VICMR(0)));

    os_printf("Device addr: "BTADDR_FORMAT"\r\n", BTADDR(&(app_h->unit->hci_btaddr)) );
    if( app_h->flag_sm1 & APP_FLAG_ACL_CONNECTION )
        os_printf("Remote device addr: "BTADDR_FORMAT"\r\n", BTADDR(&(app_h->remote_btaddr)) );
    else
    {
        os_printf("Not connect to other device. 0x%x \r\n", app_h->flag_sm1);
        return;
    }

    os_printf("A2DP status: %s\r\n",
              (app_h->flag_sm1 & APP_FLAG_A2DP_CONNECTION )?"connected":"disconnected");

    if( app_h->flag_sm1 & APP_FLAG_MUSIC_PLAY )
        os_printf("Audio stream started now.\r\n");

    os_printf("AVRCP status: %s\r\n",
              (app_h->flag_sm1 & APP_FLAG_AVCRP_CONNECTION )?"connected":"disconnected");

    #ifdef CONFIG_BLUETOOTH_HFP
    os_printf("HFP status: %s\r\n",
              (app_h->flag_sm1 & APP_FLAG_HFP_CONNECTION )?"connected":"disconnected");
    extern void hfp_app_ptr_debug_printf(void);
    hfp_app_ptr_debug_printf()
    #endif

}


#if 1
#ifdef CONFIG_DBG_LOG_FLASH_OP
#define DBG_LOG_BUF_SZ      4096
static int dbg_log_wp = 0;//buff write pos
static uint8_t dbg_log_buf[DBG_LOG_BUF_SZ];
//write log to 4K linear buff
void dbg_log_write_buff(uint8_t* buf, int size)
{
    int wr_pos = dbg_log_wp;
    if(wr_pos + size < DBG_LOG_BUF_SZ)
    {
        memcpy(&dbg_log_buf[wr_pos], (uint8_t*)buf, size);
        wr_pos += size;
    }
    else
    {
        int sub_size1 = DBG_LOG_BUF_SZ - 1 - wr_pos;//write buf to full
        memcpy(&dbg_log_buf[wr_pos], (uint8_t*)buf, sub_size1);
        #if 0
        wr_pos = 0;
        #else
        int sub_size2 = size - sub_size1;//write remain data to buf header
        memcpy(&dbg_log_buf[0], (uint8_t*)buf, sub_size2);
        wr_pos = sub_size2;
        #endif
    }
    dbg_log_wp = wr_pos;
}

void dbg_log_flash_save(void)
{
    os_printf("%s()\n", __FUNCTION__);
    #define DBG_LOG_SAVE_FLASH_ADDR      0x1FD000 //ota info addr, refer to bootloader.c
    flash_erase_sector(DBG_LOG_SAVE_FLASH_ADDR, FLASH_ERASE_4K);
    flash_write_data((uint8_t*)&dbg_log_buf, DBG_LOG_SAVE_FLASH_ADDR, DBG_LOG_BUF_SZ);
}

void dbg_log_flash_load(void)
{
    os_printf("\n\n\n\n%s() enter\n", __FUNCTION__);
    flash_read_data((uint8_t*)dbg_log_buf, DBG_LOG_SAVE_FLASH_ADDR, DBG_LOG_BUF_SZ);
    for(int i = 0; i < DBG_LOG_BUF_SZ; i++) os_printf("%c", dbg_log_buf[i]);
    os_printf("%s() exit\n\n\n\n", __FUNCTION__);
}
#endif

void mailbox_debug_show(void)
{
    uint32_t buf[9];
    uint32_t *dat = &buf[0];
    uint32_t *rsp = &buf[5];
    
//mcu2dsp send info
    dat[0] = REG_MBOX0_MAIL0;
    dat[1] = REG_MBOX0_0x01 ;
    dat[2] = REG_MBOX0_0x02 ;
    dat[3] = REG_MBOX0_0x03 ;
    dat[4] = REG_MBOX0_READY;
//dsp2mcu rsp
    rsp[0] = REG_MBOX1_MAIL1;
    rsp[1] = REG_MBOX1_0x05;
    rsp[2] = REG_MBOX1_0x06;
    rsp[3] = REG_MBOX1_0x07;
    os_printf("mailbox info:");
    int i; for(i = 0; i < 9; i++) os_printf("0x%08X ");
    os_printf("\n");
}
#include "bt_a2dp_mpeg_aac_decode.h"
extern uint32_t a2dp_get_codec_type(void);

//redirect:  1->flash save(log to buff and save to flash)
void sys_debug_show(uint8_t redirect)
{
    static uint32_t show_cnt = 0;
/////////////////// 将log打印到4K的buff
    show_cnt++;
    os_printf("\n\n\n\n >>>>>>>> %s() enter times:%d @ %dms\n", __FUNCTION__, show_cnt, sys_time_get());
    os_printf("Partner SW Compliled @ %s %s\n", __TIME__, __DATE__ );

    mailbox_debug_show();
    dbg_show_dynamic_mem_info(1);

    uint16_t tempe_data[3];
    temperature_saradc_data_get(&tempe_data[0], 3);
    os_printf("tempe_data:%d, %d, %d\n", tempe_data[0], tempe_data[1], tempe_data[2]);

    //audio
    os_printf("\n");
    os_printf("as fg in:0x%08X, out:0x%08X, dac_free:%d\n", audio_asi_type_get(-1), audio_aso_type_get(-1), aud_dac_get_free_buffer_size());
    os_printf("as fs:%d, bt mode:%d, is sbc:%d\n", aud_dac_sample_rate_cur_get(), app_is_bt_mode(), a2dp_get_codec_type());
    extern app_sbc_t app_sbc;
    os_printf("as node:%d\n", (a2dp_get_codec_type() == 1) ? ring_buffer_node_get_fill_nodes(&aac_frame_nodes) : app_sbc.sbc_ecout);

    //sdcard
    os_printf("\n");
    os_printf("sd_host_state:%d\n", sdio_host_state_get());

    //bt
    os_printf("\n");
    app_bt_status_show();

    //dsp
//    os_printf("\n");
 //   uint32_t dsp_drv_ver_get(void);
 //  os_printf(" dsp_drv:0x%X\n", dsp_drv_ver_get());
#ifdef CONFIG_DBG_LOG_FLASH_OP
    os_printf(" <<<<<<<< %s() exit, pos:%d\n\n\n\n", __FUNCTION__, dbg_log_wp);
#else
    os_printf(" <<<<<<<< %s() exit\n\n\n\n", __FUNCTION__);
#endif

#ifdef CONFIG_DBG_LOG_FLASH_OP
/////////////////// 将存有log的4K数据写到flash中， 在系统死机前保存这些信息
    if(redirect == 1) dbg_log_flash_save();
#endif
}
#endif


