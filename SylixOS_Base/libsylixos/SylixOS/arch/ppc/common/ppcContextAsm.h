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
** ��   ��   ��: ppcContextAsm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 17 ��
**
** ��        ��: PowerPC ��ϵ�����������л�.
*********************************************************************************************************/

#ifndef __ARCH_PPCCONTEXTASM_H
#define __ARCH_PPCCONTEXTASM_H

#include <config/kernel/vmm_cfg.h>
#include "arch/ppc/arch_regs.h"

/*********************************************************************************************************
  ����Ĵ���(���� R4: ARCH_REG_CTX ��ַ)
*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    STW     R0  ,  XR(0)(R4)                                            /*  ���� R0 - R31               */
    STW     R1  ,  XR(1)(R4)
    STW     R2  ,  XR(2)(R4)
    STW     R3  ,  XR(3)(R4)
    STW     R4  ,  XR(4)(R4)
    STW     R5  ,  XR(5)(R4)
    STW     R6  ,  XR(6)(R4)
    STW     R7  ,  XR(7)(R4)
    STW     R8  ,  XR(8)(R4)
    STW     R9  ,  XR(9)(R4)
    STW     R10 , XR(10)(R4)
    STW     R11 , XR(11)(R4)
    STW     R12 , XR(12)(R4)
    STW     R13 , XR(13)(R4)
    STW     R14 , XR(14)(R4)
    STW     R15 , XR(15)(R4)
    STW     R16 , XR(16)(R4)
    STW     R17 , XR(17)(R4)
    STW     R18 , XR(18)(R4)
    STW     R19 , XR(19)(R4)
    STW     R20 , XR(20)(R4)
    STW     R21 , XR(21)(R4)
    STW     R22 , XR(22)(R4)
    STW     R23 , XR(23)(R4)
    STW     R24 , XR(24)(R4)
    STW     R25 , XR(25)(R4)
    STW     R26 , XR(26)(R4)
    STW     R27 , XR(27)(R4)
    STW     R28 , XR(28)(R4)
    STW     R29 , XR(29)(R4)
    STW     R30 , XR(30)(R4)
    STW     R31 , XR(31)(R4)

    MFLR    R0                                                          /*  LR ���� SRR0 ������         */
    ISYNC
    STW     R0  , XSRR0(R4)
    SYNC

    MFMSR   R0                                                          /*  MSR ���� SRR1 ������        */
    ISYNC
    STW     R0  , XSRR1(R4)
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(R4)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(R4)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(R4)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(R4)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  �ָ��Ĵ���(���� R4: ARCH_REG_CTX ��ַ)
*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    LWZ     R1  ,  XR(1)(R4)                                            /*  �ָ� R1 - R31               */
    LWZ     R2  ,  XR(2)(R4)
    LWZ     R3  ,  XR(3)(R4)
    LWZ     R5  ,  XR(5)(R4)
    LWZ     R6  ,  XR(6)(R4)
    LWZ     R7  ,  XR(7)(R4)
    LWZ     R8  ,  XR(8)(R4)
    LWZ     R9  ,  XR(9)(R4)
    LWZ     R10 , XR(10)(R4)
    LWZ     R11 , XR(11)(R4)
    LWZ     R12 , XR(12)(R4)
    LWZ     R13 , XR(13)(R4)
    LWZ     R14 , XR(14)(R4)
    LWZ     R15 , XR(15)(R4)
    LWZ     R16 , XR(16)(R4)
    LWZ     R17 , XR(17)(R4)
    LWZ     R18 , XR(18)(R4)
    LWZ     R19 , XR(19)(R4)
    LWZ     R20 , XR(20)(R4)
    LWZ     R21 , XR(21)(R4)
    LWZ     R22 , XR(22)(R4)
    LWZ     R23 , XR(23)(R4)
    LWZ     R24 , XR(24)(R4)
    LWZ     R25 , XR(25)(R4)
    LWZ     R26 , XR(26)(R4)
    LWZ     R27 , XR(27)(R4)
    LWZ     R28 , XR(28)(R4)
    LWZ     R29 , XR(29)(R4)
    LWZ     R30 , XR(30)(R4)
    LWZ     R31 , XR(31)(R4)

    LWZ     R0  , XCR(R4)                                               /*  �ָ� CR                     */
    SYNC
    MTCR    R0
    ISYNC

    LWZ     R0  , XXER(R4)                                              /*  �ָ� XER                    */
    SYNC
    MTXER   R0
    ISYNC

    LWZ     R0  , XCTR(R4)                                              /*  �ָ� CTR                    */
    SYNC
    MTCTR   R0
    ISYNC

    LWZ     R0  , XLR(R4)                                               /*  �ָ� LR                     */
    SYNC
    MTLR    R0
    ISYNC

    LWZ     R0  , XSRR1(R4)                                             /*  �ָ� SRR1                   */
    SYNC
    MTSPR   SRR1, R0
    ISYNC

    LWZ     R0  , XSRR0(R4)                                             /*  �ָ� SRR0                   */
    SYNC
    MTSPR   SRR0, R0
    ISYNC

    LWZ     R0  , XR(0)(R4)                                             /*  �ָ� R0                     */
    LWZ     R4  , XR(4)(R4)                                             /*  �ָ� R4                     */
    SYNC

    RFI                                                                 /*  �� SRR0 ����, ͬʱ MSR=SRR1 */
    MACRO_END()

