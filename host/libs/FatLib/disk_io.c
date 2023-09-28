//#include "Driver_sdcard.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "driver_beken_includes.h"
#include "driver_sdcard.h"
#include "diskio.h"
#include "tc_const.h"
#ifdef CONFIG_APP_UDISK
#include "usb_host\usbh_udisk.h"
#endif

#if 1 //(CONFIG_APP_MP3PLAYER == 1)

static uint8 cur_disk_type = DISK_TYPE_SD;

DSTATUS disk_initialize (uint8 pdrv)
{
	os_printf("disk initialize--- \r\n");
	
    cur_disk_type = pdrv;
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        int cnt = 10;
        while(cnt--)
        {
            if(SD_init() == SD_OK) return RES_OK;     
            os_printf("SD init retry cnt = %d\r\n",cnt);
            Delay(50);
            sd_close();
        }
    #endif
    }
    else
    {
    #ifdef CONFIG_APP_UDISK
        if(udisk_init()== UDISK_RET_OK)
            return RES_OK;
    #endif
    }
    return STA_NOINIT;
}

DSTATUS disk_status (BYTE pdrv)
{
	//test 
	return RES_OK;
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff, 
	DWORD sector, 
	UINT count 
)
{
    int res = RES_ERROR;
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        if(sdcard_read_multi_block_dma(buff, sector, count, NULL) == SD_OK) { res = RES_OK; }
    #endif
    }
    else if(pdrv == DISK_TYPE_UDISK)
    {
    #ifdef CONFIG_APP_UDISK
        if(udisk_rd_blk_sync(sector, count, buff) == UDISK_RET_OK) { res = RES_OK; }
    #endif
    }
    return res;
}

DRESULT disk_write (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff,
	DWORD sector,
	UINT count
)
{
    int res = RES_ERROR;
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        if(sdcard_write_multi_block_dma(buff, sector, count, NULL) == SD_OK) { res = RES_OK; }
    #endif
    }
    else if(pdrv == DISK_TYPE_UDISK)
    {
    #ifdef CONFIG_APP_UDISK
        if(udisk_write_blk_sync(sector, count, buff) == UDISK_RET_OK) { res = RES_OK; }
    #endif
    }
    return res;
}

DRESULT disk_ioctl (
	BYTE drv,
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return RES_OK;
}


DRESULT disk_unmount(uint8 pdrv)
{
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
    	sd_close();// sdcard_uninitialize();
    #endif
    }
    else
    {
    #ifdef CONFIG_APP_UDISK
        udisk_uninit();
    #endif
    }
    return RES_OK;
}

uint32_t get_disk_size(void)
{
    uint32_t ret = 0;
    if(cur_disk_type == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        ret = sdcard_get_total_block();
    #endif
    }
    else
    {
    #ifdef CONFIG_APP_UDISK
        ret = udisk_get_size();
    #endif
    }
    return ret;
}

uint8 Media_is_online(void)
{
    uint8 ret = 0;
    if(cur_disk_type == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        ret = sd_is_attached();
    #endif
    }
    else
    {
    #ifdef CONFIG_APP_UDISK
        ret = udisk_is_attached();
    #endif
    }
    return ret;
}

uint8 get_cur_media_type(void)
{
    return cur_disk_type;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// Added by kaigan.jiang,on 2021-09-26
int disk_read_async (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff,
	DWORD sector,
	UINT count,
	void*cbk
)
{
    int res = RES_ERROR;
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        if(sdcard_read_multi_block_dma(buff, sector, count, cbk) == SD_OK) { res = RES_OK; }
    #endif
    }
    else if(pdrv == DISK_TYPE_UDISK)
    {
    #ifdef CONFIG_APP_UDISK
        if(udisk_rd_blk_async(sector, count, buff, cbk) == UDISK_RET_OK) { res = RES_OK; }
    #endif
    }
    return res;
}

