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
** 文   件   名: ppcMmuHash.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 14 日
**
** 描        述: PowerPC 体系构架 HASH 页表 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                 _G_uiTlbSize;                               /*  TLB 数组大小                */
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
static PHASH_PAGE_TBL       _G_pHashPageTbl;                            /*  HASH 页表                   */
/*********************************************************************************************************
  访问权限 PP
*********************************************************************************************************/
#define PP_NA               (0)                                         /*  无访问权限                  */
#define PP_RO               (1)                                         /*  只读                        */
#define PP_RW               (2)                                         /*  可读写                      */
/*********************************************************************************************************
  WIMG
*********************************************************************************************************/
#define G_BIT               (1 << 0)                                    /*  阻止猜测访问                */
#define M_BIT               (1 << 1)                                    /*  多核一致性                  */
#define I_BIT               (1 << 2)                                    /*  不可以 CACHE                */
#define W_BIT               (1 << 3)                                    /*  写穿透 CACHE                */

#define WI_MASK             (W_BIT | I_BIT)
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppcHashMmuInvalidateTLBNr(UINT  uiTlbNr);
extern VOID  ppcHashMmuInvalidateTLBEA(addr_t  uiEffectiveAddr);
extern VOID  ppcHashMmuEnable(VOID);
extern VOID  ppcHashMmuDisable(VOID);
/*********************************************************************************************************
** 函数名称: ppcHashMmuInvalidateTLBNr
** 功能描述: 无效所有的 TBL
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  VOID  ppcHashMmuInvalidateTLB (VOID)
{
    ppcHashMmuInvalidateTLBNr(_G_uiTlbSize >> 1);                       /*  两路组相连                  */
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              二级页表基地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcHashMmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuBuildPteEntry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : paBaseAddr              物理页地址
**           ulAddr                  虚拟页地址
**           ulFlag                  标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcHashMmuBuildPteEntry (phys_addr_t  paBaseAddr,
                                                   addr_t       ulAddr,
                                                   ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   ulDescriptor;
    UINT8               ucWIMG;
    UINT8               ucPP;
    ULONG               ulSegmentFlag;

    ulDescriptor.PTE_uiValue = 0;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulDescriptor.PTE_bValid = LW_TRUE;

        ulSegmentFlag = ppcHashPageTblSegmentFlag(_G_pHashPageTbl, ulAddr);
        if (!(ulSegmentFlag & LW_VMM_FLAG_EXECABLE)) {                  /*  该段不可执行                */
            if (ulFlag & LW_VMM_FLAG_EXECABLE) {                        /*  映射却可执行                */
                _BugFormat(LW_TRUE, LW_TRUE,
                           "Segment %d can't execable! Please check your _G_HashMmuSR!\r\n",
                           ulAddr >> MMU_EA_SR_SHIFT);
            }
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            ucPP = PP_RW;                                               /*  可读写                      */

        } else {
            ucPP = PP_RO;                                               /*  只读                        */
        }

    } else{
        ucPP = PP_NA;                                                   /*  无访问权限                  */
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  回写 CACHE                  */
        ucWIMG = M_BIT;                                                 /*  多核一致性                  */

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  写穿透 CACHE                */
        ucWIMG = M_BIT | W_BIT;                                         /*  多核一致性 | 写穿透 CACHE   */

    } else {
        ucWIMG = I_BIT | G_BIT;                                         /*  不可以 CACHE | 阻止猜测访问 */
    }

    /*
     * 这里把 R 和 C 位设置为 0，因为设置完 PTE word1 后，会无效 TLB，
     * 所以不用担心 PTE 与对应的 TLB 不一致的问题
     */
    ulDescriptor.PTE_bRef        = 0;
    ulDescriptor.PTE_bChange     = 0;
    ulDescriptor.PTE_ucPP        = ucPP;
    ulDescriptor.PTE_ucWIMG      = ucWIMG;
    ulDescriptor.PTE_uiRPN       = paBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcHashMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (1024 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  (((4 * LW_CFG_MB_SIZE) / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))
    PVOID   pvPgdTable;
    PVOID   pvPteTable;

    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);

    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory, can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);

    if (!_G_hPGDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory, can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    _G_pHashPageTbl = ppcHashPageTblMemInit(pmmuctx, MMU_PTE_MIN_SIZE_128M);
    if (_G_pHashPageTbl == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory, can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcHashMmuGlobalInit (CPCHAR  pcMachineName)
{
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 CPU 0 复位 CACHE         */
        archCacheReset(pcMachineName);                                  /*  复位 CACHE                  */
    }

    ppcHashPageTblGlobalInit(pcMachineName);

    ppcHashMmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcHashMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcHashMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC 无 PMD 项           */
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppcHashMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  获得一级页表描述符          */

    p_pteentry = (LW_PTE_TRANSENTRY *)ulTemp;                           /*  获得二级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;                                   /*  不要使用LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  ppcHashMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  ppcHashMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.PTE_bValid ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcHashMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcHashMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PGD_TRANSENTRY  *p_pgdentry,
                                               addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量:
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcHashMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PMD_TRANSENTRY  *p_pmdentry,
                                               addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = ppcHashMmuBuildPgdEntry((addr_t)p_pteentry);          /*  设置二级页表描述符          */

    return  (ppcHashMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcHashMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.PTE_uiRPN;                              /*  获得物理页面号              */

    *ppaPhysicalAddr = uiRPN << LW_CFG_VMM_PAGE_SHIFT;                  /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  ppcHashMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址      */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            ULONG  ulFlag;
            ULONG  ulSegmentFlag;

            ulFlag = LW_VMM_FLAG_GUARDED;                               /*  进行严格权限检查            */

            switch (p_pteentry->PTE_ucPP) {

            case PP_RO:                                                 /*  只读                        */
                ulFlag |= LW_VMM_FLAG_VALID | LW_VMM_FLAG_ACCESS;
                break;

            case PP_RW:                                                 /*  可读写                      */
                ulFlag |= LW_VMM_FLAG_VALID | LW_VMM_FLAG_ACCESS | LW_VMM_FLAG_WRITABLE;
                break;

            case PP_NA:                                                 /*  无访问权限                  */
            default:
                break;
            }

            ulSegmentFlag = ppcHashPageTblSegmentFlag(_G_pHashPageTbl, ulAddr);
            if ((ulSegmentFlag & LW_VMM_FLAG_EXECABLE) &&               /*  该段可执行                  */
                (ulFlag & LW_VMM_FLAG_ACCESS)) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;
            }

            if (p_pteentry->PTE_ucWIMG == M_BIT) {
                ulFlag |= LW_VMM_FLAG_CACHEABLE;                        /*  回写 CACHE                  */

            } else if (p_pteentry->PTE_ucWIMG == (M_BIT | W_BIT)) {
                ulFlag |= LW_VMM_FLAG_WRITETHROUGH;                     /*  写穿透 CACHE                */
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  ppcHashMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  获得一级描述符地址          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            addr_t   ulPhysicalAddr = (addr_t)(p_pteentry->PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);

            *p_pteentry = ppcHashMmuBuildPteEntry(ulPhysicalAddr, ulAddr, ulFlag);

            return  (ppcHashPageTblFlagSet(_G_pHashPageTbl,
                                           ulAddr,
                                           p_pteentry->PTE_uiValue));
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuMakeTrans
** 功能描述: 设置页面映射关系
** 输　入  : pmmuctx        mmu 上下文
**           p_pteentry     对应的页表项
**           ulVirtualAddr  虚拟地址
**           paPhysicalAddr 物理地址
**           ulFlag         对应的类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static VOID  ppcHashMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                  LW_PTE_TRANSENTRY  *p_pteentry,
                                  addr_t              ulVirtualAddr,
                                  phys_addr_t         paPhysicalAddr,
                                  ULONG               ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return;
    }

    /*
     * 构建二级描述符并设置二级描述符
     */
    *p_pteentry = ppcHashMmuBuildPteEntry(paPhysicalAddr, ulVirtualAddr, ulFlag);

    ppcHashPageTblMakeTrans(_G_pHashPageTbl,
                            ulVirtualAddr,
                            p_pteentry->PTE_uiValue);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcHashPageTblMakeCurCtx(_G_pHashPageTbl);
    ppcHashMmuInvalidateTLB();                                          /*  全部清除 TLB                */
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (_G_uiTlbSize >> 1)) {
        ppcHashMmuInvalidateTLB();                                      /*  全部清除 TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            ppcHashMmuInvalidateTLBEA((UINT32)ulPageAddr);              /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPteMissHandle
** 功能描述: 处理 PTE 匹配失败异常
** 输　入  : ulAddr        异常处理
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppcHashMmuPteMissHandle (addr_t  ulAddr)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  获得一级描述符地址          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            return  (ppcHashPageTblPteMissHandle(_G_pHashPageTbl,
                                                 ulAddr,
                                                 p_pteentry->PTE_uiValue));
        }
    }

    return  (LW_VMM_ABORT_TYPE_MAP);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuPtePreLoad
** 功能描述: PTE 预加载
** 输　入  : ulAddr        数据访问地址
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcHashMmuPtePreLoad (addr_t  ulAddr)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  获得一级描述符地址          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            return  (ppcHashPageTblPtePreLoad(_G_pHashPageTbl,
                                              ulAddr,
                                              p_pteentry->PTE_uiValue));
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: bspMmuTlbSize
** 功能描述: 获得 TLB 的数目
** 输　入  : NONE
** 输　出  : TLB 的数目
** 全局变量:
** 调用模块:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuTlbSize (VOID)
{
    /*
     * 128 适合 750/E600/745x 机器
     * 64  适合 603e/E300(MPC82XX/MPC83XX) 机器
     */
    return  (128);
}
/*********************************************************************************************************
** 函数名称: ppcHashMmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcHashMmuInit (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    _G_uiTlbSize = bspMmuTlbSize();                                     /*  获得 TLB 的数目             */

    pmmuop->MMUOP_ulOption = 0ul;                                       /*  tlbsync 指令会自动多核同步  */

    pmmuop->MMUOP_pfuncMemInit       = ppcHashMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcHashMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc      = ppcHashMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcHashMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcHashMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcHashMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcHashMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcHashMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk       = ppcHashMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcHashMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcHashMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset     = ppcHashMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcHashMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcHashMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcHashMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet       = ppcHashMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcHashMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = ppcHashMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcHashMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcHashMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcHashMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcHashMmuDisable;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, _G_uiTlbSize);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
