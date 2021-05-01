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
** ��   ��   ��: riscvCache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 12 ��
**
** ��        ��: RISC-V ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "riscvCache.h"
#include "arch/riscv/inc/sbi.h"
/*********************************************************************************************************
** ��������: riscvLocalICacheFlushAll
** ��������: ��Ч�������� I-CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  VOID  riscvLocalICacheFlushAll (VOID)
{
#if LW_CFG_RISCV_M_LEVEL > 0
    __asm__ __volatile__ ("fence.i" : : : "memory");
#else
    sbi_remote_fence_i(0);
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
}
/*********************************************************************************************************
** ��������: riscvCacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvCacheEnable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheDisable
** ��������: ���� CACHE (α����)
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheDisable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheFlush
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheFlushPage
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheFlushPage (LW_CACHE_TYPE  cachetype,
                                 PVOID          pvAdrs,
                                 PVOID          pvPdrs,
                                 size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        riscvLocalICacheFlushAll();
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheInvalidatePage (LW_CACHE_TYPE  cachetype,
                                      PVOID          pvAdrs,
                                      PVOID          pvPdrs,
                                      size_t         stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        riscvLocalICacheFlushAll();
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        riscvLocalICacheFlushAll();
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  riscvCacheClearPage (LW_CACHE_TYPE  cachetype,
                                 PVOID          pvAdrs,
                                 PVOID          pvPdrs,
                                 size_t         stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        riscvLocalICacheFlushAll();
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	riscvCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: riscvCacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	riscvCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: riscvCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	riscvCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    riscvLocalICacheFlushAll();
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  riscvCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvCacheInit
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  riscvCacheInit (LW_CACHE_OP  *pcacheop,
                      CACHE_MODE    uiInstruction,
                      CACHE_MODE    uiData,
                      CPCHAR        pcMachineName)
{
#if LW_CFG_RISCV_M_LEVEL > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0;
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;                      /*  VIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;
    
    pcacheop->CACHEOP_iICacheLine = LW_CFG_CPU_ARCH_CACHE_LINE;
    pcacheop->CACHEOP_iDCacheLine = LW_CFG_CPU_ARCH_CACHE_LINE;

    pcacheop->CACHEOP_iICacheWaySize = LW_CFG_VMM_PAGE_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize = LW_CFG_VMM_PAGE_SIZE;

    pcacheop->CACHEOP_pfuncEnable         = riscvCacheEnable;
    pcacheop->CACHEOP_pfuncDisable        = riscvCacheDisable;
    
    pcacheop->CACHEOP_pfuncFlush          = riscvCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = riscvCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = riscvCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = riscvCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = riscvCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = riscvCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = riscvCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = riscvCacheDataUpdate;
    
    pcacheop->CACHEOP_pfuncLock           = riscvCacheLock;             /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock         = riscvCacheUnlock;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;

#elif LW_CFG_RISCV_MPU_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = riscvMpuDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = riscvMpuDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = riscvMpuDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: riscvCacheReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  riscvCacheReset (CPCHAR  pcMachineName)
{
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
