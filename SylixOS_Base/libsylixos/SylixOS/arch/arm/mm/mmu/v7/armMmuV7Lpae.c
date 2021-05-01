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
** 文   件   名: armMmuV7Lpae.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 11 月 14 日
**
** 描        述: ARMv7 体系构架支持 Large Physical Address Extension 的 MMU 驱动.
**               当前设计要求页表所占用内存应当在 4G 范围以内.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
#include "../armMmuCommon.h"
#include "../../cache/armCacheCommon.h"
#include "../../../common/cp15/armCp15.h"
#include "../../../param/armParam.h"
/*********************************************************************************************************
  一级、二级描述符类型定义
*********************************************************************************************************/
#define COARSE_TBASE          (3)                                       /*  二级粗粒度页表基地址        */
#define SEGMENT_BASE          (1)                                       /*  段映射基地址                */
/*********************************************************************************************************
  一级、二级页表表项格式
       63      62        61     60          59     58   40 39                       12 11  2  1   0
  +---------+--------------+-----------+----------+-------+---------------------------+-----+---+---+
  | NSTable |    APTable   |  XNTable  | PXNTable |       | Next Level Table Address  |     | 1 | 1 |
  +---------+--------------+-----------+----------+-------+---------------------------+-----+---+---+

  三级页表表项格式
    63   55   54   53     52   51     40 39             12  11   10   9 8  7 6  5    4   2     1   0
  +--------+-----+-----+-----+---------+-----------------+----+----+----+----+----+---------+---+---+
  |        | XN  | PXN | Con |         |  Output address | nG | AF | SH | AP | NS | AttrInx | 1 | 1 |
  +--------+-----+-----+-----+---------+-----------------+----+----+----+----+----+---------+---+---+
*********************************************************************************************************/
#define ARM_MMU_ADDR_MASK     (0xfffffff000ULL)                         /*  MMU 地址掩码                */
#define ARM_MMU_NS_SHIFT      (63)                                      /*  Non-Secure 标志             */
#define ARM_MMU_AP_SHIFT      (61)                                      /*  Access permissions 标志     */
#define ARM_MMU_XN_SHIFT      (60)                                      /*  XN 标志                     */
#define ARM_MMU_PXN_SHIFT     (59)                                      /*  PXN 标志                    */

#define ARM_PTE_UXN_SHIFT     (54)
#define ARM_PTE_UXN_MASK      (1ULL << ARM_PTE_UXN_SHIFT)               /*  User XN                     */
#define ARM_PTE_PXN_SHIFT     (53)
#define ARM_PTE_PXN_MASK      (1ULL << ARM_PTE_PXN_SHIFT)               /*  Privileged XN               */
#define ARM_PTE_CONT_SHIFT    (52)
#define ARM_PTE_CONT_MASK     (1ULL << ARM_PTE_CONT_SHIFT)              /*  Contiguous range            */
#define ARM_PTE_NG_SHIFT      (11)
#define ARM_PTE_NG_MASK       (0x1 << ARM_PTE_NG_SHIFT)                 /*  PTE 中的 nG 标志            */
#define ARM_PTE_AF_SHIFT      (10)
#define ARM_PTE_AF_MASK       (0x1 << ARM_PTE_AF_SHIFT)                 /*  PTE 中的访问标志            */
#define ARM_PTE_SH_SHIFT      (8)
#define ARM_PTE_SH_MASK       (0x3 << ARM_PTE_SH_SHIFT)                 /*  PTE 中的共享权限掩码        */
#define ARM_PTE_AP_SHIFT      (6)
#define ARM_PTE_AP_MASK       (0x3 << ARM_PTE_AP_SHIFT)                 /*  PTE 中的访问权限掩码        */
#define ARM_PTE_NS_SHIFT      (5)
#define ARM_PTE_NS_MASK       (0x1 << ARM_PTE_NS_SHIFT)                 /*  PTE 中的 Non-Secure         */
#define ARM_PTE_AIN_SHIFT     (2)
#define ARM_PTE_AIN_MASK      (0x7 << ARM_PTE_AIN_SHIFT)                /*  PTE 中的 AttrIndex          */

