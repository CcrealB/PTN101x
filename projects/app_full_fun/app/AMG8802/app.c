
#include "USER_Config.h"
#ifdef AMG8802

#include "app.h"


//#define modbus_nodeAddr     0x0f                            //从机地址
#define AMG8802_CONFIG_ADDR 0x08010000


//struct list_usartmsg_s      gs_uart1Config;                 //串口配置信息
//list_modbusSlaveNode_S      gs_modbusSlaverNode;            //modbus 从机节点
//unsigned char               ga_rs485_buf[256];              //485数据
short                       gs16_random=0;
unsigned short              gus16_Trim11Data=0;

//uint8_t flag_b=0;//标志位
static uint16_t battery[12], batteryR[12];//电池

BMS_DATA                    gs_BMSDATA;                     //bms数据包
AMG8802_REG                 gs_AMG8802_MSG;                 //AMG8802 控制及采样数据
AMG8802_CONFIG              gs_AMG8802_CONFIG;              //AMG8802 OTP映射数据
APP_FLAG RDS_FLAG={0};                                      //应用程序流程控制标志
AMG8802_CONFIG *            pGs_AMG8802_LOCALCONFIG=(AMG8802_CONFIG *)AMG8802_CONFIG_ADDR;


void modbus_nodeInit(void); 
unsigned short node_regdata(unsigned short regAddr,unsigned char act,unsigned char type,unsigned short* data);
void AMG8802CONFIG_Init(void);
void DFECONFIG_Write(void);
unsigned char AMG8802TEMP_Get(void);
unsigned int adler32(unsigned int * pData,unsigned short length);
uint16_t temp_Triming[10];

uint8_t AMG8802_EN;
uint16_t AMG8802_Totalvoltage;	//mv
//************************************************
void APP_Init(void)
{
    
    gs_BMSDATA.R12K_100uA=1800;
    gs_BMSDATA.R12K_12uA=15266;
//    LED_Init();                                                                   //LED初始化
//    ExternGpio_Init();                                                            //外部GPIO-EDSG/ELOCK初始化
//    modbus_nodeInit();                                                            //modbus节点初始化
//    AMG8802_IICInit();                                                            //amg8802 iic初始化
//    AMG8802_Init(&gs_AMG8802_MSG);                                                //amg8802初始化
//    AMG8802_DelayMs(5000);
//    while(!AMG8802_Read_Reg(REG_CRRT0,(unsigned short *)&gs_AMG8802_MSG.CRRT0));  //采集ADC1电流值
    
    AMG8802_EN = AMG8802_UnlockRemapRigister();	//1.解锁寄存器；2.停止内部映射；
    if(AMG8802_EN){
    	AMG_DBG_INFO("====== AMG8803 Discovery ...\n");
    }else{
    	Stop();	//停止iic
    	AMG_DBG_INFO("xxxxxx AMG8803 can't find !!!\n");
    	return;
    }

//    if(((unsigned int *)pGs_AMG8802_LOCALCONFIG)[63]==adler32((unsigned int *)pGs_AMG8802_LOCALCONFIG,63)){  //校验本地配置是否曾写入
//       AMG8802CONFIG_Set(*pGs_AMG8802_LOCALCONFIG);		//导入本地配置
//    }else{
    	//=======================================================
    	uint16_t packvoltageTemp;	// 電池電壓變量
    	if(AMG8802_PackVoltage_Collection(&packvoltageTemp)){
    		AMG8802_Totalvoltage = (packvoltageTemp*3.183);
    		AMG_DBG_INFO("====== Totalvoltage1: %d mv\n",AMG8802_Totalvoltage);
    	}

    	AMG8802_DelayMs(3);
        AMG8802CONFIG_DeInit();    //AMG8802复位
//    }

        AMG8802_DelayMs(3);
        AMG8802_LockRemapRigister();	//yuan++

    AMG_DBG_INFO("====== AMG8802CONFIG_DeInit Ok...\n");

/*
    AMG8802_Read_Reg(REG_TRIM11,&gus16_Trim11Data); 
    AMG8802_Write_Reg(REG_TRIM11,0x0000);                                               //ADC1电流校准
    AMG8802_Write_Reg(REG_TRIM12,0);                                                        
    AMG8802_Write_Reg(0xE4,0x0104);                                                     //PGA关闭
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_ADC2EN,0x03);                                                     
    
    AMG8802_DelayMs(3);
    AMG8802_Read_Reg(REG_CFGLOCK,(unsigned short *)&gs_AMG8802_CONFIG.CFGLOCK); 
    AMG8802_Write_Reg(REG_CFGLOCK,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.CFGLOCK));
    AMG8802_DelayMs(3);
    
    AMG8802_Read_Reg(REG_OPTION,(unsigned short *)&gs_AMG8802_CONFIG.OPTION); 
    (*(unsigned short *)&(gs_AMG8802_CONFIG.OPTION))|=0x0002;                           //buck驱动增强
    //gs_AMG8802_CONFIG.OPTION.LDCHK_MD=0;                                              //负载检测周期配置
    AMG8802_Write_Reg(REG_OPTION,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OPTION));        //
    AMG8802_DelayMs(3);
   
    AMG8802_Write_Reg(0xe7,0x7C);                                                       //buck驱动增强
    
    AMG8802_DelayMs(3);
    AMG8802_Read_Reg(REG_OPTION,(unsigned short *)&gs_AMG8802_CONFIG.OPTION); //  保存option配置， 以下零漂校准修改了该配置
    AMG8802_ADC1CurrentoffsetSet(AMG8802_ADC1CurrentoffsetCalibration());               //ADC1零漂校准
    AMG8802_ADC2CurrentoffsetSet(AMG8802_ADC2CurrentoffsetCalibration());               //ADC2零漂校准
    AMG8802_Write_Reg(REG_OPTION,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OPTION));//  恢复option配置，
    
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_ADC2EN,0x03);// ADC2默认配置成20bit，以上零漂校准修改了该配置为18bit
    
    AMG8802_DelayMs(3);
    AMG8802_Write_Reg(REG_IE1,0x2400);                                                  //事件中断使能
    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_ADC2ZERO,0);                                                //事件中断使能
    AMG8802_DelayMs(3);
    AMG8802_ClearAllFlag();                                                             //清楚所有标志位
    
    AMG8802_DelayMs(3);
    AMG8802_LockRemapRigister();                                                        //锁定寄存器
    AMG8802_DelayMs(3);

    AMG8802TEMP_Get();
    
//    ELOCK=1;
//    EFETC=1;
    gs_BMSDATA.ADC1_Current_B = 0;
    
    gs_BMSDATA.ADC1_Current_K = 0.792832792;
    
    gs_BMSDATA.ADC2_Current_B = 0;
    gs_BMSDATA.ADC2_Current_K = 1;
*/
}

//uint16_t calculate_temp(uint32_t resistance);
//void AMG8802_RegCollection(AMG8802_REG *pAMG8802_REG);


// OVER v pAGE 79  80
//


