
#include "USER_Config.h"
#ifdef AMG8802

#include "amg8802.h"
//#include "math.h"
//#include "stdlib.h"
//#include "stdio.h"

int16_t  gs_ADC1_CurrentOffset;        //ADC1零漂电流
int16_t  gs_ADC2_CurrentOffset;        //ADC1零漂电流

AMG8802_REG *pAMG8802MSG;

const unsigned short tempL[200]=
{
32450,30562,28807,27173,25650,24228,22899,21656,20492,19401,
18377,17416,16513,15663,14864,14111,13402,12733,12103,11508,
10945,10415, 9913, 9438, 8990, 8565, 8163, 7783, 7422, 7080,
 6757, 6450, 6158, 5882, 5619, 5370, 5133, 4909, 4695, 4492,
 4299, 4126, 3940, 3774, 3616, 3465, 3321, 3184, 3054, 2929,
 2811, 2702, 2590, 2487, 2389, 2295, 2205, 2120, 2038, 1960,
 1885, 1813, 1745, 1679, 1617, 1557, 1500, 1444, 1392, 1341,
 1293, 1247, 1202, 1160, 1119, 1079, 1042, 1006,  971,  938,
  906,  875,  845,  817,  790,  763,  738,  714,  690,  668,
  646,  625,  605,  586,  567,  549,  532,  515,  499,  484,
  469,  454,  441,  427,  414,  402,  390,  378,  367,  356,
  346,  336,  326,  316,  307,  298,  290,  282,  274,  266,
  258,  251,  244,  238,  231,  225,  219,  213,  207,  201,
  196,  191,  186,  181,  176,  172,  167,  163,  159,  155,
  151,  147,  143,  140,  136,  133,  130,  126,  123,  120,
  117,  115,  112,  109,  107,  104,  102,   99,   97,   95,
   93,   91,   89,   87,   85,   83,   81
};

const unsigned short tempH[200]=
{
31452,30133,28876,27680,26538,25452,24416,23427,22520,21586,
20728,19910,19127,18380,17667,16985,16333,15710,15115,14545,
13998,13477,12977,12500,12041,11602,11182,10778,10392,10022,
 9667, 9326, 8998, 8685, 8383, 8095, 7817, 7550, 7293, 7047,
 6811, 6583, 6363, 6153, 5951, 5756, 5568, 5388, 5210, 5047,
 4886, 4731, 4581, 4437, 4298, 4165, 4036, 3911, 3791, 3676,
 3563, 3456, 3352, 3252, 3155, 3061, 2971, 2883, 2800, 2718,
 2640, 2563, 2490, 2418, 2350, 2283, 2220, 2157, 2097, 2040,
 1983, 1928, 1876, 1826, 1776, 1728, 1682, 1637, 1593, 1552,
 1511, 1472, 1433, 1396, 1361, 1326, 1292, 1260, 1228, 1197,
 1167, 1138, 1111, 1083, 1057, 1031, 1006,  982,  958,  936,
  915,  892,  872,  852,  832,  813,  795,  777,  760,  742,
  726,  710,  695,  680
};

double tempResistance,tempResistance1,tempResistance2,tempResistance3;

/**************************************
 * @brief  char cal_crc
 * @note   CRC计算
 * @param  data：
 * @retval 
 *************************************/
