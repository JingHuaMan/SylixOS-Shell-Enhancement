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
** 文   件   名: cskyCache.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系架构 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/cskyCache.h"
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
    LW_CACHE_OP  *pcacheop = API_CacheGetLibBlock();

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L1 cache controller initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, CSKY_MACHINE_510) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_610) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_801) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_802) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_803) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_807) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_810) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0)) {
        cskyCacheInit(pcacheop, uiInstruction, uiData, pcMachineName);

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
    if ((lib_strcmp(pcMachineName, CSKY_MACHINE_510) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_610) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_801) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_802) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_803) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_807) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_810) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0)) {
        cskyCacheReset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