//************************************************
void AMG82xx_Main(void)
{
	if(AMG8802_EN==0)	return;	// AMG8802 I2C UNLINE

	static uint8_t VCont = 1;
	static uint8_t ms10_Cont = 0;
	uint16_t packvoltageTemp;	// 電池電壓變量
	uint16_t MaxV;

	if(ms10_Cont==100){	// 1S Over
		ms10_Cont = 0;
		static uint8_t BattTVol, BattTVolR;
//		AMG8802_Collection();
		if(AMG8802_PackVoltage_Collection(&packvoltageTemp)){
			if(AMG8802_Totalvoltage<30000){
				MaxV = (packvoltageTemp*3.183)/6;
				BattTVol = 100-((25200-packvoltageTemp*3.183)/60);
			}else{
				MaxV = (packvoltageTemp*3.183)/12;
				BattTVol = 100-((50400-packvoltageTemp*3.183)/120);
			}
		}
#ifdef LCD_ST7789_EN
		if(BattTVolR != BattTVol){
			BattTVolR = BattTVol;
			LCD_ShowxNum(171, 214,BattTVol, 2, 16, 0);//剩余电量
			AMG_DBG_INFO("====== MaxV[]=%d\n",MaxV);
		}
#if 0	// 溫度偵測
		static uint8_t TCont = 1;
		switch(TCont){
			case  1:	temp=gs_BMSDATA.CellT[0]; break;
			case  2:	temp=gs_BMSDATA.CellT[1]; break;
			case  3:	temp=gs_BMSDATA.CellT[2]; break;
		}
		LCD_ShowxNum(149, 35, TCont, 1, 24, 0);
		LCD_ShowxNum(173, 35, temp-2731, 4, 24, 0);
		if(++TCont > 3) TCont = 1;
#endif
		uint8_t num;
		//AMG_DBG_INFO("=========================VCont=%d  flag_a=%d flag_b=%d \n",VCont,flag_a,flag_b);
		if(AMG8802_Totalvoltage<30000){
			battery[0]=(gs_AMG8802_MSG.CELL[0]*0.16-3000)/150;
			battery[1]=(gs_AMG8802_MSG.CELL[1]*0.16-3000)/150;
			battery[2]=(gs_AMG8802_MSG.CELL[13]*0.16-3000)/150;
			battery[3]=(gs_AMG8802_MSG.CELL[14]*0.16-3000)/150;
			battery[4]=(gs_AMG8802_MSG.CELL[15]*0.16-3000)/150;
			battery[5]=(gs_AMG8802_MSG.CELL[16]*0.16-3000)/150;
			num=6;
		}else{
			battery[0]=(gs_AMG8802_MSG.CELL[0]*0.16-3000)/150;
			battery[1]=(gs_AMG8802_MSG.CELL[1]*0.16-3000)/150;
			battery[2]=(gs_AMG8802_MSG.CELL[7]*0.16-3000)/150;
			battery[3]=(gs_AMG8802_MSG.CELL[8]*0.16-3000)/150;
			battery[4]=(gs_AMG8802_MSG.CELL[9]*0.16-3000)/150;
			battery[5]=(gs_AMG8802_MSG.CELL[10]*0.16-3000)/150;
			battery[6]=(gs_AMG8802_MSG.CELL[11]*0.16-3000)/150;
			battery[7]=(gs_AMG8802_MSG.CELL[12]*0.16-3000)/150;
			battery[8]=(gs_AMG8802_MSG.CELL[13]*0.16-3000)/150;
			battery[9]=(gs_AMG8802_MSG.CELL[14]*0.16-3000)/150;
			battery[10]=(gs_AMG8802_MSG.CELL[15]*0.16-3000)/150;
			battery[11]=(gs_AMG8802_MSG.CELL[16]*0.16-3000)/150;
			num=12;
		}

		for(int i=0;i<num;i++){
			if(battery[i]>65000)	battery[i]=0;
			if(batteryR[i] != battery[i]){
				batteryR[i] = battery[i];
				if(battery[i]<(MaxV-3000)/150){
					DMAimage_display(11+i*13,210,(u8*)gImage_pow_frameRed);
					LCD_DrawRectangle(10, 210,157,24, 1, GREEN);//总电量大框
				}else{
					DMAimage_display(11+i*13,210,(u8*)gImage_pow_frame);
					LCD_DrawRectangle(10, 210,157,24, 1, GREEN);//总电量大框
				}
				AMG_DBG_INFO("====== battery[%d]=%d\n",i,battery[i]);
				battery_status(num);//电池状态显示
			}
		}

/*
		if(abs(MaxV-(Vol*0.16)) >150){
			POINT_COLOR = RED;
//			LCD_ShowxNum(233, 35, VCont, 2, 24, 0);
//			LCD_ShowxNum(269, 35, Vol*0.16, 4, 24, 0);
			POINT_COLOR = WHITE;
		}else{
			POINT_COLOR = WHITE;
		}
*/
#endif
		if(++VCont > 12) VCont = 1;
	//	if(AMG8802_Totalvoltage<30000 && VCont==3)	VCont = 9;
#if 0
		AMG_DBG_INFO("====== FLAG2: %d  %d  %d  %d  %d  %d  %d\n",	gs_AMG8802_MSG.FLAG2.IDLE_FLAG,\
																	gs_AMG8802_MSG.FLAG2.CHG_FLAG , \
																	gs_AMG8802_MSG.FLAG2.DISCHG_FLAG, \
																	gs_AMG8802_MSG.FLAG2.LDON_FLAG, \
																	gs_AMG8802_MSG.FLAG2.LDOFF_FLAG, \
																	gs_AMG8802_MSG.FLAG2.CHGRIN_FLAG, \
																	gs_AMG8802_MSG.FLAG2.CHGROFF_FLAG);
		AMG_DBG_INFO("====== battery[%d]=%d\n",0,battery[0]);
#endif

		static uint8_t Chg_En, Chg_EnR=3;
		if(gs_AMG8802_MSG.FLAG2.IDLE_FLAG || gs_AMG8802_MSG.FLAG2.DISCHG_FLAG){
			AMG8802_UnlockRemapRigister();
//			AMG_DBG_INFO("====== battery[%d]=%d\n",1,battery[1]);
			if(gs_AMG8802_MSG.FLAG2.IDLE_FLAG)		AMG8802_Write_Reg(REG_FLAG2,0x0001);
			if(gs_AMG8802_MSG.FLAG2.DISCHG_FLAG)	AMG8802_Write_Reg(REG_FLAG2,0x0004);
//			AMG_DBG_INFO("====== battery[%d]=%d\n",2,battery[2]);
			AMG8802_LockRemapRigister();
			Chg_En = 0;
		}
		if(gs_AMG8802_MSG.FLAG2.CHG_FLAG){
			AMG8802_UnlockRemapRigister();
			AMG8802_Write_Reg(REG_FLAG2,0x0002);
			AMG8802_LockRemapRigister();
			Chg_En = 1;
		}
#ifdef LCD_ST7789_EN
		if(Chg_EnR != Chg_En){
			Chg_EnR = Chg_En;
			if(Chg_En)	DMAimage_display(198,214,(u8*)gImage_lightning);//充电
				else	LCD_Color_Fill(198,214,20,20,BLACK);
		}
#endif

//		if(gs_AMG8802_MSG.FLAG2.LDOFF_FLAG)	AMG8802_Write_Reg(REG_FLAG2,0x0008);
//		if(gs_AMG8802_MSG.FLAG2.LDOFF_FLAG)	AMG8802_Write_Reg(REG_FLAG2,0x0010);
//		if(gs_AMG8802_MSG.FLAG2.CHGRIN_FLAG)	AMG8802_Write_Reg(REG_FLAG2,0x0020);
//		if(gs_AMG8802_MSG.FLAG2.CHGROFF_FLAG)	AMG8802_Write_Reg(REG_FLAG2,0x0040);
	}else{
		ms10_Cont++;
		//----------------------------------
//		if(ms10_Cont==10)	convert_cellsdata_to_bmsdata();	// 100ms Over
		//---------------------------------------
//		if(ms10_Cont==50)	find_max_min_cell();			// 500ms Over

		//----------------------------------
		if(ms10_Cont==1){
		    AMG8802_ConfigCollection(&gs_AMG8802_CONFIG);	// Init  							// Conv
	//	}else if(ms10_Cont==10){
		}else if(ms10_Cont==10 || ms10_Cont==20 || ms10_Cont==30 ||ms10_Cont==40){
			AMG8802_RegCollection(&gs_AMG8802_MSG);			// Read
		}else if(ms10_Cont==50){	// 500ms Over
			AMG8802TEMP_Get();
		}
	}
}

#if 0

void rec_config_to_write(void)
{
    modbus_slaverProcess(&gs_modbusSlaverNode);
    DFECONFIG_Write();
    
    if(RDS_FLAG.FLASHOP)
    {
        RDS_FLAG.FLASHOP=0;
        PWR_UnlockANA();
        ANCTL_FHSICmd(ENABLE);
        PWR_LockANA();
          /* Erase the specified FLASH page */
        FMC_ErasePage(AMG8802_CONFIG_ADDR);
        
        /* Clear page latch */
        FMC_ClearPageLatch();
        
        /* Write data to page latch */
        for(unsigned char iter = 0; iter < 63; iter++) 
        {
            FMC->BUF[iter] = *(((unsigned int*)&gs_AMG8802_CONFIG)+iter);
        }
        FMC->BUF[63]=adler32((unsigned int *)&gs_AMG8802_CONFIG,63);
        /* Program data in page latch to the specified FLASH page */
        FMC_ProgramPage(AMG8802_CONFIG_ADDR);
    }

}
#endif


