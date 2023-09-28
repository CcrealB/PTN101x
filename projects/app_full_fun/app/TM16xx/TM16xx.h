
#ifndef	__TM1668_H
#define	__TM1668_H










void UpDisplayReg(uint8_t Addr, uint8_t OnOff);
void UpDisplayAll();
void UpDisplayOnOff(uint8_t WData);
void LED_TEST(uint8_t UpDn);
uint32_t read_key();
void TM16xx_Init(uint8_t Step);

#endif

