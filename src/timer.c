#include "timer.h"
#include "KEY_LED.h"


/**************************************************************************** 
* 
* 名称:  RCC_Configuration 
* 
* 描述:  设置系统时钟 
* 
* 返回:  无 
* 
****************************************************************************/  
void RCC_Configuration(void)  
{  
  ErrorStatus HSEStartUpStatus;     
  
  RCC_DeInit();                       //RCC复位   
  RCC_HSEConfig(RCC_HSE_ON);              //使能外部时钟    
  HSEStartUpStatus = RCC_WaitForHSEStartUp();     //等待外部时钟就绪   
  
  if(HSEStartUpStatus == SUCCESS)  
  {  
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);   //预取址缓存使能   
    FLASH_SetLatency(FLASH_Latency_2);          //FLASH存储器延迟2周期   
    RCC_HCLKConfig(RCC_SYSCLK_Div1);            //设置AHB时钟为系统时钟   
    RCC_PCLK2Config(RCC_HCLK_Div1);             //设置高速时钟为AHB时钟    
    RCC_PCLK1Config(RCC_HCLK_Div2);             //设置低速时钟为AHB时钟2分频   
    
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);    //设置PLL为72M   
    RCC_PLLCmd(ENABLE);                 //使能PLL   
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)  //等待PLL   
    {  
    }  
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);      //设置PLL为系统时钟   
    while(RCC_GetSYSCLKSource() != 0x08)            //等待设置完成   
    {  
    }  
  }  
}  



//通用定时器中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器4!
//frq=72000000/psc/arr hz
//TIM4_Int_Init(65535,100);

void TIM5_Int_Init(u16 arr,u16 psc)//用于433学习
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  
  TIM_DeInit(TIM5);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //时钟使能
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
  //TIM_ITConfig(TIM3,TIM_CounterMode_Up,ENABLE);
  
  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  TIM_Cmd(TIM5, ENABLE);	//使能TIMx外设
}

//psc/72 000 000 为每计数一次的时间间隔。5000 651
void TIM3_Int_Init(u16 arr,u16 psc)//用于433学习
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  //NVIC_InitTypeDef NVIC_InitStructure;
  /* NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //占先优先级、副优先级的资源分配
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  
  NVIC_Init(&NVIC_InitStructure);*/
  
  TIM_DeInit(TIM3);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
  //TIM_ITConfig(TIM3,TIM_CounterMode_Up,ENABLE);
  
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  TIM_Cmd(TIM3, ENABLE);	//使能TIMx外设
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

//GPIO_SetBits(GPIOA,GPIO_Pin_8);//输出高电平
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

GPIO_ResetBits(GPIOA,GPIO_Pin_8);//输出低电平
}
*/
void TIM4_Int_Init(u16 arr,u16 psc)//用于超声波测距
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  TIM_DeInit(TIM4);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能
  
  
  TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为45.20ms
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
  //TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  
  
  TIM_Cmd(TIM4, ENABLE);	//使能TIMx外设
}

void GPIO_IOOUT(void)  
{ 
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  /* GPIOA and GPIOB clock enable */ 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);  
  
  /*GPIOA Configuration: TIM3 channel 1 and 2 as alternate function push-pull */ 
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           // 复用推挽输出 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
  GPIO_ResetBits(GPIOA,GPIO_Pin_11);//输出低电平
} 

void GPIO_Config(void)  
{ 
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  /* GPIOA and GPIOB clock enable */ 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO,ENABLE);  
  
  /*GPIOA Configuration: TIM3 channel 1 and 2 as alternate function push-pull */ 
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;           // 复用推挽输出 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
} 
void TIM1_Config(void)  
{  
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);// 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);  //使能GPIO外设时钟使能
  
  
  //设置该引脚为复用输出功能,输出TIM1 CH1的PWM脉冲波形
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; //TIM_CH1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  TIM_TimeBaseStructure.TIM_Period = 2000; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 80K
  TIM_TimeBaseStructure.TIM_Prescaler =0; //设置用来作为TIMx时钟频率除数的预分频值  不分频
  TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
  TIM_OCInitStructure.TIM_Pulse = 1500; //设置待装入捕获比较寄存器的脉冲值
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
  TIM_OC4Init(TIM1, &TIM_OCInitStructure);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
  
  TIM_CtrlPWMOutputs(TIM1,ENABLE);	//MOE 主输出使能	
  
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);  //CH1预装载使能	 
  
  TIM_ARRPreloadConfig(TIM1, ENABLE); //使能TIMx在ARR上的预装载寄存器
  
  TIM_Cmd(TIM1, DISABLE);  //使能TIM1 
} 



void TIM3_IRQHandler(void)   //TIM3中断
{
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
  {
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx的中断待处理位:TIM 中断源 
  }
}
