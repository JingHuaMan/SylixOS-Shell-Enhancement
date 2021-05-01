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
** ��   ��   ��: arm64Mmu64K.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 11 �� 01 ��
**
** ��        ��: ARM64 ��ϵ���� MMU ���� (64K ҳ��С).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_ARM64_PAGE_SHIFT == 16
#include "../../param/arm64Param.h"
#include "../cache/arm64Cache.h"
#include "arm64Mmu.h"
/*********************************************************************************************************
  ��������
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
  PGD �е�λ����
*********************************************************************************************************/
#define ARM64_PGD_TYPE_MASK     (0x3 << 0)                              /*  PGD �е���������            */
#define ARM64_PGD_TYPE_FAULT    (0x0 << 0)                              /*  PGD ������Ч                */
#define ARM64_PGD_TYPE_TABLE    (0x3 << 0)                              /*  PGD Ϊ Table ����           */
#define ARM64_PGD_TYPE_BLOCK    (0x1 << 0)                              /*  PGD Ϊ Block ����           */
/*********************************************************************************************************
  PMD �е�λ����
*********************************************************************************************************/
#define ARM64_PMD_TYPE_MASK     (0x3 << 0)                              /*  PMD �е���������            */
#define ARM64_PMD_TYPE_FAULT    (0x0 << 0)                              /*  PMD ������Ч                */
#define ARM64_PMD_TYPE_TABLE    (0x3 << 0)                              /*  PMD Ϊ Table ����           */
#define ARM64_PMD_TYPE_BLOCK    (0x1 << 0)                              /*  PMD Ϊ Block ����           */
/*********************************************************************************************************
  PTE �е�λ����
*********************************************************************************************************/
#define ARM64_PTE_TYPE_MASK     (0x3 << 0)                              /*  PTE �е���������            */
#define ARM64_PTE_TYPE_FAULT    (0x0 << 0)                              /*  PTE ������Ч                */
#define ARM64_PTE_TYPE_PAGE     (0x3 << 0)                              /*  PTE ����Ϊ PAGE             */
/*********************************************************************************************************
  PGD��PMD��PTS��PTE�е����Զ���
  
  PGD��PMD��PTS ��ʽ��[58:55] Reserved for software use
       63      62        61     60          59     58     55
  +---------+--------------+-----------+----------+---------+
  | NSTable |    APTable   |  XNTable  | PXNTable |  Check  |
  +---------+--------------+-----------+----------+---------+
*********************************************************************************************************/
#define ARM64_MMU_NS_SHIFT      (63)                                    /*  PGD��PMD��PTS �е� Secure & */
#define ARM64_MMU_NS_MASK       (0x1 << ARM64_MMU_NS_SHIFT)             /*  Non-Secure ��־             */
#define ARM64_MMU_AP_SHIFT      (61)                                    /*  PGD��PMD��PTS �е� Access   */
#define ARM64_MMU_AP_MASK       (0x3 << ARM64_MMU_AP_SHIFT)             /*  permissions ��־            */
#define ARM64_MMU_XN_SHIFT      (60)
#define ARM64_MMU_XN_MASK       (0x1 << ARM64_MMU_XN_SHIFT)             /*  PGD��PMD��PTS �е� XN       */
#define ARM64_MMU_PXN_SHIFT     (59)
#define ARM64_MMU_PXN_MASK      (0x1 << ARM64_MMU_PXN_SHIFT)            /*  PGD��PMD��PTS �е� PXN      */

