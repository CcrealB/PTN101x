#ifndef _AMG8802_ALGO_H_
#define _AMG8802_ALGO_H_

#include "stdint.h"

#define AMG8802_RegCELL2mV(x)           x*0.16                              //电压寄存器     转mV
#define AMG8802_RegCELL2HundreduV(x)    x*1.6                              //电压寄存器     转mV
#define AMG8802_mV2RegOV_RANGE(x)       (x-3276.8)/5.12                     //过压寄存器     mV转过压电压值
#define AMG8802_mV2RegOV_RLS_HYS(x)     x/10.24                             //过压释放寄存器 mV转过压释放值
#define AMG8802_mV2RegUV_RANGE(x)       (x-1024)/10.24                      //欠压寄存器     mV转欠压电压值
#define AMG8802_mV2ReguV_RLS_HYS(x)     x/20.48                             //欠压释放寄存器 mV转欠压释放值


extern  void    AMG8802_ADC1CurrentoffsetSet(int16_t currentOffset);
extern  void    AMG8802_ADC2CurrentoffsetSet(int16_t currentOffset);
extern  int     AMG8802_REGCurrent2mV(int16_t CRRT0,uint16_t CRRT1);
extern  int     AMG8802_REGCurADC2ExmV(int16_t CRRT0,uint16_t CRRT1);


#endif
