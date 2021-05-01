;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: armExcV7MSvcAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 11 月 14 日
;**
;** 描        述: ARMv7M 体系构架异常处理(SVC 方式任务切换).
;*********************************************************************************************************/

#ifndef __ARMEXCV7MSVCASM_H
#define __ARMEXCV7MSVCASM_H

;/*********************************************************************************************************
;  中断入口宏
;*********************************************************************************************************/

;/*********************************************************************************************************
;                                              IRQ_ENTRY
;                                                  +
;                                                  |
;                                                  |
;                                                  |
;                                                  v
;                                           API_InterEnter
;                                                  +
;                                                  |  中断嵌套
;                                                  |
;                           关中断前               +--------------------------+
;                           或                     |                          |
;                           任务切换时发生强占     |                          |
;                         +------------------------+                          v
;                         |                        |                    SAVE_CTX_REG
;                         |                        v                          +
;                         |               API_InterStackBaseGet               |
;                         |                        +                          |
;                         v                        |                          v
;                   SAVE_CTX_REG                   |                     ISR_HANDLE
;                         +                        v                          +
;                         |                API_ThreadTcbInter                 |
;                         |                        +                          |
;                         v                        |                          |
;                    ISR_HANDLE                    |                          v
;                         +                        v                    API_InterExit
;                         |                  SAVE_CTX_REG                     +
;                         |                        +                          |
;                         |                        |                          |
;                         v                        |                          |
;               API_InterExitNoSched               v                          v
;                         +                   ISR_HANDLE                LOAD_CTX_REG
;                         |                        +
;                         |                        |
;                         v                        |
;                    SEND PendSV                   v
;                         +                  API_InterExit
;                         |
;                         |
;                         |
;                         v
;                   LOAD_CTX_REG
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY  irq  handle)
    ;/*
    ; * 异常会自动将 xPSR, PC, LR, R12 以及 R3-R0 依次压入适当的堆栈中：
    ; * 如果异常前的代码正在使用 PSP, 则压入 PSP, 也就是使用进程堆栈；
    ; * 否则就压入 MSP, 使用主堆栈.
    ; * 一旦进入了服务例程, 就将一直使用主堆栈
    ; */

    ;/*
    ; * R1 记录 BASEPRI, 并关中断(利用优先级屏蔽中断, 才能使用 SVC)
    ; */
    CPSID   I
    MRS     R1 , BASEPRI                                        ;/*  Set prio int mask to mask all       */
    MOV     R2 , #LW_CFG_CORTEX_M_INT_MASK                      ;/*  mask prio >= 2                      */
    MSR     BASEPRI , R2
    CPSIE   I

    ;/*
    ; * R0 等于异常自动压栈前使用的 SP, 通过 EXC_RETURN(保存在 LR) 判断异常前使用何种 SP
    ; */
    TST     LR , #CORTEX_M_EXC_RETURN_MODE_MASK                 ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R0 , MSP
    MRSNE   R0 , PSP
    ADD     R0 , R0 , #(ARCH_HW_SAVE_CTX_SIZE)

    ;/*
    ; * 现在用 MSP, 如果发生中断嵌套, 需要保存寄存器上下文到 MSP 指向的栈空间里, 预留 SW SAVE 寄存器上下文空间
    ; */
    MRS     R2 , MSP
    SUB     R2 , R2 , #(ARCH_SW_SAVE_CTX_SIZE)
    MSR     MSP, R2

    PUSH    {R0 , LR}

    ;/*
    ; * API_InterEnter(异常自动压栈前使用的 SP, BASEPRI, EXC_RETURN) 进入中断,
    ; * 如果是第一次中断,   会将 异常前 SP, BASEPRI, EXC_RETURN 保存到当前任务 TCB 的 ARCH_REG_CTX 里
    ; * 如果不是第一次中断, 会将 异常前 SP, BASEPRI, EXC_RETURN 保存到上面预留的栈空间里
    ; */
    MOV     R2 , LR
    LDR     R3 , =API_InterEnter
    BLX     R3

    ;/*
    ; * 如果不是第一次进入中断, 那么上一次中断已经设置 MSP
    ; */
    CMP     R0 , #1
    BNE     2f

    ;/*
    ; * 这里可以使用 R12，因为硬件自动保存了
    ; * 判断是否在关中断前或者任务切换时发生抢占
    ; */
    POP     {R0 , R12}
    TST     R12 , #CORTEX_M_EXC_RETURN_MODE_MASK
    BEQ     1f

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 MSP
    ; */
    LDR     R3 , =API_InterStackBaseGet
    BLX     R3
    MSR     MSP , R0

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    LDR     R3 , =API_ThreadTcbInter
    BLX     R3

    ;/*
    ; * 保存 R4-R11 到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    ADD     R1 , R0 , #(2 * 4)                                  ;/*  跳过 SP, BASEPRI 的保存             */
    STM     R1 , {R4-R11}                                       ;/*  保存 R4-R11, 跳过 EXC_RETURN 的保存 */

    LDR     R1 , [R0]                                           ;/*  R1 = 异常自动压栈前使用的 SP        */
    SUB     R1 , R1 , #(ARCH_HW_SAVE_CTX_SIZE)                  ;/*  R1 = 异常自动压栈后使用的 SP        */

    ADD     R2 , R0 , #(ARCH_SW_SAVE_CTX_SIZE)

    LDM     R1 , {R4-R11}
    STM     R2 , {R4-R11}

    MOV     R1 , R0
