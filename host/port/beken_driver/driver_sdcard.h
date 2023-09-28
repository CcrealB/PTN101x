#ifndef __DRIVER_SDCARD_H
#define __DRIVER_SDCARD_H
// #include "types.h"

// #include "app_debug.h"

#define SD_LITTLE_EDIAN    1 // 1:little endian, 0:big endian

#if DRV_DEBUG_SDCARD_ENABLE
#define SDCARD_PRT(fmt,...)    do{if(mcu_dbg_drv_ctrl & MCU_DBG_DRV_SDCARD_ENABLE_MASK)DEBUG_DRV_PRINTF("_SD1",fmt,##__VA_ARGS__);}while(0)
#define SDCARD_WARN(fmt,...)   do{if(mcu_dbg_drv_ctrl & MCU_DBG_DRV_SDCARD_ENABLE_MASK)DEBUG_DRV_PRINTF("_SD2",fmt,##__VA_ARGS__);}while(0)
#define SDCARD_DEBUG(fmt,...)  do{if(mcu_dbg_drv_ctrl & MCU_DBG_DRV_SDCARD_ENABLE_MASK)DEBUG_DRV_PRINTF("_SD3",fmt,##__VA_ARGS__);}while(0)
#else
#define SDCARD_PRT(fmt,...)  
#define SDCARD_WARN(fmt,...) 
#define SDCARD_DEBUG(fmt,...)
#endif
#define SDCARD_FATAL(fmt,...)    os_printf("[SD_FATAL]%d "fmt, __LINE__, ##__VA_ARGS__)

#define SD_PRINTF          os_printf
#define SD_LOG_I(fmt,...)  SD_PRINTF("[SD|I]"fmt, ##__VA_ARGS__)
#define SD_LOG_W(fmt,...)  SD_PRINTF("[SD|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SD_LOG_E(fmt,...)  SD_PRINTF("[SD|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SD_LOG_D(fmt,...)  SD_PRINTF("[SD|D]"fmt, ##__VA_ARGS__)



// SDCARD REG
#define SDCARD_BASE_ADDR                    (0x01878000)

#define REG_SDCARD_CMD_SEND_CTRL            (SDCARD_BASE_ADDR + 0*4)
#define SDCARD_CMD_SEND_CTRL_CMD_START        (1 << 0)
#define SDCARD_CMD_SEND_CTRL_CMD_FLAGS_MASK   (0x7)
#define SDCARD_CMD_SEND_CTRL_CMD_FLAGS_POSI   (1)
#define SDCARD_CMD_SEND_CTRL_CMD_INDEX_MASK   (0x3f)
#define SDCARD_CMD_SEND_CTRL_CMD_INDEX_POSI   (4)

#define REG_SDCARD_CMD_SEND_AGUMENT         (SDCARD_BASE_ADDR + 1*4)
#define REG_SDCARD_CMD_RSP_TIMER            (SDCARD_BASE_ADDR + 2*4)

#define REG_SDCARD_DATA_REC_CTRL            (SDCARD_BASE_ADDR + 3*4)
#define SDCARD_DATA_REC_CTRL_DATA_EN          (1 << 0)
#define SDCARD_DATA_REC_CTRL_DATA_STOP_EN     (1 << 1)
#define SDCARD_DATA_REC_CTRL_DATA_BUS         (1 << 2)
#define SDCARD_DATA_REC_CTRL_DATA_MUL_BLK     (1 << 3)
#define SDCARD_DATA_REC_CTRL_BLK_SIZE_MASK    (0xfff)
#define SDCARD_DATA_REC_CTRL_BLK_SIZE_POSI    (4)
#define SDCARD_DATA_REC_CTRL_DATA_WR_DATA_EN  (1 << 16)
#define SDCARD_DATA_REC_CTRL_DATA_BYTE_SEL    (1 << 17)

