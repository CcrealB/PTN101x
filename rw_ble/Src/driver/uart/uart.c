/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART driver
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup UART
 * @{
 ****************************************************************************************
 */ 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stddef.h>     // standard definition
//#include "timer.h"      // timer definition
#include "uart.h"       // uart definition
//#include "reg_uart.h"   // uart register
#include "rwip.h"       // SW interface
#include "h4tl.h"
#include "nvds.h"       // NVDS

#include "dbg.h"
#include  <stdarg.h>
#include  <stdio.h>
#include <string.h>
//#include "ea.h"
#include "bkreg.h"
#include "arch.h"

#include "user_config.h"
#include "arch.h"
#include "h4tl.h"
#include "co_list.h"
#include "ke_event.h"
#include "ke_mem.h"

/*
 * DEFINES
 *****************************************************************************************
 */

/// Max baudrate supported by this UART (in bps)
#define UART_BAUD_MAX      		  3500000
/// Min baudrate supported by this UART (in bps)
#define UART_BAUD_MIN      		  9600

/// Duration of 1 byte transfer over UART (10 bits) in us (for 921600 default baudrate)
#define UART_CHAR_DURATION        11

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */
 
 //extern void uart_stack_register(void *cb);
 
 /*
 * common.c
 *
 *  Created on: 2013-5-7
 *  Author: pujie
 */



#define bit_UART_TXFIFO_NEED_WRITE        0x01
#define bit_UART_RXFIFO_NEED_READ         0x02
#define bit_UART_RXFIFO_OVER_FLOW         0x04
#define bit_UART_RX_PARITY_ERROR          0x08
#define bit_UART_RX_STOP_ERROR            0x10
#define bit_UART_TX_PACKET_END            0x20
#define bit_UART_RX_PACKET_END            0x40
#define bit_UART_RXD_WAKEUP_DETECT        0x80


#define uart_tx_en    0x1      // 0: Disable TX, 1: Enable TX 
#define uart_rx_en    0x1      // 0: Disable RX, 1: Enable RX
#define irda_mode     0x0      // 0: UART  MODE, 1: IRDA MODE
#define data_len      0x3      // 0: 5 bits, 1: 6 bits, 2: 7 bits, 3: 8 bits
#define parity_en     0x0      // 0: NO Parity, 1: Enable Parity
#define parity_mode   0x1      // 0: Odd Check, 1: Even Check 
#define stop_bits     0x0      // 0: 1 stop-bit, 1: 2 stop-bit 
#define uart_clks     16000000 // UART's Main-Freq, 26M 
#define baud_rate     115200   // UART's Baud-Rate,  1M 


unsigned char le_uart_rx_buf[UART0_RX_FIFO_MAX_COUNT];
unsigned char le_uart_tx_buf[UART0_TX_FIFO_MAX_COUNT];
volatile bool le_uart_rx_done = 0;
volatile unsigned long le_uart_rx_index = 0;
uint8_t cur_read_buf_idx = 0;

///UART Character format
enum UART_CHARFORMAT
{
    UART_CHARFORMAT_8 = 0,
    UART_CHARFORMAT_7 = 1
};

///UART Stop bit
enum UART_STOPBITS
{
    UART_STOPBITS_1 = 0,
    UART_STOPBITS_2 = 1  /* Note: The number of stop bits is 1.5 if a character format
                            with 5 bit is chosen*/
};

///UART Parity enable
enum UART_PARITY
{
    UART_PARITY_DISABLED = 0,
    UART_PARITY_ENABLED  = 1
};

///UART Parity type
enum UART_PARITYBIT
{
    UART_PARITYBIT_EVEN  = 0,
    UART_PARITYBIT_ODD   = 1,
    UART_PARITYBIT_SPACE = 2, // The parity bit is always 0.
    UART_PARITYBIT_MARK  = 3  // The parity bit is always 1.
};

///UART HW flow control
enum UART_HW_FLOW_CNTL
{
    UART_HW_FLOW_CNTL_DISABLED = 0,
    UART_HW_FLOW_CNTL_ENABLED = 1
};

///UART Input clock select
enum UART_INPUT_CLK_SEL
{
    UART_INPUT_CLK_SEL_0 = 0,
    UART_INPUT_CLK_SEL_1 = 1,
    UART_INPUT_CLK_SEL_2 = 2,
    UART_INPUT_CLK_SEL_3 = 3
};

///UART Interrupt enable/disable
enum UART_INT
{
    UART_INT_DISABLE = 0,
    UART_INT_ENABLE = 1
};

///UART Error detection
enum UART_ERROR_DETECT
{
    UART_ERROR_DETECT_DISABLED = 0,
    UART_ERROR_DETECT_ENABLED  = 1
};

enum UART_RX_STATE
{
    UART_STATE_RX_IDLE, 
    UART_STATE_RX_ONGOING
};
	


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// uart environment structure
struct uart_env_tag uart_env;
char uart_buff[64];

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

typedef void (*uart_rx_done_cb)(void);
typedef struct uart_tl_rx_info
{
    co_list_hdr_t hdr;
    uart_rx_done_cb   cb_done;
    uint16_t      length;
    uint8_t       p_buf[0];
} uart_tl_rx_info_t;

typedef struct uart_tl_tx_info
{
    co_list_hdr_t hdr;
    uint16_t      length;
    uint8_t       p_buf[0];
} uart_tl_tx_info_t;

struct uart_tl_env_tag
{
    /// Queue of kernel messages corresponding to uart rx packets
    co_list_t rx_queue;
    uart_tl_rx_info_t *uart_rx_info;
    uint8_t rx_state;
};

static struct uart_tl_env_tag uart_tl_env;
static uart_tl_rx_info_t *p_cur_uart_rx_info = NULL;
static uint32_t s_rx_data_pos = 0;

void rw_host2tl_iteration(void);
void rw_tl2host_iteration(void);

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */
uint8_t Read_Uart_Buf(void)
{
	return le_uart_rx_buf[cur_read_buf_idx++];
}	

uint8_t Uart_Read_Byte(void)
{
	return 0;//(REG_APB3_UART_PORT&0xff);
}	


int uart_putchar(char * st)
{
	uint8_t num = 0;
   /* while (*st) 
    {
		if(UART_TX_WRITE_READY)
		{
			REG_APB3_UART_PORT = *st;
	    	st++;
			num++;
	    }
	} */
    return num;
}

int uart_printf(const char *fmt,...)
{ 
	int n;	
	
	va_list ap;
	va_start(ap, fmt);
	n=vsprintf(uart_buff, fmt, ap);
	va_end(ap);
    uart_putchar(uart_buff);
	if(n > sizeof(uart_buff))
	{
		uart_putchar("buff full \r\n");
	}
		
    return n;
}

int uart_printf_null(const char *fmt,...)
{
	return 0;
}


char *hex2Str( uint8_t data)
{

  char hex[] = "0123456789ABCDEF";
  static char str[3];
  char *pStr = str;
 
  *pStr++ = hex[data >> 4];
  *pStr++ = hex[data & 0x0F];
  *pStr = 0;

  return str;
}

void uart_print_int(unsigned int num)
{
	uint8_t i;
	uint8_t m;

	uart_putchar((char *)"0x");
	for (i = 4;i > 0;i--)
	{
		m = ((num >> (8 * (i - 1)))& 0xff);
		uart_putchar(hex2Str(m));
	}
	uart_putchar("\r\n");
}

void uart_init(void)
{
    memset(&uart_env, 0, sizeof(struct uart_env_tag));
    uart_clear_rxfifo();
    s_rx_data_pos = 0;
    p_cur_uart_rx_info = NULL;

    ke_event_callback_set(KE_EVENT_RW_UART_RX, &rw_host2tl_iteration);
    ke_event_callback_set(KE_EVENT_RW_UART_TX, &rw_tl2host_iteration);
}	

void uart_flow_on(void)
{
    // Configure modem (HW flow control enable)
    // uart_flow_en_setf(0);
}

void uart_clear_rxfifo(void)
{
	/*while(uart_rx_fifo_rd_ready_getf())
	{
		Uart_Read_Byte();
	}*/
	memset(le_uart_rx_buf,0,UART0_RX_FIFO_MAX_COUNT);
	
}

bool uart_flow_off(void)
{
	  return true;
}

void uart_finish_transfers(void)
{
    //uart_flow_en_setf(1);

    // Wait TX FIFO empty
    //while(!uart_tx_fifo_empty_getf());
}

void uart_read(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy)
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(callback != NULL);
    
    uart_env.rx.callback = callback;
    uart_env.rx.dummy    = dummy;
    uart_env.rx.buf      = bufptr;
    uart_env.rx.len      = size;

    ke_event_set(KE_EVENT_RW_UART_RX);
}

