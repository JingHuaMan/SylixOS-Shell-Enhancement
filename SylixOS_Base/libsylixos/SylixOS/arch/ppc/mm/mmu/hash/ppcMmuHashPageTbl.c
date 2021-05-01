/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcMmuHashPageTbl.c
**
** 创   建   人: Dang.YueDong (党跃东)
**
** 文件创建日期: 2016 年 01 月 15 日
**
** 描        述: PowerPC 体系构架 HASH 页表 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppcHashMmuSetSR(UINT32  uiSRn, UINT32  uiValue);
extern VOID  ppcHashMmuSetSDR1(UINT32);
extern VOID  ppcHashMmuHashPageTblPteSet(HASH_MMU_PTE  *pPte,
                                         UINT32         uiWord0,
                                         UINT32         uiWord1,
                                         UINT32         uiEffectiveAddr);
/*********************************************************************************************************
  全局变量定义
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
** 函数名称: __ppcHashPageTblPteSet
** 功能描述: 设置 HASH 页表的 PTE
** 输　入  : pPte                  PTE
**           uiWord0               字 0
**           uiWord1               字 1
**           uiEffectiveAddr       有效地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  VOID  __ppcHashPageTblPteSet (HASH_MMU_PTE  *pPte,
                                                UINT32         uiWord0,
                                                UINT32         uiWord1,
                                                UINT32         uiEffectiveAddr)
{
    ppcHashMmuHashPageTblPteSet(pPte, uiWord0, uiWord1, uiEffectiveAddr);
}
/*********************************************************************************************************
** 函数名称: __ppcHashPageTblPtegAddrGet
** 功能描述: 通过有效地址获得 PTEG
** 输　入  : pHashPageTbl          HASH 页表
**           effectiveAddr         有效地址
**           ppPrimPteg            主 PTEG
**           ppSecPteg             次 PTEG
**           puiAPI                API
**           puiVSID               VSID
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: __ppcHashPageTblSearchInvalidPte
** 功能描述: 搜索无效 PTE
** 输　入  : pPteg                 PTEG
** 输　出  : PTE
** 全局变量:
** 调用模块:
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
** 函数名称: __ppcHashPageTblSearchEliminatePte
** 功能描述: 强制淘汰一个 PTE
** 输　入  : pPrimPteg             主 PTEG
**           pSecPteg              次 PTEG
**           pbIsPrimary           是否主 PTE
** 输　出  : PTE
** 全局变量:
** 调用模块:
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
** 函数名称: __ppcHashPageTblPteAdd
** 功能描述: 增加一个 PTE 到 HASH 页表
** 输　入  : pPte                  PTE
**           effectiveAddr         有效地址
**           uiPteValue1           PTE 值1
**           uiAPI                 API
**           uiVSID                VSID
**           bR                    Referenced bit
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: __ppcHashPageTblSearchPteByEA
** 功能描述: 通过有效地址查找 PTE
** 输　入  : pPteg                 PTEG
**           effectiveAddr         有效地址
**           uiAPI                 API
**           uiVSID                VSID
** 输　出  : PTE
** 全局变量:
** 调用模块:
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
** 函数名称: ppcHashPageTblSegmentFlag
** 功能描述: 获得段映射标志
** 输　入  : pHashPageTbl      HASH 页表
**           ulAddr            地址
** 输　出  : 段映射标志
** 全局变量:
** 调用模块:
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
** 函数名称: ppcHashPageTblGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcHashPageTblGlobalInit (CPCHAR  pcMachineName)
{
    INT  i;

    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 CPU 0 初始化 KernelSR    */
        for (i = 0; i < MMU_SEG_NR; i++) {
            _G_HashMmuKernelSR[i].SR_uiValue = _G_HashMmuSR[i].SR_uiValue & (~MMU_SEG_KP);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblMemInit
** 功能描述: 初始化 HASH 页表
** 输　入  : pmmuctx                 MMU 上下文
**           uiMemSize               内存大小
** 输　出  : HASH 页表
** 全局变量:
** 调用模块:
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
     * HASH 页表所在的内存的内存访问属性 M 位必须为 1 (即硬件保证内存一致性)
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
** 函数名称: ppcHashPageTblMemFree
** 功能描述: 释放 HASH 页表
** 输　入  : pHashPageTbl          HASH 页表
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcHashPageTblMemFree (PHASH_PAGE_TBL  pHashPageTbl)
{
    __KHEAP_FREE(pHashPageTbl->HPT_pvHashPageTblAddr);
    __KHEAP_FREE(pHashPageTbl);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pHashPageTbl          HASH 页表
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcHashPageTblMakeCurCtx (PHASH_PAGE_TBL  pHashPageTbl)
{
    INT  i;

    for (i = 0; i < MMU_SEG_NR; i++) {                                  /*  配置段寄存器                */
        ppcHashMmuSetSR(i, pHashPageTbl->HPT_SR[i].SR_uiValue);
    }

    ppcHashMmuSetSDR1(pHashPageTbl->HPT_uiHashPageTblOrg |
                      pHashPageTbl->HPT_uiHashPageTblMask);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblMakeTrans
** 功能描述: 构建映射关系
** 输　入  : pHashPageTbl          HASH 页表
**           ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
     * 搜索 EA 的 PTE
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

    if (pPte) {                                                         /*  找到了                      */
        if (uiPteValue1) {
            __ppcHashPageTblPteSet(pPte,
                                   pPte->words.PTE_uiWord0,
                                   uiPteValue1,
                                   ulEffectiveAddr);                    /*  改变 PTE                    */
        } else {
            __ppcHashPageTblPteSet(pPte,
                                   0,
                                   0,
                                   ulEffectiveAddr);                    /*  删除 PTE                    */
        }
    } else {                                                            /*  EA 不存在 PTE               */
        if (uiPteValue1) {                                              /*  需要增加 EA 的 PTE          */
            /*
             * 搜索一个无效的 PTE
             */
            pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
            if (!pPte) {
                pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
                if (pPte) {
                    bIsPrimary = LW_FALSE;
                }
            }

            if (pPte) {                                                 /*  找到无效的 PTE              */
                __ppcHashPageTblPteAdd(pPte,
                                       effectiveAddr,
                                       uiPteValue1,
                                       bIsPrimary,
                                       uiAPI,
                                       uiVSID,
                                       LW_FALSE);                       /*  增加一个 PTE                */
            } else {
                /*
                 * 没有无效的 PTE，外部会无效 TLB，不作强制淘汰 PTE，让其产生 MISS
                 */
            }
        } else {
            /*
             * 为取消映射, 外部会无效 TLB
             */
        }
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblFlagSet
** 功能描述: 设置映射标志
** 输　入  : pHashPageTbl          HASH 页表
**           ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块: 
** 说  明  : 如果没有找到，外部会无效 TLB，不作查找无效 PTE 和强制淘汰 PTE，让其产生 MISS
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
     * 搜索 EA 的 PTE
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

    if (pPte) {                                                         /*  找到了                      */
        if (uiPteValue1) {
            __ppcHashPageTblPteSet(pPte,
                                   pPte->words.PTE_uiWord0,
                                   uiPteValue1,
                                   ulEffectiveAddr);                    /*  改变 PTE                    */
        } else {
            __ppcHashPageTblPteSet(pPte,
                                   0,
                                   0,
                                   ulEffectiveAddr);                    /*  删除 PTE                    */
        }
    } else {
        /*
         * 没有找到，外部会无效 TLB
         */
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblPteMissHandle
** 功能描述: 处理 PTE MISS
** 输　入  : pHashPageTbl          HASH 页表
**           ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
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
     * 搜索一个无效的 PTE
     */
    pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
    if (!pPte) {
        pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
        if (pPte) {
            bIsPrimary = LW_FALSE;
        }
    }

    if (!pPte) {                                                        /*  没有无效的 PTE              */
        pPte = __ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                  pSecPteg,
                                                  &bIsPrimary);         /*  强制淘汰一个 PTE            */

        /*
         * 这里并不无效被淘汰的 PTE 对应的 TLB
         * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
         */
    }

    __ppcHashPageTblPteAdd(pPte,
                           effectiveAddr,
                           uiPteValue1,
                           bIsPrimary,
                           uiAPI,
                           uiVSID,
                           LW_FALSE);                                   /*  增加一个 PTE                */

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (LW_VMM_ABORT_TYPE_NOINFO);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblPtePreLoad
** 功能描述: PTE 预加载
** 输　入  : pHashPageTbl          HASH 页表
**           ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
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
     * 搜索 EA 的 PTE
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

    if (!pPte) {                                                        /*  EA 不存在 PTE, 需要增加     */
        /*
         * 搜索一个无效的 PTE
         */
        pPte = __ppcHashPageTblSearchInvalidPte(pPrimPteg);
        if (!pPte) {
            pPte = __ppcHashPageTblSearchInvalidPte(pSecPteg);
            if (pPte) {
                bIsPrimary = LW_FALSE;
            }
        }

        if (!pPte) {                                                    /*  没有无效的 PTE              */
            pPte = __ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                      pSecPteg,
                                                      &bIsPrimary);     /*  强制淘汰一个 PTE            */

            /*
             * 这里并不无效被淘汰的 PTE 对应的 TLB
             * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
             */
        }

        __ppcHashPageTblPteAdd(pPte,
                               effectiveAddr,
                               uiPteValue1,
                               bIsPrimary,
                               uiAPI,
                               uiVSID,
                               LW_TRUE);                                /*  增加一个 PTE(避免立即被淘汰)*/
    }

    LW_SPIN_UNLOCK_QUICK(&pHashPageTbl->HPT_splLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblDump
** 功能描述: 打印 MMU HASH PAGE TABLE 信息
** 输　入  : pHashPageTbl          HASH 页表
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
