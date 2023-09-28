/********************************************
 OLED屏驅動程序
********************************************/
#include "USER_Config.h"

#ifdef	OLED128x64

#include <font.h>

#define	Brigh	255		//設定亮度  0~255
#define	OLED_ID 0x3C	//0x78

#if defined(PTN1011_DEMO_V2p2) || defined(PTN1012_DEMO_V2p2)
//	#define reverse
#endif

#define mode 1

#if mode == 1	//0.96吋 128x64
	#define X_Width	128		//螢幕寬度
	#define Y_High	64		//螢幕高度
	#define Y_St	0xB0	//設定Y偏移
	#define XH_St	0x10	//設定X高位偏移
	#define XL_St	0x00	//設定X低位偏移
#endif

#if mode == 2	//1.3吋	128x64
	#define X_Width	128		//螢幕寬度
	#define Y_High	64		//螢幕高度
	#define Y_St	0xB0	//設定Y偏移
	#define XH_St	0x10	//設定X高位偏移
	#define XL_St	0x02	//設定X低位偏移
#endif

#if mode == 3	//0.66吋	64x48
	#define X_Width	64		//螢幕寬度
	#define Y_High	48		//螢幕高度
//	#define Y_St	0xB0	//設定Y偏移		反向顯示
	#define Y_St	0xB2	//設定Y偏移		正常顯示
	#define XH_St	0x12	//設定X高位偏移
	#define XL_St	0x00	//設定X低位偏移
#endif

#if mode == 4	//0.91吋 128x32
	#define X_Width	128		//螢幕寬度
	#define Y_High	32		//螢幕高度
	#define Y_St	0xB0	//設定Y偏移
	#define XH_St	0x10	//設定X高位偏移
	#define XL_St	0x00	//設定X低位偏移
#endif


//*******************************************************
void LCD_WrDat(uint8_t dat)
{
	I2C_WriteA8D8(OLED_ID, 0x40, dat);
}

//********************************************************
void LCD_WrCmd(uint8_t cmd)
{
	I2C_WriteA8D8(OLED_ID, 0x00, cmd);
}

//*******************************************
void LCD_Set_Pos(uint8_t x, uint8_t y)
{ 
	LCD_WrCmd(Y_St+y);
	LCD_WrCmd(((x&0xf0)>>4)+XH_St);
	LCD_WrCmd((x&0x0f)+XL_St);
} 

//********************************************
void LCD_Fill(uint8_t val)
{
	uint8_t y,x;
	for(y=0;y<(Y_High/8);y++){
		LCD_WrCmd(Y_St+y);
		LCD_WrCmd(XL_St);
		LCD_WrCmd(XH_St);
		for(x=0;x<X_Width;x++){
			LCD_WrDat(val);
		}
	}
}

//==============================================================
//函數名：LCD_P6x8Str(unsigned char x,unsigned char y,unsigned char *p)
//功能描述：寫入一組標準ASCII字符串
//參數：顯示的位置（x,y），y為頁範圍0～7，要顯示的字符串
//返回：無
//==============================================================  
void OLED_x6y8str(uint8_t x,uint8_t y,char *p)
{
	if(OLED_EN==0)	return;
	uint8_t c=0,i=0,j=0;
	while (p[j]!='\0'){    
		c =p[j]-32;
		if(x>126){x=0;y++;}
		LCD_Set_Pos(x,y);    
		for(i=0;i<6;i++)	LCD_WrDat(F6x8[c][i]);  
		x+=6;
		j++;
	}
}
//==============================================================
//函數名：LCD_P8x16Str(uint8_t x,uint8_t y,uint8_t *p)
//功能描述：寫入一組標準ASCII字符串
//參數：顯示的位置（x,y），y為頁範圍0～7，要顯示的字符串
//返回：無
//============================================================== 
void OLED_x8y16str(uint8_t x,uint8_t y, char *p)
{
	if(OLED_EN==0)	return;
//	uint8_t c=0,i=0,j=0;
	uint8_t c=0,j=0;
	while (p[j]!='\0' && j<16){
		c =p[j]-32;
		if(x>120){x=0;y++;}
		LCD_Set_Pos(x,y);    
	//	for(i=0;i<8;i++)	LCD_WrDat(F8X16[c][i]);
		I2C_WriteA8Nbyte(OLED_ID, 0x40, (uint8_t*)&F8X16[c][0],8);
		LCD_Set_Pos(x,y+1);    
	//	for(i=0;i<8;i++)	LCD_WrDat(F8X16[c][i+8]);
		I2C_WriteA8Nbyte(OLED_ID, 0x40, (uint8_t*)&F8X16[c][8],8);
		x+=8;
		j++;
	}
}

