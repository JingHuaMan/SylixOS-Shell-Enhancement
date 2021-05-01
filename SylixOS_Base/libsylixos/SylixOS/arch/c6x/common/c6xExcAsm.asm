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
;** ��   ��   ��: c6xExcAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2017 �� 03 �� 17 ��
;**
;** ��        ��: c6x ��ϵ�����쳣/�жϴ���.
;*********************************************************************************************************/

    .include "c6xContextAsmInc.asm"

    .ref    bspIntHandle
    .ref    archExcHandle
    .ref    API_InterEnter
    .ref    API_InterExit
    .ref    API_InterStackBaseGet
    .ref    API_ThreadTcbInter

    .ref    _G_ulIntSafeStack
    .ref    _G_ulIntNesting
    .ref    _G_ulCpu

    .global archIntEntry1
    .global archIntEntry2
    .global archIntEntry3
    .global archIntEntry4
    .global archIntEntry5
    .global archIntEntry6
    .global archIntEntry7
    .global archIntEntry8
    .global archIntEntry9
    .global archIntEntry10
    .global archIntEntry11
    .global archIntEntry12
    .global archIntEntry13
    .global archIntEntry14
    .global archIntEntry15

    .sect .text

;/*********************************************************************************************************
;  �жϽ����
;*********************************************************************************************************/

ARCH_INT_ENTRY  .macro  vector
    .newblock

    ;/*
    ; * �ж�Ƕ�׼�����һ
    ; */
    MVKL    _G_ulIntNesting , B1
    MVKH    _G_ulIntNesting , B1
    LDW     *+B1(0) , B1
    NOP     4

    LDW     *+B1(0) , B0                                        ;/*  B0 = �ж�Ƕ�׼���                   */
    NOP     4

    ADDK    +1 , B0
    STW     B0 , *+B1(0)

    CMPGT   B0  , 1 , B1
    [B1]    BNOP $4 , 5

    ;/*
    ; * ��һ�ν����ж�
    ; */
    ;/*
    ; * ��ȡ��ǰ TCB �� REG_CTX ��ַ
    ; */
    MVKL    _G_ulCpu , B1
    MVKH    _G_ulCpu , B1
    LDW     *+B1(0) , B0
    NOP     4

    LDW     *+B0(0) , B1                                        ;/*  B1 = ��ǰ TCB �� REG_CTX ��ַ       */
    NOP     4

    ;/*
    ; * ����Ĵ�������ǰ TCB �� REG_CTX
    ; */
    IRQ_SAVE_BIG_REG_CTX    IRP

    ;/*
    ; * ��һ�ν����ж�: ��õ�ǰ CPU �ж϶�ջջ��, ������ SP
    ; */
    MVKL    _G_ulIntSafeStack , B1
    MVKH    _G_ulIntSafeStack , B1
    LDW     *+B1(0) , B15
    NOP     4

$1:
    ;/*
    ; * handle(vector)
    ; */
    MVK     vector , A4
    B       bspIntHandle
    ADDKPC  $2  , B3 , 4
$2:

    ;/*
    ; * API_InterExit()
    ; * ���û�з����ж�Ƕ��, �� API_InterExit ����� archIntCtxLoad ����
    ; */
    B       API_InterExit
    ADDKPC  $3  , B3 , 4
$3:

    ;/*
    ; * ��������, ˵���������ж�Ƕ��
    ; */
    IRQ_NEST_RESTORE_BIG_REG_CTX

$4:
    ;/*
    ; * ���ǵ�һ�ν����ж�
    ; */
    IRQ_NEST_SAVE_BIG_REG_CTX   IRP

    BNOP    $1 , 5
    .endm

;/*********************************************************************************************************
;  �쳣�����
;*********************************************************************************************************/

