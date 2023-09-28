#ifndef _AMG8802_H_
#define _AMG8802_H_

#include "USER_Config.h"

#define AMG8802_DelayMs(x)      for(uint16_t i=0; i<x; i++) {os_delay_us(1600);}
                                
#define AMG8802_IIC_Write 0x18                  // Write addrַ
#define AMG8802_IIC_Read  0x19                  // Read addr
//#define PackVol_LSB  3.2                        // ��ѹ�����ķֱ���

#define AMG8802_REG_U16pX(pREG)         ((unsigned short*)(&pREG))
#define AMG8802_REG_U16READ(pREG)       *((unsigned short*)(&pREG))


enum    AMG8802_REGREQ
{
    REG_HWID      = 0x00    ,                   //����ID
    REG_CVER      = 0x01    ,                   //�����汾

    REG_TRIM11    = 0x4B   ,                    //ADC1������Ư&����triming��
    REG_TRIM12    = 0x4C   ,                    //ADC2������Ư&����triming��    
    REG_OVCFG     = 0x55   ,                    //��ѹ����
    REG_UVCFG     = 0x56   ,                    //Ƿѹ����
    REG_OCDCFG    = 0x57   ,                    //�ŵ��������
    REG_OCCCFG    = 0x58   ,                    //����������
    REG_OTDCFG    = 0x59   ,                    //�ŵ����
    REG_OTCCFG    = 0x5A   ,                    //������
    REG_UTCCFG    = 0x5B   ,                    //������
    REG_UTDCFG    = 0x5C   ,                    //�ŵ����
    REG_CBCFG     = 0x5D   ,                    //��������
    REG_OPTION    = 0x5E   ,                    //����ѡ������
    REG_CFGLOCK   = 0x5F   ,                    //��������          
    
    REG_ADC1REQ   = 0x70   ,                    //ADC1����
    REG_CHKREQ    = 0x71   ,                    //�ֶ�ɨ�败��
    REG_COCHKREQ  = 0x72   ,                    //�ֶ����ߴ���
                        
    REG_CELL01    = 0x91   ,                    //��о1�ĵ�ѹ
    REG_CELL02    = 0x92   ,                    //��о2�ĵ�ѹ
    REG_CELL03    = 0x93   ,                    //��о3�ĵ�ѹ
    REG_CELL04    = 0x94   ,                    //��о4�ĵ�ѹ
    REG_CELL05    = 0x95   ,                    //��о5�ĵ�ѹ
    REG_CELL06    = 0x96   ,                    //��о6�ĵ�ѹ
    REG_CELL07    = 0x97   ,                    //��о7�ĵ�ѹ
    REG_CELL08    = 0x98   ,                    //��о8�ĵ�ѹ
    REG_CELL09    = 0x99   ,                    //��о9�ĵ�ѹ
    REG_CELL10    = 0x9A   ,                    //��о10�ĵ�ѹ
    REG_CELL11    = 0x9B   ,                    //��о11�ĵ�ѹ
    REG_CELL12    = 0x9C   ,                    //��о12�ĵ�ѹ
    REG_CELL13    = 0x9D   ,                    //��о13�ĵ�ѹ
    REG_CELL14    = 0x9E   ,                    //��о14�ĵ�ѹ
    REG_CELL15    = 0x9F   ,                    //��о15�ĵ�ѹ
    REG_CELL16    = 0xA0   ,                    //��о16�ĵ�ѹ
    REG_CELL17    = 0xA1   ,                    //��о17�ĵ�ѹ
    REG_TS0       = 0xA2   ,                    //�¶�0
    REG_TS1       = 0xA3   ,                    //�¶�1
    REG_TS2       = 0xA4   ,                    //�¶�2
    REG_CRRT0     = 0xA5   ,                    //������16bit
    REG_CRRT1     = 0xA6   ,                    //������2bit
    REG_VR12K     = 0xA7   ,                    //R12K
    REG_VBAT      = 0xA8   ,                    //VBAT
    
