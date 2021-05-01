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
** ��   ��   ��: armCacheV8.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 01 ��
**
** ��        ��: ARMv8 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv8A, R ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID     armDCacheV7Disable(VOID);
extern VOID     armDCacheV7FlushPoU(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     armDCacheV7FlushAll(VOID);
extern VOID     armDCacheV7FlushAllPoU(VOID);
extern VOID     armDCacheV7ClearAll(VOID);
extern UINT32   armCacheV7CCSIDR(VOID);
/*********************************************************************************************************
  ѡ�� CACHE ����
*********************************************************************************************************/
#define ARMV7_CSSELR_IND_DATA_UNIFIED   0
#define ARMV7_CSSELR_IND_INSTRUCTION    1
extern VOID     armCacheV7SetCSSELR(UINT32  uiValue);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static UINT32                           uiArmV8ICacheLineSize;
static UINT32                           uiArmV8DCacheLineSize;
#define ARMv8_CACHE_LOOP_OP_MAX_SIZE    (32 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: armCacheV8Enable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV8Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
        armBranchPredictionEnable();

    } else {
        armDCacheEnable();
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8Disable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV8Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
        armBranchPredictionDisable();
        
    } else {
        armDCacheV7Disable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT	armCacheV8Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV8FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT	armCacheV8Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV8InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT	armCacheV8Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV8ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8Lock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV8Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV8Unlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV8Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV8TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV8TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV7FlushAllPoU();                                       /*  DCACHE ȫ����д             */
        armICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
        armDCacheV7FlushPoU(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV8ICacheLineSize);
        armICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV8ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV8DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV8DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV8Init
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV8Init (LW_CACHE_OP *pcacheop,
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

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    armCacheV7SetCSSELR(ARMV7_CSSELR_IND_INSTRUCTION);
    uiICCSIDR = armCacheV7CCSIDR();

    armCacheV7SetCSSELR(ARMV7_CSSELR_IND_DATA_UNIFIED);
    uiDCCSIDR = armCacheV7CCSIDR();

    pcacheop->CACHEOP_iICacheLine = ARMv8_CACHE_LINESIZE(uiICCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv8_CACHE_LINESIZE(uiDCCSIDR);
    
    uiArmV8ICacheLineSize = (UINT32)pcacheop->CACHEOP_iICacheLine;
    uiArmV8DCacheLineSize = (UINT32)pcacheop->CACHEOP_iDCacheLine;
    
    pcacheop->CACHEOP_iICacheWaySize = uiArmV8ICacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiICCSIDR);   /*  ICACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV8DCacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiDCCSIDR);   /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {            /*  ARMv8                       */
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    }
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV8Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV8Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV8Lock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV8Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV8Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV8FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV8Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV8InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV8Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV8ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV8TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV8DataUpdate;
    
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
VOID  armCacheV8Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV7Disable();
    armICacheDisable();
    armBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
