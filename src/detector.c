/************************************************************
* Copyright (C), 2009-2011, Donjin Tech. Co., Ltd.
* FileName:		// 文件名
* Author:			// 作者
* Date:			// 日期
* Description:		// 模块描述
* Version:			// 版本信息
* Function List:	// 主要函数及其功能
*     1. -------
* History:			// 历史修改记录
*     <author>  <time>   <version >   <desc>
*     David    96/10/12     1.0     build this moudle
***********************************************************/
#include "detector.h"
#include "timer.h"
#include "delay.h"
#include "usart.h"
#include "Crc16.h"
#include "jq6500.h"
#include "KEY_LED.h"
#include "tick.h"
#include "string.h"
#include "wifi.h"
#include "Rtc.h"

#define   dev_typ_cnt   7
#define   N	        5
#define   M	       21



uint8					Receive[N];         //FOR 433 STUDY
char					rec[N][8];          //自学习地址、数据存储
uint16					rel[N][8];
uint8					have_body  = 1;
uint16					LOW_T	   = 0x30;  //440us   低电平时长
uint16					SYNO_T	   = 0x4a5; //10.7ms  同步头时长
uint16					FIRST_LT   = 0x182;
uint16					HIGH_T	   = 0x90;  //1.3ms   高电平时长


extern u8				keyflag;            //按键标志
extern u8 first_IR;
u8						first_bf = 1;

extern char				addr[35];
extern u16				port;
extern u8 GprsData[300];
extern char				*SSID;
extern char				*PassWord;
extern WIFI_INFO wifi_info;
extern Ctl_Relation    C_Relation;
extern Scenario_mode   S_MODE;
extern STOR_433_DATA	StudyDat;
extern SEND_433_DATA   SendDat;

extern KeyMenu_TypeDef	Kmenu;          //按键菜单
extern PA_data         PA_DATA;
u8 copyi;
u8 copyj;
u32						send_time = 0;  //数据发送标志
u32						net_send_timer = 0;

u8 IR_data[50];
u8 IR_memu[4];

#define ABS16( A, B ) ( ( A > B ) ? ( A - B ) : ( B - A ) )


extern IR_Data  ir_data;


extern Alarm_Flags		AlarmFlags;
extern BOOL			FlagFirstRun;

void voice_delete_app(u8 *temp,u8 i);

//初始化函数
//PB6 433_TXOUT 433发射 输出
//PB7 433_RXIN  433接收 输入
//PC0 BAT_DET   电池检测 模拟输入 adc
//PC1 IR_IN     红外输入 输入
//PC2 MV_IN     微波输入 输入
//PA11 TRIG     输出10us高电平，开始测距  输出
//PA12 echo     输入高电平，测距         输入
//距离=echo 高电平时间*340m/s / 2

void Detector_Init( void )
{GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC| RCC_APB2Periph_AFIO, ENABLE ); //使能PORTA,PORTB,PORTC时钟



//433输入管脚
GPIO_InitStructure.GPIO_Pin	   = GPIO_Pin_1;            //PB7
GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_IPU;          //设置成上拉输入
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
GPIO_Init( GPIOC, &GPIO_InitStructure );


GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);
EXTI_InitStructure.EXTI_Line	  = EXTI_Line1; 	 
EXTI_InitStructure.EXTI_Mode	  = EXTI_Mode_Interrupt;
EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
EXTI_InitStructure.EXTI_LineCmd = ENABLE;  

EXTI_Init(&EXTI_InitStructure);


//中断分配
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //占先优先级、副优先级的资源分配 
NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn; 
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;	 
NVIC_InitStructure.NVIC_IRQChannelCmd  = ENABLE;
NVIC_Init(&NVIC_InitStructure);	 
NVIC_DisableIRQ(EXTI1_IRQn);


}


//初始化ADC
//这里我们仅以规则通道为例
//我们默认将开启通道0~3																	   
void  Adc_Init(void)
{ 	
  ADC_InitTypeDef ADC_InitStructure; 
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1, ENABLE );	  //使能ADC1通道时钟
  
  
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M
  
  //PA1 作为模拟通道输入引脚                         
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚
  GPIO_Init(GPIOA, &GPIO_InitStructure);	
  
  ADC_DeInit(ADC1);  //复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值
  
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC工作模式:ADC1和ADC2工作在独立模式
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//模数转换工作在单通道模式
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//模数转换工作在单次转换模式
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//转换由软件而不是外部触发启动
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC数据右对齐
  ADC_InitStructure.ADC_NbrOfChannel = 1;	//顺序进行规则转换的ADC通道的数目
  ADC_Init(ADC1, &ADC_InitStructure);	//根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   
  
  
  ADC_Cmd(ADC1, ENABLE);	//使能指定的ADC1
  
  ADC_ResetCalibration(ADC1);	//使能复位校准  
  
  while(ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
  
  ADC_StartCalibration(ADC1);	 //开启AD校准
  
  while(ADC_GetCalibrationStatus(ADC1));	 //等待校准结束
  
  //	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能
  
}				  


void Delay(uint16_t time)
{      
  TIM3->CNT = 0;
  TIM3->CR1|=0x01;    //使能定时器3
  while(TIM3->CNT!=time);
  TIM3->CR1&=0xFFFE;    //关闭定时器3
  TIM3->CNT = 0;
  
}


void DELAY(uint16_t time)
{      
  TIM5->CNT = 0;
  TIM5->CR1|=0x01;    //使能定时器3
  while(TIM5->CNT!=time);
  TIM5->CR1&=0xFFFE;    //关闭定时器3
  TIM5->CNT = 0;
  
}




void Data_out_0()
{
  DATA_HIGH;
  DELAY(100);//4.8ms
  DATA_LOW;
  DELAY(100);//1.5ms
}
void Data_out_1()
{
  DATA_HIGH;
  DELAY(100);//4.8ms
  DATA_LOW;
  DELAY(300);//1.5ms
}
void Data_out_start()
{
  DATA_HIGH;
  DELAY(200);//4.8ms
  DATA_LOW;
  DELAY(600);//1.5ms
}




void jxr_kaguan(u8 *data)
{
  u8 i,j,k;
  u8 val;
  u8 datatemp;
  
  // datatemp=data[0];
  Set_Ask_TxMode();
  
  Data_out_0();
  Data_out_1();
  Data_out_0();
  Data_out_1();
  for(i=0;i<5;i++)
  {
    
    Data_out_start();
    for(j=0;j<40;j=j+2){
      
      if(j%8==0) datatemp =data[j/8];
      
      val=datatemp&0xc0;
      switch(val)
      {
      case 0x0:
        Data_out_0();
        Data_out_0();
        break;
      case 0x40:
        Data_out_0();
        Data_out_1();
        break;
      case 0x80:
        Data_out_1();
        Data_out_0();
        break;
      case 0xc0:
        Data_out_1();
        Data_out_1();
        break;
      }
      
      datatemp=datatemp<<2;
    }
    
  }
  Set_Ask_RxMode();
  
}



void C_Sys_out(u16 SYNO)
{
  
  DATA_HIGH;
  Delay(84);
  DATA_LOW;
  Delay(SYNO);
}

void Sys_out(u16 SYNO)
{
  
  DATA_HIGH;
  if(SYNO<800)
    Delay(SYNO>>4);
  else
    Delay(SYNO/25);
  DATA_LOW;
  Delay(SYNO);
}
void Data_out(u8 LOW,u8 HIGH)
{
  DATA_HIGH;
  Delay(531);//4.8ms
  DATA_LOW;
  Delay(166);//1.5ms
  DATA_HIGH;
  Delay(LOW);
  DATA_LOW;
  Delay(HIGH);
  
  
  
}

void Data0_out(u16 First_LT,u8 LOW,u8 HIGH)
{
  if((First_LT&0x3FF)== 0)
  {
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    Delay(HIGH);
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    Delay(HIGH);
  }
  else
  {
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(First_LT&0x3FF);
    else	  Delay(HIGH);
    
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(HIGH);
    else  Delay(First_LT&0x3FF);
  }
}


void Data1_out(u16 First_LT,u8 LOW,u8 HIGH)
{
  if((First_LT&0x3FF)== 0)
  {
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    Delay(LOW);
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    Delay(LOW);
  }
  else
  {
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(First_LT&0x3FF);
    else Delay(LOW);
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(LOW);
    else Delay(First_LT&0x3FF);
  }
}

void DataF_out(u16 First_LT,u8 LOW,u8 HIGH)
{
  if((First_LT&0x3FF)== 0)
  {
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    Delay(HIGH);
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    Delay(LOW);
  }
  else
  {
    
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(First_LT&0x3FF);
    else Delay(HIGH);
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(LOW);
    else Delay(First_LT&0x3FF);
  }
}

void Non_out(u16 First_LT,u8 LOW,u8 HIGH)
{
  if((First_LT&0x3FF)== 0)
  {
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    Delay(LOW);
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    Delay(HIGH);
  }
  else
  {
    
    DATA_HIGH;
    Delay(HIGH);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(First_LT&0x3FF);
    else Delay(LOW);
    DATA_HIGH;
    Delay(LOW);
    DATA_LOW;
    if((((uint8_t)((First_LT&0x1C00)>>10)-1)%2)==0) Delay(HIGH);
    else Delay(First_LT&0x3FF);
  }
}

void send_out_433(uint8_t temp,u16 First_LT,u8 LOW,u8 HIGH)
{
  
  switch(temp&0xc0)
  {
  case 0x00 :
    Data1_out(First_LT,LOW,HIGH);
    break;
  case 0x40:
    Non_out(First_LT,LOW,HIGH);
    break;
  case 0x80:
    DataF_out(First_LT,LOW,HIGH);
    break;
  case 0xc0:
    Data0_out(First_LT,LOW,HIGH);
    break;
  default: break;
  }
}
//输出 模拟波形发出

//输出 模拟波形发出
void cl_out(u8 * pdata,u16 SYNO, u16 First_LT,u8 LOW, u8 HIGH)
{
  uint8_t i,j,m;
  uint8_t *p;
  uint8_t temp;
  // SYNO = 984;
  // LOW=0x2C;
  //  HIGH=0X54;
  IWDG_Feed();
  //z=First_LT&0x03FF;//低电平时间长度
  
  for(m=0;m<8;m++)//连续发射5个波形
  {
    p =pdata;
    temp =*p;
    // delay_ms(50);
    C_Sys_out(SYNO);
    for(i=0;i<N;i++)//N个字
    {
      for(j=0;j<4;j++)//一个字
      {
        
	if((i==0)&&(j==0)){
          Data_out(LOW,HIGH);
          temp <<=2;
          j++;
        }
        
        send_out_433(temp,0,LOW,HIGH);
        temp <<=2;
      }
      p++;
      temp =*p;
    }
  }//end 多个波形
}//fashe_out



void fashe_out(u8 * pdata,u16 SYNO, u16 First_LT,u8 LOW, u8 HIGH,u8 times)
{
  uint8_t i,j,m;
  uint8_t *p;
  uint8_t temp;
  uint8_t x,y;
  x=(uint8_t)((First_LT&0xE000)>>13);//宽低电平字节位置
  y=(uint8_t)((First_LT&0x1C00)>>10);//宽低电平位 位置
  IWDG_Feed();
  for(m=0;m<times;m++)//连续发射5个波形
  {
    p =pdata;
    temp =*p;
    
    
    CPU_IntDis();
    // CPU_IntEn();
    Sys_out(SYNO);
    if(First_LT == 0)//无需添加宽低电平
    {
      for(i=0;i<N;i++)//N个字
      {
        for(j=0;j<4;j++)//一个字
        {
          if((i==3)&&(*(p+1)==0))
          {
            i=5;
            j=4;
            break; 
          }
          send_out_433(temp,0,LOW,HIGH);
          temp <<=2;
        }
        p++;
        temp =*p;
      }
    }
    else//需要添加宽低电平
    {
      for(i=0;i<N;i++)//三个字
      {
        for(j=0;j<4;j++)//一个字
        {
          if(i == x)//查找到是第几个字节需要加宽低电平
          {
            if(j != y/2)//找到发射宽低电平位置
            {
              send_out_433(temp,0,LOW,HIGH);
            }
            else//找到位置了
            {
              send_out_433(temp,First_LT,LOW,HIGH);  
            }
          }
          else
          {
            send_out_433(temp,0,LOW,HIGH);
          }
          temp <<=2;
        }//结束单个字发射
        p++;
        temp =*p;
      }
    }//end增加宽低电平发射
    
    // CPU_IntDis();
    CPU_IntEn();
    
  }//end 多个波形
}//fashe_out

