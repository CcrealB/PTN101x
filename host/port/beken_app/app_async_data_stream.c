#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "driver_beken_includes.h"
#include "sys_types.h"
// #include "driver_qspi.h"
// #include "app_lcd.h"
// #include "app_lcd_touch.h"
#include "ff.h"
#include "driver_sdcard.h"
#include "app_async_data_stream.h"
// #include "driver_external_flash.h"
#include "tc_const.h"

#if (CONFIG_APP_ASYNC_DATA_STREAM == 1)
typedef enum
{
    driver_trans_block,      //驱动属于阻塞发送模式
    driver_trans_aysnc,      //驱动属于异步发送模式
}driver_trans_mode;


typedef async_errcode    (*async_preparation_fxn)(void *obj);
typedef async_errcode    (*async_read_fxn)(void *obj,uint32_t *len);
typedef async_errcode    (*async_write_fxn)(void *obj,uint32_t *len);
typedef void    (*async_cancel_fxn)(void *obj);
typedef void    (*async_finish_fxn)(void *obj);
typedef void    (*async_gen0_fxn)(void *obj);
typedef void    (*drv_trans_finish_cb)(uint32_t arg1,uint32_t arg2,void *obj);

#define APP_ASYNC_READ_EVT             0X01
#define APP_ASYNC_WRITE_EVT            0X02
#define APP_ASYNC_CANCEL_EVT           0X04
#define APP_ASYNC_FINISH_EVT           0X08


typedef struct
{

    uint32_t state:1;                           //0 idle,1 busy
    uint32_t event:8;
    uint32_t option:1;                          // async_opt
    uint32_t trans_mode:1;                      // driver_trans_mode
    uint32_t :22;
    uint32_t driver_trans_aysnc_time;          //当 trans_mode = driver_trans_aysnc，且无 drv_trans_finish_cb 时使用
    async_buffer_t buffer;
    jtask_h   task;

    void *internal_arg;
    uint8_t*p_buffer_head;
    uint8_t*p_cur_buffer;
    uint8_t*p_buffer_end;
    
    uint32_t uarg1;
    uint32_t uarg2;

    async_preparation_fxn pre_fxn;
    async_read_fxn  read_fxn;
    async_write_fxn write_fxn;
    async_cancel_fxn cancel_fxn;
    async_finish_fxn finish_fxn;
    drv_trans_finish_cb trans_finish_cb;
    
    async_delegation_cb delegation_cb;
}app_async_data_object,*app_async_data_handle;

 
#if (ASYNC_SDCARD == 1)
static async_errcode sdcard_preparation(void *obj);
#if(APP_ASYNC_SDCARD_ENABLE==0)
static async_errcode sdcard_read(void *obj,uint32_t *len);
#else
static async_errcode sdcard_read_async(void *obj,uint32_t *len);
#endif
static async_errcode sdcard_write(void *obj,uint32_t *len);
static void sdcard_cancel(void *obj);
static void sdcard_finish(void *obj); 
static void sdcard_finish_cbk(uint32_t arg1,uint32_t arg2,void *obj);
#if 0  // not used warning
static void sdcard_open_cbk(uint32_t arg1,uint32_t arg2,void *obj);
#endif

#endif

#if (ASYNC_SPI_FLASH == 1)
static async_errcode spi_flash_preparation(void *obj);
static async_errcode spi_flash_read(void *obj,uint32_t *len);
static void spi_trans_finish_cb(uint32_t arg1,uint32_t arg2,void *obj);
#endif

static app_async_data_object async_data_obj[ASYNC_DRV_NUM] = 
{
#if (ASYNC_SDCARD == 1)
    {   
        .state = 0,
        .event = 0,
        .trans_mode = driver_trans_block,
        .driver_trans_aysnc_time = 0,
        .uarg1 = 0,
        .uarg2 = 0,
        .internal_arg = NULL,
        .p_cur_buffer = NULL,
        .p_buffer_end = NULL,
        .pre_fxn = sdcard_preparation,
#if (APP_ASYNC_SDCARD_ENABLE == 0)
        .read_fxn = sdcard_read,
#else
		.read_fxn = sdcard_read_async,
#endif
        .write_fxn = sdcard_write,
        .cancel_fxn = sdcard_cancel,
        .finish_fxn = sdcard_finish,
        .trans_finish_cb = sdcard_finish_cbk,
    },
#endif

#if (ASYNC_SPI_FLASH == 1)
    {   
        .state = 0,
        .event = 0,
        .trans_mode = driver_trans_aysnc,
        .driver_trans_aysnc_time = 0,
        .uarg1 = 0,
        .uarg2 = 0,
        .internal_arg = NULL,
        .p_cur_buffer = NULL,
        .p_buffer_end = NULL,
        .pre_fxn = spi_flash_preparation,
        .read_fxn = spi_flash_read,
        .write_fxn = NULL,
        .cancel_fxn = NULL,
        .finish_fxn = NULL,
        .trans_finish_cb = spi_trans_finish_cb
    },
#endif
    

};



