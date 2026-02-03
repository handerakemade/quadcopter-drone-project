;==========================================================
;  usart1.s
;  功能: USART1 驱动 (TX=PB6, RX=PB7) & printf 重定向
;==========================================================
                PRESERVE8
                THUMB

;-------------------------- 寄存器地址定义 ---------------------------
; RCC (时钟控制)
RCC_BASE          EQU     0x40023800
RCC_AHB1ENR       EQU     (RCC_BASE + 0x30)
RCC_APB2ENR       EQU     (RCC_BASE + 0x44)

; GPIOB (PB6, PB7)
GPIOB_BASE        EQU     0x40020400  ; 注意: GPIOB 基地址是 0x40020400
GPIOB_MODER       EQU     (GPIOB_BASE + 0x00)
GPIOB_OTYPER      EQU     (GPIOB_BASE + 0x04)
GPIOB_OSPEEDR     EQU     (GPIOB_BASE + 0x08)
GPIOB_PUPDR       EQU     (GPIOB_BASE + 0x0C)
GPIOB_AFRL        EQU     (GPIOB_BASE + 0x20)

; USART1
USART1_BASE       EQU     0x40011000  ; USART1 在 APB2 总线
USART1_SR         EQU     (USART1_BASE + 0x00)
USART1_DR         EQU     (USART1_BASE + 0x04)
USART1_BRR        EQU     (USART1_BASE + 0x08)
USART1_CR1        EQU     (USART1_BASE + 0x0C)

; 位掩码常量
RCC_AHB1ENR_GPIOBEN   EQU     (1<<1)  ; Bit 1: GPIOB 时钟
RCC_APB2ENR_USART1EN  EQU     (1<<4)  ; Bit 4: USART1 时钟
GPIO_AF_USART1        EQU     0x07    ; USART1 对应的复用功能号是 AF7
USART_CR1_UE          EQU     (1<<13) ; USART Enable
USART_CR1_TE          EQU     (1<<3)  ; Transmitter Enable
USART_CR1_RE          EQU     (1<<2)  ; Receiver Enable
USART_SR_TXE          EQU     (1<<7)  ; Transmit Data Register Empty

;-------------------- 导出函数符号 -------------------------
                EXPORT USART1_Init
                EXPORT USART1_SendByte
                EXPORT USART1_SendString
                EXPORT fputc

;-------------------- 代码段 ------------------------------
                AREA    |.text|, CODE, READONLY

