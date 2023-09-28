#include "USER_Config.h"
#ifdef AMG8802

#define     p100Length  57
#define     p12Length   49
#define     otdSt       869
#define     otcSt       509
#define     utdSt       582
#define     utcSt       512
#define     otcStep     6
#define     otdStep     10
#define     utcStep     12
#define     utdStep     20

const unsigned short tempP12[p12Length]=
{
    
3074,2916,2765,2630,2498,2374,2255,2144,2038,1938,          //-35 ～ -26
1844,1755,1671,1592,1517,1446,1377,1313,1252,1194,          //-25 ～ -16
1139,1088,1039,992 ,948 ,906 ,865 ,827 ,791 ,756 ,          //-15 ～ -06
723 ,692 ,662 ,634 ,608 ,582 ,557 ,534 ,512 ,491 ,          //-05 ～   4
470 ,451 ,432 ,415 ,398 ,382 ,366 ,352 ,338 ,               //  5 ～  13
};

const unsigned short tempP100[p100Length]=
{
356 ,369 ,383 ,397 ,412 ,427 ,442 ,458 ,474 ,491 ,          //29 ～ 38
509 ,527 ,546 ,565 ,585 ,605 ,626 ,647 ,669 ,691 ,          //39 ～ 48
714 ,738 ,763 ,789 ,815 ,841 ,869 ,897 ,926 ,956 ,          //49 ～ 58
986 ,1017,1050,1082,1117,1151,1187,1223,1261,1299,          //59 ～ 68
1339,1379,1420,1463,1507,1552,1597,1644,1692,1741,          //69 ～ 78
1790,1842,1894,1948,2004,2059,2117                          //79 ～ 88

};
const unsigned short tempPUT[]=
{
4347 ,4097 ,3865 ,3648 ,3446 ,3257 ,3080 ,2914 ,2759 ,2614 ,    //-40 ～ -31
2477 ,2349 ,2228 ,2114 ,2007 ,1906 ,1811 ,1721 ,1637 ,1557 ,    //-30 ～ -21
1481 ,1410 ,1342 ,1279 ,1218 ,1161 ,1107 ,1056 ,1007 ,961  ,    //-20 ～ -11
917  ,876  ,837  ,799  , 764 ,730  ,698  ,668  , 639 ,611  ,    //-10 ～ -01
587  ,560  ,537  ,514  , 493 ,472  ,453  ,434  ,417  ,400  ,    // 00 ～  09
384  ,368  ,354  ,340  , 326 ,314  ,302  ,290  ,279  ,268  ,    // 10 ～  19
258  ,248  ,239  ,230  , 221 ,213  ,206  ,198  ,191  ,184  ,    // 20 ～  29
177  ,171  ,165  ,159  , 154 ,148  ,143  ,138  ,133  ,129  ,    // 30 ～  39
124  ,120  ,116  ,112  , 109 ,105  ,102  ,98   ,95   ,92   ,    // 40 ～  49
89   ,86   ,83   ,81   ,  78 ,76   ,73   ,71   ,69   ,67   ,    // 50 ～  59
65   ,63   ,61   ,59   ,  57 ,56   ,54   ,52   ,51   ,49   ,    // 60 ～  69
48   ,46   ,45   ,44   ,  42 ,41   ,40   ,39   ,38   ,37   ,    // 70 ～  79
36   ,35   ,34   ,33   ,  32 ,31   ,30   ,30   ,29   ,28   ,    // 80 ～  89
27   ,26   ,26   ,25   ,  24 ,24   ,23   ,23   ,22   ,22   ,    // 90 ～  99
21   
};


