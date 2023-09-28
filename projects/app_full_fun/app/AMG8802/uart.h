
#if 0

#ifndef _uart_h_
#define _uart_h_

enum UARTSTAUS_EVENT
{
    UNINITIALIZED   = 0,
    INITALIZED      = 1,
    IDLE            = 2,
    TRANSFERING     = 3,
    RECEIVING       = 4,
    BUSY            = 5
    
};

struct list_usartmsg_s
{
    enum UARTSTAUS_EVENT uartstaus;
    unsigned char uartbufLength;        //buf长度配置
    unsigned char *uartbuf_p;           //buf地址
    unsigned char txcount;              //发送长度
    unsigned char receiveLenth;         //实时接受数据长度
    unsigned char transLength;
    unsigned char resetDelay;
};

extern void UART1_Process(void);
extern void RS485_Init(struct  list_usartmsg_s *p_uartCfg);
extern void Uart1Send(uint8_t txData);
extern void Uart1_Init(void);
extern unsigned char UART1_TransPush(unsigned char * p_transData,unsigned char length);
extern unsigned char UART1_receiveLengthNow(void);

#endif

#endif
