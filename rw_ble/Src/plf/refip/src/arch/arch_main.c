/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ******** ********************************************************************************
 */

 
/*
 * INCLUDES
 ****************************************************************************************
 */
 
#include "rwip_config.h" // RW SW configuration

#include "arch.h"      // architectural platform definitions
#include <stdlib.h>    // standard lib functions
#include <stddef.h>    // standard definitions
#include <stdint.h>    // standard integer definition
#include <stdbool.h>   // boolean definition
#include <string.h>
//#include "boot.h"      // boot definition
#include "rwip.h"      // RW SW initialization
#include "uart.h"      	// UART initialization
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
#include "rw_rf.h"        // RF initialization
#endif // BLE_EMB_PRESENT || BT_EMB_PRESENT

#if (BLE_APP_PRESENT)
#include "app.h"       // application functions
#endif // BLE_APP_PRESENT
#include "nvds.h"         // NVDS definitions 
#include "app_task.h" 
#include "co_math.h"
#include "co_utils.h"
#include "app_adv.h"

#include "app_beken_includes.h"
/*******************************************************************/

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
// Creation of uart external interface api
struct rwip_eif_api uart_api;

void rwip_1st_wakeup(void);
/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#if ((UART_PRINTF_EN) &&(UART_DRIVER))
void assert_err(const char *condition, const char * file, int line)
{
	os_printf("%s,condition %s,file %s,line = %d\r\n",__func__,condition,file,line);
  
}

void assert_param(int param0, int param1, const char * file, int line)
{
	os_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
  
}

void assert_warn(int param0, int param1, const char * file, int line)
{
	 os_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
 
}

void dump_data(uint8_t* data, uint16_t length)
{
	os_printf("%s,data = %d,length = %d,file = %s,line = %d\r\n",__func__,data,length);
 
}
#else
void assert_err(const char *condition, const char * file, int line)
{
  
}

void assert_param(int param0, int param1, const char * file, int line)
{
  
}

void assert_warn(int param0, int param1, const char * file, int line)
{
 
}

void dump_data(uint8_t* data, uint16_t length)
{
 
}
#endif //UART_PRINTF_EN

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void platform_reset(uint32_t error)
{
    //void (*pReset)(void);
	UART_PRINTF("BLE---error = 0x%x\r\n", error);

    // Disable interrupts
    GLOBAL_INT_STOP();

    #if UART_PRINTF_EN
    // Wait UART transfer finished
    uart_finish_transfers();
    #endif //UART_PRINTF_EN


    if(error == RESET_AND_LOAD_FW || error == RESET_TO_ROM)
    {
        // Not yet supported
    }
    else
    {
        //Restart FW
        //pReset = (void * )(0x0);
        //pReset();
        //wdt_enable(10);
        //while(1);
    }
}

void emi_init(void)
{
	unsigned long *p=(unsigned long *)0x814000;
	int i;
	for(i=0;i<1000;i++)
	{
		*p++ =  0;					
	}
}

/**
 *******************************************************************************
 * @brief RW main function.
 *
 * This function is called right after the booting process has completed.
 *
 * @return status   exit status
 *******************************************************************************
 */

void rwip_eif_api_init(void)
{
	uart_api.read = &uart_read;
	uart_api.write = &uart_write;
	uart_api.flow_on = &uart_flow_on;
	uart_api.flow_off = &uart_flow_off;
}

const struct rwip_eif_api* rwip_eif_get(uint8_t idx)
{
    const struct rwip_eif_api* ret = NULL;

    if(uart_api.read == NULL)
    {
        return ret;
    }
    switch(idx)
    {
        case 0:
        {
            ret = &uart_api;
        }
        break;
        #if PLF_UART2
        case 1:
        {
            ret = &uart_api;
        }
        break;
        #endif // PLF_UART2
        default:
        {
            ASSERT_INFO(0, idx, 0);
        }
        break;
    }
    return ret;
}

#if	(ROM_REGISTER_CALLBACK)
extern struct rom_env_tag rom_env;
void rom_env_init(struct rom_env_tag *api)
{
	memset(&rom_env,0,sizeof(struct rom_env_tag));
	rom_env.prf_get_id_from_task = prf_get_id_from_task;
	rom_env.prf_get_task_from_id = prf_get_task_from_id;
	rom_env.prf_init = prf_init;	
	rom_env.prf_create = prf_create;
	rom_env.prf_cleanup = prf_cleanup;
	rom_env.prf_add_profile = prf_add_profile;
	rom_env.rwble_hl_reset = rwble_hl_reset;
	rom_env.rwip_reset = rwip_reset;
#if SYSTEM_SLEEP		
	rom_env.rwip_prevent_sleep_set = rwip_prevent_sleep_set;
       rom_env.rwip_prevent_sleep_clear = rwip_prevent_sleep_clear;
	rom_env.rwip_sleep_lpcycles_2_us = rwip_sleep_lpcycles_2_us;
	rom_env.rwip_us_2_lpcycles = rwip_us_2_lpcycles;
	rom_env.rwip_wakeup_delay_set = rwip_wakeup_delay_set;
#endif	
	rom_env.platform_reset = platform_reset;
	rom_env.assert_err = assert_err;
	rom_env.assert_param = assert_param;
	rom_env.Read_Uart_Buf = Read_Uart_Buf;
	rom_env.uart_clear_rxfifo = uart_clear_rxfifo;
	
}
#endif

