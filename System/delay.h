//delay.h
#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f4xx.h"  // STM32F4标准库头文件

// 函数声明
void Delay_Init(void);          // 延时函数初始化
void Delay_us(uint32_t us);     // 微秒级延时
void Delay_ms(uint32_t ms);     // 毫秒级延时

#endif /* __DELAY_H__ */
