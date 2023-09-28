#include "driver_vusb_dlp.h"
//#include "bk3000_reg.h"
#include "bkreg.h"
#include "drv_system.h"
#include "driver_serio.h"
#include "drv_audio.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "le_cts_app.h"
#include "app_beken_includes.h"
#include "driver_vusb.h"
#include "le_scan.h"

#define VUSB_DLP_TX_WRITE_READY     (REG_VUSB_DLP_FIFO_STATUS & MSK_DLP_FIFO_STATUS_FIFO_WR_READY)
#define VUSB_DLP_RX_READ_READY      (REG_VUSB_DLP_FIFO_STATUS & MSK_DLP_FIFO_STATUS_FIFO_RD_READY)
#define VUSB_DLP_WRITE_BYTE(v)      (REG_VUSB_DLP_FIFO_PORT=v)
#define VUSB_DLP_READ_BYTE()        ((REG_VUSB_DLP_FIFO_PORT>>8)&0xff)
static uint8 s_dlp_rx_data[64];
static uint8 s_dlp_tx_data[64];
//static uint8 s_dlp_box_bat=100;
//static uint8 s_dlp_putin_status=0;
//static uint8 s_dlp_m_s_com_status=0;
static uint16 s_dlp_rx_index=0;
HCI_COMMAND_PACKET *pDLPrxBuf = (HCI_COMMAND_PACKET *)(&s_dlp_rx_data[0]);
HCI_EVENT_PACKET   *pDLPtxBuf = (HCI_EVENT_PACKET *)(&s_dlp_tx_data[0]);

//extern uint8 GAP_Adv_Data[31];
//extern uint8 GAP_Adv_Data_Length;
//extern void LEadv_Advertise_Disable_Enable(unsigned char enable);

void vusb_dlp_init(uint32 baudrate)
{
    uint32 apll_clk = ((REG_SYSTEM_0x4B & 0x3FFFFFFF) == AUDIO_DIV_441K) ? SYS_APLL_90p3168_MHZ : SYS_APLL_98p3040_MHZ;
    uint32 clk = (baudrate > DLP_HIGH_BAUDRATE) ? apll_clk : UART_CLOCK_FREQ_26M;
    uint32    baud_divisor = clk/baudrate;
    baud_divisor = (baud_divisor - 1) & MAX_DLP_CONF_CLK_DIVID;
    system_peri_clk_enable(SYS_PERI_CLK_DLP);
    vusb_dlp_clk_select(baudrate > DLP_HIGH_BAUDRATE);

    REG_VUSB_DLP_CONF = (DEF_STOP_BIT << SFT_DLP_CONF_STOP_LEN)
                     | (DEF_PARITY_MODE << SFT_DLP_CONF_PAR_MODE)
                     | (DEF_PARITY_EN << SFT_DLP_CONF_PAR_EN)
                     | (DEF_DATA_LEN << SFT_DLP_CONF_LEN)
                     | (baud_divisor << SFT_DLP_CONF_CLK_DIVID)
                     | (DEF_RX_EN << SFT_DLP_CONF_RX_ENABLE)
                     | (DEF_TX_EN << SFT_DLP_CONF_TX_ENABLE);

    REG_VUSB_DLP_FIFO_CONF  = ((RX_FIFO_THRD << SFT_DLP_FIFO_CONF_RX_FIFO_THRESHOLD) | (TX_FIFO_THRD << SFT_DLP_FIFO_CONF_TX_FIFO_THRESHOLD) | (0x0 << SFT_DLP_FIFO_CONF_RX_STOP_DETECT_TIME));
    REG_VUSB_DLP_INT = 0;
    REG_VUSB_DLP_INT = MSK_DLP_INT_ENABLE_RX_STOP_END_MASK | MSK_DLP_INT_ENABLE_RX_FIFO_NEED_READ_MASK;

    
}

/*
DLP_CLK_XTAL            0
DLP_CLK_APLL            1
*/
void vusb_dlp_clk_select(uint8 sel)
{
    REG_SYSTEM_0x01 &= ~MSK_SYSTEM_0x01_DLP_SEL;
    REG_SYSTEM_0x01 |= ((sel&MAX_SYSTEM_0x01_DLP_SEL) << SFT_SYSTEM_0x01_DLP_SEL);
}
void vusb_dlp_enable(void)
{
    system_peri_clk_enable(SYS_PERI_CLK_DLP);  
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_DLP);
    
}
void vusb_dlp_disable(void)
{
    system_peri_clk_disable(SYS_PERI_CLK_DLP); 
    system_peri_mcu_irq_disable(SYS_PERI_IRQ_DLP);
}
#if 0
uint8 vusb_dlp_get_putin(void)
{
	return 0; 
}

