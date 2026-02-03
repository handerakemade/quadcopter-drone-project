//oled.h
#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx.h"

#define OLED_I2C_ADDR   0x3C         // 7-bit 地址

/* 对外接口 */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t val, uint8_t len, uint8_t size);

#endif