    REG_COM       = 0xAF   ,                    //COM
    REG_REMAP     = 0xB0   ,                    //remap
    REG_SWOPTION  = 0xB1   ,                    //�����ź�Դ����
    REG_SWCB0     = 0xB2   ,                    //����ʹ�ܸ�16bit
    REG_SWCB1     = 0xB3   ,                    //����ʹ�ܵ�1bit
    REG_SWCFG     = 0xB5   ,                    //����ģʽ����
    REG_PWMC      = 0xB6   ,                    //PWMC����
    REG_SWFET     = 0xB8   ,                    //��������FET
    REG_PDSGON    = 0xB9   ,                    //Ԥ�书������
    REG_BUCKDOWN  = 0xBA   ,                    //BUCK�µ����
    REG_SLPWKUP   = 0xBB   ,                    //����˯�߿���
    
    REG_FLAG1     = 0xC0   ,                    //��־�Ĵ���1
    REG_IE1       = 0xC1   ,                    //�ж�ʹ�ܼĴ���1
    REG_STATUS1   = 0xC2   ,                    //״̬�Ĵ���1
    REG_FLAG2     = 0xC3   ,                    //��־�Ĵ���2
    REG_IE2       = 0xC4   ,                    //�ж�ʹ�ܼĴ���2
    REG_STATUS2   = 0xC5   ,                    //״̬�Ĵ���2
    REG_FLAG3     = 0xC6   ,                    //��־�Ĵ���3
    REG_IE3       = 0xC7   ,                    //�ж�ʹ�ܼĴ���3
    REG_FLAG4     = 0xC8   ,                    //��־�Ĵ���4
    REG_STATUS4   = 0xC9   ,                    //״̬�Ĵ���4
    REG_STATUS5   = 0xCA   ,                    //״̬�Ĵ���5
    
    REG_CBSEL1    = 0xCE   ,                    //���վ��������źŸ�16bit
    REG_CBSEL2    = 0xCF   ,                    //���վ��������źŵ�1bit
    REG_ADC2EN    = 0xD0   ,                    //ADC2��������
    
    REG_ADC2ZERO  = 0xD2   ,                    //ADC2������������
    
    REG_ADC2D0    = 0xD8   ,                    //ADC2ת�������16bit
    REG_ADC2D1    = 0xD9   ,                    //ADC2ת�������4bit
    REG_CCH16     = 0xDA   ,                    //���ؼƸ�16bit
    REG_CCL16     = 0xDB   ,                    //���ؼƵ�16bit
    
    REG_EFUSE_MD  = 0xF0                        //OTP����
};

typedef struct
{
    uint16_t OV_RANGE       :8;
    uint16_t OV_RLS_HYS     :6;
    uint16_t OV_DT          :2;
}AMG8802_OVCFG_TypeDef; 
    
typedef struct  
{   
    uint16_t UV_RANGE       :8;
    uint16_t UV_RLS_HYS     :6;
    uint16_t UV_DT          :2;
}AMG8802_UVCFG_TypeDef; 
    
typedef struct  
{   
    uint16_t OCD1_RANGE     :9;
    uint16_t OCSC_RLS       :1;
    uint16_t OCD1_DT        :2;
    uint16_t OCD2_TH        :4;
}AMG8802_OCDCFG_TypeDef;    
    
typedef struct  
{   
    uint16_t OCC_RANGE      :9;
    uint16_t OCC_RLS        :1;
    uint16_t OCC_DT         :2;
    uint16_t OCD2_DT        :4;
}AMG8802_OCCCFG_TypeDef;    
    
typedef struct  
{   
    uint16_t OTD_RANGE      :7;
    uint16_t OTC_RANGE      :7;
    uint16_t OT_DT          :2;
}AMG8802_OTDCFG_TypeDef;

typedef struct
{
    uint16_t OTD_RLS_HYS    :6;
    uint16_t OTC_RLS_HYS    :6;
    uint16_t SCD_DT         :4;
}AMG8802_OTCCFG_TypeDef;

typedef struct
{
    uint16_t UTD_RANGE      :7;
    uint16_t UTC_RANGE      :7;
    uint16_t UT_DT          :2;
}AMG8802_UTCCFG_TypeDef;

