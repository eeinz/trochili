/**
  ******************************************************************************
  * @file    TIMER/TIMER2_PWMOutput/main.c 
  * @author  MCU SD
  * @version V2.0.0
  * @date    15-Jan-2016
  * @brief   Main program 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <gd32f1x0.h>
#include "gd32f1x0_eval.h"
#include "systick.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);
void TIMER_Configuration(void);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Configures system clocks.
  * @param  None
  * @retval None
  */
void RCC_Configuration(void)
{
    /* GPIOA, GPIOB and GPIOF clocks enable */
    RCC_AHBPeriphClock_Enable( RCC_AHBPERIPH_GPIOA |
                               RCC_AHBPERIPH_GPIOB |
                               RCC_AHBPERIPH_GPIOF  , ENABLE);
}

/**
  * @brief  Configure the GPIO ports.
  * @param  None
  * @retval None
  */
void GPIO_Configuration(void)
{
    GPIO_InitPara GPIO_InitStructure;

    /*Configure PB3 PB10 PB11(TIMER2 CH2 CH3 CH4) as alternate function*/
    GPIO_InitStructure.GPIO_Pin   = GPIO_PIN_3|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_MODE_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_50MHZ;
    GPIO_InitStructure.GPIO_OType = GPIO_OTYPE_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PUPD_NOPULL;
    GPIO_Init(GPIOB,&GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB,GPIO_PINSOURCE3,GPIO_AF_2);
    GPIO_PinAFConfig(GPIOB,GPIO_PINSOURCE10,GPIO_AF_2);
    GPIO_PinAFConfig(GPIOB,GPIO_PINSOURCE11,GPIO_AF_2);
}

/**
  * @brief  Configure the TIMER peripheral.
  * @param  None
  * @retval None
  */
void TIMER_Configuration(void)
{
    /* -----------------------------------------------------------------------
    TIMER2 Configuration: generate 3 PWM signals with 3 different duty cycles:
    TIMER2CLK = SystemCoreClock / 72 = 1MHz

    TIMER2 Channel2 duty cycle = (4000/ 16000)* 100  = 25%
    TIMER2 Channel3 duty cycle = (8000/ 16000)* 100  = 50%
    TIMER2 Channel4 duty cycle = (12000/ 16000)* 100 = 75%
    ----------------------------------------------------------------------- */
    TIMER_BaseInitPara TIMER_TimeBaseStructure;
    TIMER_OCInitPara  TIMER_OCInitStructure;

    /* TIMERS clock enable */
    RCC_APB2PeriphClock_Enable(RCC_APB2PERIPH_TIMER1|RCC_APB2PERIPH_TIMER15|RCC_APB2PERIPH_TIMER16|RCC_APB2PERIPH_TIMER17,ENABLE);
    RCC_APB1PeriphClock_Enable(RCC_APB1PERIPH_TIMER2|RCC_APB1PERIPH_TIMER3|RCC_APB1PERIPH_TIMER6|RCC_APB1PERIPH_TIMER14,ENABLE);

    /* TIMER2  configuration */
    TIMER_DeInit(TIMER2);
    TIMER_TimeBaseStructure.TIMER_Prescaler     = 71;
    TIMER_TimeBaseStructure.TIMER_CounterMode   = TIMER_COUNTER_UP;
    TIMER_TimeBaseStructure.TIMER_Period        = 15999;
    TIMER_TimeBaseStructure.TIMER_ClockDivision = TIMER_CDIV_DIV1;
    TIMER_BaseInit(TIMER2,&TIMER_TimeBaseStructure);

    /* CH2,CH3 and CH4 Configuration in PWM mode */
    TIMER_OCInitStructure.TIMER_OCMode      = TIMER_OC_MODE_PWM1;
    TIMER_OCInitStructure.TIMER_OCPolarity  = TIMER_OC_POLARITY_HIGH;
    TIMER_OCInitStructure.TIMER_OutputState = TIMER_OUTPUT_STATE_ENABLE;

    TIMER_OCInitStructure.TIMER_Pulse = 3999;
    TIMER_OC2_Init(TIMER2, &TIMER_OCInitStructure);
    TIMER_OC2_Preload(TIMER2,TIMER_OC_PRELOAD_DISABLE);

    TIMER_OCInitStructure.TIMER_Pulse = 7999;
    TIMER_OC3_Init(TIMER2, &TIMER_OCInitStructure);
    TIMER_OC3_Preload(TIMER2,TIMER_OC_PRELOAD_DISABLE);

    TIMER_OCInitStructure.TIMER_Pulse = 11999;
    TIMER_OC4_Init(TIMER2, &TIMER_OCInitStructure);
    TIMER_OC4_Preload(TIMER2,TIMER_OC_PRELOAD_DISABLE);

    /* Auto-reload preload enable */
    TIMER_CARLPreloadConfig(TIMER2,ENABLE);
    /* TIMER enable counter*/
    TIMER_Enable( TIMER2, ENABLE );
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main()
{
    RCC_Configuration();
    GPIO_Configuration();
    GD_EVAL_LEDInit(LED1);
    SysTick_Configuration();
    TIMER_Configuration();

    while(1)
    {
        GD_EVAL_LEDOn(LED1);
        Delay_1ms(1000);
        GD_EVAL_LEDOff(LED1);
        Delay_1ms(1000);
    }    
}

/******************* (C) COPYRIGHT 2016 GIGADEVICE *****END OF FILE****/