int measure_level()
{
  u8 rec_val=3;
  u16 tempdata[2]={0};
  u32 tiem_measure=0;
  tiem_measure=TickGet();
  TIM5->CR1 = 0x01;
  while(CSIO_DATA!=0)
  {
    if(TickGet()-tiem_measure>100)
    {
      return 0;
    }
    IWDG_Feed();
  }
  
  tempdata[0]= TIM5->CNT;
  //i++;
  
  TIM5->CR1  = 0x00;          //关定时器
  TIM5->CNT  = 0;             //计数清零
  
  TIM5->CR1 = 0x01;
  while(CSIO_DATA==0)
  {
    if(TickGet()-tiem_measure>200)
    {
      return 0;
    }
    IWDG_Feed();
  }
  
  tempdata[1]= TIM5->CNT;
  //i++;
  
  TIM5->CR1  = 0x00;          //关定时器
  TIM5->CNT  = 0;             //计数清零
  
  if((tempdata[0]>50)&&(tempdata[0]<150))
  {
    if((tempdata[1]>50)&&(tempdata[1]<150))
      return 0;
    if((tempdata[1]>220)&&(tempdata[1]<380))
      return 1;
  }
  else if((tempdata[0]>150)&&(tempdata[0]<250))
  {
    if((tempdata[1]>450)&&(tempdata[0]<750))
      return 2;
  }
  else 
    return 3;
  
  return rec_val;
  
}

unsigned char crol(unsigned char c,unsigned b){  
  unsigned char left=c<<b;  
  unsigned char right=c>>(8-b);
  unsigned char temp=left|right;  
  return temp;  
}

int get_crc(u8 *temp1)
{
  u8 val=0;
  val=temp1[0]+temp1[1]+temp1[2]+temp1[3];
  // val=val<<2;
  val=crol(val,2);
  val=((u8)val)^0xff;
  val=val+0x33;
  return val;
} 

void data_IR_0(u16 L_L)
{
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
  DELAY(550);
  
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
  DELAY(L_L);  
  
}
void data_IR_1(u16 L_H)
{
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
  DELAY(550);
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
  DELAY(L_H);  
}
void IR_out_start(u16 S_L,u16 S_H)
{
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
  DELAY(S_L);
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
  DELAY(S_H);  
  
}

void IR_out_over()
{
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
  DELAY(550);
  
  
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
  DELAY(10000); 
}

void IR_OUT(u8 *data)
{
  u8 i,j,k;
  u8 val;
  u8 datatemp;
  u8 num;
  u16 data_temp;
  u16 S_L,S_H,L_L,L_H;
  
  //LED_send(200);
  
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  CPU_IntDis();
  
  S_L=((data[4]<<8)|data[3]);
  S_H=((data[6]<<8)|data[5]);
  
  if(data[0]==xiaomi)
  {
    num=data[7];
    for(j=0;j<5;j++){
      IR_out_start(1000,530);
      
      GPIO_Config();
      TIM_CtrlPWMOutputs(TIM1, ENABLE);
      
      for(i=0;i<(num);i+=2)
      {
        
        data_temp=((data[8+i+1]<<8)|data[8+i]);
        
        GPIO_Config();
        TIM_CtrlPWMOutputs(TIM1, ENABLE);
        TIM_Cmd(TIM1, ENABLE);
        DELAY(625);
        TIM_CtrlPWMOutputs(TIM1, DISABLE);
        GPIO_IOOUT();     
        TIM_Cmd(TIM1, DISABLE); 
        DELAY(data_temp);  
        
      }
      
      //IR_out_over();
      GPIO_Config();
      TIM_CtrlPWMOutputs(TIM1, ENABLE);
      TIM_Cmd(TIM1, ENABLE);
      DELAY(625);
      TIM_CtrlPWMOutputs(TIM1, DISABLE);
      TIM_Cmd(TIM1,DISABLE); 
      GPIO_IOOUT();
      
      DELAY(10000);
    }
    EXTI_ClearFlag(EXTI_Line1);	
    CPU_IntEn();
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    TIM_Cmd(TIM1,DISABLE); 
    GPIO_IOOUT();
    return ;
  }
  // else if(data[0]==letv)
  // {
  
  // }
  num=data[7]*8;
  L_L=((data[9]<<8)|data[8]);
  L_H=((data[11]<<8)|data[10]);
  
  for(i=0;i<1;i++)
  {
    IR_out_start(S_L,S_H);
    
    
    for(j=0;j<num;j=j+2){
      
      if(j%8==0) datatemp =data[(j/8)+12];
      
      val=datatemp&0xc0;
      switch(val)
      {
      case 0x0:
        data_IR_0(L_L);
        data_IR_0(L_L);
        break;
      case 0x40:
        data_IR_0(L_L);
        data_IR_1(L_H);
        break;
      case 0x80:
        data_IR_1(L_H);
        data_IR_0(L_L);
        break;
      case 0xc0:
        data_IR_1(L_H);
        data_IR_1(L_H);
        break;
      }
      datatemp=datatemp<<2;
    }
    
    IR_out_over();
    if(data[0]==letv)
    {
      TIM_CtrlPWMOutputs(TIM1, DISABLE);
      TIM_Cmd(TIM1,DISABLE); 
      GPIO_IOOUT();
      DELAY(30000); 
      GPIO_Config();
      TIM_CtrlPWMOutputs(TIM1, ENABLE);
      TIM_Cmd(TIM1, ENABLE);
      DELAY(9000);
      TIM_CtrlPWMOutputs(TIM1, DISABLE);
      TIM_Cmd(TIM1,DISABLE); 
      GPIO_IOOUT();
      DELAY(2000); 
      GPIO_Config();
      TIM_CtrlPWMOutputs(TIM1, ENABLE);
      TIM_Cmd(TIM1, ENABLE);
      DELAY(571);
      TIM_CtrlPWMOutputs(TIM1, DISABLE);
      TIM_Cmd(TIM1,DISABLE); 
      GPIO_IOOUT();
    }
  }
  EXTI_ClearFlag(EXTI_Line1);	
  CPU_IntEn();
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
}
/*
void IR_OUT(u8 *data)
{
u8 i,j,k;
u8 val;
u8 datatemp;
u8 num;
u16 data_temp;
u16 S_L,S_H,L_L,L_H;

//LED_send(200);

GPIO_Config();
TIM_CtrlPWMOutputs(TIM1, ENABLE);
CPU_IntDis();

S_L=((data[4]<<8)|data[3]);
S_H=((data[6]<<8)|data[5]);



if(data[0]==xiaomi)
{
num=data[7];
for(j=0;j<5;j++){
IR_out_start(1000,530);

for(i=0;i<(num);i+=2)
{

data_temp=((data[8+i+1]<<8)|data[8+i]);
TIM_Cmd(TIM1, ENABLE);
Delay(625);
TIM_Cmd(TIM1, DISABLE); 
Delay(data_temp);  

  }

//IR_out_over();
TIM_Cmd(TIM1, ENABLE);
Delay(625);
TIM_Cmd(TIM1, DISABLE); 
Delay(10000);
  }
EXTI_ClearFlag(EXTI_Line1);	
CPU_IntEn();
TIM_CtrlPWMOutputs(TIM1, DISABLE);
TIM_Cmd(TIM1,DISABLE); 
GPIO_IOOUT();
return ;
  }
num=data[7]*8;
L_L=((data[9]<<8)|data[8]);
L_H=((data[11]<<8)|data[10]);

for(i=0;i<1;i++)
{
IR_out_start(S_L,S_H);
for(j=0;j<num;j=j+2){

if(j%8==0) datatemp =data[(j/8)+12];

val=datatemp&0xc0;
switch(val)
{
    case 0x0:
data_IR_0(L_L);
data_IR_0(L_L);
break;
    case 0x40:
data_IR_0(L_L);
data_IR_1(L_H);
break;
    case 0x80:
data_IR_1(L_H);
data_IR_0(L_L);
break;
    case 0xc0:
data_IR_1(L_H);
data_IR_1(L_H);
break;
    }
datatemp=datatemp<<2;
    }
IR_out_over();
  }
EXTI_ClearFlag(EXTI_Line1);	
CPU_IntEn();
TIM_CtrlPWMOutputs(TIM1, DISABLE);
TIM_Cmd(TIM1,DISABLE); 
GPIO_IOOUT();
}
*/
void hand_SET_IR_MODE(u8 *data)
{
  memcpy(&ir_data.IR_info[data[0]][0],&data[1],100);
  Save_ir_data(&ir_data);
  
}
void hand_DELETE_IR_MODE(u8 data)
{
  memset(&ir_data.IR_info[data][0],0,100);
  Save_ir_data(&ir_data);
  
}


void hand_ir_mode(u8 type)
{ u8 len;
u8 len1;
u8 len2;
u8 temp[100];
if(type<5)
{
  if(ir_data.IR_info[type][0]==1){
    
    if(ir_data.IR_info[type][1]==xiaomi){
      len=ir_data.IR_info[type][8]+8;
    }
    else
    {
      len=ir_data.IR_info[type][8]+12;
    }
    if(len<8||len>50)
    {
      return;
    }
    memcpy(temp,&ir_data.IR_info[type][1],len);
    IR_OUT(&temp[0]);
  }
  else if(ir_data.IR_info[type][0]==2)
  {
    if(ir_data.IR_info[type][1]==xiaomi){
      len=ir_data.IR_info[type][8]+8;
    }
    else
    {
      len=ir_data.IR_info[type][8]+12;
    }
    if(len<8||len>50)
    {
      return;
    }
    memcpy(temp,&ir_data.IR_info[type][1],len);
    IR_OUT(&temp[0]);
    
    if(ir_data.IR_info[type][len+1]==xiaomi)
    {
      len1=ir_data.IR_info[type][len+8]+8;
    }
    else
    {
      len1=ir_data.IR_info[type][len+8]+12;
    }
    
    if(len<8||len>50)
    {
      return;
    }memcpy(temp,&ir_data.IR_info[type][len+1],len1);
    IR_OUT(temp);
  }else if(ir_data.IR_info[type][0]==3){
    
    if(ir_data.IR_info[type][1]==xiaomi){
      len=ir_data.IR_info[type][8]+8;
    }
    else
    {
      len=ir_data.IR_info[type][8]+12;
    }
    
    if(len<8||len>50)
    {
      return;
    }
    memcpy(temp,&ir_data.IR_info[type][1],len);
    IR_OUT(&temp[0]);
    
    if(ir_data.IR_info[type][len+1]==xiaomi)
    {
      len1=ir_data.IR_info[type][len+8]+8;
    }
    else
    {
      len1=ir_data.IR_info[type][len+8]+12;
    }
    if(len<8||len>50)
    {
      return;
    }
    memcpy(temp,&ir_data.IR_info[type][1+len],len1);
    IR_OUT(temp);
    
    if(ir_data.IR_info[type][len+1]==xiaomi)
    {
      len2=ir_data.IR_info[type][len+len1+8]+8;
    }
    else
    {
      len2=ir_data.IR_info[type][len+len1+8]+12;
    }
    
    //len2=ir_data.IR_info[type][len+len1+8]+12;
    if(len<8||len>50)
    {
      return;
    }
    memcpy(temp,&ir_data.IR_info[type][1+len+len1],len2);
    IR_OUT(temp);
    
  }
}
}

u16 s[200];
u8 frist_send_rf=0;
void ir_out_handler(int rf_times)
{
  u16 i=0;
  GPIO_Config();
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  CPU_IntDis();
  // CPU_IntEn();
  if(s[0]>800&&s[1]>300){
    for(i=0;i<rf_times;){ 
      if(i%2==0)
      {
        TIM1->CR1 |= TIM_CR1_CEN;
        
        Delay(s[i]);
        
        i++;
      }
      else 
      {
        //  remote_high();
        // TIM_SetCompare1(TIM1,0); 
        TIM_Cmd(TIM1, DISABLE); 
        // DATA_HIGH;
        //  TIM1->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
        Delay(s[i]);
        //delay_us(10*s[i]);
        i++;
      }
    }
  }
  // remote_stop();
  // TIM1->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
  EXTI_ClearFlag(EXTI_Line1);	
  // CPU_IntDis();
  CPU_IntEn();
  // TIM1->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
  //TIM_ForcedOC4Config(TIM1, TIM_ForcedAction_InActive);
  TIM_CtrlPWMOutputs(TIM1, DISABLE);
  TIM_Cmd(TIM1,DISABLE); 
  GPIO_IOOUT();
}