#if (BT_DUALMODE_RW == 1)
void rw_ble_env_init(void)
{
    uint8_t i;
    struct bd_addr ble_addr;
    struct bd_name ble_name;
    app_env_handle_t env_h = app_env_get_handle();

    if(env_h->env_cfg.ble_para.ble_flag & ENV_LE_FLAG_LE_ENABLE)
    {
        uint8_t name_len = 0;
        uint8_t name[32];

        nvds_put(NVDS_TAG_BLE_ENABLE, 2, (uint8_t *)&env_h->env_cfg.ble_para.ble_flag);
        
        if(env_h->env_cfg.ble_para.ble_flag & ENV_LE_FLAG_ADDR_NAME_CONFIG_ENABLE)
        {
            memcpy((uint8_t *)&ble_addr, (uint8_t *)env_h->env_cfg.ble_para.device_addr.b, 6);
            memcpy((uint8_t *)&(ble_name.name[0]), (uint8_t *)env_h->env_cfg.ble_para.device_name, 32);
            
            name_len = j_strlcpy((char*)name, (char*)env_h->env_cfg.ble_para.device_name, 32);
            name_len = co_min(name_len, 18);
        }
        else
        {
            memcpy((uint8_t *)&ble_addr, (uint8_t *)app_env_local_bd_addr(), 6);
            memcpy((uint8_t *)&(ble_name.name[0]), (uint8_t *)app_env_local_bd_name(), 32);
            
            ble_addr.addr[0] ^= 0x55;
            
            ble_name.namelen = co_min(strlen((char *)&(ble_name.name[0])), 15);
            ble_name.name[ble_name.namelen] = '_';
            ble_name.namelen++;
            ble_name.name[ble_name.namelen] = 'L';
            ble_name.namelen++;
            ble_name.name[ble_name.namelen] = 'E';
            ble_name.namelen++;
            name_len = ble_name.namelen;
        }
        nvds_put(NVDS_TAG_BD_ADDRESS, 6, (uint8_t *)&ble_addr);
        nvds_put(NVDS_TAG_DEVICE_NAME, name_len, (uint8_t *)&(ble_name.name[0]));
        
        if(env_h->env_cfg.ble_para.ble_flag & ENV_LE_FLAG_ADV_CONFIG_ENABLE)
        {
            nvds_put(NVDS_TAG_BLE_ADV_CONFIG_ENABLE, 2, (uint8_t *)&env_h->env_cfg.ble_para.adv_interval);
        }
        
        if(env_h->env_cfg.ble_para.ble_flag & ENV_LE_FLAG_UPDATE_CONFIG_ENABLE)
        {
            nvds_put(NVDS_TAG_BLE_UPDATE_CONFIG_ENABLE, 6, (uint8_t *)&env_h->env_cfg.ble_para.conn_update_delay_time);
        }

        if(1)
        {
            os_printf("------------------------------\r\n");
            
            os_printf("| Rwble name: ");
            for(i = 0; i < name_len; i++)
            {
                os_printf("%c", ble_name.name[i]);
            }
            os_printf("\r\n");
            
            os_printf("| Rwble addr: ");
            for(i = 0; i < 6; i++)
            {
                os_printf("%x", ble_addr.addr[5-i]);
                if(i < 5)
                    os_printf(":");
            }
            os_printf("\r\n");
            
            os_printf("------------------------------\r\n");
        }
    }
}

void rw_ble_init(void)
{
    uint8_t ext_wakeup_enable = 0x01;
    
	////IP Clock source enable
	system_peri_clk_enable(SYS_PERI_CLK_RWBT);
	system_peri_clk_gating_disable(SYS_PERI_CLK_RWBT);
	system_ctrl(SYS_CTRL_CMD_RW_CLK_BYPASS, MAX_SYSTEM_0x1D_RWCLK_BPS);
	system_ctrl(SYS_CTRL_CMD_BK24_EN, MAX_SYSTEM_0x1D_BK24_ENABLE);
    system_ctrl(SYS_CTRL_CMD_RW_ISO_EN, MAX_SYSTEM_0x1D_RW_ISO_EN);
	system_mem_clk_enable(SYS_MEM_CLK_RWBT0 | SYS_MEM_CLK_RWBT1);

    srand(1);
    
    rwip_eif_api_init();

    // Initialize NVDS module
    struct nvds_env_tag env;
    env.flash_read = flash_read_data;
    env.flash_write = flash_write_data;
    env.flash_erase = flash_erase_sector;
    nvds_init(env);

    rw_ble_env_init();

    // ext_wakeup enable, yangyang, 2021/02/27
    nvds_put(NVDS_TAG_EXT_WAKEUP_ENABLE, 1, (uint8_t *)&ext_wakeup_enable);
    
#if	(ROM_REGISTER_CALLBACK)
    rom_env_init(&rom_env);
#endif
#if (BT_DUALMODE_RW == 1)
    rw_ext_wakeup_generate();  // bug fix: watch dog can't reset RW sleep 
#endif	
	// Initialize RW SW stack
    rwip_init(0);

    rwip_1st_wakeup();         // bug fix: watch dog can't reset RW sleep 
    
	system_peri_mcu_irq_enable(SYS_PERI_IRQ_RWBT0 | SYS_PERI_IRQ_RWBT1 | SYS_PERI_IRQ_RWBT2);
}

void rw_ble_schedule(void)
{
    rwip_schedule();

}
#endif
/// @} DRIVERS
