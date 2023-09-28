#include "config.h"
#include "beken_ota.h"
#include "lmp_utils.h"
#if (CONFIG_OTA_BLE == 1)
#include "rwip_config.h"     // SW configuration
#include "ke_task.h"                 // Kernel
#include "app.h"                   // Application Definitions
#include "app_task.h"              // application task definitions
#include "otas_task.h"            // health thermometer functions
#include "prf_types.h"             // Profile common types definition
#include "prf.h"
#include "otas.h"
#include "ke_timer.h"
#include "gattc.h"
#include "app.h"
#endif

#if (BEKEN_OTA == 1)

enum beken_ota_cmd
{
    INQUIRY_INFO_REQ_CMD       = 0x01,
    INQUIRY_INFO_RESP_CMD      = 0x02,
    START_REQ_CMD              = 0x03,
    START_RESP_CMD             = 0x04,
    DATA_SEND_CMD              = 0x05,
    DATA_ERROR_CMD             = 0x06,
    END_REQ_CMD                = 0x07,
    END_RESP_CMD               = 0x08,
    UPDATE_SIZE_REQ_CMD        = 0x09,
    UPDATE_SIZE_RESP_CMD       = 0x0A,
    REBOOT_CMD                 = 0x0B
};

enum beken_ota_location
{
    OTA_LOCATION_A             = 0x01,
    OTA_LOCATION_B             = 0x02
};

typedef struct
{
    uint8_t cmd;
    uint8_t frame_seq;
    uint16_t length;
    uint8_t data[0];
}__attribute__((packed)) beken_ota_pkt_s;

typedef struct
{
    uint8_t frame_seq;
    uint32_t data_addr;
}__attribute__((packed)) beken_ota_param_s;

#if (CONFIG_OTA_SPP == 1)
static uint8_t beken_ota_spp_buff[OTA_BUFFER_SIZE] = {0};
static uint16_t beken_ota_spp_buff_cnt = 0;
#endif

static beken_ota_param_s beken_ota_param;

void beken_ota_pkt_decode(uint8_t *pValue, uint16_t length);
#if (CONFIG_OTA_BLE == 1)
void app_ota_add_otas(void)
{
    struct otas_db_cfg *db_cfg;
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_profile_task_add_cmd, sizeof(struct otas_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 0;//PERM(SVC_AUTH, ENABLE);
    req->prf_task_id = TASK_ID_OTAS;
    req->app_task = TASK_APP;
    req->start_hdl = 0; 

	// Set parameters
    db_cfg = (struct otas_db_cfg* ) req->param;
    // Sending of notifications is supported
    db_cfg->features = OTAS_NTF_SUP;
    
	//bk_printf("app_oad_add_otas d = %x,s = %x\r\n", TASK_GAPM,TASK_APP);
    ke_msg_send(req);
}

void app_ota_ble_send(uint8 *pValue, uint16_t length)
{
    struct otas_tx_pdu * tx_pdu = KE_MSG_ALLOC( OTAS_TX,
                                                prf_get_task_from_id(TASK_ID_OTAS),
                                                TASK_APP,
                                                otas_tx_pdu);

    // Fill in the parameter structure
    tx_pdu->length = length;
    memcpy((uint8*)&tx_pdu->data, pValue, length);

    // Send the message
    ke_msg_send(tx_pdu);
}

static int app_ota_ble_pkt_decode(ke_msg_id_t const msgid, struct otas_rx_pdu *param,
                                    ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    beken_ota_pkt_decode(param->data, param->length);
    return (KE_MSG_CONSUMED);
}

const struct ke_msg_handler app_otas_msg_handler_list[] =
{   
    {OTAS_RX,                 (ke_msg_func_t)app_ota_ble_pkt_decode},	
};

const struct app_subtask_handlers app_otas_handlers =
                              {&app_otas_msg_handler_list[0], (sizeof(app_otas_msg_handler_list)/sizeof(struct ke_msg_handler))};
#endif

void beken_ota_pkt_encode(uint8_t cmd, uint8_t frame_seq, uint8_t *pValue, uint16_t length)
{
    uint8_t data[30];
    __unused uint16_t hanlde = 0;
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)data;
    
    if((length + sizeof(beken_ota_pkt_s)) > sizeof(data)/sizeof(data[0]))
    {
        LOG_I(OTA, "beken_ota_pkt_encode error!!!:%x, %x\r\n", length + sizeof(beken_ota_pkt_s), sizeof(data)/sizeof(data[0]));
        return;
    }
    
    beken_ota_pkt->cmd = cmd;
    beken_ota_pkt->frame_seq = frame_seq;
    beken_ota_pkt->length = length;
    memcpy(&beken_ota_pkt->data, pValue, length);
    
    driver_ota_pdu_send(data, length + sizeof(beken_ota_pkt_s), hanlde);
}

