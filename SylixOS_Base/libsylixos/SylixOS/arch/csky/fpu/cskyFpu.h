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
** 文   件   名: cskyFpu.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/

#ifndef __ARCH_CSKYFPU_H
#define __ARCH_CSKYFPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  C-SKY FPU 操作函数
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
  C-SKY FPU 基本操作
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