3:
    ;/*
    ; * handle(vector, 寄存器上下文)
    ; */
    MOV     R0 , \irq
    LDR     R3 , =\handle
    BLX     R3

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数,
    ; * 注意 MSP 并没有调整, 但不会有问题(因为下次进入中断时 MSP 会首先使用异常完全压栈后的 SP)
    ; */
    LDR     R3 , =API_InterExit
    BLX     R3

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    ;/*
    ; * 从 MSP 指向的堆栈恢复原 SP, BASEPRI, R4-R11, EXC_RETURN(保存在 LR)寄存器
    ; */
    MRS     R0 , MSP
    LDM     R0 , {R2-R11, LR}
    ADD     R0 , R0 , #(ARCH_SW_SAVE_CTX_SIZE)
    MSR     MSP , R0

    MSR     BASEPRI , R3

    ;/*
    ; * 从 MSP 指向的堆栈恢复 xPSR, PC, LR, R12 以及 R3-R0 寄存器
    ; */
    BX      LR

2:
    POP     {R0 , R12}
    ;/*
    ; * 中断嵌套
    ; */
    MRS     R1 , MSP
    ADD     R0 , R1 , #(2 * 4)                                  ;/*  跳过 SP, BASEPRI 的保存             */
    STM     R0 , {R4-R11}                                       ;/*  保存 R4-R11, 跳过 EXC_RETURN 的保存 */

    B       3b

1:
    ;/*
    ; * 来到这里，说明在关中断前或者任务切换时发生抢占
    ; */
    MRS     R1 , MSP
    ADD     R0 , R1 , #(2 * 4)                                  ;/*  跳过 SP, BASEPRI 的保存             */
    STM     R0 , {R4-R11}                                       ;/*  保存 R4-R11, 跳过 EXC_RETURN 的保存 */

    ;/*
    ; * handle(vector, 寄存器上下文)
    ; */
    MOV     R0 , \irq
    LDR     R3 , =\handle
    BLX     R3

    LDR     R3 , =API_InterExitNoSched
    BLX     R3

    ;/*
    ; * 从 MSP 指向的堆栈恢复原 SP, BASEPRI, R4-R11, EXC_RETURN(保存在 LR)寄存器
    ; */
    MRS     R1 , MSP
    LDM     R1 , {R2-R11, LR}
    ADD     R1 , R1 , #(ARCH_SW_SAVE_CTX_SIZE)
    MSR     MSP , R1

    CBZ     R0 , 4f
    ;/*
    ; * Send PendSV
    ; */
    LDR     R0 , =CORTEX_M_NVIC_INT_CTRL
    LDR     R1 , =CORTEX_M_NVIC_PENDSVSET
    STR     R1 , [R0]

