 
// EC11旋轉編碼器底層驅動程序/
// 版本：V1.0

#include "USER_Config.h"

#ifdef EC11_Encoder
//==== IO口定義 ===========================================================
#define EC11_A_Now	gpio_input(EC11_A)	// EC11 的 A 引腳，視為時鐘線
#define EC11_B_Now	gpio_input(EC11_B)	// EC11 的 B 引腳，視為信號線
#ifdef EC11_KEY
	#define EC11_Key	gpio_input(EC11_KEY)	// EC11 的 KEY
#endif
//==== 編碼器動作代碼相關定義 ============
//static unsigned char EC11_NUM_SW = 0;

//==== 編碼器參數微調宏定義 ==============
#define EC11_SCAN_PERIOD_MS            1                            //EC11編碼器掃瞄週期
#define KEY_COUNT_DESHAKING         ( 20/EC11_SCAN_PERIOD_MS)       //按鍵消抖時間
#define KEY_COUNT_LONGTIME          (600/EC11_SCAN_PERIOD_MS)       //長按按鍵判斷時間
#define KEY_COUNT_DUALCLICKTIME     (150/EC11_SCAN_PERIOD_MS)       //雙擊按鍵判斷時間
#define KEY_LONG_REPEAT_TIME        (200/EC11_SCAN_PERIOD_MS)       //長按按鍵的回報率的倒數，即一直長按按鍵時響應的時間間隔

//==== 局部文件內變量列表 ====================================
static  char    EC11_A_Last = 0;	// A 引腳上一次的狀態
static  char    EC11_B_Last = 0;	// B 引腳上一次的狀態
static  char    EC11_Type = 0;		// EC11 的類型   0：一定位對應一脈衝；  1：兩定位對應一脈衝
									// 所謂一定位對應一脈衝，是旋轉編碼器每轉動一格，A 和 B 都會輸出一個完整的方波
									// 兩定位對應一脈衝，是旋轉編碼器每轉動兩格，A 和 B 才會輸出一個完整的方波，
									// 只轉動一格只輸出 A 和 B 的上升沿或下降沿

static   int    EC11_KEY_COUNT = 0;				// 按鍵動作計數器
static   int    EC11_KEY_DoubleClick_Count = 0;	// 按鍵雙擊動作計數器
static  char    FLAG_EC11_KEY_ShotClick = 0;	// 按鍵短按動作標誌
static  char    FLAG_EC11_KEY_LongClick = 0;	// 按鍵長按動作標誌
static  char    FLAG_EC11_KEY_DoubleClick = 0;	// 按鍵雙擊動作標誌

//注意事項：EC11旋轉編碼器的掃瞄時間間隔控制在1~4ms之間，否則5ms及以上的掃瞄時間在快速旋轉時可能會誤判旋轉方向

