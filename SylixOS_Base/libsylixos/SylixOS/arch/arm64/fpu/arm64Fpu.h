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
** ��   ��   ��: arm64Fpu.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 29 ��
**
** ��        ��: ARM64 ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARM64FPU_H
#define __ARM64FPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  ARM64 fpu ��������
*********************************************************************************************************/

typedef struct {
    ULONGFUNCPTR    AFPU_pfuncHwSid;
    VOIDFUNCPTR     AFPU_pfuncEnable;
    VOIDFUNCPTR     AFPU_pfuncDisable;
    BOOLFUNCPTR     AFPU_pfuncIsEnable;
    VOIDFUNCPTR     AFPU_pfuncSave;
    VOIDFUNCPTR     AFPU_pfuncRestore;
    VOIDFUNCPTR     AFPU_pfuncCtxShow;
} ARM64_FPU_OP;
typedef ARM64_FPU_OP *PARM64_FPU_OP;

/*********************************************************************************************************
  ARM64 fpu ��������
*********************************************************************************************************/

#define ARM64_VFP_HW_SID(op)              op->AFPU_pfuncHwSid()
#define ARM64_VFP_ENABLE(op)              op->AFPU_pfuncEnable()
#define ARM64_VFP_DISABLE(op)             op->AFPU_pfuncDisable()
#define ARM64_VFP_ISENABLE(op)            op->AFPU_pfuncIsEnable()
#define ARM64_VFP_SAVE(op, ctx)           op->AFPU_pfuncSave((ctx))
#define ARM64_VFP_RESTORE(op, ctx)        op->AFPU_pfuncRestore((ctx))
#define ARM64_VFP_CTXSHOW(op, fd, ctx)    op->AFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARM64FPU_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
