/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arm64Fpu.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: ARM64 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/

#ifndef __ARM64FPU_H
#define __ARM64FPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  ARM64 fpu 操作函数
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
  ARM64 fpu 基本操作
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