#define ARM64_PTE_GUARD_SHIFT   (55)
#define ARM64_PTE_GUARD_MASK    (1UL << ARM64_PTE_GUARD_SHIFT)          /*  ���ڼ�¼ GUARD ��־         */
#define ARM64_PTE_UXN_SHIFT     (54)
#define ARM64_PTE_UXN_MASK      (1UL << ARM64_PTE_UXN_SHIFT)            /*  User XN                     */
#define ARM64_PTE_PXN_SHIFT     (53)
#define ARM64_PTE_PXN_MASK      (1UL << ARM64_PTE_PXN_SHIFT)            /*  Privileged XN               */
#define ARM64_PTE_CONT_SHIFT    (52)
#define ARM64_PTE_CONT_MASK     (1UL << ARM64_PTE_CONT_SHIFT)           /*  Contiguous range            */
#define ARM64_PTE_NG_SHIFT      (11)
#define ARM64_PTE_NG_MASK       (0x1 << ARM64_PTE_NG_SHIFT)             /*  PTE �е� nG ��־            */
#define ARM64_PTE_AF_SHIFT      (10)
#define ARM64_PTE_AF_MASK       (0x1 << ARM64_PTE_AF_SHIFT)             /*  PTE �еķ��ʱ�־            */
#define ARM64_PTE_SH_SHIFT      (8)
#define ARM64_PTE_SH_MASK       (0x3 << ARM64_PTE_SH_SHIFT)             /*  PTE �еĹ���Ȩ������        */
#define ARM64_PTE_AP_SHIFT      (6)
#define ARM64_PTE_AP_MASK       (0x3 << ARM64_PTE_AP_SHIFT)             /*  PTE �еķ���Ȩ������        */
#define ARM64_PTE_NS_SHIFT      (5)
#define ARM64_PTE_NS_MASK       (0x1 << ARM64_PTE_NS_SHIFT)             /*  PTE �е� Non-Secure         */
#define ARM64_PTE_AIN_SHIFT     (2)
#define ARM64_PTE_AIN_MASK      (0x7 << ARM64_PTE_AIN_SHIFT)            /*  PTE �е� AttrIndex          */

