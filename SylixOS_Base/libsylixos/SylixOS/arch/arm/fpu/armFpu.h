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
** ��   ��   ��: armFpu.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARMFPU_H
#define __ARMFPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  ARM fpu ��������
*********************************************************************************************************/

typedef struct {
#if !defined(__SYLIXOS_ARM_ARCH_M__)
    ULONGFUNCPTR    AFPU_pfuncHwSid;
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
    VOIDFUNCPTR     AFPU_pfuncEnable;
    VOIDFUNCPTR     AFPU_pfuncDisable;
    BOOLFUNCPTR     AFPU_pfuncIsEnable;
    VOIDFUNCPTR     AFPU_pfuncSave;
    VOIDFUNCPTR     AFPU_pfuncRestore;
    VOIDFUNCPTR     AFPU_pfuncCtxShow;
} ARM_FPU_OP;
typedef ARM_FPU_OP *PARM_FPU_OP;

/*********************************************************************************************************
  ARM fpu ��������
*********************************************************************************************************/

#if !defined(__SYLIXOS_ARM_ARCH_M__)
#define ARM_VFP_HW_SID(op)              op->AFPU_pfuncHwSid()
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
#define ARM_VFP_ENABLE(op)              op->AFPU_pfuncEnable()
#define ARM_VFP_DISABLE(op)             op->AFPU_pfuncDisable()
#define ARM_VFP_ISENABLE(op)            op->AFPU_pfuncIsEnable()
#define ARM_VFP_SAVE(op, ctx)           op->AFPU_pfuncSave((ctx))
#define ARM_VFP_RESTORE(op, ctx)        op->AFPU_pfuncRestore((ctx))
#define ARM_VFP_CTXSHOW(op, fd, ctx)    op->AFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARMFPU_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
