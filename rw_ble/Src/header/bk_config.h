#ifndef _BK3633_CONFIG_H_
#define _BK3633_CONFIG_H_

#include <stdint.h>
#include "types.h"
#if 0
typedef unsigned char               uint8;
typedef unsigned short int          uint16;
typedef unsigned int                uint32;
typedef unsigned long long          uint64;
typedef unsigned char               uint8_t;
typedef unsigned short int          uint16_t;
typedef unsigned int                uint32_t;
typedef unsigned long long          uint64_t;
#endif

#define NCV_SIM                     0x0
#define	TEST_ID_MAX					100

#if 0
#define TRUE                        0x1
#define FALSE                       0x0
#endif
#define TX                          0x0
#define RX                          0x1

//Clock Manager
#define MCLK_DCO                    0x0
#define MCLK_16M                    0x1
#define MCLK_DPLL                   0x2
#define MCLK_LPO                    0x3


#define FLASH_EN                    (0x1U << 0 )
#define DCO_EN                      (0x1U << 1 )
#define ROSC_EN                     (0x1U << 2 )
#define XTAL26M_EN                  (0x1U << 3 )
#define XTAL32K_EN                  (0x1U << 4 )
#define DPLL_EN                     (0x1U << 5 )
#define CORE_LPEN                   (0x1U << 7 )
#define ANA_LDO_EN                  (0x1U << 8 )
#define IO_LPEN                     (0x1U << 9 )
#define XTAL2RF_EN                  (0x1U << 10)
#define XTAL26M_LPEN                (0x1U << 11)
#define TEMPSEN_EN                  (0x1U << 12)
#define SARADC_EN                   (0x1U << 13)
#define AUDL_EN                     (0x1U << 15)
#define AUDR_EN                     (0x1U << 16)
#define MICL_EN                     (0x1U << 17)
#define MICR_EN                     (0x1U << 18)
#define LINEIN_EN                   (0x1U << 19)


#define SLEEP_NORMAL_VOLT           0x4f89
#define SLEEP_LOW_VOLT              0xB706
#define DEEP_SLEEP                  0xadc1

#define ENTER_SLEEP(mode)           set_PMU_Reg0x1_sleep_mode(mode)





#define UART1CLK_PWD                (0x1U << 0 )
#define UART2CLK_PWD                (0x1U << 1 )
#define UART3CLK_PWD                (0x1U << 2 )
#define FMI2CCLK_PWD                (0x1U << 3 )
#define I2C1CLK_PWD                 (0x1U << 4 )
#define I2C2CLK_PWD                 (0x1U << 5 )



#define UART1_INT                (0x1U << 0 )
#define UART2_INT                (0x1U << 1 )
#define UART3_INT                (0x1U << 2 )
#define FMI2C_INT                (0x1U << 3 )
#define I2C1_INT                 (0x1U << 4 )




//Uart Debug
#if (NCV_SIM == 0)
#define UART_BAUD_RATE           1000000// 115200
#else
#define UART_BAUD_RATE           1000000
#endif

#define UART_CLK_FREQ            16

int32_t os_printf(const char *fmt, ...);
void cpu_delay( volatile unsigned int times);


#define bk_printf(fmt, ...)     os_printf("[BLE|I]"fmt, ##__VA_ARGS__)
//#define bk_printf				os_printf
#define stack_printf			bk_printf

typedef struct DbgPrtBuf
{
    uint16_t    pos_sta ;
    uint16_t    pos_end ;
    char        buf[1024]   ;
} DbgPrtBuf_t;

#define UART_RX_FIFO_MAX_COUNT   512
#define UART_TX_FIFO_MAX_COUNT   512

//void Delay(volatile uint16_t times);
void AnaRegWrite(unsigned char idx, unsigned int data);

#define REG_READ(addr)          *((volatile uint32_t *)(addr))
#define REG_WRITE(addr, _data)  (*((volatile uint32_t *)(addr)) = (_data))

uint32_t FlashGetId(void);


#endif
