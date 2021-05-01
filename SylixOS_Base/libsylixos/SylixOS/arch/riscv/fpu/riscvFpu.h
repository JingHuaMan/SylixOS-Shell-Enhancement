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
** ��   ��   ��: riscvFpu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_RISCV_FPU_H
#define __ARCH_RISCV_FPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  RISC-V fpu ��������
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
} RISCV_FPU_OP;
typedef RISCV_FPU_OP *PRISCV_FPU_OP;

/*********************************************************************************************************
  RISC-V fpu ��������
*********************************************************************************************************/

#define RISCV_VFP_ENABLE(op)              op->SFPU_pfuncEnable()
#define RISCV_VFP_DISABLE(op)             op->SFPU_pfuncDisable()
#define RISCV_VFP_ISENABLE(op)            op->SFPU_pfuncIsEnable()
#define RISCV_VFP_SAVE(op, ctx)           op->SFPU_pfuncSave((ctx))
#define RISCV_VFP_RESTORE(op, ctx)        op->SFPU_pfuncRestore((ctx))
#define RISCV_VFP_CTXSHOW(op, fd, ctx)    op->SFPU_pfuncCtxShow((fd), (ctx))
#define RISCV_VFP_ENABLE_TASK(op, tcb)    op->SFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_RISCV_FPU_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
