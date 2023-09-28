#ifndef _PLAYMODE_H_
#define _PLAYMODE_H_
#include <stdbool.h>

typedef char *MYSTRING;
enum {
    F_TYP_MIN_SUPPORTED = 1,
    F_TYP_WAV = 1, // wav file
    F_TYP_MP3,     // mp3 file
    F_TYP_NOT_SUPPORTED = -1,
};


void strn2upper(char *str, int len); //字符串转换为大写
int ptn_strnicmp(const void* dst, const void* src, int len);
char* ptn_strrchr(char * str, char c);

int get_file_ext_name(const char *fn, char* fn_ext_out);//get ext name from file name

//检查文件是否为支持的音频文件，
//返回:F_TYP_NOT_SUPPORTED := 非支持的音频文件，>=F_TYP_MIN_SUPPORTED := 音频文件类型
int is_audio_file(char*fn);

uint16_t get_musicdir_num(void);  //获取文件系统上的所有含音乐文件的目录数量
uint16_t get_musicfile_count(void); //获取文件系统上的所有音乐文件的数量

//获取指定目录下的音乐文件的数量
uint16_t get_curDirMusicfile_count(uint16_t curDir);
//获取当前打开的文件的信息(FILE_INFO*)
void *get_file_info(void);

//为音乐文件播放分配/释放内存
int fat_malloc_files_buffer(void);
int fat_free_files_buffer(void);

//根据音乐文件编号打开文件
void*Get_File_From_Number(int number);

//根据音乐文件编号获得文件名
bool Get_File_Name_From_Number(int number,char *name);

//根据 [目录索引和文件索引] 获得 [全局文件索引]，以及反向操作
int Get_gFileIndex_By_DirFileIdx(uint16_t DirIdx, uint16_t FileIdx);
int Get_DirFileIdx_By_gFileIndex(int gFileIdx, uint16_t *pDirIdx, uint16_t *pFileIdx);

int Media_Fs_Init(uint8_t type);
int Media_Fs_Uninit(uint8_t type);

#endif
