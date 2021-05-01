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
** 文   件   名: arm64VfpV4.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 07 月 03 日
**
** 描        述: ARM64 体系架构 VFPv4 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../arm64Fpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static ARM64_FPU_OP     _G_fpuopVfpV4;
static INT              _G_iVfpV4DNum;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID     arm64VfpV4HwInit(VOID);
extern UINT32   arm64VfpV4Mvfr0EL1(VOID);
extern VOID     arm64VfpV4Enable(VOID);
extern VOID     arm64VfpV4Disable(VOID);
extern BOOL     arm64VfpV4IsEnable(VOID);
extern VOID     arm64VfpV4Save(PVOID pvFpuCtx);
extern VOID     arm64VfpV4Restore(PVOID pvFpuCtx);
/*********************************************************************************************************
** 函数名称: arm64VfpV4CtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  arm64VfpV4CtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT              i;
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    
    fdprintf(iFd,    "FPCR      = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpcr);
    fdprintf(iFd,    "FPSR      = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpsr);
  
    for (i = 0; i < _G_iVfpV4DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: arm64VfpV4PrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PARM64_FPU_OP  arm64VfpV4PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    UINT32  uiMvfr0;
    
    arm64VfpV4HwInit();
    
    uiMvfr0  = arm64VfpV4Mvfr0EL1();
    uiMvfr0 &= 0xf;
    
    if (uiMvfr0 == 0x2) {
        _G_iVfpV4DNum = 64;
        _G_fpuopVfpV4.AFPU_pfuncSave    = arm64VfpV4Save;
        _G_fpuopVfpV4.AFPU_pfuncRestore = arm64VfpV4Restore;
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown vfp register number.\r\n");
        return  (LW_NULL);
    }
    
    _G_fpuopVfpV4.AFPU_pfuncEnable   = arm64VfpV4Enable;
    _G_fpuopVfpV4.AFPU_pfuncDisable  = arm64VfpV4Disable;
    _G_fpuopVfpV4.AFPU_pfuncIsEnable = arm64VfpV4IsEnable;
    _G_fpuopVfpV4.AFPU_pfuncCtxShow  = arm64VfpV4CtxShow;

    return  (&_G_fpuopVfpV4);
}
/*********************************************************************************************************
** 函数名称: arm64VfpV4SecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  arm64VfpV4SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
    
    arm64VfpV4HwInit();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
