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
;** ��   ��   ��: sparcContextAsm.h
;**
;** ��   ��   ��: Xu.Guizhou (�����)
;**
;** �ļ���������: 2017 �� 05 �� 15 ��
;**
;** ��        ��: SPARC ��ϵ�����������л�.
;*********************************************************************************************************/

#ifndef __ARCH_SPARC_CTX_ASM_H
#define __ARCH_SPARC_CTX_ASM_H

;/*********************************************************************************************************
;  ����Ĵ���(���� %g1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    ST      %g1  , [%g1 + REG_GLOBAL(1)]                                ;/*  ����ȫ�ּĴ���(���� %g0)    */
    STD     %g2  , [%g1 + REG_GLOBAL(2)]
    SPARC_B2BST_NOP
    STD     %g4  , [%g1 + REG_GLOBAL(4)]
    SPARC_B2BST_NOP
    STD     %g6  , [%g1 + REG_GLOBAL(6)]
    SPARC_B2BST_NOP

    STD     %l0  , [%g1 + REG_LOCAL(0)]                                 ;/*  ���汾�ؼĴ���              */
    SPARC_B2BST_NOP
    STD     %l2  , [%g1 + REG_LOCAL(2)]
    SPARC_B2BST_NOP
    STD     %l4  , [%g1 + REG_LOCAL(4)]
    SPARC_B2BST_NOP
    STD     %l6  , [%g1 + REG_LOCAL(6)]
    SPARC_B2BST_NOP

    STD     %i0  , [%g1 + REG_INPUT(0)]                                 ;/*  ��������Ĵ���              */
    SPARC_B2BST_NOP
    STD     %i2  , [%g1 + REG_INPUT(2)]
    SPARC_B2BST_NOP
    STD     %i4  , [%g1 + REG_INPUT(4)]
    SPARC_B2BST_NOP
    STD     %i6  , [%g1 + REG_INPUT(6)]
    SPARC_B2BST_NOP

    STD     %o0  , [%g1 + REG_OUTPUT(0)]                                ;/*  ��������Ĵ���              */
    SPARC_B2BST_NOP
    STD     %o2  , [%g1 + REG_OUTPUT(2)]
    SPARC_B2BST_NOP
    STD     %o4  , [%g1 + REG_OUTPUT(4)]
    SPARC_B2BST_NOP
    STD     %o6  , [%g1 + REG_OUTPUT(6)]

    RD      %y   , %g2
    ST      %g2  , [%g1 + REG_Y]                                        ;/*  ���� Y �Ĵ���               */

    ADD     %o7  , 0x8  , %g2                                           ;/*  %o7 = CALL ָ��ĵ�ַ       */
    ST      %g2  , [%g1 + REG_PC]                                       ;/*  ���� PC (��ʱ�۵���һ��ָ��)*/

    ADD     %g2  , 0x4  , %g2                                           ;/*  Next PC = PC + 4            */
    ST      %g2  , [%g1 + REG_NPC]                                      ;/*  ���� Next PC                */

    RD      %psr , %g2
    ST      %g2  , [%g1 + REG_PSR]                                      ;/*  ���� PSR ״̬�Ĵ���         */
    MACRO_END()