typedef struct
{
    uint16_t UTD_RLS_HYS    :6;
    uint16_t TS_CFG         :2;
    uint16_t UTC_RLS_HYS    :6;
    uint16_t SCD_TH         :2;
}AMG8802_UTDCFG_TypeDef;

typedef struct
{
    uint16_t CB_RANGE       :7;
    uint16_t CB_CTRL        :1;
    uint16_t CELL_COUNT     :4;
    uint16_t CB_DIFF        :2;
    uint16_t CHK_PERIOD     :2;
}AMG8802_CBCFG_TypeDef;

typedef struct
{
    uint16_t NA             :2;
    uint16_t INDSG_TH       :2;
    uint16_t ADC1_VLTG_LSB	:1;
    uint16_t LDCHK_MD	    :1;
    uint16_t ADC1_CRRT_LSB  :2;
    uint16_t TS_DLY_SEL	    :1;
    uint16_t OTDUTD_RLS	    :1;
    uint16_t CHG_DRV_CRRT   :2;
    uint16_t UV_RLS	        :1;
    uint16_t CO_EN 	        :1;
    uint16_t CHGON_INDSG	:1;
    uint16_t DSGON_INDSG	:1;
}AMG8802_OPTION_TypeDef;   

typedef struct
{
    uint16_t IDLE_RANGE     :5;
    uint16_t BUCK_SEL       :1;
    uint16_t SLEEP_OPTION	:2;
    uint16_t PCHG_RANGE     :6;
    uint16_t EDSG_CTRL      :1;
    uint16_t CFG_LOCK       :1;
}AMG8802_CFGLOCK_TypeDef;           

typedef struct
{
    uint16_t ADC1_REQ_CH        :5;
    uint16_t CRRT_SYNC	        :1;
    uint16_t SW_CO_SEL	        :1;
    uint16_t SW_TREF_SEL	    :1;
    uint16_t SW_ADC1_CRRT_LSB   :2;
    uint16_t SW_ADC1_VLTG_LSB	:1;
    uint16_t NA0	            :1;
    uint16_t SW_CB_ADC	        :1;
    uint16_t NA1	            :2;
    uint16_t SW_ADC1_REQ	    :1;
}AMG8802_ADC1REQ_TypeDef;

typedef struct
{
    uint16_t SW_CHK_REQ    :16;
}AMG8802_CHKREQ_TypeDef;

typedef struct
{
    uint16_t SW_COCHK_REQ   :16;
}AMG8802_COCHKREQ_TypeDef;


typedef struct
{
    int16_t CRRT_ADC1_HIGH    :16;
}AMG8802_CRRT0_TypeDef;

typedef struct
{
    uint16_t CRRT_ADC1_LOW    :2;
}AMG8802_CRRT1_TypeDef;

typedef struct
{
    uint16_t SW_REMAP    :1;
    uint16_t NA          :15;
}AMG8802_REMAP_TypeDef;

typedef struct
{
    uint16_t SW_CB_EN   :1;
    uint16_t SW_RLS     :1;
}AMG8802_SWOPTION_TypeDef;

typedef struct
{
    uint16_t SW_CB01    :1;
    uint16_t SW_CB02    :1;
    uint16_t SW_CB03    :1;
    uint16_t SW_CB04    :1;
    uint16_t SW_CB05    :1;
    uint16_t SW_CB06    :1;
    uint16_t SW_CB07    :1;
    uint16_t SW_CB08    :1;
    uint16_t SW_CB09    :1;
    uint16_t SW_CB10    :1;
    uint16_t SW_CB11    :1;
    uint16_t SW_CB12    :1;
    uint16_t SW_CB13    :1;
    uint16_t SW_CB14    :1;
    uint16_t SW_CB15    :1;
    uint16_t SW_CB16    :1;
}AMG8802_SWCB0_TypeDef;

typedef struct
{
    uint16_t SW_CB00    :1;
    uint16_t NA         :15;
}AMG8802_SWCB1_TypeDef;

