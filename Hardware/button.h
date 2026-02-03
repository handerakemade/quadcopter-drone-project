//button.h
#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f4xx.h"

#define BUTTON_PIN  GPIO_Pin_1
#define BUTTON_PORT GPIOC

void BUTTON_Init(void);
uint8_t BUTTON_Read(void);

#endif
