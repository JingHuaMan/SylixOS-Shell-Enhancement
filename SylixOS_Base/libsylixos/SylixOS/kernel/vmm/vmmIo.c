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
** ��   ��   ��: vmmIo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 21 ��
**
** ��        ��: ƽ̨�޹������ڴ����, �豸�ڴ�ӳ��.

** BUG
2018.04.06  ���� API_VmmIoRemapEx() ��ԷǶ��������ڴ�ӳ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
** ��������: API_VmmIoRemapEx2
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�û���ָ�� CACHE ���)
** �䡡��  : paPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlags            �ڴ�����
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapEx2 (phys_addr_t  paPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    REGISTER ULONG          ulPageNum;
    REGISTER phys_addr_t    paPhyPageAddr;
    REGISTER phys_addr_t    paUpPad;
    
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             ULONG          ulError;
    
    paUpPad        = paPhysicalAddr & (LW_CFG_VMM_PAGE_SIZE - 1);
    paPhyPageAddr  = paPhysicalAddr - paUpPad;
    stSize        += (size_t)paUpPad;
    
    ulPageNum = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    if (stSize & ~LW_CFG_VMM_PAGE_MASK) {
        ulPageNum++;
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __vmmVirDevPageAlloc(ulPageNum);                   /*  ������������ҳ��            */
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap2(paPhyPageAddr,
                               pvmpageVirtual->PAGE_ulPageAddr,
                               ulPageNum, 
                               ulFlags);                                /*  ӳ��Ϊ���������ַ          */
    if (ulError) {                                                      /*  ӳ�����                    */
        __vmmVirDevPageFree(pvmpageVirtual);                            /*  �ͷ������ַ�ռ�            */
        __VMM_UNLOCK();
        _ErrorHandle(ulError);
        return  (LW_NULL);
    }
    
    pvmpageVirtual->PAGE_ulFlags = ulFlags;
    
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOREMAP,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      (addr_t)((UINT64)paPhysicalAddr >> 32), (addr_t)(paPhysicalAddr),
                      stSize, LW_NULL);
    
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr + (addr_t)paUpPad));
}
/*********************************************************************************************************
** ��������: API_VmmIoRemapEx
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�û���ָ�� CACHE ���)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlags            �ڴ�����
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    return  (API_VmmIoRemapEx2((phys_addr_t)pvPhysicalAddr, stSize, ulFlags));
}
/*********************************************************************************************************
** ��������: API_VmmIoRemap2
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : paPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemap2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx2(paPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_VmmIoRemap
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx2((phys_addr_t)pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_VmmIoRemapNocache2
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : paPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapNocache2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx2(paPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_VmmIoRemapNocache
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapNocache (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx2((phys_addr_t)pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_VmmIoUnmap
** ��������: �ͷ� ioremap ռ�õ��߼��ռ�
** �䡡��  : pvVirtualMem    �����ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmIoUnmap (PVOID  pvVirtualAddr)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulVirtualAddr = (addr_t)pvVirtualAddr;
    
    ulVirtualAddr &= LW_CFG_VMM_PAGE_MASK;                              /*  ҳ������ַ                */
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulVirtualAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return;
    }
    
#if LW_CFG_CACHE_EN > 0
    API_CacheClear(DATA_CACHE, (PVOID)pvmpageVirtual->PAGE_ulPageAddr,
                   (size_t)(pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  ���������                  */
    
    __areaVirtualUnlinkPage(pvmpageVirtual->PAGE_ulPageAddr,
                            pvmpageVirtual);
    
    __vmmVirDevPageFree(pvmpageVirtual);                                /*  ɾ������ҳ��                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOUNMAP,
                      ulVirtualAddr, LW_NULL);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
