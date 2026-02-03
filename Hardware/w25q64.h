//w25q64.h
#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f4xx.h"

// ------------------- 引脚定义 -------------------
// CS (片选) -> PB12 (软件控制)
// SCK      -> PB13
// MISO     -> PB14
// MOSI     -> PB15
#define W25Q64_SPI              SPI2
#define W25Q64_SPI_CLK          RCC_APB1Periph_SPI2
#define W25Q64_GPIO_CLK         RCC_AHB1Periph_GPIOB
#define W25Q64_GPIO_PORT        GPIOB

#define W25Q64_CS_PIN           GPIO_Pin_12
#define W25Q64_SCK_PIN          GPIO_Pin_13
#define W25Q64_MISO_PIN         GPIO_Pin_14
#define W25Q64_MOSI_PIN         GPIO_Pin_15

// ------------------- 片选控制宏 -------------------
#define W25Q64_CS_LOW()         GPIO_ResetBits(W25Q64_GPIO_PORT, W25Q64_CS_PIN)
#define W25Q64_CS_HIGH()        GPIO_SetBits(W25Q64_GPIO_PORT, W25Q64_CS_PIN)

// ------------------- W25Q64 指令集 -------------------
#define W25X_WriteEnable        0x06 
#define W25X_WriteDisable       0x04 
#define W25X_ReadStatusReg      0x05 
#define W25X_WriteStatusReg     0x01 
#define W25X_ReadData           0x03 
#define W25X_FastReadData       0x0B 
#define W25X_FastReadDual       0x3B 
#define W25X_PageProgram        0x02 
#define W25X_BlockErase         0xD8 
#define W25X_SectorErase        0x20 
#define W25X_ChipErase          0xC7 
#define W25X_PowerDown          0xB9 
#define W25X_ReleasePowerDown   0xAB 
#define W25X_DeviceID           0xAB 
#define W25X_ManufactDeviceID   0x90 
#define W25X_JedecDeviceID      0x9F 

// ------------------- 函数声明 -------------------
void W25Q64_Init(void);                         // 初始化
uint16_t W25Q64_ReadID(void);                   // 读取ID
void W25Q64_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);   // 读取数据
void W25Q64_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite); // 写一页
void W25Q64_Erase_Sector(uint32_t Dst_Addr);    // 擦除扇区
void W25Q64_Write_Enable(void);                 // 写使能
void W25Q64_Wait_Busy(void);                    // 等待忙碌结束

#endif