static void app_async_data_stream_finish_evt_process(app_async_data_handle hdl);
static void app_async_data_stream_write_evt_process(app_async_data_handle hdl);
static void app_async_data_stream_read_evt_process(app_async_data_handle hdl);
static void app_async_data_stream_cancel_evt_process(app_async_data_handle hdl);
static void app_async_trans_time_cb(void *arg);
static void app_async_set_event(app_async_data_handle hdl,uint8_t event);
static uint32_t calculate_trans_length(app_async_data_handle hdl);



volatile APP_ASYNC_STATE gs_flg_async_pic_init = APP_ASYNC_IDLE;
volatile APP_ASYNC_STATE gs_flg_async_open_init = APP_ASYNC_IDLE;



int async_debug(const char *fmt, ...)
{
    extern volatile unsigned int g_debug_mcu_switch;
    if(g_debug_mcu_switch & (0x1 << DEBUG_MCU_ASYNC))
    {
        os_printf(fmt);
    }
    return 0;
}



void app_async_data_stream_init(void)
{
    //异步读取数据硬件初始化
#if (ASYNC_SDCARD == 1)
    SD_init();
#endif

#if (ASYNC_SPI_FLASH == 1)
    external_flash_init();
#endif
    
    app_async_data_handle hdl = async_data_obj;
    for(uint8_t i=0;i<ASYNC_DRV_NUM;i++)
    {
        jtask_init(&hdl->task, J_TASK_TIMEOUT );
    }
}

bool app_async_data_stream_busy(void)
{
    for(uint8_t i=0;i<ASYNC_DRV_NUM;i++)
    {
        if(async_data_obj[i].state)
        {
            return true;
        }
    }
    return false;
}

async_errcode app_async_option(async_drv drv,async_opt opt,async_buffer_t *buffer,async_delegation_cb delegation_cb)
{
    async_errcode errcode = async_err_none;
    app_async_data_handle hdl;
    if(drv >= ASYNC_DRV_NUM || delegation_cb == NULL)
    {
        return async_err_param;
    }
    hdl = &async_data_obj[drv];
    if(hdl->state)
    {
        return async_err_busy;
    }
    memcpy(&hdl->buffer,buffer,sizeof(async_buffer_t));
    hdl->p_buffer_head = hdl->buffer.buffer;
    hdl->p_cur_buffer = hdl->buffer.buffer;
    hdl->p_buffer_end = hdl->buffer.buffer + hdl->buffer.size;
    hdl->option = opt;
    hdl->delegation_cb = delegation_cb;
    //读准备
    if(hdl->pre_fxn)
    {
        errcode = hdl->pre_fxn(hdl);
        if(errcode != async_err_none)
        {
            return errcode;
        }
    }
    if(opt == ASYNC_READ)
    {
        if(hdl->read_fxn == NULL)
        {
            return async_err_not_support;
        }
        app_async_set_event(hdl,APP_ASYNC_READ_EVT);
    }
    else
    {
        if(hdl->write_fxn == NULL)
        {
            return async_err_not_support;
        }
        app_async_set_event(hdl,APP_ASYNC_WRITE_EVT);
    }
    hdl->state = 1;
    return errcode;
}

async_errcode app_async_data_stream_cancel(async_drv drv)
{
     app_async_data_handle hdl;
     if(drv >= ASYNC_DRV_NUM)
     {
         return async_err_param;
     }
     hdl = &async_data_obj[drv];
    if(hdl->state)
    {
        app_async_set_event(hdl,APP_ASYNC_CANCEL_EVT);
    }
    return async_err_none;
}


void app_async_data_stream_handler(void)
{
    app_async_data_handle hdl = async_data_obj;
    app_async_data_handle hdl_end = async_data_obj + ASYNC_DRV_NUM;
    do
    {
        if(hdl->state == true)
        {
            if(hdl->event & APP_ASYNC_CANCEL_EVT)
            {
                hdl->event &= ~APP_ASYNC_CANCEL_EVT;
                //优先处理取消
                app_async_data_stream_cancel_evt_process(hdl);
            }
            if(hdl->event & APP_ASYNC_READ_EVT)
            {
                hdl->event &= ~APP_ASYNC_READ_EVT;
                app_async_data_stream_read_evt_process(hdl);
            }
            if(hdl->event & APP_ASYNC_WRITE_EVT)
            {
                hdl->event &= ~APP_ASYNC_WRITE_EVT;
                app_async_data_stream_write_evt_process(hdl);
            }
            if(hdl->event & APP_ASYNC_FINISH_EVT)
            {
                hdl->event &= ~APP_ASYNC_FINISH_EVT;
                app_async_data_stream_finish_evt_process(hdl);
            }
        }
        hdl++;
    }while(hdl < hdl_end);
}