int disk_write_async (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff,
	DWORD sector,
	UINT count,
	void*cbk
)
{
    int res = RES_ERROR;
    if(pdrv == DISK_TYPE_SD)
    {
    #ifdef CONFIG_APP_SDCARD
        if(sdcard_write_multi_block_dma(buff, sector, count, cbk) == SD_OK) { res = RES_OK; }
    #endif
    }
    else if(pdrv == DISK_TYPE_UDISK)
    {
    #ifdef CONFIG_APP_UDISK
        if(udisk_write_blk_async(sector, count, buff, cbk) == UDISK_RET_OK) { res = RES_OK; }
    #endif
    }
    return res;
}

///no fat sys, async function
///add by guangming.wang at 2021-12-28
static volatile DREAD_ASYNC_S gsv_dread_async = {0};
static volatile DWRITE_ASYNC_S gsv_dwrite_async = {0};
//extern uint8 *g_read_buf;
extern volatile uint8 bUsb;

static int disk_async_read_check_status(void);
static FRESULT disk_async_read(void);
static void disk_async_read_cbk(uint32_t res);
static int disk_async_read_check_result(uint8 *rbuff, int data_len);
static int disk_async_write_check_status(void);
static FRESULT disk_async_write(void);
static void disk_async_write_cbk(uint32_t res);

int get_read_pending(void)
{
	DREAD_ASYNC_S *p_read_async = (DREAD_ASYNC_S *)&gsv_dread_async;
	DREAD_ASYNC_FLG_S *p_flg = &p_read_async->flg;	
	if(p_flg->init)
	{
	    return TRUE;
	}
	return FALSE;
}
int get_write_pending(void)
{
	DWRITE_ASYNC_S *p_write_async = (DWRITE_ASYNC_S *)&gsv_dwrite_async;
	DWRITE_ASYNC_FLG_S *p_flg = &p_write_async->flg;	
	if(p_flg->init)
	{
	    return TRUE;
	}
	return FALSE;
}

void disk_async_timming_call(void)
{
    if(get_cur_media_type() != DISK_TYPE_SD) return;

    sdcard_read_dma_check_finish();
    if(f_get_async_state() != ASYNC_BUSY)
    {
        if(disk_async_read_check_status() == TRUE)
        {
            disk_async_read();
        }
    }
    sdcard_write_dma_check_finish();
    if(f_get_async_state() != ASYNC_BUSY)
    {
        if(disk_async_write_check_status() == TRUE)
        {
            disk_async_write();
        }   
    }
}

