#include <string.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

/*****************PTN101 8M bit Flash mapping*********************
obsolute relative                                                
000H     000H     |----------------------------------------------|
008H     008H     |   PC Entrance                | [encrypt,crc] |
                  |----------------------------------------------|
110H     100H     |   Keyword(6B=="PTN101")      |               |
118H     108H     |   Version(1B)                | [unencrypted] |
119H     109H     |   Flash_info(1B)             | [no_crc]      |
220H     200H     |----------------------------------------------|
                  |   BOOTLOADER(8KB-220B)       | [encrypt,crc] |
2002H             |----------------------------------------------|
                  |   MCU_INFO(34B)        |     | [unencrypted] |
2024H    1E40H    |------------------------|     |---------------|
                  |   MCU_CODE(450KB)      | MCU |               |
                  |                        |     | [encrypt,crc] |
                  |                        |     |               |
                  |----------------------------------------------|
                  |   DSP_INFO(34B)        |     | [unencrypted] |
                  |------------------------|     |---------------|
                  |   DSP_CODE(0KB)        | DSP |               |
                  |                        |     | [encrypt,crc] |
                  |                        |     |               |
07D000H           |----------------------------------------------|
                  |   OTA_DATA(450KB)      |     |               |
                  |                        | OTA |               |
                  |                        |     |               |
0E3000H           |------------------------------|               |
                  |   ENV_WAV(96KB)        |     |               |
                  |                        |     |               |
0FB000H           |------------------------| ENV | [unencrypted] |
                  |   ENV_CFG(4KB)         |     | [no_crc]      |
0FBFE0H           |------------------------|     |               |
                  |   ENV_CFG_INFO(34B)    |     |               |
0FC000H           |------------------------------|               |
                  |   ENV_DATA/Linkkey(4KB)      |               |
0FD000H           |------------------------------|               |
                  |   OTA_INFO(4KB)              |               |
0FE000H           |------------------------------|               |
                  |   BLE_NVDS/Reserved(4KB)     |               |
0FF000H           |------------------------------|               |
                  |   Calibration Data(4KB)      |               |
100000H           |----------------------------------------------|
*****************************************************************/

/*****************PTN101 16M bit Flash mapping*********************
obsolute relative                                                 
000H     000H     |----------------------------------------------|
008H     008H     |   PC Entrance                | [encrypt,crc] |
                  |----------------------------------------------|
110H     100H     |   Keyword(6B=="PTN101")      |               |
118H     108H     |   Version(1B)                | [unencrypted] |
119H     109H     |   Flash_info(1B)             | [no_crc]      |
220H     200H     |----------------------------------------------|
                  |   BOOTLOADER(8KB-220B)       | [encrypt,crc] |
2002H             |----------------------------------------------|
                  |   MCU_INFO(34B)        |     | [unencrypted] |
2024H    1E40H    |------------------------|     |---------------|
                  |   MCU_CODE(650KB)      | MCU |               |
                  |                        |     | [encrypt,crc] |
                  |                        |     |               |
                  |----------------------------------------------|
                  |   DSP_INFO(34B)        |     | [unencrypted] |
                  |------------------------|     |---------------|
                  |   DSP_CODE(512KB)      | DSP |               |
                  |                        |     | [encrypt,crc] |
                  |                        |     |               |
                  |----------------------------------------------|
                  |   OTA_DATA(650KB)      |     |               |
                  |                        | OTA |               |
                  |                        |     |               |
                  |------------------------------|               |
                  |   ENV_WAV(208KB)       |     |               |
                  |                        |     |               |
1FB000H           |------------------------| ENV | [unencrypted] |
                  |   ENV_CFG(4KB)         |     | [no_crc]      |
1FBFE0H           |------------------------|     |               |
                  |   ENV_CFG_INFO(34B)    |     |               |
1FC000H           |------------------------------|               |
                  |   ENV_DATA/Linkkey(4KB)      |               |
1FD000H           |------------------------------|               |
                  |   OTA_INFO(4KB)              |               |
1FE000H           |------------------------------|               |
                  |   BLE_NVDS/Reserved(4KB)     |               |
1FF000H           |------------------------------|               |
                  |   Calibration Data(4KB)      |               |
200000H           |----------------------------------------------|
*****************************************************************/

