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
** ��   ��   ��: cskyFpu.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_CSKYFPU_H
#define __ARCH_CSKYFPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  C-SKY FPU ��������
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     CFPU_pfuncEnable;
    VOIDFUNCPTR     CFPU_pfuncDisable;
    BOOLFUNCPTR     CFPU_pfuncIsEnable;
    VOIDFUNCPTR     CFPU_pfuncSave;
    VOIDFUNCPTR     CFPU_pfuncRestore;
    VOIDFUNCPTR     CFPU_pfuncCtxShow;
} CSKY_FPU_OP;
typedef CSKY_FPU_OP *PCSKY_FPU_OP;

/*********************************************************************************************************
  C-SKY FPU ��������
*********************************************************************************************************/

#define CSKY_VFP_ENABLE(op)              op->CFPU_pfuncEnable()
#define CSKY_VFP_DISABLE(op)             op->CFPU_pfuncDisable()
#define CSKY_VFP_ISENABLE(op)            op->CFPU_pfuncIsEnable()
#define CSKY_VFP_SAVE(op, ctx)           op->CFPU_pfuncSave((ctx))
#define CSKY_VFP_RESTORE(op, ctx)        op->CFPU_pfuncRestore((ctx))
#define CSKY_VFP_CTXSHOW(op, fd, ctx)    op->CFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_CSKYFPU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
