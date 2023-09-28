/**
 * @file   tlv.c
 * @author
 *
 * @brief
 *
 * (c) 2018 BEKEN Corporation, Ltd. All Rights Reserved
 */
	 #include "driver_beken_includes.h"
	 #include "app_beken_includes.h"
	 #include "beken_external.h"

#include "app_tlv.h"
CONST char env_chip_magic[8] = {'B','K','3','2','8','8','0','1'};

t_CHECK_OF_TLV app_check_tlv_data_chip_magic(uint8_t *chip_magic,uint8_t encrypt)
{
    uint8_t chip_magic_tmp[8];
    char env_chip_null[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    if(encrypt)
        memcpy((uint8_t *)chip_magic_tmp, chip_magic, sizeof(env_chip_magic));
    else
        flash_read_data((uint8_t*)chip_magic_tmp,(uint32_t)chip_magic,sizeof(env_chip_magic));
    if(!os_memcmp(chip_magic_tmp,env_chip_magic,sizeof(env_chip_magic))) /* correct cfg data */
    {
        //LOG_I(APP, "Get TLV data from FLASH\r\n");
        return TLV_RST_TRUE;
    }
    else if(!os_memcmp(chip_magic_tmp,env_chip_null,sizeof(env_chip_null)))
    {
        
        return TLV_RST_NULL;
    }
    else
    {
    //LOG_I(APP, "Set default TLV data\r\n");
        return TLV_RST_ERR;
    }
}
/* Find the end of tlv struct */
uint32_t app_search_tlv_end(t_TYPE_SECTOR sector,uint16_t len)
{    
    uint32_t pos = app_env_get_flash_addr(sector);
    uint32_t pos_limit = app_env_get_flash_addr(sector+1);
    BOOL tlv_valid = TRUE;
    BOOL tlv_loop = TRUE;
    TLV_TYPE tlv_data;

    if(sector>=TLV_SECTOR_ENVEND)
        return 0xffffffff;
    pos = app_env_get_flash_addr(sector);
    pos_limit = app_env_get_flash_addr(sector+1);
    if(app_check_tlv_data_chip_magic((uint8_t *)pos ,0) == TLV_RST_TRUE)
    {
        pos += sizeof(env_chip_magic);
        while(tlv_loop)
        {
            flash_read_data((uint8_t *)&tlv_data, (uint32_t)pos, sizeof(TLV_TYPE));
            switch(tlv_data.type)
            {
                case TLV_TYPE_CALI_END:
                    tlv_valid = FALSE;
                    break;
                case TLV_TYPE_CALI_END2:
                    tlv_loop = FALSE;
                    break;
                default:break;
            }
            if(tlv_loop)
            {
                pos += sizeof(TLV_TYPE);
                if(tlv_valid)   // this type is valid;
                {
                    pos += tlv_data.len;        
                } 
            }
        }
        if(pos_limit > (pos + len + sizeof(TLV_TYPE)))
        {
            return pos;
        }
        else
        {
            return 0xffffffff;
        }
    }
    else if( app_check_tlv_data_chip_magic((uint8_t *)pos ,0) == TLV_RST_NULL)
    {
        return pos;  // the sector 
    }
    else
    {
        return 0xffffffff;  
    }
}
/* Appending write the tlv data */
uint32_t app_append_tlv_data(t_TYPE_SECTOR sector,t_TYPE_of_TLV type,uint8_t *buf,uint16_t len)
{
    uint32_t pos = app_search_tlv_end(sector,len);
    uint32_t sect_begin = app_env_get_flash_addr(sector);
    if(pos != 0xffffffff)
    {
        uint8_t *data = NULL;
        uint8_t index = 0;
        if(pos == sect_begin)
        {
            data = jmalloc(len + sizeof(TLV_TYPE)+sizeof(env_chip_magic), M_ZERO);
            if(data)
            {
                memcpy(data,env_chip_magic,sizeof(env_chip_magic));
                index = sizeof(env_chip_magic);
            }
        }
        else
        {
            data = jmalloc(len + sizeof(TLV_TYPE), M_ZERO);
        }
        if(data)
        {
            TLV_TYPE *tlv_data = (TLV_TYPE *)(data+index); 
            tlv_data->type = type;
            tlv_data->len = len;
            memcpy(tlv_data->value,buf,len);
            flash_write_data(data, (uint32)pos, len + sizeof(TLV_TYPE)+index);
            jfree(data);
            INFO_PRT("TLV.append.end:0x%x\r\n",pos);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        FATAL_PRT("TLV.append.err\r\n");
        return FALSE;
    }
}

void app_get_tlv_data(uint8_t *pos, uint8_t encrypt)
{
    app_env_handle_t env_h = app_env_get_handle();
    _t_101xbin_base_info base_info_tmp;
    BOOL tlv_loop = TRUE;
    BOOL tlv_valid = TRUE;
    TLV_TYPE tlv_data;
    uint8_t *dst;
    uint32_t len = 0;

    while(tlv_loop)
    {
        if(encrypt)
            memcpy((uint8_t *)&tlv_data, pos, sizeof(TLV_TYPE));
        else
            flash_read_data((uint8_t *)&tlv_data, (uint32_t)pos, sizeof(TLV_TYPE));
        dst = NULL;
        tlv_valid = TRUE;
        switch(tlv_data.type)
        {
            case TLV_TYPE_CALI_END:
                tlv_valid = FALSE;
                break;
            case TLV_TYPE_CALI_END2:
            	tlv_loop = FALSE;
            	break;
            case TLV_TYPE_CFG_TOTAL:
            	dst = (uint8_t *)&env_h->env_cfg;
            	len = sizeof(env_h->env_cfg);
            	break;
            case TLV_TYPE_CFG_BT_NAME:
            	memset((uint8*)&env_h->env_cfg.bt_para.device_name[0],0,sizeof(env_h->env_cfg.bt_para.device_name));
            	dst = (uint8_t *)&env_h->env_cfg.bt_para.device_name;
            	len =  sizeof(env_h->env_cfg.bt_para.device_name);
            	break;
            case TLV_TYPE_CFG_BT_ADDR:
            	dst = (uint8_t *)&env_h->env_cfg.bt_para.device_addr;
            	len =  sizeof(env_h->env_cfg.bt_para.device_addr);
            	break;
            case TLV_TYPE_CFG_SYS_PARA:
            	dst = (uint8_t *)&env_h->env_cfg.system_para;
            	len =  sizeof(env_h->env_cfg.system_para);
            	break;
            case TLV_TYPE_FACT_BT_NAME:
            	dst = (uint8_t *)&env_h->env_cfg.bt_para.device_name;
            	len =  sizeof(env_h->env_cfg.bt_para.device_name);
            	break;
            case TLV_TYPE_FACT_BT_ADDR:
            	dst = (uint8_t *)&env_h->env_cfg.bt_para.device_addr;
            	len =  sizeof(env_h->env_cfg.bt_para.device_addr);
            	break;
            case TLV_TYPE_BASE_INFO:
            	dst = (uint8_t *)&base_info_tmp;
            	len =  sizeof(base_info_tmp);
            	break;
            default:  // the type is valid,but don't parse;
                len = 0;
            	break;
        }

        if(tlv_loop)
        {
            pos += sizeof(TLV_TYPE);
            if(tlv_valid)   // this type is valid;
            {
                if(dst != NULL)
                {
            	    if(tlv_data.len > len) // TLV.len is error
                    {
                        FATAL_PRT("TLV.err\r\n");
                    }
                    else
                    {
                        len = tlv_data.len;
                    }
            		if(encrypt)
            			memcpy(dst, pos, len);
            		else
            			flash_read_data(dst, (uint32_t)pos, len);
                }
            	pos += tlv_data.len;
            }
        }

#if (CONFIG_DRIVER_OTA == 1)
        if(tlv_loop == FALSE)
        {
         driver_ota_info_s ota_info;

         flash_read_data((uint8_t*)&ota_info, OTA_INFO_ADDR, sizeof(driver_ota_info_s));

         if((OTA_MARK_USED == ota_info.mark) && (IMAGE_ENV == ota_info.flag))
         {
             uint8_t data[40];  // tlv head = 4,ota_info = 32;
             dst = (uint8_t *)&env_h->env_cfg.bt_para.device_addr;
        	 len = sizeof(env_h->env_cfg.bt_para.device_addr);
             flash_read_data(dst, OTA_INFO_BT_ADDR_ADDR, len);
             ((TLV_TYPE*)data)->type = TLV_TYPE_CFG_BT_ADDR;
             ((TLV_TYPE*)data)->len = len;
             memcpy(&(((TLV_TYPE*)data)->value), dst, len);
             flash_write_data(data, (uint32_t)pos, len + sizeof(TLV_TYPE));
             pos += len + sizeof(TLV_TYPE);

             dst = (uint8_t *)&env_h->env_cfg.bt_para.device_name;
        	 len = sizeof(env_h->env_cfg.bt_para.device_name);
             flash_read_data(dst, OTA_INFO_BT_NAME_ADDR, len);
             ((TLV_TYPE*)data)->type = TLV_TYPE_CFG_BT_NAME;
             ((TLV_TYPE*)data)->len = len;
             memcpy(&(((TLV_TYPE*)data)->value), dst, len);
             flash_write_data(data, (uint32_t)pos, len + sizeof(TLV_TYPE));
             pos += len + sizeof(TLV_TYPE);
         }
        }
#endif
    }
}
