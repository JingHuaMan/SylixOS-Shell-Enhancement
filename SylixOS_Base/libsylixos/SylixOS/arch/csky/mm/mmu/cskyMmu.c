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
** 文   件   名: cskyMmu.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY 体系架构
*********************************************************************************************************/
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "cskyMmu.h"
#include "arch/csky/inc/cskyregs.h"
#include "arch/csky/param/cskyParam.h"
/*********************************************************************************************************
  全局宏定义
*********************************************************************************************************/
#define CSKY_MMU_TLB_SIZE               _G_ulMmuTlbSize                 /*  TLB 数组大小                */
/*********************************************************************************************************
  ENTRYLO PFN 掩码
*********************************************************************************************************/
#define CSKY_ENTRYLO_PFN_MASK           (0xfffff << CSKY_ENTRYLO_PFN_SHIFT)
/*********************************************************************************************************
  UNIQUE ENTRYHI
*********************************************************************************************************/
#define SSEG0                           0x80000000
#define CSKY_UNIQUE_ENTRYHI(idx)        (SSEG0 + ((idx) << (LW_CFG_VMM_PAGE_SHIFT + 1)))
/*********************************************************************************************************
  PAGE 掩码
*********************************************************************************************************/
#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_64K
#else
#error "LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!"
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */
/*********************************************************************************************************
  ENTRYLO 相关定义
*********************************************************************************************************/
#define ENTRYLO_B                       (1 << 6)                        /*  可写缓冲                    */
#define ENTRYLO_SO                      (1 << 5)                        /*  Strong Order                */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static BOOL                 _G_bMmuEnByBoot  = LW_TRUE;                 /*  BOOT 是否已经启动了 MMU     */
static ULONG                _G_ulMmuTlbSize  = 128;                     /*  TLB 数组大小                */
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition = LW_HANDLE_INVALID;       /*  PTE 缓冲区                  */
/*********************************************************************************************************
  外部函数
*********************************************************************************************************/
extern VOID  cskyMmuEnableHw(VOID);
extern VOID  cskyMmuDisableHw(VOID);
extern VOID  cskyMmuContextSet(ULONG  ulValue);
extern VOID  cskyMmuPageMaskSet(ULONG  ulValue);
extern INT   cskyCacheFlush(LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes);
/*********************************************************************************************************
** 函数名称: cskyMmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  cskyMmuEnable (VOID)
{
    if (!_G_bMmuEnByBoot) {
        cskyMmuEnableHw();
    }
}
/*********************************************************************************************************
** 函数名称: cskyMmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  cskyMmuDisable (VOID)
{
    if (!_G_bMmuEnByBoot) {
        cskyMmuDisableHw();
    }
}
/*********************************************************************************************************
** 函数名称: cskyMmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              二级页表基地址
** 输　出  : 一级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  cskyMmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    if (_G_bMmuEnByBoot) {
        return  (CSKY_SSEG0_PA(ulBaseAddr));                            /*  一级描述符就是二级页表基地址*/
    } else {
        return  (ulBaseAddr);
    }
}
/*********************************************************************************************************
** 函数名称: cskyMmuBuildPteEntry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : ulBaseAddr              物理页地址
**           ulFlag                  标志
** 输　出  : 二级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  cskyMmuBuildPteEntry (addr_t  ulBaseAddr, ULONG  ulFlag)
{
    LW_PTE_TRANSENTRY   pteentry;
    ULONG               ulPFN;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulPFN = ulBaseAddr >> 12;                                       /*  计算 PFN                    */

        pteentry = ulPFN << CSKY_ENTRYLO_PFN_SHIFT;                     /*  填充 PFN                    */

        pteentry |= ENTRYLO_V;                                          /*  有效                        */

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {                            /*  可以写                      */
            pteentry |= ENTRYLO_D;
        }

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
            pteentry |= ENTRYLO_C;                                      /*  可以 CACHE                  */
            pteentry |= ENTRYLO_B;                                      /*  可以写缓冲                  */

        } else if (ulFlag & LW_VMM_FLAG_WRITECOMBINING) {
            pteentry |= ENTRYLO_B;                                      /*  可以写缓冲                  */
        }

        if (!(ulFlag & (LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_WRITETHROUGH))) {
            pteentry |= ENTRYLO_SO;                                     /*  Strong Order                */
        }

    } else {
        pteentry = 0;
    }

    return  (pteentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (1024 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  ((4 * LW_CFG_MB_SIZE / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))

    PVOID   pvPgdTable;
    PVOID   pvPteTable;

    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);

    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);

    if (!_G_hPGDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *cskyMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *cskyMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  无 PMD 项                   */
}
/*********************************************************************************************************
** 函数名称: cskyMmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *cskyMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   pmdentry;
    REGISTER ULONG               ulPageNum;

    if (_G_bMmuEnByBoot) {
        pmdentry = CSKY_SSEG0_VA(*p_pmdentry);                          /*  获得一级页表描述符          */
    } else {
        pmdentry = (*p_pmdentry);
    }

    p_pteentry = (LW_PTE_TRANSENTRY *)(pmdentry);                       /*  获得二级页表基地址          */

    ulAddr    &= ~LW_CFG_VMM_PGD_MASK;
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry +
                  (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  cskyMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  cskyMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & ENTRYLO_V) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *cskyMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *cskyMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PGD_TRANSENTRY  *p_pgdentry,
                                           addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: cskyMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量:
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *cskyMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                            LW_PMD_TRANSENTRY  *p_pmdentry,
                                            addr_t              ulAddr)
{
#if (LW_CFG_CACHE_EN > 0) && (LW_CFG_CSKY_HARD_TLB_REFILL > 0)
    INTREG              iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)cskyMmuBuildPgdEntry((addr_t)p_pteentry);  /*  设置二级页表基地址  */

