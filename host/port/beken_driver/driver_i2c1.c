/*************************************************************
 * @file        driver_i2c1.c
 * @brief       code of HW I2C1 driver of BK3262N
 * @author      GuWenFu
 * @version     V1.2
 * @date        2016-09-29
 * @par         
 * @attention   
 *
 * @history     2016-09-29 gwf    create this file
 * @rebulid     2021-03-29 zhangbing    update to ptn101
 */

#ifdef CONFIG_DRIVER_I2C1
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bkreg.h"
#define NUMBER_ROUND(a,b)        ((a) / (b) + (((a) % (b)) ? 1 : 0))

#define I2C1_DEFAULT_CLK            26000000
#define I2C1_BAUD_10KHZ              10000
#define I2C1_BAUD_100KHZ            100000
#define I2C1_BAUD_200KHZ            200000
#define I2C1_BAUD_390KHZ            390000
#define I2C1_BAUD_400KHZ            400000
#define I2C1_BAUD_553KHZ            553000
#define I2C1_BAUD_609KHZ            609000
#define I2C1_DEFAULT_BAUD          I2C1_BAUD_400KHZ
#define I2C1_CLK_DIVID_SET          (NUMBER_ROUND(NUMBER_ROUND(I2C1_DEFAULT_CLK, I2C1_DEFAULT_BAUD) - 6, 3) - 1)


static volatile i2c1_message        *sg_p_i2c1_msg = NULL;

/*************************************************************
 * i2c1_get_last_msg
 * Description: get i2c1 last message point, so that you can check errno
 *              or deal with received date. if you want to free memory,
 *              call function i2c1_msg_reset()
 * Parameters:  none
 * return:      i2c1_message *: the i2c1 last message point
 * error:       none
 */
i2c1_message * i2c1_get_last_msg(void)
{
    if (sg_p_i2c1_msg != NULL)
    {
        if (sg_p_i2c1_msg->trans_done != 1)
        {
            return NULL;
        }
    }
    return (i2c1_message *)sg_p_i2c1_msg;
}

/*************************************************************
 * i2c1_msg_reset
 * Description: reset i2c1 message point, be caution, you must make sure
 *              the last i2c1 message Tx/Rx has finished
 * Parameters:  none
 * return:      none
 * error:       none
 */
void i2c1_msg_reset(void)
{
    if (sg_p_i2c1_msg != NULL)
    {
        jfree((void *)sg_p_i2c1_msg->pData);
        jfree((void *)sg_p_i2c1_msg);
        sg_p_i2c1_msg = NULL;
    }
}

/*************************************************************
 * i2c1_msg_init
 * Description: initialize i2c1 message, prepare for Tx/Rx.
 *              call function i2c1_send_start() to start Tx/Rx
 * Parameters:  p_i2c1_message: message description
 * return:      OK: message is good
 *              ERROR: message is wrong
 * error:       none
 */
STATUS i2c1_msg_init(i2c1_message *p_set_i2c1_message)
{
    if ((sg_p_i2c1_msg != NULL) && (sg_p_i2c1_msg->trans_done != 1))
    {
        return ERROR;
    }

    if (p_set_i2c1_message == NULL)
    {
        return ERROR;
    }

    if (p_set_i2c1_message->pData == NULL)
    {
        return ERROR;
    }

    if (p_set_i2c1_message->current_data_cnt != 0)
    {
        return ERROR;
    }

    if (p_set_i2c1_message->all_data_cnt == 0)
    {
        return ERROR;
    }

    if (p_set_i2c1_message->addr_flg != 0)
    {
        return ERROR;
    }

    if (p_set_i2c1_message->trans_done != 0)
    {
        return ERROR;
    }

	p_set_i2c1_message->work_mode &= 0x0F;
	p_set_i2c1_message->ack_check &= 0x01;
	p_set_i2c1_message->errno      = 0x00;

    i2c1_msg_reset();
    sg_p_i2c1_msg = p_set_i2c1_message;

    return OK;
}
void i2c1_gpio_init(void)
{
      I2C1_SCL_PIN_REG=0x40;//0x48;gpio16
      I2C1_SDA_PIN_REG=0x40;//0x48;gpio17
      //gpio_config(I2C1_SCL_PIN,2);
      //gpio_config(I2C1_SDA_PIN,2);
      REG_SYSTEM_0x19 = (REG_SYSTEM_0x19 & (~(GPIO2_X_FUNTION_MODE_MASK(I2C1_SCL_PIN) | GPIO2_X_FUNTION_MODE_MASK(I2C1_SDA_PIN))));
      REG_SYSTEM_0x19 |= (GPIO2_X_FUNTION_MODE_1_FUNC(I2C1_SCL_PIN)	//gpio16-17 1st fun enabale
                              	   | GPIO2_X_FUNTION_MODE_1_FUNC(I2C1_SDA_PIN));	
}
/*************************************************************
 * i2c1_init
 * Description: initialize i2c1 interface
 * Parameters:  ulSlaveAddr: set the slave address of i2c1 interface
 * return:      none
 * error:       none
 */
