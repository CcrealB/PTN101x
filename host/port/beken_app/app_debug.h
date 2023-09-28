#ifndef _APP_DEBUG_H_
#define _APP_DEBUG_H_

#define BEKEN_OCF                   0XE0
#define TX_FIFO_THRD                0x40
#define RX_FIFO_THRD                0x40
#define HCI_EVENT_HEAD_LENGTH       0X03
#define HCI_COMMAND_HEAD_LENGTH     0X04

#ifndef __PACKED_POST__
#define __PACKED_POST__  __attribute__((packed))
#endif

typedef struct {
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    struct {
        unsigned short ocf : 10;    /**< OpCode Command Field */
        unsigned short ogf : 6;     /**< OpCode Group Field */
    } __PACKED_POST__ opcode;
    unsigned char total;
    unsigned char param[];
} __PACKED_POST__ HCI_PACKET;

typedef struct {
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    struct {
        unsigned short ocf : 10;    /**< OpCode Command Field */
        unsigned short ogf : 6;     /**< OpCode Group Field */
    } __PACKED_POST__ opcode;
    unsigned char total;
    unsigned char cmd;              /**< private command */
    unsigned char param[];
} __PACKED_POST__ HCI_COMMAND_PACKET;

typedef struct {
    unsigned char code;             /**< 0x01: HCI Command Packet
                                         0x02: HCI ACL Data Packet
                                         0x03: HCI Synchronous Data Packet
                                         0x04: HCI Event Packet */
    unsigned char event;            /**< 0x00-0xFF: Each event is assigned a 1-Octet event code used to uniquely identify different types of events*/
    unsigned char total;            /**< Parameter Total Length */
    unsigned char param[];
} __PACKED_POST__ HCI_EVENT_PACKET;

typedef struct {
    unsigned int addr;
    unsigned int value;
} __PACKED_POST__ REGISTER_PARAM;

typedef struct {
    unsigned int  addr;
    unsigned char len;
    unsigned char data[];
} __PACKED_POST__ FLASH_PARAM;



typedef enum USER_COM_ALL_e{
    SUB_CMD_LOOP_TEST     = 0x00,//loopback test, recieve and send
    SUB_CMD_DEBUG         = 0x01,
    SUB_CMD_SYS           = 0x02,//about system, chip, cmd ver, etc...
    SUB_CMD_LOG_CTRL      = 0x03,
    SUB_CMD_MODE_SW       = 0x04,
    SUB_CMD_USB_SW        = 0x05,
    SUB_CMD_PLAYER        = 0x06,
    SUB_CMD_RECORDER      = 0x07,
    SUB_CMD_SYS_DPLL      = 0x08,
    SUB_CMD_USB_AUDIO     = 0x09,
    SUB_CMD_MAILBOX       = 0x80,
    SUB_CMD_MAILBOX_DBG   = 0x81,
    SUB_CMD_DSP_SET       = 0x82,
    SUB_CMD_DSP_SET_SINGLE = 0x83,
    SUB_CMD_DSP_EFT_SET   = 0x85,
}USER_COM_ALL_et;


typedef struct _SUB_CMD_PKG_e{
    uint8_t cmd;
    uint8_t param[];
} __attribute__((packed)) SUB_CMD_PKG_e;

// #define CONFIG_SOFT_FIFO_DCOM//not supported @230318
#ifdef CONFIG_SOFT_FIFO_DCOM
//sub dsp cmd
typedef enum DSP_COM_ALL_e{
    D_COM_CMD_LOOP_TEST     = 0x00,//loopback test, recieve and send
    D_COM_CMD_PRINT         = 0x01,
    D_COM_CMD_SYS           = 0x02,//about system, chip, cmd ver, etc...
    D_COM_CMD_AUD_EFT       = 0x10,
    D_COM_CMD_AUD_GAIN      = 0x20,
}DSP_COM_ALL_et;

uint32_t com_cmd_mcu2dsp_proc(uint8_t *buff, uint8_t size);
int mcu2dsp_dfifo_free_num_get(void);
void mailbox_mcu_cmd_handler(MailBoxCmd* mbc);
#endif


#define SYS_LOG_PORT_NO         0x00
#define SYS_LOG_PORT_UART0      0x01
#define SYS_LOG_PORT_UART1      0x02
#define SYS_LOG_PORT_UART1_P67  0x04
#define SYS_LOG_PORT_UART2      0x08
#define SYS_LOG_PORT_DLP        0x10
#define SYS_LOG_PORT_USB_HID    0x20
#define SYS_LOG_PORT_DBG        0x40
#define SYS_LOG_PORT_ALL        0xFF

extern uint8_t g_dsp_log_on_fg;
extern uint8_t g_mcu_log_on_fg;
extern uint8_t g_hid_log_on_fg;

uint32_t sys_log_port_get(uint32_t flag);
void sys_log_port_set(uint32_t flag, int en);
void app_sys_log_init(uint8_t en);
int app_sys_log_write(uint8_t *buf, int size);
void hid_log_open(uint8_t en);
uint8_t hid_log_is_open(void);
void sys_log_open(uint8_t en);
uint8_t sys_log_is_open(void);
void dsp_log_open(uint8_t en);
uint8_t dsp_log_is_open(void);


void uart_initialise(uint32_t baud_rate);
void uart_gpio_disable(void);
void uart_gpio_enable(void);
void uart1_gpio_enable(void);

void sys_debug_show(uint8_t redirect);
int32_t os_printf(const char *fmt, ...);
#endif
