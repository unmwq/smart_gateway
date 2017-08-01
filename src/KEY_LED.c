#include "KEY_LED.H"
#include "delay.h"
#include "Usart.h"
#include "Detector.h"
#include "tick.h"
#include "jq6500.h"
#include "wifi.h"

u8 keyflag = K_CLR;// K_CLR K_SET  K_ENC
KeyMenu_TypeDef Kmenu;

//按键初始化函数 
//PC4和PC5 设置成按键输入
//PB0   LED1  输出  0，led亮；1，led灭
//PB1   LED2  输出  0，led亮；1，led灭
//PA0   LED3  输出  0，led亮；1，led灭
//PA1   LED4  输出  0，led亮；1，led灭
void KEY_LED_Init(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO,ENABLE);//使能PORTA,PORTB,PORTC时钟RCC_APB2Periph_AFIO
  //按键初始化
  
  
  
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1;//PA0、PA1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_ResetBits(GPIOB,GPIO_Pin_0);//输出高 LED off
 GPIO_SetBits(GPIOB,GPIO_Pin_1);//输出高 LED off
  
  // GPIO_ResetBits(GPIOB,GPIO_Pin_0);//输出高 LED off
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;//PA0、PA1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  //GPIO_SetBits(GPIOC,GPIO_Pin_4);//输出高 LED off
  
  
  GPIO_InitStructure.GPIO_Pin  =  GPIO_Pin_5;//PC4、PC5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  
  //按键中断分配
  EXTI_Init(&EXTI_InitStructure);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource5);
  EXTI_InitStructure.EXTI_Line	  = EXTI_Line5;  
  EXTI_InitStructure.EXTI_Mode	  = EXTI_Mode_Interrupt;  
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;   
  EXTI_Init(&EXTI_InitStructure);
  
  //中断分配
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	 
  NVIC_InitStructure.NVIC_IRQChannelCmd  = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	 
  
  
  //变量初始化
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


