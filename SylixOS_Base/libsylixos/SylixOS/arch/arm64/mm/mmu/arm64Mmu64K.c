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
** 文   件   名: arm64Mmu64K.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 11 月 01 日
**
** 描        述: ARM64 体系构架 MMU 驱动 (64K 页大小).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_ARM64_PAGE_SHIFT == 16
#include "../../param/arm64Param.h"
#include "../cache/arm64Cache.h"
#include "arm64Mmu.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID    arm64MmuEnable(VOID);
extern VOID    arm64MmuDisable(VOID);
extern VOID    arm64MmuInvalidateTLB(VOID);
extern VOID    arm64MmuSetMAIR(VOID);
extern VOID    arm64MmuSetTCR(UINT64  ulTcr);
extern UINT64  arm64MmuGetMAIR(VOID);
extern VOID    arm64MmuSetTTBR(PVOID  pvAddr);
extern VOID    arm64MmuInvalidateTLBMVA(PVOID  pvAddr);
extern ULONG   arm64MmuAbtFaultAddr(VOID);
extern VOID    arm64DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
/*********************************************************************************************************
  PGD 中的位定义
*********************************************************************************************************/
#define ARM64_PGD_TYPE_MASK     (0x3 << 0)                              /*  PGD 中的类型掩码            */
#define ARM64_PGD_TYPE_FAULT    (0x0 << 0)                              /*  PGD 类型无效                */
#define ARM64_PGD_TYPE_TABLE    (0x3 << 0)                              /*  PGD 为 Table 类型           */
#define ARM64_PGD_TYPE_BLOCK    (0x1 << 0)                              /*  PGD 为 Block 类型           */
/*********************************************************************************************************
  PMD 中的位定义
*********************************************************************************************************/
#define ARM64_PMD_TYPE_MASK     (0x3 << 0)                              /*  PMD 中的类型掩码            */
#define ARM64_PMD_TYPE_FAULT    (0x0 << 0)                              /*  PMD 类型无效                */
#define ARM64_PMD_TYPE_TABLE    (0x3 << 0)                              /*  PMD 为 Table 类型           */
#define ARM64_PMD_TYPE_BLOCK    (0x1 << 0)                              /*  PMD 为 Block 类型           */
/*********************************************************************************************************
  PTE 中的位定义
*********************************************************************************************************/
#define ARM64_PTE_TYPE_MASK     (0x3 << 0)                              /*  PTE 中的类型掩码            */
#define ARM64_PTE_TYPE_FAULT    (0x0 << 0)                              /*  PTE 类型无效                */
#define ARM64_PTE_TYPE_PAGE     (0x3 << 0)                              /*  PTE 类型为 PAGE             */
/*********************************************************************************************************
  PGD、PMD、PTS、PTE中的属性定义
  
  PGD、PMD、PTS 格式，[58:55] Reserved for software use
       63      62        61     60          59     58     55
  +---------+--------------+-----------+----------+---------+
  | NSTable |    APTable   |  XNTable  | PXNTable |  Check  |
  +---------+--------------+-----------+----------+---------+
*********************************************************************************************************/
#define ARM64_MMU_NS_SHIFT      (63)                                    /*  PGD、PMD、PTS 中的 Secure & */
#define ARM64_MMU_NS_MASK       (0x1 << ARM64_MMU_NS_SHIFT)             /*  Non-Secure 标志             */
#define ARM64_MMU_AP_SHIFT      (61)                                    /*  PGD、PMD、PTS 中的 Access   */
#define ARM64_MMU_AP_MASK       (0x3 << ARM64_MMU_AP_SHIFT)             /*  permissions 标志            */
#define ARM64_MMU_XN_SHIFT      (60)
#define ARM64_MMU_XN_MASK       (0x1 << ARM64_MMU_XN_SHIFT)             /*  PGD、PMD、PTS 中的 XN       */
#define ARM64_MMU_PXN_SHIFT     (59)
#define ARM64_MMU_PXN_MASK      (0x1 << ARM64_MMU_PXN_SHIFT)            /*  PGD、PMD、PTS 中的 PXN      */