#if (LW_CFG_CACHE_EN > 0) && (LW_CFG_CSKY_HARD_TLB_REFILL > 0)
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    cskyCacheFlush(DATA_CACHE, p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (cskyMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: cskyMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: cskyMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    ULONG  ulPFN = (pteentry & CSKY_ENTRYLO_PFN_MASK) >>
                    CSKY_ENTRYLO_PFN_SHIFT;                             /*  获得物理页面号              */

    *ppaPhysicalAddr = ulPFN << 12;                                     /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  cskyMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    p_pgdentry = cskyMmuPgdOffset(pmmuctx, ulAddr);                     /*  获得一级描述符地址          */
    if (cskyMmuPgdIsOk(*p_pgdentry)) {                                  /*  一级描述符有效              */

        p_pteentry = cskyMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                      ulAddr);                          /*  获得二级描述符地址          */
        pteentry = *p_pteentry;                                         /*  获得二级描述符              */
        if (cskyMmuPteIsOk(pteentry)) {                                 /*  二级描述符有效              */
            ULONG   ulFlag;

            ulFlag  = LW_VMM_FLAG_VALID;                                /*  映射有效                    */
            ulFlag |= LW_VMM_FLAG_GUARDED;                              /*  进行严格权限检查            */
            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  可以访问                    */
            ulFlag |= LW_VMM_FLAG_EXECABLE;                             /*  可以执行                    */

            if (pteentry & ENTRYLO_D) {
                ulFlag |= LW_VMM_FLAG_WRITABLE;                         /*  可以写                      */
            }
           
            if (pteentry & ENTRYLO_C) {
                ulFlag |= LW_VMM_FLAG_CACHEABLE;                        /*  可以 CACHE                  */

            } else if (pteentry & ENTRYLO_B) {
                ulFlag |= LW_VMM_FLAG_WRITECOMBINING;                   /*  写合并                      */
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: cskyMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  cskyMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    p_pgdentry = cskyMmuPgdOffset(pmmuctx, ulAddr);                     /*  获得一级描述符地址          */
    if (cskyMmuPgdIsOk(*p_pgdentry)) {                                  /*  一级描述符有效              */

        p_pteentry = cskyMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                      ulAddr);                          /*  获得二级描述符地址          */
        pteentry= *p_pteentry;                                          /*  获得二级描述符              */
        if (cskyMmuPteIsOk(pteentry)) {                                 /*  二级描述符有效              */
            ULONG   ulPFN = (pteentry & CSKY_ENTRYLO_PFN_MASK) >>
                             CSKY_ENTRYLO_PFN_SHIFT;                    /*  获得物理页面号              */
            addr_t  ulPhysicalAddr = ulPFN << 12;                       /*  计算页面物理地址            */

            *p_pteentry = cskyMmuBuildPteEntry(ulPhysicalAddr, ulFlag);
#if (LW_CFG_CACHE_EN > 0) && (LW_CFG_CSKY_HARD_TLB_REFILL > 0)
            cskyCacheFlush(DATA_CACHE, p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: cskyMmuMakeTrans
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
static VOID  cskyMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = cskyMmuBuildPteEntry(paPhysicalAddr, ulFlag);
#if (LW_CFG_CACHE_EN > 0) && (LW_CFG_CSKY_HARD_TLB_REFILL > 0)
    cskyCacheFlush(DATA_CACHE, p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: cskyMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    if (_G_bMmuEnByBoot) {
        cskyMmuContextSet(CSKY_SSEG0_PA((addr_t)pmmuctx->MMUCTX_pgdEntry));
    } else {
        cskyMmuContextSet((addr_t)pmmuctx->MMUCTX_pgdEntry);
    }
}
/*********************************************************************************************************
** 函数名称: cskyMmuInvalidateTLB
** 功能描述: 无效 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 内部会无效 Micro TLB
*********************************************************************************************************/
VOID  cskyMmuInvalidateTLB (VOID)
{
    ULONG   ulEntryHiBak = cskyEntryHiRead();
    INT     i;

    for (i = 0; i < CSKY_MMU_TLB_SIZE; i++) {
        cskyIndexWrite(i);
        cskyEntryLo0Write(0);
        cskyEntryLo1Write(0);
        cskyEntryHiWrite(CSKY_UNIQUE_ENTRYHI(i));
        cskyTlbWriteIndexed();
    }

    cskyEntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: cskyMmuInvalidateTLBMVA
** 功能描述: 无效指定 MVA 的 TLB
** 输　入  : ulAddr            指定 MVA
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 内部不会无效 Micro TLB, 外部操作完成后必须无效 Micro TLB
*********************************************************************************************************/
VOID  cskyMmuInvalidateTLBMVA (addr_t  ulAddr)
{
    ULONG   ulEntryHiBak = cskyEntryHiRead();
    ULONG   ulEntryHi    = ulAddr & (LW_CFG_VMM_PAGE_MASK << 1);
    INT32   iIndex;
    INT     iReTry;

    for (iReTry = 0; iReTry < 2; iReTry++) {                            /*  不会出现两条一样的 TLB 条目 */
        cskyEntryHiWrite(ulEntryHi);
        cskyTlbProbe();
        iIndex = cskyIndexRead();
        if (iIndex >= 0) {
            cskyIndexWrite(iIndex);
            cskyEntryLo0Write(0);
            cskyEntryLo1Write(0);
            cskyEntryHiWrite(CSKY_UNIQUE_ENTRYHI(iIndex));
            cskyTlbWriteIndexed();

        } else {
            break;
        }
    }

    cskyEntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: cskyMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    REGISTER ULONG   i;

    if (ulPageNum > (CSKY_MMU_TLB_SIZE >> 1)) {
        cskyMmuInvalidateTLB();                                         /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            cskyMmuInvalidateTLBMVA(ulPageAddr);                        /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: cskyMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  cskyMmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);                                      /*  复位 CACHE                  */

    cskyMmuPageMaskSet(CSKY_MMU_PAGE_MASK);                             /*  PAGE MASK                   */

    cskyEntryHiWrite(0);                                                /*  ASID = 0                    */

    cskyMmuInvalidateTLB();                                             /*  无效 TLB                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cskyMmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  cskyMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    CSKY_PARAM  *param;

    param = archKernelParamGet();
    _G_bMmuEnByBoot = param->CP_bMmuEnByBoot;

#if LW_CFG_SMP_EN > 0
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;
#else
    pmmuop->MMUOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    pmmuop->MMUOP_pfuncMemInit    = cskyMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = cskyMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc   = cskyMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree    = cskyMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc   = cskyMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree    = cskyMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc   = cskyMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree    = cskyMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk    = cskyMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk    = cskyMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk    = cskyMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset  = cskyMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset  = cskyMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset  = cskyMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet = cskyMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet    = cskyMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet    = cskyMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = cskyMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = cskyMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = cskyMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = cskyMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = cskyMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  !__SYLIXOS_CSKY_ARCH_CK803__*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
