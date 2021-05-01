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
** ��   ��   ��: sparcFpu.h
**
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_SPARC_FPU_H
#define __ARCH_SPARC_FPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  SPARC fpu ��������
*********************************************************************************************************/

typedef struct {
    ULONGFUNCPTR    SFPU_pfuncHwSid;
    VOIDFUNCPTR     SFPU_pfuncEnable;
    VOIDFUNCPTR     SFPU_pfuncDisable;
    BOOLFUNCPTR     SFPU_pfuncIsEnable;
    VOIDFUNCPTR     SFPU_pfuncSave;
    VOIDFUNCPTR     SFPU_pfuncRestore;
    VOIDFUNCPTR     SFPU_pfuncCtxShow;
    VOIDFUNCPTR     SFPU_pfuncEnableTask;
} SPARC_FPU_OP;
typedef SPARC_FPU_OP *PSPARC_FPU_OP;

/*********************************************************************************************************
  SPARC fpu ��������
*********************************************************************************************************/

#define SPARC_VFP_ENABLE(op)              op->SFPU_pfuncEnable()
#define SPARC_VFP_DISABLE(op)             op->SFPU_pfuncDisable()
#define SPARC_VFP_ISENABLE(op)            op->SFPU_pfuncIsEnable()
#define SPARC_VFP_SAVE(op, ctx)           op->SFPU_pfuncSave((ctx))
#define SPARC_VFP_RESTORE(op, ctx)        op->SFPU_pfuncRestore((ctx))
#define SPARC_VFP_CTXSHOW(op, fd, ctx)    op->SFPU_pfuncCtxShow((fd), (ctx))
#define SPARC_VFP_ENABLE_TASK(op, tcb)    op->SFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_SPARC_FPU_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