void i2c1_init(unsigned long ulSlaveAddr, unsigned long ulBaudRate)
{
	unsigned long ul_freq_div;
	unsigned long oldmask = get_spr(SPR_VICMR(0));    // read old spr_vicmr

	
	if (ulBaudRate == 0)
	{
		ul_freq_div = I2C1_CLK_DIVID_SET;
	}
	else
	{
		ul_freq_div = NUMBER_ROUND(NUMBER_ROUND(I2C1_DEFAULT_CLK, ulBaudRate) - 6, 3) - 1;
	}
	i2c1_gpio_init();
    	if (REG_I2C1_SMB_CN & MSK_I2C1_SMB_CN_ENSMB)
    	{
	        REG_I2C1_SMB_STAT |= MSK_I2C1_SMB_STAT_STO;
	        REG_I2C1_SMB_CN &= 0x00000000UL;
	        system_peri_clk_disable(SYS_PERI_CLK_I2C1);
    	}	
      system_peri_clk_enable(SYS_PERI_CLK_I2C1);
      system_peri_mcu_irq_enable(SYS_PERI_IRQ_I2C1);
      REG_I2C1_SMB_STAT = 0x00000040;       // 0100 0000; RXINT_MODE = 0x01, slvstop_stre_scl_en = 0x0
      REG_I2C1_SMB_CN |=  (MSK_I2C1_SMB_CN_ENSMB)
		                     | (MSK_I2C1_SMB_CN_INH)
		                     | (MSK_I2C1_SMB_CN_SMBFTE)
		                     | (MSK_I2C1_SMB_CN_SMBTOE)
		                     | (MSK_I2C1_SMB_CN_SMBCS)
		                     | ((ulSlaveAddr & 0x03FFUL) << SFT_I2C1_SMB_CN_SLV_ADDR)
		                     | ((ul_freq_div & 0x03FFUL) << SFT_I2C1_SMB_CN_FREQ_DIV) 
		                     | (0x04UL     << SFT_I2C1_SMB_CN_SCL_CR)   //SCL �͵�ƽ�����ֵ
		                     | (0x03UL     << SFT_I2C1_SMB_CN_IDLE_CR);//SMBus ���е�ƽ��ֵ
    os_printf("===>>i2c1_init finish:%08x\r\n",REG_I2C1_SMB_CN&0xffffffff);
    set_spr(SPR_VICMR(0), oldmask | (1<<VIC_IDX_I2C1));      // set  new spr_vicmr
    i2c1_msg_reset();
}

/*************************************************************
 * i2c1_send_addr
 * Description: send i2c1 slave address, only support in master mode
 * Parameters:  none
 * return:      none
 * error:       none
 */
static void i2c1_send_addr(void)
{
    unsigned char   tx_data;

 	if ((sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_AL_BIT) == 0) // 7bit address
    {
        tx_data = 0;
        //tx_data |= (sg_p_i2c1_msg->send_addr << 1);
        //tx_data &= ~0x01;
		  tx_data = sg_p_i2c1_msg->send_addr ;
        if (sg_p_i2c1_msg->work_mode == 0x01)      // master read,  7bit address, without inner address
        {
            tx_data |= 0x01;
        }
      
        REG_I2C1_SMB_DAT = tx_data;
        sg_p_i2c1_msg->addr_flg ++;
        if (sg_p_i2c1_msg->work_mode == 0x01       // master read,  7bit address, without inner address
         || sg_p_i2c1_msg->work_mode == 0x00)      // master write, 7bit address, without inner address
        {
            sg_p_i2c1_msg->addr_flg |= 0x13;       // all address tx over
        }
    }
    else //10bit address
    {
        tx_data = 0xF0;         // tx the first address byte with a WRITE in RW bit
        tx_data |= (sg_p_i2c1_msg->send_addr >> 7) & 0x06;
        REG_I2C1_SMB_DAT = tx_data;
        sg_p_i2c1_msg->addr_flg ++;
    }
}

/*************************************************************
 * i2c1_send_start
 * Description: start i2c1 Tx/Rx, only support in master mode
 * Parameters:  none
 * return:      none
 * error:       none
 */
