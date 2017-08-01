#include "KEY_LED.H"
#include "delay.h"
#include "Usart.h"
#include "Detector.h"
#include "tick.h"
#include "jq6500.h"
#include "wifi.h"

u8 keyflag = K_CLR;// K_CLR K_SET  K_ENC
KeyMenu_TypeDef Kmenu;

//������ʼ������ 
//PC4��PC5 ���óɰ�������
//PB0   LED1  ���  0��led����1��led��
//PB1   LED2  ���  0��led����1��led��
//PA0   LED3  ���  0��led����1��led��
//PA1   LED4  ���  0��led����1��led��
void KEY_LED_Init(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO,ENABLE);//ʹ��PORTA,PORTB,PORTCʱ��RCC_APB2Periph_AFIO
  //������ʼ��
  
  
  
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1;//PA0��PA1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_ResetBits(GPIOB,GPIO_Pin_0);//����� LED off
 GPIO_SetBits(GPIOB,GPIO_Pin_1);//����� LED off
  
  // GPIO_ResetBits(GPIOB,GPIO_Pin_0);//����� LED off
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;//PA0��PA1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  //GPIO_SetBits(GPIOC,GPIO_Pin_4);//����� LED off
  
  
  GPIO_InitStructure.GPIO_Pin  =  GPIO_Pin_5;//PC4��PC5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  
  //�����жϷ���
  EXTI_Init(&EXTI_InitStructure);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource5);
  EXTI_InitStructure.EXTI_Line	  = EXTI_Line5;  
  EXTI_InitStructure.EXTI_Mode	  = EXTI_Mode_Interrupt;  
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;   
  EXTI_Init(&EXTI_InitStructure);
  
  //�жϷ���
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	 
  NVIC_InitStructure.NVIC_IRQChannelCmd  = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	 
  
  
  //������ʼ��
  keyflag = K_CLR;
} 


void KeyProcess(void)
{  
  u32 keyprocess_time;
  u32 stay_time=0;
  u8  del_flag=1;
  if(keyflag)
  {
  keyprocess_time = TickGet();
 stay_time=TickGet();
   while(KEY1_SET==0)
   {
      IWDG_Feed();

      if((TickGet()-stay_time)>5000)
      {
        smart_link();
       //  delay_ms(500);
        Wifi_udp_connect();
        keyflag=K_CLR;
        return;
      }
    }
  }                
}


