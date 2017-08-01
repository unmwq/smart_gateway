#include "usart.h"	  
#include <stdarg.h>
#include "JQ6500.h"
#include "Tick.h"
#include "sys.h"

#include <stdarg.h>
#include "delay.h"
#include "timer.h"
#include "rtc.h"
#include "string.h"
#include <stdlib.h>
#include "detector.h"

#include "wifi.h"
#include "main.h"
extern Alarm_Flags AlarmFlags;

#define CMD_BUFFER_LEN 300

int zp;
//char sdata[300];
char rpdata[50];

u32  TotalItemNum = 0 ;//用于记录总的电话号码条目数

u32  TotalNum = 0 ;//用于记录缓存接收到的总条目数
u32  memItemNum = 0;

u8*	R1Fifo_Start;
u8*	R1Fifo_End;
u8*	R1Fifo_Read;
u8*	R1Fifo_Write;

#define DISABLE_TXRXINT (CPU_IntDis())
#define ENABLE_TXRXINT (CPU_IntEn())

char    UARTR1Buf[400];    // Receive FIFO buffer


int zj=0;
int zk=0;
#define  OK    "OK"
#define  ERROR    "ERROR"
#define  CONNECT "CONNECT"

u32 set_vol_level();

//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  

int fputc(int ch, FILE *f)
{
  UART4->DR = (u8) ch; 
  // Loop until the end of transmission //
  while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  return ch;
}

#if 1
void UART4_printf (u8 *fmt, ...)
{
  u8 buffer[CMD_BUFFER_LEN+1];  // CMD_BUFFER_LEN长度自己定义吧
  u8 i = 0;
  va_list arg_ptr;//初始化指向可变参数列表的指针
  va_start(arg_ptr, fmt);  //将第一个可变参数的地址付给arg_ptr，即arg_ptr指向可变参数列表的开始   
  //vsnprintf(buffer, CMD_BUFFER_LEN+1, fmt, arg_ptr);
  vsprintf(buffer,fmt,arg_ptr);
  
  while ((i < CMD_BUFFER_LEN) && buffer[i])
  {
    USART_SendData(UART4, (u8) buffer[i++]);
    while (USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET); 
  }
  va_end(arg_ptr);
}

#endif


void USART5_printf (u8 *fmt, ...)
{
  u8 buffer[CMD_BUFFER_LEN+1];  // CMD_BUFFER_LEN长度自己定义吧
  u8 i = 0;
  va_list arg_ptr;
  va_start(arg_ptr, fmt);
  //vsnprintf(buffer, CMD_BUFFER_LEN+1, fmt, arg_ptr);
  vsprintf(buffer,fmt,arg_ptr);
  
  while ((i < CMD_BUFFER_LEN) && buffer[i])
  {
    USART_SendData(UART5, (u8) buffer[i++]);
    while (USART_GetFlagStatus(UART5, USART_FLAG_TC) == RESET); 
  }
  va_end(arg_ptr);
}


void UART4_send_string(u8 *buf,u8 len)
{	u8 i;

for(i=0;i<len;i++)
{	 
  while (USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
  USART_SendData(UART4, (u16)buf[i]);
  
}
}
void UART5_send_string(u8 *buf,u8 len)
{	u8 i;
for(i=0;i<len;i++)
{	
  while (USART_GetFlagStatus(UART5, USART_FLAG_TC) == RESET); 
  USART_SendData(UART5, (u16)buf[i]);
  
}
}



void Uart1_Init(u32 bound)//GPRS 模块 
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USART_DeInit(USART1);  //复位串口2
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//GPIOA时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART2
  
  
  
  //USART1_TX	 PA.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.2
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
  
  //USART1_RX	PA.3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3
  
  
  //Usart1 NVIC 配置
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //占先优先级、副优先级的资源分配
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;		//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
  //USART 初始化设置
  
  USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
  USART_Init(USART1, &USART_InitStructure); //初始化串口
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
  USART_Cmd(USART1, ENABLE);                    //使能串口 
  
}

void Uart2_Init(u32 bound)//GPRS 模块 
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USART_DeInit(USART2);  //复位串口2
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);	//GPIOA时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//使能USART2
  
  
  
  //USART1_TX	 PA.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
  
  //USART1_RX	PA.3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3
  
  
  //Usart1 NVIC 配置
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //占先优先级、副优先级的资源分配
  
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
  //USART 初始化设置
  
  USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
  USART_Init(USART2, &USART_InitStructure); //初始化串口
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
  USART_Cmd(USART2, ENABLE);                    //使能串口 
  
}



