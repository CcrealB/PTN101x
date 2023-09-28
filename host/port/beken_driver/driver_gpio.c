#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bkreg.h"

#define BK_XVR_BASEBAND_TXRX_BIT            0
#define gpio_context ((GPIOContext*)MDU_GPIO_BASE_ADDR)

typedef struct _GPIOConfig
{
    volatile uint32_t input         : 1;
    volatile uint32_t output        : 1;
    volatile uint32_t dir           : 2;
    volatile uint32_t pull_mode     : 2;
    volatile uint32_t peri_en       : 1;
    volatile uint32_t input_monitor : 1;
    volatile uint32_t drv_capacity  : 2;
    volatile uint32_t               : 0;
}GPIOConfig;

typedef struct _GPIOContext
{
    volatile GPIOConfig config[GPIO_NUM];

    volatile uint32_t   int_mode0;
    volatile uint32_t   int_mode1;
    volatile uint32_t   int_mode2;

    volatile uint64_t   int_en;
    volatile uint64_t   int_status;
}GPIOContext;

void BK3000_GPIO_Initial(void)
{
     uint8_t index = 0;

    for(index = 0; index < GPIO_NUM; index++) (*(volatile unsigned long*)(MDU_GPIO_BASE_ADDR + index * 4)) = 0x8;
#if (BK_XVR_BASEBAND_TXRX_BIT == 1)
     //gpio_config_new(GPIO2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     //gpio_config_new(GPIO3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     gpio_config_new(GPIO6, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     gpio_config_new(GPIO7, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     //gpio_config_new(GPIO8, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     //gpio_config_new(GPIO9, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
     system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_MODE_EN, 0x000000FC);                ////Debug[2~7]
     system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_MUX, 3);                             ////Debug Mux : BT

     if(1)
     {
         REG_SYSTEM_0x29 |= (1<<9);                                          //baseband debug bus enable
         
         //system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX0, 50);               //Debug GPIO 2: 50(XVR_Tx_bit)
         //system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX1, 51);               //Debug GPIO 3: 51(XVR_Rx_bit)
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX2, 50);               //Debug GPIO 6: 63(XVR_PLL_Unlock)
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX3, 51);               //Debug GPIO 7: 54(XVR_Radio_On)
         //system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX4, 55);               //Debug GPIO 8: 55(XVR_T/Rx_Mode)
         //system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX5, 57);               //Debug GPIO 9: 57(XVR_SyncFind)
     }
     else
     {
         REG_SYSTEM_0x29 &= ~(1<<9);                                         //rw debug bus enable
         /* Value of DIAGCNTL: 0x03 */
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX0, 32);                 //DiagOut[0]: radcntl_txen
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX1, 33);                 //DiagOut[1]: radcntl_rxen
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX2, 34);                 //DiagOut[2]: sync_window
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX3, 35);                 //DiagOut[3]: sync_found_pulse 
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX4, 36);                 //DiagOut[4]: event_in_process 
         system_ctrl(SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX5, 37);                 //DiagOut[5]: endactintstat
     }

	 gpio_config_new(GPIO25, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	 gpio_config_new(GPIO26, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);

     /*general GPIO function use*/
     //gpio_config_new(GPIO4, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO5, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO6, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO7, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO8, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
     //gpio_config_new(GPIO9, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
     //gpio_config_new(GPIO10, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
     //gpio_config_new(GPIO16, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO17, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO18, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
     //gpio_config_new(GPIO24, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
#endif
}

/***************************
 * Function name:   gpio_int_enable
 * Description:       
 * Parameters:  index: gpio no.
                        level:  0: low level interrupt;1: high level interrupt;    
                                  2: rise edge interrupt;3: fall down interrup
 * Return value: none
********************************/
void gpio_int_enable(int index, int level)
{
    //TODO
    if(index<16)
    {
		REG_GPIO_0x33 &= ~(1<<index);
        REG_GPIO_0x35 |= 1<<index; // clear status
        REG_GPIO_0x30 &= ~(3 << (index*2));   // Low level detect
        sys_delay_us(4);
        REG_GPIO_0x30 |= (level << (index*2)); 
        REG_GPIO_0x33 |= 1<<index;  // int enable
    }
    else if((index>=16)&&(index<32))
    {
		REG_GPIO_0x33 &= ~(1<<index);
        REG_GPIO_0x35 |= 1<<index; 
        REG_GPIO_0x31 &= ~(3 << ((index-16)*2)); // Low level detect
        sys_delay_us(4);
        REG_GPIO_0x31 |= (level << ((index-16)*2));
        REG_GPIO_0x33 |= 1<<index;
    }
    else if((index>=32)&&(index<40))
    {
		 REG_GPIO_0x34 &= ~(1<<(index-32));
         REG_GPIO_0x36 |= 1<<(index-32);
         REG_GPIO_0x32 &= ~(3 << ((index-32)*2)); // Low level detect
         sys_delay_us(4);
         REG_GPIO_0x32 |= (level << ((index-32)*2));
         REG_GPIO_0x34 |= 1<<(index-32);
    }
}

