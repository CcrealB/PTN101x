/**
 * @file   tlv.c
 * @author
 *
 * @brief
 *
 * (c) 2018 BEKEN Corporation, Ltd. All Rights Reserved
 */

#ifndef __TLV_H__
#define __TLV_H__

#include <stdint.h>

extern CONST char env_chip_magic[8];

typedef union{
    uint8_t b[28];
    struct{
    	uint32_t szSrcBin;
    	uint32_t szWav;
    	uint16_t szCfg;
    	};
    }_t_101xbin_base_info;

typedef enum {
    TLV_TYPE_CALI_END            = 0,   //  invalid type
    TLV_TYPE_CALI_DC_OFFSET_DIFF_DISPGA,
    TLV_TYPE_CALI_DC_OFFSET_SNGL_DISPGA,
    TLV_TYPE_CALI_DC_OFFSET_DIFF_ENPGA,
    TLV_TYPE_CALI_DC_OFFSET_SNGL_ENPGA,
    TLV_TYPE_CALI_SARADC,
    TLV_TYPE_CALI_VOLTAGE,
    TLV_TYPE_CALI_CHARGE,
    TLV_TYPE_CALI_TEMPR,
    
    TLV_TYPE_CALI_ANC_PARAM = 0xAA01,
    TLV_TYPE_CALI_ANC_COEFS = 0xAA02,
    TLV_TYPE_CALI_ANC_SCALER= 0xAA03,

    TLV_TYPE_CFG_TOTAL          = 0xCF01, //0xCF01 for PTN101 toolkit7.0?  0xCF00,
    TLV_TYPE_CFG_BT_NAME        = 0xCF10,
    TLV_TYPE_CFG_BT_ADDR,
    TLV_TYPE_CFG_SYS_PARA,

    TLV_TYPE_FACT_BT_NAME       = 0xFA10,
    TLV_TYPE_FACT_BT_ADDR,
    TLV_TYPE_BASE_INFO          = 0xBA10,

    TLV_TYPE_CALI_END2          = 0xFFFF     // tlv data end
}t_TYPE_of_TLV;

typedef enum {
    TLV_SECTOR_ENVCFG,
    TLV_SECTOR_ENVDATA,
    TLV_SECTOR_ENVCALI,
    TLV_SECTOR_ENVEND
}t_TYPE_SECTOR;

typedef enum {
    TLV_RST_ERR,
    TLV_RST_NULL,
    TLV_RST_TRUE,
}t_CHECK_OF_TLV;    

typedef struct {
    uint16_t type;
    uint16_t len;
    uint8_t  value[];
}__PACKED_POST__ TLV_TYPE;

t_CHECK_OF_TLV app_check_tlv_data_chip_magic(uint8_t *chip_magic,uint8_t encrypt);
void app_get_tlv_data(uint8_t *pos,uint8_t encrypt);
uint32_t app_append_tlv_data(t_TYPE_SECTOR sector,t_TYPE_of_TLV type,uint8_t *buf,uint16_t len);
uint32_t app_search_tlv_end(t_TYPE_SECTOR sector,uint16_t len);


#endif//__tlv_H__
