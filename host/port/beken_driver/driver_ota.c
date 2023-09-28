#include "config.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "bt_spp.h"
#if (CONFIG_OTA_BLE == 1)
#include "app.h"
#endif

#if (CONFIG_DRIVER_OTA == 1)  

driver_ota_param_s driver_ota_param;

extern void set_flash_protect(uint8_t all);

void driver_ota_init(void)
{
    memset((uint8_t*)&driver_ota_param, 0, sizeof(driver_ota_param_s));
}

uint8_t driver_ota_is_ongoing(void)
{
    return driver_ota_param.update_flag; 
}

void driver_ota_set_ongoing(uint8_t flag)
{
    driver_ota_param.update_flag = flag; 
}

uint8_t driver_ota_tx_arqn_nak_flag_get(void)
{
    return driver_ota_param.tx_arqn_nak_flag;
}

void driver_ota_tx_arqn_nak_flag_set(uint8_t value)
{
    driver_ota_param.tx_arqn_nak_flag = value;
}

uint16_t driver_ota_get_mark(uint32_t addr)
{
    uint16_t mark = 0x1234;
    
    flash_read_data((uint8_t*)&mark, addr, sizeof(mark));
    
    return mark;
}

uint16_t driver_ota_get_version(uint8_t flag)
{
    uint16_t version = OTA_INIT_VERSION;
    uint32_t addr = 0;
 	uint16_t mark = 0;
    
    if(IMAGE_MCU == flag)
    {
        addr = OTA_MCU_ADDR;
    }
    else if(IMAGE_DSP == flag)
    {
        addr = ((*((volatile uint32_t*)OTA_DSP_ADDR_POINT))*34)/32;
    }
    else if((IMAGE_ENV == flag) || (IMAGE_ENV_ADDR == flag))
    {
        addr = FLASH_ENVDATA_DEF_ADDR_ABS;
    }
    
    if(addr >= OTA_MCU_ADDR)    
    {
        addr -= INFO_LEN;                            // Info Section Address
        mark = driver_ota_get_mark(addr);
        
        if(OTA_MARK_SUCC == mark)
            flash_read_data((uint8_t*)&version, addr + INFO_VER_OFFSET, sizeof(version));
    }

    return version;
}

uint16_t gen_crc16(uint16_t crc, uint8_t* data, uint32_t len) 
{        
    uint32_t i;
    for (i = 0; i < len; i++) 
    {        
         crc = ((crc >> 8) | (crc << 8)) & 0xFFFF;   
         crc ^= (data[i] & 0xFF);// byte to int, trunc sign    
         crc ^= ((crc & 0xFF) >> 4);      
         crc ^= (crc << 12) & 0xFFFF;   
         crc ^= ((crc & 0xFF) << 5) & 0xFFFF;   
    }       
    return (crc & 0xFFFF);
}

uint16_t driver_ota_calc_crc(void)
{
    uint8_t data[500];
    uint16_t crc = 0xFFFF;
    uint16_t len = 0;
    uint32_t read_addr = driver_ota_param.flash_addr;
    uint32_t end_addr = driver_ota_param.flash_addr + driver_ota_param.flash_offset;

    while(read_addr < end_addr)
    {
        if((read_addr + sizeof(data)/sizeof(data[0])) <= end_addr)
            len = sizeof(data)/sizeof(data[0]);
        else
            len = end_addr - read_addr;
        
        flash_read_data(data, read_addr, len);
        crc = gen_crc16(crc, data, len);
        read_addr += len; 
    }
    return crc;
}

void driver_ota_erase_flash(void)
{
    uint32_t start_addr = 0;
    uint32_t end_addr = 0;
    uint32_t addr = 0;
    driver_ota_info_s ota_info;
    
    flash_read_data((uint8_t*)&ota_info, OTA_INFO_ADDR, sizeof(driver_ota_info_s));

    if((OTA_MARK_INIT != ota_info.mark) && ota_info.len)
    {
        start_addr = ota_info.addr1;
        end_addr   = ota_info.addr1 + ota_info.len;
        
        LOG_I(OTA, "driver_ota_erase_flash, addr:0x%x - 0x%x, please waiting...\r\n", start_addr, end_addr);
        LOG_I(OTA, "......\r\n"); 
        
        driver_ota_param.update_flag = 1;
        
        for(addr = end_addr; addr > start_addr; )  /* erase addr from high to low, ensure ota_mark erased last */
        {
            if((((addr - 0x10000) % 0x10000) == 0) && ((addr - 0x10000) >= start_addr))
            {
                addr -= 0x10000;
                flash_erase_sector(addr, FLASH_ERASE_64K);
            }
            else if((((addr - 0x8000) % 0x8000) == 0) && ((addr - 0x8000) >= start_addr))
            {
                addr -= 0x8000;
                flash_erase_sector(addr, FLASH_ERASE_32K);
            }
            else
            {
                addr -= 0x1000;
                flash_erase_sector(addr, FLASH_ERASE_4K);
            }
            CLEAR_WDT;
        }
        
        flash_erase_sector(OTA_INFO_ADDR, FLASH_ERASE_4K);
        
        driver_ota_param.update_flag = 0;
        
        LOG_I(OTA, "driver_ota_erase_flash FINISH!\r\n"); 
    }
}