typedef struct
{
    uint16_t mark;
    uint16_t flag;
    uint32_t len;
    uint16_t ver;
    uint8_t reserved[6];
}__PACKED_POST__ mcu_dsp_info_s;

MCU_INFO_CODE uint16_t mcu_info[8] = 
{
    0x5555,   
    0x0001,
    0x0000,
    0x0000,
    0x260C,  // 4.2.0.1
    0x0000,
    0x0000,
    0x0000         
};

BOOT_CODE uint32_t Boot_flash_read_mID(void) 
{
    unsigned int temp0;
    uint32_t flash_id;
    
    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (temp0             &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_RDID << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);
    
    flash_id = REG_FLASH_0x04;
    
    return (flash_id & 0xffffff);
}

BOOT_CODE void Boot_flash_set_line_mode(uint8_t mode) 
{
    if(mode == FLASH_LINE_2) 
    {       
        REG_FLASH_0x07 &= (~(7<<SFT_FLASH_0x07_MODE_SEL));
        REG_FLASH_0x07 |= (1<<SFT_FLASH_0x07_MODE_SEL);
    }
}

BOOT_CODE void Boot_flash_set_config_start(uint16_t op)
{
    uint32_t temp0 = REG_FLASH_0x00;
    
	REG_FLASH_0x00 = (  (temp0             &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (op                << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (0x1               << SFT_FLASH_0x00_WP_VALUE)); // make WP equal 1 not protect SRP
    while(REG_FLASH_0x00 & 0x80000000);
}

BOOT_CODE void Boot_flash_set_protect(uint8_t all) 
{
    unsigned int temp0;
    uint16_t wrsr_data = 0;
    uint32_t flash_id = Boot_flash_read_mID();

    while(REG_FLASH_0x00 & 0x80000000){}

    temp0 = REG_FLASH_0x07; // ����WRSR Status data
    temp0 &= 0xfefe0fff;    // set cmp = 0 , set [BP4:BP0] = 0
    
    switch(flash_id)
    {
        case 0xc84015:  // 16M flash
        case 0x0b4015:  // 16M flash 
            if(all == 1)
            {
                wrsr_data = 0x0011;
            }
        	else
            {
                wrsr_data = 0x0000;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            Boot_flash_set_config_start(FLASH_OPCODE_WRSR); 
            break;
        
        case 0x514014:  // 8M flash (MD25D80)
        case 0xc86516:  // 8M flash
            if(all == 1)
            {
                wrsr_data = 0x001C;
            }
        	else
        	{
                wrsr_data = 0x0000;  // protect none
        	}               
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            Boot_flash_set_config_start(FLASH_OPCODE_WRSR);   
            break;
            
        case 0xC84014:  // GD 8M   (GD25Q80CSIG)
        case 0x856013:  // PUYA 4M  (P25Q40H)
        case 0x856014:  // PUYA 8M  (P25Q80H)
            if(all == 1)
            {
                wrsr_data = 0x4000;       // protect ALL
            }
        	else
        	{
                wrsr_data = 0x0068;       // protect 0H~1FFFH
        	}               
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            Boot_flash_set_config_start(FLASH_OPCODE_WRSR2);
            break;
        
        case 0x856015:  // PUYA 16M (P25Q16H)
        case 0x854215:  // PUYA 16M (P25Q16SH)
            // CMP bit = 1, QE bit = 1, set by Toolkit_V6.0.0 always, WRSR only
            if(all == 1)
            {
                wrsr_data = 0x4000;       // protect ALL
            }
        	else
            {
                wrsr_data = 0x4018;       // protect NONE
            }  
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            Boot_flash_set_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0x856016:  // PUYA 32M (P25Q32H)
            //CMP bit = 1, QE bit = 1, set by Toolkit_V6.0.0 always, WRSR only
            if(all == 1)
            {
                wrsr_data = 0x4000;      // protect ALL
            }
            else
            {
                wrsr_data = 0x401C;      // protect NONE
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            Boot_flash_set_config_start(FLASH_OPCODE_WRSR);
            break;
            
        default:
            break;
    }
    
    while(REG_FLASH_0x00 & 0x80000000);   
}

BOOT_CODE void Boot_flash_erase_sector(uint32_t address, uint8_t erase_size) 
{
    unsigned int temp0;
    uint32_t flash_opcode;

    if(erase_size == FLASH_ERASE_32K)
        flash_opcode = FLASH_OPCODE_BE1;
    else if(erase_size == FLASH_ERASE_64K)
        flash_opcode = FLASH_OPCODE_BE2;
    else
        flash_opcode = FLASH_OPCODE_SE;

    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (address      << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (flash_opcode << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1          << SFT_FLASH_0x00_OP_SW)
                      | (temp0        &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);
}

BOOT_CODE void Boot_flash_read_data(uint8_t *buffer, uint32_t address, uint32_t len) 
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];

    while(REG_FLASH_0x00 & 0x80000000);
    while(len) 
    {
        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr              << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_READ << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1               << SFT_FLASH_0x00_OP_SW)
                          | (reg_value         &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        
        addr += 32;

        for(i = 0; i < 8; i++)
            buf[i] = REG_FLASH_0x02;

        for(i = (address & 0x1F); i < 32; i++) 
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if(len == 0)
                break;
        }
    }
}

BOOT_CODE void Boot_flash_write_data(uint8_t *buffer, uint32_t address, uint32_t len) 
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];
 
    while(REG_FLASH_0x00 & 0x80000000);
    while(len) 
    {
        if((address & 0x1F) || (len < 32))
            Boot_flash_read_data(pb, addr, 32);
        
        for(i = (address & 0x1F); i < 32; i++)
        {
            if(len)
            {
                pb[i] = *buffer++;
                address++;
                len--;
            }
        }

        for(i = 0; i < 8; i++)
            REG_FLASH_0x01 = buf[i];

        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr            << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_PP << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1             << SFT_FLASH_0x00_OP_SW)
                          | (reg_value       &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        addr += 32;
    }
}

