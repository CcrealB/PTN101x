
#include "USER_Config.h"
#ifdef AMG8802

#include "amg8802_algo.h"

static  int16_t  AMG8802_ADC1CurrDrift;        //零漂电流
static  int16_t  AMG8802_ADC2CurrDrift;        //零漂电流
/**---------------------------------------------------------------------------------
 * @brief  void AMG8802_ADC1CurrentoffsetSet(void) 
 * @note   set Currentoffset 
 *         
 * @param  
 *          
 * @retval 
 */
void AMG8802_ADC1CurrentoffsetSet(int16_t currentOffset)
{
    AMG8802_ADC1CurrDrift=currentOffset * 2.5;
}


void AMG8802_ADC2CurrentoffsetSet(int16_t currentOffset)
{
    AMG8802_ADC2CurrDrift=currentOffset * 0.5;
}

int AMG8802_REGCurrent2mV(int16_t CRRT0,uint16_t CRRT1)      
{
    int current_buf;
    current_buf=((int)((CRRT0<<16)|CRRT1<<14))*0.0001525-AMG8802_ADC1CurrDrift;
    return current_buf;
}


int AMG8802_REGCurADC2ExmV(int16_t CRRT0,uint16_t CRRT1)      
{
    int current_buf;
    current_buf=((int)((CRRT0<<16)|CRRT1<<12))*0.0001221-AMG8802_ADC2CurrDrift;
    return current_buf;
}

#endif
