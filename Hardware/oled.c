//oled.c，因为控制oled只需要i2c写时序，而不需要从oled读取，所以没有读时序
#include "oled.h"
#include "oledfont.h"
#include "delay.h"     // 你的延时函数头文件

/* ------------- 软件 I2C2 引脚定义 ------------- */
#define I2C_PORT        GPIOB
#define I2C_SCL_PIN     GPIO_Pin_10
#define I2C_SDA_PIN     GPIO_Pin_3
#define I2C_CLK         RCC_AHB1Periph_GPIOB

/* ------------- 显存 ------------- */
static uint8_t g_ucaOledRam[8][128];

/* ------------- 底层软件 I2C 时序 ------------- */
static void SDA_OUT(void)
{
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin  = I2C_SDA_PIN;
    g.GPIO_Mode = GPIO_Mode_OUT;
    g.GPIO_OType= GPIO_OType_OD;
    g.GPIO_PuPd = GPIO_PuPd_UP;
    g.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &g);
}
static void SDA_IN(void)
{
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin  = I2C_SDA_PIN;
    g.GPIO_Mode = GPIO_Mode_IN;
    g.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(I2C_PORT, &g);
}
//想跑 400 kHz 就改成 1 µs，但要确认从机能跟上
static void I2C_Delay(void) { Delay_us(5); }   // 100 kHz 级别

static void I2C_Start(void)
{
    SDA_OUT();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
}
static void I2C_Stop(void)
{
    SDA_OUT();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT,  I2C_SCL_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT,  I2C_SDA_PIN);
    I2C_Delay();
}
static uint8_t I2C_WaitAck(void)
{
    uint8_t t = 0;
    SDA_IN();
    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
    I2C_Delay();
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    while (GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN))
    {
        if (++t > 250) { I2C_Stop(); return 1; }
    }
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    I2C_Delay();
    return 0;
}
static void I2C_SendByte(uint8_t dat)
{
    SDA_OUT();
    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
    for (uint8_t i = 0; i < 8; i++)
    {
        if (dat & 0x80) GPIO_SetBits(I2C_PORT, I2C_SDA_PIN);
        else            GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN);
        dat <<= 1;
        I2C_Delay();
        GPIO_SetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
        GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN);
        I2C_Delay();
    }
}
static void I2C_WriteBuf(uint8_t devAddr, uint8_t *buf, uint16_t len)
{
    I2C_Start();
    I2C_SendByte(devAddr << 1);
    if (I2C_WaitAck()) { I2C_Stop(); return; }
    for (uint16_t i = 0; i < len; i++)
    {
        I2C_SendByte(buf[i]);
        if (I2C_WaitAck()) break;
    }
    I2C_Stop();
}

/* ------------- OLED 命令/数据封装 ------------- */
static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t pkt[2] = {0x00, cmd};   // Control byte = Co=0 D/C#=0
    I2C_WriteBuf(OLED_I2C_ADDR, pkt, 2);
}
static void OLED_WriteDat(uint8_t dat)
{
    uint8_t pkt[2] = {0x40, dat};   // Control byte = Co=0 D/C#=1
    I2C_WriteBuf(OLED_I2C_ADDR, pkt, 2);
}

/* ------------- 清屏 & 刷新 ------------- */
void OLED_Clear(void)
{
    for (int i = 0; i < 1024; i++) ((uint8_t *)g_ucaOledRam)[i] = 0x00;
}
void OLED_Refresh(void)
{
    OLED_WriteCmd(0x20); OLED_WriteCmd(0x00);  // 页地址模式
    OLED_WriteCmd(0x21); OLED_WriteCmd(0x00); OLED_WriteCmd(0x7F);
    OLED_WriteCmd(0x22); OLED_WriteCmd(0x00); OLED_WriteCmd(0x07);
    uint8_t *p = &g_ucaOledRam[0][0];
    for (uint16_t i = 0; i < 1024; i++) OLED_WriteDat(*p++);
}

/* ------------- 画点 & 字符 ------------- */
static void OLED_DrawPoint(uint8_t x, uint8_t y)
{
    if (x >= 128 || y >= 64) return;
    g_ucaOledRam[y >> 3][x] |= (1 << (y & 0x07));
}
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size)
{
    uint16_t index = chr - ' ';
    if (size == 16) {
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t tmp = OLED_F8x16[index][i];
            for (uint8_t j = 0; j < 8; j++)
                if (tmp & (1 << j)) OLED_DrawPoint(x + i, y * 8 + j);
            tmp = OLED_F8x16[index][i + 8];
            for (uint8_t j = 0; j < 8; j++)
                if (tmp & (1 << j)) OLED_DrawPoint(x + i, y * 8 + j + 8);
        }
    } else {
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t tmp = OLED_F6x8[index][i];
            for (uint8_t j = 0; j < 8; j++)
                if (tmp & (1 << j)) OLED_DrawPoint(x + i, y * 8 + j);
        }
    }
}
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size)
{
    while (*str) {
        OLED_ShowChar(x, y, *str++, size);
        x += (size == 16) ? 8 : 6;
        if (x > 120) { x = 0; y++; }
    }
}

/* ------------- 初始化序列（SSD1315） ------------- */
void OLED_Init(void)
{
    /* 使能 GPIOB 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* 引脚初始化为开漏上拉 */
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin  = I2C_SCL_PIN | I2C_SDA_PIN;
    g.GPIO_Mode = GPIO_Mode_OUT;
    g.GPIO_OType= GPIO_OType_OD;
    g.GPIO_PuPd = GPIO_PuPd_UP;
    g.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &g);
    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN | I2C_SDA_PIN);

    /* 上电延时 > 100 ms */
    Delay_ms(100);

    /* 发送完整初始化命令表 */
    const uint8_t cmd[] = {
        0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF,
        0xA1, 0xA6, 0xA8, 0x3F, 0xC8, 0xD3, 0x00,
        0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12,
        0xDB, 0x40, 0x8D, 0x14, 0xAF
    };
    for (uint8_t i = 0; i < sizeof(cmd); i++) OLED_WriteCmd(cmd[i]);

    OLED_Clear();
    OLED_Refresh();
}

/* 辅助：10 的幂 */
static uint32_t OLED_Pow(uint32_t base, uint32_t exp)
{
    uint32_t res = 1;
    while (exp--) res *= base;
    return res;
}

/* 主函数：十进制，前补 0，支持 8×16 和 6×8 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t val, uint8_t len, uint8_t size)
{
    uint8_t i, temp;
    uint8_t charW = (size == 16) ? 8 : 6;   /* 字符宽度 */

    for (i = 0; i < len; i++)
    {
        /* 从高到低取位 */
        temp = (val / OLED_Pow(10, len - i - 1)) % 10;
        /* 转成字符并显示 */
        OLED_ShowChar(x + charW * i, y, temp + '0', size);
    }
}
