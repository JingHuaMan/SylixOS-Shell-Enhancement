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
** �ļ���������: 2017 �� 02 �� 28 ��
**
** ��        ��: TI C6X DSP �Ĵ������.
*********************************************************************************************************/

#ifndef __C6X_ARCH_REGS_H
#define __C6X_ARCH_REGS_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  (18 + 32 + 32)                          /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  256                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           4                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     8                                       /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  28                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32      ARCH_REG_T;

#if LW_CFG_CPU_ENDIAN == 1
#define ARCH_REG_PAIR(a,b) ARCH_REG_T a; ARCH_REG_T b

#elif LW_CFG_CPU_ENDIAN == 0
#define ARCH_REG_PAIR(a,b) ARCH_REG_T b; ARCH_REG_T a

#else
#error TI Compiler does not specify endianness
#endif

struct arch_reg_ctx {
    ARCH_REG_T      REG_uiContextType;
    ARCH_REG_T      REG_uiReserved1;

    ARCH_REG_PAIR(REG_uiA15,  REG_uiA14);
    ARCH_REG_PAIR(REG_uiB15,  REG_uiB14);
    ARCH_REG_PAIR(REG_uiA13,  REG_uiA12);
    ARCH_REG_PAIR(REG_uiB13,  REG_uiB12);
    ARCH_REG_PAIR(REG_uiA11,  REG_uiA10);
    ARCH_REG_PAIR(REG_uiB11,  REG_uiB10);
    ARCH_REG_PAIR(REG_uiA5,   REG_uiA4);

    ARCH_REG_T      REG_uiCsr;
    ARCH_REG_T      REG_uiAmr;

    ARCH_REG_T      REG_uiGfpgfr;
    ARCH_REG_T      REG_uiIrp;

    ARCH_REG_T      REG_uiReserved2;
    ARCH_REG_T      REG_uiFmcr;

    ARCH_REG_T      REG_uiFaucr;
    ARCH_REG_T      REG_uiFadcr;

    ARCH_REG_T      REG_uiSsr;
    ARCH_REG_T      REG_uiRilc;

    ARCH_REG_T      REG_uiItsr;
    ARCH_REG_T      REG_uiGplyb;

    ARCH_REG_T      REG_uiGplya;
    ARCH_REG_T      REG_uiIlc;

    ARCH_REG_PAIR(REG_uiB31, REG_uiB30);
    ARCH_REG_PAIR(REG_uiA31, REG_uiA30);
    ARCH_REG_PAIR(REG_uiB29, REG_uiB28);
    ARCH_REG_PAIR(REG_uiA29, REG_uiA28);
    ARCH_REG_PAIR(REG_uiB27, REG_uiB26);
    ARCH_REG_PAIR(REG_uiA27, REG_uiA26);
    ARCH_REG_PAIR(REG_uiB25, REG_uiB24);
    ARCH_REG_PAIR(REG_uiA25, REG_uiA24);
    ARCH_REG_PAIR(REG_uiB23, REG_uiB22);
    ARCH_REG_PAIR(REG_uiA23, REG_uiA22);
    ARCH_REG_PAIR(REG_uiB21, REG_uiB20);
    ARCH_REG_PAIR(REG_uiA21, REG_uiA20);
    ARCH_REG_PAIR(REG_uiB19, REG_uiB18);
    ARCH_REG_PAIR(REG_uiA19, REG_uiA18);
    ARCH_REG_PAIR(REG_uiB17, REG_uiB16);
    ARCH_REG_PAIR(REG_uiA17, REG_uiA16);
    ARCH_REG_PAIR(REG_uiB9,  REG_uiB8);
    ARCH_REG_PAIR(REG_uiA9,  REG_uiA8);
    ARCH_REG_PAIR(REG_uiB7,  REG_uiB6);
    ARCH_REG_PAIR(REG_uiA7,  REG_uiA6);
    ARCH_REG_PAIR(REG_uiB5,  REG_uiB4);
    ARCH_REG_PAIR(REG_uiA3,  REG_uiA2);
    ARCH_REG_PAIR(REG_uiB3,  REG_uiB2);
    ARCH_REG_PAIR(REG_uiA1,  REG_uiA0);
    ARCH_REG_T      REG_uiReserved3;
    ARCH_REG_PAIR(REG_uiB1,  REG_uiB0);
    ARCH_REG_T      REG_uiReserved4;

#define REG_uiSp    REG_uiB15
#define REG_uiFp    REG_uiA15
#define REG_uiDp    REG_uiB14
} __attribute__ ((aligned(8)));

typedef struct arch_reg_ctx ARCH_REG_CTX;

/*********************************************************************************************************
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T          FP_uiRetAddr;
    ARCH_REG_T          FP_uiArg;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiIrp)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __C6X_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
