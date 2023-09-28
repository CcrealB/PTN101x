#ifndef _DRIVER_FLASH_H
#define _DRIVER_FLASH_H

#define MEMORIZE_INTO_FLASH

#define FLASH_SPEED_QUAD_READ                   1
#define FLASH_SPEED_STANDARD_READ               0
#define FLASH_SR_SEL_2BYTE                      0
#define FLASH_SR_SEL_1BYTE                      1
#define FLASH_FCKDIV_PLL_90M                    ( 0x0 << 2 )
#define FLASH_FCKDIV_PLL_120M                   ( 0x1 << 2 )
#define FLASH_FCKDIV_XTAL                       ( 0x2 << 2 )
#define FLASH_FCKDIV_DIV1                       0
#define FLASH_FCKDIV_DIV2                       1
#define FLASH_FCKDIV_DIV4                       2
#define FLASH_FCKDIV_DIV8                       3

extern uint32_t FLASH_ENVDATA_DEF_ADDR;
#define FLASH_LINE_1    0
#define FLASH_LINE_2	1
#define FLASH_LINE_4 	2

#define FLASH_CLK_26mHz   8

#define FLASH_CLK_DPLL_2  0     /* DPLL/2 */
#define FLASH_CLK_DPLL_4  1     /* DPLL/4 */
#define FLASH_CLK_DPLL_8  2     /* DPLL/8 */
#define FLASH_CLK_DPLL_16 3     /* DPLL/16 */

#define FLASH_CLK_APLL_1  4     /* APLL/1 */
#define FLASH_CLK_APLL_2  5     /* APLL/2 */
#define FLASH_CLK_APLL_4  6     /* APLL/4 */
#define FLASH_CLK_APLL_8  7      /* APLL/8 */

#if (CPU_CLK_SEL == CPU_CLK_DPLL)
    #if (CPU_DPLL_CLK > 300000000)//flash clk should <= 78MHz  //borg @220804
        #define FLASH_CLK_SEL   FLASH_CLK_DPLL_8
    #else
        #define FLASH_CLK_SEL   FLASH_CLK_DPLL_4
    #endif
#else
    #define FLASH_CLK_SEL   FLASH_CLK_APLL_2 //FLASH_CLK_39mHz//FLASH_CLK_78mHz
#endif

#define DEFAULT_LINE_MODE  FLASH_LINE_4

/// flash operation command type(decimal)
enum {
	FLASH_OPCODE_WREN    = 1,
	FLASH_OPCODE_WRDI    = 2,
	FLASH_OPCODE_RDSR    = 3,
	FLASH_OPCODE_WRSR    = 4,
	FLASH_OPCODE_READ    = 5,
	FLASH_OPCODE_RDSR2   = 6,
	FLASH_OPCODE_WRSR2   = 7,
	FLASH_OPCODE_PP      = 12,
	FLASH_OPCODE_SE      = 13,
	FLASH_OPCODE_BE1     = 14,
	FLASH_OPCODE_BE2     = 15,
	FLASH_OPCODE_CE      = 16,
	FLASH_OPCODE_DP      = 17,
	FLASH_OPCODE_RFDP    = 18,
	FLASH_OPCODE_RDID    = 20,
	FLASH_OPCODE_HPM     = 22,
	FLASH_OPCODE_CRMR    = 22,
	FLASH_OPCODE_CRMR2   = 23,
} FLASH_OPCODE;

enum 
{
	FLASH_ERASE_4K       = 1,
	FLASH_ERASE_32K      = 2,
	FLASH_ERASE_64K      = 3,
} FLASH_ERASE_SIZE;

/*Memorize System Information into Flash, The Address based on Flash Size
    --8,   If 4Mbit Flash  4*1024*1024/8/0x10000
    --32,  If 8Mbit Flash
*/
#define FLASH_LAST_BLOCK_NUM		          (8)

#ifdef MEMORIZE_INTO_FLASH
/*suppose the size of backup info is 127 Bytes or less*/

#define FLASH_LAST_SECTOR_ADDRESS              (FLASH_LAST_BLOCK_NUM*0x10000-0x1000)
#define END_OF_INFO_SECTOR                     (4096/128)
#define MAX_STATION                            30
#define FILE_BYTE_NUM                          16   //(20+(MAX_DIR_DEPTH+1)*4)//�ϵ㱣���ֽ���  48byte

#define FLASH_4MBIT     					  0

//#define FLASH_TYPE_ZETTA					  0xF2	 // zetta flash
//#define FLASH_TYPE_BOYA_GD					  0xF1   // boya  flash

//#define FLASH_TYPE_DEF						  FLASH_TYPE_ZETTA	
/*BYTE ADDRESS*/
#define BT_VOLUME_POS                         1
#define HFP_VOLUME_POS                        2
#define MP3_VOLUME_POS                        3
#define FM_VOLUME_POS                         4
#define LINEIN_VOLUME_POS                     5

//16byte
#define SONG_INDEX_POS                        8

//����8���ֽ�
#define CUR_STATION_POS                       (SONG_INDEX_POS+FILE_BYTE_NUM+8) // 8+16+8 = 32
#define TOTAL_STATION_POS                     (CUR_STATION_POS+1)// 33
#define STATION_ARRAY_POS                     (TOTAL_STATION_POS+1)// 34

typedef enum{
    VOL_INFO_BT,
    VOL_INFO_HFP,
    VOL_INFO_MUSIC,
    VOL_INFO_LINEIN,
    SONGIDX_INFO,
    CUR_STATION_INFO,
    ALL_STATION_INFO
} flash_info_t;

#endif

void flash_init(void);
uint32_t flash_read_mID(void);
void save_volume_task(void *arg);
void set_flash_ctrl_config(void);
void set_flash_clk(unsigned char clk_conf);
void set_flash_qe(void);
uint8_t flash_get_4M_flag(void);
uint8_t flash_get_16M_flag(void);
void flash_set_line_mode(uint8_t mode);
uint8_t flash_get_line_mode(void);
void flash_memcpy(uint8_t *dst,uint8_t *src,uint32_t len,BOOL first);
void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len);
void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len);
void flash_write_page_data (uint8_t *buffer, uint32_t address, uint32_t len);
void flash_crc_remove(uint32_t address);
void flash_erase_sector(uint32_t address, uint8_t erase_size);
//void flash_self_destroyed(void);
void flash_config(void);
//void app_write_info(flash_info_t type,void * ptr1,void *ptr2,void *ptr3);
//uint8_t app_read_info(flash_info_t type,void * ptr1,void *ptr2,void *ptr3);
#endif