void uart_write(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy)
{
	// Sanity check
	ASSERT_ERR(bufptr != NULL);
	ASSERT_ERR(size != 0);
	ASSERT_ERR(callback != NULL);
	
	uart_env.tx.callback = callback;
	uart_env.tx.dummy    = dummy;
    uart_env.tx.buf      = bufptr;
    uart_env.tx.len      = size;
    
    ke_event_set(KE_EVENT_RW_UART_TX);
}

uint8_t check_uart_stop(void)
{
	//return uart_tx_fifo_empty_getf();
	return 0;
}

void rw_host2tl_iteration(void)
{
	void (*callback) (void*, uint8_t) = NULL;
    void* data =NULL;

    ke_event_clear(KE_EVENT_RW_UART_RX);  

    if(p_cur_uart_rx_info == NULL)
    {
        uart_tl_env.rx_state = UART_STATE_RX_ONGOING;
        GLOBAL_INT_DISABLE();
        p_cur_uart_rx_info = (uart_tl_rx_info_t *)co_list_pop_front(&uart_tl_env.rx_queue);
        GLOBAL_INT_RESTORE();
    }

    if(p_cur_uart_rx_info != NULL)
    {
        if(s_rx_data_pos < p_cur_uart_rx_info->length)
        {
            memcpy(uart_env.rx.buf, p_cur_uart_rx_info->p_buf + s_rx_data_pos, uart_env.rx.len);
            s_rx_data_pos += uart_env.rx.len;
        	callback = uart_env.rx.callback;
    	    data = uart_env.rx.dummy;
        	if(callback != NULL)
        	{
        		// Clear callback pointer
        		uart_env.rx.callback = NULL;
        		uart_env.rx.dummy    = NULL;

        		// Call handler
          	 	callback(data, RWIP_EIF_STATUS_OK);
        	}
        }
        else
        {
            s_rx_data_pos = 0;
            if(p_cur_uart_rx_info->cb_done)
                p_cur_uart_rx_info->cb_done();
            ke_free(p_cur_uart_rx_info);
            uart_tl_env.rx_state = UART_STATE_RX_IDLE;
            p_cur_uart_rx_info = NULL;
            //rw_host2tl_start();
        }
    }
}