void i2c1_send_start(void)
{
    unsigned int cfg_data;
    unsigned int int_mode;

    if (sg_p_i2c1_msg == NULL)
    {
        return;
    }

    if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_MS_BIT)      // slave mode
    {
        return;
    }
 //for set i2c1 interrupt mode
    if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_RW_BIT)      // read mode
    {
        if (sg_p_i2c1_msg->all_data_cnt > 12)
        {
            int_mode = 0x00;
        }
        else if (sg_p_i2c1_msg->all_data_cnt > 8)
        {
            int_mode = 0x01;
        }
        else if (sg_p_i2c1_msg->all_data_cnt > 4)
        {
            int_mode = 0x02;
        }
        else
        {
            int_mode = 0x03;
        }
    }
    else      // write mode
    {
        if (sg_p_i2c1_msg->all_data_cnt < 4 || (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_MS_BIT))
        {
            int_mode = 0x00;
        }
        else
        {
            int_mode = 0x01;
        }
    }

    i2c1_send_addr();  //write address into REG_I2C1_DATA

    cfg_data = REG_I2C1_SMB_STAT & REG_I2C1_SMB_STAT_MASK;
    cfg_data &= ~MSK_I2C1_SMB_STAT_INT_MODE;
    cfg_data |= (int_mode << 6);
    cfg_data |= MSK_I2C1_SMB_STAT_STA;
    REG_I2C1_SMB_STAT = cfg_data;
}

/*************************************************************
 * I2C1_InterruptHandler
 * Description: i2c1 interrupt handler
 * Parameters:  none
 * return:      none
 * error:       none
 */
