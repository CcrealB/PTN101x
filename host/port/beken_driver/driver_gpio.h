#ifndef _DRIVER_GPIO_H_
#define _DRIVER_GPIO_H_

#include <stdint.h>

#define GPIO_CON_UART2                   ( 0 << 0 )
#define GPIO_CON_I2C2                    ( 1 << 0 )
#define GPIO_CON_I2S                     ( 0 << 1 )
#define GPIO_CON_UART1_MONITOR_FLOWCTL   ( 1 << 1 )
#define GPIO_CON_JTAG                    ( 0 << 2)
#define GPIO_CON_NOJTAG                  ( 1 << 2 )
#define GPIO_CON_I2C_FM                  ( 1 << 3 )  
#define GPIO_CON_SDIO                    ( 1 << 3 )
#define GPIO_CON_INT_FM_ON               ( 1 << 4 )
#define GPIO_CON_INT_FM_OFF              ( 0 << 4 )

typedef enum
{
            // 1st FUNC     2nd FUNC        3rd FUNC            4th FUNC
    GPIO0 , //UART1_TXD       I2C1_SCL
    GPIO1 , //UART1_RXD       I2C1_SDA
    GPIO2 , //SPI1_CSN        IrDA
    GPIO3 , //SPI1_SCK        PWM0
    GPIO4 , //SPI1_MOSI       PWM1
    GPIO5 , //SPI1_MISO       PWM2
    GPIO6 , //UART2_TXD       PWM3
    GPIO7 , //UART2_RXD       PWM4
    GPIO8 , //SD_CLK          PWM5
    GPIO9 , //SD_CMD          TX_EN
    GPIO10, //SD_DATA0        RX_EN
    GPIO11, //SPDIF1          JTAG_TCK
    GPIO12, //SPDIF2          JTAG_TMS
    GPIO13, //HDMI_CEC        JTAG_TDI
    GPIO14, //SPDIF3          JTAG_TDO
    GPIO15, //
    GPIO16, //I2C2_SCL        UART2_TXD
    GPIO17, //I2C2_SDA        UART2_RXD
    GPIO18, //I2S1_BCLK       PWM6
    GPIO19, //I2S1_SCLK       PWM7
    GPIO20, //I2S1_DIN        PWM8
    GPIO21, //I2S1_DOUT1      PWM9
    GPIO22, //SPI2_CSN        I2S1_DOUT2      QSPI_FLASH_CLK
    GPIO23, //SPI2_SCK        I2S1_DOUT3      QSPI_FLASH_CSN
    GPIO24, //SPI2_MOSI       I2S_MCLK
    GPIO25, //SPI2_MISO       PWM10
    GPIO26, //I2S2_BCLK       PWM6            QSPI_RAM_CLK
    GPIO27, //I2S2_SCLK       PWM7            QSPI_RAM_CSN
    GPIO28, //I2S2_DIN        PWM8            QSPI_IO0
    GPIO29, //I2S2_DOUT1      PWM9            QSPI_IO1
    GPIO30, //I2S3_BCLK       PWM6            QSPI_IO2            SPI3_SCK
    GPIO31, //I2S3_SCLK       PWM7            QSPI_IO3            SPI3_CSN
    GPIO32, //I2S3_DIN        PWM8            QSPI_FLASH_CLK      SPI3_MISO
    GPIO33, //I2S3_DOUT1      PWM9            QSPI_FLASH_CSN      SPI3_MOSI
    GPIO34, //UART3_TXD                       DMIC2_CLK           SD_DATA2
    GPIO35, //UART3_RXD       PWM11           DMIC2_DAT           SD_DATA1
    GPIO36, //SPI3_CSN        DIG_CLKOUT1                         SD_DATA3
    GPIO37, //SPI3_SCK        DIG_CLKOUT2                         SD_CLK
    GPIO38, //SPI3_MOSI                       DMIC1_CLK           SD_CMD
    GPIO39, //SPI3_MISO                       DMIC1_DAT           SD_DATA0
    GPIO_NUM,
}GPIO_PIN;

enum
{
    GPIO_FUNCTION_UART2 = 0,
    GPIO_FUNCTION_I2C1,
    GPIO_FUNCTION_PCM2,
    GPIO_FUNCTION_UART1_FLOW_CTL,
    GPIO_FUNCTION_UART1_MONITOR,
    GPIO_FUNCTION_SPI,
    GPIO_FUNCTION_PWM0,
    GPIO_FUNCTION_PWM1,
    GPIO_FUNCTION_SDIO,
    GPIO_FUNCTION_SDIO_DATA1_3_ENABLE,
    GPIO_FUNCTION_I2C_FM,
    GPIO_FUNCTION_JTAG
};