void Get_nec(u16 sync_L,u16 sync_H,u8 num)
{   
  u8 k = 1;  //元素数,编码数据从 s[3]开始
  u8 n = 0;   //字节数
  u8 m = 0;   //位数
  u8 x = 0;   //位或值
  u8 i,j,k_flag1=0,k_flag2=0;
  u16 SN_ll=600,SN_lh=1200;
  memset(IR_data,0,sizeof(IR_data));
  IR_data[0] = IR_memu[0]; //每次都要初始化，否则影响之后的位或运算
  IR_data[1] = IR_memu[1];
  IR_data[2] = IR_memu[2];
  IR_data[3] = IR_memu[3];
  IR_data[4] = sync_L; //每次都要初始化，否则影响之后的位或运算
  IR_data[5] = sync_L>>8;
  IR_data[6] = sync_H;
  IR_data[7] = sync_H>>8;
  
  //LED_send(100);
  
  if(IR_data[0]==xiaomi)
    
  {
    //低电平625us
    
    IR_data[8] =num;
    
    // memcpy(&IR_data[8],&s[0],num)
    // IR_data[7] =(num/2);
    for(i=1,j=1;i<(num);i+=2)
    {
      IR_data[8+j]=(u8)(s[i]);
      IR_data[8+j+1]=(u8)(s[i]>>8);
      j+=2;
    }
    NVIC_DisableIRQ(EXTI1_IRQn);
    EXTI_ClearFlag(EXTI_Line1);
    return ;
  }
  //小米
  //索尼
  //nec
  if(sync_L>3500){
    
    IR_data[8] = (num/16);
    
    for(i=1;i<num;i+=2)
    {
      if(s[i] > 1500&&(k_flag1==0))
      {
        IR_data[9]=s[i];
        IR_data[10]=s[i]>>8;
        k_flag1=1;
        //  i+=2;
      }else if((s[i] > 400)&&(k_flag2==0)&&(s[i]<800))
      {
        IR_data[11]=s[i];
        IR_data[12]=s[i]>>8;
        k_flag2=1;
        //i+=2;
      }else if((k_flag1==1)&&(k_flag2==1))
      {
        break;
      }
    }
    
    for(n=0;n<IR_data[8];n++)  //四个字节
    {
      for(m=0;m<8;m++)  //八位
      {
        if(s[k] > 1500)   //高电平时间大于1500us为1
        {
          x = 0x80>>m;    //0x80右移零位，一位,两位。。等于0x80,0x40,0x20...0x01
          IR_data[n+13] = IR_data[n+13] | x;  //如果第一位为1，值为0x80，按位或
        }
        k = k+2;       //跳过低电平，至下一高电平
      }
      
      
    }
    
  }
  else 
  {
    IR_data[8] = (i/8);
    IR_data[9]=SN_lh;
    IR_data[10]=SN_lh>>8;
    IR_data[11]=SN_ll;
    IR_data[12]=SN_ll>>8;
    k=0;
    for(n=0;n<IR_data[8];n++)  //四个字节
    {
      for(m=0;m<8;m++)  //八位
      {
        if(s[k] > 1000)   //高电平时间大于1500us为1
        {
          x = 0x80>>m;    //0x80右移零位，一位,两位。。等于0x80,0x40,0x20...0x01
          IR_data[n+13] = IR_data[n+13] | x;  //如果第一位为1，值为0x80，按位或
        }
        k = k+1;       //跳过低电平，至下一高电平
      }
      
    }
    
  }
  //解析完成 
  if(IR_data[8]==0)
  {
    return;
  }
  
  NVIC_DisableIRQ(EXTI1_IRQn);
  EXTI_ClearFlag(EXTI_Line1);
  //  wifi_sendcmd((BYTE *)IR_data,IR_data[7]+12,Study_IR,0);
  //   handly_get_value(1,5); 
  
  // printf("解析完成\r\n");
}



void hand_study_IR(u8 *data)
{
  IR_memu[0]=data[0];
  IR_memu[1]=data[1];
  IR_memu[2]=data[2];
  IR_memu[3]=data[3];
  //bee(100);
  NVIC_EnableIRQ(EXTI1_IRQn);
  
  
}

int ir_in_handler()
{
  u16 i=0;
  u16 sync_L;
  u16 sync_H; 
  memset(&s[0],0,sizeof(s));
  TIM5->CR1  = 0x01;          //关定时器
  TIM5->CNT  = 0;             //计数清零
  while(IR_DATA==0)
  {
    IWDG_Feed();
  }
  sync_L= TIM5->CNT;
  //  i++;
  TIM5->CNT  = 0;             //计数清零
  
  while(IR_DATA!=0)
  {
    IWDG_Feed();
    if(TIM5->CNT>10000)
    {
      printf("监测出错");
      return 0;
    }
  }
  
  sync_H= TIM5->CNT;
  TIM5->CNT  = 0;             //计数清零
  
  
  for(i=0;i<200;)
  {
    
    
    while(IR_DATA==0)
    {
      IWDG_Feed();
    }
    
    s[i]= TIM5->CNT;
    TIM5->CNT  = 0;             //计数清零
    if(3500<s[i])
    {
      printf("监测完成");
      Get_nec(sync_L,sync_H,i-1);
      return i;
    }
    i++;
    while(IR_DATA!=0)
    {
      IWDG_Feed();
      if(TIM5->CNT>10000)
      {
        Get_nec(sync_L,sync_H,i-1);
        return i;
      }
    }
    
    s[i]= TIM5->CNT;
    TIM5->CNT  = 0;             //计数清零
    if(s[i]>3500)
    {   
      Get_nec(sync_L,sync_H,i-1);
      return i;    
    }
    i++;
  }
  
}

void hand_IR_data()
{
  if(first_IR!=0)
  {
    first_IR=0;
    if(IR_data[7]<3)
    {
      return;
    } 
    if(IR_data[0]==xiaomi){
      //   IR_OUT(IR_data);
      wifi_sendcmd((BYTE *)IR_data,IR_data[7]+8,Study_IR,GprsData[4]);
      handly_get_value(1,5);
    }else 
    {
      //  IR_OUT(IR_data);
      wifi_sendcmd((BYTE *)IR_data,IR_data[7]+12,Study_IR,GprsData[4]);
      handly_get_value(1,5);
    }
    //bee_bee(100,100);
  }
}


int wxkg_study()
{
  
  u16 i=0,j=0;
  u16 val=0;
  u8 temp1[5]={0};
  u8 temp[100]={0};
  
  static u32 time_jxr=0;
  time_jxr=TickGet();
  
  while(1){
    
    while(CSIO_DATA==0)
    {
      if(TickGet()-time_jxr>100)
      {
        return 0;
      }
      IWDG_Feed();
    }
    temp[0]=measure_level();
    
    if(temp[0]==0)
    {
      temp[1]=measure_level();
      if(temp[1]==1)
      {
        temp[2]=measure_level();
        if( temp[2]==0)
        {
          temp[3]= measure_level();
          if(temp[3]==1)
          {
            for(i=0;i<100;i++)
            {
              temp[4+i]= measure_level();
              // if(temp[4+i] ==3) return 0;
            }
            for(i=0;i<100;i++)
            {
              if(temp[i]==0x02)
              {
                i++;
                break;
              }
            }
            
            
            for(j=0;j<40,i<100;i++,j++)
            {
              temp1[j/8]=temp1[j/8]<<1;
              temp1[j/8]+=temp[i];
            }
            // printf("同步头解析完成,开始解析数据\r\n");
            /*  val=temp1[0]+temp1[1]+temp1[2]+temp1[3];
            // val=val<<2;
            val=crol(val,2);
            val=((u8)val)^0xff;
            val=val+0x33;
            */
            val=get_crc(&temp1[0]);
            
            
            if(val==temp1[4])
            {
              
              temp1[3]=0x41;
              temp1[4]=get_crc(&temp1[0]);
              memcpy(&temp[1],temp1,5);
              temp1[3]=0x44;
              temp1[4]=get_crc(&temp1[0]);
              memcpy(&temp[6],temp1,5);
              
              Specify_musi_play(43);             //匹配成功
              delay_ms(500);
              TIME_Get( TIM2_tick );                  //获取系统时间
              temp[0]=0x23;
              //   memcpy(&temp[1],temp1,5);
              
              temp[11]=calendar.hour;
              temp[12]=calendar.min;
              temp[13]=calendar.sec;
              temp[14]= AlarmFlags.bIsRmt_Deploy;
              wifi_sendcmd((BYTE *)temp,15,Study_433_device,GprsData[4]); 
              handly_get_value(1,5);     
              return 0;
              
            }
            
            // jxr_kaguan(temp1);   
            
          }
        }
      }
    }
    if(TickGet()-time_jxr>4000)
    {
      
      return 0;
    }
    memset(temp,0,sizeof(temp));
  }
  
}



