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
** 文   件   名: arm64Cache.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 23 日
**
** 描        述: ARM64 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/arm64Cache.h"
/*********************************************************************************************************
** 函数名称: archCacheInit
** 功能描述: 初始化 CACHE 
** 输　入  : uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archCacheInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    LW_CACHE_OP *pcacheop = API_CacheGetLibBlock();
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L1 cache controller initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        arm64CacheInit(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}
/*********************************************************************************************************
** 函数名称: archCacheReset
** 功能描述: 复位 CACHE, MMU 初始化时需要调用此函数
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archCacheReset (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        arm64CacheReset(pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
