#ifndef __ST7789V_H
#define __ST7789V_H

#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t

//LCD重要参数集
typedef struct
{
	u16 width;			//LCD 宽度
	u16 height;			//LCD 高度
	u16 id;				//LCD ID
	u8  dir;			//横屏还是竖屏控制：0，竖屏；1，横屏。
	u8	wramcmd;		//开始写gram指令
	u8  setxcmd;		//设置x坐标指令
	u8  setycmd;		//设置y坐标指令
}_lcd_dev;

//LCD参数
extern _lcd_dev lcddev;	//管理LCD重要参数
//LCD的画笔颜色和背景色
extern u16  POINT_COLOR;
extern u16  BACK_COLOR;


//////////////////////////////////////////////////////////////////////////////////	 
//-----------------LCD端口定义----------------
//#define	LCD_RESET PAout(0)	 //LCD复位    		 PA0
//#define	LCD_BL    PBout(1)   //LCD背光    		 PB1

//#define	LCD_CS	PAout(1)  //片选端口  	     PA1
//#define	LCD_RS	PAout(2)  //数据/命令        PA2
//#define	LCD_SCL	PAout(3)  //写数据			 PA3
//#define	LCD_SDA	PAout(4)  //读数据			 PA4
//#define LCD_SDO PCin(0)   //PC0    SDO
	 
//扫描方向定义
#define L2R_U2D  0 //从左到右,从上到下
#define L2R_D2U  1 //从左到右,从下到上
#define R2L_U2D  2 //从右到左,从上到下
#define R2L_D2U  3 //从右到左,从下到上

#define U2D_L2R  4 //从上到下,从左到右
#define U2D_R2L  5 //从上到下,从右到左
#define D2U_L2R  6 //从下到上,从左到右
#define D2U_R2L  7 //从下到上,从右到左

extern u8 DFT_SCAN_DIR;


//PC0~15,作为数据口
//#define DATAOUT(x) GPIOC->ODR=x; //数据输出
//#define DATAIN     GPIOC->IDR;   //数据输入

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色
 
#define LIGHTGREEN     	 0X841F //浅绿色
//#define LIGHTGRAY        0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)
	    															  
//void LCD_Init(void);													   	//初始化
//void LCD_DisplayOn(void);													//开显示
//void LCD_DisplayOff(void);													//关显示
void LCD_Clear(u16 Color);	 												//清屏
//void LCD_SetCursor(u16 Xpos, u16 Ypos);										//设置光标
//void LCD_DrawPoint(u16 x,u16 y);											//画点
//void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color);								//快速画点
//u16  LCD_ReadPoint(u16 x,u16 y); 											//读点
//void Draw_Circle(u16 x0,u16 y0,u8 r);										//画圆
//void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);							//画线
//void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);		   				//画矩形
//void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color);		   				//填充单色
//void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color);				//填充指定颜色
//void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode);						//显示一个字符
//void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size);  						//显示一个数字
//void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode);				//显示 数字
//void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p);		//显示一个字符串,12/16字体
//void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height);
//void IO_init(void);

void LCD_PutString(uint16_t x, uint16_t y, uint8_t *s, uint8_t size, uint16_t fColor, uint16_t bColor);
void LCD_ShowxNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint8_t mode);
void LCD_ShowxNum_gre(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode);
void LCD_ShowxNum_red(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode);
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 fColor);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 lineW, u16 fColor);
void DisplayMenuChg();



void LCD_PutString(u16 x, u16 y, u8 *s, u8 size, u16 fColor, u16 bColor);
 					   																			 
