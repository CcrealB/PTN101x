#include <string.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

#if (BT_DUALMODE_RW == 1)
extern void rw_ext_wakeup(void);
#endif
#if 1
#define FLS_GLOBAL_INT_DISABLE(); \
  do{ \
        uint32_t cpu_flags; \
        SYSirq_Disable_Interrupts_Except(&cpu_flags, INTR_PRI_CLS_DEF/*(1 << VIC_IDX_CEVA)*/);
#define FLS_GLOBAL_INT_RESTORE(); \
        SYSirq_Enable_All_Interrupts(cpu_flags); \
    }while(0);
#elif 1
#define FLS_GLOBAL_INT_DISABLE(); \
  do{ \
        uint32_t cpu_flags, mask;\
        SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
#define FLS_GLOBAL_INT_RESTORE(); \
        SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);\
    }while(0);
#else
#define USB_GLOBAL_INT_DISABLE();
#define USB_GLOBAL_INT_RESTORE();
#endif

static uint32_t flash_mid = 0;
static uint32_t flash_crc_idx = 0;

uint32_t FLASH_ENVDATA_DEF_ADDR;
/* Flash Protect & QE Setting 

1.mID:856014/P25Q80H,PuYa 8M
Status Reg: SUS1 CMP LB3 LB2 LB1 SUS2 QE SRP1    SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
             0    1   0   0   0   0    1    0     0   0   0   0   0   0   0   0   Protect:all
             0    1   0   0   0   0    1    0     0   1   0   0   1   1   0   0   Protect:000000H-0FBFFFH
             0    0   0   0   0   0    1    0     0   1   1   0   1   0   0   0   Protect:000000H-001FFFH

2.mID:856016/P25Q32H,PuYa 32M
Status Reg: SUS1 CMP LB3 LB2 LB1 SUS2 QE SRP1    SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
             0    1   0   0   0   0    1    0     0   0   0   0   0   0   0   0   Protect:all
             0    1   0   0   0   0    1    0     0   0   0   1   1   1   0   0   Protect:none
             0    1   0   0   0   0    1    0     0   1   0   0   1   1   0   0   Protect:000000H-3FBFFFH

3.mID:c86515/GD25WQ16E GD 16M
Status Reg: SUS1 CMP Reserve DC LB1 LB0 QE SRP1  SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
             0    0     0    0   0   0   1  0     0    0   0   1   1   1   0   0  Protect:all
             0    0     0    0   0   0   1  0     0    0   1   1   0   0   0   0  Protect:000000H-07FFFFH  

4.mID:c86516/GD25WQ32E GD 32M
Status Reg: SUS1 CMP Reserve DC LB1 LB0 QE SRP1  SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
             0    0     0    0   0   0   1  0     0    0   0   1   1   1   0   0  Protect:all
             0    0     0    0   0   0   1  0     0    0   1   1   0   0   0   0  Protect:000000H-07FFFFH   
5.mID:c84015/GD25Q16B GD 16M
 Status Reg: SUS1 CMP Reserve DC LB1 LB0 QE SRP1  SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
			  0    0	 0	  0   0   0   1  0	   0	x	x	1	1	x	0	0  Protect:all
			  0    0	 0	  0   0   0   1  0	   0	0	1	1	0	1	0	0  Protect:000000H-0FFFFFH	 
6..........................                                 
*/

uint8_t flash_get_4M_flag(void)
{
    unsigned int flash_flag;

    switch (flash_mid)
    {
        case 0x1C3113: //xtx 4M flash 
        case 0x514013: //MD25D40 4M flash
        case 0x856013: //P25Q40 4M flash
            flash_flag = 1;
            break;
            
        default:
            flash_flag = 0;
            break;
    }
    return flash_flag;
}

uint8_t flash_get_16M_flag(void)
{
    unsigned int flash_flag;

    switch (flash_mid)
    {
        case 0x0b4015:
        case 0xc84015:
            flash_flag = 1;
            break;

        default:
            flash_flag = 0;
            break;
    }
    return flash_flag;
}