#define REG_SDCARD_DATA_REC_TIMER           (SDCARD_BASE_ADDR + 4*4)
#define REG_SDCARD_CMD_RSP_AGUMENT0         (SDCARD_BASE_ADDR + 5*4)
#define REG_SDCARD_CMD_RSP_AGUMENT1         (SDCARD_BASE_ADDR + 6*4)
#define REG_SDCARD_CMD_RSP_AGUMENT2         (SDCARD_BASE_ADDR + 7*4)
#define REG_SDCARD_CMD_RSP_AGUMENT3         (SDCARD_BASE_ADDR + 8*4)

#define REG_SDCARD_CMD_RSP_INT_SEL          (SDCARD_BASE_ADDR + 9*4)
#define SDCARD_CMDRSP_NORSP_END_INT           (1 << 0)
#define SDCARD_CMDRSP_RSP_END_INT             (1 << 1)
#define SDCARD_CMDRSP_TIMEOUT_INT             (1 << 2)
#define SDCARD_CMDRSP_DATA_REC_END_INT        (1 << 3)
#define SDCARD_CMDRSP_DATA_WR_END_INT         (1 << 4)
#define SDCARD_CMDRSP_DATA_TIME_OUT_INT       (1 << 5)
#define SDCARD_CMDRSP_RX_FIFO_NEED_READ       (1 << 6)
#define SDCARD_CMDRSP_TX_FIFO_NEED_WRITE      (1 << 7)
#define SDCARD_CMDRSP_RX_OVERFLOW             (1 << 8)
#define SDCARD_CMDRSP_TX_FIFO_EMPTY           (1 << 9)
#define SDCARD_CMDRSP_CMD_CRC_OK              (1 << 10)
#define SDCARD_CMDRSP_CMD_CRC_FAIL            (1 << 11)
#define SDCARD_CMDRSP_DATA_CRC_OK             (1 << 12)
#define SDCARD_CMDRSP_DATA_CRC_FAIL           (1 << 13)
#define SDCARD_CMDRSP_RSP_INDEX               (0x3f<<14)
#define SDCARD_CMDRSP_WR_STATU                (0x7<<20)
#define SDCARD_CMDRSP_DATA_BUSY               (0x1<<23)

#define REG_SDCARD_CMD_RSP_INT_MASK         (SDCARD_BASE_ADDR + 10*4)
#define SDCARD_CMDRSP_NORSP_END_INT_MASK      (1 << 0)
#define SDCARD_CMDRSP_RSP_END_INT_MASK        (1 << 1)
#define SDCARD_CMDRSP_TIMEOUT_INT_MASK        (1 << 2)
#define SDCARD_CMDRSP_DATA_REC_END_INT_MASK   (1 << 3)
#define SDCARD_CMDRSP_DATA_WR_END_INT_MASK    (1 << 4)
#define SDCARD_CMDRSP_DATA_TIME_OUT_INT_MASK  (1 << 5)
#define SDCARD_CMDRSP_RX_FIFO_NEED_READ_MASK  (1 << 6)
#define SDCARD_CMDRSP_TX_FIFO_NEED_WRITE_MASK (1 << 7)
#define SDCARD_CMDRSP_RX_OVERFLOW_MASK        (1 << 8)
#define SDCARD_CMDRSP_TX_FIFO_EMPTY_MASK      (1 << 9)

#define REG_SDCARD_WR_DATA_ADDR             (SDCARD_BASE_ADDR + 11*4)
#define REG_SDCARD_RD_DATA_ADDR             (SDCARD_BASE_ADDR + 12*4)

