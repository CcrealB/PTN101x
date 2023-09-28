
#include "USER_Config.h"

#ifdef __HI2C_H

#define I2C_DBG_INFO(fmt,...)       os_printf("[I2C]"fmt, ##__VA_ARGS__)

#define	Soft_i2c

#ifndef	 SCL1
	#define SCL1	GPIO16
#endif
#ifndef	 SDA1
	#define SDA1  	GPIO17
#endif
#ifndef	 SDA2
	#define SDA2  	SDA1	// for ZY_OA_002
#endif

uint8_t SCL_IO = SCL1;
uint8_t SDA_IO = SDA1;
uint8_t SDAIN_MODE = GPIO_INPUT;
uint8_t SDAOUT_MODE = GPIO_OUTPUT;

#define	SDA_H	gpio_output(SDA_IO, 1)
#define	SDA_L	gpio_output(SDA_IO, 0)
#define	SCL_R	gpio_input(SCL_IO)
#define	SDA_R	gpio_input(SDA_IO)
#define	SetInputSDA		gpio_config_new(SDA_IO, SDAIN_MODE, GPIO_PULL_UP, GPIO_PERI_NONE)
#define	SetOutputSDA	gpio_config_new(SDA_IO, SDAOUT_MODE, GPIO_PULL_UP, GPIO_PERI_NONE)

//#define	SetInputSCL		gpio_config_new(SCL_IO, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE)
//#define	SetOutputSCL	gpio_config_new(SCL_IO, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE)

#define	SCL_H	gpio_output(SCL_IO, 1)
#define	SCL_L	gpio_output(SCL_IO, 0)

uint8_t Ack_Flag;

#ifdef Soft_i2c

int I2cSpeed = 0;	//0=555k, 1=500k, 2=434k, 3=384k, 4=344k
//*********************************************************
void I2C_Delay()
{
	if(I2cSpeed==0)	return;
	 volatile int i;
	 for(i=0;i<I2cSpeed; i++){
		 ;
	 }
}
#endif

//*********************************************************
void SET_SDA_IO(uint8_t nio)
{
	if(nio==0)	SDA_IO = SDA1;
		else	SDA_IO = SDA2;
}
//*********************************************************
void SET_I2C_IO(uint8_t scl,uint8_t sda)
{
	SCL_IO = scl;
	SDA_IO = sda;
}
//*****************************************
/*
void Wait()
{
    while (!(I2CMSST & 0x40));
    I2CMSST &= ~0x40;
	HAck_Flag = ~SDA;
}
*/

