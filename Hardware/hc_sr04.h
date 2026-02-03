//hc_sr04.h
#ifndef __HC_SR04_H
#define __HC_SR04_H

#include "stm32f4xx.h"

/* 引脚宏 --------------------------------------------------------------------*/
#define TRIG_PORT           GPIOC
#define TRIG_PIN            GPIO_Pin_0
#define ECHO_PORT           GPIOA
#define ECHO_PIN            GPIO_Pin_1

/* 对外接口 ------------------------------------------------------------------*/
void    HC_SR04_Init(void);         // 初始化 GPIO + TIM2
uint32_t HC_SR04_GetDistance(void); // 返回距离（单位 cm，0 表示超时）

#endif
