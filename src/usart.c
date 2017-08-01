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

u32  TotalItemNum = 0 ;//���ڼ�¼�ܵĵ绰������Ŀ��

u32  TotalNum = 0 ;//���ڼ�¼������յ�������Ŀ��
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

//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  

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
  u8 buffer[CMD_BUFFER_LEN+1];  // CMD_BUFFER_LEN�����Լ������
  u8 i = 0;
  va_list arg_ptr;//��ʼ��ָ��ɱ�����б��ָ��
  va_start(arg_ptr, fmt);  //����һ���ɱ�����ĵ�ַ����arg_ptr����arg_ptrָ��ɱ�����б�Ŀ�ʼ   
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
  u8 buffer[CMD_BUFFER_LEN+1];  // CMD_BUFFER_LEN�����Լ������
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



void Uart1_Init(u32 bound)//GPRS ģ�� 
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USART_DeInit(USART1);  //��λ����2
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//GPIOAʱ��
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//ʹ��USART2
  
  
  
  //USART1_TX	 PA.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.2
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA2
  
  //USART1_RX	PA.3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA3
  
  
  //Usart1 NVIC ����
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //ռ�����ȼ��������ȼ�����Դ����
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;		//�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
  //USART ��ʼ������
  
  USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  
  USART_Init(USART1, &USART_InitStructure); //��ʼ������
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�����ж�
  USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 
  
}

void Uart2_Init(u32 bound)//GPRS ģ�� 
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USART_DeInit(USART2);  //��λ����2
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);	//GPIOAʱ��
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//ʹ��USART2
  
  
  
  //USART1_TX	 PA.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA2
  
  //USART1_RX	PA.3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA3
  
  
  //Usart1 NVIC ����
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //ռ�����ȼ��������ȼ�����Դ����
  
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
  //USART ��ʼ������
  
  USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  
  USART_Init(USART2, &USART_InitStructure); //��ʼ������
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�����ж�
  USART_Cmd(USART2, ENABLE);                    //ʹ�ܴ��� 
  
}



//��ʼ��IO ����3 
//bound:������
//��ʼ��IO ����3 
//bound:������
void Uart3_Init(u32 bound)////��������ͷ
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  USART_DeInit(USART3);  //��λ����1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);	//GPIOBʱ��
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//ʹ��USART3
  
  
  
  //USART3_TX   PB.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
  GPIO_Init(GPIOB, &GPIO_InitStructure); //��ʼ��PB.10
  
  //USART3_RX	  PB.11
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOB, &GPIO_InitStructure);  //��ʼ��PB.11
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  //����ͷ��Դ���� ��ע��PC9
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//��������
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  GPIO_SetBits(GPIOC,GPIO_Pin_9);
  
  
  //Usart3 NVIC ����
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //ռ�����ȼ��������ȼ�����Դ����
  
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//��ռ���ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//�����ȼ�1
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ��� 
  
  //USART ��ʼ������
  USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  
  USART_Init(USART3, &USART_InitStructure); //��ʼ������
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//�����ж�
  USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ��� 
}


void Uart4_Init(u32 bound)//GPRS ģ�� 
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//GPIOCʱ��
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//ʹ��USART4
  
  USART_DeInit(UART4);  //��λ����4
  
  //USART4_TX	 PC.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_Init(GPIOC, &GPIO_InitStructure); 
  //USART4_RX	PC.11
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  
  
  //Usart NVIC ����
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //ռ�����ȼ��������ȼ�����Դ����
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;		//�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
  //USART ��ʼ������
  
  USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  
  
  USART_Init(UART4, &USART_InitStructure); //��ʼ������
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//�����ж�
  USART_Cmd(UART4, ENABLE);                    //ʹ�ܴ��� 
  //485�շ����ƹܽ�
  //  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 
  //  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  //  GPIO_Init(GPIOC, &GPIO_InitStructure);  
}
void Uart5_Init(u32 bound)//����ģ�����
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOD, ENABLE);	//GPIOʱ��
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);//ʹ��USART5
  
  USART_DeInit(UART5);  //��λ����5
  
  //USART5_TX	 PC.12
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_Init(GPIOC, &GPIO_InitStructure); 
  
  //USART5_RX	PD.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOD, &GPIO_InitStructure);  
  
  
  
  //USART ��ʼ������
  
  USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  
  
  
  
  //Usart1 NVIC ����
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //ռ�����ȼ��������ȼ�����Դ����
  
  NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;		  //�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 		  //IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);   //����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
  USART_Init(UART5, &USART_InitStructure); //��ʼ������
  USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//�����ж�
  USART_Cmd(UART5, ENABLE);                    //ʹ�ܴ��� 
  
}