void gpio_int_disable(int index)
{
	if(index > 31)
		REG_GPIO_0x34 &= ~(1<<(index-32));  // int disable
	else
		REG_GPIO_0x33 &= ~(1<<index);  		// int disable
}

void gpio_button_wakeup_enable(void)
{
    int i;    
    app_handle_t app_h = app_get_sys_handler();  
#if (CONFIG_SHUTDOWN_WAKEUP_IO==1)
    uint64_t wakeup_pin_mask = (1ULL << SECOND_WAKEUP_IO);
    REG_GPIO_0x35 |= (wakeup_pin_mask & 0xffffffff);
    REG_GPIO_0x36 |= (wakeup_pin_mask >> 32);
#endif
    REG_GPIO_0x35 |= (app_h->button_mask & 0xffffffff); // clear interrupt status;   
    REG_GPIO_0x36 |= (app_h->button_mask >> 32); // clear interrupt status;   
    REG_GPIO_0x33 = 0;                  
    REG_GPIO_0x34 = 0;
   
    for(i =  0; i < 16; i++) 
    {
        if(app_h->button_mask & (1UL << i)) 
            REG_GPIO_0x30 &= ~(3 << (i * 2)); // Low level detect  
    }
    for(i = 16; i < 32; i++) 
    {
        if(app_h->button_mask & (1UL << i)) 
            REG_GPIO_0x31 &= ~(3 << ((i - 16) * 2)); // Low level detect 
    }  
    for(i = 32; i < GPIO_NUM; i++) 
    {
        if(app_h->button_mask & (1UL << i)) 
            REG_GPIO_0x32 &= ~(3 << ((i - 32) * 2)); // Low level detect 
    }  
 
    REG_GPIO_0x33 |= (app_h->button_mask & 0xffffffff);
    REG_GPIO_0x34 |= (app_h->button_mask >> 32);

}

void gpio_config_new (GPIO_PIN pin, GPIO_DIR dir, GPIO_PULL_MODE pull, GPIO_PERI_MODE peri)
{
    if(pin >= GPIO_NUM) return;

    GPIOConfig* gpio = (GPIOConfig*)&gpio_context->config[pin];

    gpio->dir           = dir;
    gpio->pull_mode     = pull;
    gpio->peri_en       = peri >> 2;
    gpio->input_monitor = 0;
    gpio->drv_capacity  = 0;

    if(peri)
    {
        system_gpio_peri_config(pin, peri & 3);
    }
}

//capacity 0 -5ma  1 -10ma  2 -15ma  3 -20ma
void gpio_config_capacity(GPIO_PIN pin, uint8_t capacity)
{
    if(pin >= GPIO_NUM) return;

    GPIOConfig* gpio = (GPIOConfig*)&gpio_context->config[pin];

    gpio->drv_capacity  = capacity;
}

// 0 -input & pulldown, 1-output, 2--second function, 3-input & pullup , 4 -input , 5 -High impedance
void gpio_config(int index, int dir)
{
    volatile unsigned long *ptr;

    assert(dir < 0 || dir > 5 || index >= GPIO_NUM);

    if(dir < 0 || dir > 5 || index > GPIO_NUM || index <0) return;

    ptr = (volatile unsigned long *)(MDU_GPIO_BASE_ADDR + index*4);

    if(dir  == 1)
        *ptr = 0;
    else if(dir == 0)
        *ptr = 0x2C;
    else if(dir == 2)
        *ptr = 0x70;
    else if(dir == 3)
        *ptr = 0x3C;
    else if(dir == 4)
        *ptr = 0x0C;
    else if(dir == 5)
        *ptr = 0x08;
}

uint32_t gpio_input( int index )
{
    volatile unsigned long *ptr;

	if(index > GPIO_NUM) return 0;

    ptr = (volatile unsigned long *)(MDU_GPIO_BASE_ADDR + index*4);

    return (*ptr)&( 1 << SFT_GPIO_0x00_INPUT );
}

void gpio_output( int index, uint32_t val )
{
    volatile unsigned long *ptr;
    uint32_t data;

	if(index > GPIO_NUM) return;
	
    ptr = (volatile unsigned long *)(MDU_GPIO_BASE_ADDR + index*4);

    data = *ptr;
    data &= ~(1 << SFT_GPIO_0x00_OUTPUT);
    data |= (val&0x01) << SFT_GPIO_0x00_OUTPUT;
    *ptr = data;
}

void gpio_output_reverse( int index )
{
    volatile unsigned long *ptr;
    uint32_t data;

	if(index > GPIO_NUM) return;

    ptr = (volatile unsigned long *)(MDU_GPIO_BASE_ADDR+index*4);
    data = *ptr;
    data ^= ( 1 << SFT_GPIO_0x00_OUTPUT );
    *ptr = data;
}

