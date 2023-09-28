#ifndef _DRIVER_EFUSE_H_
#define _DRIVER_EFUSE_H_
#include <stdint.h>
#define EFUSE_EN   1
void eFuse_enable(uint8_t enable);
uint8_t eFuse_write(uint8_t* data,uint8_t addr,uint8_t len);
uint8_t eFuse_read(uint8_t* data,uint8_t addr,uint8_t len);

#endif