void driver_ota_write_flash(void)
{

    if(driver_ota_param.flash_protect_flag == 0x01)         //flash write unprotect
    {
        flash_set_line_mode(FLASH_LINE_2);
        set_flash_protect(0);
        driver_ota_param.flash_protect_flag = 0;
        /* write ota information in flash */
        flash_write_data((uint8_t*)&driver_ota_param.ota_info, OTA_INFO_ADDR, OTA_INFO_LEN); 
    }
    else if(driver_ota_param.flash_protect_flag == 0x02)    //flash write protect
    {
        flash_set_line_mode(FLASH_LINE_2);
        set_flash_protect(1);
        driver_ota_param.flash_protect_flag = 0;
    }

    if(driver_ota_is_ongoing() && driver_ota_param.data_len)
    {
        flash_write_page_data(driver_ota_param.data, driver_ota_param.flash_addr + driver_ota_param.flash_offset, driver_ota_param.data_len);
        driver_ota_param.flash_offset += driver_ota_param.data_len;
        driver_ota_param.data_len = 0;
    }
}

void driver_ota_start(driver_ota_head_s* head_info)
{
    driver_ota_param.crc = head_info->crc;
    driver_ota_param.flash_protect_flag = 0x01;         //flash write unprotect
    driver_ota_param.flash_addr = head_info->addr1;
    
    /* save ota information to write in flash*/
    driver_ota_param.ota_info.mark = OTA_MARK_ONGO;
    memcpy((uint8_t*)&driver_ota_param.ota_info.flag, (uint8_t*)&head_info->flag, OTA_INFO_LEN - sizeof(driver_ota_param.ota_info.mark));
    
    /* set BLE adv disable & BT scan disable & exit sniff mode */
    if(1)
    {        
        //LEadv_Advertise_Disable_Enable(0);
        
        bt_unit_set_scan_enable(app_get_sys_handler()->unit, HCI_NO_SCAN_ENABLE);         
        app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(0), 0);  
    }
}

void driver_ota_save_data(uint8_t* data_ptr, uint16_t data_len)
{
    if((driver_ota_param.data_len + data_len) > (sizeof(driver_ota_param.data)/sizeof(driver_ota_param.data[0])))
    {
        while(driver_ota_param.data_len)
        {
            driver_ota_write_flash();
            LOG_I(OTA, "driver_ota_save_data overflow!!!\r\n");
        }
    }
    memcpy(driver_ota_param.data + driver_ota_param.data_len, data_ptr, data_len);
    driver_ota_param.data_len += data_len;
}

uint8_t driver_ota_finish_result(void)
{
    uint8_t result = OTA_FAIL;
    uint16_t crc;
    uint16_t ota_mark = 0xFFFF;

    while(driver_ota_param.data_len)
    {
        driver_ota_write_flash();         //ensure last data write to flash
    }
    
    crc = driver_ota_calc_crc();
    
    LOG_I(OTA, "crc: calc[0x%x], refer[0x%x]\r\n", crc, driver_ota_param.crc);
    if(crc == driver_ota_param.crc)
    {
        result = OTA_SUCCESS;
        ota_mark = OTA_MARK_SUCC;
        LOG_I(OTA, "OTA_SUCCESS!!!\r\n");
    }
    else
    {
        result = OTA_FAIL;
        ota_mark = OTA_MARK_FAIL;
        LOG_I(OTA, "OTA_FAIL!!!\r\n");
    }

    /* bt_addr & bt_name reserved */
    if((driver_ota_param.ota_info.flag == IMAGE_ENV) || (driver_ota_param.ota_info.flag == IMAGE_MCU_ENV))
    {  
        app_env_handle_t env_h = app_env_get_handle();
        
        flash_write_data((uint8_t*)&env_h->env_cfg.bt_para.device_addr, OTA_INFO_BT_ADDR_ADDR, sizeof(env_h->env_cfg.bt_para.device_addr));    
        flash_write_data((uint8_t*)&env_h->env_cfg.bt_para.device_name, OTA_INFO_BT_NAME_ADDR, sizeof(env_h->env_cfg.bt_para.device_name));     
    }

    /* write ota mark information into flash */
    driver_ota_param.ota_info.mark = ota_mark;
    driver_ota_param.ota_info.info_crc = gen_crc16(0xFFFF, (uint8_t*)&driver_ota_param.ota_info, OTA_INFO_LEN);
    flash_write_data((uint8_t*)&driver_ota_param.ota_info, OTA_INFO_ADDR, sizeof(driver_ota_info_s));
    
    driver_ota_param.flash_protect_flag = 0x02;        //flash write protcet
    
    return result;
}

void driver_ota_reboot(uint16_t time_us)
{
    LOG_I(OTA, "driver_ota_reboot\r\n\r\n\r\n");
    
    driver_ota_init();
    
    jtask_stop(app_get_sys_handler()->ota_reboot_task);
    jtask_schedule(app_get_sys_handler()->ota_reboot_task,
                   time_us,
                   (jthread_func)BK3000_wdt_reset,
                   (void *)0);
}

void driver_ota_pdu_send(uint8_t *pValue, uint16_t length, uint16_t handle)
{
#if (CONFIG_OTA_SPP == 1)
    if(spp_is_connected())
    {
        spp_send((char*)pValue, length);
    }
    else
#endif
#if (CONFIG_OTA_BLE == 1)
    if(appm_get_connection_num())
    {
        app_ota_ble_send(pValue, length);
    }
    else
#endif
    {
        LOG_I(OTA, "driver_ota_pdu_send error!!!\r\n");
    }
}

#endif