void gpio_enable_second_function( int gpio_function )
{
    int i, start_index=-1, end_index=-1;

    assert(gpio_function < 0 || gpio_function > 15);

    switch( gpio_function )
    {
        case GPIO_FUNCTION_UART2:
            break;
        case GPIO_FUNCTION_I2C1:
            start_index = 16;
            end_index = 17;
            break;
        case GPIO_FUNCTION_PCM2:
            break;
        case GPIO_FUNCTION_UART1_FLOW_CTL:
            break;
        case GPIO_FUNCTION_UART1_MONITOR:
            break;
        case GPIO_FUNCTION_SPI:
            start_index = 12;
            end_index = 14;
            break;
    #ifdef ENABLE_PWM
        case GPIO_FUNCTION_PWM0:
            start_index = end_index = 10; // 6
            break;
        case GPIO_FUNCTION_PWM1:
            start_index = end_index = 11; // 7
            break;
    #endif
        case GPIO_FUNCTION_SDIO:
            start_index = 8;
	        end_index = 10;		
            break;
	#ifdef CONFIG_APP_SDCARD_4_LINE		
        case GPIO_FUNCTION_SDIO_DATA1_3_ENABLE:
            start_index = 34;
	        end_index = 36;		
            break;	
	#endif		
        case GPIO_FUNCTION_I2C_FM:
            break;

        case GPIO_FUNCTION_JTAG:
            break;

        default:
            return;
     }

     for(i = start_index; i <= end_index; i++)
        gpio_config(i, 2);
}

#if (CPU_CLK_SEL == CPU_CLK_DPLL)
extern volatile uint8_t dpll_checked;
extern volatile uint8_t dpll_result;
extern volatile uint32_t dpll_gpio10_int_cnt;
#endif
void gpio_isr( void )
{
    uint32_t gpiosts1 = REG_GPIO_0x35;
    uint32_t gpiosts2 = REG_GPIO_0x36;

#if (CPU_CLK_SEL == CPU_CLK_DPLL)
    if(dpll_checked == 0)        //power on dpll check
    {
        if(gpiosts1 & (1 << 10))
        {
             dpll_gpio10_int_cnt ++;
             if(dpll_gpio10_int_cnt >= 100)
             {
                 dpll_checked = 1;
                 dpll_result = 1;
                 REG_GPIO_0x33 &= ~(1 << GPIO10);
             }
        }
        REG_GPIO_0x35 = gpiosts1;
        REG_GPIO_0x36 = gpiosts2;

        return;
    }
#endif
    app_handle_t app_h = app_get_sys_handler();
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE

#if (CONFIG_UART_IN_SNIFF == 1)
     if(gpiosts1 & (1<<1))		//GPIO01 = Uart Rx
    {
        REG_GPIO_0x33 &= ~(1 << 1);

        sniffmode_wakeup_dly = 1000;
        app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
        //uart_gpio_enable();
        //uart_initialise(115200);
        //os_printf("===uart wake-up!!!\r\n");  // The first rx datas only wake up CPU from halting state;
    }
#endif
    if((gpiosts1 & (app_h->button_mask & 0xffffffff))
        ||(gpiosts2 & (app_h->button_mask >>32)))
    {
        sniffmode_wakeup_dly = 100;
        sniff_enable_timer0_pt0();
    }
#endif

    #ifdef CONFIG_SDCARD_DETECT
    extern void sd_pull_out_isr(uint32_t io0_31, uint32_t io32_39);
    sd_pull_out_isr(gpiosts1, gpiosts2);
    #endif

    #ifdef CONFIG_DRV_IR_RX_SW
    extern void irda_rx_gpio_isr(uint32_t io0_31, uint32_t io32_39);
    irda_rx_gpio_isr(gpiosts1, gpiosts2);
    #endif

    #ifdef CONFIG_USER_GPIO_INT
    extern void user_gpio_isr(uint32_t io0_31, uint32_t io32_39);
    user_gpio_isr(gpiosts1, gpiosts2);
    #endif

    REG_GPIO_0x33 &= ~(app_h->button_mask & 0xffffffff);    // diable button isr
    REG_GPIO_0x34 &= ~(app_h->button_mask >>32);    // diable button isr

    REG_GPIO_0x35 = gpiosts1;
    REG_GPIO_0x36 = gpiosts2;
}
void debug_show_gpio(uint8_t value)
{
    REG_GPIO_0x1D=0X00;
    
    REG_GPIO_0x02=(value&0X01)<<1;
    REG_GPIO_0x03=((value&0X02)>>1)<<1;
    REG_GPIO_0x04=((value&0X04)>>2)<<1;
    REG_GPIO_0x05=((value&0X08)>>3)<<1;
    REG_GPIO_0x06=((value&0X10)>>4)<<1;
    REG_GPIO_0x07=((value&0X20)>>5)<<1;
    REG_GPIO_0x12=((value&0X40)>>6)<<1;
    REG_GPIO_0x13=((value&0X80)>>7)<<1;
    
    REG_GPIO_0x1D=0X02; 
}
