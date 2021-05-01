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
** ��   ��   ��: arch_regs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ARMv7M �Ĵ������.
**
** BUG:
2015.08.19 �� CPU �ֳ������ƶ�������.
*********************************************************************************************************/

#ifndef __ARMV7M_ARCH_REGS_H
#define __ARMV7M_ARCH_REGS_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  19                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  256                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           4                                       /*  �Ĵ�����С                  */
#define ARCH_HW_SAVE_CTX_SIZE   (8  * ARCH_REG_SIZE)                    /*  Ӳ���Զ����������Ĵ�С      */
#define ARCH_SW_SAVE_CTX_SIZE   (11 * ARCH_REG_SIZE)                    /*  ����ֶ����������Ĵ�С      */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     8                                       /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  18                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32      ARCH_REG_T;

typedef struct {
    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiR1;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR12;
    ARCH_REG_T      REG_uiR14;
    ARCH_REG_T      REG_uiR15;
    ARCH_REG_T      REG_uiXpsr;
} ARCH_HW_SAVE_REG_CTX;

typedef struct {
    ARCH_REG_T      REG_uiR13;
    ARCH_REG_T      REG_uiBasePri;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiR11;
    ARCH_REG_T      REG_uiExcRet;
} ARCH_SW_SAVE_REG_CTX;

typedef struct {
    ARCH_REG_T      REG_uiR13;
    ARCH_REG_T      REG_uiBasePri;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiR11;
    ARCH_REG_T      REG_uiExcRet;

    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiR1;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR12;
    ARCH_REG_T      REG_uiR14;
    ARCH_REG_T      REG_uiR15;
    ARCH_REG_T      REG_uiXpsr;

#define REG_uiFp    REG_uiR11
#define REG_uiIp    REG_uiR12
#define REG_uiSp    REG_uiR13
#define REG_uiLr    REG_uiR14
#define REG_uiPc    REG_uiR15
} ARCH_REG_CTX;

/*********************************************************************************************************
  EABI ��׼���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiFp;
    ARCH_REG_T      FP_uiLr;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiPc)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __ARMV7M_ARCH_REGS_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
