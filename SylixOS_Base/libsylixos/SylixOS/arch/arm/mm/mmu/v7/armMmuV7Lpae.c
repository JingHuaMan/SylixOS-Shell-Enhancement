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
** ��   ��   ��: armMmuV7Lpae.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 11 �� 14 ��
**
** ��        ��: ARMv7 ��ϵ����֧�� Large Physical Address Extension �� MMU ����.
**               ��ǰ���Ҫ��ҳ����ռ���ڴ�Ӧ���� 4G ��Χ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
#include "../armMmuCommon.h"
#include "../../cache/armCacheCommon.h"
#include "../../../common/cp15/armCp15.h"
#include "../../../param/armParam.h"
/*********************************************************************************************************
  һ�����������������Ͷ���
*********************************************************************************************************/
#define COARSE_TBASE          (3)                                       /*  ����������ҳ�����ַ        */
#define SEGMENT_BASE          (1)                                       /*  ��ӳ�����ַ                */
/*********************************************************************************************************
  һ��������ҳ������ʽ
       63      62        61     60          59     58   40 39                       12 11  2  1   0
  +---------+--------------+-----------+----------+-------+---------------------------+-----+---+---+
  | NSTable |    APTable   |  XNTable  | PXNTable |       | Next Level Table Address  |     | 1 | 1 |
  +---------+--------------+-----------+----------+-------+---------------------------+-----+---+---+

  ����ҳ������ʽ
    63   55   54   53     52   51     40 39             12  11   10   9 8  7 6  5    4   2     1   0
  +--------+-----+-----+-----+---------+-----------------+----+----+----+----+----+---------+---+---+
  |        | XN  | PXN | Con |         |  Output address | nG | AF | SH | AP | NS | AttrInx | 1 | 1 |
  +--------+-----+-----+-----+---------+-----------------+----+----+----+----+----+---------+---+---+
*********************************************************************************************************/
#define ARM_MMU_ADDR_MASK     (0xfffffff000ULL)                         /*  MMU ��ַ����                */
#define ARM_MMU_NS_SHIFT      (63)                                      /*  Non-Secure ��־             */
#define ARM_MMU_AP_SHIFT      (61)                                      /*  Access permissions ��־     */
#define ARM_MMU_XN_SHIFT      (60)                                      /*  XN ��־                     */
#define ARM_MMU_PXN_SHIFT     (59)                                      /*  PXN ��־                    */

#define ARM_PTE_UXN_SHIFT     (54)
#define ARM_PTE_UXN_MASK      (1ULL << ARM_PTE_UXN_SHIFT)               /*  User XN                     */
#define ARM_PTE_PXN_SHIFT     (53)
#define ARM_PTE_PXN_MASK      (1ULL << ARM_PTE_PXN_SHIFT)               /*  Privileged XN               */
#define ARM_PTE_CONT_SHIFT    (52)
#define ARM_PTE_CONT_MASK     (1ULL << ARM_PTE_CONT_SHIFT)              /*  Contiguous range            */
#define ARM_PTE_NG_SHIFT      (11)
#define ARM_PTE_NG_MASK       (0x1 << ARM_PTE_NG_SHIFT)                 /*  PTE �е� nG ��־            */
#define ARM_PTE_AF_SHIFT      (10)
#define ARM_PTE_AF_MASK       (0x1 << ARM_PTE_AF_SHIFT)                 /*  PTE �еķ��ʱ�־            */
#define ARM_PTE_SH_SHIFT      (8)
#define ARM_PTE_SH_MASK       (0x3 << ARM_PTE_SH_SHIFT)                 /*  PTE �еĹ���Ȩ������        */
#define ARM_PTE_AP_SHIFT      (6)
#define ARM_PTE_AP_MASK       (0x3 << ARM_PTE_AP_SHIFT)                 /*  PTE �еķ���Ȩ������        */
#define ARM_PTE_NS_SHIFT      (5)
#define ARM_PTE_NS_MASK       (0x1 << ARM_PTE_NS_SHIFT)                 /*  PTE �е� Non-Secure         */
#define ARM_PTE_AIN_SHIFT     (2)
#define ARM_PTE_AIN_MASK      (0x7 << ARM_PTE_AIN_SHIFT)                /*  PTE �е� AttrIndex          */

