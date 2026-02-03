//delay.c
#include "delay.h"

static uint32_t SystemCoreClock = 84000000; // F401RE默认主频84MHz

// 延时函数初始化
void Delay_Init(void)
{
    // 配置SysTick定时器
    SysTick->CTRL = 0;  // 先禁用SysTick
    SysTick->LOAD = 0x00FFFFFF; // 设置重装载值为最大值(24位)
    SysTick->VAL = 0;   // 清空当前计数值
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // 选择处理器时钟
                    SysTick_CTRL_ENABLE_Msk;       // 使能SysTick但不开启中断
}

// 微秒级延时函数
void Delay_us(uint32_t us)
{
    uint32_t temp;
    // 计算需要延时的时钟周期数
    // SystemCoreClock/1000000 = 每微秒的时钟周期数
    SysTick->LOAD = (uint32_t)(us * (SystemCoreClock / 1000000)); 
    SysTick->VAL = 0;  // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 启动计数器
    
    // 等待时间到达
    do
    {
        temp = SysTick->CTRL;
    }
    while((temp & SysTick_CTRL_ENABLE_Msk) &&  // 检查定时器仍使能
          !(temp & SysTick_CTRL_COUNTFLAG_Msk)); // 检查计数标志是否置位
    
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  // 关闭计数器
    SysTick->VAL = 0;  // 清空计数器
}

// 毫秒级延时函数
void Delay_ms(uint32_t ms)
{
    uint32_t i;
    for(i = 0; i < ms; i++)
    {
        Delay_us(1000);  // 调用1000次微秒延时实现毫秒延时
    }
}