enum
{
    GPIO_INT_LEVEL_LOW  = 0,
    GPIO_INT_LEVEL_HIGH = 1,
    GPIO_INT_EDGE_POS   = 2,
    GPIO_INT_EDGE_NEG   = 3,
};

/**
 * @brief GPIO direction definition
 */
typedef enum
{
    GPIO_INPUT  = 3,
    GPIO_OUTPUT = 0,
    GPIO_INOUT  = 1,
    GPIO_HRES   = 2,
}GPIO_DIR;

/**
 * @brief GPIO pull mode definition
 */
typedef enum
{
    GPIO_PULL_NONE = 0,
    GPIO_PULL_DOWN = 2,
    GPIO_PULL_UP   = 3,
}GPIO_PULL_MODE;

/**
 * @brief GPIO peripheral mode definition
 */
typedef enum
{
    GPIO_PERI_NONE  = 0,
    GPIO_PERI_FUNC1 = 4,
    GPIO_PERI_FUNC2 = 5,
    GPIO_PERI_FUNC3 = 6,
    GPIO_PERI_FUNC4 = 7,
}GPIO_PERI_MODE;

/**
 * @brief GPIO driver capacity definition
 */
typedef enum
{
    GPIO_DRV_5mA = 0,
    GPIO_DRV_10mA,
    GPIO_DRV_15mA,
    GPIO_DRV_20mA,
}GPIO_DRV_CAP;

#define GPIO_X_FUNTION_MODE_POSI(x)         	((x)*2)
#define GPIO_X_FUNTION_MODE_MASK(x)         	(0x03UL << GPIO_X_FUNTION_MODE_POSI(x))
#define GPIO_X_FUNTION_MODE_1_FUNC(x)       	(0x00UL << GPIO_X_FUNTION_MODE_POSI(x))
#define GPIO_X_FUNTION_MODE_2_FUNC(x)       	(0x01UL << GPIO_X_FUNTION_MODE_POSI(x))
#define GPIO_X_FUNTION_MODE_3_FUNC(x)       	(0x02UL << GPIO_X_FUNTION_MODE_POSI(x))
#define GPIO_X_FUNTION_MODE_4_FUNC(x)       	(0x03UL << GPIO_X_FUNTION_MODE_POSI(x))

#define GPIO2_X_FUNTION_MODE_POSI(x)        	(((x)-16)*2)
#define GPIO2_X_FUNTION_MODE_MASK(x)        	(0x03UL << GPIO2_X_FUNTION_MODE_POSI(x))
#define GPIO2_X_FUNTION_MODE_1_FUNC(x)      	(0x00UL << GPIO2_X_FUNTION_MODE_POSI(x))
#define GPIO2_X_FUNTION_MODE_2_FUNC(x)      	(0x01UL << GPIO2_X_FUNTION_MODE_POSI(x))
#define GPIO2_X_FUNTION_MODE_3_FUNC(x)      	(0x02UL << GPIO2_X_FUNTION_MODE_POSI(x))
#define GPIO2_X_FUNTION_MODE_4_FUNC(x)      	(0x03UL << GPIO2_X_FUNTION_MODE_POSI(x))

#define GPIO3_X_FUNTION_MODE_POSI(x)        	(((x)-32)*2)
#define GPIO3_X_FUNTION_MODE_MASK(x)        	(0x03UL << GPIO3_X_FUNTION_MODE_POSI(x))
#define GPIO3_X_FUNTION_MODE_1_FUNC(x)      	(0x00UL << GPIO3_X_FUNTION_MODE_POSI(x))
#define GPIO3_X_FUNTION_MODE_2_FUNC(x)      	(0x01UL << GPIO3_X_FUNTION_MODE_POSI(x))
#define GPIO3_X_FUNTION_MODE_3_FUNC(x)      	(0x02UL << GPIO3_X_FUNTION_MODE_POSI(x))
#define GPIO3_X_FUNTION_MODE_4_FUNC(x)      	(0x03UL << GPIO3_X_FUNTION_MODE_POSI(x))
void gpio_config( int index, int dir );
uint32_t gpio_input(int index);
void gpio_output(int index, uint32_t val);
void gpio_output_reverse(int index);
void gpio_int_enable( int index, int level );
void gpio_int_disable(int index);
void gpio_button_wakeup_enable(void);
void gpio_isr( void );
void BK3000_GPIO_Initial (void);
void gpio_config_new (GPIO_PIN pin, GPIO_DIR dir, GPIO_PULL_MODE pull, GPIO_PERI_MODE peri);
void gpio_enable_second_function( int gpio_function );
void gpio_config_capacity(GPIO_PIN pin, uint8_t capacity);

#endif