typedef struct
{
    uint16_t EDSG_MD            :2;
    uint16_t NA0                :2;
    uint16_t PDSG_OFF_OPTION    :1;
    uint16_t NA1                :1;
    uint16_t SW_CHGR_CHK        :1;
    uint16_t SW_LOAD_CHK        :1;
    uint16_t NA2                :4;
    uint16_t INCHG_WKUP_EN      :1;
    uint16_t INDSG_WKUP_EN      :1;
    uint16_t CHGRIN_WKUP_EN     :1;
    uint16_t IDON_WKUP_EN       :1;
}AMG8802_SWCFG_TypeDef;

typedef struct
{
    uint16_t OCD2_PWM_DT    :10;
    uint16_t NA             :1;
    uint16_t SCD_PWM_DT     :5;
}AMG8802_PWMC_TypeDef;

typedef struct
{
    uint16_t SW_DFET_CTRL   :1;
    uint16_t SW_CFET_CTRL   :1;
    uint16_t SW_PFET_CTRL   :1;
    uint16_t NA             :13;
}AMG8802_SWFET_TypeDef;

typedef struct
{
    uint16_t SW_FORCE_PDSG_ON   :1;
    uint16_t NA0                :3;
    uint16_t SW_PDSG_EN         :1;
    uint16_t NA1                :11;
}AMG8802_PDSGON_TypeDef;

typedef struct
{
    uint16_t DEFAULT    :16;
}AMG8802_BUCKDOWN_TypeDef;

typedef struct
{
    uint16_t DEFAULT    :16;
}AMG8802_SLPWKUP_TypeDef;

typedef struct
{
    uint16_t OV_FLAG        :1;
    uint16_t UV_FLAG        :1;
    uint16_t OCD1_FLAG      :1;
    uint16_t OCD2_FLAG      :1;
    uint16_t SCD_FLAG       :1;
    uint16_t OCC_FLAG       :1;
    uint16_t OCD2_PWM_FLAG  :1;
    uint16_t SCD_PWM_FLAG   :1;
    uint16_t OTD_FLAG       :1;
    uint16_t OTC_FLAG       :1;
    uint16_t UTC_FLAG       :1;
    uint16_t UTD_FLAG       :1;
    uint16_t OHT_FLAG       :1;
    uint16_t CO_FLAG        :1;
    uint16_t NA             :1;
    uint16_t POR_FLAG       :1;
}AMG8802_FLAG1_TypeDef;

typedef struct
{
    uint16_t OV_IE          :1;
    uint16_t UV_IE          :1;
    uint16_t OCD1_IE        :1;
    uint16_t OCD2_IE        :1;
    uint16_t SCD_IE         :1;
    uint16_t OCC_IE         :1;
    uint16_t OCD2_PWM_IE    :1;
    uint16_t SCD_PWM_IE     :1;
    uint16_t OTD_IE         :1;
    uint16_t OTC_IE         :1;
    uint16_t UTC_IE         :1;
    uint16_t UTD_IE         :1;
    uint16_t OHT_IE         :1;
    uint16_t CO_IE          :1;
    uint16_t NA             :2;
    
}AMG8802_IE1_TypeDef;

typedef struct
{
    uint16_t OV_STATUS          :1;
    uint16_t UV_STATUS          :1;
    uint16_t OCD1_STATUS        :1;
    uint16_t OCD2_STATUS        :1;
    uint16_t SCD_STATUS         :1;
    uint16_t OCC_STATUS         :1;
    uint16_t OCD2_PWM_STATUS    :1;
    uint16_t SCD_PWM_STATUS     :1;
    uint16_t OTD_STATUS         :1;
    uint16_t OTC_STATUS         :1;
    uint16_t UTC_STATUS         :1;
    uint16_t UTD_STATUS         :1;
    uint16_t OHT_STATUS         :1;
    uint16_t CO_STATUS          :1;
    uint16_t NA                 :2;
}AMG8802_STATUS1_TypeDef;