void i2c1_isr(void)
{
    unsigned long  i2c1_stat,i2c1_config;
    unsigned long   work_mode, ack, sta, sto, si;
    volatile unsigned char  fifo_empty_num=0;
    volatile unsigned char  data_num=0;
    unsigned char   i;
    unsigned char   ucTemp;
    unsigned char   remain_data_cnt;
	
    i2c1_stat = REG_I2C1_SMB_STAT;
    si = i2c1_stat & MSK_I2C1_SMB_STAT_SI;
     if (!si)     // not SMBUS/I2C Interrupt
    {
        if (i2c1_stat & 0x02)        // SCL low level over time
        {
            i2c1_init(I2C1_DEFAULT_SLAVE_ADDRESS, 0);
        }

        if (i2c1_stat & 0x08)        // ARB lost
        {  
            REG_I2C1_SMB_STAT = i2c1_stat & ~0x08;      // clear ARB
        }
        return;
    }

    if (sg_p_i2c1_msg == NULL)
    {
            REG_I2C1_SMB_STAT = (i2c1_stat | 0x0200) & ~0x01; // send stop, clear si
        return;
    }

    i2c1_config = REG_I2C1_SMB_CN;                      // fix bug
    REG_I2C1_SMB_CN = i2c1_config & (~MSK_I2C1_SMB_CN_SMBCS);

    ack = i2c1_stat & 0x0100;
    sto = i2c1_stat & 0x0200;
    sta = i2c1_stat & 0x0400;
    work_mode = sg_p_i2c1_msg->work_mode & 0x03;
    remain_data_cnt = sg_p_i2c1_msg->all_data_cnt - sg_p_i2c1_msg->current_data_cnt;

    switch (work_mode)
    {
        case 0x00:      // master write
        {
            i2c1_stat &= ~0x0400;

            if (sg_p_i2c1_msg->ack_check && !ack)  // slave dont ack
            { 
                i2c1_stat |= 0x0200;       // send stop
                sg_p_i2c1_msg->trans_done = 1;
                sg_p_i2c1_msg->errno = I2C1_ERRNO_WRITE_SLAVE_NO_RESPONSE;

                break;
            }

            ucTemp = sg_p_i2c1_msg->addr_flg;
            if (ucTemp & 0x10)          // all address bytes has been tx, now tx data
            {
                if (remain_data_cnt == 0)   // all data bytes has been tx, now send stop
                {
                    i2c1_stat |= 0x0200;     // send stop
                    sg_p_i2c1_msg->trans_done = 1;

                    break;
                }

                switch (i2c1_stat & 0x00C0)
                {
                    case 0x0000:   fifo_empty_num = 16;  break;
                    case 0x0040:   fifo_empty_num = 12;  break;
                    case 0x0080:   fifo_empty_num = 8;   break;
                    case 0x00C0:   fifo_empty_num = 4;   break;
                    default    :   fifo_empty_num = 0;
                                   sg_p_i2c1_msg->errno = I2C1_ERRNO_2;      // it's impossible
                                   break;
                }

                if (remain_data_cnt < fifo_empty_num)
                {
                    data_num = remain_data_cnt;
                }
                else
                {
                    data_num = fifo_empty_num;
                }

                for (i = 0; i < data_num; i ++)
                {
                    REG_I2C1_SMB_DAT = sg_p_i2c1_msg->pData[sg_p_i2c1_msg->current_data_cnt];
                    sg_p_i2c1_msg->current_data_cnt ++;
                    remain_data_cnt --;
                }

                if (remain_data_cnt < fifo_empty_num)
                {
                    i2c1_stat &= ~0x04C0;        // clear start, set int_mode
                }
                break;
            }

            if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_AL_BIT)      // 10bit address
            {
                if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_IA_BIT)  // with inner address
                {
                    if ((ucTemp & 0x08) == 0)       // inner address need to be tx
                    {
                        if ((ucTemp & 0x03) == 0x00)        // the first address byte should have been tx already
                        {
                            sg_p_i2c1_msg->errno = I2C1_ERRNO_1;
                            break;
                        }
                        else if ((ucTemp & 0x03) == 0x01)   // tx the second address byte
                        {
                            REG_I2C1_SMB_DAT = ((unsigned char)sg_p_i2c1_msg->send_addr & 0x00ff);
                            sg_p_i2c1_msg->addr_flg ++;
                        }
                        else
                        {
                            REG_I2C1_SMB_DAT = sg_p_i2c1_msg->inner_addr;
                            sg_p_i2c1_msg->addr_flg |= 0x1B;
                        }
                    }
                    else                            // inner address has been tx
                    {
                        sg_p_i2c1_msg->addr_flg |= 0x13;   // it's impossible
                    }
                }
                else        // without inner address
                {
                    if ((ucTemp & 0x03) == 0x00)        // the first address byte should have been tx already
                    {
                        sg_p_i2c1_msg->errno = I2C1_ERRNO_1;
                        break;
                    }
                    else if ((ucTemp & 0x03) == 0x01)   // tx the second address byte
                    {
                        REG_I2C1_SMB_DAT = ((unsigned char)sg_p_i2c1_msg->send_addr & 0x00ff);
                        sg_p_i2c1_msg->addr_flg |= 0x13;
                    }
                    else
                    {
                        sg_p_i2c1_msg->addr_flg |= 0x13;  // it's impossible
                    }
                }
            }
            else        // 7bit address
            {
                if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_IA_BIT)  // with inner address
                {
                    if ((ucTemp & 0x08) == 0)       // inner address need to be tx
                    {
                        REG_I2C1_SMB_DAT = sg_p_i2c1_msg->inner_addr;
                        sg_p_i2c1_msg->addr_flg |= 0x13;
                    }
                    else                            // inner address has been tx
                    {
                        sg_p_i2c1_msg->addr_flg |= 0x13;  // it's impossible
                    }
                }
                else        // without inner address
                {

                    sg_p_i2c1_msg->addr_flg |= 0x13;      // it's impossible
                }
            }
            break;
        }

        case 0x01:      // master read
        {
            i2c1_stat &= ~0x0400;
            if (sta && sg_p_i2c1_msg->ack_check && !ack)         // when tx address, we need ACK
            {
                i2c1_stat = i2c1_stat | 0x0200;
                sg_p_i2c1_msg->trans_done = 1;
                sg_p_i2c1_msg->errno = I2C1_ERRNO_READ_SLAVE_NO_RESPONSE;
                break;
            }

            ucTemp = sg_p_i2c1_msg->addr_flg;
            if (ucTemp & 0x10)          // all address has been tx, now rx data
            {
                if (sta)
                {
                    i2c1_stat = i2c1_stat | 0x100;        // send ACK
                    break;
                }

                switch (i2c1_stat & 0x00C0)
                {
                    case 0x0000:   	data_num = 12;  break;
                    case 0x0040:   	data_num = 8;   break;
                    case 0x0080:   	data_num = 4;   break;
                    case 0x00C0:   	data_num = 1;   break;
                    default       :   	data_num = 0;
                                   		sg_p_i2c1_msg->errno = I2C1_ERRNO_2;      // it's impossible
                                   		break;
                }

                for (i = 0; i < data_num; i ++)
                {
                    sg_p_i2c1_msg->pData[sg_p_i2c1_msg->current_data_cnt] = REG_I2C1_SMB_DAT;
                    sg_p_i2c1_msg->current_data_cnt ++;
                    remain_data_cnt --;
                }

                if (remain_data_cnt == 0)
                {
                    i2c1_stat = (i2c1_stat & (~0x0500)) | 0x0200;     // send NACK and STOP
                    sg_p_i2c1_msg->trans_done = 1;
                }
                else if (remain_data_cnt < data_num)
                {
                    i2c1_stat = i2c1_stat | 0x01C0;     // send ACK, set int_mode
                }
                else
                {
                    i2c1_stat = i2c1_stat | 0x0100;     // send ACK
                }
                break;
            }

            if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_AL_BIT)      // 10bit address
            {
                if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_IA_BIT)  // with inner address
                {
                    if ((ucTemp & 0x08) == 0)       // inner address need to be tx
                    {
                        if ((ucTemp & 0x03) == 0x00)        // the first address byte should have been tx already
                        {
                            sg_p_i2c1_msg->errno = I2C1_ERRNO_1;
                            break;
                        }
                        else if ((ucTemp & 0x03) == 0x01)   // tx the second address byte
                        {
                            REG_I2C1_SMB_DAT = ((unsigned char)sg_p_i2c1_msg->send_addr & 0x00ff);
                            sg_p_i2c1_msg->addr_flg ++;
                        }
                        else
                        {
                            REG_I2C1_SMB_DAT = sg_p_i2c1_msg->inner_addr;
                            sg_p_i2c1_msg->addr_flg |= 0x08;
                        }
                    }
                    else                            // inner address has been tx
                    {
                        if ((ucTemp & 0x03) == 0x02)        // tx the first address byte with a READ in RW bit
                        {
                            i2c1_stat |= 0x400;
                            REG_I2C1_SMB_DAT = ((sg_p_i2c1_msg->send_addr >> 7) & 0x06) | 0xF1;
                            sg_p_i2c1_msg->addr_flg |= 0x13;
                        }
                        else
                        {
                            sg_p_i2c1_msg->errno = I2C1_ERRNO_2;      // it's impossible to get here
                            break;
                        }
                    }
                }
                else        // without inner address
                {
                    if ((ucTemp & 0x03) == 0x00)        // the first address byte should have been tx already
                    {
                        	sg_p_i2c1_msg->errno = I2C1_ERRNO_1;
                        	break;
                    }
                    else if ((ucTemp & 0x03) == 0x01)   // tx the second address byte
                    {
                        	REG_I2C1_SMB_DAT = ((unsigned char)sg_p_i2c1_msg->send_addr & 0x00ff);
                        	sg_p_i2c1_msg->addr_flg ++;
                    }
                    else if ((ucTemp & 0x03) == 0x02)   // tx the first address byte with a READ in RW bit
                    {
                        	i2c1_stat |= 0x400;
                        	REG_I2C1_SMB_DAT = ((sg_p_i2c1_msg->send_addr >> 7) & 0x06) | 0xF1;
                        	sg_p_i2c1_msg->addr_flg |= 0x13;
                    }
                    else
                    {
                        	sg_p_i2c1_msg->errno = I2C1_ERRNO_2;      // it's impossible to get here
                        	break;
                    }
                }
            }
            else        // 7bit address
            {
                if (sg_p_i2c1_msg->work_mode & I2C1_MSG_WORK_MODE_IA_BIT)  // with inner address
                {
                    if ((ucTemp & 0x08) == 0)       // inner address need to be tx
                    {
                        	REG_I2C1_SMB_DAT = sg_p_i2c1_msg->inner_addr;
                        	sg_p_i2c1_msg->addr_flg = (sg_p_i2c1_msg->addr_flg & ~0x0B) | 0x0A;
                    }
                    else                            // inner address has been tx
                    {
                        	i2c1_stat |= 0x400;
                        	REG_I2C1_SMB_DAT = (sg_p_i2c1_msg->send_addr<<1) | 0x01;
                        	sg_p_i2c1_msg->addr_flg |= 0x13;
                    }
                }
                else        // without inner address
                {
                    	sg_p_i2c1_msg->addr_flg |= 0x13;      // it's impossible
                }
            }
            break;
        }

        case 0x02:      // slave write
        {
            if (sto)    // detect a STOP
            {
                	sg_p_i2c1_msg->trans_done = 1;
                	break;
            }

            if (i2c1_stat & 0x0800)      // match address byte
            {
                i2c1_stat |= 0x0100;     // send ACK for address byte
            }

            if ((i2c1_stat & 0x2000) == 0)      // read mode
            {
                break;
            }

            if (i2c1_stat & 0x0100)      // detect an ACK
            {
                	REG_I2C1_SMB_DAT = sg_p_i2c1_msg->pData[sg_p_i2c1_msg->current_data_cnt];     // send data
                	sg_p_i2c1_msg->current_data_cnt ++;

                	remain_data_cnt --;
                if (remain_data_cnt == 0)
                {
                    // TODO
                }
            }
            break;
        }

        case 0x03:      // slave read
        {
            if (sto)    // detect a STOP
            {
                	sg_p_i2c1_msg->trans_done = 1;
                	break;
            }

            if (i2c1_stat & 0x0800)      // match address byte
            {
                	i2c1_stat |= 0x0100;     // send ACK
                	break;
            }

            switch (i2c1_stat & 0x00C0)
            {
                case 0x0000:   	data_num = 12;  break;
                case 0x0040:   	data_num = 8;   break;
                case 0x0080:   	data_num = 4;   break;
                case 0x00C0:   	data_num = 1;   break;
                default       :   	data_num = 0;
                               		sg_p_i2c1_msg->errno = I2C1_ERRNO_2;      // it's impossible
                               		break;
            }

            for (i = 0; i < data_num; i ++)
            {
               	sg_p_i2c1_msg->pData[sg_p_i2c1_msg->current_data_cnt] = REG_I2C1_SMB_DAT;
                	sg_p_i2c1_msg->current_data_cnt ++;
                	remain_data_cnt --;
            }

            if (remain_data_cnt == 0)
            {
                	i2c1_stat &= ~0x0100;     // send NACK
                	sg_p_i2c1_msg->trans_done = 1;
            }
            else if (remain_data_cnt < data_num)
            {
               	i2c1_stat = i2c1_stat | 0x01C0;     // send ACK, set int_mode
            }
            else
            {
                	i2c1_stat |= 0x0100;     // send ACK
            }
            break;
        }

		default:		// by gwf
            break;
    }

    REG_I2C1_SMB_STAT = (i2c1_stat & (~0x01));  //clear si
    delay_us(1);
    REG_I2C1_SMB_CN = i2c1_config;            // fix bug
}