static void app_async_set_event(app_async_data_handle hdl,uint8_t event)
{
    hdl->event |= event;
}

static uint32_t calculate_trans_length(app_async_data_handle hdl)
{
    uint32_t buffer_size = hdl->p_buffer_end - hdl->p_cur_buffer;
    if(buffer_size >hdl->buffer.split_size)
    {
        buffer_size = hdl->buffer.split_size;
    }
    return buffer_size;
}

static void app_async_trans_time_cb(void *arg)
{
    app_async_data_handle hdl = arg;
    if(hdl->option == ASYNC_READ)
    {
         app_async_set_event(hdl,APP_ASYNC_READ_EVT);
    }
    else
    {
         app_async_set_event(hdl,APP_ASYNC_WRITE_EVT);
    }
}
static void app_async_data_stream_cancel_evt_process(app_async_data_handle hdl)
{
    if(hdl->cancel_fxn)
    {
        hdl->cancel_fxn(hdl);
    }
    if(hdl->delegation_cb)
    {
        uint32_t len = hdl->p_buffer_end - hdl->p_cur_buffer;
        hdl->delegation_cb(ASYNC_CANCEL,hdl->buffer.buffer,len);
    }
    hdl->state = 0;
}
static void app_async_data_stream_read_evt_process(app_async_data_handle hdl)
{
    static uint32_t len;
    len = calculate_trans_length(hdl);
    //DEBUG_ASYNC_DATA_PRINTF("Async read calculate_trans_length=%d\r\n",len);
    //继续读
    if(hdl->read_fxn)
    {
        async_errcode errcode =  hdl->read_fxn(hdl,&len);
        //DEBUG_ASYNC_DATA_PRINTF("read_evt,errcode=%d,len=%d\r\n",errcode,len);
        if(errcode == async_err_none)
        {
            if( len == 0 )
            {
                DEBUG_ASYNC_DATA_PRINTF("Async read finish\r\n");
                //无数据可读跳转到完成
                goto ASYNC_READ_FINISH;
            }
            hdl->p_cur_buffer += len;
            if(hdl->trans_mode == driver_trans_block)
            {
                app_async_set_event(hdl,APP_ASYNC_READ_EVT);
            }
            else if(hdl->trans_mode == driver_trans_aysnc)
            {
                if(hdl->trans_finish_cb == NULL)
                {
                    //如果有回调，则直接在硬件回调触发下次传输
                    //没有则通过延时
                    jtask_schedule(hdl->task, 
                                    hdl->driver_trans_aysnc_time, 
                                    (jthread_func)app_async_trans_time_cb, hdl);
                }
            }
        }
        else
        {
        	if(errcode != async_err_busy)
        	{
            	app_async_set_event(hdl,APP_ASYNC_CANCEL_EVT);
            }
        }
    }
    return;
ASYNC_READ_FINISH:
    app_async_set_event(hdl,APP_ASYNC_FINISH_EVT);
}
static void app_async_data_stream_write_evt_process(app_async_data_handle hdl)
{
    uint32_t len = calculate_trans_length(hdl);
    //继续写
    if(hdl->write_fxn)
    {
        async_errcode errcode =  hdl->write_fxn(hdl,&len);
        if(errcode == async_err_none)
        {
            if( len == 0 )
            {
                goto ASYNC_WRITE_FINISH;
            }
            hdl->p_cur_buffer += len;
            if(hdl->trans_mode == driver_trans_block)
            {
                app_async_set_event(hdl,APP_ASYNC_WRITE_EVT);
            }
            else if(hdl->trans_mode == driver_trans_aysnc)
            {
                if(hdl->trans_finish_cb == NULL)
                {
                    //如果有回调，则直接在硬件回调触发下次传输
                    //没有则通过延时
                    jtask_schedule(hdl->task, 
                                    hdl->driver_trans_aysnc_time, 
                                    (jthread_func)app_async_trans_time_cb, hdl);
                }
            }
        }
        else
        {
            app_async_set_event(hdl,APP_ASYNC_CANCEL_EVT);
        }
    }
    return;
ASYNC_WRITE_FINISH:
    app_async_set_event(hdl,APP_ASYNC_FINISH_EVT);
}
static void app_async_data_stream_finish_evt_process(app_async_data_handle hdl)
{
    if(hdl->finish_fxn)
    {
        hdl->finish_fxn(hdl);
    }
    if(hdl->delegation_cb)
    {
        uint32_t len = hdl->p_buffer_end - hdl->p_buffer_head;
        async_opt_result opt_result = ASYNC_READ_FINISH;
        if(hdl->option == ASYNC_WRITE)
        {
            opt_result = ASYNC_WRITE_FINISH;
        }
        hdl->delegation_cb(opt_result,hdl->buffer.buffer,len);
    }
    hdl->state = 0;
}


