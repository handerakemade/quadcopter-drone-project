//pwm.h
#ifndef PWM_H
#define PWM_H

#include "stm32f4xx.h"

#define PWM_MIN_US   1000
#define PWM_MAX_US   2000

void PWM3_Init(void);

/* 四通道独立接口 */
void PWM3_SetCh1(uint16_t us);   /* PB4  TIM3_CH1 */
void PWM3_SetCh2(uint16_t us);   /* PB5  TIM3_CH2 */
void PWM3_SetCh3(uint16_t us);   /* PC8  TIM3_CH3 */
void PWM3_SetCh4(uint16_t us);   /* PC9  TIM3_CH4 */

#endif
