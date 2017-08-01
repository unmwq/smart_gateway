#ifndef __KEY_LED_H
#define __KEY_LED_H	 
#include "GenericTypeDefs.h"
#include "stm32f10x.h"
 
#define KEY1_SET  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)//��ȡ����1
#define KEY2_ENC  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_4)//��ȡ����2

#define LED1_ON  GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define LED1_OFF GPIO_SetBits(GPIOB, GPIO_Pin_0)

#define LED2_ON  GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define LED2_OFF GPIO_SetBits(GPIOB, GPIO_Pin_1)

#define LED3_ON  GPIO_ResetBits(GPIOA, GPIO_Pin_0)
#define LED3_OFF GPIO_SetBits(GPIOA, GPIO_Pin_0)

#define LED4_ON  GPIO_ResetBits(GPIOA, GPIO_Pin_1)
#define LED4_OFF GPIO_SetBits(GPIOA, GPIO_Pin_1)


#define K_CLR   0
#define K_SET	1		//KEY1 
#define K_ENC	2		//KEY2  

//�˵�
typedef struct
{
    unsigned char Layer;   //�˵����ڼ���
    unsigned char s1;   //�˵���1���µ������
    unsigned char s2;   //�˵���2���µ������
    unsigned char s3;   //�˵���3���µ������
    unsigned char s4;   //�˵���4���µ�����
     unsigned char s5;
    unsigned char s6;
     unsigned char s7;
     
}KeyMenu_TypeDef;


void KEY_LED_Init(void);//IO��ʼ��

void GetKey(void);
void KeyProcess(void);
uint8 KEY_Scan(uint8 mode);  	//����ɨ�躯��					    
#endif