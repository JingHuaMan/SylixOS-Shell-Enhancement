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
** 文   件   名: ppcMmu460.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2019 年 08 月 14 日
**
** 描        述: PowerPC 460 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/ppcSpr.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmu460Reg.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
       UINT                 ppc460MmuTblIndex = PPC460_TLB_BASE;        /*  TLB 重填索引号              */
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppc460MmuInvalidateTLBEA(addr_t  ulAddr, UINT  uiPid);
/*********************************************************************************************************
** 函数名称: ppc460MmuInvalidateTLB
** 功能描述: 无效所有 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppc460MmuInvalidateTLB (VOID)
{
    ULONG   i;

    for (i = PPC460_TLB_BASE; i < PPC460_TLB_SIZE; i++) {
        __asm__ __volatile__(
            "sync\n"
            "tlbwe  %0, %0, 0\n"
            "isync\n"
            :
            : "r" (i));
    }
}
/*********************************************************************************************************
** 函数名称: ppc460MmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppc460MmuEnable (VOID)
{
    /*
     * PPC460 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppc460MmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppc460MmuDisable (VOID)
{
    /*
     * PPC460 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppc460MmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              二级页表基地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppc460MmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: ppc460MmuBuildPteEntry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : paBaseAddr              物理页地址
**           ulFlag                  标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppc460MmuBuildPteEntry (phys_addr_t  paBaseAddr,
                                                  ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   ulDescriptor;
    UINT32              uiRPN;

    ulDescriptor.WORD_uiValue = 0;

    uiRPN = paBaseAddr >> MMU_RPN_SHIFT;                                /*  计算 RPN                    */
    ulDescriptor.WORD1_uiRPN = uiRPN & MMU_RPN_MASK;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulDescriptor.WORD0_bValid     = LW_TRUE;                        /*  有效                        */
        ulDescriptor.WORD2_bSuperRead = LW_TRUE;                        /*  特权态可读                  */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {
        ulDescriptor.WORD2_bSuperWrite = LW_TRUE;                       /*  特权态可写                  */
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        ulDescriptor.WORD2_bSuperExec = LW_TRUE;                        /*  特权态可执行                */
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  回写 CACHE                  */
        ulDescriptor.WORD2_bUnCache = LW_FALSE;
        ulDescriptor.WORD2_bWT      = LW_FALSE;
        ulDescriptor.WORD2_bGuarded = LW_FALSE;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  写穿透 CACHE                */
        ulDescriptor.WORD2_bUnCache = LW_FALSE;
        ulDescriptor.WORD2_bWT      = LW_TRUE;
        ulDescriptor.WORD2_bGuarded = LW_FALSE;

    } else {                                                            /*  UNCACHE                     */
        ulDescriptor.WORD2_bUnCache = LW_TRUE;
        ulDescriptor.WORD2_bWT      = LW_TRUE;
        ulDescriptor.WORD2_bGuarded = LW_TRUE;                          /*  阻止猜测访问                */
    }

    ulDescriptor.WORD2_bGlobal = LW_TRUE;                               /*  全局映射                    */

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppc460MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppc460MmuGlobalInit (CPCHAR  pcMachineName)
{
    ppc460MmuSetPID(0);                                                 /*  设置 PID:0                  */
    ppc460MmuSetMMUCR(0);                                               /*  设置 MMUCR(STID:0, STS:0,   */
                                                                        /*  SWOA:0 写分配)              */
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 Core0 复位 CACHE         */
        archCacheReset(pcMachineName);                                  /*  复位 CACHE                  */
    }

    ppc460MmuInvalidateTLB();                                           /*  无效 TLB                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppc460MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: ppc460MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppc460MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC 无 PMD 项           */
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppc460MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
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
** 函数名称: ppc460MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppc460MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppc460MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.WORD0_bValid ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppc460MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: ppc460MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppc460MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppc460MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PGD_TRANSENTRY  *p_pgdentry,
                                              addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppc460MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppc460MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = ppc460MmuBuildPgdEntry((addr_t)p_pteentry);          /*  设置二级页表描述符          */

    return  (ppc460MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppc460MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppc460MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.WORD1_uiRPN;                            /*  获得物理页面号              */

    *ppaPhysicalAddr = uiRPN << MMU_RPN_SHIFT;                          /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  ppc460MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);   /*  获得一级描述符地址      */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  二级描述符有效              */
            ULONG  ulFlag;

            ulFlag = LW_VMM_FLAG_GUARDED;                               /*  进行严格权限检查            */

            if (pteentry.WORD2_bSuperRead) {
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  映射有效                    */
                ulFlag |= LW_VMM_FLAG_ACCESS;                           /*  特权态可读                  */
            }

            if (pteentry.WORD2_bSuperWrite) {
                ulFlag |= LW_VMM_FLAG_WRITABLE;                         /*  特权态可写                  */
            }

            if (pteentry.WORD2_bSuperExec) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  特权态可执行                */
            }

            if (!pteentry.WORD2_bUnCache && !pteentry.WORD2_bWT) {
                ulFlag |= LW_VMM_FLAG_CACHEABLE;

            } else if (!pteentry.WORD2_bUnCache) {
                ulFlag |= LW_VMM_FLAG_WRITETHROUGH;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  ppc460MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);                   /*  获得一级描述符地址          */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  二级描述符有效              */
            UINT32        uiRPN = pteentry.WORD1_uiRPN;                 /*  获得物理页号                */
            phys_addr_t   paPhysicalAddr = ((phys_addr_t)uiRPN) << MMU_RPN_SHIFT;

            /*
             * 构建二级描述符并设置二级描述符
             */
            *p_pteentry = ppc460MmuBuildPteEntry(paPhysicalAddr, ulFlag);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuMakeTrans
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
static VOID  ppc460MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = ppc460MmuBuildPteEntry(paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppc460MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcSetSPRG3((addr_t)pmmuctx->MMUCTX_pgdEntry);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppc460MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (PPC460_TLB_SIZE >> 1)) {
        ppc460MmuInvalidateTLB();                                       /*  全部清除 TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            ppc460MmuInvalidateTLBEA(ulPageAddr, 0);                    /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ppc460MmuStorageAbortType
** 功能描述: 访问终止类型
** 输　入  : ulAddr        终止地址
**           uiMethod      访问方法(LW_VMM_ABORT_METHOD_XXX)
** 输　出  : 数据访问终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppc460MmuStorageAbortType (addr_t  ulAddr, UINT  uiMethod)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);                   /*  获得一级描述符地址          */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */
        LW_PTE_TRANSENTRY *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY  pteentry = *p_pteentry;                      /*  获得二级描述符              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  二级描述符有效              */
            if (uiMethod == LW_VMM_ABORT_METHOD_READ) {
                if (!pteentry.WORD2_bSuperRead) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }

            } else if (uiMethod == LW_VMM_ABORT_METHOD_WRITE) {
                if (!pteentry.WORD2_bSuperWrite) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }

            } else if (uiMethod == LW_VMM_ABORT_METHOD_EXEC) {
                if (!pteentry.WORD2_bSuperExec) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }
            }
        }
    }

    return  (LW_VMM_ABORT_TYPE_MAP);
}
/*********************************************************************************************************
** 函数名称: ppc460MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppc460MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit       = ppc460MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppc460MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = ppc460MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppc460MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppc460MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppc460MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppc460MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppc460MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = ppc460MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppc460MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppc460MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = ppc460MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppc460MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppc460MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = ppc460MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = ppc460MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppc460MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = ppc460MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppc460MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppc460MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppc460MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppc460MmuDisable;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, PPC460_TLB_SIZE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