#if 0
void convert_cellsdata_to_bmsdata(void)
{
    static unsigned char adc1switch_flag=0;
    AMG8802_Read_Reg(REG_ADC2D0,(uint16_t*)&gs_AMG8802_MSG.ADC2D0);
    AMG8802_Read_Reg(REG_ADC2D1,(uint16_t*)&gs_AMG8802_MSG.ADC2D1);
    for(unsigned char i=0;i<17;i++)
    {
        gs_BMSDATA.CellV[i]=AMG8802_RegCELL2HundreduV(gs_AMG8802_MSG.CELL[i]);     //
    }
    gs16_random=gs16_random<<1;
    gs16_random&=0xfffe;
    gs16_random|=((gs_AMG8802_MSG.CRRT0.CRRT_ADC1_HIGH)&0x1);
    if(gs16_random&0x8000)
    {
        
        gs16_random=gs16_random>>12;
        gs16_random=gs16_random|0xfff0;
    
    }
    else
    {
        gs16_random=gs16_random&0x000f;
    }
    gs_BMSDATA.ADC1_Current=AMG8802_REGCurrent2mV(gs_AMG8802_MSG.CRRT0.CRRT_ADC1_HIGH,gs_AMG8802_MSG.CRRT1.CRRT_ADC1_LOW)*gs_BMSDATA.ADC1_Current_K+gs_BMSDATA.ADC1_Current_B;
    gs_BMSDATA.ADC2_Current=AMG8802_REGCurADC2ExmV(gs_AMG8802_MSG.ADC2D0.FINAL_ADC2_RESULT,gs_AMG8802_MSG.ADC2D1.FINAL_ADC2_RESULT)*gs_BMSDATA.ADC2_Current_K+gs_BMSDATA.ADC2_Current_B;
    if(gs_BMSDATA.ADC2_Current<3000
        ||gs_BMSDATA.ADC2_Current>-3000)
    {
        if(adc1switch_flag)
        {
            AMG8802_UnlockRemapRigister();                                                //1.解锁寄存器；2.停止内部映射；
            AMG8802_Write_Reg(0xE4,0x0004);                                                     //PGA关闭
            AMG8802_Write_Reg(REG_TRIM11,0);                                                    //ADC1电流校准
            AMG8802_LockRemapRigister();                                                        //锁定寄存器
        }
        adc1switch_flag=0;
        
    }
    else
    {
        if(!adc1switch_flag)
        {
            AMG8802_UnlockRemapRigister();                                                      //1.解锁寄存器；2.停止内部映射；
            AMG8802_Write_Reg(0xE4,0x0104);                                                     //PGA关闭
            AMG8802_Write_Reg(REG_TRIM11,gus16_Trim11Data);                                     //ADC1电流校准
            AMG8802_LockRemapRigister();                                                        //锁定寄存器
        }
        adc1switch_flag=1;
    }
}

void find_max_min_cell(void)	//温度测量
{
//    LED1=!LED1;
    gs_BMSDATA.CellVmaxPosition=cal_max(gs_BMSDATA.CellV,17);
    gs_BMSDATA.CellVminPosition=cal_min(gs_BMSDATA.CellV,17);

    gs_BMSDATA.CellTmaxPosition=cal_max(gs_BMSDATA.CellT,3);
    gs_BMSDATA.CellTminPosition=cal_min(gs_BMSDATA.CellT,3);//以上为自动测量，当在高低温临界点时会出现温度检测异常；

}

#endif

void AMG8802_Collection(void)
{
    AMG8802_ConfigCollection(&gs_AMG8802_CONFIG);	// Init
    AMG8802_RegCollection(&gs_AMG8802_MSG);			// Read
    AMG8802TEMP_Get();								// TEMP Conv
}

#if 0

void modbus_nodeInit(void)
{   
    gs_uart1Config.uartbuf_p=ga_rs485_buf;
    gs_uart1Config.uartbufLength=255;
    gs_uart1Config.receiveLenth=255;
    gs_uart1Config.transLength=255;
    memset(ga_rs485_buf,0,256);
    RS485_Init(&gs_uart1Config);
       
    gs_modbusSlaverNode.bufLength=255;
    gs_modbusSlaverNode.msg=ga_rs485_buf;
    gs_modbusSlaverNode.reg=node_regdata;
    gs_modbusSlaverNode.rxLength=UART1_receiveLengthNow;
    gs_modbusSlaverNode.modbus_slaverSend=UART1_TransPush;
    gs_modbusSlaverNode.SlaveAddr=modbus_nodeAddr;
    gs_modbusSlaverNode.deviceProcess=UART1_Process;
}

//void AMG8802CONFIG_Init(void)
//{
//    //过充延时8次2s，释放值40.96mV，保护值4200mV(CB)； 4500mV(EF)
//    gs_AMG8802_CONFIG.OVCFG.OV_DT=0;                //过压保护计数值
//    gs_AMG8802_CONFIG.OVCFG.OV_RANGE=45;           //过压保护阈值        3276.8+ov_range*5.12
//    gs_AMG8802_CONFIG.OVCFG.OV_RLS_HYS=10;          //过压保护恢复阈值    3276.8+ov_range*5.12-ov_rls_hys*10.24     0为关闭过压保护

//    //过放延时8次2s，释放值184mV，保护值2800mV；
//    gs_AMG8802_CONFIG.UVCFG.UV_DT=0;                //欠压保护计数值
//    gs_AMG8802_CONFIG.UVCFG.UV_RANGE=148;           //欠压保护阈值        1024+uv_range*10.24
//    gs_AMG8802_CONFIG.UVCFG.UV_RLS_HYS=10;          //欠压保护恢复阈值    1024+uv_range*10.24+uv_rls_hys*20.48      0为关闭过压保护

//    //放电过流2阈值40A,放电过流1延时2s，负载断开释放，放电过流1阈值30A
//    gs_AMG8802_CONFIG.OCDCFG.OCD1_DT=0;             //放电过流计数值
//    gs_AMG8802_CONFIG.OCDCFG.OCD1_RANGE=15;         //放电过流1的阈值      0.32*ocd1_range
//    gs_AMG8802_CONFIG.OCDCFG.OCD2_TH=0;             //放电过流2的阈值      20+10*ocd2_th
//    gs_AMG8802_CONFIG.OCDCFG.OCSC_RLS=1;            //放电保护释放模式

//    //放电过流2延时100ms，充电过流延时2s，定时释放，充电过流阈值12A
//    gs_AMG8802_CONFIG.OCCCFG.OCC_DT=0;              //充电过流保护计数值
//    gs_AMG8802_CONFIG.OCCCFG.OCC_RANGE=19;          //充电过流保护阈值      0.32*occ_range
//    gs_AMG8802_CONFIG.OCCCFG.OCC_RLS=1;             //充电保护释放模式
//    gs_AMG8802_CONFIG.OCCCFG.OCD2_DT=5;             //放电过流保护2延时阈值

//    //过温延时2s，充电过温阈值67℃，放电过温阈值85℃
//    gs_AMG8802_CONFIG.OTDCFG.OTC_RANGE=0;         //充电过温保护阈值
//    gs_AMG8802_CONFIG.OTDCFG.OTD_RANGE=0;         //放电过温保护阈值
//    gs_AMG8802_CONFIG.OTDCFG.OT_DT=0;               //过温保护计数值

//    //短路延时91us，过温释放回滞3℃；
//    gs_AMG8802_CONFIG.OTCCFG.OTC_RLS_HYS=1;         //充电过温释放阈值
//    gs_AMG8802_CONFIG.OTCCFG.OTD_RLS_HYS=1;         //放电过温释放阈值
//    gs_AMG8802_CONFIG.OTCCFG.SCD_DT=1;              //短路保护延时

//    //低温延时2s，充电低温阈值0℃，放电低温阈值-35℃
//    gs_AMG8802_CONFIG.UTCCFG.UT_DT=2;               //低温保护计数值
//    gs_AMG8802_CONFIG.UTCCFG.UTC_RANGE=10;          //充电低温保护阈值
//    gs_AMG8802_CONFIG.UTCCFG.UTD_RANGE=127;         //放电低温保护阈值