#define ARM64_MMU_NS_SECURE     (0)
#define ARM64_MMU_NS_NONSECURE  (1)
#define ARM64_MMU_AP_NO_EFFECT  (0)
#define ARM64_MMU_XN_NO_EFFECT  (0)
#define ARM64_MMU_PXN_NO_EFFECT (0)
/*********************************************************************************************************
  PGM PMD PTE �е�����
*********************************************************************************************************/
#define ARM64_MMU_ADDR_MASK     (0xffffffff0000ul)                      /*  [47:16]                     */
/*********************************************************************************************************
  2 �����Ȩ�޶���
*********************************************************************************************************/
#define GUARDED_CHK          0                                          /*  ����ϸȨ�޼��              */
#define GUARDED_NOT_CHK      1                                          /*  ������ϸȨ�޼��            */
/*********************************************************************************************************
  ȫ�ֶ���
*********************************************************************************************************/
#define NON_SHAREABLE           0x0
#define OUTER_SHAREABLE         0x2
#define INNER_SHAREABLE         0x3
#define VMSA_S                  _G_uiVMSAShare                          /*  ����λֵ                    */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hPGDPartition;                       /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE         _G_hPMDPartition;                       /*  PMD ������                  */
static LW_OBJECT_HANDLE         _G_hPTEPartition;                       /*  PTE ������                  */
static UINT                     _G_uiVMSAShare = INNER_SHAREABLE;       /*  ����λֵ                    */
/*********************************************************************************************************
** ��������: arm64MmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� ARM64 MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                 �ڴ����Ȩ��
** �䡡��  : pucGuard               �����ϸ��Ȩ�޼��
**           pucXN                  ��ִ��Ȩ�ޱ�־
**           pucPXN                 ��Ȩ��ִ��Ȩ�ޱ�־
**           pucCon                 Contiguous ��־
**           pucnG                  nG ��־
**           pucAF                  �Ƿ�ӵ�з���Ȩ�ޱ�־
**           pucSH                  ����Ȩ�ޱ�־
**           pucAP                  �Ƿ��дȨ�ޱ�־
**           pucNS                  Non-Secure ��־
**           pucAIn                 Cache �� Bufferable Ȩ�ޱ�־
**           ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ӵ�з���Ȩ��            */
        if (ulFlag & LW_VMM_FLAG_GUARDED) {
            *pucGuard = GUARDED_CHK;
        } else {
            *pucGuard = GUARDED_NOT_CHK;
        }
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
** ��������: arm64MmuAttr2Flags
** ��������: ���� ARM64 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucGuard               �ϸ��Ȩ�޼��
**           ucXN                  ��ִ��Ȩ�ޱ�־
**           ucPXN                 ��Ȩ��ִ��Ȩ�ޱ�־
**           ucCon                 Contiguous ��־
**           ucnG                  nG ��־
**           ucAF                  �Ƿ�ӵ�з���Ȩ�ޱ�־
**           ucSH                  ����Ȩ�ޱ�־
**           ucAP                  �Ƿ��дȨ�ޱ�־
**           ucNS                  Non-Secure ��־
**           ucAIn                 Cache �� Bufferable Ȩ�ޱ�־
** �䡡��  : ulFlag                �ڴ����Ȩ��
**           ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucNS                    �Ƿ���ʰ�ȫ����
**           ucAP                    ����Ȩ��
**           ucXN                    ��ִ��Ȩ�ޱ�־
**           ucPXN                   ��Ȩ��ִ��Ȩ�ޱ�־
**           ucType                  ����������
** �䡡��  : һ��������
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuBuildPmdEntry
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
** ��������: arm64MmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ַ     (ҳ��ַ)
**           ucGuard                 �����ϸ��Ȩ�޼��
**           ucXN                    ��ִ��Ȩ�ޱ�־
**           ucPXN                   ��Ȩ��ִ��Ȩ�ޱ�־
**           ucCon                   Contiguous ��־
**           ucnG                    nG ��־
**           ucAF                    ���ʱ�־
**           ucSH                    ����Ȩ�ޱ�־
**           ucAP                    ����Ȩ�ޱ�־
**           ucNS                    Non-Secure ��־
**           ucAIn                   Attribute Index
**           ucType                  ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ARM ��ϵ�ṹҪ��: һ��ҳ�����ַ��Ҫ���� 16 KByte ����, ����Ŀӳ�� 1 MByte �ռ�.
                               ����ҳ�����ַ��Ҫ����  1 KByte ����, ����Ŀӳ�� 4 KByte �ռ�.
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
** ��������: arm64MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *arm64MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: arm64MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *arm64MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = *p_pgdentry;                                               /*  ���һ��ҳ��������          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ulTemp & ARM64_MMU_ADDR_MASK);   /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: arm64MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *arm64MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = *p_pmdentry;                                               /*  ��ö���ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(ulTemp & ARM64_MMU_ADDR_MASK);   /*  �������ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;                                   /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: arm64MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  arm64MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & ARM64_PGD_TYPE_MASK) == ARM64_PGD_TYPE_TABLE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: arm64MmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PMD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  arm64MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  (((pmdentry & ARM64_PMD_TYPE_MASK) == ARM64_PMD_TYPE_TABLE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: arm64MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  arm64MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry & ARM64_PTE_TYPE_MASK) == ARM64_PTE_TYPE_PAGE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: arm64MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *arm64MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;
    
    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
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
** ��������: arm64MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: arm64MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���:
** ����ģ��:
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

    *p_pgdentry = arm64MmuBuildPgdEntry((addr_t)p_pmdentry,             /*  ����һ��ҳ��������          */
                                        ARM64_MMU_NS_SECURE,
                                        ARM64_MMU_AP_NO_EFFECT,
                                        ARM64_MMU_XN_NO_EFFECT,
                                        ARM64_MMU_PXN_NO_EFFECT,
                                        ARM64_PGD_TYPE_TABLE);

