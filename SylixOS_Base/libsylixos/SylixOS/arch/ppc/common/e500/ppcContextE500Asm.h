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
** ��   ��   ��: ppcContextE500Asm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 12 ��
**
** ��        ��: PowerPC E500 ��ϵ�����������л�.
*********************************************************************************************************/

#ifndef __ARCH_PPCCONTEXTE500ASM_H
#define __ARCH_PPCCONTEXTE500ASM_H

#include "arch/ppc/arch_regs.h"

/*********************************************************************************************************
  E500 �쳣
  ʹ���쳣��ʱջ, �����쳣��ʱջ������ʱ�����ı�����, �� volatile �Ĵ������浽��ʱ�����ı�����
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_EXC_SAVE_VOLATILE)
    MTSPR   SPRG0 , SP                                                  /*  SPRG0 �ݴ��쳣ǰ SP(R1)     */
    ISYNC

    MFSPR   SP  , SPRG1                                                 /*  �����쳣��ʱ��ջ��ַ        */
    ISYNC

    SUBI    SP  , SP , ARCH_REG_CTX_SIZE                                /*  ����ʱ��ջ���������ı�����  */

    STW     R0  , XR(0)(SP)

    MFSPR   R0  , SPRG0                                                 /*  �����쳣ǰ SP(R1)           */
    ISYNC
    STW     R0  , XR(1)(SP)

    STW     R2  , XR(2)(SP)
    STW     R3  , XR(3)(SP)
    STW     R4  , XR(4)(SP)
    STW     R5  , XR(5)(SP)
    STW     R6  , XR(6)(SP)
    STW     R7  , XR(7)(SP)
    STW     R8  , XR(8)(SP)
    STW     R9  , XR(9)(SP)
    STW     R10 , XR(10)(SP)
    STW     R11 , XR(11)(SP)
    STW     R12 , XR(12)(SP)
    STW     R13 , XR(13)(SP)

    MFSPR   R0  , SRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� SRR0                   */
    SYNC

    MFSPR   R0  , SRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� SRR1                   */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  E500 �ٽ������쳣
  ʹ���쳣��ʱջ, �����쳣��ʱջ������ʱ�����ı�����, �� volatile �Ĵ������浽��ʱ�����ı�����
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_CI_EXC_SAVE_VOLATILE)
    MTSPR   SPRG0 , SP                                                  /*  SPRG0 �ݴ��쳣ǰ SP(R1)     */
    ISYNC

    MFSPR   SP  , SPRG1                                                 /*  �����쳣��ʱ��ջ��ַ        */
    ISYNC

    SUBI    SP  , SP , ARCH_REG_CTX_SIZE                                /*  ����ʱ��ջ���������ı�����  */

    STW     R0  , XR(0)(SP)

    MFSPR   R0  , SPRG0                                                 /*  �����쳣ǰ SP(R1)           */
    ISYNC
    STW     R0  , XR(1)(SP)

    STW     R2  , XR(2)(SP)
    STW     R3  , XR(3)(SP)
    STW     R4  , XR(4)(SP)
    STW     R5  , XR(5)(SP)
    STW     R6  , XR(6)(SP)
    STW     R7  , XR(7)(SP)
    STW     R8  , XR(8)(SP)
    STW     R9  , XR(9)(SP)
    STW     R10 , XR(10)(SP)
    STW     R11 , XR(11)(SP)
    STW     R12 , XR(12)(SP)
    STW     R13 , XR(13)(SP)

    MFSPR   R0  , CSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� CSRR0                  */
    SYNC

    MFSPR   R0  , CSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� CSRR1                  */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  E500 ��������쳣
  ʹ���쳣��ʱջ, �����쳣��ʱջ������ʱ�����ı�����, �� volatile �Ĵ������浽��ʱ�����ı�����
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_MC_EXC_SAVE_VOLATILE)
    MTSPR   SPRG0 , SP                                                  /*  SPRG0 �ݴ��쳣ǰ SP(R1)     */
    ISYNC

    MFSPR   SP  , SPRG1                                                 /*  �����쳣��ʱ��ջ��ַ        */
    ISYNC

    SUBI    SP  , SP , ARCH_REG_CTX_SIZE                                /*  ����ʱ��ջ���������ı�����  */

    STW     R0  , XR(0)(SP)

    MFSPR   R0  , SPRG0                                                 /*  �����쳣ǰ SP(R1)           */
    ISYNC
    STW     R0  , XR(1)(SP)

    STW     R2  , XR(2)(SP)
    STW     R3  , XR(3)(SP)
    STW     R4  , XR(4)(SP)
    STW     R5  , XR(5)(SP)
    STW     R6  , XR(6)(SP)
    STW     R7  , XR(7)(SP)
    STW     R8  , XR(8)(SP)
    STW     R9  , XR(9)(SP)
    STW     R10 , XR(10)(SP)
    STW     R11 , XR(11)(SP)
    STW     R12 , XR(12)(SP)
    STW     R13 , XR(13)(SP)

    MFSPR   R0  , MCSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� MCSRR0                 */
    SYNC

    MFSPR   R0  , MCSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� MCSRR1                 */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

#endif                                                                  /* __ARCH_PPCCONTEXTE500ASM_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