typedef struct
{
    uint16_t IDLE_FLAG          :1;
    uint16_t CHG_FLAG           :1;
    uint16_t DISCHG_FLAG        :1;
    uint16_t LDON_FLAG          :1;
    uint16_t LDOFF_FLAG         :1;
    uint16_t CHGRIN_FLAG        :1;
    uint16_t CHGROFF_FLAG       :1;
    uint16_t SWCB_TOUT_FLAG     :1;
    uint16_t SW_WKUP_FLAG       :1;
    uint16_t SLEEP_FLAG         :1;
    uint16_t INCHG_WKUP_FLAG    :1;
    uint16_t CHGRIN_WKUP_FLAG   :1;
    uint16_t LDON_WKUP_FLAG     :1;
    uint16_t INDSG_WKUP_FLAG    :1;
    uint16_t CCOF_FLAG          :1;
    uint16_t CCUF_FLAG          :1;
}AMG8802_FLAG2_TypeDef;

typedef struct
{
    uint16_t IDLE_IE        :1;
    uint16_t CHG_IE         :1;
    uint16_t DISCHG_IE      :1;
    uint16_t LDON_IE        :1;
    uint16_t LDOFF_IE       :1;
    uint16_t CHGRIN_IE      :1;
    uint16_t CHGROFF_IE     :1;
    uint16_t SWCB_TOUT_IE   :1;
    uint16_t SW_WKUP_IE     :1;
    uint16_t SLEEP_IE       :1;
    uint16_t INCHG_WKUP_IE  :1;
    uint16_t CHGRIN_WKUP_IE :1;
    uint16_t LDON_WKUP_IE   :1;
    uint16_t INDSG_WKUP_IE  :1;
    uint16_t CCOF_IE        :1;
    uint16_t CCUF_IE        :1;
}AMG8802_IE2_TypeDef;

typedef struct
{
    uint16_t IDLE_STATUS            :1;
    uint16_t CHG_STATUS             :1;
    uint16_t DISCHG_STATUS          :1;
    uint16_t LDON_STATUS            :1;
    uint16_t LDOFF_STATUS           :1;
    uint16_t CHGRIN_STATUS          :1;
    uint16_t CHGROFF_STATUS         :1;
    uint16_t SWCB_TOUT_STATUS       :1;
    uint16_t SW_WKUP_STATUS         :1;
    uint16_t SLEEP_STATUS           :1;
    uint16_t INCHG_WKUP_STATUS      :1;
    uint16_t CHGRIN_WKUP_STATUS     :1;
    uint16_t LDON_WKUP_STATUS       :1;
    uint16_t INDSG_WKUP_STATUS      :1;
    uint16_t CCOF_STATUS            :1;
    uint16_t CCUF_STATUS            :1;
}AMG8802_STATUS2_TypeDef;

typedef struct
{
    uint16_t SW_ADC1_DONE_FLAG      :1;
    uint16_t SW_CHK_DONE_FLAG       :1;
    uint16_t SW_COCHK_DONE_FLAG     :1;
    uint16_t NA0                    :1;
    
    uint16_t AUTO_CHK_DONE_FLAG     :1;
    uint16_t AUTO_COCHK_DONE_FLAG   :1;
    uint16_t NA1                    :1;
    
    uint16_t ADC2_DONE_FLAG         :1;
    uint16_t NA2                    :8;
}AMG8802_FLAG3_TypeDef;             

typedef struct
{
    uint16_t SW_ADC1_DONE_IE    :1;
    uint16_t SW_CHK_DONE_IE     :1;
    uint16_t SW_COCHK_DONE_IE   :1;
    uint16_t NA0                :1;
    
    uint16_t AUTO_CHK_DONE_IE   :1;
    uint16_t AUTO_COCHK_DONE_IE :1;
    uint16_t NA1                :1;
    
    uint16_t ADC2_DONE_IE       :1;
    uint16_t NA2                :8;
}AMG8802_IE3_TypeDef;

typedef struct
{
   uint16_t TS0_OTD_FLAG    :1;
   uint16_t TS0_OTC_FLAG    :1;
   uint16_t TS0_UTC_FLAG    :1;
   uint16_t TS0_UTD_FLAG    :1;
   uint16_t TS1_OTD_FLAG    :1;
   uint16_t TS1_OTC_FLAG    :1;
   uint16_t TS1_UTC_FLAG    :1;
   uint16_t TS1_UTD_FLAG    :1;
   uint16_t TS2_OTD_FLAG    :1;
   uint16_t TS2_OTC_FLAG    :1;
   uint16_t TS2_UTC_FLAG    :1;
   uint16_t TS2_UTD_FLAG    :1;
   uint16_t NA	            :4;
}AMG8802_FLAG4_TypeDef;