static FRESULT disk_async_read(void)
{
    static DREAD_ASYNC_FLG_S *p_flg = (DREAD_ASYNC_FLG_S *)&gsv_dread_async.flg;
    static uint8 *p_index_in = (uint8 *)&gsv_dread_async.index_in;
    static uint8 *p_index_out = (uint8 *)&gsv_dread_async.index_out;
    static DREAD_ASYNC_PARA_S *p_para;
    uint8 busy_flg = FALSE;
    int res=0;

    p_para = (DREAD_ASYNC_PARA_S *)&gsv_dread_async.para[*p_index_out%DISK_MUL_ASYNC_MAX];
    if(*p_index_in == *p_index_out)
    {
        return FR_OK;
    }
    
    p_flg->init = TRUE;
    p_flg->running = TRUE;


    DISK_DEBUG("    read,index_in=%d,index_out=%d,pdrv=%x,buff=%x,sect=%x,cc=%d,cbk=%x,cbk_arg=%x\r\n", 
    *p_index_in, *p_index_out, p_para->pdrv,p_para->buff,p_para->sect,p_para->cc,p_para->cbk,p_para->cbk_arg);

    f_set_async_state(ASYNC_BUSY);

    if(p_para->cc)
    {
        if(p_flg->intterupt.read1 == FALSE)
        {
            DISK_DEBUG("    disk_read1_0,p_para->sect=%x,cc=%d\r\n",p_para->sect, p_para->cc);
            res = disk_read_async(0, p_para->buff, p_para->sect, p_para->cc, disk_async_read_cbk);
            if(res == SD_OK)
            // if(res == SD_DMA_RUNNING)
            {
                if(p_flg->intterupt.read1)
                {
                    DISK_DEBUG("    disk_read1_0_has_been_int\r\n");     
                    if(sdcard_read_dma_check_finish() == FALSE)
                    {
                        DISK_FATAL("    sd_int_reg_err\r\n");   
                        p_flg->abnormal.flg = TRUE;
                        p_flg->abnormal.try_cnt = 0;
                        busy_flg = 1;
                    }
                }
                else
                {            
                    DISK_DEBUG("        return disk_read1_0\r\n");
                    busy_flg = 1;
                }
            }
            else if(res)
            {
                p_flg->abnormal.flg = TRUE;
            }
           
        }
        else
        {
            DISK_DEBUG("    disk_read1_1\r\n");
        }
    }
    p_flg->running = FALSE;
    if(busy_flg == TRUE)
    {
        return FR_BUSY;
    } 
    DISK_DEBUG("    disk_read_over\r\n");
    p_flg->init = FALSE;
    if(f_get_async_state() == ASYNC_OK)
    {
        DISK_DEBUG("f_read_async_has_been_ok\r\n");
        return FR_OK;
    }
    memset((uint8 *)&p_flg->intterupt, 0x00, sizeof(DREAD_ASYNC_FLG_INT_S));
    if(0 ==disk_async_read_check_result((uint8 *)p_para->buff, p_para->cc*512))
    {
        *p_index_out += 1;
        if(*p_index_out >= DISK_MUL_ASYNC_MAX)
        {
            *p_index_out = 0;
        }
        if(p_flg->abnormal.flg)
        {
            res = 1;
        }

        DISK_DEBUG("  read_end,index_in=%d,index_out=%d,br=%d\r\n\r\n",*p_index_in, *p_index_out,p_para->cc);

        if(p_para->cbk)
        {
            CBK_FUN5 pfun = p_para->cbk;
            void *cbk_arg = p_para->cbk_arg;
            memset((uint8 *)p_flg, 0x00, sizeof(DREAD_ASYNC_FLG_S));
            pfun(res, 0, cbk_arg);
            f_set_async_state(ASYNC_OK);
        }
        else
        {
            memset((uint8 *)p_flg, 0x00, sizeof(DREAD_ASYNC_FLG_S));
            f_set_async_state(ASYNC_OK);
        }

        return FR_OK;
    }
    else
    {
        f_set_async_state(ASYNC_FAIL);
        return FR_BUSY;
    }  
        
}

FRESULT disk_async_read_init (
    BYTE pdrv,
    void * buff,    /* Pointer to data buffer */
    UINT sect,  /* Number of bytes to read */
    UINT cc,    /* Pointer to number of bytes read */
    void *cbk,
    void *cbk_arg
)
{
    static DREAD_ASYNC_PARA_S *p_para;
    static uint8 *p_index_in = (uint8 *)&gsv_dread_async.index_in;
    static uint8 *p_index_out = (uint8 *)&gsv_dread_async.index_out;
    p_para = (DREAD_ASYNC_PARA_S *)&gsv_dread_async.para[*p_index_out%DISK_MUL_ASYNC_MAX];

    if(bUsb == 1)
    {
        disk_async_para_init();
        return FR_INT_ERR;
    }

    if(NULL == buff)
    {
        return FR_INT_ERR;
    }
    
    if(*p_index_in >= *p_index_out)
    {
        if((*p_index_in - *p_index_out) >= (DISK_MUL_ASYNC_MAX-1))
        {
            DISK_DEBUG("FF_INT_ERR_1");
            return FR_INT_ERR;
        }
    }
    else
    {
        if((*p_index_in + DISK_MUL_ASYNC_MAX - *p_index_out) >= (DISK_MUL_ASYNC_MAX-1))
        {
            DISK_DEBUG("FF_INT_ERR_2");
            return FR_INT_ERR;
        }
    }
    
    p_para->pdrv = pdrv;
    p_para->buff = buff;
    p_para->sect = sect;
    p_para->cc = cc;
    p_para->cbk = cbk;
    p_para->cbk_arg = cbk_arg;
    *p_index_in += 1;
    if(*p_index_in >= DISK_MUL_ASYNC_MAX)
    {
        *p_index_in = 0;
    }
    
    DISK_DEBUG("    read_init,index_in=%d,index_out=%d,pdrv=%x,buff=%x,sect=%x,cc=%d,cbk=%x,cbk_arg=%x\r\n", 
    *p_index_in, *p_index_out, p_para->pdrv,p_para->buff,p_para->sect,p_para->cc,p_para->cbk,p_para->cbk_arg);
    
    return FR_OK;
        
}