#define ARM_MMU_NS_SECURE     (0)
#define ARM_MMU_NS_NONSECURE  (1)
#define ARM_MMU_AP_NO_EFFECT  (0)
#define ARM_MMU_XN_NO_EFFECT  (0)
#define ARM_MMU_PXN_NO_EFFECT (0)
/*********************************************************************************************************
  �������������Ͷ���
*********************************************************************************************************/
#define FAIL_DESC             (0)                                       /*  �任ʧЧ                    */
#define SMALLPAGE_DESC        (3)                                       /*  Сҳ����ַ                  */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD ������                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
  �ڴ�ӳ����������
*********************************************************************************************************/
#define NON_SHAREABLE       0x0
#define OUTER_SHAREABLE     0x2
#define INNER_SHAREABLE     0x3
#define VMSA_S              _G_uiVMSAShare                              /*  ����λֵ                    */
static UINT                 _G_uiVMSAShare = OUTER_SHAREABLE;           /*  ����λֵ                    */
/*********************************************************************************************************
  ��ຯ��
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
** ��������: armMmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� ARM MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  �ڴ����Ȩ��
**           pucAP                   ����Ȩ��
**           pucDomain               ����������
**           pucCB                   CACHE ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ӵ�з���Ȩ��            */
        *pucAF = 1;
    } else {
        *pucAF = 0;                                                     /*  ����ʧЧ                    */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucAP = 0x0;                                                   /*  ��д                        */
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
** ��������: armMmuAttr2Flags
** ��������: ���� ARM MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucAP                    ����Ȩ��
**           ucAP2                   ����Ȩ��
**           ucDomain                ����������
**           ucCB                    CACHE ���Ʋ���
**           ucTEX                   CACHE ���Ʋ���
**           ucXN                    ��ִ��Ȩ��
**           pulFlag                 �ڴ����Ȩ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: armMmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ַ     (�λ���ַ������ҳ�����ַ)
**           ucAP                    ����Ȩ��
**           ucDomain                ��
**           ucCB                    CACHE �� WRITEBUFFER ����
**           ucType                  һ������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
    
    case COARSE_TBASE:                                                  /*  ������ҳ��������            */
        u64Descriptor = (ulBaseAddr & ARM_MMU_ADDR_MASK)
                      | ((UINT64)ucNS  << ARM_MMU_NS_SHIFT)
                      | ((UINT64)ucAP  << ARM_MMU_AP_SHIFT)
                      | ((UINT64)ucXN  << ARM_MMU_XN_SHIFT)
                      | ((UINT64)ucPXN << ARM_MMU_PXN_SHIFT)
                      | ucType;
        break;
        
    default:
        u64Descriptor = 0;                                              /*  ����ʧЧ                    */
        break;
    }
    
    return  (u64Descriptor);
}
/*********************************************************************************************************
** ��������: armMmuBuildPmdEntry
** ��������: ����һ������������ (PMD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucNS                    �Ƿ���ʰ�ȫ����
**           ucAP                    ����Ȩ��
**           ucXN                    ��ִ��Ȩ�ޱ�־
**           ucPXN                   ��Ȩ��ִ��Ȩ�ޱ�־
**           ucType                  ����������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: armMmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : u64BaseAddr             ����ַ     (ҳ��ַ)
**           ucAP                    ����Ȩ��
**           ucDomain                ��
**           ucCB                    CACHE �� WRITEBUFFER ����
**           ucType                  ��������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
    
    case SMALLPAGE_DESC:                                                /*  Сҳ������                  */
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
        u64Descriptor = 0;                                              /*  ����ʧЧ                    */
        break;
    }
    
    return  (u64Descriptor);
}
/*********************************************************************************************************
** ��������: armMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ֧�� LPAE ʱ    : ��Ҫ��������ҳ��ṹ.
                               һ��ҳ�����ַ��Ҫ����  32 Byte ���룬����ȡһ��ҳ 4KByte ����.
                               ����ҳ�����ַ��Ҫ����  4 KByte ����.
                               ����ҳ�����ַ��Ҫ����  4 KByte ����.
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
** ��������: armMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuGlobalInit (CPCHAR  pcMachineName)
{
    ARM_PARAM   *param = archKernelParamGet();
    UINT         uiDefaultCfg;
    
    archCacheReset(pcMachineName);

    armMmuInvalidateTLB();

    /*
     *  ��ַ���ѡ�� (Qt �����õ��˷Ƕ���ָ��)
     *  ע ��: ���ʹ�ܵ�ַ������, GCC ���������� -mno-unaligned-access ѡ�� (�����ɷǶ������ָ��)
     */
    if (param->AP_bUnalign) {
        armMmuDisableAlignFault();
        
    } else {
        armMmuEnableAlignFault();                                       /*  -mno-unaligned-access       */
    }
    
    uiDefaultCfg = 0x85000500 | (VMSA_S << 28) | (VMSA_S << 12);
    armMmuV7SetTTBCR(uiDefaultCfg);                                     /*  ֧�� LPAE                   */

    armMmuV7SetMAIR0();
    armMmuV7SetMAIR1();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
             LW_PMD_TRANSENTRY  *p_pmdentry;
             LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = *p_pgdentry;                                               /*  ���һ��ҳ��������          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ULONG)(ulTemp &
                                              ARM_MMU_ADDR_MASK);       /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: armMmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *armMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
             LW_PTE_TRANSENTRY  *p_pteentry;
             LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = *p_pmdentry;                                               /*  ��ö���ҳ��������          */

    p_pteentry = (LW_PTE_TRANSENTRY *)(ULONG)(ulTemp &
                                              ARM_MMU_ADDR_MASK);       /*  �������ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: armMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  armMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armMmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PMD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  armMmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  (((pmdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  armMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry & 0x03) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER ULONG               ulPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  �µ� PGD ����Ч��ҳ����     */

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
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

    *p_pgdentry = armMmuBuildPgdEntry((addr_t)p_pmdentry,             /*  ����һ��ҳ��������          */
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
** ��������: armMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: armMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
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
    
    *p_pmdentry = armMmuBuildPmdEntry((addr_t)p_pteentry,               /*  ��������ҳ��������          */
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
** ��������: armMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: armMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (pteentry & ARM_MMU_ADDR_MASK);                  /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  armMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);  /*  ��ȡһ��������              */
    INT                 iDescType;
    ULONG               ulFlag = 0;

    UINT8               ucXN;                                           /*  ��ִ��Ȩ�ޱ�־              */
    UINT8               ucPXN;                                          /*  ��Ȩ��ִ��Ȩ�ޱ�־          */
    UINT8               ucCon;                                          /*  Contiguous ��־             */
    UINT8               ucnG;                                           /*  nG ��־                     */
    UINT8               ucAF;                                           /*  �Ƿ�ӵ�з���Ȩ�ޱ�־        */
    UINT8               ucSH;                                           /*  ����Ȩ�ޱ�־                */
    UINT8               ucAP;                                           /*  �Ƿ��дȨ�ޱ�־            */
    UINT8               ucNS;                                           /*  Non-Secure ��־             */
    UINT8               ucAIn;

    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == SEGMENT_BASE) {                                    /*  ���ڶε�ӳ��                */
       return  (LW_VMM_FLAG_UNVALID);

    } else if (iDescType == COARSE_TBASE) {                             /*  ��������ҳ��ӳ��            */
        LW_PMD_TRANSENTRY  *p_pmdentry = armMmuPmdOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  ��ȡ����������              */
        if (armMmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                             ulAddr);
                                                                        /*  ��ȡ����������              */
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
** ��������: armMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  armMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);  /*  ��ȡһ��������              */
    INT                 iDescType;
    
    UINT8               ucXN;                                           /*  ��ִ��Ȩ�ޱ�־              */
    UINT8               ucPXN;                                          /*  ��Ȩ��ִ��Ȩ�ޱ�־          */
    UINT8               ucContiguous;                                   /*  Contiguous ��־             */
    UINT8               ucnG;                                           /*  nG ��־                     */
    UINT8               ucAF;                                           /*  �Ƿ�ӵ�з���Ȩ�ޱ�־        */
    UINT8               ucSH;                                           /*  ����Ȩ�ޱ�־                */
    UINT8               ucAP;                                           /*  �Ƿ��дȨ�ޱ�־            */
    UINT8               ucNS;                                           /*  Non-Secure ��־             */
    UINT8               ucAttrIndx;
    UINT8               ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                              /*  ���ʽ�ʧЧ                 */
    }

    if (armMmuFlags2Attr(ulFlag,
                         &ucXN, &ucPXN,
                         &ucContiguous,
                         &ucnG, &ucAF,
                         &ucSH, &ucAP,
                         &ucNS, &ucAttrIndx) < 0) {                     /*  ��Ч��ӳ���ϵ              */
        return (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == SEGMENT_BASE) {                                    /*  ���ڶε�ӳ��                */
        return  (ERROR_NONE);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  ��������ҳ��ӳ��            */
        LW_PMD_TRANSENTRY  *p_pmdentry = armMmuPmdOffset((LW_PGD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  ��ȡ����������              */
        if (armMmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                             ulAddr);
                                                                        /*  ��ȡ����������              */
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
** ��������: armMmuMakeTrans
** ��������: ����ҳ��ӳ���ϵ
** �䡡��  : pmmuctx        mmu ������
**           p_pteentry     ��Ӧ��ҳ����
**           ulVirtualAddr  �����ַ
**           paPhysicalAddr �����ַ
**           ulFlag         ��Ӧ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static VOID  armMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
    UINT8               ucXN;                                           /*  �洢Ȩ��                    */
    UINT8               ucPXN;                                          /*  ��                          */
    UINT8               ucContiguous;                                   /*  CACHE �뻺��������          */
    UINT8               ucnG;                                           /*  �洢Ȩ��                    */
    UINT8               ucAF;                                           /*  CACHE �뻺��������          */
    UINT8               ucSH;                                           /*  ����ִ��λ                  */
    UINT8               ucAP;
    UINT8               ucNS;
    UINT8               ucAttrIndx;
    UINT8               ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }

    if (armMmuFlags2Attr(ulFlag,
                         &ucXN, &ucPXN,
                         &ucContiguous,
                         &ucnG, &ucAF,
                         &ucSH, &ucAP,
                         &ucNS, &ucAttrIndx) < 0) {                     /*  ��Ч��ӳ���ϵ              */
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
    armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 64);           /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: armMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������, ����ʹ�� TTBR1 ҳ���ַ�Ĵ���.
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
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

    armMmuV7SetTTBase((UINT32)p_pgdentry, 0);                           /*  ��λӦ��Ϊ 0               */
    armMmuV7SetTTBase1((UINT32)p_pgdentry, 0);                          /*  ��λӦ��Ϊ 0               */
}
/*********************************************************************************************************
** ��������: armMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        armMmuInvalidateTLB();                                          /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            armMmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: armMmuV7Init
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: armMmuV7ShareableSet
** ��������: MMU ϵͳ share ģʽ����
** �䡡��  : uiInnerOrOuter     0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: armMmuV7ShareableGet
** ��������: MMU ϵͳ share ģʽ��ȡ
** �䡡��  : NONE
** �䡡��  : 0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** ȫ�ֱ���:
** ����ģ��:
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