//*****************************************
void Start()
{
#ifdef Soft_i2c
	I2C_Delay();I2C_Delay();
	SDA_H;
	I2C_Delay();I2C_Delay();
	SCL_H;
	I2C_Delay();I2C_Delay();
	SDA_L;
	I2C_Delay();I2C_Delay();
	SCL_L;
	I2C_Delay();I2C_Delay();

#else
    I2CMSCR = 0x01;	//发送START命令
    Wait();
#endif
}
//*****************************************
void SendData(char dat)
{
#ifdef Soft_i2c
	uint8_t i;
	for (i=0;i<8;i++){	
		I2C_Delay();
		if ((dat & 0x80) != 0x80)	SDA_L;
	   		else SDA_H;
		dat = dat << 1;
		I2C_Delay();
		SCL_H;
		I2C_Delay();
		SCL_L;
	}
	I2C_Delay();
#else
    I2CTXD = dat;	//写数据到数据缓冲区
    I2CMSCR = 0x02;	//发送SEND命令
    Wait();
#endif
}
//*****************************************
uint8_t RecvACK()
{
#ifdef Soft_i2c
	SDA_H;
	SetInputSDA;	//yuan++
	I2C_Delay();I2C_Delay();
	SCL_H;
	I2C_Delay();
	Ack_Flag = SDA_R;
	SCL_L;
	I2C_Delay();I2C_Delay();
	SetOutputSDA;	//yuan++
	return Ack_Flag;
#else
    I2CMSCR = 0x03;	//发送读ACK命令
    Wait();
#endif
}
//*****************************************
char RecvData()
{
#ifdef Soft_i2c
	uint8_t i,temp,receivedata=0;
	SDA_H;
	SetInputSDA;	//yuan++
	for (i=0;i<8;i++){
		I2C_Delay();
		I2C_Delay();
		SCL_H;
#if 0
		uint16_t count=0;
		SetInputSCL;	//yuan++
	 	do{
			I2C_Delay();
			count++;
		}while((SCL_R==0)&&(count<5000));	// Wait for KT070x I2C Idle		Add 2018.5.29
	 	if(count==5000) DBG_LOG_INFO("==== count Over!!!\n");
	 	SetOut4putSCL;	//yuan++
#else
	 	I2C_Delay();
#endif
		temp = SDA_R;
		SCL_L;
		receivedata = (receivedata | temp);
		if (i<7){
			 receivedata = (receivedata << 1);
		}
	}
	SetOutputSDA;	//yuan++
	I2C_Delay();
	return(receivedata);
#else
    I2CMSCR = 0x04;	//发送RECV命令
    Wait();
    return I2CRXD;
#endif
}
//*****************************************
void SendACK()
{
#ifdef Soft_i2c
	SDA_L;
	I2C_Delay();I2C_Delay();
	SCL_H;
	I2C_Delay();I2C_Delay();
	SCL_L;
	I2C_Delay();I2C_Delay();
	SDA_H;
#else
    I2CMSST = 0x00;	//设置ACK信号
    I2CMSCR = 0x05;	//发送ACK命令
    Wait();
#endif
}
//*****************************************
void SendNAK()
{
#ifdef Soft_i2c
	SDA_H;
	I2C_Delay();I2C_Delay();
	SCL_H;
	I2C_Delay();I2C_Delay();
	SCL_L;
	I2C_Delay();I2C_Delay();
	SDA_H;
#else
    I2CMSST = 0x01;	//设置NAK信号
    I2CMSCR = 0x05;	//发送ACK命令
    Wait();
#endif
}
//*****************************************
void Stop()
{
#ifdef Soft_i2c
	SCL_L;
	I2C_Delay();I2C_Delay();
	SDA_L;
	I2C_Delay();I2C_Delay();
	SCL_H;
	I2C_Delay();I2C_Delay();
	SDA_H;
	I2C_Delay();I2C_Delay();
#else
    I2CMSCR = 0x06;	//发送STOP命令
    Wait();
#endif
}

//**********************************************
uint8_t I2C_WriteA8D8(uint8_t id, uint8_t addr, uint8_t Sdata)
{
	Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    if(RecvACK()){
    	I2C_DBG_INFO("WriteA8D8 Err 1  %02X\n",id);
    }else{
    	SendData(addr);			// REG ADDR
    	if(RecvACK()){
    		I2C_DBG_INFO("WriteA8D8 Err 2\n");
    	}else{
    		SendData(Sdata);	// DATA
    		if(RecvACK()){
    			I2C_DBG_INFO("WriteA8D8 Err 3\n");
    		}
    	}
    }
    Stop();						// 停止命令
    return Ack_Flag;
}
//**********************************************
uint8_t I2C_WriteA8D16(uint8_t id, uint8_t addr, uint16_t Sdata)
{
    uint8_t temp;
	Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData(addr);				//发送存储地址
    RecvACK();
	temp = (uint8_t)(Sdata>>8);
	SendData(temp);				// DATA_H
    RecvACK();
	temp = (uint8_t)(Sdata&0x00FF);
	SendData(temp);				// DATA_L
    RecvACK();
    Stop();						//发送停止命令
    return Ack_Flag;
}
//**********************************************
uint8_t I2C_WriteA16D8(uint8_t id, uint16_t addr, uint8_t Sdata)
{
	Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
	SendData(Sdata);			// DATA
    RecvACK();
    Stop();						//发送停止命令
    return Ack_Flag;
}
//**********************************************
uint8_t I2C_WriteA16D16(uint8_t id, uint16_t addr, uint16_t Sdata)
{
	Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
	SendData((Sdata>>8)&0xFF);	// DATA H
    RecvACK();
	SendData(Sdata&0xFF);		// DATA	L
    RecvACK();
    Stop();						//发送停止命令
    return Ack_Flag;
}

