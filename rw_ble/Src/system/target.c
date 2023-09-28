#include <bk_config.h>
#include "../header/BK_HCI_Protocol.h"


//============== Interrupt  FLAG_bits ============

#define        INT_FLAG_PWM0          (0x1 << 0  )
#define        INT_FLAG_PWM1          (0x1 << 1  )
#define        INT_FLAG_TIMER0        (0x1 << 2  )
#define        INT_FLAG_TIMER1        (0x1 << 3  )
#define        INT_FLAG_UART0         (0x1 << 4  )
#define        INT_FLAG_UART1         (0x1 << 5  )
#define        INT_FLAG_SPI           (0x1 << 6  )
#define        INT_FLAG_I2C           (0x1 << 7  )
#define        INT_FLAG_SADC          (0x1 << 8  )
#define        INT_FLAG_GPIO          (0x1 << 9  )
#define        INT_FLAG_RTC           (0x1 << 10 )
#define        INT_FLAG_I2S           (0x1 << 11 )
#define        INT_FLAG_AON_RTC       (0x1 << 12 )
#define        INT_FLAG_USB           (0x1 << 17 )
#define        INT_FLAG_DMA           (0x1 << 18 )
#define        INT_FLAG_BK24          (0x1 << 19 )
#define        INT_FLAG_RWBLE         (0x1 << 20 )
#define        INT_FLAG_RWBT          (0x1 << 21 )
#define        INT_FLAG_RWDM          (0x1 << 22 )


//================================================
//====================== FIQ =====================
//================================================

