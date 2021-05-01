/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mipsExcAsm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ��쳣����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSEXCASM_H
#define __ARCH_MIPSEXCASM_H

    IMPORT_LABEL(archInterruptEntry)
    IMPORT_LABEL(archCacheErrorEntry)
    IMPORT_LABEL(archExceptionEntry)
    IMPORT_LABEL(mipsMmuTlbRefillEntry)

;/*********************************************************************************************************
;  CACHE �����쳣�����ת
;*********************************************************************************************************/

MACRO_DEF(MIPS_CACHE_ERROR_HANDLE)
    .set    push
    .set    noat

    J       archCacheErrorEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  ͨ���쳣�����ת
;*********************************************************************************************************/

MACRO_DEF(MIPS_EXCEPTION_HANDLE)
    .set    push
    .set    noat

    J       archExceptionEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  �ж������ת
;*********************************************************************************************************/

MACRO_DEF(MIPS_INTERRUPT_HANDLE)
    .set    push
    .set    noat

    J       archInterruptEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  TLB ���������ת
;*********************************************************************************************************/

MACRO_DEF(MIPS_TLB_REFILL_HANDLE)
    .set    push
    .set    noat

#if LW_CFG_CPU_WORD_LENGHT == 64
    DMFC0   K0 , CP0_CONTEXT                                            ;/* CP0_XCONTEXT =  CP0_CONTEXT  */
    MIPS_EHB
    DMTC0   K0 , CP0_XCONTEXT
    MIPS_EHB
#endif

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  XTLB ���������ת
;*********************************************************************************************************/

#if LW_CFG_CPU_WORD_LENGHT == 64
MACRO_DEF(MIPS_XTLB_REFILL_HANDLE)
    .set    push
    .set    noat

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()
#endif

;/*********************************************************************************************************
;  TLB ���������ת(�� BSP ���ݽӿ�, �� BSP ����ʹ�� MIPS_TLB_REFILL_HANDLE ��)
;*********************************************************************************************************/

MACRO_DEF(MIPS32_TLB_REFILL_HANDLE)
    .set    push
    .set    noat

#if LW_CFG_CPU_WORD_LENGHT == 64
    DMFC0   K0 , CP0_CONTEXT                                            ;/* CP0_XCONTEXT =  CP0_CONTEXT  */
    MIPS_EHB
    DMTC0   K0 , CP0_XCONTEXT
    MIPS_EHB
#endif

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
