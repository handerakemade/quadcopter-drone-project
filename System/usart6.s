;==========================================================
;  usart6.s  -- 汇编实现 USART6 驱动（TX=PC6，RX=PC7）
;==========================================================
                PRESERVE8
                THUMB

;-------------------------- 常量 ---------------------------
RCC_BASE          EQU     0x40023800
RCC_AHB1ENR       EQU     (RCC_BASE + 0x30)
RCC_APB2ENR       EQU     (RCC_BASE + 0x44)

GPIOC_BASE        EQU     0x40020800
GPIOC_MODER       EQU     (GPIOC_BASE + 0x00)
GPIOC_OTYPER      EQU     (GPIOC_BASE + 0x04)
GPIOC_OSPEEDR     EQU     (GPIOC_BASE + 0x08)
GPIOC_PUPDR       EQU     (GPIOC_BASE + 0x0C)
GPIOC_AFRL        EQU     (GPIOC_BASE + 0x20)

USART6_BASE       EQU     0x40011400
USART6_SR         EQU     (USART6_BASE + 0x00)
USART6_DR         EQU     (USART6_BASE + 0x04)
USART6_BRR        EQU     (USART6_BASE + 0x08)
USART6_CR1        EQU     (USART6_BASE + 0x0C)

RCC_AHB1ENR_GPIOCEN   EQU     (1<<2)
RCC_APB2ENR_USART6EN  EQU     (1<<5)
GPIO_AF_USART6        EQU     0x08
USART_CR1_UE          EQU     (1<<13)
USART_CR1_TE          EQU     (1<<3)
USART_CR1_RE          EQU     (1<<2)
USART_SR_TXE          EQU     (1<<7)

;-------------------- 导出函数符号 -------------------------
                EXPORT USART6_Init
                EXPORT USART6_SendChar
                EXPORT USART6_SendString
				EXPORT USART6_SendInt			

;-------------------- 代码段 ------------------------------
                AREA    |.text|, CODE, READONLY

;---------------------------------------------------------
USART6_Init
;  void USART6_Init(uint32_t baudrate);
;  baudrate 在 R0 里
;---------------------------------------------------------
                PUSH    {R4-R7, LR}

;因为参数存在R0里面，所以下面的代码不用R0传数
                ; 1) 开 GPIOC 时钟
                LDR     R1, =RCC_AHB1ENR
                LDR     R2, [R1]
                ORR     R2, R2, #RCC_AHB1ENR_GPIOCEN
                STR     R2, [R1]

                ; 2) 开 USART6 时钟
                LDR     R1, =RCC_APB2ENR
                LDR     R2, [R1]
                ORR     R2, R2, #RCC_APB2ENR_USART6EN
                STR     R2, [R1]

; 3) PC6/PC7 配置成AF模式，配置成复用模式
                LDR     R1, =GPIOC_MODER
                LDR     R2, [R1]
                BIC     R2, R2, #(3<<12)    ; Clear PC6
                ORR     R2, R2, #(2<<12)    ; PC6 -> AF
                BIC     R2, R2, #(3<<14)    ; Clear PC7
                ORR     R2, R2, #(2<<14)    ; PC7 -> AF
                STR     R2, [R1]

                LDR     R1, =GPIOC_OTYPER
                LDR     R2, [R1]
                BIC     R2, R2, #(3<<6)     ; PC6/7 -> Push Pull
                STR     R2, [R1]

                LDR     R1, =GPIOC_OSPEEDR
                LDR     R2, [R1]
                BIC     R2, R2, #(3<<12)
                ORR     R2, R2, #(2<<12)    ; High Speed
                BIC     R2, R2, #(3<<14)
                ORR     R2, R2, #(2<<14)
                STR     R2, [R1]

                LDR     R1, =GPIOC_PUPDR
                LDR     R2, [R1]
                BIC     R2, R2, #(3<<12)
                ORR     R2, R2, #(1<<12)    ; Pull-up
                BIC     R2, R2, #(3<<14)
                ORR     R2, R2, #(1<<14)
                STR     R2, [R1]

; 4) 复用功能 AF8 (USART6)，复用功能选择USART6
                LDR     R1, =GPIOC_AFRL
                LDR     R2, [R1]
                BIC     R2, R2, #(0xF<<24)
                ORR     R2, R2, #(GPIO_AF_USART6<<24) ; PC6
                BIC     R2, R2, #(0xF<<28)
                ORR     R2, R2, #(GPIO_AF_USART6<<28) ; PC7
                STR     R2, [R1]

