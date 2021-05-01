/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: vmmMalloc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 05 �� 16 ��
**
** ��        ��: ƽ̨�޹������ڴ��������չ���.
**
** ע        ��: ������ȱҳ�ж�ʱ, ���е����I/O����(��������mmap���)������ vmm lock �½��е�!
                 SylixOS ����ȱҳ�쳣����������������, ͨ������������, ����, ��Ӱ���޹�����ĵ���������.
                 ����Ҳ�������������. ������������ͬһ��ַ�����쳣�ȵ�, ��������õ�һЩ�������������
                 �������.

** BUG:
2011.05.19  �������ڴ����ʧЧʱ, ��������ڴ����, ���ж϶�дȨ��, ���дȨ��������ֱ���˳�.
            (�п���Ϊ���߳�ͬʱ����ͬһ���ڴ�, һ���߳��Ѿ�����������ڴ�, ��һ����ֱ���˳�����).
            ����дȨ�޴���, ��ֱ�ӷǷ��ڴ���ʴ���.
2011.07.28  ���� API_VmmRemapArea() ����, ������ linux remap_pfn_range() ����, �����������ʹ�ô˺���ʵ��
            mmap �ӿ�.
2011.11.09  ���� API_VmmPCountInArea() �ɲ鿴����ռ��ȱҳ�жϷ������.
2011.11.18  ����� dtrace ��֧��.
2011.12.08  �ж��з����쳣, ֱ��������������ϵͳ.
            �쳣��������ڲ���ʱ�˵�ʱ�䷢��, ��ʱ��Ҫ��ӡ��������Ϣ.
2011.12.10  ��ȡ��ӳ���ϵ���߸ı�ӳ���ϵ����ʱ, ��Ҫ��д����Ч cache.
2012.04.26  �����������쳣ʱ, ��ӡ�Ĵ������.
2012.05.07  ʹ�� OSStkSetFp() ������ͬ�� fp ָ��, ��֤ unwind ��ջ����ȷ��.
2012.09.05  ���쳣������ش������ vmmAbort.c �ļ���.
2013.09.13  API_VmmShareArea() ������ڵĹ���ҳ�淢���ı���߲��Ҿ��� MAP_PRIVATE ѡ��, ��Ӧ����������.
2013.09.14  ���� API_VmmSetFindShare() ����.
2013.12.20  API_VmmShareArea() Ŀ��ҳ�治����ʱ, �������µ�����ҳ��.
2015.05.26  ���� API_VmmMergeArea() �ϲ�����ռ书��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "vmmSwap.h"
#include "phyPage.h"
#include "virPage.h"
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
/*********************************************************************************************************
** ��������: __vmmPCountInAreaHook
** ��������: API_VmmCounterArea �ص�����
** �䡡��  : pvmpagePhysical   pvmpageVirtual �ڵ�һ������ҳ����ƿ�
**           pulCounter        ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmPCountInAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                    ULONG        *pulCounter)
{
    PLW_VMM_PAGE  pvmpageReal;
    
    if (LW_VMM_PAGE_IS_FAKE(pvmpagePhysical)) {
        pvmpageReal  = LW_VMM_PAGE_GET_REAL(pvmpagePhysical);
        if (pvmpageReal->PAGE_ulRef == 1) {
            *pulCounter += pvmpagePhysical->PAGE_ulCount;
        }
    } else {
        *pulCounter += pvmpagePhysical->PAGE_ulCount;
    }
}
/*********************************************************************************************************
** ��������: __vmmInvalidateAreaHook
** ��������: API_VmmInvalidateArea �ص�����
** �䡡��  : pvmpagePhysical   pvmpageVirtual �ڵ�һ������ҳ����ƿ�
**           pvmpageVirtual    ����ҳ����ƿ�
**           ulSubMem          �ڲ���ʼ��ַ
**           stSize            ��С
**           pulCount          �ͷŵĸ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmInvalidateAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                      PLW_VMM_PAGE  pvmpageVirtual, 
                                      addr_t        ulSubMem,
                                      size_t        stSize,
                                      ULONG        *pulCount)
{
    if ((pvmpagePhysical->PAGE_ulMapPageAddr >= ulSubMem) &&
        (pvmpagePhysical->PAGE_ulMapPageAddr < (ulSubMem + stSize))) {  /*  �ж϶�Ӧ�������ڴ淶Χ      */
        
