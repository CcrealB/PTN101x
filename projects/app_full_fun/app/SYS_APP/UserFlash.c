

#include "USER_Config.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "u_include.h"

extern void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len);
extern void flash_erase_sector(uint32_t address, uint8_t erase_size);
extern void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len);

#ifdef CONFIG_APP_PLAYER
extern int app_player_breakpoint_get(uint32_t* p_addr, int* p_size);
extern int app_player_breakpoint_set(void* addr, int size);
#endif
//==========================================================
#define SysInf_Len 	124u		// 128		  0  ~	127
#define EfLab_Len	 80u		//  80		 128 ~	207
#define EqLab_Len	 48u		//  48		 208 ~	255
#define ioGain_Len	106u		// 128		 256 ~ 	383
#define UserEf_Len	 72u		// 500		 384 ~	883		100 *5Pag=500
#define UserEq_Len  640u		// 640		 884 ~	1523	80ch*8=640
#define OutEq_Len   480u		// 1440		1524 ~	2963	10ch*8*6=480 *3Pagh=1440
#define DreSet_Len  264u		// 264		2964 ~	3227	32ch*8=264+8=264
#define ACM_Set_Len	792u		// 792		3228 ~  4019	396*2=792
//==========================================================
#define Sys_Pointer		FLASH_ENVCALI_DEF_ADDR_ABS
#define EfLab_Pointer	128u
#define EqLab_Pointer	208u
#define ioGain_Pointer	256u
#define Ef_Pointer		384u
#define Eq_Pointer		884u
#define OutEq_Pointer	1524u
#define DreSet_Pointer	2964u
#define ACm_Pointer		3228u

#define Sys_Pointer2	FLASH_OTADATA_DEF_ADDR_ABS

//*****************************************
void GroupRead(uint8_t Group, uint8_t GrType)
{
	uint16_t addr;
	if(GrType==0){
		if(Group >4) Group = 4;	// Ef 5Pagh
		addr = Group*100;
		flash_read_data((uint8_t*)&UserEf, (Sys_Pointer+Ef_Pointer+addr), UserEf_Len);
		flash_read_data((uint8_t*)&LabEf, (Sys_Pointer+EfLab_Pointer), EfLab_Len);	// 更新 Ef Lab Len:80
	}else if(GrType==1){
		if(Group >2) Group = 2;	// OutEq 3Pagh
		addr = Group*480;
		flash_read_data((uint8_t*)&OutEq, (Sys_Pointer+OutEq_Pointer+addr), OutEq_Len);
	}else if(GrType==2){
		flash_read_data((uint8_t*)&ioGain, (Sys_Pointer+ioGain_Pointer), ioGain_Len);	// Len:256
		flash_read_data((uint8_t*)&SysInf, Sys_Pointer, SysInf_Len);					// Len:128
		flash_read_data((uint8_t*)&LabEq, (Sys_Pointer+EqLab_Pointer), EqLab_Len);	// 更新 Eq Lab Len:48
		flash_read_data((uint8_t*)&UserEq, (Sys_Pointer+Eq_Pointer), UserEq_Len);
		flash_read_data((uint8_t*)&DreSet, (Sys_Pointer+DreSet_Pointer), DreSet_Len);
#ifdef ACM86xxId1
	}else if(GrType==3){
		flash_read_data((uint8_t*)&ACM_Set,(Sys_Pointer+ACm_Pointer), ACM_Set_Len);		// Len:256
#endif
	}


	if(GrType==0){
//		for(uint8_t i=0; i<41; i++) DBG_LOG_INFO("== i:%d  %d\n",i, ioGain[i]);
#ifdef ACM86xxId1
		DBG_LOG_INFO("==== 1.Len_SysInf:%d, UserEf:%d, UserEq:%d, ioGain:%d  ACM:%d  Group:%d\n", \
				sizeof(SysInf), sizeof(UserEf), sizeof(UserEq),  sizeof(ioGain), sizeof(ACM_Set), Group);
#else
		DBG_LOG_INFO("==== 1. Len_SysInf:%d, UserEf:%d, UserEq:%d, ioGain:%d  Group:%d\n", \
						sizeof(SysInf), sizeof(UserEf), sizeof(UserEq), sizeof(ioGain), Group);
#endif
	}
}

