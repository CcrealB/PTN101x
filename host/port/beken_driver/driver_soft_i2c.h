#ifndef _DRIVER_SOFT_I2C_H_
#define _DRIVER_SOFT_I2C_H_

void Soft_I2C_Read(uint8_t dev, uint8_t addr, uint8_t Size, void *d);
void Soft_I2C_Write(uint8_t dev, uint8_t addr, uint8_t Size, void *d);

#endif


