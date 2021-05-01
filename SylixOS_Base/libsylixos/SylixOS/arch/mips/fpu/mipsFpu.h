/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mipsFpu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 17 ��
**
** ��        ��: MIPS ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_MIPSFPU_H
#define __ARCH_MIPSFPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  MIPS fpu ��������
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     MFPU_pfuncEnable;
    VOIDFUNCPTR     MFPU_pfuncDisable;
    BOOLFUNCPTR     MFPU_pfuncIsEnable;
    VOIDFUNCPTR     MFPU_pfuncSave;
    VOIDFUNCPTR     MFPU_pfuncRestore;
    VOIDFUNCPTR     MFPU_pfuncCtxShow;
    ULONGFUNCPTR    MFPU_pfuncGetFIR;
    VOIDFUNCPTR     MFPU_pfuncEnableTask;
} MIPS_FPU_OP;
typedef MIPS_FPU_OP *PMIPS_FPU_OP;

/*********************************************************************************************************
  MIPS fpu ��������
*********************************************************************************************************/

#define MIPS_VFP_ENABLE(op)              op->MFPU_pfuncEnable()
#define MIPS_VFP_DISABLE(op)             op->MFPU_pfuncDisable()
#define MIPS_VFP_ISENABLE(op)            op->MFPU_pfuncIsEnable()
#define MIPS_VFP_SAVE(op, ctx)           op->MFPU_pfuncSave((ctx))
#define MIPS_VFP_RESTORE(op, ctx)        op->MFPU_pfuncRestore((ctx))
#define MIPS_VFP_CTXSHOW(op, fd, ctx)    op->MFPU_pfuncCtxShow((fd), (ctx))
#define MIPS_VFP_GETFIR(op)              op->MFPU_pfuncGetFIR()
#define MIPS_VFP_ENABLE_TASK(op, tcb)    op->MFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_MIPSFPU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