#if LW_CFG_CACHE_EN > 0
        __vmmPhysicalPageClear(pvmpagePhysical);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        __vmmLibPageMap(pvmpagePhysical->PAGE_ulMapPageAddr,
                        pvmpagePhysical->PAGE_ulMapPageAddr,
                        pvmpagePhysical->PAGE_ulCount,                  /*  һ���� 1                    */
                        LW_VMM_FLAG_FAIL);                              /*  ��Ӧ�������ڴ治�������    */
                        
        __pageUnlink(pvmpageVirtual, pvmpagePhysical);                  /*  ������ӹ�ϵ                */
                       
        __vmmPhysicalPageFree(pvmpagePhysical);                         /*  �ͷ�����ҳ��                */
        
        if (pulCount) {
            (*pulCount)++;
        }
    }
}
/*********************************************************************************************************
** ��������: __vmmSplitAreaHook
** ��������: API_VmmSplitArea �ص�����
** �䡡��  : pvmpagePhysical       pvmpageVirtual �ڵ�һ������ҳ����ƿ�
**           pvmpageVirtual        ԭʼ�������
**           pvmpageVirtualSplit   �²�ֳ����������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺������� mmap �ռ����, ���뱣֤����ҳ�涼��ȱҳ�����ĵ�ҳ���ƿ�.
*********************************************************************************************************/
static VOID  __vmmSplitAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                 PLW_VMM_PAGE  pvmpageVirtual,
                                 PLW_VMM_PAGE  pvmpageVirtualSplit)
{
    REGISTER addr_t     ulSubMem = pvmpageVirtualSplit->PAGE_ulPageAddr;

    if (pvmpagePhysical->PAGE_ulMapPageAddr >= ulSubMem) {              /*  �ж϶�Ӧ�������ڴ淶Χ      */
        __pageUnlink(pvmpageVirtual, pvmpagePhysical);                  /*  ������ӹ�ϵ                */
        __pageLink(pvmpageVirtualSplit, pvmpagePhysical);
    }
}
/*********************************************************************************************************
** ��������: __vmmMergeAreaHook
** ��������: API_VmmMergeArea �ص�����
** �䡡��  : pvmpagePhysical       pvmpageVirtualR �ڵ�һ������ҳ����ƿ�
**           pvmpageVirtualL       ��������͵�ַ��
**           pvmpageVirtualR       ��������ߵ�ַ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMergeAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                 PLW_VMM_PAGE  pvmpageVirtualL,
                                 PLW_VMM_PAGE  pvmpageVirtualR)
{
    __pageUnlink(pvmpageVirtualR, pvmpagePhysical);                     /*  ������ӹ�ϵ                */
    __pageLink(pvmpageVirtualL, pvmpagePhysical);
}
/*********************************************************************************************************
** ��������: __vmmMoveAreaHook
** ��������: API_VmmMoveArea �ص�����
** �䡡��  : pvmpagePhysical       pvmpageVirtualFrom �ڵ�һ������ҳ����ƿ�
**           pvmpageVirtualL       Ŀ���������
**           pvmpageVirtualR       Դ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMoveAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                PLW_VMM_PAGE  pvmpageVirtualTo,
                                PLW_VMM_PAGE  pvmpageVirtualFrom)
{
    addr_t  ulNewMap;
    ULONG   ulOff;
    
    __pageUnlink(pvmpageVirtualFrom, pvmpagePhysical);                  /*  ������ӹ�ϵ                */
    
    ulOff = pvmpagePhysical->PAGE_ulMapPageAddr
          - pvmpageVirtualFrom->PAGE_ulPageAddr;                        /*  ����ҳ��ƫ����              */
          
    ulNewMap = pvmpageVirtualTo->PAGE_ulPageAddr
             + ulOff;                                                   /*  �µ�ӳ���ַ                */
             
    __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr, 
                    ulNewMap, 1, pvmpageVirtualTo->PAGE_ulFlags);       /*  �����µ�ӳ���ϵ            */
                    
    pvmpagePhysical->PAGE_ulMapPageAddr = ulNewMap;                     /*  ��¼�µ�ӳ���ϵ            */
                    
    __pageLink(pvmpageVirtualTo, pvmpagePhysical);                      /*  ���ӵ�Ŀ���������          */
}
/*********************************************************************************************************
** ��������: API_VmmMalloc
** ��������: �����߼�������, ������ܲ��������ڴ�.
** �䡡��  : stSize     ��Ҫ������ڴ��С
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMalloc (size_t  stSize)
{
    return  (API_VmmMallocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, LW_VMM_FLAG_RDWR));
}
/*********************************************************************************************************
** ��������: API_VmmMallocEx
** ��������: �����߼�������, ������ܲ��������ڴ�.
** �䡡��  : stSize     ��Ҫ������ڴ��С
**           ulFlag     ��������. (����Ϊ LW_VMM_FLAG_TEXT �� LW_VMM_FLAG_DATA)
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMallocEx (size_t  stSize, ULONG  ulFlag)
{
    return  (API_VmmMallocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, ulFlag));
}
/*********************************************************************************************************
** ��������: API_VmmMallocAlign
** ��������: �����߼�������, ������ܲ��������ڴ�. (�����ַ��������ϵ)
** �䡡��  : stSize     ��Ҫ������ڴ��С
**           stAlign    �����ϵ
**           ulFlag     ��������. (����Ϊ LW_VMM_FLAG_TEXT �� LW_VMM_FLAG_DATA)
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMallocAlign (size_t  stSize, size_t  stAlign, ULONG  ulFlag)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
    REGISTER PLW_VMM_PAGE   pvmpagePhysical;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
             ULONG          ulZoneIndex;
             ULONG          ulPageNumTotal = 0;
             ULONG          ulVirtualAddr;
             ULONG          ulError;
    
    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
#if LW_CFG_CACHE_EN > 0
    if (API_CacheAliasProb() && 
        (stAlign < API_CacheWaySize(DATA_CACHE))) {                     /*  �������� cache alias        */
        stAlign = API_CacheWaySize(DATA_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum, stAlign);/*  ������������ҳ��            */
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum);              /*  ������������ҳ��            */
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr;                    /*  ��ʼ�����ڴ��ַ            */

    if (ulFlag & LW_VMM_FLAG_PHY_CONTINUOUS) {                          /*  ����Ϊ���������ڴ�          */
        pvmpagePhysical = __vmmPhysicalPageAlloc(ulPageNum, LW_ZONE_ATTR_NONE, &ulZoneIndex);
        if (pvmpagePhysical == LW_NULL) {
            _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
            goto    __error_handle;
        }

        ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,     /*  ʹ�� CACHE                  */
                                  ulVirtualAddr,
                                  ulPageNum,
                                  ulFlag);                              /*  ӳ��Ϊ���������ַ          */
        if (ulError) {                                                  /*  ӳ�����                    */
            __vmmPhysicalPageFree(pvmpagePhysical);
            _ErrorHandle(ulError);
            goto    __error_handle;
        }

        pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualAddr;
        pvmpagePhysical->PAGE_ulFlags = ulFlag;

        __pageLink(pvmpageVirtual, pvmpagePhysical);                    /*  ������ҳ������������ռ�    */

    } else {                                                            /*  ���������ڴ�ʹ��            */
        do {
            ULONG   ulPageNumOnce = ulPageNum - ulPageNumTotal;
            ULONG   ulMinContinue = __vmmPhysicalPageGetMinContinue(&ulZoneIndex, LW_ZONE_ATTR_NONE);
                                                                        /*  ���ȷ�����Ƭҳ��            */
        
            if (ulPageNumOnce > ulMinContinue) {                        /*  ѡ���ʵ���ҳ�泤��          */
                ulPageNumOnce = ulMinContinue;
            }
        
            pvmpagePhysical = __vmmPhysicalPageAllocZone(ulZoneIndex, ulPageNumOnce, LW_ZONE_ATTR_NONE);
            if (pvmpagePhysical == LW_NULL) {
                _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
                goto    __error_handle;
            }

            ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr, /*  ʹ�� CACHE                  */
                                      ulVirtualAddr,
                                      ulPageNumOnce,
                                      ulFlag);                          /*  ӳ��Ϊ���������ַ          */
            if (ulError) {                                              /*  ӳ�����                    */
                __vmmPhysicalPageFree(pvmpagePhysical);
                _ErrorHandle(ulError);
                goto    __error_handle;
            }

            pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualAddr;
            pvmpagePhysical->PAGE_ulFlags = ulFlag;

            __pageLink(pvmpageVirtual, pvmpagePhysical);                /*  ������ҳ������������ռ�    */

            ulPageNumTotal += ulPageNumOnce;
            ulVirtualAddr  += (ulPageNumOnce << LW_CFG_VMM_PAGE_SHIFT);

        } while (ulPageNumTotal < ulPageNum);
    }
    
    pvmpageVirtual->PAGE_ulFlags = ulFlag;
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, ulFlag, LW_NULL);
    
    return  ((PVOID)pvmpageVirtual->PAGE_ulPageAddr);                   /*  ���������ַ                */
        