void Jiema( uint8 leixing )
{
  u8			i  = 0;
  u8			j  = 0;
  u16			temp_l;         //用于判断波形高低电平时长
  u8			k		   = 0;
  u8			m		   = 0;
  u8			PulseCnt   = 0; //丢弃第一次的同步头
  static u32	jiema_time = 0;
  u16			timer3	   = 0;
  for( i = 0; i < N; i++ )
  {
    Receive[i] = 0x00;
  }
  while( 1 )
  {
    IWDG_Feed();
    
    jiema_time = TickGet( );
    TIM4->CNT  = 0;
    while( ( CSIO_DATA ) != 0 ) //等待低电平(需要一个等待超时，防止该管脚一直高电平，无法退出)
    {
      if( TickGet( ) - jiema_time > 1000 )
      {
        LED3_ON;            //检查315接收模块是否损坏
        return;
      }
    }
    
    TIM4->CR1 = 0x01;           //开定时器
    while( ( CSIO_DATA ) == 0 ) //等待高电平
    {
      IWDG_Feed();
      if( TickGet( ) - jiema_time > 1000 )
      {
        LED3_ON;            //检查315接收模块是否损坏
        return;
      }
    }
    
    //temp= TIM4->CNT ;
    SYNO_T	   = TIM4->CNT;
    TIM4->CR1  = 0x00;          //关定时器
    TIM4->CNT  = 0;             //计数清零
    
    TIM3->CR1  = 0x01;          //开启定时器
    k++;                        //一定次数判断，跳出JIEMA函数，防止死循环
    if( SYNO_T < 800 )
    {
      temp_l = SYNO_T >> 4;
    } else
    {
      temp_l = SYNO_T >> 5;
    }
    //SYNO_T = temp;//同步头时长，用于模拟波形发送
    //temp = 0;
    //  if( ( 0x20b< temp ) && ( temp < 0x3F0) )//取2.9ms----21.87ms同步头
    if( ( 0x200 < SYNO_T ) && ( SYNO_T < 0x5F0 ) )  //取2.9ms----21.87ms同步头
    {
      // UART4_printf("temp=%d \r\n",temp);
      PulseCnt++;
      m++;                                        //防止干扰，一直死循环，让其跳出循环，继续下一次检测
      if( PulseCnt != 1 )                         //第一次检测到同步码，丢弃
      {
        TIM3->CR1 = 0x00;                       //关闭定时器
        continue;
      }
      for( j = 0; j < N; j++ )                    //循环4次
      {
        for( i = 0; i < 8; i++ )
        {
          while( ( CSIO_DATA ) == 0 )         //等待高电平
          {
            if( TickGet( ) - jiema_time > 1000 )
            {
              LED3_ON;
              //检查315接收模块是否损坏
              return;
            }
          }
          TIM4->CR1  = 0x01;          //开启定时器
          rel[j][i]  = TIM3->CNT;
          
          TIM3->CR1  = 0x00;          //关闭定时器
          TIM3->CNT  = 0;
          if( rel[j][i] > ( SYNO_T - 100 ) )
          {
            if( j < 3 )
            {
              TIM3->CR1  = 0x00;  //关闭定时器
              TIM4->CR1  = 0x00;  //关闭定时器
              TIM3->CNT  = 0;
              TIM4->CNT  = 0;
              return;
            }else
            {
              i  = 8;
              j  = N;
              break;
            }
          }
          while( ( CSIO_DATA ) != 0 )                                                             //等待低电平
          {
            IWDG_Feed();	
            if( TickGet( ) - jiema_time > 1000 )
            {
              LED3_ON;                                                                        //检查315接收模块是否损坏
              return;
            }
          }
          
          TIM3->CR1  = 0x01;                                                                      //开启定时器
          rec[j][i]  = TIM4->CNT;
          // UART4_printf("t(%d,%d)=%d \r\n",j,i,timer4);
          TIM4->CR1  = 0x00;                                                                      //关闭定时器
          TIM4->CNT  = 0;
        }
      }
      for( j = 0; j < N; j++ )                                                                        //循环4次
      {
        for( i = 0; i < 8; i++ )
        {
          //UART4_printf("%d%d=%d\r\n",j,i,rec[j][i]);
          Receive[j] = Receive[j] << 1;                                                           //存下所有地址和数据
          if( ( ( temp_l / 3 ) < rec[j][i] ) && ( rec[j][i] < ( temp_l * 7 / 3 ) ) )              //判断窄脉冲
          {
            LOW_T	   = rec[j][i];                                                             //窄脉冲时间长度
            Receive[j] = Receive[j] + 0x01;
          }else if( ( ( temp_l * 8 / 3 ) < rec[j][i] ) && ( rec[j][i] < ( temp_l * 13 / 3 ) ) )   //宽脉冲
            
          {
            HIGH_T	   = rec[j][i];                                                             //宽脉冲时间长度
            Receive[j] = Receive[j] + 0x00;                                                     //<< 1 ;
          }
          if( ( rel[j][i] > ( HIGH_T + 0x50 ) ) && ( j < 3 ) )
          {
            timer3	   = rel[j][i];
            FIRST_LT   = timer3 | ( j << 13 ) | ( i << 10 );
          }else if( ( j > 2 ) && ( timer3 == 0 ) )
          {
            FIRST_LT = 0;
          }
          rec[j][i]  = 0;
          rel[j][i]  = 0;
        }
      }
      return;
    }else
    {
      switch( leixing )
      {
      case changgui:
        
        if( ( k > 40 ) || ( m > 10 ) )
        {
          return;
        }
        
      case study:
        // if((k>250)||(m>20))
        if( ( k > 100 ) || ( m > 20 ) )
          
        {
          return;
        }
        
      default:
        ;
      } /**/
      TIM3->CR1 = 0x00; //关闭定时器
    }
  }
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Study_Remote( void ) //遥控器
{
  UART4_printf( "语音播报:学习遥控器" );
  Specify_musi_play( 0x02 );  //学习遥控器
  Study( remote,GprsData[4]);
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Study_Menci( void )        //门磁
{
  UART4_printf( "语音播报:学习门磁" );
  Specify_musi_play( 0x03 );  //学习门磁窗磁
  Study( Menci,GprsData[4]);
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Study_Infrared( void )     //红外
{
  UART4_printf( "语音播报:学习红外控制器" );
  Specify_musi_play( 0x04 );  //学习红外窗幕
  Study( Infrared,GprsData[4]);
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Study_Smoke( void )        //其它
{
  UART4_printf( "语音播报:学习烟雾传感器" );
  Specify_musi_play( 0x05 );  //学习其它设备
  Study( smoke,GprsData[4]);
}
void Study_Gas( void )        //其它
{
  UART4_printf( "语音播报:学习燃气传感器" );
  Specify_musi_play(15);  //学习其它设备
  Study( gas,GprsData[4]);
}
void Study_calling( void )        //其它
{
  UART4_printf( "语音播报:学习紧急按钮" );
  Specify_musi_play(37);  //学习其它设备
  Study( arlrm_calling,GprsData[4]);
}

void Study_app(u8 type)
{
  switch (type){
  case remote:
    Study_Remote();
    break;
  case Menci:
    Study_Menci();
    break;
  case Infrared:
    Study_Infrared();
    break;
  case smoke:
    Study_Smoke();
    break;
  case gas:
    Study_Gas();
    break;
  case arlrm_calling:
    Study_calling();
    break;
  default:
    break;
  }
}

void check_sensor()
{
  u8 i,j;
  u8 k=0;
  u16 lens;
  u8 temp[300]={0};
  STOR_433_DATA	*pStudyStorDat;
  uint8		StudyCntAddr;
  pStudyStorDat = &StudyDat;
  Load433StudyDat( pStudyStorDat );
  StudyCntAddr = pStudyStorDat->Count_433;
  //lens=StudyCntAddr*6+1;
  lens=1;
  temp[0]=0;
  for( i = 0; i < NODE_CNT; i++ )
  {						
    if( pStudyStorDat->addr_data_433[i].Area == 0 )
    {
      continue;
    }
    if( (pStudyStorDat->addr_data_433[i].addr_433[0]!=0x00) && ( pStudyStorDat->addr_data_433[i].addr_433[1] !=0x00))    //地址已经学习
      
      for( j = 0; j < 4; j++ )
      {
        if(pStudyStorDat->addr_data_433[i].data_433[j][0] !=0x00)
        {
          
          temp[k*6+1]=pStudyStorDat->addr_data_433[i].Dev_Type+5;
          temp[k*6+2]=pStudyStorDat->addr_data_433[i].addr_433[0];
          temp[k*6+3]=pStudyStorDat->addr_data_433[i].addr_433[1];
          temp[k*6+4]=pStudyStorDat->addr_data_433[i].data_433[j][0];
          temp[k*6+5]=pStudyStorDat->addr_data_433[i].data_433[j][1];
          temp[k*6+6]=pStudyStorDat->addr_data_433[i].data_433[j][2];
          lens+=6;
          temp[0]+=1;
          k++;
        }
      }
  }
  wifi_sendcmd((BYTE *)temp,lens,check_app,GprsData[4]);
  handly_get_value(1,2); 
}

void save_sensor(u8 sensor,u8 *data,u8 info)
{
  
  u8 temp[30];
  u8 TempData[10];
  
  uint8			i, j;
  STOR_433_DATA	*pStudyStorDat;
  uint8			StudyCntAddr;
  u8				Already_Study = 0;
  
  pStudyStorDat = &StudyDat;
  memcpy(TempData,data,5);
  {
    if( Already_Study == 1 )
    {
      Already_Study = 0;
    }
    
    //两次学习结果相同，学习成功
    Load433StudyDat( pStudyStorDat );
    StudyCntAddr = pStudyStorDat->Count_433;
    switch( sensor )    //读取存储位置,还需要判断，该设备之前有没有学习过，若学习过，覆盖原来位置
    {
    case remote:
      for( i = 0; i < NODE_CNT; i++ )
      {						
	if( pStudyStorDat->addr_data_433[i].Area == 0 )
	{
          continue;
	}
	if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            for( j = 0; j < 4; j++ )
            {
              
              if( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][2] == TempData[4] ) )
              {
                UART4_printf( "语音提示:遥控器已经学习!" );
                Specify_musi_play( 8 );             //已经学习
                delay_ms(1000);
                i			   = NODE_CNT;          //跳出查找
                Already_Study  = 1;
                
                //return;
                break;
              }else if( ( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0xff ) ) )
              {
                UART4_printf( "语音提示:遥控器学习另外一个按键值!" );
                if(info==0)
                  Specify_musi_play( 0x06 );          //学习成功
                
                pStudyStorDat->addr_data_433[i].data_433[j][0] = TempData[2];
                pStudyStorDat->addr_data_433[i].data_433[j][1] = TempData[3];
                pStudyStorDat->addr_data_433[i].data_433[j][2] = TempData[4];
                Save433StudyDat( pStudyStorDat );   //存储遥控器的另外键值
                i			   = NODE_CNT;          //跳出查找
                
                Already_Study  = 1;
                
                
                
                TIME_Get( TIM2_tick );                  //获取系统时间
                
                
                memcpy(temp,TempData,5);
                
                temp[5]=calendar.hour;
                temp[6]=calendar.min;
                temp[7]=calendar.sec;
                temp[8]= AlarmFlags.bIsRmt_Deploy;
                
                wifi_sendcmd((BYTE *)temp,9,sensor+5,0); 
                handly_get_value(1,2);
                delay_ms(500);        
                //return;
                break;
              }
            }
          }else //已经学习到其它传感器区
          {
            if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
            {
              UART4_printf( "语音提示:已经学习到门磁!" );
              Specify_musi_play( 23 );        //已经学习到门磁
              delay_ms(1000);
              i			   = NODE_CNT;      //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
            {
              UART4_printf( "语音提示:已经学习到红外窗幕!" );
              Specify_musi_play( 24 );        //已经学习到红外窗幕
              delay_ms(1000);
              i			   = NODE_CNT;      //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
            {
              UART4_printf( "语音提示:已经学习到烟雾传感器!" );
              Specify_musi_play( 25 );                                                                                                        //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
            {
              UART4_printf( "语音提示:已经学习到燃气报警器!" );
              Specify_musi_play( 40 );
              delay_ms(1000);
              //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
            {
              UART4_printf( "语音提示:已经学习到紧急按钮!" );
              Specify_musi_play(39);     
              delay_ms(1000);                   //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              //return;
              break;
            }
          }
        }
      }
      break;
      
      //门磁判断
    case Menci:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "语音提示:已经学习到遥控器!" );
            Specify_musi_play( 26 );                                                                                                        //已经学习到遥控器
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "语音提示:已经学习!" );
              delay_ms(1000);
              Specify_musi_play( 8 );                                                                                                     //已经学习
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //学习成功
              
              TIME_Get( TIM2_tick );                  //获取系统时间
              
              
              memcpy(temp,TempData,5);
              
              temp[5]=calendar.hour;
              temp[6]=calendar.min;
              temp[7]=calendar.sec;
              temp[8]= AlarmFlags.bIsRmt_Deploy;
              
              wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
              handly_get_value(1,2);                                                                                                                            //return;
              break;
            }
          }else
          {
            if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
            {
              UART4_printf( "语音提示:已经学习到红外窗幕!" );
              Specify_musi_play( 24 );        //已经学习到红外窗幕
              i			   = NODE_CNT;      //跳出查找
              Already_Study  = 1;
              
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5) )
            {
              UART4_printf( "语音提示:已经学习到烟雾传感器!" );
              Specify_musi_play( 25 );  
              delay_ms(1000);
              //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
            {
              UART4_printf( "语音提示:已经学习到燃气报警器!" );
              Specify_musi_play( 40 );  
              delay_ms(1000);
              //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 6<= i ) && ( i < NODE_CNT ) )
            {
              UART4_printf( "语音提示:已经学习到紧急按钮!" );
              Specify_musi_play(39);        
              delay_ms(1000);
              //已经学习到红外窗幕
              i			   = NODE_CNT;                                                                                                      //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }
          }
        }
      }
      break;
      //红外窗幕
    case Infrared:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "语音提示:已经学习到遥控器!" );
            Specify_musi_play( 26 );                                                                                                        //已经学习到遥控器
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt *3 ) )
          {
            UART4_printf( "语音提示:已经学习到门磁!" );
            Specify_musi_play( 23 );    
            delay_ms(1000);
            //已经学习到门磁
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "语音提示:已经学习!" );
              Specify_musi_play( 8 );       
              delay_ms(1000);
              //已经学习
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //学习成功
              
              TIME_Get( TIM2_tick );                  //获取系统时间
              
              
              memcpy(temp,TempData,5);
              
              temp[5]=calendar.hour;
              temp[6]=calendar.min;
              temp[7]=calendar.sec;
              temp[8]= AlarmFlags.bIsRmt_Deploy;
              
              wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
              handly_get_value(1,2);	                                                                                                                            //return;
              break;
            }
          }
          else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt *5 ) )
          {
            UART4_printf( "语音提示:已经学习到烟雾传感器!" );
            Specify_musi_play( 25 );   
            delay_ms(1000);
            //已经学习到红外窗幕
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
          {
            UART4_printf( "语音提示:已经学习到燃气报警器!" );
            
            Specify_musi_play( 40 ); 
            delay_ms(1000);
            //已经学习到红外窗幕
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
          {
            UART4_printf( "语音提示:已经学习到紧急按钮!" );
            Specify_musi_play(39); 
            delay_ms(1000);
            //已经学习到红外窗幕
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
        }
      }
      break;
    case smoke:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "语音提示:已经学习到遥控器!" );
            
            Specify_musi_play( 26 ); 
            delay_ms(1000);
            //已经学习到遥控器
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "语音提示:已经学习到门磁!" );
            Specify_musi_play( 23 );      
            //已经学习到门磁
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "语音提示:已经学习到红外窗幕!" );
            
            Specify_musi_play( 24 );    
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "语音提示:已经学习!" );
              Specify_musi_play( 8 );                 
              //已经学习
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              if(info==0)
                Specify_musi_play( 0x06 );          //学习成功
              
              TIME_Get( TIM2_tick );                  //获取系统时间
              
              
              memcpy(temp,TempData,5);
              
              temp[5]=calendar.hour;
              temp[6]=calendar.min;
              temp[7]=calendar.sec;
              temp[8]= AlarmFlags.bIsRmt_Deploy;
              
              wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
              handly_get_value(1,2);
              //return;
              break;
            }
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
          {
            UART4_printf( "语音提示:已经学习到燃气报警器!" );
            Specify_musi_play( 40 );  
            delay_ms(2000);
            //已经学习到红外窗幕
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
          {
            UART4_printf( "语音提示:已经学习到紧急按钮!" );
            Specify_musi_play(39); 
            //已经学习到红外窗幕
            delay_ms(1500);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          
        }
      }
      
      break;
      //其它传感器
    case gas:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "语音提示:已经学习到遥控器!" );
            Specify_musi_play( 26 );                   
            //已经学习到遥控器
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "语音提示:已经学习到门磁!" );
            Specify_musi_play( 23 );                 
            //已经学习到门磁
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "语音提示:已经学习到红外窗幕!" );
            Specify_musi_play( 24 );    
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            UART4_printf( "语音提示:已经学习到烟雾传感器!" );
            Specify_musi_play( 25 ); 
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "语音提示:已经学习!" );
              
              Specify_musi_play( 8 );      
              //已经学习
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //学习成功            
              TIME_Get( TIM2_tick );                  //获取系统时间
              
              
              memcpy(temp,TempData,5);
              
              temp[5]=calendar.hour;
              temp[6]=calendar.min;
              temp[7]=calendar.sec;
              temp[8]= AlarmFlags.bIsRmt_Deploy;
              
              wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
              handly_get_value(1,2);  
              
              break;
            }
          }
          else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
          {
            UART4_printf( "语音提示:已经学习到紧急按钮!" );
            Specify_musi_play(39);            
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          
          
        }
      }
      
      break;
      //其它传感器
    case arlrm_calling:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
        {                                                                                                                                       //已学习过相同地址的传感器
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "语音提示:已经学习到遥控器!" );
            Specify_musi_play( 26 );    
            //已经学习到遥控器
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "语音提示:已经学习到门磁!" );
            Specify_musi_play( 23 );          
            //已经学习到门磁
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt *3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "语音提示:已经学习到红外窗幕!" );
            Specify_musi_play( 24 );        
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            UART4_printf( "语音提示:已经学习到烟雾传感器!" );
            Specify_musi_play( 25 );   
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
          {
            UART4_printf( "语音提示:已经学习到燃气报警器!" );
            Specify_musi_play( 40 );   
            //已经学习到红外窗幕
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //跳出查找
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt *6<= i ) && ( i < NODE_CNT ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "语音提示:已经学习!" );
              Specify_musi_play( 8 );      
              //已经学习
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //跳出查找
              Already_Study  = 1;
              if(info==0)
                Specify_musi_play( 0x06 );          //学习成功
              
              TIME_Get( TIM2_tick );                  //获取系统时间
              
              
              memcpy(temp,TempData,5);
              
              temp[5]=calendar.hour;
              temp[6]=calendar.min;
              temp[7]=calendar.sec;
              temp[8]= AlarmFlags.bIsRmt_Deploy;
              
              wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
              handly_get_value(1,2);
              //return;
              break;
            }
          }
          
          
          
        }
      }
      
      break;
    default: break;
    }
    
    if( StudyCntAddr == NODE_CNT )
    {
      UART4_printf( "语音提示:已经达到学习20个设备最大值，无法学习!" );
      Specify_musi_play( 10 ); //存储区满，请删除后再次学习
      delay_ms(1000);
      return;
    }
    switch( sensor )
    {
    case remote:
      for( i = 0; i < ( NODE_CNT / dev_typ_cnt ); i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {
          StudyCntAddr++;
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433			   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = remote;
          pStudyStorDat->addr_data_433[i].Area		   = i + 1; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "remote存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          
          Already_Study  = 1;
          
          //return;
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          break;
        }else
        {
          if( i == ( NODE_CNT / dev_typ_cnt - 1 ) )
          {
            UART4_printf( "remote已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    
            //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    case  Menci:
      for( i = ( NODE_CNT / dev_typ_cnt ); i < ( NODE_CNT / dev_typ_cnt * 3 ); i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {
          StudyCntAddr++;
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433					   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = Menci;
          pStudyStorDat->addr_data_433[i].Area		   = i; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "Menci存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          break;
        }else
        {
          if( i == ( NODE_CNT / dev_typ_cnt * 3 - 1 ) )
          {
            UART4_printf( "Menci已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 ); //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    case  Infrared:
      for( i = ( NODE_CNT / dev_typ_cnt * 3 ); i < ( NODE_CNT / dev_typ_cnt * 4 ); i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {
          StudyCntAddr++;	
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433					   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = Infrared;
          pStudyStorDat->addr_data_433[i].Area		   = i; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "Infrared存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          break;
        }else
        {
          if( i == ( NODE_CNT / dev_typ_cnt * 4 - 1 ) )
          {
            UART4_printf( "Infrared已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 ); //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    case smoke:
      for( i = ( NODE_CNT / dev_typ_cnt * 4 ); i < NODE_CNT / dev_typ_cnt * 5; i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {
          StudyCntAddr++;
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433					   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = smoke;
          pStudyStorDat->addr_data_433[i].Area		   = i; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "smoke存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          break;
        }else
        {
          if( i == ( NODE_CNT / dev_typ_cnt * 5 - 1 ) )
          {
            UART4_printf( "Other已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    case gas:
      for( i = ( NODE_CNT / dev_typ_cnt * 5 ); i < dev_typ_cnt * 6; i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {
          StudyCntAddr++;
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433					   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = gas;
          pStudyStorDat->addr_data_433[i].Area		   = i; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "gas存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          break;
        }else
        {
          if( i == (  NODE_CNT / dev_typ_cnt * 6 - 1 ) )
          {
            UART4_printf( "Other已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    case arlrm_calling:
      for( i = ( NODE_CNT / dev_typ_cnt * 6 ); i < NODE_CNT; i++ )
      {
        if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
        {      
          StudyCntAddr++;
          pStudyStorDat->addr_data_433[i].addr_433[0]	   = TempData[0];
          pStudyStorDat->addr_data_433[i].addr_433[1]	   = TempData[1];
          pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
          pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
          pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
          pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
          pStudyStorDat->ad_time_433[i].First_LTime	   = FIRST_LT;
          pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
          pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
          //pStudyStorDat->Count_433					   = i + 1;
          pStudyStorDat->Count_433                           =StudyCntAddr;
          pStudyStorDat->addr_data_433[i].Dev_Type	   = arlrm_calling;
          pStudyStorDat->addr_data_433[i].Area		   = i; //存储位号
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "arlrm_calling存储成功!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //学习成功
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //获取系统时间
          
          
          memcpy(temp,TempData,5);
          
          temp[5]=calendar.hour;
          temp[6]=calendar.min;
          temp[7]=calendar.sec;
          temp[8]= AlarmFlags.bIsRmt_Deploy;
          
          wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
          handly_get_value(1,2);
          
          break;
        }else
        {
          if( i == ( NODE_CNT - 1 ) )
          {
            UART4_printf( "Other已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //存储区满，请删除后再次学习
            delay_ms(1000);
          }
        }
      }
      break;
    default: break;
    }
  }
}

void Study( u8 sensor,u8 typeinfo)
{
  u8 temp[30];
  //char szTmp[100];
  uint8			DataTemp[2 * N]; //用于学习两次jiama()值，对比，一致才学习成功。
  uint8			i, j;
  u32				study_time	   = 0;
  u32				key_delay_time = 0;
  STOR_433_DATA	*pStudyStorDat;
  uint8			StudyCntAddr;
  u8				Already_Study = 0;
  
  pStudyStorDat = &StudyDat;
  
  study_time	   = TickGet( );
  key_delay_time = TickGet( );
  
  //UART4_printf("kdt1=%d!\r\n",TickGet());
  while( ( TickGet( ) > ( study_time + 4000 ) ) ? 0 : 1 ) //8秒超时退出
  {
    IWDG_Feed();
    if( ( TickGet( ) - key_delay_time ) > 500 )
    {
      key_delay_time = TickGet( );
      if( keyflag == K_SET )                          //按键用于退出当前学习，进入下一级
      {
        return;
      }
    }
    
    Jiema( study );
    for( i = 0; i < N; i++ )
    {
      DataTemp[i] = Receive[i];
    }
    
    delay_ms( 20 );
    Jiema( study );
    for( i = 0; i < N; i++ )
    {
      DataTemp[i + N] = Receive[i];
    }
    
    if( Already_Study == 1 )
    {
      Already_Study = 0;
    }
    if( ( DataTemp[0] != 0x00 ) && ( DataTemp[1] != 0x00 ) && ( DataTemp[2] != 0x00 ) && ( DataTemp[N] != 0x00 ) && ( DataTemp[N + 1] != 0x00 ) && ( DataTemp[N + 2] != 0x00 ) )
    {
      
      if( ( DataTemp[0] == DataTemp[N] ) && ( DataTemp[1] == DataTemp[N + 1] ) && ( DataTemp[2] == DataTemp[N + 2] ) && ( DataTemp[3] == DataTemp[N + 3] ) && ( DataTemp[4] == DataTemp[N + 4] ) )
      {                       //两次学习结果相同，学习成功
        UART4_printf( "R=0x%x %x %x %x %x\r\n", Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
        Load433StudyDat( pStudyStorDat );
        StudyCntAddr = pStudyStorDat->Count_433;
        switch( sensor )    //读取存储位置,还需要判断，该设备之前有没有学习过，若学习过，覆盖原来位置
        {
        case remote:
          for( i = 0; i < NODE_CNT; i++ )
          {
            //copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                for( j = 0; j < 4; j++ )
                {
                  copyj = j;
                  if( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][2] == Receive[4] ) )
                  {
                    UART4_printf( "语音提示:遥控器已经学习!" );
                    Specify_musi_play( 8 );             //已经学习
                    delay_ms(1000);
                    i			   = NODE_CNT;          //跳出查找
                    Already_Study  = 1;
                    //	study_time	   = TickGet( );        //时间重新更新
                    break;
                  }else if( ( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0xff ) ) )
                  {
                    UART4_printf( "语音提示:遥控器学习另外一个按键值!" );
                    Specify_musi_play( 0x06 );          //学习成功
                    pStudyStorDat->addr_data_433[i].data_433[j][0] = Receive[2];
                    pStudyStorDat->addr_data_433[i].data_433[j][1] = Receive[3];
                    pStudyStorDat->addr_data_433[i].data_433[j][2] = Receive[4];
                    
                    Save433StudyDat( pStudyStorDat );   //存储遥控器的另外键值
                    i			   = NODE_CNT;          //跳出查找
                    study_time	   = TickGet( );        //时间重新更新
                    
                    
                    Already_Study  = 1;
                    
                    TIME_Get( TIM2_tick );                  //获取系统时间
                    
                    
                    memcpy(temp,Receive,5);
                    
                    temp[5]=calendar.hour;
                    temp[6]=calendar.min;
                    temp[7]=calendar.sec;
                    temp[8]= AlarmFlags.bIsRmt_Deploy;
                    wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo); 
                    handly_get_value(1,2);
                    delay_ms(500);                                          //return;
                    break;
                  }
                }
              }else //已经学习到其它传感器区
              {
                if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
                {
                  UART4_printf( "语音提示:已经学习到门磁!" );
                  Specify_musi_play( 23 );        //已经学习到门磁
                  delay_ms(2000);
                  i			   = NODE_CNT;      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //时间重新更
                  break;
                }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
                {
                  UART4_printf( "语音提示:已经学习到红外窗幕!" );
                  Specify_musi_play( 24 );        //已经学习到红外窗幕
                  delay_ms(2000);
                  i			   = NODE_CNT;      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
                {
                  UART4_printf( "语音提示:已经学习到烟雾传感器!" );
                  Specify_musi_play( 25 ); 
                  delay_ms(2000);
                  //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
                {
                  UART4_printf( "语音提示:已经学习到燃气报警器!" );
                  Specify_musi_play( 40 );
                  delay_ms(2000);
                  //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
                {
                  UART4_printf( "语音提示:已经学习到紧急按钮!" );
                  Specify_musi_play(39);     
                  delay_ms(2000);                //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
              }
            }
          }
          break;
          
          //门磁判断
        case Menci:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "语音提示:已经学习到遥控器!" );
                Specify_musi_play( 26 );                                                                                                        //已经学习到遥控器
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "语音提示:已经学习!" );
                  delay_ms(1000);
                  Specify_musi_play( 8 );                                                                                                     //已经学习
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  
                  
                  TIME_Get( TIM2_tick );                  //获取系统时间
                  
                  
                  memcpy(temp,Receive,5);
                  
                  temp[5]=calendar.hour;
                  temp[6]=calendar.min;
                  temp[7]=calendar.sec;
                  temp[8]= AlarmFlags.bIsRmt_Deploy;
                  
                  wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo);
                  handly_get_value(1,2);                                                                                                                            //return;
                  break;
                }
              }else
              {
                if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
                {
                  UART4_printf( "语音提示:已经学习到红外窗幕!" );
                  Specify_musi_play( 24 );        //已经学习到红外窗幕
                  delay_ms(2000);
                  i			   = NODE_CNT;      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5) )
                {
                  UART4_printf( "语音提示:已经学习到烟雾传感器!" );
                  Specify_musi_play( 25 );  
                  delay_ms(1000);
                  //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
                {
                  UART4_printf( "语音提示:已经学习到燃气报警器!" );
                  Specify_musi_play( 40 );  
                  delay_ms(2000);
                  //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
                {
                  UART4_printf( "语音提示:已经学习到紧急按钮!" );
                  Specify_musi_play(39);        
                  delay_ms(2000);
                  //已经学习到红外窗幕
                  i			   = NODE_CNT;                                                                                                      //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                  
                  //return;
                  break;
                }
              }
            }
          }
          break;
          //红外窗幕
        case Infrared:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "语音提示:已经学习到遥控器!" );
                Specify_musi_play( 26 );                                                                                                        //已经学习到遥控器
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "语音提示:已经学习到门磁!" );
                Specify_musi_play( 23 );    
                delay_ms(2000);
                //已经学习到门磁
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //		study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "语音提示:已经学习!" );
                  Specify_musi_play( 8 );       
                  delay_ms(1000);
                  //已经学习
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  
                  
                  TIME_Get( TIM2_tick );                  //获取系统时间
                  
                  
                  memcpy(temp,Receive,5);
                  
                  temp[5]=calendar.hour;
                  temp[6]=calendar.min;
                  temp[7]=calendar.sec;
                  temp[8]= AlarmFlags.bIsRmt_Deploy;
                  
                  wifi_sendcmd((BYTE *)temp,9,sensor+5,0);
                  handly_get_value(1,2);	                                                                                                                            //return;
                  break;
                }
              }
              else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
              {
                UART4_printf( "语音提示:已经学习到烟雾传感器!" );
                Specify_musi_play( 25 );   
                delay_ms(2000);
                //已经学习到红外窗幕
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
              {
                UART4_printf( "语音提示:已经学习到燃气报警器!" );
                
                Specify_musi_play( 40 ); 
                delay_ms(2000);
                //已经学习到红外窗幕
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
              {
                UART4_printf( "语音提示:已经学习到紧急按钮!" );
                Specify_musi_play(39); 
                delay_ms(2000);
                //已经学习到红外窗幕
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
            }
          }
          break;
          //其它传感器
        case smoke:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "语音提示:已经学习到遥控器!" );
                
                Specify_musi_play( 26 ); 
                delay_ms(2000);
                //已经学习到遥控器
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "语音提示:已经学习到门磁!" );
                Specify_musi_play( 23 );      
                //已经学习到门磁
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "语音提示:已经学习到红外窗幕!" );
                
                Specify_musi_play( 24 );    
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "语音提示:已经学习!" );
                  Specify_musi_play( 8 );                 
                  //已经学习
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  
                  TIME_Get( TIM2_tick );                  //获取系统时间
                  
                  
                  memcpy(temp,Receive,5);
                  
                  temp[5]=calendar.hour;
                  temp[6]=calendar.min;
                  temp[7]=calendar.sec;
                  temp[8]= AlarmFlags.bIsRmt_Deploy;
                  
                  wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo);
                  handly_get_value(1,2);
                  
                  break;
                }
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
              {
                UART4_printf( "语音提示:已经学习到燃气报警器!" );
                Specify_musi_play( 40 );  
                delay_ms(2000);
                //已经学习到红外窗幕
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
              {
                UART4_printf( "语音提示:已经学习到紧急按钮!" );
                Specify_musi_play(39); 
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              
            }
          }
          
          break;
          //其它传感器
        case gas:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "语音提示:已经学习到遥控器!" );
                Specify_musi_play( 26 );                   
                //已经学习到遥控器
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "语音提示:已经学习到门磁!" );
                Specify_musi_play( 23 );                 
                //已经学习到门磁
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "语音提示:已经学习到红外窗幕!" );
                Specify_musi_play( 24 );    
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt *5 ) )
              {
                UART4_printf( "语音提示:已经学习到烟雾传感器!" );
                Specify_musi_play( 25 ); 
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5<= i ) && ( i < NODE_CNT / dev_typ_cnt * 6) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "语音提示:已经学习!" );
                  
                  Specify_musi_play( 8 );      
                  //已经学习
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  //return;
                  return ;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  
                  TIME_Get( TIM2_tick );                  //获取系统时间
                  
                  
                  memcpy(temp,Receive,5);
                  
                  temp[5]=calendar.hour;
                  temp[6]=calendar.min;
                  temp[7]=calendar.sec;
                  temp[8]= AlarmFlags.bIsRmt_Deploy;
                  
                  wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo);
                  handly_get_value(1,2);  
                  
                  break;
                }
              }
              else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
              {
                UART4_printf( "语音提示:已经学习到紧急按钮!" );
                Specify_musi_play(39);            
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                return ;
              }
              
              
            }
          }
          
          break;
          //其它传感器
        case arlrm_calling:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
            {                                                                                                                                       //已学习过相同地址的传感器
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "语音提示:已经学习到遥控器!" );
                Specify_musi_play( 26 );    
                //已经学习到遥控器
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt *3 ) )
              {
                UART4_printf( "语音提示:已经学习到门磁!" );
                Specify_musi_play( 23 );          
                //已经学习到门磁
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt *3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "语音提示:已经学习到红外窗幕!" );
                Specify_musi_play( 24 );        
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
              {
                UART4_printf( "语音提示:已经学习到烟雾传感器!" );
                Specify_musi_play( 25 );   
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
              {
                UART4_printf( "语音提示:已经学习到燃气报警器!" );
                Specify_musi_play( 40 );   
                //已经学习到红外窗幕
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //跳出查找
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //时间重新更新
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt *6 <= i ) && ( i < NODE_CNT ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "语音提示:已经学习!" );
                  Specify_musi_play( 8 );      
                  //已经学习
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  //return;
                  break;
                }
                else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //存储位号
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //跳出查找
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //时间重新更新
                  
                  
                  TIME_Get( TIM2_tick );                  //获取系统时间
                  
                  
                  memcpy(temp,Receive,5);
                  
                  temp[5]=calendar.hour;
                  temp[6]=calendar.min;
                  temp[7]=calendar.sec;
                  temp[8]= AlarmFlags.bIsRmt_Deploy;
                  
                  wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo);
                  handly_get_value(1,2);
                  //return;
                  break;
                }
              }
              
              
              
            }
          }
          
          break;
        default: break;
        }
        if( Already_Study == 1 )
        {
          if(typeinfo)
          {
            get_back_study();
            return ;
          }           
          continue;
        }
        //新设备学习
        //新设备学习
        if( StudyCntAddr == NODE_CNT )
        {
          UART4_printf( "语音提示:已经达到学习20个设备最大值，无法学习!" );
          Specify_musi_play( 10 ); //存储区满，请删除后再次学习
          delay_ms(1000);
          return;
        }
        switch( sensor )
        {
        case remote:
          if(check_sensor_remote(Receive))
          {
            Save_433_flash(0,1,StudyCntAddr,sensor,typeinfo);
            study_time	           = TickGet( );                        //时间重新更新
            Already_Study             = 1;
            if(typeinfo)
            {
              get_back_study();
              return ;
            }          
          }
          break;
        case  Menci:
          Save_433_flash(1,3,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //时间重新更新
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case  Infrared:
          Save_433_flash(3,4,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //时间重新更新
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case smoke:
          Save_433_flash(4,5,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //时间重新更新
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case gas:
          Save_433_flash(5,6,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //时间重新更新
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case arlrm_calling:
          Save_433_flash(6,7,StudyCntAddr,sensor,typeinfo);
          study_time	   = TickGet( );                        //时间重新更新
          Already_Study  = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        default:
          break;
        }
      }
    }
  }
  UART4_printf( "自学习超时，退出!" );
  
  Specify_musi_play( 0x09 ); //退出学习
  
  keyflag = K_CLR;
  
  Kmenu.Layer =1;
}