//初始化IO 串口3 
//bound:波特率
//初始化IO 串口3 
//bound:波特率
void Uart3_Init(u32 bound)////串口摄像头
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  USART_DeInit(USART3);  //复位串口1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);	//GPIOB时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3
  
  
  
  //USART3_TX   PB.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化PB.10
  
  //USART3_RX	  PB.11
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOB, &GPIO_InitStructure);  //初始化PB.11
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  //摄像头电源开关 备注下PC9
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//浮空输入
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  GPIO_SetBits(GPIOC,GPIO_Pin_9);
  
  
  //Usart3 NVIC 配置
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //占先优先级、副优先级的资源分配
  
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级1
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器 
  
  //USART 初始化设置
  USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
  USART_Init(USART3, &USART_InitStructure); //初始化串口
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启中断
  USART_Cmd(USART3, ENABLE);                    //使能串口 
}


void Uart4_Init(u32 bound)//GPRS 模块 
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//GPIOC时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//使能USART4
  
  USART_DeInit(UART4);  //复位串口4
  
  //USART4_TX	 PC.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_Init(GPIOC, &GPIO_InitStructure); 
  //USART4_RX	PC.11
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  
  
  //Usart NVIC 配置
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //占先优先级、副优先级的资源分配
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;		//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
  //USART 初始化设置
  
  USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
  
  USART_Init(UART4, &USART_InitStructure); //初始化串口
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启中断
  USART_Cmd(UART4, ENABLE);                    //使能串口 
  //485收发控制管脚
  //  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 
  //  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  //  GPIO_Init(GPIOC, &GPIO_InitStructure);  
}
void Uart5_Init(u32 bound)//语音模块控制
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOD, ENABLE);	//GPIO时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);//使能USART5
  
  USART_DeInit(UART5);  //复位串口5
  
  //USART5_TX	 PC.12
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_Init(GPIOC, &GPIO_InitStructure); 
  
  //USART5_RX	PD.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOD, &GPIO_InitStructure);  
  
  
  
  //USART 初始化设置
  
  USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
  
  
  
  //Usart1 NVIC 配置
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //占先优先级、副优先级的资源分配
  
  NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;		  //子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 		  //IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);   //根据指定的参数初始化VIC寄存器
  
  USART_Init(UART5, &USART_InitStructure); //初始化串口
  USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//开启中断
  USART_Cmd(UART5, ENABLE);                    //使能串口 
  
}

void AllUart_Init(void)
{
  Uart1_Init(115200);	//wifi
  Uart2_Init(9600);	//语音模块
  //Uart3_Init(38400);	// 移动侦测摄像头
  //  Uart3_Init(115200);	// 移动侦测摄像头
  Uart4_Init(115200);	// to PC
  //  Uart5_Init(9600);	// 语音模块
  
}

//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	

//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
//初始化IO 串口1 
//bound:波特率

/******************************************************************
- 功能描述：将一个32位的变量dat转为字符串，比如把1234转为"1234"
- 隶属模块：公开函数模块
- 函数属性：外部，用户可调用
- 参数说明：dat:带转的long型的变量
str:指向字符数组的指针，转换后的字节串放在其中           
- 返回说明：无
******************************************************************/
void u32tostr(unsigned long dat,char *str) 
{
  char temp[20];
  unsigned char i=0,j=0;
  i=0;
  while(dat)
  {
    temp[i]=dat%10+0x30;
    i++;
    dat/=10;
  }
  j=i;
  for(i=0;i<j;i++)
  {
    str[i]=temp[j-i-1];
  }
  if(!i) {str[i++]='0';}
  str[i]=0;
}

/******************************************************************
- 功能描述：将一个字符串转为32位的变量，比如"1234"转为1234
- 隶属模块：公开函数模块
- 函数属性：外部，用户可调用
- 参数说明：str:指向待转换的字符串           
- 返回说明：转换后的数值
******************************************************************/

unsigned long strtou32(char *str) 
{
  unsigned long temp=0;
  unsigned long fact=1;
  unsigned char len=strlen(str);
  unsigned char i;
  for(i=len;i>0;i--)
  {
    temp+=((str[i-1]-0x30)*fact);
    fact*=10;
  }
  return temp;
}

