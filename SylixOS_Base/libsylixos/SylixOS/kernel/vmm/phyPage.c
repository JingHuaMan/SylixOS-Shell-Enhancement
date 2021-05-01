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
** ��   ��   ��: phyPage.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ����ҳ�����.

** BUG
2009.03.05  __vmmPhysicalGetZone() ��������Ķ�λ����.
2009.06.23  ���������ڴ���뿪�ٹ���.
2009.11.13  �����ȡ��Ƭ�����ڴ��С�ӿ�.
            ͬʱ�����ָ���� zone �з��������ڴ�.
2011.03.02  ���� __vmmPhysicalPageSetListFlag() ����.
2011.05.19  ��������ҳ������������ __vmmPhysicalPageTraversalList().
2011.08.11  ����ҳ���������ʹ��������ͬ�ķ���, �������Ա��� DMA ���������������˷�.
2013.05.30  ��������ҳ�����ù���.
2014.07.27  ��������ҳ�� CACHE ��������.
2016.08.19  __vmmPhysicalCreate() ֧�ֶ�ε�����������ڴ�.
2020.02.22  ����ȱҳ�ж��ڴ���С����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
#include "pageTable.h"
/*********************************************************************************************************
  ��ַ�ռ��ͻ���
*********************************************************************************************************/
extern BOOL     __vmmLibVirtualOverlap(addr_t  ulAddr, size_t  stSize);
/*********************************************************************************************************
  ���� zone ���ƿ�����
*********************************************************************************************************/
LW_VMM_ZONE                     _G_vmzonePhysical[LW_CFG_VMM_ZONE_NUM]; /*  ��������                    */
/*********************************************************************************************************
  �����ڴ� text data �δ�С
*********************************************************************************************************/
static LW_MMU_PHYSICAL_DESC     _G_vmphydescKernel[2];                  /*  �ں��ڴ���Ϣ                */
/*********************************************************************************************************
  �����ڴ����
*********************************************************************************************************/
static LW_VMM_PAGE_FAULT_LIMIT  _G_vmpflPhysical;                       /*  ȱҳ�ж������ڴ�����        */
static LW_OBJECT_HANDLE         _G_ulPageFaultGuarder = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
** ��������: __vmmPhysicalCreate
** ��������: ����һ�������ҳ����.
** �䡡��  : pphydesc          ��������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmPhysicalCreate (LW_MMU_PHYSICAL_DESC  pphydesc[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
    static   ULONG  ulZone  = 0;                                        /*  �ɶ��׷β����ڴ�          */
             INT    i;
             
    for (i = 0; ; i++) {
        if (pphydesc[i].PHYD_stSize == 0) {
            break;
        }
        
        _BugFormat(!ALIGNED(pphydesc[i].PHYD_ulPhyAddr, LW_CFG_VMM_PAGE_SIZE), LW_TRUE,
                   "physical zone vaddr 0x%08lx not page aligned.\r\n", 
                   pphydesc[i].PHYD_ulPhyAddr);
        
        switch (pphydesc[i].PHYD_uiType) {
        
        case LW_PHYSICAL_MEM_TEXT:
            if (_G_vmphydescKernel[LW_PHYSICAL_MEM_TEXT].PHYD_stSize) {
                _G_vmphydescKernel[LW_PHYSICAL_MEM_TEXT].PHYD_stSize += pphydesc[i].PHYD_stSize;
            
            } else {
                _G_vmphydescKernel[LW_PHYSICAL_MEM_TEXT] = pphydesc[i];
            }
            break;
            
        case LW_PHYSICAL_MEM_DATA:
            if (_G_vmphydescKernel[LW_PHYSICAL_MEM_DATA].PHYD_stSize) {
                _G_vmphydescKernel[LW_PHYSICAL_MEM_DATA].PHYD_stSize += pphydesc[i].PHYD_stSize;
            
            } else {
                _G_vmphydescKernel[LW_PHYSICAL_MEM_DATA] = pphydesc[i];
            }
            break;
        
        case LW_PHYSICAL_MEM_DMA:
            _BugHandle((pphydesc[i].PHYD_ulPhyAddr == (addr_t)LW_NULL), LW_TRUE,
                       "physical DMA zone can not use NULL address, you can move offset page.\r\n");
                                                                        /*  Ŀǰ��֧�� NULL ��ʼ��ַ    */
            if (ulZone < LW_CFG_VMM_ZONE_NUM) {
                _BugFormat(__vmmLibVirtualOverlap(pphydesc[i].PHYD_ulPhyAddr, 
                                                  pphydesc[i].PHYD_stSize), LW_TRUE,
                           "physical zone paddr 0x%08lx size: 0x%08zx overlap with virtual space.\r\n",
                           pphydesc[i].PHYD_ulPhyAddr, pphydesc[i].PHYD_stSize);
            
                ulError = __pageZoneCreate(&_G_vmzonePhysical[ulZone], 
                                           pphydesc[i].PHYD_ulPhyAddr, 
                                           pphydesc[i].PHYD_stSize,
                                           LW_ZONE_ATTR_DMA, 
                                           __VMM_PAGE_TYPE_PHYSICAL);   /*  ��ʼ������ zone             */
                if (ulError) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                    _ErrorHandle(ulError);
                    return  (ulError);
                }
                ulZone++;
            }
            break;
            
        case LW_PHYSICAL_MEM_APP:
            _BugHandle((pphydesc[i].PHYD_ulPhyAddr == (addr_t)LW_NULL), LW_TRUE,
                       "physical APP zone can not use NULL address, you can move offset page.\r\n");
                                                                        /*  Ŀǰ��֧�� NULL ��ʼ��ַ    */
            if (ulZone < LW_CFG_VMM_ZONE_NUM) {
                ulError = __pageZoneCreate(&_G_vmzonePhysical[ulZone], 
                                           pphydesc[i].PHYD_ulPhyAddr, 
                                           pphydesc[i].PHYD_stSize,
                                           LW_ZONE_ATTR_NONE, 
                                           __VMM_PAGE_TYPE_PHYSICAL);   /*  ��ʼ������ zone             */
                if (ulError) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                    _ErrorHandle(ulError);
                    return  (ulError);
                }
                ulZone++;
            }
            break;
            
        default:
            break;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageAlloc
** ��������: ����ָ������������ҳ��
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           uiAttr        ��Ҫ���������ҳ������
**           pulZoneIndex  ���� zone �±�
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmPhysicalPageAlloc (ULONG  ulPageNum, UINT  uiAttr, ULONG  *pulZoneIndex)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {                         /*  ����ѡ�� uiAttr ��ͬ�� zone */
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_uiAttr == uiAttr) {
            if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
                pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_PHYSICAL);
                if (pvmpage) {
                    *pulZoneIndex = (ULONG)i;
                    return  (pvmpage);
                }
            }
        }
    }
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((pvmzone->ZONE_uiAttr & uiAttr) == uiAttr) {                /*  ���ѡ��ӵ�� uiAttr �� zone */
            if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
                pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_PHYSICAL);
                if (pvmpage) {
                    *pulZoneIndex = (ULONG)i;
                    return  (pvmpage);
                }
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageAllocZone
** ��������: ����ָ������������ҳ�� (ָ����������)
** �䡡��  : ulZoneIndex   ��������
**           ulPageNum     ��Ҫ���������ҳ�����
**           uiAttr        ��Ҫ�����ҳ������
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmPhysicalPageAllocZone (ULONG  ulZoneIndex, ULONG  ulPageNum, UINT  uiAttr)
{
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    if (ulZoneIndex >= LW_CFG_VMM_ZONE_NUM) {
        return  (LW_NULL);
    }
        
    pvmzone = &_G_vmzonePhysical[ulZoneIndex];
    if (!pvmzone->ZONE_stSize) {                                        /*  ��Ч zone                   */
        return  (LW_NULL);
    }
    
    if ((pvmzone->ZONE_uiAttr & uiAttr) == uiAttr) {
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_PHYSICAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageAllocAlign
** ��������: ����ָ������������ҳ�� (��ָ�������ϵ)
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           stAlign       �ڴ�����ϵ
**           uiAttr        ��Ҫ���������ҳ������
**           pulZoneIndex  ���� zone �±�
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmPhysicalPageAllocAlign (ULONG   ulPageNum, 
                                           size_t  stAlign, 
                                           UINT    uiAttr, 
                                           ULONG  *pulZoneIndex)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_uiAttr == uiAttr) {                           /*  ����ѡ�� uiAttr ��ͬ�� zone */
            if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
                pvmpage = __pageAllocateAlign(pvmzone, ulPageNum, stAlign, __VMM_PAGE_TYPE_PHYSICAL);
                if (pvmpage) {
                    *pulZoneIndex = (ULONG)i;
                    return  (pvmpage);
                }
            }
        }
    }
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((pvmzone->ZONE_uiAttr & uiAttr) == uiAttr) {                /*  ���ѡ��ӵ�� uiAttr �� zone */
            if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
                pvmpage = __pageAllocateAlign(pvmzone, ulPageNum, stAlign, __VMM_PAGE_TYPE_PHYSICAL);
                if (pvmpage) {
                    *pulZoneIndex = (ULONG)i;
                    return  (pvmpage);
                }
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageClone
** ��������: ��¡һ������ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : �µ�����ҳ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmPhysicalPageClone (PLW_VMM_PAGE  pvmpage)
{
    PLW_VMM_PAGE    pvmpageNew;
    addr_t          ulSwitchAddr = __vmmVirtualSwitch();
    ULONG           ulZoneIndex;
    ULONG           ulError;
    
    if ((pvmpage->PAGE_ulCount != 1) ||
        (pvmpage->PAGE_iPageType != __VMM_PAGE_TYPE_PHYSICAL) ||
        (pvmpage->PAGE_ulMapPageAddr == PAGE_MAP_ADDR_INV)) {           /*  ��������ӳ���ϵ�ĵ�ҳ��    */
        _ErrorHandle(ERROR_VMM_PHYSICAL_PAGE);
        return  (LW_NULL);
    }
    
    pvmpageNew = __vmmPhysicalPageAlloc(1, LW_ZONE_ATTR_NONE, &ulZoneIndex);
    if (pvmpageNew == LW_NULL) {
        _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(pvmpageNew->PAGE_ulPageAddr,              /*  ʹ�� CACHE ����             */
                              ulSwitchAddr, 1,                          /*  �����������ַ              */
                              LW_VMM_FLAG_RDWR);                        /*  ӳ��ָ���������ַ          */
    if (ulError) {
        __vmmPhysicalPageFree(pvmpageNew);
        return  (LW_NULL);
    }
    
    KN_COPY_PAGE((PVOID)ulSwitchAddr, 
                 (PVOID)pvmpage->PAGE_ulMapPageAddr);                   /*  ����ҳ������                */
               
    if (API_CacheAliasProb()) {                                         /*  cache ��������              */
        API_CacheClearPage(DATA_CACHE, 
                           (PVOID)ulSwitchAddr,
                           (PVOID)pvmpageNew->PAGE_ulPageAddr,
                           LW_CFG_VMM_PAGE_SIZE);                       /*  ������д���ڴ沢��������    */
    }
    
    __vmmLibSetFlag(ulSwitchAddr, 1, LW_VMM_FLAG_FAIL, LW_TRUE);        /*  VIRTUAL_SWITCH ���������   */
    
    return  (pvmpageNew);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageRef
** ��������: ��������ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmPhysicalPageRef (PLW_VMM_PAGE  pvmpage)
{
    PLW_VMM_PAGE    pvmpageFake = (PLW_VMM_PAGE)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE));
    PLW_VMM_PAGE    pvmpageReal;
    
    if (pvmpageFake == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  ȱ���ں��ڴ�                */
        return  (LW_NULL);
    }
    
    *pvmpageFake = *pvmpage;
    
    if (LW_VMM_PAGE_IS_FAKE(pvmpage)) {
        pvmpageReal = LW_VMM_PAGE_GET_REAL(pvmpage);
    
    } else {
        pvmpageReal = pvmpage;
    }
    
    pvmpageReal->PAGE_ulRef++;                                          /*  ��ʵҳ������++              */
    
    _INIT_LIST_LINE_HEAD(&pvmpageFake->PAGE_lineFreeHash);
    _INIT_LIST_LINE_HEAD(&pvmpageFake->PAGE_lineManage);
    
    pvmpageFake->PAGE_ulRef         = 0ul;                              /*  fake ҳ��, ���ü�����Ч     */
    pvmpageFake->PAGE_pvmpageReal   = pvmpageReal;
    pvmpageFake->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;
    
    return  (pvmpageFake);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFree
** ��������: ����ָ������������ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageFree (PLW_VMM_PAGE  pvmpage)
{
    REGISTER PLW_VMM_ZONE   pvmzone = pvmpage->PAGE_pvmzoneOwner;
             PLW_VMM_PAGE   pvmpageReal;
    
    if (LW_VMM_PAGE_IS_FAKE(pvmpage)) {
        pvmpageReal = LW_VMM_PAGE_GET_REAL(pvmpage);
        __KHEAP_FREE(pvmpage);                                          /*  �ͷ� fake ���ƿ�            */
    
    } else {
        pvmpageReal = pvmpage;
        pvmpageReal->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;            /*  ��Ӧ�ĵ�ַ������Ч          */
    }
    
    pvmpageReal->PAGE_ulRef--;
    
    if (pvmpageReal->PAGE_ulRef == 0) {
        __pageFree(pvmzone, pvmpageReal);                               /*  û��������ֱ���ͷ�          */
    }
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFreeAll
** ��������: ����ָ������ռ����������ҳ��
** �䡡��  : pvmpageVirtual        ����ռ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����û��Ϊÿһ������ҳ����� __pageUnlink ��Ϊ���ô˺�����, ����ռ伴�����ͷ�.
*********************************************************************************************************/
VOID  __vmmPhysicalPageFreeAll (PLW_VMM_PAGE  pvmpageVirtual)
{
    __pageTraversalLink(pvmpageVirtual, __vmmPhysicalPageFree, 
                        LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageSetFlag
** ��������: ��������ҳ��� flag ��־
** �䡡��  : pvmpage       ҳ����ƿ�
**           ulFlag        flag ��־
**           bFlushTlb     ��� TLB
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageSetFlag (PLW_VMM_PAGE  pvmpage, ULONG  ulFlag, BOOL  bFlushTlb)
{
    if (pvmpage->PAGE_ulMapPageAddr != PAGE_MAP_ADDR_INV) {
        __vmmLibSetFlag(pvmpage->PAGE_ulMapPageAddr, 1, ulFlag, bFlushTlb);
        pvmpage->PAGE_ulFlags = ulFlag;
    }
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageSetFlagAll
** ��������: ��������ռ�����������ҳ��� flag ��־
** �䡡��  : pvmpageVirtual ����ռ�
**           ulFlag         flag ��־
**           bFlushTlb      ��� TLB
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageSetFlagAll (PLW_VMM_PAGE  pvmpageVirtual, ULONG  ulFlag)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    PLW_MMU_CONTEXT    pmmuctx = __vmmGetCurCtx();
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    __pageTraversalLink(pvmpageVirtual, __vmmPhysicalPageSetFlag, (PVOID)ulFlag,
                        (PVOID)LW_FALSE, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
                        
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    __vmmLibFlushTlb(pmmuctx, pvmpageVirtual->PAGE_ulPageAddr, pvmpageVirtual->PAGE_ulCount);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFlush
** ��������: ��������ҳ������ CACHE ��д
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

VOID  __vmmPhysicalPageFlush (PLW_VMM_PAGE  pvmpage)
{
    if (pvmpage->PAGE_ulMapPageAddr != PAGE_MAP_ADDR_INV) {
        API_CacheFlushPage(DATA_CACHE, 
                           (PVOID)pvmpage->PAGE_ulMapPageAddr, 
                           (PVOID)pvmpage->PAGE_ulPageAddr,
                           (size_t)(pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
    }
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFlushAll
** ��������: ʹ����ռ�����������ҳ���д
** �䡡��  : pvmpageVirtual ����ռ�
**           ulFlag         flag ��־
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageFlushAll (PLW_VMM_PAGE  pvmpageVirtual)
{
    __pageTraversalLink(pvmpageVirtual, __vmmPhysicalPageFlush,
                        LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageInvalidate
** ��������: ��������ҳ������ CACHE ��Ч
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageInvalidate (PLW_VMM_PAGE  pvmpage)
{
    if (pvmpage->PAGE_ulMapPageAddr != PAGE_MAP_ADDR_INV) {
        API_CacheInvalidatePage(DATA_CACHE, 
                                (PVOID)pvmpage->PAGE_ulMapPageAddr, 
                                (PVOID)pvmpage->PAGE_ulPageAddr,
                                (size_t)(pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
    }
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageInvalidateAll
** ��������: ʹ����ռ�����������ҳ����Ч
** �䡡��  : pvmpageVirtual ����ռ�
**           ulFlag         flag ��־
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageInvalidateAll (PLW_VMM_PAGE  pvmpageVirtual)
{
    __pageTraversalLink(pvmpageVirtual, __vmmPhysicalPageInvalidate,
                        LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageClear
** ��������: ��������ҳ������ CACHE ��д����Ч
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageClear (PLW_VMM_PAGE  pvmpage)
{
    if (pvmpage->PAGE_ulMapPageAddr != PAGE_MAP_ADDR_INV) {
        API_CacheClearPage(DATA_CACHE, 
                           (PVOID)pvmpage->PAGE_ulMapPageAddr, 
                           (PVOID)pvmpage->PAGE_ulPageAddr,
                           (size_t)(pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
    }
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageClearAll
** ��������: ʹ����ռ�����������ҳ���д����Ч
** �䡡��  : pvmpageVirtual ����ռ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalPageClearAll (PLW_VMM_PAGE  pvmpageVirtual)
{
    __pageTraversalLink(pvmpageVirtual, __vmmPhysicalPageClear,
                        LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: __vmmPhysicalGetZone
** ��������: ���������ַ, ȷ����Ч�� zone
** �䡡��  : ulAddr        �����ַ
** �䡡��  : ulZoneIndex   ���� zone �±�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmPhysicalGetZone (addr_t  ulAddr)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;

    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((ulAddr >= pvmzone->ZONE_ulAddr) &&
            (ulAddr <= pvmzone->ZONE_ulAddr + pvmzone->ZONE_stSize - 1)) {
            return  ((ULONG)i);
        }
    }
    
    return  (LW_CFG_VMM_ZONE_NUM);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageGetMinContinue
** ��������: �������ҳ����, ��С������ҳ�ĸ���
** �䡡��  : pulZoneIndex      �����Сҳ��� zone �±�
**           uiAttr            ��Ҫ���������ҳ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmPhysicalPageGetMinContinue (ULONG  *pulZoneIndex, UINT  uiAttr)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
             ULONG          ulMin = __ARCH_ULONG_MAX;
             ULONG          ulTemp;
             
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_uiAttr == uiAttr) {                           /*  ����ѡ�� uiAttr ��ͬ�� zone */
            ulTemp = __pageGetMinContinue(pvmzone);
            if ((ulTemp > 0) && (ulMin > ulTemp)) {
                ulMin = ulTemp;
                *pulZoneIndex = (ULONG)i;
            }
        }
    }
    
    if (ulMin != __ARCH_ULONG_MAX) {
        return  (ulMin);
    }
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pvmzone = &_G_vmzonePhysical[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((pvmzone->ZONE_uiAttr & uiAttr) == uiAttr) {                /*  ���ѡ��ӵ�� uiAttr �� zone */
            ulTemp = __pageGetMinContinue(pvmzone);
            if ((ulTemp > 0) && (ulMin > ulTemp)) {
                ulMin = ulTemp;
                *pulZoneIndex = (ULONG)i;
            }
        }
    }
    
    return  (ulMin);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalCreate
** ��������: ����ȱҳ�ж������ڴ�����.
** �䡡��  : pvpflNew          �µ�����
**           pvpflOld          ֮ǰ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __vmmPhysicalPageFaultLimit (PLW_VMM_PAGE_FAULT_LIMIT  pvpflNew,
                                  PLW_VMM_PAGE_FAULT_LIMIT  pvpflOld)
{
    if (pvpflOld) {
        *pvpflOld = _G_vmpflPhysical;
    }

    if (pvpflNew) {
        if ((pvpflNew->VPFL_ulRootWarnPages < pvpflNew->VPFL_ulRootMinPages) ||
            (pvpflNew->VPFL_ulUserWarnPages < pvpflNew->VPFL_ulUserMinPages)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);

        } else {
            _G_vmpflPhysical = *pvpflNew;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFaultCheck
** ��������: ���ȱҳ�ж������ڴ�����.
** �䡡��  : ulPageNum         ��Ҫ���������ҳ�����
**           ptcbCur           ��ǰ������ƿ�
**           pulWarn           ��Ҫ����ʱ, ֪ͨ�������߳�.
** �䡡��  : TRUE: �������, FALSE �ܾ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  __vmmPhysicalPageFaultCheck (ULONG  ulPageNum, PLW_CLASS_TCB  ptcbCur, LW_OBJECT_HANDLE  *pulWarn)
{
#define __VMM_PAGE_FAULT_CHECK(warnpages, minpages) \
        ulFreePage = 0; \
        for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) { \
            pvmzone = &_G_vmzonePhysical[i]; \
            if (!pvmzone->ZONE_stSize) { \
                break; \
            } \
            ulFreePage += pvmzone->ZONE_ulFreePage; \
            if (ulFreePage > (warnpages)) { \
                return  (LW_TRUE); \
            } \
        } \
        if (ulFreePage > (minpages)) { \
            if (_G_ulPageFaultGuarder) { \
                *pulWarn = _G_ulPageFaultGuarder; \
                _G_ulPageFaultGuarder = LW_OBJECT_HANDLE_INVALID; \
            } \
            return  (LW_TRUE); \
        }

             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER ULONG          ulFreePage;

    if (ptcbCur->TCB_euid == 0) {
        if (_G_vmpflPhysical.VPFL_ulRootWarnPages == 0) {
            return  (LW_TRUE);
        } else {
            __VMM_PAGE_FAULT_CHECK(_G_vmpflPhysical.VPFL_ulRootWarnPages,
                                   _G_vmpflPhysical.VPFL_ulRootMinPages);
        }

    } else {
        if (_G_vmpflPhysical.VPFL_ulUserWarnPages == 0) {
            return  (LW_TRUE);
        } else {
            __VMM_PAGE_FAULT_CHECK(_G_vmpflPhysical.VPFL_ulUserWarnPages,
                                   _G_vmpflPhysical.VPFL_ulUserMinPages);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFaultClear
** ��������: ȱҳ�жϾ����������ɾ��
** �䡡��  : ulId      ��ɾ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

VOID  __vmmPhysicalPageFaultClear (LW_OBJECT_HANDLE  ulId)
{
    if (_G_ulPageFaultGuarder == ulId) {
        _G_ulPageFaultGuarder =  LW_OBJECT_HANDLE_INVALID;
        KN_SMP_WMB();
    }
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
/*********************************************************************************************************
** ��������: __vmmPhysicalPageFaultGuarder
** ��������: ����ȱҳ�жϾ����߳�
** �䡡��  : ulGuarder      �µľ����߳� ID (������Ч)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __vmmPhysicalPageFaultGuarder (LW_OBJECT_HANDLE  ulGuarder)
{
    _G_ulPageFaultGuarder = ulGuarder;
    KN_SMP_WMB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmPhysicalGetKernelDesc
** ��������: ��������ڴ��ں� TEXT DATA ������
** �䡡��  : pphydescText      �ں� TEXT ������
**           pphydescData      �ں� DATA ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmPhysicalGetKernelDesc (PLW_MMU_PHYSICAL_DESC  pphydescText, 
                                  PLW_MMU_PHYSICAL_DESC  pphydescData)
{
    if (pphydescText) {
        *pphydescText = _G_vmphydescKernel[LW_PHYSICAL_MEM_TEXT];
    }
    
    if (pphydescData) {
        *pphydescData = _G_vmphydescKernel[LW_PHYSICAL_MEM_DATA];
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