#define ARM_MMU_NS_SECURE     (0)
#define ARM_MMU_NS_NONSECURE  (1)
#define ARM_MMU_AP_NO_EFFECT  (0)
#define ARM_MMU_XN_NO_EFFECT  (0)
#define ARM_MMU_PXN_NO_EFFECT (0)
/*********************************************************************************************************
  三级描述符类型定义
*********************************************************************************************************/
#define FAIL_DESC             (0)                                       /*  变换失效                    */
#define SMALLPAGE_DESC        (3)                                       /*  小页基地址                  */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD 缓冲区                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
/*********************************************************************************************************
  内存映射属性配置
*********************************************************************************************************/
#define NON_SHAREABLE       0x0
#define OUTER_SHAREABLE     0x2
#define INNER_SHAREABLE     0x3
#define VMSA_S              _G_uiVMSAShare                              /*  共享位值                    */
static UINT                 _G_uiVMSAShare = OUTER_SHAREABLE;           /*  共享位值                    */
/*********************************************************************************************************
  汇编函数
*********************************************************************************************************/
extern UINT32 armMmuV7GetTTBCR(VOID);
extern VOID   armMmuV7SetTTBCR(UINT32 uiTTBCR);
extern VOID   armMmuV7SetTTBase(UINT32  uiPgdEntryLow, UINT32  uiPgdEntryHi);
extern VOID   armMmuV7SetTTBase1(UINT32  uiPgdEntryLow, UINT32  uiPgdEntryHi);
extern VOID   armMmuV7GetTTBase(UINT32  *puiPgdEntryLow, UINT32  *puiPgdEntryHi);
extern VOID   armMmuV7GetTTBase1(UINT32  *puiPgdEntryLow, UINT32  *puiPgdEntryHi);
extern UINT32 armMmuV7GetMAIR0(VOID);
extern UINT32 armMmuV7GetMAIR1(VOID);
extern VOID   armMmuV7SetMAIR0(VOID);
extern VOID   armMmuV7SetMAIR1(VOID);
/*********************************************************************************************************
** 函数名称: armMmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 ARM MMU 权限标志
** 输　入  : ulFlag                  内存访问权限
**           pucAP                   访问权限
**           pucDomain               所属控制域
**           pucCB                   CACHE 控制参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuFlags2Attr (ULONG   ulFlag,
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
        *pucXN = 0x0;
    } else {
        *pucXN = 0x1;
    }

    *pucPXN = 0x0;
    *pucSH  = VMSA_S;
    *pucNS  = 0x0;
    *pucCon = 0x0;
    *pucnG  = 0x0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuAttr2Flags
** 功能描述: 根据 ARM MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucAP                    访问权限
**           ucAP2                   访问权限
**           ucDomain                所属控制域
**           ucCB                    CACHE 控制参数
**           ucTEX                   CACHE 控制参数
**           ucXN                    可执行权限
**           pulFlag                 内存访问权限
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuAttr2Flags (UINT8  ucXN,
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
    
    if (ucXN == 0x0) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              基地址     (段基地址、二级页表基地址)
**           ucAP                    访问权限
**           ucDomain                域
**           ucCB                    CACHE 和 WRITEBUFFER 控制
**           ucType                  一级描述符类型
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  armMmuBuildPgdEntry (addr_t  ulBaseAddr,
                                               UINT8   ucNS,
                                               UINT8   ucAP,
                                               UINT8   ucXN,
                                               UINT8   ucPXN,
                                               UINT8   ucType)
{
    LW_PGD_TRANSENTRY   u64Descriptor;

    switch (ucType) {
    
    case COARSE_TBASE:                                                  /*  粗粒度页表描述符            */
        u64Descriptor = (ulBaseAddr & ARM_MMU_ADDR_MASK)
                      | ((UINT64)ucNS  << ARM_MMU_NS_SHIFT)
                      | ((UINT64)ucAP  << ARM_MMU_AP_SHIFT)
                      | ((UINT64)ucXN  << ARM_MMU_XN_SHIFT)
                      | ((UINT64)ucPXN << ARM_MMU_PXN_SHIFT)
                      | ucType;
        break;
        
    default:
        u64Descriptor = 0;                                              /*  访问失效                    */
        break;
    }
    
    return  (u64Descriptor);
}
/*********************************************************************************************************
** 函数名称: armMmuBuildPmdEntry
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
static LW_PMD_TRANSENTRY  armMmuBuildPmdEntry (addr_t  ulBaseAddr,
                                               UINT8   ucNS,
                                               UINT8   ucAP,
                                               UINT8   ucXN,
                                               UINT8   ucPXN,
                                               UINT8   ucType)
{
    LW_PGD_TRANSENTRY  u64Descriptor;

    u64Descriptor = (ulBaseAddr & ARM_MMU_ADDR_MASK)
                  | ((UINT64)ucNS  << ARM_MMU_NS_SHIFT)
                  | ((UINT64)ucAP  << ARM_MMU_AP_SHIFT)
                  | ((UINT64)ucXN  << ARM_MMU_XN_SHIFT)
                  | ((UINT64)ucPXN << ARM_MMU_PXN_SHIFT)
                  | ucType;

    return  (u64Descriptor);
}
/*********************************************************************************************************
** 函数名称: armMmuBuildPteEntry
** 功能描述: 生成一个三级描述符 (PTE 描述符)
** 输　入  : u64BaseAddr             基地址     (页地址)
**           ucAP                    访问权限
**           ucDomain                域
**           ucCB                    CACHE 和 WRITEBUFFER 控制
**           ucType                  三级描述符类型
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  armMmuBuildPteEntry (phys_addr_t  u64BaseAddr,
                                               UINT8        ucXN,
                                               UINT8        ucPXN,
                                               UINT8        ucCon,
                                               UINT8        ucnG,
                                               UINT8        ucAF,
                                               UINT8        ucSH,
                                               UINT8        ucAP,
                                               UINT8        ucNS,
                                               UINT8        ucAIn,
                                               UINT8        ucType)
{
    LW_PTE_TRANSENTRY   u64Descriptor;

    switch (ucType) {
    
    case SMALLPAGE_DESC:                                                /*  小页描述符                  */
        u64Descriptor = (u64BaseAddr & ARM_MMU_ADDR_MASK)
                      | ((UINT64)ucXN    << ARM_PTE_UXN_SHIFT)
                      | ((UINT64)ucPXN   << ARM_PTE_PXN_SHIFT)
                      | ((UINT64)ucCon   << ARM_PTE_CONT_SHIFT)
                      | (ucnG  << ARM_PTE_NG_SHIFT)
                      | (ucAF  << ARM_PTE_AF_SHIFT)
                      | (ucSH  << ARM_PTE_SH_SHIFT)
                      | (ucAP  << ARM_PTE_AP_SHIFT)
                      | (ucNS  << ARM_PTE_NS_SHIFT)
                      | (ucAIn << ARM_PTE_AIN_SHIFT)
                      | ucType;
        break;

    default:
        u64Descriptor = 0;                                              /*  访问失效                    */
        break;
    }
    
    return  (u64Descriptor);
}
/*********************************************************************************************************
** 函数名称: armMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 支持 LPAE 时    : 需要采用三级页表结构.
                               一级页表基地址需要保持  32 Byte 对齐，这里取一个页 4KByte 对齐.
                               二级页表基地址需要保持  4 KByte 对齐.
                               二级页表基地址需要保持  4 KByte 对齐.
*********************************************************************************************************/
static INT  armMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPmdTable;
    PVOID   pvPteTable;
    
    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPmdNum = bspMmuPmdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();
    
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
** 函数名称: armMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuGlobalInit (CPCHAR  pcMachineName)
{
    ARM_PARAM   *param = archKernelParamGet();
    UINT         uiDefaultCfg;
    
    archCacheReset(pcMachineName);

    armMmuInvalidateTLB();

    /*
     *  地址检查选项 (Qt 里面用到了非对齐指令)
     *  注 意: 如果使能地址对齐检查, GCC 编译必须加入 -mno-unaligned-access 选项 (不生成非对齐访问指令)
     */
    if (param->AP_bUnalign) {
        armMmuDisableAlignFault();
        
    } else {
        armMmuEnableAlignFault();                                       /*  -mno-unaligned-access       */
    }
    
    uiDefaultCfg = 0x85000500 | (VMSA_S << 28) | (VMSA_S << 12);
    armMmuV7SetTTBCR(uiDefaultCfg);                                     /*  支持 LPAE                   */

    armMmuV7SetMAIR0();
    armMmuV7SetMAIR1();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: armMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
             LW_PMD_TRANSENTRY  *p_pmdentry;
             LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = *p_pgdentry;                                               /*  获得一级页表描述符          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ULONG)(ulTemp &
                                              ARM_MMU_ADDR_MASK);       /*  获得二级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  计算 PMD 号                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  获得二级页表描述符地址      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *armMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
             LW_PTE_TRANSENTRY  *p_pteentry;
             LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = *p_pmdentry;                                               /*  获得二级页表描述符          */

    p_pteentry = (LW_PTE_TRANSENTRY *)(ULONG)(ulTemp &
                                              ARM_MMU_ADDR_MASK);       /*  获得三级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  armMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armMmuPmdIsOk
** 功能描述: 判断 PMD 项的描述符是否正确
** 输　入  : pmdentry       PMD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  armMmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  (((pmdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  armMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry & 0x03) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER ULONG               ulPgdNum;
    
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
** 函数名称: armMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx, 
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

    *p_pgdentry = armMmuBuildPgdEntry((addr_t)p_pmdentry,             /*  设置一级页表描述符          */
                                      ARM_MMU_NS_SECURE,
                                      ARM_MMU_AP_NO_EFFECT,
                                      ARM_MMU_XN_NO_EFFECT,
                                      ARM_MMU_PXN_NO_EFFECT,
                                      COARSE_TBASE);

