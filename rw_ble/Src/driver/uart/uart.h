/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief UART Driver for HCI over UART operation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _UART_H_
#define _UART_H_

/**
 ****************************************************************************************
 * @defgroup UART UART
 * @ingroup DRIVERS
 * @brief UART driver
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>          // standard boolean definitions
#include "types.h"           // standard integer functions
#include "rwip_config.h"
#include "user_config.h"

#if (UART_PRINTF_EN && UART_DRIVER)

#if  !BLE_TESTER
#define UART_PRINTF	os_printf //uart_printf
#else
#define UART_PRINTF uart_printf_null //uart_printf 
#endif //!BLE_TESTER

#else
#define UART_PRINTF uart_printf_null 
#endif // #if UART_PRINTF_EN

#define ITEST_ENABLE 0

 /*
  * STRUCT DEFINITIONS
  *****************************************************************************************
  */
 /* TX and RX channel class holding data used for asynchronous read and write data
  * transactions
  */
 /// UART TX RX Channel
 struct uart_txrxchannel
 {
	 /// call back function pointer
	 void (*callback) (void*, uint8_t);
	 /// Dummy data pointer returned to callback when operation is over.
	 void* dummy;
	 
	 uint8_t *buf;
	 
	 uint32_t len;
 };
 
 /// UART environment structure
struct uart_env_tag
{
    /// tx channel
    struct uart_txrxchannel tx;
    /// rx channel
    struct uart_txrxchannel rx;
    /// error detect
    uint8_t errordetect;
    /// external wakeup
    bool ext_wakeup;
    
    uint8_t ble_dut_enable;
};


/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initializes the UART to default values.
 *****************************************************************************************
 */
void uart_init(void);

void dbg_initial(void);

void uart_clear_rxfifo(void);

uint8_t Uart_Read_Byte(void);
uint8_t Read_Uart_Buf(void);

int dbg_putchar(char * st);
int uart_putchar(char * st);
int uart_printf(const char *fmt,...);
int uart_printf_null(const char *fmt,...);
int dbg_printf(const char *fmt,...);
void uart_print_int(unsigned int num);
uint8_t check_uart_stop(void);

/****** REG  ****************/
void uart_send(unsigned char* buf, unsigned int len);
void TRAhcit_UART_Rx(void);

#define UART0_RX_FIFO_MAX_COUNT  64

#define UART0_TX_FIFO_MAX_COUNT  64

extern unsigned char le_uart_rx_buf[UART0_RX_FIFO_MAX_COUNT];
extern unsigned char le_uart_tx_buf[UART0_TX_FIFO_MAX_COUNT];
extern volatile bool le_uart_rx_done ;
extern volatile unsigned long le_uart_rx_index ;
/****** REG  ****************/



#ifndef CFG_ROM
/**
 ****************************************************************************************
 * @brief Enable UART flow.
 *****************************************************************************************
 */
void uart_flow_on(void);

/**
 ****************************************************************************************
 * @brief Disable UART flow.
 *****************************************************************************************
 */
bool uart_flow_off(void);
#endif //CFG_ROM

/**
 ****************************************************************************************
 * @brief Finish current UART transfers
 *****************************************************************************************
 */
void uart_finish_transfers(void);

/**
 ****************************************************************************************
 * @brief Starts a data reception.
 *
 * @param[out] bufptr   Pointer to the RX buffer
 * @param[in]  size     Size of the expected reception
 * @param[in]  callback Pointer to the function called back when transfer finished
 * @param[in]  dummy    Dummy data pointer returned to callback when reception is finished
 *****************************************************************************************
 */
void uart_read(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy);

/**
 ****************************************************************************************
 * @brief Starts a data transmission.
 *
 * @param[in] bufptr   Pointer to the TX buffer
 * @param[in] size     Size of the transmission
 * @param[in] callback Pointer to the function called back when transfer finished
 * @param[in] dummy    Dummy data pointer returned to callback when transmission is finished
 *****************************************************************************************
 */
void uart_write(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy);

#if defined(CFG_ROM)
/**
 ****************************************************************************************
 * @brief Poll UART on reception and transmission.
 *
 * This function is used to poll UART for reception and transmission.
 * It is used when IRQ are not used to detect incoming bytes.
 *****************************************************************************************
 */
void uart_poll(void);
#endif //CFG_ROM

/**
 ****************************************************************************************
 * @brief Serves the data transfer interrupt requests.
 *
 * It clears the requests and executes the appropriate callback function.
 *****************************************************************************************
 */
void rw_uart_tl_rx_packet(uint8_t *buff, uint16_t len);
void rw_uart_ble_dut_mode_set(uint8_t value);

typedef void (*UART_RX_CALLBACK_T)(uint8_t *buf, uint8_t len);   

/// @} UART
#endif /* _UART_H_ */
