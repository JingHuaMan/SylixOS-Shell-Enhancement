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
** 文   件   名: ppcMmuE500.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC E500 体系构架 MMU 驱动.
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
#include "./ppcMmuE500Reg.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                 _G_uiTlb0Size = 0;                          /*  TLB0 数组大小               */
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID    ppcE500MmuInvalidateTLB(VOID);
extern VOID    ppcE500MmuInvalidateTLBEA(addr_t  ulAddr);
/*********************************************************************************************************
** 函数名称: ppcE500MmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuEnable (VOID)
{
    /*
     * E500 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuDisable (VOID)
{
    /*
     * E500 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              二级页表基地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcE500MmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuBuildPteEntry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : paBaseAddr              物理页地址
**           ulFlag                  标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcE500MmuBuildPteEntry (phys_addr_t  paBaseAddr,
                                                   ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   ulDescriptor;
    UINT32              uiRPN;

    ulDescriptor.MAS3_uiValue = 0;
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
    ulDescriptor.MAS7_uiValue = 0;
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/

    uiRPN = paBaseAddr >> MMU_RPN_SHIFT;                                /*  计算 RPN                    */
    ulDescriptor.MAS3_uiRPN = uiRPN & MMU_RPN_MASK;                     /*  填充 RPN                    */

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulDescriptor.MAS3_bValid     = LW_TRUE;                         /*  有效                        */
        ulDescriptor.MAS3_bSuperRead = LW_TRUE;                         /*  特权态可读                  */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {
        ulDescriptor.MAS3_bSuperWrite = LW_TRUE;                        /*  特权态可写                  */
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        ulDescriptor.MAS3_bSuperExec = LW_TRUE;                         /*  特权态可执行                */
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  回写 CACHE                  */
        ulDescriptor.MAS3_bUnCache = LW_FALSE;
        ulDescriptor.MAS3_bWT      = LW_FALSE;
        ulDescriptor.MAS3_bGuarded = LW_FALSE;

        if (MMU_MAS2_M) {
            ulDescriptor.MAS3_bMemCoh = LW_TRUE;                        /*  多核一致性                  */
        }

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  写穿透 CACHE                */
        ulDescriptor.MAS3_bUnCache = LW_FALSE;
        ulDescriptor.MAS3_bWT      = LW_TRUE;
        ulDescriptor.MAS3_bGuarded = LW_FALSE;

        if (MMU_MAS2_M) {
            ulDescriptor.MAS3_bMemCoh = LW_TRUE;                        /*  多核一致性                  */
        }

    } else {                                                            /*  UNCACHE                     */
        ulDescriptor.MAS3_bUnCache = LW_TRUE;
        ulDescriptor.MAS3_bWT      = LW_TRUE;
        ulDescriptor.MAS3_bGuarded = LW_TRUE;                           /*  阻止猜测访问                */
    }

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
    ulDescriptor.MAS7_uiHigh4RPN = uiRPN >> MMU_RPN_BITS;
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/

    ulDescriptor.MAS3_bGlobal = LW_TRUE;                                /*  全局映射                    */

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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
** 函数名称: ppcE500MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuGlobalInit (CPCHAR  pcMachineName)
{
    MMUCFG_REG  uiMMUCFG;
    MAS4_REG    uiMAS4;
    UINT32      uiHID1;

    /*
     * 设置 PID
     */
    uiMMUCFG.MMUCFG_uiValue = ppcE500MmuGetMMUCFG();

    ppcE500MmuSetPID0(0);
    if (uiMMUCFG.MMUCFG_ucNPIDS > 1) {
        ppcE500MmuSetPID1(0);
        if (uiMMUCFG.MMUCFG_ucNPIDS > 2) {
            ppcE500MmuSetPID2(0);
        }
    }

    /*
     * 设置 MAS4
     */
    uiMAS4.MAS4_uiValue   = 0;
    uiMAS4.MAS4_ucTLBSELD = 0;                                          /*  默认选择 TLB0               */
    uiMAS4.MAS4_ucTSIZED  = MMU_TSIZED;                                 /*  默认页面大小                */
    uiMAS4.MAS4_bX0D      = MMU_MAS4_X0D;
    uiMAS4.MAS4_bX1D      = MMU_MAS4_X1D;
    uiMAS4.MAS4_bWD       = LW_TRUE;                                    /*  默认写穿透 CACHE            */
    uiMAS4.MAS4_bID       = LW_TRUE;                                    /*  默认不可 CACHE              */
    uiMAS4.MAS4_bMD       = LW_TRUE;                                    /*  默认一致性                  */
    uiMAS4.MAS4_bGD       = LW_TRUE;                                    /*  默认阻止猜测访问            */
    uiMAS4.MAS4_bED       = LW_FALSE;                                   /*  默认大端                    */

    ppcE500MmuSetMAS4(uiMAS4.MAS4_uiValue);

    /*
     * 使能地址广播
     */
    if (_G_bHasHID1) {
        uiHID1 = ppcE500GetHID1();
        if (MMU_MAS2_M) {
            uiHID1 |=  ARCH_PPC_HID1_ABE;
        } else {
            uiHID1 &= ~ARCH_PPC_HID1_ABE;
        }
        ppcE500SetHID1(uiHID1);
    }

    /*
     * 有 MAS7 寄存器, 则使能 MAS7 寄存器的访问
     */
    if (_G_bHasMAS7) {
        UINT32  uiHID0;

        uiHID0  = ppcE500GetHID0();
        uiHID0 |= ARCH_PPC_HID0_MAS7EN;
        ppcE500SetHID0(uiHID0);
    }

    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 Core0 复位 CACHE         */
        archCacheReset(pcMachineName);                                  /*  复位 CACHE                  */
    }

    ppcE500MmuInvalidateTLB();                                          /*  无效 TLB0                   */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: ppcE500MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC 无 PMD 项           */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppcE500MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
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
** 函数名称: ppcE500MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.MAS3_bValid ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: ppcE500MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PGD_TRANSENTRY  *p_pgdentry,
                                               addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcE500MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PMD_TRANSENTRY  *p_pmdentry,
                                               addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = ppcE500MmuBuildPgdEntry((addr_t)p_pteentry);          /*  设置二级页表描述符          */

    return  (ppcE500MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.MAS3_uiRPN;                             /*  获得物理页面号              */

    *ppaPhysicalAddr = uiRPN << MMU_RPN_SHIFT;                          /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  ppcE500MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址      */

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcE500MmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            ULONG  ulFlag;

            ulFlag = LW_VMM_FLAG_GUARDED;                               /*  进行严格权限检查            */

            if (pteentry.MAS3_bSuperRead) {
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  映射有效                    */
                ulFlag |= LW_VMM_FLAG_ACCESS;                           /*  特权态可读                  */
            }

            if (pteentry.MAS3_bSuperWrite) {
                ulFlag |= LW_VMM_FLAG_WRITABLE;                         /*  特权态可写                  */
            }

            if (pteentry.MAS3_bSuperExec) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  特权态可执行                */
            }

            if (!pteentry.MAS3_bUnCache && !pteentry.MAS3_bWT) {
                ulFlag |= LW_VMM_FLAG_CACHEABLE;

            } else if (!pteentry.MAS3_bUnCache) {
                ulFlag |= LW_VMM_FLAG_WRITETHROUGH;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  ppcE500MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);                  /*  获得一级描述符地址          */

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcE500MmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            UINT32        uiRPN = pteentry.MAS3_uiRPN;                  /*  获得物理页号                */
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
            uiRPN |= pteentry.MAS7_uiHigh4RPN << MMU_RPN_BITS;
