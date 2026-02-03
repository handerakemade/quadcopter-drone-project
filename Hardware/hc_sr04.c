//hc_sr04.c
#include "hc_sr04.h"
#include "delay.h"

/**
 * @brief 配置 PC0(Trig) 为推挽输出，PA1(Echo) 为浮空输入，并启动 TIM2（1 MHz 计数）
 */
void HC_SR04_Init(void)
{
    GPIO_InitTypeDef  gpio;
    TIM_TimeBaseInitTypeDef tim;

    /* 1. 开时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* 2. Trig 引脚 */
    gpio.GPIO_Pin   = TRIG_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &gpio);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);   // 先拉低

    /* 3. Echo 引脚 */
    gpio.GPIO_Pin  = ECHO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Init(ECHO_PORT, &gpio);

    /* 4. TIM2 1 MHz → 1 us 计数 */
    tim.TIM_Prescaler = 84 - 1;            // 84 MHz / 84 = 1 MHz
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Period = 0xFFFF;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief 产生 10 us 触发脉冲
 */
static void HC_SR04_Trigger(void)
{
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);
    Delay_us(2);
    GPIO_SetBits  (TRIG_PORT, TRIG_PIN);
    Delay_us(10);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);
}

/**
 * @brief 获取一次距离（cm），超时返回 0
 */
uint32_t HC_SR04_GetDistance(void)
{
    uint32_t timeout = 10000;          // 防止死等
    HC_SR04_Trigger();

    /* 等待高电平到来 */
    while (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == Bit_RESET)
        if (--timeout == 0) return 0;

    TIM_SetCounter(TIM2, 0);           // 清零计数器
    /* 等待高电平结束 */
    while (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == Bit_SET)
        if (TIM_GetCounter(TIM2) > 50000) break;  // 约 50 ms 保护

    uint32_t duration = TIM_GetCounter(TIM2);     // 单位 us
    return duration / 58;                         // 换算成 cm
}
