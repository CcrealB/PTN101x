#if 0

//#include "wb32f10x_rcc.h"
//#include "wb32f10x_uart.h"
//#include "wb32f10x_dmac.h"
//#include "gpio.h"
#include "uart.h"
//#include "dma.h"
#include <string.h>
#include <stdbool.h>

#define RS485_CTL           GPIOB_USER->ODR.ODR15       //485 ctl  PB15
#define UART1TXDMA_SOUCE    p_uart1Config->uartbuf_p                    //串口发送源地址指针
#define RS485RD_Transfer()  RS485_CTL=1
#define RS485RD_Receive()   RS485_CTL=0
#define RESETDELAY_VPT      5

static struct  list_usartmsg_s  *p_uart1Config;
//unsigned char uart1Count=0,dma1ch12_stage=0;


void DMAUart1TX_Init(void)                      //暂时未调好，不用
{
    DMAC_Channel_InitTypeDef DMAC_Channel_InitStruct;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMAC1Bridge, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BMX1 | RCC_APB1Periph_DMAC1, ENABLE);

    DMAC_DeInit(DMAC1);
    DMAC_Channel_InitStruct.DMAC_SourceBaseAddr = (uint32_t)UART1TXDMA_SOUCE;                                             //源地址
    DMAC_Channel_InitStruct.DMAC_DestinationBaseAddr = (uint32_t)&UART1->THR;                                                   //目标地址
    DMAC_Channel_InitStruct.DMAC_Interrupt = DMAC_Interrupt_Enable;                                                             //中断使能控制
    DMAC_Channel_InitStruct.DMAC_SourceTransferWidth = DMAC_SourceTransferWidth_8b;                                             //传递宽度
    DMAC_Channel_InitStruct.DMAC_DestinationTransferWidth = DMAC_DestinationTransferWidth_8b;                                   //目标宽度
    DMAC_Channel_InitStruct.DMAC_SourceAddrInc = DMAC_SourceAddrInc_Increment;                                                  //源地址自增控制
    DMAC_Channel_InitStruct.DMAC_DestinationAddrInc = DMAC_DestinationAddrInc_NoChange;                                         //目标地址自增控制
    DMAC_Channel_InitStruct.DMAC_SourceTransactionLength = DMAC_SourceTransactionLength_1;                                      //源传送长度
    DMAC_Channel_InitStruct.DMAC_DestinationTransactionLength = DMAC_DestinationTransactionLength_1;                            //目标传送长度
    DMAC_Channel_InitStruct.DMAC_TransferTypeAndFlowControl = DMAC_TransferTypeAndFlowControl_MemoryToPeripheral_DMAC;          //传送模式控制
    DMAC_Channel_InitStruct.DMAC_SourceMasterInterface = DMAC_SourceMasterInterface_AHB;                                        //源总线
    DMAC_Channel_InitStruct.DMAC_DestinationMasterInterface = DMAC_DestinationMasterInterface_APB;                              //目标总线
    DMAC_Channel_InitStruct.DMAC_BlockTransferSize = 20;                                                                        //块大小
    DMAC_Channel_InitStruct.DMAC_SourceHandshakingInterfaceSelect = DMAC_SourceHandshakingInterfaceSelect_Hardware;             //源接口
    DMAC_Channel_InitStruct.DMAC_DestinationHandshakingInterfaceSelect = DMAC_DestinationHandshakingInterfaceSelect_Hardware;   //目标接口
    DMAC_Channel_InitStruct.DMAC_SourceHandshakingInterfacePolarity = DMAC_SourceHandshakingInterfacePolarity_High;             //源接口优先级
    DMAC_Channel_InitStruct.DMAC_DestinationHandshakingInterfacePolarity = DMAC_DestinationHandshakingInterfacePolarity_High;   //目标接口优先级
    DMAC_Channel_InitStruct.DMAC_AutomaticSourceReload = DMAC_AutomaticSourceReload_Enable;                                     //源地址自动重载
    DMAC_Channel_InitStruct.DMAC_AutomaticDestinationReload = DMAC_AutomaticDestinationReload_Disable;                          //目标地址自动重载
    DMAC_Channel_InitStruct.DMAC_FlowControlMode = DMAC_FlowControlMode_0;                                                      //
    DMAC_Channel_InitStruct.DMAC_FIFOMode = DMAC_FIFOMode_0;                                                                    //
    DMAC_Channel_InitStruct.DMAC_ChannelPriority = 0;                                                                           //
    DMAC_Channel_InitStruct.DMAC_ProtectionControl = 0x1;                                                                       //保护控制
    DMAC_Channel_InitStruct.DMAC_SourceHardwareHandshakingInterfaceAssign = 0;                                                  //源接口模式
    DMAC_Channel_InitStruct.DMAC_DestinationHardwareHandshakingInterfaceAssign = DMAC_HardwareHandshakingInterface_UART1_TX;    //目标接口模式
    DMAC_Channel_InitStruct.DMAC_MaximumAMBABurstLength = 0;                                                                    //
    DMAC_Channel_Init(DMAC1, DMAC_Channel_0, &DMAC_Channel_InitStruct);                                                         //DMA1初始化
    
    DMAC_ITConfig(DMAC1, DMAC_Channel_0, DMAC_IT_BLOCK, ENABLE);
    DMAC_ITConfig(DMAC1, DMAC_Channel_0, DMAC_IT_TFR, ENABLE);
    DMAC_ITConfig(DMAC1, DMAC_Channel_0, DMAC_IT_ERR, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = DMAC1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMAC_Cmd(DMAC1, ENABLE);
    DMAC_ChannelCmd(DMAC1, DMAC_Channel_0, ENABLE);

}

