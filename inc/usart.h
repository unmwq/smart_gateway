#ifndef __USART_H
#define __USART_H
#include <stdio.h>	
#include "GenericTypeDefs.h"
#include "stm32f10x.h"
//////////////////////////////////////////////////////////////////
//--------------------------
//以下的宏定义用于readbook

#define  UCS2   "AT+CSCS=\"UCS2\"\r\n"  //设置为UCS2格式
#define  IRA_MODE "AT+CSCS=\"IRA\"\r\n"  //设置为GSM模式
#define  SM     "AT+CPBS=\"MT\"\r\n"   //指定要存储位置为SIM卡
#define  NUM    "AT+CPBS?\r\n"        //询问现在有多少个条目
#define  DELETE_ALL_MESSAGE "AT+CMGD=1,4\r\n"
#define  SLEEP  "AT+ESLP=1\r\n"
#define  WAKEUP  "AT+ESLP=0\r\n"
#define  SETADMIN		"8BBE7F6E7BA174065458"    //设置管理员
#define  DELADMIN		"522096647BA174065458"    //删除管理yuan
#define  BD				"8BBE7F6E4EB260C553F7"     //设置亲情号
#define  JC				"522096644EB260C553F7"       	  //设置亲情号
#define  JT				"76D1542C"        				 //监听
#define  VOLset         "8BBE7F6E97F391CF7B497EA7"  	//设置音量等级



#define  BD_ADM "7ED15B9A8D857EA7752862376210529F"    //绑定超级用户成功
#define  JC_ADM "89E396648D857EA7752862376210529F"    //解除超级用户成功
//#define  BD_CUSTOMER "53F778015DF288AB7ED15B9A"       //号码已被绑定
#define  BD_CUSTOMER "4EB260C553F78BBE7F6E6210529F"//亲情号码设置成功
//#define  JC_CUSTOMER "53F778015DF288AB89E39664"       //号码已被解除
#define  JC_CUSTOMER  "4EB260C553F7522096646210529F"//亲情号码删除成功
#define  CUSTOMER_NOTSET "4EB260C553F74E0D5B585728"  //亲情号不存在
#define  CUSTOMER_HAVE "4EB260C553F75DF25B585728" //亲情号已存在
#define  BF      "5E039632"					//布防
#define  CF		 "64A49632"	//撤防
#define  BFOK "672C7CFB7EDF5E0396326210529F"            //本系统布防成功
#define  CFOK "672C7CFB7EDF64A496326210529F"            //本系统撤防成功

//#define  FULL "4EB260C553F778014E2A65705DF26EE1"          //亲情号码个数已满
#define  FULL "4EB260C553F75DF26EE1"          //亲情号已满
#define  EXIST "8D857EA7752862375DF27ED15B9A"   //超级用户已绑定
//#define  EXIST "60A85DF27ECF662F8D857EA775286237FF0C65E09700518D7ED15B9A"  //你已经是超级用户，无需再绑定
#define  MECI_ALARM "95E878C162A58B66"      //门磁报警
#define  HW_ALARM "7EA259167A975E5562A58B66"      //红外窗幕报警
#define  OTHER_ALARM "51764ED64F20611F566862A58B66"      //其他传感器报警

#define  ALARM_F  "4E3B673A0034927462A58B66"     //主机4鉴报警
#define  ALARM_T  "4E3B673A0033927462A58B66"    //主机3鉴报警                            
#define  VOLset_OK "97F391CF7B497EA78BBE7F6E6210529F"     //音量等级设置成功
//--------------------------------------------------------
#define Photosize  1200


typedef unsigned char BYTE;  

extern u8 wifi_gprs_flags;


#define  MAXMEMITEM 11
#define  MAX_C_NUMBER 6 //5个亲情从1开始计算小于6 恰好5个
extern char telebook[MAXMEMITEM][100];
extern u8 BF_flag;
extern u32  TotalNum;
extern u32  Num;
//extern char  sdata[300];
extern int zj;
extern u8 send_flag;

int readTeleBook(void);

int net_sendcmd(BYTE *buffer, int count,u8 cmd,u8 svrProfileId);
u32 net_reconnection(char *addr, unsigned short port);
u32 delete_number_net(u8 fromwhat);
u32 add_customer_net(u8 fromwhat);
extern int readTeleBook();
extern u32 handle_message();
extern u32 handle_calling();

void SendTimeRtc1 (void);
unsigned long strtou32(char *str);
void u32tostr(unsigned long dat,char *str);
void SendTimeRtc1 (void);
u32 Look_up_Message(void);
u32 GPRS_init(void);
BOOL send_sms_pdu(char*number,char *pdu);
u32 send_all_customs(u8 mesg);

u32 send_mut_message(char *number, char *send_msg);


BOOL net_close(u8 srvProfileId);
void UART4_SendData(char *buf,u8 Len);

void Uart3_Init(u32 bound);
void UART4_Send_Str(char *str); 
void UART4_SendByte(char dat);
void Uart2_Init(u32 bound);
void Usart2_Send_Str(char *str);
void Usart2_SendData(char *buf,u8 Len);
void Usart2_SendByte(char dat);

void Usart1_Send_Str(char *str);
void Usart1_SendData(char *buf,u8 Len);
void Usart1_SendByte(char dat);

void UART3_SendByte(char dat);
void UART3_SendData(char *buf,u8 Len);
void Usart3_Send_Str(char *str);


void UARTBUF_Init(void);
u16 UARTR1BUF_HoldNum(void);
void UARTR1BUF_Read(BYTE* pBuffer,u16 wCount );
u16 UARTR3BUF_HoldNum(void);
void UARTR3BUF_Read(BYTE* pBuffer,u16 wCount );

u32 delete_administactor(void);

u32 add_administactor(void);
int gprs_init(void);

int net_send_TC(BYTE *buffer, int count,u8 cmd,u8 svrProfileId);

u32 NET_open(char *addr, unsigned short port,u8 svrProfileId);
int net_sendchar(BYTE *buffer, int count);
int net_send(BYTE *buffer, int count);
//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 bound);	  	

void UART4_printf (u8 *fmt, ...);
void USART5_printf (u8 *fmt, ...);
void UART4_send_string(u8 *buf,u8 len);

void UART5_send_string(u8 *buf,u8 len);

void Uart1_Init(u32 bound);
void Uart2_Init(u32 bound);
void Uart3_Init(u32 bound);
void Uart4_Init(u32 bound);
void Uart5_Init(u32 bound);
void AllUart_Init(void);

void Camera_Power_ON();
void Camera_Power_OFF();

#endif