uint8 vusb_dlp_get_sec_putin(void)
{
    return 0;	
}
uint8 vusb_dlp_get_putin_popup_rand(void)
{
    return 0;	
}
static void vusb_dlp_putin_popup_window(void)
{

}

static void vusb_dlp_putin_detect(void)
{

}

void vusb_dlp_sec_putin_status(uint8 *param)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    INFO_PRT("DLP.putin_state:0x%x,%d\r\n",sys_hdl->sec_putin_status,param[1]);
}

uint8 vusb_dlp_is_box_bat(void)
{

	return 100;
}

uint8 vusb_dlp_is_com_status(void)
{
	return 0;
}

void vusb_dlp_popup_pairing(uint8 para)
{
 
}

uint8 vusb_dlp_popup_window(uint8 *param)
{
}
#endif
void vusb_dlp_clear_buffer(void) 
{
	s_dlp_rx_index = 0;
    memset(s_dlp_rx_data, 0, sizeof(s_dlp_rx_data)); /**< Clear the RX buffer */
    memset(s_dlp_tx_data, 0, sizeof(s_dlp_tx_data)); /**< Clear the TX buffer */
}

void vusb_dlp_tx(void) 
{	
	//app_handle_t sys_hdl = app_get_sys_handler();
    unsigned int tx_len = HCI_EVENT_HEAD_LENGTH+2;//,len;
    //INFO_PRT("DLP.dlp_tx:0x%x,0x%x,0x%x\r\n",pDLPrxBuf->param[0],sys_hdl->flag_ble_scan_adv_cnt,sys_hdl->flag_ble_scan_adv);
    pDLPtxBuf->code  = TRA_HCIT_EVENT;
    pDLPtxBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    pDLPtxBuf->total = 2;
    pDLPtxBuf->param[0] = pDLPrxBuf->cmd;
    pDLPtxBuf->param[1] = pDLPrxBuf->param[0]; 
#if 0
	if (pDLPtxBuf->param[1] == VUSB_DLP_POP)
	{
		tx_len ++; 
		pDLPtxBuf->total ++;
		if (!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
			pDLPtxBuf->param[2] = 0;
		else if (bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION))
			pDLPtxBuf->param[2] = VUSB_DLP_POLL_RSP_PHONE;
		else
			pDLPtxBuf->param[2] = VUSB_DLP_POLL_RSP_TWS;
		if ((s_dlp_m_s_com_status==VUSB_DLP_M_S_COM_INACTIVE)
			&& ((sys_hdl->flag_ble_scan_adv_cnt)||(sys_hdl->flag_ble_scan_adv)))
		{
			s_dlp_m_s_com_status = VUSB_DLP_M_S_COM_ING;
			for (len=0; len<GAP_Adv_Data_Length; len++)
				pDLPtxBuf->param[3+len] = GAP_Adv_Data[len];
			tx_len += (GAP_Adv_Data_Length-3);
			pDLPtxBuf->total += (GAP_Adv_Data_Length-3); 
			INFO_PRT("DLP.tx:%d,%d\r\n",pDLPtxBuf->total,tx_len);
		}
	}
    else if ((pDLPtxBuf->param[1]==VUSB_DLP_POLL) || (pDLPtxBuf->param[1]==VUSB_DLP_PWR))
	{
		tx_len ++; 
		pDLPtxBuf->total ++;
		if (pDLPtxBuf->param[1] == VUSB_DLP_POLL)
			pDLPtxBuf->param[2] = (vusb_dlp_conn_status_rsp()<<1);

	}
#endif
	vusb_dlp_send(s_dlp_tx_data, tx_len);
}

void vusb_dlp_slave_tx(void *arg)
{
	uint32 sub_cmd =(uint32)arg;
	
	pDLPrxBuf->cmd = BEKEN_CMD_VUSB_DLP; 
	pDLPrxBuf->param[0] = sub_cmd; 
	vusb_dlp_tx();
	vusb_dlp_clear_buffer();
}

