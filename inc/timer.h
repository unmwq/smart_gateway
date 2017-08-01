#ifndef __TIMER_H
#define __TIMER_H
#include "GenericTypeDefs.h"
#include "stm32f10x.h"


void RCC_Configuration(void)  ;
void TIM3_Int_Init(u16 arr,u16 psc);

void TIM4_Int_Init(u16 arr,u16 psc); 
 
void TIM2_Int_Init(u16 arr,u16 psc); 
extern volatile u32 TIM2_tick;
#endif