const unsigned short tempPOT[]=
{
15   ,16   ,17   ,18   ,19   ,20   ,21   ,22   ,24   ,25   ,    //-40 ～ -31
26   ,28   ,29   ,31   ,33   ,34   ,36   ,38   ,40   ,42   ,    //-30 ～ -21
44   ,46   ,49   ,51   ,54   ,56   ,59   ,62   ,65   ,68   ,    //-20 ～ -11
71   ,75   ,78   ,82   ,86   ,90   ,94   ,98   ,103  ,107  ,    //-10 ～ -01
112  ,117  ,122  ,127  ,133  ,139  ,145  ,151  ,157  ,164  ,    // 00 ～  09
171  ,178  ,185  ,193  ,201  ,209  ,217  ,226  ,235  ,244  ,    // 10 ～  19
254  ,264  ,274  ,285  ,296  ,307  ,319  ,331  ,343  ,356  ,    // 20 ～  29
369  ,383  ,397  ,412  ,427  ,442  ,458  ,474  ,491  ,509  ,    // 30 ～  39
526  ,545  ,564  ,583  ,603  ,624  ,645  ,667  ,690  ,713  ,    // 40 ～  49
737  ,761  ,786  ,812  ,838  ,865  ,893  ,922  ,951  ,982  ,    // 50 ～  59
1013 ,1045 ,1078 ,1111 ,1145 ,1181 ,1217 ,1254 ,1292 ,1332 ,    // 60 ～  69
1371 ,1412 ,1455 ,1498 ,1542 ,1588 ,1634 ,1681 ,1730 ,1780 ,    // 70 ～  79
1831 ,1882 ,1936 ,1991 ,2047 ,2103 ,2162 ,2221 ,2282 ,2345 ,    // 80 ～  89
2409 ,2473 ,2541 ,2608 ,2678 ,2750 ,2821 ,2895 ,2971 ,3048 ,    // 90 ～  99
3125 
};


//查P表,
//返回值 以-100℃为0点
unsigned short AMG8802_P100Serch(unsigned short p100_data)                                      //P值转温度,零点-100℃,分辨率1℃
{
    unsigned char queue_h=0,queue_t=p100Length;
    unsigned short p100data=0;
    if(p100_data<=tempP100[queue_h]) return 129;
    if(p100_data>=tempP100[queue_t]) return 188;
    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p100data>tempP100[queue_h])
            {
                if(p100data>tempP100[queue_h+1])
                {
                    unsigned short buf_p100=(tempP100[queue_h+1]+tempP100[queue_h+2])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+1+129;
                    }
                    else
                    {
                        return queue_h+2+129;
                    }
                }
                else
                {
                    unsigned short buf_p100=(tempP100[queue_h]+tempP100[queue_h+1])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+129;
                    }
                    else
                    {
                        return queue_h+1+129;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+129;
            }
        }
        if(p100data<=tempP100[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p100data>tempP100[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }
    
    };
    return 0;
}