;/*********************************************************************************************************
;  �ָ��Ĵ���(���� %g1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    LD      [%g1 + REG_PSR] , %g2                                       ;/*  %g2 = ���ָ��� %psr         */
    ANDN    %g2  , PSR_ET   , %g2                                       ;/*  �����쳣                    */
    WR      %g2  , %psr                                                 ;/*  �ָ� %psr                   */
    NOP
    NOP
    NOP

    AND     %g2  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    ADD     %g3  , 1 , %g3                                              ;/*  %g3 = (CWP + 1) % NWIN      */
    AND     %g3  , LW_CFG_SPARC_REG_WIN_NR - 1 , %g3

    MOV     1    , %g4
    SLL     %g4  , %g3 , %g4                                            ;/*  %wim = %g4 = 1 << %g3       */
    WR      %g4  , %wim
    NOP
    NOP
    NOP

    LD      [%g1 + REG_Y] , %g2
    WR      %g2  , %y                                                   ;/*  �ָ� Y �Ĵ���               */

    LDD     [%g1 + REG_GLOBAL(2)] , %g2                                 ;/*  �ָ�ȫ�ּĴ���              */
    LDD     [%g1 + REG_GLOBAL(4)] , %g4
    LDD     [%g1 + REG_GLOBAL(6)] , %g6

    LDD     [%g1 + REG_OUTPUT(0)] , %o0                                 ;/*  �ָ�����Ĵ���              */
    LDD     [%g1 + REG_OUTPUT(2)] , %o2
    LDD     [%g1 + REG_OUTPUT(4)] , %o4
    LDD     [%g1 + REG_OUTPUT(6)] , %o6

    LDD     [%g1 + REG_LOCAL(0)]  , %l0                                 ;/*  �ָ����ؼĴ���              */
    LDD     [%g1 + REG_LOCAL(2)]  , %l2
    LDD     [%g1 + REG_LOCAL(4)]  , %l4
    LDD     [%g1 + REG_LOCAL(6)]  , %l6

    LDD     [%g1 + REG_INPUT(0)]  , %i0                                 ;/*  �ָ�����Ĵ���              */
    LDD     [%g1 + REG_INPUT(2)]  , %i2
    LDD     [%g1 + REG_INPUT(4)]  , %i4
    LDD     [%g1 + REG_INPUT(6)]  , %i6

    SAVE                                                                ;/*  ���뵽һ�� dummy ����       */

    MOV     %g1 , %l3
    LD      [%l3 + REG_GLOBAL(1)] , %g1                                 ;/*  �ָ� %g1                    */
    LD      [%l3 + REG_PC]        , %l1                                 ;/*  %l1 �� dummy ����!          */
    LD      [%l3 + REG_NPC]       , %l2                                 ;/*  %l2 �� dummy ����!          */

    JMP     %l1
    RETT    %l2
    MACRO_END()

;/*********************************************************************************************************
;  FLUSH ����(%g1 %g6 %g7 ���ᱻ�ƻ�)
;*********************************************************************************************************/

MACRO_DEF(FLUSH_WINDOWS)
    RD      %psr , %g2
    ANDN    %g2  , PSR_ET , %g2                                         ;/*  �����쳣                    */
    OR      %g2  , PSR_PIL, %g2                                         ;/*  ���ж�                      */
    WR      %g2  , %psr
    NOP
    NOP
    NOP

    AND     %g2  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    MOV     1    , %g4                                                  ;/*  %g4 = 1                     */
    SLL     %g4  , %g3 , %g4                                            ;/*  %g4 = WIM mask for CW invalid*/

    RD      %wim , %g2                                                  ;/*  %g2 = wim                   */

123:
    SLL     %g4  , 1   , %g5                                            ;/*  rotate the "wim" left 1     */
    SRL     %g4  , LW_CFG_SPARC_REG_WIN_NR - 1 , %g4
    OR      %g4  , %g5 , %g4                                            ;/*  %g4 = wim if we do one restore*/

    ;/*
    ; * If a restore would not underflow, then continue.
    ; */
    ANDCC   %g4  , %g2 , %g0                                            ;/*  Any windows to flush?       */
    BNZ     456f                                                        ;/*  No, then continue           */
    NOP

    RESTORE                                                             ;/*  back one window             */

    ;/*
    ; * Now save the window just as if we overflowed to it.
    ; */
    STD     %l0  , [%sp + SF_LOCAL(0)]
    SPARC_B2BST_NOP
    STD     %l2  , [%sp + SF_LOCAL(2)]
    SPARC_B2BST_NOP
    STD     %l4  , [%sp + SF_LOCAL(4)]
    SPARC_B2BST_NOP
    STD     %l6  , [%sp + SF_LOCAL(6)]
    SPARC_B2BST_NOP

    STD     %i0  , [%sp + SF_INPUT(0)]
    SPARC_B2BST_NOP
    STD     %i2  , [%sp + SF_INPUT(2)]
    SPARC_B2BST_NOP
    STD     %i4  , [%sp + SF_INPUT(4)]
    SPARC_B2BST_NOP
    STD     %i6  , [%sp + SF_INPUT(6)]

    BA      123b
    NOP

456:
    RD      %psr , %g2
    OR      %g2  , PSR_ET , %g2                                         ;/*  ʹ���쳣                    */
    WR      %g2  , %psr
    NOP
    NOP
    NOP
    MACRO_END()

#endif                                                                  /*  __ARCH_SPARC_CTX_ASM_H       */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