/**************************************************************************
- 功能描述：串口发送数值
- 隶属模块：串口操作
- 函数属性：外部，使用户使用
- 参数说明：dat:要发送的数值
- 返回说明：无
- 注：函数中会将数值转为相应的字符串，发送出去。比如 4567 转为 "4567" 
**************************************************************************/



//--------------------------------------------------------------------------
void UART4_RecData(){
  
  u8 Res;
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
  {
    USART_ClearITPendingBit(UART4,USART_IT_RXNE);  //清中断标志？？？？有问题！！！
    
    Res =USART_ReceiveData(UART4);//(USART1->DR);	//读取接收到的数据
    //UART3_SendByte(Res);
    
    rpdata[zk]=Res;
    if(zk<99) zk++;
    else zk=0;
  }
  if(USART_GetFlagStatus(UART4,USART_FLAG_ORE)==SET) 
  { 
    USART_ClearFlag(UART4,USART_FLAG_ORE); 
    Res =USART_ReceiveData(UART4);		//? SR 
    //UART3_SendByte(Res); 						//? DR 
    rpdata[zk]=Res;
    if(zk<99) zk++;
    else zk=0;
  } 
  
}


/*
void Usart3_RecData(){

u8 Res;
if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
{
USART_ClearITPendingBit(USART3,USART_IT_RXNE);
Res=USART_ReceiveData(USART3);
//UART4_SendByte(Res);
//  Res = USART2->DR;
*R3Fifo_Write++ = Res;
// check if it is the bottom of FIFO
if( R3Fifo_Write >= R3Fifo_End )
{
R3Fifo_Write = R3Fifo_Start;
	}		
  }

if(USART_GetFlagStatus(USART3,USART_FLAG_ORE)==SET) 
{ 
USART_ClearFlag(USART3,USART_FLAG_ORE);  //? SR 
Res=USART_ReceiveData(USART3);		//? DR 
//  UART4_SendByte(Res);
*R3Fifo_Write++ = Res;

if( R3Fifo_Write >= R3Fifo_End )
{
R3Fifo_Write = R3Fifo_Start;
	}		
  }

}
*/


//---------------------------------------------------------------------------------------
void Usart2_RecData(){
  
  u8 Res;
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
  {
    USART_ClearITPendingBit(USART2,USART_IT_RXNE);
    Res=USART_ReceiveData(USART2);
    UART4_SendByte(Res);
    
    //----------------
    //sdata[zj]=Res;
    //  if(zj<299)  zj++;
    //else zj=0;
  }
  
  if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET) 
  { 
    USART_ClearFlag(USART2,USART_FLAG_ORE);  //? SR 
    Res=USART_ReceiveData(USART2);		//? DR 
    UART4_SendByte(Res);   
    //-------------------------	
    //sdata[zj]=Res;	
    //if(zj<299)  zj++;
    //else zj=0;
  }
  
}

//--------------------------------------------------------------------------------------------------------

void Usart1_RecData(){
  
  u8 Res;
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
  {
    USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    Res=USART_ReceiveData(USART1);
    //  Res = USART2->DR;
    *R1Fifo_Write++ = Res;
    UART4_SendByte(Res);
    // check if it is the bottom of FIFO
    if( R1Fifo_Write >= R1Fifo_End )
    {
      R1Fifo_Write = R1Fifo_Start;
    }		
  }
  
  if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET) 
  { 
    USART_ClearFlag(USART1,USART_FLAG_ORE);  //? SR 
    Res=USART_ReceiveData(USART1);		//? DR 
    
    *R1Fifo_Write++ = Res;
    UART4_SendByte(Res);  
    // check if it is the bottom of FIFO
    if( R1Fifo_Write >= R1Fifo_End )
    {
      R1Fifo_Write = R1Fifo_Start;
    }
    
  }
  
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

void UART3_SendByte(char dat)
{
  USART3->DR=dat;	
  
  while (USART_GetFlagStatus(USART3,USART_FLAG_TXE) == RESET);
  
}


void UART3_SendData(char *buf,u8 Len)   //发数组 
{          	
  int t=0;
  for(t=0;t<Len;t++)		
  {
    UART3_SendByte(buf[t]);
    
  }
}

void UART3_Send_Str(char *str)   //发字符串
{ 
  while(*str !='\0')
  {
    UART3_SendByte(*str++);
  }
}


