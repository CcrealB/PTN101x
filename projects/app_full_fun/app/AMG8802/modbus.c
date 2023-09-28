
#if 0

#include "modbus.h"
#include <stdbool.h>
#include <string.h>
//static struct list_modbusMasterState_s nobeMsg;
unsigned char modbus_masterProcess(list_modbusnode_S *nobe);
modbusMaster_reg reg_point;           //regpoint
                              
const unsigned short crctalbeabs[] = { 
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 
};
 
unsigned short crc16(unsigned char *ptr, unsigned short len) 
{
	unsigned short crc = 0xffff; 
	unsigned short i;
	unsigned char ch;
 
	for (i = 0; i < len; i++) {
		ch = *ptr++;
		crc = crctalbeabs[(ch ^ crc) & 15] ^ (crc >> 4);
		crc = crctalbeabs[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
	} 
	
	return crc;
}

unsigned char modbus_decode(
                            list_modbusSlaveNode_S *nobe
                            ,unsigned char localaddr
                            )
{
	if((*nobe).rxLength()>3)
	{
		unsigned short length;
		if((*nobe).msg[0]==localaddr)
		{
			switch((*nobe).msg[1])
			{
				case 0:
					
				break;

				case 1:
				break;

				case 2:
				break;

				case 3:

					if((*nobe).rxLength()>=8)
					{


					  if(((*nobe).msg[7]<<8|(*nobe).msg[6])==crc16((*nobe).msg,6))
					  {
							unsigned short start_addr=(*nobe).msg[2]<<8|(*nobe).msg[3];
							unsigned short databuf;
							unsigned short crc16_r;
              length=(*nobe).msg[4]<<8|(*nobe).msg[5];
              if(length>123)
              {
                memset((*nobe).msg,0xff,8);
                (*nobe).txlength=0;
                return true;
              };
              (*nobe).msg[2]=length*2;
              (*nobe).txlength=(*nobe).msg[2]+5;
              for(unsigned char i=0;i<length;i++)
              {
			        	(*nobe).reg(start_addr+i,modbus_regRead,modbus_regType,&databuf);
                (*nobe).msg[3+i*2]=(databuf>>8)&0xff;
                (*nobe).msg[4+i*2]=databuf&0xff;
              }
              crc16_r=crc16((*nobe).msg,(*nobe).msg[2]+3);
              (*nobe).msg[(*nobe).msg[2]+4]=(crc16_r>>8)&0xff;
              (*nobe).msg[(*nobe).msg[2]+3]=crc16_r&0xff;
              (*nobe).modbus_slaverSend((*nobe).msg,(*nobe).txlength);
					  	return true;
					  }
					}
				break;

				case 4:
				break;

				case 5:
				break;
        case  0x10:
					if((*nobe).rxLength()>=(((*nobe).msg[6])+6))
					{
            
            if((*nobe).msg[6]!=(((*nobe).msg[4]<<8|(*nobe).msg[5])*2))
            {
              (*nobe).msg[0]=0x03;
              (*nobe).txlength=0x01;
              return true;
            }
            
				   	if(((*nobe).msg[8+(*nobe).msg[6]]<<8|(*nobe).msg[7+(*nobe).msg[6]])==crc16((*nobe).msg,(*nobe).msg[6]+7))
				   	{
				   	  unsigned short start_addr=(*nobe).msg[2]<<8|(*nobe).msg[3];
				   	  unsigned short databuf;
				   	  unsigned short crc16_r;
              length=(*nobe).msg[4]<<8|(*nobe).msg[5];
              if(length>123)
              {
                memset((*nobe).msg,0xff,8);  
                (*nobe).msg[0]=0x02;
                (*nobe).txlength=0x01;
                return true;
              }; 
                for(unsigned char i=0;i<length;i++)
                {
			          	databuf=((*nobe).msg[7+i*2]<<8|(*nobe).msg[8+i*2]);
                  (*nobe).reg(start_addr+i,modbus_regWrite,modbus_regType,&databuf);
                }
                crc16_r=crc16((*nobe).msg,6);
          
                (*nobe).msg[6]=crc16_r&0xff;       
                (*nobe).msg[7]=(crc16_r>>8)&0xff;       
                
                (*nobe).txlength=0x08  ;
                
                (*nobe).modbus_slaverSend((*nobe).msg,(*nobe).txlength);
                return true;
            }              
          }
        break;
				default:
          (*nobe).msg[0]=0x01;
          (*nobe).txlength=0x01;
				break;
			}
		}
	}
	return false;
}


unsigned char modbus_ask(
                          list_modbusnode_S *nobe,
                          unsigned char slaveaddr,
                          unsigned char ask_fun,
                          unsigned short ask_length, //received msg length
                          unsigned short ask_regaddr //decode result
                        )
{
  unsigned short crc16_r,buf;
  (*nobe).askSlaveAddr=slaveaddr;
  (*nobe).msg[0]=slaveaddr;
  switch(ask_fun)
  {
    case 0:					
    break;
    
    case 1:
    break;
    
    case 2:
    break;
    
    case 3:
      
      (*nobe).askFun=ask_fun;
      (*nobe).msg[1]=ask_fun;
    
      (*nobe).askRegAddr=ask_regaddr;
      buf=modbus_endian(ask_regaddr);
    
      (*nobe).askLength=ask_length;
      memcpy(&(*nobe).msg[2],&buf,2);
    
      buf=modbus_endian(ask_length);
      memcpy(&(*nobe).msg[4],&buf,2); 
      crc16_r=crc16((*nobe).msg,6);
      (*nobe).msg[6]=crc16_r&0xff;       
      (*nobe).msg[7]=(crc16_r>>8)&0xff;       
      (*nobe).txlength=8;             
    break;
    
    case 4:
    break;
    
    case 5:
    break;
    
    case  0x10:
    break;
    
    default:
    break;
  }
  (*nobe).modbus_masterSend((*nobe).msg,(*nobe).txlength);
  return true;
	
}

unsigned char modbus_write(
                          list_modbusnode_S *nobe,
                          unsigned char slaveaddr,
                          unsigned char ask_fun,
                          unsigned short ask_length, //received msg length
                          unsigned short ask_startRegaddr, //decode result
                          unsigned short *ask_data
                        )
{
  unsigned short crc16_r,buf;
  (*nobe).askSlaveAddr=slaveaddr;
  (*nobe).msg[0]=slaveaddr;
  switch(ask_fun)
  {
    case 0:					
    break;
    
    case 1:
    break;
    
    case 2:
    break;
    

    
    case 4:
    break;
    
    case 5:
    break;
    
    case  0x10:
    
      (*nobe).askFun=ask_fun;
      (*nobe).msg[1]=ask_fun;
    
      (*nobe).askRegAddr=ask_startRegaddr;
      buf=modbus_endian(ask_startRegaddr);
    
      (*nobe).askLength=ask_length;
      memcpy(&(*nobe).msg[2],&buf,2);
    
      buf=modbus_endian(ask_length);
      memcpy(&(*nobe).msg[4],&buf,2); 
    
      
      memcpy(&(*nobe).msg[6],&ask_data,ask_length); 
    
      crc16_r=crc16((*nobe).msg,6+ask_length*2);
      (*nobe).msg[6+ask_length*2]=crc16_r&0xff;       
      (*nobe).msg[7+ask_length*2]=(crc16_r>>8)&0xff;       
      (*nobe).txlength=ask_length*2+8;            
    
    break;
    
    default:
    break;
  }
  (*nobe).modbus_masterSend((*nobe).msg,(*nobe).txlength);
  return true;
	
}

unsigned char modbusMaster_Init(
                          list_modbusnode_S *nobe
                          ,modbusMaster_reg reg_data
                          ,unsigned char * recePoint
                          ,unsigned char (* receLength)(void)
                          ,unsigned char bufLength
                          )
{
  (*nobe).reg=reg_data;
  (*nobe).msg=recePoint;
//  (*nobe).receLength=receLength;
  (*nobe).bufLength=bufLength;
  return true;
}

unsigned char modbusSlaver_Init(
                          list_modbusnode_S *nobe
                          ,modbusMaster_reg reg_data
                          ,unsigned char * recePoint
                          ,unsigned char (* receLength)(void)
                          ,unsigned char bufLength
                          )
{
  (*nobe).reg=reg_data;
  (*nobe).msg=recePoint;
//  (*nobe).receLength=receLength;
  (*nobe).bufLength=bufLength;
  return true;
}

unsigned char modbusMasterProcess(list_modbusnode_S *nobe)
{
  switch((*nobe).askFun)
  {
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
      if(((unsigned char)(*nobe).rxLength())>(((*nobe).askLength)*2+4))
      {
        
        unsigned short crc16_r=0;
        crc16_r=crc16((*nobe).msg,((*nobe).askLength)*2+4);
				if(((*nobe).msg[(*nobe).askLength*2+5]<<8|(*nobe).msg[(*nobe).askLength*2+4])==crc16_r)
        reg_point(
                  (*nobe).askSlaveAddr
                  ,(*nobe).askRegAddr
                  ,modbus_regWrite
                  ,modbus_regType
                  ,(unsigned short *)(*nobe).msg);
      }
    break;
    case 4:
    break;
    case 5:
    break;
    case 6:
    break;
    default:
    break;
  }
  return 0;

}

unsigned char modbus_slaverProcess(list_modbusSlaveNode_S *nobe)
{
    (*nobe).deviceProcess();
    if((*nobe).msg[0]==(*nobe).SlaveAddr)
    {
        unsigned char askNum=0,rxcnt=0;
        switch((*nobe).msg[1])
        {
            case 0x00:
                break;
            case 0x01:
                break;
            case 0x02:
                break;
            case 0x03:
                if(((unsigned char)(*nobe).rxLength())>7)
                {
                    unsigned short crc16_r=0;
                    crc16_r=crc16((*nobe).msg,6);
                    
                    if(((*nobe).msg[7]<<8|(*nobe).msg[6])==crc16_r)
                    {
                        unsigned char send_buf[256];
                        unsigned short buf16=0;
                        askNum=((*nobe).msg[4]<<8)|(*nobe).msg[5];
                        (*nobe).askRegAddr=((*nobe).msg[2]<<8)|(*nobe).msg[3];
                        if(askNum>123)
                        { 
                            memset(nobe,0xff,8);
                            return true;
                        };
                        memset((*nobe).msg,0,askNum*2+5);
                        send_buf[0]=(*nobe).SlaveAddr;
                        send_buf[1]=3;
                        send_buf[2]=askNum*2;
                        for(unsigned char i=0;i<askNum;i++)
                        {
                            (*nobe).reg(
                                        (*nobe).askRegAddr+i
                                        ,modbus_regRead
                                        ,modbus_regType
                                        ,&buf16
                                        );
                             send_buf[3+i*2]=(buf16>>8)&0xff;
                             send_buf[4+i*2]=buf16&0xff;
                        }
                        crc16_r=crc16(send_buf,send_buf[2]+3);
                        send_buf[send_buf[2]+4]=(crc16_r>>8)&0xff;
                        send_buf[send_buf[2]+3]=crc16_r&0xff;
                        (*nobe).modbus_slaverSend(send_buf,askNum*2+5);
                        return true;
                    }
                    
                }
                break;
            case 0x04:
                break;
            case 0x05:
                break;      
            case 0x06:
                break;
            case  0x10:
                rxcnt=(*nobe).rxLength();
                if(rxcnt>=(((*nobe).msg[6])+6)
                   &&rxcnt>9 
                   )       //接收长度大于写入字节数加固定格式长度
                {
                    unsigned char send_buf[256];  
                    if(((*nobe).msg[8+(*nobe).msg[6]]<<8|(*nobe).msg[7+(*nobe).msg[6]])==crc16((*nobe).msg,(*nobe).msg[6]+7))
                    {
                        unsigned short start_addr=(*nobe).msg[2]<<8|(*nobe).msg[3];
                        unsigned short databuf=0;
                        unsigned short crc16_r=0;
                                  
                        if((*nobe).msg[6]!=(((*nobe).msg[4]<<8|(*nobe).msg[5])*2))      //写入字节数和写入寄存器数不匹配
                        {
                            send_buf[0]=0x03;
                            (*nobe).modbus_slaverSend(send_buf,0x01);
                            return true;
                        }
//                        unsigned short length=0;
//                        length=(*nobe).msg[4]<<8|(*nobe).msg[5];
                        askNum=((*nobe).msg[4]<<8)|(*nobe).msg[5];
                        if(askNum>123)
                        {
                            memset((*nobe).msg,0xff,8);  
                            send_buf[0]=0x02;
                            (*nobe).modbus_slaverSend(send_buf,0x01);
                            return true;
                        }
                        memcpy(send_buf,(*nobe).msg,askNum*2+9);
                        memset((*nobe).msg,0,askNum*2+9);
                        
                        for(unsigned char i=0;i<askNum;i++)
                        {
                            databuf=(send_buf[7+i*2]<<8|send_buf[8+i*2]);
                            (*nobe).reg(
                                        start_addr+i
                                        ,modbus_regWrite
                                        ,modbus_regType
                                        ,&databuf
                                        );
                        }
          
                        crc16_r=crc16(send_buf,6);      
                        send_buf[6]=crc16_r&0xff;       
                        send_buf[7]=(crc16_r>>8)&0xff;  
                        
                        (*nobe).modbus_slaverSend(send_buf,8);
                
                        return true;
                    }              
                }
                break;
            default:
                break;
        }
    }
    return false;
}


unsigned char modbus_masterProcess(list_modbusnode_S *nobe)
{
  switch((*nobe).askFun)
  {
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
      if(((unsigned char)(*nobe).rxLength())>(((*nobe).askLength)*2+4))
      {
        
        unsigned short crc16_r=0;
        crc16_r=crc16((*nobe).msg,((*nobe).askLength)*2+3);

				if(((*nobe).msg[(*nobe).askLength*2+4]<<8|(*nobe).msg[(*nobe).askLength*2+3])==crc16_r)
        {   
          unsigned short buf16=0;
          for(unsigned char i=0;i<(*nobe).askLength;i++)
          {
            buf16=*(unsigned short*)&(*nobe).msg[3+i*2];
            buf16=modbus_endian(buf16);
            (*nobe).reg
              (
                (*nobe).askSlaveAddr
                ,(*nobe).askRegAddr+i
                ,modbus_regWrite
                ,modbus_regType
                ,&buf16
              );
          }
         
          memset((*nobe).msg,0,(*nobe).askLength*2+5);
          (*nobe).askFun=0;
          (*nobe).askLength=0;
          (*nobe).askSlaveAddr=0;
           return 1;
        }
      }
    break;
    case 4:
    break;
    case 5:
    break;
    case 6:
    break;
    default:
    break;
  }
  return 0;
}

#endif