#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    arm64DCacheFlush((PVOID)p_pgdentry, (PVOID)p_pgdentry, 32);         /*  ������������Ӱ��            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (arm64MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: arm64MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: arm64MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���:
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
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

    *p_pmdentry = arm64MmuBuildPmdEntry((addr_t)p_pteentry,             /*  ���ö���ҳ��������          */
                                        ARM64_MMU_NS_SECURE,
                                        ARM64_MMU_AP_NO_EFFECT,
                                        ARM64_MMU_XN_NO_EFFECT,
                                        ARM64_MMU_PXN_NO_EFFECT,
                                        ARM64_PMD_TYPE_TABLE);
    
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    arm64DCacheFlush((PVOID)p_pmdentry, (PVOID)p_pmdentry, 32);         /*  ������������Ӱ��            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (arm64MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: arm64MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: arm64MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  arm64MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(pteentry & ARM64_MMU_ADDR_MASK);        /*  ��������ַ                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arm64MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  arm64MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = arm64MmuPgdOffset(pmmuctx, ulAddr);/*  ��ȡһ��������              */
    INT                 iDescType;
    ULONG               ulFlag = 0;

    UINT8               ucGuard;                                        /*  �ϸ��Ȩ�޼��              */
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
    if (iDescType == ARM64_PGD_TYPE_BLOCK) {                            /*  ���ڶε�ӳ��                */
       return  (LW_VMM_FLAG_UNVALID);

    } else if (iDescType == ARM64_PGD_TYPE_TABLE) {                     /*  ��������ҳ��ӳ��            */
        LW_PMD_TRANSENTRY  *p_pmdentry = arm64MmuPmdOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  ��ȡ����������              */
        if (arm64MmuPmdIsOk(*p_pmdentry)) {
              LW_PTE_TRANSENTRY  *p_pteentry = arm64MmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                                 ulAddr);
                                                                        /*  ��ȡ����������              */
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
** ��������: arm64MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  arm64MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = arm64MmuPgdOffset(pmmuctx, ulAddr);/*  ��ȡһ��������              */
    INT                 iDescType;    
    
    UINT8               ucGuard;                                        /*  �ϸ��Ȩ�޼��              */
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
        ucType = ARM64_PTE_TYPE_PAGE;

    } else {
        ucType = ARM64_PTE_TYPE_FAULT;                                   /*  ���ʽ�ʧЧ                 */
    }

    if (arm64MmuFlags2Attr(ulFlag,
                           &ucGuard,
                           &ucXN, &ucPXN,
                           &ucContiguous,
                           &ucnG, &ucAF,
                           &ucSH, &ucAP,
                           &ucNS, &ucAttrIndx) < 0) {                   /*  ��Ч��ӳ���ϵ              */
        return (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == ARM64_PGD_TYPE_BLOCK) {                            /*  ���ڶε�ӳ��                */
        return  (ERROR_NONE);
    
    } else if (iDescType == ARM64_PGD_TYPE_TABLE) {                     /*  ��������ҳ��ӳ��            */
        LW_PMD_TRANSENTRY  *p_pmdentry = arm64MmuPmdOffset((LW_PGD_TRANSENTRY *)p_pgdentry,
                                                           ulAddr);     /*  ��ȡ����������              */
        if (arm64MmuPmdIsOk(*p_pmdentry)) {
            LW_PTE_TRANSENTRY  *p_pteentry = arm64MmuPteOffset((LW_PTE_TRANSENTRY *)p_pmdentry,
                                                               ulAddr);
                                                                        /*  ��ȡ����������              */
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
                                                                        /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: arm64MmuMakeTrans
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
static VOID  arm64MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                LW_PTE_TRANSENTRY  *p_pteentry,
                                addr_t              ulVirtualAddr,
                                phys_addr_t         paPhysicalAddr,
                                ULONG               ulFlag)
{
    UINT8               ucGuard;                                        /*  �ϸ��Ȩ�޼��              */
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
        ucType = ARM64_PTE_TYPE_PAGE;
    } else {
        ucType = ARM64_PTE_TYPE_FAULT;                                  /*  ���ʽ�ʧЧ                  */
    }

    if (arm64MmuFlags2Attr(ulFlag,
                           &ucGuard,
                           &ucXN, &ucPXN,
                           &ucContiguous,
                           &ucnG, &ucAF,
                           &ucSH, &ucAP,
                           &ucNS, &ucAttrIndx) < 0) {                   /*  ��Ч��ӳ���ϵ              */
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
    arm64DCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);         /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: arm64MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������, ����ʹ�� TTBR1 ҳ���ַ�Ĵ���.
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)pmmuctx->MMUCTX_pgdEntry);
          
    arm64MmuSetTTBR((PVOID)p_pgdentry);                                 /*  ����ҳ�����ַ              */
}
/*********************************************************************************************************
** ��������: arm64MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        arm64MmuInvalidateTLB();                                        /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            arm64MmuInvalidateTLBMVA((PVOID)ulPageAddr);                /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: arm64MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuShareableSet
** ��������: MMU ϵͳ share ģʽ���� (��� CACHE ʹ�� SNOOP ����ǰ����Ϊ OUTER_SHAREABLE)
** �䡡��  : bInnerOrOuter     0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arm64MmuShareableGet
** ��������: MMU ϵͳ share ģʽ��ȡ
** �䡡��  : NONE
** �䡡��  : 0: INNER_SHAREABLE  1: OUTER_SHAREABLE
** ȫ�ֱ���:
** ����ģ��:
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
