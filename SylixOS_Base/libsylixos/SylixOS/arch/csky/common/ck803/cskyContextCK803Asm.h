;/*********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: cskyContextCK803Asm.h
;**
;** ��   ��   ��: Wang.Xuan (���Q)
;**
;** �ļ���������: 2018 �� 11 �� 12 ��
;**
;** ��        ��: C-SKY CK803 ��ϵ�ܹ������Ĵ���.
;*********************************************************************************************************/

#ifndef __ARCH_CSKYCONTEXTCK803ASM_H
#define __ARCH_CSKYCONTEXTCK803ASM_H

#include "arch/csky/arch_regs.h"

;/*********************************************************************************************************
;  ����Ĵ���(���� A1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    STM         R0-R15 , (A1)

    ST.W        R28, (A1 , XGREG28)

    ST.W        RA , (A1 , XPC)                                         ;/*  RA ���� PC ����             */
    
    MFCR        A2 , PSR
    ST.W        A2 , (A1 , XPSR)                                        ;/*  ���� PSR �Ĵ���             */
    MACRO_END()

;/*********************************************************************************************************
;  �ָ��Ĵ���(���� A1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    LD.W        A0 , (A1 , XPSR)                                        ;/*  �ָ� PSR �Ĵ���             */
    MTCR        A0 , EPSR

    LD.W        A0 , (A1 , XPC)                                         ;/*  �ָ� PC �Ĵ���              */
    MTCR        A0 , EPC

    LD.W        R28, (A1 , XGREG28)

    LD.W        R0 , (A1 , XGREG(0))

    ADDI        A1 , (2 * ARCH_REG_SIZE)
    LDM         R2-R15 , (A1)
    SUBI        A1 , (2 * ARCH_REG_SIZE)
    LD.W        A1 , (A1 , XGREG(1))

    RTE
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/