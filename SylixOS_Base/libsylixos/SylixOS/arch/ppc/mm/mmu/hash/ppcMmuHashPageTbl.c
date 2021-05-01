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
** ��   ��   ��: ppcMmuHashPageTbl.c
**
** ��   ��   ��: Dang.YueDong (��Ծ��)
**
** �ļ���������: 2016 �� 01 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���� HASH ҳ�� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID  ppcHashMmuSetSR(UINT32  uiSRn, UINT32  uiValue);
extern VOID  ppcHashMmuSetSDR1(UINT32);
extern VOID  ppcHashMmuHashPageTblPteSet(HASH_MMU_PTE  *pPte,
                                         UINT32         uiWord0,
                                         UINT32         uiWord1,
                                         UINT32         uiEffectiveAddr);
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static HASH_MMU_SR      _G_HashMmuKernelSR[MMU_SEG_NR];

LW_WEAK HASH_MMU_SR     _G_HashMmuSR[MMU_SEG_NR] = {
    /*
     * 0x00000000 - 0x0fffffff, kernel segment, can execute
     */
    [0 ].SR_uiValue = (0  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0x10000000 - 0x1fffffff, kernel segment, can execute
     */
    [1 ].SR_uiValue = (1  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0x20000000 - 0x2fffffff, user segment, can execute
     */
    [2 ].SR_uiValue = (2  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS | MMU_SEG_KP,
    /*
     * 0x30000000 - 0x3fffffff, user segment, can execute
     */
    [3 ].SR_uiValue = (3  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS | MMU_SEG_KP,
    /*
     * 0x40000000 - 0x4fffffff, user segment, can execute
     */
    [4 ].SR_uiValue = (4  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS | MMU_SEG_KP,
    /*
     * 0x50000000 - 0x5fffffff, user segment, can execute
     */
    [5 ].SR_uiValue = (5  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS | MMU_SEG_KP,
    /*
     * 0x60000000 - 0x6fffffff, kernel segment, can execute
     */
    [6 ].SR_uiValue = (6  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0x70000000 - 0x7fffffff, kernel segment, can execute
     */
    [7 ].SR_uiValue = (7  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0x80000000 - 0x8fffffff, user segment, can execute
     */
    [8 ].SR_uiValue = (8  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS | MMU_SEG_KP,
    /*
     * 0x90000000 - 0x9fffffff, kernel segment, can execute
     */
    [9 ].SR_uiValue = (9  << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xa0000000 - 0xafffffff, kernel segment, can execute
     */
    [10].SR_uiValue = (10 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xb0000000 - 0xbfffffff, kernel segment, can execute
     */
    [11].SR_uiValue = (11 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xc0000000 - 0xcfffffff, kernel segment, can execute
     */
    [12].SR_uiValue = (12 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xd0000000 - 0xdfffffff, kernel segment, can execute
     */
    [13].SR_uiValue = (13 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xe0000000 - 0xefffffff, kernel segment, can execute
     */
    [14].SR_uiValue = (14 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
    /*
     * 0xf0000000 - 0xffffffff, kernel segment, can execute
     */
    [15].SR_uiValue = (15 << MMU_SEG_VSID_SHIFT) | MMU_SEG_KS,
};
/*********************************************************************************************************
** ��������: __ppcHashPageTblPteSet
** ��������: ���� HASH ҳ��� PTE
** �䡡��  : pPte                  PTE
**           uiWord0               �� 0
**           uiWord1               �� 1
**           uiEffectiveAddr       ��Ч��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  VOID  __ppcHashPageTblPteSet (HASH_MMU_PTE  *pPte,
                                                UINT32         uiWord0,
                                                UINT32         uiWord1,
                                                UINT32         uiEffectiveAddr)
{
    ppcHashMmuHashPageTblPteSet(pPte, uiWord0, uiWord1, uiEffectiveAddr);
}
/*********************************************************************************************************
** ��������: __ppcHashPageTblPtegAddrGet
** ��������: ͨ����Ч��ַ��� PTEG
** �䡡��  : pHashPageTbl          HASH ҳ��
**           effectiveAddr         ��Ч��ַ
**           ppPrimPteg            �� PTEG
**           ppSecPteg             �� PTEG
**           puiAPI                API
**           puiVSID               VSID
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ppcHashPageTblPtegAddrGet (PHASH_PAGE_TBL  pHashPageTbl,
                                          HASH_MMU_EA     effectiveAddr,
                                          HASH_MMU_PTEG **ppPrimPteg,
                                          HASH_MMU_PTEG **ppSecPteg,
                                          UINT32         *puiAPI,
                                          UINT32         *puiVSID)
{
    UINT32  uiPageIndex;
    UINT32  uiHashValue1;
    UINT32  uiHashValue2;
    UINT32  uiHashValue1L;
    UINT32  uiHashValue1H;
    UINT32  uiHashValue2L;
    UINT32  uiHashValue2H;

    *puiVSID      = pHashPageTbl->HPT_SR[effectiveAddr.field.EA_ucSRn].field.SR_uiVSID;
    uiPageIndex   = effectiveAddr.field.EA_uiPageIndex;
    *puiAPI       = (uiPageIndex & MMU_EA_API) >> MMU_EA_API_SHIFT;

    uiHashValue1  = (*puiVSID & MMU_VSID_PRIM_HASH) ^ uiPageIndex;
    uiHashValue2  = ~uiHashValue1;

    uiHashValue1L = (uiHashValue1 & MMU_HASH_VALUE_LOW)  << MMU_PTE_HASH_VALUE_LOW_SHIFT;
    uiHashValue1H = (uiHashValue1 & MMU_HASH_VALUE_HIGH) >> MMU_HASH_VALUE_HIGH_SHIFT;

    uiHashValue2L = (uiHashValue2 & MMU_HASH_VALUE_LOW)  << MMU_PTE_HASH_VALUE_LOW_SHIFT;
    uiHashValue2H = (uiHashValue2 & MMU_HASH_VALUE_HIGH) >> MMU_HASH_VALUE_HIGH_SHIFT;

    uiHashValue1H = (uiHashValue1H & pHashPageTbl->HPT_uiHashPageTblMask) << MMU_PTE_HASH_VALUE_HIGH_SHIFT;
    uiHashValue2H = (uiHashValue2H & pHashPageTbl->HPT_uiHashPageTblMask) << MMU_PTE_HASH_VALUE_HIGH_SHIFT;

    *ppPrimPteg   = (HASH_MMU_PTEG *)(pHashPageTbl->HPT_uiHashPageTblOrg | uiHashValue1H | uiHashValue1L);
    *ppSecPteg    = (HASH_MMU_PTEG *)(pHashPageTbl->HPT_uiHashPageTblOrg | uiHashValue2H | uiHashValue2L);
}
/*********************************************************************************************************
** ��������: __ppcHashPageTblSearchInvalidPte
** ��������: ������Ч PTE
** �䡡��  : pPteg                 PTEG
** �䡡��  : PTE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static HASH_MMU_PTE  *__ppcHashPageTblSearchInvalidPte (HASH_MMU_PTEG  *pPteg)
{
    UINT           uiIndex;
    HASH_MMU_PTE  *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bV) {
            return  (pPte);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ppcHashPageTblSearchEliminatePte
** ��������: ǿ����̭һ�� PTE
** �䡡��  : pPrimPteg             �� PTEG
**           pSecPteg              �� PTEG
**           pbIsPrimary           �Ƿ��� PTE
** �䡡��  : PTE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static HASH_MMU_PTE  *__ppcHashPageTblSearchEliminatePte (HASH_MMU_PTEG  *pPrimPteg,
                                                          HASH_MMU_PTEG  *pSecPteg,
                                                          BOOL           *pbIsPrimary)
{
    UINT           uiIndex;
    HASH_MMU_PTE  *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPrimPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bR) {
            *pbIsPrimary = LW_TRUE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pSecPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bR) {
            *pbIsPrimary = LW_FALSE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPrimPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bC) {
            *pbIsPrimary = LW_TRUE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pSecPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bC) {
            *pbIsPrimary = LW_FALSE;
            return  (pPte);
        }
    }

    pPte = &pPrimPteg->PTEG_PTEs[0];
    *pbIsPrimary = LW_TRUE;

    return  (pPte);
}
/*********************************************************************************************************
** ��������: __ppcHashPageTblPteAdd
** ��������: ����һ�� PTE �� HASH ҳ��
** �䡡��  : pPte                  PTE
**           effectiveAddr         ��Ч��ַ
**           uiPteValue1           PTE ֵ1
**           uiAPI                 API
**           uiVSID                VSID
**           bR                    Referenced bit
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ppcHashPageTblPteAdd (HASH_MMU_PTE  *pPte,
                                     HASH_MMU_EA    effectiveAddr,
                                     UINT32         uiPteValue1,
                                     BOOL           bIsPrimary,
                                     UINT32         uiAPI,
                                     UINT32         uiVSID,
                                     BOOL           bR)
{
    HASH_MMU_PTE  pteTemp;

    pteTemp.words.PTE_uiWord0 = 0;
    pteTemp.words.PTE_uiWord1 = 0;

    pteTemp.field.PTE_uiVSID  = uiVSID;
    pteTemp.field.PTE_bH      = !bIsPrimary;
    pteTemp.field.PTE_ucAPI   = uiAPI;
    pteTemp.field.PTE_bV      = 1;
    pteTemp.field.PTE_bR      = bR;
    __ppcHashPageTblPteSet(pPte,
                           pteTemp.words.PTE_uiWord0,
                           uiPteValue1,
                           effectiveAddr.EA_uiValue);
}
/*********************************************************************************************************
** ��������: __ppcHashPageTblSearchPteByEA
** ��������: ͨ����Ч��ַ���� PTE
** �䡡��  : pPteg                 PTEG
**           effectiveAddr         ��Ч��ַ
**           uiAPI                 API
**           uiVSID                VSID
** �䡡��  : PTE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static HASH_MMU_PTE  *__ppcHashPageTblSearchPteByEA (HASH_MMU_PTEG *pPteg,
                                                     HASH_MMU_EA    effectiveAddr,
                                                     UINT32         uiAPI,
                                                     UINT32         uiVSID)
{
    UINT           uiIndex;
    HASH_MMU_PTE  *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPteg->PTEG_PTEs[uiIndex];
        if ((pPte->field.PTE_bV) &&
            (pPte->field.PTE_uiVSID == uiVSID) &&
            (pPte->field.PTE_ucAPI  == uiAPI)) {
            return  (pPte);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblSegmentFlag
** ��������: ��ö�ӳ���־
** �䡡��  : pHashPageTbl      HASH ҳ��
**           ulAddr            ��ַ
** �䡡��  : ��ӳ���־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcHashPageTblSegmentFlag (PHASH_PAGE_TBL  pHashPageTbl, addr_t  ulAddr)
{
    ULONG  ulFlag = 0;
    UCHAR  ucSRn  = ulAddr >> MMU_EA_SR_SHIFT;

    if (!pHashPageTbl->HPT_SR[ucSRn].field.SR_bN) {
        ulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ulFlag);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcHashPageTblGlobalInit (CPCHAR  pcMachineName)
{
    INT  i;

    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  �� CPU 0 ��ʼ�� KernelSR    */
        for (i = 0; i < MMU_SEG_NR; i++) {
            _G_HashMmuKernelSR[i].SR_uiValue = _G_HashMmuSR[i].SR_uiValue & (~MMU_SEG_KP);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblMemInit
** ��������: ��ʼ�� HASH ҳ��
** �䡡��  : pmmuctx                 MMU ������
**           uiMemSize               �ڴ��С
** �䡡��  : HASH ҳ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHASH_PAGE_TBL  ppcHashPageTblMemInit (PLW_MMU_CONTEXT  pmmuctx, UINT32  uiMemSize)
{
    UINT32          stHashPageTblSize;
    PHASH_PAGE_TBL  pHashPageTbl;

    if (uiMemSize < MMU_PTE_MIN_SIZE_8M) {
        return  (LW_NULL);
    }

    pHashPageTbl = __KHEAP_ALLOC(sizeof(HASH_PAGE_TBL));
    if (!pHashPageTbl) {
        return  (LW_NULL);
    }

    LW_SPIN_INIT(&pHashPageTbl->HPT_splLock);

    if (uiMemSize >= MMU_PTE_MIN_SIZE_4G) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_4G;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_4G;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_4G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_2G) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_2G;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_2G;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_2G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_1G) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_1G;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_1G;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_1G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_512M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_512M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_512M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_512M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_256M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_256M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_256M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_256M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_128M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_128M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_128M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_128M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_64M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_64M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_64M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_64M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_32M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_32M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_32M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_32M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_16M) {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_16M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_16M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_16M;

    } else {
        pHashPageTbl->HPT_uiHashPageTblOrg  = MMU_SDR1_HTABORG_8M;
        pHashPageTbl->HPT_uiHashPageTblMask = MMU_SDR1_HTABMASK_8M;
        stHashPageTblSize = MMU_PTE_MIN_SIZE_8M;
    }

    /*
     * HASH ҳ�����ڵ��ڴ���ڴ�������� M λ����Ϊ 1 (��Ӳ����֤�ڴ�һ����)
     */
    pHashPageTbl->HPT_pvHashPageTblAddr = __KHEAP_ALLOC_ALIGN(stHashPageTblSize, stHashPageTblSize);
    if (pHashPageTbl->HPT_pvHashPageTblAddr) {
        lib_bzero(pHashPageTbl->HPT_pvHashPageTblAddr, stHashPageTblSize);

        pHashPageTbl->HPT_uiHashPageTblOrg &= (UINT32)pHashPageTbl->HPT_pvHashPageTblAddr;

#ifdef MMU_PTE_DEBUG
        pHashPageTbl->HPT_uiPtegNr = stHashPageTblSize / 64;
#endif                                                                  /*  defined(MMU_PTE_DEBUG)      */
    } else {
        __KHEAP_FREE(pHashPageTbl);
        pHashPageTbl = LW_NULL;
    }

    pHashPageTbl->HPT_SR = _G_HashMmuKernelSR;

    return  (pHashPageTbl);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblMemFree
** ��������: �ͷ� HASH ҳ��
** �䡡��  : pHashPageTbl          HASH ҳ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcHashPageTblMemFree (PHASH_PAGE_TBL  pHashPageTbl)
{
    __KHEAP_FREE(pHashPageTbl->HPT_pvHashPageTblAddr);
    __KHEAP_FREE(pHashPageTbl);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pHashPageTbl          HASH ҳ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcHashPageTblMakeCurCtx (PHASH_PAGE_TBL  pHashPageTbl)
{
    INT  i;

    for (i = 0; i < MMU_SEG_NR; i++) {                                  /*  ���öμĴ���                */
        ppcHashMmuSetSR(i, pHashPageTbl->HPT_SR[i].SR_uiValue);
    }

    ppcHashMmuSetSDR1(pHashPageTbl->HPT_uiHashPageTblOrg |
                      pHashPageTbl->HPT_uiHashPageTblMask);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblMakeTrans
** ��������: ����ӳ���ϵ
** �䡡��  : pHashPageTbl          HASH ҳ��
**           ulEffectiveAddr       ��Ч��ַ
**           uiPteValue1           PTE ֵ1
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcHashPageTblMakeTrans (PHASH_PAGE_TBL  pHashPageTbl,
                               addr_t          ulEffectiveAddr,
                               UINT32          uiPteValue1)
{
    INTREG          iregInterLevel;
    HASH_MMU_PTEG  *pPrimPteg;
    HASH_MMU_PTEG  *pSecPteg;
    HASH_MMU_PTE   *pPte;
    BOOL            bIsPrimary = LW_TRUE;
    HASH_MMU_EA     effectiveAddr;
    UINT32          uiAPI;
    UINT32          uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    __ppcHashPageTblPtegAddrGet(pHashPageTbl,
                                effectiveAddr,
                                &pPrimPteg,
                                &pSecPteg,
                                &uiAPI,
                                &uiVSID);

    LW_SPIN_LOCK_QUICK(&pHashPageTbl->HPT_splLock, &iregInterLevel);

    /*
     * ���� EA �� PTE
     */
    pPte = __ppcHashPageTblSearchPteByEA(pPrimPteg,
                                         effectiveAddr,
                                         uiAPI,
                                         uiVSID);
    if (!pPte) {
        pPte = __ppcHashPageTblSearchPteByEA(pSecPteg,
                                             effectiveAddr,
                                             uiAPI,
                                             uiVSID);
    }

    if (pPte) {                                                         /*  �ҵ���                      */
        if (uiPteValue1) {
            __ppcHashPageTblPteSet(pPte,
                                   pPte->words.PTE_uiWord0,
                                   uiPteValue1,
                                   ulEffectiveAddr);                    /*  �ı� PTE                    */
        } else {
            __ppcHashPageTblPteSet(pPte,
                                   0,
                                   0,
                                   ulEffectiveAddr);                    /*  ɾ�� PTE                    */
        }
    } else {                                                            /*  EA ������ PTE               */
        if (uiPteValue1) {                                              /*  ��Ҫ���� EA �� PTE          */
            /*
             * ����һ����Ч�� PTE
             */
            pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
            if (!pPte) {
                pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
                if (pPte) {
                    bIsPrimary = LW_FALSE;
                }
            }

            if (pPte) {                                                 /*  �ҵ���Ч�� PTE              */
                __ppcHashPageTblPteAdd(pPte,
                                       effectiveAddr,
                                       uiPteValue1,
                                       bIsPrimary,
                                       uiAPI,
                                       uiVSID,
                                       LW_FALSE);                       /*  ����һ�� PTE                */
            } else {
                /*
                 * û����Ч�� PTE���ⲿ����Ч TLB������ǿ����̭ PTE��������� MISS
                 */
            }
        } else {
            /*
             * Ϊȡ��ӳ��, �ⲿ����Ч TLB
             */
        }
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblFlagSet
** ��������: ����ӳ���־
** �䡡��  : pHashPageTbl          HASH ҳ��
**           ulEffectiveAddr       ��Ч��ַ
**           uiPteValue1           PTE ֵ1
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��: 
** ˵  ��  : ���û���ҵ����ⲿ����Ч TLB������������Ч PTE ��ǿ����̭ PTE��������� MISS
*********************************************************************************************************/
INT  ppcHashPageTblFlagSet (PHASH_PAGE_TBL  pHashPageTbl,
                            addr_t          ulEffectiveAddr,
                            UINT32          uiPteValue1)
{
    INTREG          iregInterLevel;
    HASH_MMU_PTEG  *pPrimPteg;
    HASH_MMU_PTEG  *pSecPteg;
    HASH_MMU_PTE   *pPte;
    HASH_MMU_EA     effectiveAddr;
    UINT32          uiAPI;
    UINT32          uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    __ppcHashPageTblPtegAddrGet(pHashPageTbl,
                                effectiveAddr,
                                &pPrimPteg,
                                &pSecPteg,
                                &uiAPI,
                                &uiVSID);

    LW_SPIN_LOCK_QUICK(&pHashPageTbl->HPT_splLock, &iregInterLevel);

    /*
     * ���� EA �� PTE
     */
    pPte = __ppcHashPageTblSearchPteByEA(pPrimPteg,
                                         effectiveAddr,
                                         uiAPI,
                                         uiVSID);
    if (!pPte) {
        pPte = __ppcHashPageTblSearchPteByEA(pSecPteg,
                                             effectiveAddr,
                                             uiAPI,
                                             uiVSID);
    }

    if (pPte) {                                                         /*  �ҵ���                      */
        if (uiPteValue1) {
            __ppcHashPageTblPteSet(pPte,
                                   pPte->words.PTE_uiWord0,
                                   uiPteValue1,
                                   ulEffectiveAddr);                    /*  �ı� PTE                    */
        } else {
            __ppcHashPageTblPteSet(pPte,
                                   0,
                                   0,
                                   ulEffectiveAddr);                    /*  ɾ�� PTE                    */
        }
    } else {
        /*
         * û���ҵ����ⲿ����Ч TLB
         */
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblPteMissHandle
** ��������: ���� PTE MISS
** �䡡��  : pHashPageTbl          HASH ҳ��
**           ulEffectiveAddr       ��Ч��ַ
**           uiPteValue1           PTE ֵ1
** �䡡��  : ��ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcHashPageTblPteMissHandle (PHASH_PAGE_TBL  pHashPageTbl,
                                    addr_t          ulEffectiveAddr,
                                    UINT32          uiPteValue1)
{
    INTREG          iregInterLevel;
    HASH_MMU_PTEG  *pPrimPteg;
    HASH_MMU_PTEG  *pSecPteg;
    HASH_MMU_PTE   *pPte;
    BOOL            bIsPrimary = LW_TRUE;
    HASH_MMU_EA     effectiveAddr;
    UINT32          uiAPI;
    UINT32          uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    __ppcHashPageTblPtegAddrGet(pHashPageTbl,
                                effectiveAddr,
                                &pPrimPteg,
                                &pSecPteg,
                                &uiAPI,
                                &uiVSID);

    LW_SPIN_LOCK_QUICK(&pHashPageTbl->HPT_splLock, &iregInterLevel);

#ifdef MMU_PTE_DEBUG
    pHashPageTbl->HPT_uiPteMissCounter++;
#endif                                                                  /*  defined(MMU_PTE_DEBUG)      */

    /*
     * ����һ����Ч�� PTE
     */
    pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
    if (!pPte) {
        pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
        if (pPte) {
            bIsPrimary = LW_FALSE;
        }
    }

    if (!pPte) {                                                        /*  û����Ч�� PTE              */
        pPte = __ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                  pSecPteg,
                                                  &bIsPrimary);         /*  ǿ����̭һ�� PTE            */

        /*
         * ���ﲢ����Ч����̭�� PTE ��Ӧ�� TLB
         * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
         */
    }

    __ppcHashPageTblPteAdd(pPte,
                           effectiveAddr,
                           uiPteValue1,
                           bIsPrimary,
                           uiAPI,
                           uiVSID,
                           LW_FALSE);                                   /*  ����һ�� PTE                */

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (LW_VMM_ABORT_TYPE_NOINFO);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblPtePreLoad
** ��������: PTE Ԥ����
** �䡡��  : pHashPageTbl          HASH ҳ��
**           ulEffectiveAddr       ��Ч��ַ
**           uiPteValue1           PTE ֵ1
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcHashPageTblPtePreLoad (PHASH_PAGE_TBL  pHashPageTbl,
                               addr_t          ulEffectiveAddr,
                               UINT32          uiPteValue1)
{
    INTREG          iregInterLevel;
    HASH_MMU_PTEG  *pPrimPteg;
    HASH_MMU_PTEG  *pSecPteg;
    HASH_MMU_PTE   *pPte;
    BOOL            bIsPrimary = LW_TRUE;
    HASH_MMU_EA     effectiveAddr;
    UINT32          uiAPI;
    UINT32          uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    __ppcHashPageTblPtegAddrGet(pHashPageTbl,
                                effectiveAddr,
                                &pPrimPteg,
                                &pSecPteg,
                                &uiAPI,
                                &uiVSID);

    LW_SPIN_LOCK_QUICK(&pHashPageTbl->HPT_splLock, &iregInterLevel);

    /*
     * ���� EA �� PTE
     */
    pPte = __ppcHashPageTblSearchPteByEA(pPrimPteg,
                                         effectiveAddr,
                                         uiAPI,
                                         uiVSID);
    if (!pPte) {
        pPte = __ppcHashPageTblSearchPteByEA(pSecPteg,
                                             effectiveAddr,
                                             uiAPI,
                                             uiVSID);
    }

    if (!pPte) {                                                        /*  EA ������ PTE, ��Ҫ����     */
        /*
         * ����һ����Ч�� PTE
         */
        pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
        if (!pPte) {
            pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
            if (pPte) {
                bIsPrimary = LW_FALSE;
            }
        }

        if (!pPte) {                                                    /*  û����Ч�� PTE              */
            pPte = __ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                      pSecPteg,
                                                      &bIsPrimary);     /*  ǿ����̭һ�� PTE            */

            /*
             * ���ﲢ����Ч����̭�� PTE ��Ӧ�� TLB
             * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
             */
        }

        __ppcHashPageTblPteAdd(pPte,
                               effectiveAddr,
                               uiPteValue1,
                               bIsPrimary,
                               uiAPI,
                               uiVSID,
                               LW_TRUE);                                /*  ����һ�� PTE(������������̭)*/
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashPageTblDump
** ��������: ��ӡ MMU HASH PAGE TABLE ��Ϣ
** �䡡��  : pHashPageTbl          HASH ҳ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef MMU_PTE_DEBUG

VOID  ppcHashPageTblDump (PHASH_PAGE_TBL  pHashPageTbl)
{
    HASH_MMU_PTEG  *pPteg = (HASH_MMU_PTEG *)pHashPageTbl->HPT_uiHashPageTblOrg;
    INT             i, j;
    INT             iCount;
    INT             iNewLine = 0;

    printf("PTEG usage:\n");

    for (i = 0; i < pHashPageTbl->HPT_uiPtegNr; i++, pPteg++) {
        iCount = 0;
        for (j = 0; j < 8; j++) {
            if (pPteg->PTEG_PTEs[j].field.PTE_bV) {
                iCount++;
            }
        }

        if (iCount) {
            printf("%5d-%d ", i, iCount);
            if (++iNewLine % 10 == 0) {
                printf("\n");
            }
        }
    }

    printf("\n\nPTE Miss Counter = %d\n", pHashPageTbl->HPT_uiPteMissCounter);
    printf("PTEG Number\t = %d\n", pHashPageTbl->HPT_uiPtegNr);
}

#endif                                                                  /*  defined(MMU_PTE_DEBUG)      */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