__error_handle:                                                         /*  ���ִ���                    */
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  �ͷ�ҳ������                */
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  �ͷ������ַ�ռ�            */
    __VMM_UNLOCK();
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmFree
** ��������: �ͷ����������ڴ�
** �䡡��  : pvVirtualMem    ���������ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmFree (PVOID  pvVirtualMem)
{
    API_VmmFreeArea(pvVirtualMem);
}
/*********************************************************************************************************
** ��������: API_VmmMallocArea
** ��������: ����������ռ�, �����ֵ�һ�η���ʱ, ��ͨ��ȱҳ�жϷ��������ڴ�, 
             ��ȱҳ�ж����޷���������ҳ��ʱ, ���յ� SIGSEGV �źŲ������߳�. 
** �䡡��  : stSize        ��Ҫ������ڴ��С
**           pfuncFiller   ������ȱҳʱ, ��ȡ�����ҳ�����亯��, һ��Ϊ NULL.
**           pvArg         ��亯������.
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocArea (size_t  stSize, FUNCPTR  pfuncFiller, PVOID  pvArg)
{
    return  (API_VmmMallocAreaAlign(stSize, LW_CFG_VMM_PAGE_SIZE, 
                                    pfuncFiller, pvArg, LW_VMM_PRIVATE_CHANGE, LW_VMM_FLAG_RDWR));
}
/*********************************************************************************************************
** ��������: API_VmmMallocAreaEx
** ��������: ����������ռ�, �����ֵ�һ�η���ʱ, ��ͨ��ȱҳ�жϷ��������ڴ�, 
             ��ȱҳ�ж����޷���������ҳ��ʱ, ���յ� SIGSEGV �źŲ������߳�. 
** �䡡��  : stSize        ��Ҫ������ڴ��С
**           pfuncFiller   ������ȱҳʱ, ��ȡ�����ҳ�����亯��, һ��Ϊ NULL.
**           pvArg         ��亯������.
**           iFlag         MAP_SHARED or MAP_PRIVATE
**           ulFlag        ��������. (����Ϊ LW_VMM_FLAG_TEXT �� LW_VMM_FLAG_DATA)
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocAreaEx (size_t  stSize, FUNCPTR  pfuncFiller, PVOID  pvArg, INT  iFlags, ULONG  ulFlag)
{
    return  (API_VmmMallocAreaAlign(stSize, LW_CFG_VMM_PAGE_SIZE, pfuncFiller, pvArg, iFlags, ulFlag));
}
/*********************************************************************************************************
** ��������: API_VmmMallocAreaAlign
** ��������: ����������ռ�, �����ֵ�һ�η���ʱ, ��ͨ��ȱҳ�жϷ��������ڴ�, 
             ��ȱҳ�ж����޷���������ҳ��ʱ, ���յ� SIGSEGV �źŲ������߳�. 
** �䡡��  : stSize        ��Ҫ������ڴ��С
**           stAlign       �����ϵ
**           pfuncFiller   ������ȱҳʱ, ��ȡ�����ҳ�����亯��, һ��Ϊ NULL.
**           pvArg         ��亯������.
**           iFlag         MAP_SHARED or MAP_PRIVATE
**           ulFlag        ��������. (����Ϊ LW_VMM_FLAG_TEXT �� LW_VMM_FLAG_DATA)
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocAreaAlign (size_t  stSize, size_t  stAlign, 
                               FUNCPTR  pfuncFiller, PVOID  pvArg, INT  iFlags, ULONG  ulFlag)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             
             ULONG          ulError;
    
    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
#if LW_CFG_CACHE_EN > 0
    if (API_CacheAliasProb() && 
        (stAlign < API_CacheWaySize(DATA_CACHE))) {                     /*  �������� cache alias        */
        stAlign = API_CacheWaySize(DATA_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum, stAlign);/*  ������������ҳ��            */
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum);              /*  ������������ҳ��            */
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_PRIVATE));
    if (pvmpagep == LW_NULL) {
        __vmmVirtualPageFree(pvmpageVirtual);
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                              pvmpageVirtual->PAGE_ulPageAddr,
                              ulPageNum, 
                              LW_VMM_FLAG_FAIL);                        /*  �˶��ڴ�ռ����ʧЧ        */
    if (ulError) {                                                      /*  ӳ�����                    */
        __KHEAP_FREE(pvmpagep);
        __vmmVirtualPageFree(pvmpageVirtual);                           /*  �ͷ������ַ�ռ�            */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    pvmpagep->PAGEP_ulPtStart = 0;
    pvmpagep->PAGEP_stPtSize  = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
    
    pvmpagep->PAGEP_pfuncFindShare = LW_NULL;
    pvmpagep->PAGEP_pvFindArg      = LW_NULL;
    
    pvmpagep->PAGEP_iFlags = iFlags;
    
    pvmpagep->PAGEP_pvmpageVirtual = pvmpageVirtual;                    /*  �������ӹ�ϵ                */
    pvmpageVirtual->PAGE_pvAreaCb  = (PVOID)pvmpagep;                   /*  ��¼˽�����ݽṹ            */
    
    _List_Line_Add_Ahead(&pvmpagep->PAGEP_lineManage, 
                         &_K_plineVmmVAddrSpaceHeader);                 /*  ����ȱҳ�жϲ��ұ�          */
    
    pvmpageVirtual->PAGE_ulFlags = ulFlag;                              /*  ��¼�ڴ�����                */
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC_A,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, iFlags, ulFlag, LW_NULL);
    
    return  ((PVOID)pvmpageVirtual->PAGE_ulPageAddr);
}
/*********************************************************************************************************
** ��������: API_VmmFreeArea
** ��������: �ͷ����������ڴ�
** �䡡��  : pvVirtualMem    ���������ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmFreeArea (PVOID  pvVirtualMem)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return;
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (pvmpagep) {
        _List_Line_Del(&pvmpagep->PAGEP_lineManage, &_K_plineVmmVAddrSpaceHeader);
        pvmpageVirtual->PAGE_pvAreaCb = LW_NULL;
        __KHEAP_FREE(pvmpagep);                                         /*  �ͷ�ȱҳ�жϹ���ģ��        */
    }
    
