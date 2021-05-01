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
** ��   ��   ��: riscvSv32Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 11 ��
**
** ��        ��: RISC-V32 ��ϵ���� Sv32 MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 32
#include "arch/riscv/inc/sbi.h"
#include "riscvMmu.h"
/*********************************************************************************************************
  PTE λ����
*********************************************************************************************************/
#define SV32_MMU_V                  (1)                                 /*  ӳ����Ч                    */
#define SV32_MMU_V_SHIFT            (0)

#define SV32_MMU_R                  (1)                                 /*  �ɶ�                        */
#define SV32_MMU_R_NO               (0)
#define SV32_MMU_R_SHIFT            (1)

#define SV32_MMU_W                  (1)                                 /*  ��д                        */
#define SV32_MMU_W_NO               (0)
#define SV32_MMU_W_SHIFT            (2)

#define SV32_MMU_X                  (1)                                 /*  ��ִ��                      */
#define SV32_MMU_X_NO               (0)
#define SV32_MMU_X_SHIFT            (3)

#define SV32_MMU_U                  (1)                                 /*  �û�ģʽ�ɷ���              */
#define SV32_MMU_U_NO               (0)
#define SV32_MMU_U_SHIFT            (4)

#define SV32_MMU_G                  (1)                                 /*  ȫ��                        */
#define SV32_MMU_G_NO               (0)
#define SV32_MMU_G_SHIFT            (5)

#define SV32_MMU_RSW_ZERO           (0)
#define SV32_MMU_RSW_CACHE          (1)                                 /*  ��д CACHE                  */
#define SV32_MMU_RSW_WT             (2)                                 /*  д��͸ CACHE                */
#define SV32_MMU_RSW_SHIFT          (8)

#define SV32_MMU_A_SHIFT            (6)
#define SV32_MMU_D_SHIFT            (7)

