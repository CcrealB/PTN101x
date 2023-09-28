#ifndef _USBH_UDISK_H_
#define _USBH_UDISK_H_

typedef enum{
	UDISK_RET_OK=0,
	UDISK_RET_ERR=-1,	
}ERR_UDISK;

int udisk_is_attached(void);

uint32_t udisk_get_size(void);
ERR_UDISK udisk_init(void);
void udisk_uninit(void);

void usbh_udisk_init_cmp_callback(void);
void usbh_udisk_lost_callback(void);

ERR_UDISK udisk_rd_blk_sync(int sector,int count,void*buff);
ERR_UDISK udisk_write_blk_sync(int sector,int count,void*buff);
ERR_UDISK udisk_rd_blk_async(int sector,int count,void*buff,void*cbk);
ERR_UDISK udisk_write_blk_async(int sector,int count,void*buff,void*cbk);



#endif /* _USBH_UDISK_H_ */
