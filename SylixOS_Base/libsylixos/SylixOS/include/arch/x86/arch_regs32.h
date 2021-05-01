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
** ��   ��   ��: arch_regs32.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 06 �� 25 ��
**
** ��        ��: x86 �Ĵ������.
*********************************************************************************************************/

#ifndef __X86_ARCH_REGS32_H
#define __X86_ARCH_REGS32_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  14                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  256                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           4                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     8                                       /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  14                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
*********************************************************************************************************/

#define XEAX                    (0  * ARCH_REG_SIZE)
#define XEBX                    (1  * ARCH_REG_SIZE)
#define XECX                    (2  * ARCH_REG_SIZE)
#define XEDX                    (3  * ARCH_REG_SIZE)
#define XESI                    (4  * ARCH_REG_SIZE)
#define XEDI                    (5  * ARCH_REG_SIZE)
#define XEBP                    (6  * ARCH_REG_SIZE)
#define XPAD                    (7  * ARCH_REG_SIZE)
#define XERROR                  (8  * ARCH_REG_SIZE)
#define XEIP                    (9  * ARCH_REG_SIZE)
#define XCS                     (10 * ARCH_REG_SIZE)
#define XEFLAGS                 (11 * ARCH_REG_SIZE)
#define XESP                    (12 * ARCH_REG_SIZE)
#define XSS                     (13 * ARCH_REG_SIZE)

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

struct __ARCH_REG_CTX {
    ARCH_REG_T      REG_uiEAX;                                          /*  4 �����ݼĴ���              */
    ARCH_REG_T      REG_uiEBX;
    ARCH_REG_T      REG_uiECX;
    ARCH_REG_T      REG_uiEDX;

    ARCH_REG_T      REG_uiESI;                                          /*  2 ����ַ��ָ��Ĵ���        */
    ARCH_REG_T      REG_uiEDI;

    ARCH_REG_T      REG_uiEBP;                                          /*  ջָ֡��Ĵ���              */
    ARCH_REG_T      REG_uiPad;                                          /*  PAD                         */

    ARCH_REG_T      REG_uiError;                                        /*  ERROR CODE                  */
    ARCH_REG_T      REG_uiEIP;                                          /*  ָ��ָ��Ĵ���(EIP)         */
    ARCH_REG_T      REG_uiCS;                                           /*  ����μĴ���(CS)            */
    ARCH_REG_T      REG_uiEFLAGS;                                       /*  ��־�Ĵ���(EFLAGS)          */
    ARCH_REG_T      REG_uiESP;                                          /*  ��ջָ��Ĵ���              */
    ARCH_REG_T      REG_uiSS;                                           /*  ����μĴ���(CS)            */

#define REG_ERROR   REG_uiError
#define REG_XIP     REG_uiEIP
#define REG_XFLAGS  REG_uiEFLAGS
} __attribute__ ((packed));

typedef struct __ARCH_REG_CTX   ARCH_REG_CTX;

/*********************************************************************************************************
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiRetAddr;
    ARCH_REG_T      FP_uiArg;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((PVOID)(ctx).REG_uiEIP)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((PVOID)(ctx).REG_uiEBP)
#define ARCH_REG_CTX_GET_STACK(ctx) ((PVOID)&(ctx))                     /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __X86_ARCH_REGS32_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