#define REG_SDCARD_FIFO_THRESHOLD           (SDCARD_BASE_ADDR + 13*4)
#define SDCARD_FIFO_RX_FIFO_THRESHOLD_MASK   (0xff)
#define SDCARD_FIFO_RX_FIFO_THRESHOLD_POSI   (0)
#define SDCARD_FIFO_TX_FIFO_THRESHOLD_MASK   (0xff)
#define SDCARD_FIFO_TX_FIFO_THRESHOLD_POSI   (8)
#define SDCARD_FIFO_RX_FIFO_RST              (1 << 16)
#define SDCARD_FIFO_TX_FIFO_RST              (1 << 17)
#define SDCARD_FIFO_RXFIFO_RD_READY          (1 << 18)
#define SDCARD_FIFO_TXFIFO_WR_READY          (1 << 19)
#define SDCARD_FIFO_SD_STA_RST               (1 << 20)
#define SDCARD_FIFO_SD_RATE_SELECT_POSI      (21)
#define SDCARD_FIFO_SD_RATE_SELECT_MASK      (0x3)
#define SDCARD_FIFO_SD_CLK_REC_SEL			 (1 << 25)


// SDcard defination
/* Exported types ------------------------------------------------------------*/
typedef enum
{
	SD_BUSY =-1,
    SD_OK   =   0,
    SD_CMD_CRC_FAIL               = (1), /*!< Command response received (but CRC check failed) */
    SD_DATA_CRC_FAIL              = (2), /*!< Data bock sent/received (CRC check Failed) */
    SD_CMD_RSP_TIMEOUT            = (3), /*!< Command response timeout */
    SD_DATA_TIMEOUT               = (4), /*!< Data time out */

    SD_INVALID_VOLTRANGE,
    SD_R5_ERROR,            /* A general or an unknown error occurred during the operation */
    SD_R5_ERR_FUNC_NUMB,    /* An invalid function number was requested */
    SD_R5_OUT_OF_RANGE,     /*The command's argument was out of the allowed range for this card*/
    SD_ERROR,
    SD_ERR_LONG_TIME_NO_RESPONS,
    SD_READ_AGAIN,
	SD_DMA_RUNNING = 1000,	
	SD_OUT_RANGE = 1024,	
    SD_ERR_CMD41_CNT = 0xfffe
} SDIO_Error;


#define SD_CMD_NORESP             0
#define SD_CMD_SHORT             (CMD_FLAG_RESPONSE|CMD_FLAG_CRC_CHECK)
#define SD_CMD_LONG              (CMD_FLAG_RESPONSE|CMD_FLAG_LONG_CMD\
                                 |CMD_FLAG_CRC_CHECK)

#define SD_CMD_RSP               (SDCARD_CMDRSP_NORSP_END_INT\
                                 |SDCARD_CMDRSP_RSP_END_INT\
                                 |SDCARD_CMDRSP_TIMEOUT_INT\
                                 |SDCARD_CMDRSP_CMD_CRC_FAIL)

#define SD_DATA_RSP              (SDCARD_CMDRSP_DATA_REC_END_INT\
                                 |SDCARD_CMDRSP_DATA_CRC_FAIL\
                                 |SDCARD_CMDRSP_DATA_WR_END_INT\
                                 |SDCARD_CMDRSP_DATA_TIME_OUT_INT)

#define MSK_SD_CMD_DAT_STUS     (SDCARD_FIFO_SD_STA_RST | SDCARD_FIFO_TX_FIFO_RST | SDCARD_FIFO_RX_FIFO_RST)

#define SD_DATA_DIR_RD           0
#define SD_DATA_DIR_WR           1

#define OCR_MSK_BUSY             0x80000000 // Busy flag
#define OCR_MSK_HC               0x40000000 // High Capacity flag
#define OCR_MSK_VOLTAGE_3_2V_3_3V           0x00100000 // Voltage 3.2V to 3.3V flag
#define OCR_MSK_VOLTAGE_ALL      0x00FF8000 // All Voltage flag

#define SD_DEFAULT_OCR           (OCR_MSK_VOLTAGE_ALL|OCR_MSK_HC)

#define SD_MAX_VOLT_TRIAL        (0xFF)

#define SD_DEFAULT_BLOCK_SIZE    512
#define SDCARD_TX_FIFO_THRD      (0x01) // 16byte
#define SDCARD_RX_FIFO_THRD      (0x01)