//****type 0:W_MODEm, 1:W_EF, 2:W_EQ, *****
void GroupWrite_1(uint8_t *EF_RAM, uint8_t Group, uint8_t type)
{
	uint16_t addr;
	if(type&0x02){
		if(Group >4) Group = 4;	// Ef 5Pagh
		addr = Group*100;
		memcpy(&EF_RAM[Ef_Pointer+addr], &UserEf, UserEf_Len);
		memcpy(&EF_RAM[EfLab_Pointer], &LabEf, EfLab_Len);
	}
	if(type&0x04){
		if(Group >2) Group = 2;
		addr = Group*480;
		memcpy(&EF_RAM[Eq_Pointer], &UserEq, UserEq_Len);
		memcpy(&EF_RAM[DreSet_Pointer], &DreSet, DreSet_Len);
		memcpy(&EF_RAM[OutEq_Pointer+addr], &OutEq, OutEq_Len);	// Eq 3Pagh
		memcpy(&EF_RAM[EqLab_Pointer], &LabEq, EqLab_Len);
#ifdef ACM86xxId1
		memcpy(&EF_RAM[ACm_Pointer],&ACM_Set, ACM_Set_Len);
#endif
	}
	//===================================================
	if(type&0x01){
#if (defined(SDCARD_DETECT_IO) || (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK))
		//---- 获取断点信息 ------
		uint32_t addr;
		int size;
		app_player_breakpoint_get(&addr, &size);//获取内部断点信息的地址和长度
		memcpy(SysInf.PlayInf, (uint32_t*)addr, size);//读断点信息到buff打印 或 保存到flash
	    DBG_LOG_INFO("player_breakpoint_set: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X  Ef:%d\n",\
	    		SysInf.PlayInf[0],SysInf.PlayInf[1],SysInf.PlayInf[2],SysInf.PlayInf[3],SysInf.PlayInf[4],\
				SysInf.PlayInf[5],SysInf.PlayInf[6],SysInf.PlayInf[7],SysInf.PlayInf[8],SysInf.PlayInf[9],\
				SysInf.PlayInf[10],SysInf.PlayInf[11],SysInf.PlayInf[12],SysInf.PlayInf[13],SysInf.PlayInf[14], SysInf.EfGruup);

#endif
		memcpy(&EF_RAM[0], 	&SysInf, SysInf_Len);
		memcpy(&EF_RAM[ioGain_Pointer],&ioGain, ioGain_Len);
	}
	//===================================================
	if(type&0x10){
		flash_erase_sector(Sys_Pointer, FLASH_ERASE_4K);
		EF_RAM[4095] = 0x55;	// 結尾驗證碼
		flash_write_data(EF_RAM, Sys_Pointer, 4096);
	}
}

//*********************************************
void GroupWrite(uint8_t Group, uint8_t GrType)
{
	uint8_t EF_RAM[4096];
	flash_read_data(EF_RAM, Sys_Pointer, 4096);
	if(GrType==0){			// Ef+sysinf write
		if(Group >4) Group = 4;
		GroupWrite_1(EF_RAM, Group, 0x13);
		EF_Mode = Group;
	}else if(GrType==1){	// Eq write
		if(Group >2) Group = 2;
		GroupWrite_1(EF_RAM, Group, 0x14);
		EQ_Mode = Group;
	}else if(GrType==2){	// sysinf write
		GroupWrite_1(EF_RAM, Group, 0x11);
	}
}

#include "bkreg.h"
//***** Restore Defaults *********************************
void ClearUserFlash()
{
	IOGainSet(Out1L_DacL, -90);
	IOGainSet(Out1R_DacR, -90);
	IOGainSet(Out2L_I2s2L, -90);
	IOGainSet(Out2R_I2s2R, -90);
	IOGainSet(Max2L_I2s3L, -90);
	IOGainSet(Max2R_I2s3R, -90);
	flash_erase_sector(Sys_Pointer, FLASH_ERASE_4K);
	os_delay_us(3200);	//2ms
	// watchdog reset
	dsp_shutdown();
	REG_SYSTEM_0x1D &= ~0x2;
	system_wdt_reset_ana_dig(1, 1);
	BK3000_wdt_reset();
	os_delay_us(3200);	//1ms
}

