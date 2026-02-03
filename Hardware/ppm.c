//ppm.c
#include "ppm.h"

volatile uint16_t ppm_values[PPM_MAX_CHANNELS] = {0};
volatile uint8_t  ppm_frame_ready = 0;

static volatile uint32_t last_cap = 0;
static volatile uint8_t  ch_idx   = 0;

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

        uint32_t cap = TIM_GetCapture1(TIM2);
        uint32_t diff = (cap >= last_cap) ? (cap - last_cap)
                                          : (0xFFFF - last_cap + cap);
        last_cap = cap;

        if (diff > 2500)          /* 同步间隔 */
        {
            ch_idx = 0;
            ppm_frame_ready = 1;
        }
        else if (ch_idx < PPM_MAX_CHANNELS)
        {
            ppm_values[ch_idx++] = diff;
        }
    }
}

//PA0,TIM2
void PPM_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tim;
    TIM_ICInitTypeDef       ic;
    NVIC_InitTypeDef        nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_0;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);

    tim.TIM_Period        = 0xFFFF;
    tim.TIM_Prescaler     = 84 - 1;          /* 1 MHz */
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim);

    ic.TIM_Channel     = TIM_Channel_1;
    ic.TIM_ICPolarity  = TIM_ICPolarity_Rising;
    ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
    ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    ic.TIM_ICFilter    = 0;
    TIM_ICInit(TIM2, &ic);

    TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    nvic.NVIC_IRQChannel                   = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}