void Uart1Send(uint8_t txData)          //单字节发送
{
    RS485RD_Transfer();
        /* Loop until the THR is empty */
    while(!(UART_GetLineStatus(UART1) & UART_LINE_STATUS_THRE));
    UART_WriteData(UART1, txData);
    /* Loop until the end of transmission */
    while(!(UART_GetLineStatus(UART1) & UART_LINE_STATUS_TEMT));
}

void Uart1_Init(void)
{
    
    NVIC_InitTypeDef NVIC_InitStructure;
    UART_InitTypeDef UART_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BMX1 |RCC_APB1Periph_GPIOA   |RCC_APB1Periph_GPIOB    |RCC_APB1Periph_UART1, ENABLE);
    GPIO_Init(GPIOA, GPIO_Pin_9 | GPIO_Pin_10, GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_PUPD_UP | GPIO_SPEED_HIGH |GPIO_AF7);
      /* UART1 configuration */
    UART_DeInit(UART1);
    UART_InitStructure.UART_BaudRate = 19200;
    UART_InitStructure.UART_WordLength = UART_WordLength_8b;
    UART_InitStructure.UART_StopBits = UART_StopBits_One;
    UART_InitStructure.UART_Parity = UART_Parity_None;
    UART_InitStructure.UART_AutoFlowControl = UART_AutoFlowControl_None;
    UART_Init(UART1, &UART_InitStructure);
    UART_ITConfig(UART1, UART_IT_RDA, ENABLE);
    
    /* Enable the UART1 Interrupt Channel */
    NVIC_InitStructure.NVIC_IRQChannel = UART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    
}


void RS485_Init(struct  list_usartmsg_s *p_uartCfg)
{
    p_uart1Config=p_uartCfg;
    Uart1_Init();                                                                                                           //串口初始化
    GPIO_Init(GPIOB, GPIO_Pin_15, GPIO_MODE_OUT|GPIO_OTYPE_PP |GPIO_PUPD_NOPULL |GPIO_SPEED_HIGH);                           //收发控制器初始化
    RS485RD_Receive();                                                                                                      //485半双工切换回接受状态
    UART_TxFIFOThresholdConfig(UART1, UART_TxFIFOThreshold_8);
    UART_FIFOCmd(UART1, ENABLE);
    UART_ProgrammableTHREModeCmd(UART1, ENABLE);
    p_uart1Config->uartstaus=IDLE;
//    DMAUart1TX_Init();                                                                                                      //DMA初始化
}

