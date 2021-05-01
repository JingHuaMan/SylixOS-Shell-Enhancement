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
** 文   件   名: arm64Fpu.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: ARM64 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "arm64Fpu.h"
#include "vfpnone/arm64VfpNone.h"
#include "vfpv4/arm64VfpV4.h"
#include "vfpv4el2/arm64VfpV4El2.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit __attribute__ ((aligned(ARCH_FPU_CTX_ALIGN)));
static PARM64_FPU_OP    _G_pfpuop;
/*********************************************************************************************************
** 函数名称: archFpuPrimaryInit
** 功能描述: 主核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU pri-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        _G_pfpuop = arm64VfpNonePrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        _G_pfpuop = arm64VfpV4PrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4EL2) == 0) {
        _G_pfpuop = arm64VfpV4El2PrimaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
    
    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
    
    ARM64_VFP_ENABLE(_G_pfpuop);

    ARM64_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);
        
    ARM64_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuSecondaryInit
** 功能描述: 从核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU sec-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        arm64VfpNoneSecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        arm64VfpV4SecondaryInit(pcMachineName, pcFpuName);

    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4EL2) == 0) {
        arm64VfpV4El2SecondaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: archFpuCtxInit
** 功能描述: 初始化一个 Fpu 上下文控制块 (这里并没有使能 FPU)
** 输　入  : pvFpuCtx   FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuCtxInit (PVOID  pvFpuCtx)
{
    lib_memcpy(pvFpuCtx, &_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
}
/*********************************************************************************************************
** 函数名称: archFpuEnable
** 功能描述: 使能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuEnable (VOID)
{
    ARM64_VFP_ENABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuDisable
** 功能描述: 禁能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuDisable (VOID)
{
    ARM64_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuSave
** 功能描述: 保存 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuSave (PVOID  pvFpuCtx)
{
    ARM64_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuRestore
** 功能描述: 恢复 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuRestore (PVOID  pvFpuCtx)
{
    ARM64_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuCtxShow
** 功能描述: 显示 FPU 上下文.
** 输　入  : iFd       文件描述符
**           pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
    ARM64_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuUndHandle
** 功能描述: 系统发生 undef 异常时, 调用此函数. 
**           只有某个任务或者中断, 真正使用浮点运算时 (即运行到浮点运算指令产生异常)
**           这时才可以打开浮点运算库.
** 输　入  : ptcbCur   当前任务 TCB
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archFpuUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    if (LW_CPU_GET_CUR_NESTING() > 1) {                                 /*  中断中发生异常, 返回出错    */
        return  (PX_ERROR);
    }

    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        return  (PX_ERROR);
    }

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    ARM64_VFP_RESTORE(_G_pfpuop, ptcbCur->TCB_pvStackFP);               /*  使能 FPU, 初始化 FPU 寄存器 */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
