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
;** ��   ��   ��: mipsContextAsm.h
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2015 �� 09 �� 01 ��
;**
;** ��        ��: MIPS ��ϵ�ܹ������Ĵ���.
;*********************************************************************************************************/

#ifndef __ARCH_MIPSCONTEXTASM_H
#define __ARCH_MIPSCONTEXTASM_H

#include "arch/mips/arch_regs.h"

;/*********************************************************************************************************
;  ���� $0 - $31 �Ĵ���(���� T9: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_GREGS)
    .set    push
    .set    noat

    REG_S       $0  , XGREG(0)(T9)
    REG_S       $1  , XGREG(1)(T9)
    REG_S       $2  , XGREG(2)(T9)
    REG_S       $3  , XGREG(3)(T9)
    REG_S       $4  , XGREG(4)(T9)
    REG_S       $5  , XGREG(5)(T9)
    REG_S       $6  , XGREG(6)(T9)
    REG_S       $7  , XGREG(7)(T9)
    REG_S       $8  , XGREG(8)(T9)
    REG_S       $9  , XGREG(9)(T9)
    REG_S       $10 , XGREG(10)(T9)
    REG_S       $11 , XGREG(11)(T9)
    REG_S       $12 , XGREG(12)(T9)
    REG_S       $13 , XGREG(13)(T9)
    REG_S       $14 , XGREG(14)(T9)
    REG_S       $15 , XGREG(15)(T9)
    REG_S       $16 , XGREG(16)(T9)
    REG_S       $17 , XGREG(17)(T9)
    REG_S       $18 , XGREG(18)(T9)
    REG_S       $19 , XGREG(19)(T9)
    REG_S       $20 , XGREG(20)(T9)
    REG_S       $21 , XGREG(21)(T9)
    REG_S       $22 , XGREG(22)(T9)
    REG_S       $23 , XGREG(23)(T9)
    REG_S       $24 , XGREG(24)(T9)
    REG_S       $25 , XGREG(25)(T9)
    ;/*
    ; * $26 $27 �� K0 K1
    ; */
    REG_S       $28 , XGREG(28)(T9)
    REG_S       $29 , XGREG(29)(T9)
    REG_S       $30 , XGREG(30)(T9)
    REG_S       $31 , XGREG(31)(T9)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  �ָ� $0 - $31 �Ĵ���(���� T9: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_GREGS)
    .set    push
    .set    noat

    ;/*
    ; * $0 �̶�Ϊ 0
    ; */
    REG_L       $1  , XGREG(1)(T9)
    REG_L       $2  , XGREG(2)(T9)
    REG_L       $3  , XGREG(3)(T9)
    REG_L       $4  , XGREG(4)(T9)
    REG_L       $5  , XGREG(5)(T9)
    REG_L       $6  , XGREG(6)(T9)
    REG_L       $7  , XGREG(7)(T9)
    REG_L       $8  , XGREG(8)(T9)
    REG_L       $9  , XGREG(9)(T9)
    REG_L       $10 , XGREG(10)(T9)
    REG_L       $11 , XGREG(11)(T9)
    REG_L       $12 , XGREG(12)(T9)
    REG_L       $13 , XGREG(13)(T9)
    REG_L       $14 , XGREG(14)(T9)
    REG_L       $15 , XGREG(15)(T9)
    REG_L       $16 , XGREG(16)(T9)
    REG_L       $17 , XGREG(17)(T9)
    REG_L       $18 , XGREG(18)(T9)
    REG_L       $19 , XGREG(19)(T9)
    REG_L       $20 , XGREG(20)(T9)
    REG_L       $21 , XGREG(21)(T9)
    REG_L       $22 , XGREG(22)(T9)
    REG_L       $23 , XGREG(23)(T9)
    ;/*
    ; * $24 $25 �� T8 T9(�����ָ�), O32 �� N64 ABI, T8 T9 ���� $24 $25
    ; */
    ;/*
    ; * $26 $27 �� K0 K1
    ; */
    REG_L       $28 , XGREG(28)(T9)
    REG_L       $29 , XGREG(29)(T9)
    REG_L       $30 , XGREG(30)(T9)
    REG_L       $31 , XGREG(31)(T9)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  ����Ĵ���(���� T9: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    .set    push
    .set    noat

    SAVE_GREGS                                                          ;/*  ���� $0 - $31 �Ĵ���        */

    REG_S       RA , XEPC(T9)                                           ;/*  RA ���� EPC ����            */

    MFC0_EHB(T1, CP0_STATUS)                                            ;/*  ���� STATUS �Ĵ���          */
    REG_S       T1 , XSR(T9)

    MFC0_LONG_EHB(T1, CP0_BADVADDR)                                     ;/*  ���� BADVADDR �Ĵ���        */
    REG_S       T1 , XBADVADDR(T9)

    MFC0_EHB(T1, CP0_CAUSE)                                             ;/*  ���� CAUSE �Ĵ���           */
    REG_S       T1 , XCAUSE(T9)

    MFLO        T1                                                      ;/*  ���� LO �Ĵ���              */
    REG_S       T1 , XLO(T9)

    MFHI        T1                                                      ;/*  ���� HI �Ĵ���              */
    REG_S       T1 , XHI(T9)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  �ָ��Ĵ���(���� T9: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    .set    push
    .set    noat

    RESTORE_GREGS                                                       ;/*  �ָ� $0 - $31 �Ĵ���        */

    REG_L       T8 , XLO(T9)                                            ;/*  �ָ� LO �Ĵ���              */
    MTLO        T8

    REG_L       T8 , XHI(T9)                                            ;/*  �ָ� HI �Ĵ���              */
    MTHI        T8

    ;/*
    ; * O32 �� N64 ABI, T8 T9 ���� $24 $25
    ; */

    REG_L       K0 , XSR(T9)                                            ;/*  �ָ� SR  �Ĵ���             */
    REG_L       K1 , XEPC(T9)                                           ;/*  �ָ� EPC �Ĵ���             */

    REG_L       T8 , XGREG(24)(T9)                                      ;/*  �ָ� T8 �Ĵ���              */
    REG_L       T9 , XGREG(25)(T9)                                      ;/*  �ָ� T9 �Ĵ���              */

    MTC0_LONG_EHB(K1, CP0_EPC)                                          ;/*  �ָ� EPC �Ĵ���             */

    ORI         K0 , K0 , ST0_EXL                                       ;/*  ͨ������ EXL λ             */
    MTC0_EHB(K0 , CP0_STATUS)                                           ;/*  �����ں�ģʽ�������ж�      */

    ERET                                                                ;/*  ��� EXL λ������           */
    NOP

    .set    pop
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