void AllUart_Init(void)
{
  Uart1_Init(115200);	//wifi
  Uart2_Init(9600);	//����ģ��
  //Uart3_Init(38400);	// �ƶ��������ͷ
  //  Uart3_Init(115200);	// �ƶ��������ͷ
  Uart4_Init(115200);	// to PC
  //  Uart5_Init(9600);	// ����ģ��
  
}

//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	

//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
//��ʼ��IO ����1 
//bound:������

/******************************************************************
- ������������һ��32λ�ı���datתΪ�ַ����������1234תΪ"1234"
- ����ģ�飺��������ģ��
- �������ԣ��ⲿ���û��ɵ���
- ����˵����dat:��ת��long�͵ı���
str:ָ���ַ������ָ�룬ת������ֽڴ���������           
- ����˵������
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
- ������������һ���ַ���תΪ32λ�ı���������"1234"תΪ1234
- ����ģ�飺��������ģ��
- �������ԣ��ⲿ���û��ɵ���
- ����˵����str:ָ���ת�����ַ���           
- ����˵����ת�������ֵ
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
- �������������ڷ�����ֵ
- ����ģ�飺���ڲ���
- �������ԣ��ⲿ��ʹ�û�ʹ��
- ����˵����dat:Ҫ���͵���ֵ
- ����˵������
- ע�������лὫ��ֵתΪ��Ӧ���ַ��������ͳ�ȥ������ 4567 תΪ "4567" 
**************************************************************************/



