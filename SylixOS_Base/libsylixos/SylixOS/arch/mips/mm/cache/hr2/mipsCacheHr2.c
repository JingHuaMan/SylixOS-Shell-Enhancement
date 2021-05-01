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
** ��   ��   ��: mipsCacheHr2.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 14 ��
**
** ��        ��: ��� 2 �Ŵ����� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  hr2CacheEnableHw(VOID);
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT  _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
** ��������: hr2BranchPredictionDisable
** ��������: ���ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  hr2BranchPredictionDisable (VOID)
{
}
/*********************************************************************************************************
** ��������: hr2BranchPredictionEnable
** ��������: ʹ�ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  hr2BranchPredictionEnable (VOID)
{
}
/*********************************************************************************************************
** ��������: hr2BranchPredictorInvalidate
** ��������: ��Ч��֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  hr2BranchPredictorInvalidate (VOID)
{
}
/*********************************************************************************************************
** ��������: hr2CacheFlushAll
** ��������: ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  hr2CacheFlushAll (VOID)
{
    /*
     * Not in opensource version
     */
}
/*********************************************************************************************************
** ��������: hr2CacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  hr2CacheEnable (LW_CACHE_TYPE  cachetype)
{
    /*
     * Not in opensource version
     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  hr2CacheDisable (LW_CACHE_TYPE  cachetype)
{
    /*
     * Not in opensource version
     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheFlushNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  hr2CacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheFlushPageNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  hr2CacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheInvalidateNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  hr2CacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheInvalidatePageNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  hr2CacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                        PVOID          pvAdrs,
                                        PVOID          pvPdrs,
                                        size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheClearNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  hr2CacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheClearPageNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  hr2CacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  hr2CacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: hr2CacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  hr2CacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: hr2CacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  hr2CacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hr2CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  hr2CacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheHr2Init
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheHr2Init (LW_CACHE_OP  *pcacheop,
                        CACHE_MODE    uiInstruction,
                        CACHE_MODE    uiData,
                        CPCHAR        pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    mipsCacheInfoShow();                                                /*  ��ӡ CACHE ��Ϣ             */
    /*
     * ���ܹر� CACHE
     */
    hr2BranchPredictorInvalidate();                                     /*  ��Ч��֧Ԥ��                */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  ���� TEXT_UPDATE_MP ѡ��    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;                      /*  VIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWaySize;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWaySize;

    pcacheop->CACHEOP_pfuncEnable  = hr2CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = hr2CacheDisable;
    /*
     * CETC-HR2 ʵ���˸��� CACHE ��Ӳ��һ���ԣ����� CACHE �������� NONE ����
     */
    pcacheop->CACHEOP_pfuncFlush          = hr2CacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = hr2CacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = hr2CacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = hr2CacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = hr2CacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = hr2CacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = hr2CacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = hr2CacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = hr2CacheLockNone;           /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock         = hr2CacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mipsCacheHr2Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsCacheHr2Reset (CPCHAR  pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    /*
     * ���ܹر� CACHE
     */
    hr2BranchPredictorInvalidate();                                     /*  ��Ч��֧Ԥ��                */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
