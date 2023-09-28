#ifndef __ACM86xx_H
#define __ACM86xx_H

typedef enum{
	LchEq1,  LchEq2,  LchEq3,  LchEq4,  LchEq5,  LchEq6,  LchEq7,  LchEq8,  LchEq9,  LchEq10,
	LchEq11, LchEq12, LchEq13, LchEq14, LchEq15, LchPEq1, LchPEq2, LchPEq3, LchPEq4, LchPEq5,
	LLowEq1, LLowEq2, LHighEq1,LHighEq2,
	RchEq1,  RchEq2,  RchEq3,  RchEq4,  RchEq5,  RchEq6,  RchEq7,  RchEq8,  RchEq9,  RchEq10,
	RchEq11, RchEq12, RchEq13, RchEq14, RchEq15, RchPEq1, RchPEq2, RchPEq3, RchPEq4, RchPEq5,
	RLowEq1, RLowEq2, RHighEq1,RHighEq2,AcmEnd
} AcmEqIndex;

//***************************************************
typedef struct
{
	uint16_t	type;	// eqFiltType
    uint16_t 	freq;	// 1Hz step
    uint16_t 	Q;		// 0.001 step
	int16_t		gain;	// 0.1db step
}ACM_EQ_TAB;		// 8 Byte	==== 變量宣告長度最小占用 uint16_t =====

typedef struct
{
	ACM_EQ_TAB 	EQ[48];	//48*8 = 384
	uint8_t	DrbOnOff;
	int8_t	DrbLowGain;
	int8_t	DrbHighGain;
	uint8_t	LR_Mix;
	float	ClassDGaie;
	float	InGaie;	//3x4 = 12 + 384 = 396
} ACM_SET;

extern	ACM_SET ACM_Set[2];
extern	ACM_SET ACM_SetR[2];
extern	uint8_t	ACM862xWId;
extern	uint8_t	ACM862x_IIC_ADDR[2];

void ACM8625_init();
uint8_t ACM_REPORT();
uint8_t ACM_main();
void ACMIIR_ReSend(uint8_t id);


#endif
