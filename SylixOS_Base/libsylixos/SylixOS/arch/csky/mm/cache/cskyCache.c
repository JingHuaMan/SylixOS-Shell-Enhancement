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
** 描        述: C-SKY 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY 体系架构
*********************************************************************************************************/
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cskyCache.h"
#include "arch/csky/inc/cskyregs.h"
#include "arch/csky/arch_mmu.h"
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_CSKY_CACHE_L2 > 0
#include "../l2/cskyL2.h"
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
#define L1_CACHE_I_EN                      0x01
#define L1_CACHE_D_EN                      0x02
#define L1_CACHE_EN                        (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS                       0x00

static INT      iCacheStatus = L1_CACHE_DIS;
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
/*********************************************************************************************************
  CACHE 配置
*********************************************************************************************************/
static UINT32   uiCacheCfg = 0;
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  cskyICacheInvalidateAll(VOID);
extern VOID  cskyICacheEnableHw(VOID);
extern VOID  cskyICacheDisableHw(VOID);

extern VOID  cskyDCacheInvalidateAll(VOID);
extern VOID  cskyDCacheClearAll(VOID);
extern VOID  cskyDCacheFlushAll(VOID);
extern VOID  cskyDCacheDisableHw(VOID);
extern VOID  cskyDCacheEnableHw(UINT32  uiCacheCfg);

extern VOID  cskyBranchPredictorInvalidate(VOID);
extern VOID  cskyBranchPredictionEnable(VOID);
extern VOID  cskyBranchPredictionDisable(VOID);
/*********************************************************************************************************
  CACHE 参数
*********************************************************************************************************/
typedef struct {
    UINT32      CACHE_uiSize;                                           /*  CACHE 大小                  */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE 行大小                */
    UINT32      CACHE_uiWaySize;                                        /*  路大小                      */
} CSKY_CACHE;

static CSKY_CACHE _G_ICacheInfo = {
    8 * LW_CFG_KB_SIZE,                                                 /*  8KB 的四路组相联 ICACHE     */
    32,
    2 * LW_CFG_KB_SIZE
};

static CSKY_CACHE _G_DCacheInfo = {
    8 * LW_CFG_KB_SIZE,                                                 /*  8KB 的四路组相联 DCACHE     */
    32,
    2 * LW_CFG_KB_SIZE
};

#define CSKY_ICACHE_SIZE                   _G_ICacheInfo.CACHE_uiSize
#define CSKY_DCACHE_SIZE                   _G_DCacheInfo.CACHE_uiSize

#define CSKY_ICACHE_LINE_SIZE              _G_ICacheInfo.CACHE_uiLineSize
#define CSKY_DCACHE_LINE_SIZE              _G_DCacheInfo.CACHE_uiLineSize

#define CSKY_ICACHE_WAY_SIZE               _G_ICacheInfo.CACHE_uiWaySize
#define CSKY_DCACHE_WAY_SIZE               _G_DCacheInfo.CACHE_uiWaySize

#define CSKY_L1_CACHE_LOOP_OP_MAX_SIZE     (CSKY_DCACHE_SIZE >> 1)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
** 函数名称: cskyCacheEnable
** 功能描述: C-SKY 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyICacheEnableHw();                                           /*  使能 ICACHE                 */

        cskyBranchPredictionEnable();

    } else {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyDCacheEnableHw(uiCacheCfg);                                 /*  使能 DCACHE                 */
    }

#if LW_CFG_CSKY_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        cskyL2Enable();
    }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheDisable
** 功能描述: C-SKY 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : DCACHE 为写穿透模式, 不用回写.
*********************************************************************************************************/
static INT  cskyCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyICacheDisableHw();                                          /*  禁能 ICACHE                 */

        cskyBranchPredictionDisable();

    } else {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyDCacheDisableHw();                                          /*  禁能 DCACHE                 */
    }

#if LW_CFG_CSKY_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        cskyL2Disable();
    }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheOp
