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
** ��   ��   ��: pageLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ����ҳ��������ղ�����.

** BUG
2008.12.23  ���� __pageFreeHashIndex() ����ֵ���� LW_CFG_VMM_MAX_ORDER - 1 �� BUG.
2008.12.25  ������ __pageFree() ��ʹ�ñ�־�ĸ�ֵ.
2009.06.23  ������ָ�������ϵҳ�濪�ٹ���, ��Ҫ������һЩ DMA ������Ҫ���ڴ�����ϵ��Ҫ���ϵͳ.
2009.07.03  ������һЩ GCC ����.
2009.11.10  ������������.
2009.11.13  ���������С����ҳ����㷨.
2010.03.10  ������ page alloc ��Բ������С������ҳ��ǿ�з���� BUG.
            ������ page alloc align ������ҳ�������жϵ�һ��С BUG.
2011.12.08  ����ҳ���ҳ����ƿ���ҪԤ�ȴ� kheap ����, �������Ա����� VMM_LOCK() �еȴ� kheap lock.
2013.05.30  ������¼���ҳ���ֶεĳ�ʼ��.
2013.06.03  ��������ҳ��������ҳ������ӹ�ϵ����.
2014.04.30  ���� split �� expand ��������ҳ���������չ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  ����ҳ�� hash ��
*********************************************************************************************************/
#define __VMM_PAGE_LINK_HASH_SIZE   32
#define __VMM_PAGE_LINK_HASH_MASK   (__VMM_PAGE_LINK_HASH_SIZE - 1)
#define __VMM_PAGE_LINK_HASH(key)   (((key) >> LW_CFG_VMM_PAGE_SHIFT) & __VMM_PAGE_LINK_HASH_MASK)
/*********************************************************************************************************
  ����ҳ����ƿ�
*********************************************************************************************************/
typedef union {
    LW_LIST_MONO        PAGECB_monoFreeList;
    LW_VMM_PAGE         PAGECB_vmpage;
} LW_VMM_PAGE_CB;
static LW_LIST_MONO_HEADER  _K_pmonoPhyPageFreeList = LW_NULL;
/*********************************************************************************************************
** ��������: __pageCbInit
** ��������: ��ʼ��ҳ����ƿ��
** �䡡��  : ulPageNum     ����ҳ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageCbInit (ULONG  ulPageNum)
{
    INT                  i;
    LW_VMM_PAGE_CB      *pvmpagecb;
    
    pvmpagecb = (LW_VMM_PAGE_CB *)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_CB) * (size_t)ulPageNum);
    if (pvmpagecb == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  ȱ���ں��ڴ�                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    for (i = 0; i < ulPageNum; i++) {
        _list_mono_free(&_K_pmonoPhyPageFreeList, &pvmpagecb[i].PAGECB_monoFreeList);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pageCbAlloc
** ��������: ��ȡһ��ҳ����ƿ�
** �䡡��  : iPageType     ҳ������
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VMM_PAGE  __pageCbAlloc (INT  iPageType)
{
    PLW_LIST_MONO    pmonoAlloc;
    size_t           stAllocSize;

    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        if (_K_pmonoPhyPageFreeList) {
            pmonoAlloc = _list_mono_allocate(&_K_pmonoPhyPageFreeList);
        } else {
            pmonoAlloc = LW_NULL;
        }
        return  ((PLW_VMM_PAGE)pmonoAlloc);
    
    } else {
        stAllocSize  = ROUND_UP(sizeof(LW_VMM_PAGE), sizeof(LW_STACK));
        stAllocSize += (sizeof(LW_LIST_LINE_HEADER) * __VMM_PAGE_LINK_HASH_SIZE);
        return  ((PLW_VMM_PAGE)__KHEAP_ALLOC(stAllocSize));
    }
}
/*********************************************************************************************************
** ��������: __pageCbFree
** ��������: �ͷ�һ��ҳ����ƿ�
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __pageCbFree (PLW_VMM_PAGE  pvmpage)
{
    PLW_LIST_MONO    pmonoFree = (PLW_LIST_MONO)pvmpage;

    if (pvmpage->PAGE_iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        _list_mono_free(&_K_pmonoPhyPageFreeList, pmonoFree);
    
    } else {
        __KHEAP_FREE(pvmpage);
    }
}
/*********************************************************************************************************
** ��������: __pageInitLink
** ��������: ��ʼ������ҳ�� hash ����
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pageInitLink (PLW_VMM_PAGE  pvmpage)
{
    INT     i;
    size_t  stOffset = ROUND_UP(sizeof(LW_VMM_PAGE), sizeof(LW_STACK));
    
    pvmpage->PAGE_plinePhyLink = (LW_LIST_LINE_HEADER *)((PCHAR)pvmpage + stOffset);
    
    for (i = 0; i < __VMM_PAGE_LINK_HASH_SIZE; i++) {
        pvmpage->PAGE_plinePhyLink[i] = LW_NULL;
    }
}
/*********************************************************************************************************
** ��������: __pageFreeHashIndex
** ��������: ����ָ����ҳ���Сѡ�� free area hash �����
** �䡡��  : ulPageNum     ��Ҫ��ȡ��ҳ������
** �䡡��  : free area hash �����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pageFreeHashIndex (ULONG  ulPageNum)
{
    REGISTER INT    i;
    
    for (i = 0; (1ul << i) < ulPageNum; i++);                           /*  ȷ����ں�                  */
    
    if (i >= LW_CFG_VMM_MAX_ORDER) {                                    /*  max LW_CFG_VMM_MAX_ORDER-1  */
        i =  LW_CFG_VMM_MAX_ORDER - 1;
    }
    
    return  (i);
}
/*********************************************************************************************************
** ��������: __pageAddToFreeHash
** ��������: ��һ��ҳ����������������� hash ��
** �䡡��  : pvmzone       ָ��������
**           pvmpage       ��Ҫ�����ҳ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pageAddToFreeHash (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
    REGISTER INT              iHashIndex;
    REGISTER PLW_VMM_FREEAREA pvmfaEntry;
    
    iHashIndex = __pageFreeHashIndex(pvmpage->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  ����ҷ�ҳ�� hash ���      */
    
    _List_Line_Add_Ahead(&pvmpage->PAGE_lineFreeHash,
                         &pvmfaEntry->FA_lineFreeHeader);               /*  ��������                    */
    pvmfaEntry->FA_ulCount++;
}
/*********************************************************************************************************
** ��������: __pageDelFromFreeHash
** ��������: ��һ��ҳ���������ӿ��� hash ����ɾ��.
** �䡡��  : pvmzone       ָ��������
**           pvmpage       ��Ҫ�����ҳ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pageDelFromFreeHash (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
    REGISTER INT              iHashIndex;
    REGISTER PLW_VMM_FREEAREA pvmfaEntry;
    
    iHashIndex = __pageFreeHashIndex(pvmpage->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  ����ҷ�ҳ�� hash ���      */
    
    _List_Line_Del(&pvmpage->PAGE_lineFreeHash,
                   &pvmfaEntry->FA_lineFreeHeader);                     /*  ��������ɾ��                */
    pvmfaEntry->FA_ulCount--;
}
/*********************************************************************************************************
** ��������: __pageStructSeparate
** ��������: ��һ��������ҳ����������������
** �䡡��  : pvmpage       ��ҳ����ƿ�
**           pvmpageNew    ��Ҫ�����ҳ����ƿ�
**           ulPageNum     ҳ������
**           iPageType     ҳ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pageStructSeparate (PLW_VMM_PAGE  pvmpage, 
                                   PLW_VMM_PAGE  pvmpageNew, 
                                   ULONG         ulPageNum,
                                   INT           iPageType)
{
    PLW_LIST_LINE      plineHeader = &pvmpage->PAGE_lineManage;

    _List_Line_Add_Tail(&pvmpageNew->PAGE_lineManage, &plineHeader);    /*  �����ھ�����                */
    pvmpageNew->PAGE_ulCount      = pvmpage->PAGE_ulCount - ulPageNum;
    pvmpageNew->PAGE_ulPageAddr   = pvmpage->PAGE_ulPageAddr + (ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
    pvmpageNew->PAGE_iPageType    = iPageType;
    pvmpageNew->PAGE_ulFlags      = pvmpage->PAGE_ulFlags;              /*  ҳ������                    */
    pvmpageNew->PAGE_pvmzoneOwner = pvmpage->PAGE_pvmzoneOwner;         /*  ��¼��������                */
    
    pvmpage->PAGE_ulCount = ulPageNum;                                  /*  �޸���ҳ����ƿ�Ĵ�С      */
}
/*********************************************************************************************************
** ��������: __pageFree
** ��������: ��ָ���� zone �ͷ�ҳ��
** �䡡��  : pvmzone       ָ��������
**           pvmpage       ��Ҫ�黹��ҳ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __pageFree (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
             PLW_VMM_PAGE     pvmpageLeft;
             PLW_VMM_PAGE     pvmpageRight;
    
    REGISTER PLW_LIST_LINE    plineLeft;
    REGISTER PLW_LIST_LINE    plineRight;
    
             PLW_LIST_LINE    plineDummyHeader = LW_NULL;               /*  ���ڲ������ݵ�ͷ            */
             
             ULONG            ulFreePageNum = pvmpage->PAGE_ulCount;    /*  �ͷŵ�ҳ������              */
             
    
    plineLeft  = _list_line_get_prev(&pvmpage->PAGE_lineManage);        /*  ���ҳ��                    */
    plineRight = _list_line_get_next(&pvmpage->PAGE_lineManage);        /*  �ҷ�ҳ��                    */
    
    if (plineLeft) {
        pvmpageLeft = _LIST_ENTRY(plineLeft, LW_VMM_PAGE, 
                                  PAGE_lineManage);                     /*  ���ҳ�ε�ַ                */
    } else {
        pvmpageLeft = LW_NULL;
    }
    
    if (plineRight) {
        pvmpageRight = _LIST_ENTRY(plineRight, LW_VMM_PAGE, 
                                   PAGE_lineManage);                    /*  �ҷ�ҳ�ε�ַ                */
    } else {
        pvmpageRight = LW_NULL;
    }
    
    if (pvmpageLeft) {
        if (pvmpageLeft->PAGE_bUsed) {
            goto    __merge_right;                                      /*  �����ҷ�ҳ�ξۺ�            */
        }
        
        __pageDelFromFreeHash(pvmzone, pvmpageLeft);                    /*  �ӿ��б���ɾ��              */
        pvmpageLeft->PAGE_ulCount += pvmpage->PAGE_ulCount;             /*  �ϲ���ҳ��                  */
        
        _List_Line_Del(&pvmpage->PAGE_lineManage,
                       &plineDummyHeader);                              /*  ���ھ�������ɾ��            */
        __pageCbFree(pvmpage);                                          /*  �ͷ�ҳ����ƿ��ڴ�          */

        pvmpage = pvmpageLeft;                                          /*  �Ѿ��ϲ������ҳ��          */
    }
    
