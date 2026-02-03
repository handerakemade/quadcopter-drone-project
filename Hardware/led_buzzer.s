;==========================================================
;  led_buzzer.s  -- 汇编实现 LED + Buzzer 驱动
;==========================================================
                PRESERVE8
                THUMB

;EQU伪指令相当于#define，全称Equate
;-------------------------- 宏 -----------------------------
LED_PORT        EQU     0x40020000          ; GPIOA
LED_PIN         EQU     (1<<5)              ; PA5
LED_CLK_EN      EQU     (1<<0)              ; RCC_AHB1ENR bit0

Buzzer_PORT     EQU     0x40020C00          ; GPIOD
Buzzer_PIN      EQU     (1<<2)              ; PD2
Buzzer_CLK_EN   EQU     (1<<3)              ; RCC_AHB1ENR bit3

RCC_BASE        EQU     0x40023800
RCC_AHB1ENR     EQU     (RCC_BASE + 0x30)

; GPIO 寄存器 offsets
GPIO_MODER      EQU     0x00
GPIO_OTYPER     EQU     0x04
GPIO_OSPEEDR    EQU     0x08
GPIO_PUPDR      EQU     0x0C
GPIO_BSRR       EQU     0x18
GPIO_ODR        EQU     0x14

;-------------------- 导出函数符号，外部可用 -------------------------
                EXPORT LED_Init
                EXPORT LED_On
                EXPORT LED_Off
                EXPORT LED_Toggle
                EXPORT LED_Blink_1Hz
                EXPORT Buzzer_Init
                EXPORT Buzzer_On
                EXPORT Buzzer_Off
                EXPORT Buzzer_Toggle
                EXPORT Buzzer_Blink_1Hz

;-------------------- 外部符号 ----------------------------
                IMPORT Delay_ms

;AREA告诉汇编器，从此开始定义一片新的内存区域，汇编代码都必须位于某个AREA中，链接器会自己安排AREA的位置
;|.text| .text表示可执行代码，双竖线是ARM汇编语法要求
;CODE告诉汇编器这一段是机器指令，不是数据变量
;READONLY表示访问权限是只读，烧录在flash中，如果是READWRITE会放在RAM中
;-------------------- 代码段 ------------------------------
                AREA    |.text|, CODE, READONLY

;---------------------------------------------------------
LED_Init
;  void LED_Init(void);
;---------------------------------------------------------
                PUSH    {LR}

                ; 1) 使能 GPIOA 时钟
                LDR     R0, =RCC_AHB1ENR
                LDR     R1, [R0]
                ORR     R1, R1, #LED_CLK_EN
                STR     R1, [R0]

                ; 2) 等待时钟就绪
