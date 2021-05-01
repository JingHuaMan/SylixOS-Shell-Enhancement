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
** ��   ��   ��: arch_regs64.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS64 �Ĵ������.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_REGS64_H
#define __MIPS_ARCH_REGS64_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_GREG_NR            32                                      /*  ͨ�üĴ�����Ŀ              */

#define ARCH_REG_CTX_WORD_SIZE  38                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  512                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           8                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#if defined(_MIPS_ARCH_HR2)
#define ARCH_STK_ALIGN_SIZE     32                                      /*  ��ջ����Ҫ��                */
#else
#define ARCH_STK_ALIGN_SIZE     16                                      /*  ��ջ����Ҫ��                */
#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */

#define ARCH_JMP_BUF_WORD_SIZE  38                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
*********************************************************************************************************/

#define XGREG(n)                ((n) * ARCH_REG_SIZE)
#define XLO                     ((ARCH_GREG_NR + 0) * ARCH_REG_SIZE)
#define XHI                     ((ARCH_GREG_NR + 1) * ARCH_REG_SIZE)
#define XCAUSE                  ((ARCH_GREG_NR + 2) * ARCH_REG_SIZE)
#define XSR                     ((ARCH_GREG_NR + 3) * ARCH_REG_SIZE)
#define XEPC                    ((ARCH_GREG_NR + 4) * ARCH_REG_SIZE)
#define XBADVADDR               ((ARCH_GREG_NR + 5) * ARCH_REG_SIZE)

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64      ARCH_REG_T;

typedef struct {
    ARCH_REG_T  REG_ulReg[ARCH_GREG_NR];                                /*  32 ��ͨ��Ŀ�ļĴ���         */
    ARCH_REG_T  REG_ulCP0DataLo;                                        /*  �˷������λ�Ĵ���          */
    ARCH_REG_T  REG_ulCP0DataHi;                                        /*  �˷������λ�Ĵ���          */
    ARCH_REG_T  REG_ulCP0Cause;                                         /*  �����жϻ����쳣�鿴�ļĴ���*/
    ARCH_REG_T  REG_ulCP0Status;                                        /*  CP0 Э������״̬�Ĵ���      */
    ARCH_REG_T  REG_ulCP0Epc;                                           /*  ����������Ĵ���            */
    ARCH_REG_T  REG_ulCP0BadVAddr;                                      /*  �����ַ�Ĵ���              */

/*********************************************************************************************************
  MIPS64 N64 ABI �ļĴ�������
*********************************************************************************************************/
#define REG_ZERO                0                                       /*  wired zero                  */
#define REG_AT                  1                                       /*  assembler temp              */
#define REG_V0                  2                                       /*  return reg 0                */
#define REG_V1                  3                                       /*  return reg 1                */
#define REG_A0                  4                                       /*  arg reg 0                   */
#define REG_A1                  5                                       /*  arg reg 1                   */
#define REG_A2                  6                                       /*  arg reg 2                   */
#define REG_A3                  7                                       /*  arg reg 3                   */
#define REG_A4                  8                                       /*  arg reg 4                   */
#define REG_A5                  9                                       /*  arg reg 5                   */
#define REG_A6                  10                                      /*  arg reg 6                   */
#define REG_A7                  11                                      /*  arg reg 7                   */
#define REG_T0                  12                                      /*  caller saved 0              */
#define REG_T1                  13                                      /*  caller saved 1              */
#define REG_T2                  14                                      /*  caller saved 2              */
#define REG_T3                  15                                      /*  caller saved 3              */
#define REG_S0                  16                                      /*  callee saved 0              */
#define REG_S1                  17                                      /*  callee saved 1              */
#define REG_S2                  18                                      /*  callee saved 2              */
#define REG_S3                  19                                      /*  callee saved 3              */
#define REG_S4                  20                                      /*  callee saved 4              */
#define REG_S5                  21                                      /*  callee saved 5              */
#define REG_S6                  22                                      /*  callee saved 6              */
#define REG_S7                  23                                      /*  callee saved 7              */
#define REG_T8                  24                                      /*  caller saved 8              */
#define REG_T9                  25                                      /*  caller saved 9              */
#define REG_K0                  26                                      /*  kernel temp 0               */
#define REG_K1                  27                                      /*  kernel temp 1               */
#define REG_GP                  28                                      /*  global pointer              */
#define REG_SP                  29                                      /*  stack pointer               */
#define REG_S8                  30                                      /*  callee saved 8              */
#define REG_FP                  REG_S8                                  /*  callee saved 8              */
#define REG_RA                  31                                      /*  return address              */
} ARCH_REG_CTX;

/*********************************************************************************************************
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
#define ARCH_ARG_REG_NR         4
    ARCH_REG_T  FP_ulArg[ARCH_ARG_REG_NR];                              /*  N64 ABI ����ҪΪ����Ԥ��ջ��*/
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_ulCP0Epc)

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __MIPS_ARCH_REGS64_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