4:
    MSR     BASEPRI , R3
    ;/*
    ; * 从 MSP 指向的堆栈恢复 xPSR, PC, LR, R12 以及 R3-R0 寄存器
    ; */
    BX      LR
    MACRO_END()

;/*********************************************************************************************************
;  异常入口宏
;*********************************************************************************************************/

#define TRAP_ENTRY      IRQ_ENTRY

;/*********************************************************************************************************
;  SVC 入口函数
;*********************************************************************************************************/

FUNC_DEF(archSvcEntry)
    ;/*
    ; * R0 用于记录异常号
    ; */

    ;/*
    ; * 异常会自动将 xPSR, PC, LR, R12 以及 R3-R0 依次压入适当的堆栈中：
    ; * 如果异常前的代码正在使用 PSP, 则压入 PSP, 也就是使用进程堆栈；
    ; * 否则就压入 MSP, 使用主堆栈.
    ; * 一旦进入了服务例程, 就将一直使用主堆栈
    ; */

    ;/*
    ; * R3 记录 BASEPRI, 并关中断(利用优先级屏蔽中断, 才能使用 SVC)
    ; */
    MRS     R3 , BASEPRI                                        ;/*  Set prio int mask to mask all       */
    MOV     R2 , #LW_CFG_CORTEX_M_INT_MASK                      ;/*  mask prio >= 2                      */
    MSR     BASEPRI , R2

    ;/*
    ; * R0 等于异常自动压栈后使用的 SP
    ; * R2 等于异常自动压栈前使用的 SP, 通过 EXC_RETURN(保存在 LR) 判断异常前使用何种 SP
    ; */
    TST     LR , #CORTEX_M_EXC_RETURN_MODE_MASK                 ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R0 , MSP
    MRSNE   R0 , PSP
    ADD     R2 , R0 , #(ARCH_HW_SAVE_CTX_SIZE)

    ;/*
    ; * 现在用 MSP, 需要保存寄存器上下文到 MSP 指向的栈空间里, 预留 SW SAVE 寄存器上下文空间
    ; */
    MRS     R1 , MSP
    SUB     R1 , R1 , #(ARCH_SW_SAVE_CTX_SIZE)
    MSR     MSP , R1

    ;/*
    ; * 保存原 SP, BASEPRI, R4-R11, EXC_RETURN(保存在 LR)寄存器到预留 SW SAVE 寄存器上下文空间
    ; */
    STM     R1 , {R2-R11, LR}

    ;/*
    ; * armv7mSvcHandle(HW SAVE 寄存器上下文空间, SW SAVE 寄存器上下文空间)
    ; */
    LDR     R3 , =armv7mSvcHandle
    BLX     R3

    MRS     R1 , MSP
    ADD     R1 , R1 , #(ARCH_SW_SAVE_CTX_SIZE)
    MSR     MSP , R1

    ADD     R12 , R0 , #4                                       ;/*  最后将跳过 SP 的恢复                */

    LDR     R2 , [R0]                                           ;/*  读出 SP 到 R2                       */
    SUB     R2 , R2 , #(ARCH_HW_SAVE_CTX_SIZE)                  ;/*  从 SP 开辟自动出栈寄存器空间        */
    MSR     PSP , R2                                            ;/*  设置 PSP                            */

    ADD     R0 , R0 , #(ARCH_SW_SAVE_CTX_SIZE)                  ;/*  R0 指向 REG_CTX 中自动出栈寄存器    */
    LDM     R0 , {R3-R10}                                       ;/*  将 REG_CTX 中自动出栈寄存器复制到   */
    STM     R2 , {R3-R10}                                       ;/*  SP 中自动出栈寄存器空间             */

    LDM     R12 , {R3-R11, LR}                                  ;/*  恢复 BASEPRI, R4-R11, EXC_RETURN    */
    MSR     BASEPRI , R3                                        ;/*  恢复 BASEPRI                        */

    ;/*
    ; * 从 SP 指向的堆栈恢复 xPSR, PC, LR, R12 以及 R3-R0 寄存器
    ; */
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  PendSV 入口函数
;*********************************************************************************************************/

