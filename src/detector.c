/************************************************************
* Copyright (C), 2009-2011, Donjin Tech. Co., Ltd.
* FileName:		// �ļ���
* Author:			// ����
* Date:			// ����
* Description:		// ģ������
* Version:			// �汾��Ϣ
* Function List:	// ��Ҫ�������书��
*     1. -------
* History:			// ��ʷ�޸ļ�¼
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
char					rec[N][8];          //��ѧϰ��ַ�����ݴ洢
uint16					rel[N][8];
uint8					have_body  = 1;
uint16					LOW_T	   = 0x30;  //440us   �͵�ƽʱ��
uint16					SYNO_T	   = 0x4a5; //10.7ms  ͬ��ͷʱ��
uint16					FIRST_LT   = 0x182;
uint16					HIGH_T	   = 0x90;  //1.3ms   �ߵ�ƽʱ��


extern u8				keyflag;            //������־
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

extern KeyMenu_TypeDef	Kmenu;          //�����˵�
extern PA_data         PA_DATA;
u8 copyi;
u8 copyj;
u32						send_time = 0;  //���ݷ��ͱ�־
u32						net_send_timer = 0;

u8 IR_data[50];
u8 IR_memu[4];

#define ABS16( A, B ) ( ( A > B ) ? ( A - B ) : ( B - A ) )


extern IR_Data  ir_data;


extern Alarm_Flags		AlarmFlags;
extern BOOL			FlagFirstRun;

void voice_delete_app(u8 *temp,u8 i);

//��ʼ������
//PB6 433_TXOUT 433���� ���
//PB7 433_RXIN  433���� ����
//PC0 BAT_DET   ��ؼ�� ģ������ adc
//PC1 IR_IN     �������� ����
//PC2 MV_IN     ΢������ ����
//PA11 TRIG     ���10us�ߵ�ƽ����ʼ���  ���
//PA12 echo     ����ߵ�ƽ�����         ����
//����=echo �ߵ�ƽʱ��*340m/s / 2

void Detector_Init( void )
{GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC| RCC_APB2Periph_AFIO, ENABLE ); //ʹ��PORTA,PORTB,PORTCʱ��



//433����ܽ�
GPIO_InitStructure.GPIO_Pin	   = GPIO_Pin_1;            //PB7
GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_IPU;          //���ó���������
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
GPIO_Init( GPIOC, &GPIO_InitStructure );


GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);
EXTI_InitStructure.EXTI_Line	  = EXTI_Line1; 	 
EXTI_InitStructure.EXTI_Mode	  = EXTI_Mode_Interrupt;
EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
EXTI_InitStructure.EXTI_LineCmd = ENABLE;  

EXTI_Init(&EXTI_InitStructure);


//�жϷ���
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //ռ�����ȼ��������ȼ�����Դ���� 
NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn; 
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;	 
NVIC_InitStructure.NVIC_IRQChannelCmd  = ENABLE;
NVIC_Init(&NVIC_InitStructure);	 
NVIC_DisableIRQ(EXTI1_IRQn);


}


//��ʼ��ADC
//�������ǽ��Թ���ͨ��Ϊ��
//����Ĭ�Ͻ�����ͨ��0~3																	   
void  Adc_Init(void)
{ 	
  ADC_InitTypeDef ADC_InitStructure; 
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1, ENABLE );	  //ʹ��ADC1ͨ��ʱ��
  
  
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M
  
  //PA1 ��Ϊģ��ͨ����������                         
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//ģ����������
  GPIO_Init(GPIOA, &GPIO_InitStructure);	
  
  ADC_DeInit(ADC1);  //��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ
  
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//ģ��ת�������ڵ�ͨ��ģʽ
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//ģ��ת�������ڵ���ת��ģʽ
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
  ADC_InitStructure.ADC_NbrOfChannel = 1;	//˳����й���ת����ADCͨ������Ŀ
  ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���   
  
  
  ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
  
  ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
  
  while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
  
  ADC_StartCalibration(ADC1);	 //����ADУ׼
  
  while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
  
  //	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
  
}				  


void Delay(uint16_t time)
{      
  TIM3->CNT = 0;
  TIM3->CR1|=0x01;    //ʹ�ܶ�ʱ��3
  while(TIM3->CNT!=time);
  TIM3->CR1&=0xFFFE;    //�رն�ʱ��3
  TIM3->CNT = 0;
  
}


void DELAY(uint16_t time)
{      
  TIM5->CNT = 0;
  TIM5->CR1|=0x01;    //ʹ�ܶ�ʱ��3
  while(TIM5->CNT!=time);
  TIM5->CR1&=0xFFFE;    //�رն�ʱ��3
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
//��� ģ�Ⲩ�η���

//��� ģ�Ⲩ�η���
void cl_out(u8 * pdata,u16 SYNO, u16 First_LT,u8 LOW, u8 HIGH)
{
  uint8_t i,j,m;
  uint8_t *p;
  uint8_t temp;
  // SYNO = 984;
  // LOW=0x2C;
  //  HIGH=0X54;
  IWDG_Feed();
  //z=First_LT&0x03FF;//�͵�ƽʱ�䳤��
  
  for(m=0;m<8;m++)//��������5������
  {
    p =pdata;
    temp =*p;
    // delay_ms(50);
    C_Sys_out(SYNO);
    for(i=0;i<N;i++)//N����
    {
      for(j=0;j<4;j++)//һ����
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
  }//end �������
}//fashe_out



void fashe_out(u8 * pdata,u16 SYNO, u16 First_LT,u8 LOW, u8 HIGH,u8 times)
{
  uint8_t i,j,m;
  uint8_t *p;
  uint8_t temp;
  uint8_t x,y;
  x=(uint8_t)((First_LT&0xE000)>>13);//��͵�ƽ�ֽ�λ��
  y=(uint8_t)((First_LT&0x1C00)>>10);//��͵�ƽλ λ��
  IWDG_Feed();
  for(m=0;m<times;m++)//��������5������
  {
    p =pdata;
    temp =*p;
    
    
    CPU_IntDis();
    // CPU_IntEn();
    Sys_out(SYNO);
    if(First_LT == 0)//������ӿ�͵�ƽ
    {
      for(i=0;i<N;i++)//N����
      {
        for(j=0;j<4;j++)//һ����
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
    else//��Ҫ��ӿ�͵�ƽ
    {
      for(i=0;i<N;i++)//������
      {
        for(j=0;j<4;j++)//һ����
        {
          if(i == x)//���ҵ��ǵڼ����ֽ���Ҫ�ӿ�͵�ƽ
          {
            if(j != y/2)//�ҵ������͵�ƽλ��
            {
              send_out_433(temp,0,LOW,HIGH);
            }
            else//�ҵ�λ����
            {
              send_out_433(temp,First_LT,LOW,HIGH);  
            }
          }
          else
          {
            send_out_433(temp,0,LOW,HIGH);
          }
          temp <<=2;
        }//���������ַ���
        p++;
        temp =*p;
      }
    }//end���ӿ�͵�ƽ����
    
    // CPU_IntDis();
    CPU_IntEn();
    
  }//end �������
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
  
  TIM5->CR1  = 0x00;          //�ض�ʱ��
  TIM5->CNT  = 0;             //��������
  
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
  
  TIM5->CR1  = 0x00;          //�ض�ʱ��
  TIM5->CNT  = 0;             //��������
  
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
  u8 k = 1;  //Ԫ����,�������ݴ� s[3]��ʼ
  u8 n = 0;   //�ֽ���
  u8 m = 0;   //λ��
  u8 x = 0;   //λ��ֵ
  u8 i,j,k_flag1=0,k_flag2=0;
  u16 SN_ll=600,SN_lh=1200;
  memset(IR_data,0,sizeof(IR_data));
  IR_data[0] = IR_memu[0]; //ÿ�ζ�Ҫ��ʼ��������Ӱ��֮���λ������
  IR_data[1] = IR_memu[1];
  IR_data[2] = IR_memu[2];
  IR_data[3] = IR_memu[3];
  IR_data[4] = sync_L; //ÿ�ζ�Ҫ��ʼ��������Ӱ��֮���λ������
  IR_data[5] = sync_L>>8;
  IR_data[6] = sync_H;
  IR_data[7] = sync_H>>8;
  
  //LED_send(100);
  
  if(IR_data[0]==xiaomi)
    
  {
    //�͵�ƽ625us
    
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
  //С��
  //����
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
    
    for(n=0;n<IR_data[8];n++)  //�ĸ��ֽ�
    {
      for(m=0;m<8;m++)  //��λ
      {
        if(s[k] > 1500)   //�ߵ�ƽʱ�����1500usΪ1
        {
          x = 0x80>>m;    //0x80������λ��һλ,��λ��������0x80,0x40,0x20...0x01
          IR_data[n+13] = IR_data[n+13] | x;  //�����һλΪ1��ֵΪ0x80����λ��
        }
        k = k+2;       //�����͵�ƽ������һ�ߵ�ƽ
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
    for(n=0;n<IR_data[8];n++)  //�ĸ��ֽ�
    {
      for(m=0;m<8;m++)  //��λ
      {
        if(s[k] > 1000)   //�ߵ�ƽʱ�����1500usΪ1
        {
          x = 0x80>>m;    //0x80������λ��һλ,��λ��������0x80,0x40,0x20...0x01
          IR_data[n+13] = IR_data[n+13] | x;  //�����һλΪ1��ֵΪ0x80����λ��
        }
        k = k+1;       //�����͵�ƽ������һ�ߵ�ƽ
      }
      
    }
    
  }
  //������� 
  if(IR_data[8]==0)
  {
    return;
  }
  
  NVIC_DisableIRQ(EXTI1_IRQn);
  EXTI_ClearFlag(EXTI_Line1);
  //  wifi_sendcmd((BYTE *)IR_data,IR_data[7]+12,Study_IR,0);
  //   handly_get_value(1,5); 
  
  // printf("�������\r\n");
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
  TIM5->CR1  = 0x01;          //�ض�ʱ��
  TIM5->CNT  = 0;             //��������
  while(IR_DATA==0)
  {
    IWDG_Feed();
  }
  sync_L= TIM5->CNT;
  //  i++;
  TIM5->CNT  = 0;             //��������
  
  while(IR_DATA!=0)
  {
    IWDG_Feed();
    if(TIM5->CNT>10000)
    {
      printf("������");
      return 0;
    }
  }
  
  sync_H= TIM5->CNT;
  TIM5->CNT  = 0;             //��������
  
  
  for(i=0;i<200;)
  {
    
    
    while(IR_DATA==0)
    {
      IWDG_Feed();
    }
    
    s[i]= TIM5->CNT;
    TIM5->CNT  = 0;             //��������
    if(3500<s[i])
    {
      printf("������");
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
    TIM5->CNT  = 0;             //��������
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
            // printf("ͬ��ͷ�������,��ʼ��������\r\n");
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
              
              Specify_musi_play(43);             //ƥ��ɹ�
              delay_ms(500);
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
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
  u16			temp_l;         //�����жϲ��θߵ͵�ƽʱ��
  u8			k		   = 0;
  u8			m		   = 0;
  u8			PulseCnt   = 0; //������һ�ε�ͬ��ͷ
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
    while( ( CSIO_DATA ) != 0 ) //�ȴ��͵�ƽ(��Ҫһ���ȴ���ʱ����ֹ�ùܽ�һֱ�ߵ�ƽ���޷��˳�)
    {
      if( TickGet( ) - jiema_time > 1000 )
      {
        LED3_ON;            //���315����ģ���Ƿ���
        return;
      }
    }
    
    TIM4->CR1 = 0x01;           //����ʱ��
    while( ( CSIO_DATA ) == 0 ) //�ȴ��ߵ�ƽ
    {
      IWDG_Feed();
      if( TickGet( ) - jiema_time > 1000 )
      {
        LED3_ON;            //���315����ģ���Ƿ���
        return;
      }
    }
    
    //temp= TIM4->CNT ;
    SYNO_T	   = TIM4->CNT;
    TIM4->CR1  = 0x00;          //�ض�ʱ��
    TIM4->CNT  = 0;             //��������
    
    TIM3->CR1  = 0x01;          //������ʱ��
    k++;                        //һ�������жϣ�����JIEMA��������ֹ��ѭ��
    if( SYNO_T < 800 )
    {
      temp_l = SYNO_T >> 4;
    } else
    {
      temp_l = SYNO_T >> 5;
    }
    //SYNO_T = temp;//ͬ��ͷʱ��������ģ�Ⲩ�η���
    //temp = 0;
    //  if( ( 0x20b< temp ) && ( temp < 0x3F0) )//ȡ2.9ms----21.87msͬ��ͷ
    if( ( 0x200 < SYNO_T ) && ( SYNO_T < 0x5F0 ) )  //ȡ2.9ms----21.87msͬ��ͷ
    {
      // UART4_printf("temp=%d \r\n",temp);
      PulseCnt++;
      m++;                                        //��ֹ���ţ�һֱ��ѭ������������ѭ����������һ�μ��
      if( PulseCnt != 1 )                         //��һ�μ�⵽ͬ���룬����
      {
        TIM3->CR1 = 0x00;                       //�رն�ʱ��
        continue;
      }
      for( j = 0; j < N; j++ )                    //ѭ��4��
      {
        for( i = 0; i < 8; i++ )
        {
          while( ( CSIO_DATA ) == 0 )         //�ȴ��ߵ�ƽ
          {
            if( TickGet( ) - jiema_time > 1000 )
            {
              LED3_ON;
              //���315����ģ���Ƿ���
              return;
            }
          }
          TIM4->CR1  = 0x01;          //������ʱ��
          rel[j][i]  = TIM3->CNT;
          
          TIM3->CR1  = 0x00;          //�رն�ʱ��
          TIM3->CNT  = 0;
          if( rel[j][i] > ( SYNO_T - 100 ) )
          {
            if( j < 3 )
            {
              TIM3->CR1  = 0x00;  //�رն�ʱ��
              TIM4->CR1  = 0x00;  //�رն�ʱ��
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
          while( ( CSIO_DATA ) != 0 )                                                             //�ȴ��͵�ƽ
          {
            IWDG_Feed();	
            if( TickGet( ) - jiema_time > 1000 )
            {
              LED3_ON;                                                                        //���315����ģ���Ƿ���
              return;
            }
          }
          
          TIM3->CR1  = 0x01;                                                                      //������ʱ��
          rec[j][i]  = TIM4->CNT;
          // UART4_printf("t(%d,%d)=%d \r\n",j,i,timer4);
          TIM4->CR1  = 0x00;                                                                      //�رն�ʱ��
          TIM4->CNT  = 0;
        }
      }
      for( j = 0; j < N; j++ )                                                                        //ѭ��4��
      {
        for( i = 0; i < 8; i++ )
        {
          //UART4_printf("%d%d=%d\r\n",j,i,rec[j][i]);
          Receive[j] = Receive[j] << 1;                                                           //�������е�ַ������
          if( ( ( temp_l / 3 ) < rec[j][i] ) && ( rec[j][i] < ( temp_l * 7 / 3 ) ) )              //�ж�խ����
          {
            LOW_T	   = rec[j][i];                                                             //խ����ʱ�䳤��
            Receive[j] = Receive[j] + 0x01;
          }else if( ( ( temp_l * 8 / 3 ) < rec[j][i] ) && ( rec[j][i] < ( temp_l * 13 / 3 ) ) )   //������
            
          {
            HIGH_T	   = rec[j][i];                                                             //������ʱ�䳤��
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
      TIM3->CR1 = 0x00; //�رն�ʱ��
    }
  }
}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Study_Remote( void ) //ң����
{
  UART4_printf( "��������:ѧϰң����" );
  Specify_musi_play( 0x02 );  //ѧϰң����
  Study( remote,GprsData[4]);
}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Study_Menci( void )        //�Ŵ�
{
  UART4_printf( "��������:ѧϰ�Ŵ�" );
  Specify_musi_play( 0x03 );  //ѧϰ�ŴŴ���
  Study( Menci,GprsData[4]);
}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Study_Infrared( void )     //����
{
  UART4_printf( "��������:ѧϰ���������" );
  Specify_musi_play( 0x04 );  //ѧϰ���ⴰĻ
  Study( Infrared,GprsData[4]);
}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Study_Smoke( void )        //����
{
  UART4_printf( "��������:ѧϰ��������" );
  Specify_musi_play( 0x05 );  //ѧϰ�����豸
  Study( smoke,GprsData[4]);
}
void Study_Gas( void )        //����
{
  UART4_printf( "��������:ѧϰȼ��������" );
  Specify_musi_play(15);  //ѧϰ�����豸
  Study( gas,GprsData[4]);
}
void Study_calling( void )        //����
{
  UART4_printf( "��������:ѧϰ������ť" );
  Specify_musi_play(37);  //ѧϰ�����豸
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
    if( (pStudyStorDat->addr_data_433[i].addr_433[0]!=0x00) && ( pStudyStorDat->addr_data_433[i].addr_433[1] !=0x00))    //��ַ�Ѿ�ѧϰ
      
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
    
    //����ѧϰ�����ͬ��ѧϰ�ɹ�
    Load433StudyDat( pStudyStorDat );
    StudyCntAddr = pStudyStorDat->Count_433;
    switch( sensor )    //��ȡ�洢λ��,����Ҫ�жϣ����豸֮ǰ��û��ѧϰ������ѧϰ��������ԭ��λ��
    {
    case remote:
      for( i = 0; i < NODE_CNT; i++ )
      {						
	if( pStudyStorDat->addr_data_433[i].Area == 0 )
	{
          continue;
	}
	if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            for( j = 0; j < 4; j++ )
            {
              
              if( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][2] == TempData[4] ) )
              {
                UART4_printf( "������ʾ:ң�����Ѿ�ѧϰ!" );
                Specify_musi_play( 8 );             //�Ѿ�ѧϰ
                delay_ms(1000);
                i			   = NODE_CNT;          //��������
                Already_Study  = 1;
                
                //return;
                break;
              }else if( ( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0xff ) ) )
              {
                UART4_printf( "������ʾ:ң����ѧϰ����һ������ֵ!" );
                if(info==0)
                  Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
                
                pStudyStorDat->addr_data_433[i].data_433[j][0] = TempData[2];
                pStudyStorDat->addr_data_433[i].data_433[j][1] = TempData[3];
                pStudyStorDat->addr_data_433[i].data_433[j][2] = TempData[4];
                Save433StudyDat( pStudyStorDat );   //�洢ң�����������ֵ
                i			   = NODE_CNT;          //��������
                
                Already_Study  = 1;
                
                
                
                TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                
                
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
          }else //�Ѿ�ѧϰ��������������
          {
            if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
              Specify_musi_play( 23 );        //�Ѿ�ѧϰ���Ŵ�
              delay_ms(1000);
              i			   = NODE_CNT;      //��������
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
              Specify_musi_play( 24 );        //�Ѿ�ѧϰ�����ⴰĻ
              delay_ms(1000);
              i			   = NODE_CNT;      //��������
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
              Specify_musi_play( 25 );                                                                                                        //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
              Specify_musi_play( 40 );
              delay_ms(1000);
              //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
              Specify_musi_play(39);     
              delay_ms(1000);                   //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              //return;
              break;
            }
          }
        }
      }
      break;
      
      //�Ŵ��ж�
    case Menci:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
            Specify_musi_play( 26 );                                                                                                        //�Ѿ�ѧϰ��ң����
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
              delay_ms(1000);
              Specify_musi_play( 8 );                                                                                                     //�Ѿ�ѧϰ
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
              
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
              
              
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
              UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
              Specify_musi_play( 24 );        //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;      //��������
              Already_Study  = 1;
              
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
              Specify_musi_play( 25 );  
              delay_ms(1000);
              //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
              Specify_musi_play( 40 );  
              delay_ms(1000);
              //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              //return;
              break;
            }
            else if( ( NODE_CNT / dev_typ_cnt * 6<= i ) && ( i < NODE_CNT ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
              Specify_musi_play(39);        
              delay_ms(1000);
              //�Ѿ�ѧϰ�����ⴰĻ
              i			   = NODE_CNT;                                                                                                      //��������
              Already_Study  = 1;
              
              //return;
              break;
            }
          }
        }
      }
      break;
      //���ⴰĻ
    case Infrared:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
            Specify_musi_play( 26 );                                                                                                        //�Ѿ�ѧϰ��ң����
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt *3 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
            Specify_musi_play( 23 );    
            delay_ms(1000);
            //�Ѿ�ѧϰ���Ŵ�
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
              Specify_musi_play( 8 );       
              delay_ms(1000);
              //�Ѿ�ѧϰ
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
              
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
              
              
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
            UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
            Specify_musi_play( 25 );   
            delay_ms(1000);
            //�Ѿ�ѧϰ�����ⴰĻ
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
            
            Specify_musi_play( 40 ); 
            delay_ms(1000);
            //�Ѿ�ѧϰ�����ⴰĻ
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
            Specify_musi_play(39); 
            delay_ms(1000);
            //�Ѿ�ѧϰ�����ⴰĻ
            i			   = NODE_CNT;                                                                                                      //��������
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
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
            
            Specify_musi_play( 26 ); 
            delay_ms(1000);
            //�Ѿ�ѧϰ��ң����
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
            Specify_musi_play( 23 );      
            //�Ѿ�ѧϰ���Ŵ�
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
            
            Specify_musi_play( 24 );    
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
              Specify_musi_play( 8 );                 
              //�Ѿ�ѧϰ
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              if(info==0)
                Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
              
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
              
              
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
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
            Specify_musi_play( 40 );  
            delay_ms(2000);
            //�Ѿ�ѧϰ�����ⴰĻ
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
            Specify_musi_play(39); 
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1500);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          
        }
      }
      
      break;
      //����������
    case gas:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
            Specify_musi_play( 26 );                   
            //�Ѿ�ѧϰ��ң����
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
            Specify_musi_play( 23 );                 
            //�Ѿ�ѧϰ���Ŵ�
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
            Specify_musi_play( 24 );    
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
            Specify_musi_play( 25 ); 
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
              
              Specify_musi_play( 8 );      
              //�Ѿ�ѧϰ
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              if(info==0)
                Specify_musi_play( 0x06 );          //ѧϰ�ɹ�            
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
              
              
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
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
            Specify_musi_play(39);            
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          
          
        }
      }
      
      break;
      //����������
    case arlrm_calling:
      for( i = 0; i < NODE_CNT; i++ )
      {
        copyi = i;
        if( pStudyStorDat->addr_data_433[i].Area == 0 )
        {
          continue;
        }
        if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
        {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
          if( i < ( NODE_CNT / dev_typ_cnt ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
            Specify_musi_play( 26 );    
            //�Ѿ�ѧϰ��ң����
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
            Specify_musi_play( 23 );          
            //�Ѿ�ѧϰ���Ŵ�
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }else if( ( NODE_CNT / dev_typ_cnt *3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
            Specify_musi_play( 24 );        
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
            Specify_musi_play( 25 );   
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
          {
            UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
            Specify_musi_play( 40 );   
            //�Ѿ�ѧϰ�����ⴰĻ
            delay_ms(1000);
            i			   = NODE_CNT;                                                                                                      //��������
            Already_Study  = 1;
            
            //return;
            break;
          }
          else if( ( NODE_CNT / dev_typ_cnt *6<= i ) && ( i < NODE_CNT ) )
          {
            if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == TempData[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == TempData[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == TempData[4] ) )
            {
              UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
              Specify_musi_play( 8 );      
              //�Ѿ�ѧϰ
              delay_ms(1000);
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              
              //return;
              break;
            }else
            {
              pStudyStorDat->addr_data_433[i].data_433[0][0] = TempData[2];
              pStudyStorDat->addr_data_433[i].data_433[0][1] = TempData[3];
              pStudyStorDat->addr_data_433[i].data_433[0][2] = TempData[4];
              pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
              Save433StudyDat( pStudyStorDat );
              i			   = NODE_CNT;                                                                                                  //��������
              Already_Study  = 1;
              if(info==0)
                Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
              
              TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
              
              
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
      UART4_printf( "������ʾ:�Ѿ��ﵽѧϰ20���豸���ֵ���޷�ѧϰ!" );
      Specify_musi_play( 10 ); //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i + 1; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "remote�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          
          Already_Study  = 1;
          
          //return;
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "remote�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    
            //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "Menci�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "Menci�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 ); //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "Infrared�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "Infrared�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 ); //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "smoke�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "Other�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "gas�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "Other�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //�洢��������ɾ�����ٴ�ѧϰ
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
          pStudyStorDat->addr_data_433[i].Area		   = i; //�洢λ��
          Save433StudyDat( pStudyStorDat );
          UART4_printf( "arlrm_calling�洢�ɹ�!" );
          if(info==0)
            Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
          Already_Study  = 1;
          
          TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
          
          
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
            UART4_printf( "Other�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
            Specify_musi_play( 10 );    //�洢��������ɾ�����ٴ�ѧϰ
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
  uint8			DataTemp[2 * N]; //����ѧϰ����jiama()ֵ���Աȣ�һ�²�ѧϰ�ɹ���
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
  while( ( TickGet( ) > ( study_time + 4000 ) ) ? 0 : 1 ) //8�볬ʱ�˳�
  {
    IWDG_Feed();
    if( ( TickGet( ) - key_delay_time ) > 500 )
    {
      key_delay_time = TickGet( );
      if( keyflag == K_SET )                          //���������˳���ǰѧϰ��������һ��
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
      {                       //����ѧϰ�����ͬ��ѧϰ�ɹ�
        UART4_printf( "R=0x%x %x %x %x %x\r\n", Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
        Load433StudyDat( pStudyStorDat );
        StudyCntAddr = pStudyStorDat->Count_433;
        switch( sensor )    //��ȡ�洢λ��,����Ҫ�жϣ����豸֮ǰ��û��ѧϰ������ѧϰ��������ԭ��λ��
        {
        case remote:
          for( i = 0; i < NODE_CNT; i++ )
          {
            //copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                for( j = 0; j < 4; j++ )
                {
                  copyj = j;
                  if( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[j][2] == Receive[4] ) )
                  {
                    UART4_printf( "������ʾ:ң�����Ѿ�ѧϰ!" );
                    Specify_musi_play( 8 );             //�Ѿ�ѧϰ
                    delay_ms(1000);
                    i			   = NODE_CNT;          //��������
                    Already_Study  = 1;
                    //	study_time	   = TickGet( );        //ʱ�����¸���
                    break;
                  }else if( ( ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][0] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][1] == 0xff ) ) && ( ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0 ) || ( pStudyStorDat->addr_data_433[i].data_433[j][2] == 0xff ) ) )
                  {
                    UART4_printf( "������ʾ:ң����ѧϰ����һ������ֵ!" );
                    Specify_musi_play( 0x06 );          //ѧϰ�ɹ�
                    pStudyStorDat->addr_data_433[i].data_433[j][0] = Receive[2];
                    pStudyStorDat->addr_data_433[i].data_433[j][1] = Receive[3];
                    pStudyStorDat->addr_data_433[i].data_433[j][2] = Receive[4];
                    
                    Save433StudyDat( pStudyStorDat );   //�洢ң�����������ֵ
                    i			   = NODE_CNT;          //��������
                    study_time	   = TickGet( );        //ʱ�����¸���
                    
                    
                    Already_Study  = 1;
                    
                    TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                    
                    
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
              }else //�Ѿ�ѧϰ��������������
              {
                if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
                  Specify_musi_play( 23 );        //�Ѿ�ѧϰ���Ŵ�
                  delay_ms(2000);
                  i			   = NODE_CNT;      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //ʱ�����¸�
                  break;
                }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
                  Specify_musi_play( 24 );        //�Ѿ�ѧϰ�����ⴰĻ
                  delay_ms(2000);
                  i			   = NODE_CNT;      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
                  Specify_musi_play( 25 ); 
                  delay_ms(2000);
                  //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt *6 ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
                  Specify_musi_play( 40 );
                  delay_ms(2000);
                  //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
                  Specify_musi_play(39);     
                  delay_ms(2000);                //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
              }
            }
          }
          break;
          
          //�Ŵ��ж�
        case Menci:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
                Specify_musi_play( 26 );                                                                                                        //�Ѿ�ѧϰ��ң����
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
                  delay_ms(1000);
                  Specify_musi_play( 8 );                                                                                                     //�Ѿ�ѧϰ
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  
                  
                  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                  
                  
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
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
                  Specify_musi_play( 24 );        //�Ѿ�ѧϰ�����ⴰĻ
                  delay_ms(2000);
                  i			   = NODE_CNT;      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
                  Specify_musi_play( 25 );  
                  delay_ms(1000);
                  //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
                  Specify_musi_play( 40 );  
                  delay_ms(2000);
                  //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
                  Specify_musi_play(39);        
                  delay_ms(2000);
                  //�Ѿ�ѧϰ�����ⴰĻ
                  i			   = NODE_CNT;                                                                                                      //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                  
                  //return;
                  break;
                }
              }
            }
          }
          break;
          //���ⴰĻ
        case Infrared:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
                Specify_musi_play( 26 );                                                                                                        //�Ѿ�ѧϰ��ң����
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
                Specify_musi_play( 23 );    
                delay_ms(2000);
                //�Ѿ�ѧϰ���Ŵ�
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //		study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
                  Specify_musi_play( 8 );       
                  delay_ms(1000);
                  //�Ѿ�ѧϰ
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  
                  
                  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                  
                  
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
                UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
                Specify_musi_play( 25 );   
                delay_ms(2000);
                //�Ѿ�ѧϰ�����ⴰĻ
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
                
                Specify_musi_play( 40 ); 
                delay_ms(2000);
                //�Ѿ�ѧϰ�����ⴰĻ
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
                Specify_musi_play(39); 
                delay_ms(2000);
                //�Ѿ�ѧϰ�����ⴰĻ
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
            }
          }
          break;
          //����������
        case smoke:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
                
                Specify_musi_play( 26 ); 
                delay_ms(2000);
                //�Ѿ�ѧϰ��ң����
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
                Specify_musi_play( 23 );      
                //�Ѿ�ѧϰ���Ŵ�
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
                
                Specify_musi_play( 24 );    
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
                  Specify_musi_play( 8 );                 
                  //�Ѿ�ѧϰ
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  //return;
                  break;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  
                  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                  
                  
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
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
                Specify_musi_play( 40 );  
                delay_ms(2000);
                //�Ѿ�ѧϰ�����ⴰĻ
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 6 <= i ) && ( i < NODE_CNT ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
                Specify_musi_play(39); 
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              
            }
          }
          
          break;
          //����������
        case gas:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
                Specify_musi_play( 26 );                   
                //�Ѿ�ѧϰ��ң����
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt * 3 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
                Specify_musi_play( 23 );                 
                //�Ѿ�ѧϰ���Ŵ�
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
                Specify_musi_play( 24 );    
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt *5 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
                Specify_musi_play( 25 ); 
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5<= i ) && ( i < NODE_CNT / dev_typ_cnt * 6) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
                  
                  Specify_musi_play( 8 );      
                  //�Ѿ�ѧϰ
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  //return;
                  return ;
                }else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  
                  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                  
                  
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
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��������ť!" );
                Specify_musi_play(39);            
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                return ;
              }
              
              
            }
          }
          
          break;
          //����������
        case arlrm_calling:
          for( i = 0; i < NODE_CNT; i++ )
          {
            copyi = i;
            if( pStudyStorDat->addr_data_433[i].Area == 0 )
            {
              continue;
            }
            if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
            {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
              if( i < ( NODE_CNT / dev_typ_cnt ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ң����!" );
                Specify_musi_play( 26 );    
                //�Ѿ�ѧϰ��ң����
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt <= i ) && ( i < NODE_CNT / dev_typ_cnt *3 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ���Ŵ�!" );
                Specify_musi_play( 23 );          
                //�Ѿ�ѧϰ���Ŵ�
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }else if( ( NODE_CNT / dev_typ_cnt *3 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 4 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ�����ⴰĻ!" );
                Specify_musi_play( 24 );        
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 4 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 5 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ����������!" );
                Specify_musi_play( 25 );   
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //	study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt * 5 <= i ) && ( i < NODE_CNT / dev_typ_cnt * 6 ) )
              {
                UART4_printf( "������ʾ:�Ѿ�ѧϰ��ȼ��������!" );
                Specify_musi_play( 40 );   
                //�Ѿ�ѧϰ�����ⴰĻ
                delay_ms(2000);
                i			   = NODE_CNT;                                                                                                      //��������
                Already_Study  = 1;
                //study_time	   = TickGet( );                                                                                                    //ʱ�����¸���
                
                //return;
                break;
              }
              else if( ( NODE_CNT / dev_typ_cnt *6 <= i ) && ( i < NODE_CNT ) )
              {
                if( ( pStudyStorDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyStorDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )
                {
                  UART4_printf( "������ʾ:�Ѿ�ѧϰ!" );
                  Specify_musi_play( 8 );      
                  //�Ѿ�ѧϰ
                  delay_ms(1000);
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  //	study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  //return;
                  break;
                }
                else
                {
                  pStudyStorDat->addr_data_433[i].data_433[0][0] = Receive[2];
                  pStudyStorDat->addr_data_433[i].data_433[0][1] = Receive[3];
                  pStudyStorDat->addr_data_433[i].data_433[0][2] = Receive[4];
                  pStudyStorDat->addr_data_433[i].Area		   = i + 1;                                                                     //�洢λ��
                  Save433StudyDat( pStudyStorDat );
                  i			   = NODE_CNT;                                                                                                  //��������
                  Already_Study  = 1;
                  study_time	   = TickGet( );                                                                                                //ʱ�����¸���
                  
                  
                  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
                  
                  
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
        //���豸ѧϰ
        //���豸ѧϰ
        if( StudyCntAddr == NODE_CNT )
        {
          UART4_printf( "������ʾ:�Ѿ��ﵽѧϰ20���豸���ֵ���޷�ѧϰ!" );
          Specify_musi_play( 10 ); //�洢��������ɾ�����ٴ�ѧϰ
          delay_ms(1000);
          return;
        }
        switch( sensor )
        {
        case remote:
          if(check_sensor_remote(Receive))
          {
            Save_433_flash(0,1,StudyCntAddr,sensor,typeinfo);
            study_time	           = TickGet( );                        //ʱ�����¸���
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
          study_time	           = TickGet( );                        //ʱ�����¸���
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case  Infrared:
          Save_433_flash(3,4,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //ʱ�����¸���
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case smoke:
          Save_433_flash(4,5,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //ʱ�����¸���
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case gas:
          Save_433_flash(5,6,StudyCntAddr,sensor,typeinfo);
          study_time	           = TickGet( );                        //ʱ�����¸���
          Already_Study             = 1;
          if(typeinfo)
          {
            get_back_study();
            return ;
          }  
          break;
        case arlrm_calling:
          Save_433_flash(6,7,StudyCntAddr,sensor,typeinfo);
          study_time	   = TickGet( );                        //ʱ�����¸���
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
  UART4_printf( "��ѧϰ��ʱ���˳�!" );
  
  Specify_musi_play( 0x09 ); //�˳�ѧϰ
  
  keyflag = K_CLR;
  
  Kmenu.Layer =1;
}


void get_back_study()
{
  Specify_musi_play( 0x09 ); //�˳�ѧϰ
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
      pStudyStorDat->addr_data_433[i].Area		   = i+1; //�洢λ��
      Save433StudyDat( pStudyStorDat );
      UART4_printf( "�洢�ɹ�!" );
      Specify_musi_play( 0x06 );                          //ѧϰ�ɹ�
      delay_ms(800);
      
      TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
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
        UART4_printf( "Other�Ѿ�����%d�������������ѧϰ!", ( NODE_CNT / dev_typ_cnt ) );
        Specify_musi_play( 10 );    //�洢��������ɾ�����ٴ�
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
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Delete_OneSensor( void )
{
  u8 temp[20];
  u32				del_time;
  uint8			StudyCntAddr;
  STOR_433_DATA	* pStudyStorDat;
  u8				i;
  //��Ҫ����һ���������洢��ǰ��⵽�ĸ��ִ�����ֵ��?
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
    for( i = 0; i < NODE_CNT; i++ )                                                                                                             //�ж�
    {
      if( pStudyStorDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == Receive[1] ) )    //��ַ�Ѿ�ѧϰ
      {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
        UART4_printf( "R[%d]=0x%x %x %x %x %x\r\n", i, Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
        memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );                                           //��������򴫸�����Ϣ
        memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
        StudyCntAddr--;
        if( StudyCntAddr > NODE_CNT )
        {
          StudyCntAddr = 0;
        }
        pStudyStorDat->Count_433 = StudyCntAddr;
        Save433StudyDat( pStudyStorDat );
        
        TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
        memcpy(temp,Receive,5);
        temp[5]=calendar.hour;
        temp[6]=calendar.min;
        temp[7]=calendar.sec;
        temp[8]= AlarmFlags.bIsRmt_Deploy;
        voice_delete_app(temp,i); //ɾ��������������Ϣ�ϱ�
        break;  //�˳�����
      }
      // printf("��δѧϰ\r\n");
    }//printf("��δѧϰ\r\n");
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
  
  memcpy(TempData,data,5); //Ĭ��5���ֽ�  
  
  TIME_Get( TIM2_tick );
  memcpy(temp,data,5);
  temp[5]=calendar.hour;
  temp[6]=calendar.min;
  temp[7]=calendar.sec;
  temp[8]= AlarmFlags.bIsRmt_Deploy; 
  
  for( i = 0; i < NODE_CNT; i++ )                                                                                                             //�ж�
  {
    if( pStudyStorDat->addr_data_433[i].Area == 0 )
    {
      continue;
    }
    
    if( ( pStudyStorDat->addr_data_433[i].addr_433[0] == TempData[0] ) && ( pStudyStorDat->addr_data_433[i].addr_433[1] == TempData[1] ) )    //��ַ�Ѿ�ѧϰ
    {                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
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
  Specify_musi_play(27);    //�Ѿ�ɾ��
  wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
  handly_get_value(1,2);
}

void voice_delete_app(u8 *temp,u8 i)
{
  if( i < 5 )
  {//���� ɾ������ң����
    Specify_musi_play( 11 );    //ɾ��ң����
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i < 15 )
  {					//���� ɾ�������Ŵ�
    Specify_musi_play( 12 );    //ɾ���ŴŴ���
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i < 20 )
  {//���� ɾ����������
    Specify_musi_play( 13 );    //ɾ�����ⴰĻ
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i <25)
  {//���� ɾ����������������
    Specify_musi_play( 14 );    //ɾ����������
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
  }
  else if( i <30)
  {//���� ɾ����������������
    Specify_musi_play( 36 );    //ɾ��ȼ��������
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
  else if( i <35)
  {//���� ɾ����������������
    Specify_musi_play( 32 );    //ɾ��������ť
    delay_ms(1500);
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,9,Delete_remote,GprsData[4]); 
    handly_get_value(1,2);
  }
}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void Delete_Sensor( u8 sensor )
{
  STOR_433_DATA	*pStudyStorDat;
  
  uint8			StudyCntAddr;
  u8 temp[20];
  u8		             i;
  pStudyStorDat = &StudyDat;
  StudyCntAddr = pStudyStorDat->Count_433;
  TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��                
  temp[0]=calendar.hour;
  temp[1]=calendar.min;
  temp[2]=calendar.sec;
  temp[3]= AlarmFlags.bIsRmt_Deploy;
  switch( sensor )
  {
  case remote:
    for( i = 0; i < ( NODE_CNT /dev_typ_cnt ); i++ )                    
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ� 
    wifi_sendcmd((BYTE *)temp,4,Delete_remote_all,0);                                                             //   Specify_Musi_Play(16);//ɾ���ɹ�
    break;
  case Menci:
    for( i = ( NODE_CNT /dev_typ_cnt ); i < ( NODE_CNT /dev_typ_cnt * 3 ); i++ )      //
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ�     
    wifi_sendcmd((BYTE *)temp,4,Delete_meci_all,0);                                                            //  Specify_Musi_Play(16);//ɾ���ɹ�
    break;
  case Infrared:
    for( i = ( NODE_CNT /dev_typ_cnt * 3 ); i < ( NODE_CNT /dev_typ_cnt * 4 ); i++ )  //
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ�
    wifi_sendcmd((BYTE *)temp,4,Delete_infrared_all,0);                                                          //     Specify_Musi_Play(16);//ɾ���ɹ�
    break;
  case smoke:
    for( i = ( NODE_CNT /dev_typ_cnt * 4 ); i < ( NODE_CNT/dev_typ_cnt*5); i++ )          //
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ�     
    wifi_sendcmd((BYTE *)temp,4,Delete_smoke_all,0);                                                            //    Specify_Musi_Play(16);//ɾ���ɹ�
    break;
  case gas:
    for( i = ( NODE_CNT / dev_typ_cnt * 5 ); i < ( NODE_CNT/dev_typ_cnt*6 ); i++ )          //
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ�   
    wifi_sendcmd((BYTE *)temp,4,Delete_gas_all,0);                                                            //    Specify_Musi_Play(16);//ɾ���ɹ�
    break;
  case arlrm_calling:
    for( i = ( NODE_CNT /dev_typ_cnt *6); i < ( NODE_CNT ); i++ )          //
    {//������������д�������Ϣ
      memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
      memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
      StudyCntAddr--;
    }
    Specify_musi_play( 16 );    //ɾ���ɹ� 
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
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
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
    //������������д�������Ϣ
    memset( &pStudyStorDat->addr_data_433[i].addr_433[0], 0x00, sizeof( RX_Data_433 ) );
    memset( &pStudyStorDat->ad_time_433[i].Syno_Time, 0x00, sizeof( TX_Time_433 ) );
  }
  
  StudyCntAddr			   = 0;
  pStudyStorDat->Count_433   = StudyCntAddr;
  Save433StudyDat( pStudyStorDat );
  UART4_printf( "ɾ�������Ѿ�ѧϰ������\r\n" );
  //Specify_Musi_Play( 15 );    //ɾ�����д�����
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
  
  if(AlarmFlags.bIsRmt_Deploy ==0)     //���ⱨ����Ϣ�ϴ������1 5����һ�� ����״̬��  
  {
    if((TickGet()-send_time)>1000*5*60){
      send_time= TickGet();
      wifi_sendcmd(temp,9,cmd,0); 
      handly_get_value(1,5); 
    } 
  }
  else if((TickGet()-send_time)>1000*2*60)        ////���ⱨ����Ϣ�ϴ������2 2����һ�� ����״̬�£�����Ϣģʽ��  
  {
    send_time= TickGet();
    wifi_sendcmd(temp,9,cmd,0); 
    handly_get_value(1,5); 
  }
}


/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
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
  case 1:                             //��������
    if( TickGet()-remote_time[0]>300)
    {
      remote_time[0] = TickGet( );
      Specify_musi_play( 18 );    //����
      AlarmFlags.bIsRmt_Deploy = 1;
      
      hand_send_alrm(datatemp,Bufang_remote);
      return ;
    }
    break;
  case 2:                         //��������
    if( TickGet( ) - remote_time[1] > 300 )
    {
      remote_time[1] = TickGet( );
      Specify_musi_play( 19 );	
      AlarmFlags.bIsRmt_Deploy   = 0;
      hand_send_alrm(datatemp,Chefang_remote);
      return ;
    }
    break;
  case 3:                              //������
    if( TickGet( ) - remote_time[2] > 300 )
    {
      remote_time[2] = TickGet();                 //��ȡϵͳʱ��
      AlarmFlags.bIsRmt_Deploy =2; 
      Specify_musi_play(53);  //�ػ�ģʽ
      hand_send_alrm(datatemp,Doorbell_remote);
      delay_ms(200); //ȷ��������������
      return ;
    }
    break;
  case 4:                              //������
    if( TickGet( ) - remote_time[2] > 300 )
    {
      remote_time[2] = TickGet();            
      Specify_Musi_Play(38);  //��������
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
    Specify_Musi_Play(voice_type); // ������
  }
  else if(AlarmFlags.bIsRmt_Deploy==2)
  {
    
    if((SendDat.Count_433!=0)&&(SendDat.Count_433!=16))
    {
      for(k=0;k<SendDat.Count_433;k++)
      {
        if((SendDat.send_data[k][1]==Receive[0] ) && ( SendDat.send_data[k][2] == Receive[1])
           &&(SendDat.send_data[k][3]==Receive[2] ) && (SendDat.send_data[k][4] == Receive[3])) //�Ƿ��ǹؼ�����                                           
          Specify_Musi_Play(voice_type); // ������
        break;
      }
    }
    else if(SendDat.Count_433==0&&(sensor==ALM_meci))
    {
      Specify_Musi_Play(voice_type); // ������
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

u8 cout,mencout;                        //���ڽ�����������\����� ��⵽�Ĵ���
int Changgui( void )
{
  
  u8 key_value=0;
  u8			temp[20];
  static u8		datatemp[N * 2];
  STOR_433_DATA	* pStudyDat;
  SEND_433_DATA	* pSendDat;
  u8				i, j,k;
  IWDG_Feed();
  pStudyDat = &StudyDat;   //��Ҫ����һ���������洢��ǰ��⵽�ĸ��ִ�����ֵ��?
  pSendDat = &SendDat;
  Jiema(changgui);
  for( i = 0; i < N; i++ )
  {
    datatemp[i] = Receive[i];
  }
  if( ( datatemp[0] != 0x00 ) && ( datatemp[1] != 0x00 )  && ( datatemp[2] != 0x00 ))
  {                                                                                                                                                                                                                       //����ѧϰ�����ͬ��ѧϰ�ɹ�
    
    UART4_printf( "R=0x%x %x %x %x %x\r\n", Receive[0], Receive[1], Receive[2], Receive[3], Receive[4] );
    
    check_guanling();
    for( i = 0; i < ( NODE_CNT / dev_typ_cnt ); i++ )                                                                                                                                                                             //�ж�ң����
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;//��λ��û�д洢������
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                                                                               //��ѧϰ����ͬ��ַ�Ĵ�����
	for( j = 0; j < 4; j++ )
	{
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ))                                 //�洢�а���ֵ
          {
            if( ( pStudyDat->addr_data_433[i].data_433[j][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[j][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[j][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
            {
              break; //������ֵ����
            }
          }
          else //�洢�ð���
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
      } // end   if  ѧϰ��
    }
    
    for( i = ( NODE_CNT / dev_typ_cnt ); i < ( NODE_CNT /dev_typ_cnt * 3); i++ )                                                                                                                                                      //�ж��ŴŴ���
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      
      if(  (pStudyDat->addr_data_433[i].addr_433[0] == Receive[0])  && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                //��ѧϰ����ͬ��ַ�Ĵ�����
        if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] )
           && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] )
             && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
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
    
    for( i = ( NODE_CNT /dev_typ_cnt * 3 ); i < ( NODE_CNT /dev_typ_cnt * 4); i++ ) //�жϺ���Ļ��
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                            //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                                                                       //��ѧϰ����ͬ��ַ�Ĵ�����
        if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
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
    
    for( i = ( NODE_CNT /dev_typ_cnt * 4); i < (NODE_CNT /dev_typ_cnt * 5); i++ ) //�ж�����������
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
	continue;
      }
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                                                                               //��ѧϰ����ͬ��ַ�Ĵ�����
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //�洢�а���ֵ
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
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
          Specify_Musi_Play(22);//��������
          hand_send_alrm(&datatemp[0],ALM_smoke);
          //return 0;					
	}
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 5 ); i < (NODE_CNT/dev_typ_cnt*6); i++ ) //�ж�����������
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      // break;
      
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                                                                               //��ѧϰ����ͬ��ַ�Ĵ�����
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //�洢�а���ֵ
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
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
          Specify_Musi_Play(30);//ȼ��������
          hand_send_alrm(&datatemp[0],ALM_gas);
          //    return 0;
          
        }
      }
    }
    
    for( i = ( NODE_CNT /dev_typ_cnt * 6); i < (NODE_CNT); i++ ) //�ж�����������
    {
      if( pStudyDat->addr_data_433[i].Area == 0 )
      {
        continue;
      }
      // break;
      
      if( ( pStudyDat->addr_data_433[i].addr_433[0] == Receive[0] ) && ( pStudyDat->addr_data_433[i].addr_433[1] == Receive[1] ) )                                                                                    //��ַ�Ѿ�ѧϰ
      {                                                                                                                                                                                                               //��ѧϰ����ͬ��ַ�Ĵ�����
        for( j = 0; j < 4; j++ )
        {
          if( ( pStudyDat->addr_data_433[i].data_433[j][0] != 0 ) && ( pStudyDat->addr_data_433[i].data_433[j][1] != 0 ) || ( pStudyDat->addr_data_433[i].data_433[j][2] != 0 ) )                                 //�洢�а���ֵ
          {
            if( ( pStudyDat->addr_data_433[i].data_433[0][0] == Receive[2] ) && ( pStudyDat->addr_data_433[i].data_433[0][1] == Receive[3] ) && ( pStudyDat->addr_data_433[i].data_433[0][2] == Receive[4] ) )  //�Ѿ��洢�ð�����Ϣ
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
          Specify_Musi_Play(29);//������ť
          hand_send_alrm(&datatemp[0],ALM_key);
          //  return 0; 
        }
      }
    }
    
  }   //end if  ���μ��ֵ��ͬ
  //end if  ��⵽ֵ������
}

/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
u16 GenerateRandomU16( void ) //����adc�������������
{
  u16 tmp	   = 0;
  u16 mask   = 0x8000;
  
  do
  {
    while( ADC_GetFlagStatus( ADC2, ADC_FLAG_EOC ) == RESET )
    {
      ;                                                           //�ȴ�ADCת���
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
    cl_out(&DataTemp[1],1000,0,0x2C,0X54);//���ƴ��� //984
    
    //   delay_ms(10);
    //    }
    Set_Ask_RxMode();
  }
  else if(DataTemp[0]==0x20)
  {
    Set_Ask_TxMode();
    //    printf("d1=%x,d2=%x,d3=%x,d4=%x,d5=%x\r\n",DataTemp[1],DataTemp[2],DataTemp[3],DataTemp[4],DataTemp[5]);
    //  for(i=0;i<3;i++){
    
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],8);//���Ʋ��������ص�
    delay_ms(400);
    //  }
    Set_Ask_RxMode();
  }
  else if(DataTemp[0]==0x25)
  {
    Set_Ask_TxMode();
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],30);//���Ʋ��������ص�
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
    fashe_out(&DataTemp[1],s_t,f_t,DataTemp[10],DataTemp[11],8);//���Ʋ��������ص�
    
    //  }
    Set_Ask_RxMode();
  }
  
}                



void hand_save_433(u8 dev_typ){
  
  uint8			k_value = 0;
  //static u8		k2 = 0;
  u8 temp[30];
  
  uint8			DataTemp[N*2]; //����ѧϰ����jiama()ֵ���Աȣ�һ�²�ѧϰ�ɹ���
  SEND_433_DATA	*pSendStorDat;
  uint8			SendCntAddr;
  uint8			SendTemp[2 * N]; //����ѧϰ����jiama()ֵ���Աȣ�һ�²�ѧϰ�ɹ���
  uint8			i, j;
  u32				send_time	   = 0;
  u32				study_time	   = 0;     
  u8				Already_Study = 0;
  pSendStorDat = &SendDat;
  study_time	   = TickGet( );
  
  while( ( TickGet( ) > ( study_time + 5000 ) ) ? 0 : 1 ) //8�볬ʱ�˳�
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
        Specify_musi_play(43);             //ƥ��ɹ�
        delay_ms(500);
        TIME_Get( TIM2_tick );                  //��ȡϵͳʱ��
        
        temp[0]=dev_typ;
        memcpy(&temp[1],Receive,5);
        temp[6]=SYNO_T;
        temp[7]=(SYNO_T>>8);
        temp[8]=FIRST_LT;
        temp[9]=(FIRST_LT>>8);
        temp[10]=LOW_T;  //440us   �͵�ƽʱ��
        temp[11]=HIGH_T;  //440us   �͵�ƽʱ��
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
  //��ȡCPUΨһID
  CpuID[0]   = *(u32*)( 0x1ffff7e8 );
  CpuID[1]   = *(u32*)( 0x1ffff7ec );
  CpuID[2]   = *(u32*)( 0x1ffff7f0 );
  printf( "CPUID1=%x,CPUID2=%x,CPUID3=%x\r\n", CpuID[0], CpuID[1], CpuID[2] );
  //�ܼ򵥵��㷨
  tempID = CpuID[0] + CpuID[1] + CpuID[2];
  printf( "ID=%d\r\n", tempID );
  printf( "ID=%X\r\n", tempID );
  
  *pdeviceID = (u8)tempID;
  pdeviceID++;
  *pdeviceID = (u8)( tempID >> 8 );
  pdeviceID++;                                                                                              //���ڲ���32λID
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
  //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
  ADC_RegularChannelConfig( ADC1, 10, 1, ADC_SampleTime_239Cycles5 ); //ADC1,ADCͨ��,����ʱ��Ϊ239.5����
  
  ADC_SoftwareStartConvCmd( ADC1, ENABLE );                           //ʹ��ָ����ADC1�����ת����������
  
  while( !ADC_GetFlagStatus( ADC1, ADC_FLAG_EOC ) )
  {
    ;                                                               //�ȴ�ת������
  }
  return ADC_GetConversionValue( ADC1 );                              //�������һ��ADC1�������ת�����
}

/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
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
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
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
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/

/************************************** The End Of File **************************************/
