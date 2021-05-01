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
** ��   ��   ��: x86Cache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 18 ��
**
** ��        ��: x86 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/x86/common/x86Topology.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static INT      _G_iX86CacheStatus    = L1_CACHE_DIS;                   /*  L1 CACHE ״̬               */
static FUNCPTR  _G_pfuncX86CacheFlush = LW_NULL;                        /*  CACHE FLUSH ����ָ��        */
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  x86CacheEnableHw(VOID);
extern VOID  x86CacheDisableHw(VOID);
extern VOID  x86CacheResetHw(VOID);
extern VOID  x86CacheFlushX86Hw(VOID);
extern VOID  x86CacheFlushPen4Hw(PVOID  pvAdrs, size_t  stBytes);
extern VOID  x86CacheClearPen4Hw(PVOID  pvAdrs, size_t  stBytes);
/*********************************************************************************************************
** ��������: x86CacheEnableNone
** ��������: ʹ�� CACHE (α����)
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheEnableNone (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus |= L1_CACHE_D_EN;
        }
    }

    if (_G_iX86CacheStatus == L1_CACHE_EN) {
        x86CacheEnableHw();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheDisableNone
** ��������: ���� CACHE (α����)
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheDisableNone (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus &= ~L1_CACHE_D_EN;
        }
    }

    if (_G_iX86CacheStatus == L1_CACHE_DIS) {
        x86CacheDisableHw();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlushNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheFlushX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        X86_WBINVD();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlushPen4
** ��������: CACHE �����ݻ�д ����4����ר��
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheFlushPen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlushPageNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheFlushPageX86 (LW_CACHE_TYPE  cachetype,
                                  PVOID          pvAdrs,
                                  PVOID          pvPdrs,
                                  size_t         stBytes)
{
    if (cachetype == DATA_CACHE) {
        X86_WBINVD();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheFlushPagePen4
** ��������: CACHE �����ݻ�д ����4����ר��
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheFlushPagePen4 (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    if (cachetype == DATA_CACHE) {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidateNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheInvalidateX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidatePen4
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) ����4����ר��
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheInvalidatePen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidatePageNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                        PVOID          pvAdrs,
                                        PVOID          pvPdrs,
                                        size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheInvalidatePageX86 (LW_CACHE_TYPE  cachetype,
                                       PVOID          pvAdrs,
                                       PVOID          pvPdrs,
                                       size_t         stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) ����4����ר��
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheInvalidatePagePen4 (LW_CACHE_TYPE  cachetype,
                                        PVOID          pvAdrs,
                                        PVOID          pvPdrs,
                                        size_t         stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClearNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheClearX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClearPen4
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) ����4����ר��
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheClearPen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClearPageNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86CacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT	x86CacheClearPageX86 (LW_CACHE_TYPE  cachetype,
                                  PVOID          pvAdrs,
                                  PVOID          pvPdrs,
                                  size_t         stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheClearPagePen4
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CacheClearPagePen4 (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	x86CacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: x86CacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	x86CacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: x86CacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	x86CacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  x86CacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  x86CacheDataUpdateX86 (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    X86_WBINVD();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE) ����4����ר��
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	x86CacheDataUpdatePen4 (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    if (bInv) {
        x86CacheClearPen4Hw(pvAdrs, stBytes);
    } else {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86CacheInit
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  x86CacheInit (LW_CACHE_OP  *pcacheop,
                    CACHE_MODE    uiInstruction,
                    CACHE_MODE    uiData,
                    CPCHAR        pcMachineName)
{
    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  ���� TEXT_UPDATE_MP ѡ��    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;                      /*  PIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    
    pcacheop->CACHEOP_iICacheLine     = X86_FEATURE_CACHE_FLUSH_BYTES;
    pcacheop->CACHEOP_iDCacheLine     = X86_FEATURE_CACHE_FLUSH_BYTES;

    pcacheop->CACHEOP_iICacheWaySize  = X86_FEATURE_ICACHE_WAY_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize  = X86_FEATURE_DCACHE_WAY_SIZE;

    /*
     * Ĭ������ CACHE �������� NONE ����
     */
    pcacheop->CACHEOP_pfuncEnable         = x86CacheEnableNone;
    pcacheop->CACHEOP_pfuncDisable        = x86CacheDisableNone;
    
    pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = x86CacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = x86CacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = x86CacheLockNone;           /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock         = x86CacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    /*
     * ����������������Ӧ�� CACHE ����
     * i386 û�� CACHE, ����̫������, ��������֧��, ��Ϊ���� CACHE
     */
    pcacheop->CACHEOP_pfuncEnable = x86CacheEnable;

    if (LW_NCPUS == 1) {                                                /*  ���˵� CPU                  */
        pcacheop->CACHEOP_pfuncDisable = x86CacheDisable;               /*  ���� CACHE �رպ���         */
                                                                        /*  �����µĺ���                */
        if ((uiData & CACHE_SNOOP_ENABLE) == 0) {                       /*  SNOOP ��ʹ�ܲ���Ҫ          */
            if (X86_FEATURE_HAS_CLFLUSH) {                              /*  �� CLFLUSH ָ��             */
                pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushPen4;
                pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPagePen4;
                pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidatePen4;
                pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePagePen4;
                pcacheop->CACHEOP_pfuncClear          = x86CacheClearPen4;
                pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPagePen4;
                pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdatePen4;

            } else {                                                    /*  �����ͺ�ʹ�� WBINVD ָ��    */
                pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushX86;
                pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPageX86;
                pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidateX86;
                pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePageX86;
                pcacheop->CACHEOP_pfuncClear          = x86CacheClearX86;
                pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPageX86;
                pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdateX86;
            }
        }
    }

    _G_pfuncX86CacheFlush = pcacheop->CACHEOP_pfuncFlush;               /*  ��¼ FLUSH ����             */
}
/*********************************************************************************************************
** ��������: x86CacheReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : i386 û�� CACHE, ����̫������, ��������֧��, ��Ϊ���� CACHE.
*********************************************************************************************************/
VOID  x86CacheReset (CPCHAR  pcMachineName)
{
#if LW_CFG_SMP_EN == 0
    x86CacheResetHw();
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: x86DCacheFlush
** ��������: D-CACHE ��д
** �䡡��  : pvStart         ��ʼ��ַ
**           stSize          ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86DCacheFlush (PVOID  pvStart, size_t  stSize)
{
    if (_G_pfuncX86CacheFlush) {
        _G_pfuncX86CacheFlush(DATA_CACHE, pvStart, stSize);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