FUNC_DEF(archPendSVEntry)
    ;/*
    ; * 异常会自动将 xPSR, PC, LR, R12 以及 R3-R0 依次压入适当的堆栈中：
    ; * 如果异常前的代码正在使用 PSP, 则压入 PSP, 也就是使用进程堆栈；
    ; * 否则就压入 MSP, 使用主堆栈.
    ; * 一旦进入了服务例程, 就将一直使用主堆栈
    ; */

    ;/*
    ; * R1 记录 BASEPRI, 并关中断(利用优先级屏蔽中断, 才能使用 SVC)
    ; */
    CPSID   I
    MRS     R1 , BASEPRI                                        ;/*  Set prio int mask to mask all       */
    MOV     R2 , #LW_CFG_CORTEX_M_INT_MASK                      ;/*  mask prio >= 2                      */
    MSR     BASEPRI , R2
    CPSIE   I

    ;/*
    ; * R0 等于异常自动压栈前使用的 SP, 通过 EXC_RETURN(保存在 LR) 判断异常前使用何种 SP
    ; */
    TST     LR , #CORTEX_M_EXC_RETURN_MODE_MASK                 ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R0 , MSP
    MRSNE   R0 , PSP
    ADD     R0 , R0 , #(ARCH_HW_SAVE_CTX_SIZE)

    ;/*
    ; * API_InterEnter(异常自动压栈前使用的 SP, BASEPRI, EXC_RETURN) 进入中断,
    ; * 如果是第一次中断,   会将 异常前 SP, BASEPRI, EXC_RETURN 保存到当前任务 TCB 的 ARCH_REG_CTX 里
    ; * 如果不是第一次中断, 会将 异常前 SP, BASEPRI, EXC_RETURN 保存到上面预留的栈空间里
    ; */
    MOV     R2 , LR
    LDR     R3 , =API_InterEnter
    BLX     R3

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    LDR     R3 , =API_ThreadTcbInter
    BLX     R3

    ;/*
    ; * 保存 R4-R11 到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    ADD     R1 , R0 , #(2 * 4)                                  ;/*  跳过 SP, BASEPRI 的保存             */
    STM     R1 , {R4-R11}                                       ;/*  保存 R4-R11, 跳过 EXC_RETURN 的保存 */

    LDR     R1 , [R0]                                           ;/*  R1 = 异常自动压栈前使用的 SP        */
    SUB     R1 , R1 , #(ARCH_HW_SAVE_CTX_SIZE)                  ;/*  R1 = 异常自动压栈后使用的 SP        */

    ADD     R2 , R0 , #(ARCH_SW_SAVE_CTX_SIZE)

    LDM     R1 , {R4-R11}
    STM     R2 , {R4-R11}

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 MSP
    ; */
    LDR     R3 , =API_InterStackBaseGet
    BLX     R3
    MSR     MSP , R0

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数,
    ; * 注意 MSP 并没有调整, 但不会有问题(因为下次进入中断时 MSP 会首先使用异常完全压栈后的 SP)
    ; */
    LDR     R3 , =API_InterExit
    BLX     R3

    ;/*
    ; * 无法执行到这里，任务切换
    ; */
    BX      LR
    FUNC_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