; 5) 波特率计算 (这里USART6在APB2，为84MHz)
                ; 公式: BRR = PCLK2 / BaudRate
                ; 注意：这里直接除法得到的结果写入BRR是正确的，
                ; 因为 BRR 的格式正好符合 16倍过采样时的除法结果分布。
                LDR     R1, =84000000
                UDIV    R2, R1, R0
                LDR     R1, =USART6_BRR
                STR     R2, [R1]

                ; 6) CR1 = 8N1, TE+RE+UE
				;UE是USART ENABLE串口模块总使能
                LDR     R1, =USART6_CR1
                MOV     R2, #(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE)
                STR     R2, [R1]

                POP     {R4-R7, PC}

;---------------------------------------------------------
USART6_SendChar
;  void USART6_SendChar(uint8_t ch);
;  ch 在 R0 低 8 位 (这是重点，SendString 必须把字给 R0)
;SR是状态寄存器（status register），DR是数据寄存器
;SR有一位是TXE，TXE为1时表示数据寄存器为空可以继续发数据了，为0表示上一包数据还没处理完
;DR是双向寄存器，STRB的时候是发数据，数据送到发送移位寄存器，LDRB会送到接收移位寄存器
;STRB和STR的区别是STRB只会处理一个字节（Byte）
;---------------------------------------------------------
                LDR     R1, =USART6_SR
wait_txe
                LDR     R2, [R1]
                TST     R2, #USART_SR_TXE
                BEQ     wait_txe

                LDR     R1, =USART6_DR
                STRB    R0, [R1]
                BX      LR

;---------------------------------------------------------
USART6_SendString
;  void USART6_SendString(char *str);
;  输入：str 的地址在 R0
;---------------------------------------------------------
                PUSH    {R4, LR}        ; 保存 R4 (被调用者保存寄存器) 和 LR

                MOV     R4, R0          ; 把字符串指针从 R0 转移到 R4 保存
                                        ; 因为 R0 将被用来给 SendChar 传参

loop_str
                LDRB    R0, [R4]        ; 从指针 R4 处读取字符 -> 放入 R0
                CMP     R0, #0          ; 检查是不是 '\0'
                BEQ     end_str         ; 如果是 0，结束

                BL      USART6_SendChar ; 调用发送函数 (它会发送 R0 里的字符)

                ADD     R4, R4, #1      ; R4 指针加 1，也就是地址+1，能一路处理下去
                B       loop_str

end_str
                POP     {R4, PC}        ; 恢复 R4 并返回

;---------------------------------------------------------
USART6_SendInt
;  void USART6_SendInt(uint32_t num);
;  功能：将 R0 中的无符号整数转换为 ASCII 并发送
;---------------------------------------------------------
                PUSH    {R4-R8, LR}     ; 保存寄存器

                ; --- 0 的特殊处理 ---
                CMP     R0, #0
                BNE     convert_start
                MOV     R0, #'0'        ; 如果是0，直接发字符 '0'
                BL      USART6_SendChar
                B       send_int_exit

convert_start
                MOV     R4, R0          ; R4 = 待处理的数字
                MOV     R5, #10         ; R5 = 除数 10
                MOV     R6, SP          ; R6 = 记录当前的栈顶位置 (基准线)

convert_loop
                ; R4 / 10 = R7 (商)
                UDIV    R7, R4, R5      
                
                ; R0 = R4 - (R7 * 10) = 余数 (Cortex-M4 乘减指令 MLS)
                MLS     R0, R7, R5, R4  
                
                ADD     R0, R0, #'0'    ; 转换为字符 (0 -> '0')
                PUSH    {R0}            ; 压栈 (因为算出来是个位，要最后发)
                
                MOV     R4, R7          ; 更新被除数为商
                CMP     R4, #0          ; 商为0了吗？
                BNE     convert_loop    ; 没为0，继续算下一位

print_stack_loop
                CMP     SP, R6          ; 检查栈指针是否回到基准线
                BEQ     send_int_exit   ; 如果相等，说明全发出去了

                POP     {R0}            ; 弹出一个字符
                BL      USART6_SendChar ; 发送
                B       print_stack_loop

send_int_exit
                POP     {R4-R8, PC}     ; 恢复寄存器并返回

;---------------------------------------------------------
                END
					