unsigned short AMG8802_P12Serch(unsigned short p12_data)                                        //P值转温度,零点-100℃,分辨率1℃
{
    unsigned char queue_h=0,queue_t=p12Length;
    if(p12_data>=tempP12[queue_h]) return 65;
    if(p12_data<=tempP12[queue_t]) return 113;
    
    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p12_data>tempP12[queue_h])
            {
                if(p12_data>tempP12[queue_h+1])
                {
                    unsigned short buf_p12=(tempP12[queue_h+1]+tempP12[queue_h+2])/2;
                    if(p12_data<buf_p12)
                    {
                        return queue_h+2+65;
                    }
                    else
                    {
                        return queue_h+1+65;
                    }
                }
                else
                {
                    unsigned short buf_p12=(tempP12[queue_h]+tempP12[queue_h+1])/2;
                    if(p12_data<buf_p12)
                    {
                        return queue_h+1+65;
                    }
                    else
                    {
                        return queue_h+65;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+65;
            }
        }
        if(p12_data>tempP12[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p12_data<=tempP12[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }
    
    };
    return 0;
}



unsigned short AMG8802_P100Get(unsigned short temp)                                             //温度转P值,零点-100℃,分辨率1℃
{
    if(temp<129)return tempP100[0];
    if(temp>188)return tempP100[p100Length-1];
    
    return tempP100[temp-129];

}

unsigned short AMG8802_P12Get(unsigned short temp)                                              //温度转P值,零点-100℃,分辨率1℃
{
    if(temp<65)return tempP12[0];
    if(temp>113)return tempP12[p12Length-1];
    
    return tempP12[temp-65];

}


/**utc**/
unsigned short AMG8802_utcEx(unsigned short utcTemp)                                            //充电低温保护,温度转为P值，起始温度3℃，对应p值512，步长12
{
    unsigned char P12_sel=0,p12_data;
    if(utcTemp>=73&&utcTemp<=103)                                                         //温度范围有效性检查
    {
        p12_data=AMG8802_P12Get(utcTemp);                                                             //P表温度起始差值计算
        P12_sel=(p12_data-utcSt)/utcStep;                                                 //计算保护设定值
        return  P12_sel;
    }
    else
    {
        return 0;
    }
}



unsigned short AMG8802_utcReleaseEx(unsigned short utcTemp,unsigned short releaseTemp)          //充电低温释放,步长12
{
    unsigned short P12_sel=0,p12_releasedata=0,p12_utcdata=0;
    if(utcTemp<releaseTemp)
    {
        

        if(releaseTemp-utcTemp>80)return 0;                                                     //释放小于触发8℃以上
        
        p12_utcdata=AMG8802_P12Get(utcTemp);                                                    //计算utc的P值
        p12_releasedata=AMG8802_P12Get(releaseTemp);                                            //计算释放的P值
        
        
        P12_sel=(p12_utcdata-p12_releasedata-12)/utcStep;                                            //计算保护设定值
        
        return P12_sel;
    }
    else
    {
        return 0;
    }
}


/**utd**/
unsigned short AMG8802_utdEx(unsigned short utdTemp)                                            //放电低温保护,温度转为P值，起始温度0℃，对应p值582，步长20
{
    unsigned char P12_sel=0,p12_data;                                                           //温度范围有效性检查
    if(utdTemp>=65&&utdTemp<=100)                                                         
    {                                                                                           //P表温度起始差值计算
        p12_data=AMG8802_P12Get(utdTemp);
        P12_sel=(p12_data-utdSt)/utdStep;                                                     //计算保护设定值
        return  P12_sel;
    }
    else
    {
        return 0;
    }
}



unsigned short AMG8802_utdReleaseEx(unsigned short utdTemp,unsigned short releaseTemp)          //放电低温释放,步长20
{
    unsigned short P12_sel=0,p12_releasedata=0,p12_utddata=0;
    if(utdTemp<releaseTemp)
    {
        if(releaseTemp-utdTemp>80)return 0;                                                     //释放小于触发8℃以上
        
        
        p12_utddata=AMG8802_P12Get(utdTemp);                                                    //计算utd的P值
        p12_releasedata=AMG8802_P12Get(releaseTemp);                                            //计算释放的P值
        
        
        
        P12_sel=(p12_utddata-p12_releasedata-20)/utdStep;                                            //计算保护设定值
        
        return P12_sel;
    }
    else
    {
        return 0;
    }
}



/**otc**/
unsigned short AMG8802_otcEx(unsigned short otcTemp)                                            //充电高温保护,温度转为P值,起始温度29℃，对应p值509,步长6
{
    unsigned char P100_sel,P100_data;
    if(otcTemp>=139&&otcTemp<=167)                                                        //温度范围有效性检查
    {                                                                                       
        P100_data = AMG8802_P100Get(otcTemp);                                                          //P表温度起始差值计算
        P100_sel = (P100_data-otcSt)/otcStep;                                              
        return  P100_sel;                                                                   //计算保护设定值
    }
    else
    {
        return 0;
    }
}

unsigned short AMG8802_otcReleaseEx(unsigned short otcTemp,unsigned short releaseTemp)          //充电高温释放.步长6
{
    unsigned short P100_sel=0,p100_releasedata=0,p100_otcdata=0;
    if(otcTemp>releaseTemp)
    {
        if(otcTemp-releaseTemp>100)return 0;                                                    //释放小于触发10℃以上
      
        p100_otcdata=AMG8802_P100Get(otcTemp);                                                  //计算otc的P值
        p100_releasedata=AMG8802_P100Get(releaseTemp);                                          //计算释放的P值 
        
        P100_sel=(p100_otcdata-p100_releasedata-12)/otcStep;                                          //计算保护设定值
        return P100_sel;
        
    }
    else
    {
        return 0;
    }
}

/**otd**/
unsigned short AMG8802_otdEx(unsigned short otdTemp)                                            //放电高温保护,温度转为P值，起始温度55℃，对应P值869，步长10
{
    unsigned char P100_sel,P100_data;   
    if(otdTemp>=155&&otdTemp<=185)                                                            //温度范围有效性检查
    {                                                                                       
        P100_data=AMG8802_P100Get(otdTemp);                                                              //P表温度起始差值计算
        P100_sel=(P100_data-otdSt)/otdStep;                                                
        return  P100_sel;                                                                       //计算保护设定值
    }
    else
    {
        return 0;
    }
}

unsigned short AMG8802_otdReleaseEx(unsigned short otdTemp,unsigned short releaseTemp)          //放电高温释放,步长10
{
    unsigned short P100_sel=0,p100_releasedata=0,p100_otddata=0;
    if(otdTemp>releaseTemp)
    {
        if(otdTemp-releaseTemp>100)return 0;                                                    //释放小于触发10℃以上

        p100_otddata=AMG8802_P100Get(otdTemp);                                                  //计算otd的P值
        p100_releasedata=AMG8802_P100Get(releaseTemp);                                          //计算释放的P值
        
        P100_sel=(p100_otddata-p100_releasedata-50)/otdStep;                                         //计算保护设定值
        return P100_sel;        
    }
    else
    {
        return 0;
    }
}

unsigned short AMG8802_utcInverse(unsigned short utcData)                                       //充电低温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p12data=utcData*utcStep+utcSt;
    
    temp=AMG8802_P12Serch(p12data);
    if(temp>103) temp = 103;
    if(temp<65) temp = 65;
    return temp;
   

}

