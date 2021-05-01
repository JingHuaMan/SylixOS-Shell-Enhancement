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
** ��   ��   ��: riscvSv39Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 11 ��
**
** ��        ��: RISC-V64 ��ϵ���� Sv39 MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if (LW_CFG_CPU_WORD_LENGHT == 64) && (LW_CFG_RISCV_MMU_SV39 > 0)
#include "arch/riscv/inc/sbi.h"
#include "riscvMmu.h"
/*********************************************************************************************************
  PTE λ����
*********************************************************************************************************/
#define SV39_MMU_V                  (1)                                 /*  ӳ����Ч                    */
#define SV39_MMU_V_SHIFT            (0)

#define SV39_MMU_R                  (1)                                 /*  �ɶ�                        */
#define SV39_MMU_R_NO               (0)
#define SV39_MMU_R_SHIFT            (1)

#define SV39_MMU_W                  (1)                                 /*  ��д                        */
#define SV39_MMU_W_NO               (0)
#define SV39_MMU_W_SHIFT            (2)

#define SV39_MMU_X                  (1)                                 /*  ��ִ��                      */
#define SV39_MMU_X_NO               (0)
#define SV39_MMU_X_SHIFT            (3)

#define SV39_MMU_U                  (1)                                 /*  �û�ģʽ�ɷ���              */
#define SV39_MMU_U_NO               (0)
#define SV39_MMU_U_SHIFT            (4)

#define SV39_MMU_G                  (1)                                 /*  ȫ��                        */
#define SV39_MMU_G_NO               (0)
#define SV39_MMU_G_SHIFT            (5)

#define SV39_MMU_RSW_ZERO           (0)
#define SV39_MMU_RSW_CACHE          (1)                                 /*  ��д CACHE                  */
#define SV39_MMU_RSW_WT             (2)                                 /*  д��͸ CACHE                */
#define SV39_MMU_RSW_SHIFT          (8)

#define SV39_MMU_A_SHIFT            (6)
#define SV39_MMU_D_SHIFT            (7)

