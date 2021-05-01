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
** 文   件   名: cskyVfp.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系架构 VFP 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../cskyFpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static CSKY_FPU_OP  _G_fpuopVfp;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID  cskyVfpInit(VOID);
extern VOID  cskyVfpSave(PVOID  pvFpuCtx);
extern VOID  cskyVfpRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** 函数名称: cskyVfpEnable
** 功能描述: 使能 VFP
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyVfpEnable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: cskyVfpDisable
** 功能描述: 禁能 VFP
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyVfpDisable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: cskyVfpIsEnable
** 功能描述: 判断 VFP 是否使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  cskyVfpIsEnable (VOID)
{
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: cskyVfpCtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyVfpCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;
    
    fdprintf(iFd, "FCR  = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcr);
    fdprintf(iFd, "FESR = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpesr);

    for (i = 0; i < FPU_REG_NR; i++) {
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
        fdprintf(iFd, "FP%02d = 0x%08x\n", i,
                 pcpufpuCtx->FPUCTX_uiDreg[i].val32[0]);
#else
        fdprintf(iFd, "FP%02d = 0x%08x0x%08x\n", i,
                 pcpufpuCtx->FPUCTX_uiDreg[i].val32[0],
                 pcpufpuCtx->FPUCTX_uiDreg[i].val32[1]);
#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
    }
#endif
}
/*********************************************************************************************************
** 函数名称: cskyVfpPrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCSKY_FPU_OP  cskyVfpPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    cskyVfpInit();

    _G_fpuopVfp.CFPU_pfuncEnable   = cskyVfpEnable;
    _G_fpuopVfp.CFPU_pfuncDisable  = cskyVfpDisable;
    _G_fpuopVfp.CFPU_pfuncIsEnable = cskyVfpIsEnable;
    _G_fpuopVfp.CFPU_pfuncCtxShow  = cskyVfpCtxShow;
    _G_fpuopVfp.CFPU_pfuncSave     = cskyVfpSave;
    _G_fpuopVfp.CFPU_pfuncRestore  = cskyVfpRestore;

    return  (&_G_fpuopVfp);
}
/*********************************************************************************************************
** 函数名称: cskyVfpSecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  cskyVfpSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    cskyVfpInit();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