ARCH_EXC_ENTRY  .macro  vector
    .newblock

    ;/*
    ; * �ж�Ƕ�׼�����һ
    ; */
    MVKL    _G_ulIntNesting , B1
    MVKH    _G_ulIntNesting , B1
    LDW     *+B1(0) , B1
    NOP     4

    LDW     *+B1(0) , B0                                        ;/*  B0 = �ж�Ƕ�׼���                   */
    NOP     4

    ADDK    +1 , B0
    STW     B0 , *+B1(0)

    CMPGT   B0  , 1 , B1
    [B1]    BNOP $4 , 5

    ;/*
    ; * ��һ�ν����ж�
    ; */
    ;/*
    ; * ��ȡ��ǰ TCB �� REG_CTX ��ַ
    ; */
    MVKL    _G_ulCpu , B1
    MVKH    _G_ulCpu , B1
    LDW     *+B1(0) , B0
    NOP     4

    LDW     *+B0(0) , B1                                        ;/*  B1 = ��ǰ TCB �� REG_CTX ��ַ       */
    NOP     4

    ;/*
    ; * ����Ĵ�������ǰ TCB �� REG_CTX
    ; */
    IRQ_SAVE_BIG_REG_CTX    NRP

    ;/*
    ; * ��һ�ν����ж�: ��õ�ǰ CPU �ж϶�ջջ��, ������ SP
    ; */
    MVKL    _G_ulIntSafeStack , B1
    MVKH    _G_ulIntSafeStack , B1
    LDW     *+B1(0) , B15
    NOP     4

    MV      A1 , A4                                             ;/*  ��ǰ TCB �� REG_CTX ��ַ��Ϊ����    */

$1:
    ;/*
    ; * archExcHandle(�Ĵ���������)
    ; */
    B       archExcHandle
    ADDKPC  $2  , B3 , 4
$2:

    ;/*
    ; * API_InterExit()
    ; * ���û�з����ж�Ƕ��, �� API_InterExit ����� archIntCtxLoad ����
    ; */
    B       API_InterExit
    ADDKPC  $3  , B3 , 4
$3:

    ;/*
    ; * ��������, ˵���������ж�Ƕ��
    ; */
    IRQ_NEST_RESTORE_BIG_REG_CTX

$4:
    ;/*
    ; * ���ǵ�һ�ν����ж�
    ; */
    IRQ_NEST_SAVE_BIG_REG_CTX   NRP

    B       $1
    ADD     +8  , B15 , A4                                      ;/*  ջ�� ARCH_REG_CTX ��ַ��Ϊ����      */
    NOP     4
    .endm

;/*********************************************************************************************************
;  �жϽ����
;*********************************************************************************************************/

archIntEntry1: .asmfunc
    ARCH_EXC_ENTRY
    .endasmfunc

archIntEntry2: .asmfunc
    ARCH_INT_ENTRY 2
    .endasmfunc

archIntEntry3: .asmfunc
    ARCH_INT_ENTRY 3
    .endasmfunc

archIntEntry4: .asmfunc
    ARCH_INT_ENTRY 4
    .endasmfunc

archIntEntry5: .asmfunc
    ARCH_INT_ENTRY 5
    .endasmfunc

archIntEntry6: .asmfunc
    ARCH_INT_ENTRY 6
    .endasmfunc

archIntEntry7: .asmfunc
    ARCH_INT_ENTRY 7
    .endasmfunc

archIntEntry8: .asmfunc
    ARCH_INT_ENTRY 8
    .endasmfunc

archIntEntry9: .asmfunc
    ARCH_INT_ENTRY 9
    .endasmfunc

archIntEntry10: .asmfunc
    ARCH_INT_ENTRY 10
    .endasmfunc

archIntEntry11: .asmfunc
    ARCH_INT_ENTRY 11
    .endasmfunc

archIntEntry12: .asmfunc
    ARCH_INT_ENTRY 12
    .endasmfunc

archIntEntry13: .asmfunc
    ARCH_INT_ENTRY 13
    .endasmfunc

archIntEntry14: .asmfunc
    ARCH_INT_ENTRY 14
    .endasmfunc

archIntEntry15: .asmfunc
    ARCH_INT_ENTRY 15
    .endasmfunc

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
