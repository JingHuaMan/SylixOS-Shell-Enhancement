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
** ��   ��   ��: vmmArea.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ��ַ�ռ����. �ṩ��ַ���鹦��.
                 ��ַ����ʹ�ù�ϣ�����.
** BUG
2008.12.23  ��������ռ�����Ĺ���.
2009.03.03  ��������������ȫʹ�ÿ⺯��.
2016.08.19  __areaPhysicalSpaceInit() ֧�ֶ�ε�����������ڴ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  ȫ�ֵ�ַ�ռ� hash ��С�趨
*********************************************************************************************************/
static const ULONG  _G_ulVmmAreaHashSizeTbl[][2] = {
/*********************************************************************************************************
        AREA SIZE      HASH SIZE
        (address)       (entry)
*********************************************************************************************************/
    {   (   8 * LW_CFG_MB_SIZE),       5       },
    {   (  16 * LW_CFG_MB_SIZE),       7       },
    {   (  32 * LW_CFG_MB_SIZE),      13       },
    {   (  64 * LW_CFG_MB_SIZE),      29       },
    {   ( 128 * LW_CFG_MB_SIZE),     157       },
    {   ( 256 * LW_CFG_MB_SIZE),     269       },
    {   ( 512 * LW_CFG_MB_SIZE),     557       },
    {   (1024 * LW_CFG_MB_SIZE),    1039       },
    {                         0,    1999       },
};
/*********************************************************************************************************
  ȫ�ֵ�ַ�ռ�
*********************************************************************************************************/
static LW_VMM_AREA  _G_vmareaZoneSpace[LW_CFG_VMM_ZONE_NUM];            /*  ����ҳ������                */
/*********************************************************************************************************
  hash ����
*********************************************************************************************************/
#define __VMM_AREA_HASH_INDEX(pvmarea, ulAddr)      \
        ((ulAddr >> LW_CFG_VMM_PAGE_SHIFT) % (pvmarea)->AREA_ulHashSize)
