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
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC �Ĵ������.
**
*********************************************************************************************************/

#ifndef __SPARC_ARCH_REGS_H
#define __SPARC_ARCH_REGS_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_GREG_NR            32                                      /*  ͨ�üĴ�����Ŀ              */

#define ARCH_REG_CTX_WORD_SIZE  36                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  512                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           4                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     8                                       /*  ��ջ����Ҫ��                */
#define ARCH_STK_FRAME_SIZE     96                                      /*  ��Сջ֡��С                */

#define ASM_STACK_FRAME_SIZE    ARCH_STK_FRAME_SIZE                     /*  �� BSP ����                 */

#define ARCH_JMP_BUF_WORD_SIZE  62                                      /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
*********************************************************************************************************/

#define REG_GLOBAL(x)           (((x) + 0)  * ARCH_REG_SIZE)
#define REG_OUTPUT(x)           (((x) + 8)  * ARCH_REG_SIZE)
#define REG_LOCAL(x)            (((x) + 16) * ARCH_REG_SIZE)
#define REG_INPUT(x)            (((x) + 24) * ARCH_REG_SIZE)
#define REG_PSR                 ((ARCH_GREG_NR + 0) * ARCH_REG_SIZE)
#define REG_PC                  ((ARCH_GREG_NR + 1) * ARCH_REG_SIZE)
#define REG_NPC                 ((ARCH_GREG_NR + 2) * ARCH_REG_SIZE)
#define REG_Y                   ((ARCH_GREG_NR + 3) * ARCH_REG_SIZE)

/*********************************************************************************************************
  �Ĵ����� ARCH_FP_CTX (ջ֡�ṹ)�е�ƫ����
*********************************************************************************************************/

#define SF_LOCAL(x)             (((x) + 0) * ARCH_REG_SIZE)
#define SF_INPUT(x)             (((x) + 8) * ARCH_REG_SIZE)

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32      ARCH_REG_T;

struct arch_reg_ctx {
    ARCH_REG_T      REG_uiGlobal[8];                                    /*  Global regs                 */
    ARCH_REG_T      REG_uiOutput[8];                                    /*  Output regs                 */
    ARCH_REG_T      REG_uiLocal[8];                                     /*  Local regs                  */
    ARCH_REG_T      REG_uiInput[8];                                     /*  Input regs                  */
    ARCH_REG_T      REG_uiPsr;                                          /*  Psr reg                     */
    ARCH_REG_T      REG_uiPc;                                           /*  Pc reg                      */
    ARCH_REG_T      REG_uiNPc;                                          /*  NPc reg                     */
    ARCH_REG_T      REG_uiY;                                            /*  Y reg                       */

#define REG_uiFp    REG_uiInput[6]
#define REG_uiRet   REG_uiInput[7]
#define REG_uiSp    REG_uiOutput[6]
} __attribute__ ((aligned(8)));                                         /*  Use STD & LDD ins           */

typedef struct arch_reg_ctx  ARCH_REG_CTX;

/*********************************************************************************************************
  ��Сջ֡�ṹ
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiLocal[8];
    ARCH_REG_T      FP_uiInput[6];
    ARCH_REG_T      FP_uiFp;
    ARCH_REG_T      FP_uiRetAddr;
    ARCH_REG_T      FP_uiStructPtr;
    ARCH_REG_T      FP_uiXArgs[6];
    ARCH_REG_T      FP_uiXXArgs[1];
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiPc)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

#endif                                                                  /*  !defined __ASSEMBLY__       */
#endif                                                                  /*  __SPARC_ARCH_REGS_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