#define ARM64_PTE_GUARD_SHIFT   (55)
#define ARM64_PTE_GUARD_MASK    (1UL << ARM64_PTE_GUARD_SHIFT)          /*  用于记录 GUARD 标志         */
#define ARM64_PTE_UXN_SHIFT     (54)
#define ARM64_PTE_UXN_MASK      (1UL << ARM64_PTE_UXN_SHIFT)            /*  User XN                     */
#define ARM64_PTE_PXN_SHIFT     (53)
#define ARM64_PTE_PXN_MASK      (1UL << ARM64_PTE_PXN_SHIFT)            /*  Privileged XN               */
#define ARM64_PTE_CONT_SHIFT    (52)
#define ARM64_PTE_CONT_MASK     (1UL << ARM64_PTE_CONT_SHIFT)           /*  Contiguous range            */
#define ARM64_PTE_NG_SHIFT      (11)
#define ARM64_PTE_NG_MASK       (0x1 << ARM64_PTE_NG_SHIFT)             /*  PTE 中的 nG 标志            */
#define ARM64_PTE_AF_SHIFT      (10)
#define ARM64_PTE_AF_MASK       (0x1 << ARM64_PTE_AF_SHIFT)             /*  PTE 中的访问标志            */
#define ARM64_PTE_SH_SHIFT      (8)
#define ARM64_PTE_SH_MASK       (0x3 << ARM64_PTE_SH_SHIFT)             /*  PTE 中的共享权限掩码        */
#define ARM64_PTE_AP_SHIFT      (6)
#define ARM64_PTE_AP_MASK       (0x3 << ARM64_PTE_AP_SHIFT)             /*  PTE 中的访问权限掩码        */
#define ARM64_PTE_NS_SHIFT      (5)
#define ARM64_PTE_NS_MASK       (0x1 << ARM64_PTE_NS_SHIFT)             /*  PTE 中的 Non-Secure         */
#define ARM64_PTE_AIN_SHIFT     (2)
#define ARM64_PTE_AIN_MASK      (0x7 << ARM64_PTE_AIN_SHIFT)            /*  PTE 中的 AttrIndex          */

#define ARM64_MMU_NS_SECURE     (0)
#define ARM64_MMU_NS_NONSECURE  (1)
#define ARM64_MMU_AP_NO_EFFECT  (0)
#define ARM64_MMU_XN_NO_EFFECT  (0)
#define ARM64_MMU_PXN_NO_EFFECT (0)
/*********************************************************************************************************
  PGM PMD PTE 中的掩码
*********************************************************************************************************/
#define ARM64_MMU_ADDR_MASK     (0xffffffff0000ul)                      /*  [47:16]                     */
/*********************************************************************************************************
  2 个检查权限定义
*********************************************************************************************************/
#define GUARDED_CHK          0                                          /*  做详细权限检查              */
#define GUARDED_NOT_CHK      1                                          /*  不做详细权限检查            */
/*********************************************************************************************************
  全局定义
*********************************************************************************************************/
#define NON_SHAREABLE           0x0
#define OUTER_SHAREABLE         0x2
#define INNER_SHAREABLE         0x3
#define VMSA_S                  _G_uiVMSAShare                          /*  共享位值                    */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hPGDPartition;                       /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE         _G_hPMDPartition;                       /*  PMD 缓冲区                  */
static LW_OBJECT_HANDLE         _G_hPTEPartition;                       /*  PTE 缓冲区                  */
static UINT                     _G_uiVMSAShare = INNER_SHAREABLE;       /*  共享位值                    */
/*********************************************************************************************************
** 函数名称: arm64MmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 ARM64 MMU 权限标志
** 输　入  : ulFlag                 内存访问权限
** 输　出  : pucGuard               进行严格的权限检查
**           pucXN                  可执行权限标志
**           pucPXN                 特权可执行权限标志
**           pucCon                 Contiguous 标志
**           pucnG                  nG 标志
**           pucAF                  是否拥有访问权限标志
**           pucSH                  共享权限标志
**           pucAP                  是否可写权限标志
**           pucNS                  Non-Secure 标志
**           pucAIn                 Cache 和 Bufferable 权限标志
**           ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  arm64MmuFlags2Attr (ULONG   ulFlag,
                                UINT8  *pucGuard,
                                UINT8  *pucXN,
                                UINT8  *pucPXN,
                                UINT8  *pucCon,
                                UINT8  *pucnG,
                                UINT8  *pucAF,
                                UINT8  *pucSH,
                                UINT8  *pucAP,
                                UINT8  *pucNS,
                                UINT8  *pucAIn)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否拥有访问权限            */
        if (ulFlag & LW_VMM_FLAG_GUARDED) {
            *pucGuard = GUARDED_CHK;
        } else {
            *pucGuard = GUARDED_NOT_CHK;
        }
        *pucAF = 1; 
    } else {
        *pucAF = 0;                                                     /*  访问失效                    */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  是否可写                    */
        *pucAP = 0x0;                                                   /*  可写                        */
    } else {        
        *pucAP = 0x2;
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /* CACHE && BUFFER WRITE BACK   */
        *pucAIn = 5;
    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /* CACHE && BUFFER WRITE THROUGH*/
        *pucAIn = 6;
    } else {
        *pucAIn = 0;                                                    /* UNCACHE && UNBUFFER          */
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        *pucXN = 0x1;
    } else {
        *pucXN = 0x0;
    }
    
    *pucPXN = 0x0;
    *pucSH  = VMSA_S;
    *pucNS  = 0x0;
    *pucCon = 0x0;
    *pucnG  = 0x0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuAttr2Flags
