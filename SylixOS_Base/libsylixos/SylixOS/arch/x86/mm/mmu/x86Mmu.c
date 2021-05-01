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
** ��   ��   ��: x86Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 09 ��
**
** ��        ��: x86 ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/x86/common/x86Cr.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/pentium/x86Pentium.h"
/*********************************************************************************************************
  PDPTE PDE PTE �е�λ����
*********************************************************************************************************/
#define X86_MMU_PRESENT             (1)                                 /*  ӳ�����                    */
#define X86_MMU_PRESENT_SHIFT       (0)

#define X86_MMU_RW                  (1)                                 /*  ��д                        */
#define X86_MMU_RW_NO               (0)
#define X86_MMU_RW_SHIFT            (1)

#define X86_MMU_US                  (1)                                 /*  �û�ģʽ�ɷ���              */
#define X86_MMU_US_NO               (0)
#define X86_MMU_US_SHIFT            (2)

#define X86_MMU_PWT                 (1)                                 /*  д��͸                      */
#define X86_MMU_PWT_NO              (0)
#define X86_MMU_PWT_SHIFT           (3)

#define X86_MMU_PCD                 (1)                                 /*  ���� CACHE                  */
#define X86_MMU_PCD_NO              (0)
#define X86_MMU_PCD_SHIFT           (4)

#define X86_MMU_A                   (1)                                 /*  ���ʹ�                      */
#define X86_MMU_A_NO                (0)
#define X86_MMU_A_SHIFT             (5)

/*
 * ���� PDE ��Ч
 */
#define X86_MMU_PS                  (1)                                 /*  If CR4.PSE = 1, must be 0   */
#define X86_MMU_PS_NO               (0)                                 /*  otherwise, this entry maps a*/
#define X86_MMU_PS_SHIFT            (7)                                 /*  4-MByte page                */

/*
 * ���� PTE ��Ч
 */
#define X86_MMU_D                   (1)                                 /*  ��λ                        */
#define X86_MMU_D_NO                (0)
#define X86_MMU_D_SHIFT             (6)

/*
 * �����ڴ�����д�ϲ�ʱ, PAT = 1, PCD = 0, PWT = 1, ���ֵΪ 0b101=5, ��ѡ�� PA5,
 * IA32_PAT MSR �� PA5 ��Ҫ����Ϊ 0x01 �� Write Combining (WC)
 * �����ڴ�����, PAT = 0 ����
 */
#define X86_MMU_PAT                 (1)                                 /*  If the PAT is supported,    */
#define X86_MMU_PAT_NO              (0)                                 /*  indirectly determines       */
#define X86_MMU_PAT_SHIFT           (7)                                 /*  the memory type             */

#define X86_MMU_G                   (1)                                 /*  ȫ��                        */
#define X86_MMU_G_NO                (0)
#define X86_MMU_G_SHIFT             (8)
/*********************************************************************************************************
  PDPTE PDE PTE �е�����
*********************************************************************************************************/
#define X86_MMU_MASK                (0xfffff000)
/*********************************************************************************************************
  ҳ���������� 1/2 ʱ, ȫ�� TLB
*********************************************************************************************************/
#define X86_MMU_TLB_NR              (64)                                /*  Intel x86 vol 3 Table 11-1  */
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  x86MmuInvalidateTLB(VOID);
extern VOID  x86MmuInvalidateTLBMVA(PVOID  pvAddr);

extern VOID  x86MmuEnable(VOID);
extern VOID  x86MmuDisable(VOID);