/*********************************************************************************************************
** ��������: __areaSpaceRbTreeTraversal
** ��������: ����һ�ź����
** �䡡��  : ptrbn          ��������ڵ�
**           pfuncCallback  �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __areaSpaceRbTreeTraversal (PLW_TREE_RB_NODE   ptrbn, VOIDFUNCPTR  pfuncCallback)
{
    PLW_VMM_PAGE       pvmpage;
    
    if (ptrbn) {
        __areaSpaceRbTreeTraversal(ptrbn->TRBN_ptrbnLeft, pfuncCallback);
        pvmpage = _TREE_ENTRY(ptrbn, LW_VMM_PAGE, PAGE_trbnNode);
        pfuncCallback(pvmpage);
        __areaSpaceRbTreeTraversal(ptrbn->TRBN_ptrbnRight, pfuncCallback);
    }
}
/*********************************************************************************************************
** ��������: __areaSpaceTraversal
** ��������: �ռ����
** �䡡��  : pvmarea        ��ַ�ռ�����
**           pfuncCallback  �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __areaSpaceTraversal (PLW_VMM_AREA  pvmarea, VOIDFUNCPTR  pfuncCallback)
{
    REGISTER INT                i;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn;
    
    for (i = 0; i < pvmarea->AREA_ulHashSize; i++) {
        ptrbr = &pvmarea->AREA_ptrbrHash[i];
        ptrbn = ptrbr->TRBR_ptrbnNode;
        
        if (ptrbn) {
            __areaSpaceRbTreeTraversal(ptrbn, pfuncCallback);
        }
    }
}
/*********************************************************************************************************
** ��������: __areaVirtualSpaceInit
** ��������: ���������ַ�ռ�����
** �䡡��  : pfuncCallback  �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaVirtualSpaceTraversal (VOIDFUNCPTR  pfuncCallback)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    
    __areaSpaceTraversal(&pmmuctx->MMUCTX_vmareaVirSpace, pfuncCallback);
}
/*********************************************************************************************************
** ��������: __areaPhysicalSpaceTraversal
** ��������: ����ռ����
** �䡡��  : ulZoneIndex    �����ַ zone �±�
**           pfuncCallback  �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaPhysicalSpaceTraversal (ULONG  ulZoneIndex, VOIDFUNCPTR  pfuncCallback)
{
    __areaSpaceTraversal(&_G_vmareaZoneSpace[ulZoneIndex], pfuncCallback);
}
/*********************************************************************************************************
** ��������: __areaSpaceInit
** ��������: ��ʼ����ַ�ռ�����
** �䡡��  : pvmarea       ��ַ�ռ�����
**           ulAddr        ��ʼ��ַ
**           stSize        ��С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __areaSpaceInit (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, size_t  stSize)
{
             INT    i;
    REGISTER ULONG  ulHashSize;
    
    for (i = 0; ; i++) {
        if (_G_ulVmmAreaHashSizeTbl[i][0] == 0) {
            ulHashSize = _G_ulVmmAreaHashSizeTbl[i][1];                 /*  �����С                  */
            break;
        
        } else {
            if (stSize >= _G_ulVmmAreaHashSizeTbl[i][0]) {
                continue;
            
            } else {
                ulHashSize = _G_ulVmmAreaHashSizeTbl[i][1];             /*  ȷ��                        */
                break;
            }
        } 
    }
    
    pvmarea->AREA_ulAreaAddr = ulAddr;
    pvmarea->AREA_stAreaSize = stSize;
    pvmarea->AREA_ulHashSize = ulHashSize;
    
    pvmarea->AREA_ptrbrHash = 
        (PLW_TREE_RB_ROOT)__KHEAP_ALLOC(sizeof(LW_TREE_RB_ROOT) * (size_t)ulHashSize);
    if (pvmarea->AREA_ptrbrHash == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  ȱ���ں��ڴ�                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    lib_bzero(pvmarea->AREA_ptrbrHash,
              (size_t)(sizeof(LW_TREE_RB_ROOT) * ulHashSize));          /*  ��� hash ��                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __areaVirtualSpaceInit
** ��������: ��ʼ�������ַ�ռ�����
** �䡡��  : pvirdes       ����ռ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __areaVirtualSpaceInit (LW_MMU_VIRTUAL_DESC   pvirdes[])
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
             INT              i;
             addr_t           ulAddr  = __ARCH_ULONG_MAX;
             addr_t           ulEnd   = 0;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        if (pvirdes[i].VIRD_stSize == 0) {
            break;
        }
        if (ulAddr > pvirdes[i].VIRD_ulVirAddr) {
            ulAddr = pvirdes[i].VIRD_ulVirAddr;
        }
        if (ulEnd < (pvirdes[i].VIRD_ulVirAddr + pvirdes[i].VIRD_stSize - 1)) {
            ulEnd = (pvirdes[i].VIRD_ulVirAddr + pvirdes[i].VIRD_stSize - 1);
        }
    }
    
    if (ulAddr == __ARCH_ULONG_MAX) {
        _ErrorHandle(ENOSPC);
        return  (ENOSPC);
    }
    
    return  (__areaSpaceInit(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, (size_t)(ulEnd - ulAddr + 1)));
}
/*********************************************************************************************************
** ��������: __areaPhysicalSpaceInit
** ��������: ��ʼ�������ַ�ռ�����
** �䡡��  : pphydesc      ����ռ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __areaPhysicalSpaceInit (LW_MMU_PHYSICAL_DESC  pphydesc[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
    static   ULONG  ulZone  = 0;                                        /*  �ɶ��׷β����ڴ�          */
             INT    i;
             
    for (i = 0; ; i++) {
        if (pphydesc[i].PHYD_stSize == 0) {
            break;
        }
        
        switch (pphydesc[i].PHYD_uiType) {
        
        case LW_PHYSICAL_MEM_DMA:
        case LW_PHYSICAL_MEM_APP:
            if (ulZone < LW_CFG_VMM_ZONE_NUM) {
                ulError = __areaSpaceInit(&_G_vmareaZoneSpace[ulZone], 
                                          pphydesc[i].PHYD_ulPhyAddr, 
                                          pphydesc[i].PHYD_stSize);
                if (ulError) {
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
** ��������: __areaSearchPage
** ��������: ͨ����ַ�����ѯҳ����ƿ�
** �䡡��  : pvmarea       ��ַ�ռ�����
**           ulAddr        ��ʼ��ַ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VMM_PAGE  __areaSearchPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn;
             PLW_VMM_PAGE       pvmpage;
             
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  ȷ�� hash ��λ��            */
    ptrbn       = ptrbr->TRBR_ptrbnNode;
    
    while (ptrbn) {                                                     /*  ��ʼ����������              */
        pvmpage = _TREE_ENTRY(ptrbn, LW_VMM_PAGE, PAGE_trbnNode);
        
        if (ulAddr < pvmpage->PAGE_ulPageAddr) {
            ptrbn = _tree_rb_get_left(ptrbn);                           /*  ����������                  */
        } else if (ulAddr > pvmpage->PAGE_ulPageAddr) {
            ptrbn = _tree_rb_get_right(ptrbn);                          /*  ����������                  */
        } else {
            return  (pvmpage);
        }
    }
    
    return  (LW_NULL);                                                  /*  û���ҵ�                    */
}
/*********************************************************************************************************
** ��������: __areaVirtualSearchPage
** ��������: �������ַ�ռ���, ͨ����ַ�����ѯҳ����ƿ�
** �䡡��  : ulAddr        ��ʼ��ַ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __areaVirtualSearchPage (addr_t  ulAddr)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    return  (__areaSearchPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr));
}
/*********************************************************************************************************
** ��������: __areaPhysicalSearchPage
** ��������: �������ַ�ռ���, ͨ����ַ�����ѯҳ����ƿ�
** �䡡��  : ulZoneIndex   �����ַ zone �±�
**           ulAddr        ��ʼ��ַ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __areaPhysicalSearchPage (ULONG  ulZoneIndex, addr_t  ulAddr)
{
    return  (__areaSearchPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr));
}
/*********************************************************************************************************
** ��������: __areaInsertPage
** ��������: ���ַ�ռ���ע��һ��ҳ����ƿ�
** �䡡��  : pvmarea       ��ַ�ռ�����
**           ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __areaInsertPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE  *pptrbnRoot;
    REGISTER PLW_TREE_RB_NODE   ptrbnParent = LW_NULL;
             PLW_VMM_PAGE       pvmpageTemp;
    
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  ȷ�� hash ��λ��            */
    
    pptrbnRoot = &ptrbr->TRBR_ptrbnNode;
    
    while (*pptrbnRoot) {
        ptrbnParent = *pptrbnRoot;
        pvmpageTemp = _TREE_ENTRY(ptrbnParent, LW_VMM_PAGE, PAGE_trbnNode);
        
        if (ulAddr < pvmpageTemp->PAGE_ulPageAddr) {
            pptrbnRoot = _tree_rb_get_left_addr(*pptrbnRoot);
        } else if (ulAddr > pvmpageTemp->PAGE_ulPageAddr) {
            pptrbnRoot = _tree_rb_get_right_addr(*pptrbnRoot);
        } else {
            return;                                                     /*  ���������е�����            */
        }
    }
    
    _tree_rb_link_node(&pvmpage->PAGE_trbnNode, 
                       ptrbnParent, 
                       pptrbnRoot);                                     /*  ���뵽����                  */
                       
    _Tree_Rb_Insert_Color(&pvmpage->PAGE_trbnNode, 
                          &pvmarea->AREA_ptrbrHash[ulHashIndex]);       /*  ά��ƽ��                    */
}
/*********************************************************************************************************
** ��������: __areaVirtualInsertPage
** ��������: �������ַ�ռ���ע��һ��ҳ����ƿ�
** �䡡��  : ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaVirtualInsertPage (addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    __areaInsertPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, pvmpage);
}
/*********************************************************************************************************
** ��������: __areaPhysicalInsertPage
** ��������: �������ַ�ռ���ע��һ��ҳ����ƿ�
** �䡡��  : ulZoneIndex   �����ַ zone �±�
**           ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaPhysicalInsertPage (ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    __areaInsertPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr, pvmpage);
}
/*********************************************************************************************************
** ��������: __areaUnlinkPage
** ��������: �ӵ�ַ�ռ��н���һ��ҳ����ƿ�
** �䡡��  : pvmarea       ��ַ�ռ�����
**           ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __areaUnlinkPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn = &pvmpage->PAGE_trbnNode;
    
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  ȷ�� hash ��λ��            */
    
    _Tree_Rb_Erase(ptrbn, ptrbr);
}
/*********************************************************************************************************
** ��������: __areaVirtualUnlinkPage
** ��������: �������ַ�ռ��н���һ��ҳ����ƿ�
** �䡡��  : ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaVirtualUnlinkPage (addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    __areaUnlinkPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, pvmpage);
}
/*********************************************************************************************************
** ��������: __areaPhysicalUnlinkPage
** ��������: �������ַ�ռ��н���һ��ҳ����ƿ�
** �䡡��  : pvmarea       ��ַ�ռ�����
**           ulAddr        ��ʼ��ַ
**           pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __areaPhysicalUnlinkPage (ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    __areaUnlinkPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr, pvmpage);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