static void disk_async_read_cbk(uint32_t res)
{  
    DREAD_ASYNC_FLG_S *p_flg = (DREAD_ASYNC_FLG_S *)&gsv_dread_async.flg;

    DISK_DEBUG("      read1_cbk1\r\n");
    p_flg->intterupt.read1 = TRUE;
    f_set_async_state(ASYNC_PART_OK);
    if(p_flg->running == FALSE)
    {
        DISK_DEBUG("      not running,todo disk_async_read\r\n");
        disk_async_read();
    }
}

static int disk_async_read_check_status(void)
{
    DREAD_ASYNC_FLG_S *p_flg = (DREAD_ASYNC_FLG_S *)&gsv_dread_async.flg;
    if(f_get_async_state() == ASYNC_PART_OK)
    {
        if(p_flg->intterupt.read1)
        {
            if(sdcard_read_dma_check_finish() == TRUE)
            {
                return TRUE;
            }
        }
    }
    else
    {
        return TRUE;
    }
    
    return FALSE;
}

static int disk_async_read_check_result(uint8 *rbuff, int data_len)
{
	return 0;
}
static FRESULT disk_async_write(void)
{    
    static DWRITE_ASYNC_FLG_S *p_flg = (DWRITE_ASYNC_FLG_S *)&gsv_dwrite_async.flg;
    static uint8 *p_index_in = (uint8 *)&gsv_dwrite_async.index_in;
    static uint8 *p_index_out = (uint8 *)&gsv_dwrite_async.index_out;
    static DWRITE_ASYNC_PARA_S *p_para;
    uint8 busy_flg = 0;
    int res=0;
    p_para = (DWRITE_ASYNC_PARA_S *)&gsv_dwrite_async.para[*p_index_out%DISK_MUL_ASYNC_MAX];
    if(*p_index_in == *p_index_out)
    {
        return FR_OK;
    }
    if(p_para)
    {
        //p_para = p_para;
    }
    p_flg->init = TRUE;
    p_flg->running = TRUE;
    DISK_DEBUG("    write,index_in=%d,index_out=%d,pdrv=%x,buff=%x,sect=%x,cc=%d,cbk=%x,cbk_arg=%x\r\n", 
    *p_index_in, *p_index_out, p_para->pdrv,p_para->buff,p_para->sect,p_para->cc,p_para->cbk,p_para->cbk_arg);
    f_set_async_state(ASYNC_BUSY);
    if(p_para->cc)
    {
        if(p_flg->intterupt.write1 == FALSE)
        {
            DISK_DEBUG("    disk_write1_0,p_para->sect=%x,cc=%d\r\n",p_para->sect, p_para->cc);
            res = disk_write_async(p_para->pdrv, p_para->buff, p_para->sect, p_para->cc, disk_async_write_cbk);
            if(res == SD_OK)
            // if(res == SD_DMA_RUNNING)
            {
                if(p_flg->intterupt.write1 == TRUE)
                {
                    //gs_fwrite_async_flg.flg_disk_write1 = 0;
                    DISK_FATAL("    disk_write1_0_has_been_int\r\n");
                }
                else
                {
                    DISK_DEBUG("        return disk_write1_0\r\n");
                }
                busy_flg = 1;
            }
            else if(res)
            {
                p_flg->abnormal.flg = 1;
            }
        }
        else
        {
            DISK_DEBUG("    disk_write1_1,cc=%d\r\n",p_para->cc);
        }
    }




    p_flg->running = FALSE;
    if(busy_flg)
    {
        return FR_BUSY;
    }
    p_flg->init = FALSE;
    DISK_DEBUG("    disk_write_over\r\n");

    if(f_get_async_state() == ASYNC_OK)
    {
        DISK_DEBUG("d_write_async_has_been_ok\r\n");
        return FR_OK;
    }
    if(p_flg->abnormal.flg)
    {
        res = 1;
    }
    memset((uint8 *)p_flg, 0x00, sizeof(DWRITE_ASYNC_FLG_S));
    if(p_para->cbk)
    {
        CBK_FUN5 pfun = p_para->cbk;
        pfun(res, 0, p_para->cbk_arg);
    }
    *p_index_out += 1;
    if(*p_index_out >= DISK_MUL_ASYNC_MAX)
    {
        *p_index_out = 0;
    }
    DISK_DEBUG("    write_end,index_in=%d,index_out=%d,cc=%d\r\n\r\n",*p_index_in, *p_index_out,p_para->cc);
    f_set_async_state(ASYNC_OK);
    return FR_OK;
}