wait_led_clk
                LDR     R1, [R0]
                TST     R1, #LED_CLK_EN
                BEQ     wait_led_clk

                ; 3) MODER: PA5 -> 输出 (01)
                LDR     R0, =LED_PORT
                LDR     R1, [R0, #GPIO_MODER]
                BIC     R1, R1, #(3<<10)      ; 清 bit11:10
                ORR     R1, R1, #(1<<10)      ; 置 01
                STR     R1, [R0, #GPIO_MODER]

                ; 4) OTYPER: 推挽
                LDR     R1, [R0, #GPIO_OTYPER]
                BIC     R1, R1, #LED_PIN
                STR     R1, [R0, #GPIO_OTYPER]

                ; 5) OSPEEDR: 50 MHz  (10)
                LDR     R1, [R0, #GPIO_OSPEEDR]
                BIC     R1, R1, #(3<<10)
                ORR     R1, R1, #(2<<10)
                STR     R1, [R0, #GPIO_OSPEEDR]

                ; 6) PUPDR: 无上下拉
                LDR     R1, [R0, #GPIO_PUPDR]
                BIC     R1, R1, #(3<<10)
                STR     R1, [R0, #GPIO_PUPDR]

                ; 7) 关灯
                BL      LED_Off

                POP     {PC}

;---------------------------------------------------------
LED_On
;  void LED_On(void);
;---------------------------------------------------------
                LDR     R0, =LED_PORT
                LDR     R1, =LED_PIN
                STR     R1, [R0, #GPIO_BSRR]      ; 置位
                ; 记录状态 = 1
                LDR     R0, =led_state
                MOVS    R1, #1
                STRB    R1, [R0]
                BX      LR

;---------------------------------------------------------
LED_Off
;  void LED_Off(void);
;---------------------------------------------------------
                LDR     R0, =LED_PORT
                LDR     R1, =LED_PIN
                LSL     R1, R1, #16               ; 复位位
                STR     R1, [R0, #GPIO_BSRR]
                ; 记录状态 = 0
                LDR     R0, =led_state
                MOVS    R1, #0
                STRB    R1, [R0]
                BX      LR

;---------------------------------------------------------
LED_Toggle
;  void LED_Toggle(void);
;---------------------------------------------------------
                LDR     R0, =LED_PORT
                LDR     R1, [R0, #GPIO_ODR]
                LDR     R2, =LED_PIN
                EOR     R1, R1, R2
                STR     R1, [R0, #GPIO_ODR]
                ; 更新状态变量
                LDR     R0, =led_state
                LDRB    R1, [R0]
                EOR     R1, R1, #1
                STRB    R1, [R0]
                BX      LR

;---------------------------------------------------------
LED_Blink_1Hz
;  void LED_Blink_1Hz(void);
;---------------------------------------------------------
                PUSH    {LR}
                BL      LED_On
                MOVS    R0, #500
                BL      Delay_ms
                BL      LED_Off
                MOVS    R0, #500
                BL      Delay_ms
                POP     {PC}

;=========================================================
;  蜂鸣器部分 —— 与 LED 完全对称
;=========================================================
Buzzer_Init
                PUSH    {LR}
                ; 1) 使能 GPIOD 时钟
                LDR     R0, =RCC_AHB1ENR
                LDR     R1, [R0]
                ORR     R1, R1, #Buzzer_CLK_EN
                STR     R1, [R0]
wait_bz_clk
                LDR     R1, [R0]
                TST     R1, #Buzzer_CLK_EN
                BEQ     wait_bz_clk

                ; 2) PD2 配置
                LDR     R0, =Buzzer_PORT
                ; MODER
                LDR     R1, [R0, #GPIO_MODER]
                BIC     R1, R1, #(3<<4)     ; PD2 -> bit5:4
                ORR     R1, R1, #(1<<4)
                STR     R1, [R0, #GPIO_MODER]
                ; OTYPER 推挽
                LDR     R1, [R0, #GPIO_OTYPER]
                BIC     R1, R1, #Buzzer_PIN
                STR     R1, [R0, #GPIO_OTYPER]
                ; 速度 50 MHz
                LDR     R1, [R0, #GPIO_OSPEEDR]
                BIC     R1, R1, #(3<<4)
                ORR     R1, R1, #(2<<4)
                STR     R1, [R0, #GPIO_OSPEEDR]
                ; 无上下拉
                LDR     R1, [R0, #GPIO_PUPDR]
                BIC     R1, R1, #(3<<4)
                STR     R1, [R0, #GPIO_PUPDR]

                BL      Buzzer_Off
                POP     {PC}

;---------------------------------------------------------
Buzzer_On
;  void Buzzer_On(void);
;---------------------------------------------------------
                LDR     R0, =Buzzer_PORT
                LDR     R1, =Buzzer_PIN
                STR     R1, [R0, #GPIO_BSRR]
                BX      LR

;---------------------------------------------------------
Buzzer_Off
;  void Buzzer_Off(void);
;---------------------------------------------------------
                LDR     R0, =Buzzer_PORT
                LDR     R1, =Buzzer_PIN
                LSL     R1, R1, #16
                STR     R1, [R0, #GPIO_BSRR]
                BX      LR

;---------------------------------------------------------
Buzzer_Toggle
;  void Buzzer_Toggle(void);
;---------------------------------------------------------
                LDR     R0, =Buzzer_PORT
                LDR     R1, [R0, #GPIO_ODR]
                LDR     R2, =Buzzer_PIN
                EOR     R1, R1, R2
                STR     R1, [R0, #GPIO_ODR]
                BX      LR

;---------------------------------------------------------
Buzzer_Blink_1Hz
;  void Buzzer_Blink_1Hz(void);
;---------------------------------------------------------
                PUSH    {LR}
                BL      Buzzer_On
                MOVS    R0, #500
                BL      Delay_ms
                BL      Buzzer_Off
                MOVS    R0, #500
                BL      Delay_ms
                POP     {PC}

;=========================================================
; AREA表示区域
;|.data|表示这里存放数据而不是代码
;DATA同
;READWRITE表示可读可写，通常存放在SRAM中
;led_state其实表示的是这个数据的地址
;DCB是Define Constant Byte，定义常量
;=========================================================
                AREA    |.data|, DATA, READWRITE
led_state       DCB     0                   ; 0=灭 1=亮

;---------------------------------------------------------
                END
					