BOOT_CODE void Boot_flash_write_page_data(uint8_t *buffer, uint32_t address, uint32_t len) 
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0xFF);
    uint8_t buf[256];
 
    while(REG_FLASH_0x00 & 0x80000000);
    while(len) 
    {
        if((address & 0xFF) || (len < 256))
            Boot_flash_read_data(buf, addr, 256);
        
        for(i = (address & 0xFF); i < 256; i++)
        {
            if(len)
            {
                buf[i] = *buffer++;
                address++;
                len--;
            }
        }
        
        REG_FLASH_0x05 |= MSK_FLASH_0x05_PW_WRITE;
        REG_FLASH_0x09 |= MSK_FLASH_0x09_MEM_ADDR_CLR;
        REG_SYSTEM_0x20 &= ~MSK_SYSTEM_0x20_FLS_MEM_SD;
        
        for(i = 0; i < 256; i++)
            REG_FLASH_0x09 = buf[i];

        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr            << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_PP << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1             << SFT_FLASH_0x00_OP_SW)
                          | (reg_value       &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        
        REG_FLASH_0x05 &= ~MSK_FLASH_0x05_PW_WRITE;
        REG_SYSTEM_0x20 |= MSK_SYSTEM_0x20_FLS_MEM_SD;
        
        addr += 256;
    }
}

BOOT_CODE void Boot_erase_code(uint32_t start_addr, uint32_t end_addr)
{
    while(start_addr < end_addr)
    {
        if(((start_addr % 0x10000) == 0) && ((start_addr + 0x10000) < end_addr))
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_64K);
            start_addr += 0x10000;
        }
        else if(((start_addr % 0x8000) == 0) && ((start_addr+ 0x8000) < end_addr))
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_32K);
            start_addr += 0x8000;
        }
        else
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_4K);
            start_addr += 0x1000;
        }
    }
}