void UART4_SendByte(char dat)
{
  
  //USART_ClearFlag(UART4,USART_FLAG_TC); 
  //USART_GetFlagStatus(UART4, USART_FLAG_TC);// 第一个数据丢失问题 先读sr 再写DR
  UART4->DR=dat;	// ??????
  //while((UART4->SR&0X40)==0);//等待发送结束
  while (USART_GetFlagStatus(UART4,USART_FLAG_TXE) == RESET);
  
  /*****************************************************************
  TC 是发送完成标志，TXE是数据发送至移位寄存器的标志，用法略有区别，至于
  while (USART_GetFlagStatus(usart,USART_FLAG_TXE) == RESET); 
  USART_SendData(usart,data);
  这个while语句在前还是在后，个人认为，ST原本目的是为了照顾引脚复用的情况，当判断while 在前判断TC或TXE的时候，
  这两条语句执行完毕之后，如果串口被复用被改变了输出状态，则可能导致串口内的数据无法正常送出。
  如果判断TC在前，判断while 在后（st现有做法），那么就出现丢首字节的问题。
  如果判断TXE在前判断while 在后，则因TXE并不代表当前传输完毕，所以串口复用的时候依然会存在可能导致尾字节无法送出的问题。
  
  所以这个问题的解决方案要根据各人的实际使用情况来确定：
  
  如果串口不会复用，则建议选用判断TXE标志。亦即：
  USART_SendData(usart,data);  //A
  while (USART_GetFlagStatus(usart,USART_FLAG_TXE) == RESET);  //B
  //这里选用TXE而不是TC,是因为TXE默认为1，当A执行后，TXE就立刻变成了0,所以接下来的while就起作用了。
  //如果选用TC,同样TC默认是1，但是A执行后TC在某种情况下，【注意不是绝对的，同样一个printf语句，在程序的不同地方执行，
  有时候TC会立即变成0，有时候却不会，具体什么原因懒得查了】却不能立刻变成0，所以接下来的while语句不成立，所以就直接跳走了，
  所以再来第二个字节的话，就可能第一个字节就被覆盖了，从而导致丢字节
  
  
  如果串口被复用则建议采用保险的做法，亦即楼主给出的解决方案，虽然该方案会浪费一点点时间和略有些麻烦。
  ******************************************************************/
  
}
void UART4_SendData(char *buf,u8 Len)   //只能发数组，暂时凑合用吧 
{ 
  
  u8 t=0;
  for(t=0;t<Len;t++)		
  {
    //UART4->DR=buf[t];
    UART4_SendByte(buf[t]);
    //while((UART4->SR&0X40)==0);//等待发送结束
    
    
  }
}

void UART4_Send_Str(char *str)   //只能发数组，暂时凑合用吧 
{ //strtobuf(str);
  
  
  //u8 len=strlen(str);
  //	Usart3_SendData(str,len);
  //	while((UART4->SR&0X40)==0);//等待发送结束
  while(*str !='\0')
    UART4_SendByte(*str++);
  
  
}