//**** display num, font style:8*16 dots ****
void OLED_num(uint8_t column, uint8_t page, uint32_t num, uint8_t len, uint8_t size)
{
	if(OLED_EN==0)	return;
  	uint8_t i;
	uint16_t	Digit;
	uint32_t Divisor;
	uint8_t Printed = 0;
	char StringBuf[6];
	for(i=0; i<6; i++)	StringBuf[i] = ' ';
	if(num){
		Divisor=10000;
		for(i=0; i<5; i++){
			Digit = num/Divisor;
			if(Digit || Printed){
				StringBuf[i] = Digit +'0';	//數值 + 文字 '0' 指標
				num -= Digit*Divisor;
				Printed = 1;
			}else{
				StringBuf[i] = ' ';
			}
			Divisor /= 10;
		}
		StringBuf[i] = '\0';
	}else{
		StringBuf[4] = '0';
		StringBuf[5] = '\0';
	}
	if(size==6){
		OLED_x6y8str(column, page, &StringBuf[5-len]);
	}else{
		OLED_x8y16str(column, page, &StringBuf[5-len]);
	}
}

//***********************************************************
/*
void OLED_12x16_cht(uint8_t x, uint8_t y, uint8_t *s)
{
	uint8_t i, k;
	uint8_t c[2];
	uint8_t HZnum=sizeof(font12x16Index)/2;	//自動統計漢字數目
 
	while(*s){
	
		c[0]=*s++;

		if(c[0] <128){
			c[1]= '\0';
			OLED_x8y16str(x, y, c);
			x+=8;
		}else{
			c[1]=*s++; 
			for (k=0;k<HZnum;k++) { //自建漢字庫中的個數，循環查詢內碼
				if ((font12x16Index[(k*2)]==c[0])&&(font12x16Index[(k*2+1)]==c[1])){ 
					LCD_Set_Pos(x , y);
  					for(i=0; i<12; i++){
  						LCD_WrDat(font12x16[k][i*2]);		
  					}
					LCD_Set_Pos(x , y+1);
  					for(i=0; i<12; i++){
  						LCD_WrDat(font12x16[k][i*2+1]);		
  					}
					x+=12;
				}	 	  	
			}
		}	

	}
}
*/

//***** 顯示BMP圖片64x48起始點坐標(x,y),x的範圍0～127，y為頁的範圍0～7 *****
/*
void Draw_BMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,const uint8_t *p)
{
	uint8_t x,y;
	if(y1%8==0) y=y1/8;      
		else y=y1/8+1;
	for(y=y0;y<y1;y++){
		LCD_Set_Pos(x0,y);
    	for(x=x0;x<x1;x++){      
	    	LCD_WrDat(p[x]);
	    }
	}
}
*/
//***** 顯示 電池 BMP圖片64x48 起始點坐標(x,y),x的範圍0～127，y為頁的範圍0～7
/*
void Batt_BMP(uint8_t x, uint8_t y, uint8_t v, uint8_t e)	//v= 0~14
{
	uint8_t i;
	uint8_t code Batt[]={0x3C,0x24,0xE7,0x81,0xBD,0xFF};
	if(v>10)	v=10;	
	LCD_Set_Pos(x,y);
   	for(i=0; i<16;i++){
		if(e){
			if(i<=3){
				LCD_WrDat(Batt[i]);	
			}else if(i==14){
				LCD_WrDat(Batt[3]);
			}else if(i==15){
				LCD_WrDat(Batt[5]);
			}else{
				if((i-3)<=(10-v)){
			   		LCD_WrDat(Batt[3]);
				}else{
					LCD_WrDat(Batt[4]);	
				}
			}
	  	}else{
			LCD_WrDat(0);	
		}
    }
}
*/
//*****************************************************************
/*
void Ant_BMP(uint8_t x, uint8_t y, uint8_t v)
{
	uint8_t data i;
	uint8_t code Ant[]={0x03,0x05,0x09,0x11,0xFF,0x11,0x89,0x05,0xC3,0x00,0xE0,0x00,0xF0,0x00,0xF8,0x00,0xFC};
	LCD_Set_Pos(x,y);
   	for(i=0; i<17 ;i++){
		if(i<9+v*2){
			LCD_WrDat(Ant[i]);	
		}else{
			LCD_WrDat(0);
		}
    }
}
*/

//**** 開啟OLED顯示 *************************************************
void OLED_Display_On(void)
{
	LCD_WrCmd(0X8D);  //SET DCDC命令
	LCD_WrCmd(0X14);  //DCDC ON
	LCD_WrCmd(0XAF);  //DISPLAY ON
}
//**** 關閉OLED顯示 *************************************************
void OLED_Display_Off(void)
{
	LCD_WrCmd(0X8D);  //SET DCDC命令
	LCD_WrCmd(0X10);  //DCDC OFF
	LCD_WrCmd(0XAE);  //DISPLAY OFF
}

