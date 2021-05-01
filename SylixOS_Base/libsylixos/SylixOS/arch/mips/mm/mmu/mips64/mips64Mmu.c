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
** ��   ��   ��: mips64Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 30 ��
**
** ��        ��: MIPS64 ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 64
#include "arch/mips/common/cp0/mipsCp0.h"
#include "../mipsMmuCommon.h"
#include "./mips64MmuAlgorithm.h"
/*********************************************************************************************************
  ENTRYLO PFN ����
*********************************************************************************************************/
#define MIPS64_ENTRYLO_PFN_MASK         (0xfffffffffull << MIPS_ENTRYLO_PFN_SHIFT)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD ������                  */
static LW_OBJECT_HANDLE     _G_hPTSPartition;                           /*  PTS ������                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
** ��������: mips64MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : һ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PGD_TRANSENTRY  mips64MmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: mips64MmuBuildPmdEntry
** ��������: ����һ������������ (PMD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PMD_TRANSENTRY  mips64MmuBuildPmdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  ������������������ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: mips64MmuBuildPtsEntry
** ��������: ����һ������������ (PTS ������)
** �䡡��  : ulBaseAddr              �ļ�ҳ�����ַ
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PTS_TRANSENTRY  mips64MmuBuildPtsEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  ���������������ļ�ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: mips64MmuBuildPteEntry
** ��������: ����һ���ļ������� (PTE ������)
** �䡡��  : ulBaseAddr              ����ҳ��ַ
**           ulFlag                  ��־
** �䡡��  : �ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  mips64MmuBuildPteEntry (addr_t  ulBaseAddr, ULONG  ulFlag)
{
    LW_PTE_TRANSENTRY   pteentry;
    ULONG               ulPFN;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulPFN = ulBaseAddr >> 12;                                       /*  ���� PFN                    */

        pteentry = ulPFN << MIPS_ENTRYLO_PFN_SHIFT;                     /*  ��� PFN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            pteentry |= ENTRYLO_V;                                      /*  ��� V λ                   */
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            pteentry |= ENTRYLO_D;                                      /*  ��� D λ                   */
        }

        pteentry |= ENTRYLO_G;                                          /*  ��� G λ                   */

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                           /*  ��� C λ                   */
            pteentry |= MIPS_MMU_ENTRYLO_CACHE << ENTRYLO_C_SHIFT;

        } else if (ulFlag & LW_VMM_FLAG_WRITECOMBINING) {               /*  д�ϲ�                      */
            pteentry |= MIPS_MMU_ENTRYLO_UNCACHE_WB << ENTRYLO_C_SHIFT;

        } else {
            pteentry |= MIPS_MMU_ENTRYLO_UNCACHE << ENTRYLO_C_SHIFT;
        }

        if (MIPS_MMU_HAS_XI) {
            if (!(ulFlag & LW_VMM_FLAG_EXECABLE)) {                     /*  ����ִ��                    */
                pteentry |= MIPS_ENTRYLO_XI;                            /*  ��� XI λ                  */
            }
        }
    } else {
        pteentry = 0;
    }

    return  (pteentry);
}
/*********************************************************************************************************
** ��������: mips64MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mips64MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPtsTable;
    PVOID  pvPteTable;

    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPtsNum = bspMmuPtsMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * LW_CFG_VMM_PGD_BLKSIZE, LW_CFG_VMM_PGD_BLKSIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * LW_CFG_VMM_PMD_BLKSIZE, LW_CFG_VMM_PMD_BLKSIZE);
    pvPtsTable = __KHEAP_ALLOC_ALIGN((size_t)ulPtsNum * LW_CFG_VMM_PTS_BLKSIZE, LW_CFG_VMM_PTS_BLKSIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * LW_CFG_VMM_PTE_BLKSIZE, LW_CFG_VMM_PTE_BLKSIZE);

    if (!pvPgdTable || !pvPmdTable || !pvPtsTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, LW_CFG_VMM_PGD_BLKSIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPMDPartition = API_PartitionCreate("pmd_pool", pvPmdTable, ulPmdNum, LW_CFG_VMM_PMD_BLKSIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTSPartition = API_PartitionCreate("pts_pool", pvPtsTable, ulPtsNum, LW_CFG_VMM_PTS_BLKSIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, LW_CFG_VMM_PTE_BLKSIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);

    if (!_G_hPGDPartition || !_G_hPMDPartition || !_G_hPTSPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips64MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *mips64MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: mips64MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *mips64MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = *p_pgdentry;                                               /*  ���һ��ҳ��������          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ulTemp);                         /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPtsOffset
** ��������: ͨ�������ַ���� PTS ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTS �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY  *mips64MmuPtsOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTS_TRANSENTRY  *p_ptsentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPtsNum;

    ulTemp = *p_pmdentry;                                               /*  ��ö���ҳ��������          */

    p_ptsentry = (LW_PTS_TRANSENTRY *)(ulTemp);                         /*  �������ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTS_MASK;
    ulPtsNum   = ulAddr >> LW_CFG_VMM_PTS_SHIFT;                        /*  ���� PTS ��                 */

    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry |
                 (ulPtsNum * sizeof(LW_PTS_TRANSENTRY)));               /*  �������ҳ����������ַ      */

    return  (p_ptsentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_ptsentry     pts ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *mips64MmuPteOffset (LW_PTS_TRANSENTRY  *p_ptsentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PTS_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = *p_ptsentry;                                               /*  �������ҳ��������          */

    p_pteentry = (LW_PTE_TRANSENTRY *)(ulTemp);                         /*  ����ļ�ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;                                   /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PTE_SHIFT;                        /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  mips64MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips64MmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PMD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  mips64MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  (pmdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips64MmuPtsIsOk
** ��������: �ж� PTS ����������Ƿ���ȷ
** �䡡��  : ptsentry       PTS ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  mips64MmuPtsIsOk (LW_PTS_TRANSENTRY  ptsentry)
{
    return  (ptsentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips64MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  mips64MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & ENTRYLO_V) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips64MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *mips64MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, LW_CFG_VMM_PGD_BLKSIZE);

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips64MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(LW_CFG_VMM_PGD_BLKSIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *mips64MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PGD_TRANSENTRY  *p_pgdentry,
                                              addr_t              ulAddr)
{
    LW_PMD_TRANSENTRY  *p_pmdentry = (LW_PMD_TRANSENTRY *)API_PartitionGet(_G_hPMDPartition);

    if (!p_pmdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pmdentry, LW_CFG_VMM_PMD_BLKSIZE);

    *p_pgdentry = mips64MmuBuildPgdEntry((addr_t)p_pmdentry);           /*  ����һ��ҳ��������          */

    return  (mips64MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: mips64MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips64MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(LW_CFG_VMM_PMD_BLKSIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPtsAlloc
** ��������: ���� PTS ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTS ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY  *mips64MmuPtsAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    LW_PTS_TRANSENTRY  *p_ptsentry = (LW_PTS_TRANSENTRY *)API_PartitionGet(_G_hPTSPartition);

    if (!p_ptsentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_ptsentry, LW_CFG_VMM_PTS_BLKSIZE);

    *p_pmdentry = mips64MmuBuildPmdEntry((addr_t)p_ptsentry);           /*  ���ö���ҳ��������          */

    return  (mips64MmuPtsOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: mips64MmuPtsFree
** ��������: �ͷ� PTS ��
** �䡡��  : p_ptsentry     pts ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips64MmuPtsFree (LW_PTS_TRANSENTRY  *p_ptsentry)
{
    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry & (~(LW_CFG_VMM_PTS_BLKSIZE - 1)));

    API_PartitionPut(_G_hPTSPartition, (PVOID)p_ptsentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���:
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *mips64MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PTS_TRANSENTRY  *p_ptsentry,
                                              addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, LW_CFG_VMM_PTE_BLKSIZE);

    *p_ptsentry = mips64MmuBuildPtsEntry((addr_t)p_pteentry);           /*  ��������ҳ��������          */

    return  (mips64MmuPteOffset(p_ptsentry, ulAddr));
}
/*********************************************************************************************************
** ��������: mips64MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips64MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(LW_CFG_VMM_PTE_BLKSIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: mips64MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mips64MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    ULONG  ulPFN = (pteentry & MIPS64_ENTRYLO_PFN_MASK) >>
                    MIPS_ENTRYLO_PFN_SHIFT;                             /*  �������ҳ���              */

    *ppaPhysicalAddr = ulPFN << 12;                                     /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips64MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  mips64MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PMD_TRANSENTRY  *p_pmdentry;
    LW_PTS_TRANSENTRY  *p_ptsentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    p_pgdentry = mips64MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */
    if (p_pgdentry && mips64MmuPgdIsOk(*p_pgdentry)) {                  /*  һ����������Ч              */

        p_pmdentry = mips64MmuPmdOffset(p_pgdentry, ulAddr);            /*  ��ö�����������ַ          */
        if (mips64MmuPmdIsOk(*p_pmdentry)) {                            /*  ������������Ч              */

            p_ptsentry = mips64MmuPtsOffset(p_pmdentry, ulAddr);        /*  ���������������ַ          */
            if (mips64MmuPtsIsOk(*p_ptsentry)) {                        /*  ������������Ч              */

                p_pteentry = mips64MmuPteOffset(p_ptsentry, ulAddr);    /*  ����ļ���������ַ          */
                pteentry   = *p_pteentry;                               /*  ����ļ�������              */
                if (mips64MmuPteIsOk(pteentry)) {                       /*  �ļ���������Ч              */
                    ULONG   ulFlag = 0;
                    ULONG   ulCacheAttr;

                    if (pteentry & ENTRYLO_V) {                         /*  ��Ч                        */
                        ulFlag |= LW_VMM_FLAG_VALID;                    /*  ӳ����Ч                    */
                    }

                    ulFlag |= LW_VMM_FLAG_ACCESS;                       /*  ���Է���                    */

                    if (MIPS_MMU_HAS_XI) {
                        if (!(pteentry & MIPS_ENTRYLO_XI)) {
                            ulFlag |= LW_VMM_FLAG_EXECABLE;             /*  ����ִ��                    */
                        }
                    } else {
                        ulFlag |= LW_VMM_FLAG_EXECABLE;                 /*  ����ִ��                    */
                    }

                    if (pteentry & ENTRYLO_D) {                         /*  ��д                        */
                        ulFlag |= LW_VMM_FLAG_WRITABLE;
                    }

                    ulCacheAttr = (pteentry & ENTRYLO_C)
                                  >> ENTRYLO_C_SHIFT;                   /*  ��� CACHE ����             */
                    if (ulCacheAttr == MIPS_MMU_ENTRYLO_CACHE) {        /*  ���� CACHE                  */
                        ulFlag |= LW_VMM_FLAG_CACHEABLE;

                    } else if ((ulCacheAttr == MIPS_MMU_ENTRYLO_UNCACHE_WB) &&
                               (MIPS_MMU_ENTRYLO_UNCACHE_WB != MIPS_MMU_ENTRYLO_UNCACHE)) {
                        ulFlag |= LW_VMM_FLAG_WRITECOMBINING;
                    }

                    return  (ulFlag);
                }
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: mips64MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  mips64MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PMD_TRANSENTRY  *p_pmdentry;
    LW_PTS_TRANSENTRY  *p_ptsentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    p_pgdentry = mips64MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */
    if (p_pgdentry && mips64MmuPgdIsOk(*p_pgdentry)) {                  /*  һ����������Ч              */

        p_pmdentry = mips64MmuPmdOffset(p_pgdentry, ulAddr);            /*  ��ö�����������ַ          */
        if (mips64MmuPmdIsOk(*p_pmdentry)) {                            /*  ������������Ч              */

            p_ptsentry = mips64MmuPtsOffset(p_pmdentry, ulAddr);        /*  ���������������ַ          */
            if (mips64MmuPtsIsOk(*p_ptsentry)) {                        /*  ������������Ч              */

                p_pteentry = mips64MmuPteOffset(p_ptsentry, ulAddr);    /*  ����ļ���������ַ          */
                pteentry   = *p_pteentry;                               /*  ����ļ�������              */
                if (mips64MmuPteIsOk(pteentry)) {                       /*  �ļ���������Ч              */
                    ULONG   ulPFN = (pteentry & MIPS64_ENTRYLO_PFN_MASK) >>
                                     MIPS_ENTRYLO_PFN_SHIFT;            /*  �������ҳ���              */
                    addr_t  ulPhysicalAddr = ulPFN << 12;               /*  ����ҳ�������ַ            */

                    *p_pteentry = mips64MmuBuildPteEntry(ulPhysicalAddr, ulFlag);
                    return  (ERROR_NONE);
                }
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mips64MmuMakeTrans
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
static VOID  mips64MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                 LW_PTE_TRANSENTRY  *p_pteentry,
                                 addr_t              ulVirtualAddr,
                                 phys_addr_t         paPhysicalAddr,
                                 ULONG               ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return;
    }

    /*
     * �����ļ��������������ļ�������
     */
    *p_pteentry = mips64MmuBuildPteEntry((addr_t)paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** ��������: mips64MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips64MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_MIPS_HAS_RDHWR_INSTR > 0)
    extern CHAR    *_G_mips64MmuTlbRefillCtxMp;
    MIPS64_TLB_REFILL_CTX  *pCtx = (MIPS64_TLB_REFILL_CTX *)((addr_t)&_G_mips64MmuTlbRefillCtxMp +
                                    MIPS64_TLB_CTX_SIZE * LW_CPU_GET_CUR_ID());
#else
    extern CHAR    *_G_mips64MmuTlbRefillCtx;
    MIPS64_TLB_REFILL_CTX  *pCtx = (MIPS64_TLB_REFILL_CTX *)((addr_t)&_G_mips64MmuTlbRefillCtx);
#endif                                                                  /*  HAS_RDHWR_INSTR > 0         */
    pCtx->CTX_ulSpinLock = 0;
    pCtx->CTX_ulPGD      = (addr_t)pmmuctx->MMUCTX_pgdEntry;
    KN_WMB();

    mipsCp0ContextWrite(0);
    mipsCp0XContextWrite(0);
}
/*********************************************************************************************************
** ��������: mips64MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mips64MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_pfuncMemInit = mips64MmuMemInit;

    pmmuop->MMUOP_pfuncPGDAlloc = mips64MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = mips64MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = mips64MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = mips64MmuPmdFree;
    pmmuop->MMUOP_pfuncPTSAlloc = mips64MmuPtsAlloc;
    pmmuop->MMUOP_pfuncPTSFree  = mips64MmuPtsFree;
    pmmuop->MMUOP_pfuncPTEAlloc = mips64MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = mips64MmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk = mips64MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = mips64MmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTSIsOk = mips64MmuPtsIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = mips64MmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset = mips64MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = mips64MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTSOffset = mips64MmuPtsOffset;
    pmmuop->MMUOP_pfuncPTEOffset = mips64MmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet = mips64MmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet = mips64MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = mips64MmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans  = mips64MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx = mips64MmuMakeCurCtx;
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
