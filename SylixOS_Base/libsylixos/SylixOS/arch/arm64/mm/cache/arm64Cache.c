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
** 描        述: ARMv8 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arm64Cache.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID    arm64ICacheEnable(VOID);
extern VOID    arm64DCacheEnable(VOID);
extern VOID    arm64ICacheDisable(VOID);
extern VOID    arm64DCacheDisable(VOID);
extern VOID    arm64DCacheFlushAll(VOID);
extern VOID    arm64DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID    arm64DCacheFlushPoU(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID    arm64ICacheInvalidateAll(VOID);
extern VOID    arm64ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID    arm64DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID    arm64DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID    arm64DCacheClearAll(VOID);
extern UINT32  arm64CacheCCSIDR(VOID);
/*********************************************************************************************************
  选择 CACHE 类型
*********************************************************************************************************/
#define ARM64_CSSELR_IND_DATA_UNIFIED   0
#define ARM64_CSSELR_IND_INSTRUCTION    1
extern VOID    arm64CacheSetCSSELR(UINT32  uiValue);
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE 参数
*********************************************************************************************************/
static UINT32                           uiArmV8ICacheLineSize;
static UINT32                           uiArmV8DCacheLineSize;
#define ARMv8_CACHE_LOOP_OP_MAX_SIZE    (32 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** 函数名称: arm64CacheEnable
** 功能描述: 使能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        arm64ICacheEnable();

    } else {
        arm64DCacheEnable();       
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheDisable
** 功能描述: 禁能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        arm64ICacheDisable();
        
    } else {
        arm64DCacheDisable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT  arm64CacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT  arm64CacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  仅无效对齐部分              */
                arm64DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  仅无效对齐部分              */
                arm64DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT  arm64CacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写并无效              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写并无效              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheLock
** 功能描述: 锁定指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arm64CacheUnlock
** 功能描述: 解锁指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  arm64CacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arm64CacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  arm64CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
        arm64DCacheFlushAll();                                          /*  DCACHE 全部回写             */
        arm64ICacheInvalidateAll();                                     /*  ICACHE 全部无效             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
        arm64DCacheFlushPoU(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV8ICacheLineSize);
        arm64ICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV8ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  arm64CacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写                    */
        }
    
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64CacheInit
** 功能描述: 初始化 CACHE 
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  arm64CacheInit (LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    UINT32  uiICCSIDR;
    UINT32  uiDCCSIDR;

#define ARMv8_CCSIDR_LINESIZE_MASK      0x7
#define ARMv8_CCSIDR_LINESIZE(x)        ((x) & ARMv8_CCSIDR_LINESIZE_MASK)
#define ARMv8_CACHE_LINESIZE(x)         (16 << ARMv8_CCSIDR_LINESIZE(x))

#define ARMv8_CCSIDR_NUMSET_MASK        0xfffe000
#define ARMv8_CCSIDR_NUMSET(x)          ((x) & ARMv8_CCSIDR_NUMSET_MASK)
#define ARMv8_CACHE_NUMSET(x)           ((ARMv8_CCSIDR_NUMSET(x) >> 13) + 1)

#define ARMv8_CCSIDR_WAYNUM_MSK         0x1ff8
#define ARMv8_CCSIDR_WAYNUM(x)          ((x) & ARMv8_CCSIDR_WAYNUM_MSK)
#define ARMv8_CACHE_WAYNUM(x)           ((ARMv8_CCSIDR_NUMSET(x) >> 3) + 1)

    if ((uiData & CACHE_SNOOP_ENABLE) && !arm64MmuShareableGet()) {
        _DebugFormat(__ERRORMESSAGE_LEVEL,
                     "ARMv8 D-Cache initialize with 'CACHE_SNOOP_ENABLE' flag"
                     " you MUST set OUTER_SHAREABLE.\r\n");
    }

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    arm64CacheSetCSSELR(ARM64_CSSELR_IND_INSTRUCTION);
    uiICCSIDR = arm64CacheCCSIDR();

    arm64CacheSetCSSELR(ARM64_CSSELR_IND_DATA_UNIFIED);
    uiDCCSIDR = arm64CacheCCSIDR();

    pcacheop->CACHEOP_iICacheLine = ARMv8_CACHE_LINESIZE(uiICCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv8_CACHE_LINESIZE(uiDCCSIDR);
    
    uiArmV8ICacheLineSize = pcacheop->CACHEOP_iICacheLine;
    uiArmV8DCacheLineSize = pcacheop->CACHEOP_iDCacheLine;
    
    pcacheop->CACHEOP_iICacheWaySize = uiArmV8ICacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiICCSIDR);   /*  ICACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV8DCacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiDCCSIDR);   /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    
    pcacheop->CACHEOP_pfuncEnable  = arm64CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = arm64CacheDisable;
    
    pcacheop->CACHEOP_pfuncLock    = arm64CacheLock;                    /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock  = arm64CacheUnlock;
    
    pcacheop->CACHEOP_pfuncFlush          = arm64CacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = arm64CacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = arm64CacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = arm64CacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = arm64CacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = arm64CacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = arm64CacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = arm64CacheDataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archCacheV8Reset
** 功能描述: 复位 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
VOID  arm64CacheReset (CPCHAR  pcMachineName)
{
    arm64ICacheInvalidateAll();
    arm64DCacheDisable();
    arm64ICacheDisable();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