typedef struct
{
   uint16_t TS0_OTD_STATUS  :1;
   uint16_t TS0_OTC_STATUS  :1;
   uint16_t TS0_UTC_STATUS  :1;
   uint16_t TS0_UTD_STATUS  :1;
   uint16_t TS1_OTD_STATUS  :1;
   uint16_t TS1_OTC_STATUS  :1;
   uint16_t TS1_UTC_STATUS  :1;
   uint16_t TS1_UTD_STATUS  :1;
   uint16_t TS2_OTD_STATUS  :1;
   uint16_t TS2_OTC_STATUS  :1;
   uint16_t TS2_UTC_STATUS  :1;
   uint16_t TS2_UTD_STATUS  :1;
   uint16_t NA	            :4;
}AMG8802_STATUS4_TypeDef;

typedef struct
{
    uint16_t DSG_EN_STATUS          :1;
    uint16_t CHG_EN_STATUS          :1;
    uint16_t PCHG_EN_STATUS        :1;
    uint16_t PDSG_EN_STATUS        :1;
    uint16_t TS0_TREF_SEL_STATUS    :1;
    uint16_t TS1_TREF_SEL_STATUS    :1;
    uint16_t TS2_TREF_SEL_STATUS    :1;
    uint16_t NA                     :9;
}AMG8802_STATUS5_TypeDef;

typedef struct
{
    uint16_t CB_SEL01    :1;
    uint16_t CB_SEL02    :1;
    uint16_t CB_SEL03    :1;
    uint16_t CB_SEL04    :1;
    uint16_t CB_SEL05    :1;
    uint16_t CB_SEL06    :1;
    uint16_t CB_SEL07    :1;
    uint16_t CB_SEL08    :1;
    uint16_t CB_SEL09    :1;
    uint16_t CB_SEL10    :1;
    uint16_t CB_SEL11    :1;
    uint16_t CB_SEL12    :1;
    uint16_t CB_SEL13    :1;
    uint16_t CB_SEL14    :1;
    uint16_t CB_SEL15    :1;
    uint16_t CB_SEL16    :1;
}AMG8802_CBSEL1_TypeDef;

typedef struct
{
    uint16_t CB_SEL00   :1;
    uint16_t NA         :15;
}AMG8802_CBSEL2_TypeDef;

typedef struct
{
    uint16_t SW_ADC2_EN :1;
    uint16_t ADC2_LSB   :1;
    uint16_t NA         :14;
}AMG8802_ADC2EN_TypeDef;

typedef struct
{
    uint16_t ADC2_ZERO_RANGE    :8;
    uint16_t NA                 :8;
}AMG8802_ADC2ZERO_TypeDef;

typedef struct
{
    uint16_t ADC2_RESULT_INTER  :4;
    uint16_t NA                 :12;
}AMG8802_ADC2RES1_TypeDef;

typedef struct
{
    uint16_t FINAL_ADC2_RESULT  :16;
}AMG8802_ADC2D0_TypeDef;

typedef struct
{
    uint16_t FINAL_ADC2_RESULT  :4;
    uint16_t NA                 :12;
}AMG8802_ADC2D1_TypeDef;

typedef struct
{
    uint16_t CC_VALUE       :16;
}AMG8802_CCH16_TypeDef;

typedef struct
{
    uint16_t CC_VALUE    :16;
}AMG8802_CCL16_TypeDef;

typedef struct
{
    uint16_t OTP_CTRL       :2;
    uint16_t NA0            :6;
    uint16_t REMAP_STOP     :1;
    uint16_t AUTO_CHK_STOP  :1;
    uint16_t NA1            :5;
    uint16_t WR_UNLOCK      :1;
}AMG8802_EFUSE_MD_TypeDef;