uint8_t cal_crc(uint8_t* buffer, int off, int len)
{
    uint8_t crc = 0;
    int i;
    while(len-- > 0){
        crc ^= buffer[off++];		// 每次先与需要计算的数据异或,计算完指向下一数据
        for(i = 8; i > 0; --i){		// 下面这段计算过程与计算一个字节crc一样
            if((crc & 0x80) != 0){
                crc = (uint8_t)((crc << 1) ^ 0x07);
            }else{
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return (crc);
}

/******************************************************************
 * @brief  AMG8802ReadBytes
 * @note
 * @param
 * @retval 
 *******************************************************************/
uint8_t AMG8802_Read_Reg(enum AMG8802_REGREQ reg,uint16_t* value)
{
	uint8_t SpeedBak = I2cSpeed;
	I2cSpeed = 3;
	uint8_t ErrNum;

	uint8_t buff[6];
    uint8_t CRC_ReadValue;
	
    Start();						// 第1步：发起I2C总线启动信号/
    SendData(AMG8802_IIC_Write);	// 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 *//* 此处是写指令 */
	if (RecvACK() != 0){			// 第3步：等待ACK
		Stop();
		ErrNum = 1;
		goto AmgRerr;
	}
	
	SendData((uint8_t)reg);			// 第4步：发送字节地址
	if (RecvACK() != 0){			// 第5步：等待ACK
		Stop();
		ErrNum = 2;
		goto AmgRerr;
	}		
	
	Start();					// 第6步：重新启动I2C总线。前面的代码的目的向 AMG8802 传送地址以及数据长度，下面开始读取数据
	SendData(AMG8802_IIC_Read);	// 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 , 此处是读指令
	if (RecvACK() != 0){		// 第8步：等待ACK
		Stop();
		ErrNum = 3;
		goto AmgRerr;
	}	
	
//	/* 第9步：发送字节地址 */
//	i2c_SendByte((uint8_t)reg);	
//	
//	/* 第10步：等待ACK */	
//	if (i2c_WaitAck() != 0)
//	{
//		res=0u;
//		goto cmd_fail;	/* AMG8802器件无应答 */
//	}		
	
    buff[0] = 0x18 ;
    buff[1] = (unsigned char)reg ;
    buff[2] = 0x19;
	buff[3] = RecvData();	// 第11步：接收寄存器数据
	SendACK();
	buff[4] = RecvData();
	SendACK();
	buff[5] = RecvData();
	SendNAK();
	// 可加入CRC校验
    CRC_ReadValue = cal_crc(buff,0,5);
    if(CRC_ReadValue == buff[5]){
        *value = ((uint16_t)buff[3] << 8) + (uint16_t)buff[4];	
        Stop();		// 第12步：发送I2C总线停止信号
//		res = 1 ;
//        AMG_DBG_INFO("=====R ok...\n");
        return 1;
    }
    Stop();		// 第12步：发送I2C总线停止信号
    AMG_DBG_INFO("==== CRC Err!!! %02X  %02X  %02X\n",buff[3],buff[4],buff[5]);
AmgRerr:
    AMG_DBG_INFO("==== Err R%d.\n",ErrNum);
    I2cSpeed = SpeedBak;
    return 0;
}


/*******************************************************************
 * @brief  AMG8802WriteBytes
 * @note
 * @param
 * @retval 
 *******************************************************************/
uint8_t AMG8802_Write_Reg(enum    AMG8802_REGREQ reg,uint16_t value)
{
	uint8_t SpeedBak = I2cSpeed;
	I2cSpeed = 3;

	uint8_t ErrNum;

	uint8_t buff[4];
	uint8_t valueH,valueL,CRC_Value;
	
	Start();	/* 第1步：发起I2C总线启动信号 */
	SendData(AMG8802_IIC_Write);	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 *//* 此处是写指令 */
	if (RecvACK() != 0){	/* 第3步：等待ACK */
		Stop();
		ErrNum = 1;
		goto Amgerr;
	}
	
	SendData((uint8_t)reg);	/* 第4步：发送字节地址*/
	if (RecvACK() != 0){	/* 第5步：等待ACK */
		Stop();
		ErrNum = 2;
		goto Amgerr;
	}	

	valueH = (uint8_t)((value & 0xff00)>>8);
	valueL = (uint8_t)(value & 0x00ff);
	
	SendData((uint8_t)valueH);
	if (RecvACK() != 0){
		Stop();
		ErrNum = 3;
		goto Amgerr;
	}	
	
	SendData((uint8_t)valueL);
	if (RecvACK() != 0){
		Stop();
		ErrNum = 4;
		goto Amgerr;
	}	
	
	/*计算并发送CRC*/
	buff[0] = AMG8802_IIC_Write ; 
	buff[1] = reg;
	buff[2] = (value & 0xff00)>>8;
	buff[3] = (value & 0x00ff);
	CRC_Value = cal_crc(buff,0,4);
	
	SendData(CRC_Value);
	if (RecvACK() != 0){
		Stop();
		ErrNum = 5;
		goto Amgerr;
	}	
	Stop();	/* 第12步：发送I2C总线停止信号 */
	return 1;
Amgerr:
	AMG_DBG_INFO("==== Err W%d.\n",ErrNum);
	I2cSpeed = SpeedBak;
	return 0;
}

/*****************************************************
 * @brief  AMG8802_UnlockRemapRigister()
 * @retval 返回是否设定成功：1、成功  0、失败
 */
uint8_t AMG8802_UnlockRemapRigister(void)
{
	if(AMG8802_Write_Reg(REG_EFUSE_MD,0x8000) == 1){
		if(AMG8802_Write_Reg(REG_EFUSE_MD,0x8101) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}

/*****************************************************
 * @brief  AMG8802_lockRemapRigister()
 * @retval 返回是否设定成功：1、成功  0、失败
 ****************************************************/
uint8_t AMG8802_LockRemapRigister(void)
{
	if(AMG8802_Write_Reg(REG_EFUSE_MD,0x8100) == 1){
		if(AMG8802_Write_Reg(REG_EFUSE_MD,0x0100) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}


/***************************************************************
 * @brief  AMG8802_CheckPeriodSetting()
 * @note   1.读取寄存器值；2.只修改需要修改的部分，保留其他的值；3.写寄存器的值
 * @param  input parameter:用户输入检测周期；
 * @retval 返回是否设定成功
 **************************************************************/
uint8_t AMG8802_CheckPeriodSetting(uint16_t CheckPeriod)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_CBCFG,&tempdata) == 1){
		tempdata = (tempdata&0x3FFF) | (((uint16_t)CheckPeriod<<14)&0xC000);//不影响其他寄存器的情况下修改checkperiod
		if(AMG8802_Write_Reg(REG_CBCFG,tempdata) == 1){
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}


/**---------------------------------------------------------------------------------
 * @brief  AMG8802_CellCountSetting()
 * @note   
 * @param  input parameter:电池组实际电芯串数，此值不能大于17且不能小于9；
 * @retval 返回是否设定成功
 */
uint8_t AMG8802_CellCountSetting(uint8_t CellCount)
{
	uint8_t CellCount_DownLoad;
	uint16_t tempdata;
	if(CellCount>17||CellCount<3){
		return 0;
	}else{
		CellCount_DownLoad = CellCount ;
		if(AMG8802_Read_Reg(REG_CBCFG,&tempdata) == 1){
			tempdata = (tempdata & 0xF0FF) | ((CellCount_DownLoad<<8)&0x0F00);//不影响其他寄存器的情况下修改CellCount 
			if(AMG8802_Write_Reg(REG_CBCFG,tempdata) == 1){
				return 1;
			}else{
				return 0;
			}
		}else{
			return 0;
		}
	}
}


/*****************************************************
 * @brief  AMG8802_NTCCountSetting()
 * @param  input parameter:需要用户输入实际寄存器值，00代表ts0；01代表ts0和ts1;10代表ts0和ts2；11代表ts0、ts1、ts2；
 * @retval 返回是否设定成功
 ****************************************************/
uint8_t AMG8802_NTCCountSetting(uint8_t NTCCount)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_UTDCFG,&tempdata) == 1){
		tempdata = (tempdata & 0xFF3F) | ((NTCCount<<6)&0x00C0);//不影响其他寄存器的情况下修改NTCCount 
		if(AMG8802_Write_Reg(REG_UTDCFG,tempdata) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}

/***************************************************************
 * @brief  	AMG8802_BalanceSetting()
 * @param  	BalanceEnable:是否使能均衡；0 disable;1 enable;若此位为0 均衡开启电压强制置零，若此位为1 均衡开启电压置零也可以关闭均衡功能
 *         	BalanceController：均衡控制者：0 8802；1 MCU;
 *		 	BalanceOpenVoltage: 均衡开启电压：
 *			BalanceDeltaVoltage：均衡开启压差条件；
 *			BalanceMode：0：充电均衡；1：充电及静置均衡；1
 * @retval 返回是否设定成功
 */
uint8_t AMG8802_BalanceSetting(uint8_t BalanceEnable,uint8_t BalanceController,uint8_t BalanceOpenVoltage,uint8_t BalanceDeltaVoltage,uint8_t BalanceMode)
{
	uint16_t tempdata;
	if(BalanceEnable == 0){	//如果关闭均衡，则设置cb_range == 0;  其他设置忽略
		if(AMG8802_Read_Reg(REG_CBCFG,&tempdata) == 1){
			tempdata = tempdata & 0xFF80;
			if(AMG8802_Write_Reg(REG_CBCFG,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{	//开启均衡
		if(BalanceController == 1){	//判断控制者为MCU
			if(AMG8802_Read_Reg(REG_SWOPTION,&tempdata) == 1){
				//tempdata = (tempdata&0xFFFD) | 0x0002;
                tempdata = (tempdata&0xFFFD) | 0x0001;
				if(AMG8802_Write_Reg(REG_SWOPTION,tempdata) == 1)	return 1;
					else											return 0;
			}else{
				return 0;
			}
		}else{	//控制者为8802
			if(AMG8802_Read_Reg(REG_SWOPTION,&tempdata) == 1){
				tempdata = tempdata&0xFFFD;
				if(AMG8802_Write_Reg(REG_SWOPTION,tempdata) == 1)	return 1;
					else											return 0;
			}
			if(AMG8802_Read_Reg(REG_CBCFG,&tempdata) == 1){	//设置均衡开启电压，压差，和均衡模式
				tempdata = (tempdata & 0xCF00) | (BalanceDeltaVoltage << 12) | (BalanceMode << 7) | (BalanceOpenVoltage);
				if(AMG8802_Write_Reg(REG_CBCFG,tempdata) == 1)	return 1;
					else										return 0;
			}else{
				return 0;
			}
		}	
	}
}

/*****************************************************
 * @brief  AMG8802_Dsgon_inchg_Setting()
 * @note
 * @param  uint8_t state: 0 disable , 1 enable;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Dsgon_inchg_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0x7FFF) | 0x8000;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0x7FFF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Chgon_indsg_Setting()
* @param  uint8_t state: 0 disable , 1 enable;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Chgon_indsg_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xBFFF) | 0x4000;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xBFFF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Coen_Setting()
 * @note   cell connection is open
 * @param  uint8_t state: 0 disable , 1 enable;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Coen_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xDFFF) | 0x2000;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xDFFF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_ChgDrvCrrt_Setting()
 * @note   select charge mosfet driver current
 * @param  uint8_t ChgDrvCrrt: 00:10uA ;01: 20uA ; 10:30uA ; 11:40uA
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_ChgDrvCrrt_Setting(uint8_t ChgDrvCrrt)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
		tempdata = (tempdata & 0xF3FF) | ((ChgDrvCrrt<<10)&0x0C00);
		if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}

/**---------------------------------------------------------------------------------
 * @brief  AMG8802_TsDelay_Setting()
 * @note   select ts  delay
 * @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_TsDelay_Setting(uint8_t TsDelay)
{
	uint16_t tempdata;
	if(TsDelay == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFEFF) | 0x0100;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFEFF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Adc1CrrtLsb_Setting()
 * @note   select adc1 curretn lsb
* @param  uint8_t Adc1CrrtLsb: 00/01: 14bit; 10:16bit;  11:18bit
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Adc1CrrtLsb_Setting(uint8_t Adc1CrrtLsb)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
		tempdata = (tempdata & 0xFF3F) | ((Adc1CrrtLsb<<6)&0x00C0);
		if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}

/*****************************************************
 * @brief  AMG8802_LDChk_Setting()
 * @note
 * @param  uint8_t state: 0 disable , 1 enable;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_LDChk_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFFDF) | 0x0020;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFFDF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Adc1VltgLsb_Setting()
 * @note
* @param  uint8_t state: 0: 14bit , 1: 16bit;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Adc1VltgLsb_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFFEF) | 0x0010;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
			tempdata = (tempdata & 0xFFEF) ;
			if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Adc2CrrtLsb_Setting()
* @param  uint8_t state: 0: 18bit , 1: 20bit;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Adc2CrrtLsb_Setting(uint8_t state)
{
	uint16_t tempdata;
	if(state == 1){
		if(AMG8802_Read_Reg(REG_ADC2EN,&tempdata) == 1){
			tempdata = (tempdata & 0xFFFD) | 0x0002;
			if(AMG8802_Write_Reg(REG_ADC2EN,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_ADC2EN,&tempdata) == 1){
			tempdata = (tempdata & 0xFFFD) ;
			if(AMG8802_Write_Reg(REG_ADC2EN,tempdata) == 1)	return 1;
				else										return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_Indsgth_Setting()
 * @param  uint8_t Indsgth: 00 :2.5mV; 01:5mV ; 10:7.5mV ;  11: 10mV;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Indsgth_Setting(uint8_t Indsgth)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_OPTION,&tempdata) == 1){
		tempdata = (tempdata & 0xFFF3) | ((Indsgth<<2)&0x000C);
		if(AMG8802_Write_Reg(REG_OPTION,tempdata) == 1)	return 1;
			else										return 0;
	}else{
		return 0;
	}
}

/*****************************************************
 * @brief  AMG8802_SleepMode_Setting()
 * @param  uint8_t SleepMode: 00 :dont support sleep mode; 01:supprot sleep mode when 8802 into idle,and chg/dsg is closed ;  10/11: support sleep mode ,chg/dsg is opened;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_SleepMode_Setting(uint8_t SleepMode)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_CFGLOCK,&tempdata) == 1){
		tempdata = (tempdata & 0xFF3F) | ((SleepMode<<6)&0x00C0);
		if(AMG8802_Write_Reg(REG_CFGLOCK,tempdata) == 1)	return 1;
			else											return 0;
	}else{
		return 0;
	}
}

/*****************************************************
 * @brief  AMG8802_BuckVoltage_Setting()
* @param  uint8_t Outputstate: 0: 3.3V , 1: 5V;
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_BuckVoltage_Setting(uint8_t Outputstate)
{
	uint16_t tempdata;
	if(Outputstate == 1){
		if(AMG8802_Read_Reg(REG_CFGLOCK,&tempdata) == 1){
			tempdata = (tempdata & 0xFFDF) | 0x0020;
			if(AMG8802_Write_Reg(REG_CFGLOCK,tempdata) == 1)	return 1;
				else											return 0;
		}else{
			return 0;
		}
	}else{
		if(AMG8802_Read_Reg(REG_CFGLOCK,&tempdata) == 1){
			tempdata = (tempdata & 0xFFDF) ;
			if(AMG8802_Write_Reg(REG_CFGLOCK,tempdata) == 1)	return 1;
				else											return 0;
		}else{
			return 0;
		}
	}
}

/*****************************************************
 * @brief  AMG8802_IDLEThreshold_Setting()
* @param  uint8_t IDLERange: 5bit,40*(2n+1);
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_IDLEThreshold_Setting(uint8_t IDLERange)
{
	uint16_t tempdata;
	if(AMG8802_Read_Reg(REG_CFGLOCK,&tempdata) == 1){
		tempdata = (tempdata & 0xFFE0) | (IDLERange&0x001F);
		if(AMG8802_Write_Reg(REG_CFGLOCK,tempdata) == 1)	return 1;
			else											return 0;
	}else{
		return 0;
	}
}

//*****************************************************
void AMG8802_Mosfet_Operation(void)
{

}
/*****************************************************
 * @brief  AMG8802_VoltageProtectPara_Setting()
 * @note
 * @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_VoltageProtectPara_Setting(uint8_t *RxBuffer)
{
	//OverChargeThresholdSetting;
	return 1;
}
/*****************************************************
 * @brief  AMG8802_VoltageProtectPara_Read()
 * @note
* @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_VoltageProtectPara_Read(uint8_t *RxBuffer)
{
	//OverChargeThresholdRead;
	return 1;
}

//*****************************************************
uint8_t AMG8802_OverChargeThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OverChargeThreshold;
	OverChargeThreshold = (uint16_t)((RxBuffer[4]<<8) + RxBuffer[5]);
	AMG8802_Write_Reg(REG_OVCFG,OverChargeThreshold);//后续添加是否成功操作，IIC操作容易被打断而失败；
	AMG8802_DelayMs(3);
	return 1;
}
//*****************************************************
void AMG8802_OverDischargeThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OverDischargeThreshold;
	OverDischargeThreshold = (uint16_t)((RxBuffer[4]<<8) + RxBuffer[5]);
	AMG8802_Write_Reg(REG_UVCFG,OverDischargeThreshold);//后续添加是否成功操作，IIC操作容易被打断而失败；
	AMG8802_DelayMs(3);
}
//*****************************************************
void AMG8802_OTCThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OTCThreshold,OTCDelay,OTCThresholdRealease;
	uint16_t temp_OTDCFG,temp_OTCCFG;
	OTCDelay = RxBuffer[4];
	OTCThreshold = RxBuffer[5];
	OTCThresholdRealease = RxBuffer[6];
	AMG8802_Read_Reg(REG_OTDCFG,&temp_OTDCFG); 
	AMG8802_Read_Reg(REG_OTCCFG,&temp_OTCCFG); //读出原有的值，防止其他寄存器被改动；
	temp_OTDCFG = (temp_OTDCFG & 0x3fff) | ((OTCDelay<<8) & 0xC000);//设置delay
	temp_OTDCFG = (temp_OTDCFG & 0xc07f) | ((OTCThreshold<<7) & 0x3f80);//设置阈值
	AMG8802_Write_Reg(REG_OTDCFG,temp_OTDCFG);//写OTDCFG;
	AMG8802_DelayMs(3);
	temp_OTCCFG = (temp_OTCCFG & 0xf03f) |((OTCThresholdRealease<<6) & 0x0fc0);
	AMG8802_Write_Reg(REG_OTCCFG,temp_OTCCFG);//写OTCCFG;
	AMG8802_DelayMs(3);
}
//*****************************************************
void AMG8802_OTDThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OTDThreshold,OTDDelay,OTDThresholdRealease;
	uint16_t temp_OTDCFG,temp_OTCCFG;
	OTDDelay = RxBuffer[4];
	OTDThreshold = RxBuffer[5];
	OTDThresholdRealease = RxBuffer[6];
	AMG8802_Read_Reg(REG_OTDCFG,&temp_OTDCFG); 
	AMG8802_Read_Reg(REG_OTCCFG,&temp_OTCCFG); //读出原有的值，防止其他寄存器被改动；
	temp_OTDCFG = (temp_OTDCFG & 0x3fff) | ((OTDDelay<<8) & 0xC000);//设置delay
	temp_OTDCFG = (temp_OTDCFG & 0xff80) | ((OTDThreshold) & 0x007f);//设置阈值
	AMG8802_Write_Reg(REG_OTDCFG,temp_OTDCFG);//写OTDCFG;
	AMG8802_DelayMs(3);
	temp_OTCCFG = (temp_OTCCFG & 0xffc0) |((OTDThresholdRealease) & 0x003f);
	AMG8802_Write_Reg(REG_OTCCFG,temp_OTCCFG);//写OTCCFG;
	AMG8802_DelayMs(3);
}
//*****************************************************
void AMG8802_UTCThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t UTCThreshold,UTCDelay,UTCThresholdRealease;
	uint16_t temp_UTCCFG,temp_UTDCFG;
	UTCDelay = RxBuffer[4];
	UTCThreshold = RxBuffer[5];
	UTCThresholdRealease = RxBuffer[6];
	AMG8802_Read_Reg(REG_UTCCFG,&temp_UTCCFG); 
	AMG8802_Read_Reg(REG_UTDCFG,&temp_UTDCFG); //读出原有的值，防止其他寄存器被改动；
	temp_UTCCFG = (temp_UTCCFG & 0x3fff) | ((UTCDelay<<14) & 0xC000);//设置delay
	temp_UTCCFG = (temp_UTCCFG & 0xc07f) | ((UTCThreshold<<7) & 0x3f80);//设置阈值
	AMG8802_Write_Reg(REG_UTCCFG,temp_UTCCFG);//写OTDCFG;
	AMG8802_DelayMs(3);
	temp_UTDCFG = (temp_UTDCFG & 0xc0ff) |((UTCThresholdRealease<<8) & 0x3f00);
	AMG8802_Write_Reg(REG_UTDCFG,temp_UTDCFG);//写OTCCFG;
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_UTDThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t UTDThreshold,UTDDelay,UTDThresholdRealease;
	uint16_t temp_UTDCFG,temp_UTCCFG;
	UTDDelay = RxBuffer[4];
	UTDThreshold = RxBuffer[5];
	UTDThresholdRealease = RxBuffer[6];
	AMG8802_Read_Reg(REG_UTDCFG,&temp_UTDCFG); 
	AMG8802_Read_Reg(REG_UTCCFG,&temp_UTCCFG); //读出原有的值，防止其他寄存器被改动；
	temp_UTCCFG = (temp_UTCCFG & 0x3fff) | ((UTDDelay<<14) & 0xC000);//设置delay
	temp_UTCCFG = (temp_UTCCFG & 0xff80) | ((UTDThreshold) & 0x007f);//设置阈值
	AMG8802_Write_Reg(REG_UTCCFG,temp_UTCCFG);//写OTDCFG;
	AMG8802_DelayMs(3);
	temp_UTDCFG = (temp_UTDCFG & 0xffc0) |((UTDThresholdRealease) & 0x003f);
	AMG8802_Write_Reg(REG_UTDCFG,temp_UTDCFG);//写OTCCFG;
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_OCCThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OCCThreshold;
	uint16_t temp_OCCCFG;
	OCCThreshold = (RxBuffer[4]<<8)+RxBuffer[5];	
	AMG8802_Read_Reg(REG_OCCCFG,&temp_OCCCFG); //读出原有的值，防止其他寄存器被改动；
	temp_OCCCFG = (temp_OCCCFG & 0xF000) | ((OCCThreshold) & 0x0FFF);//设置delay
	AMG8802_Write_Reg(REG_OCCCFG,temp_OCCCFG);//写OTDCFG;
  AMG8802_DelayMs(3);	
}

//*****************************************************
void AMG8802_OCD1ThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OCCThreshold;
	uint16_t temp_OCDCFG;
	OCCThreshold = (RxBuffer[4]<<8)+RxBuffer[5];	
	AMG8802_Read_Reg(REG_OCDCFG,&temp_OCDCFG); //读出原有的值，防止其他寄存器被改动；
	temp_OCDCFG = (temp_OCDCFG & 0xF000) | ((OCCThreshold) & 0x0FFF);//设置delay
	AMG8802_Write_Reg(REG_OCDCFG,temp_OCDCFG);//写OTDCFG;		
	AMG8802_DelayMs(3);
}
//*****************************************************
void AMG8802_OCD2ThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t OCD2Threshold,OCD2Delay;
	uint16_t temp_OCCCFG,temp_OCDCFG;
	OCD2Threshold = RxBuffer[4];	//threshold  and  releasemode
	OCD2Delay     = RxBuffer[5]; //delay
	AMG8802_Read_Reg(REG_OCDCFG,&temp_OCDCFG);
	temp_OCDCFG = (temp_OCDCFG & 0x0dff) | ((OCD2Threshold<<8) & 0xf200);
	AMG8802_Write_Reg(REG_OCDCFG,temp_OCDCFG);
	AMG8802_DelayMs(3);
	AMG8802_Read_Reg(REG_OCCCFG,&temp_OCCCFG);
	temp_OCCCFG = (temp_OCCCFG & 0x0fff) | ((OCD2Delay<<8) & 0xf000);
	AMG8802_Write_Reg(REG_OCCCFG,temp_OCCCFG);
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_SCThresholdSetting(uint8_t *RxBuffer)
{
	uint16_t SCThreshold,SCDelay,SCThresholdRealease;
	uint16_t temp_OCDCFG,temp_OTCCFG,temp_UTDCFG;
	SCThreshold = RxBuffer[4];
	SCThresholdRealease = RxBuffer[5];
	SCDelay = RxBuffer[6];
	AMG8802_Read_Reg(REG_OCDCFG,&temp_OCDCFG); 
	AMG8802_Read_Reg(REG_OTCCFG,&temp_OTCCFG); //读出原有的值，防止其他寄存器被改动；
	AMG8802_Read_Reg(REG_UTDCFG,&temp_UTDCFG);
	temp_OCDCFG = (temp_OCDCFG & 0xfdff) | ((SCThresholdRealease<<8) & 0x0200);
	AMG8802_Write_Reg(REG_OCDCFG,temp_OCDCFG);//写OTDCFG;
	AMG8802_DelayMs(3);
	temp_OTCCFG = (temp_OTCCFG & 0x0fff) | ((SCDelay<<8) & 0xf000);
	AMG8802_Write_Reg(REG_OTCCFG,temp_OTCCFG);
	AMG8802_DelayMs(3);
	temp_UTDCFG = (temp_UTDCFG & 0x3fff) | ((SCThreshold<<8) & 0xc000);
	AMG8802_Write_Reg(REG_UTDCFG,temp_UTDCFG);
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_WakeupOperation(void) 	//AMG8802唤醒
{
	AMG8802_Write_Reg(REG_SLPWKUP,0X00CC);
	AMG8802_DelayMs(3);
}
//*****************************************************
void AMG8802_SleepOperation(void)	//AMG8802强制睡眠
{
	AMG8802_Write_Reg(REG_SLPWKUP,0x00EE);
	AMG8802_DelayMs(3);
}

//*****************************************************
int16_t AMG8802_ADC1CurrentoffsetCalibration(void)
{
    unsigned int ul_TempCurrent;
    int32_t l_TempCurrent;
    int32_t al_FilterCurrent[10] = {0} ;
    int32_t l_Sum;
    
    /*电流校准零漂	18bit   */
    AMG8802_Write_Reg(REG_SWFET,0x0000);//关闭充放电MOS管
    AMG8802_DelayMs(200);
    
    AMG8802_Write_Reg(REG_OPTION,0x00C0);//ADC1电流采集精度配置18bit
    AMG8802_DelayMs(3);
    
    for(uint8_t i = 0; i < 10; i++){
        AMG8802_Read_Reg(REG_CRRT0, (uint16_t *)&pAMG8802MSG->CRRT0);
        AMG8802_Read_Reg(REG_CRRT1, (uint16_t *)&pAMG8802MSG->CRRT1);//读取零漂电流值；
        ul_TempCurrent = (pAMG8802MSG->CRRT0.CRRT_ADC1_HIGH << 2) + (pAMG8802MSG->CRRT1.CRRT_ADC1_LOW);
        
        //18位补码转成32位补码
        if((pAMG8802MSG->CRRT0.CRRT_ADC1_HIGH & 0x8000) == 0x8000){	//判断正负值
            ul_TempCurrent |= 0xFFFC0000;
            l_TempCurrent = (int32_t)ul_TempCurrent;
        }else{
            l_TempCurrent = (int32_t)ul_TempCurrent;
        }
        al_FilterCurrent[i] = l_TempCurrent;
        AMG8802_DelayMs(1000);                           //默认125ms扫描周期；
        l_Sum += al_FilterCurrent[i];
    }
    gs_ADC1_CurrentOffset = l_Sum / 10;	               //取平均值
    AMG8802_Write_Reg(REG_SWFET,0x0007);//打开MOSFET	
    AMG8802_DelayMs(3);
    return gs_ADC1_CurrentOffset;
}  

//*****************************************************
int16_t AMG8802_ADC2CurrentoffsetCalibration(void)
{
    unsigned int ul_TempCurrent;
    int32_t l_TempCurrent;
    int32_t al_FilterCurrent[10] = {0} ;
    int32_t l_Sum;
    
    /*电流校准零漂	20bit   */
    AMG8802_Write_Reg(REG_ADC2EN,0x0001);//打开ADC2,精度配置18bit
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_SWFET,0x0000);//关闭充放电MOS管
    AMG8802_DelayMs(200);
    
    for(uint8_t i = 0; i < 10; i++){
        AMG8802_Read_Reg(REG_ADC2D0, (uint16_t *)&pAMG8802MSG->ADC2D0);
        AMG8802_Read_Reg(REG_ADC2D1, (uint16_t *)&pAMG8802MSG->ADC2D1);//读取零漂电流值；
        ul_TempCurrent = (pAMG8802MSG->ADC2D0.FINAL_ADC2_RESULT << 4) + (pAMG8802MSG->ADC2D1.FINAL_ADC2_RESULT );
        
        //18位补码转成32位补码
        if((pAMG8802MSG->ADC2D0.FINAL_ADC2_RESULT & 0x8000) == 0x8000){	//判断正负值
            ul_TempCurrent |= 0xFFF00000;
            l_TempCurrent = (int32_t)ul_TempCurrent;
        }else{
            l_TempCurrent = (int32_t)ul_TempCurrent;
        }
        
        al_FilterCurrent[i] = l_TempCurrent;
        AMG8802_DelayMs(10000);                           //默认62.5ms扫描周期；
        l_Sum += al_FilterCurrent[i];
    }
    
    gs_ADC2_CurrentOffset = l_Sum / 10;	               //取平均值
    AMG8802_Write_Reg(REG_SWFET,0x0007);//打开MOSFET	
    AMG8802_DelayMs(3); 
    return gs_ADC2_CurrentOffset;
} 

/*初始化8802*/
void  AMG8802_Init(AMG8802_REG *pMSG)
{
    pAMG8802MSG = pMSG;
}

//*****************************************************
void AMG8802_WakeupByCurrentInterruptConfig(void)
{
	AMG8802_Write_Reg(REG_IE2,0xfC00);
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_ClearAllFlag(void)
{
	AMG8802_Write_Reg(REG_FLAG1,0xffff);//清除上电标志位
	AMG8802_DelayMs(3);
	AMG8802_Write_Reg(REG_FLAG2,0xffff);//清除上电标志位
	AMG8802_DelayMs(3);
	AMG8802_Write_Reg(REG_FLAG3,0xffff);//清除上电标志位
	AMG8802_DelayMs(3);
	AMG8802_Write_Reg(REG_FLAG4,0xffff);//清除上电标志位
	AMG8802_DelayMs(3);
}

//*****************************************************
void AMG8802_CloseBuck(void)
{
    AMG8802_Write_Reg(REG_BUCKDOWN,0X00A9);//清除上电标志位
    AMG8802_DelayMs(3);
}

/*****************************************************
 * @brief  AMG8802_PackVoltage_Collection 
 * @note   read PackVoltage
 * @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_PackVoltage_Collection(uint16_t *pAMG8802_PackVolt)
{
	if(AMG8802_Write_Reg(REG_ADC1REQ,0x8618) == 1){   //请求Vbat采样，精度16bit
		if(AMG8802_Read_Reg(REG_VBAT,pAMG8802_PackVolt) == 1)	return 1;
			else												return 0;
	}else{
        return 0;
    }
}

/*****************************************************
 * @brief  AMG8802_Voltage_Collection 
 * @note   read cells voltage ,
* @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t  AMG8802_Voltage_Collection(uint16_t *pAMG8802_CELL)
{
    uint16_t CellVoltage_ADC[17];
	for(uint8_t i = 0; i<17 ; i++){
		if(AMG8802_Read_Reg((enum    AMG8802_REGREQ)(REG_CELL01 + i) , &CellVoltage_ADC[i]) == 1){
            pAMG8802_CELL[i]=CellVoltage_ADC[i];
//			pAMG8802_MSG->CellV[i] = CellVoltage_ADC[i] * 0.16 * 10;
			if(pAMG8802_CELL[i] >= 65535){
				pAMG8802_CELL[i] = 65535 ;
			}
			if(i >= 16){
				return 1;
			}
		}else{
			return 0;
		}	
	}	
	return 0;
}

/*****************************************************
 * @brief  AMG8802_Temperature_Collection
 * @note   read Temperature ,
 * @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t  AMG8802_Temperature_Collection(uint16_t *pAMG8802_TEMPERATURE)
{
	AMG8802_Read_Reg(REG_TS0,&pAMG8802_TEMPERATURE[0]);
	AMG8802_Read_Reg(REG_TS1,&pAMG8802_TEMPERATURE[1]);
	AMG8802_Read_Reg(REG_TS2,&pAMG8802_TEMPERATURE[2]);
	return 1 ;//諎驭諠时return   鄢迅穴要呒脟贌謲蟼葠刍矛訑馨蟼讏俣私賱窑时謩脽墉
}

/**---------------------------------------------------------------------------------
 * @brief  AMG8802_Current_Collection 
 * @note   read Current
 * @param  
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Current1_Collection(uint16_t *pAMG8802_Current)
{	
	if(AMG8802_Read_Reg(REG_CRRT0,&pAMG8802_Current[0]) == 1)	return 0;
    if(AMG8802_Read_Reg(REG_CRRT1,&pAMG8802_Current[1]) == 1)	return 0;
    return 1;
}

/**---------------------------------------------------------------------------------
 * @brief  AMG8802_Current2_Collection 
 * @note   read Current
 * @param
 * @retval return 1 refer to sucess, 0  fail
 */
uint8_t AMG8802_Current2_Collection(uint16_t *pAMG8802_Current2)
{
	if(AMG8802_Read_Reg(REG_ADC2D0, pAMG8802_Current2) == 1){
        if(AMG8802_Read_Reg(REG_ADC2D1, &pAMG8802_Current2[1]) == 1)	return 1;
        	else														return 0;
	}else{
        return 0;
    }
}

//*******************************************************************
uint8_t  AMG8802_ConfigCollection(AMG8802_CONFIG *pAMG8802_CONFIG)
{
    AMG8802_CONFIG l_AMG8802_CONFIG;
    AMG8802_Read_Reg(REG_OVCFG,(uint16_t*)&l_AMG8802_CONFIG.OVCFG);
    AMG8802_Read_Reg(REG_UVCFG  ,(uint16_t*)&l_AMG8802_CONFIG.UVCFG);
    AMG8802_Read_Reg(REG_OCDCFG ,(uint16_t*)&l_AMG8802_CONFIG.OCDCFG);
    AMG8802_Read_Reg(REG_OCCCFG ,(uint16_t*)&l_AMG8802_CONFIG.OCCCFG);
    AMG8802_Read_Reg(REG_OTDCFG ,(uint16_t*)&l_AMG8802_CONFIG.OTDCFG);
    AMG8802_Read_Reg(REG_OTCCFG ,(uint16_t*)&l_AMG8802_CONFIG.OTCCFG);
    AMG8802_Read_Reg(REG_UTCCFG ,(uint16_t*)&l_AMG8802_CONFIG.UTCCFG );
    AMG8802_Read_Reg(REG_UTDCFG ,(uint16_t*)&l_AMG8802_CONFIG.UTDCFG );
    AMG8802_Read_Reg(REG_CBCFG  ,(uint16_t*)&l_AMG8802_CONFIG.CBCFG  );
    AMG8802_Read_Reg(REG_OPTION ,(uint16_t*)&l_AMG8802_CONFIG.OPTION );
    AMG8802_Read_Reg(REG_CFGLOCK,(uint16_t*)&l_AMG8802_CONFIG.CFGLOCK);
    *pAMG8802_CONFIG=l_AMG8802_CONFIG;
	return 0;
}
//************************************************************
void AMG8802_RegCollection(AMG8802_REG *pAMG8802_REG)
{
    AMG8802_REG l_AMG8802_REG;
    uint8_t Err=0;
    static uint8_t ReadCont=0;
if(ReadCont==0){
    Err  = AMG8802_Read_Reg(REG_CELL01,(uint16_t*)&l_AMG8802_REG.CELL[0]);
    Err &= AMG8802_Read_Reg(REG_CELL02,(uint16_t*)&l_AMG8802_REG.CELL[1]);
    Err &= AMG8802_Read_Reg(REG_CELL03,(uint16_t*)&l_AMG8802_REG.CELL[2]);
    Err &= AMG8802_Read_Reg(REG_CELL04,(uint16_t*)&l_AMG8802_REG.CELL[3]);
    Err &= AMG8802_Read_Reg(REG_CELL05,(uint16_t*)&l_AMG8802_REG.CELL[4]);
    Err &= AMG8802_Read_Reg(REG_CELL06,(uint16_t*)&l_AMG8802_REG.CELL[5]);
    Err &= AMG8802_Read_Reg(REG_CELL07,(uint16_t*)&l_AMG8802_REG.CELL[6]);
    Err &= AMG8802_Read_Reg(REG_CELL08,(uint16_t*)&l_AMG8802_REG.CELL[7]);
    Err &= AMG8802_Read_Reg(REG_CELL09,(uint16_t*)&l_AMG8802_REG.CELL[8]);
    Err &= AMG8802_Read_Reg(REG_CELL10,(uint16_t*)&l_AMG8802_REG.CELL[9]);
    Err &= AMG8802_Read_Reg(REG_CELL11,(uint16_t*)&l_AMG8802_REG.CELL[10]);
    Err &= AMG8802_Read_Reg(REG_CELL12,(uint16_t*)&l_AMG8802_REG.CELL[11]);
    Err &= AMG8802_Read_Reg(REG_CELL13,(uint16_t*)&l_AMG8802_REG.CELL[12]);
    Err &= AMG8802_Read_Reg(REG_CELL14,(uint16_t*)&l_AMG8802_REG.CELL[13]);
    Err &= AMG8802_Read_Reg(REG_CELL15,(uint16_t*)&l_AMG8802_REG.CELL[14]);
    Err &= AMG8802_Read_Reg(REG_CELL16,(uint16_t*)&l_AMG8802_REG.CELL[15]);
    Err &= AMG8802_Read_Reg(REG_CELL17,(uint16_t*)&l_AMG8802_REG.CELL[16]);
    if(Err ==1) *pAMG8802_REG = l_AMG8802_REG;
   ReadCont++;
}else if(ReadCont==1){
    Err &= AMG8802_Read_Reg(REG_ADC1REQ,(uint16_t*)&l_AMG8802_REG.ADC1REQ);
    Err &= AMG8802_Read_Reg(REG_ADC2D0,(uint16_t*)&l_AMG8802_REG.ADC2D0);
    Err &= AMG8802_Read_Reg(REG_ADC2D1,(uint16_t*)&l_AMG8802_REG.ADC2D1);
    Err &= AMG8802_Read_Reg(REG_ADC2EN,(uint16_t*)&l_AMG8802_REG.ADC2EN);
    Err &= AMG8802_Read_Reg(REG_ADC2ZERO,(uint16_t*)&l_AMG8802_REG.ADC2ZERO);
    Err &= AMG8802_Read_Reg(REG_CBSEL1,(uint16_t*)&l_AMG8802_REG.CBSEL1);
    Err &= AMG8802_Read_Reg(REG_CBSEL2,(uint16_t*)&l_AMG8802_REG.CBSEL2);
    Err &= AMG8802_Read_Reg(REG_CCH16,(uint16_t*)&l_AMG8802_REG.CCH16);
    Err &= AMG8802_Read_Reg(REG_CCL16,(uint16_t*)&l_AMG8802_REG.CCL16);
    Err &= AMG8802_Read_Reg(REG_HWID,(uint16_t*)&l_AMG8802_REG.HWID);
    Err &= AMG8802_Read_Reg(REG_COM,(uint16_t*)&l_AMG8802_REG.COM);
    if(Err ==1) *pAMG8802_REG = l_AMG8802_REG;
    ReadCont++;
}else if(ReadCont==2){
    Err &= AMG8802_Read_Reg(REG_CRRT0,(uint16_t*)&l_AMG8802_REG.CRRT0);
    Err &= AMG8802_Read_Reg(REG_CRRT1,(uint16_t*)&l_AMG8802_REG.CRRT1);
    Err &= AMG8802_Read_Reg(REG_CVER,(uint16_t*)&l_AMG8802_REG.CVER);
    Err &= AMG8802_Read_Reg(REG_FLAG1,(uint16_t*)&l_AMG8802_REG.FLAG1);
    Err &= AMG8802_Read_Reg(REG_FLAG2,(uint16_t*)&l_AMG8802_REG.FLAG2);
    Err &= AMG8802_Read_Reg(REG_FLAG3,(uint16_t*)&l_AMG8802_REG.FLAG3);
    Err &= AMG8802_Read_Reg(REG_FLAG4,(uint16_t*)&l_AMG8802_REG.FLAG4);
    Err &= AMG8802_Read_Reg(REG_IE1,(uint16_t*)&l_AMG8802_REG.IE1);
    Err &= AMG8802_Read_Reg(REG_IE2,(uint16_t*)&l_AMG8802_REG.IE2);
    Err &= AMG8802_Read_Reg(REG_IE3,(uint16_t*)&l_AMG8802_REG.IE3);
    Err &= AMG8802_Read_Reg(REG_PDSGON,(uint16_t*)&l_AMG8802_REG.PDSGON);
    Err &= AMG8802_Read_Reg(REG_PWMC,(uint16_t*)&l_AMG8802_REG.PWMC);
    Err &= AMG8802_Read_Reg(REG_REMAP,(uint16_t*)&l_AMG8802_REG.REMAP);
    if(Err ==1) *pAMG8802_REG = l_AMG8802_REG;
    ReadCont++;
}else if(ReadCont==3){
    Err &= AMG8802_Read_Reg(REG_STATUS1,(uint16_t*)&l_AMG8802_REG.STATUS1);
    Err &= AMG8802_Read_Reg(REG_STATUS2,(uint16_t*)&l_AMG8802_REG.STATUS2);
    Err &= AMG8802_Read_Reg(REG_STATUS4,(uint16_t*)&l_AMG8802_REG.STATUS4);
    Err &= AMG8802_Read_Reg(REG_STATUS5,(uint16_t*)&l_AMG8802_REG.STATUS5);
    Err &= AMG8802_Read_Reg(REG_SWCB0,(uint16_t*)&l_AMG8802_REG.SWCB0);
    Err &= AMG8802_Read_Reg(REG_SWCB1,(uint16_t*)&l_AMG8802_REG.SWCB1);
    Err &= AMG8802_Read_Reg(REG_SWCFG,(uint16_t*)&l_AMG8802_REG.SWCFG);
    Err &= AMG8802_Read_Reg(REG_SWOPTION,(uint16_t*)&l_AMG8802_REG.SWOPTION);
    Err &= AMG8802_Read_Reg(REG_TS0,(uint16_t*)&l_AMG8802_REG.TS[0]);
    Err &= AMG8802_Read_Reg(REG_TS1,(uint16_t*)&l_AMG8802_REG.TS[1]);
    Err &= AMG8802_Read_Reg(REG_TS2,(uint16_t*)&l_AMG8802_REG.TS[2]);
    Err &= AMG8802_Read_Reg(REG_VBAT,(uint16_t*)&l_AMG8802_REG.VBAT);
    Err &= AMG8802_Read_Reg(REG_VR12K,(uint16_t*)&l_AMG8802_REG.VR12K);
    Err &= AMG8802_Read_Reg(REG_EFUSE_MD,(uint16_t*)&l_AMG8802_REG.OTP_MD);
    if(Err ==1) *pAMG8802_REG = l_AMG8802_REG;
    ReadCont=0;
}
}


///**---------------------------------------------------------------------------------
// * @brief  AMG8802_Capacity_Collection 
// * @note   read Capacity
// *         
// * @param  
// *          
// * @retval return 1 refer to sucess, 0  fail
// */
//uint8_t AMG8802_Capacity_Collection(void)
//{
//	if(AMG8802_Read_Reg(0xDA,&AMG8802_CapacityH)  == 1)
//	{
//		if(AMG8802_Read_Reg(0xDB,&AMG8802_CapacityL) == 1)
//		{
//			Bms_Data.CapacityHigh = AMG8802_CapacityH ;
//			Bms_Data.CapacityLow  = AMG8802_CapacityL ;
//			return 1 ;
//		}
//		else 
//			return 0;
//	}
//	else
//		return 0;
//}


///**---------------------------------------------------------------------------------
// * @brief  AMG8802_Balance_Collection 
// * @note   read balance status
// *         
// * @param  
// *          
// * @retval return 1 refer to sucess, 0  fail
// */
//uint8_t AMG8802_Balance_Collection(void)
//{
//	if(AMG8802_Read_Reg(0xCE,&BalanceH) == 1)
//	{
//		if(AMG8802_Read_Reg(0xCF,&BalanceL) == 1)
//		{
//			Bms_Data.BalanceHigh = BalanceH ;
//			Bms_Data.BalanceLow = BalanceL ;
//			return 1 ;
//		}
//		else
//			return 0 ;
//	}
//	else
//		return 0 ; 
//}


///**---------------------------------------------------------------------------------
// * @brief  AMG8802_PackInfo_Collection 
// * @note   read pack information 
// *         
// * @param  
// *          
// * @retval return 1 refer to sucess, 0  fail
// */
//uint8_t  AMG8802_PackInfo_Collection(void)
//{
//	if(AMG8802_Read_Reg(AMG8802_REG_FLAG2,&PackStatus) == 1)
//	{
//		Bms_Data.PackInfo = PackStatus;
//		return 1;
//	}
//	else
//		return 0 ;
//}
///**---------------------------------------------------------------------------------
// * @brief  AMG8802_AlarmInfo_Collection 
// * @note   read Alarm information 
// *         
// * @param  
// *          
// * @retval return 1 refer to sucess, 0  fail
// */
//uint8_t AMG8802_AlarmInfo_Collection(void)
//{
//	if(AMG8802_Read_Reg(0xC2,&AMG8802_AlarmInfo) == 1)
//	{
//		 Bms_Data.AlarmInformation = AMG8802_AlarmInfo;
//		 return 1 ;
//	}
//	else
//		return 0 ;
//}

//******************************************************************************
void AMG8802CONFIG_DeInit(void)
{
    AMG8802_CONFIG ls_AMG8802_CONFIG;

    AMG8802_DelayMs(3);
    //过充延时8次2s，释放值40.96mV，保护值4200mV(CB)； 4500mV(EF)
    ls_AMG8802_CONFIG.OVCFG.OV_RANGE   = 190;	// 过压保护阈值        3276.8+ov_range*5.12 // =4.250v
    ls_AMG8802_CONFIG.OVCFG.OV_RLS_HYS = 20;	// 过压保护恢复阈值    3276.8+ov_range*5.12-ov_rls_hys*10.24     0为关闭过压保护  //204mv
    ls_AMG8802_CONFIG.OVCFG.OV_DT      =  1;	// 过压保护计数值	// 0~3

    //过放延时8次2s，释放值184mV，保护值2800mV；
    ls_AMG8802_CONFIG.UVCFG.UV_RANGE   = 154;	// 欠压保护阈值        1024+uv_range*10.24	// 2600mv
    ls_AMG8802_CONFIG.UVCFG.UV_RLS_HYS = 15;	// 欠压保护恢复阈值    1024+uv_range*10.24+uv_rls_hys*20.48      0为关闭过压保护
    ls_AMG8802_CONFIG.UVCFG.UV_DT      = 1;		// 欠压保护计数值

    // 放电过流1阈值30A, 负载断开释放, 放电过流1延时2s, 放电过流2阈值40A
    ls_AMG8802_CONFIG.OCDCFG.OCD1_RANGE = 32;	//放电过流1的阈值      0.32*ocd1_range
    ls_AMG8802_CONFIG.OCDCFG.OCSC_RLS   = 0;	//放电保护释放模式
    ls_AMG8802_CONFIG.OCDCFG.OCD1_DT    = 1;	//放电过流计数值
    ls_AMG8802_CONFIG.OCDCFG.OCD2_TH    = 5;//0;	//放电过流2的阈值      20+(10*ocd2_th)   ===== 1

    //充电过流阈值10.24A, 充电保护定时释放, 充电过流延时2s, 放电过流2延时100ms，
    ls_AMG8802_CONFIG.OCCCFG.OCC_RANGE  = 32;	//充电过流保护阈值      0.32*occ_range
    ls_AMG8802_CONFIG.OCCCFG.OCC_RLS    = 0;	//充电保护释放模式
    ls_AMG8802_CONFIG.OCCCFG.OCC_DT     = 1;	//充电过流保护计数值
    ls_AMG8802_CONFIG.OCCCFG.OCD2_DT    = 2;	//放电过流保护2延时阈值

    //过温延时2s，充电过温阈值67℃，放电过温阈值85℃
    ls_AMG8802_CONFIG.OTDCFG.OTD_RANGE  = 15;	//放电过温保护阈值
    ls_AMG8802_CONFIG.OTDCFG.OTC_RANGE  = 20;	//充电过温保护阈值
    ls_AMG8802_CONFIG.OTDCFG.OT_DT      = 2;	//过温保护计数值

    //短路延时91us，过温释放回滞3℃；
    ls_AMG8802_CONFIG.OTCCFG.SCD_DT      = 3;//2;              //短路保护延时  ===== 3  61us/step
    ls_AMG8802_CONFIG.OTCCFG.OTC_RLS_HYS = 18;	//充电过温释放阈值
    ls_AMG8802_CONFIG.OTCCFG.OTD_RLS_HYS = 18;	//放电过温释放阈值

    //低温延时2s，充电低温阈值0℃，放电低温阈值-35℃
    ls_AMG8802_CONFIG.UTCCFG.UTD_RANGE  = 43;	//放电低温保护阈值
    ls_AMG8802_CONFIG.UTCCFG.UTC_RANGE  = 6;	//充电低温保护阈值
    ls_AMG8802_CONFIG.UTCCFG.UT_DT      = 1;	//低温保护计数值

    //短路阈值160A,低温释放回滞3℃，设置三路温度采样
    ls_AMG8802_CONFIG.UTDCFG.UTD_RLS_HYS = 1;	//放电低温保护释放阈值
    ls_AMG8802_CONFIG.UTDCFG.TS_CFG      = 3;	//温度采样数量配置
    ls_AMG8802_CONFIG.UTDCFG.UTC_RLS_HYS = 1;	//充电低温保护释放阈值
    ls_AMG8802_CONFIG.UTDCFG.SCD_TH      = 0;	//放电短路阈值            放电过流2*(scd_th+2)

    //设置6 or 12串电芯，均衡压差40.96mV、均衡电压4.096v、充电均衡、检测周期250ms
    ls_AMG8802_CONFIG.CBCFG.CB_RANGE    = 50;    //均衡启动电压阈值  3287.04+cb_range*10.24
    ls_AMG8802_CONFIG.CBCFG.CB_CTRL     = 0;     //均衡启动模式  0:充电均衡  1:充电&静置均衡
//===============================================================
    extern uint16_t AMG8802_Totalvoltage;
    if(AMG8802_Totalvoltage>30000){
    	ls_AMG8802_CONFIG.CBCFG.CELL_COUNT  = 10;	//电芯数量设置           CELL_COUNT+2
    }else{
    	ls_AMG8802_CONFIG.CBCFG.CELL_COUNT  = 4;	//电芯数量设置           CELL_COUNT+2
    }
    ls_AMG8802_CONFIG.CBCFG.CB_DIFF     = 0;	//均衡启动压差           cb_diff*10.24
    ls_AMG8802_CONFIG.CBCFG.CHK_PERIOD  = 1;	//检测周期

    //1110 0001 0000 0000 强开MOS管，自动释放，10uA驱动能力充电管
    ls_AMG8802_CONFIG.OPTION.INDSG_TH      = 1;	// 放电电流比较阈值
    ls_AMG8802_CONFIG.OPTION.ADC1_VLTG_LSB = 1;	// adc1除电流以外精度
    ls_AMG8802_CONFIG.OPTION.LDCHK_MD      = 0;	// 负载检测模式设置
    ls_AMG8802_CONFIG.OPTION.ADC1_CRRT_LSB = 3;	// adc1电流采样精度
    ls_AMG8802_CONFIG.OPTION.TS_DLY_SEL    = 1;	// 温度测量建立时间
    ls_AMG8802_CONFIG.OPTION.OTDUTD_RLS    = 0;	// 温度保护释放条件设置
    ls_AMG8802_CONFIG.OPTION.CHG_DRV_CRRT  = 0;	// 充电mos驱动能力设置
    ls_AMG8802_CONFIG.OPTION.UV_RLS        = 0;	// 欠压保护释放条件设置
    ls_AMG8802_CONFIG.OPTION.CO_EN         = 1;	// 采样开路检测
    ls_AMG8802_CONFIG.OPTION.CHGON_INDSG   = 1;	// 放电状态强制启动充电管设置
    ls_AMG8802_CONFIG.OPTION.DSGON_INDSG   = 1;	// 充电状态强制启动放电管设置

    //0000 0000 1100 0001  闭合MOS管的休眠 设置不充不放阈值为240mA
    ls_AMG8802_CONFIG.CFGLOCK.IDLE_RANGE   = 3;	// 8802零电流标志阈值
    ls_AMG8802_CONFIG.CFGLOCK.BUCK_SEL     = 0;	// 外部BUCK电压
    ls_AMG8802_CONFIG.CFGLOCK.SLEEP_OPTION = 0;	// 睡眠启动模式配置
    ls_AMG8802_CONFIG.CFGLOCK.PCHG_RANGE   = 0;	// 预放电，预充电阈值
    ls_AMG8802_CONFIG.CFGLOCK.EDSG_CTRL    = 0;	// 外部控制MOS信号电平配置
    ls_AMG8802_CONFIG.CFGLOCK.CFG_LOCK     = 0;	// 配置flash锁定标志
    
    AMG8802_Write_Reg(REG_OVCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OVCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UVCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.UVCFG));    
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OCDCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OCDCFG));  //
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OCCCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OCCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OTDCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OTDCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OTCCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OTCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UTCCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.UTCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UTDCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.UTDCFG));  	//===
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_CBCFG,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.CBCFG));		//===
    AMG8802_DelayMs(3);
#if 0
    uint16_t reg;
    AMG8802_Read_Reg(REG_CBCFG,&reg);
    USER_DBG_INFO("====REG_OCDCFG: %04X\n",reg);
#endif
    AMG8802_Write_Reg(REG_OPTION,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.OPTION));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_CFGLOCK,AMG8802_REG_U16READ(ls_AMG8802_CONFIG.CFGLOCK)); 
    AMG8802_DelayMs(3);  
}