//--------------------------------------------------------------------------
void UART4_RecData(){
  
  u8 Res;
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
  {
    USART_ClearITPendingBit(UART4,USART_IT_RXNE);  //���жϱ�־�������������⣡����
    
    Res =USART_ReceiveData(UART4);//(USART1->DR);	//��ȡ���յ�������
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
if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
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
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
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
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
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


void UART3_SendData(char *buf,u8 Len)   //������ 
{          	
  int t=0;
  for(t=0;t<Len;t++)		
  {
    UART3_SendByte(buf[t]);
    
  }
}

void UART3_Send_Str(char *str)   //���ַ���
{ 
  while(*str !='\0')
  {
    UART3_SendByte(*str++);
  }
}


void UART4_SendByte(char dat)
{
  
  //USART_ClearFlag(UART4,USART_FLAG_TC); 
  //USART_GetFlagStatus(UART4, USART_FLAG_TC);// ��һ�����ݶ�ʧ���� �ȶ�sr ��дDR
  UART4->DR=dat;	// ??????
  //while((UART4->SR&0X40)==0);//�ȴ����ͽ���
  while (USART_GetFlagStatus(UART4,USART_FLAG_TXE) == RESET);
  
  /*****************************************************************
  TC �Ƿ�����ɱ�־��TXE�����ݷ�������λ�Ĵ����ı�־���÷�������������
  while (USART_GetFlagStatus(usart,USART_FLAG_TXE) == RESET); 
  USART_SendData(usart,data);
  ���while�����ǰ�����ں󣬸�����Ϊ��STԭ��Ŀ����Ϊ���չ����Ÿ��õ���������ж�while ��ǰ�ж�TC��TXE��ʱ��
  ���������ִ�����֮��������ڱ����ñ��ı������״̬������ܵ��´����ڵ������޷������ͳ���
  ����ж�TC��ǰ���ж�while �ں�st��������������ô�ͳ��ֶ����ֽڵ����⡣
  ����ж�TXE��ǰ�ж�while �ں�����TXE��������ǰ������ϣ����Դ��ڸ��õ�ʱ����Ȼ����ڿ��ܵ���β�ֽ��޷��ͳ������⡣
  
  �����������Ľ������Ҫ���ݸ��˵�ʵ��ʹ�������ȷ����
  
  ������ڲ��Ḵ�ã�����ѡ���ж�TXE��־���༴��
  USART_SendData(usart,data);  //A
  while (USART_GetFlagStatus(usart,USART_FLAG_TXE) == RESET);  //B
  //����ѡ��TXE������TC,����ΪTXEĬ��Ϊ1����Aִ�к�TXE�����̱����0,���Խ�������while���������ˡ�
  //���ѡ��TC,ͬ��TCĬ����1������Aִ�к�TC��ĳ������£���ע�ⲻ�Ǿ��Եģ�ͬ��һ��printf��䣬�ڳ���Ĳ�ͬ�ط�ִ�У�
  ��ʱ��TC���������0����ʱ��ȴ���ᣬ����ʲôԭ�����ò��ˡ�ȴ�������̱��0�����Խ�������while��䲻���������Ծ�ֱ�������ˣ�
  ���������ڶ����ֽڵĻ����Ϳ��ܵ�һ���ֽھͱ������ˣ��Ӷ����¶��ֽ�
  
  
  ������ڱ�����������ñ��յ��������༴¥�������Ľ����������Ȼ�÷������˷�һ���ʱ�������Щ�鷳��
  ******************************************************************/
  
}
void UART4_SendData(char *buf,u8 Len)   //ֻ�ܷ����飬��ʱ�պ��ð� 
{ 
  
  u8 t=0;
  for(t=0;t<Len;t++)		
  {
    //UART4->DR=buf[t];
    UART4_SendByte(buf[t]);
    //while((UART4->SR&0X40)==0);//�ȴ����ͽ���
    
    
  }
}

void UART4_Send_Str(char *str)   //ֻ�ܷ����飬��ʱ�պ��ð� 
{ //strtobuf(str);
  
  
  //u8 len=strlen(str);
  //	Usart3_SendData(str,len);
  //	while((UART4->SR&0X40)==0);//�ȴ����ͽ���
  while(*str !='\0')
    UART4_SendByte(*str++);
  
  
}

//--------------------------------------------------------------------------------
void Usart2_SendByte(char dat)
{			
  
  
  //USART_ClearFlag(UART4,USART_FLAG_TC); 
  //USART_GetFlagStatus(USART2, USART_FLAG_TC);// ��һ�����ݶ�ʧ���� �ȶ�sr ��дDR
  
  USART2->DR=dat;	// ??????
  while (USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
  
  
  
  //printf("\r\n send dat=%x %c\r\n",dat,dat);
  //while((USART2->SR&0X40)==0);//�ȴ����ͽ���
  //		USART2_RX_STA=0;
  //		USART_RX_STA=0;
}

/*void Usart2_SendByte(u8 dat)
{
//USART_ClearFlag(UART4,USART_FLAG_TC); 
USART_GetFlagStatus(USART2, USART_FLAG_TC);// ��һ�����ݶ�ʧ���� �ȶ�sr ��дDR
USART2->DR=dat;	// ??????
while((USART2->SR&0X40)==0);//�ȴ����ͽ���		
USART2_RX_STA=0;
}*/
void Usart2_SendData(char *buf,u8 Len)   //ֻ�ܷ����飬��ʱ�պ��ð� 
{ 
  u8 t=0;
  for(t=0;t<Len;t++)		
  {
    //UART4->DR=buf[t];		
    Usart2_SendByte(buf[t]);
    //while((UART4->SR&0X40)==0);//�ȴ����ͽ���
  }
}

void Usart2_Send_Str(char *str)   //ֻ�ܷ����飬��ʱ�պ��ð� 
{ //strtobuf(str);
  //u8 len=strlen(str);
  //Usart2_SendData(str,len);
  //	while((USART3->SR&0X40)==0);//�ȴ����ͽ���
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


void Usart1_SendData(char *buf,u8 Len)   //ֻ�ܷ����飬��ʱ�պ��ð� 
{ 
  u8 t=0;
  for(t=0;t<Len;t++)		
  {	
    Usart1_SendByte(buf[t]);	
  }
}

void Usart1_Send_Str(char *str)   //ֻ�ܷ����飬��ʱ�պ��ð� 
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
** �������� ��send_sms_pdu()
** �������� ��
**   PDUģʽ���Ͷ���Ϣ
** ����˵����
**   PDU����ʹ�ù��̸�����PDU���빤������
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
Num = atoi(pointer);   //����δ��������Ϣ���� 

// UART4_Send_Str(pointer);
// delay_ms (100);

pointer = strchr(pointer,',');
printf("pointer=%s\r\n",pointer); 
//pointer=pointer+13;  //����һ��AT^SISR=0,0\r\n  �ľ���  ��ȡ�ַ�
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
//TotalNum= atoi(*pointer);   //���յ���������
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
*��ȡ�绰����������Ҫ�ǰѳ����û��Ͱ��û��ĺ���ӵ绰����������������ڴ��
*�����Ժ���ҵ�Ч��
*����ֵ1��ʾ�ɹ�,-1��ʾʧ��0
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
void USART1_IRQHandler(void)                	//����1�жϷ������
{
  Usart1_RecData();
}
void USART2_IRQHandler(void)                	//����1�жϷ������
{
  Usart2_RecData();
}
void USART3_IRQHandler(void)                	//����3�жϷ������
{
  // Usart3_RecData(); 
} 
void UART4_IRQHandler(void)                	//����1�жϷ������
{
  UART4_RecData();    
} 

