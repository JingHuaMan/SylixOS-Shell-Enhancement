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
;** 文   件   名: armExcV7MPendSvAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 11 月 14 日
;**
;** 描        述: ARMv7M 体系构架异常处理(PendSV 方式任务切换).
;*********************************************************************************************************/

#ifndef __ARMEXCV7MPENDSVASM_H
#define __ARMEXCV7MPENDSVASM_H

;/*********************************************************************************************************
;  中断入口宏
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY  irq  handle)
    ;/*
    ; * 异常会自动将 xPSR, PC, LR, R12 以及 R3-R0 依次压入适当的堆栈中：
    ; * 如果异常前的代码正在使用 PSP, 则压入 PSP, 也就是使用进程堆栈；
    ; * 否则就压入 MSP, 使用主堆栈.
    ; * 一旦进入了服务例程, 就将一直使用主堆栈
    ; */
    CPSID   I                                                   ;/*  关中断                              */

    PUSH    {R0 , LR}                                           ;/*  保存 LR                             */

    LDR     R3 , =API_InterEnter                                ;/*  API_InterEnter();                   */
    BLX     R3

    ;/*
    ; * handle(vector)
    ; */
    MOV     R0 , \irq
    LDR     R3 , =\handle
    BLX     R3

    LDR     R3 , =API_InterExit                                 ;/*  API_InterExit();                    */
    BLX     R3

    POP     {R0 , LR}                                           ;/*  恢复 LR                             */

    CPSIE   I                                                   ;/*  开中断                              */
    BX      LR                                                  ;/*  异常返回,将会恢复剩余上下文         */
    MACRO_END()

;/*********************************************************************************************************
;  异常入口宏
;*********************************************************************************************************/

MACRO_DEF(TRAP_ENTRY  irq  handle)
    ;/*
    ; * 异常会自动将 xPSR, PC, LR, R12 以及 R3-R0 依次压入适当的堆栈中：
    ; * 如果异常前的代码正在使用 PSP, 则压入 PSP, 也就是使用进程堆栈；
    ; * 否则就压入 MSP, 使用主堆栈.
    ; * 一旦进入了服务例程, 就将一直使用主堆栈
    ; */
    CPSID   I                                                   ;/*  关中断                              */

    TST     LR , #CORTEX_M_EXC_RETURN_MODE_MASK                 ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R1 , MSP
    MRSNE   R1 , PSP                                            ;/*  读出异常前的 SP                     */
    LDR     R0 , [R1 , #24]                                     ;/*  读出异常时的 PC                     */

    PUSH    {R0 , LR}                                           ;/*  保存异常时的 PC 和 LR               */

    LDR     R3 , =API_InterEnter                                ;/*  API_InterEnter();                   */
    BLX     R3

    ;/*
    ; * handle(vector, 异常时的 PC)
    ; */
    MOV     R0 , \irq
    LDR     R1 , [SP]
    LDR     R3 , =\handle
    BLX     R3

    LDR     R3 , =API_InterExit                                 ;/*  API_InterExit();                    */
    BLX     R3

    POP     {R0 , LR}                                           ;/*  恢复 LR                             */

    CPSIE   I                                                   ;/*  开中断                              */
    BX      LR                                                  ;/*  异常返回,将会恢复剩余上下文         */
    MACRO_END()

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
    CPSID   I                                                   ;/*  关中断                              */

    ;/*
    ; * PSP 是进程堆栈指针, 如果为 0, 说明是启动操作系统多任务, 将跳转上下文保存
    ; */
    MRS     R0 , PSP
    CBZ     R0 , archPendSvNoSave

    ;/*
    ; * R0 等于异常自动压栈前使用的 SP, 通过 EXC_RETURN(保存在 LR) 判断异常前使用何种 SP
    ; */
    TST     LR , #CORTEX_M_EXC_RETURN_MODE_MASK                 ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R0 , MSP
    MRSNE   R0 , PSP
    ADD     R0 , R0 , #(ARCH_HW_SAVE_CTX_SIZE)

    ;/*
    ; * archPendSvSaveReg(异常自动压栈前使用的 SP, EXC_RETURN) 进入中断,
    ; * 会将 异常前 SP, EXC_RETURN 保存到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    MOV     R1 , LR
    LDR     R3 , =archPendSvSaveReg
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
    ; * 获得当前 CPU 结构指针, 并暂存到 R4
    ; */
    LDR     R3 , =_CpuGetCur
    BLX     R3
    MOV     R4 , R0

    ;/*
    ; * 调用 _SchedSwp();
    ; */
    LDR     R3 , =_SchedSwp
    BLX     R3

archPendSvcCtxLoad:
    LDR     R0 , [R4]                                           ;/*  获取当前 TCB 的 REG_CTX 地址        */
    ADD     R12 , R0 , #4                                       ;/*  最后将跳过 SP 的恢复                */

    LDR     R2 , [R0]                                           ;/*  读出 SP 到 R2                       */
    SUB     R2 , R2 , #(ARCH_HW_SAVE_CTX_SIZE)                  ;/*  从 SP 开辟自动出栈寄存器空间        */
    MSR     PSP , R2                                            ;/*  设置 PSP                            */

    ADD     R0 , R0 , #(ARCH_SW_SAVE_CTX_SIZE)                  ;/*  R0 指向 REG_CTX 中自动出栈寄存器    */
    LDM     R0 , {R3-R10}                                       ;/*  将 REG_CTX 中自动出栈寄存器复制到   */
    STM     R2 , {R3-R10}                                       ;/*  SP 中自动出栈寄存器空间             */

    LDM     R12 , {R3-R11, LR}                                  ;/*  恢复 BASEPRI, R4-R11, EXC_RETURN    */

    CPSIE   I                                                   ;/*  开中断                              */

    ;/*
    ; * 从 SP 指向的堆栈恢复 xPSR, PC, LR, R12 以及 R3-R0 寄存器
    ; */
    BX      LR

archPendSvNoSave:
    ;/*
    ; * 获得当前 CPU 中断堆栈栈顶, 并设置 MSP
    ; */
    LDR     R3 , =API_InterStackBaseGet
    BLX     R3
    MSR     MSP , R0

    LDR     R3 , =_CpuGetCur
    BLX     R3
    MOV     R4 , R0

    B       archPendSvcCtxLoad
    FUNC_END()

;/*********************************************************************************************************
;  SVC 入口函数
;*********************************************************************************************************/

FUNC_DEF(archSvcEntry)
    TRAP_ENTRY  11  armv7mSvcHandle
    FUNC_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
