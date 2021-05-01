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
** ��   ��   ��: armCacheV4.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv4 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARM920 ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID  armDCacheV4Disable(VOID);
extern VOID  armDCacheV4FlushAll(VOID);
extern VOID  armDCacheV4ClearAll(VOID);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static UINT32                           uiArmV4ICacheLineSize;
static UINT32                           uiArmV4DCacheLineSize;
#define ARMv4_CACHE_LOOP_OP_MAX_SIZE    (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: armCacheV4Enable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV4Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
    
    } else {
        armDCacheEnable();
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Disable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV4Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
    
    } else {
        armDCacheV4Disable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV4ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & (uiArmV4DCacheLineSize - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(uiArmV4DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV4DCacheLineSize);
                ulStart += uiArmV4DCacheLineSize;
            }
            
            if (ulEnd & (uiArmV4DCacheLineSize - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(uiArmV4DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV4DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV4DCacheLineSize);
            }

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV4ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & (uiArmV4DCacheLineSize - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(uiArmV4DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV4DCacheLineSize);
                ulStart += uiArmV4DCacheLineSize;
            }
            
            if (ulEnd & (uiArmV4DCacheLineSize - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(uiArmV4DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV4DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV4DCacheLineSize);
            }

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV4ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV4ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Lock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV4Unlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV4TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV4TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV4FlushAll();                                          /*  DCACHE ȫ����д             */
        armICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
        armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);    /*  ���ֻ�д                    */

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV4ICacheLineSize);
        armICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV4ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV4DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV4DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV4DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV4Init
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV4Init (LW_CACHE_OP *pcacheop, 
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    static const INT     iCacheLineTbl[] = {8, 16, 32, 64};
                 UINT32  uiCacheType, uiICache, uiDCache;

    /*
     *   31 30 29 28   25  24 23     12 11      0
     * +---------+-------+---+---------+---------+
     * | 0  0  0 | ctype | S |  Dsize  |  Isize  |
     * +---------+-------+---+---------+---------+
     *
     * ctype The ctype field determines the cache type.
     *     S bit Specifies whether the cache is a unified cache or separate instruction and data caches.
     * Dsize Specifies the size, line length, and associativity of the data cache.
     * Isize Specifies the size, line length, and associativity of the instruction cache.
     *
     *  11 10 9 8    6 5     3  2  1   0
     * +-------+------+-------+---+-----+
     * | 0 0 0 | size | assoc | M | len |
     * +-------+------+-------+---+-----+
     *
     *  size The size field determines the cache size in conjunction with the M bit.
     * assoc The assoc field determines the cache associativity in conjunction with the M bit.
     *     M bit The multiplier bit. Determines the cache size and cache associativity values in
     *       conjunction with the size and assoc fields.
     *   len The len field determines the line length of the cache.
     */

    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;
    
    uiCacheType = armCacheTypeReg();
    uiICache = uiCacheType & 0xfff;
    uiDCache = (uiCacheType >> 12) & 0xfff;

    _BugHandle(((uiICache >> 6) & 0x7) == 0x6, LW_TRUE, "ARMv4 I-Cache must be 64-ways.\r\n");
    _BugHandle(((uiDCache >> 6) & 0x7) == 0x6, LW_TRUE, "ARMv4 D-Cache must be 64-ways.\r\n");

    _BugHandle(((uiICache >> 3) & 0x7) == 0x5, LW_TRUE, "ARMv4 I-Cache must be 16KBytes.\r\n");
    _BugHandle(((uiDCache >> 3) & 0x7) == 0x5, LW_TRUE, "ARMv4 D-Cache must be 16KBytes.\r\n");

    pcacheop->CACHEOP_iICacheLine = iCacheLineTbl[uiICache & 0x3];
    pcacheop->CACHEOP_iDCacheLine = iCacheLineTbl[uiDCache & 0x3];
    
    uiArmV4ICacheLineSize = pcacheop->CACHEOP_iICacheLine;
    uiArmV4DCacheLineSize = pcacheop->CACHEOP_iDCacheLine;

    pcacheop->CACHEOP_iICacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    pcacheop->CACHEOP_iDCacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv4 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv4 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = armCacheV4Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV4Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV4Lock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV4Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV4Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV4FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV4Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV4InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV4Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV4ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV4TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV4DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheV4Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV4Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV4Disable();
    armICacheDisable();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
