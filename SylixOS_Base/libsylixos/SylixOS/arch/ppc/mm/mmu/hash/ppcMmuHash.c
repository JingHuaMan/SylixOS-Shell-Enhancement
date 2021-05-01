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
** ��   ��   ��: ppcMmuHash.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 01 �� 14 ��
**
** ��        ��: PowerPC ��ϵ���� HASH ҳ�� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                 _G_uiTlbSize;                               /*  TLB �����С                */
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
static PHASH_PAGE_TBL       _G_pHashPageTbl;                            /*  HASH ҳ��                   */
/*********************************************************************************************************
  ����Ȩ�� PP
*********************************************************************************************************/
#define PP_NA               (0)                                         /*  �޷���Ȩ��                  */
#define PP_RO               (1)                                         /*  ֻ��                        */
#define PP_RW               (2)                                         /*  �ɶ�д                      */
/*********************************************************************************************************
  WIMG
*********************************************************************************************************/
#define G_BIT               (1 << 0)                                    /*  ��ֹ�²����                */
#define M_BIT               (1 << 1)                                    /*  ���һ����                  */
#define I_BIT               (1 << 2)                                    /*  ������ CACHE                */
#define W_BIT               (1 << 3)                                    /*  д��͸ CACHE                */

#define WI_MASK             (W_BIT | I_BIT)
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID  ppcHashMmuInvalidateTLBNr(UINT  uiTlbNr);
extern VOID  ppcHashMmuInvalidateTLBEA(addr_t  uiEffectiveAddr);
extern VOID  ppcHashMmuEnable(VOID);
extern VOID  ppcHashMmuDisable(VOID);
/*********************************************************************************************************
** ��������: ppcHashMmuInvalidateTLBNr
** ��������: ��Ч���е� TBL
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  VOID  ppcHashMmuInvalidateTLB (VOID)
{
    ppcHashMmuInvalidateTLBNr(_G_uiTlbSize >> 1);                       /*  ��·������                  */
}
/*********************************************************************************************************
** ��������: ppcHashMmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcHashMmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: ppcHashMmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : paBaseAddr              ����ҳ��ַ
**           ulAddr                  ����ҳ��ַ
**           ulFlag                  ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcHashMmuBuildPteEntry (phys_addr_t  paBaseAddr,
                                                   addr_t       ulAddr,
                                                   ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   ulDescriptor;
    UINT8               ucWIMG;
    UINT8               ucPP;
    ULONG               ulSegmentFlag;

    ulDescriptor.PTE_uiValue = 0;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulDescriptor.PTE_bValid = LW_TRUE;

        ulSegmentFlag = ppcHashPageTblSegmentFlag(_G_pHashPageTbl, ulAddr);
        if (!(ulSegmentFlag & LW_VMM_FLAG_EXECABLE)) {                  /*  �öβ���ִ��                */
            if (ulFlag & LW_VMM_FLAG_EXECABLE) {                        /*  ӳ��ȴ��ִ��                */
                _BugFormat(LW_TRUE, LW_TRUE,
                           "Segment %d can't execable! Please check your _G_HashMmuSR!\r\n",
                           ulAddr >> MMU_EA_SR_SHIFT);
            }
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            ucPP = PP_RW;                                               /*  �ɶ�д                      */

        } else {
            ucPP = PP_RO;                                               /*  ֻ��                        */
        }

    } else{
        ucPP = PP_NA;                                                   /*  �޷���Ȩ��                  */
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        ucWIMG = M_BIT;                                                 /*  ���һ����                  */

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸ CACHE                */
        ucWIMG = M_BIT | W_BIT;                                         /*  ���һ���� | д��͸ CACHE   */

    } else {
        ucWIMG = I_BIT | G_BIT;                                         /*  ������ CACHE | ��ֹ�²���� */
    }

    /*
     * ����� R �� C λ����Ϊ 0����Ϊ������ PTE word1 �󣬻���Ч TLB��
     * ���Բ��õ��� PTE ���Ӧ�� TLB ��һ�µ�����
     */
    ulDescriptor.PTE_bRef        = 0;
    ulDescriptor.PTE_bChange     = 0;
    ulDescriptor.PTE_ucPP        = ucPP;
    ulDescriptor.PTE_ucWIMG      = ucWIMG;
    ulDescriptor.PTE_uiRPN       = paBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: ppcHashMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcHashMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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

    _G_pHashPageTbl = ppcHashPageTblMemInit(pmmuctx, MMU_PTE_MIN_SIZE_128M);
    if (_G_pHashPageTbl == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory, can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcHashMmuGlobalInit (CPCHAR  pcMachineName)
{
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  �� CPU 0 ��λ CACHE         */
        archCacheReset(pcMachineName);                                  /*  ��λ CACHE                  */
    }

    ppcHashPageTblGlobalInit(pcMachineName);

    ppcHashMmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcHashMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: ppcHashMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcHashMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC �� PMD ��           */
}
/*********************************************************************************************************
** ��������: ppcHashMmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppcHashMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ���һ��ҳ��������          */

    p_pteentry = (LW_PTE_TRANSENTRY *)ulTemp;                           /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;                                   /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  ppcHashMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  ppcHashMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.PTE_bValid ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcHashMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: ppcHashMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcHashMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcHashMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PGD_TRANSENTRY  *p_pgdentry,
                                               addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcHashMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: ppcHashMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���:
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcHashMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PMD_TRANSENTRY  *p_pmdentry,
                                               addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = ppcHashMmuBuildPgdEntry((addr_t)p_pteentry);          /*  ���ö���ҳ��������          */

    return  (ppcHashMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: ppcHashMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcHashMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcHashMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.PTE_uiRPN;                              /*  �������ҳ���              */

    *ppaPhysicalAddr = uiRPN << LW_CFG_VMM_PAGE_SHIFT;                  /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcHashMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  ppcHashMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ      */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  ������������Ч              */
            ULONG  ulFlag;
            ULONG  ulSegmentFlag;

            ulFlag = LW_VMM_FLAG_GUARDED;                               /*  �����ϸ�Ȩ�޼��            */

            switch (p_pteentry->PTE_ucPP) {

            case PP_RO:                                                 /*  ֻ��                        */
                ulFlag |= LW_VMM_FLAG_VALID | LW_VMM_FLAG_ACCESS;
                break;

            case PP_RW:                                                 /*  �ɶ�д                      */
                ulFlag |= LW_VMM_FLAG_VALID | LW_VMM_FLAG_ACCESS | LW_VMM_FLAG_WRITABLE;
                break;

            case PP_NA:                                                 /*  �޷���Ȩ��                  */
            default:
                break;
            }

            ulSegmentFlag = ppcHashPageTblSegmentFlag(_G_pHashPageTbl, ulAddr);
            if ((ulSegmentFlag & LW_VMM_FLAG_EXECABLE) &&               /*  �öο�ִ��                  */
                (ulFlag & LW_VMM_FLAG_ACCESS)) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;
            }

            if (p_pteentry->PTE_ucWIMG == M_BIT) {
                ulFlag |= LW_VMM_FLAG_CACHEABLE;                        /*  ��д CACHE                  */

            } else if (p_pteentry->PTE_ucWIMG == (M_BIT | W_BIT)) {
                ulFlag |= LW_VMM_FLAG_WRITETHROUGH;                     /*  д��͸ CACHE                */
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: ppcHashMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  ppcHashMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  ���һ����������ַ          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  ������������Ч              */
            addr_t   ulPhysicalAddr = (addr_t)(p_pteentry->PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);

            *p_pteentry = ppcHashMmuBuildPteEntry(ulPhysicalAddr, ulAddr, ulFlag);

            return  (ppcHashPageTblFlagSet(_G_pHashPageTbl,
                                           ulAddr,
                                           p_pteentry->PTE_uiValue));
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppcHashMmuMakeTrans
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
static VOID  ppcHashMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = ppcHashMmuBuildPteEntry(paPhysicalAddr, ulVirtualAddr, ulFlag);

    ppcHashPageTblMakeTrans(_G_pHashPageTbl,
                            ulVirtualAddr,
                            p_pteentry->PTE_uiValue);
}
/*********************************************************************************************************
** ��������: ppcHashMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcHashMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcHashPageTblMakeCurCtx(_G_pHashPageTbl);
    ppcHashMmuInvalidateTLB();                                          /*  ȫ����� TLB                */
}
/*********************************************************************************************************
** ��������: ppcHashMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcHashMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (_G_uiTlbSize >> 1)) {
        ppcHashMmuInvalidateTLB();                                      /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            ppcHashMmuInvalidateTLBEA((UINT32)ulPageAddr);              /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: ppcHashMmuPteMissHandle
** ��������: ���� PTE ƥ��ʧ���쳣
** �䡡��  : ulAddr        �쳣����
** �䡡��  : ��ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcHashMmuPteMissHandle (addr_t  ulAddr)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  ���һ����������ַ          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  ������������Ч              */
            return  (ppcHashPageTblPteMissHandle(_G_pHashPageTbl,
                                                 ulAddr,
                                                 p_pteentry->PTE_uiValue));
        }
    }

    return  (LW_VMM_ABORT_TYPE_MAP);
}
/*********************************************************************************************************
** ��������: ppcHashMmuPtePreLoad
** ��������: PTE Ԥ����
** �䡡��  : ulAddr        ���ݷ��ʵ�ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcHashMmuPtePreLoad (addr_t  ulAddr)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppcHashMmuPgdOffset(pmmuctx, ulAddr);                  /*  ���һ����������ַ          */

    if (ppcHashMmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcHashMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppcHashMmuPteIsOk(pteentry)) {                              /*  ������������Ч              */
            return  (ppcHashPageTblPtePreLoad(_G_pHashPageTbl,
                                              ulAddr,
                                              p_pteentry->PTE_uiValue));
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: bspMmuTlbSize
** ��������: ��� TLB ����Ŀ
** �䡡��  : NONE
** �䡡��  : TLB ����Ŀ
** ȫ�ֱ���:
** ����ģ��:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuTlbSize (VOID)
{
    /*
     * 128 �ʺ� 750/E600/745x ����
     * 64  �ʺ� 603e/E300(MPC82XX/MPC83XX) ����
     */
    return  (128);
}
/*********************************************************************************************************
** ��������: ppcHashMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcHashMmuInit (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    _G_uiTlbSize = bspMmuTlbSize();                                     /*  ��� TLB ����Ŀ             */

    pmmuop->MMUOP_ulOption = 0ul;                                       /*  tlbsync ָ����Զ����ͬ��  */

    pmmuop->MMUOP_pfuncMemInit       = ppcHashMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcHashMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc      = ppcHashMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcHashMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcHashMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcHashMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcHashMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcHashMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk       = ppcHashMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcHashMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcHashMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset     = ppcHashMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcHashMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcHashMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcHashMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet       = ppcHashMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcHashMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = ppcHashMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcHashMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcHashMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcHashMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcHashMmuDisable;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, _G_uiTlbSize);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