//    //短路阈值160A,低温释放回滞3℃，设置三路温度采样
//    gs_AMG8802_CONFIG.UTDCFG.SCD_TH=2;              //放电短路阈值            放电过流2*(scd_th+2)
//    gs_AMG8802_CONFIG.UTDCFG.TS_CFG=0;              //温度采样数量设置
//    gs_AMG8802_CONFIG.UTDCFG.UTC_RLS_HYS=1;         //充电低温保护释放阈值
//    gs_AMG8802_CONFIG.UTDCFG.UTD_RLS_HYS=1;         //放电低温保护释放阈值

//    //设置13串电芯，均衡压差40.96mV、均衡电压4.096v、充电均衡、检测周期250ms
//    gs_AMG8802_CONFIG.CBCFG.CB_CTRL=0;              //均衡启动模式           3287.04+cb_range*10.24
//    gs_AMG8802_CONFIG.CBCFG.CB_DIFF=2;              //均衡启动压差           cb_diff*10.24
//    gs_AMG8802_CONFIG.CBCFG.CB_RANGE=96;            //均衡启动电压阈值
//    gs_AMG8802_CONFIG.CBCFG.CELL_COUNT=15;          //电芯数量设置           CELL_COUNT+2
//    gs_AMG8802_CONFIG.CBCFG.CHK_PERIOD=1;           //检测周期

//    //1110 0001 0000 0000 强开MOS管，自动释放，10uA驱动能力充电管
//    gs_AMG8802_CONFIG.OPTION.DSGON_INDSG=1;         //充电状态强制启动放电管设置
//    gs_AMG8802_CONFIG.OPTION.CHGON_INDSG=1;         //放电状态强制启动充电管设置
//    gs_AMG8802_CONFIG.OPTION.CO_EN=1;               //采样开路检测
//    gs_AMG8802_CONFIG.OPTION.UV_RLS=0;              //欠压保护释放条件设置
//    gs_AMG8802_CONFIG.OPTION.CHG_DRV_CRRT=0;        //充电mos驱动能力设置
//    gs_AMG8802_CONFIG.OPTION.OTDUTD_RLS=0;          //温度保护释放条件设置
//    gs_AMG8802_CONFIG.OPTION.TS_DLY_SEL=1;          //温度测量建立时间
//    gs_AMG8802_CONFIG.OPTION.ADC1_CRRT_LSB=3;       //adc1电流采样精度
//    gs_AMG8802_CONFIG.OPTION.LDCHK_MD=0;            //负载检测模式设置
//    gs_AMG8802_CONFIG.OPTION.ADC1_VLTG_LSB=1;       //adc1除电流以外精度
//    gs_AMG8802_CONFIG.OPTION.INDSG_TH=0;            //放电电流比较阈值

//    //0000 0000 1100 0001  闭合MOS管的休眠 设置不充不放阈值为240mA
//    gs_AMG8802_CONFIG.CFGLOCK.CFG_LOCK=0;           //配置flash锁定标志
//    gs_AMG8802_CONFIG.CFGLOCK.EDSG_CTRL=0;          //外部控制MOS信号电平配置
//    gs_AMG8802_CONFIG.CFGLOCK.PCHG_RANGE=0;         //预放电，预充电阈值
//    gs_AMG8802_CONFIG.CFGLOCK.SLEEP_OPTION=0;       //睡眠启动模式配置
//    gs_AMG8802_CONFIG.CFGLOCK.BUCK_SEL=0;           //外部BUCK电压
//    gs_AMG8802_CONFIG.CFGLOCK.IDLE_RANGE=3;         //8802零电流标志阈值
//    
//    AMG8802_Write_Reg(REG_OVCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OVCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_UVCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.UVCFG));    
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_OCDCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OCDCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_OCCCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OCCCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_OTDCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OTDCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_OTCCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OTCCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_UTCCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.UTCCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_UTDCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.UTDCFG));   
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_CBCFG,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.CBCFG));    
//    AMG8802_DelayMs(3);
//    AMG8802_Write_Reg(REG_OPTION,AMG8802_REG_U16READ(gs_AMG8802_CONFIG.OPTION));   
//    AMG8802_DelayMs(3);
////    AMG8802_Write_Reg(REG_CFGLOCK,AMG8802_REG_U16SELECT(gs_AMG8802_CONFIG.CFGLOCK));  

//}

void Task_Lowpower(void)
{

	
}