/*********************************************************************************************************
  ʹ�� MMU
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
MACRO_DEF(ENABLE_MMU)
    MTSPR   SPRG0 , R3                                                  /*  SPRG0 �ݴ� R3               */
    ISYNC

    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_DR | ARCH_PPC_MSR_IR                 /*  ʹ�� DR �� IR λ            */
    ISYNC
    MTMSR   R3
    ISYNC

    MFSPR   R3 , SPRG0                                                  /*  �ָ� R3                     */
    ISYNC
    MACRO_END()
#else
MACRO_DEF(ENABLE_MMU)
    MACRO_END()
#endif

/*********************************************************************************************************
  ʹ���쳣��ʱջ, �����쳣��ʱջ������ʱ�����ı�����, �� volatile �Ĵ������浽��ʱ�����ı�����
*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_VOLATILE)
    ENABLE_MMU

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
  ���� non volatile �Ĵ���(���� R3: ARCH_REG_CTX ��ַ)
*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_NON_VOLATILE)
    STW     R14 , XR(14)(R3)
    STW     R15 , XR(15)(R3)
    STW     R16 , XR(16)(R3)
    STW     R17 , XR(17)(R3)
    STW     R18 , XR(18)(R3)
    STW     R19 , XR(19)(R3)
    STW     R20 , XR(20)(R3)
    STW     R21 , XR(21)(R3)
    STW     R22 , XR(22)(R3)
    STW     R23 , XR(23)(R3)
    STW     R24 , XR(24)(R3)
    STW     R25 , XR(25)(R3)
    STW     R26 , XR(26)(R3)
    STW     R27 , XR(27)(R3)
    STW     R28 , XR(28)(R3)
    STW     R29 , XR(29)(R3)
    STW     R30 , XR(30)(R3)
    STW     R31 , XR(31)(R3)
    MACRO_END()

/*********************************************************************************************************
  ���� volatile �Ĵ���(���� R3: Ŀ�� ARCH_REG_CTX ��ַ, ���� SP: Դ ARCH_REG_CTX ��ַ)
*********************************************************************************************************/

MACRO_DEF(EXC_COPY_VOLATILE)
    LWZ     R0  , XR(0)(SP)
    STW     R0  , XR(0)(R3)

    LWZ     R0  , XR(1)(SP)
    STW     R0  , XR(1)(R3)

    LWZ     R0  , XR(2)(SP)
    STW     R0  , XR(2)(R3)

    LWZ     R0  , XR(3)(SP)
    STW     R0  , XR(3)(R3)

    LWZ     R0  , XR(4)(SP)
    STW     R0  , XR(4)(R3)

    LWZ     R0  , XR(5)(SP)
    STW     R0  , XR(5)(R3)

    LWZ     R0  , XR(6)(SP)
    STW     R0  , XR(6)(R3)

    LWZ     R0  , XR(7)(SP)
    STW     R0  , XR(7)(R3)

    LWZ     R0  , XR(8)(SP)
    STW     R0  , XR(8)(R3)

    LWZ     R0  , XR(9)(SP)
    STW     R0  , XR(9)(R3)

    LWZ     R0  , XR(10)(SP)
    STW     R0  , XR(10)(R3)

    LWZ     R0  , XR(11)(SP)
    STW     R0  , XR(11)(R3)

    LWZ     R0  , XR(12)(SP)
    STW     R0  , XR(12)(R3)

    LWZ     R0  , XR(13)(SP)
    STW     R0  , XR(13)(R3)

    LWZ     R0  , XSRR0(SP)                                             /*  ���� SRR0                   */
    STW     R0  , XSRR0(R3)

    LWZ     R0  , XSRR1(SP)                                             /*  ���� SRR1                   */
    STW     R0  , XSRR1(R3)

    LWZ     R0  , XLR(SP)                                               /*  ���� LR                     */
    STW     R0  , XLR(R3)

    LWZ     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    STW     R0  , XCTR(R3)

    LWZ     R0  , XXER(SP)                                              /*  ���� XER                    */
    STW     R0  , XXER(R3)

    LWZ     R0  , XCR(SP)                                               /*  ���� CR                     */
    STW     R0  , XCR(R3)
    MACRO_END()

#endif                                                                  /*  __ARCH_PPCCONTEXTASM_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