uint8_t OLED_EN = 0;
//***********************************************************************
void OLED_Init(void)        
{  
	uint8_t i;
	for(i=0;i<10; i++){
		I2C_WriteA8D8(OLED_ID, 0x00, 0xAE);
		OLED_EN = (~Ack_Flag)&1;
		if(OLED_EN==1) break;
	}

	if(OLED_EN==1){
		DBG_LOG_INFO("OLED_I2C_ID = 0x78   %d\n", i);	//print ID
	}else{
		DBG_LOG_ERR("OLED_I2C_ID = ERR.....\n");	//print ID
		return;
	}

	//從上電到下面開始初始化要有足夠的時間，即等待RC復位完畢
//	HAL_Delay(120);
	
	LCD_WrCmd(0xAE);	// 關閉顯示
	LCD_WrCmd(0x00);	// 設置顯示行位址 (3:0) 低位元
	LCD_WrCmd(0x10);	// 設置顯示行位址 (3:0) 高位元
   	LCD_WrCmd(0x40);	// 設置顯示開始行 [5:0],行數.(0x00~0x3F)
	LCD_WrCmd(0xB0);	// --set page address		//Set Page Start Address for Page Addressing Mode,0-7
#ifdef reverse
	LCD_WrCmd(0xA0);	//0xa0左右反置 0xa1正常	//段重定義設置,bit0:0,0->0;1,0->127;
	LCD_WrCmd(0xC0);	//0xc0上下反置 0xc8正常	//設置COM掃瞄方向;bit3:0,普通模式;1,重定義模式 COM[N-1]->COM0;N:驅動路數
#else
	LCD_WrCmd(0xA1);	//0xa0左右反置 0xa1正常	//段重定義設置,bit0:0,0->0;1,0->127;
	LCD_WrCmd(0xC8);	//0xc0上下反置 0xc8正常	//設置COM掃瞄方向;bit3:0,普通模式;1,重定義模式 COM[N-1]->COM0;N:驅動路數
#endif
	LCD_WrCmd(0xD5); LCD_WrCmd(0x80);		//0x80 F0 設置時鐘分頻因子,震盪頻率;	//[3:0],分頻因子;[7:4],震盪頻率
#if Y_High == 32	//128x32 OLED
	LCD_WrCmd(0xA8); LCD_WrCmd(0x1F);		//0x3F 1F 設置驅動路數;	默認0X3F (1/32)
	LCD_WrCmd(0xDA); LCD_WrCmd(0x02);		//0x12 02 設置COM硬件引腳配置;	[5:4]配
#else
	LCD_WrCmd(0xA8); LCD_WrCmd(0x3F);		//0x3F 1F 設置驅動路數;	默認0X3F (1/64)
	LCD_WrCmd(0xDA); LCD_WrCmd(0x12);		//0x12 02 設置COM硬件引腳配置;	[5:4]配
#endif
	LCD_WrCmd(0xD3); LCD_WrCmd(0x00);		//設置顯示偏移;	//默認為0
	LCD_WrCmd(0x8D); LCD_WrCmd(0x14);		//0x14,10 電荷泵設置;	(0x10) disable	//bit2，開啟/關閉
	LCD_WrCmd(0x20); LCD_WrCmd(0x02);		//Set Page Addressing Mode (0x00/0x01/0x02)
	LCD_WrCmd(0x81); LCD_WrCmd(Brigh);		//對比度設置;		//1~255;默認0X7F (亮度設置,越大越亮)
	LCD_WrCmd(0xD9); LCD_WrCmd(0xF1);		//0xF1 22 設置預充電週期 	//[3:0],PHASE 1;[7:4],PHASE 2;
	LCD_WrCmd(0xDB); LCD_WrCmd(0x30);		//0x30 49 設置VCOMH 電壓倍率	//[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

//	LCD_WrCmd(0xD8); LCD_WrCmd(0x05);		//set area color mode off

//	LCD_WrCmd(0x2E); 	//2012-05-27: Deactivate scroll
	LCD_WrCmd(0xA4);	//(0xa4/0xa5)	//全局顯示開啟;bit0:1,開啟;0,關閉;(白屏/黑屏)
	LCD_WrCmd(0xA6);	//(0xa6/a7)		//設置顯示方式;bit0:1,反相顯示;0,正常顯示
	
	LCD_Fill(0);		//初始清屏
	LCD_Set_Pos(0,0); 
	LCD_WrCmd(0xAF);	//--turn on oled panel 						//開啟顯示

//	Draw_BMP(0,0,128,8,BMP1);  //圖片顯示(圖片顯示慎用，生成的字表較大，會佔用較多空間，FLASH空間8K以下慎用)
#if 0
	OLED_x6y8str(0,0,(uint8_t*)"1234567890");
	OLED_x6y8str(0,1,(uint8_t*)"123456789012345678901");
	OLED_x6y8str(0,2,(uint8_t*)"ABCDEFGHIJ");
	OLED_x6y8str(0,3,(uint8_t*)"ABCDEFGHIJKLMNOPQRSTU");
	OLED_x8y16str(0,4,(uint8_t*)"1234567890123456");
	OLED_x8y16str(0,6,(uint8_t*)"ABCDEFGHIJKLMNOP");
#endif
} 

#endif

