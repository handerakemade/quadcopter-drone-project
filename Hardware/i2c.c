//i2c.c，这里使用的是软件i2c1，这里需要从,gy-86读取数据后输出，所以这里包含读时序的过程
#include "i2c.h"
#include "delay.h"

// I2C引脚定义：SCL=PB8, SDA=PB9
#define I2C_PORT                GPIOB
#define I2C_SCL_PIN             GPIO_Pin_8
#define I2C_SDA_PIN             GPIO_Pin_9
#define I2C_CLK                 RCC_AHB1Periph_GPIOB

// I2C延时函数
static void I2C_Delay(void) {
    Delay_us(5);
}

// 设置SDA为输入模式
static void SDA_IN(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
}

// 设置SDA为输出模式
static void SDA_OUT(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
}

// 产生I2C起始信号
static void I2C_Start(void) {
    SDA_OUT();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
}

// 产生I2C停止信号
static void I2C_Stop(void) {
    SDA_OUT();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
}

// 等待应答信号
static uint8_t I2C_Wait_Ack(void) {
    uint8_t timeout = 0;
    SDA_IN();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    
    while(GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)) {
        if(timeout++ > I2C_TIMEOUT) {
            I2C_Stop();
            return 1;
        }
    }
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    return 0;
}

// 产生应答信号
static void I2C_Ack(void) {
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    SDA_OUT();
    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
}

// 不产生应答信号
static void I2C_NAck(void) {
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    SDA_OUT();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
}

// I2C发送一个字节
static void I2C_Send_Byte(uint8_t data) {
    uint8_t i;
    SDA_OUT();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    
    for(i = 0; i < 8; i++) {
        if(data & 0x80)
            GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
        else
            GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
        data <<= 1;
        I2C_Delay();
        GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
        GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
    }
}

// I2C读取一个字节
static uint8_t I2C_Read_Byte(uint8_t ack) {
    uint8_t i, data = 0;
    SDA_IN();
    
    for(i = 0; i < 8; i++) {
        GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
        data <<= 1;
        if(GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN))
            data |= 0x01;
        GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
    }
    
    if(ack) I2C_Ack();
    else I2C_NAck();
    
    return data;
}

// I2C初始化函数
void GY86_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_AHB1PeriphClockCmd(I2C_CLK, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
    
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN | I2C_SDA_PIN);
}

// 写入数据函数
uint8_t I2C_WriteData(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    uint8_t i;
    
    I2C_Start();
    I2C_Send_Byte(devAddr << 1);
    if(I2C_Wait_Ack()) return 1;
    
    I2C_Send_Byte(regAddr);
    if(I2C_Wait_Ack()) return 1;
    
    for(i = 0; i < len; i++) {
        I2C_Send_Byte(data[i]);
        if(I2C_Wait_Ack()) return 1;
    }
    
    I2C_Stop();
    return 0;
}

// 读取数据函数
uint8_t I2C_ReadData(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    I2C_Start();
    I2C_Send_Byte(devAddr << 1);
    if(I2C_Wait_Ack()) return 1;
    
    I2C_Send_Byte(regAddr);
    if(I2C_Wait_Ack()) return 1;
    
    I2C_Start();
    I2C_Send_Byte((devAddr << 1) | 0x01);//bit7(第八位)为0时写指令，为1时读指令
    if(I2C_Wait_Ack()) return 1;
    
    while(len) {
        if(len == 1) {
            *data = I2C_Read_Byte(0);
        } else {
            *data = I2C_Read_Byte(1);
        }
        data++;
        len--;
    }
    
    I2C_Stop();
    return 0;
}

// 写入单个字节
uint8_t I2C_WriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    return I2C_WriteData(devAddr, regAddr, &data, 1);
}

// 读取单个字节
uint8_t I2C_ReadByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data) {
    return I2C_ReadData(devAddr, regAddr, data, 1);
}