void UART1_IRQHandler(void)
{
    unsigned char int_id;
    int_id = UART_GetIntID(UART1);
    if(int_id==UART_INTID_RDA)
    {
        p_uart1Config->uartbuf_p[p_uart1Config->receiveLenth] = UART_ReadData(UART1);
        
        p_uart1Config->receiveLenth=(p_uart1Config->receiveLenth+1)&0xff;
        p_uart1Config->resetDelay=0;
    }
    if(int_id==UART_INTID_THRE)
    {
        UART_WriteData(UART1,p_uart1Config->uartbuf_p[p_uart1Config->txcount]);
        p_uart1Config->txcount=(p_uart1Config->txcount+1)&0xff;
        if(p_uart1Config->txcount>p_uart1Config->transLength)
        {
            UART_ITConfig(UART1, UART_IT_THRE, DISABLE);
            p_uart1Config->transLength=0;
            p_uart1Config->txcount=0;
            UART_ITConfig(UART1, UART_IT_RDA, ENABLE);
            p_uart1Config->uartstaus=IDLE;                                                                                      //状态机切换到空闲
        }
    }
}

void DMAC1_IRQHandler(void)
{
    if(DMAC_GetITStatus(DMAC1, DMAC_Channel_0, DMAC_IT_BLOCK) != RESET)
    {
        RS485RD_Receive();                                                                                                      //485半双工切换回接受状态
        DMAC_ClearITPendingBit(DMAC1, DMAC_Channel_0, DMAC_IT_BLOCK);
    }
    
    if(DMAC_GetITStatus(DMAC1, DMAC_Channel_0, DMAC_IT_TFR) != RESET)
    {
        RS485RD_Receive();                                                                                                      //485半双工切换回接受状态
        DMAC_ClearITPendingBit(DMAC1, DMAC_Channel_0, DMAC_IT_TFR);
    }
    
    if(DMAC_GetITStatus(DMAC1, DMAC_Channel_0, DMAC_IT_ERR) != RESET)
    {
        DMAC_ClearITPendingBit(DMAC1, DMAC_Channel_0, DMAC_IT_ERR);
    }
}

void UART1_Process(void)                         //串口控制节拍进程
{
    if(p_uart1Config->uartstaus==IDLE)           //空闲
    {
        if(p_uart1Config->resetDelay>RESETDELAY_VPT)
        {
            RS485RD_Receive();
            p_uart1Config->uartstaus=RECEIVING;         //状态机切换到接收模式
            p_uart1Config->resetDelay=0;
        }
        else
        {
            p_uart1Config->resetDelay++;
        }
    }
    else if(p_uart1Config->uartstaus==TRANSFERING)           //发送
    {
        UART_ITConfig(UART1, UART_IT_THRE, ENABLE);
    }
    else if(p_uart1Config->uartstaus==RECEIVING)
    {
        if(p_uart1Config->receiveLenth!=0)                  //接收复位机制
        {
            if(p_uart1Config->resetDelay>RESETDELAY_VPT)
            {
                p_uart1Config->resetDelay=0;
                p_uart1Config->receiveLenth=0;
                memset(p_uart1Config->uartbuf_p,0,256);
            }
            else
            {
                p_uart1Config->resetDelay++;
            }
        }
    }

}

unsigned char UART1_TransPush(unsigned char * p_transData,unsigned char length)         //串口发送数据注入，启动中断发送
{
    if(length>255) 
    {
        return false;
    }
    RS485RD_Transfer();
    memcpy(p_uart1Config->uartbuf_p,p_transData,length);
    p_uart1Config->txcount=0;
    p_uart1Config->transLength=length-1;
    p_uart1Config->resetDelay=0;
    p_uart1Config->uartstaus=TRANSFERING;
    UART_ITConfig(UART1, UART_IT_RDA, DISABLE);
    p_uart1Config->receiveLenth=0;
//    DMAC_Channel_SetBlockTransferSize(DMAC1,DMAC_Channel_0,length);
//    DMAC_ChannelCmd(DMAC1, DMAC_Channel_0, ENABLE);
    return true;
}


unsigned char UART1_receiveLengthNow(void)                                              //串口接收数据量
{
	return p_uart1Config->receiveLenth;
}

#endif
