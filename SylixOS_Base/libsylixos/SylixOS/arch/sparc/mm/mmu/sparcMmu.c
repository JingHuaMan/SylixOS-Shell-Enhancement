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
** ��   ��   ��: sparcMmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 10 �� 09 ��
**
** ��        ��: SPARC ��ϵ���� LEON ���ִ����� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "SylixOS.h"
#if LW_CFG_CACHE_EN > 0
#include "../cache/sparcCache.h"
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  PTD PTE �е�λ����
*********************************************************************************************************/
#define SPARC_MMU_ET_INVALID            (0)                             /*  ӳ����Ч                    */
#define SPARC_MMU_ET_PTD                (1)
#define SPARC_MMU_ET_PTE                (2)
#define SPARC_MMU_ET_SHIFT              (0)
#define SPARC_MMU_ET_MASK               (0x3 << SPARC_MMU_ET_SHIFT)

#define SPARC_MMU_ACC_S_RO              (0)                             /*  ֻ��                        */
#define SPARC_MMU_ACC_S_RW              (1)                             /*  �ɶ�д                      */
#define SPARC_MMU_ACC_S_RX              (2)                             /*  �ɶ�ִ��                    */
#define SPARC_MMU_ACC_S_RWX             (3)                             /*  �ɶ�дִ��                  */
#define SPARC_MMU_ACC_SHIFT             (2)
#define SPARC_MMU_ACC_MASK              (0x7 << SPARC_MMU_ACC_SHIFT)

#define SPARC_MMU_R                     (1)                             /*  ���ʹ�                      */
#define SPARC_MMU_R_NO                  (0)
#define SPARC_MMU_R_SHIFT               (5)
#define SPARC_MMU_R_MASK                (1 << SPARC_MMU_R_SHIFT)

#define SPARC_MMU_M                     (1)                             /*  �޸Ĺ�                      */
#define SPARC_MMU_M_NO                  (0)
#define SPARC_MMU_M_SHIFT               (6)
#define SPARC_MMU_M_MASK                (1 << SPARC_MMU_M_SHIFT)

#define SPARC_MMU_C                     (1)                             /*  �� CACHE                    */
#define SPARC_MMU_C_NO                  (0)
#define SPARC_MMU_C_SHIFT               (7)
#define SPARC_MMU_C_MASK                (1 << SPARC_MMU_C_SHIFT)

#define SPARC_MMU_PTP_SHIFT             (2)                             /*  Page Table Pointer          */
#define SPARC_MMU_PPN_SHIFT             (8)                             /*  Physical Page Number        */

#define SPARC_MMU_PTP_PA_SHIFT          (6)
#define SPARC_MMU_PPN_PA_SHIFT          (LW_CFG_VMM_PAGE_SHIFT)
/*********************************************************************************************************
  MMU ���ƼĴ�����غ궨��
*********************************************************************************************************/
#define SPARC_MMU_CTRL_EN               (1 << 0)                        /*  Enable bit                  */
#define SPARC_MMU_CTRL_NF               (1 << 1)                        /*  No Fault                    */
#define SPARC_MMU_CTRL_PSO              (1 << 7)                        /*  Partial Store Ordering      */
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern UINT32  leonMmuGetCtrlReg(VOID);
extern VOID    leonMmuSetCtrlReg(UINT32  uiValue);
extern VOID    leonMmuSetCtxTblPtr(UINT32  uiValue);
extern VOID    leonMmuSetCtx(UINT32  uiValue);
extern UINT32  leonMmuGetCtx(VOID);
extern UINT32  leonMmuGetFaultStatus(VOID);
extern UINT32  leonMmuGetFaultAddr(VOID);
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD ������                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
static UINT32              *_G_puiCtxTbl;                               /*  �����ı�                    */
       BOOL                 _G_bSparcCacheCanWt = LW_TRUE;              /*  CACHE �Ƿ��������Ϊд��͸  */
