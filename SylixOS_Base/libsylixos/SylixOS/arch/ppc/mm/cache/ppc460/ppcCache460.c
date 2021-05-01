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
** 文   件   名: ppcCache460.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2019 年 08 月 21 日
**
** 描        述: PowerPC 460 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID     ppc460DCacheDisable(VOID);
extern VOID     ppc460DCacheEnable(VOID);
extern VOID     ppc460ICacheDisable(VOID);
extern VOID     ppc460ICacheEnable(VOID);

extern VOID     ppc460DCacheInvalidateAll(VOID);
extern VOID     ppc460ICacheInvalidateAll(VOID);

extern VOID     ppc460DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc460BranchPredictionDisable(VOID);
extern VOID     ppc460BranchPredictionEnable(VOID);
extern VOID     ppc460BranchPredictorInvalidate(VOID);

extern VOID     ppc460TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                 UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: bspCacheSetSize
** 功能描述: 获得 CACHE Set 的数目
** 输　入  : NONE
** 输　出  : CACHE Set 的数目
** 全局变量:
** 调用模块:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspCacheSetSize (VOID)
{
    return  (8);                                                        /*  8 / 16                      */
}
/*********************************************************************************************************
** 函数名称: ppc460CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               ICACHE 信息
**           pDCache               DCACHE 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppc460CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_460) == 0) {
        pICache->CACHE_uiLineSize  = 32;
        pICache->CACHE_uiWayNr     = 64;
        pICache->CACHE_uiSetNr     = bspCacheSetSize();
        pICache->CACHE_uiSize      = pICache->CACHE_uiSetNr * pICache->CACHE_uiWayNr * \
                                     pICache->CACHE_uiLineSize;
        pICache->CACHE_uiWaySize   = pICache->CACHE_uiSetNr * pICache->CACHE_uiLineSize;

        pDCache->CACHE_uiLineSize  = 32;
        pDCache->CACHE_uiWayNr     = 64;
        pDCache->CACHE_uiSetNr     = bspCacheSetSize();
        pDCache->CACHE_uiSize      = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiWayNr * \
                                     pDCache->CACHE_uiLineSize;
        pDCache->CACHE_uiWaySize   = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  460 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc460CacheDriver = {
    "460",
    ppc460CacheProbe,

    ppc460DCacheDisable,
    ppc460DCacheEnable,
    ppc460ICacheDisable,
    ppc460ICacheEnable,

    LW_NULL,
    LW_NULL,
    ppc460ICacheInvalidateAll,

    ppc460DCacheClear,
    ppc460DCacheFlush,
    ppc460DCacheInvalidate,
    ppc460ICacheInvalidate,

    ppc460BranchPredictionDisable,
    ppc460BranchPredictionEnable,
    ppc460BranchPredictorInvalidate,

    ppc460TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
