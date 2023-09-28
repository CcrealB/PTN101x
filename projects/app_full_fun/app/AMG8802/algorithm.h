#ifndef _algorithm_h_
#define _algorithm_h_
#include "stdbool.h"

#define endian_shortChange(x)  (x>>8&0xff)|(x<<8&0xff00)

extern unsigned char cal_max(unsigned short *data_p,unsigned char num);
extern unsigned char cal_min(unsigned short *data_p,unsigned char num);
extern unsigned short cal_avarage(unsigned short *data_p,unsigned char num);

extern unsigned short cal_sum(unsigned short *data_p,unsigned char num);
extern unsigned char cal_charMax(unsigned char *data_p,unsigned char num);
extern unsigned char cal_charMin(unsigned char *data_p,unsigned char num);
extern bool leapYearJudge(unsigned short year);

extern unsigned int cal_batCapacitance(unsigned int capnow,unsigned int current,unsigned int cur_zero);

#endif
