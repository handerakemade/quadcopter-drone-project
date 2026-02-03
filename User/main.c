//main.c
#include "stm32f4xx.h"
#include "usart1.h"
//#include "usart6.h"
#include "i2c.h"
#include "gy86.h"
#include "delay.h"
#include <stdio.h>
//#include "led_buzzer.h"
#include "pwm.h"
#include "ppm.h"
#include "oled.h"
//#include "button.h"
//#include "hc_sr04.h"
//#include "w25q64.h"

// 测试用的读写缓存
//uint8_t WriteBuffer[] = "Hello STM32F4 & W25Q64";
//uint8_t ReadBuffer[30];

int main(void)
{
	GY86_Data sensor_data;
	
//    SystemInit();          // 时钟
    Delay_Init();          // 延时初始化
//    LED_Init();            // LED 初始化
//	LED_On();
//	Delay_ms(3000);
//	LED_Toggle();
//	Buzzer_Init();			//Buzzer初始化，可接单片机3.3或5V	
//	Buzzer_On();
//	Delay_ms(3000);
//	Buzzer_Toggle();
	
//串口调试用
	USART1_Init(9600);  // 设置波特率115200匹配ATK-XCOM
    GY86_I2C_Init();		//I2C初始化
	
//蓝牙调试用	
//	USART6_Init(9600);		//USART蓝牙初始化
//	Delay_ms(50);


	//PPM，也就是接收机驱动，这里接收PWM2
	PPM_Init();
	PWM3_Init();
//	PWM3_SetCh1(1500);
//	Delay_ms(2000);
	
/*
	//PWM3，也就是电机驱动
	PWM3_SetCh1(2000);
	Delay_ms(2000);
	PWM3_SetCh1(1000);
	Delay_ms(2000);
	PWM3_SetCh1(1200);
//	Delay_ms(2000);
*/

	//显示屏OLED驱动，oled接3.3V和5V均可
    OLED_Init();
    OLED_Clear();	
	
//	uint32_t a = 0;
	
//    OLED_ShowString(0, 0, "ABCDE", 16);
//    OLED_ShowString(0, 2, "01234", 16);
//    OLED_ShowString(0, 4, "abcde", 16);
//    OLED_ShowString(0, 6, "{}|~,", 16);
//	OLED_ShowNum(0, 0, 12345, 5, 16);   // "12345"
//	OLED_ShowNum(0, 2,  789,  5, 16);   // "00789"
//	OLED_ShowNum(0, 4, 255, 3, 8);     // "255"
	
//    OLED_Refresh();


//按钮驱动
//    BUTTON_Init();//VCC接3.3V和5V均可

//gy-86驱动
//	printf("GY-86 Sensor Test Start...\r\n");
 
 
    // GY-86初始化
    if(GY86_Init()) {
        printf("GY-86 Initialization Failed!\r\n");
        while(1);
    }
	printf("GY-86 Initialization Success!\r\n");
  
	
//超声波驱动，接3.3V或5V均可，5V效果更佳
//    HC_SR04_Init();
//	uint32_t distance;//用来保存距离值
	
/*	
	//w25q64存储器初始化，先检验ID是否正确，只能接3.3V!
	uint16_t flash_id;
    W25Q64_Init();	
    flash_id = W25Q64_ReadID();
    // 0xEF16 (Winbond W25Q64型号，前8位EF是厂商ID，后8位16是设备ID)
    if (flash_id == 0xEF16) 
    {
		printf("W25Q64 initialization succeed!\r\n");
        // ID读取成功
    }else
    {
		printf("W25Q64 initialization fail!\r\n");
        // 错误：检查接线
        while(1); 
    }

	//擦除0x000000处，然后写入，然后读出验证
    W25Q64_Erase_Sector(0x000000);
    W25Q64_Write_Page(WriteBuffer, 0x000000, sizeof(WriteBuffer));
    W25Q64_Read(ReadBuffer, 0x000000, sizeof(WriteBuffer));
	
	printf("read: %s\r\n",(char*)ReadBuffer);
*/

    while(1) {
		

      if(GY86_ReadAll(&sensor_data) == 0) {
            // 打印加速度计数据
            printf("Accel: X=%6d, Y=%6d, Z=%6d\r\n", 
                   sensor_data.accel_x, sensor_data.accel_y, sensor_data.accel_z);
            
            // 打印陀螺仪数据
            printf("Gyro:  X=%6d, Y=%6d, Z=%6d\r\n", 
                   sensor_data.gyro_x, sensor_data.gyro_y, sensor_data.gyro_z);
            
            // 打印磁力计数据
            printf("Mag:   X=%6d, Y=%6d, Z=%6d\r\n", 
                   sensor_data.mag_x, sensor_data.mag_y, sensor_data.mag_z);
            
            // 打印温度和压力数据
            printf("Temp: %ld, Pressure: %ld\r\n", 
                   (long)sensor_data.temperature, (long)sensor_data.pressure);
            
            printf("----------------------------------------\r\n");
        } else {
            printf("Sensor Read Error!\r\n");
        }

//		Delay_ms(1000); 

		

//		USART6_SendString("123我是\r\n");
//		USART6_SendString("[");
//		USART6_SendInt(123456);
//		USART6_SendString("\r\n");
//		USART6_SendString("]");
//		Delay_ms(500);  // 500ms读取一次
		
//		USART1_SendString("123我是\r\n");
//		USART1_SendString("[");
//		USART1_SendString("123456");
//		USART1_SendString("\r\n");
//		USART1_SendString("]");
//		Delay_ms(500);  // 500ms读取一次	
	
//		LED_Blink_1Hz();
//		Buzzer_Blink_1Hz();
//		Buzzer_Toggle();
//		Delay_ms(500); 


		if (ppm_frame_ready)
        {
            ppm_frame_ready = 0;

//			printf("%d\n",ppm_values[7]);
//			Delay_ms(500);
            PWM3_SetCh1(ppm_values[0]);
            PWM3_SetCh2(ppm_values[1]);
            PWM3_SetCh3(ppm_values[2]);
			PWM3_SetCh4(ppm_values[3]);
			
			OLED_Clear();
			OLED_ShowNum(0, 0, ppm_values[0], 4, 16);
			OLED_ShowNum(0, 2, ppm_values[1], 4, 16);
			OLED_ShowNum(0, 4, ppm_values[2], 4, 16);
			OLED_ShowNum(0, 6, ppm_values[3], 4, 16);
			OLED_ShowNum(64, 0, ppm_values[4], 4, 16);
			OLED_ShowNum(64, 2, ppm_values[5], 4, 16);
			OLED_ShowNum(64, 4, ppm_values[6], 4, 16);
			OLED_ShowNum(64, 6, ppm_values[7], 4, 16);
			OLED_Refresh();						
			
        }


/*
		if (BUTTON_Read() == Bit_SET) {
            LED_On();
        } else {
            LED_Off();
        }
		Delay_ms(10);
*/
/*
		OLED_Clear();	
		OLED_ShowNum(0, 0, a, 5, 16);
		a++;
		OLED_Refresh();
*/
/*
		OLED_Clear();
        distance = HC_SR04_GetDistance();	
		
		OLED_ShowString(0, 0, "Distance:", 16);
        OLED_ShowNum (0, 2, distance, 3, 16);
        OLED_ShowString(48,2, "cm", 16);
		
		Delay_ms(80); // 厂商大于 60 ms，避免发射信号干扰回响
		
		OLED_Refresh();
*/
	}
}

