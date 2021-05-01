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
** ��   ��   ��: x86Fpu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ�ܹ�Ӳ������������ (FPU).
*********************************************************************************************************/

#ifndef __ARCH_X86FPU_H
#define __ARCH_X86FPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  x86 fpu ��������
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     PFPU_pfuncEnable;
    VOIDFUNCPTR     PFPU_pfuncDisable;
    BOOLFUNCPTR     PFPU_pfuncIsEnable;
    VOIDFUNCPTR     PFPU_pfuncSave;
    VOIDFUNCPTR     PFPU_pfuncRestore;
    VOIDFUNCPTR     PFPU_pfuncCtxShow;
    VOIDFUNCPTR     PFPU_pfuncEnableTask;
} X86_FPU_OP;
typedef X86_FPU_OP *PX86_FPU_OP;

/*********************************************************************************************************
  x86 fpu ��������
*********************************************************************************************************/

#define X86_FPU_ENABLE(op)              op->PFPU_pfuncEnable()
#define X86_FPU_DISABLE(op)             op->PFPU_pfuncDisable()
#define X86_FPU_ISENABLE(op)            op->PFPU_pfuncIsEnable()
#define X86_FPU_SAVE(op, ctx)           op->PFPU_pfuncSave((ctx))
#define X86_FPU_RESTORE(op, ctx)        op->PFPU_pfuncRestore((ctx))
#define X86_FPU_CTXSHOW(op, fd, ctx)    op->PFPU_pfuncCtxShow((fd), (ctx))
#define X86_FPU_ENABLE_TASK(op, tcb)    op->PFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_X86FPU_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
