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
** �ļ���������: 2017 �� 06 �� 05 ��
**
** ��        ��: x86-64 �Ĵ������.
*********************************************************************************************************/

#ifndef __X86_ARCH_REGS64_H
#define __X86_ARCH_REGS64_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  22                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  512                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           8                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     16                                      /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  22                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
*********************************************************************************************************/

#define XRAX                    (0  * ARCH_REG_SIZE)
#define XRBX                    (1  * ARCH_REG_SIZE)
#define XRCX                    (2  * ARCH_REG_SIZE)
#define XRDX                    (3  * ARCH_REG_SIZE)
#define XRSI                    (4  * ARCH_REG_SIZE)
#define XRDI                    (5  * ARCH_REG_SIZE)
#define XR8                     (6  * ARCH_REG_SIZE)
#define XR9                     (7  * ARCH_REG_SIZE)
#define XR10                    (8  * ARCH_REG_SIZE)
#define XR11                    (9  * ARCH_REG_SIZE)
#define XR12                    (10 * ARCH_REG_SIZE)
#define XR13                    (11 * ARCH_REG_SIZE)
#define XR14                    (12 * ARCH_REG_SIZE)
#define XR15                    (13 * ARCH_REG_SIZE)
#define XRBP                    (14 * ARCH_REG_SIZE)
#define XPAD                    (15 * ARCH_REG_SIZE)
#define XERROR                  (16 * ARCH_REG_SIZE)
#define XRIP                    (17 * ARCH_REG_SIZE)
#define XCS                     (18 * ARCH_REG_SIZE)
#define XRFLAGS                 (19 * ARCH_REG_SIZE)
#define XRSP                    (20 * ARCH_REG_SIZE)
#define XSS                     (21 * ARCH_REG_SIZE)

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

typedef UINT64          ARCH_REG_T;

struct __ARCH_REG_CTX {
    ARCH_REG_T          REG_ulRAX;                                      /*  4 �����ݼĴ���              */
    ARCH_REG_T          REG_ulRBX;
    ARCH_REG_T          REG_ulRCX;
    ARCH_REG_T          REG_ulRDX;

    ARCH_REG_T          REG_ulRSI;                                      /*  2 ����ַ��ָ��Ĵ���        */
    ARCH_REG_T          REG_ulRDI;

    ARCH_REG_T          REG_ulR8;                                       /*  8 �����ݼĴ���              */
    ARCH_REG_T          REG_ulR9;
    ARCH_REG_T          REG_ulR10;
    ARCH_REG_T          REG_ulR11;
    ARCH_REG_T          REG_ulR12;
    ARCH_REG_T          REG_ulR13;
    ARCH_REG_T          REG_ulR14;
    ARCH_REG_T          REG_ulR15;

    ARCH_REG_T          REG_ulRBP;                                      /*  ջָ֡��Ĵ���              */
    ARCH_REG_T          REG_uiPad;                                      /*  PAD                         */

    ARCH_REG_T          REG_ulError;                                    /*  ERROR CODE                  */
    ARCH_REG_T          REG_ulRIP;                                      /*  ָ��ָ��Ĵ���(RIP)         */
    ARCH_REG_T          REG_ulCS;                                       /*  ����μĴ���(CS)            */
    ARCH_REG_T          REG_ulRFLAGS;                                   /*  ��־�Ĵ���(RFLAGS)          */
    ARCH_REG_T          REG_ulRSP;                                      /*  ԭջָ��Ĵ���(RSP)         */
    ARCH_REG_T          REG_ulSS;                                       /*  ջ�μĴ���(SS)              */

#define REG_ERROR       REG_ulError
#define REG_XIP         REG_ulRIP
#define REG_XFLAGS      REG_ulRFLAGS
} __attribute__ ((packed));

typedef struct __ARCH_REG_CTX   ARCH_REG_CTX;

/*********************************************************************************************************
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T          FP_ulRetAddr;
    ARCH_REG_T          FP_ulSavedRBP;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((PVOID)(ctx).REG_ulRIP)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((PVOID)(ctx).REG_ulRBP)
#define ARCH_REG_CTX_GET_STACK(ctx) ((PVOID)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined(ASSEMBLY)          */
#endif                                                                  /*  __X86_ARCH_REGS64_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
