/*************************************************************
 * @file        driver_i2c1.h
 * @brief       Header file of driver_i2c1.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par         
 * @attention   
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_I2C1_H__
#define __DRIVER_I2C1_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
typedef enum        // by gwf
{
    OK = 0,
    ERROR = -1
} STATUS;

typedef enum        // by gwf
{
    NO = 0,
    YES = 1
} ASK;
typedef enum        // by zb
{
    NOASK = 0,
    NEEDASK = 1
} ask_t;
#define I2C1_SCL_PIN                 16
#define I2C1_SDA_PIN                 17
#define I2C1_SCL_PIN_REG       REG_GPIO_0x10
#define I2C1_SDA_PIN_REG      REG_GPIO_0x11
#define I2C1_DEFAULT_SLAVE_ADDRESS      (0x21)      //7 bits

#define I2C1_MSG_WORK_MODE_RW_BIT        (1<<0)      /* 0:write,  1:read */
#define I2C1_MSG_WORK_MODE_MS_BIT        (1<<1)      /* 0:master, 1:slave */
#define I2C1_MSG_WORK_MODE_AL_BIT        (1<<2)      /* 0:7bit address, 1:10bit address */
#define I2C1_MSG_WORK_MODE_IA_BIT        (1<<3)      /* 0:without inner address, 1: with inner address */

#define I2C1_ERRNO_1             0x0001
#define I2C1_ERRNO_2             0x0002              /* logically impossible position */
#define I2C1_ERRNO_WRITE_SLAVE_NO_RESPONSE	0x0010	 /* master write : slave no ack */
#define I2C1_ERRNO_READ_SLAVE_NO_RESPONSE	0x0011	 /* master read  : slave no ack */

#define REG_I2C1_SMB_STAT_MASK            0x0000FFFB

//----------------------------------------------
// I2C1 data typedef 
//----------------------------------------------
typedef  struct
{
    unsigned char     * pData;              // data buffer
	unsigned short      send_addr;			// send address value, only for Master
	unsigned short      slave_mode_addr;    // slave mode address, only for Slaver
    unsigned char       current_data_cnt;   // current data count
	unsigned char		all_data_cnt;       // TX or RX Data Bytes
    unsigned char		inner_addr;         // inner address, only for master write/read
	unsigned char		work_mode;			// work mode
                                            // RW(bit 0):  0:write,  1:read
                                            // MS(bit 1):  0:master, 1:slave
                                            // AL(bit 2):  0:7bit address, 1:10bit address
                                            // IA(bit 3):  0:without inner address, 1: with inner address
                                            // reserved(bit [4:7]):  reserved
	unsigned char		addr_flg;
/* addr_flg: Address Byte Flag for Master
 * bit[1:0]: 10bit/7bit device address flag
 *          00: first byte need to be tx
 *          01: first byte has been tx , second byte need to be tx (not supply in 7bit mode)
 *          10: second byte has been tx, after restart, first byte need to be tx again
 *          11: after restart, first byte has been tx again
 * bit[3]:   inner address flag
 *          0:  inner address need to be tx
 *          1:  inner address has been tx
 * bit[4]:   all address (device address and inner address) tx over flag
 *          0:  all address tx not over
 *          1:  all address has been tx, data tx begin
 * bit[7:5]: reserve
 */

	unsigned char		trans_done;			// I2C1 Transfer Finish Flag:  0:I2C1 Transfer havn't finished
                                            //                            1:I2C1 Transfer has finished
	unsigned char		ack_check;			// 0: Don't Care ACK, 1: Care ACK
   	unsigned char       errno;              // error number
}  i2c1_message;

uint8 I2C1_ReadByte(uint8 RegAddr);
STATUS I2C1_WriteByte_without_inner(uint8 this_byte);
STATUS I2C1_WriteByte(uint8 send_addr,uint8 RegAddr, ask_t ask,uint8 this_byte);
STATUS I2C1_Write_Buffer(uint8 RegAddr, uint8 *this_buffer, int32 byte_count);
STATUS I2C1_Read_Buffer(uint8 RegAddr, uint8 *this_buffer, int32 byte_count);
STATUS I2C1_prepareReadBurst(void);
STATUS I2C1_postReadBurst(void);
STATUS I2C1_startReadBurst(uint8 RegAddr, uint8 *this_buffer, int32 byte_count);

/*************************************************************
 * i2c1_init
 * Description: initialize i2c1 interface
 * Parameters:  ulSlaveAddr: set the slave address of i2c1 interface
 * 				ulBaudRate : set the baud rate of i2c1 interface
 * return:      none
 * error:       none
 */
void i2c1_init(unsigned long ulSlaveAddr, unsigned long ulBaudRate);

/*************************************************************
 * i2c1_msg_init
 * Description: initialize i2c1 message, prepare for Tx/Rx.
 *              call function i2c1_send_start() to start Tx/Rx
 * Parameters:  p_i2c1_message: message description
 * return:      OK: message is good
 *              ERROR: message is wrong
 * error:       none
 */
STATUS i2c1_msg_init(i2c1_message *p_i2c1_message);

/*************************************************************
 * i2c1_send_start
 * Description: start i2c1 Tx/Rx, only support in master mode
 * Parameters:  none
 * return:      none
 * error:       none
 */
void i2c1_send_start(void);

/*************************************************************
 * i2c1_get_last_msg
 * Description: get i2c1 last message point, so that you can check errno
 *              or deal with received date. if you want to free memory, 
 *              call function i2c1_msg_reset()
 * Parameters:  none
 * return:      i2c1_message *: the i2c1 last message point
 * error:       none
 */
i2c1_message * i2c1_get_last_msg(void);

/*************************************************************
 * i2c1_msg_reset
 * Description: reset i2c1 message point, be caution, you must make sure
 *              the last i2c1 message Tx/Rx has finished
 * Parameters:  none
 * return:      none
 * error:       none
 */
void i2c1_msg_reset(void);

/*************************************************************
 * is_i2c1_busy
 * Description: ask if i2c1 interface is busy
 * Parameters:  none
 * return:      YES: i2c1 interface is busy
 *              NO:  i2c1 interface is free
 * error:       none
 */
ASK is_i2c1_busy(void);

/*************************************************************
 * I2C1_InterruptHandler
 * Description: i2c1 interrupt handler
 * Parameters:  channel number
 * return:      none
 * error:       none
 */
void i2c1_isr(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_I2C1_H__ */
