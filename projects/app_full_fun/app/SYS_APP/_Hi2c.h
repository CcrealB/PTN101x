#ifndef __HI2C_H
#define __HI2C_H


void Start();
uint8_t RecvACK();
void Stop();
void SendData(char dat);
char RecvData();
void SendACK();
void SendNAK();



extern uint8_t Ack_Flag;

//#ifdef Soft_i2c
	extern int I2cSpeed;
	extern uint8_t SDAIN_MODE;
	extern uint8_t SDAOUT_MODE;
	extern uint8_t SCL_IO;
//#endif
void Hi2c_Init();
void SET_SDA_IO(uint8_t nio);

uint8_t I2C_ReadA8D8(uint8_t id, uint8_t addr);
uint16_t I2C_ReadA8D16(uint8_t id, uint8_t addr);
uint8_t  I2C_ReadA16D8(uint8_t id, uint16_t addr);
uint16_t I2C_ReadA16D16(uint8_t id, uint16_t addr);

uint8_t I2C_WriteA8D8(uint8_t id, uint8_t addr, uint8_t Sdata);
uint8_t I2C_WriteA8D16(uint8_t id, uint8_t addr, uint16_t Sdata);
uint8_t I2C_WriteA16D8(uint8_t id, uint16_t addr, uint8_t Sdata);
uint8_t I2C_WriteA16D16(uint8_t id, uint16_t addr, uint16_t Sdata);

uint8_t I2C_WriteA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number);
void I2C_ReadA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number);

uint8_t I2C_BkWriteA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number);
void I2C_BkReadA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number);

uint8_t I2C_WriteA16Nbyte(uint8_t id, uint16_t addr, uint8_t *p, uint16_t number);
void I2C_ReadA16Nbyte(uint8_t id, uint16_t addr, uint8_t *p, uint16_t number);


#endif
