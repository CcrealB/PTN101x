#ifndef _CONFIG_SYSTEM_H_
#define _CONFIG_SYSTEM_H_


#define CODE_VERSION 0x00
#define FLASH_INFO_ADDR   0x119

#define FLASH_INFO_4M     1
#define FLASH_INFO_8M     2
#define FLASH_INFO_16M    3
#define FLASH_INFO_32M    4

#include "u_config.h"

#define FLASH_INFO        FLASH_SIZE//FLASH_INFO_16M  //1=4M;2=8M;3=16M;4=32M

#define BOOT_WAIT_TIME    16              // 1=1ms
#endif

//EOF