unsigned short node_regdata(unsigned short regAddr,unsigned char act,unsigned char type,unsigned short* data)           //modbus寄存器控制函数
{	
    if(type==modbus_regType)
    {
        if(act==modbus_regRead)
        {
            uint32_t regdata=0;
            switch (regAddr)
            {
                case 0x00:
                    regdata=gs_AMG8802_MSG.VBAT*3.2/10;                                                                         //总压
                    *data=regdata;
                    break;
                case 0x01:
                    regdata=gs_BMSDATA.ADC2_Current*2+5000000;                                                                  //ADC1电流高位
                    regdata=(regdata>>16)&0XFFFF;
                    *data=regdata;
                    break;
                case 0x02:
                    regdata=gs_BMSDATA.ADC2_Current*2+4999990+gs16_random;                                                                  //ADC1电流
                    regdata=(regdata)&0XFFFF;
                    *data=regdata;
                    break;
                case 0x03:
                    regdata=gs_BMSDATA.ADC2_Current*2+5000000;                                                                  //ADC2电流
                    regdata=(regdata>>16)&0XFFFF;
                    *data=regdata;
                    break;
                case 0x04:
                    regdata=gs_BMSDATA.ADC2_Current*2+5000000;                                                                  //ADC2电流
                    regdata=(regdata)&0XFFFF;
                    *data=regdata;
                    break;
                case 0x05:
                    *data=10;                                                                                                   //SOC
                    break;
                case 0x06:
                    *data=6;                                                                                                    //SOH
                    break;
                case 0x07:
                    *data=7;                                                                                                    //CHG CAP
                    break;
                case 0x08:
                    *data=8;                                                                                                    //DSG CAP
                    break;
                case 0x09:
                    *data=0;                                                                                                    //cycle num
                    break;
                case 0x0A:
                    *data=10;                                                                                                   //bat cap
                    break;
                case 0x0B:
                    regdata=gs_BMSDATA.CellV[gs_BMSDATA.CellVminPosition];                                                      //min volt cell 
                    *data=regdata;
                    break;
                case 0x0C:
                    regdata=gs_BMSDATA.CellVminPosition;                                                                        //min volt cell position
                    *data=regdata;
                    break;
                case 0x0D:
                    regdata=gs_BMSDATA.CellV[gs_BMSDATA.CellVmaxPosition];                                                      //max volt cell 
                    *data=regdata;
                    break;
                case 0x0E:
                    regdata=gs_BMSDATA.CellVmaxPosition;                                                                        //max volt cell position
                    *data=regdata;
                    break;
                case 0x0F:
                    regdata=gs_BMSDATA.CellV[gs_BMSDATA.CellVmaxPosition]-gs_BMSDATA.CellV[gs_BMSDATA.CellVminPosition];        //max diff volt
                    *data=regdata;
                    break;
                
                case 0x10:
                    regdata=gs_BMSDATA.CellT[gs_BMSDATA.CellTmaxPosition]-1731;                                                 //max temp
                    *data=regdata;
                    break;
                case 0x11:
                    regdata=gs_BMSDATA.CellTmaxPosition;                                                                        //max temp position
                    *data=regdata;
                    break;
                case 0x12:
                    regdata=gs_BMSDATA.CellT[gs_BMSDATA.CellTminPosition]-1731;                                                 //min temp
                    *data=regdata;
                    break;
                case 0x13:
                    regdata=gs_BMSDATA.CellTminPosition;                                                                        //min temp position
                    *data=regdata;
                    break;
                case 0x16:
                    regdata=hard_vol;                                                                                           //hardware vol
                    *data=regdata;
                    break;
                case 0x17:
                    regdata=soft_vol;                                                                                           //software vol
                    *data=regdata;
                    break;
                case 0x18:
                    regdata=SN&0xffff;                                                                                          //sn
                    break;
                case 0x19:
                    regdata=(SN>>16)&0xffff;                                                                                    //sn
                    break;
                case 0x1A:
                    regdata=((SN>>16)>>16)&0xffff;                                                                              //sn
                    break;
                case 0x1B:
                    regdata=(((SN>>16)>>16)>>16)&0xffff;                                                                        //sn
                    break;
                
                
                case 0x1C:
                    regdata=gs_AMG8802_MSG.HWID;                                                                                //AMG8802 HWID
                    *data=regdata;
                    break;
                case 0x1D:
                    regdata=gs_AMG8802_MSG.CVER;                                                                                //AMG8802 CVER
                    *data=regdata;
                    break;
                case 0x1E:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCFG);                                                          //AMG8802 SWCFG
                    *data=regdata;
                    break;
                case 0x1F:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.PDSGON);                                                         //AMG8802 PDSGON
                    *data=regdata;
                    break;
                case 0x20:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG1);                                                          //AMG8802 FLAG1
                    *data=regdata;
                    break;
                case 0x21:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.IE1);                                                            //AMG8802 IE1
                    *data=regdata;
                    break;
                case 0x22:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.STATUS1);                                                        //AMG8802 STATUS1
                    *data=regdata;
                    break;
                case 0x23:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG2);                                                          //AMG8802 FLAG2 
                    *data=regdata;
                    break;
                case 0x24:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.IE2);                                                            //AMG8802 IE2
                    *data=regdata;
                    break;
                case 0x25:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.STATUS2);                                                        //AMG8802 STATUS2
                    *data=regdata;
                    break;
                case 0x26:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG3);                                                          //AMG8802 FLAG3
                    *data=regdata;
                    break;
                case 0x27:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.IE3);                                                            //AMG8802 IE3
                    *data=regdata;
                    break;
                case 0x28:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG4);                                                          //AMG8802 FLAG4
                    *data=regdata;
                    break;
                
                case 0x29:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.STATUS4);                                                        //AMG8802 STATUS4
                    *data=regdata;
                    break;
                case 0x2A:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.STATUS5);                                                        //AMG8802 STATUS5
                    *data=regdata;
                    break;
                case 0x2B:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL1);                                                         //AMG8802 CBSEL1
                    
                    *data=regdata;
                    break;
                case 0x2C:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL2);                                                         //AMG8802 CBSEL2
                    
                    *data=regdata;
                    break;
                case 0x2D:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2EN);                                                         //AMG8802 ADC2EN
                    *data=regdata;
                    break;
                case 0x2E:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2ZERO);                                                       //AMG8802 ADC2ZERO
                    *data=regdata;
                    break;
                case 0x2F:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.CCH16);                                                          //AMG8802 CCH16
                    *data=regdata;
                    break;
                case 0x30:
                    regdata=AMG8802_REG_U16READ(gs_AMG8802_MSG.CCL16);                                                          //AMG8802 CCL16
                    *data=regdata;
                    break;
                
                
                case 0x33:
                    regdata=gs_AMG8802_CONFIG.CBCFG.CHK_PERIOD;                                                                 //检测周期
                    *data=regdata;
                    break;
                case 0x34:
                    regdata=gs_AMG8802_CONFIG.CBCFG.CELL_COUNT;                                                                 //电芯串数
                    *data=regdata;
                    break;
                case 0x35:
                    regdata=gs_AMG8802_CONFIG.UTDCFG.TS_CFG;                                                                    //NTC个数
                    *data=regdata;
                    break;
                case 0x36:
                    regdata=gs_AMG8802_CONFIG.OPTION.DSGON_INDSG;                                                               //DSGON使能
                    *data=regdata;
                    break;
                case 0x37:
                    regdata=gs_AMG8802_CONFIG.OPTION.CHGON_INDSG;                                                               //CHGON使能
                    *data=regdata;
                    break;
                case 0x38:
                    //regdata=gs_AMG8802_MSG.ADC1REQ.SW_CO_SEL;                                                                 //断线检测使能
                    regdata = gs_AMG8802_CONFIG.OPTION.CO_EN;
                    *data=regdata;
                    break;
                case 0x39:
                    regdata=gs_AMG8802_CONFIG.OPTION.CHG_DRV_CRRT;                                                              //充电管驱动电流
                    *data=regdata;
                    break;
                case 0x3A:
                    regdata=gs_AMG8802_CONFIG.OPTION.TS_DLY_SEL;                                                                //温度采样建立时间
                    *data=regdata;
                    break;
                case 0x3B:
                    regdata=gs_AMG8802_CONFIG.OPTION.ADC1_CRRT_LSB;                                                             //ADC1电流采样精度
                    *data=regdata;
                    break;
                case 0x3C:
                    regdata=gs_AMG8802_CONFIG.OPTION.ADC1_VLTG_LSB;                                                             //ADC1电压采样精度
                    *data=regdata;
                    break;
                case 0x3D:
                    regdata=gs_AMG8802_MSG.ADC2EN.ADC2_LSB;                                                                     //ADC2电流采样精度
                    *data=regdata;
                    break;
                case 0x3E:
                    //regdata=gs_AMG8802_MSG.SWCFG.IDON_WKUP_EN;                                                                //负载唤醒
                    regdata = gs_AMG8802_CONFIG.OPTION.LDCHK_MD;
                    *data=regdata;
                    break;
                case 0x3F:
                    regdata=gs_AMG8802_CONFIG.OPTION.INDSG_TH;                                                                  //比较器保护值
                    *data=regdata;
                    break;
                case 0x40:
                    regdata=gs_AMG8802_CONFIG.CFGLOCK.SLEEP_OPTION;                                                             //休眠模式设置
                    *data=regdata;
                    break;
                case 0x41:
                    regdata=gs_AMG8802_CONFIG.CFGLOCK.BUCK_SEL; 
                    *data=regdata;
                                                                                                                                //BUCK电压设置(暂时不用)
                    
                    break;
                case 0x42:
                    regdata=gs_AMG8802_CONFIG.CFGLOCK.IDLE_RANGE;                                                               //IDLE阈值
                    *data=regdata;
                    break;
                case 0x43:
                    regdata=gs_AMG8802_CONFIG.OVCFG.OV_RANGE;                                                                   //过充阈值
                    *data=regdata;
                    break;
                
                
                case 0x44:
                    regdata=gs_AMG8802_CONFIG.OVCFG.OV_DT;                                                                      //过放判定次数
                    *data=regdata;
                    break;
                case 0x45:
                    regdata=gs_AMG8802_CONFIG.OVCFG.OV_RLS_HYS;                                                                 //过充释放阈值
                    *data=regdata;
                    break;
                case 0x46:
                    regdata=gs_AMG8802_CONFIG.UVCFG.UV_RANGE;                                                                   //过放阈值
                    *data=regdata;
                    break;
                case 0x47:
                    regdata=gs_AMG8802_CONFIG.UVCFG.UV_DT;                                                                      //过放判定次数
                    *data=regdata;
                                                                           
                    break;
                case 0x48:
                    regdata=gs_AMG8802_CONFIG.UVCFG.UV_RLS_HYS;                                                                 //过放释放阈值
                    *data=regdata;
                                                                            
                    break;
                case 0x49:                                                                                                      
                    regdata=gs_AMG8802_CONFIG.CFGLOCK.PCHG_RANGE;                                                               //预充阈值
                    *data=regdata;
                                                                            
                    break;
                case 0x4A:
                    regdata=gs_AMG8802_CONFIG.OCCCFG.OCC_RANGE;                                                                 //充电过流阈值
                    *data=regdata;
                
                    break;
                case 0x4B:
                    regdata=gs_AMG8802_CONFIG.OCCCFG.OCC_DT;
                    *data=regdata;
                                                                            //
                    break;
                case 0x4C:
                    regdata=gs_AMG8802_CONFIG.OCCCFG.OCC_RLS;
                    *data=regdata;
                                                                            //
                    break;
                case 0x4D:
                    regdata=gs_AMG8802_CONFIG.OCDCFG.OCD1_RANGE;
                    *data=regdata;
                    
                    break;
                case 0x4E:
                    regdata=gs_AMG8802_CONFIG.OCDCFG.OCD1_DT;
                    *data=regdata;
                    
                    break;
                case 0x4F:
                    regdata=gs_AMG8802_CONFIG.OCDCFG.OCD2_TH;
                    *data=regdata;
                    
                    break;
                case 0x50:
                    regdata=gs_AMG8802_CONFIG.OCCCFG.OCD2_DT;
                    *data=regdata;
                    
                    break;
                case 0x51:
                    regdata=gs_AMG8802_CONFIG.UTDCFG.SCD_TH;
                    *data=regdata;
                                                                            
                    break;
                case 0x52:
                    regdata=gs_AMG8802_CONFIG.OTCCFG.SCD_DT;
                    *data=regdata;
                    
                    break;
                case 0x53:
                    regdata=gs_AMG8802_CONFIG.OCDCFG.OCSC_RLS;                //
                    *data=regdata;
                    break;
                case 0x54:
                    regdata=gs_AMG8802_CONFIG.OTDCFG.OTC_RANGE;
                    *data=regdata;
                                                                            //
                    break;
                case 0x55:
                    regdata=gs_AMG8802_CONFIG.OTDCFG.OTD_RANGE;

                    *data=regdata;
                                                                            //
                    break;
                case 0x56:
                    regdata=gs_AMG8802_CONFIG.OTDCFG.OT_DT;          //放电低温
                    *data=regdata;
                                                                            //
                    break;
                case 0x57:
                    regdata=gs_AMG8802_CONFIG.OTCCFG.OTC_RLS_HYS; 
                    *data=regdata;
                    break;
                case 0x58:
                    regdata=gs_AMG8802_CONFIG.OTCCFG.OTD_RLS_HYS; 
                    *data=regdata;
                                                                            //
                    break;
                case 0x59: 
                    regdata=gs_AMG8802_CONFIG.UTCCFG.UTC_RANGE;
                    *data=regdata;
                                                                            //
                    break;
                case 0x5A:
                    regdata=gs_AMG8802_CONFIG.UTCCFG.UTD_RANGE;
                    *data=regdata;
                                                                            //
                    break;
                case 0x5B:
                    regdata=gs_AMG8802_CONFIG.UTCCFG.UT_DT;          //放电低温
                    *data=regdata;
                                                                            //
                    break;
                case 0x5C:
                    regdata=gs_AMG8802_CONFIG.UTDCFG.UTC_RLS_HYS; 
                    *data=regdata;
                                                                            //
                    break;
                case 0x5D:
                    regdata=gs_AMG8802_CONFIG.UTDCFG.UTD_RLS_HYS;  
                    *data=regdata;
                    
                    break;
                case 0x5E:
                    regdata=gs_AMG8802_CONFIG.CBCFG.CB_RANGE; 
                    *data=regdata;
                    break;
                case 0x5F:
                    regdata=gs_AMG8802_CONFIG.CBCFG.CB_DIFF; 
                    *data=regdata;
                    
                    break;
                case 0x60:
                    regdata=gs_AMG8802_CONFIG.CBCFG.CB_CTRL; 
                    *data=regdata;
                    
                    break;
                
                

                case 0x61:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x62:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x63:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x64:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x65:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x66:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x67:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x68:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x69:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6A:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6B:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6C:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6D:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6E:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x6F:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x70:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x71:
                    regdata=gs_BMSDATA.CellV[regAddr-0x61];
                    *data=regdata;
                    break;
                case 0x72:
                    regdata=gs_BMSDATA.CellT[0]-1731;
                    *data=regdata;
                    break;
                case 0x73:
                    regdata=gs_BMSDATA.CellT[1]-1731;
                    *data=regdata;
                    break;
                case 0x74:
                    regdata=gs_BMSDATA.CellT[2]-1731;
                    *data=regdata;
                    break;
        
                case 0xD0:
                    regdata=(unsigned short)(gs_BMSDATA.ADC1_Current_K*10000);
                    *data=regdata;
                    break;
                case 0xD1:
                    regdata=(unsigned short)(gs_BMSDATA.ADC1_Current_B+1000);
                    *data=regdata;
                    break;
                case 0xD2:
                    regdata=(unsigned short)(gs_BMSDATA.ADC2_Current_K*10000);
                    *data=regdata;
                    break;
                case 0xD3:
                    regdata=(unsigned short)(gs_BMSDATA.ADC2_Current_B+1000);
                    *data=regdata;
                    break;
                default:return false;
            }
        }
        else if(act==modbus_regWrite)
        {
            float buf=0;
            switch(regAddr)
            {
                
                case 0x20:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.FLAG1) = *data;                               //FLAG1清除
                    RDS_FLAG.FLAG1WRITE=1;
                    break;
                case 0x21:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.IE1) = *data;                               //FLAG1清除
                    
                    RDS_FLAG.DFEWRITE=1;
                    break;
                                               
                case 0x23:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.FLAG2) = *data;                               //FLAG2清除
                    RDS_FLAG.FLAG2WRITE=1;
                    break;
                
                case 0x24:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.IE2) = *data;                               //FLAG1清除
                    RDS_FLAG.DFEWRITE=1;
                    break;
                
                
                case 0x26:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.FLAG3) = *data;                               //FLAG3清除
                    RDS_FLAG.FLAG3WRITE=1;
                    
                    break;
                case 0x27:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.IE3) = *data;                               //FLAG1清除
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x28:
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.FLAG4) = *data;                               //FLAG4清除
                    RDS_FLAG.FLAG4WRITE=1;
                    
                    RDS_FLAG.DFEWRITE=1;
                    break;
                
                case 0x2B:
                    //*AMG8802_REG_U16pX(gs_AMG8802_MSG.CBSEL1) = *data;                               
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.SWCB0) = *data;    //手动设置均衡电芯
                
                    RDS_FLAG.DFEWRITE=1;
                    break;
                
                case 0x2C:
                    //*AMG8802_REG_U16pX(gs_AMG8802_MSG.CBSEL2) = *data;                               
                    *AMG8802_REG_U16pX(gs_AMG8802_MSG.SWCB1) = *data;    //手动设置均衡电芯
                    RDS_FLAG.DFEWRITE=1;
                    
                    break;
                case 0x2D:
                    gs_AMG8802_MSG.ADC2EN.SW_ADC2_EN = *data;               //ADC2失能
                    
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x2E:
                    gs_AMG8802_MSG.ADC2ZERO.ADC2_ZERO_RANGE = *data;       //零区设置
                    break;
                
                case 0x31:
                    gs_AMG8802_MSG.ADC2EN.SW_ADC2_EN = *data;               //ADC2失能
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x32:
                    gs_AMG8802_MSG.ADC2ZERO.ADC2_ZERO_RANGE = *data;        //ADC2零区控制
                    AMG8802_Write_Reg(REG_ADC2ZERO,AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2ZERO));
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x33:
                    gs_AMG8802_CONFIG.CBCFG.CHK_PERIOD = *data;               //检测周期
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x34:
                    gs_AMG8802_CONFIG.CBCFG.CELL_COUNT = *data;               //电芯串数
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x35:
                    gs_AMG8802_CONFIG.UTDCFG.TS_CFG = *data;                  //NTC个数
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x36:
                    gs_AMG8802_CONFIG.OPTION.DSGON_INDSG = *data;             //DSGON使能
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x37:
                    gs_AMG8802_CONFIG.OPTION.CHGON_INDSG = *data;             //CHGON使能
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x38:       
                    gs_AMG8802_CONFIG.OPTION.CO_EN = *data;                   //断线检测使能
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x39:
                    gs_AMG8802_CONFIG.OPTION.CHG_DRV_CRRT = *data;            //充电管驱动电流
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3A:
                    gs_AMG8802_CONFIG.OPTION.TS_DLY_SEL = *data;              //温度采样建立时间
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3B:
                    gs_AMG8802_CONFIG.OPTION.ADC1_CRRT_LSB=*data;           //ADC1电流采样精度
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3C:
                    gs_AMG8802_CONFIG.OPTION.ADC1_VLTG_LSB = *data;           //ADC1电压采样精度
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3D:
                    gs_AMG8802_MSG.ADC2EN.ADC2_LSB = *data;                   //ADC2电流采样精度
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3E:                 
                    gs_AMG8802_CONFIG.OPTION.LDCHK_MD = *data;               //负载检测
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x3F:
                    gs_AMG8802_CONFIG.OPTION.INDSG_TH = *data;                //比较器保护值
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x40:
                    gs_AMG8802_CONFIG.CFGLOCK.SLEEP_OPTION = *data;           //休眠模式设置
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x41:
                                                                           //BUCK电压设置(暂时不用)
                    
                    break;
                case 0x42:
                    gs_AMG8802_CONFIG.CFGLOCK.IDLE_RANGE = *data;            //IDLE阈值
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x43:
                    gs_AMG8802_CONFIG.OVCFG.OV_RANGE = *data;                //过充阈值
                    RDS_FLAG.DFEWRITE=1;
                    break;               
                case 0x44:
                    gs_AMG8802_CONFIG.OVCFG.OV_DT = *data;                   //过充延时
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x45:
                    gs_AMG8802_CONFIG.OVCFG.OV_RLS_HYS = *data;              //过充释放阈值
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x46:
                    gs_AMG8802_CONFIG.UVCFG.UV_RANGE = *data;                //过放阈值
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x47:
                    gs_AMG8802_CONFIG.UVCFG.UV_DT = *data;                   //过放延时
                    RDS_FLAG.DFEWRITE=1;                                                     
                    break;
                case 0x48:
                    gs_AMG8802_CONFIG.UVCFG.UV_RLS_HYS=*data;              //过放恢复阈值
                    RDS_FLAG.DFEWRITE=1;                                                      
                    break;
                case 0x49:
                    gs_AMG8802_CONFIG.CFGLOCK.PCHG_RANGE = *data;            //预充电压阈值
                    RDS_FLAG.DFEWRITE=1;                                                        
                    break;
                case 0x4A:
                    gs_AMG8802_CONFIG.OCCCFG.OCC_RANGE = *data;              //充电过流阈值
                    RDS_FLAG.DFEWRITE=1;                                                     
                    break;
                case 0x4B:
                    gs_AMG8802_CONFIG.OCCCFG.OCC_DT = *data;                 //充电过流延时
                    RDS_FLAG.DFEWRITE=1;                                                      
                    break;
                case 0x4C:
                    gs_AMG8802_CONFIG.OCCCFG.OCC_RLS = *data;                //充电过流恢复方式
                    RDS_FLAG.DFEWRITE=1;                            
                    break;
                case 0x4D:
                    gs_AMG8802_CONFIG.OCDCFG.OCD1_RANGE = *data;             //放电过流1阈值
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x4E:
                    gs_AMG8802_CONFIG.OCDCFG.OCD1_DT = *data;                //放电过流1延时
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x4F:
                    gs_AMG8802_CONFIG.OCDCFG.OCD2_TH = *data;                //放电过流2阈值
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x50:
                    gs_AMG8802_CONFIG.OCCCFG.OCD2_DT = *data;                //放电过流2延时
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x51:
                    gs_AMG8802_CONFIG.UTDCFG.SCD_TH = *data;          //短路阈值
                    RDS_FLAG.DFEWRITE=1;                                                        
                    break;
                case 0x52:
                    gs_AMG8802_CONFIG.OTCCFG.SCD_DT = *data;          //短路延时
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x53:
                    gs_AMG8802_CONFIG.OCDCFG.OCSC_RLS = *data;        //过流释放方式
                    RDS_FLAG.DFEWRITE=1;                
                    break;
                case 0x54:
                    gs_AMG8802_CONFIG.OTDCFG.OTC_RANGE=*data;//充电过温阈值
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x55:
                    gs_AMG8802_CONFIG.OTDCFG.OTD_RANGE=*data;//放电过温阈值
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x56:
                    gs_AMG8802_CONFIG.OTDCFG.OT_DT = *data;          //过温延时
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x57:
                    gs_AMG8802_CONFIG.OTCCFG.OTC_RLS_HYS=*data;                  
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x58:
                    gs_AMG8802_CONFIG.OTCCFG.OTD_RLS_HYS=*data;
                    RDS_FLAG.DFEWRITE=1;                
                    break;
                case 0x59:
                    gs_AMG8802_CONFIG.UTCCFG.UTC_RANGE=*data;//充电低温阈值
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x5A:
                    gs_AMG8802_CONFIG.UTCCFG.UTD_RANGE=*data;//放电低温阈值
                    RDS_FLAG.DFEWRITE=1; 
                    break;
                case 0x5B:
                    gs_AMG8802_CONFIG.UTCCFG.UT_DT = *data;                                                                                         //低温保护延时
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x5C:
                    gs_AMG8802_CONFIG.UTDCFG.UTC_RLS_HYS=*data;//充电低温释放
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x5D:
                    gs_AMG8802_CONFIG.UTDCFG.UTD_RLS_HYS=*data;//放电低温释放
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x5E:
                    gs_AMG8802_CONFIG.CBCFG.CB_RANGE = *data;          //均衡启动电压
                    RDS_FLAG.DFEWRITE=1;
                    break;
                case 0x5F:
                    gs_AMG8802_CONFIG.CBCFG.CB_DIFF = *data;           //均衡压差
                    RDS_FLAG.DFEWRITE=1;                              
                    break;
                case 0x60:
                    gs_AMG8802_CONFIG.CBCFG.CB_CTRL = *data;            //均衡方式
                    RDS_FLAG.DFEWRITE=1;
                    break;
                
                case 0xD0:
                    buf=*data;
                    gs_BMSDATA.ADC1_Current_K=buf/10000;

                    break;
                case 0xD1:
                    buf=*data;
                    gs_BMSDATA.ADC1_Current_B=buf-1000;

                    break;
                case 0xD2:
                    buf=*data;
                    gs_BMSDATA.ADC2_Current_K=buf/10000;

                    break;
                case 0xD3:
                    buf=*data;
                    gs_BMSDATA.ADC2_Current_B=buf-1000;

                    break;
                default:
                    return false;
            }
            
        }
        else return false;
    }
    else if (type==modbus_coilType)
    {
        if(act==modbus_regRead)
        {
            switch (regAddr)
            {
                default:
                    return false;
            }
        }
        else if(act==modbus_regWrite)
        {
            switch (regAddr)
            {
                default:return false;
            }
        } 
        else return false;    
    }
    else
    {        
        return false;
    }
    return true;
}

