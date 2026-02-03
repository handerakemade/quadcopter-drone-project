//gy86.c
#include "gy86.h"
#include "i2c.h"
#include "delay.h"
#include <string.h>

static uint16_t ms5611_cal[6];

// MPU6050初始化
uint8_t MPU6050_Init(void) {
    uint8_t check;
    
    // 检查设备ID
    if(I2C_ReadByte(MPU6050_ADDRESS, MPU6050_WHO_AM_I, &check)) return 1;
    if(check != 0x68) return 1;
    
    // 复位设备
    if(I2C_WriteByte(MPU6050_ADDRESS, MPU6050_PWR_MGMT_1, 0x80)) return 1;
    Delay_ms(100);
    
    // 唤醒设备，使用PLL作为时钟源
    if(I2C_WriteByte(MPU6050_ADDRESS, MPU6050_PWR_MGMT_1, 0x01)) return 1;
    // 设置加速度计±8g范围，陀螺仪±500°/s范围
    if(I2C_WriteByte(MPU6050_ADDRESS, MPU6050_ACCEL_CONFIG, 0x10)) return 1;
    if(I2C_WriteByte(MPU6050_ADDRESS, MPU6050_GYRO_CONFIG, 0x08)) return 1;
    // 设置DLPF带宽44Hz
    if(I2C_WriteByte(MPU6050_ADDRESS, MPU6050_CONFIG, 0x03)) return 1;
    
    return 0;
}

// 使能Bypass模式以访问HMC5883L
uint8_t MPU6050_EnableBypass(void) {
    return I2C_WriteByte(MPU6050_ADDRESS, MPU6050_INT_PIN_CFG, 0x02);
}

// 读取MPU6050加速度计和陀螺仪数据
uint8_t MPU6050_ReadAccelGyro(int16_t* accel, int16_t* gyro) {
    uint8_t buffer[14];
    
    if(I2C_ReadData(MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, buffer, 14)) return 1;
    
    accel[0] = (int16_t)((buffer[0] << 8) | buffer[1]);   // Accel X
    accel[1] = (int16_t)((buffer[2] << 8) | buffer[3]);   // Accel Y
    accel[2] = (int16_t)((buffer[4] << 8) | buffer[5]);   // Accel Z
    gyro[0] = (int16_t)((buffer[8] << 8) | buffer[9]);    // Gyro X
    gyro[1] = (int16_t)((buffer[10] << 8) | buffer[11]);  // Gyro Y
    gyro[2] = (int16_t)((buffer[12] << 8) | buffer[13]); // Gyro Z
    
    return 0;
}

// HMC5883L初始化
uint8_t HMC5883L_Init(void) {
    // 设置采样率75Hz，正常测量模式
    if(I2C_WriteByte(HMC5883L_ADDRESS, HMC5883L_CONFIG_A, 0x78)) return 1;
    // 设置增益±1.3Ga
    if(I2C_WriteByte(HMC5883L_ADDRESS, HMC5883L_CONFIG_B, 0x20)) return 1;
    // 设置连续测量模式
    if(I2C_WriteByte(HMC5883L_ADDRESS, HMC5883L_MODE, 0x00)) return 1;
    
    return 0;
}

// 读取HMC5883L磁力计数据
uint8_t HMC5883L_ReadMag(int16_t* mag) {
    uint8_t buffer[6];
    
    if(I2C_ReadData(HMC5883L_ADDRESS, HMC5883L_DATA_X_MSB, buffer, 6)) return 1;
    
    mag[0] = (int16_t)((buffer[0] << 8) | buffer[1]);   // Mag X
    mag[1] = (int16_t)((buffer[2] << 8) | buffer[3]);   // Mag Y  
    mag[2] = (int16_t)((buffer[4] << 8) | buffer[5]);   // Mag Z
    
    return 0;
}

