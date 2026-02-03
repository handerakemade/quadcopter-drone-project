//gy86.h
#ifndef __GY86_H
#define __GY86_H

#include "stm32f4xx.h"
#include <stdint.h>

// 传感器地址定义
#define MPU6050_ADDRESS         0x68
#define HMC5883L_ADDRESS        0x1E
#define MS5611_ADDRESS          0x77

// MPU6050寄存器定义
#define MPU6050_WHO_AM_I        0x75
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_CONFIG          0x1A
#define MPU6050_GYRO_CONFIG     0x1B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_INT_PIN_CFG     0x37
#define MPU6050_ACCEL_XOUT_H    0x3B

// HMC5883L寄存器定义
#define HMC5883L_CONFIG_A       0x00
#define HMC5883L_CONFIG_B       0x01
#define HMC5883L_MODE           0x02
#define HMC5883L_DATA_X_MSB     0x03

// MS5611寄存器定义
#define MS5611_RESET            0x1E
#define MS5611_CONV_D1          0x40
#define MS5611_CONV_D2          0x50
#define MS5611_ADC_READ         0x00

// 传感器数据结构体
//注意一下这里结构体属性的有符号16位整数，读取时它是由两个无符号8位整数拼凑成的，需考虑正负性
typedef struct {
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t mag_x, mag_y, mag_z;
    int32_t temperature;
    int32_t pressure;
} GY86_Data;

uint8_t MPU6050_Init(void);
uint8_t MPU6050_ReadAccelGyro(int16_t* accel, int16_t* gyro);
uint8_t MPU6050_EnableBypass(void);

uint8_t HMC5883L_Init(void);
uint8_t HMC5883L_ReadMag(int16_t* mag);

uint8_t MS5611_Init(void);
uint8_t MS5611_ReadData(int32_t* temperature, int32_t* pressure);

uint8_t GY86_Init(void);
uint8_t GY86_ReadAll(GY86_Data* data);

#endif
