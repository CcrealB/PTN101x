
#ifndef	__OLED_H
#define	__OLED_H

extern uint8_t OLED_EN;
void OLED_Init(void);
void LCD_WrDat(uint8_t dat);
void LCD_Set_Pos(uint8_t x, uint8_t y);
void OLED_x6y8str(uint8_t x,uint8_t y,char *p);
void OLED_x8y16str(uint8_t x,uint8_t y,char *p);
//void OLED_12x16_cht(uint8_t x, uint8_t y, s8 *s);
void OLED_num(uint8_t column, uint8_t page, uint32_t num, uint8_t len, uint8_t size);
void LCD_Fill(uint8_t val);
//void Draw_BMP(uint8_t x0, uint8_t y0,uint8_t x1, uint8_t y1,const s8 *p);
//void Batt_BMP(uint8_t x, uint8_t y, uint8_t v, uint8_t e);
//void Ant_BMP(uint8_t x, uint8_t y, uint8_t v);
//void OLED_Display_Off(void);


#endif

