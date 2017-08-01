#ifndef __RTC_H
#define __RTC_H	    
//Mini STM32开发板
//RTC实时时钟 驱动代码			 
//正点原子@ALIENTEK
//2010/6/6
//#include "sys.h"
//时间结构体
typedef struct 
{
	u8 hour;
	u8 min;
	u8 sec;	
  //结构体内存对齐在此有体现，内存中此处多存了一个0x00；可以指定长度
        //公历日月年周
	u16 w_year;
	u8  w_month;
	u8  w_date;
	u8  week;		 
}_calendar_obj;					 
extern _calendar_obj calendar;	//日历结构体

typedef struct 
{
	u8 battery_adcbai;
	u8 battery_adcshi;
	u8 wifi_apr;
	
}adc_apr;
extern adc_apr change;	

extern u8 const mon_table[12];	//月份日期数据表
//void Disp_Time(u8 x,u8 y,u8 size);//在制定位置开始显示时间
//void Disp_Week(u8 x,u8 y,u8 size,u8 lang);//在指定位置显示星期
int RTC_Init(void);        //初始化RTC,返回0,失败;1,成功;
int Is_Leap_Year(u16 year);//平年,闰年判断
int TIME_Get(u32 TIMCOUNT);         //更新时间   
int Adc_Get();
int RTC_Get_Week(u16 year,u8 month,u8 day);
int TIME_Set(u16 syear,u8 smon,u8 sday,u8 hour,u8 min,u8 sec);//设置时间			 
#endif


