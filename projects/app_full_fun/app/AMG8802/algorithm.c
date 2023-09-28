

#include "USER_Config.h"
#ifdef AMG8802

#include "algorithm.h"

unsigned char cal_max(unsigned short *data_p,unsigned char num)
{
  unsigned long max_num;
  max_num=0;
  for(unsigned char i=0;i<num;i++)
  {
    if(data_p[i]>data_p[max_num])
    {
      max_num=i;
    }
  }
  return max_num;
}

unsigned char cal_charMax(unsigned char *data_p,unsigned char num)
{
  unsigned long max_num;
  max_num=0;
  for(unsigned char i=0;i<num;i++)
  {
    if(data_p[i]>data_p[max_num])
    {
      max_num=i;
    }
  }
  return max_num;
}

unsigned char cal_charMin(unsigned char *data_p,unsigned char num)
{
  unsigned long min_num;
  min_num=0;
  for(unsigned char i=0;i<num;i++)
  {
    if(*(data_p+i)<data_p[min_num])
    {
      min_num=i;
    }
  }
  return min_num;
}

unsigned char cal_min(unsigned short *data_p,unsigned char num)
{
  unsigned long min_num;
  min_num=0;
  for(unsigned char i=0;i<num;i++)
  {
    if(*(data_p+i)<data_p[min_num])min_num=i;
  }
  return min_num;
}

unsigned short cal_sum(unsigned short *data_p,unsigned char num)
{
  unsigned long sum;
  sum=0;
  for(unsigned char i=0;i<num;i++)
  {
    sum+=data_p[i];
  }
  return sum;
}

unsigned short cal_avarage(unsigned short *data_p,unsigned char num)
{
  unsigned long buf=0;
  for(unsigned char i=0;i<num;i++)
  {
    buf+=data_p[i];
  }
  buf=buf/num;
  return buf;
}

unsigned int cal_batCapacitance(unsigned int capnow,unsigned int current,unsigned int cur_zero)
{
  unsigned int capbuf=capnow;
  if(current>cur_zero)
  {
    capbuf=capbuf+current-cur_zero;
  }
  else 
  {
    capbuf+=current;
    if(capbuf<cur_zero)capbuf=0;
    else{capbuf-=cur_zero;}
  }
  return capbuf;
}

bool leapYearJudge(unsigned short year)
{
    if((((year % 100) != 0) && ((year % 4) == 0)) || ((year % 400) == 0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif
