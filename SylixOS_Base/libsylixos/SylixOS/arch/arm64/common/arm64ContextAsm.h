;/*********************************************************************************************************
;**
;**                                    �й������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: arm64ContextAsm.h
;**
;** ��   ��   ��: Wang.Xuan (���Q)
;**
;** �ļ���������: 2018 �� 06 �� 23 ��
;**
;** ��        ��: ARM64 ��ϵ�ܹ������Ĵ���.
;*********************************************************************************************************/

#ifndef __ARCH_ARM64CONTEXTASM_H
#define __ARCH_ARM64CONTEXTASM_H

#include "arch/arm64/arch_def.h"
#include "arch/arm64/arch_regs.h"

;/*********************************************************************************************************
;  ���� PSTATE ��ؼĴ���(���� X18: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_PSTATE)
    MRS     X3 , NZCV

    MRS     X2 , DAIF
    ORR     X3 , X3 , X2

    STR     X3 , [X18 , #XPSTATE_OFFSET]
    MACRO_END()

;/*********************************************************************************************************
;  �ָ� PSTATE ��ؼĴ���(���� X18: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_PSTATE)
    LDR     X1 , [X18 , #XPSTATE_OFFSET]                                ;/*  �ָ� PSTATE                 */
    BIC     X1 , X1 , #0xf
    MOV     X2 , SPSR_MODE_EL1h
    ORR     X1 , X1 , X2

    MSR     SPSR_EL1 , X1
    MACRO_END()

;/*********************************************************************************************************
;  ����Ĵ���(���� X18: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_SMALL_REG_CTX)
    MOV    X9 , #1
    STR    X9 , [X18 , #CTX_TYPE_OFFSET]                                ;/*  С����������                */

    STP    X19 , X20 , [X18 , #XGREG_OFFSET(19)]
    STP    X21 , X22 , [X18 , #XGREG_OFFSET(21)]
    STP    X23 , X24 , [X18 , #XGREG_OFFSET(23)]
    STP    X25 , X26 , [X18 , #XGREG_OFFSET(25)]
    STP    X27 , X28 , [X18 , #XGREG_OFFSET(27)]
    STP    X29 , LR  , [X18 , #XGREG_OFFSET(29)]

    SAVE_PSTATE                                                         ;/*  ���� PSTATE                 */

    MOV    X9 , SP
    STR    X9 , [X18 , #XSP_OFFSET]                                     ;/*  ���� SP ָ��                */

    STR    LR , [X18 , #XPC_OFFSET]                                     ;/*  LR ���� PC ����             */
    MACRO_END()

;/*********************************************************************************************************
;  �ָ�С�����ļĴ���(���� X18: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_SMALL_REG_CTX)
    LDR     X1 , [X18 , #XSP_OFFSET]                                    ;/*  �ָ� SP ָ��                */
    MOV     SP , X1

    RESTORE_PSTATE                                                      ;/*  �ָ� PSTATE                 */

    LDR     X1 , [X18 , #XPC_OFFSET]                                    ;/*  �ָ� PC �� ELR_EL1          */
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
;  �ָ��������ļĴ���(���� X18: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_BIG_REG_CTX)
    LDR     X1 , [X18 , #XSP_OFFSET]                                    ;/*  �ָ� SP ָ��                */
    MOV     SP , X1

    RESTORE_PSTATE                                                      ;/*  �ָ� PSTATE                 */

    LDR     X1 , [X18 , #XPC_OFFSET]                                    ;/*  �ָ� PC �� ELR_EL1          */
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
;  ʹ���쳣��ʱջ, �����쳣��ʱջ������ʱ�����ı�����, �� volatile �Ĵ������浽��ʱ�����ı�����
;*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_VOLATILE)
    MRS     X18 , TPIDR_EL1                                             ;/*  �����쳣��ʱ��ջ��ַ        */
    SUB     X18 , X18 , ARCH_REG_CTX_SIZE                               ;/*  ����ʱ��ջ���������ı�����  */

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
    STR     X2 , [X18, #XPSTATE_OFFSET]                                 ;/*  ���� PSTATE                 */

    MOV     X2 , SP
    STR     X2 , [X18, #XSP_OFFSET]                                     ;/*  ���� SP ָ��                */

    MRS     X2 , ELR_EL1
    STR     X2 , [X18, #XPC_OFFSET]

    MOV     SP , X18
    MACRO_END()

;/*********************************************************************************************************
;  ���� volatile �Ĵ���(���� X0: Ŀ�� ARCH_REG_CTX ��ַ, ���� SP: Դ ARCH_REG_CTX ��ַ)
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
;  ���� non volatile �Ĵ���(���� X0: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_NON_VOLATILE)
    MOV     X1  , #0
    STR     X1  , [X0, #CTX_TYPE_OFFSET]                                ;/*  ����Ϊ��������              */
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