/*********************************************************************************************************
  �봦����ʵ��(LEON)�йصĺ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: leonMmuFlushTlbAll
** ��������: LEON ��Ч���� TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  leonMmuFlushTlbAll (VOID)
{
    __asm__ __volatile__ ("sta %%g0, [%0] %1\n\t" : : "r"(0x400),
                          "i"(ASI_LEON_MMUFLUSH)  : "memory");
}
/*********************************************************************************************************
** ��������: sparcMmuGetFaultAddr
** ��������: ��ô����ַ
** �䡡��  : NONE
** �䡡��  : �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
addr_t  sparcMmuGetFaultAddr (VOID)
{
    return  (leonMmuGetFaultAddr());
}
/*********************************************************************************************************
** ��������: sparcMmuGetFaultStatus
** ��������: ��ô���״̬
** �䡡��  : NONE
** �䡡��  : ����״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  sparcMmuGetFaultStatus (VOID)
{
    return  (leonMmuGetFaultStatus());
}
/*********************************************************************************************************
** ��������: sparcMmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuEnable (VOID)
{
    UINT32  uiCtrl;

    uiCtrl  = leonMmuGetCtrlReg();
    uiCtrl |= SPARC_MMU_CTRL_EN;
    uiCtrl |= SPARC_MMU_CTRL_PSO;
    uiCtrl &= ~(SPARC_MMU_CTRL_NF);
    leonMmuSetCtrlReg(uiCtrl);
}
/*********************************************************************************************************
** ��������: sparcMmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuDisable (VOID)
{
    UINT32  uiCtrl;

    uiCtrl  = leonMmuGetCtrlReg();
    uiCtrl &= ~(SPARC_MMU_CTRL_EN);
    leonMmuSetCtrlReg(uiCtrl);
}
/*********************************************************************************************************
** ��������: sparcMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    UINT32  uiCtxPde;
    UINT32  uiCtxTblPtr;

    /*
     * ���������ı�ĵ� 0 ����Ŀ
     */
    uiCtxPde   = ((UINT32)pmmuctx->MMUCTX_pgdEntry) >> SPARC_MMU_PTP_PA_SHIFT;
    uiCtxPde <<= SPARC_MMU_PTP_SHIFT;
    uiCtxPde  |= SPARC_MMU_ET_PTD;
    _G_puiCtxTbl[0] = uiCtxPde;

    /*
     * �����ı�ָ��Ĵ���ֵ
     */
    uiCtxTblPtr   = (addr_t)_G_puiCtxTbl >> SPARC_MMU_PTP_PA_SHIFT;
    uiCtxTblPtr <<= SPARC_MMU_PTP_SHIFT;

    /*
     * ���������ĺ������ı�ָ��Ĵ���
     */
    leonMmuSetCtx(0);
    leonMmuSetCtxTblPtr(uiCtxTblPtr);
}
/*********************************************************************************************************
** ��������: sparcMmuInvalidateTLB
** ��������: ��Ч���� TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuInvalidateTLB (VOID)
{
    leonMmuFlushTlbAll();
}
/*********************************************************************************************************
** ��������: sparcMmuInvalidateTLBMVA
** ��������: ��Чָ�� MVA �� TLB
** �䡡��  : pvAddr            �����ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuInvalidateTLBMVA (PVOID  pvAddr)
{
    leonMmuFlushTlbAll();                                               /*  LEON �޵��� TLB flush ����  */
}
/*********************************************************************************************************
  �봦����ʵ���޹�, �� SPARCv8 ��صĺ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sparcMmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� sparc MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  SylixOS Ȩ�ޱ�־
**           pucACC                  ����Ȩ��
**           pucC                    CACHE ����
**           pucET                   Entry Type
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sparcMmuFlags2Attr (ULONG   ulFlag,
                                UINT8  *pucACC,
                                UINT8  *pucC,
                                UINT8  *pucET)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ɷ���                  */
        *pucET = SPARC_MMU_ET_PTE;

    } else {
        *pucET = SPARC_MMU_ET_INVALID;
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        if (ulFlag & LW_VMM_FLAG_EXECABLE) {                            /*  �Ƿ��ִ��                  */
            *pucACC = SPARC_MMU_ACC_S_RWX;
        } else {
            *pucACC = SPARC_MMU_ACC_S_RW;
        }

    } else {
        if (ulFlag & LW_VMM_FLAG_EXECABLE) {                            /*  �Ƿ��ִ��                  */
            *pucACC = SPARC_MMU_ACC_S_RX;
        } else {
            *pucACC = SPARC_MMU_ACC_S_RO;
        }
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        *pucC = SPARC_MMU_C;

    } else if ((ulFlag & LW_VMM_FLAG_WRITETHROUGH) && _G_bSparcCacheCanWt) {
        *pucC = SPARC_MMU_C;                                            /*  д��͸ CACHE                */

    } else {
        *pucC = SPARC_MMU_C_NO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcMmuAttr2Flags
** ��������: ���� sparc MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucACC                   ����Ȩ��
**           ucC                     CACHE ����
**           ucET                    Entry Type
**           pulFlag                 SylixOS Ȩ�ޱ�־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sparcMmuAttr2Flags (UINT8   ucACC,
                                UINT8   ucC,
                                UINT8   ucET,
                                ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucET == SPARC_MMU_ET_PTE) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }

    if (ucC == SPARC_MMU_C) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
    }

    switch (ucACC) {

    case SPARC_MMU_ACC_S_RO:
        break;

    case SPARC_MMU_ACC_S_RW:
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
        break;

    case SPARC_MMU_ACC_S_RX:
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
        break;

    case SPARC_MMU_ACC_S_RWX:
        *pulFlag |= LW_VMM_FLAG_WRITABLE | LW_VMM_FLAG_EXECABLE;
        break;

    default:
        break;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcMmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucACC                   ����Ȩ��
**           ucC                     CACHE ����
**           ucET                    Entry Type
** �䡡��  : һ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  sparcMmuBuildPgdEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PGD_TRANSENTRY  uiDescriptor;
    UINT32             uiPTP = uiBaseAddr >> SPARC_MMU_PTP_PA_SHIFT;

    uiDescriptor = (uiPTP << SPARC_MMU_PTP_SHIFT)
                 | ((ucET ? SPARC_MMU_ET_PTD : SPARC_MMU_ET_INVALID) << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: sparcMmuBuildPmdEntry
** ��������: ����һ������������ (PMD ������)
** �䡡��  : uiBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucACC                   ����Ȩ��
**           ucC                     CACHE ����
**           ucET                    Entry Type
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  sparcMmuBuildPmdEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PMD_TRANSENTRY  uiDescriptor;
    UINT32             uiPTP = uiBaseAddr >> SPARC_MMU_PTP_PA_SHIFT;

    uiDescriptor = (uiPTP << SPARC_MMU_PTP_SHIFT)
                 | ((ucET ? SPARC_MMU_ET_PTD : SPARC_MMU_ET_INVALID) << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: sparcMmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ַ     (ҳ��ַ)
**           ucACC                   ����Ȩ��
**           ucC                     CACHE ����
**           ucET                    Entry Type
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  sparcMmuBuildPteEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PTE_TRANSENTRY  uiDescriptor;
    UINT32             uiPPN = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;
    
    uiDescriptor = (uiPPN << SPARC_MMU_PPN_SHIFT)
                 | (ucACC << SPARC_MMU_ACC_SHIFT)
                 | (ucC   << SPARC_MMU_C_SHIFT)
                 | (ucET  << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: sparcMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : sparc ��ϵ�ṹҪ��: һ��ҳ�����ַ��Ҫ���� 1  KByte ����, ����Ŀӳ�� 16  MByte �ռ�.
                                 ����ҳ�����ַ��Ҫ���� 256 Byte ����, ����Ŀӳ�� 256 KByte �ռ�.
                                 ����ҳ�����ַ��Ҫ���� 256 Byte ����, ����Ŀӳ�� 4   KByte �ռ�.
                                 �����ı���Ҫ���� 1 KByte ����.
*********************************************************************************************************/
static INT  sparcMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define CTX_TBL_SIZE    (1 * LW_CFG_KB_SIZE)
#define PGD_BLOCK_SIZE  (1 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (256)
#define PTE_BLOCK_SIZE  (256)

    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    _G_puiCtxTbl = __KHEAP_ALLOC_ALIGN(CTX_TBL_SIZE, CTX_TBL_SIZE);

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * PMD_BLOCK_SIZE, PMD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!_G_puiCtxTbl || !pvPgdTable || !pvPmdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    lib_bzero(_G_puiCtxTbl, CTX_TBL_SIZE);

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
** ��������: sparcMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sparcMmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);
    
    sparcMmuInvalidateTLB();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *sparcMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *sparcMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPmdNum;
    REGISTER UINT32              uiPTP;
    REGISTER UINT32              uiPhyAddr;

    uiTemp = (LW_PGD_TRANSENTRY)(*p_pgdentry);                          /*  ���һ��ҳ��������          */

    uiPTP     = uiTemp >> SPARC_MMU_PTP_SHIFT;
    uiPhyAddr = uiPTP  << SPARC_MMU_PTP_PA_SHIFT;

    p_pmdentry = (LW_PMD_TRANSENTRY *)(uiPhyAddr);                      /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    uiPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (uiPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPteOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *sparcMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPageNum;
    REGISTER UINT32              uiPTP;
    REGISTER UINT32              uiPhyAddr;

    uiTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ��ö���ҳ��������          */

    uiPTP     = uiTemp >> SPARC_MMU_PTP_SHIFT;
    uiPhyAddr = uiPTP  << SPARC_MMU_PTP_PA_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)(uiPhyAddr);                      /*  �������ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;
    uiPageNum  = ulAddr  >> LW_CFG_VMM_PAGE_SHIFT;                      /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  sparcMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (SPARC_MMU_ET_PTD << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sparcMmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  sparcMmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  ((pmdentry & (SPARC_MMU_ET_PTD << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sparcMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  sparcMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (SPARC_MMU_ET_PTE << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: sparcMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *sparcMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  ��Чһ��ҳ����              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sparcMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *sparcMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
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

    *p_pgdentry = sparcMmuBuildPgdEntry((UINT32)p_pmdentry,
                                        SPARC_MMU_ACC_S_RWX,
                                        SPARC_MMU_C,
                                        SPARC_MMU_ET_PTD);              /*  ����һ��ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pgdentry, sizeof(LW_PGD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (sparcMmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: sparcMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sparcMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *sparcMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
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
    
    *p_pmdentry = sparcMmuBuildPmdEntry((UINT32)p_pteentry,
                                        SPARC_MMU_ACC_S_RWX,
                                        SPARC_MMU_C,
                                        SPARC_MMU_ET_PTD);              /*  ���ö���ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (sparcMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: sparcMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  sparcMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: sparcMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  sparcMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    UINT32      uiPPN;
    UINT32      uiPhyAddr;

    uiPPN     = pteentry >> SPARC_MMU_PPN_SHIFT;
    uiPhyAddr = uiPPN    << SPARC_MMU_PPN_PA_SHIFT;

    *ppaPhysicalAddr = (addr_t)(uiPhyAddr);                             /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  sparcMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = sparcMmuPgdOffset(pmmuctx, ulAddr);/*  ���һ����������ַ          */

    if (p_pgdentry && sparcMmuPgdIsOk(*p_pgdentry)) {                   /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sparcMmuPmdOffset(p_pgdentry,
                                                           ulAddr);     /*  ��ö�����������ַ          */

        if (sparcMmuPmdIsOk(*p_pmdentry)) {                             /*  ������������Ч              */
            LW_PTE_TRANSENTRY  *p_pteentry = sparcMmuPteOffset(p_pmdentry,
                                                               ulAddr); /*  ���������������ַ          */
            LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

            if (sparcMmuPteIsOk(pteentry)) {                            /*  ������������Ч              */
                UINT8   ucACC, ucC, ucET;
                ULONG   ulFlag;

                ucACC = (UINT8)((pteentry & SPARC_MMU_ACC_MASK) >> SPARC_MMU_ACC_SHIFT);
                ucC   = (UINT8)((pteentry & SPARC_MMU_C_MASK)   >> SPARC_MMU_C_SHIFT);
                ucET  = (UINT8)((pteentry & SPARC_MMU_ET_MASK)  >> SPARC_MMU_ET_SHIFT);

                sparcMmuAttr2Flags(ucACC, ucC, ucET, &ulFlag);

                return  (ulFlag);
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: sparcMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  sparcMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8   ucACC, ucC, ucET;

    if (sparcMmuFlags2Attr(ulFlag, &ucACC,
                           &ucC,   &ucET) != ERROR_NONE) {              /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = sparcMmuPgdOffset(pmmuctx, ulAddr);/*  ���һ����������ַ          */

    if (p_pgdentry && sparcMmuPgdIsOk(*p_pgdentry)) {                   /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sparcMmuPmdOffset(p_pgdentry,
                                                           ulAddr);     /*  ��ö�����������ַ          */

        if (sparcMmuPmdIsOk(*p_pmdentry)) {                             /*  ������������Ч              */
            LW_PTE_TRANSENTRY  *p_pteentry = sparcMmuPteOffset(p_pmdentry,
                                                               ulAddr); /*  ���������������ַ          */
            LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;               /*  �������������              */

            if (sparcMmuPteIsOk(pteentry)) {                            /*  ������������Ч              */
                addr_t  ulPhysicalAddr;
                UINT32  uiPPN;

                uiPPN          = pteentry >> SPARC_MMU_PPN_SHIFT;
                ulPhysicalAddr = uiPPN    << SPARC_MMU_PPN_PA_SHIFT;

                *p_pteentry = sparcMmuBuildPteEntry(ulPhysicalAddr,
                                                    ucACC, ucC, ucET);
#if LW_CFG_CACHE_EN > 0
                sparcCacheFlush(DATA_CACHE, (PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                sparcMmuInvalidateTLBMVA((PVOID)ulAddr);

                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sparcMmuMakeTrans
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
static VOID  sparcMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                LW_PTE_TRANSENTRY  *p_pteentry,
                                addr_t              ulVirtualAddr,
                                phys_addr_t         paPhysicalAddr,
                                ULONG               ulFlag)
{
    UINT8   ucACC, ucC, ucET;
    
    if (sparcMmuFlags2Attr(ulFlag, &ucACC,
                           &ucC,   &ucET) != ERROR_NONE) {              /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = sparcMmuBuildPteEntry((UINT32)paPhysicalAddr, ucACC, ucC, ucET);
                                                        
#if LW_CFG_CACHE_EN > 0
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: sparcMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > 0) {
        sparcMmuInvalidateTLB();                                        /*  ȫ����� TLB                */
    }
}
/*********************************************************************************************************
** ��������: sparcMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : LEON ��� SPARC ����������Ҫ LW_VMM_MMU_FLUSH_TLB_MP ����.
*********************************************************************************************************/
VOID  sparcMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;                                       /* No LW_VMM_MMU_FLUSH_TLB_MP   */

    pmmuop->MMUOP_pfuncMemInit    = sparcMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = sparcMmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = sparcMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = sparcMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = sparcMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = sparcMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = sparcMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = sparcMmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = sparcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = sparcMmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = sparcMmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = sparcMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = sparcMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = sparcMmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = sparcMmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = sparcMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = sparcMmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = sparcMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = sparcMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = sparcMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = sparcMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = sparcMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
