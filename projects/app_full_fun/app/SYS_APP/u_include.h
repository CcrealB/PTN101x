
/*****************************************
 * 給底層引用的  #include  和函數聲明放這裡
 *****************************************/
//#ifndef _U_INCLUDE_H_
#define _U_INCLUDE_H_

#include <stdint.h>
#include "drv_timer.h"
#include "udrv_saradc.h"
#include "udrv_misc.h"

#include "u_config.h"
//#include "u_gpio.h"
#include "u_com.h"
#include "u_debug.h"
//#include "u_app.h"
#include "u_key.h"


#ifndef REG_READ
#define REG_READ(addr)          *((volatile uint32_t *)(addr))
#endif
#ifndef REG_WRITE
#define REG_WRITE(addr, _data)  (*((volatile uint32_t *)(addr)) = (_data))
#endif

// #define FILE_NAME(x)    strrchr(x,'\\') ? (strrchr(x,'\\') + 1) : x /* windows */
#define FILE_NAME(x)    strrchr(x,'/') ? (strrchr(x,'/') + 1) : x   /* linux */


void user_init_prv(void);
void user_init(void);
void user_init_post(void);

void user_loop(void);
void user_loop_10ms(void);
void user_loop_100ms(void);



#ifdef CONFIG_USER_SPI_FUNC

int user_spi_is_busy(void);
void user_spi_init(uint8_t spi_16bit_en);
void user_spi_uninit(void);
int user_spi_transfer(uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz, uint8_t tx_addr_fixed);
#define user_spi_read(buff, size)                   user_spi_transfer(NULL, 0, (buff), (size), 0)
#define user_spi_write(buff, size, tx_addr_fixed)   user_spi_transfer((buff), (size), NULL, 0, (tx_addr_fixed))

#endif //CONFIG_USER_SPI_FUNC

//#endif /* _U_INCLUDE_H_ */