//--------------------------------------------------------------------------------
void Usart2_SendByte(char dat)
{			
  
  
  //USART_ClearFlag(UART4,USART_FLAG_TC); 
  //USART_GetFlagStatus(USART2, USART_FLAG_TC);// 第一个数据丢失问题 先读sr 再写DR
  
  USART2->DR=dat;	// ??????
  while (USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
  
  
  
  //printf("\r\n send dat=%x %c\r\n",dat,dat);
  //while((USART2->SR&0X40)==0);//等待发送结束
  //		USART2_RX_STA=0;
  //		USART_RX_STA=0;
}

/*void Usart2_SendByte(u8 dat)
{
//USART_ClearFlag(UART4,USART_FLAG_TC); 
USART_GetFlagStatus(USART2, USART_FLAG_TC);// 第一个数据丢失问题 先读sr 再写DR
USART2->DR=dat;	// ??????
while((USART2->SR&0X40)==0);//等待发送结束		
USART2_RX_STA=0;
}*/
void Usart2_SendData(char *buf,u8 Len)   //只能发数组，暂时凑合用吧 
{ 
  u8 t=0;
  for(t=0;t<Len;t++)		
  {
    //UART4->DR=buf[t];		
    Usart2_SendByte(buf[t]);
    //while((UART4->SR&0X40)==0);//等待发送结束
  }
}

void Usart2_Send_Str(char *str)   //只能发数组，暂时凑合用吧 
{ //strtobuf(str);
  //u8 len=strlen(str);
  //Usart2_SendData(str,len);
  //	while((USART3->SR&0X40)==0);//等待发送结束
  while(*str !='\0')
    Usart2_SendByte(*str++);
}

//--------------------------------------------------------------------------------
void Usart1_SendByte(char dat)
{
  USART1->DR=dat;	// ??????
  while (USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET); 	
  //  R1Fifo_Read++;
}


void Usart1_SendData(char *buf,u8 Len)   //只能发数组，暂时凑合用吧 
{ 
  u8 t=0;
  for(t=0;t<Len;t++)		
  {	
    Usart1_SendByte(buf[t]);	
  }
}

void Usart1_Send_Str(char *str)   //只能发数组，暂时凑合用吧 
{ 
  while(*str !='\0'){
    Usart1_SendByte(*str++);
    
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void UART_Put_Num(unsigned long dat)
{
  char temp[20];
  u32tostr(dat,temp);
  UART4_Send_Str(temp);
}
//-----------------------------------------------------------------------------

void UART_Put_Hex(unsigned int hex)
{
  char temp[20];
  Hex2Str_16b(hex,temp);
  Usart2_Send_Str(temp);
}



//-------------------------------------------------------
int net_sendchar(BYTE *buffer, int count)
{
  
  unsigned char *pByte;
  int i = 0;
  char buffer1[50];
  u8 typeinfo[2]={0x45,0x43};  //EU
  
  sprintf(buffer1,"AT+CIPSEND=%d\r\n",count+6);
  Usart1_Send_Str(buffer1);
  delay_ms (100); 
  for(i=0;i<2;i++){
    Usart1_SendByte(typeinfo[i]);
  }
  //pByte=ID;
  for(i=0;i<4;i++){
    Usart1_SendByte(Deviceinfo.DevcieID[i]);
  }
  //delay_ms (80);
  pByte = buffer;
  for (i = 0; i<count; i++)
  {
    Usart1_SendByte(pByte[i]);
    
  }
  
  return count;
}
int net_sendcmd(BYTE *buffer, int count,u8 cmd,u8 svrProfileId)
{
  unsigned char *pByte;
  int i = 0;
  char buffer1[50];
  u8 typeinfo[2]={0x45,0x43};  //EU
  
  sprintf(buffer1,"AT+CIPSEND=%d\r\n",count+7);
  Usart1_Send_Str(buffer1);
  delay_ms (10); 
  for(i=0;i<2;i++){
    Usart1_SendByte(typeinfo[i]);
  }
  
  for(i=0;i<4;i++){
    Usart1_SendByte(Deviceinfo.DevcieID[i]);
  }
  
  Usart1_SendByte(cmd);
  
  pByte = buffer;
  for (i = 0; i<count; i++)
  {
    Usart1_SendByte(pByte[i]);
    
  }
  return count;
}

int net_send_TC(BYTE *buffer, int count,u8 cmd,u8 svrProfileId)
{
  
  unsigned char *pByte;
  int i = 0;
  //char buffer1[50];
  u8 typeinfo[2]={0x45,0x43};  //EU
  
  
  //sprintf(buffer1,"AT+CIPSEND\r\n");
  //Usart1_Send_Str(buffer1);
  //delay_ms (10); 
  for(i=0;i<2;i++){
    Usart1_SendByte(typeinfo[i]);
  }
  
  for(i=0;i<4;i++){
    Usart1_SendByte(Deviceinfo.DevcieID[i]);
  }
  
  Usart1_SendByte(cmd);
  
  pByte = buffer;
  for (i = 0; i<count; i++)
  {
    Usart1_SendByte(pByte[i]);
    
  }
  return count;
}


//-------------------------------------------------------------
/*
*********************************************************************************************************
** 函数名称 ：send_sms_pdu()
** 函数功能 ：
**   PDU模式发送短信息
** 参数说明：
**   PDU串，使用光盘附带的PDU编码工具生成
*********************************************************************************************************
*/

/*
u32 Look_up_Message(void)
{ 
char *pointer;
char s[2]={0};
int i=0;
// char my_buf[50];

// UART4_Send_Str(sdata);
pointer = strchr(sdata,':');
printf("pointer=%s\r\n",pointer); 
pointer++;
//pointer+=11;
Num = atoi(pointer);   //这是未读过的信息条数 

// UART4_Send_Str(pointer);
// delay_ms (100);

pointer = strchr(pointer,',');
printf("pointer=%s\r\n",pointer); 
//pointer=pointer+13;  //加上一条AT^SISR=0,0\r\n  的距离  读取字符
pointer++;
//printf("pointer=%s\r\n",pointer); 
s[0]=*pointer;
pointer++;
if(*pointer!=','){
s[1]=*pointer;
i=1;
  }else 
s[1]='0';
//	TotalNum=100;
if(i==1)
TotalNum=(s[0]-'0')*10+(s[1]-'0');
  else
TotalNum=(s[0]-'0');
//TotalNum=(s1[0]-'0')*10+(s1[1]-'0');
//pointer=pointer+17;
//printf("pointer=%c\r\n",*pointer); 
//TotalNum= atoi(*pointer);   //接收到的总条数
//TotalNum= *pointer;	
// UART4_Send_Str(pointer);
delay_ms(100);
printf("TotalNum=%d\r\n",TotalNum); 
zj=0; 
memset(sdata,0,strlen(sdata));     
return 1;
}
*/
/********************************************************************************
** ???? :SendTimeRtc1()
** ???? :??RTC????,?????????PRS?????????
** ???? :?
** ???? :?
*********************************************************************************/
/*
void SendTimeRtc1 (void)
{
char szTmpl[50];
//	u32 times;
// times = TIM2_tick;    // ??????????
//datas = CTIME1;
TIME_Get(TIM2_tick);

sprintf(szTmpl,"%4d.%2d.%2d-%2d.%2d.%2d",calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec);
net_sendchar((BYTE *)szTmpl,strlen(szTmpl));
handly_get_value(1,2);
zj=0; 
memset(sdata,0,strlen(sdata));   
} 
//---------------------------------------------------

*/

//-------------------------------------------------
/*********************************************************************************
*int readTeleBook(void)
*读取电话本函数，主要是把超级用户和绑定用户的号码从电话本里读出来，放在内存里，
*增加以后查找的效率
*返回值1表示成功,-1表示失败0
*********************************************************************************/



//------------------------------------------------------------






void UARTBUF_Init(void)
{
  R1Fifo_Start = (u8*)UARTR1Buf;  //com -->en
  R1Fifo_End   = (u8*)UARTR1Buf + sizeof(UARTR1Buf);
  R1Fifo_Read  = (u8*)UARTR1Buf;
  R1Fifo_Write = (u8*)UARTR1Buf;
  
  
  
  
}

u16 UARTR1BUF_HoldNum(void)
{
  u16 wHoldNum;
  DISABLE_TXRXINT;            // disable high level priority interrupt
  if( R1Fifo_Write == R1Fifo_Read )
    wHoldNum = 0;
  else if( R1Fifo_Write > R1Fifo_Read )
    wHoldNum = R1Fifo_Write - R1Fifo_Read;
  else
    wHoldNum = R1Fifo_End - R1Fifo_Start + R1Fifo_Write - R1Fifo_Read;
  ENABLE_TXRXINT;            // enable high level priority interrupt
  return wHoldNum;
}



void UARTR1BUF_Read(BYTE* pBuffer,u16 wCount )
{
  while( wCount-- > 0 )
  {
    //   IWDG_Feed();
    // read from the FIFO, and pointe to the next
    *pBuffer++ = *R1Fifo_Read;
    DISABLE_TXRXINT;            // disable high level priority interrupt
    R1Fifo_Read++;
    ENABLE_TXRXINT;            // enable high level priority interrupt
    // check if it is the bottom of FIFO
    if( R1Fifo_Read >= R1Fifo_End )
    {
      DISABLE_TXRXINT;            // disable high level priority interrupt
      R1Fifo_Read = R1Fifo_Start;
      ENABLE_TXRXINT;            // enable high level priority interrupt
    }
  } 
}




/**********************************************************************************************************************
**************************************************************************************************************************/

//------------------------------------------------------
//------------------------------------------------------
void USART1_IRQHandler(void)                	//串口1中断服务程序
{
  Usart1_RecData();
}
void USART2_IRQHandler(void)                	//串口1中断服务程序
{
  Usart2_RecData();
}
void USART3_IRQHandler(void)                	//串口3中断服务程序
{
  // Usart3_RecData(); 
} 
void UART4_IRQHandler(void)                	//串口1中断服务程序
{
  UART4_RecData();    
} 