** 功能描述: CACHE 操作
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
**           uiVal          操作参数
**           pCacheOp       操作函数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyCacheOp (PVOID        pvStart,
                          PVOID        pvEnd,
                          size_t       uiStep,
                          UINT32       uiVal,
                          VOIDFUNCPTR  pCacheOp)
{    
    UINT32   uiCr22Value      = (UINT32)pvStart;
    UINT32   uiCr17Value      = M_CFR_OMS | uiVal;
    UINT32   uiCr17ReadValue  = 0;
    UINT32   uiTmp            = 0;
    UINT32   i;
   
    SET_CIR(uiCr22Value);                                               /*  写入起始虚拟地址            */
    GET_CFR(uiCr17Value, uiCr17ReadValue, uiTmp);
    if (unlikely((uiCr17ReadValue & M_CFR_LICF) != 0)) {                /*  出现了操作异常              */
#if LW_CFG_CSKY_HARD_TLB_REFILL > 0
        pCacheOp();
        return;
#else
        LDW_ADDR(pvStart, uiTmp);
#endif
    }

    uiCr22Value = (UINT32)pvEnd - 1;                                    /*  写入结束虚拟地址            */
    SET_CIR(uiCr22Value);
    GET_CFR(uiCr17Value, uiCr17ReadValue, uiTmp);
    if (unlikely((uiCr17ReadValue & M_CFR_LICF) != 0)) {                /*  出现了操作异常              */
#if LW_CFG_CSKY_HARD_TLB_REFILL > 0
        pCacheOp();
        return;
#else
        LDW_ADDR(pvEnd - 1, uiTmp);
#endif
    }
    
    for (i = (UINT32)pvStart; i < (UINT32)pvEnd; i += uiStep) {
       SET_CIR(i);
       SET_CFR(uiCr17Value);
    }
}
/*********************************************************************************************************
** 函数名称: cskyDCacheFlush
** 功能描述: DCACHE 脏数据回写
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyDCacheFlush (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_CLR,
                cskyDCacheFlushAll);
}
/*********************************************************************************************************
** 函数名称: cskyDCacheClear
** 功能描述: DCACHE 脏数据回写并无效
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyDCacheClear (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_CLR | M_CFR_INV,
                cskyDCacheClearAll);
}
/*********************************************************************************************************
** 函数名称: cskyDCacheInvalidate
** 功能描述: DCACHE 脏数据置无效
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyDCacheInvalidate (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_INV,
                cskyDCacheInvalidateAll);
}
/*********************************************************************************************************
** 函数名称: cskyICacheInvalidate
** 功能描述: ICACHE 脏数据置无效
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE VOID  cskyICacheInvalidate (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_I | M_CFR_INV,
                cskyICacheInvalidateAll);
}
/*********************************************************************************************************
** 函数名称: cskyCacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
INT  cskyCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  全部回写                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2FlushAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  全部回写                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT  cskyCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  仅无效对齐部分              */
                cskyDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2Invalidate(pvAdrs, stBytes);                          /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  仅无效对齐部分              */
                cskyDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2Invalidate(pvPdrs, stBytes);                          /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT  cskyCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  全部回写并无效              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写并无效              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2ClearAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  全部回写并无效              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写并无效              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: cskyCacheUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: cskyCacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  cskyCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
        cskyDCacheClearAll();                                           /*  DCACHE 全部回写             */
        cskyICacheInvalidateAll();                                      /*  ICACHE 全部无效             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
        cskyDCacheFlush(pvAdrs, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);

        CSKY_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
        cskyICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  cskyCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  全部回写                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写                    */
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  全部回写                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  部分回写                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyCacheInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  cskyCacheInit (LW_CACHE_OP *pcacheop,
                     CACHE_MODE   uiInstruction,
                     CACHE_MODE   uiData,
                     CPCHAR       pcMachineName)
{
    if (uiData & CACHE_COPYBACK) {
        uiCacheCfg |= M_CACHE_CFG_WB;
    }

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
    if (LW_NCPUS > 1) {
        uiCacheCfg |= M_CACHE_CFG_WA;                                   /*  多核使能 CACHE 写分配       */
    }
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    pcacheop->CACHEOP_iICacheLine = CSKY_ICACHE_LINE_SIZE;
    pcacheop->CACHEOP_iDCacheLine = CSKY_DCACHE_LINE_SIZE;

    pcacheop->CACHEOP_iICacheWaySize = CSKY_ICACHE_WAY_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize = CSKY_ICACHE_WAY_SIZE;

    pcacheop->CACHEOP_pfuncEnable  = cskyCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = cskyCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = cskyCacheLock;                      /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock = cskyCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = cskyCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = cskyCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = cskyCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = cskyCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = cskyCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = cskyCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = cskyCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = cskyCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: cskyCacheReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  cskyCacheReset (CPCHAR  pcMachineName)
{
    cskyICacheInvalidateAll();
    cskyDCacheDisableHw();
    cskyICacheDisableHw();
    cskyBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_CSKY_ARCH_CK803__*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
