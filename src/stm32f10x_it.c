/**
******************************************************************************
* @file    ADC/ADC1_DMA/stm32f10x_it.c 
* @author  MCD Application Team
* @version V3.5.0
* @date    08-April-2011
* @brief   Main Interrupt Service Routines.
*          This file provides template for all exceptions handler and peripherals
*          interrupt service routine.
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
******************************************************************************
*/ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "key_led.h"
#include "tick.h"
#include "detector.h"
#include "jq6500.h"


extern u8 keyflag ;
/** @addtogroup STM32F10x_StdPeriph_Examples
* @{
*/

/** @addtogroup ADC_ADC1_DMA
* @{
*/ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
* @brief  This function handles NMI exception.
* @param  None
* @retval None
*/
void NMI_Handler(void)
{
}

/**
* @brief  This function handles Hard Fault exception.
* @param  None
* @retval None
*/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  // pEsto_4U[1].errCode=0x01;
  while (1)
  {
  }
}

/**
* @brief  This function handles Memory Manage exception.
* @param  None
* @retval None
*/
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles Bus Fault exception.
* @param  None
* @retval None
*/
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles Usage Fault exception.
* @param  None
* @retval None
*/
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles SVCall exception.
* @param  None
* @retval None
*/
void SVC_Handler(void)
{
}

/**
* @brief  This function handles Debug Monitor exception.
* @param  None
* @retval None
*/
void DebugMon_Handler(void)
{
}

/**
* @brief  This function handles PendSV_Handler exception.
* @param  None
* @retval None
*/
void PendSV_Handler(void)
{
}

/**
* @brief  This function handles SysTick Handler.
* @param  None
* @retval None
*/
void SysTick_Handler(void)
{
  extern volatile u32 sys_ticks;
  sys_ticks++;
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
/*
void EXTI1_IRQHandler(void)//红外
{
static u32 IR_OldTime=0;

if (EXTI_GetITStatus(EXTI_Line1) != RESET)
{ 
EXTI_ClearFlag(EXTI_Line1);
//set按键，进行学习
if(((TickGet()) - IR_OldTime) > 50)//20ms
{

filter16(ultrasonic_detect(1),&Old_Udata[0]);
IR_OldTime = TickGet();
	}


//  IR_OldTime=TickGet();



	}
}
*/

/*
void EXTI2_IRQHandler(void)//微波
{
static u32 MW_OldTime=0;

if (EXTI_GetITStatus(EXTI_Line2) != RESET)
{ 
EXTI_ClearFlag(EXTI_Line2);
//set按键，进行学习

if(((TickGet()) - MW_OldTime) > 50)//20ms
{

filter16(ultrasonic_detect(1),&Old_Udata[0]);
MW_OldTime = TickGet();
	}


//   MW_OldTime = TickGet();

	}
}
*/

u8 first_IR;
void EXTI1_IRQHandler(void)//红外
{

if (EXTI_GetITStatus(EXTI_Line1) != RESET)
{ 
  
  first_IR= ir_in_handler();
  // first_IR=1;
   EXTI_ClearFlag(EXTI_Line1);

}
}


//u8 flag=0;
extern u8 keyflag;
void EXTI9_5_IRQHandler(void)//删除按键
{
  static u32 key2_touchOldTime=0;
  if (EXTI_GetITStatus(EXTI_Line5) != RESET)
  {
	
	
	//删除按键。删除存储数据
	// if((TickGet() - key2_touchOldTime) > 50)//20ms
	if((TickGet() - key2_touchOldTime) > 250)//20ms
	{
	  keyflag = K_ENC;
	  key2_touchOldTime = TickGet();
	}
	
	EXTI_ClearFlag(EXTI_Line5);	
  }
}


/**
* @brief  This function handles PPP interrupt request.
* @param  None
* @retval None
*/
/*void PPP_IRQHandler(void)
{
}*/































/**
* @}
*/ 

/**
* @}
*/ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
