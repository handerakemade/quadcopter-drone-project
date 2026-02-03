//button.c
#include "button.h"

void BUTTON_Init(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = BUTTON_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;  // 下拉：默认低电平，按下为高电平
    GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}

uint8_t BUTTON_Read(void) {
    return GPIO_ReadInputDataBit(BUTTON_PORT, BUTTON_PIN);
}
