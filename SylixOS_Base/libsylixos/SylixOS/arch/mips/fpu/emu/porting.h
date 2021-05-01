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
** ��   ��   ��: porting.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 11 �� 24 ��
**
** ��        ��: MIPS FPU ģ����ֲ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_MIPSFPUEMUPORTING_H
#define __ARCH_MIPSFPUEMUPORTING_H

#include "arch/mips/inc/porting.h"
#include "mipsFpuEmu.h"

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define IS_ENABLED(x)   (x)

#ifndef CONFIG_64BIT
#define CONFIG_64BIT    0
#else
#define CONFIG_32BIT    0
#endif

#define cp0_epc         REG_ulCP0Epc
#define regs            REG_ulReg

#define fpr             FPUCTX_reg
#define fcr31           FPUCTX_uiFcsr

/*********************************************************************************************************
  ��õ�ǰ CPU ��ǰ�̵߳� FPU �Ĵ���
*********************************************************************************************************/

#define GET_CUR_THREAD_FPU_FPR          \
        ((ARCH_FPU_CTX *)(API_ThreadTcbSelf()->TCB_pvStackFP))->FPUCTX_reg

#define GET_CUR_THREAD_FPU_FCR31        \
        ((ARCH_FPU_CTX *)(API_ThreadTcbSelf()->TCB_pvStackFP))->FPUCTX_uiFcsr

/*********************************************************************************************************
  FPU REG ����
*********************************************************************************************************/

#if BYTE_ORDER == BIG_ENDIAN
# define FPR_IDX(width, idx)    ((idx) ^ ((64 / (width)) - 1))
#else
# define FPR_IDX(width, idx)    (idx)
#endif

#define BUILD_FPR_ACCESS(width) \
static inline UINT##width get_fpr##width(union fpureg *fpr, unsigned idx)  \
{                                   \
    return fpr->val##width[FPR_IDX(width, idx)];            \
}                                   \
                                    \
static inline void set_fpr##width(union fpureg *fpr, unsigned idx,  \
                  UINT##width val)             \
{                                   \
    fpr->val##width[FPR_IDX(width, idx)] = val;         \
}

BUILD_FPR_ACCESS(32)
BUILD_FPR_ACCESS(64)

#endif                                                                  /*  __ARCH_MIPSFPUEMUPORTING_H  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