//*****************************************************************************
// 功能：初始化EC11旋轉編碼器相關參數
// 形參：EC11旋轉編碼器的類型 Set_EC11_TYPE  0:一定位對應一脈衝;	非0:兩定位對應一脈衝
// 詳解：對EC11旋轉編碼器的連接IO口做IO口模式設置。以及將相關的變量進行初始化
//*****************************************************************************
void Encoder_EC11_Init(unsigned char Set_EC11_TYPE)
{
    // IO口模式初始化
	gpio_config_new(EC11_A, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);		// EC11	VOL +
	gpio_config_new(EC11_B, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);		// EC11	Vol -
#ifdef EC11_KEY
	gpio_config_new(EC11_KEY, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// EC11	PushKey
#endif
    // EC11類型選擇：0-一定位一脈衝；1-兩定位一脈衝
    if(Set_EC11_TYPE == 0)	EC11_Type = 0;
    	else				EC11_Type = 1;

    // 避免上電時EC11旋鈕位置不確定導致一次動作誤判
    EC11_A_Last = EC11_A_Now;   
    EC11_B_Last = EC11_B_Now;

    // 清除按鍵計數器和標誌位
    EC11_KEY_COUNT = 0;                     //EC11按鍵動作計數器
    EC11_KEY_DoubleClick_Count = 0;         //EC11按鍵雙擊動作計數器
    FLAG_EC11_KEY_ShotClick = 0;            //EC11按鍵短按動作標誌
    FLAG_EC11_KEY_LongClick = 0;            //EC11按鍵長按動作標誌
    FLAG_EC11_KEY_DoubleClick = 0;          //EC11按鍵雙擊動作標誌
}


/****************************************************************************************************
	功能：掃瞄EC11旋轉編碼器的動作並將參數返回給動作分析函數使用
	形參：EC11旋轉編碼器的類型	Set_EC11_TYPE 0:一定位對應一脈衝;	非0 兩定位對應一脈衝
	返回：EC11旋轉編碼器的掃瞄結果
		ScanResult 0:無動作;	1:正轉;	-1:反轉:	2:只按下按鍵;	3:按著按鍵正轉;	-3:按著按鍵反轉
	詳解：只掃瞄EC11旋轉編碼器有沒有動作，不關心是第幾次按下按鍵或長按或雙擊。
	返回值直接作為形參傳給 [ void Encoder_EC11_Analyze(char EC11_Value); ] 函數使用
***************************************************************************************************/
char Encoder_EC11_Scan()
{
    char ScanResult = 0;    //返回編碼器掃瞄結果，用於分析編碼器的動作
    						//返回值：0：無動作   1：正轉   -1：反轉    2：按下按鍵   3：按下按鍵正轉   -3：按下按鍵反轉
                          
    if(EC11_Type == 0){	//一定位對應一脈衝的EC11
        if(EC11_A_Now != EC11_A_Last){   //以A為時鐘，B為數據。正轉時AB反相，反轉時AB同相
            if(EC11_A_Now == 0){
                if(EC11_B_Now ==1){	//只需要採集A的上升沿或下降沿的任意一個狀態，若A下降沿時B為1，正轉
                    ScanResult = 1;	//正轉
                }else{				//反轉
                    ScanResult = -1;
				}
                USER_DBG_INFO("========================\n");
                USER_DBG_INFO("========================\n");
                USER_DBG_INFO("========================\n");
            }else{
                if(EC11_B_Now ==0){	//只需要採集A的上升沿或下降沿的任意一個狀態，若A下降沿時B為1，正轉
                    ScanResult = 1;	//正轉
                }else{				//反轉
                    ScanResult = -1;
				}
                USER_DBG_INFO("========================1\n");
                USER_DBG_INFO("========================1\n");
                USER_DBG_INFO("========================1\n");
            }
            EC11_A_Last = EC11_A_Now;   //更新編碼器上一個狀態暫存變量
            EC11_B_Last = EC11_B_Now;   //更新編碼器上一個狀態暫存變量
        }

    }else{	// 兩定位對應一脈衝的EC11

        if(EC11_A_Now !=EC11_A_Last){	//當A發生跳變時採集B當前的狀態，並將B與上一次的狀態進行對比。
       	                            	//若A 0->1 時，B 1->0 正轉；若A 1->0 時，B 0->1 正轉；
        								//若A 0->1 時，B 0->1 反轉；若A 1->0 時，B 1->0 反轉
            if(EC11_A_Now == 1){	//EC11_A和上一次狀態相比，為上升沿
                if((EC11_B_Last == 1)&&(EC11_B_Now == 0))	ScanResult = 1;   //EC11_B和上一次狀態相比，為下降沿
                if((EC11_B_Last == 0)&&(EC11_B_Now == 1))	ScanResult = -1;    //EC11_B和上一次狀態相比，為上升沿
                //>>>>>>>>>>>>>>>>下面為正轉一次再反轉或反轉一次再正轉處理<<<<<<<<<<<<<<<<//
                if((EC11_B_Last == EC11_B_Now)&&(EC11_B_Now == 0))	ScanResult = 1;   //A上升沿時，採集的B不變且為0
                if((EC11_B_Last == EC11_B_Now)&&(EC11_B_Now == 1))	ScanResult = -1;  //A上升沿時，採集的B不變且為1
            }else{	//EC11_A和上一次狀態相比，為下降沿
                if((EC11_B_Last == 1)&&(EC11_B_Now == 0))	ScanResult = -1;   //EC11_B和上一次狀態相比，為下降沿
                if((EC11_B_Last == 0)&&(EC11_B_Now == 1))	ScanResult = 1;   //EC11_B和上一次狀態相比，為上升沿
                //>>>>>>>>>>>>>>>>下面為正轉一次再反轉或反轉一次再正轉處理<<<<<<<<<<<<<<<<//
                if((EC11_B_Last == EC11_B_Now)&&(EC11_B_Now == 0))	ScanResult = -1;  //A上升沿時，採集的B不變且為0
                if((EC11_B_Last == EC11_B_Now)&&(EC11_B_Now == 1))	ScanResult = 1;  //A上升沿時，採集的B不變且為1
            }               
            EC11_A_Last = EC11_A_Now;   //更新編碼器上一個狀態暫存變量
            EC11_B_Last = EC11_B_Now;   //更新編碼器上一個狀態暫存變量
        }
    }                                                                       
#ifdef EC11_KEY
    if(EC11_Key == 0){	//如果EC11的按鍵按下:
        if(ScanResult == 0){	//按下按鍵時未轉動
            ScanResult = 2;		//返回值為2
        }else{
            if(ScanResult == 1)     //按下按鍵時候正轉
                ScanResult = 3;     //返回值為3
            if(ScanResult == -1)    //按下按鍵時候反轉
                ScanResult = -3;    //返回值為-3
        }
    }
#endif
    return ScanResult;      //返回值的取值：   0：無動作；      1：正轉；           -1：反轉；
}                           //              2：只按下按鍵；    3：按著按鍵正轉；   -3：按著按鍵反轉

#endif

#ifdef EC11_Encoder2
//==== IO口定義 ===========================================================
#define EC11_A_Now2	gpio_input(EC11_A2)	// EC11 的 A 引腳，視為時鐘線
#define EC11_B_Now2	gpio_input(EC11_B2)	// EC11 的 B 引腳，視為信號線
#define EC11_Key2	gpio_input(EC11_KEY2)	// EC11 的 KEY

//==== 編碼器動作代碼相關定義 ============
//static unsigned char EC11_NUM_SW = 0;

//==== 編碼器參數微調宏定義 ==============
#define EC11_SCAN_PERIOD_MS2            1                            //EC11編碼器掃瞄週期
#define KEY_COUNT_DESHAKING2         ( 20/EC11_SCAN_PERIOD_MS2)       //按鍵消抖時間
#define KEY_COUNT_LONGTIME2          (600/EC11_SCAN_PERIOD_MS2)       //長按按鍵判斷時間
#define KEY_COUNT_DUALCLICKTIME2     (150/EC11_SCAN_PERIOD_MS2)       //雙擊按鍵判斷時間
#define KEY_LONG_REPEAT_TIME2        (200/EC11_SCAN_PERIOD_MS2)       //長按按鍵的回報率的倒數，即一直長按按鍵時響應的時間間隔

//==== 局部文件內變量列表 ====================================
static  char    EC11_A_Last2 = 0;	// A 引腳上一次的狀態
static  char    EC11_B_Last2 = 0;	// B 引腳上一次的狀態
static  char    EC11_Type2 = 0;		// EC11 的類型   0：一定位對應一脈衝；  1：兩定位對應一脈衝
									// 所謂一定位對應一脈衝，是旋轉編碼器每轉動一格，A 和 B 都會輸出一個完整的方波
									// 兩定位對應一脈衝，是旋轉編碼器每轉動兩格，A 和 B 才會輸出一個完整的方波，
									// 只轉動一格只輸出 A 和 B 的上升沿或下降沿

static   int    EC11_KEY_COUNT2 = 0;				// 按鍵動作計數器
static   int    EC11_KEY_DoubleClick_Count2 = 0;	// 按鍵雙擊動作計數器
static  char    FLAG_EC11_KEY_ShotClick2 = 0;	// 按鍵短按動作標誌
static  char    FLAG_EC11_KEY_LongClick2 = 0;	// 按鍵長按動作標誌
static  char    FLAG_EC11_KEY_DoubleClick2 = 0;	// 按鍵雙擊動作標誌

//注意事項：EC11旋轉編碼器的掃瞄時間間隔控制在1~4ms之間，否則5ms及以上的掃瞄時間在快速旋轉時可能會誤判旋轉方向

//*****************************************************************************
// 功能：初始化EC11旋轉編碼器相關參數
// 形參：EC11旋轉編碼器的類型 Set_EC11_TYPE  0:一定位對應一脈衝;	非0:兩定位對應一脈衝
// 詳解：對EC11旋轉編碼器的連接IO口做IO口模式設置。以及將相關的變量進行初始化
//*****************************************************************************
void Encoder_EC11_Init2(unsigned char Set_EC11_TYPE2)
{
    // IO口模式初始化
	gpio_config_new(EC11_A2, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);		// EC11	VOL +
	gpio_config_new(EC11_B2, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);		// EC11	Vol -
	gpio_config_new(EC11_KEY2, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// EC11	PushKey

    // EC11類型選擇：0-一定位一脈衝；1-兩定位一脈衝
    if(Set_EC11_TYPE2 == 0)	EC11_Type2 = 0;
    	else				EC11_Type2 = 1;

    // 避免上電時EC11旋鈕位置不確定導致一次動作誤判
    EC11_A_Last2 = EC11_A_Now2;
    EC11_B_Last2 = EC11_B_Now2;

    // 清除按鍵計數器和標誌位
    EC11_KEY_COUNT2 = 0;                     //EC11按鍵動作計數器
    EC11_KEY_DoubleClick_Count2 = 0;         //EC11按鍵雙擊動作計數器
    FLAG_EC11_KEY_ShotClick2 = 0;            //EC11按鍵短按動作標誌
    FLAG_EC11_KEY_LongClick2 = 0;            //EC11按鍵長按動作標誌
    FLAG_EC11_KEY_DoubleClick2 = 0;          //EC11按鍵雙擊動作標誌
}


/****************************************************************************************************
	功能：掃瞄EC11旋轉編碼器的動作並將參數返回給動作分析函數使用
	形參：EC11旋轉編碼器的類型	Set_EC11_TYPE 0:一定位對應一脈衝;	非0 兩定位對應一脈衝
	返回：EC11旋轉編碼器的掃瞄結果
		ScanResult 0:無動作;	1:正轉;	-1:反轉:	2:只按下按鍵;	3:按著按鍵正轉;	-3:按著按鍵反轉
	詳解：只掃瞄EC11旋轉編碼器有沒有動作，不關心是第幾次按下按鍵或長按或雙擊。
	返回值直接作為形參傳給 [ void Encoder_EC11_Analyze(char EC11_Value); ] 函數使用
***************************************************************************************************/
char Encoder_EC11_Scan2()
{
    char ScanResult = 0;    //返回編碼器掃瞄結果，用於分析編碼器的動作
    						//返回值：0：無動作   1：正轉   -1：反轉    2：按下按鍵   3：按下按鍵正轉   -3：按下按鍵反轉

    if(EC11_Type2 == 0){	//一定位對應一脈衝的EC11
        if(EC11_A_Now2 != EC11_A_Last2){   //以A為時鐘，B為數據。正轉時AB反相，反轉時AB同相
            if(EC11_A_Now2 == 0){
                if(EC11_B_Now2 ==1){	//只需要採集A的上升沿或下降沿的任意一個狀態，若A下降沿時B為1，正轉
                    ScanResult = 1;	//正轉
                }else{				//反轉
                    ScanResult = -1;
				}
            }
            EC11_A_Last2 = EC11_A_Now2;   //更新編碼器上一個狀態暫存變量
            EC11_B_Last2 = EC11_B_Now2;   //更新編碼器上一個狀態暫存變量
        }
    }else{	// 兩定位對應一脈衝的EC11

        if(EC11_A_Now2 !=EC11_A_Last2){	//當A發生跳變時採集B當前的狀態，並將B與上一次的狀態進行對比。
       	                            	//若A 0->1 時，B 1->0 正轉；若A 1->0 時，B 0->1 正轉；
        								//若A 0->1 時，B 0->1 反轉；若A 1->0 時，B 1->0 反轉
            if(EC11_A_Now2 == 1){	//EC11_A和上一次狀態相比，為上升沿
                if((EC11_B_Last2 == 1)&&(EC11_B_Now2 == 0))   //EC11_B和上一次狀態相比，為下降沿
                    ScanResult = 1;                         //正轉
                if((EC11_B_Last2 == 0)&&(EC11_B_Now2 == 1))   //EC11_B和上一次狀態相比，為上升沿
                    ScanResult = -1;                        //反轉
                //>>>>>>>>>>>>>>>>下面為正轉一次再反轉或反轉一次再正轉處理<<<<<<<<<<<<<<<<//
                if((EC11_B_Last2 == EC11_B_Now2)&&(EC11_B_Now2 == 0))  //A上升沿時，採集的B不變且為0
                    ScanResult = 1;                                 //正轉

                if((EC11_B_Last2 == EC11_B_Now2)&&(EC11_B_Now2 == 1))  //A上升沿時，採集的B不變且為1
                    ScanResult = -1;                                //反轉
            }else{	//EC11_A和上一次狀態相比，為下降沿
                if((EC11_B_Last2 == 1)&&(EC11_B_Now2 == 0))   //EC11_B和上一次狀態相比，為下降沿
                    ScanResult = -1;                        //反轉
                if((EC11_B_Last2 == 0)&&(EC11_B_Now2 == 1))   //EC11_B和上一次狀態相比，為上升沿
                    ScanResult = 1;                         //正轉
                //>>>>>>>>>>>>>>>>下面為正轉一次再反轉或反轉一次再正轉處理<<<<<<<<<<<<<<<<//
                if((EC11_B_Last2 == EC11_B_Now2)&&(EC11_B_Now2 == 0))  //A上升沿時，採集的B不變且為0
                    ScanResult = -1;                                //反轉
                if((EC11_B_Last2 == EC11_B_Now2)&&(EC11_B_Now2 == 1))  //A上升沿時，採集的B不變且為1
                    ScanResult = 1;                                 //正轉
            }
            EC11_A_Last2 = EC11_A_Now2;   //更新編碼器上一個狀態暫存變量
            EC11_B_Last2 = EC11_B_Now2;   //更新編碼器上一個狀態暫存變量
        }
    }

    if(EC11_Key2 == 0){	//如果EC11的按鍵按下，並且沒有EC11沒有轉動，
        if(ScanResult == 0){	//按下按鍵時未轉動
            ScanResult = 2;		//返回值為2
        }else{
            if(ScanResult == 1)     //按下按鍵時候正轉
                ScanResult = 3;     //返回值為3
            if(ScanResult == -1)    //按下按鍵時候反轉
                ScanResult = -3;    //返回值為-3
        }
    }
    return ScanResult;      //返回值的取值：   0：無動作；      1：正轉；           -1：反轉；
}                           //              2：只按下按鍵；    3：按著按鍵正轉；   -3：按著按鍵反轉

#endif

//*******************************************************************/
//功能：對EC11旋轉編碼器的動作進行分析，並作出相應的動作處理代碼
//形參：無
//返回：char AnalyzeResult = 0;目前無用。若在該函數里做了動作處理，則函數的返回值無需理會
//詳解：對EC11旋轉編碼器的動作進行模式分析，是單擊還是雙擊還是長按鬆手還是一直按下。
//形參從 [ char Encoder_EC11_Scan(unsigned char Set_EC11_TYPE) ] 函數傳入。
//在本函數內修改需要的動作處理代碼
//*******************************************************************/
#if 0
u8 EC_Add_Count = 0, EC_Sub_Count = 0;
char Encoder_EC11_Analyze(char EC11_Value)
{
    char AnalyzeResult = 0;
    static unsigned int TMP_Value = 0;  //中間計數值，用於連續長按按鍵的動作延時間隔
    //>>>>>>>>>>>>>>>>編碼器正轉處理程序<<<<<<<<<<<<<<<<//
    if(EC11_Value == 1){ //正轉
//		printf("EC11_Value = 1 \n");
		EC_Add_Count++;
        //--------編碼器正轉動作代碼--------//
        switch(EC11_NUM_SW){
            case 1: 
				break;
            case 2: 
				break;
            case 3: 
				break;
            case 4: 
				break;
            case 5: 
				break;
            default:
				break;
        }
        
    }

    //>>>>>>>>>>>>>>>>編碼器反轉處理程序<<<<<<<<<<<<<<<<//
    if(EC11_Value == -1){	//反轉
//		printf("EC11_Value = -1 \n");
		EC_Sub_Count++;
        //--------編碼器反轉動作代碼--------//

        
    }


    //>>>>>>>>>>>>>>>>編碼器按鍵按下並正轉處理程序<<<<<<<<<<<<<<<<//
    if(EC11_Value == 3){
        //--------編碼器按鍵按下並正轉動作代碼--------//
//       	printf("EC11_Value = 3 \n"); 
        
    }

    //>>>>>>>>>>>>>>>>編碼器按鍵按下並反轉處理程序<<<<<<<<<<<<<<<<//
    if(EC11_Value == -3){
        //--------編碼器按鍵按下並反轉動作代碼--------//
//        printf("EC11_Value = -3 \n");
        
    }


    //>>>>>>>>>>>>>>>>編碼器按鍵按下處理程序<<<<<<<<<<<<<<<<//
    if(EC11_Value == 2){     //====檢測到按鍵按下====//
//		printf("EC11_Value = 2 \n");
        if(EC11_KEY_COUNT<10000) EC11_KEY_COUNT++;   //打開按鍵按下時間定時器
        
		// 按下按鍵到達消抖時間時, 置位短按按鍵標誌
        if(EC11_KEY_COUNT == KEY_COUNT_DESHAKING){     
 //           printf("短按按鍵標誌... \n");
			FLAG_EC11_KEY_ShotClick = 1;
        }

		// 鬆開按鍵後，又在雙擊時間內按下按鍵, 置位雙擊按鍵標誌
        if((EC11_KEY_DoubleClick_Count > 0)&&(EC11_KEY_DoubleClick_Count <= KEY_COUNT_DUALCLICKTIME)){   
//     		printf("雙擊按鍵標誌... \n");
            FLAG_EC11_KEY_DoubleClick = 1;
        }

		// 按下按鍵 到達長按時間, 置位長按按鍵標誌並復位短按按鍵標誌
        if(EC11_KEY_COUNT == KEY_COUNT_LONGTIME){    
//			printf("長按按鍵標誌... \n");
            FLAG_EC11_KEY_LongClick = 1;
            FLAG_EC11_KEY_ShotClick = 0;
        }

    }else{	// 檢測到按鍵鬆開
		// 沒到消抖時長就鬆開按鍵，復位所有定時器和按鍵標誌
        if(EC11_KEY_COUNT < KEY_COUNT_DESHAKING){ 
//			printf("沒到消抖時長就鬆開按鍵，復位所有定時器和按鍵標誌... \n");
            EC11_KEY_COUNT = 0;
            FLAG_EC11_KEY_ShotClick = 0;
            FLAG_EC11_KEY_LongClick = 0;
            FLAG_EC11_KEY_DoubleClick = 0;
            EC11_KEY_DoubleClick_Count = 0;
        }else{
            
            if(FLAG_EC11_KEY_ShotClick == 1){        //短按按鍵定時有效期間
                if((FLAG_EC11_KEY_DoubleClick == 0)&&(EC11_KEY_DoubleClick_Count >= 0)) 
                    EC11_KEY_DoubleClick_Count++;
                if((FLAG_EC11_KEY_DoubleClick == 1)&&(EC11_KEY_DoubleClick_Count <= KEY_COUNT_DUALCLICKTIME))   //如果在規定雙擊時間內再次按下按鍵
                {                                                                                               //認為按鍵是雙擊動作
                    FLAG_EC11_KEY_DoubleClick = 2;
                }   

                if((FLAG_EC11_KEY_DoubleClick == 0)&&(EC11_KEY_DoubleClick_Count > KEY_COUNT_DUALCLICKTIME))    //如果沒有在規定雙擊時間內再次按下按鍵
                    FLAG_EC11_KEY_ShotClick = 0;                                                                //認為按鍵是單擊動作
            }

            if(FLAG_EC11_KEY_LongClick == 1)        //檢測到長按按鍵鬆開
                FLAG_EC11_KEY_LongClick = 0;
        }

    }


    //>>>>>>>>>>>>>>>>編碼器按鍵分析處理程序<<<<<<<<<<<<<<<<//
    if(EC11_KEY_COUNT > KEY_COUNT_DESHAKING){    //短按按鍵延時到了時間
        //短按按鍵動作結束代碼
        if((FLAG_EC11_KEY_ShotClick == 0)&&(EC11_KEY_DoubleClick_Count > KEY_COUNT_DUALCLICKTIME)&&(EC11_KEY_COUNT < KEY_COUNT_LONGTIME))   //短按按鍵動作結束代碼
        {
            //--------短按按鍵動作結束代碼--------//
            EC11_NUM_SW++;
            if(EC11_NUM_SW >= 4)
                EC11_NUM_SW = 1;
            AnalyzeResult = 1;
            //--------清除標誌位--------//
            EC11_KEY_COUNT = 0;
            EC11_KEY_DoubleClick_Count = 0;
            FLAG_EC11_KEY_DoubleClick = 0;
        }

        //雙擊按鍵動作結束代碼
        if((FLAG_EC11_KEY_DoubleClick == 2)&&(EC11_KEY_DoubleClick_Count > 0)&&(EC11_KEY_DoubleClick_Count <= KEY_COUNT_DUALCLICKTIME)) //雙擊按鍵動作結束代碼
        {
            //--------雙擊按鍵動作結束代碼--------//

            AnalyzeResult = 2;
            //--------清除標誌位--------//
            EC11_KEY_COUNT = 0;
            EC11_KEY_DoubleClick_Count = 0;
            FLAG_EC11_KEY_ShotClick = 0;
            FLAG_EC11_KEY_DoubleClick = 0;
            
        }

        //連續長按按鍵按下代碼
        if((FLAG_EC11_KEY_LongClick == 1)&&(EC11_KEY_COUNT >= KEY_COUNT_LONGTIME))  //連續長按按鍵按下代碼
        {
            TMP_Value ++;
            if(TMP_Value % KEY_LONG_REPEAT_TIME == 0)
            {
                TMP_Value = 0;
                //-------連續長按按鍵按下代碼--------//
                AnalyzeResult = 4;
            }
        }

        //長按按鍵動作結束代碼
        if((FLAG_EC11_KEY_LongClick == 0)&&(EC11_KEY_COUNT >= KEY_COUNT_LONGTIME))  //長按按鍵動作結束代碼
        {                                                                           
            //--------長按按鍵按下動作結束代碼--------//

            AnalyzeResult = 3;
            //--------清除標誌位--------//
            EC11_KEY_COUNT = 0;
        }


    }
    return AnalyzeResult;
}
#endif