#endif                                                                  /*  计算页面物理地址            */
            phys_addr_t   paPhysicalAddr = ((phys_addr_t)uiRPN) << MMU_RPN_SHIFT;

            /*
             * 构建二级描述符并设置二级描述符
             */
            *p_pteentry = ppcE500MmuBuildPteEntry(paPhysicalAddr, ulFlag);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMakeTrans
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
static VOID  ppcE500MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = ppcE500MmuBuildPteEntry(paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcSetSPRG3((addr_t)pmmuctx->MMUCTX_pgdEntry |
                (_G_bHasMAS7 ? 1 : 0));                                 /*  使用SPRG3记录 PgdTbl HasMAS7*/
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (_G_uiTlb0Size >> 1)) {
        ppcE500MmuInvalidateTLB();                                      /*  全部清除 TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            ppcE500MmuSetMAS6(0);                                       /*  地址空间 0, PID=0(全局映射) */
            ppcE500MmuInvalidateTLBEA(ulPageAddr);                      /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuStorageAbortType
** 功能描述: 访问终止类型
** 输　入  : ulAddr        终止地址
**           uiMethod      访问方法(LW_VMM_ABORT_METHOD_XXX)
** 输　出  : 数据访问终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppcE500MmuStorageAbortType (addr_t  ulAddr, UINT  uiMethod)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);                  /*  获得一级描述符地址          */

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  获得二级描述符              */

        if (ppcE500MmuPteIsOk(pteentry)) {                              /*  二级描述符有效              */
            if (uiMethod == LW_VMM_ABORT_METHOD_READ) {
                if (!pteentry.MAS3_bSuperRead) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }

            } else if (uiMethod == LW_VMM_ABORT_METHOD_WRITE) {
                if (!pteentry.MAS3_bSuperWrite) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }

            } else if (uiMethod == LW_VMM_ABORT_METHOD_EXEC) {
                if (!pteentry.MAS3_bSuperExec) {
                    return  (LW_VMM_ABORT_TYPE_PERM);
                }
            }
        }
    }

    return  (LW_VMM_ABORT_TYPE_MAP);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcE500MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    TLBCFG_REG  uiTLB0CFG;

    /*
     * 使能了地址广播(HID1[ABE] 位)后, tlbsync 指令会自动多核同步
     */
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit       = ppcE500MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcE500MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = ppcE500MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcE500MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcE500MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcE500MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcE500MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcE500MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcE500MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = ppcE500MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcE500MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcE500MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcE500MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = ppcE500MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcE500MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = ppcE500MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcE500MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcE500MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcE500MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcE500MmuDisable;

    /*
     * 多核一致性须使能 HID1[ABE] 位
     */
    MMU_MAS2_M = (LW_NCPUS > 1) ? 1 : 0;                                /*  多核一致性位设置            */

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * 获得 TLB0 条目数
     */
    uiTLB0CFG.TLBCFG_uiValue = ppcE500MmuGetTLB0CFG();
    _G_uiTlb0Size = uiTLB0CFG.TLBCFG_usNENTRY;
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB0 size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, _G_uiTlb0Size);

    _BugFormat(MMU_TSIZED > uiTLB0CFG.TLBCFG_ucMAXSIZE, LW_TRUE,
               "MMU_TSIZED MUST <= %d!\r\n", uiTLB0CFG.TLBCFG_ucMAXSIZE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