static void set_flash_config_start(uint16_t op)
{
    uint32_t	temp0 = REG_FLASH_0x00;
    
	REG_FLASH_0x00 = (  (temp0             &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (op                << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (0x1               << SFT_FLASH_0x00_WP_VALUE)); // make WP equal 1 not protect SRP
    while(REG_FLASH_0x00 & 0x80000000);
}

void set_flash_crc_en(void)
{
    unsigned int temp0;
    
    temp0 = REG_FLASH_0x07;
    REG_FLASH_0x07 = (  (temp0 &  MSK_FLASH_0x07_CLK_CONF)
                      | (temp0 &  MSK_FLASH_0x07_MODE_SEL)  //QWFR��������MODE use QWFR to fetch data  //for BG flash
                      | (temp0 &  MSK_FLASH_0x07_FWREN_FLASH_CPU)
                      | (temp0 &  MSK_FLASH_0x07_WRSR_DATA)
                      | (0x1   << SFT_FLASH_0x07_CRC_EN));
    
    while(REG_FLASH_0x00 & 0x80000000);
}

void set_flash_clk(unsigned char clk_conf) 
{
    unsigned int temp0;
    
    temp0 = REG_FLASH_0x07;
    REG_FLASH_0x07 = (  (clk_conf &  MSK_FLASH_0x07_CLK_CONF)
                      | (temp0 &  MSK_FLASH_0x07_MODE_SEL)  //QWFR��������MODE use QWFR to fetch data  //for BG flash
                      | (temp0 &  MSK_FLASH_0x07_FWREN_FLASH_CPU)
                      | (temp0 &  MSK_FLASH_0x07_WRSR_DATA)
                      | (temp0 &  MSK_FLASH_0x07_CRC_EN));

	/* Flash read data operation after setting clock */
    while(REG_FLASH_0x00 & 0x80000000);

    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (0                 << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_READ << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    
    while(REG_FLASH_0x00 & 0x80000000);

    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
}

void set_flash_qe(void) 
{
    unsigned int temp0;
    uint32_t flash_sts_reg;
    
    while(REG_FLASH_0x00 & 0x80000000);
    //WRSR QE=1
    temp0 = REG_FLASH_0x07; //WRSR Status data
    flash_sts_reg = (temp0 >> SFT_FLASH_0x07_WRSR_DATA) & 0xffff;
    flash_sts_reg |= (1<<9);// QE enable;
    
    if(flash_mid == 0xC22314)   ////MXIC
    {
         REG_FLASH_0x05 = (0xa5 << 22);            
    }
    REG_FLASH_0x07 = (  (temp0 &  MSK_FLASH_0x07_CLK_CONF)
                        | (temp0 &  MSK_FLASH_0x07_MODE_SEL)
                        | (temp0 &  MSK_FLASH_0x07_FWREN_FLASH_CPU)
                        | (flash_sts_reg << SFT_FLASH_0x07_WRSR_DATA) // SET QE=1, set flash protect all
                        | (temp0 &  MSK_FLASH_0x07_CRC_EN));
    //Start WRSR
    set_flash_config_start(FLASH_OPCODE_WRSR2);
}

void set_flash_protect(uint8_t all) 
{
    unsigned int temp0;
    uint16_t wrsr_data = 0;
	uint8_t bit_QE = 0;
#if(DEFAULT_LINE_MODE == FLASH_LINE_4)
	bit_QE = 1;
#endif

    while(REG_FLASH_0x00 & 0x80000000);

    temp0 = REG_FLASH_0x07; //����WRSR Status data
    temp0 &= 0xfefe0fff;    //set cmp = 0 , set [BP4:BP0] = 0
    
    switch(flash_mid)
    {
        case 0xC22014://ZETTA
            if(all == 1)
            {
                wrsr_data = 0x003C;
            }
            else
            {
                wrsr_data = 0x0028;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0xC22314: //MXIC
            if(READ_CHIPID == 0x326283)  //chipid = '3262S'
            {
                REG_FLASH_0x05 = (0xa5 << 22);            
                if(all == 1)
                {
                    wrsr_data = (bit_QE << 6) | 0x003C;
                }
                else
                {
                    wrsr_data = (bit_QE << 6) | 0x0000;
                }
            }    
            else
            {
                if(all == 1)
                {
                    wrsr_data = 0x003C;
                }
                else
                {
                    wrsr_data = 0x0000;
                }
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0x5e4014: // XTX
            if(all == 1)
            {
                wrsr_data = 0x001C;
            }
        	else
            {
                wrsr_data = 0x0000;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0x0b4015:
        case 0xc84015: //GD 16M
            if(all == 1)
            {
                wrsr_data = (bit_QE << 9) | 0x001C;
            }
        	else
            {
                wrsr_data = (bit_QE << 9) | 0x0032;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR2);
            break;
            
        case 0x514014:  // GD/MD
        case 0xc86515:
        case 0xc86516:
            if(all == 1)
            {
                wrsr_data = (bit_QE << 9) | 0x001C;
            }
        	else
            {
                wrsr_data = (bit_QE << 9) | 0x0030;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR2);
            break;
            
        case 0x1C3113:  // xtx
            if(all == 1)
            {
                wrsr_data = 0x003C;
            }
        	else
            {
                wrsr_data = 0x0034;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0x514013: // xtx
            if(all == 1)
            {
                wrsr_data = 0x003C;
            }
        	else
            {
                wrsr_data = 0x0008;
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
            
        case 0xC84014:  // GD 8M   (GD25Q80CSIG)
        case 0x856013:  // PUYA 4M  (P25Q40H)
        case 0x856014:  // PUYA 8M  (P25Q80H)
            if(all == 1)
            {
                wrsr_data = (bit_QE << 9) | 0x4000;       // protect ALL
            }
        	else
            {
                //wrsr_data = (bit_QE << 9) | 0x404C;       // protect 0H~FBFFFH/0H~1FBFFFH
                wrsr_data = (bit_QE << 9) | 0x4050;         // protect 0H~F7FFFH/0H~1F7FFFH
                
                #if (CONFIG_DRIVER_OTA == 1)
                if(driver_ota_is_ongoing())
                    wrsr_data = (bit_QE << 9) | 0x0068;   // protect 0H~1FFFH
                #endif
            }   
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR2);
            break;
        case 0x856015:  // PUYA 16M (P25Q16SH)    
        case 0x854215:  // PUYA 16M (P25Q16SH)
            //CMP bit = 1, QE bit = 1, set by Toolkit_V6.0.0 always, WRSR only
            if(all == 1)
            {
                wrsr_data = (bit_QE << 9) | 0x4000;      // protect ALL
            }
        	else
            {
                //wrsr_data = (bit_QE << 9) | 0x404C;       // protect 0H~1FBFFFH
                wrsr_data = (bit_QE << 9) | 0x4050;         // protect 0H~F7FFFH/0H~1F7FFFH
                
                #if (CONFIG_DRIVER_OTA == 1) 
                if(driver_ota_is_ongoing())
                    wrsr_data = (bit_QE << 9) | 0x4018;   // protect NONE
                #endif
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break; 
            
        case 0x856016:  // PUYA 32M (P25Q32H)
            //CMP bit = 1, QE bit = 1, set by Toolkit_V6.0.0 always, WRSR only
            if(all == 1)
            {
                wrsr_data = (bit_QE << 9) | 0x4000;      // protect ALL
            }
        	else
            {
                wrsr_data = (bit_QE << 9) | 0x404C;       // protect 0H~3FBFFFH
                           
                #if (CONFIG_DRIVER_OTA == 1)
                if(driver_ota_is_ongoing())
                    wrsr_data = (bit_QE << 9) | 0x401C;   // protect NONE
                #endif
            }
            REG_FLASH_0x07 = temp0 | ((wrsr_data & MAX_FLASH_0x07_WRSR_DATA) << SFT_FLASH_0x07_WRSR_DATA);
            set_flash_config_start(FLASH_OPCODE_WRSR);
            break;
        
        default:  
            LOG_I(DRV, "FLASH mID: 0x%x, not find!!!\r\n", flash_mid);
            break;
    }
    while(REG_FLASH_0x00 & 0x80000000);
}


#if(DEFAULT_LINE_MODE == FLASH_LINE_4)
static void set_flash_qwfr(void)
{
    unsigned int temp0;
    //QWFR
    temp0 = REG_FLASH_0x07;
    REG_FLASH_0x07 = (  (temp0 &  MSK_FLASH_0x07_CLK_CONF)
                      | (0x2   << SFT_FLASH_0x07_MODE_SEL)  //QWFR��������MODE use QWFR to fetch data  //for BG flash
                      | (temp0 &  MSK_FLASH_0x07_FWREN_FLASH_CPU)
                      | (temp0 &  MSK_FLASH_0x07_WRSR_DATA)
                      | (temp0 &  MSK_FLASH_0x07_CRC_EN));

    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (0                 << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_CRMR << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);

}
#endif

void clr_flash_qwfr(void) 
{
    unsigned int temp0;
    //CRMR �ر�continuous���� �����mode_sel�еĵ���λģʽѡ��
    temp0 = REG_FLASH_0x07;
    REG_FLASH_0x07 = (  (temp0 &  MSK_FLASH_0x07_CLK_CONF)
                      | (0x1   << SFT_FLASH_0x07_MODE_SEL)
                      | (temp0 &  MSK_FLASH_0x07_FWREN_FLASH_CPU)
                      | (temp0 &  MSK_FLASH_0x07_WRSR_DATA)
                      | (temp0 &  MSK_FLASH_0x07_CRC_EN));

    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (0                 << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_CRMR << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);
}

void flash_erase_sector(uint32_t address, uint8_t erase_size) 
{
    unsigned int temp0;
    uint32_t flash_opcode;
    uint32_t cpu_flags, mask;

    if(erase_size == FLASH_ERASE_32K)
    {
        flash_opcode = FLASH_OPCODE_BE1;
    }
    else if(erase_size == FLASH_ERASE_64K)
    {
        flash_opcode = FLASH_OPCODE_BE2;
    }
    else
    {
        flash_opcode = FLASH_OPCODE_SE;
    }
    #if (BT_DUALMODE_RW == 1)
    rw_ext_wakeup();
    #endif
    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
	set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    set_flash_protect(0);

    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (address      << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (flash_opcode << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1          << SFT_FLASH_0x00_OP_SW)
                      | (temp0        &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);
    set_flash_protect(1);
    flash_config();
    
    set_spr( SPR_VICTR(0), 0x00000000 );
    LSLCirq_Clear_All_Interrupts();          /* clear outdated irq, avoid BT_disconn:0x08. yangyang, 2019/09/05 */

    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}

void flash_set_line_mode(uint8_t mode) 
{
	uint32_t temp0 = 0;
    
    switch(mode) 
    {
        case FLASH_LINE_1:
            clr_flash_qwfr();
            REG_FLASH_0x07 &= (~(0x1F<<SFT_FLASH_0x07_MODE_SEL));
            break;
            
        case FLASH_LINE_2:
            clr_flash_qwfr();
            REG_FLASH_0x07 &= (~(7<<SFT_FLASH_0x07_MODE_SEL));
            REG_FLASH_0x07 |= (1<<SFT_FLASH_0x07_MODE_SEL);
            break;

        case FLASH_LINE_4:
#if(DEFAULT_LINE_MODE == FLASH_LINE_4)
            if(((flash_mid&0x00ff0000) == 0x00C20000)||((flash_mid&0x00ff0000) == 0x00BA0000)) //MXIC or ZETTA
            {
                if(READ_CHIPID == 0x326283) //chipid = '3262S'
                {   
                    set_flash_qwfr();                  /**< 4�ߴ� */        
                }
                else
                {
                    clr_flash_qwfr();
                    REG_FLASH_0x07 &= (~(7<<SFT_FLASH_0x07_MODE_SEL));
                    REG_FLASH_0x07 |= (1<<SFT_FLASH_0x07_MODE_SEL);
                }
            }
    	    else
            {
                set_flash_qwfr();                  /**< 4�ߴ� */
            }
#else
            clr_flash_qwfr();
            REG_FLASH_0x07 &= (~(7<<SFT_FLASH_0x07_MODE_SEL));
            REG_FLASH_0x07 |= (1<<SFT_FLASH_0x07_MODE_SEL);
#endif
            break;

        default:
            break;
    }

    /* Flash read data operation after setting 4 line mode */
	while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (0                 << SFT_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_READ << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    
    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
    temp0 = REG_FLASH_0x01;
}

uint32_t flash_read_sr(void)
{
	unsigned int temp0;
    uint32_t flash_id;

    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (temp0             &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_RDSR << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);

    flash_id = REG_FLASH_0x05;

    return (flash_id & 0x00ff);
}

uint32_t flash_read_sr2(void)
{
	unsigned int temp0;
    uint32_t flash_id;

    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (temp0              &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_RDSR2 << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1                << SFT_FLASH_0x00_OP_SW)
                      | (temp0              &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);

    flash_id = REG_FLASH_0x05;

    return (flash_id & 0x00ff);
}

void set_flash_ctrl_config(void)
{
    unsigned int temp0;
    unsigned int cfg = 0;
    
    cfg |= (flash_read_sr() & 0x000000ff);
    cfg |= ((flash_read_sr2() & 0x000000ff) << 8);
    temp0 = REG_FLASH_0x07;
    temp0 &= ~(0xffff << SFT_FLASH_0x07_WRSR_DATA);  // set Flash Status Register = 0
    REG_FLASH_0x07 = temp0 | (cfg << SFT_FLASH_0x07_WRSR_DATA);
}

uint32_t flash_read_mID(void) 
{
    unsigned int temp0;
    uint32_t flash_id;

    while(REG_FLASH_0x00 & 0x80000000);
    temp0 = REG_FLASH_0x00;
    REG_FLASH_0x00 = (  (temp0             &  MSK_FLASH_0x00_ADDR_SW_REG)
                      | (FLASH_OPCODE_RDID << SFT_FLASH_0x00_OP_TYPE_SW)
                      | (0x1               << SFT_FLASH_0x00_OP_SW)
                      | (temp0             &  MSK_FLASH_0x00_WP_VALUE));
    while(REG_FLASH_0x00 & 0x80000000);

    flash_id = REG_FLASH_0x04;

    return (flash_id & 0xffffff);
}

void flash_init(void) 
{
    uint32_t id1;

    flash_mid = flash_read_mID();
    //Delay(50);
    os_delay_us(50);
    id1 = flash_read_mID();
	
    if(flash_mid != id1)
    {
        LOG_E(DRV, "FLASH MID = 0x%X != 0x%x\r\n", flash_mid,id1);
        BK3000_start_wdt(0xfff);
        while(1);
    }

    LOG_I(DRV, "FLASH MID = 0x%X\r\n", flash_mid);
    set_flash_ctrl_config();
    FLASH_ENVDATA_DEF_ADDR = app_env_get_flash_addr(TLV_SECTOR_ENVDATA);

    
    set_flash_crc_en();
#if(DEFAULT_LINE_MODE == FLASH_LINE_4)
    set_flash_qe();
#endif
    set_flash_protect(1);
    flash_config();
}

void flash_config(void)
{
/*
    if(((flash_mid&0x00ff0000) == 0x00C20000)||((flash_mid&0x00ff0000) == 0x00BA0000)) //MXIC or ZETTA
    {
        if(READ_CHIPID == 0x326283) //chipid = '3262S'
        {   
            flash_set_line_mode(FLASH_LINE_4);
        }
        else
            flash_set_line_mode(FLASH_LINE_2);
    }
    else
        flash_set_line_mode(FLASH_LINE_4);
	*/

    flash_set_line_mode(DEFAULT_LINE_MODE);	
    set_flash_clk(FLASH_CLK_SEL);                /**< �ָ�flashʱ�� */
}

void flash_memcpy(uint8_t *dst,uint8_t *src,uint32_t len,BOOL first)
{
	uint32_t i, reg_value;
    uint32_t addr = (uint32_t )src&(~0x1f);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];

    if (len == 0) return;
	if(first) flash_crc_idx = ((uint32_t)src) % 34;
	
    while(REG_FLASH_0x00 & 0x80000000);
    while (len) 
	{
        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr              << SFT_FLASH_0x00_ADDR_SW_REG)
                                | (FLASH_OPCODE_READ << SFT_FLASH_0x00_OP_TYPE_SW)
                                | (0x1               << SFT_FLASH_0x00_OP_SW)
                                | (reg_value         &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        addr+=32;

        for (i=0; i<8; i++)
            buf[i] = REG_FLASH_0x02;

        for (i=(uint32_t)src&0x1f; i<32; i++)
		{
			if(flash_crc_idx < 32)
			{
            	*dst++ = pb[i];
			}
			flash_crc_idx++;
			if(flash_crc_idx == 34)
				flash_crc_idx = 0;
            src++;
            len--;
            if (len == 0)
                break;
        }
	}
}

void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len) 
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];
	addr &= MSK_FLASH_0x00_ADDR_SW_REG;
    if (len == 0)
        return;

    while(REG_FLASH_0x00 & 0x80000000);

    FLS_GLOBAL_INT_DISABLE();
    while (len)
    {
        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr              << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_READ << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1               << SFT_FLASH_0x00_OP_SW)
                          | (reg_value         &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        addr += 32;

        for (i = 0; i < 8; i++)
            buf[i] = REG_FLASH_0x02;

        for (i = (address & 0x1F); i < 32; i++)
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if (len == 0)
                break;
        }
    }
    FLS_GLOBAL_INT_RESTORE();
}

void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8] = {~0x00UL};
    uint8_t *pb = (uint8_t *)&buf[0];
    uint32_t cpu_flags, mask;

    #if (BT_DUALMODE_RW == 1)
    rw_ext_wakeup();
    #endif
    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
    
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    
#if (CONFIG_DRIVER_OTA == 1)
    if(!driver_ota_is_ongoing())
#endif
    {
        set_flash_protect(0);
    }
    while(REG_FLASH_0x00 & 0x80000000);
    
    while(len) 
    {
        if((address & 0x1F) || (len < 32))
            flash_read_data(pb, addr, 32);

        for(i = (address & 0x1F); i < 32; i++) 
        {
            if(len)
            {
                pb[i] = *buffer++;
                address++;
                len--;
            }
        }

        for(i = 0; i < 8; i++)
            REG_FLASH_0x01 = buf[i];

        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr            << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_PP << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1             << SFT_FLASH_0x00_OP_SW)
                          | (reg_value       &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);
        addr += 32;
    }
    
#if (CONFIG_DRIVER_OTA == 1)
    if(!driver_ota_is_ongoing())
#endif
    {
        set_flash_protect(1);
    }
    flash_config();
    
#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())
    {
        driver_ota_tx_arqn_nak_flag_set(1);      /* rescue to send NAK pkt, avoid miss receive pkt, yangyang, 2019/09/26*/
    }
#endif	

    set_spr( SPR_VICTR(0), 0x00000000 );
    LSLCirq_Clear_All_Interrupts();          /* clear outdated irq, avoid BT_disconn:0x08. yangyang, 2019/09/05 */

    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}

void flash_write_page_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t i, reg_value;
    uint32_t addr = address&(~0xFF);
    uint8_t buf[256];
    uint32_t cpu_flags, mask;
    #if (BT_DUALMODE_RW == 1)
    rw_ext_wakeup();
    #endif
    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
    
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    
#if (CONFIG_DRIVER_OTA == 1)
    if(!driver_ota_is_ongoing())
#endif
        set_flash_protect(0);
	
    while(REG_FLASH_0x00 & 0x80000000);
    
    while(len) 
    { 
        if((address & 0xFF) || (len < 256))
            flash_read_data(buf, addr, 256);

        for(i = (address & 0xFF); i < 256; i++) 
        {
            if(len)
            {
                buf[i] = *buffer++;
                address++;
                len--;
            }
        }

        REG_FLASH_0x05 |= MSK_FLASH_0x05_PW_WRITE;
        REG_FLASH_0x09 |= MSK_FLASH_0x09_MEM_ADDR_CLR;
        REG_SYSTEM_0x20 &= ~MSK_SYSTEM_0x20_FLS_MEM_SD;

        for(i = 0; i < 256; i++)
            REG_FLASH_0x09 = buf[i];

        reg_value = REG_FLASH_0x00;
        REG_FLASH_0x00 = (  (addr            << SFT_FLASH_0x00_ADDR_SW_REG)
                          | (FLASH_OPCODE_PP << SFT_FLASH_0x00_OP_TYPE_SW)
                          | (0x1             << SFT_FLASH_0x00_OP_SW)
                          | (reg_value       &  MSK_FLASH_0x00_WP_VALUE));
        while(REG_FLASH_0x00 & 0x80000000);

        REG_FLASH_0x05 &= ~MSK_FLASH_0x05_PW_WRITE;
        REG_SYSTEM_0x20 |= MSK_SYSTEM_0x20_FLS_MEM_SD;
        
        addr += 256;
    }
    
#if (CONFIG_DRIVER_OTA == 1)
    if(!driver_ota_is_ongoing())
#endif
        set_flash_protect(1);

    flash_config();
    
#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())
    {
        driver_ota_tx_arqn_nak_flag_set(1);      /* rescue to send NAK pkt, avoid miss receive pkt, yangyang, 2019/09/26*/
    }
#endif	

    set_spr( SPR_VICTR(0), 0x00000000 );
    LSLCirq_Clear_All_Interrupts();          /* clear outdated irq, avoid BT_disconn:0x08. yangyang, 2019/09/05 */

    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}

void flash_crc_remove(uint32_t address) 
{
    uint16_t i, j, k;
    uint32_t addrsrc = address&(~0x1f);
	uint32_t addrstr = address&(~0x1f);
    uint8_t buf[4352];
    uint8_t *pbsrc = (uint8_t *)&buf[0];
	uint8_t *pbstr = (uint8_t *)&buf[0];

	if (!(addrstr & 0x0FFF))
	{
		flash_read_data(pbsrc, addrsrc, 4352);
		flash_erase_sector(addrstr, FLASH_ERASE_4K);
	}
    
	for (i = 0, j = 0; j < 4352; )
    {
		for (k = 0; k < 32; k++, i++, j++)
			pbstr[i] = pbsrc[j];
		j += 2;
	}
    
	flash_write_data(pbstr, addrstr, 4096);
    
	addrsrc += 4352;
	addrstr += 4096;
	CLEAR_WDT;
    CLEAR_SLEEP_TICK;
    
	if (!(addrstr & 0x0FFF))
	{
		flash_read_data(pbsrc, addrsrc, 4352);
		flash_erase_sector(addrstr, FLASH_ERASE_4K);
	}
    
	for (i = 0,j = 0; j < 4352; ) 
    {
		for (k = 0; k < 32; k++, i++, j++)
			pbstr[i] = pbsrc[j];
		j += 2;
	}
    
	flash_write_data(pbstr, addrstr, 4096);

	CLEAR_WDT;
    CLEAR_SLEEP_TICK;
}

#ifdef MEMORIZE_INTO_FLASH
void save_volume_task(void *arg)
{
 //   uint8_t vol_type = ((uint32_t)arg) & 0xFF;

 //   app_write_info(vol_type, NULL, NULL, NULL);
}
#endif