void get_back_study()
{
  Specify_musi_play( 0x09 ); //退出学习
  keyflag = K_CLR;
  Kmenu.Layer =1;
}
void Save_433_flash(u8 memu1,u8 memu2,u8 StudyCntAddr,u8 sensor,u8 typeinfo)
{
  u8 i;
  u8 temp[30];
  STOR_433_DATA	*pStudyStorDat;
  pStudyStorDat = &StudyDat;
  
  for( i = ( NODE_CNT / dev_typ_cnt * memu1 ); i < NODE_CNT/dev_typ_cnt * memu2; i++ )
  {	
    if( ( pStudyStorDat->addr_data_433[i].Area == 0 ) || ( pStudyStorDat->addr_data_433[i].Area == 0xff ) )
    {
      StudyCntAddr++;	
      pStudyStorDat->addr_data_433[i].addr_433[0]	   = Receive[0];
      pStudyStorDat->addr_data_433[i].addr_433[1]	   = Receive[1];
      pStudyStorDat->addr_data_433[i].data_433[0][0]       = Receive[2];
      pStudyStorDat->addr_data_433[i].data_433[0][1]       = Receive[3];
      pStudyStorDat->addr_data_433[i].data_433[0][2]       = Receive[4];
      pStudyStorDat->ad_time_433[i].Syno_Time		   = SYNO_T;
      pStudyStorDat->ad_time_433[i].First_LTime	         = FIRST_LT;
      pStudyStorDat->ad_time_433[i].Low_Time		   = LOW_T;
      pStudyStorDat->ad_time_433[i].High_Time		   = HIGH_T;
      //pStudyStorDat->Count_433					   = i + 1;
      pStudyStorDat->Count_433                           =StudyCntAddr;
      pStudyStorDat->addr_data_433[i].Dev_Type	   = sensor;
      pStudyStorDat->addr_data_433[i].Area		   = i+1; //存储位号
      Save433StudyDat( pStudyStorDat );
      UART4_printf( "存储成功!" );
      Specify_musi_play( 0x06 );                          //学习成功
      delay_ms(800);
      
      TIME_Get( TIM2_tick );                  //获取系统时间
      memcpy(temp,Receive,5);   
      temp[5]=calendar.hour;
      temp[6]=calendar.min;
      temp[7]=calendar.sec;
      temp[8]= AlarmFlags.bIsRmt_Deploy;
      
      wifi_sendcmd((BYTE *)temp,9,sensor+5,typeinfo);
      handly_get_value(1,2);
      break;
      
    }else
    {
      if( i == ( NODE_CNT / dev_typ_cnt *(memu2) - 1 ) )
      {
        UART4_printf( "Other已经超过%d个，请铲除后再学习!", ( NODE_CNT / dev_typ_cnt ) );
        Specify_musi_play( 10 );    //存储区满，请删除后再次
        delay_ms(1500);
      }
    }
  }
  
}

