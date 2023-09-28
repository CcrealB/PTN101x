
#include "USER_Config.h"
#ifdef AMG8802

//-45~125
const uint32_t con_ntc_table[] = 
{
    203750,192049,181156,171001,161522,152664,144377,136617,129343,122518,      //-40~-31
    116110,110088,104425,99096 ,94078 ,89350 ,84892 ,80688 ,76720 ,72973 ,      //-30~-21
    69434 ,66089 ,62926 ,59935 ,57104 ,54425 ,51888 ,49484 ,47206 ,45047 ,      //-20~-11
    43000 ,41057 ,39214 ,37465 ,35804 ,34226 ,32727 ,31303 ,29949 ,28661 ,      //-10~-1
    27513 ,26271 ,25162 ,24107 ,23101 ,22144 ,21231 ,20362 ,19533 ,18742 ,      //0~9
    18016 ,17269 ,16583 ,15928 ,15302 ,14704 ,14134 ,13588 ,13067 ,12568 ,      //10~19
    12092 ,11636 ,11199 ,10782 ,10382 ,10000 ,9633  ,9282,  8946  ,8623  ,      //20~29
    8314  ,8018  ,7734  ,7461  ,7199  ,6948  ,6707  ,6476  ,6254  ,6040  ,      //30~39
    5835  ,5638  ,5449  ,5267  ,5091  ,4923  ,4761  ,4605  ,4455  ,4311  ,      //40~49
    4168  ,4038  ,3909  ,3785  ,3665  ,3550  ,3439  ,3332  ,3229  ,3129  ,      //50~59
    3033  ,2941  ,2851  ,2765  ,2682  ,2602  ,2524  ,2449  ,2377  ,2307  ,      //60~69
    2240  ,2175  ,2112  ,2051  ,1992  ,1935  ,1880  ,1827  ,1776  ,1726  ,      //70~79
    1678  ,1632  ,1587  ,1543  ,1501  ,1461  ,1421  ,1383  ,1346  ,1310  ,      //80~89
    1275  ,1242  ,1209  ,1178  ,1147  ,1117  ,1089  ,1061  ,1034  ,1008  ,983   //90~100 
};

//*************************************************************
uint16_t calculate_temp(uint32_t resistance)
{
    uint32_t a = 0, b = (sizeof(con_ntc_table) >> 2) - 1;
    uint32_t i = b / 2;
    float f1;
    
    if(resistance >= con_ntc_table[a])	return (uint16_t)(a * 10);
    if(resistance <= con_ntc_table[b])	return (uint16_t)(b * 10);
    
    while(1){
        if(resistance == con_ntc_table[i]){
            a = b = i;
            break;
        }else if(resistance < con_ntc_table[i]){
            a = i;
        }else{
            b = i;
        }
        i = (a + b) / 2;
        if(0 == ((b - a) / 2)) break;
    }
    
    if(a == b)	return (uint16_t)(a * 10);
    
    f1 = (float)(resistance - con_ntc_table[b]) / (float)(con_ntc_table[a] - con_ntc_table[b]);
    f1 = (float)b - f1;
    f1 *= 10;
    return (uint16_t)f1;
}

#endif