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
;** 文   件   名: arm64ExcAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 07 月 05 日
;**
;** 描        述: ARM64 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "arch/arm64/arch_regs.h"
#include "arm64ContextAsm.h"

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archSyncExcHandle)
    IMPORT_LABEL(archInvalidExcHandle)

    EXPORT_LABEL(archEL1IrqEntry)
    EXPORT_LABEL(archEL1SyncExcEntry)
    EXPORT_LABEL(archEL1SyncInvalidEntry)
    EXPORT_LABEL(archEL1IrqInvalidEntry)
    EXPORT_LABEL(archEL1FiqInvalidEntry)
    EXPORT_LABEL(archEL1ErrInvalidEntry)
    EXPORT_LABEL(vector)

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

FUNC_DEF(archEL1IrqEntry)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter

    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

2:
    ;/*
    ; * bspIntHandle()
    ; */
    BL      bspIntHandle

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数
    ; */
    BL      API_InterExit

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                     ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                                ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                     ;/*  使用异常堆栈                */
    B       2b
    FUNC_END()
    
;/*********************************************************************************************************
;  同步异常入口
;*********************************************************************************************************/

FUNC_DEF(archEL1SyncExcEntry)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter
            
    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f
    
    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     X18, X0
    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

    MOV     X0 , X18
2:
    ;/*
    ; * archSyncExcHandle()
    ; */
    MRS     X1 , ESR_EL1    
    BL      archSyncExcHandle

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数
    ; */
    BL      API_InterExit

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                     ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                                ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                     ;/*  使用异常堆栈                */
    B       2b
    FUNC_END()

;/*********************************************************************************************************
;  Invalid 异常入口
;*********************************************************************************************************/

MACRO_DEF(ARCH_INVALID_EXC_ENTRY type)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter

    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     X18, X0

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

    MOV     X0 , X18
2:
    ;/*
    ; * archInvalidExcHandle()
    ; */
    MRS     X1 , ESR_EL1
    MOV     X2 , \type
    BL      archInvalidExcHandle

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                     ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                                ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                     ;/*  使用异常堆栈                */
    B       2b
    MACRO_END()

FUNC_DEF(archEL1SyncInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_SYN_INVALID
    FUNC_END()

FUNC_DEF(archEL1IrqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_IRQ_INVALID
    FUNC_END()

FUNC_DEF(archEL1FiqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_FIQ_INVALID
    FUNC_END()

FUNC_DEF(archEL1ErrInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_ERR_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch64IrqEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_IRQ_AARCH64_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch64FiqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_FIQ_AARCH64_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch64ErrInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_ERR_AARCH64_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch32SyncExcEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_SYN_AARCH32_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch32IrqEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_IRQ_AARCH32_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch32FiqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_FIQ_AARCH32_INVALID
    FUNC_END()

FUNC_DEF(archEL2AArch32ErrInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL2_ERR_AARCH32_INVALID
    FUNC_END()

;/*********************************************************************************************************
;  EL2 Sync 异常处理入口
;*********************************************************************************************************/

FUNC_DEF(archEL2AArch64SyncExcEntry)
    MOV     X2 , X0                                                     ;/*  保存参数到 X2               */
    MRS     X3 , ELR_EL2                                                ;/*  记录异常返回地址            */

    MRS     X0 , ESR_EL2
    LSR     X1 , X0, #26                                                ;/*  获取 ESR 中的 EC            */

    CMP     X1 , #0x16                                                  ;/*  判断 HVC 的特征码           */
    B.NE    switchEL1                                                   ;/*  目前只处理 HVC 调用         */

    AND     X0 , X0, #0xff
    CMP     X0 , #HVC_CMD_FPU                                           ;/*  判断是否是处理 FPU 寄存器   */
    B.NE    switchEL1                                                   ;/*  如果不是，跳转回 EL1        */

    BL      arm64VfpV4El2FpuHandle                                      ;/*  进行 FPU 操作               */

LINE_LABEL(switchEL1)                                                   ;/*  返回 EL1                    */
    ;/*
    ; * 从 EL2 跳转至 EL1 AARCH64
    ; */
    MSR     ELR_EL2  , X3                                               ;/*  置回返回地址                */
    ERET
    FUNC_END()

;/*********************************************************************************************************
;  向量表定义
;*********************************************************************************************************/

MACRO_DEF(VENTRY label)
    .balign  128
    B      \label
MACRO_END()

    .balign  2048
FUNC_DEF(vector)
    VENTRY(archEL1SyncInvalidEntry)                                     ;/*  Sync  EL1t                  */
    VENTRY(archEL1IrqInvalidEntry)                                      ;/*  IRQ   EL1t                  */
    VENTRY(archEL1FiqInvalidEntry)                                      ;/*  FIQ   EL1t                  */
    VENTRY(archEL1ErrInvalidEntry)                                      ;/*  Error EL1t                  */

    VENTRY(archEL1SyncExcEntry)                                         ;/*  Sync  EL1h                  */
    VENTRY(archEL1IrqEntry)                                             ;/*  IRQ   EL1h                  */
    VENTRY(archEL1FiqInvalidEntry)                                      ;/*  FIQ   EL1h                  */
    VENTRY(archEL1ErrInvalidEntry)                                      ;/*  Error EL1h                  */

    VENTRY(archEL2AArch64SyncExcEntry)                                  ;/*  Sync  EL2 AArch64           */
    VENTRY(archEL2AArch64IrqEntry)                                      ;/*  IRQ   EL2 AArch64           */
    VENTRY(archEL2AArch64FiqInvalidEntry)                               ;/*  FIQ   EL2 AArch64           */
    VENTRY(archEL2AArch64ErrInvalidEntry)                               ;/*  Error EL2 AArch64           */

    VENTRY(archEL2AArch32SyncExcEntry)                                  ;/*  Sync  EL2 AArch32           */
    VENTRY(archEL2AArch32IrqEntry)                                      ;/*  IRQ   EL2 AArch32           */
    VENTRY(archEL2AArch32FiqInvalidEntry)                               ;/*  FIQ   EL2 AArch32           */
    VENTRY(archEL2AArch32ErrInvalidEntry)                               ;/*  Error EL2 AArch32           */
    FUNC_END()

    FILE_END()
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