//***** **********************************************
void UserFlash_Init()
{
	uint8_t ver_test[2];
	flash_read_data(&ver_test[0], Sys_Pointer, 1);
	flash_read_data(&ver_test[1], (Sys_Pointer+4095), 1);
	DBG_LOG_INFO("==== 1. Flash TEST(0x%06X): %02X  %02X\n",Sys_Pointer, ver_test[0], ver_test[1]);
	extern const uint8_t AudioDef[4096];
	if(AudioDef[0] != ver_test[0] || AudioDef[4095] != ver_test[1]){	// Flash no USER DATA
		flash_erase_sector(Sys_Pointer, FLASH_ERASE_4K);
		flash_write_data((uint8_t*)AudioDef, Sys_Pointer, 4096);
		DBG_LOG_INFO("==== 1.Write EF_Mode:%d  %d  %d\n",EF_Mode, sizeof(UserEf),sizeof(SysInf));
		flash_read_data(&ver_test[0], Sys_Pointer, 1);
		flash_read_data(&ver_test[1], (Sys_Pointer+4095), 1);
		DBG_LOG_INFO("==== Flash WriteRead :%02X  %02X\n",ver_test[0], ver_test[1]);
	}
	// READ USER EF DATA
	GroupRead(0,2);
	EF_Mode = SysInf.EfGruup;
	EQ_Mode = SysInf.EqGruup;
	EF_ModeR = EF_Mode;
	EQ_ModeR = EQ_Mode;
	GroupRead(EF_Mode,0);
	GroupRead(EQ_Mode,1);
	GroupRead(0,2);	//20230404 edit
#ifdef ACM86xxId1
	GroupRead(0,3);
#endif
#ifdef CONFIG_APP_PLAYER
    //设置断点信息
    app_player_breakpoint_set(SysInf.PlayInf, 15);//将buff数据设置到内部断点变量
    DBG_LOG_INFO("player_breakpoint_set: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",\
	    		SysInf.PlayInf[0],SysInf.PlayInf[1],SysInf.PlayInf[2],SysInf.PlayInf[3],SysInf.PlayInf[4],\
				SysInf.PlayInf[5],SysInf.PlayInf[6],SysInf.PlayInf[7],SysInf.PlayInf[8],SysInf.PlayInf[9],\
				SysInf.PlayInf[10],SysInf.PlayInf[11],SysInf.PlayInf[12],SysInf.PlayInf[13],SysInf.PlayInf[14]);
#endif
	DBG_LOG_INFO("==== 1.Read EF_Mode:%d  %d  %d\n",EF_Mode, sizeof(UserEf),sizeof(SysInf));
}

//***************************************************
void UserFlash4K_read()
{
	uint8_t EF_RAM[4096];
	flash_read_data(EF_RAM, FLASH_ENVCALI_DEF_ADDR_ABS, 4096);
	SendBuff[8] = 0x10;
	SendBuff[9] = 101;
	for(int i=0; i<4096; i+=16){
		SendBuff[10] = i/16;
		memcpy(&SendBuff[12], &EF_RAM[i], 16);
		SendUpComp(SendBuff, 32);
	}
}

//***************************************************
void UserFlash4K_Write(uint8_t *buff)
{
	// uint8_t EF_RAM[4096];
	//定义一个静态指针变量，如果外部需要，可定义在外部。
	static uint8_t* EF_RAM = NULL;
	//申请4096 byte 内存块，不能反复申请，否则会出现内存泄漏死机。
	if(EF_RAM == NULL) EF_RAM = (uint8_t*)user_malloc(4096);

	static uint8_t FChk;
	if(buff[10]==0)	FChk = 0;
		else	FChk++;

	for(uint8_t i=0; i<16; i++){
		EF_RAM[(buff[10]*16)+i] =  buff[12+i];
	}
//	DBG_LOG_INFO("==== buff[10]:%02X\n",buff[10]);
	if(buff[10]==0xFF){
		if(FChk == 0xFF){
			PowerDownFg=1;
			EF_RAM[0] = SysInf.Ver;	//0729 add
			flash_erase_sector(Sys_Pointer, FLASH_ERASE_4K);
			flash_write_data(EF_RAM, Sys_Pointer, 4096);
			os_delay_us(3200);	//2ms
			// watchdog reset
			dsp_shutdown();
			REG_SYSTEM_0x1D &= ~0x2;
			system_wdt_reset_ana_dig(1, 1);
			BK3000_wdt_reset();
			os_delay_us(3200);	//1ms
		}else{
			SendBuff[8] = 0x10;
			SendBuff[9] = 103;
			SendBuff[10] = FChk;
			SendUpComp(SendBuff, 32);
		}
		//处理完后释放内存块
		if(EF_RAM){
			user_free(EF_RAM);
			EF_RAM = NULL;
		}
	}
}