#define SV32_MMU_PPN_MASK           (0xfffffc00ul)                      /*  [31:10]                     */
#define SV32_MMU_PPN_SHIFT          (10)
#define SV32_MMU_PA(ulTemp)         ((ulTemp & SV32_MMU_PPN_MASK) << (LW_CFG_VMM_PAGE_SHIFT - SV32_MMU_PPN_SHIFT))
#define SV32_MMU_PPN(pa)            (((pa) >> (LW_CFG_VMM_PAGE_SHIFT - SV32_MMU_PPN_SHIFT)) & SV32_MMU_PPN_MASK)
/*********************************************************************************************************
  ҳ���������� 1/2 ʱ, ȫ�� TLB
*********************************************************************************************************/
#define SV32_MMU_TLB_NR             (128)                               /*  TLB ��Ŀ                    */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
** ��������: sv32MmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv32MmuEnable (VOID)
{
    UINT32  ulSPTBR = read_csr("sptbr");

    ulSPTBR &= ~(SATP32_MODE);
    ulSPTBR |=  (1UL << 31);                                            /*  Sv32                        */

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv32MmuDisable
** ��������: �ر� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv32MmuDisable (VOID)
{
    UINT32  ulSPTBR = read_csr("sptbr");

    ulSPTBR &= ~(SATP32_MODE);                                          /*  Bare                        */

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv32MmuInvalidateTLBMVA
** ��������: ��Чָ����ַ�� TLB
** �䡡��  : pvAddr        ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv32MmuInvalidateTLBMVA (PVOID  pvAddr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (pvAddr) : "memory");
}
/*********************************************************************************************************
** ��������: sv32MmuInvalidateTLB
** ��������: ��Ч���� TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv32MmuInvalidateTLB (VOID)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}
/*********************************************************************************************************
** ��������: sv32MmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� sv32 MMU Ȩ�ޱ�־
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
static INT  sv32MmuFlags2Attr (ULONG   ulFlag,
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

    *pucV = SV32_MMU_V;                                                 /*  ��Ч                        */
    *pucU = SV32_MMU_U_NO;                                              /*  User ���ܷ���               */
    *pucG = SV32_MMU_G;                                                 /*  ȫ��ӳ��                    */

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ɷ���                  */
        *pucR = SV32_MMU_R;

    } else {
        *pucR = SV32_MMU_R_NO;
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucW = SV32_MMU_W;

    } else {
        *pucW = SV32_MMU_W_NO;
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {                                /*  �Ƿ��ִ��                  */
        *pucX = SV32_MMU_X;

    } else {
        *pucX = SV32_MMU_X_NO;
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        *pucRSW = SV32_MMU_RSW_CACHE;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸ CACHE                */
        *pucRSW = SV32_MMU_RSW_WT;

    } else {
        *pucRSW = SV32_MMU_RSW_ZERO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv32MmuAttr2Flags
** ��������: ���� sv32 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
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
static INT  sv32MmuAttr2Flags (UINT8   ucV,
                               UINT8   ucR,
                               UINT8   ucW,
                               UINT8   ucX,
                               UINT8   ucU,
                               UINT8   ucG,
                               UINT8   ucRSW,
                               ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucR == SV32_MMU_R) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }

    if (ucW == SV32_MMU_W) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }

    if (ucX == SV32_MMU_X) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    if (ucRSW & SV32_MMU_RSW_CACHE) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

    } else if (ucRSW & SV32_MMU_RSW_WT) {
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv32MmuBuildPgdEntry
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
static LW_INLINE LW_PGD_TRANSENTRY  sv32MmuBuildPgdEntry (addr_t  ulBaseAddr,
                                                          UINT8   ucV,
                                                          UINT8   ucR,
                                                          UINT8   ucW,
                                                          UINT8   ucX,
                                                          UINT8   ucU,
                                                          UINT8   ucG,
                                                          UINT8   ucRSW)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (SV32_MMU_PPN(ulBaseAddr))
                 | (ucR << SV32_MMU_A_SHIFT)
                 | (ucW << SV32_MMU_D_SHIFT)
                 | (ucV << SV32_MMU_V_SHIFT)
                 | (ucR << SV32_MMU_R_SHIFT)
                 | (ucW << SV32_MMU_W_SHIFT)
                 | (ucX << SV32_MMU_X_SHIFT)
                 | (ucU << SV32_MMU_U_SHIFT)
                 | (ucG << SV32_MMU_G_SHIFT)
                 | ((ULONG)ucRSW << SV32_MMU_RSW_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: sv32MmuBuildPteEntry
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
static LW_INLINE LW_PTE_TRANSENTRY  sv32MmuBuildPteEntry (addr_t  ulBaseAddr,
                                                          UINT8   ucV,
                                                          UINT8   ucR,
                                                          UINT8   ucW,
                                                          UINT8   ucX,
                                                          UINT8   ucU,
                                                          UINT8   ucG,
                                                          UINT8   ucRSW)
{
    LW_PTE_TRANSENTRY  ulDescriptor;

    ulDescriptor = (SV32_MMU_PPN(ulBaseAddr))
                 | (ucR << SV32_MMU_A_SHIFT)
                 | (ucW << SV32_MMU_D_SHIFT)
                 | (ucR << SV32_MMU_V_SHIFT)
                 | (ucR << SV32_MMU_R_SHIFT)
                 | (ucW << SV32_MMU_W_SHIFT)
                 | (ucX << SV32_MMU_X_SHIFT)
                 | (ucU << SV32_MMU_U_SHIFT)
                 | (ucG << SV32_MMU_G_SHIFT)
                 | ((ULONG)ucRSW << SV32_MMU_RSW_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: sv32MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv32MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

    PVOID  pvPgdTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
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
** ��������: sv32MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv32MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);

    sv32MmuInvalidateTLB();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv32MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *sv32MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: sv32MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *sv32MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  Sv32 �� PMD ��              */
}
/*********************************************************************************************************
** ��������: sv32MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pts ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *sv32MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ���һ��ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(SV32_MMU_PA(ulTemp));            /*  ��ö���ҳ�����ַ          */

    ulAddr    &= 0x3fful << LW_CFG_VMM_PAGE_SHIFT;                      /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: sv32MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  sv32MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (SV32_MMU_V << SV32_MMU_V_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sv32MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  sv32MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (SV32_MMU_V << SV32_MMU_V_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sv32MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *sv32MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

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
** ��������: sv32MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv32MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: sv32MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *sv32MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PGD_TRANSENTRY  *p_pgdentry,
                                           addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: sv32MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv32MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: sv32MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *sv32MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                            LW_PMD_TRANSENTRY  *p_pmdentry,
                                            addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    *p_pmdentry = sv32MmuBuildPgdEntry((addr_t)p_pteentry,
                                       SV32_MMU_V,
                                       SV32_MMU_R_NO,
                                       SV32_MMU_W_NO,
                                       SV32_MMU_X_NO,
                                       SV32_MMU_U_NO,
                                       SV32_MMU_G_NO,
                                       SV32_MMU_RSW_ZERO);              /*  ���ö���ҳ��������          */

    return  (sv32MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: sv32MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv32MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: sv32MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sv32MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(SV32_MMU_PA(pteentry));                 /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sv32MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  sv32MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = sv32MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv32MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = sv32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */
        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (sv32MmuPteIsOk(pteentry)) {                                 /*  ������������Ч              */
            UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;
            ULONG  ulFlag;

            ucV   = (UINT8)((pteentry >> SV32_MMU_V_SHIFT)   & 0x01);
            ucR   = (UINT8)((pteentry >> SV32_MMU_R_SHIFT)   & 0x01);
            ucW   = (UINT8)((pteentry >> SV32_MMU_W_SHIFT)   & 0x01);
            ucX   = (UINT8)((pteentry >> SV32_MMU_X_SHIFT)   & 0x01);
            ucU   = (UINT8)((pteentry >> SV32_MMU_U_SHIFT)   & 0x01);
            ucG   = (UINT8)((pteentry >> SV32_MMU_G_SHIFT)   & 0x01);
            ucRSW = (UINT8)((pteentry >> SV32_MMU_RSW_SHIFT) & 0x03);

            sv32MmuAttr2Flags(ucV, ucR, ucW, ucX, ucU, ucG, ucRSW, &ulFlag);

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: sv32MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  sv32MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;

    if (sv32MmuFlags2Attr(ulFlag, &ucV, &ucR, &ucW,
                          &ucX, &ucU, &ucG, &ucRSW) != ERROR_NONE) {    /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = sv32MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv32MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = sv32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */
        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (sv32MmuPteIsOk(pteentry)) {                                 /*  ������������Ч              */
            addr_t  ulPhysicalAddr = (addr_t)(SV32_MMU_PA(pteentry));

            *p_pteentry = sv32MmuBuildPteEntry(ulPhysicalAddr,
                                               ucV, ucR, ucW, ucX, ucU, ucG, ucRSW);
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sv32MmuMakeTrans
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
static VOID  sv32MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                               LW_PTE_TRANSENTRY  *p_pteentry,
                               addr_t              ulVirtualAddr,
                               phys_addr_t         paPhysicalAddr,
                               ULONG               ulFlag)
{
    UINT8  ucV, ucR, ucW, ucX, ucU, ucG, ucRSW;
    
    if (sv32MmuFlags2Attr(ulFlag, &ucV, &ucR, &ucW,
                          &ucX, &ucU, &ucG, &ucRSW) != ERROR_NONE) {    /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = sv32MmuBuildPteEntry((addr_t)paPhysicalAddr,
                                       ucV, ucR, ucW, ucX, ucU, ucG, ucRSW);
}
/*********************************************************************************************************
** ��������: sv32MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sv32MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ULONG  ulSPTBR = read_csr("sptbr");

    ulSPTBR = ((ULONG)pmmuctx->MMUCTX_pgdEntry) >> LW_CFG_VMM_PAGE_SHIFT;

    write_csr("sptbr", ulSPTBR);
}
/*********************************************************************************************************
** ��������: sv32MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sv32MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (SV32_MMU_TLB_NR >> 1)) {
        sv32MmuInvalidateTLB();                                         /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            sv32MmuInvalidateTLBMVA((PVOID)ulPageAddr);                 /*  ���ҳ����� TLB            */
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
    LW_PGD_TRANSENTRY  *p_pgdentry = sv32MmuPgdOffset(pmmuctx, ulAddr); /*  ���һ����������ַ          */

    if (p_pgdentry && sv32MmuPgdIsOk(*p_pgdentry)) {                    /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = sv32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                          ulAddr);      /*  ��ö�����������ַ          */
        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (sv32MmuPteIsOk(pteentry)) {                                 /*  ������������Ч              */
            addr_t  ulPhysicalAddr = (addr_t)(SV32_MMU_PA(pteentry));

            if (ulPhysicalAddr) {
                if (uiMethod == LW_VMM_ABORT_METHOD_READ) {
                    UINT8  ucR;

                    ucR = (UINT8)((pteentry >> SV32_MMU_R_SHIFT) & 0x01);
                    if (!ucR) {
                        return  (LW_VMM_ABORT_TYPE_PERM);
                    }

                } else if (uiMethod == LW_VMM_ABORT_METHOD_WRITE) {
                    UINT8  ucW;

                    ucW = (UINT8)((pteentry >> SV32_MMU_W_SHIFT) & 0x01);
                    if (!ucW) {
                        return  (LW_VMM_ABORT_TYPE_PERM);
                    }

                } else {
                    UINT8  ucX;

                    ucX = (UINT8)((pteentry >> SV32_MMU_X_SHIFT) & 0x01);
                    if (!ucX) {
                        return  (LW_VMM_ABORT_TYPE_PERM);
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

    pmmuop->MMUOP_pfuncMemInit    = sv32MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = sv32MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = sv32MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = sv32MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = sv32MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = sv32MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = sv32MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = sv32MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = sv32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = sv32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = sv32MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = sv32MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = sv32MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = sv32MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = sv32MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = sv32MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = sv32MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = sv32MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = sv32MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = sv32MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = sv32MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = sv32MmuDisable;
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