void DFECONFIG_Write(void)
{
    if(RDS_FLAG.DFEWRITE)                                   //和上面解码器顺序不可颠倒,不然会导致写入数据丢失.
    {
        unsigned short amg8802_config = 0;
        unsigned short amg8802_msg = 0;
        AMG8802_UnlockRemapRigister();                      //解锁保护配置映射寄存器
        
               
        for(unsigned char i=0;i<11;i++)
        {
            amg8802_config=0;
            if(AMG8802_Read_Reg(REG_OVCFG+i,&amg8802_config))
            {    
                if(AMG8802_REG_U16READ(*((unsigned short *)&gs_AMG8802_CONFIG)+i)!=amg8802_config)
                {
                    AMG8802_Write_Reg(REG_OVCFG+i,*((unsigned short *)(&gs_AMG8802_CONFIG)+i));
                    RDS_FLAG.FLASHOP=1; //配置信息不一致后写入FLASH
                }
            
            }        
        
        }
        
        //msg 
        
        //  自动均衡切换
        if(AMG8802_REG_U16READ(gs_AMG8802_CONFIG.CBCFG) & 0x007f)
        {
            amg8802_msg = 0;
            if(AMG8802_Read_Reg(REG_SWOPTION,&amg8802_msg))
            {
                if(amg8802_msg & 0x0001)
                {
                    amg8802_msg &= 0xFFFE;  // 将MCU控制均衡设置为自动均衡
                    AMG8802_Write_Reg(REG_SWOPTION, amg8802_msg);
                }               
            }            
        }
        
        //swcb0
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_SWCB0,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCB0) != amg8802_msg)
        {
            AMG8802_BalanceSetting(0,1,0,0,0); // 先关闭自动均衡
            AMG8802_BalanceSetting(1,1,0,0,0); // 将自动均衡设置为MCU控制均衡
            
            AMG8802_Write_Reg(REG_SWCB0,AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCB0));
        }
        
        //swcb1
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_SWCB1,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCB1) != amg8802_msg)
        {
            AMG8802_BalanceSetting(0,1,0,0,0); // 先关闭自动均衡
            AMG8802_BalanceSetting(1,1,0,0,0); // 使能MCU控制均衡
          
            AMG8802_Write_Reg(REG_SWCB1,AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCB1));
        }
        
        //swcfg
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_SWCFG,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCFG)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_SWCFG,AMG8802_REG_U16READ(gs_AMG8802_MSG.SWCFG));
        }        
                
        //IE1
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_IE1,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE1)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_IE1,(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE1)));
        }
        
        //IE2
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_IE2,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE2)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_IE2,(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE2)));
        }
        
        
        //IE3
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_IE3,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE3)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_IE3,(AMG8802_REG_U16READ(gs_AMG8802_MSG.IE3)));
        }
        
        
        //cbsel1
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_CBSEL1,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL1)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_CBSEL1,(AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL1)));
        }
        
        //cbsel2
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_CBSEL2,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL2)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_CBSEL2,(AMG8802_REG_U16READ(gs_AMG8802_MSG.CBSEL2)));
        }
        
        //adc2 en
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_ADC2EN,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2EN)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_ADC2EN,AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2EN));
        }
        
        amg8802_msg = 0;
        AMG8802_Read_Reg(REG_ADC2ZERO,&amg8802_msg);        
        if(AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2ZERO)!=amg8802_msg)
        {
            AMG8802_Write_Reg(REG_ADC2ZERO,AMG8802_REG_U16READ(gs_AMG8802_MSG.ADC2ZERO));
        }
        
        RDS_FLAG.DFEWRITE=0;
        AMG8802_LockRemapRigister();                        //上锁保护配置映射寄存器
                 
    }
    if(RDS_FLAG.FLAG1WRITE)
    {
        RDS_FLAG.FLAG1WRITE=0;
        AMG8802_Write_Reg(REG_FLAG1,AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG1));
    }
    if(RDS_FLAG.FLAG2WRITE)
    {
        RDS_FLAG.FLAG2WRITE=0;
        AMG8802_Write_Reg(REG_FLAG2,AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG2));
    }
    if(RDS_FLAG.FLAG3WRITE)
    {
        RDS_FLAG.FLAG3WRITE=0;
        AMG8802_Write_Reg(REG_FLAG3,AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG3));
    }
    if(RDS_FLAG.FLAG4WRITE)
    {
        RDS_FLAG.FLAG4WRITE=0;
        AMG8802_Write_Reg(REG_FLAG4,AMG8802_REG_U16READ(gs_AMG8802_MSG.FLAG4));
    }
}

