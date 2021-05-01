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
;** 文   件   名: arm64ContextAsm.h
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 06 月 23 日
;**
;** 描        述: ARM64 体系架构上下文处理.
;*********************************************************************************************************/

#ifndef __ARCH_ARM64CONTEXTASM_H
#define __ARCH_ARM64CONTEXTASM_H

#include "arch/arm64/arch_def.h"
#include "arch/arm64/arch_regs.h"

;/*********************************************************************************************************
;  保存 PSTATE 相关寄存器(参数 X18: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(SAVE_PSTATE)
    MRS     X3 , NZCV

    MRS     X2 , DAIF
    ORR     X3 , X3 , X2

    STR     X3 , [X18 , #XPSTATE_OFFSET]
    MACRO_END()

;/*********************************************************************************************************
;  恢复 PSTATE 相关寄存器(参数 X18: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_PSTATE)
    LDR     X1 , [X18 , #XPSTATE_OFFSET]                                ;/*  恢复 PSTATE                 */
    BIC     X1 , X1 , #0xf
    MOV     X2 , SPSR_MODE_EL1h
    ORR     X1 , X1 , X2

    MSR     SPSR_EL1 , X1
    MACRO_END()

;/*********************************************************************************************************
;  保存寄存器(参数 X18: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(SAVE_SMALL_REG_CTX)
    MOV    X9 , #1
    STR    X9 , [X18 , #CTX_TYPE_OFFSET]                                ;/*  小上下文类型                */

    STP    X19 , X20 , [X18 , #XGREG_OFFSET(19)]
    STP    X21 , X22 , [X18 , #XGREG_OFFSET(21)]
    STP    X23 , X24 , [X18 , #XGREG_OFFSET(23)]
    STP    X25 , X26 , [X18 , #XGREG_OFFSET(25)]
    STP    X27 , X28 , [X18 , #XGREG_OFFSET(27)]
    STP    X29 , LR  , [X18 , #XGREG_OFFSET(29)]

    SAVE_PSTATE                                                         ;/*  保存 PSTATE                 */

    MOV    X9 , SP
    STR    X9 , [X18 , #XSP_OFFSET]                                     ;/*  保存 SP 指针                */

    STR    LR , [X18 , #XPC_OFFSET]                                     ;/*  LR 代替 PC 保存             */
    MACRO_END()

;/*********************************************************************************************************
;  恢复小上下文寄存器(参数 X18: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_SMALL_REG_CTX)
    LDR     X1 , [X18 , #XSP_OFFSET]                                    ;/*  恢复 SP 指针                */
    MOV     SP , X1

    RESTORE_PSTATE                                                      ;/*  恢复 PSTATE                 */

    LDR     X1 , [X18 , #XPC_OFFSET]                                    ;/*  恢复 PC 到 ELR_EL1          */
    MSR     ELR_EL1 , X1

    LDP     X0  , X1  , [X18 , #XGREG_OFFSET(0) ]
    LDP     X19 , X20 , [X18 , #XGREG_OFFSET(19)]
    LDP     X21 , X22 , [X18 , #XGREG_OFFSET(21)]
    LDP     X23 , X24 , [X18 , #XGREG_OFFSET(23)]
    LDP     X25 , X26 , [X18 , #XGREG_OFFSET(25)]
    LDP     X27 , X28 , [X18 , #XGREG_OFFSET(27)]
    LDP     X29 , LR  , [X18 , #XGREG_OFFSET(29)]

#if LW_CFG_ARM64_FAST_TCB_CUR > 0
    LDR     X18 ,       [X18 , #XGREG_OFFSET(18)]
#endif

    ERET
    MACRO_END()

;/*********************************************************************************************************
;  恢复大上下文寄存器(参数 X18: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_BIG_REG_CTX)
    LDR     X1 , [X18 , #XSP_OFFSET]                                    ;/*  恢复 SP 指针                */
    MOV     SP , X1

    RESTORE_PSTATE                                                      ;/*  恢复 PSTATE                 */

    LDR     X1 , [X18 , #XPC_OFFSET]                                    ;/*  恢复 PC 到 ELR_EL1          */
    MSR     ELR_EL1 , X1

    LDP     X0  , X1  ,  [X18 , #XGREG_OFFSET(0)]
    LDP     X2  , X3  ,  [X18 , #XGREG_OFFSET(2)]
    LDP     X4  , X5  ,  [X18 , #XGREG_OFFSET(4)]
    LDP     X6  , X7  ,  [X18 , #XGREG_OFFSET(6)]
    LDP     X8  , X9  ,  [X18 , #XGREG_OFFSET(8)]
    LDP     X10 , X11 ,  [X18 , #XGREG_OFFSET(10)]
    LDP     X12 , X13 ,  [X18 , #XGREG_OFFSET(12)]
    LDP     X14 , X15 ,  [X18 , #XGREG_OFFSET(14)]
    LDP     X16 , X17 ,  [X18 , #XGREG_OFFSET(16)]
    LDP     X19 , X20 ,  [X18 , #XGREG_OFFSET(19)]
    LDP     X21 , X22 ,  [X18 , #XGREG_OFFSET(21)]
    LDP     X23 , X24 ,  [X18 , #XGREG_OFFSET(23)]
    LDP     X25 , X26 ,  [X18 , #XGREG_OFFSET(25)]
    LDP     X27 , X28 ,  [X18 , #XGREG_OFFSET(27)]
    LDP     X29 , LR  ,  [X18 , #XGREG_OFFSET(29)]
    LDR     X18 ,        [X18 , #XGREG_OFFSET(18)]

    ERET
    MACRO_END()

;/*********************************************************************************************************
;  使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
;*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_VOLATILE)
    MRS     X18 , TPIDR_EL1                                             ;/*  读出异常临时堆栈地址        */
    SUB     X18 , X18 , ARCH_REG_CTX_SIZE                               ;/*  在临时堆栈开辟上下文保存区  */

    STP     X0  , X1  ,  [X18 , #XGREG_OFFSET(0)]
    STP     X2  , X3  ,  [X18 , #XGREG_OFFSET(2)]
    STP     X4  , X5  ,  [X18 , #XGREG_OFFSET(4)]
    STP     X6  , X7  ,  [X18 , #XGREG_OFFSET(6)]
    STP     X8  , X9  ,  [X18 , #XGREG_OFFSET(8)]
    STP     X10 , X11 ,  [X18 , #XGREG_OFFSET(10)]
    STP     X12 , X13 ,  [X18 , #XGREG_OFFSET(12)]
    STP     X14 , X15 ,  [X18 , #XGREG_OFFSET(14)]
    STP     X16 , X17 ,  [X18 , #XGREG_OFFSET(16)]
    STP     X29 , LR  ,  [X18 , #XGREG_OFFSET(29)]

    MRS     X2 , SPSR_EL1
    STR     X2 , [X18, #XPSTATE_OFFSET]                                 ;/*  保存 PSTATE                 */

    MOV     X2 , SP
    STR     X2 , [X18, #XSP_OFFSET]                                     ;/*  保存 SP 指针                */

    MRS     X2 , ELR_EL1
    STR     X2 , [X18, #XPC_OFFSET]

    MOV     SP , X18
    MACRO_END()

;/*********************************************************************************************************
;  拷贝 volatile 寄存器(参数 X0: 目的 ARCH_REG_CTX 地址, 参数 SP: 源 ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(EXC_COPY_VOLATILE)
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(0)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(0)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(2)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(2)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(4)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(4)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(6)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(6)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(8)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(8)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(10)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(10)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(12)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(12)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(14)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(14)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(16)]
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(16)]
    LDP     X9  , X10 ,  [SP , #XGREG_OFFSET(29)]                       ;/*  X29, LR                     */
    STP     X9  , X10 ,  [X0 , #XGREG_OFFSET(29)]
    LDP     X9  , X10 ,  [SP , #XSP_OFFSET]                             ;/*  SP, PC                      */
    STP     X9  , X10 ,  [X0 , #XSP_OFFSET]
    LDR     X9  ,        [SP , #XPSTATE_OFFSET]                         ;/*  PSTATE                      */
    STR     X9  ,        [X0 , #XPSTATE_OFFSET]
    MACRO_END()

;/*********************************************************************************************************
;  保存 non volatile 寄存器(参数 X0: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_NON_VOLATILE)
    MOV     X1  , #0
    STR     X1  , [X0, #CTX_TYPE_OFFSET]                                ;/*  设置为大上下文              */
    STP     X19 , X20 , [X0 , #XGREG_OFFSET(19)]
    STP     X21 , X22 , [X0 , #XGREG_OFFSET(21)]
    STP     X23 , X24 , [X0 , #XGREG_OFFSET(23)]
    STP     X25 , X26 , [X0 , #XGREG_OFFSET(25)]
    STP     X27 , X28 , [X0 , #XGREG_OFFSET(27)]
    MACRO_END()

#endif                                                                   /*  __ARCH_ARM64CONTEXTASM_H    */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