void beken_ota_inquiry_req_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    uint8_t data[7];
    uint8_t location = OTA_LOCATION_A;
    uint16_t version = driver_ota_get_version(IMAGE_MCU);

    if(beken_ota_pkt->length != 0x00)
    {
        LOG_E(OTA, "beken_ota_inquiry_req_handler, length ERROR:%x!\r\n", beken_ota_pkt->length);
        return;
    }
    LOG_I(OTA, "beken_ota_inquiry_req_handler\r\n");

    driver_ota_init();
    driver_ota_set_ongoing(1);

    data[0] = LO_UINT16(OTA_VID);
    data[1] = HI_UINT16(OTA_VID);
    data[2] = LO_UINT16(OTA_PID);
    data[3] = HI_UINT16(OTA_PID);
    data[4] = LO_UINT16(version);
    data[5] = HI_UINT16(version);
    data[6] = location;

    beken_ota_pkt_encode(INQUIRY_INFO_RESP_CMD, beken_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void beken_ota_start_req_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    driver_ota_head_s* head_info = (driver_ota_head_s*)beken_ota_pkt->data;
    uint16_t local_ver = driver_ota_get_version(head_info->flag);
    uint16_t data_size = OTA_BUFFER_SIZE - sizeof(beken_ota_pkt_s) - OTA_PKT_ADDR_LEN;
    uint8_t data[11];

    if(beken_ota_pkt->length != 0x20)
    {
        LOG_E(OTA, "beken_ota_start_req_handler, length ERROR!\r\n");
        return;
    }
    LOG_I(OTA, "beken_ota_start_req_handler\r\n");
    LOG_I(OTA, "flag:0x%x, len:0x%x, crc:0x%x\r\n", head_info->flag, head_info->len, head_info->crc);
    LOG_I(OTA, "[VID:PID:version]: local[0x%x:0x%x:0x%x], update[0x%x:0x%x:0x%x]\r\n", OTA_VID, OTA_PID, local_ver,head_info->vid, head_info->pid, head_info->ver);

    memset(data, 0, sizeof(data)/sizeof(data[0]));
    if((OTA_VID == head_info->vid) && (OTA_PID == head_info->pid) /*&& (head_info->ver > local_ver)*/ && (OTA_MARK_INIT == driver_ota_get_mark(head_info->addr1)))
    {
        uint32_t addr = 0;
        uint32_t len = 0;
        
#if (CONFIG_OTA_BLE == 1)
        if(appm_get_connection_num())
        {
            appm_update_param(16, 20, 0, 600); /* BLE interval:20ms~25ms, no latency, timeout:6s */

            data_size = gattc_get_mtu(0) - sizeof(beken_ota_pkt_s) - OTA_PKT_ADDR_LEN - 3;           /* 3 = opcode(1 Byte) + Attribute handle(2 Byte) */  
            if(app_bt_flag1_get(APP_FLAG_MUSIC_PLAY) && (data_size > 300))
                data_size = 300;                                                              /* reduce OTA speed while BT music playing in BLE mode, size 300 is safe */                                                      
        }
#endif
        addr = OTA_ADDR_OFFSET;
        len = head_info->len;

        data[0] = OTA_SUCCESS;
        memcpy(&data[1], (uint8_t*)&addr, 4);
        memcpy(&data[5], (uint8_t*)&len, 4);
        memcpy(&data[9], (uint8_t*)&data_size, 2);

        memset((uint8_t*)&beken_ota_param, 0, sizeof(beken_ota_param_s));
        beken_ota_param.frame_seq = beken_ota_pkt->frame_seq;
        beken_ota_param.data_addr = addr;
        
        driver_ota_start(head_info);

        LOG_I(OTA, "beken_ota_start OK!\r\n");
    }
    else
    {
        data[0] = OTA_FAIL;
        LOG_I(OTA, "beken_ota_start FAIL!\r\n");
    }
    beken_ota_pkt_encode(START_RESP_CMD, beken_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void beken_ota_data_send_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    uint32_t addr = LMutils_Get_Uint32(beken_ota_pkt->data);
    uint8_t* data_ptr = beken_ota_pkt->data + sizeof(beken_ota_pkt_s);
    uint16_t data_len = beken_ota_pkt->length - OTA_PKT_ADDR_LEN;
        
    if(((uint8_t)(beken_ota_param.frame_seq + 1) == beken_ota_pkt->frame_seq) && (beken_ota_param.data_addr == addr))
    {
        driver_ota_save_data(data_ptr, data_len);
        
        beken_ota_param.frame_seq++;
        beken_ota_param.data_addr += data_len;
    }
    else
    {
        beken_ota_pkt_encode(DATA_ERROR_CMD, beken_ota_param.frame_seq, (uint8_t*)&beken_ota_param.data_addr, sizeof(beken_ota_param.data_addr));
        LOG_I(OTA, "beken_ota_data_error:%x,%x\r\n", beken_ota_param.frame_seq, beken_ota_param.data_addr);
    }
}

void beken_ota_end_req_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    uint8_t data[1];
    
    if(beken_ota_pkt->length != 0x00)
    {
        LOG_E(OTA, "beken_ota_end_req_handler, length ERROR!\r\n");
        return;
    }
    LOG_I(OTA, "beken_ota_end_req_handler\r\n");
    
    data[0] = driver_ota_finish_result();
    memset((uint8_t*)&beken_ota_param, 0, sizeof(beken_ota_param_s));

    beken_ota_pkt_encode(END_RESP_CMD, beken_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void beken_ota_update_size_req(uint16_t size)
{
    uint16_t max_size = OTA_BUFFER_SIZE - sizeof(beken_ota_pkt_s) - OTA_PKT_ADDR_LEN;

    LOG_I(OTA, "beken_ota_update_size_req:%x\r\n",size);
    
    if(size < max_size)
        beken_ota_pkt_encode(UPDATE_SIZE_REQ_CMD, 0xFF, (uint8_t*)&size, sizeof(uint16_t));
    else
        LOG_I(OTA, "beken_ota_update_size error:%x,%x\r\n", size, max_size);
}

void beken_ota_update_size_resp_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    
    if(beken_ota_pkt->length != 0x00)
    {
        LOG_E(OTA, "beken_ota_update_size_resp_handler, length ERROR!\r\n");
        return;
    }
    if(beken_ota_pkt->frame_seq == 0xFF)
        LOG_I(OTA, "beken_ota_update_size_resp_handler\r\n");
}

void beken_ota_reboot_handler(uint8_t *pValue, uint16_t length)
{
    beken_ota_pkt_s* beken_ota_pkt = (beken_ota_pkt_s*)pValue;
    
    if(beken_ota_pkt->length != 0x00)
    {
        LOG_E(OTA, "beken_ota_reboot_handler, length ERROR!\r\n");
        return;
    }
    
    driver_ota_reboot(0);
}

void beken_ota_pkt_decode(uint8_t *pValue, uint16_t length)
{
    switch(pValue[0])
    {
        case INQUIRY_INFO_REQ_CMD:
            beken_ota_inquiry_req_handler(pValue, length);
            break;
            
        case START_REQ_CMD:
            beken_ota_start_req_handler(pValue, length);
            break; 
            
        case DATA_SEND_CMD:
            beken_ota_data_send_handler(pValue, length);
            break;
            
        case END_REQ_CMD:
            beken_ota_end_req_handler(pValue, length);
            break;
            
        case UPDATE_SIZE_RESP_CMD:
            beken_ota_update_size_resp_handler(pValue, length);
            break;
            
        case REBOOT_CMD:
            beken_ota_reboot_handler(pValue, length);
            break;

        default:
            LOG_I(OTA, "beken_ota_pkt_decode error!!!\r\n");
            break;
    }
}

#if (CONFIG_OTA_SPP == 1)
void beken_ota_spp_pkt_reframe(uint8_t *pValue, uint16_t length)
{
    uint16_t copy_len = 0;
    uint16_t pkt_len = 0;
    
    do
    {
        if((beken_ota_spp_buff_cnt + length) <= OTA_BUFFER_SIZE)
            copy_len = length;
        else
            copy_len = OTA_BUFFER_SIZE - beken_ota_spp_buff_cnt;
        
        memcpy(&beken_ota_spp_buff[beken_ota_spp_buff_cnt], pValue, copy_len);
        beken_ota_spp_buff_cnt += copy_len;
        pValue += copy_len;
        length -= copy_len;

        while(beken_ota_spp_buff_cnt >= sizeof(beken_ota_pkt_s))
        {
            memcpy((uint8_t*)&pkt_len, &beken_ota_spp_buff[2], 2);
            pkt_len += sizeof(beken_ota_pkt_s);
            if(pkt_len <= beken_ota_spp_buff_cnt)
            {
                beken_ota_pkt_decode(beken_ota_spp_buff, pkt_len);
                memcpy(beken_ota_spp_buff, &beken_ota_spp_buff[pkt_len], beken_ota_spp_buff_cnt - pkt_len);
                beken_ota_spp_buff_cnt -= pkt_len;
            }
            else
            {
                break;
            }
        }
        
        if(beken_ota_spp_buff_cnt >= OTA_BUFFER_SIZE)    // If beken_ota_spp_buff is full, we need reset buffer and break!
        {
            memset(beken_ota_spp_buff, 0, OTA_BUFFER_SIZE);
            beken_ota_spp_buff_cnt = 0;
            //LOG_I(OTA, "beken_ota_spp_buff FULL, CLEAR!!!\r\n");
            break;
        }
    }while(length);
}
#endif

#endif