#endif



extern uint16_t calculate_temp(uint32_t resistance);
//***************************************************
unsigned char AMG8802TEMP_Get(void)
{
    double  tempResistance;

    AMG8802_Read_Reg(REG_UTDCFG,(unsigned short *)&gs_AMG8802_CONFIG.UTDCFG);
    AMG8802_Read_Reg(REG_STATUS5,(unsigned short *)&gs_AMG8802_MSG.STATUS5);
    AMG8802_Read_Reg(REG_TS0,&gs_AMG8802_MSG.TS[0]);
    AMG8802_Read_Reg(REG_TS1,&gs_AMG8802_MSG.TS[1]);
    AMG8802_Read_Reg(REG_TS2,&gs_AMG8802_MSG.TS[2]);
    switch(gs_AMG8802_CONFIG.UTDCFG.TS_CFG){
        case 0:
            if(gs_AMG8802_MSG.STATUS5.TS0_TREF_SEL_STATUS){          //R12K RD
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_12uA);
            }else{
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_100uA);
            }
            
            if(gs_BMSDATA.R12K_100uA>1850||gs_BMSDATA.R12K_100uA<1750){    //R12K VAILD
                gs_BMSDATA.R12K_100uA=1800;
            }

            if(gs_BMSDATA.R12K_12uA>15300||gs_BMSDATA.R12K_12uA<15200){    //R12K VAILD
                gs_BMSDATA.R12K_12uA=15266;
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS0_TREF_SEL_STATUS){          //TS0 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }
            gs_BMSDATA.CellT[1]=0;
            gs_BMSDATA.CellT[2]=0;
            break;
        case 1:
            if(gs_AMG8802_MSG.STATUS5.TS1_TREF_SEL_STATUS){          //R12K RD
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_12uA); 
            }else{
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_100uA); 
            }
            
            if(gs_BMSDATA.R12K_100uA>1850||gs_BMSDATA.R12K_100uA<1750){    //R12K VAILD
                gs_BMSDATA.R12K_100uA=1800;
            }

            if(gs_BMSDATA.R12K_12uA>15300||gs_BMSDATA.R12K_12uA<15200){    //R12K VAILD
                gs_BMSDATA.R12K_12uA=15266;
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS0_TREF_SEL_STATUS){           //TS0 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS1_TREF_SEL_STATUS){          //TS1 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[1] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[1] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[1] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[1] =  2731+calculate_temp(tempResistance)-400;  
            }
            
            gs_BMSDATA.CellT[2]=0;
            break;
        case 2:
            if(gs_AMG8802_MSG.STATUS5.TS2_TREF_SEL_STATUS){           //R12K RD
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_12uA);
            }else{
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_100uA); 
            }
        
            if(gs_BMSDATA.R12K_100uA>1850||gs_BMSDATA.R12K_100uA<1750){    //R12K VAILD
                gs_BMSDATA.R12K_100uA=1800;
            }

            if(gs_BMSDATA.R12K_12uA>15300||gs_BMSDATA.R12K_12uA<15200){    //R12K VAILD
                gs_BMSDATA.R12K_12uA=15266;
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS0_TREF_SEL_STATUS){          //TS0 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS2_TREF_SEL_STATUS){           //TS2 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[2] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[2] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[2] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[2] =  2731+calculate_temp(tempResistance)-400;  
            }
            gs_BMSDATA.CellT[1]=0;
            break;
        case 3:
            if(gs_AMG8802_MSG.STATUS5.TS2_TREF_SEL_STATUS){          //R12K RD
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_12uA);
            }else{
                AMG8802_Read_Reg(REG_VR12K,(unsigned short *)&gs_BMSDATA.R12K_100uA);
            }
                  
            if(gs_BMSDATA.R12K_100uA>1850||gs_BMSDATA.R12K_100uA<1750){    //R12K VAILD
                gs_BMSDATA.R12K_100uA=1800;
            }

            if(gs_BMSDATA.R12K_12uA>15300||gs_BMSDATA.R12K_12uA<15200){    //R12K VAILD
                gs_BMSDATA.R12K_12uA=15266;
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS0_TREF_SEL_STATUS){          //TS0 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[0] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[0] =  2731+calculate_temp(tempResistance)-400;  
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS1_TREF_SEL_STATUS){          //TS1 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[1] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[1] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[1] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[1] =  2731+calculate_temp(tempResistance)-400;  
            }
            
            if(gs_AMG8802_MSG.STATUS5.TS2_TREF_SEL_STATUS){          //TS2 CAL
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[2] / gs_BMSDATA.R12K_12uA ); 
                gs_BMSDATA.CellT[2] =  2731+calculate_temp(tempResistance)-400;  
            }else{
                tempResistance  = (double)(12000 * gs_AMG8802_MSG.TS[2] / gs_BMSDATA.R12K_100uA ); 
                gs_BMSDATA.CellT[2] =  2731+calculate_temp(tempResistance)-400;  
            }
            break;
        default:
        	return 0;
    }
    return 1;
}
void battery_status(int num)
{
	int x=13;
					for(int i=0;i<num;i++)
					{

							if(battery[i]<8&&battery[i]>=0)
								LCD_Color_Fill(x+(x*i),214,8,16-(battery[i]*2),BLACK);
							if(battery[i]>=8&&battery[i]<65000)
								LCD_Color_Fill(x+(x*i),230-16,8,16,GREEN);
							else if(battery[i]<=0)
								LCD_Color_Fill(x+(x*i),230-1,8,1,GREEN);
							else
								LCD_Color_Fill(x+(x*i),230-(battery[i]*2),8,battery[i]*2,GREEN);
//							AMG_DBG_INFO("====== battery[%d]=%d\n",i,battery[i]);

					}

}

//******************************************************************
unsigned int adler32(unsigned int * pData,unsigned short length)
{
    unsigned long adlerBuf=0;
    unsigned int adlerResult;
    adlerBuf=800;                   //避免初始化 全0通过校验
    for(unsigned short i=0;i<length;i++){
        adlerBuf+=pData[i];
    }
    adlerResult=adlerBuf&0xffffffff;
    return adlerResult;
}

#endif