int check_sensor_remote(u8 *data)
{
  u8 sensor_len=0;
  u8 sensor_key;
  u8 sensor_key1;
  
  sensor_len=data[4];
  
  sensor_key=(data[2]&0x0f);
  sensor_key1=(data[3]&0xf0);
  
  if(sensor_len==0)
  {
    
    switch(sensor_key)
    {
    case 0x07:
      return 4;
    case 0x0b:
      return 1;
    case 0x0d:
      return 2;
    case 0x0e:
      return 3;
    default:
      return 0;
    }
  }
  else if(((sensor_len<<2)&0x3f)==0)
  {
    switch(sensor_key)
    {
    case 0x00:
      return 2;
    case 0x01:
      return 1;
    case 0x02:
      return 3;
    case 0x04:
      return 4;
    default:
      return 0;
    }
  }
  else
  {
    switch(sensor_key1)
    {
    case 0x00:
      return 2;
    case 0x10:
      return 1;
    case 0x20:
      return 3;
    case 0x40:
      return 4;
    default:
      return 0;
    }
  }
  return 0;
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Delete_OneSensor( void )
{
  u8 temp[20];
  u32				del_time;
  uint8			StudyCntAddr;
  STOR_433_DATA	* pStudyStorDat;
  u8				i;
  //需要开辟一个数组来存储当前检测到的各种传感器值吗?
  del_time = TickGet( );
  while( ( TickGet( ) > ( del_time + 3000 ) ) ? 0 : 1 )
  {
    pStudyStorDat  = &StudyDat;
    StudyCntAddr   = pStudyStorDat->Count_433;
    if(keyflag==K_ENC)
    {
      return;
    }
    Jiema( changgui );
    for( i = 0; i < NODE_CNT; i++ )                                                                                                             //判断
    {
      if( pStudyStorDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //地址已经学习
      {                                                                                                                                       //已学习过相同地址的传感器
        UART4_printf( "R[%d]=0x%x %x %x %x %x\r\n", i, Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
        memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );                                           //清除该区域传感器信息
        memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
        StudyCntAddr--;
        if( StudyCntAddr > NODE_CNT )
        {
          StudyCntAddr = 0;
        }
        pStudyStorDat->Count_433 = StudyCntAddr;
        Save433StudyDat( pStudyStorDat );
        
        TIME_Get( TIM2_tick );                  //获取系统时间
        memcpy(temp,Receive,5);
        temp[5]=calendar.hour;
        temp[6]=calendar.min;
        temp[7]=calendar.sec;
        temp[8]= AlarmFlags.bIsRmt_Deploy;
        voice_delete_app(temp,i); //删除语音播报，信息上报
        break;  //退出查找
      }
      // printf("尚未学习\r\n");
    }//printf("尚未学习\r\n");
  }
}



void Delete_app(u8 *data)
{
  u8 temp[20];
  u8 TempData[10];
  u32		del_time;
  uint8		StudyCntAddr;
  STOR_433_DATA	* pStudyStorDat;
  u8				i;
  pStudyStorDat  = &StudyDat;
  StudyCntAddr   = pStudyStorDat->Count_433;
  
  memcpy(TempData,data,5); //默认5个字节  
  
  TIME_Get( TIM2_tick );
  memcpy(temp,data,5);
  temp[5]=calendar.hour;
  temp[6]=calendar.min;
  temp[7]=calendar.sec;
  temp[8]= AlarmFlags.bIsRmt_Deploy; 
  
  for( i = 0; i < NODE_CNT; i++ )                                                                                                             //判断
  {
    if( pStudyStorDat->addr_data_433[i].Area == 0 )
    {
      continue;
    }
    
    if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //地址已经学习
    {                                                                                                                                       //已学习过相同地址的传感器
      //UART4_printf( "R[%d]=0x%x %x %x %x %x\r\n", i, temp[0], temp[1], temp[2], temp[3], temp[4] );
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
      if( StudyCntAddr > NODE_CNT )
      {
	StudyCntAddr = 0;
      }
      pStudyStorDat->Count_433 = StudyCntAddr;
      Save433StudyDat( pStudyStorDat );
      voice_delete_app(temp,i);  
      return;
    }
  }
  Specify_musi_play(27);    //已经删除
  wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
  handly_get_value(1,2);
}

void voice_delete_app(u8 *temp,u8 i)
{
  if( i < 5 )
  {//语音 删除单个遥控器
    Specify_musi_play( 11 );    //删除遥控器
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i < 15 )
  {					//语音 删除单个门磁
    Specify_musi_play( 12 );    //删除门磁窗磁
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i < 20 )
  {//语音 删除单个红外
    Specify_musi_play( 13 );    //删除红外窗幕
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i <25)
  {//语音 删除单个其它传感器
    Specify_musi_play( 14 );    //删除烟雾传感器
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
  }
  else if( i <30)
  {//语音 删除单个其它传感器
    Specify_musi_play( 36 );    //删除燃气传感器
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i <35)
  {//语音 删除单个其它传感器
    Specify_musi_play( 32 );    //删除紧急按钮
    delay_ms(1500);
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
}
/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Delete_Sensor( u8 sensor )
{
  STOR_433_DATA	*pStudyStorDat;
  
  uint8			StudyCntAddr;
  u8 temp[20];
  u8		             i;
  pStudyStorDat = &StudyDat;
  StudyCntAddr = pStudyStorDat->Count_433;
  TIME_Get( TIM2_tick );                  //获取系统时间                
  temp[0]=calendar.hour;
  temp[1]=calendar.min;
  temp[2]=calendar.sec;
  temp[3]= AlarmFlags.bIsRmt_Deploy;
  switch( sensor )
  {
  case remote:
    for( i = 0; i < ( NODE_CNT /dev_typ_cnt ); i++ )                    
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功 
    wifi_sendcmd((BYTE *)temp,4,Delete_remote_all,0);                                                             //   Specify_Musi_Play(16);//删除成功
    break;
  case Menci:
    for( i = ( NODE_CNT /dev_typ_cnt ); i < ( NODE_CNT /dev_typ_cnt * 3 ); i++ )      //
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功     
    wifi_sendcmd((BYTE *)temp,4,Delete_meci_all,0);                                                            //  Specify_Musi_Play(16);//删除成功
    break;
  case Infrared:
    for( i = ( NODE_CNT /dev_typ_cnt * 3 ); i < ( NODE_CNT /dev_typ_cnt * 4 ); i++ )  //
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功
    wifi_sendcmd((BYTE *)temp,4,Delete_infrared_all,0);                                                          //     Specify_Musi_Play(16);//删除成功
    break;
  case smoke:
    for( i = ( NODE_CNT /dev_typ_cnt * 4 ); i < ( NODE_CNT/dev_typ_cnt*5); i++ )          //
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功     
    wifi_sendcmd((BYTE *)temp,4,Delete_smoke_all,0);                                                            //    Specify_Musi_Play(16);//删除成功
    break;
  case gas:
    for( i = ( NODE_CNT / dev_typ_cnt * 5 ); i < ( NODE_CNT/dev_typ_cnt*6 ); i++ )          //
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功   
    wifi_sendcmd((BYTE *)temp,4,Delete_gas_all,0);                                                            //    Specify_Musi_Play(16);//删除成功
    break;
  case arlrm_calling:
    for( i = ( NODE_CNT /dev_typ_cnt *6); i < ( NODE_CNT ); i++ )          //
    {//清除该区域所有传感器信息
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //删除成功 
    wifi_sendcmd((BYTE *)temp,4,Delete_arlrm_all,0);
    break;
  default: break;
  }
  if( StudyCntAddr > NODE_CNT )
  {
    StudyCntAddr = 0;
  }
  pStudyStorDat->Count_433 = StudyCntAddr;
  Save433StudyDat( pStudyStorDat );
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void Delete_AllSensor( void )
{
  STOR_433_DATA	*pStudyStorDat;
  uint8			StudyCntAddr;
  u8				i;
  
  pStudyStorDat = &StudyDat;
  
  Load433StudyDat( pStudyStorDat );
  
  for( i = 0; i < NODE_CNT; i++ ) //
  {
    //清除该区域所有传感器信息
    memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
    memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
  }
  
  StudyCntAddr			   = 0;
  pStudyStorDat->Count_433   = StudyCntAddr;
  Save433StudyDat( pStudyStorDat );
  UART4_printf( "删除所有已经学习传感器\r\n" );
  //Specify_Musi_Play( 15 );    //删除所有传感器
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Send_ALM_infrared(u8 *datatemp,u8 cmd)
{
  
  static u32 send_time;
  u8 temp[20];
  TIME_Get( TIM2_tick );
  memcpy(temp,datatemp,5);
  temp[5]=calendar.hour;
  temp[6]=calendar.min;
  temp[7]=calendar.sec;
  temp[8]   = AlarmFlags.bIsRmt_Deploy;
  
  if(AlarmFlags.bIsRmt_Deploy ==0)     //红外报警信息上传，情况1 5分钟一次 撤防状态下  
  {
    if((TickGet()-send_time)>1000*5*60){
      send_time= TickGet();
      wifi_sendcmd(temp,9,cmd,0); 
      handly_get_value(1,5); 
    } 
  }
  else if((TickGet()-send_time)>1000*2*60)        ////红外报警信息上传，情况2 2分钟一次 布防状态下，或休息模式下  
  {
    send_time= TickGet();
    wifi_sendcmd(temp,9,cmd,0); 
    handly_get_value(1,5); 
  }
}


/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
u32		remote_time[9] = { 0 };

void hand_send_alrm(u8* datatemp,u8 cmd)
{
  u8 temp[20];
  TIME_Get( TIM2_tick ); 
  memcpy(temp,datatemp,5);      
  temp[5]=calendar.hour;
  temp[6]=calendar.min;
  temp[7]=calendar.sec;
  temp[8] = AlarmFlags.bIsRmt_Deploy;
  wifi_sendcmd((BYTE *)temp,9,cmd,0);                                                   
  handly_get_value(1,5); 
  delay_ms(500);	
  
}

void study_romte_PA()
{
  
}

void chuangui_remote(u8 j,u8* datatemp)
{
  switch( j )
  {
  case 1:                             //布防按键
    if( TickGet()-remote_time[0]>300)
    {
      remote_time[0] = TickGet( );
      Specify_musi_play( 18 );    //布防
      AlarmFlags.bIsRmt_Deploy = 1;
      
      hand_send_alrm(datatemp,Bufang_remote);
      return ;
    }
    break;
  case 2:                         //撤防按键
    if( TickGet( ) - remote_time[1] > 300 )
    {
      remote_time[1] = TickGet( );
      Specify_musi_play( 19 );	
      AlarmFlags.bIsRmt_Deploy   = 0;
      hand_send_alrm(datatemp,Chefang_remote);
      return ;
    }
    break;
  case 3:                              //紧急键
    if( TickGet( ) - remote_time[2] > 300 )
    {
      remote_time[2] = TickGet();                 //获取系统时间
      AlarmFlags.bIsRmt_Deploy =2; 
      Specify_musi_play(53);  //守护模式
      hand_send_alrm(datatemp,Doorbell_remote);
      delay_ms(200); //确保语音播报完整
      return ;
    }
    break;
  case 4:                              //紧急键
    if( TickGet( ) - remote_time[2] > 300 )
    {
      remote_time[2] = TickGet();            
      Specify_Musi_Play(38);  //紧急报警
      hand_send_alrm(datatemp,ALM_remote);
      return ;
    }
    break;
  default:
    break;
  } //end of  switch
  
  
  //return ;
  
}

void hand_pa_send(u8 i)
{
  u8 j=0;
  if(PA_DATA.en_disable==1)
  {
    
    if(i!=0){
      for(j=0;j<2;j++){
        send_433data(&PA_DATA.pa_data[i-1][0],0);
      }
    }
    return ;
  }
  
}

void hand_menci_infrared_voice(u8 voice_type,u8 sensor)
{
  SEND_433_DATA	* pSendDat;
  u8 k;
  pSendDat = &SendDat;
  
  if(AlarmFlags.bIsRmt_Deploy==1)
  {
    Specify_Musi_Play(voice_type); // 报警音
  }
  else if(AlarmFlags.bIsRmt_Deploy==2)
  {
    
    if((SendDat.Count_433!=0)&&(SendDat.Count_433!=16))
    {
      for(k=0;k<SendDat.Count_433;k++)
      {
        if((SendDat.send_data[k][1]==Receive[0] ) && ( SendDat.send_data[k][2] == Receive[1])
           &&(SendDat.send_data[k][3]==Receive[2] ) && (SendDat.send_data[k][4] == Receive[3])) //是否是关键红外                                           
          Specify_Musi_Play(voice_type); // 报警音
        break;
      }
    }
    else if(SendDat.Count_433==0&&(sensor==ALM_meci))
    {
      Specify_Musi_Play(voice_type); // 报警音
    }
  }
}

u32 time_guanlin=0;
void check_guanling()
{
  u8 i=0;
  
  if(TickGet()-time_guanlin>(60000)||time_guanlin==0)
  {
    
    for(i=0;i<5;i++)
    {             
      
      
      if((C_Relation.rec_data[i][0]==Receive[0])
         &&(C_Relation.rec_data[i][1]==Receive[1])
           &&(C_Relation.rec_data[i][2]==Receive[2])
             &&(C_Relation.rec_data[i][3]==Receive[3])
               &&(C_Relation.rec_data[i][4]==Receive[4]))
      {
        
        time_guanlin= TickGet();
        Specify_musi_play(55);
        if(C_Relation.area[i]!=4)
        {
          AlarmFlags.bIsRmt_Deploy=C_Relation.area[i]-1;
        }
        
        delay_ms(1500);
        
        
        zhuhe_send(&C_Relation.Send_433[i]);
        
        wifi_sendcmd((BYTE *)&Receive[0],5,get_back_gl,0);                                                   
        handly_get_value(1,5); 
        
      }
    }  
  }
}

u8 cout,mencout;                        //用于紧急按键按下\门铃键 检测到的次数
int Changgui( void )
{
  
  u8 key_value=0;
  u8			temp[20];
  static u8		datatemp[N * 2];
  STOR_433_DATA	* pStudyDat;
  SEND_433_DATA	* pSendDat;
  u8				i, j,k;
  IWDG_Feed();
  pStudyDat = &StudyDat;   //需要开辟一个数组来存储当前检测到的各种传感器值吗?
  pSendDat = &SendDat;
  Jiema(changgui);
  for( i = 0; i < N; i++ )
  {
    datatemp[i] = Receive[i];
  }
  if( ( datatemp[0] != 0x00 ) && ( datatemp[1] != 0x00 )  && ( datatemp[2] != 0x00 ))
  {                                                                                                                                                                                                                       //两次学习结果相同，学习成功
    
    UART4_printf( "R=0x%x %x %x %x %x\r\n", Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
    
    check_guanling();
    for( i = 0; i < ( NODE_CNT / dev_typ_cnt ); i++ )                                                                                                                                                                             //判断遥控器
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;//该位置没有存储传感器
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //地址已经学习
      {                                                                                                                                                                                                               //已学习过相同地址的传感器
	for( j = 0; j < 4; j++ )
	{
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ))                                 //存储有按键值
          {
            if( ( pStudyDat->addr_data_433[i].data_433[j][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[j][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[j][2] == Receive[4] ) )  //已经存储该按键信息
            {
              break; //跳出键值查找
            }
          }
          else //存储该按键
          {
            pStudyDat->addr_data_433[i].data_433[j][0] = Receive[2];
            pStudyDat->addr_data_433[i].data_433[j][1] = Receive[3];
            pStudyDat->addr_data_433[i].data_433[j][2] = Receive[4];
            Save433StudyDat(pStudyDat);
            break;
          }
	}
        IWDG_Feed();
        key_value=check_sensor_remote(datatemp);
        chuangui_remote(key_value,&datatemp[0]);
        hand_pa_send(key_value);
        break;
        //  return 0;
      } // end   if  学习过
    }
    
    for( i = ( NODE_CNT / dev_typ_cnt ); i < ( NODE_CNT /dev_typ_cnt * 3); i++ )                                                                                                                                                      //判断门磁窗磁
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      
      if(  (pStudyDat->addr_data_433[i].addr_433[0] == Receive[0])  && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                //地址已经学习
      {                                                                                                                                                //已学习过相同地址的传感器
        if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] )
           && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] )
             && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //已经存储该按键信息
        {
          if( TickGet( ) - remote_time[8] > 2000 )
          {
            remote_time[8] = TickGet( );
            
            hand_menci_infrared_voice(21,ALM_meci);
            hand_send_alrm(&datatemp[0],ALM_meci);
            //   return 0;
          }
        }
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 3 ); i < ( NODE_CNT /dev_typ_cnt * 4); i++ ) //判断红外幕帘
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                            //地址已经学习
      {                                                                                                                                                                                                       //已学习过相同地址的传感器
        if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //已经存储该按键信息
        {
          if( TickGet( ) - remote_time[4] >2000)
          {
            remote_time[4] = TickGet( );
            
            hand_menci_infrared_voice(20,ALM_infrared);
            Send_ALM_infrared(&datatemp[0],ALM_infrared);
            
            delay_ms(500);
            //  Send_ALM_infrared((BYTE *)temp);
            //  return 0;
          }
        }
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 4); i < (NODE_CNT /dev_typ_cnt * 5); i++ ) //判断其它传感器
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
	continue;
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //地址已经学习
      {                                                                                                                                                                                                               //已学习过相同地址的传感器
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //存储有按键值
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //已经存储该按键信息
            {
              break;
            }
          }
          else
          {
            pStudyDat->addr_data_433[i].data_433[j][0] = Receive[2];
            pStudyDat->addr_data_433[i].data_433[j][1] = Receive[3];
            pStudyDat->addr_data_433[i].data_433[j][2] = Receive[4];
            Save433StudyDat( pStudyDat );
            break;
          }
        }
        if( TickGet( ) - remote_time[5] > 2000 )
	{
          remote_time[5] = TickGet( );
          Specify_Musi_Play(22);//烟雾传感器
          hand_send_alrm(&datatemp[0],ALM_smoke);
          //return 0;					
	}
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 5 ); i < (NODE_CNT/dev_typ_cnt*6); i++ ) //判断其它传感器
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      // break;
      
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //地址已经学习
      {                                                                                                                                                                                                               //已学习过相同地址的传感器
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //存储有按键值
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //已经存储该按键信息
            {
              
              break;
              
            }
          }else
          {
            pStudyDat->addr_data_433[i].data_433[j][0] = Receive[2];
            pStudyDat->addr_data_433[i].data_433[j][1] = Receive[3];
            pStudyDat->addr_data_433[i].data_433[j][2] = Receive[4];
            Save433StudyDat( pStudyDat );
            break;
          }
        }
        if( TickGet( ) - remote_time[6] > 2000 )
        {
          remote_time[6] = TickGet( );
          Specify_Musi_Play(30);//燃气传感器
          hand_send_alrm(&datatemp[0],ALM_gas);
          //    return 0;
          
        }
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 6); i < (NODE_CNT); i++ ) //判断其它传感器
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      // break;
      
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //地址已经学习
      {                                                                                                                                                                                                               //已学习过相同地址的传感器
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //存储有按键值
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //已经存储该按键信息
            {
              
              break;
            }
          }else
          {
            pStudyDat->addr_data_433[i].data_433[j][0] = Receive[2];
            pStudyDat->addr_data_433[i].data_433[j][1] = Receive[3];
            pStudyDat->addr_data_433[i].data_433[j][2] = Receive[4];
            Save433StudyDat(pStudyDat);
            break;
          }
        }
        if( TickGet( ) - remote_time[7] > 2000 )
        {
          remote_time[7] = TickGet( );
          Specify_Musi_Play(29);//紧急按钮
          hand_send_alrm(&datatemp[0],ALM_key);
          //  return 0; 
        }
      }
    }
    
  }   //end if  两次检测值相同
  //end if  检测到值不是零
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
u16 GenerateRandomU16( void ) //利用adc产生随机数种子
{
  u16 tmp	   = 0;
  u16 mask   = 0x8000;
  
  do
  {
    while( ADC_GetFlagStatus( ADC2, ADC_FLAG_EOC ) == RESET )
    {
      ;                                                           //等待ADC转完成
    }
    tmp	  |= ( ADC_GetConversionValue( ADC2 ) & 0x01 ) ? mask : 0;  //set tmp based on adc's lsb
    mask   = mask >> 1;                                             //shift to the next bit
  }
  while( mask );
  
  return tmp;
}





