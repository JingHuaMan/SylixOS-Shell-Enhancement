/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: arm64Cache.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 23 ��
**
** ��        ��: ARMv8 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arm64Cache.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  ��������
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
  ѡ�� CACHE ����
*********************************************************************************************************/
#define ARM64_CSSELR_IND_DATA_UNIFIED   0
#define ARM64_CSSELR_IND_INSTRUCTION    1
extern VOID    arm64CacheSetCSSELR(UINT32  uiValue);
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static UINT32                           uiArmV8ICacheLineSize;
static UINT32                           uiArmV8DCacheLineSize;
#define ARMv8_CACHE_LOOP_OP_MAX_SIZE    (32 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: arm64CacheEnable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: arm64CacheDisable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: arm64CacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT  arm64CacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  arm64CacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT  arm64CacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                arm64DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  arm64CacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                arm64DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                arm64DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT  arm64CacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  arm64CacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64ICacheInvalidateAll();                                 /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            arm64ICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  arm64CacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: arm64CacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  arm64CacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: arm64CacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  arm64CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
        arm64DCacheFlushAll();                                          /*  DCACHE ȫ����д             */
        arm64ICacheInvalidateAll();                                     /*  ICACHE ȫ����Ч             */
        
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
** ��������: arm64CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  arm64CacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheFlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheFlush(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            arm64DCacheClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            arm64DCacheClear(pvAdrs, (PVOID)ulEnd,
                             uiArmV8DCacheLineSize);                    /*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64CacheInit
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
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
    
    pcacheop->CACHEOP_pfuncLock    = arm64CacheLock;                    /*  ��ʱ��֧����������          */
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
** ��������: archCacheV8Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
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