#if LW_CFG_CACHE_EN > 0
    __vmmPhysicalPageClearAll(pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  ���������                  */
    
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  �ͷ�����ҳ��                */
    
    __areaVirtualUnlinkPage(pvmpageVirtual->PAGE_ulPageAddr,
                            pvmpageVirtual);
    
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  ɾ������ҳ��                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_FREE,
                      pvVirtualMem, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmExpandArea
** ��������: ��չ���������ڴ�
** �䡡��  : pvVirtualMem    ���������ַ
**           stExpSize       ��Ҫ����Ĵ�С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmExpandArea (PVOID  pvVirtualMem, size_t  stExpSize)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulAddr = (addr_t)pvVirtualMem;
             ULONG          ulError;
             
    REGISTER ULONG          ulExpPageNum = (ULONG) (stExpSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess     = (size_t)(stExpSize & ~LW_CFG_VMM_PAGE_MASK);
    
    if (stExcess) {
        ulExpPageNum++;                                                 /*  ȷ����ҳ����                */
    }
    
    if (ulExpPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    ulError = __pageExpand(pvmpageVirtual, ulExpPageNum);               /*  ��չ����ҳ��                */
    if (ulError) {
        __VMM_UNLOCK();
        return  (ulError);
    }
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmSplitArea
** ��������: ������������ڴ�
** �䡡��  : pvVirtualMem    ���������ַ
**           stSize          pvVirtualMem �������Ҫ�����Ĵ�С.
** �䡡��  : �µ����������ַ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺������� mmap �ռ����, ���뱣֤����ҳ�涼��ȱҳ�����ĵ�ҳ���ƿ�.
**
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmSplitArea (PVOID  pvVirtualMem, size_t  stSize)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
             PLW_VMM_PAGE           pvmpageVirtualSplit;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagepSplit;
    
             addr_t         ulAddr = (addr_t)pvVirtualMem;
             ULONG          ulError;
             
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
    
    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }
    
    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pvmpagepSplit = (PLW_VMM_PAGE_PRIVATE)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_PRIVATE));
    if (pvmpagepSplit == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (LW_NULL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (pvmpagep == LW_NULL) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (LW_NULL);
    }
    
    ulError = __pageSplit(pvmpageVirtual, &pvmpageVirtualSplit, ulPageNum, pvmpagepSplit);
    if (ulError) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        return  (LW_NULL);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmSplitAreaHook, 
                        pvmpageVirtual,
                        pvmpageVirtualSplit,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  ������Ӧ������ҳ��          */
                        
    *pvmpagepSplit = *pvmpagep;
    pvmpagepSplit->PAGEP_pvmpageVirtual = pvmpageVirtualSplit;          /*  ��ָ���µ������ַ���ƿ�    */
    
    _List_Line_Add_Ahead(&pvmpagepSplit->PAGEP_lineManage, 
                         &_K_plineVmmVAddrSpaceHeader);                 /*  ����ȱҳ�жϲ��ұ�          */
                         
    __areaVirtualInsertPage(pvmpageVirtualSplit->PAGE_ulPageAddr, 
                            pvmpageVirtualSplit);                       /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();

    return  ((PVOID)pvmpageVirtualSplit->PAGE_ulPageAddr);
}
/*********************************************************************************************************
** ��������: API_VmmMergeArea
** ��������: �ϲ�����ռ�
** �䡡��  : pvVirtualMem1   �����ַ���� 1
**           pvVirtualMem2   �����ַ���� 2
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmMergeArea (PVOID  pvVirtualMem1, PVOID  pvVirtualMem2)
{
    PLW_VMM_PAGE    pvmpageVirtualL;
    PLW_VMM_PAGE    pvmpageVirtualR;
    
    PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    addr_t          ulAddrL;
    addr_t          ulAddrR;
    
    if (pvVirtualMem1 < pvVirtualMem2) {
        ulAddrL = (addr_t)pvVirtualMem1;
        ulAddrR = (addr_t)pvVirtualMem2;
    
    } else {
        ulAddrL = (addr_t)pvVirtualMem2;
        ulAddrR = (addr_t)pvVirtualMem1;
    }
    
    __VMM_LOCK();
    pvmpageVirtualL = __areaVirtualSearchPage(ulAddrL);
    if (pvmpageVirtualL == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtualR = __areaVirtualSearchPage(ulAddrR);
    if (pvmpageVirtualR == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((pvmpageVirtualL->PAGE_ulPageAddr + pvmpageVirtualL->PAGE_ulCount) != 
         pvmpageVirtualR->PAGE_ulPageAddr) {                            /*  ������ҳ�治�ܽ��кϲ�      */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_ADDR);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtualR->PAGE_pvAreaCb;
    if (pvmpagep == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    __pageTraversalLink(pvmpageVirtualR, 
                        __vmmMergeAreaHook, 
                        pvmpageVirtualL,
                        pvmpageVirtualR,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  ������Ӧ������ҳ��          */

    __areaVirtualUnlinkPage(pvmpageVirtualR->PAGE_ulPageAddr,
                            pvmpageVirtualR);
                            
    _List_Line_Del(&pvmpagep->PAGEP_lineManage, &_K_plineVmmVAddrSpaceHeader);
    __KHEAP_FREE(pvmpagep);                                             /*  �ͷ�ȱҳ�жϹ���ģ��        */
    
    __pageMerge(pvmpageVirtualL, pvmpageVirtualR);                      /*  �ϲ�ҳ��                    */
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmMoveArea
** ��������: ��һ������ҳ��������ڴ��ƶ�����һ�������ڴ�
** �䡡��  : pvVirtualTo     Ŀ�������ַ����
**           pvVirtualFrom   Դ�����ַ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmMoveArea (PVOID  pvVirtualTo, PVOID  pvVirtualFrom)
{
    PLW_VMM_PAGE    pvmpageVirtualTo;
    PLW_VMM_PAGE    pvmpageVirtualFrom;
    
    addr_t          ulAddrTo   = (addr_t)pvVirtualTo;
    addr_t          ulAddrFrom = (addr_t)pvVirtualFrom;
    
    __VMM_LOCK();
    pvmpageVirtualTo = __areaVirtualSearchPage(ulAddrTo);
    if (pvmpageVirtualTo == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtualFrom = __areaVirtualSearchPage(ulAddrFrom);
    if (pvmpageVirtualFrom == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if (pvmpageVirtualTo->PAGE_ulCount < 
        pvmpageVirtualFrom->PAGE_ulCount) {                             /*  Ŀ����������̫С            */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_LOW_PAGE);
        return  (ERROR_VMM_LOW_PAGE);
    }
    
#if LW_CFG_CACHE_EN > 0
    __vmmPhysicalPageClearAll(pvmpageVirtualFrom);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtualFrom->PAGE_ulPageAddr,
                    pvmpageVirtualFrom->PAGE_ulPageAddr,
                    pvmpageVirtualFrom->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  ���������                  */
    
    __pageTraversalLink(pvmpageVirtualFrom, 
                        __vmmMoveAreaHook, 
                        pvmpageVirtualTo,
                        pvmpageVirtualFrom,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  ������Ӧ������ҳ��          */
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmSetFiller
** ��������: ��������ռ����亯��
**           �˺������� loader �ṩ����, ���ڴ���ι���.
** �䡡��  : pvVirtualMem    ���������ַ  (����Ϊ vmmMallocArea ���ص�ַ)
**           pfuncFiller     ������ȱҳʱ, ��ȡ�����ҳ�����亯��, һ��Ϊ NULL.
**           pvArg           ��亯������.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_VmmSetFiller (PVOID  pvVirtualMem, FUNCPTR  pfuncFiller, PVOID  pvArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
    
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmSetFindShare
** ��������: ��������ռ��ѯ����ҳ�溯��.
** �䡡��  : pvVirtualMem    ���������ַ  (����Ϊ vmmMallocArea ���ص�ַ)
**           pfuncFindShare  ������ȱҳʱ, ���ҳ������Ϊ����, ���ѯ�Ƿ��п��Թ����ҳ��
**           pvArg           ��������.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_VmmSetFindShare (PVOID  pvVirtualMem, PVOIDFUNCPTR  pfuncFindShare, PVOID  pvArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFindShare = pfuncFindShare;
    pvmpagep->PAGEP_pvFindArg      = pvArg;
    
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmSetProtect
** ��������: ��������ռ��ܱ�������.
** �䡡��  : pvVirtualMem    ���������ַ  (����Ϊ vmmMallocArea ���ص�ַ)
**           pvSubMem        ������ȱҳʱ, ������ copy-on-write �ı�������ʼ��ַ
**           stSize          ��������С.
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0

LW_API
ULONG  API_VmmSetProtect (PVOID  pvVirtualMem, PVOID  pvSubMem, size_t  stSize)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;

    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }

    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }

    pvmpagep->PAGEP_ulPtStart = (addr_t)pvSubMem;
    pvmpagep->PAGEP_stPtSize  = stSize;

    __VMM_UNLOCK();

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */
/*********************************************************************************************************
** ��������: API_VmmPCountInArea
** ��������: API_VmmMallocAreaEx ��������������ڴ��а���������ҳ����� (�������ڴ�Ϊȱҳ�жϷ���)
** �䡡��  : pvVirtualMem    ���������ַ (����Ϊ vmmMallocArea?? ���ص�ַ)
**           pulPageNum      ��������ҳ��ĸ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmPCountInArea (PVOID  pvVirtualMem, ULONG  *pulPageNum)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
             addr_t                 ulAddr    = (addr_t)pvVirtualMem;
             ULONG                  ulCounter = 0ul;
             
    if (pulPageNum == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmPCountInAreaHook, 
                        (PVOID)&ulCounter,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  ������Ӧ������ҳ��          */
    __VMM_UNLOCK();
    
    *pulPageNum = ulCounter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmInvalidateArea
** ��������: �ͷ� vmmMallocArea ������������ڴ�, �����������ڴ�ռ�. 
** �䡡��  : pvVirtualMem    ���������ַ  (����Ϊ vmmMalloc?? ���ص�ַ)
**           pvSubMem        ��Ҫ invalidate �ĵ�ַ
**           stSize          ��Ҫ invalidate �Ĵ�С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_VmmInvalidateArea (PVOID  pvVirtualMem, PVOID  pvSubMem, size_t  stSize)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmInvalidateAreaHook, 
                        (PVOID)pvmpageVirtual,
                        pvSubMem,
                        (PVOID)stSize,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  ������Ӧ������ҳ��          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_INVAL_A,
                      pvSubMem, stSize, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmPreallocArea
** ��������: �� vmmMallocArea ������������ڴ�ռ��������ҳ��
**           �˺������� loader �ṩ����, ���װ���ٶ�.
** �䡡��  : pvVirtualMem    ���������ַ  (����Ϊ vmmMalloc?? ���ص�ַ)
**           pvSubMem        ��Ҫ���ĵ�ַ
**           stSize          ��Ҫ���Ĵ�С
**           ulFlag          �������
**           pfuncTempFiller ��ʱ��亯��, �ͱ�׼�� filler ��ͬ, ���������ʹ����ʱ��亯��, ���������,
                             ��ʹ�� PLW_VMM_PAGE_PRIVATE ���õ���亯��.
**           pvTempArg       ��ʱ��亯������.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_VmmPreallocArea (PVOID       pvVirtualMem, 
                            PVOID       pvSubMem, 
                            size_t      stSize, 
                            FUNCPTR     pfuncTempFiller, 
                            PVOID       pvTempArg,
                            ULONG       ulFlag)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             ULONG                  ulZoneIndex;
    
             ULONG                  i;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
             addr_t                 ulAddrPage;
             ULONG                  ulPageNum;
             size_t                 stRealSize;
             ULONG                  ulOldFlag;
             ULONG                  ulError;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((pvSubMem < pvVirtualMem) ||
        (((addr_t)pvSubMem + stSize) > 
        ((addr_t)pvVirtualMem + (pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT)))) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_ADDR);                           /*  ��ַ������Χ                */
        return  (ERROR_VMM_VIRTUAL_ADDR);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    ulAddrPage = (addr_t)pvSubMem & LW_CFG_VMM_PAGE_MASK;               /*  ҳ������ַ                */
    stRealSize = stSize + (size_t)((addr_t)pvSubMem - ulAddrPage);
    stRealSize = ROUND_UP(stRealSize, LW_CFG_VMM_PAGE_SIZE);
    ulPageNum  = (ULONG)(stRealSize >> LW_CFG_VMM_PAGE_SHIFT);          /*  ��ҪԤ���������ҳ�����    */
    
    for (i = 0; i < ulPageNum; i++) {
        if (__vmmLibGetFlag(ulAddrPage, &ulOldFlag) == ERROR_NONE) {
            if (ulOldFlag != ulFlag) {
                __vmmLibSetFlag(ulAddrPage, 1, ulFlag, LW_TRUE);        /*  ��������ҳ�������flag����  */
            }
        
        } else {
            pvmpagePhysical = __vmmPhysicalPageAlloc(1, LW_ZONE_ATTR_NONE, &ulZoneIndex);
            if (pvmpagePhysical == LW_NULL) {
                ulError = ERROR_VMM_LOW_PHYSICAL_PAGE;
                printk(KERN_CRIT "kernel no more physical page.\n");    /*  ϵͳ�޷���������ҳ��        */
                goto    __error_handle;
            }
            ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,
                                      ulAddrPage, 1, ulFlag);           /*  ����ֱ��ӳ�䵽��Ҫ�ĵ�ַ    */
            if (ulError) {
                __vmmPhysicalPageFree(pvmpagePhysical);
                printk(KERN_CRIT "kernel physical page map error.\n");  /*  ϵͳ�޷�ӳ������ҳ��        */
                goto    __error_handle;
            }
            
            /*
             *  ע��: ��亯��ֱ����Ŀ���ַ�������, ���Զ��������ʱ������ռ, ���ܷ��ʵ����ݻ�û�о���
             *  ���, ���Դ˺���Ӧ�ý���Ϊ��ʼ���׶εĲ���, �����ķ���, ʹ��ȱҳ�жϷ��伴��.
             */
            if (pfuncTempFiller) {
                pfuncTempFiller(pvTempArg, ulAddrPage, ulAddrPage, 1);
            } else {
                if (pvmpagep->PAGEP_pfuncFiller) {
                    pvmpagep->PAGEP_pfuncFiller(pvmpagep->PAGEP_pvArg, 
                                                ulAddrPage, ulAddrPage, 1);
                }
            }
            
            pvmpagePhysical->PAGE_ulMapPageAddr = ulAddrPage;
            pvmpagePhysical->PAGE_ulFlags       = ulFlag;
            
            __pageLink(pvmpageVirtual, pvmpagePhysical);                /*  �������ӹ�ϵ                */
        }
        ulAddrPage += LW_CFG_VMM_PAGE_SIZE;
    }
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_PREALLOC_A,
                      pvSubMem, stSize, ulFlag, LW_NULL);
    
    return  (ERROR_NONE);
    
__error_handle:
    __VMM_UNLOCK();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmShareArea
** ��������: ������ area �ڴ���ĳһ���ֹ���һ�������ڴ�, 
**           �˺������� loader �ṩ����, ���ڴ���ι���.
** �䡡��  : pvVirtualMem1   ���������ַ  (����Ϊ vmmMallocArea ���ص�ַ)
**           pvVirtualMem2   ���������ַ  (����Ϊ vmmMallocArea ���ص�ַ)
**           stStartOft1     �ڴ���1������ʼƫ����
**           stStartOft2     �ڴ���2������ʼƫ����
**           stSize          �������򳤶�
**           bExecEn         �����Ƿ��ִ��
**           pfuncTempFiller ��ʱ��亯��, �ͱ�׼�� filler ��ͬ, ���������ʹ����ʱ��亯��, ���������,
                             ��ʹ�� PLW_VMM_PAGE_PRIVATE ���õ���亯��.
**           pvTempArg       ��ʱ��亯������.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���, pvVirtualMem2 ָ���������Ѿ���������ҳ��, ���ͷŶ�Ӧ������ҳ��, Ȼ���� fake ����ҳ��
             �� pvVirtualMem1 �����������, ͬʱ ulStartOft1 �� ulStartOft2 ����ҳ�����, ulSize Ҳ����ҳ
             �����, 
             ������� pvVirtualMem1 ���б����ĵ�ҳ��, pvVirtualMem2 ���¿�������ҳ�沢���.
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_VmmShareArea (PVOID      pvVirtualMem1, 
                         PVOID      pvVirtualMem2,
                         size_t     stStartOft1, 
                         size_t     stStartOft2, 
                         size_t     stSize,
                         BOOL       bExecEn,
                         FUNCPTR    pfuncTempFiller, 
                         PVOID      pvTempArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual1;
    REGISTER PLW_VMM_PAGE           pvmpageVirtual2;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical1;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical2;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             ULONG                  ulZoneIndex;
    
             ULONG                  i;
             addr_t                 ulAddr1 = (addr_t)pvVirtualMem1;
             addr_t                 ulAddr2 = (addr_t)pvVirtualMem2;
             ULONG                  ulPageNum;
             ULONG                  ulError;
             ULONG                  ulFlag;
    
    
    if (!ALIGNED(stStartOft1, LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(stStartOft2, LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(stSize,      LW_CFG_VMM_PAGE_SIZE)) {                  /*  ƫ�����ͳ��ȱ���ҳ�����    */
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (ERROR_VMM_ALIGN);
    }
    
    if (bExecEn) {
        ulFlag = LW_VMM_FLAG_EXEC | LW_VMM_FLAG_READ;
    } else {
        ulFlag = LW_VMM_FLAG_READ;
    }
    
    ulPageNum = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    
    __VMM_LOCK();
    pvmpageVirtual1 = __areaVirtualSearchPage(ulAddr1);
    if (pvmpageVirtual1 == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtual2 = __areaVirtualSearchPage(ulAddr2);
    if (pvmpageVirtual2 == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if (!pvmpageVirtual1->PAGE_pvAreaCb) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual2->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((stStartOft1 > (pvmpageVirtual1->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT)) ||
        (stStartOft2 > (pvmpageVirtual2->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT))) {
        __VMM_UNLOCK();
        _ErrorHandle(EINVAL);                                           /*  ƫ����Խ��                  */
        return  (EINVAL);
    }
    
    ulAddr1 += stStartOft1;
    ulAddr2 += stStartOft2;
    for (i = 0; i < ulPageNum; i++) {
        pvmpagePhysical1 = __pageFindLink(pvmpageVirtual1, ulAddr1);    /*  ��ȡ����Ŀ������ҳ��        */
        
        if ((pvmpagePhysical1 == LW_NULL) ||                            /*  ����ҳ�治����              */
            ((pvmpagePhysical1->PAGE_iChange) &&
             (pvmpagep->PAGEP_iFlags & LW_VMM_PRIVATE_CHANGE))) {       /*  ��������ҳ�淢���˸ı�      */
            
            pvmpagePhysical2 = __vmmPhysicalPageAlloc(1, LW_ZONE_ATTR_NONE, &ulZoneIndex);
            if (pvmpagePhysical2 == LW_NULL) {                          /*  ����ҳ�����                */
                ulError = ERROR_KERNEL_LOW_MEMORY;
                goto    __error_handle;
            }
            pvmpagePhysical2->PAGE_ulFlags = LW_VMM_FLAG_RDWR;          /*  ��д                        */
        
        } else {
            pvmpagePhysical2 = __vmmPhysicalPageRef(pvmpagePhysical1);  /*  ҳ������                    */
            if (pvmpagePhysical2 == LW_NULL) {
                ulError = ERROR_KERNEL_LOW_MEMORY;
                goto    __error_handle;
            }
            pvmpagePhysical2->PAGE_ulFlags = ulFlag;
        }
        
        ulError = __vmmLibPageMap(pvmpagePhysical2->PAGE_ulPageAddr,
                                  ulAddr2, 1, 
                                  pvmpagePhysical2->PAGE_ulFlags);      /*  ����ֱ��ӳ�䵽��Ҫ�ĵ�ַ    */
        if (ulError) {
            __vmmPhysicalPageFree(pvmpagePhysical2);
            printk(KERN_CRIT "kernel physical page map error.\n");      /*  ϵͳ�޷�ӳ������ҳ��        */
            goto    __error_handle;
        }
        
        if (!LW_VMM_PAGE_IS_FAKE(pvmpagePhysical2)) {                   /*  ���ǹ��������ڴ�            */
            if (pfuncTempFiller) {
                pfuncTempFiller(pvTempArg, ulAddr2, ulAddr2, 1);        /*  ʹ����ʱ��亯��            */
            } else {
                if (pvmpagep->PAGEP_pfuncFiller) {
                    pvmpagep->PAGEP_pfuncFiller(pvmpagep->PAGEP_pvArg, 
                                                ulAddr2, ulAddr2, 1);   /*  ����ļ�����                */
                }
            }
            pvmpagePhysical2->PAGE_ulFlags = ulFlag;
            
            __vmmLibSetFlag(ulAddr2, 1, ulFlag, LW_TRUE);               /*  ��������ģʽ                */
        }
        
        pvmpagePhysical2->PAGE_ulMapPageAddr = ulAddr2;                 /*  �����Ӧ��ӳ�������ַ      */
        
        __pageLink(pvmpageVirtual2, pvmpagePhysical2);                  /*  �������ӹ�ϵ                */
        
        ulAddr1 += LW_CFG_VMM_PAGE_SIZE;
        ulAddr2 += LW_CFG_VMM_PAGE_SIZE;
    }
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_SHARE_A,
                      pvVirtualMem1, pvVirtualMem2, 
                      stStartOft1, stStartOft2, 
                      stSize, LW_NULL);
    
    return  (ERROR_NONE);
    
__error_handle:
    __VMM_UNLOCK();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmRemapArea
** ��������: �����������ڴ�ӳ�䵽�����������ַ�� (�������� mmap ��ʹ�ô˺�����ӳ��)
** �䡡��  : pvVirtualAddr   ���������ַ (����Ϊ vmmMallocArea?? ���ص�ַ)
**           pvPhysicalAddr  �����ַ
**           stSize          ӳ�䳤��
**           ulFlag          ӳ�� flag ��־
**           pfuncFiller     ȱҳ�ж���亯�� (һ��Ϊ LW_NULL)
**           pvArg           ȱҳ�ж���亯���ײ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmRemapArea2 (PVOID         pvVirtualAddr, 
                          phys_addr_t   paPhysicalAddr, 
                          size_t        stSize, 
                          ULONG         ulFlag, 
                          FUNCPTR       pfuncFiller, 
                          PVOID         pvArg)
{
    REGISTER addr_t ulVirtualAddr = (addr_t)pvVirtualAddr;
    REGISTER ULONG  ulPageNum     = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t stExcess      = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             
    REGISTER ULONG  ulError;
    
    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_VIRTUAL_ADDR);
    }
    
    if (!ALIGNED(pvVirtualAddr,  LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(paPhysicalAddr, LW_CFG_VMM_PAGE_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_ALIGN);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulVirtualAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
                                   
    ulError = __vmmLibPageMap2(paPhysicalAddr, ulVirtualAddr, ulPageNum, ulFlag);
    if (ulError == ERROR_NONE) {
        pvmpageVirtual->PAGE_ulFlags = ulFlag;                          /*  ��¼�ڴ�����                */
    }
    __VMM_UNLOCK();
    
    if (ulError == ERROR_NONE) {
        MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_REMAP_A,
                          pvVirtualAddr, (addr_t)((UINT64)paPhysicalAddr >> 32), (addr_t)(paPhysicalAddr),
                          stSize, ulFlag, LW_NULL);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmRemapArea
** ��������: �����������ڴ�ӳ�䵽�����������ַ�� (�������� mmap ��ʹ�ô˺�����ӳ��)
** �䡡��  : pvVirtualAddr   ���������ַ (����Ϊ vmmMallocArea?? ���ص�ַ)
**           pvPhysicalAddr  �����ַ
**           stSize          ӳ�䳤��
**           ulFlag          ӳ�� flag ��־
**           pfuncFiller     ȱҳ�ж���亯�� (һ��Ϊ LW_NULL)
**           pvArg           ȱҳ�ж���亯���ײ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmRemapArea (PVOID      pvVirtualAddr, 
                         PVOID      pvPhysicalAddr, 
                         size_t     stSize, 
                         ULONG      ulFlag, 
                         FUNCPTR    pfuncFiller, 
                         PVOID      pvArg)
{
    return  (API_VmmRemapArea2(pvVirtualAddr, (phys_addr_t)pvPhysicalAddr, 
                               stSize, ulFlag, pfuncFiller, pvArg));
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