void send_433data(u8 *data,u8 times){
  u8   i;
  uint8 		DataTemp[60];
  u16			s_t;
  u16			f_t;
  
  memcpy(DataTemp,data,N+1+6); 	
  
  s_t=(DataTemp[6]<<8)|DataTemp[7];
  f_t=(DataTemp[8]<<8)|DataTemp[9];
  
  if(DataTemp[10]==0&&DataTemp[11]==0)
  {
    DataTemp[10]=28;
    DataTemp[11]=90;
  }
  if(DataTemp[0]==0x23) 
  {
    if(times==0)
    {
      jxr_kaguan(&DataTemp[1]);
    }
  }
  else if(DataTemp[0]==0x22)
  {
    Set_Ask_TxMode();
    //    printf("d1=%x,d2=%x,d3=%x,d4=%x,d5=%x\r\n",DataTemp[1],DataTemp[2],DataTemp[3],DataTemp[4],DataTemp[5]);
    //  for(i=0;i<3;i++){
    cl_out(&DataTemp[1],1000,0,0x2C,0X54);//控制窗帘 //984
    
    //   delay_ms(10);
    //    }
    Set_Ask_RxMode();
  }
  else if(DataTemp[0]==0x20)
  {
    Set_Ask_TxMode();
    //    printf("d1=%x,d2=%x,d3=%x,d4=%x,d5=%x\r\n",DataTemp[1],DataTemp[2],DataTemp[3],DataTemp[4],DataTemp[5]);
    //  for(i=0;i<3;i++){
    
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],8);//控制插座、开关灯
    delay_ms(400);
    //  }
    Set_Ask_RxMode();
  }
  else if(DataTemp[0]==0x25)
  {
    Set_Ask_TxMode();
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],30);//控制插座、开关灯
    delay_ms(100);
    //  }
    Set_Ask_RxMode();
  }
  else 
  {
    Set_Ask_TxMode();
    //    printf("d1=%x,d2=%x,d3=%x,d4=%x,d5=%x\r\n",DataTemp[1],DataTemp[2],DataTemp[3],DataTemp[4],DataTemp[5]);
    //  for(i=0;i<3;i++){
    //  delay_ms(100);
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],8);//控制插座、开关灯
    
    //  }
    Set_Ask_RxMode();
  }
  
}                