** 功能描述: 根据 ARM64 MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucGuard               严格的权限检查
**           ucXN                  可执行权限标志
**           ucPXN                 特权可执行权限标志
**           ucCon                 Contiguous 标志
**           ucnG                  nG 标志
**           ucAF                  是否拥有访问权限标志
**           ucSH                  共享权限标志
**           ucAP                  是否可写权限标志
**           ucNS                  Non-Secure 标志
**           ucAIn                 Cache 和 Bufferable 权限标志
** 输　出  : ulFlag                内存访问权限
**           ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  arm64MmuAttr2Flags (UINT8  ucGuard,
                                UINT8  ucXN,
                                UINT8  ucPXN,
                                UINT8  ucCon,
                                UINT8  ucnG,
                                UINT8  ucAF,
                                UINT8  ucSH,
                                UINT8  ucAP,
                                UINT8  ucNS,
                                UINT8  ucAIn,
                                ULONG *pulFlag)
{
    (VOID)ucPXN;

    *pulFlag = LW_VMM_FLAG_VALID;
    
    if (ucGuard == GUARDED_CHK) {
        *pulFlag |= LW_VMM_FLAG_GUARDED;
    }

    if (ucAF == 1) {        
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }

    if ((ucAP == 0) || (ucAP == 1)) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }

    switch (ucAIn) {

    case 0x5:
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
        break;
        
    case 0x6:
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
        break;

    default:
        break;
    }

    if (ucXN == 0x1) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              基地址     (二级页表基地址)