typedef struct
{
	AMG8802_OVCFG_TypeDef   OVCFG;
	AMG8802_UVCFG_TypeDef   UVCFG;
	AMG8802_OCDCFG_TypeDef  OCDCFG;
	AMG8802_OCCCFG_TypeDef  OCCCFG;
	AMG8802_OTDCFG_TypeDef  OTDCFG;
	AMG8802_OTCCFG_TypeDef  OTCCFG;
	AMG8802_UTCCFG_TypeDef  UTCCFG;
	AMG8802_UTDCFG_TypeDef  UTDCFG;
	AMG8802_CBCFG_TypeDef   CBCFG;
	AMG8802_OPTION_TypeDef  OPTION;
	AMG8802_CFGLOCK_TypeDef CFGLOCK;
}AMG8802_CONFIG;


typedef struct
{
    uint16_t                    HWID;
    uint16_t                    CVER;
    uint16_t                    OTP_DATA;
    uint16_t                    CONFIG_DATA;
    AMG8802_ADC1REQ_TypeDef     ADC1REQ;
    AMG8802_CHKREQ_TypeDef      CHKREQ;
    AMG8802_COCHKREQ_TypeDef    COCHKREQ;
    uint16_t                    CELL[17];
    uint16_t                    TS[3];
    AMG8802_CRRT0_TypeDef       CRRT0;
    AMG8802_CRRT1_TypeDef       CRRT1;
    uint16_t                    VR12K;
    uint16_t                    VBAT;
    uint16_t                    COM;
    AMG8802_REMAP_TypeDef       REMAP;
    AMG8802_SWOPTION_TypeDef    SWOPTION;
    AMG8802_SWCB0_TypeDef       SWCB0;
    AMG8802_SWCB1_TypeDef       SWCB1;
    AMG8802_SWCFG_TypeDef       SWCFG;
    AMG8802_PWMC_TypeDef        PWMC;
    AMG8802_SWFET_TypeDef       SWFET;
    AMG8802_PDSGON_TypeDef      PDSGON;
    AMG8802_BUCKDOWN_TypeDef    BUCKDOWN;
    AMG8802_SLPWKUP_TypeDef     SLPWKUP;
    AMG8802_FLAG1_TypeDef       FLAG1;
    AMG8802_IE1_TypeDef         IE1;
    AMG8802_STATUS1_TypeDef     STATUS1;
    AMG8802_FLAG2_TypeDef       FLAG2;
    AMG8802_IE2_TypeDef         IE2;
    AMG8802_STATUS2_TypeDef     STATUS2;
    AMG8802_FLAG3_TypeDef       FLAG3;
    AMG8802_IE3_TypeDef         IE3;
    AMG8802_FLAG4_TypeDef       FLAG4;
    AMG8802_STATUS4_TypeDef     STATUS4;
    AMG8802_STATUS5_TypeDef     STATUS5;
    AMG8802_CBSEL1_TypeDef      CBSEL1;
    AMG8802_CBSEL2_TypeDef      CBSEL2;
    AMG8802_ADC2EN_TypeDef      ADC2EN;
    AMG8802_ADC2ZERO_TypeDef    ADC2ZERO;
    AMG8802_ADC2RES1_TypeDef    ADC2RES1;
    AMG8802_ADC2D0_TypeDef      ADC2D0;
    AMG8802_ADC2D1_TypeDef      ADC2D1;
    AMG8802_CCH16_TypeDef       CCH16;
    AMG8802_CCL16_TypeDef       CCL16;
    AMG8802_EFUSE_MD_TypeDef    OTP_MD;
    uint16_t                    VR12KT0;
    uint16_t                    VR12KT1;
    uint16_t                    VR12KT2;
    
}AMG8802_REG;




uint8_t cal_crc(uint8_t* buffer, int off, int len);
extern void     AMG8802_Init(AMG8802_REG *pMSG);
extern uint8_t  AMG8802_Read_Reg(enum    AMG8802_REGREQ reg,uint16_t* value);
extern uint8_t  AMG8802_Write_Reg(enum    AMG8802_REGREQ reg,uint16_t value);
extern void     AMG8802CONFIG_DeInit(void);
extern void     AMG8802CONFIG_Set(AMG8802_CONFIG   pCfg);