#if (ASYNC_SDCARD == 1)
static async_errcode sdcard_preparation(void *obj)
{
    app_async_data_handle hdl = obj;
    if(hdl->internal_arg)
    {
        jfree(hdl->internal_arg);
        hdl->internal_arg = NULL;
    }
    hdl->internal_arg = jmalloc(sizeof(FIL),M_ZERO);
    if(hdl->internal_arg == NULL)
    {
        return async_err_alloc_failed;
    }
    FIL *file = hdl->internal_arg;
    uint8_t file_opt = FA_READ;
    if(hdl->option == ASYNC_WRITE)
    {
        file_opt = FA_WRITE;
    }
    
#if 1//(APP_ASYNC_SDCARD_ENABLE == 0)
    FRESULT r=f_open(file,hdl->buffer.param.filename,file_opt);
    if(r != FR_OK)
    {   
        DEBUG_ASYNC_DATA_PRINTF("Async open file failed %d\r\n",r);
        return async_err_drv;
    }
    return async_err_none;
#else	
	DEBUG_ASYNC_DATA_PRINTF("Async open file name:%s\r\n",hdl->buffer.param.filename);
    FRESULT r=f_open_async_init(file,hdl->buffer.param.filename,file_opt,sdcard_open_cbk, NULL);
    return async_err_none;
#endif
}
#if(APP_ASYNC_SDCARD_ENABLE==0)
static async_errcode sdcard_read(void *obj,uint32_t *len)
{ 
    app_async_data_handle hdl = obj;
    uint32_t read_len;
    FIL *file = hdl->internal_arg;
    FRESULT r = f_read(file, hdl->p_cur_buffer,*len, (UINT*)&read_len);   
	if(r != FR_OK)
    {
        DEBUG_ASYNC_DATA_PRINTF("Async read SD filed %d\r\n",r);
        return async_err_drv;
    }
    return async_err_none;   
}
#else
//cannot repatedly call function until last result
static async_errcode sdcard_read_async(void *obj,uint32_t *len)
{ 
	app_async_data_handle hdl = obj;
    FIL *file = hdl->internal_arg;
	FRESULT r = FR_OK;
	if(APP_ASYNC_IDLE == gs_flg_async_pic_init)
	{
		gs_flg_async_pic_init = APP_ASYNC_INIT;
		DEBUG_ASYNC_DATA_PRINTF("Async pic init,btr=%d\r\n",*len);
		r = f_read_async_init(file, hdl->p_cur_buffer,*len, (UINT*)len, hdl->trans_finish_cb, obj);
		if(r != FR_OK)
	    {
	        DEBUG_ASYNC_DATA_PRINTF("Async read SD filed %d\r\n",r);
	        gs_flg_async_pic_init = APP_ASYNC_IDLE;
	        return async_err_param;
	    }
	}
	else if(APP_ASYNC_OK == gs_flg_async_pic_init)
	{
		gs_flg_async_pic_init = APP_ASYNC_IDLE;
		if(len)
		{
			//DEBUG_ASYNC_DATA_PRINTF("Async pic ok,br=%d\r\n",*len);
		}
		return async_err_none;
	}
	return async_err_busy;
}
#endif
static async_errcode sdcard_write(void *obj,uint32_t *len)
{
    app_async_data_handle hdl = obj;
    FIL *file = hdl->internal_arg;
    uint32_t write_len;
    FRESULT r = f_write(file, hdl->p_cur_buffer,*len, (UINT*)&write_len);
    *len = write_len;
    if(r != FR_OK)
    {
        DEBUG_ASYNC_DATA_PRINTF("Async write SD filed %d",r);
        return async_err_drv;
    }
    return async_err_none;
}

static void sdcard_cancel(void *obj)
{
    app_async_data_handle hdl = obj;
    FIL *file = hdl->internal_arg;
    f_close(file);
    jfree(hdl->internal_arg);
    hdl->internal_arg = NULL;
}

