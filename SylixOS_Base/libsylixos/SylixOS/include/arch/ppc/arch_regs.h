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
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC �Ĵ������.
*********************************************************************************************************/

#ifndef __PPC_ARCH_REGS_H
#define __PPC_ARCH_REGS_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_GREG_NR            32                                      /*  ͨ�üĴ�����Ŀ              */

#define ARCH_REG_CTX_WORD_SIZE  40                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  256                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           4                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     8                                       /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  44                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
*********************************************************************************************************/

#define XR(n)                   ((n) * ARCH_REG_SIZE)
#define XSRR0                   ((ARCH_GREG_NR + 0) * ARCH_REG_SIZE)
#define XSRR1                   ((ARCH_GREG_NR + 1) * ARCH_REG_SIZE)
#define XCTR                    ((ARCH_GREG_NR + 2) * ARCH_REG_SIZE)
#define XXER                    ((ARCH_GREG_NR + 3) * ARCH_REG_SIZE)
#define XCR                     ((ARCH_GREG_NR + 4) * ARCH_REG_SIZE)
#define XLR                     ((ARCH_GREG_NR + 5) * ARCH_REG_SIZE)
#define XDAR                    ((ARCH_GREG_NR + 6) * ARCH_REG_SIZE)

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

typedef struct {
    union {
        ARCH_REG_T          REG_uiReg[32];
        struct {
            ARCH_REG_T      REG_uiR0;
            ARCH_REG_T      REG_uiR1;
            ARCH_REG_T      REG_uiR2;
            ARCH_REG_T      REG_uiR3;
            ARCH_REG_T      REG_uiR4;
            ARCH_REG_T      REG_uiR5;
            ARCH_REG_T      REG_uiR6;
            ARCH_REG_T      REG_uiR7;
            ARCH_REG_T      REG_uiR8;
            ARCH_REG_T      REG_uiR9;
            ARCH_REG_T      REG_uiR10;
            ARCH_REG_T      REG_uiR11;
            ARCH_REG_T      REG_uiR12;
            ARCH_REG_T      REG_uiR13;
            ARCH_REG_T      REG_uiR14;
            ARCH_REG_T      REG_uiR15;
            ARCH_REG_T      REG_uiR16;
            ARCH_REG_T      REG_uiR17;
            ARCH_REG_T      REG_uiR18;
            ARCH_REG_T      REG_uiR19;
            ARCH_REG_T      REG_uiR20;
            ARCH_REG_T      REG_uiR21;
            ARCH_REG_T      REG_uiR22;
            ARCH_REG_T      REG_uiR23;
            ARCH_REG_T      REG_uiR24;
            ARCH_REG_T      REG_uiR25;
            ARCH_REG_T      REG_uiR26;
            ARCH_REG_T      REG_uiR27;
            ARCH_REG_T      REG_uiR28;
            ARCH_REG_T      REG_uiR29;
            ARCH_REG_T      REG_uiR30;
            ARCH_REG_T      REG_uiR31;
        };
    };

    ARCH_REG_T              REG_uiSrr0;
    ARCH_REG_T              REG_uiSrr1;
    ARCH_REG_T              REG_uiCtr;
    ARCH_REG_T              REG_uiXer;
    ARCH_REG_T              REG_uiCr;
    ARCH_REG_T              REG_uiLr;
    ARCH_REG_T              REG_uiDar;
    ARCH_REG_T              REG_uiPad;
#define REG_uiSp            REG_uiR1
#define REG_uiFp            REG_uiR31
#define REG_uiPc            REG_uiSrr0
#define REG_uiMsr           REG_uiSrr1
} ARCH_REG_CTX;

/*********************************************************************************************************
  EABI ��׼���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T              FP_uiFp;
    ARCH_REG_T              FP_uiLr;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiPc)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __PPC_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