#define SV39_MMU_PPN_MASK           (0x3ffffffffffc00ul)                /*  [53:10]                     */
#define SV39_MMU_PPN_SHIFT          (10)
#define SV39_MMU_PA(ulTemp)         ((ulTemp & SV39_MMU_PPN_MASK) << (LW_CFG_VMM_PAGE_SHIFT - SV39_MMU_PPN_SHIFT))
#define SV39_MMU_PPN(pa)            (((pa) >> (LW_CFG_VMM_PAGE_SHIFT - SV39_MMU_PPN_SHIFT)) & SV39_MMU_PPN_MASK)
/*********************************************************************************************************
  ҳ���������� 1/2 ʱ, ȫ�� TLB
*********************************************************************************************************/
#define SV39_MMU_TLB_NR             (32)                                /*  TLB ��Ŀ                    */
/*********************************************************************************************************
  �����ַ�Ĵ�С
*********************************************************************************************************/
#define SV39_MMU_VIRT_ADDR_SIZE     (39)                                /*  39 λ�����ַ               */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD ������                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
** ��������: sv39MmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv39MmuEnable (VOID)
{
    ULONG  ulSPTBR = read_csr("sptbr");

    ulSPTBR &= ~(SATP64_MODE);
    ulSPTBR |=  (8ULL << 60);                                           /*  Sv39                        */

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv39MmuDisable
** ��������: �ر� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv39MmuDisable (VOID)
{
    ULONG  ulSPTBR = read_csr("sptbr");

    ulSPTBR &= ~(SATP64_MODE);                                          /*  Bare                        */

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv39MmuInvalidateTLBMVA
** ��������: ��Чָ����ַ�� TLB
** �䡡��  : pvAddr        ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv39MmuInvalidateTLBMVA (PVOID  pvAddr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (pvAddr) : "memory");
}
/*********************************************************************************************************
** ��������: sv39MmuInvalidateTLB
** ��������: ��Ч���� TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv39MmuInvalidateTLB (VOID)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}
/*********************************************************************************************************
** ��������: sv39MmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� sv39 MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  SylixOS Ȩ�ޱ�־
**           pucV                    �Ƿ���Ч
**           pucR                    �Ƿ�ɶ�
**           pucW                    �Ƿ��д
**           pucX                    �Ƿ�ִ��
**           pucU                    �Ƿ��û��ܷ���
**           pucG                    �Ƿ�ȫ��
**           pucRSW                  RSW
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv39MmuFlags2Attr (ULONG   ulFlag,
                               UINT8  *pucV,
                               UINT8  *pucR,
                               UINT8  *pucW,
                               UINT8  *pucX,
                               UINT8  *pucU,
                               UINT8  *pucG,
                               UINT8  *pucRSW)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    *pucV = SV39_MMU_V;                                                 /*  ��Ч                        */
    *pucU = SV39_MMU_U_NO;                                              /*  User ���ܷ���               */
    *pucG = SV39_MMU_G;                                                 /*  ȫ��ӳ��                    */

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ɷ���                  */
        *pucR = SV39_MMU_R;

    } else {
        *pucR = SV39_MMU_R_NO;
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucW = SV39_MMU_W;

    } else {
        *pucW = SV39_MMU_W_NO;
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {                                /*  �Ƿ��ִ��                  */
        *pucX = SV39_MMU_X;

    } else {
        *pucX = SV39_MMU_X_NO;
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        *pucRSW = SV39_MMU_RSW_CACHE;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸ CACHE                */
        *pucRSW = SV39_MMU_RSW_WT;

    } else {
        *pucRSW = SV39_MMU_RSW_ZERO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv39MmuAttr2Flags
** ��������: ���� sv39 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucV                     �Ƿ���Ч
**           ucR                     �Ƿ�ɶ�
**           ucW                     �Ƿ��д
**           ucX                     �Ƿ�ִ��
**           ucU                     �Ƿ��û��ܷ���
**           ucG                     �Ƿ�ȫ��
**           ucRSW                   RSW
**           pulFlag                 SylixOS Ȩ�ޱ�־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv39MmuAttr2Flags (UINT8   ucV,
                               UINT8   ucR,
                               UINT8   ucW,
                               UINT8   ucX,
                               UINT8   ucU,
                               UINT8   ucG,
                               UINT8   ucRSW,
                               ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucR == SV39_MMU_R) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }

    if (ucW == SV39_MMU_W) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }

    if (ucX == SV39_MMU_X) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    if (ucRSW & SV39_MMU_RSW_CACHE) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

    } else if (ucRSW & SV39_MMU_RSW_WT) {
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv39MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucV                     �Ƿ���Ч
**           ucR                     �Ƿ�ɶ�
**           ucW                     �Ƿ��д
**           ucX                     �Ƿ�ִ��
**           ucU                     �Ƿ��û��ܷ���
**           ucG                     �Ƿ�ȫ��
**           ucRSW                   RSW
** �䡡��  : һ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE LW_PGD_TRANSENTRY  sv39MmuBuildPgdEntry (addr_t  ulBaseAddr,
                                                          UINT8   ucV,
                                                          UINT8   ucR,
                                                          UINT8   ucW,
                                                          UINT8   ucX,
                                                          UINT8   ucU,
                                                          UINT8   ucG,
                                                          UINT8   ucRSW)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (SV39_MMU_PPN(ulBaseAddr))
                 | (ucR << SV39_MMU_A_SHIFT)
                 | (ucW << SV39_MMU_D_SHIFT)
                 | (ucV << SV39_MMU_V_SHIFT)
                 | (ucR << SV39_MMU_R_SHIFT)
                 | (ucW << SV39_MMU_W_SHIFT)
                 | (ucX << SV39_MMU_X_SHIFT)
                 | (ucU << SV39_MMU_U_SHIFT)
                 | (ucG << SV39_MMU_G_SHIFT)
                 | ((ULONG)ucRSW << SV39_MMU_RSW_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: sv39MmuBuildPmdEntry
** ��������: ����һ������������ (PMD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucV                     �Ƿ���Ч
**           ucR                     �Ƿ�ɶ�
**           ucW                     �Ƿ��д
**           ucX                     �Ƿ�ִ��
**           ucU                     �Ƿ��û��ܷ���
**           ucG                     �Ƿ�ȫ��
**           ucRSW                   RSW
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PMD_TRANSENTRY  sv39MmuBuildPmdEntry (addr_t  ulBaseAddr,
                                                          UINT8   ucV,
                                                          UINT8   ucR,
                                                          UINT8   ucW,
                                                          UINT8   ucX,
                                                          UINT8   ucU,
                                                          UINT8   ucG,
                                                          UINT8   ucRSW)
{
    LW_PMD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (SV39_MMU_PPN(ulBaseAddr))
                 | (ucR << SV39_MMU_A_SHIFT)
                 | (ucW << SV39_MMU_D_SHIFT)
                 | (ucV << SV39_MMU_V_SHIFT)
                 | (ucR << SV39_MMU_R_SHIFT)
                 | (ucW << SV39_MMU_W_SHIFT)
                 | (ucX << SV39_MMU_X_SHIFT)
                 | (ucU << SV39_MMU_U_SHIFT)
                 | (ucG << SV39_MMU_G_SHIFT)
                 | ((ULONG)ucRSW << SV39_MMU_RSW_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: sv39MmuBuildPteEntry
** ��������: ����һ���ļ������� (PTE ������)
** �䡡��  : ulBaseAddr              ����ַ     (ҳ����ַ)
**           ucV                     �Ƿ���Ч
**           ucR                     �Ƿ�ɶ�
**           ucW                     �Ƿ��д
**           ucX                     �Ƿ�ִ��
**           ucU                     �Ƿ��û��ܷ���
**           ucG                     �Ƿ�ȫ��
**           ucRSW                   RSW
** �䡡��  : �ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PTE_TRANSENTRY  sv39MmuBuildPteEntry (addr_t  ulBaseAddr,
                                                          UINT8   ucV,
                                                          UINT8   ucR,
                                                          UINT8   ucW,
                                                          UINT8   ucX,
                                                          UINT8   ucU,
                                                          UINT8   ucG,
                                                          UINT8   ucRSW)
{
    LW_PTE_TRANSENTRY  ulDescriptor;

    ulDescriptor = (SV39_MMU_PPN(ulBaseAddr))
                 | (ucR << SV39_MMU_A_SHIFT)
                 | (ucW << SV39_MMU_D_SHIFT)
                 | (ucR << SV39_MMU_V_SHIFT)
                 | (ucR << SV39_MMU_R_SHIFT)
                 | (ucW << SV39_MMU_W_SHIFT)
                 | (ucX << SV39_MMU_X_SHIFT)
                 | (ucU << SV39_MMU_U_SHIFT)
                 | (ucG << SV39_MMU_G_SHIFT)
                 | ((ULONG)ucRSW << SV39_MMU_RSW_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: sv39MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv39MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

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
** ��������: sv39MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv39MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);

    sv39MmuInvalidateTLB();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv39MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *sv39MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    if (ulAddr & (~((1ULL << SV39_MMU_VIRT_ADDR_SIZE) - 1))) {          /*  ���ںϷ��������ַ�ռ���    */
        return  (LW_NULL);
    }

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *sv39MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = (LW_PGD_TRANSENTRY)(*p_pgdentry);                          /*  ���һ��ҳ��������          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(SV39_MMU_PA(ulTemp));            /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *sv39MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ��ö���ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(SV39_MMU_PA(ulTemp));            /*  �������ҳ�����ַ          */

    ulAddr    &= 0x1fful << LW_CFG_VMM_PAGE_SHIFT;                      /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  sv39MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (SV39_MMU_V << SV39_MMU_V_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sv39MmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PMD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  sv39MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  ((pmdentry & (SV39_MMU_V << SV39_MMU_V_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sv39MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  sv39MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (SV39_MMU_V << SV39_MMU_V_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sv39MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *sv39MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    if (ulAddr & (~((1ULL << SV39_MMU_VIRT_ADDR_SIZE) - 1))) {          /*  ���ںϷ��������ַ�ռ���    */
        return  (LW_NULL);
    }

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv39MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *sv39MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PGD_TRANSENTRY  *p_pgdentry,
                                           addr_t              ulAddr)
{
    LW_PMD_TRANSENTRY  *p_pmdentry = (LW_PMD_TRANSENTRY *)API_PartitionGet(_G_hPMDPartition);

    if (!p_pmdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pmdentry, PMD_BLOCK_SIZE);

    *p_pgdentry = sv39MmuBuildPgdEntry((addr_t)p_pmdentry,
                                       SV39_MMU_V,
                                       SV39_MMU_R_NO,
                                       SV39_MMU_W_NO,
                                       SV39_MMU_X_NO,
                                       SV39_MMU_U_NO,
                                       SV39_MMU_G_NO,
                                       SV39_MMU_RSW_ZERO);              /*  ����һ��ҳ��������          */

    return  (sv39MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: sv39MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv39MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *sv39MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                            LW_PMD_TRANSENTRY  *p_pmdentry,
                                            addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    *p_pmdentry = sv39MmuBuildPmdEntry((addr_t)p_pteentry,
                                       SV39_MMU_V,
                                       SV39_MMU_R_NO,
                                       SV39_MMU_W_NO,
                                       SV39_MMU_X_NO,
                                       SV39_MMU_U_NO,
                                       SV39_MMU_G_NO,
                                       SV39_MMU_RSW_ZERO);              /*  ���ö���ҳ��������          */

    return  (sv39MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: sv39MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv39MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: sv39MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv39MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(SV39_MMU_PA(pteentry));                 /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv39MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  sv39MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = sv39MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv39MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sv39MmuPmdOffset(p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */

        if (sv39MmuPmdIsOk(*p_pmdentry)) {                              /*  ������������Ч              */
            LW_PTE_TRANSENTRY  *p_pteentry = sv39MmuPteOffset(p_pmdentry,
                                                            ulAddr);    /*  ���������������ַ          */
            LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                 /*  �������������              */

            if (sv39MmuPteIsOk(pteentry)) {                             /*  ������������Ч              */
                UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;
                ULONG  ulFlag;

                ucV   = (UINT8)((pteentry >> SV39_MMU_V_SHIFT)   & 0x01);
                ucR   = (UINT8)((pteentry >> SV39_MMU_R_SHIFT)   & 0x01);
                ucW   = (UINT8)((pteentry >> SV39_MMU_W_SHIFT)   & 0x01);
                ucX   = (UINT8)((pteentry >> SV39_MMU_X_SHIFT)   & 0x01);
                ucU   = (UINT8)((pteentry >> SV39_MMU_U_SHIFT)   & 0x01);
                ucG   = (UINT8)((pteentry >> SV39_MMU_G_SHIFT)   & 0x01);
                ucRSW = (UINT8)((pteentry >> SV39_MMU_RSW_SHIFT) & 0x03);

                sv39MmuAttr2Flags(ucV, ucR, ucW, ucX, ucU, ucG, ucRSW, &ulFlag);

                return  (ulFlag);
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: sv39MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  sv39MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;

    if (sv39MmuFlags2Attr(ulFlag, &ucV, &ucR, &ucW,
                          &ucX, &ucU, &ucG, &ucRSW) != ERROR_NONE) {    /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = sv39MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv39MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sv39MmuPmdOffset(p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */

        if (sv39MmuPmdIsOk(*p_pmdentry)) {                              /*  ������������Ч              */
            LW_PTE_TRANSENTRY  *p_pteentry = sv39MmuPteOffset(p_pmdentry,
                                                              ulAddr);  /*  ���������������ַ          */
            LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                 /*  �������������              */

            if (sv39MmuPteIsOk(pteentry)) {                             /*  ������������Ч              */
                addr_t  ulPhysicalAddr = (addr_t)(SV39_MMU_PA(pteentry));

                *p_pteentry = sv39MmuBuildPteEntry(ulPhysicalAddr,
                                                   ucV, ucR, ucW, ucX, ucU, ucG, ucRSW);

                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sv39MmuMakeTrans
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
static VOID  sv39MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                               LW_PTE_TRANSENTRY  *p_pteentry,
                               addr_t              ulVirtualAddr,
                               phys_addr_t         paPhysicalAddr,
                               ULONG               ulFlag)
{
    UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;
    
    if (sv39MmuFlags2Attr(ulFlag, &ucV, &ucR, &ucW,
                          &ucX, &ucU, &ucG, &ucRSW) != ERROR_NONE) {    /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = sv39MmuBuildPteEntry((addr_t)paPhysicalAddr,
                                       ucV, ucR, ucW, ucX, ucU, ucG, ucRSW);
}
/*********************************************************************************************************
** ��������: sv39MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv39MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ULONG  ulSPTBR = read_csr("sptbr");

    ulSPTBR = ((ULONG)pmmuctx->MMUCTX_pgdEntry) >> LW_CFG_VMM_PAGE_SHIFT;

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv39MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv39MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (SV39_MMU_TLB_NR >> 1)) {
        sv39MmuInvalidateTLB();                                         /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            sv39MmuInvalidateTLBMVA((PVOID)ulPageAddr);                 /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: riscvMmuAbortType
** ��������: ��÷�����ֹ����
** �䡡��  : ulAddr        ��ֹ��ַ
**           uiMethod      ���ʷ���(LW_VMM_ABORT_METHOD_XXX)
** �䡡��  : ������ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  riscvMmuAbortType (addr_t  ulAddr, UINT  uiMethod)
{
    PLW_MMU_CONTEXT     pmmuctx    = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry = sv39MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv39MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sv39MmuPmdOffset(p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */

        if (sv39MmuPmdIsOk(*p_pmdentry)) {                              /*  ������������Ч              */
            LW_PTE_TRANSENTRY  *p_pteentry = sv39MmuPteOffset(p_pmdentry,
                                                              ulAddr);  /*  ���������������ַ          */
            LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                 /*  �������������              */

            if (sv39MmuPteIsOk(pteentry)) {                             /*  ������������Ч              */
                addr_t  ulPhysicalAddr = (addr_t)(SV39_MMU_PA(pteentry));

                if (ulPhysicalAddr) {
                    if (uiMethod == LW_VMM_ABORT_METHOD_READ) {
                        UINT8  ucR;

                        ucR = (UINT8)((pteentry >> SV39_MMU_R_SHIFT) & 0x01);
                        if (!ucR) {
                            return  (LW_VMM_ABORT_TYPE_PERM);
                        }

                    } else if (uiMethod == LW_VMM_ABORT_METHOD_WRITE) {
                        UINT8  ucW;

                        ucW = (UINT8)((pteentry >> SV39_MMU_W_SHIFT) & 0x01);
                        if (!ucW) {
                            return  (LW_VMM_ABORT_TYPE_PERM);
                        }

                    } else {
                        UINT8  ucX;

                        ucX = (UINT8)((pteentry >> SV39_MMU_X_SHIFT) & 0x01);
                        if (!ucX) {
                            return  (LW_VMM_ABORT_TYPE_PERM);
                        }
                    }
                }
            }
        }
    }

    return  (LW_VMM_ABORT_TYPE_MAP);
}
/*********************************************************************************************************
** ��������: riscvMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  riscvMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;

    pmmuop->MMUOP_pfuncMemInit    = sv39MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = sv39MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = sv39MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = sv39MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = sv39MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = sv39MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = sv39MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = sv39MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = sv39MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = sv39MmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = sv39MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = sv39MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = sv39MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = sv39MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = sv39MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = sv39MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = sv39MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = sv39MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = sv39MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = sv39MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = sv39MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = sv39MmuDisable;
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