extern VOID  x86DCacheFlush(PVOID  pvStart, size_t  stSize);
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
** ��������: x86MmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� x86 MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  SylixOS Ȩ�ޱ�־
**           pucRW                   �Ƿ��д
**           pucUS                   �Ƿ���ͨ�û�
**           pucPWT                  �Ƿ�д��͸
**           pucPCD                  �Ƿ� CACHE �ر�
**           pucA                    �Ƿ��ܷ���
**           pucPAT                  PAT λֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86MmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucRW,
                              UINT8  *pucUS,
                              UINT8  *pucPWT,
                              UINT8  *pucPCD,
                              UINT8  *pucA,
                              UINT8  *pucPAT)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucRW = X86_MMU_RW;

    } else {
        *pucRW = X86_MMU_RW_NO;
    }

    *pucUS  = X86_MMU_US_NO;                                            /*  ʼ�� supervisor             */
    *pucPAT = X86_MMU_PAT_NO;

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        *pucPCD = X86_MMU_PCD_NO;                                       /*  ��д                        */
        *pucPWT = X86_MMU_PWT_NO;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {
        *pucPCD = X86_MMU_PCD_NO;                                       /*  д��͸                      */
        *pucPWT = X86_MMU_PWT;

        if (ulFlag & LW_VMM_FLAG_WRITECOMBINING) {                      /*  д�ϲ�                      */
            *pucPAT = X86_MMU_PAT;
        }

    } else {
        *pucPCD = X86_MMU_PCD;                                          /*  UNCACHE                     */
        *pucPWT = X86_MMU_PWT;
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ɷ���                  */
        *pucA = X86_MMU_A;

    } else {
        *pucA = X86_MMU_A_NO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86MmuAttr2Flags
** ��������: ���� x86 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucPAT                   PAT λֵ
**           pulFlag                 SylixOS Ȩ�ޱ�־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86MmuAttr2Flags (UINT8   ucRW,
                              UINT8   ucUS,
                              UINT8   ucPWT,
                              UINT8   ucPCD,
                              UINT8   ucA,
                              UINT8   ucPAT,
                              ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucRW == X86_MMU_RW) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }

    if ((ucPCD == X86_MMU_PCD_NO) && (ucPWT == X86_MMU_PWT_NO)) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

    } else if (ucPCD == X86_MMU_PCD_NO) {
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
    }

    if (ucPAT == X86_MMU_PAT) {
        *pulFlag |= LW_VMM_FLAG_WRITECOMBINING;
    }

    if (ucA == X86_MMU_A) {
        *pulFlag |= LW_VMM_FLAG_ACCESS | LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86MmuBuildPgdesc
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucPAT                   PAT λֵ
** �䡡��  : һ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  x86MmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucRW,
                                             UINT8   ucUS,
                                             UINT8   ucPWT,
                                             UINT8   ucPCD,
                                             UINT8   ucA,
                                             UINT8   ucPAT)
{
    LW_PGD_TRANSENTRY  uiDescriptor;

    uiDescriptor = (uiBaseAddr & X86_MMU_MASK)
                 | (ucA   << X86_MMU_PRESENT_SHIFT)
                 | (ucRW  << X86_MMU_RW_SHIFT)
                 | (ucUS  << X86_MMU_US_SHIFT)
                 | (ucPWT << X86_MMU_PWT_SHIFT)
                 | (ucPCD << X86_MMU_PCD_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: x86MmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ַ     (ҳ��ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucPAT                   PAT λֵ
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  x86MmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucRW,
                                              UINT8   ucUS,
                                              UINT8   ucPWT,
                                              UINT8   ucPCD,
                                              UINT8   ucA,
                                              UINT8   ucPAT)
{
    LW_PTE_TRANSENTRY  uiDescriptor;
    
    uiDescriptor = (uiBaseAddr & X86_MMU_MASK)
                 | (ucA   << X86_MMU_PRESENT_SHIFT)
                 | (ucRW  << X86_MMU_RW_SHIFT)
                 | (ucUS  << X86_MMU_US_SHIFT)
                 | (ucPWT << X86_MMU_PWT_SHIFT)
                 | (ucPCD << X86_MMU_PCD_SHIFT)
                 | ((X86_FEATURE_HAS_PAT ? ucPAT : 0) << X86_MMU_PAT_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: x86MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : x86 ��ϵ�ṹҪ��: һ��ҳ�����ַ��Ҫ���� 4 KByte ����, ����Ŀӳ�� 4 MByte �ռ�.
                               ����ҳ�����ַ��Ҫ���� 4 KByte ����, ����Ŀӳ�� 4 KByte �ռ�.
*********************************************************************************************************/
static INT  x86MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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
** ��������: x86MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);
    
    x86MmuInvalidateTLB();
    
    if (X86_FEATURE_HAS_PAT) {                                          /*  �� PAT                      */
        UINT64  ulPat;

        x86PentiumMsrGet(X86_MSR_IA32_PAT, &ulPat);                     /*  ��� IA32_PAT MSR           */

        ulPat &= ~(0x7ULL << 40);                                       /*  ��� PA5                    */
        ulPat |= (0x1ULL << 40);                                        /*  PA5 = 1 Write Combining (WC)*/

        x86PentiumMsrSet(X86_MSR_IA32_PAT, &ulPat);                     /*  ���� IA32_PAT MSR           */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *x86MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: x86MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *x86MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  x86 �� PMD ��               */
}
/*********************************************************************************************************
** ��������: x86MmuPteOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *x86MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ���һ��ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & X86_MMU_MASK);          /*  ��ö���ҳ�����ַ          */

    ulAddr    &= ~LW_CFG_VMM_PGD_MASK;
    uiPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: x86MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  x86MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (X86_MMU_PRESENT << X86_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x86MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  x86MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (X86_MMU_PRESENT << X86_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x86MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *x86MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** ��������: x86MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x86MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: x86MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *x86MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: x86MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x86MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: x86MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *x86MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
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
    
    *p_pmdentry = (LW_PMD_TRANSENTRY)x86MmuBuildPgdesc((UINT32)p_pteentry,
                                                       X86_MMU_RW,
                                                       X86_MMU_US_NO,
                                                       X86_MMU_PWT_NO,
                                                       X86_MMU_PCD_NO,
                                                       X86_MMU_A,
                                                       X86_MMU_PAT_NO); /*  ����һ��ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x86MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: x86MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x86MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: x86MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x86MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(pteentry & X86_MMU_MASK);               /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  x86MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x86MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ          */
    UINT8               ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT;
    ULONG               ulFlag;

    if (x86MmuPgdIsOk(*p_pgdentry)) {                                   /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = x86MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (x86MmuPteIsOk(uiDescriptor)) {                              /*  ������������Ч              */
            ucRW  = (UINT8)((uiDescriptor >> X86_MMU_RW_SHIFT)      & 0x01);
            ucUS  = (UINT8)((uiDescriptor >> X86_MMU_US_SHIFT)      & 0x01);
            ucPWT = (UINT8)((uiDescriptor >> X86_MMU_PWT_SHIFT)     & 0x01);
            ucPCD = (UINT8)((uiDescriptor >> X86_MMU_PCD_SHIFT)     & 0x01);
            ucA   = (UINT8)((uiDescriptor >> X86_MMU_PRESENT_SHIFT) & 0x01);
            ucPAT = (UINT8)((uiDescriptor >> X86_MMU_PAT_SHIFT)     & 0x01);

            x86MmuAttr2Flags(ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT, &ulFlag);

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: x86MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  x86MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x86MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ          */
    UINT8               ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT;

    if (x86MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA, &ucPAT) != ERROR_NONE) { /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (x86MmuPgdIsOk(*p_pgdentry)) {                                   /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = x86MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (x86MmuPteIsOk(uiDescriptor)) {                              /*  ������������Ч              */
            addr_t  ulPhysicalAddr = (addr_t)(*p_pteentry & X86_MMU_MASK);

            *p_pteentry = x86MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                             ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT);
#if LW_CFG_CACHE_EN > 0
            x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: x86MmuMakeTrans
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
static VOID  x86MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
    UINT8  ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT;
    
    if (x86MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA, &ucPAT) != ERROR_NONE) { /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = x86MmuBuildPtentry((UINT32)paPhysicalAddr,
                                     ucRW, ucUS, ucPWT, ucPCD, ucA, ucPAT);
                                                        
#if LW_CFG_CACHE_EN > 0
    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: x86MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x86MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    UINT32  uiCr3Val;

    uiCr3Val  = ((UINT32)pmmuctx->MMUCTX_pgdEntry) & X86_MMU_MASK;

    uiCr3Val |= X86_MMU_PWT_NO << X86_MMU_PWT_SHIFT;
    uiCr3Val |= X86_MMU_PCD_NO << X86_MMU_PCD_SHIFT;

    x86Cr3Set(uiCr3Val);
}
/*********************************************************************************************************
** ��������: x86MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (X86_MMU_TLB_NR >> 1)) {
        x86MmuInvalidateTLB();                                          /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            x86MmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: x86MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  x86MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;

    pmmuop->MMUOP_pfuncMemInit    = x86MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = x86MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = x86MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = x86MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = x86MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = x86MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = x86MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = x86MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = x86MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = x86MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = x86MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = x86MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = x86MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = x86MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = x86MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = x86MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = x86MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = x86MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = x86MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = x86MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = x86MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = x86MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
