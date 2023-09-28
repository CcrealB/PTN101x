#ifndef _AMG8802_APP_H_
#define _AMG8802_APP_H_

typedef struct
{
	uint16_t CellV[17];
	uint16_t CellT[3];
    uint8_t CellVmaxPosition;
    uint8_t CellVminPosition;
    uint8_t CellTmaxPosition;
    uint8_t CellTminPosition;
    
	int32_t ADC1_Current;
	float ADC1_Current_K;
	float ADC1_Current_B;
    
	int32_t ADC2_Current;
	float ADC2_Current_K;
	float ADC2_Current_B;
	uint16_t PackVoltage;
	uint16_t MosStatus;
	uint16_t AlarmInformation;
	uint16_t PackInfo;
	uint16_t BalanceHigh;
	uint16_t BalanceLow;
	uint16_t CapacityHigh;
	uint16_t CapacityLow;
	uint16_t amg8802_workstatus;
    uint16_t R12K_100uA;
    uint16_t R12K_12uA;    
}BMS_DATA;

extern unsigned short AMG8802_utcEx(unsigned short data);
extern unsigned short AMG8802_utdEx(unsigned short data);
extern unsigned short AMG8802_otcEx(unsigned short data);
extern unsigned short AMG8802_otdEx(unsigned short data);

extern unsigned short AMG8802_otdInverse(unsigned short otdData);                                        
extern unsigned short AMG8802_utcInverse(unsigned short utcData);                                       
extern unsigned short AMG8802_utdInverse(unsigned short utdData);
extern unsigned short AMG8802_otcInverse(unsigned short otcData);


extern unsigned short AMG8802_utdReleaseEx(unsigned short utdTemp,unsigned short releaseTemp);
extern unsigned short AMG8802_utcReleaseEx(unsigned short utcTemp,unsigned short releaseTemp);
extern unsigned short AMG8802_otcReleaseEx(unsigned short otcTemp,unsigned short releaseTemp);
extern unsigned short AMG8802_otdReleaseEx(unsigned short otdTemp,unsigned short releaseTemp);

extern unsigned short AMG8802_otdReleaseInverse(unsigned short otdRelease,unsigned short otd);
extern unsigned short AMG8802_otcReleaseInverse(unsigned short otcRelease,unsigned short otc);
extern unsigned short AMG8802_utdReleaseInverse(unsigned short utdRelease,unsigned short utd);
extern unsigned short AMG8802_utcReleaseInverse(unsigned short utcRelease,unsigned short utc);

#endif