#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    armDCacheFlush((PVOID)p_pgdentry, (PVOID)p_pgdentry, 64);
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (armMmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: armMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *armMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
                                           addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG              iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    *p_pmdentry = armMmuBuildPmdEntry((addr_t)p_pteentry,               /*  设置三级页表描述符          */
                                       ARM_MMU_NS_SECURE,
                                       ARM_MMU_AP_NO_EFFECT,
                                       ARM_MMU_XN_NO_EFFECT,
                                       ARM_MMU_PXN_NO_EFFECT,
                                       COARSE_TBASE);

#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    armDCacheFlush((PVOID)p_pmdentry, (PVOID)p_pmdentry, 64);
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (armMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: armMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           ppaPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (pteentry & ARM_MMU_ADDR_MASK);                  /*  获得物理地址                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  armMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);  /*  获取一级描述符              */
    INT                 iDescType;
    ULONG               ulFlag = 0;

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
    if (iDescType == SEGMENT_BASE) {                                    /*  基于段的映射                */
       return  (LW_VMM_FLAG_UNVALID);

    } else if (iDescType == COARSE_TBASE) {                             /*  基于三级页表映射            */
        LW_PMD_TRANSENTRY  *p_pmdentry = armMmuPmdOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  获取二级描述符              */
        if (armMmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                             ulAddr);
                                                                        /*  获取三级描述符              */
            if (armMmuPteIsOk(*p_pteentry)) {
                UINT64  u64Descriptor = (UINT64)(*p_pteentry);

                ucXN    = (UINT8)((u64Descriptor & ARM_PTE_UXN_MASK)   >> ARM_PTE_UXN_SHIFT);
                ucPXN   = (UINT8)((u64Descriptor & ARM_PTE_PXN_MASK)   >> ARM_PTE_PXN_SHIFT);
                ucCon   = (UINT8)((u64Descriptor & ARM_PTE_CONT_MASK)  >> ARM_PTE_CONT_SHIFT);
                ucnG    = (UINT8)((u64Descriptor & ARM_PTE_NG_MASK)    >> ARM_PTE_NG_SHIFT);
                ucAF    = (UINT8)((u64Descriptor & ARM_PTE_AF_MASK)    >> ARM_PTE_AF_SHIFT);
                ucSH    = (UINT8)((u64Descriptor & ARM_PTE_SH_MASK)    >> ARM_PTE_SH_SHIFT);
                ucAP    = (UINT8)((u64Descriptor & ARM_PTE_AP_MASK)    >> ARM_PTE_AP_SHIFT);
                ucNS    = (UINT8)((u64Descriptor & ARM_PTE_NS_MASK)    >> ARM_PTE_NS_SHIFT);
                ucAIn   = (UINT8)((u64Descriptor & ARM_PTE_AIN_MASK)   >> ARM_PTE_AIN_SHIFT);

                armMmuAttr2Flags(ucXN, ucPXN, ucCon, ucnG,
                                 ucAF, ucSH, ucAP, ucNS, ucAIn, &ulFlag);

                return  (ulFlag);
            }
        }
    }
    
    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: armMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  armMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);  /*  获取一级描述符              */
    INT                 iDescType;
    
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
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                              /*  访问将失效                 */
    }

    if (armMmuFlags2Attr(ulFlag,
                         &ucXN, &ucPXN,
                         &ucContiguous,
                         &ucnG, &ucAF,
                         &ucSH, &ucAP,
                         &ucNS, &ucAttrIndx) < 0) {                     /*  无效的映射关系              */
        return (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == SEGMENT_BASE) {                                    /*  基于段的映射                */
        return  (ERROR_NONE);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  基于三级页表映射            */
        LW_PMD_TRANSENTRY  *p_pmdentry = armMmuPmdOffset((LW_PGD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  获取二级描述符              */
        if (armMmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                             ulAddr);
                                                                        /*  获取三级描述符              */
            if (armMmuPteIsOk(*p_pteentry)) {
                phys_addr_t   paPhysicalAddr = (phys_addr_t)(*p_pteentry & ARM_MMU_ADDR_MASK);

               *p_pteentry = armMmuBuildPteEntry(paPhysicalAddr,
                                                 ucXN, ucPXN,
                                                 ucContiguous,
                                                 ucnG, ucAF,
                                                 ucSH, ucAP,
                                                 ucNS, ucAttrIndx,
                                                 ucType);
#if LW_CFG_CACHE_EN > 0
                armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 64);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armMmuMakeTrans
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
static VOID  armMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
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
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  访问将失效                  */
    }

    if (armMmuFlags2Attr(ulFlag,
                         &ucXN, &ucPXN,
                         &ucContiguous,
                         &ucnG, &ucAF,
                         &ucSH, &ucAP,
                         &ucNS, &ucAttrIndx) < 0) {                     /*  无效的映射关系              */
        return;
    }

    *p_pteentry = armMmuBuildPteEntry(paPhysicalAddr,
                                      ucXN, ucPXN,
                                      ucContiguous,
                                      ucnG, ucAF,
                                      ucSH, ucAP,
                                      ucNS, ucAttrIndx,
                                      ucType);

#if LW_CFG_CACHE_EN > 0
    armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 64);           /*  第三个参数无影响            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: armMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文, 这里使用 TTBR1 页表基址寄存器.
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;

    /*
     *  Set location of level 1 page table
     * ------------------------------------
     *  63:56 - Reserved UNK/SBZP
     *  55:48 - ASID
     *  47:40 - Reserved UNK/SBZP
     *  39:5  - Base addr
     *  4 :0  - Reserved UNK/SBZP
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)(pmmuctx->MMUCTX_pgdEntry);

    armMmuV7SetTTBase((UINT32)p_pgdentry, 0);                           /*  高位应当为 0               */
    armMmuV7SetTTBase1((UINT32)p_pgdentry, 0);                          /*  高位应当为 0               */
}
/*********************************************************************************************************
** 函数名称: armMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        armMmuInvalidateTLB();                                          /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            armMmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: armMmuV7Init
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armMmuV7Init (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit    = armMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = armMmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = armMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = armMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = armMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = armMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = armMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = armMmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = armMmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = armMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = armMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = armMmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = armMmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = armMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = armMmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = armMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = armMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = armMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = armMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = armMmuDisable;
}
/*********************************************************************************************************
** 函数名称: armMmuV7ShareableSet
** 功能描述: MMU 系统 share 模式设置
** 输　入  : uiInnerOrOuter     0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armMmuV7ShareableSet (UINT  uiInnerOrOuter)
{
    if (uiInnerOrOuter) {
        VMSA_S = OUTER_SHAREABLE;
    } else {
        VMSA_S = INNER_SHAREABLE;
    }
}
/*********************************************************************************************************
** 函数名称: armMmuV7ShareableGet
** 功能描述: MMU 系统 share 模式获取
** 输　入  : NONE
** 输　出  : 0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT  armMmuV7ShareableGet (VOID)
{
    if (VMSA_S == OUTER_SHAREABLE) {
        return  (1);
    } else {
        return  (0);
    }
}

#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT  */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
