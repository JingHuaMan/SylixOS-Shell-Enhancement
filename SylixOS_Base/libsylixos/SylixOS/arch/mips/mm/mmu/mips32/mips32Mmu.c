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
** ��   ��   ��: mips32Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 10 �� 12 ��
**
** ��        ��: MIPS32 ��ϵ���� MMU ����.
**
** BUG:
2016.04.06  �޸� TLB ��Ч�� EntryHi Register ����(JZ4780 ֧��)
2016.06.14  Ϊ֧�ַ� 4K ��Сҳ���ع�����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 32
#include "arch/mips/common/cp0/mipsCp0.h"
#include "../mipsMmuCommon.h"
/*********************************************************************************************************
  ENTRYLO PFN ����
*********************************************************************************************************/
#define MIPS32_ENTRYLO_PFN_MASK         (0xffffff << MIPS_ENTRYLO_PFN_SHIFT)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static PVOID                _G_pvPTETable    = LW_NULL;                 /*  PTE ��                      */
/*********************************************************************************************************
** ��������: mips32MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : һ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  mips32MmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: mips32MmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : ulBaseAddr              ����ҳ��ַ
**           ulFlag                  ��־
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  mips32MmuBuildPteEntry (addr_t  ulBaseAddr, ULONG  ulFlag)
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
** ��������: mips32MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4096 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  ((LW_CFG_MB_SIZE / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))
#define PTE_TABLE_SIZE  ((LW_CFG_GB_SIZE / LW_CFG_VMM_PAGE_SIZE) * 4 * sizeof(LW_PTE_TRANSENTRY))

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN(PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    if (!pvPgdTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    /*
     * PTE ����Ҫ 8MByte �������д�� Context �Ĵ���
     */
    pvPteTable = __KHEAP_ALLOC_ALIGN(PTE_TABLE_SIZE, 8 * LW_CFG_MB_SIZE);
    if (!pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        __KHEAP_FREE(pvPgdTable);
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, 1, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hPGDPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        __KHEAP_FREE(pvPgdTable);
        __KHEAP_FREE(pvPteTable);
        return  (PX_ERROR);
    }
    
    lib_bzero(pvPteTable, PTE_TABLE_SIZE);

    _G_pvPTETable = pvPteTable;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *mips32MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *mips32MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  MIPS32 �� PMD ��            */
}
/*********************************************************************************************************
** ��������: mips32MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *mips32MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   pmdentry;
    REGISTER ULONG               ulPageNum;

    pmdentry   = (*p_pmdentry);                                         /*  ���һ��ҳ��������          */
    p_pteentry = (LW_PTE_TRANSENTRY *)(pmdentry);                       /*  ��ö���ҳ�����ַ          */

    ulAddr    &= ~LW_CFG_VMM_PGD_MASK;
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  mips32MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips32MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  mips32MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & ENTRYLO_V) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                             LW_PGD_TRANSENTRY  *p_pgdentry,
                                             addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: mips32MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *mips32MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER ULONG               ulPgdNum;

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)_G_pvPTETable | (ulPgdNum * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)mips32MmuBuildPgdEntry((addr_t)p_pteentry);/*  ���ö���ҳ�����ַ  */

    return  (mips32MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: mips32MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** ��������: mips32MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    ULONG  ulPFN = (pteentry & MIPS32_ENTRYLO_PFN_MASK) >>
                    MIPS_ENTRYLO_PFN_SHIFT;                             /*  �������ҳ���              */

    *ppaPhysicalAddr = ulPFN << 12;                                     /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  mips32MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */
    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */

        p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                         ulAddr);                       /*  ��ö�����������ַ          */
        pteentry = *p_pteentry;                                         /*  ��ö���������              */
        if (mips32MmuPteIsOk(pteentry)) {                               /*  ������������Ч              */
            ULONG   ulFlag = 0;
            ULONG   ulCacheAttr;

            if (pteentry & ENTRYLO_V) {                                 /*  ��Ч                        */
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  ӳ����Ч                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  ���Է���                    */

            if (MIPS_MMU_HAS_XI) {
                if (!(pteentry & MIPS_ENTRYLO_XI)) {
                    ulFlag |= LW_VMM_FLAG_EXECABLE;                     /*  ����ִ��                    */
                }
            } else {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  ����ִ��                    */
            }

            if (pteentry & ENTRYLO_D) {                                 /*  ��д                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }

            ulCacheAttr = (pteentry & ENTRYLO_C)
                          >> ENTRYLO_C_SHIFT;                           /*  ��� CACHE ����             */
            if (ulCacheAttr == MIPS_MMU_ENTRYLO_CACHE) {                /*  ���� CACHE                  */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;

            } else if ((ulCacheAttr == MIPS_MMU_ENTRYLO_UNCACHE_WB) &&
                       (MIPS_MMU_ENTRYLO_UNCACHE_WB != MIPS_MMU_ENTRYLO_UNCACHE)) {
                ulFlag |= LW_VMM_FLAG_WRITECOMBINING;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: mips32MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  mips32MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */
    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */

        p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                        ulAddr);                        /*  ��ö�����������ַ          */
        pteentry= *p_pteentry;                                          /*  ��ö���������              */
        if (mips32MmuPteIsOk(pteentry)) {                               /*  ������������Ч              */
            ULONG   ulPFN = (pteentry & MIPS32_ENTRYLO_PFN_MASK) >>
                             MIPS_ENTRYLO_PFN_SHIFT;                    /*  �������ҳ���              */
            addr_t  ulPhysicalAddr = ulPFN << 12;                       /*  ����ҳ�������ַ            */

            *p_pteentry = mips32MmuBuildPteEntry(ulPhysicalAddr, ulFlag);
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mips32MmuMakeTrans
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
static VOID  mips32MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                 LW_PTE_TRANSENTRY  *p_pteentry,
                                 addr_t              ulVirtualAddr,
                                 phys_addr_t         paPhysicalAddr,
                                 ULONG               ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return;
    }

    /*
     * �������������������ö���������
     */
    *p_pteentry = mips32MmuBuildPteEntry((addr_t)paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** ��������: mips32MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    mipsCp0ContextWrite((addr_t)_G_pvPTETable);                         /*  �� PTE ��д�� Context �Ĵ���*/
}
/*********************************************************************************************************
** ��������: mips32MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mips32MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_pfuncMemInit = mips32MmuMemInit;

    pmmuop->MMUOP_pfuncPGDAlloc = mips32MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = mips32MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = mips32MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = mips32MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = mips32MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = mips32MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = mips32MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = mips32MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = mips32MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = mips32MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = mips32MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = mips32MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = mips32MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans  = mips32MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx = mips32MmuMakeCurCtx;
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