__merge_right:
    if (pvmpageRight) {
        if (pvmpageRight->PAGE_bUsed) {
            goto    __right_merge_fail;                                 /*  �����ҷ�ҳ�ξۺ�ʧ��        */
        }
        
        __pageDelFromFreeHash(pvmzone, pvmpageRight);                   /*  �ӿ��б���ɾ��              */
        pvmpage->PAGE_ulCount += pvmpageRight->PAGE_ulCount;
        
        _List_Line_Del(&pvmpageRight->PAGE_lineManage,
                       &plineDummyHeader);                              /*  ���ھ�������ɾ��            */
        __pageCbFree(pvmpageRight);                                     /*  �ͷ�ҳ����ƿ��ڴ�          */
    }
    
__right_merge_fail:                                                     /*  �ҷ�ҳ�κϲ�����            */
    pvmpage->PAGE_bUsed = LW_FALSE;                                     /*  û��ʹ��                    */
    __pageAddToFreeHash(pvmzone, pvmpage);                              /*  �����������                */
    
    pvmzone->ZONE_ulFreePage += ulFreePageNum;                          /*  ���� zone ���ƿ�            */
}
/*********************************************************************************************************
** ��������: __pageAllocate
** ��������: ��ָ���� zone �з����������ҳ��. (ʹ�� best fit ����) (֮ǰ��Ҫȷ�� zone �ڿ���ҳ����)
** �䡡��  : pvmzone       ָ��������
**           ulPageNum     ��Ҫ��ȡ��ҳ������
**           iPageType     ҳ������
** �䡡��  : ���ٳ���ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageAllocate (PLW_VMM_ZONE  pvmzone, 
                              ULONG         ulPageNum, 
                              INT           iPageType)
{
    REGISTER INT                i;
    REGISTER INT                iHashIndex;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
    
             PLW_LIST_LINE      plineFree;
             PLW_VMM_PAGE       pvmpageFit = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFree;
             
             ULONG              ulTemp;
             ULONG              ulFit = ~0ul;
             
    
    iHashIndex = __pageFreeHashIndex(ulPageNum);                        /*  ��� hash ������ʼ��        */
    i = iHashIndex;
    
