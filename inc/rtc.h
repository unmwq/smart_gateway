#ifndef __RTC_H
#define __RTC_H	    
//Mini STM32������
//RTCʵʱʱ�� ��������			 
//����ԭ��@ALIENTEK
//2010/6/6
//#include "sys.h"
//ʱ��ṹ��
typedef struct 
{
	u8 hour;
	u8 min;
	u8 sec;	
  //�ṹ���ڴ�����ڴ������֣��ڴ��д˴������һ��0x00������ָ������
        //������������
	u16 w_year;
	u8  w_month;
	u8  w_date;
	u8  week;		 
}_calendar_obj;					 
extern _calendar_obj calendar;	//�����ṹ��

typedef struct 
{
	u8 battery_adcbai;
	u8 battery_adcshi;
	u8 wifi_apr;
	
}adc_apr;
extern adc_apr change;	

extern u8 const mon_table[12];	//�·��������ݱ�
//void Disp_Time(u8 x,u8 y,u8 size);//���ƶ�λ�ÿ�ʼ��ʾʱ��
//void Disp_Week(u8 x,u8 y,u8 size,u8 lang);//��ָ��λ����ʾ����
int RTC_Init(void);        //��ʼ��RTC,����0,ʧ��;1,�ɹ�;
int Is_Leap_Year(u16 year);//ƽ��,�����ж�
int TIME_Get(u32 TIMCOUNT);         //����ʱ��   
int Adc_Get();
int RTC_Get_Week(u16 year,u8 month,u8 day);
int TIME_Set(u16 syear,u8 smon,u8 sday,u8 hour,u8 min,u8 sec);//����ʱ��			 
#endif