int16_t AMG8802_CurrentoffsetRead(void)
{
    return  gs_ADC1_CurrentOffset;
}

//温度手动检测
void AMG8802_TempCollectionTrigScan(unsigned short * p_tempdata,unsigned char ch)
{

    double tempResistance;
    uint16_t TimeOutCounts = 0;
    AMG8802_UnlockRemapRigister();
    
    switch(ch)
    {
        case 1:
            
            AMG8802_Read_Reg(REG_STATUS5, (uint16_t *)&pAMG8802MSG->STATUS5);
            if(pAMG8802MSG->STATUS5.TS0_TREF_SEL_STATUS == 1)//100uA
            {
                AMG8802_Write_Reg(REG_ADC1REQ,0x8696); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K <= 0x2500 && pAMG8802MSG->VR12K >= 0x0200)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS0,(uint16_t *)&pAMG8802MSG->TS[0]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[0] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731);  
            }
            else
            {
               AMG8802_Write_Reg(REG_ADC1REQ,0x8616); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K >= 0x2500)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS0,(uint16_t *)&pAMG8802MSG->TS[0]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[0] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731); 
            }
            break;
        case 2:
           AMG8802_Read_Reg(REG_STATUS5, (uint16_t *)&pAMG8802MSG->STATUS5);
            if(pAMG8802MSG->STATUS5.TS1_TREF_SEL_STATUS == 1)//100uA
            {
                AMG8802_Write_Reg(REG_ADC1REQ,0x8696); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   //TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K <= 0x2500 && pAMG8802MSG->VR12K >= 0x0200)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    //TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS1,(uint16_t *)&pAMG8802MSG->TS[1]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[1] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731);  
            }
            else
            {
               AMG8802_Write_Reg(REG_ADC1REQ,0x8616); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K >= 0x2500)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS1,(uint16_t *)&pAMG8802MSG->TS[1]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[1] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731); 
            }
            break;
        case 3:
           AMG8802_Read_Reg(REG_STATUS5, (uint16_t *)&pAMG8802MSG->STATUS5);
            if(pAMG8802MSG->STATUS5.TS2_TREF_SEL_STATUS == 1)//100uA
            {
                AMG8802_Write_Reg(REG_ADC1REQ,0x8696); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K <= 0x2500 && pAMG8802MSG->VR12K >= 0x0200)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS2,(uint16_t *)&pAMG8802MSG->TS[2]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[2] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731);  
            }
            else
            {
               AMG8802_Write_Reg(REG_ADC1REQ,0x8616); 
                while(pAMG8802MSG->ADC1REQ.SW_ADC1_REQ)
                {        
                   AMG8802_Read_Reg(REG_ADC1REQ, (uint16_t *)&pAMG8802MSG->ADC1REQ);
                   TimeOutCounts++;
//                   if(TimeOutCounts >= 65534)
//                   {
//                        break;
//                   }        
                } 
                TimeOutCounts = 0;
                while(pAMG8802MSG->VR12K >= 0x2500)
                {
                    AMG8802_Read_Reg(REG_VR12K,(uint16_t *)&pAMG8802MSG->VR12K);
                    TimeOutCounts ++;
//                    if(TimeOutCounts >= 65534)
//                    {
//                        break;
//                    }  
                }
                TimeOutCounts = 0;
                AMG8802_Read_Reg(REG_TS2,(uint16_t *)&pAMG8802MSG->TS[2]);
                tempResistance  = (double)(12000 * pAMG8802MSG->TS[2] / pAMG8802MSG->VR12K ); 
                * p_tempdata  =  (unsigned short)((1.0/(1.0/(273.15f+25.0f)+1.0f/3435.0f*logf(tempResistance/10000))-273.15)*10.0+2731); 
            }
            break;
        default:
            break;

    }
    AMG8802_LockRemapRigister();
     
}