**           ucNS                    是否访问安全区域
**           ucAP                    访问权限
**           ucXN                    可执行权限标志
**           ucPXN                   特权可执行权限标志
**           ucType                  描述符类型
** 输　出  : 一级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE LW_PGD_TRANSENTRY  arm64MmuBuildPgdEntry (addr_t  ulBaseAddr,
                                                           UINT8   ucNS,
                                                           UINT8   ucAP,
                                                           UINT8   ucXN,
                                                           UINT8   ucPXN,
                                                           UINT8   ucType)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & ARM64_MMU_ADDR_MASK)
                 | ((UINT64)ucNS  << ARM64_MMU_NS_SHIFT) 
                 | ((UINT64)ucAP  << ARM64_MMU_AP_SHIFT) 
                 | ((UINT64)ucXN  << ARM64_MMU_XN_SHIFT) 
                 | ((UINT64)ucPXN << ARM64_MMU_PXN_SHIFT)
                 | ucType;

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: arm64MmuBuildPmdEntry
** 功能描述: 生成一个二级描述符 (PMD 描述符)
** 输　入  : ulBaseAddr              基地址     (三级页表基地址)
**           ucNS                    是否访问安全区域
**           ucAP                    访问权限
**           ucXN                    可执行权限标志
**           ucPXN                   特权可执行权限标志
**           ucType                  描述符类型
** 输　出  : 二级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE LW_PMD_TRANSENTRY  arm64MmuBuildPmdEntry (addr_t  ulBaseAddr,
                                                           UINT8   ucNS,
                                                           UINT8   ucAP,
                                                           UINT8   ucXN,
                                                           UINT8   ucPXN,
                                                           UINT8   ucType)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & ARM64_MMU_ADDR_MASK)
                 | ((UINT64)ucNS  << ARM64_MMU_NS_SHIFT) 
                 | ((UINT64)ucAP  << ARM64_MMU_AP_SHIFT) 
                 | ((UINT64)ucXN  << ARM64_MMU_XN_SHIFT) 
                 | ((UINT64)ucPXN << ARM64_MMU_PXN_SHIFT)
                 | ucType;        

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: arm64MmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           ucGuard                 进行严格的权限检查
**           ucXN                    可执行权限标志
**           ucPXN                   特权可执行权限标志
**           ucCon                   Contiguous 标志
**           ucnG                    nG 标志
**           ucAF                    访问标志
**           ucSH                    共享权限标志
**           ucAP                    访问权限标志
**           ucNS                    Non-Secure 标志
**           ucAIn                   Attribute Index
**           ucType                  描述符类型
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  arm64MmuBuildPtentry (addr_t  ulBaseAddr,
                                                UINT8   ucGuard,
                                                UINT8   ucXN,
                                                UINT8   ucPXN,
                                                UINT8   ucCon,
                                                UINT8   ucnG,
                                                UINT8   ucAF,
                                                UINT8   ucSH,
                                                UINT8   ucAP,
                                                UINT8   ucNS,
                                                UINT8   ucAIn,
                                                UINT8   ucType)
{
    LW_PTE_TRANSENTRY  ulDescriptor;

    switch (ucType) {

    case ARM64_PTE_TYPE_PAGE:
        ulDescriptor = (ulBaseAddr & ARM64_MMU_ADDR_MASK)
                     | ((UINT64)ucGuard << ARM64_PTE_GUARD_SHIFT)
                     | ((UINT64)ucXN    << ARM64_PTE_UXN_SHIFT)
                     | ((UINT64)ucPXN   << ARM64_PTE_PXN_SHIFT)
                     | ((UINT64)ucCon   << ARM64_PTE_CONT_SHIFT)
                     | (ucnG  << ARM64_PTE_NG_SHIFT)
                     | (ucAF  << ARM64_PTE_AF_SHIFT)
                     | (ucSH  << ARM64_PTE_SH_SHIFT)
                     | (ucAP  << ARM64_PTE_AP_SHIFT)
                     | (ucNS  << ARM64_PTE_NS_SHIFT)
                     | (ucAIn << ARM64_PTE_AIN_SHIFT)
                     | ucType;
        break;

    default:
        ulDescriptor = 0;
        break;
    }
   
    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: arm64MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : ARM 体系结构要求: 一级页表基地址需要保持 16 KByte 对齐, 单条目映射 1 MByte 空间.
                               二级页表基地址需要保持  1 KByte 对齐, 单条目映射 4 KByte 空间.
*********************************************************************************************************/
static INT  arm64MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (512)
#define PMD_BLOCK_SIZE  (64 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (64 * LW_CFG_KB_SIZE)

    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * PMD_BLOCK_SIZE, PMD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!pvPgdTable || !pvPmdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPMDPartition = API_PartitionCreate("pmd_pool", pvPmdTable, ulPmdNum, PMD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                           
    if (!_G_hPGDPartition || !_G_hPMDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  arm64MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);

    arm64MmuInvalidateTLB();

    arm64MmuSetTCR(0x5c0827510);                                        /*  T0SZ = 2 ^ 48               */

    arm64MmuSetMAIR();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *arm64MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: arm64MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *arm64MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = *p_pgdentry;                                               /*  获得一级页表描述符          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ulTemp & ARM64_MMU_ADDR_MASK);   /*  获得二级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  计算 PMD 号                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  获得二级页表描述符地址      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *arm64MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = *p_pmdentry;                                               /*  获得二级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(ulTemp & ARM64_MMU_ADDR_MASK);   /*  获得三级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;                                   /*  不要使用LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  arm64MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & ARM64_PGD_TYPE_MASK) == ARM64_PGD_TYPE_TABLE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPmdIsOk
** 功能描述: 判断 PMD 项的描述符是否正确
** 输　入  : pmdentry       PMD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  arm64MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  (((pmdentry & ARM64_PMD_TYPE_MASK) == ARM64_PMD_TYPE_TABLE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  arm64MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry & ARM64_PTE_TYPE_MASK) == ARM64_PTE_TYPE_PAGE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *arm64MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;
    
    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  新的 PGD 无有效的页表项     */

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *arm64MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                            LW_PGD_TRANSENTRY  *p_pgdentry,
                                            addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PMD_TRANSENTRY  *p_pmdentry = (LW_PMD_TRANSENTRY *)API_PartitionGet(_G_hPMDPartition);

    if (!p_pmdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pmdentry, PMD_BLOCK_SIZE);

    *p_pgdentry = arm64MmuBuildPgdEntry((addr_t)p_pmdentry,             /*  设置一级页表描述符          */
                                        ARM64_MMU_NS_SECURE,
                                        ARM64_MMU_AP_NO_EFFECT,
                                        ARM64_MMU_XN_NO_EFFECT,
                                        ARM64_MMU_PXN_NO_EFFECT,
                                        ARM64_PGD_TYPE_TABLE);

#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    arm64DCacheFlush((PVOID)p_pgdentry, (PVOID)p_pgdentry, 32);         /*  第三个参数无影响            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (arm64MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: arm64MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量:
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *arm64MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                             LW_PMD_TRANSENTRY  *p_pmdentry,
                                             addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = arm64MmuBuildPmdEntry((addr_t)p_pteentry,             /*  设置二级页表描述符          */
                                        ARM64_MMU_NS_SECURE,
                                        ARM64_MMU_AP_NO_EFFECT,
                                        ARM64_MMU_XN_NO_EFFECT,
                                        ARM64_MMU_PXN_NO_EFFECT,
                                        ARM64_PMD_TYPE_TABLE);
    
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    arm64DCacheFlush((PVOID)p_pmdentry, (PVOID)p_pmdentry, 32);         /*  第三个参数无影响            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (arm64MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: arm64MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: arm64MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  arm64MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(pteentry & ARM64_MMU_ADDR_MASK);        /*  获得物理地址                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arm64MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  arm64MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = arm64MmuPgdOffset(pmmuctx, ulAddr);/*  获取一级描述符              */
    INT                 iDescType;
    ULONG               ulFlag = 0;

    UINT8               ucGuard;                                        /*  严格的权限检查              */
    UINT8               ucXN;                                           /*  可执行权限标志              */
    UINT8               ucPXN;                                          /*  特权可执行权限标志          */
    UINT8               ucCon;                                          /*  Contiguous 标志             */
    UINT8               ucnG;                                           /*  nG 标志                     */
    UINT8               ucAF;                                           /*  是否拥有访问权限标志        */
    UINT8               ucSH;                                           /*  共享权限标志                */
    UINT8               ucAP;                                           /*  是否可写权限标志            */
    UINT8               ucNS;                                           /*  Non-Secure 标志             */
    UINT8               ucAIn;

    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == ARM64_PGD_TYPE_BLOCK) {                            /*  基于段的映射                */
       return  (LW_VMM_FLAG_UNVALID);

    } else if (iDescType == ARM64_PGD_TYPE_TABLE) {                     /*  基于三级页表映射            */
        LW_PMD_TRANSENTRY  *p_pmdentry = arm64MmuPmdOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  获取二级描述符              */
        if (arm64MmuPmdIsOk(*p_pmdentry)) {
              LW_PTE_TRANSENTRY  *p_pteentry = arm64MmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                                 ulAddr);
                                                                        /*  获取三级描述符              */
              if (arm64MmuPteIsOk(*p_pteentry)) {
                  UINT64  u64Descriptor = (UINT64)(*p_pteentry);

                  ucGuard = (UINT8)((u64Descriptor & ARM64_PTE_GUARD_MASK) >> ARM64_PTE_GUARD_SHIFT);
                  ucXN    = (UINT8)((u64Descriptor & ARM64_PTE_UXN_MASK)   >> ARM64_PTE_UXN_SHIFT);
                  ucPXN   = (UINT8)((u64Descriptor & ARM64_PTE_PXN_MASK)   >> ARM64_PTE_PXN_SHIFT);
                  ucCon   = (UINT8)((u64Descriptor & ARM64_PTE_CONT_MASK)  >> ARM64_PTE_CONT_SHIFT);
                  ucnG    = (UINT8)((u64Descriptor & ARM64_PTE_NG_MASK)    >> ARM64_PTE_NG_SHIFT);
                  ucAF    = (UINT8)((u64Descriptor & ARM64_PTE_AF_MASK)    >> ARM64_PTE_AF_SHIFT);
                  ucSH    = (UINT8)((u64Descriptor & ARM64_PTE_SH_MASK)    >> ARM64_PTE_SH_SHIFT);
                  ucAP    = (UINT8)((u64Descriptor & ARM64_PTE_AP_MASK)    >> ARM64_PTE_AP_SHIFT);
                  ucNS    = (UINT8)((u64Descriptor & ARM64_PTE_NS_MASK)    >> ARM64_PTE_NS_SHIFT);
                  ucAIn   = (UINT8)((u64Descriptor & ARM64_PTE_AIN_MASK)   >> ARM64_PTE_AIN_SHIFT);

                  arm64MmuAttr2Flags(ucGuard, ucXN, ucPXN, ucCon, ucnG,
                                     ucAF, ucSH, ucAP, ucNS, ucAIn, &ulFlag);

                  return  (ulFlag);
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: arm64MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  arm64MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = arm64MmuPgdOffset(pmmuctx, ulAddr);/*  获取一级描述符              */
    INT                 iDescType;    
    
    UINT8               ucGuard;                                        /*  严格的权限检查              */
    UINT8               ucXN;                                           /*  可执行权限标志              */
    UINT8               ucPXN;                                          /*  特权可执行权限标志          */
    UINT8               ucContiguous;                                   /*  Contiguous 标志             */
    UINT8               ucnG;                                           /*  nG 标志                     */
    UINT8               ucAF;                                           /*  是否拥有访问权限标志        */
    UINT8               ucSH;                                           /*  共享权限标志                */
    UINT8               ucAP;                                           /*  是否可写权限标志            */
    UINT8               ucNS;                                           /*  Non-Secure 标志             */
    UINT8               ucAttrIndx;
    UINT8               ucType;
        
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = ARM64_PTE_TYPE_PAGE;

    } else {
        ucType = ARM64_PTE_TYPE_FAULT;                                   /*  访问将失效                 */
    }

    if (arm64MmuFlags2Attr(ulFlag,
                           &ucGuard,
                           &ucXN, &ucPXN,
                           &ucContiguous,
                           &ucnG, &ucAF,
                           &ucSH, &ucAP,
                           &ucNS, &ucAttrIndx) < 0) {                   /*  无效的映射关系              */
        return (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == ARM64_PGD_TYPE_BLOCK) {                            /*  基于段的映射                */
        return  (ERROR_NONE);
    
    } else if (iDescType == ARM64_PGD_TYPE_TABLE) {                     /*  基于三级页表映射            */
        LW_PMD_TRANSENTRY  *p_pmdentry = arm64MmuPmdOffset((LW_PGD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  获取二级描述符              */
        if (arm64MmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = arm64MmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                               ulAddr);
                                                                        /*  获取三级描述符              */
            if (arm64MmuPteIsOk(*p_pteentry)) {
                addr_t   ulPhysicalAddr = (addr_t)(*p_pteentry & ARM64_MMU_ADDR_MASK);

                *p_pteentry = arm64MmuBuildPtentry(ulPhysicalAddr,
                                                   ucGuard,
                                                   ucXN, ucPXN,
                                                   ucContiguous,
                                                   ucnG, ucAF,
                                                   ucSH, ucAP,
                                                   ucNS, ucAttrIndx,
                                                   ucType);
#if LW_CFG_CACHE_EN > 0
                arm64DCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);
                                                                        /*  第三个参数无影响            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arm64MmuMakeTrans
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
static VOID  arm64MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                LW_PTE_TRANSENTRY  *p_pteentry,
                                addr_t              ulVirtualAddr,
                                phys_addr_t         paPhysicalAddr,
                                ULONG               ulFlag)
{
    UINT8               ucGuard;                                        /*  严格的权限检查              */
    UINT8               ucXN;                                           /*  存储权限                    */
    UINT8               ucPXN;                                          /*  域                          */
    UINT8               ucContiguous;                                   /*  CACHE 与缓冲区控制          */
    UINT8               ucnG;                                           /*  存储权限                    */
    UINT8               ucAF;                                           /*  CACHE 与缓冲区控制          */
    UINT8               ucSH;                                           /*  永不执行位                  */
    UINT8               ucAP;
    UINT8               ucNS;
    UINT8               ucAttrIndx;
    UINT8               ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = ARM64_PTE_TYPE_PAGE;
    } else {
        ucType = ARM64_PTE_TYPE_FAULT;                                  /*  访问将失效                  */
    }

    if (arm64MmuFlags2Attr(ulFlag,
                           &ucGuard,
                           &ucXN, &ucPXN,
                           &ucContiguous,
                           &ucnG, &ucAF,
                           &ucSH, &ucAP,
                           &ucNS, &ucAttrIndx) < 0) {                   /*  无效的映射关系              */
        return;
    }

    *p_pteentry = arm64MmuBuildPtentry((addr_t)paPhysicalAddr,
                                       ucGuard,
                                       ucXN, ucPXN,
                                       ucContiguous,
                                       ucnG, ucAF,
                                       ucSH, ucAP,
                                       ucNS, ucAttrIndx,
                                       ucType);

#if LW_CFG_CACHE_EN > 0
    arm64DCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);         /*  第三个参数无影响            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: arm64MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文, 这里使用 TTBR1 页表基址寄存器.
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)pmmuctx->MMUCTX_pgdEntry);
          
    arm64MmuSetTTBR((PVOID)p_pgdentry);                                 /*  设置页表基地址              */
}
/*********************************************************************************************************
** 函数名称: arm64MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        arm64MmuInvalidateTLB();                                        /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            arm64MmuInvalidateTLBMVA((PVOID)ulPageAddr);                /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: arm64MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  arm64MmuInit (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit    = arm64MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = arm64MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = arm64MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = arm64MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = arm64MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = arm64MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = arm64MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = arm64MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = arm64MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = arm64MmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = arm64MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = arm64MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = arm64MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = arm64MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = arm64MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = arm64MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = arm64MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = arm64MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = arm64MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = arm64MmuInvTLB;
    
    pmmuop->MMUOP_pfuncSetEnable     = arm64MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = arm64MmuDisable;
}
/*********************************************************************************************************
** 函数名称: arm64MmuShareableSet
** 功能描述: MMU 系统 share 模式设置 (如果 CACHE 使用 SNOOP 则提前设置为 OUTER_SHAREABLE)
** 输　入  : bInnerOrOuter     0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  arm64MmuShareableSet (INT  iInnerOrOuter)
{
    if (iInnerOrOuter) {
        VMSA_S = OUTER_SHAREABLE;
    } else {
        VMSA_S = INNER_SHAREABLE;
    }
}
/*********************************************************************************************************
** 函数名称: arm64MmuShareableGet
** 功能描述: MMU 系统 share 模式获取
** 输　入  : NONE
** 输　出  : 0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  arm64MmuShareableGet (VOID)
{
    if (VMSA_S == OUTER_SHAREABLE) {
        return  (1);
    } else {
        return  (0);
    }
}

#endif                                                                  /*  LW_CFG_ARM64_PAGE_SHIFT==16 */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
