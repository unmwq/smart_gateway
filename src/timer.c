#include "timer.h"
#include "KEY_LED.h"


/**************************************************************************** 
* 
* ����:  RCC_Configuration 
* 
* ����:  ����ϵͳʱ�� 
* 
* ����:  �� 
* 
****************************************************************************/  
void RCC_Configuration(void)  
{  
  ErrorStatus HSEStartUpStatus;     
  
  RCC_DeInit();                       //RCC��λ   
  RCC_HSEConfig(RCC_HSE_ON);              //ʹ���ⲿʱ��    
  HSEStartUpStatus = RCC_WaitForHSEStartUp();     //�ȴ��ⲿʱ�Ӿ���   
  
  if(HSEStartUpStatus == SUCCESS)  
  {  
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);   //Ԥȡַ����ʹ��   
    FLASH_SetLatency(FLASH_Latency_2);          //FLASH�洢���ӳ�2����   
    RCC_HCLKConfig(RCC_SYSCLK_Div1);            //����AHBʱ��Ϊϵͳʱ��   
    RCC_PCLK2Config(RCC_HCLK_Div1);             //���ø���ʱ��ΪAHBʱ��    
    RCC_PCLK1Config(RCC_HCLK_Div2);             //���õ���ʱ��ΪAHBʱ��2��Ƶ   
    
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);    //����PLLΪ72M   
    RCC_PLLCmd(ENABLE);                 //ʹ��PLL   
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)  //�ȴ�PLL   
    {  
    }  
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);      //����PLLΪϵͳʱ��   
    while(RCC_GetSYSCLKSource() != 0x08)            //�ȴ��������   
    {  
    }  
  }  
}  



//ͨ�ö�ʱ���жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//����ʹ�õ��Ƕ�ʱ��4!
//frq=72000000/psc/arr hz
//TIM4_Int_Init(65535,100);

void TIM5_Int_Init(u16 arr,u16 psc)//����433ѧϰ
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  
  TIM_DeInit(TIM5);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //ʱ��ʹ��
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	 ������5000Ϊ45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  10Khz�ļ���Ƶ��  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
  //TIM_ITConfig(TIM3,TIM_CounterMode_Up,ENABLE);
  
  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  TIM_Cmd(TIM5, ENABLE);	//ʹ��TIMx����
}

//psc/72 000 000 Ϊÿ����һ�ε�ʱ������5000 651
void TIM3_Int_Init(u16 arr,u16 psc)//����433ѧϰ
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  //NVIC_InitTypeDef NVIC_InitStructure;
  /* NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //ռ�����ȼ��������ȼ�����Դ����
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  
  NVIC_Init(&NVIC_InitStructure);*/
  
  TIM_DeInit(TIM3);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	 ������5000Ϊ45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  10Khz�ļ���Ƶ��  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
  //TIM_ITConfig(TIM3,TIM_CounterMode_Up,ENABLE);
  
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  TIM_Cmd(TIM3, ENABLE);	//ʹ��TIMx����
}
/*
void pwm_ioAF(void)
{
GPIO_InitTypeDef GPIO_InitStructure;

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; 
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//
GPIO_Init(GPIOA, &GPIO_InitStructure);

//GPIO_SetBits(GPIOA,GPIO_Pin_8);//����ߵ�ƽ
}
void pwm_ioOUT(void)
{
GPIO_InitTypeDef GPIO_InitStructure;

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; 
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//
GPIO_Init(GPIOA, &GPIO_InitStructure);

GPIO_ResetBits(GPIOA,GPIO_Pin_8);//����͵�ƽ
}
*/
void TIM4_Int_Init(u16 arr,u16 psc)//���ڳ��������
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  TIM_DeInit(TIM4);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //ʱ��ʹ��
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	 ������5000Ϊ45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  10Khz�ļ���Ƶ��  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
  //TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  
  
  TIM_Cmd(TIM4, ENABLE);	//ʹ��TIMx����
}

void GPIO_IOOUT(void)  
{ 
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  /* GPIOA and GPIOB clock enable */ 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);  
  
  /*GPIOA Configuration: TIM3 channel 1 and 2 as alternate function push-pull */ 
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           // ����������� 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
  GPIO_ResetBits(GPIOA,GPIO_Pin_11);//����͵�ƽ
} 

void GPIO_Config(void)  
{ 
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  /* GPIOA and GPIOB clock enable */ 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO,ENABLE);  
  
  /*GPIOA Configuration: TIM3 channel 1 and 2 as alternate function push-pull */ 
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;           // ����������� 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
} 
void TIM1_Config(void)  
{  
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);// 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);  //ʹ��GPIO����ʱ��ʹ��
  
  
  //���ø�����Ϊ�����������,���TIM1 CH1��PWM���岨��
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; //TIM_CH1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //�����������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  TIM_TimeBaseStructure.TIM_Period = 2000; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	 80K
  TIM_TimeBaseStructure.TIM_Prescaler =0; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  ����Ƶ
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
  TIM_OCInitStructure.TIM_Pulse = 1500; //���ô�װ�벶��ȽϼĴ���������ֵ
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
  TIM_OC4Init(TIM1, &TIM_OCInitStructure);  //����TIM_OCInitStruct��ָ���Ĳ�����ʼ������TIMx
  
  TIM_CtrlPWMOutputs(TIM1,ENABLE);	//MOE �����ʹ��	
  
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);  //CH1Ԥװ��ʹ��	 
  
  TIM_ARRPreloadConfig(TIM1, ENABLE); //ʹ��TIMx��ARR�ϵ�Ԥװ�ؼĴ���
  
  TIM_Cmd(TIM1, DISABLE);  //ʹ��TIM1 
} 



void TIM3_IRQHandler(void)   //TIM3�ж�
{
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //���ָ����TIM�жϷ������:TIM �ж�Դ 
  {
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //���TIMx���жϴ�����λ:TIM �ж�Դ 
  }
}
