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
** 文   件   名: ppcL2Cache750.c
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 03 月 08 日
**
** 描        述: MPC750 体系构架 L2 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0
#include "../ppcL2.h"
#include "ppcL2Cache750.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppc750L2CacheInitHw(UINT32  uiL2CR);
extern VOID  ppc750L2CacheEnable(VOID);
extern VOID  ppc750L2CacheDisable(VOID);
extern BOOL  ppc750L2CacheIsEnable(VOID);
extern VOID  ppc750L2CacheInvalidateAll(VOID);
extern VOID  ppc750L2CacheFlushAllSW(size_t  stSize, UINT8  *pucReadBuffer);

extern VOID  ppc745xL2CacheInitHw(UINT32  uiL2CR);
extern VOID  ppc745xL2CacheEnable(VOID);
extern VOID  ppc745xL2CacheDisable(VOID);
extern BOOL  ppc745xL2CacheIsEnable(VOID);
extern VOID  ppc745xL2CacheInvalidateAll(VOID);
extern VOID  ppc745xL2CacheFlushAllHW(VOID);
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
extern UINT8                   *_G_pucPpcCacheReadBuffer;
/*********************************************************************************************************
  L2 CACHE 配置(默认不存在 L2 CACHE，其它值为配置示例)
*********************************************************************************************************/
static PPC750_L2CACHE_CONFIG    _G_l2Config = {
    .CFG_bPresent   = LW_FALSE,
    .CFG_stSize     = L2RAM_SIZE_1M,
    .CFG_uiL2CR     = L2CR_SIZE_1MB |
                      L2CR_RAM_PB2  |
                      L2CR_OH_0_5ns,
};
/*********************************************************************************************************
** 函数名称: ppc750L2CacheConfig
** 功能描述: L2 CACHE 配置
** 输　入  : pL2Config         L2 CACHE 配置
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppc750L2CacheConfig (PPC750_L2CACHE_CONFIG  *pL2Config)
{
    if (pL2Config) {
        _G_l2Config = *pL2Config;
    }
}
/*********************************************************************************************************
** 函数名称: __ppc750L2CacheFlushAllSW
** 功能描述: L2 CACHE 软件回写并无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ppc750L2CacheFlushAllSW (VOID)
{
    ppc750L2CacheFlushAllSW((_G_l2Config.CFG_stSize * 2) / L1CACHE_ALIGN_SIZE,
                            _G_pucPpcCacheReadBuffer);
}
/*********************************************************************************************************
** 函数名称: ppc750L2CacheInit
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
**           uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppc750L2CacheInit (L2C_DRVIER  *pl2cdrv,
                         CACHE_MODE   uiInstruction,
                         CACHE_MODE   uiData,
                         CPCHAR       pcMachineName)
{
    if (_G_l2Config.CFG_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n",
                     LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

        /*
         * See <<PowerPC 750 RISC Microprocessor User's Manual>>
         * The icbi instruction invalidates only L1 CACHE blocks and is never forwarded to the L2 CACHE.
         * 所以 L2 CACHE 只能是 DCACHE only，必须设置 DO 位
         */
        if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
            _G_l2Config.CFG_uiL2CR  |= L2CR_DO;
            _G_l2Config.CFG_uiL2CR <<= 16;

            pl2cdrv->L2CD_pcName             = pcMachineName;
            pl2cdrv->L2CD_stSize             = _G_l2Config.CFG_stSize;
            pl2cdrv->L2CD_pfuncEnable        = ppc750L2CacheEnable;
            pl2cdrv->L2CD_pfuncDisable       = ppc750L2CacheDisable;
            pl2cdrv->L2CD_pfuncIsEnable      = ppc750L2CacheIsEnable;
            pl2cdrv->L2CD_pfuncSync          = LW_NULL;
            pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
            pl2cdrv->L2CD_pfuncFlushAll      = __ppc750L2CacheFlushAllSW;
            pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
            pl2cdrv->L2CD_pfuncInvalidateAll = ppc750L2CacheInvalidateAll;
            pl2cdrv->L2CD_pfuncClear         = LW_NULL;
            pl2cdrv->L2CD_pfuncClearAll      = pl2cdrv->L2CD_pfuncFlushAll;

            ppc750L2CacheInitHw(_G_l2Config.CFG_uiL2CR);

        } else {
            _G_l2Config.CFG_uiL2CR  |= PPC7450_L2CR_DO;
            _G_l2Config.CFG_uiL2CR <<= 16;

            pl2cdrv->L2CD_pcName             = pcMachineName;
            pl2cdrv->L2CD_stSize             = _G_l2Config.CFG_stSize;
            pl2cdrv->L2CD_pfuncEnable        = ppc745xL2CacheEnable;
            pl2cdrv->L2CD_pfuncDisable       = ppc745xL2CacheDisable;
            pl2cdrv->L2CD_pfuncIsEnable      = ppc745xL2CacheIsEnable;
            pl2cdrv->L2CD_pfuncSync          = LW_NULL;
            pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
            pl2cdrv->L2CD_pfuncFlushAll      = ppc745xL2CacheFlushAllHW;
            pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
            pl2cdrv->L2CD_pfuncInvalidateAll = ppc745xL2CacheInvalidateAll;
            pl2cdrv->L2CD_pfuncClear         = LW_NULL;
            pl2cdrv->L2CD_pfuncClearAll      = pl2cdrv->L2CD_pfuncFlushAll;

            ppc745xL2CacheInitHw(_G_l2Config.CFG_uiL2CR);
        }
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