#define	CLK_26M                  0//div1
#define	CLK_13M                  1//div2
#define	CLK_6_5M                 2//div4
#define	CLK_203125Hz             3//div128, 26M/128=203.125KHz


#define CMD_FLAG_RESPONSE        0x01
#define CMD_FLAG_LONG_CMD        0x02
#define CMD_FLAG_CRC_CHECK       0x04
#define CMD_FLAG_MASK            0x07

//203.125KHz ~ 4.923us/cycle
#define DEF_LOW_SPEED_CMD_TIMEOUT       (5 * 203125 / 1000) //~5ms
#define DEF_LOW_SPEED_DATA_TIMEOUT      (250 * 203125 / 1000) //~250ms

//26M - about 38.46ns/cycle
#define DEF_HIGH_SPEED_CMD_TIMEOUT      (10 * (26000000 / 1000)) //~5ms 20230802 edit
#define DEF_HIGH_SPEED_DATA_TIMEOUT     (250 * (26000000 / 1000)) //~250ms


#define SDIO_RD_DATA             0
#define SDIO_WR_DATA             1
#define SDIO_RD_AF_WR            3

#define SDIO_DEF_LINE_MODE       4
#define SDIO_DEF_WORK_CLK        13


#define	SD_CLK_PIN_TIMEOUT1				0x1000
#define	SD_CLK_PIN_TIMEOUT2				0x8000
#define SD_CARD_OFFLINE				    0
#define SD_CARD_ONLINE				    1


/* Standard sd  commands (  )           type  argument     response */
#define GO_IDLE_STATE				0   /* bc                          */
#define ALL_SEND_CID				2
#define SEND_RELATIVE_ADDR			3   /* ac   [31:16] RCA        R6  */
#define SET_DSR					4	/*bc	[31:16] DSR,[15:0] stuff bits*/
#define IO_SEND_OP_COND				5   /* ac                      R4  */
#define SWITCH_FUNC					6
#define SELECT_CARD					7   /* ac   [31:16] RCA        R7  */
#define SEND_IF_COND				8   /* adtc                    R1  */
#define SEND_CSD					9
#define STOP_TRANSMISION			12

#define SEND_STATUS					13
#define GO_INACTIVE_STATE			15
#define SET_BLOCKLEN				16
#define READ_SINGLE_BLOCK			17
#define READ_MULTI_BLOCK			18
#define WRITE_BLOCK					24
#define WRITE_SINGLE_BLOCK			24
#define WRITE_MULTI_BLOCK			25
#define PROGRAM_CSD					27
#define SET_WRITE_PROT				28
#define CLR_WRITE_PROT				29
#define SEND_WRITE_PROT				30
#define ERASE_WR_BLK_START			32
#define ERASE_WR_BLK_END			33

#define SD_APP_OP_COND				41
#define LOCK_UNLOCK					42
#define IO_RW_DIRECT				52  /* ac   [31:0] See below   R5  */
#define IO_RW_EXTENDED				53  /* adtc [31:0] See below   R5  */
#define APP_CMD						55
#define GEN_CMD						56
#define R5_COM_CRC_ERROR	      (1 << 15)	/* er, b */
#define R5_ILLEGAL_COMMAND	      (1 << 14)	/* er, b */
#define R5_ERROR		          (1 << 11)	/* erx, c */
#define R5_FUNCTION_NUMBER	      (1 << 9)	/* er, c */
#define R5_OUT_OF_RANGE		      (1 << 8)	/* er, c */
#define R5_STATUS(x)		      (x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	  ((x & 0x3000) >> 12) /* s, b */

/*STM32 register bit define*/
#define SDIO_ICR_MASK             0x5FF
#define SDIO_STATIC_FLAGS         ((uint32_t)0x000005FF)
#define SDIO_FIFO_ADDRESS         ((uint32_t)0x40018080)