//9320/9325 LCD寄存器
#define R0             0x00
#define R1             0x01
#define R2             0x02
#define R3             0x03
#define R4             0x04
#define R5             0x05
#define R6             0x06
#define R7             0x07
#define R8             0x08
#define R9             0x09
#define R10            0x0A
#define R12            0x0C
#define R13            0x0D
#define R14            0x0E
#define R15            0x0F
#define R16            0x10
#define R17            0x11
#define R18            0x12
#define R19            0x13
#define R20            0x14
#define R21            0x15
#define R22            0x16
#define R23            0x17
#define R24            0x18
#define R25            0x19
#define R26            0x1A
#define R27            0x1B
#define R28            0x1C
#define R29            0x1D
#define R30            0x1E
#define R31            0x1F
#define R32            0x20
#define R33            0x21
#define R34            0x22
#define R36            0x24
#define R37            0x25
#define R40            0x28
#define R41            0x29
#define R43            0x2B
#define R45            0x2D
#define R48            0x30
#define R49            0x31
#define R50            0x32
#define R51            0x33
#define R52            0x34
#define R53            0x35
#define R54            0x36
#define R55            0x37
#define R56            0x38
#define R57            0x39
#define R59            0x3B
#define R60            0x3C
#define R61            0x3D
#define R62            0x3E
#define R63            0x3F
#define R64            0x40
#define R65            0x41
#define R66            0x42
#define R67            0x43
#define R68            0x44
#define R69            0x45
#define R70            0x46
#define R71            0x47
#define R72            0x48
#define R73            0x49
#define R74            0x4A
#define R75            0x4B
#define R76            0x4C
#define R77            0x4D
#define R78            0x4E
#define R79            0x4F
#define R80            0x50
#define R81            0x51
#define R82            0x52
#define R83            0x53
#define R96            0x60
#define R97            0x61
#define R106           0x6A
#define R118           0x76
#define R128           0x80
#define R129           0x81
#define R130           0x82
#define R131           0x83
#define R132           0x84
#define R133           0x85
#define R134           0x86
#define R135           0x87
#define R136           0x88
#define R137           0x89
#define R139           0x8B
#define R140           0x8C
#define R141           0x8D
#define R143           0x8F
#define R144           0x90
#define R145           0x91
#define R146           0x92
#define R147           0x93
#define R148           0x94
#define R149           0x95
#define R150           0x96
#define R151           0x97
#define R152           0x98
#define R153           0x99
#define R154           0x9A
#define R157           0x9D
#define R192           0xC0
#define R193           0xC1
#define R229           0xE5
//=========================================================
//---------------------image2lcd---------------------------
//4096色/16位真彩色/18位真彩色/24位真彩色/32位真彩色
//图像数据头结构体
 typedef struct _HEADCOLOR
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
   unsigned char is565;
   unsigned char rgb;
}HEADCOLOR;
//scan: 扫描模式
//Bit7: 0:自左至右扫描，1:自右至左扫描。
//Bit6: 0:自顶至底扫描，1:自底至顶扫描。
//Bit5: 0:字节内象素数据从高位到低位排列，1:字节内象素数据从低位到高位排列。
//Bit4: 0:WORD类型高低位字节顺序与PC相同，1:WORD类型高低位字节顺序与PC相反。
//Bit3~2: 保留。
//Bit1~0: [00]水平扫描，[01]垂直扫描，[10]数据水平,字节垂直，[11]数据垂直,字节水平。
//gray: 灰度值
//   灰度值，1:单色，2:四灰，4:十六灰，8:256色，12:4096色，16:16位彩色，24:24位彩色，32:32位彩色。
//w: 图像的宽度。
//h: 图像的高度。
//is565: 在4096色模式下为0表示使用[16bits(WORD)]格式，此时图像数据中每个WORD表示一个象素；为1表示使用[12bits(连续字节流)]格式，此时连续排列的每12Bits代表一个象素。
//在16位彩色模式下为0表示R G B颜色分量所占用的位数都为5Bits，为1表示R G B颜色分量所占用的位数分别为5Bits,6Bits,5Bits。
//在18位彩色模式下为0表示"6Bits in Low Byte"，为1表示"6Bits in High Byte"。
//在24位彩色和32位彩色模式下is565无效。
//rgb: 描述R G B颜色分量的排列顺序，rgb中每2Bits表示一种颜色分量，[00]表示空白，[01]表示Red，[10]表示Green，[11]表示Blue。

void image_display(u16 x,u16 y,u8 * imgx);//在指定位置显示图片
void image_show(u16 xsta,u16 ysta,u16 xend,u16 yend,u8 scan,u8 *p);//在指定区域开始显示图片
u16 image_getcolor(u8 mode,u8 *str);//获取颜色
void DMAimage_display(u16 x,u16 y,u8 * imgx);

//=========================================================
//-----------------------picture---------------------------
const unsigned char gImage_logo[3488];//logo
const unsigned char gImage_num0[708];//数字0
const unsigned char gImage_num1[708];//
const unsigned char gImage_num2[708];//
const unsigned char gImage_num3[708];//
const unsigned char gImage_num4[708];//
const unsigned char gImage_num5[708];//
const unsigned char gImage_num6[708];//
const unsigned char gImage_num7[708];//
const unsigned char gImage_num8[708];//
const unsigned char gImage_num9[708];//
const unsigned char gImage_num0_gre[708];//绿色0
const unsigned char gImage_num1_gre[708];//
const unsigned char gImage_num2_gre[708];//
const unsigned char gImage_num3_gre[708];//
const unsigned char gImage_num4_gre[708];//
const unsigned char gImage_num5_gre[708];//
const unsigned char gImage_num6_gre[708];//
const unsigned char gImage_num7_gre[708];//
const unsigned char gImage_num8_gre[708];//
const unsigned char gImage_num9_gre[708];//
const unsigned char gImage_numg1[1338];

const unsigned char gImage_pow_frame[560];//电量外框
const unsigned char gImage_pow_frameRed[560];//电量外框红

const unsigned char gImage_pow_frame1[408];

const unsigned char gImage_voice_tube[428];//音乐
const unsigned char gImage_voice_tube_gre[428];
const unsigned char gImage_Mus_icon[398];//麦克风
const unsigned char gImage_Mus_icon_gre[398];
const unsigned char gImage_rev_icon[398];//混响
const unsigned char gImage_rev_icon_gre[398];
const unsigned char gImage_ehco_icon[338];//回音
const unsigned char gImage_ehco_icon_gre[338];
const unsigned char gImage_Bluto_icon[348];//蓝牙
const unsigned char gImage_Bluto_icon_gre[348];
const unsigned char gImage_lightning[408];//充电




const unsigned char gImage_iridescence[608];//彩虹条
void LCD_ShowPic_gre_Num(u16 x,u16 y,u32 num);//指定位置显示绿色数字
void LCD_ShowPicNum(u16 x,u16 y,u32 num);//指定位置显示数字

void bat_sta(int a);//电池总电量
void ShowBattAch(uint16_t a);//麦克风电池状态
void ShowBattBch(uint16_t a);//麦克风电池状态
void ShowRssiAch(int a);//信号1
void ShowRssiBch(int a);//信号2

//void LCD_PutChar6x8(u16 x, u16 y, u8 c, u16 fColor, u16 bColor);
//void LCD_PutChar8x16(u16 x, u16 y, u8 c, u16 fColor, u16 bColor);
void ShowNum(u16 x, u16 y, u32 num,u8 len,u16 fColor, u16 bColor);
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color);
void ST7789V_BL_EN(uint8_t OnOff);
uint8_t ST7789V_main(void);
void LCD_ShowxNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint8_t mode);
#endif
	 