//************************************************
uint8_t I2C_ReadA8D8(uint8_t id, uint8_t addr)
{
	uint16_t temp;
    Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    if(RecvACK()){
        I2C_DBG_INFO("ReadA8D8 Err 1  %02X\n",id);
    }else{
    	SendData(addr);				//发送存储地址
    	if(RecvACK()){
    	    I2C_DBG_INFO("ReadA8D8 Err 2\n");
    	}else{
    		Start();					//发送起始命令
    		SendData((id<<1)|0x01);		//发送设备地址+读命令
    		if(RecvACK()){
    			I2C_DBG_INFO("ReadA8D8 Err 3\n");
    		}else{
    			temp = RecvData();
    			SendNAK();					// send NoACK
    			Stop();						// 发送停止命令
    			return temp;
    		}
    	}
    }
    Stop();
    return 0xFF;
}

//************************************************
uint16_t I2C_ReadA8D16(uint8_t id, uint8_t addr)
{
	uint16_t temp;
    Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData(addr);				//发送存储地址
    RecvACK();
    Start();					//发送起始命令
    SendData((id<<1)|0x01);		//发送设备地址+读命令
    RecvACK();
	temp = RecvData();
	SendACK();					// send ACK
	temp = (temp<<8) + RecvData(); 
    SendNAK();					// send NoACK	
    Stop();						// 发送停止命令
    return temp;
}

//************************************************
uint8_t I2C_ReadA16D8(uint8_t id, uint16_t addr)
{
	uint16_t temp;
    Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
    Start();					//发送起始命令
    SendData((id<<1)|0x01);		//发送设备地址+读命令
    RecvACK();
	temp = RecvData();
    SendNAK();					// send NoACK	
    Stop();						// 发送停止命令
    return temp;
}
//************************************************
uint16_t I2C_ReadA16D16(uint8_t id, uint16_t addr)
{
	uint16_t temp;
    Start();					//发送起始命令
    SendData((id<<1)&0xFE);		//发送设备地址+写命令
    RecvACK();
    SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
    Start();					//发送起始命令
    SendData((id<<1)|0x01);		//发送设备地址+读命令
    RecvACK();
	temp = RecvData();
	SendACK();					// send ACK
	temp = (temp<<8) + RecvData(); 
    SendNAK();					// send NoACK	
    Stop();						// 发送停止命令
    return temp;
}


//**********************************************
uint8_t I2C_WriteA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number)
{
    Start();		//发送起始命令
    SendData((id<<1)&0xFE);	//发送设备地址+写命令
    RecvACK();
    SendData(addr);	//发送存储地址
    RecvACK();
    do{
        SendData(*p++);
        RecvACK();
    }
    while(--number);
    Stop();			//发送停止命令
    return Ack_Flag;
}
//************************************************
void I2C_ReadA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number)
{
    Start();		//发送起始命令
    SendData((id<<1)&0xFE);	//发送设备地址+写命令
    RecvACK();
    SendData(addr);	//发送存储地址
    RecvACK();
    Start();		//发送起始命令
    SendData((id<<1)|0x01);	//发送设备地址+读命令
    RecvACK();
    do{
        *p = RecvData();
        p++;
        if(number != 1) SendACK();//send ACK
    }
    while(--number);
    SendNAK();		//send no ACK	
    Stop();			//发送停止命令
}


