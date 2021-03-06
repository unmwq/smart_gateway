#ifndef __KEY_LED_H
#define __KEY_LED_H	 
#include "GenericTypeDefs.h"
#include "stm32f10x.h"
 
#define KEY1_SET  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)//读取按键1
#define KEY2_ENC  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_4)//读取按键2

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

//菜单
typedef struct
{
    unsigned char Layer;   //菜单所在级数
    unsigned char s1;   //菜单第1级下的子项号
    unsigned char s2;   //菜单第2级下的子项号
    unsigned char s3;   //菜单第3级下的子项号
    unsigned char s4;   //菜单第4级下的子项�
     unsigned char s5;
    unsigned char s6;
     unsigned char s7;
     
}KeyMenu_TypeDef;


void KEY_LED_Init(void);//IO初始化

void GetKey(void);
void KeyProcess(void);
uint8 KEY_Scan(uint8 mode);  	//按键扫描函数					    
#endif