;---------------------------------------------------------
; 函数: USART1_Init
; 原型: void USART1_Init(uint32_t baudrate);
; 输入: R0 (波特率)，参数都放在R0的
; 描述: 初始化 PB6(TX) 和 PB7(RX)，配置 USART1 (8N1)
;---------------------------------------------------------
USART1_Init
                PUSH    {R4-R5, LR}

                ; 1) 使能时钟
                ; -----------------------------------
                ; 开启 GPIOB 时钟
                LDR     R1, =RCC_AHB1ENR
                LDR     R2, [R1]
                ORR     R2, R2, #RCC_AHB1ENR_GPIOBEN
                STR     R2, [R1]

                ; 开启 USART1 时钟
                LDR     R1, =RCC_APB2ENR
                LDR     R2, [R1]
                ORR     R2, R2, #RCC_APB2ENR_USART1EN
                STR     R2, [R1]

                ; 2) 配置 GPIO (PB6, PB7)
                ; -----------------------------------
                ; MODER: 配置 PB6, PB7 为复用模式 (AF = 10)
                LDR     R1, =GPIOB_MODER
                LDR     R2, [R1]
                ; 清除 PB6(bit13,12) 和 PB7(bit15,14)
                BIC     R2, R2, #(0xF << 12) 
                ; 设置 PB6=10(binary), PB7=10(binary)
                ORR     R2, R2, #(0xA << 12) 
                STR     R2, [R1]

                ; OTYPER: 推挽输出 (Push-Pull = 0)
                LDR     R1, =GPIOB_OTYPER
                LDR     R2, [R1]
                BIC     R2, R2, #(3 << 6)   ; 清除 bit 6, 7
                STR     R2, [R1]

                ; OSPEEDR: 高速 (High Speed = 10)
                LDR     R1, =GPIOB_OSPEEDR
                LDR     R2, [R1]
                BIC     R2, R2, #(0xF << 12)
                ORR     R2, R2, #(0xA << 12)
                STR     R2, [R1]

                ; PUPDR: 上拉 (Pull-up = 01)
                LDR     R1, =GPIOB_PUPDR
                LDR     R2, [R1]
                BIC     R2, R2, #(0xF << 12)
                ORR     R2, R2, #(0x5 << 12) ; 0101 binary
                STR     R2, [R1]

                ; 3) 配置复用功能映射 (AFRL)
                ; -----------------------------------
                ; PB6 -> AF7, PB7 -> AF7
                ; AFRL 控制 Pin 0-7, 每4位一个引脚
                ; Pin 6 在 bit[27:24], Pin 7 在 bit[31:28]
                LDR     R1, =GPIOB_AFRL
                LDR     R2, [R1]
                BIC     R2, R2, #(0xFF << 24)       ; 清除高8位
                ORR     R2, R2, #(GPIO_AF_USART1 << 24) ; PB6 = AF7
                ORR     R2, R2, #(GPIO_AF_USART1 << 28) ; PB7 = AF7
                STR     R2, [R1]

                ; 4) 配置波特率 (BRR)
                ; -----------------------------------
                ; 公式: BRR = PCLK2 / BaudRate
                ; STM32F401RE中APB2 (PCLK2) 为 84MHz
                LDR     R1, =84000000
                UDIV    R2, R1, R0      ; R2 = 84000000 / baudrate
                LDR     R1, =USART1_BRR
                STR     R2, [R1]

                ; 5) 配置 CR1 (使能 UE, TE, RE)
                ; -----------------------------------
                LDR     R1, =USART1_CR1
                MOV     R2, #(USART_CR1_UE | USART_CR1_TE | USART_CR1_RE)
                STR     R2, [R1]

                POP     {R4-R5, PC}

;---------------------------------------------------------
; 函数: USART1_SendByte
; 原型: void USART1_SendByte(uint8_t data);
; 输入: R0 (要发送的字节)
;---------------------------------------------------------
USART1_SendByte
                LDR     R1, =USART1_SR
wait_txe
                LDR     R2, [R1]
                TST     R2, #USART_SR_TXE   ; 检查 TXE 位 (Bit 7)
                BEQ     wait_txe            ; 如果为 0，等待

                LDR     R1, =USART1_DR
                STRB    R0, [R1]            ; 写入数据
                BX      LR

;---------------------------------------------------------
; 函数: USART1_SendString
; 原型: void USART1_SendString(char *str);
; 输入: R0 (字符串首地址)
;---------------------------------------------------------
USART1_SendString
                PUSH    {R4, LR}        ; 保存 R4
                MOV     R4, R0          ; 字符串指针存入 R4

send_str_loop
                LDRB    R0, [R4]        ; 读取字符
                CMP     R0, #0          ; 检查结束符 '\0'
                BEQ     send_str_end

                BL      USART1_SendByte ; 发送当前字符
                ADD     R4, R4, #1      ; 指针+1
                B       send_str_loop

send_str_end
                POP     {R4, PC}

;---------------------------------------------------------
; 函数: fputc (重定向 printf)
; 原型: int fputc(int ch, FILE *f);
; 输入: R0 (字符 ch), R1 (文件指针 f - 忽略)
; 返回: R0 (返回写入的字符)
;---------------------------------------------------------
fputc
                PUSH    {LR}
                ; fputc 的第一个参数 ch 已经在 R0 中
                ; 直接调用 SendByte 发送 R0
                BL      USART1_SendByte
                
                ; R0 依然保持原来的字符值，直接返回即可满足 fputc 的返回值要求
                POP     {PC}

                END
					