//ppm.h
#ifndef PPM_H
#define PPM_H

#include "stm32f4xx.h"

#define PPM_MAX_CHANNELS   8

extern volatile uint16_t ppm_values[PPM_MAX_CHANNELS];
extern volatile uint8_t  ppm_frame_ready;

/* 初始化 TIM2 通道1 输入捕获 */
void PPM_Init(void);

#endif