__check_hash:
    for (; i < LW_CFG_VMM_MAX_ORDER; i++) {                             /*  ��С��������                */
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  ��ÿ��� hash �����        */
        if (pvmfaEntry->FA_ulCount) {
            break;
        }
    }
    if (i >= LW_CFG_VMM_MAX_ORDER) {
        return  (LW_NULL);                                              /*  �޷������µ������ռ�        */
    }
    
    for (plineFree  = pvmfaEntry->FA_lineFreeHeader;
         plineFree != LW_NULL;
         plineFree  = _list_line_get_next(plineFree)) {                 /*  ��������                    */
        
        /*
         *  �� iHashIndex ������ڱ����з�ҳ����, ���ܹ���֤������ҳһ������ ulPageNum
         *  ����: iHashIndex = 5 ��, ���������������ҳӦ���� 16(2^4) �� 32(2^5)֮��, ������� ulPageNum
         *  Ϊ 29 �Ļ�,�����ܱ�֤���� iHashIndex = 5 �ڵ�������ҳ������ 29! ������������ж�!
         */
        if (((PLW_VMM_PAGE)plineFree)->PAGE_ulCount >= ulPageNum) {
            ulTemp = ((PLW_VMM_PAGE)plineFree)->PAGE_ulCount - ulPageNum;
                                                                        /*  �ҳ���ӽ������С��������ҳ*/
            if (ulTemp < ulFit) {
                pvmpageFit = (PLW_VMM_PAGE)plineFree;
                ulFit      = ulTemp;
            }
        }
    }
    
    if (pvmpageFit == LW_NULL) {                                        /*  û�д��� ulPageNum ������ҳ */
        i++;                                                            /*  �������ķ�ҳ������        */
        /*
         *  ����ֻ���ܷ���һ��, �� i == iHashIndex ʱ, �ſ���û���ҵ�������ҳ.
         */
        goto    __check_hash;
    }
    
    if (ulFit) {                                                        /*  ��Ҫ���                    */
        pvmpageNewFree = __pageCbAlloc(iPageType);
        if (pvmpageNewFree == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  ȱ���ں��ڴ�                */
            return  (LW_NULL);
        }
        
        _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  �ӿ��б���ɾ��              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageFit->PAGE_bUsed     = LW_TRUE;                           /*  ����ʹ�õķ�ҳ��            */
        pvmpageNewFree->PAGE_bUsed = LW_FALSE;                          /*  û��ʹ�õķ�ҳ��            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFree, 
                             ulPageNum, 
                             iPageType);                                /*  ҳ�����                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageNewFree);                   /*  ��ʣ��ҳ�������� hash ��  */
        
    } else {                                                            /*  �պ�ƥ��                    */
        _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  �ӿ��б���ɾ��              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageFit->PAGE_bUsed = LW_TRUE;                               /*  ����ʹ�õķ�ҳ��            */
    }
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpageFit->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;             /*  û��ӳ���ϵ                */
        pvmpageFit->PAGE_iChange       = 0;                             /*  û�б仯��                  */
        pvmpageFit->PAGE_ulRef         = 1ul;                           /*  ���ü�����ʼΪ 1            */
        pvmpageFit->PAGE_pvmpageReal   = LW_NULL;                       /*  ��ʵҳ��                    */
    
    } else {
        pvmpageFit->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpageFit);
    }
    
    pvmzone->ZONE_ulFreePage -= ulPageNum;                              /*  ���� zone ���ƿ�            */
    
    return  (pvmpageFit);
}
/*********************************************************************************************************
** ��������: __pageAllocateAlign
** ��������: ��ָ���� zone �з����������ҳ��, ͬʱ����ָ�����ڴ�����ϵ. 
**           (ʹ�� best fit ����) (֮ǰ��Ҫȷ�� zone �ڿ���ҳ����)
** �䡡��  : pvmzone       ָ��������
**           ulPageNum     ��Ҫ��ȡ��ҳ������
**           stAlign       �ڴ�����ϵ (������ҳ���С��������)
**           iPageType     ҳ������
** �䡡��  : ���ٳ���ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageAllocateAlign (PLW_VMM_ZONE  pvmzone, 
                                   ULONG         ulPageNum, 
                                   size_t        stAlign,
                                   INT           iPageType)
{
    REGISTER INT                i;
    REGISTER INT                iHashIndex;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
    
             PLW_LIST_LINE      plineFree;
             PLW_VMM_PAGE       pvmpageFit          = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFreeLeft  = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFreeRight = LW_NULL;
             
             addr_t             ulTemp;
             addr_t             ulFit = ~0;
             
             addr_t             ulAlignMask = (addr_t)(stAlign - 1);
             ULONG              ulLeftFreePageNum;
             ULONG              ulLeftFreePageNumFit = ~0ul;
             
    
    iHashIndex = __pageFreeHashIndex(ulPageNum);                        /*  ��� hash ������ʼ��        */
    
    for (i = iHashIndex; i < LW_CFG_VMM_MAX_ORDER; i++) {               /*  ��С��������                */
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  ��ÿ��� hash �����        */
        if (pvmfaEntry->FA_ulCount) {
            
            for (plineFree  = pvmfaEntry->FA_lineFreeHeader;
                 plineFree != LW_NULL;
                 plineFree  = _list_line_get_next(plineFree)) {         /*  ��������                    */
                 
                addr_t   ulPageAddr = ((PLW_VMM_PAGE)plineFree)->PAGE_ulPageAddr;
                
                if ((ulPageAddr & ulAlignMask) == 0) {                  /*  ����������������          */
                    ulLeftFreePageNum = 0;                              /*  ��˲���Ҫ����ҳ��          */
                } else {
                    ulTemp = (ulPageAddr | ulAlignMask) + 1;
                    ulLeftFreePageNum = (ulTemp - ulPageAddr) >> LW_CFG_VMM_PAGE_SHIFT;
                }
                
                if (((PLW_VMM_PAGE)plineFree)->PAGE_ulCount >=
                    (ulPageNum + ulLeftFreePageNum)) {                  /*  ���������㹻��Ŀռ�        */
                    
                    ulTemp = ((PLW_VMM_PAGE)plineFree)->PAGE_ulCount - ulPageNum;  
                                                                        /*  �ҳ���ӽ������С��������ҳ*/
                    if (ulTemp < ulFit) {
                        pvmpageFit           = (PLW_VMM_PAGE)plineFree;
                        ulFit                = ulTemp;
                        ulLeftFreePageNumFit = ulLeftFreePageNum;
                    }
                }
            }
            if (ulFit != (~0ul)) {                                      /*  ƥ��ɹ� ?                  */
                break;
            }
        }
    }
    if (i >= LW_CFG_VMM_MAX_ORDER) {
        return  (LW_NULL);                                              /*  �޷������µ������ռ�        */
    }
    
    if (ulLeftFreePageNumFit) {                                         /*  ��Ҫ��˴��                */
        pvmpageNewFreeLeft = __pageCbAlloc(iPageType);
        if (pvmpageNewFreeLeft == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  ȱ���ں��ڴ�                */
            return  (LW_NULL);
        }
    }
    if (ulFit != ulLeftFreePageNumFit) {                                /*  �Ҷ�Ҳ��Ҫ���              */
        pvmpageNewFreeRight = __pageCbAlloc(iPageType);
        if (pvmpageNewFreeRight == LW_NULL) {
            if (pvmpageNewFreeLeft) {
                __pageCbFree(pvmpageNewFreeLeft);                       /*  �ͷ���˿��ƿ��ڴ�          */
            }
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  ȱ���ں��ڴ�                */
            return  (LW_NULL);
        }
    }
    
    /*
     *  �����ҳ����ƿ�ӿ��� hash ����ɾ��.
     */
    _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                   &pvmfaEntry->FA_lineFreeHeader);                     /*  �ӿ��б���ɾ��              */
    pvmfaEntry->FA_ulCount--;
    
    /*
     *  ��ʼ��ֲ���, �����(�͵�ַ)���Ҷ�(�ߵ�ַ).
     */
    if (ulLeftFreePageNumFit) {                                         /*  ��Ҫ��˴��                */
        
        pvmpageFit->PAGE_bUsed         = LW_FALSE;                      /*  ��˱����ֶ�                */
        pvmpageNewFreeLeft->PAGE_bUsed = LW_TRUE;                       /*  �µķֶν���ʹ��            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFreeLeft, 
                             ulLeftFreePageNumFit,                      /*  ��ߺ��Ե�ҳ��              */
                             pvmpageFit->PAGE_iPageType);               /*  ҳ�����                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageFit);                       /*  �����δʹ�����������б�  */
        
        pvmpageFit = pvmpageNewFreeLeft;                                /*  ȥ����˵ķ�ҳ���ƿ�        */
    }
    
    if (ulFit != ulLeftFreePageNumFit) {                                /*  ��Ҫ�Ҷ˴��                */
        
        pvmpageFit->PAGE_bUsed          = LW_TRUE;                      /*  ����ʹ�õķ�ҳ��            */
        pvmpageNewFreeRight->PAGE_bUsed = LW_FALSE;                     /*  û��ʹ�õķ�ҳ��            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFreeRight, 
                             ulPageNum,                                 /*  ��Ҫ���ٵ�ҳ������          */
                             iPageType);                                /*  ҳ�����                    */
        
        __pageAddToFreeHash(pvmzone, pvmpageNewFreeRight);              /*  ��ʣ��ҳ�������� hash ��  */
    
    } else {
        pvmpageFit->PAGE_bUsed = LW_TRUE;                               /*  ����ʹ�õķ�ҳ��            */
    }
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpageFit->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;             /*  û��ӳ���ϵ                */
        pvmpageFit->PAGE_iChange       = 0;                             /*  û�б仯��                  */
        pvmpageFit->PAGE_ulRef         = 1ul;                           /*  ���ü�����ʼΪ 1            */
        pvmpageFit->PAGE_pvmpageReal   = LW_NULL;                       /*  ��ʵҳ��                    */
    
    } else {
        pvmpageFit->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpageFit);
    }
    
    pvmzone->ZONE_ulFreePage -= ulPageNum;                              /*  ���� zone ���ƿ�            */
    
    return  (pvmpageFit);
}
/*********************************************************************************************************
** ��������: __pageExpand
** ��������: ��һ������ҳ����ƿ�����
** �䡡��  : pvmpage              ��ҳ����ƿ�
**           ulExpPageNum         ��Ҫ�����ҳ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageExpand (PLW_VMM_PAGE   pvmpage, 
                     ULONG          ulExpPageNum)
{
    PLW_LIST_LINE       plineRight;
    PLW_LIST_LINE       plineDummyHeader = LW_NULL;                     /*  ���ڲ������ݵ�ͷ            */
    
    PLW_VMM_PAGE        pvmpageRight;
    PLW_VMM_PAGE        pvmpageNewFree;
    
    INT                 iHashIndex;
    PLW_VMM_FREEAREA    pvmfaEntry;
    PLW_VMM_ZONE        pvmzone;
    
    if (pvmpage->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) {           /*  ֻ����չ����ҳ��            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);                             /*  ȱ���ں��ڴ�                */
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    plineRight = _list_line_get_next(&pvmpage->PAGE_lineManage);        /*  �ҷ�ҳ��                    */
    if (plineRight) {
        pvmpageRight = _LIST_ENTRY(plineRight, LW_VMM_PAGE, 
                                   PAGE_lineManage);                    /*  �ҷ�ҳ�ε�ַ                */
    } else {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  ȱ�ٿɹ���չ��ҳ��          */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    if ((pvmpageRight->PAGE_bUsed) ||
        (pvmpageRight->PAGE_ulCount < ulExpPageNum)) {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  ȱ�ٿɹ���չ��ҳ��          */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    pvmzone    = pvmpage->PAGE_pvmzoneOwner;
    iHashIndex = __pageFreeHashIndex(pvmpageRight->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  ����ҷ�ҳ�� hash ���      */
    
    if (pvmpageRight->PAGE_ulCount > ulExpPageNum) {                    /*  �Ҳ�ֶλ���ʣ��            */
        
        pvmpageNewFree = __pageCbAlloc(pvmpageRight->PAGE_iPageType);
        if (pvmpageNewFree == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  ȱ���ں��ڴ�                */
            return  (ERROR_KERNEL_LOW_MEMORY);
        }
        
        _List_Line_Del(&pvmpageRight->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  �ӿ��б���ɾ��              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageRight->PAGE_bUsed   = LW_TRUE;                           /*  ����ʹ�õķ�ҳ��            */
        pvmpageNewFree->PAGE_bUsed = LW_FALSE;                          /*  û��ʹ�õķ�ҳ��            */
        __pageStructSeparate(pvmpageRight, 
                             pvmpageNewFree, 
                             ulExpPageNum, 
                             pvmpageRight->PAGE_iPageType);             /*  ҳ�����                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageNewFree);                   /*  ��ʣ��ҳ�������� hash ��  */
    
    } else {
        _List_Line_Del(&pvmpageRight->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  �ӿ��б���ɾ��              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageRight->PAGE_bUsed = LW_TRUE;                             /*  ����ʹ�õķ�ҳ��            */
    }
    
    pvmzone->ZONE_ulFreePage -= ulExpPageNum;                           /*  ���� zone ���ƿ�            */
    
    pvmpage->PAGE_ulCount += ulExpPageNum;                              /*  �ϲ��� pvmpage ��           */
    
    _List_Line_Del(&pvmpageRight->PAGE_lineManage,
                   &plineDummyHeader);                                  /*  ���ھ�������ɾ��            */
    __pageCbFree(pvmpageRight);                                         /*  �ͷ�ҳ����ƿ��ڴ�          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pageSplit
** ��������: ��һ������ҳ����ƿ���������
** �䡡��  : pvmpage              ��ҳ����ƿ�
**           ppvmpageSplit        ���������ҳ����ƿ�
**           ulPageNum            pvmpage ��Ҫ������ҳ�����, ʣ�ಿ�ֽ������ pvmpageSplit.
**           pvAreaCb             �µ� AreaCb.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageSplit (PLW_VMM_PAGE   pvmpage, 
                    PLW_VMM_PAGE  *ppvmpageSplit, 
                    ULONG          ulPageNum,
                    PVOID          pvAreaCb)
{
    PLW_VMM_PAGE    pvmpageSplit;

    if (pvmpage->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) {           /*  ֻ�ܲ������ҳ��            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }

    if (pvmpage->PAGE_ulCount <= ulPageNum) {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  ȱ���ں��ڴ�                */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    pvmpageSplit = __pageCbAlloc(pvmpage->PAGE_iPageType);
    if (pvmpageSplit == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  ȱ���ں��ڴ�                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    pvmpageSplit->PAGE_bUsed = LW_TRUE;                                 /*  ����ʹ�õķ�ҳ��            */
    __pageStructSeparate(pvmpage, 
                         pvmpageSplit, 
                         ulPageNum, 
                         pvmpage->PAGE_iPageType);                      /*  ҳ�����                    */
                         
    pvmpageSplit->PAGE_pvAreaCb = pvAreaCb;
    __pageInitLink(pvmpageSplit);
    
    *ppvmpageSplit = pvmpageSplit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pageMerge
** ��������: �ϲ���������������ҳ�� (�������豣֤ҳ��������)
** �䡡��  : pvmpageL         �͵�ַҳ��
**           pvmpageR         �ߵ�ַҳ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageMerge (PLW_VMM_PAGE   pvmpageL, 
                    PLW_VMM_PAGE   pvmpageR)
{
    PLW_LIST_LINE   plineDummyHeader = LW_NULL;                         /*  ���ڲ������ݵ�ͷ            */
    
    if ((pvmpageL->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) ||
        (pvmpageR->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL)) {        /*  ֻ�ܺϲ�����ҳ��            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    pvmpageL->PAGE_ulCount += pvmpageR->PAGE_ulCount;                   /*  �ϲ���ҳ��                  */
    
    _List_Line_Del(&pvmpageR->PAGE_lineManage,
                   &plineDummyHeader);                                  /*  ���ھ�������ɾ��            */
    
    __pageCbFree(pvmpageR);                                             /*  �ͷ�ҳ����ƿ��ڴ�          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pageLink
** ��������: ������ռ��м���һ������ҳ������
** �䡡��  : pvmpageVirtual       �����ַ��ҳ
**           pvmpagePhysical      �����ַ��ҳ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ҳ�� PAGE_ulMapPageAddr �ֶα�����Ч.
*********************************************************************************************************/
VOID  __pageLink (PLW_VMM_PAGE  pvmpageVirtual, PLW_VMM_PAGE  pvmpagePhysical)
{
    ULONG   ulKey = __VMM_PAGE_LINK_HASH(pvmpagePhysical->PAGE_ulMapPageAddr);
    
    _List_Line_Add_Ahead(&pvmpagePhysical->PAGE_lineFreeHash,
                         &pvmpageVirtual->PAGE_plinePhyLink[ulKey]);
}
/*********************************************************************************************************
** ��������: __pageUnlink
** ��������: ȡ������ҳ�������ҳ������ӹ�ϵ
** �䡡��  : pvmpageVirtual       �����ַ��ҳ
**           pvmpagePhysical      �����ַ��ҳ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ҳ�� PAGE_ulMapPageAddr �ֶα�����Ч.
*********************************************************************************************************/
VOID  __pageUnlink (PLW_VMM_PAGE  pvmpageVirtual, PLW_VMM_PAGE  pvmpagePhysical)
{
    ULONG   ulKey = __VMM_PAGE_LINK_HASH(pvmpagePhysical->PAGE_ulMapPageAddr);
    
    _List_Line_Del(&pvmpagePhysical->PAGE_lineFreeHash,
                   &pvmpageVirtual->PAGE_plinePhyLink[ulKey]);
}
/*********************************************************************************************************
** ��������: __pageFindLink
** ��������: ��ѯ����ҳ��ָ��������ҳ��
** �䡡��  : pvmpageVirtual       �����ַ��ҳ
**           ulVirAddr            �����ַ(ҳ�����)
** �䡡��  : ����ҳ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageFindLink (PLW_VMM_PAGE  pvmpageVirtual, addr_t  ulVirAddr)
{
    ULONG           ulKey = __VMM_PAGE_LINK_HASH(ulVirAddr);
    PLW_LIST_LINE   plineTemp;
    PLW_VMM_PAGE    pvmpagePhysical;
    
    for (plineTemp  = pvmpageVirtual->PAGE_plinePhyLink[ulKey];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pvmpagePhysical = _LIST_ENTRY(plineTemp, LW_VMM_PAGE, PAGE_lineFreeHash);
        if (pvmpagePhysical->PAGE_ulMapPageAddr == ulVirAddr) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pvmpagePhysical);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __pageTraversalLink
** ��������: ��������ռ���������ҳ��
** �䡡��  : pvmpageVirtual       �����ַ��ҳ
**           pfunc                ��������
**           pvArg[0 ~ 5]         ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ʹ�ð�ȫ�ı�����ʽ.
*********************************************************************************************************/
VOID  __pageTraversalLink (PLW_VMM_PAGE   pvmpageVirtual,
                           VOIDFUNCPTR    pfunc, 
                           PVOID          pvArg0,
                           PVOID          pvArg1,
                           PVOID          pvArg2,
                           PVOID          pvArg3,
                           PVOID          pvArg4,
                           PVOID          pvArg5)
{
    INT     i;
    
    for (i = 0; i < __VMM_PAGE_LINK_HASH_SIZE; i++) {
        PLW_LIST_LINE   plineTemp;
        PLW_VMM_PAGE    pvmpagePhysical;
        
        plineTemp = pvmpageVirtual->PAGE_plinePhyLink[i];
        while (plineTemp) {
            pvmpagePhysical = _LIST_ENTRY(plineTemp, LW_VMM_PAGE, PAGE_lineFreeHash);
            plineTemp       = _list_line_get_next(plineTemp);
        
            pfunc(pvmpagePhysical, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
        }
    }
}
/*********************************************************************************************************
** ��������: __pageGetMinContinue
** ��������: ��ָ���� zone �л����С��������ҳ���� (���� hash ����ѭ��, ���Թ���)
** �䡡��  : pvmzone       ָ��������
** �䡡��  : ��С������ҳ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageGetMinContinue (PLW_VMM_ZONE  pvmzone)
{
    REGISTER INT                i;
             BOOL               bOk = LW_FALSE;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
             PLW_VMM_PAGE       pvmpage;
    
    for (i = 0; i < LW_CFG_VMM_MAX_ORDER; i++) {
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  ��ÿ��� hash �����        */
        if (pvmfaEntry->FA_ulCount) {
            bOk = LW_TRUE;
            break;
        }
    }
    
    if (bOk) {
        /*
         *  ��ô� hash ����ڵĵ�һ��ҳ����ƿ�. Ԥ����С����ҳ��ĸ���.
         */
        pvmpage = (PLW_VMM_PAGE)pvmfaEntry->FA_lineFreeHeader;          /*  pvmpage ��һ��Ԫ���Ǳ�ָ��  */
        return  (pvmpage->PAGE_ulCount);
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: __pageZoneCreate
** ��������: ����һ�� page zone
** �䡡��  : pvmzone       ��Ҫ���Ľṹ��
**           ulAddr        ҳ����ʼ��ַ
**           stSize        ҳ���С
**           uiAttr        ��������
**           iPageType     ҳ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __pageZoneCreate (PLW_VMM_ZONE   pvmzone,
                         addr_t         ulAddr, 
                         size_t         stSize, 
                         UINT           uiAttr,
                         INT            iPageType)
{
    PLW_VMM_PAGE       pvmpage;
    
    pvmpage = __pageCbAlloc(iPageType);
    if (pvmpage == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  ȱ���ں��ڴ�                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    pvmzone->ZONE_ulFreePage = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    pvmzone->ZONE_ulAddr     = ulAddr;
    pvmzone->ZONE_stSize     = stSize;
    pvmzone->ZONE_uiAttr     = uiAttr;
    
    lib_bzero(pvmzone->ZONE_vmfa, sizeof(LW_VMM_FREEAREA) * LW_CFG_VMM_MAX_ORDER);
    
    _LIST_LINE_INIT_IN_CODE(pvmpage->PAGE_lineManage);                  /*  û���ھ�                    */
    pvmpage->PAGE_ulCount       = pvmzone->ZONE_ulFreePage;
    pvmpage->PAGE_ulPageAddr    = ulAddr;
    pvmpage->PAGE_iPageType     = iPageType;
    pvmpage->PAGE_ulFlags       = 0;
    pvmpage->PAGE_bUsed         = LW_FALSE;
    pvmpage->PAGE_pvmzoneOwner  = pvmzone;                              /*  ��¼���� zone               */
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpage->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;                /*  û��ӳ���ϵ                */
        pvmpage->PAGE_iChange       = 0;                                /*  û�б仯��                  */
        pvmpage->PAGE_ulRef         = 0ul;                              /*  ���ü�����ʼΪ 0            */
        pvmpage->PAGE_pvmpageReal   = LW_NULL;                          /*  ��ʵҳ��                    */
    
    } else {
        pvmpage->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpage);
    }
    
    __pageAddToFreeHash(pvmzone, pvmpage);                              /*  ������б�                  */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
