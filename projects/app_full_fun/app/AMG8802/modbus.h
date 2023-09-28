#ifndef _modbus_h_
#define _modbus_h_

typedef unsigned char   (*rtU8)(void);
typedef unsigned short  (*rtU16)(void);
typedef unsigned int    (*rtU32)(void);
typedef unsigned long   (*rtU64)(void);
typedef  char   (*rtS8)(void);
typedef  short  (*rtS16)(void);
typedef  int    (*rtS32)(void);
typedef  long   (*rtS64)(void);
typedef  _Bool   (*rtBool)(void);

enum reg_acttype
{
  read    = 0,
  write   = 1,
  clear   = 2
};
#define modbus_regRead  0  
#define modbus_regWrite 1
#define modbus_regClear 2

#define modbus_regType   0  
#define modbus_coilType  1
#define modbus_otherType 2


#define modbus_endian(a) (unsigned short)((a<<8)&0xff00)|((a>>8)&0x00ff)

typedef unsigned short (*modbus_reg_program)(void);

#define modSlaverreg(x)  unsigned char (x)(unsigned short addr ,\
                                          unsigned char act,\
                                          unsigned char type,\
                                          unsigned short* data)
                                          
typedef unsigned short (*modbusSlaver_reg)(unsigned short addr ,\
                                          unsigned char act,\
                                          unsigned char type,\
                                          unsigned short* data);
                                          
                                          
typedef unsigned short (*modbusMaster_reg)(
                                            unsigned char nodeaddr,
                                            unsigned short regAddr,
                                            unsigned char act,
                                            unsigned char type,
                                            unsigned short* data); //modbus Register data
typedef unsigned short (*modbus_reg)(
                                            unsigned char nodeaddr,
                                            unsigned short regAddr,
                                            unsigned char act,
                                            unsigned char type,
                                            unsigned short* data); //modbus Register data
struct list_modbusdecodemsg_s
{
  unsigned char fun;
  unsigned char start_addr;
  unsigned char answer_length;
};
struct list_modbusreg_s
{
  unsigned short (*date) (unsigned short addr);
};

typedef struct list_modbusnode_s
{
//  unsigned char (*ask)(
//                          unsigned char slaveaddr,
//                          unsigned char *msgbuf_p,  //received msg point
//                          unsigned char ask_fun,
//                          unsigned short ask_length, //received msg length
//                          unsigned short ask_regaddr, //decode result
//                          unsigned short *tx_length
//                        );      
  unsigned char (*masterProcess)(void);     //状态轮训函数
  unsigned char (*modbus_masterSend)(unsigned char * trans_P,unsigned char transLength);
  modbusMaster_reg reg;                     //本地寄存器数据地址
  unsigned char * msg;                     //底层数据buf地址
  rtU8  rxLength;                         //底层接收长度地址
  unsigned char txlength;
  unsigned char bufLength;                  //数据buf最大长度
  unsigned char askFun;                     //请求功能
  unsigned short askSlaveAddr;              //请求从机地址
  unsigned short askRegAddr;                //请求寄存器地址
  unsigned short askLength;                 //请求寄存器长度
}list_modbusnode_S;


typedef struct list_modbusSlaveNode_s
{
    unsigned char (*modbus_slaverSend)(unsigned char * trans_P,unsigned char transLength);          // 从机发送函数
    modbusSlaver_reg reg;                                                                           // 寄存器操作函数
    unsigned char * msg;                                                                            // 接收数组
    rtU8  rxLength;                                                                                 // 实时接收数组的长度
    unsigned char txlength;                                                                        
    unsigned char bufLength;                                                                        // 数组总长度
    unsigned char askFun;                                                                           // 请求功能码
    unsigned short SlaveAddr;                                                                       // 从机地址
    unsigned short askRegAddr;                                                                      // 请求寄存器地址
    unsigned short askLength;                                                                       // 请求寄存器长度
    void    (*deviceProcess)(void);                                                                 // 设备进程->串口使用
}list_modbusSlaveNode_S;    //MODBUS 节点信息结构体



struct list_modbusMasterState_s
{
  unsigned char * recePoint;
  unsigned char (*receLength)(void);
  unsigned char bufLength;
  unsigned char askFun;
  unsigned short slaveAddr;
  unsigned short regAddr;
  unsigned short askLength;
};


extern unsigned char modbusMaster_Init(
                          list_modbusnode_S *nobe
                          ,modbusMaster_reg reg_data
                          ,unsigned char * recePoint
                          ,unsigned char (* receLength)(void)
                          ,unsigned char bufLength
                          );
extern unsigned char modbus_decode(
                            list_modbusSlaveNode_S *nobe
                            ,unsigned char localaddr   
                            );

extern unsigned char modbus_ask(
                          list_modbusnode_S *nobe,
                          unsigned char slaveaddr,
                          unsigned char ask_fun,
                          unsigned short ask_length, //received msg length
                          unsigned short ask_regaddr //decode result
                        );

extern unsigned char modbus_masterProcess(list_modbusnode_S *nobe);
extern unsigned char modbus_slaverProcess(list_modbusSlaveNode_S *nobe);
                          
#endif