unsigned short AMG8802_utdInverse(unsigned short utdData)                                       //放电低温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p12data=utdData*utdStep+utdSt;
    
    temp=AMG8802_P12Serch(p12data);
    if(temp>100) temp = 100;
    if(temp<65) temp = 65;
    return temp;

}

unsigned short AMG8802_otcInverse(unsigned short otcData)                                       //充电高温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p100data=otcData*otcStep+otcSt+1;
    
    temp=AMG8802_P100Serch(p100data);
    if(temp>167) temp = 167;
    if(temp<139) temp = 139;
    return temp;


}

unsigned short AMG8802_otdInverse(unsigned short otdData)                                       //放电高温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p100data=otdData*otdStep+otdSt+1;
    
    temp=AMG8802_P100Serch(p100data);
    if(temp>185) temp = 185;
    if(temp<155) temp = 155;
    return temp;


}


unsigned short AMG8802_otcReleaseInverse(unsigned short otcRelease,unsigned short otc)          //充电高温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p100data=0;
    if(!otcRelease)                                                                                                           //未配置释放
    {
        p100data=otcSt+otc*otcStep;
    
    }
    else                                                                                                                      //释放
    {
        p100data=otcSt+otc*otcStep-otcRelease*otcStep-12;
    
    }
    temp = AMG8802_P100Serch(p100data);
    return temp;
    

}

unsigned short AMG8802_otdReleaseInverse(unsigned short otdRelease,unsigned short otd)          //放电高温释放逆运算,形参为保护P值,p值变回温度,起始温度为-100℃
{
    unsigned short p100data=0,temp;
    if(!otdRelease)                                                                             //未配置释放
    {
        p100data = otdSt+otd*otdStep;                                                                    //P值等于OTD
    
    }
    else                                                                                        //释放
    {
        p100data = otdSt+otd*otdStep-otdRelease*otdStep-50;                                                   //P值等于OTD-release
    
    }
    temp = AMG8802_P100Serch(p100data);
    return temp;
}