/*************************************************************
 * is_i2c1_busy
 * Description: ask if i2c1 interface is busy
 * Parameters:  none
 * return:      YES: i2c1 interface is busy
 *              NO:  i2c1 interface is free
 * error:       none
 */
ASK is_i2c1_busy(void)
{
    if (REG_I2C1_SMB_STAT & MSK_I2C1_SMB_STAT_BUSY)
    {
        return YES;
    }

    if ((sg_p_i2c1_msg != NULL) && (sg_p_i2c1_msg->trans_done == 0))//Transfer don't finish
    {
        return YES;
    }

    return NO;
}

STATUS I2C1_Write_Buffer(uint8 RegAddr, uint8 *this_buffer, int32 byte_count)
{
	i2c1_message *p_my_i2c1_msg;
	os_printf("I2C1_Write_Buffer RegAddr(%02x),byte_count(%d)\r\n",RegAddr,byte_count);
	p_my_i2c1_msg = (i2c1_message *) jmalloc(sizeof(i2c1_message),M_ZERO);
	if (p_my_i2c1_msg == NULL)
	{
		return ERROR;
	}
	memset(p_my_i2c1_msg, 0, sizeof(i2c1_message));

	p_my_i2c1_msg->work_mode =   (0 	| (I2C1_MSG_WORK_MODE_IA_BIT)) // with inner addr
									& (~I2C1_MSG_WORK_MODE_RW_BIT)  // write
                            					& (~I2C1_MSG_WORK_MODE_MS_BIT)  // master
                            					& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	p_my_i2c1_msg->send_addr = I2C1_DEFAULT_SLAVE_ADDRESS; // Destination slave address
	p_my_i2c1_msg->inner_addr = RegAddr;
	p_my_i2c1_msg->addr_flg = 0;
	p_my_i2c1_msg->trans_done = 0;
	p_my_i2c1_msg->ack_check = 1;//ask
	p_my_i2c1_msg->current_data_cnt = 0;
	p_my_i2c1_msg->all_data_cnt = byte_count;
	p_my_i2c1_msg->errno = 0;

	p_my_i2c1_msg->pData = (unsigned char *) jmalloc(p_my_i2c1_msg->all_data_cnt,M_ZERO);
	if (p_my_i2c1_msg->pData == NULL)
	{
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	memset(p_my_i2c1_msg->pData, 0, p_my_i2c1_msg->all_data_cnt);
	memcpy(p_my_i2c1_msg->pData, this_buffer,  p_my_i2c1_msg->all_data_cnt);

	if (i2c1_msg_init(p_my_i2c1_msg) != OK)
	{
		jfree((void *)p_my_i2c1_msg->pData);
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	i2c1_send_start();
	while (is_i2c1_busy() == YES) // wait until i2c1 free
	{
	  
	}
	i2c1_msg_reset();

	return OK;
}

STATUS I2C1_Read_Buffer(uint8 RegAddr, uint8 *this_buffer, int32 byte_count)
{
	i2c1_message *p_my_i2c1_msg;

	p_my_i2c1_msg = (i2c1_message *) jmalloc(sizeof(i2c1_message),M_ZERO);
	if (p_my_i2c1_msg == NULL)
	{
		return ERROR;
	}
	memset(p_my_i2c1_msg, 0, sizeof(i2c1_message));

	p_my_i2c1_msg->work_mode =   (0 	| (I2C1_MSG_WORK_MODE_RW_BIT) | (I2C1_MSG_WORK_MODE_IA_BIT)) // read, with inner addr
                            					& (~I2C1_MSG_WORK_MODE_MS_BIT) // master
                            					& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	p_my_i2c1_msg->send_addr = I2C1_DEFAULT_SLAVE_ADDRESS; // Destination slave address
	p_my_i2c1_msg->inner_addr = RegAddr;
	p_my_i2c1_msg->addr_flg = 0;
	p_my_i2c1_msg->trans_done = 0;
	p_my_i2c1_msg->ack_check = 1;
	p_my_i2c1_msg->current_data_cnt = 0;
	p_my_i2c1_msg->all_data_cnt = byte_count;
	p_my_i2c1_msg->errno = 0;

 	p_my_i2c1_msg->pData = (unsigned char *) jmalloc(p_my_i2c1_msg->all_data_cnt,M_ZERO);
	if (p_my_i2c1_msg->pData == NULL)
	{
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	memset(p_my_i2c1_msg->pData, 0, p_my_i2c1_msg->all_data_cnt);

	if (i2c1_msg_init(p_my_i2c1_msg) != OK) 
	{
		jfree((void *)p_my_i2c1_msg->pData);
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	i2c1_send_start();

	while (is_i2c1_busy() == YES) // wait until i2c1 free
	{
	}
	memcpy(this_buffer, p_my_i2c1_msg->pData, p_my_i2c1_msg->all_data_cnt);

	i2c1_msg_reset();

	return OK;
}

static i2c1_message my_i2c1_msg;

STATUS I2C1_prepareReadBurst(void)
{
	memset(&my_i2c1_msg, 0, sizeof(i2c1_message));
	my_i2c1_msg.work_mode =   (0 | (I2C1_MSG_WORK_MODE_RW_BIT)| (I2C1_MSG_WORK_MODE_IA_BIT)) // read, with inner addr
                            				& (~I2C1_MSG_WORK_MODE_MS_BIT) // master
                            				& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	my_i2c1_msg.send_addr = I2C1_DEFAULT_SLAVE_ADDRESS; // Destination slave address
	my_i2c1_msg.ack_check = 1;
  return OK;
}

STATUS I2C1_postReadBurst(void)
{
	sg_p_i2c1_msg = NULL;
	return OK;
}

STATUS I2C1_startReadBurst(uint8 RegAddr, uint8 *this_buffer, int32 byte_count)
{
	my_i2c1_msg.inner_addr = RegAddr;
	my_i2c1_msg.addr_flg = 0;
	my_i2c1_msg.trans_done = 0;
	my_i2c1_msg.current_data_cnt = 0;
	my_i2c1_msg.all_data_cnt = byte_count;
	my_i2c1_msg.errno = 0;

 	my_i2c1_msg.pData = this_buffer;

 	if(sg_p_i2c1_msg == NULL)
 	{
 		sg_p_i2c1_msg = &my_i2c1_msg;
 		i2c1_send_start();
 		return OK;
 	}
 	else
 	{
 		return ERROR;
 	}
}
STATUS I2C1_WriteByte_without_inner(uint8 this_byte)
{
	i2c1_message *p_my_i2c1_msg;

	p_my_i2c1_msg = (i2c1_message *) jmalloc(sizeof(i2c1_message),M_ZERO);
	if (p_my_i2c1_msg == NULL)
	{
		return ERROR;
	}
	memset(p_my_i2c1_msg, 0, sizeof(i2c1_message));

	p_my_i2c1_msg->work_mode =   (0 	& (~I2C1_MSG_WORK_MODE_IA_BIT)) // without inner addr
									& (~I2C1_MSG_WORK_MODE_RW_BIT)  // write
                            					& (~I2C1_MSG_WORK_MODE_MS_BIT)  // master
                            					& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	p_my_i2c1_msg->send_addr = I2C1_DEFAULT_SLAVE_ADDRESS; // Destination slave address
	p_my_i2c1_msg->inner_addr = 0;
	p_my_i2c1_msg->addr_flg = 0;
	p_my_i2c1_msg->trans_done = 0;
	p_my_i2c1_msg->ack_check = 1;
	p_my_i2c1_msg->current_data_cnt = 0;//0
	p_my_i2c1_msg->all_data_cnt = 1;
	p_my_i2c1_msg->errno = 0;

	p_my_i2c1_msg->pData = (unsigned char *) jmalloc(p_my_i2c1_msg->all_data_cnt,M_ZERO);
	if (p_my_i2c1_msg->pData == NULL)
	{
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	memset(p_my_i2c1_msg->pData, 0, p_my_i2c1_msg->all_data_cnt);
	p_my_i2c1_msg->pData[0] = this_byte;

	if (i2c1_msg_init(p_my_i2c1_msg) != OK)
	{
		jfree((void *)p_my_i2c1_msg->pData);
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	i2c1_send_start();
	//while (is_i2c1_busy() == YES) // wait until i2c1 free
	//{
	//}
	if(p_my_i2c1_msg->errno == 0)
	{
		i2c1_msg_reset();
		return OK;
	}
	else
	{
		//debug_printf("I2C error0: 0x%04X\r\n", p_my_i2c1_msg->errno);
		i2c1_msg_reset();
		return ERROR;
	}
}
STATUS I2C1_WriteByte(uint8 send_addr,uint8 RegAddr, ask_t ask,uint8 this_byte)
{
	i2c1_message *p_my_i2c1_msg;

	p_my_i2c1_msg = (i2c1_message *) jmalloc(sizeof(i2c1_message),M_ZERO);
	if (p_my_i2c1_msg == NULL)
	{
		return ERROR;
	}
	memset(p_my_i2c1_msg, 0, sizeof(i2c1_message));

	p_my_i2c1_msg->work_mode =   (0 	| (I2C1_MSG_WORK_MODE_IA_BIT)) // with inner addr
									& (~I2C1_MSG_WORK_MODE_RW_BIT)  // write
                            					& (~I2C1_MSG_WORK_MODE_MS_BIT)  // master
                            					& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	p_my_i2c1_msg->send_addr = (uint16)send_addr; // Destination slave address
	p_my_i2c1_msg->inner_addr = (uint16)RegAddr;
	p_my_i2c1_msg->addr_flg = 0;
	p_my_i2c1_msg->trans_done = 0;
	p_my_i2c1_msg->ack_check = (uint8)ask;
	p_my_i2c1_msg->current_data_cnt = 0;//0
	p_my_i2c1_msg->all_data_cnt = 1;
	p_my_i2c1_msg->errno = 0;

	p_my_i2c1_msg->pData = (unsigned char *) jmalloc(p_my_i2c1_msg->all_data_cnt,M_ZERO);
	if (p_my_i2c1_msg->pData == NULL)
	{
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	memset(p_my_i2c1_msg->pData, 0, p_my_i2c1_msg->all_data_cnt);
	p_my_i2c1_msg->pData[0] = this_byte;

	if (i2c1_msg_init(p_my_i2c1_msg) != OK)
	{
		jfree((void *)p_my_i2c1_msg->pData);
		jfree((void *)p_my_i2c1_msg);
		return ERROR;
	}
	i2c1_send_start();
	while (is_i2c1_busy() == YES) // wait until i2c1 free
	{
	}
	if(p_my_i2c1_msg->errno == 0)
	{
		//os_printf("I2C send ok:%02x\r\n",p_my_i2c1_msg->pData[0]);
		i2c1_msg_reset();
		return OK;
	}
	else
	{
		os_printf("I2C error0: 0x%04X\r\n", p_my_i2c1_msg->errno);
		i2c1_msg_reset();
		return ERROR;
	}
}

uint8 I2C1_ReadByte(uint8 RegAddr)
{
	uint8 this_data;
	i2c1_message *p_my_i2c1_msg;

	p_my_i2c1_msg = (i2c1_message *) jmalloc(sizeof(i2c1_message),M_ZERO);
	if (p_my_i2c1_msg == NULL)
	{
		return 0;
	}
	memset(p_my_i2c1_msg, 0, sizeof(i2c1_message));

	p_my_i2c1_msg->work_mode =   (0	| (I2C1_MSG_WORK_MODE_RW_BIT)
									| (I2C1_MSG_WORK_MODE_IA_BIT)) // read, with inner addr
                            					& (~I2C1_MSG_WORK_MODE_MS_BIT) // master
                            					& (~I2C1_MSG_WORK_MODE_AL_BIT); // 7bit address
	p_my_i2c1_msg->send_addr = I2C1_DEFAULT_SLAVE_ADDRESS; // Destination slave address
	p_my_i2c1_msg->inner_addr = RegAddr;
	p_my_i2c1_msg->addr_flg = 0;
	p_my_i2c1_msg->trans_done = 0;
	p_my_i2c1_msg->ack_check = 1;
	p_my_i2c1_msg->current_data_cnt = 0;
	p_my_i2c1_msg->all_data_cnt = 1;
	p_my_i2c1_msg->errno = 0;

 	p_my_i2c1_msg->pData = (unsigned char *) jmalloc(p_my_i2c1_msg->all_data_cnt,M_ZERO);
	if (p_my_i2c1_msg->pData == NULL)
	{
		jfree((void *)p_my_i2c1_msg);
		return 0;
	}
	memset(p_my_i2c1_msg->pData, 0, p_my_i2c1_msg->all_data_cnt);

	if (i2c1_msg_init(p_my_i2c1_msg) != OK) 
	{
		jfree((void *)p_my_i2c1_msg->pData);
		jfree((void *)p_my_i2c1_msg);
		return 0;
	}
	i2c1_send_start();

	while (is_i2c1_busy() == YES) // wait until i2c1 free
	{
	}

	if(p_my_i2c1_msg->errno == 0)
	{
		this_data = p_my_i2c1_msg->pData[0];
	}
	else
	{
		this_data = 0;
		//debug_printf("I2C error1: 0x%04X\r\n", p_my_i2c1_msg->errno);
	}

	i2c1_msg_reset();

	return this_data;
}
#endif