extern int16_t  AMG8802_ADC1CurrentoffsetCalibration(void);                 //��������رճ�ŵ磬����������������ʹ��.
extern int16_t  AMG8802_ADC2CurrentoffsetCalibration(void);                 //��������رճ�ŵ磬����������������ʹ��.
extern void     AMG8802_WakeupOperation(void);
extern void     AMG8802_SleepOperation(void);
extern void     AMG8802_WakeupByCurrentInterruptConfig(void);
extern void     AMG8802_ClearAllFlag(void);
extern void     AMG8802_CloseBuck(void);

extern uint8_t  AMG8802_OverChargeThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OverDischargeThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OTCThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OTDThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_UTCThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_UTDThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OCCThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OCD1ThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_OCD2ThresholdSetting(uint8_t *RxBuffer);
extern void     AMG8802_SCThresholdSetting(uint8_t *RxBuffer);

extern uint8_t  AMG8802_UnlockRemapRigister(void);//����������дӳ��Ĵ���
extern uint8_t  AMG8802_LockRemapRigister(void);   //��������ֹдӳ��Ĵ���

extern uint8_t  AMG8802_CellCountSetting(uint8_t CellCount);//���õ�о����

extern uint8_t  AMG8802_CheckPeriodSetting(uint16_t CheckPeriod);
extern uint8_t  AMG8802_NTCCountSetting(uint8_t NTCCount);
extern uint8_t  AMG8802_Dsgon_inchg_Setting(uint8_t state);
extern uint8_t  AMG8802_Chgon_indsg_Setting(uint8_t state);
extern uint8_t  AMG8802_SleepMode_Setting(uint8_t SleepMode);
extern uint8_t  AMG8802_Adc1CrrtLsb_Setting(uint8_t Adc1CrrtLsb);
extern uint8_t  AMG8802_Adc1VltgLsb_Setting(uint8_t state);
extern uint8_t  AMG8802_Adc2CrrtLsb_Setting(uint8_t state);
extern uint8_t  AMG8802_Coen_Setting(uint8_t state);
extern uint8_t  AMG8802_ChgDrvCrrt_Setting(uint8_t ChgDrvCrrt);
extern uint8_t  AMG8802_TsDelay_Setting(uint8_t TsDelay);
extern uint8_t  AMG8802_LDChk_Setting(uint8_t state);
extern uint8_t  AMG8802_Indsgth_Setting(uint8_t Indsgth);
extern uint8_t  AMG8802_IDLEThreshold_Setting(uint8_t IDLERange);


extern uint8_t  AMG8802_Voltage_Collection(uint16_t *pAMG8802_CELL);
extern uint8_t  AMG8802_Temperature_Collection(uint16_t *pAMG8802_TEMPERATURE);
extern uint8_t  AMG8802_Current1_Collection(uint16_t *pAMG8802_Current);
extern uint8_t  AMG8802_Current2_Collection(uint16_t *pAMG8802_Current2);
extern uint8_t  AMG8802_PackVoltage_Collection(uint16_t *pAMG8802_PackVolt);
extern uint8_t  AMG8802_WorkState_Collection(void);
extern uint8_t  AMG8802_MosfetStatus_Collection(void);
extern uint8_t  AMG8802_Capacity_Collection(void);
extern uint8_t  AMG8802_Balance_Collection(void);
extern uint8_t  AMG8802_PackInfo_Collection(void);
extern uint8_t  AMG8802_AlarmInfo_Collection(void);
extern uint8_t  AMG8802_ConfigCollection(AMG8802_CONFIG *pAMG8802_CONFIG);
extern void     AMG8802_RegCollection(AMG8802_REG *pAMG8802_REG);
extern int16_t  AMG8802_CurrentoffsetRead(void);
extern void AMG8802_TempCollectionTrigScan(unsigned short * p_tempdata,unsigned char ch);

extern void AMG8802_TempCollection(unsigned short * p_tempdata);

extern uint8_t AMG8802_BalanceSetting(uint8_t BalanceEnable,uint8_t BalanceController,uint8_t BalanceOpenVoltage,uint8_t BalanceDeltaVoltage,uint8_t BalanceMode);


#endif