static void sdcard_finish(void *obj)
{
    app_async_data_handle hdl = obj;
    FIL *file = hdl->internal_arg;
    f_close(file);
    jfree(hdl->internal_arg);
    hdl->internal_arg = NULL;
}

static void sdcard_finish_cbk(uint32_t arg1,uint32_t arg2,void *obj)
{
    app_async_data_handle hdl = obj;
    app_async_set_event(hdl,APP_ASYNC_READ_EVT);
	//DEBUG_ASYNC_DATA_PRINTF("async_read_cbk\r\n");
	gs_flg_async_pic_init = APP_ASYNC_OK;
}

#if 0  // not used warning
static void sdcard_open_cbk(uint32_t arg1,uint32_t arg2,void *obj)
{
    app_async_data_handle hdl = obj;
    app_async_set_event(hdl,APP_ASYNC_READ_EVT);
	//DEBUG_ASYNC_DATA_PRINTF("async_open_cbk\r\n");
	gs_flg_async_open_init = APP_ASYNC_OK;
}
#endif

#endif


#if (ASYNC_SPI_FLASH == 1)
static async_errcode spi_flash_preparation(void *obj)
{
    app_async_data_handle hdl = obj;
    external_flash_read(hdl->buffer.param.addr,PIC_FILE_INFO_HEADER_SIZE + FAST_READ_OFFSET,hdl->p_cur_buffer,NULL,NULL);
    //读取flash文件长度
    
    for(uint32_t i=0;i<PIC_FILE_INFO_HEADER_SIZE;i++)
    {
        hdl->p_cur_buffer[i] = hdl->p_cur_buffer[i + FAST_READ_OFFSET];
    }
    pic_file_header_t file_header ;
    memcpy(&file_header,hdl->p_cur_buffer,sizeof(pic_file_header_t));

    DEBUG_ASYNC_DATA_PRINTF("Async read preparation hor_size=%d,ver_size=%d,file addr=%08x\r\n",file_header.width,file_header.height,hdl->buffer.param.addr);
    DEBUG_ASYNC_DATA_PRINTF("buffer-> %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",
                            hdl->p_cur_buffer[0],hdl->p_cur_buffer[1],hdl->p_cur_buffer[2],hdl->p_cur_buffer[3],hdl->p_cur_buffer[4],
                            hdl->p_cur_buffer[5],hdl->p_cur_buffer[6],hdl->p_cur_buffer[7],hdl->p_cur_buffer[8]);

    hdl->buffer.param.addr += PIC_FILE_INFO_HEADER_SIZE; 

    DEBUG_ASYNC_DATA_PRINTF("Async read p_cur_buffer %p\r\n",hdl->p_cur_buffer);
    

    uint32_t file_size = file_header.width * file_header.height * sizeof(LCD_Rgb);
    if(hdl->buffer.size < file_size || file_size == 0)
    {
        DEBUG_ASYNC_DATA_PRINTF("Flash Read File Size Err %d\r\n",file_size);
        return async_err_drv;
    }
    hdl->buffer.size = file_size;
    hdl->p_buffer_end = hdl->buffer.buffer + hdl->buffer.size + PIC_FILE_INFO_HEADER_SIZE; 
    hdl->p_cur_buffer += PIC_FILE_INFO_HEADER_SIZE;
    DEBUG_ASYNC_DATA_PRINTF("2 Async read preparation hdl->buffer.size=%d , hdl->p_cur_buffer %p\r\n",hdl->buffer.size,hdl->p_cur_buffer);
    return async_err_none;
}

static async_errcode spi_flash_read(void *obj,uint32_t *len)
{
    app_async_data_handle hdl = obj;
    //由于目前SPI Flash读取数据，第一字节固定为0，需要偏移
    if(hdl->buffer.size <= *len)
    {
        *len = hdl->buffer.size;
    }
    if(*len > 0)
    {
        DEBUG_ASYNC_DATA_PRINTF("hdl->buffer.param.addr %08x\r\n",hdl->buffer.param.addr);
        external_flash_read(hdl->buffer.param.addr,
                            hdl->buffer.size + FAST_READ_OFFSET,
                            hdl->p_cur_buffer,
                            (external_flash_trans_finish_cb)hdl->trans_finish_cb,
                            hdl);
    }
    return async_err_none;
}



static void spi_trans_finish_cb(uint32_t arg1,uint32_t arg2,void *obj)
{
    app_async_data_handle hdl = obj;
    app_async_set_event(hdl,APP_ASYNC_READ_EVT);
    
}


#endif

#endif


