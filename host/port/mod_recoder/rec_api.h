/**
 * **************************************************************************************
 * @file    rec_api.h
 * 
 * @author  Borg Xiao
 * @date    20230515
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */

#ifndef _REC_API_H_
#define _REC_API_H_

void app_sound_record(void);

int rec_init_fs(uint8_t disk_type);//如果要一开始获取录音文件数量，先调用此接口初始化文件系统
void recorder_start(uint8_t disk_type, uint8_t en);
uint8_t recorder_is_working(void);
uint8_t rec_disk_type_get(void);


uint16_t rec_file_total_num_get(void);  //获取总的录音文件数量（需要初始化文件系统）
uint8_t rec_file_name_get_by_idx(uint16_t rec_file_idx, char* fn);  //根据索引获取文件名
void rec_file_play_by_idx(uint16_t rec_file_idx);//根据索引播放录音文件

#endif //_REC_API_H_

