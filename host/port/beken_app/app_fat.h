
/*
 * app_fat.h
 *
 *  Created on: 2021年3月23日
 *      Author: Jiang
 */
#ifndef _APP_FAT_H_
#define _APP_FAT_H_

#define FAT_PRINTF          os_printf
#define FAT_LOG_I(fmt,...)  FAT_PRINTF("[FAT|I]"fmt, ##__VA_ARGS__)
#define FAT_LOG_W(fmt,...)  FAT_PRINTF("[FAT|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FAT_LOG_E(fmt,...)  FAT_PRINTF("[FAT|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FAT_LOG_D(fmt,...)  FAT_PRINTF("[FAT|D]"fmt, ##__VA_ARGS__)

// #define TEST_FATFS_WRITE_SPEED


//参数说明: p1: 通常为文件的全路径名；p2: 一般为输入参数指针；p3: 一般为输出参数指针
typedef int (*CBK_TRAVERSAL_FATFS)(void*p1,void*p2,void*p3);

typedef struct{
	int total;//total number of media files
	int tarInd;//the index number wanted
	int curIndex;//current index cnt
}_t_cnt_info;

typedef struct{
	unsigned char c_id[4];
	unsigned int sz;
	unsigned char val[0];
}_t_riff_chunk;

typedef struct {
	unsigned short int wTag;    //压缩格式
	unsigned short int wChn;    //声道数
	unsigned int dwSampleRate;  //采样率
	unsigned int dwAvgBytesPerSec;//平均每秒采样/播放的字节数
	unsigned short int wBlockAlign;//块对齐字节数
	unsigned short int wBitsPerSample;//样本分辨率
}_t_wave_hdr_format;

enum{
	TD_ERRCODE_SUCCESS=0,
	TD_ERRCODE_FsEnd,
	TD_ERRCODE_DirEnd,
	TD_ERRCODE_Mem=-1,
	TD_ERRCODE_DirOpen=-2,
	TD_ERRCODE_DirRead=-3,
};

// 获取音频文件的头信息（.wav）
int get_wav_file_hdr_info(void *fp, void *fmt, BYTE **dp, unsigned int *dat0_len, unsigned int *dat_total_len);

// 播放音频文件
void play_wav_file(void *fp);

// 遍历指定文件夹
int Traversal_Dir(char *dir_name, uint8_t sub_dir_en, void *cbk, void *p1, void *p2);

// 通过音频文件索引号获取文件全路径名
char *get_media_file_name_by_index(int idx, int total);

// 获取音频文件总数
int get_media_file_counter(void);

void aud_cvt_to_pcm32_stereo(void *odat, int sz, void *idat, void *fmt);

void app_fat_init(void);

/* ******************** SDCard ******************** */
void sd_inserted_action(void);
void sd_pull_out_action(void);


#if 0
#define     FAT_TABLE_FILE_END      0xFFFFFFF

int fat_init( void );
int fat_get_next_file( fat_file_info_t *file_info, char *ext, int index );
int fat_get_prev_file( fat_file_info_t *file_info, char *ext, int index ); //need test, may take high mcu cost
uint32_t fat_get_file_next_cluster( uint32_t cluster );
uint32_t fat_blk_from_cluster( uint32_t cluster );
uint32_t fat_cluster_from_blk( uint32_t blk );
int fat_get_prev_subdirs( fat_file_info_t *file_info, char *ext, int index );
int fat_get_next_subdirs( fat_file_info_t *file_info, char *ext, int index );
int fat_change_cur_dir( uint32_t cluster );
int fat_change_dir_to_parent( void );
uint32_t fat_get_root_cluster( void );

uint32_t fat_get_blocks_per_cluster( void );

//add by zjw 
uint8_t fat_get_ft_file_flag( void );
void fat_get_fat_info_print( void );
void fat_exact_info_from_index_print( uint32_t index );
int fat_change_to_root_dir( void );
int fat_set_int_flag_for_first_search( int value );
int fat_check_file_exist( fat_file_info_t *file_info, char *ext, int index );
#endif

#endif /* _APP_FAT_H_ */