/*
    PTN101 MPW V1.0 Simulation of VUSB-DLP
    UART DBG CMD:
    01 e0 fc 05 c1 X1 X2 X3 X4, VSUB DLP RCV DATA = x1x2x3x4;
*/ 
uint32 vusb_dlp_cmd_proc(void)
{	
	//uint8 adv=0,status=0;
    app_handle_t sys_hdl = app_get_sys_handler();
	
    if (s_dlp_rx_index == 0)
        return 0;
#if 1
    INFO_PRT("DLP.rx_data:%02x,%02x,%02x,%02x,%02x\r\n",pDLPrxBuf->cmd
								,pDLPrxBuf->opcode.ocf
								,pDLPrxBuf->opcode.ogf
								,pDLPrxBuf->total
								,s_dlp_rx_index);	
#endif

    if ((pDLPrxBuf->code != TRA_HCIT_COMMAND) 
		|| (pDLPrxBuf->opcode.ocf != BEKEN_OCF)
        || (pDLPrxBuf->opcode.ogf != VENDOR_SPECIFIC_DEBUG_OGF) 
        || (s_dlp_rx_index != (HCI_COMMAND_HEAD_LENGTH+pDLPrxBuf->total)))
    {
		goto ret;
    }

    switch (pDLPrxBuf->cmd) 
    {
        case BEKEN_CMD_VUSB_DLP:
        {
            switch (pDLPrxBuf->param[0])
            {
                case VUSB_DLP_MEMORY:
                    msg_put(MSG_CLEAT_MEMORY);
                    break;
                
                case VUSB_DLP_MATCH:
                    msg_put(MSG_ENTER_MATCH_STATE);
                    break;
                #if 0
                case VUSB_DLP_POP:
                    if (pDLPrxBuf->total >= 5)
                    {
                        if (vusb_dlp_popup_window(&pDLPrxBuf->param[1]))
                            break;	
                    }
                    goto ret;
                #endif
                case VUSB_DLP_PWR:
                    msg_put(MSG_POWER_DOWN);
                    break;	
                case VUSB_DLP_POLL:
                    if (pDLPrxBuf->param[0] == VUSB_DLP_POLL)
                        jtask_schedule(sys_hdl->app_dlp_task,20,(jthread_func)vusb_dlp_slave_tx, (void *)VUSB_DLP_POLL);
                    else
                        jtask_schedule(sys_hdl->app_dlp_task,20,(jthread_func)vusb_dlp_slave_tx, (void *)VUSB_DLP_PWR);
                    break;					
                default:
                    goto ret;
            }
            break;
        }
        case BEKEN_CMD_VUSB_DLP_PRINTF:
        {
            if (!pDLPrxBuf->param[0])
            {
                app_bt_flag2_set(APP_FLAG2_VUSB_DLP_PRINTF,0);
                INFO_PRT("DLP.printf.close\r\n");
            }
            else
            {
                INFO_PRT("DLP.printf.open\r\n");
                vusb_dlp_init(DLP_MEDIUM_BAUDRATE);
                app_bt_flag2_set(APP_FLAG2_VUSB_DLP_PRINTF,1);
            }
            goto ret;
        }
        case BEKEN_CMD_SYS_RESET:
        {
            /* watchdog reset for uart download */
            if(pDLPrxBuf->total >= 5)
            {
            uint32 param = pDLPrxBuf->param[0]<<24 | pDLPrxBuf->param[1]<<16 | pDLPrxBuf->param[2]<<8 | pDLPrxBuf->param[3];
            if(param == 0x95279527) 
            {
            /* watchdog reset */
            BK3000_wdt_reset();
            while(1);
            }
            }
            goto ret;
        }	
        
        default:
            goto ret;
    }
	
 ret:
    return 0;	
}

void vusb_dlp_handler(uint32 step)
{
	static uint16 printf_cnt=0;
	
	//vusb_dlp_putin_detect();

	if (app_bt_flag2_get(APP_FLAG2_VUSB_DLP_PRINTF) && (vusb_get_mode()==VUSB_COMM_MODE))
	{

		if ((++printf_cnt) > 1000)
		{
			printf_cnt = 0;
			INFO_PRT("DLP.volt=%d\r\n",saradc_get_bat_value());
		}
	}
}

static void vusb_dlp_data_process(void)
{	
	if(vusb_dlp_cmd_proc())
	{
		vusb_dlp_tx();
	}
	vusb_dlp_clear_buffer();
}	

void vusb_dlp_send(uint8 *buff,uint16 len)
{
    //INFO_PRT("DLP.send\r\n");    
    if(vusb_get_mode() == VUSB_COMM_MODE)
    {
        while (len--) {
            while((!VUSB_DLP_TX_WRITE_READY) && (vusb_get_mode() == VUSB_COMM_MODE));
                VUSB_DLP_WRITE_BYTE(*buff++);
        }
    }
}

void vusb_dlp_isr(void)
{
    //INFO_PRT("DLP.isr\r\n");
    u_int32 status;
    u_int8 cnt = 0;
    status = REG_VUSB_DLP_INT_STATUS;
    REG_VUSB_DLP_INT_STATUS = status;
    if(status & (MSK_DLP_INT_STATUS_RX_FIFO_NEED_READ|MSK_DLP_INT_STATUS_RX_STOP_END)) 
    {
        while (VUSB_DLP_RX_READ_READY&& (vusb_get_mode() == VUSB_COMM_MODE)) 
        {
            s_dlp_rx_data[cnt] = VUSB_DLP_READ_BYTE();
            cnt++;
            cnt &= 0x3f;
        }
		if (vusb_get_mode() == VUSB_COMM_MODE)
			s_dlp_rx_index = cnt;		
    }
    vusb_dlp_data_process();
}