void hand_save_433(u8 dev_typ){
  
  uint8			k_value = 0;
  //static u8		k2 = 0;
  u8 temp[30];
  
  uint8			DataTemp[N*2]; //用于学习两次jiama()值，对比，一致才学习成功。
  SEND_433_DATA	*pSendStorDat;
  uint8			SendCntAddr;
  uint8			SendTemp[2 * N]; //用于学习两次jiama()值，对比，一致才学习成功。
  uint8			i, j;
  u32				send_time	   = 0;
  u32				study_time	   = 0;     
  u8				Already_Study = 0;
  pSendStorDat = &SendDat;
  study_time	   = TickGet( );
  
  while( ( TickGet( ) > ( study_time + 5000 ) ) ? 0 : 1 ) //8秒超时退出
  {
    IWDG_Feed();
    Jiema( study );
    for( i = 0; i < N; i++ )
    {
      DataTemp[i] = Receive[i];
    }
    
    //delay_ms( 20 );
    Jiema( study );
    for( i = 0; i < N; i++ )
    {
      DataTemp[i + N] = Receive[i];
    }
    
    if( Already_Study == 1 )
    {
      Already_Study = 0;
    }
    
    
    if( ( DataTemp[0] != 0x00 ) 
       && ( DataTemp[1] != 0x00 ) 
         && ( DataTemp[2] != 0x00 ) 
           && ( DataTemp[N] != 0x00 )
             && ( DataTemp[N + 1] != 0x00 ) 
               && ( DataTemp[N + 2] != 0x00 ) )
    { 
      if( ( DataTemp[0] == DataTemp[N] ) 
         && ( DataTemp[1] == DataTemp[N + 1] )
           && ( DataTemp[2] == DataTemp[N + 2] ) 
             && ( DataTemp[3] == DataTemp[N + 3] )
               && ( DataTemp[4] == DataTemp[N + 4] ) )
      {
        Specify_musi_play(43);             //匹配成功
        delay_ms(500);
        TIME_Get( TIM2_tick );                  //获取系统时间
        
        temp[0]=dev_typ;
        memcpy(&temp[1],Receive,5);
        temp[6]=SYNO_T;
        temp[7]=(SYNO_T>>8);
        temp[8]=FIRST_LT;
        temp[9]=(FIRST_LT>>8);
        temp[10]=LOW_T;  //440us   低电平时长
        temp[11]=HIGH_T;  //440us   低电平时长
        temp[12]=calendar.hour;
        temp[13]=calendar.min;
        temp[14]=calendar.sec;
        temp[15]= AlarmFlags.bIsRmt_Deploy;
        wifi_sendcmd((BYTE *)temp,16,Study_433_device,GprsData[4]); 
        handly_get_value(1,5);                     
        
        
        
        if(dev_typ==PA)
        {
          k_value=check_sensor_remote(DataTemp);
          if(k_value!=0)
          { 
            temp[7]=SYNO_T;
            temp[6]=(SYNO_T>>8);
            temp[9]=FIRST_LT;
            temp[8]=(FIRST_LT>>8);
            
            
            
            PA_DATA.en_disable=1;
            memcpy(&PA_DATA.pa_data[k_value-1][0],temp,12);
            SavePA_data(&PA_DATA);
          } 
        }
      }
    }
  }
  
}





void MakeDeviceID( u8 *pdeviceID )
{
  u32 CpuID[3];
  u32 tempID;
  //获取CPU唯一ID
  CpuID[0]   = *(u32*)( 0x1ffff7e8 );
  CpuID[1]   = *(u32*)( 0x1ffff7ec );
  CpuID[2]   = *(u32*)( 0x1ffff7f0 );
  printf( "CPUID1=%x,CPUID2=%x,CPUID3=%x\r\n", CpuID[0], CpuID[1], CpuID[2] );
  //很简单的算法
  tempID = CpuID[0] + CpuID[1] + CpuID[2];
  printf( "ID=%d\r\n", tempID );
  printf( "ID=%X\r\n", tempID );
  
  *pdeviceID = (u8)tempID;
  pdeviceID++;
  *pdeviceID = (u8)( tempID >> 8 );
  pdeviceID++;                                                                                              //用于产生32位ID
  *pdeviceID = (u8)( tempID >> 16 );
  pdeviceID++;
  *pdeviceID = (u8)( tempID >> 24 );
}




void send_device_info()
{ u8 temp[50];

InitDevInfo( &Deviceinfo );

memcpy(temp,&(Deviceinfo.DeviceName[0]),9);
memcpy(&temp[9],&(Deviceinfo.FirmwareVer[0]),4);
memcpy(&temp[13],&(Deviceinfo.SerialNO[0]),8);

wifi_sendcmd((BYTE *)temp,21,send_dev_info,GprsData[4]);
delay_ms(500);
}




u16 Get_Adc( void )
{
  //设置指定ADC的规则组通道，一个序列，采样时间
  ADC_RegularChannelConfig( ADC1, 10, 1, ADC_SampleTime_239Cycles5 ); //ADC1,ADC通道,采样时间为239.5周期
  
  ADC_SoftwareStartConvCmd( ADC1, ENABLE );                           //使能指定的ADC1的软件转换启动功能
  
  while( !ADC_GetFlagStatus( ADC1, ADC_FLAG_EOC ) )
  {
    ;                                                               //等待转换结束
  }
  return ADC_GetConversionValue( ADC1 );                              //返回最近一次ADC1规则组的转换结果
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
u16 Get_Adc_Average( u8 times )
{
  u32 temp_val = 0;
  u8	t;
  for( t = 0; t < times; t++ )
  {
    temp_val += Get_Adc( );
    //delay_ms(5);
  }
  return (u16)( ( ( temp_val / times ) * 330 * 78 ) >> 12 ); //mv
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/





void ctl_Scenar(u8 type)
{
  
  
  zhuhe_send(&S_MODE.Send_433[type][0]);
  
}

void save_Scenar(u8 *data)
{ u8 i;
if(data[0]<5){
  
  memcpy(&S_MODE.area[data[0]][0],&data[1],6);
  memcpy(&S_MODE.Send_433[data[0]][0],&data[7],data[7]*12+1);
  S_MODE.enable_num=S_MODE.area[0][1]+S_MODE.area[1][1]+S_MODE.area[2][1]+S_MODE.area[3][1]+S_MODE.area[4][1];
  
  // S_MODE.area[data[0]]=data[1];
  SaveScenario_mode(&S_MODE);
  
  return ;
}
}


/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/

/************************************** The End Of File **************************************/
