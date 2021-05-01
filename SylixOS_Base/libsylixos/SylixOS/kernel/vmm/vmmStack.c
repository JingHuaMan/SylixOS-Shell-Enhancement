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
** ��   ��   ��: vmmStack.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 06 �� 01 ��
**
** ��        ��: Ӧ�ó����ջ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
** ��������: API_VmmStackAlloc
** ��������: ���������ջ�ڴ�.
** �䡡��  : stSize     ��Ҫ������ڴ��С
** �䡡��  : �����ڴ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmStackAlloc (size_t  stSize)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
    REGISTER PLW_VMM_PAGE   pvmpagePhysical;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             size_t         stAlign   = LW_CFG_VMM_PAGE_SIZE;
    
             ULONG          ulZoneIndex;
             ULONG          ulPageNumTotal = 0;
             ULONG          ulVirtualAddr;
             ULONG          ulError;
    
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
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {                               /*  �����һ������ҳ����Ϊ����  */
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum + 1, stAlign);
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum + 1);
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
#if CPU_STK_GROWTH == 0
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr;                    /*  ��ʼ�����ڴ��ַ            */
#else
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr + LW_CFG_VMM_PAGE_SIZE;
#endif

    do {
        ULONG   ulPageNumOnce = ulPageNum - ulPageNumTotal;
        ULONG   ulMinContinue = __vmmPhysicalPageGetMinContinue(&ulZoneIndex, LW_ZONE_ATTR_NONE);
                                                                        /*  ���ȷ�����Ƭҳ��            */
    
        if (ulPageNumOnce > ulMinContinue) {                            /*  ѡ���ʵ���ҳ�泤��          */
            ulPageNumOnce = ulMinContinue;
        }
    
        pvmpagePhysical = __vmmPhysicalPageAllocZone(ulZoneIndex, ulPageNumOnce, LW_ZONE_ATTR_NONE);
        if (pvmpagePhysical == LW_NULL) {
            _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
            goto    __error_handle;
        }
        
        ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,     /*  ʹ�� CACHE                  */
                                  ulVirtualAddr,
                                  ulPageNumOnce, 
                                  LW_VMM_FLAG_RDWR);                    /*  ӳ��Ϊ���������ַ          */
        if (ulError) {                                                  /*  ӳ�����                    */
            __vmmPhysicalPageFree(pvmpagePhysical);
            _ErrorHandle(ulError);
            goto    __error_handle;
        }
        
        pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualAddr;
        pvmpagePhysical->PAGE_ulFlags = LW_VMM_FLAG_RDWR;
        
        __pageLink(pvmpageVirtual, pvmpagePhysical);                    /*  ������ҳ������������ռ�    */
        
        ulPageNumTotal += ulPageNumOnce;
        ulVirtualAddr  += (ulPageNumOnce << LW_CFG_VMM_PAGE_SHIFT);
        
    } while (ulPageNumTotal < ulPageNum);
    
    pvmpageVirtual->PAGE_ulFlags = LW_VMM_FLAG_RDWR;
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, LW_VMM_FLAG_RDWR, LW_NULL);
    
#if CPU_STK_GROWTH == 0
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr));                 /*  ���������ַ                */
#else
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr + LW_CFG_VMM_PAGE_SIZE));
#endif

__error_handle:                                                         /*  ���ִ���                    */
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  �ͷ�ҳ������                */
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  �ͷ������ַ�ռ�            */
    __VMM_UNLOCK();
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmStackFree
** ��������: �ͷ������ջ�ڴ�
** �䡡��  : pvVirtualMem    ���������ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmStackFree (PVOID  pvVirtualMem)
{
#if CPU_STK_GROWTH == 0
    API_VmmFreeArea(pvVirtualMem);
#else
    API_VmmFreeArea((PVOID)((addr_t)pvVirtualMem - LW_CFG_VMM_PAGE_SIZE));
#endif
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
