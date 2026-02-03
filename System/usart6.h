//usart6.h
#ifndef __USART6_H
#define __USART6_H

#include "stm32f4xx.h"

void USART6_Init(uint32_t baudrate);
void USART6_SendChar(uint8_t ch);
void USART6_SendString(char *str);
void USART6_SendInt(uint32_t num);

#endif