// MS5611初始化
uint8_t MS5611_Init(void) {
    uint8_t i;
    uint8_t buffer[2];
    
    // 复位MS5611
    if(I2C_WriteByte(MS5611_ADDRESS, MS5611_RESET, 0x00)) return 1;
    Delay_ms(10);
    
    // 读取校准数据
    for(i = 0; i < 6; i++) {
        if(I2C_ReadData(MS5611_ADDRESS, 0xA2 + i * 2, buffer, 2)) return 1;
        ms5611_cal[i] = (buffer[0] << 8) | buffer[1];
    }
    
    return 0;
}

// 读取MS5611数据
uint8_t MS5611_ReadData(int32_t* temperature, int32_t* pressure) {
    uint8_t buffer[3];
    uint32_t d1 = 0, d2 = 0;
    int64_t dt, temp, off, sens, t2, off2, sens2;
    
    // 开始D1转换（压力）
    I2C_WriteByte(MS5611_ADDRESS, MS5611_CONV_D1, 0x00);
    Delay_ms(10);
    
    // 读取D1
    I2C_WriteByte(MS5611_ADDRESS, MS5611_ADC_READ, 0x00);
    I2C_ReadData(MS5611_ADDRESS, 0x00, buffer, 3);
    d1 = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    
    // 开始D2转换（温度）
    I2C_WriteByte(MS5611_ADDRESS, MS5611_CONV_D2, 0x00);
    Delay_ms(10);
    
    // 读取D2
    I2C_WriteByte(MS5611_ADDRESS, MS5611_ADC_READ, 0x00);
    I2C_ReadData(MS5611_ADDRESS, 0x00, buffer, 3);
    d2 = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    
    // 计算实际温度和压力值
    dt = d2 - ((int32_t)ms5611_cal[4] << 8);
    temp = 2000 + ((dt * (int64_t)ms5611_cal[5]) >> 23);
    
    off = ((int64_t)ms5611_cal[1] << 16) + (((int64_t)ms5611_cal[3] * dt) >> 7);
    sens = ((int64_t)ms5611_cal[0] << 15) + (((int64_t)ms5611_cal[2] * dt) >> 8);
    
    // 二阶温度补偿
    if(temp < 2000) {
        t2 = (dt * dt) >> 31;
        off2 = 5 * ((temp - 2000) * (temp - 2000)) >> 1;
        sens2 = off2 >> 1;
        
        if(temp < -1500) {
            off2 += 7 * (temp + 1500) * (temp + 1500);
            sens2 += 11 * ((temp + 1500) * (temp + 1500)) >> 1;
        }
        
        temp -= t2;
        off -= off2;
        sens -= sens2;
    }
    
    *temperature = temp;
    *pressure = (((d1 * sens) >> 21) - off) >> 15;
    
    return 0;
}

// GY86整体初始化
uint8_t GY86_Init(void) {
    if(MPU6050_Init()) return 1;
    Delay_ms(10);
    
    if(MPU6050_EnableBypass()) return 1;
    Delay_ms(10);
    
    if(HMC5883L_Init()) return 1;
    Delay_ms(10);
    
    if(MS5611_Init()) return 1;
    Delay_ms(10);
    
    return 0;
}

// 读取所有传感器数据
uint8_t GY86_ReadAll(GY86_Data* data) {
    int16_t accel[3], gyro[3], mag[3];
    
    if(MPU6050_ReadAccelGyro(accel, gyro)) return 1;
    if(HMC5883L_ReadMag(mag)) return 1;
    if(MS5611_ReadData(&data->temperature, &data->pressure)) return 1;
    
    data->accel_x = accel[0]; data->accel_y = accel[1]; data->accel_z = accel[2];
    data->gyro_x = gyro[0]; data->gyro_y = gyro[1]; data->gyro_z = gyro[2];
    data->mag_x = mag[0]; data->mag_y = mag[1]; data->mag_z = mag[2];
    
    return 0;
}
