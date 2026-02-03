//pwm.c
#include "pwm.h"
#include "stm32f4xx.h"

#include "pwm.h"

static uint16_t clamp(uint16_t v)
{
    if (v < PWM_MIN_US) return PWM_MIN_US;
    if (v > PWM_MAX_US) return PWM_MAX_US;
    return v;
}

void PWM3_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tim;
    TIM_OCInitTypeDef       oc;

    /* 1. 开时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 2. GPIO 复用配置 */
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;          /* PC6/PC7/PC8/PC9 */
    GPIO_Init(GPIOC, &gpio);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);

    /* 3. 时基：1 MHz -> 20 000 = 20 ms */
    tim.TIM_Period        = 20000 - 1;
    tim.TIM_Prescaler     = 84 - 1;
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tim);

    /* 4. 四通道 PWM1 模式，默认 1000 μs */
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_Pulse       = PWM_MIN_US;

    TIM_OC1Init(TIM3, &oc); TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM3, &oc); TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC3Init(TIM3, &oc); TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC4Init(TIM3, &oc); TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/* 通道接口 */
void PWM3_SetCh1(uint16_t us) { TIM3->CCR1 = clamp(us); }
void PWM3_SetCh2(uint16_t us) { TIM3->CCR2 = clamp(us); }
void PWM3_SetCh3(uint16_t us) { TIM3->CCR3 = clamp(us); }
void PWM3_SetCh4(uint16_t us) { TIM3->CCR4 = clamp(us); }
