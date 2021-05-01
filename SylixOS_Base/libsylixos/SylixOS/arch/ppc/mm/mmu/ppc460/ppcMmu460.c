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
** ��   ��   ��: ppcMmu460.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2019 �� 08 �� 14 ��
**
** ��        ��: PowerPC 460 ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/ppcSpr.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmu460Reg.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
       UINT                 ppc460MmuTblIndex = PPC460_TLB_BASE;        /*  TLB ����������              */
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID  ppc460MmuInvalidateTLBEA(addr_t  ulAddr, UINT  uiPid);
/*********************************************************************************************************
** ��������: ppc460MmuInvalidateTLB
** ��������: ��Ч���� TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ppc460MmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppc460MmuEnable (VOID)
{
    /*
     * PPC460 ����ʹ�� MMU
     */
}
/*********************************************************************************************************
** ��������: ppc460MmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppc460MmuDisable (VOID)
{
    /*
     * PPC460 ����ʹ�� MMU
     */
}
/*********************************************************************************************************
** ��������: ppc460MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppc460MmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: ppc460MmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : paBaseAddr              ����ҳ��ַ
**           ulFlag                  ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppc460MmuBuildPteEntry (phys_addr_t  paBaseAddr,
                                                  ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   ulDescriptor;
    UINT32              uiRPN;

    ulDescriptor.WORD_uiValue = 0;

    uiRPN = paBaseAddr >> MMU_RPN_SHIFT;                                /*  ���� RPN                    */
    ulDescriptor.WORD1_uiRPN = uiRPN & MMU_RPN_MASK;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulDescriptor.WORD0_bValid     = LW_TRUE;                        /*  ��Ч                        */
        ulDescriptor.WORD2_bSuperRead = LW_TRUE;                        /*  ��Ȩ̬�ɶ�                  */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {
        ulDescriptor.WORD2_bSuperWrite = LW_TRUE;                       /*  ��Ȩ̬��д                  */
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        ulDescriptor.WORD2_bSuperExec = LW_TRUE;                        /*  ��Ȩ̬��ִ��                */
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        ulDescriptor.WORD2_bUnCache = LW_FALSE;
        ulDescriptor.WORD2_bWT      = LW_FALSE;
        ulDescriptor.WORD2_bGuarded = LW_FALSE;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸ CACHE                */
        ulDescriptor.WORD2_bUnCache = LW_FALSE;
        ulDescriptor.WORD2_bWT      = LW_TRUE;
        ulDescriptor.WORD2_bGuarded = LW_FALSE;

    } else {                                                            /*  UNCACHE                     */
        ulDescriptor.WORD2_bUnCache = LW_TRUE;
        ulDescriptor.WORD2_bWT      = LW_TRUE;
        ulDescriptor.WORD2_bGuarded = LW_TRUE;                          /*  ��ֹ�²����                */
    }

    ulDescriptor.WORD2_bGlobal = LW_TRUE;                               /*  ȫ��ӳ��                    */

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: ppc460MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: ppc460MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ppc460MmuGlobalInit (CPCHAR  pcMachineName)
{
    ppc460MmuSetPID(0);                                                 /*  ���� PID:0                  */
    ppc460MmuSetMMUCR(0);                                               /*  ���� MMUCR(STID:0, STS:0,   */
                                                                        /*  SWOA:0 д����)              */
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  �� Core0 ��λ CACHE         */
        archCacheReset(pcMachineName);                                  /*  ��λ CACHE                  */
    }

    ppc460MmuInvalidateTLB();                                           /*  ��Ч TLB                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc460MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppc460MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: ppc460MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppc460MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC �� PMD ��           */
}
/*********************************************************************************************************
** ��������: ppc460MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppc460MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
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
** ��������: ppc460MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  ppc460MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppc460MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  ppc460MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.WORD0_bValid ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppc460MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
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
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppc460MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppc460MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppc460MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppc460MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PGD_TRANSENTRY  *p_pgdentry,
                                              addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppc460MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppc460MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: ppc460MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
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

    *p_pmdentry = ppc460MmuBuildPgdEntry((addr_t)p_pteentry);          /*  ���ö���ҳ��������          */

    return  (ppc460MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: ppc460MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppc460MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: ppc460MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ppc460MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.WORD1_uiRPN;                            /*  �������ҳ���              */

    *ppaPhysicalAddr = uiRPN << MMU_RPN_SHIFT;                          /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc460MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  ppc460MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);   /*  ���һ����������ַ      */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  ������������Ч              */
            ULONG  ulFlag;

            ulFlag = LW_VMM_FLAG_GUARDED;                               /*  �����ϸ�Ȩ�޼��            */

            if (pteentry.WORD2_bSuperRead) {
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  ӳ����Ч                    */
                ulFlag |= LW_VMM_FLAG_ACCESS;                           /*  ��Ȩ̬�ɶ�                  */
            }

            if (pteentry.WORD2_bSuperWrite) {
                ulFlag |= LW_VMM_FLAG_WRITABLE;                         /*  ��Ȩ̬��д                  */
            }

            if (pteentry.WORD2_bSuperExec) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  ��Ȩ̬��ִ��                */
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
** ��������: ppc460MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  ppc460MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   pteentry = *p_pteentry;                     /*  ��ö���������              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  ������������Ч              */
            UINT32        uiRPN = pteentry.WORD1_uiRPN;                 /*  �������ҳ��                */
            phys_addr_t   paPhysicalAddr = ((phys_addr_t)uiRPN) << MMU_RPN_SHIFT;

            /*
             * �������������������ö���������
             */
            *p_pteentry = ppc460MmuBuildPteEntry(paPhysicalAddr, ulFlag);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppc460MmuMakeTrans
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
static VOID  ppc460MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = ppc460MmuBuildPteEntry(paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** ��������: ppc460MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppc460MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcSetSPRG3((addr_t)pmmuctx->MMUCTX_pgdEntry);
}
/*********************************************************************************************************
** ��������: ppc460MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppc460MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (PPC460_TLB_SIZE >> 1)) {
        ppc460MmuInvalidateTLB();                                       /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            ppc460MmuInvalidateTLBEA(ulPageAddr, 0);                    /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: ppc460MmuStorageAbortType
** ��������: ������ֹ����
** �䡡��  : ulAddr        ��ֹ��ַ
**           uiMethod      ���ʷ���(LW_VMM_ABORT_METHOD_XXX)
** �䡡��  : ���ݷ�����ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppc460MmuStorageAbortType (addr_t  ulAddr, UINT  uiMethod)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = ppc460MmuPgdOffset(pmmuctx, ulAddr);                   /*  ���һ����������ַ          */

    if (ppc460MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */
        LW_PTE_TRANSENTRY *p_pteentry = ppc460MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY  pteentry = *p_pteentry;                      /*  ��ö���������              */

        if (ppc460MmuPteIsOk(pteentry)) {                               /*  ������������Ч              */
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
** ��������: ppc460MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
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