unsigned short AMG8802_utcReleaseInverse(unsigned short utcRelease,unsigned short utc)          //充电低温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p12data=0;
    if(!utcRelease)                                                                             //未配置释放
    {
        p12data = utcSt+utc*utcStep;
    
    }
    else                                                                                        //释放
    {
        p12data = utcSt+utc*utcStep-(utcRelease)*utcStep-12;                                                  //P值等于utc-release
    
    }
    
    temp = AMG8802_P12Serch(p12data);
    return temp;

}


unsigned short AMG8802_utdReleaseInverse(unsigned short utdRelease,unsigned short utd)          //放电低温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;                                                                       //范围-30 ～ 8℃
    unsigned short p12data=0;
    if(!utdRelease)                                                                             //未配置释放
    {
        p12data = utdSt+utd*utdStep;
    
    }
    else                                                                                        //释放
    {
        p12data = utdSt+utd*utdStep-(utdRelease)*utdStep-20;                                                  //P值等于utd-release
    
    }
    temp = AMG8802_P12Serch(p12data);
    return temp;
}


/*
unsigned short AMG8802_utcEx(unsigned short utcTemp)                                            //充电低温保护,温度转为P值，起始温度3℃，对应p值512，步长12
{
    unsigned char P12_sel=0,p12_data;
    if(utcTemp>=730&&utcTemp<=1030)                                                         //温度范围有效性检查
    {
        p12_data=utcTemp/10-65;                                                             //P表温度起始差值计算
        P12_sel=(tempP12[p12_data]-512)/12;                                                 //计算保护设定值
        return  P12_sel;
    }
    else
    {
        return 0;
    }
}



unsigned short AMG8802_utcReleaseEx(unsigned short utcTemp,unsigned short releaseTemp)          //充电低温释放,步长12
{
    unsigned short P12_sel=0,p12_releasedata=0,p12_utcdata=0;
    if(utcTemp<releaseTemp)
    {
        

        if(releaseTemp-utcTemp>80)return 0;                                                    //释放小于触发8℃以上
        
        p12_utcdata=utcTemp/10-65;                                                             //P表温度起始差值计算
        
        
        p12_releasedata=tempP12[p12_utcdata-(releaseTemp-utcTemp)/10];
        
        p12_utcdata=(tempP12[p12_utcdata]);                                                 //计算保护设定值
        
        
        P12_sel=(p12_utcdata-p12_releasedata-12)/12;                                                 //计算保护设定值
        
        return P12_sel;
    }
    else
    {
        return 0;
    }
}


//utd
unsigned short AMG8802_utdEx(unsigned short utdTemp)                                            //放电低温保护,温度转为P值，起始温度0℃，对应p值582，步长20
{
    unsigned char P12_sel=0,p12_data;                                                       //温度范围有效性检查
    if(utdTemp>=650&&utdTemp<=1000)                                                         
    {                                                                                       //P表温度起始差值计算
        p12_data=utdTemp/10-65;
        P12_sel=(tempP12[p12_data]-582)/20;                                                 //计算保护设定值
        return  P12_sel;
    }
    else
    {
        return 0;
    }
}



unsigned short AMG8802_utdReleaseEx(unsigned short utdTemp,unsigned short releaseTemp)          //放电低温释放,步长20
{
    unsigned short P12_sel=0,p12_releasedata=0,p12_utddata=0;
    if(utdTemp<releaseTemp)
    {
        if(releaseTemp-utdTemp>80)return 0;                                                    //释放小于触发8℃以上
        
        p12_utddata=utdTemp/10-65;                                                             //P表温度起始差值计算
        
        
        p12_releasedata=tempP12[p12_utddata-(releaseTemp-utdTemp)/10];
        
        p12_utddata=(tempP12[p12_utddata]);                                                 //计算保护设定值
        
        
        P12_sel=(p12_utddata-p12_releasedata-20)/20;                                                 //计算保护设定值
        
        return P12_sel;
    }
    else
    {
        return 0;
    }
}



//otc
unsigned short AMG8802_otcEx(unsigned short otcTemp)                                            //充电高温保护,温度转为P值,起始温度29℃，对应p值509,步长6
{
    unsigned char P100_sel,P100_data;
    if(otcTemp>=1390&&otcTemp<=1670)                                                        //温度范围有效性检查
    {                                                                                       
        P100_data=otcTemp/10-129;                                                           //P表温度起始差值计算
        P100_sel=(tempP100[P100_data]-509)/6;                                              
        return  P100_sel;                                                                   //计算保护设定值
    }
    else
    {
        return 0;
    }
}

unsigned short AMG8802_otcReleaseEx(unsigned short otcTemp,unsigned short releaseTemp)          //充电高温释放.步长6
{
    unsigned short P100_sel=0,p100_releasedata=0,p100_otcdata=0;
    if(otcTemp>releaseTemp)
    {
        if(otcTemp-releaseTemp>100)return 0;                                                    //释放小于触发10℃以上
        p100_otcdata=otcTemp/10-129;                                                             //P表温度起始差值计算
        
        
        p100_releasedata=tempP100[p100_otcdata-(otcTemp-releaseTemp)/10];                      //释放P值
        
        p100_otcdata=(tempP100[p100_otcdata]);                                                 //计算保护设定值
        
//        p100_releasedata=otcTemp/10-65;                                                             //P表温度起始差值计算
        
        P100_sel=(p100_otcdata-p100_releasedata-12)/6;                                          //计算保护设定值
        return P100_sel;
        
    }
    else
    {
        return 0;
    }
}



//otd
unsigned short AMG8802_otdEx(unsigned short otdTemp)                                            //放电高温保护,温度转为P值，起始温度55℃，对应P值869，步长10
{
    unsigned char P100_sel,P100_data;   
    if(otdTemp>=1550&&otdTemp<=1850)                                                        //温度范围有效性检查
    {                                                                                       
        P100_data=otdTemp/10-129;                                                           //P表温度起始差值计算
        P100_sel=(tempP100[P100_data]-869)/10;                                                
        return  P100_sel;                                                                   //计算保护设定值
    }
    else
    {
        return 0;
    }
}


unsigned short AMG8802_otdReleaseEx(unsigned short otdTemp,unsigned short releaseTemp)          //放电高温释放,步长10
{
    unsigned short P100_sel=0,p100_releasedata=0,p100_otddata=0;
    if(otdTemp>releaseTemp)
    {
        if(otdTemp-releaseTemp>100)return 0;                                                //释放小于触发10℃以上
        p100_otddata=otdTemp/10-129;                                                             //P表温度起始差值计算
        
        
        p100_releasedata=tempP100[p100_otddata-(otdTemp-releaseTemp)/10];
        
        p100_otddata=(tempP100[p100_otddata]);                                                 //计算保护设定值
        
        
        P100_sel=(p100_otddata-p100_releasedata-50)/10;                                                 //计算保护设定值
        return P100_sel;        
    }
    else
    {
        return 0;
    }
}

unsigned short AMG8802_utcInverse(unsigned short utcData)                                       //充电低温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char queue_h=0,queue_t=40;
    unsigned short p12data=utcData*12+512;
    if(p12data<=tempP12[queue_t]) return 103;
    if(p12data>=tempP12[queue_h]) return 65;
    
    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p12data>tempP12[queue_h])
            {
                if(p12data>tempP12[queue_h+1])
                {
                    unsigned short buf_p12=(tempP12[queue_h+1]+tempP12[queue_h+2])/2;
                    if(p12data<buf_p12)
                    {
                        return queue_h+2+65;
                    }
                    else
                    {
                        return queue_h+1+65;
                    }
                }
                else
                {
                    unsigned short buf_p12=(tempP12[queue_h]+tempP12[queue_h+1])/2;
                    if(p12data<buf_p12)
                    {
                        return queue_h+1+65;
                    }
                    else
                    {
                        return queue_h+65;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+65;
            }
        }
        if(p12data>tempP12[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p12data<=tempP12[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }
    
    };
    return 0;

}

unsigned short AMG8802_utdInverse(unsigned short utdData)                                       //放电低温保护逆运算,p值变回温度,起始温度为-100℃
{

    unsigned char queue_h=0,queue_t=38;
    unsigned short p12data=utdData*20+582;
    if(p12data<=tempP12[queue_t]) return 100;
    if(p12data>=tempP12[queue_h]) return 65;

    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p12data>tempP12[queue_h])
            {
                if(p12data>tempP12[queue_h+1])
                {
                    unsigned short buf_p12=(tempP12[queue_h+1]+tempP12[queue_h+2])/2;
                    if(p12data<buf_p12)
                    {
                        return queue_h+2+65;
                    }
                    else
                    {
                        return queue_h+1+65;
                    }
                }
                else
                {
                    unsigned short buf_p12=(tempP12[queue_h]+tempP12[queue_h+1])/2;
                    if(p12data<buf_p12)
                    {
                        return queue_h+1+65;
                    }
                    else
                    {
                        return queue_h+65;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+65;
            }
        }
        if(p12data>tempP12[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p12data<=tempP12[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }

    };
    return 0;

}

unsigned short AMG8802_otcInverse(unsigned short otcData)                                       //充电高温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char queue_h=0,queue_t=56;
    unsigned short p100data=otcData*6+509+1;
    
    if(p100data>=tempP100[queue_t]) return 167;
    if(p100data<=tempP100[queue_h]) return 139;
    
    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p100data>tempP100[queue_h])
            {
                if(p100data>tempP100[queue_h+1])
                {
                    unsigned short buf_p100=(tempP100[queue_h+1]+tempP100[queue_h+2])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+1+129;
                    }
                    else
                    {
                        return queue_h+2+129;
                    }
                }
                else
                {
                    unsigned short buf_p100=(tempP100[queue_h]+tempP100[queue_h+1])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+129;
                    }
                    else
                    {
                        return queue_h+1+129;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+129;
            }
        }
        if(p100data<=tempP100[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p100data>tempP100[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }
    
    };
    return 0;

}

unsigned short AMG8802_otdInverse(unsigned short otdData)                                       //放电高温保护逆运算,p值变回温度,起始温度为-100℃
{
    unsigned char temp=0;
    unsigned short p100data=otdData*10+869+1;
    
    temp=AMG8802_P100Serch(p100data);
    if(temp>185)return 185;
    if(temp<155)return 155;
    return temp;
    if(p100data>=tempP100[queue_t]) return 185;
    if(p100data<=tempP100[queue_h]) return 155;
    
    for(unsigned char i=0;i<10;i++)
    {
        if((queue_t-queue_h)<2)
        {
            if(p100data>tempP100[queue_h])          //数据大于表头
            {
                if(p100data>tempP100[queue_h+1])
                {
                    unsigned short buf_p100=(tempP100[queue_h+1]+tempP100[queue_h+2])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+1+129;
                    }
                    else
                    {
                        return queue_h+2+129;
                    }
                }
                else
                {
                    unsigned short buf_p100=(tempP100[queue_h]+tempP100[queue_h+1])/2;
                    if(p100data<buf_p100)
                    {
                        return queue_h+129;
                    }
                    else
                    {
                        return queue_h+1+129;
                    
                    }
                }
            }
            else
            {
                return (queue_h)+129;           //返回表头
            }
        }
        if(p100data<=tempP100[(queue_t+queue_h)/2])
        {
            queue_t=(queue_h+queue_t)/2;
        }
        if(p100data>tempP100[(queue_t+queue_h)/2])
        {
            queue_h=(queue_h+queue_t)/2;
        }
    
    };
    return 0;

}
*/

#endif