//unsigned short AMG8802_TempSerch(unsigned short tempreg)
//{
//    unsigned char curNum=100,numL=0,numH=200;
//    for(unsigned char step=0;step<10;step++)
//    {
//        if(tempreg>=tempL[curNum]&&tempreg<=tempL[curNum+1])
//        {
//            return curNum;
//        }
//        else if(tempreg<=tempL[curNum]&&tempreg>=tempL[curNum-1])
//        {
//            return curNum;
//        }
//        else if(tempreg>tempL[curNum])
//        {
//            numH=curNum;
//            curNum=(curNum+numL)/2;
//        }
//        else
//        {
//            numL=curNum;
//            curNum=(curNum+numH)/2;
//        }
//    }
//    return 0;
//}

//void AMG8802_TempCollection(unsigned short * p_tempdata)
//{
//    AMG8802_STATUS5_TypeDef status_temp; 
//    signed short reg_temp=0,buf_temp=0;
//    AMG8802_Read_Reg(REG_TS0,(unsigned short *)&reg_temp);
//    AMG8802_Read_Reg(REG_STATUS5,(unsigned short *)&status_temp);
//    if(status_temp.TS0_TREF_SEL_STATUS)
//    {
//        *p_tempdata=AMG8802_TempSerch(reg_temp);
//    }
//    else
//    {
//        
//    }

//}

void AMG8802CONFIG_Set(AMG8802_CONFIG   pCfg)
{
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OVCFG,AMG8802_REG_U16READ(pCfg.OVCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UVCFG,AMG8802_REG_U16READ(pCfg.UVCFG));    
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OCDCFG,AMG8802_REG_U16READ(pCfg.OCDCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OCCCFG,AMG8802_REG_U16READ(pCfg.OCCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OTDCFG,AMG8802_REG_U16READ(pCfg.OTDCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OTCCFG,AMG8802_REG_U16READ(pCfg.OTCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UTCCFG,AMG8802_REG_U16READ(pCfg.UTCCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_UTDCFG,AMG8802_REG_U16READ(pCfg.UTDCFG));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_CBCFG,AMG8802_REG_U16READ(pCfg.CBCFG));    
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_OPTION,AMG8802_REG_U16READ(pCfg.OPTION));   
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_CFGLOCK,AMG8802_REG_U16READ(pCfg.CFGLOCK)); 
    AMG8802_DelayMs(3);  
}

#endif
