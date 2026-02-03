//i2c.h
#ifndef __I2C_H
#define __I2C_H

#include "stm32f4xx.h"
#include <stdint.h>

#define I2C_TIMEOUT 250

void GY86_I2C_Init(void);
uint8_t I2C_WriteData(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
uint8_t I2C_ReadData(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
uint8_t I2C_WriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
uint8_t I2C_ReadByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data);

#endif