BOOT_CODE uint16_t Boot_gen_crc16(uint16_t crc, uint8_t* data, uint32_t len) 
{        
    uint32_t i;
    for (i = 0; i < len; i++) 
    {        
         crc = ((crc >> 8) | (crc << 8)) & 0xFFFF;   
         crc ^= (data[i] & 0xFF);
         crc ^= ((crc & 0xFF) >> 4);      
         crc ^= (crc << 12) & 0xFFFF;   
         crc ^= ((crc & 0xFF) << 5) & 0xFFFF;   
    }       
    return (crc & 0xFFFF);
}

BOOT_CODE uint16_t Boot_calc_crc(uint32_t start_addr, uint32_t end_addr)
{
    uint8_t data[500];
    uint16_t crc = 0xFFFF;
    uint16_t len = 0;
    
    while(start_addr < end_addr)
    {
        len = ((start_addr + sizeof(data)) <= end_addr) ? sizeof(data) : (end_addr - start_addr);
        Boot_flash_read_data(data, start_addr, len);
        crc = Boot_gen_crc16(crc, data, len);
        start_addr += len; 
    }
    
    return crc;
}

BOOT_CODE void Boot_loader(void)
{
    REG_SYSTEM_0x4D &= ~(1<<24);
    REG_SYSTEM_0x4D |= (1<<22);
#if (CONFIG_DRIVER_OTA == 1) && (defined(CONFIG_OTA_APP))
    driver_ota_info_s ota_info;

    /* Setting wdt max */
    CLEAR_WDT;
    REG_PMU_0x06 |= MSK_PMU_0x06_FLSHSCK_IOCAP;   // Flash clk driver strength

    Boot_flash_read_data((uint8_t*)&ota_info, OTA_INFO_ADDR, sizeof(driver_ota_info_s));
    
    if((OTA_MARK_SUCC == ota_info.mark) && (Boot_gen_crc16(0xFFFF, (uint8_t*)&ota_info, OTA_INFO_LEN) == ota_info.info_crc))
    {
        uint8_t  data[256];
        uint16_t mark = OTA_MARK_FAIL;
        uint32_t len = 0;
        uint32_t offset = 0;
        uint32_t addr2 = 0;

        Boot_flash_set_protect(0);
        Boot_flash_set_line_mode(FLASH_LINE_2); 
        
        do
        {
            addr2 = ota_info.addr2/0x1000*0x1000;          // align at 4K addr

            Boot_erase_code(addr2, addr2 + ota_info.len);
            
            offset = 0;
            
            while(offset < ota_info.len)
            {
                len = ((offset + sizeof(data)) <= ota_info.len) ? sizeof(data) : (ota_info.len - offset);
                
                Boot_flash_read_data(data, ota_info.addr1 + offset, len);
                Boot_flash_write_page_data(data, addr2 + offset, len); 
                
                offset += len;
            }
            
            if(ota_info.crc == Boot_calc_crc(addr2, addr2 + ota_info.len))
            {
                mark = OTA_MARK_SUCC;
            }

            if((IMAGE_ENV == ota_info.flag) || (IMAGE_ENV_ADDR == ota_info.flag))
                Boot_flash_write_data((uint8_t*)&mark, addr2 + ota_info.len - INFO_LEN, 2);
            else
                Boot_flash_write_data((uint8_t*)&mark, ota_info.addr2 - INFO_LEN, 2);
            
        }while(OTA_MARK_SUCC != mark);

        mark = OTA_MARK_USED;
        Boot_flash_write_data((uint8_t*)&mark, OTA_INFO_ADDR, 2);     /* mark the ota data be used after boot success */

        Boot_flash_set_protect(1);  
    }
#else
    REG_PMU_0x06 |= MSK_PMU_0x06_FLSHSCK_IOCAP;   // Flash clk driver strength
#endif
}