void rw_tl2host_iteration(void)
{
    void (*callback) (void*, uint8_t) = NULL;
	void* data = NULL;
    uint32_t len = uart_env.tx.len;
    uart_tl_tx_info_t *tx_info =  (uart_tl_tx_info_t *)ke_malloc(sizeof(uart_tl_tx_info_t) + len, KE_MEM_UART_ENV);

    
    if(tx_info == NULL) 
    {
        os_printf("tx_info, Not enough!!!\r\n");
        return;
    }
    
    memcpy(tx_info->p_buf, uart_env.tx.buf, len);
    callback = uart_env.tx.callback;
    data     = uart_env.tx.dummy;
    if(callback != NULL)
    {
        callback(data, RWIP_EIF_STATUS_OK);
    }
    
    uart_send(tx_info->p_buf, len);

    ke_event_clear(KE_EVENT_RW_UART_TX);
    ke_free(tx_info);
}

#if ITEST_ENABLE
extern uint8_t itest_dut_flag;
extern void xHexToStr(uint8_t *pStr, uint8_t *pHex, uint16_t pLen);
extern void hci_itest_done(uint8_t *buff, uint16_t len);
#endif
void rw_uart_tl_rx_packet(uint8_t *buff, uint16_t len)
{
    if(uart_env.ble_dut_enable)
    {
    	#if ITEST_ENABLE
    	hci_itest_done(buff,len);
		#endif
        uart_tl_env.uart_rx_info =  (uart_tl_rx_info_t *)ke_malloc(sizeof(uart_tl_rx_info_t) + len, KE_MEM_UART_ENV);
        if(uart_tl_env.uart_rx_info == NULL) 
        {
            os_printf("rw_uart_tl_rx_packet, Not enough!!!\r\n");
            return;
        }
        uart_tl_env.uart_rx_info->length = len;
        uart_tl_env.uart_rx_info->cb_done = NULL;
        memcpy((uint8_t *)&uart_tl_env.uart_rx_info->p_buf[0], buff, len);
        co_list_push_back(&uart_tl_env.rx_queue, &(uart_tl_env.uart_rx_info->hdr));

        h4tl_read_start_trigger(0);
    }
}

void rw_uart_ble_dut_mode_set(uint8_t value)
{
    if(value)
    {
        os_printf("Enter BLE dut mode!\r\n");
        uart_env.ble_dut_enable = 1;
    }
    else
    {
        os_printf("Exit BLE dut mode!\r\n");
        uart_env.ble_dut_enable = 0;
    }
}
/// @} UART
