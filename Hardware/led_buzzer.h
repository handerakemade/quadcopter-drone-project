//led和buzzer的头文件，led用的是板载灯PA5，buzzer用的PD2
#ifndef __LED_BUZZER_H__
#define __LED_BUZZER_H__

#include "stm32f4xx.h"
#include "delay.h"          // 引用延时

void LED_Init(void);
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void LED_Blink_1Hz(void);   // 每秒闪一次，阻塞版
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Toggle(void);
void Buzzer_Blink_1Hz(void);

#endif