//**********************************************
uint8_t I2C_WriteA16Nbyte(uint8_t id, uint16_t addr, uint8_t *p, uint16_t number)
{
    Start();		//发送起始命令
    SendData((id<<1)&0xFE);	//发送设备地址+写命令
    RecvACK();
    SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
    do{
        SendData(*p++);
        RecvACK();
    }
    while(--number);
    Stop();			//发送停止命令
    return Ack_Flag;
}
//************************************************
void I2C_ReadA16Nbyte(uint8_t id, uint16_t addr, uint8_t *p, uint16_t number)
{
    Start();		//发送起始命令
    SendData((id<<1)&0xFE);	//发送设备地址+写命令
    RecvACK();
   	SendData((addr>>8)&0xFF);	//发送存储地址 H
    RecvACK();
	SendData(addr&0xFF);		//发送存储地址 L
    RecvACK();
    Start();		//发送起始命令
    SendData((id<<1)|0x01);	//发送设备地址+读命令
    RecvACK();
    do{
        *p = RecvData();
        p++;
        if(number != 1) SendACK();//send ACK
    }
    while(--number);
    SendNAK();		//send no ACK	
    Stop();			//发送停止命令
}

//**********************************************
uint8_t I2C_BkWriteA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number)
{
    uint8_t retry = 5;
    while(retry--){
    	Start();
    	SendData(id);
    	RecvACK();
    	if(Ack_Flag)	continue;
    	addr = addr<<1;
    	SendData(addr);
    	RecvACK();
    	if(Ack_Flag)	continue;

    	do{
    		SendData(*p++);
    		RecvACK();
    	}
    	while(--number);
    	Stop();			//发送停止命令
    	break;
    }
    return (~Ack_Flag)&1;
}
//************************************************
void I2C_BkReadA8Nbyte(uint8_t id, uint8_t addr, uint8_t *p, uint16_t number)
{
	uint8_t retry = 5;
    while(retry--){
    	Start();
    	SendData(id);
    	RecvACK();
    	if(Ack_Flag)	continue;
    	addr = addr<<1;
    	addr |= 0x01;
    	SendData(addr);
    	RecvACK();
    	if(Ack_Flag)	continue;
    	do{
    		*p = RecvData();
    		p++;
    		if(number != 1) SendACK();//send ACK
    	}
    	while(--number);
    	SendNAK();		//send no ACK
    	Stop();			//发送停止命令
    	break;
    }
}

//************************************************
void Hi2c_Init()  
{
#ifdef Soft_i2c
	gpio_config_new(SCL_IO, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(SDA1, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(SDA2, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	Stop();
#else
    P_SW2 |= 0x80;
//  P_SW2 |= 0x30;		//I2C功能脚选择，00:P1.5,P1.4; 01:P2.5,P2.4; 11:P3.2,P3.3
    I2CCFG = 0xFA;		//使能I2C主机模式  Bit5~0 Speed	  MsSpeed = (24M/1M/2-4)/2 = 0xC4, 400K=0xCD, 100k=0xFA, 3M=0xC0
    I2CMSST = 0x00;
#endif

#if 1
	uint8_t	i,ack;
	DBG_LOG_INFO("==== I2C ADDR TEST ====\n");
	I2cSpeed = 10;
#ifdef BK9532
	SDAIN_MODE = GPIO_INOUT;
#else
	SDAIN_MODE = GPIO_INPUT;
#endif
	for(i=0;i<127; i++){
		Start();					//发送起始命令
		SendData((i<<1)&0xFE);		//发送设备地址+写命令
		if(RecvACK()==0){
			DBG_LOG_INFO("==== id: 0x%02X   ack:%d \n", i, ack);
		}
		Stop();
		for(uint16_t j=0; j<65535; j++);
	}
	I2cSpeed = 0;
#endif

}

#endif
