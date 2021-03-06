/**
  ******************************************************************************
  * @file    DMA/FLASH_RAM/gd32f1x0_it.c 
  * @author  MCU SD
  * @version V2.0.0
  * @date    15-Jan-2016   
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "gd32f1x0_it.h"

/* Private variables ---------------------------------------------------------*/
extern __IO uint32_t count;

/* Private functions ---------------------------------------------------------*/
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
  * @brief  This function handles PendSVC exception.
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
}

#ifdef GD32F130_150
/**
  * @brief  This function handles DMA1_Channel2_3 Handler.
  * @param  None
  * @retval None
  */
void DMA1_Channel2_3_IRQHandler(void)
{
    if(DMA_GetIntBitState(DMA1_INT_TC2))
    {     
        count++;
        DMA_ClearIntBitState(DMA1_INT_GL2);
    }
}
#elif defined GD32F170_190
/**
  * @brief  This function handles DMA1_Channel4_5 Handler.
  * @param  None
  * @retval None
  */
void DMA1_Channel4_5_IRQHandler(void)
{
    if(DMA_GetIntBitState(DMA1_INT_TC4))
    {     
        count++;
        DMA_ClearIntBitState(DMA1_INT_GL4);
    }
}
#endif 

/******************* (C) COPYRIGHT 2016 GIGADEVICE *****END OF FILE****/
