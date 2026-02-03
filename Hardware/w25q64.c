//w25q64.c，注意只能接3.3V，因为它的工作电压是2.7~3.6V
#include "w25q64.h"

/**
 * @brief  SPI底层读写一个字节
 * @param  TxData: 要发送的数据
 * @retval 接收到的数据
 */
static uint8_t SPI2_ReadWriteByte(uint8_t TxData)
{
    // 等待发送缓冲区空 (TXE: Transmit buffer Empty)
    while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_TXE) == RESET);
    
    // 发送数据
    SPI_I2S_SendData(W25Q64_SPI, TxData);
    
    // 等待接收缓冲区非空 (RXNE: Receive buffer Not Empty)
    while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    
    // 返回读取的数据
    return SPI_I2S_ReceiveData(W25Q64_SPI);
}

/**
 * @brief  初始化SPI2及相关GPIO
 */
void W25Q64_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    // 1. 开启时钟：GPIOB 和 SPI2
    RCC_AHB1PeriphClockCmd(W25Q64_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(W25Q64_SPI_CLK, ENABLE);

    // 2. 配置GPIO引脚复用 (PB13, PB14, PB15)
    // F4系列必须显式配置AF映射
    GPIO_PinAFConfig(W25Q64_GPIO_PORT, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(W25Q64_GPIO_PORT, GPIO_PinSource14, GPIO_AF_SPI2);
    GPIO_PinAFConfig(W25Q64_GPIO_PORT, GPIO_PinSource15, GPIO_AF_SPI2);

    // 3. 配置GPIO参数 (SPI引脚: 复用推挽)
    GPIO_InitStructure.GPIO_Pin = W25Q64_SCK_PIN | W25Q64_MISO_PIN | W25Q64_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        // 复用模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // 推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 高速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // 上拉
    GPIO_Init(W25Q64_GPIO_PORT, &GPIO_InitStructure);

    // 4. 配置CS引脚 (PB12: 普通推挽输出)
    GPIO_InitStructure.GPIO_Pin = W25Q64_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       // 输出模式
    GPIO_Init(W25Q64_GPIO_PORT, &GPIO_InitStructure);

    W25Q64_CS_HIGH(); // 初始化时拉高片选，不选中

    // 5. 配置SPI2工作模式
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 全双工，允许同时收发
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                      // 主机模式
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                  // 8位数据
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                        // 时钟极性：空闲高电平 (Mode 3)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                       // 时钟相位：第二个边沿采样 (Mode 3)
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                          // 软件控制CS
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 预分频 (APB1=42M, /4 = 10.5M)
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                 // 高位在前
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(W25Q64_SPI, &SPI_InitStructure);

    // 6. 使能SPI2
    SPI_Cmd(W25Q64_SPI, ENABLE);
}

/**
 * @brief  读取W25Q64的ID
 * @retval ID值 (例如 0xEF16 表示 W25Q64)
 */
uint16_t W25Q64_ReadID(void)
{
    uint16_t Temp = 0;
    
    W25Q64_CS_LOW();                // 拉低CS开始通信
    SPI2_ReadWriteByte(0x90);       // 发送读取ID命令
    SPI2_ReadWriteByte(0x00);       // 发送24位地址 (Dummy)
    SPI2_ReadWriteByte(0x00);
    SPI2_ReadWriteByte(0x00);
    
    Temp |= SPI2_ReadWriteByte(0xFF) << 8;  // 读取厂商ID (0xEF)
    Temp |= SPI2_ReadWriteByte(0xFF);       // 读取设备ID (0x16)
    
    W25Q64_CS_HIGH();               // 拉高CS结束通信
    return Temp;
}

/**
 * @brief  写使能 (Flash在写或擦除前必须调用)
 */
void W25Q64_Write_Enable(void)
{
    W25Q64_CS_LOW();
    SPI2_ReadWriteByte(W25X_WriteEnable); // 发送 0x06
    W25Q64_CS_HIGH();
}

/**
 * @brief  等待Flash内部操作完成 (检查BUSY位)
 */
void W25Q64_Wait_Busy(void)
{
    uint8_t status = 0;
    W25Q64_CS_LOW();
    SPI2_ReadWriteByte(W25X_ReadStatusReg); // 发送 0x05 读状态寄存器
    do
    {
        // 持续读取状态寄存器，直到 Bit 0 (BUSY) 为 0
        status = SPI2_ReadWriteByte(0xFF);
    } while ((status & 0x01) == 0x01);
    W25Q64_CS_HIGH();
}

/**
 * @brief  擦除一个扇区 (4KB)
 * @param  Dst_Addr: 扇区地址 (必须是4096的倍数)
 */
void W25Q64_Erase_Sector(uint32_t Dst_Addr)
{
    W25Q64_Write_Enable();      // 必须先写使能
    W25Q64_Wait_Busy();         // 确保空闲

    W25Q64_CS_LOW();
    SPI2_ReadWriteByte(W25X_SectorErase);      // 发送擦除命令 0x20
    SPI2_ReadWriteByte((uint8_t)((Dst_Addr) >> 16)); // 发送24位地址
    SPI2_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI2_ReadWriteByte((uint8_t)Dst_Addr);
    W25Q64_CS_HIGH();

    W25Q64_Wait_Busy();         // 等待擦除完成 (重要！擦除很慢)
}

/**
 * @brief  写一页数据 (最多256字节，不支持跨页)
 * @param  pBuffer: 数据指针
 * @param  WriteAddr: 写入地址
 * @param  NumByteToWrite: 字节数 (0-256)
 */
void W25Q64_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    W25Q64_Write_Enable();      // 写使能
    
    W25Q64_CS_LOW();
    SPI2_ReadWriteByte(W25X_PageProgram);      // 发送页编程命令 0x02
    SPI2_ReadWriteByte((uint8_t)((WriteAddr) >> 16)); // 24位地址
    SPI2_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI2_ReadWriteByte((uint8_t)WriteAddr);

    while (NumByteToWrite--)
    {
        SPI2_ReadWriteByte(*pBuffer); // 发送数据
        pBuffer++;
    }
    W25Q64_CS_HIGH();

    W25Q64_Wait_Busy();         // 等待写入完成
}

/**
 * @brief  读取数据
 * @param  pBuffer: 存储读到数据的数组
 * @param  ReadAddr: 读取地址
 * @param  NumByteToRead: 读取字节数
 */
void W25Q64_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
    W25Q64_CS_LOW();
    SPI2_ReadWriteByte(W25X_ReadData);         // 发送读取命令 0x03
    SPI2_ReadWriteByte((uint8_t)((ReadAddr) >> 16)); // 24位地址
    SPI2_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI2_ReadWriteByte((uint8_t)ReadAddr);

    while (NumByteToRead--)
    {
        *pBuffer = SPI2_ReadWriteByte(0xFF);   // 发送Dummy字节以产生时钟读取数据
        pBuffer++;
    }
    W25Q64_CS_HIGH();
}