FRESULT disk_async_write_init (
    BYTE pdrv,
    void * buff,    /* Pointer to data buffer */
    UINT sect,  /* Number of bytes to read */
    UINT cc,    /* Pointer to number of bytes read */
    void *cbk,
    void *cbk_arg
)
{
    uint8 *p_index_in = (uint8 *)&gsv_dwrite_async.index_in;
    uint8 *p_index_out = (uint8 *)&gsv_dwrite_async.index_out;
    DWRITE_ASYNC_PARA_S *p_para;
    p_para = (DWRITE_ASYNC_PARA_S *)&gsv_dwrite_async.para[*p_index_in%DISK_MUL_ASYNC_MAX];
    if(bUsb == 1)
    {
        disk_async_para_init();
        return FR_INT_ERR;
    }
    if(NULL == buff)
    {
        return FR_INT_ERR;
    }
    
    if(*p_index_in >= *p_index_out)
    {
        if((*p_index_in - *p_index_out) >= (DISK_MUL_ASYNC_MAX-1))
        {
            DISK_DEBUG("FF_INT_ERR_1\r\n");
            return FR_INT_ERR;
        }
    }
    else
    {
        if((*p_index_in + DISK_MUL_ASYNC_MAX - *p_index_out) >= (DISK_MUL_ASYNC_MAX-1))
        {
            DISK_DEBUG("FF_INT_ERR_2\r\n");
            return FR_INT_ERR;
        }
    }
    
    p_para->pdrv = pdrv;
    p_para->buff = buff;
    p_para->sect = sect;
    p_para->cc = cc;
    p_para->cbk = cbk;
    p_para->cbk_arg = cbk_arg;
    DISK_DEBUG("    write_init0,index_in=%d,index_out=%d,pdrv=%x,buff=%x,sect=%x,cc=%d,cbk=%x,cbk_arg=%x\r\n", 
    *p_index_in, *p_index_out, p_para->pdrv,p_para->buff,p_para->sect,p_para->cc,p_para->cbk,p_para->cbk_arg);

    *p_index_in += 1;
    if(*p_index_in >= DISK_MUL_ASYNC_MAX)
    {
        *p_index_in = 0;
    }
    
    DISK_DEBUG("    write_init,index_in=%d,index_out=%d,pdrv=%x,buff=%x,sect=%x,cc=%d,cbk=%x,cbk_arg=%x\r\n", 
    *p_index_in, *p_index_out, p_para->pdrv,p_para->buff,p_para->sect,p_para->cc,p_para->cbk,p_para->cbk_arg);
    
    return FR_OK;
    
    
}

static void disk_async_write_cbk(uint32_t res)
{
    static DWRITE_ASYNC_FLG_S *p_flg = (DWRITE_ASYNC_FLG_S *)&gsv_dwrite_async.flg;

    DISK_DEBUG("        disk_write1_cbk\r\n");
    p_flg->intterupt.write1 = TRUE;
    f_set_async_state(ASYNC_PART_OK);
}

static int disk_async_write_check_status(void)
{
    DWRITE_ASYNC_FLG_S *p_flg = (DWRITE_ASYNC_FLG_S *)&gsv_dwrite_async.flg;
    if(f_get_async_state() == ASYNC_PART_OK)
    {
        if(p_flg->intterupt.write1)
        {
            return TRUE;
        }
    }
    else
    {
        return TRUE;
    }
    return FALSE;
}

void disk_async_para_init(void)
{
    f_set_async_state(ASYNC_PART_OK);
    memset((BYTE *)&gsv_dread_async, 0x00, sizeof(gsv_dread_async));
    memset((BYTE *)&gsv_dwrite_async, 0x00, sizeof(gsv_dwrite_async));
}

///////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