#define OCR_MSK_BUSY             0x80000000 // Busy flag
#define OCR_MSK_HC               0x40000000 // High Capacity flag
#define OCR_MSK_VOLTAGE_ALL      0x00FF8000 // All Voltage flag

//#define SD_DEFAULT_OCR           (OCR_MSK_VOLTAGE_ALL|OCR_MSK_HC)

//#define SD_CLK_PIN                              34
#define REG_A2_CONFIG                        ((0x0802800) + 50*4)

/*typedef enum
{
    SD_CARD_IDLE                 = 0,
    SD_CARD_READY                = 1,
    SD_CARD_IDENTIFICATION       = 2,
    SD_CARD_STANDBY              = 3,
    SD_CARD_TRANSFER             = 4,
    SD_CARD_SENDING              = 5,
    SD_CARD_RECEIVING            = 6,
    SD_CARD_PROGRAMMING          = 7,
    SD_CARD_DISCONNECTED         = 8,
    SD_CARD_ERROR                = 0xff
} SDCardState;*/

	enum
	{
		SD_ERR_NONE = 0,
		SD_ERR_CMD_TIMEOUT = 1,
		SD_ERR_CMD_CRC_FAIL = 2,
		SD_ERR_DATA_TIMEOUT = 3,
		SD_ERR_DATA_CRC_FAIL = 4

	};

#define SD_CMD_FLAG_RESPONSE    0x01
#define SD_CMD_FLAG_LONG_CMD    0x02
#define SD_CMD_FLAG_CRC_CHECK   0x04
#define DEFAULT_CMD_TIME_OUT                0x300000
#define DEFAULT_DATA_TIME_OUT               0x300000

typedef struct sdio_command
{
    uint32_t	index;
    uint32_t  arg;
    uint32_t	flags;		    /* expected response type */
    uint32_t  timeout;
    uint32_t	resp[4];
    void    *data;		    /* data segment associated with cmd */
    SDIO_Error	err;		/* command error */
} SDIO_CMD_S, *SDIO_CMD_PTR;

typedef void (*SD_DETECT_FUN)(void);

typedef struct _sdcard_
{
    uint32_t  total_block;
    uint16_t  block_size;
    uint16_t  card_rca;
    uint16_t  init_flag;
	uint16_t	Addr_shift_bit;
    SD_DETECT_FUN detect_func;
}SDCARD_S, *SDCARD_PTR;


SDIO_Error SD_init(void);
SDIO_Error sd_reinit(void);
void sdcard_uninitialize(void);
uint32_t sdcard_get_total_block( void );
void sd_detect_init(void);
uint8_t sd_is_attached(void);
void app_sd_init(void);
void app_sdcard_init(void* cbk_insert_in, void* cbk_pull_out);

void sdcard_get_card_info(SDCARD_S *card_info);
SDIO_Error sdcard_read_multi_block_dma(uint8_t *read_buff, uint32_t first_block, uint32_t block_num,void* cbk);
SDIO_Error sdcard_write_multi_block_dma(uint8_t *write_buff, uint32_t first_block, uint32_t block_num,void* cbk);

int sdcard_read_multi_block(void*buf,int first_block, int block_num);
int sdcard_write_multi_block(void*buf, int first_block, int block_num);

// void sd_detect_fun(void);
int sdcard_write_dma_check_finish(void);
int sdcard_read_dma_check_finish(void);
void sdcard_dma_read_reg_init(uint32_t block_num);
void sdcard_dma_write_reg_init(uint32_t block_num);
void sd_dma_read_int_disable(void);
void sd_dma_write_int_disable(void);


void app_sd_scanning(void);
void sdcard_idle(char enable);
uint32_t sdcard_get_block_size(void);

void clr_sd_noinitial_flag(void);
void app_sd_set_online(int on_off);
void sd_4line_read(void);
int sd_debug(const char *fmt, ...);
void sd_close(void);
int sdio_host_state_get(void);
void sdcard_print_reg(void);
void drv_sd_info_update(int dir);
void sd_test_write(void);


#endif
