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
** ��   ��   ��: x64Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 09 ��
**
** ��        ��: x86-64 ��ϵ���� MMU ����.
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
  PML4E PDPTE PDE PTE �е�λ����
*********************************************************************************************************/
#define X64_MMU_PRESENT             (1)                                 /*  ӳ�����                    */
#define X64_MMU_PRESENT_SHIFT       (0)

#define X64_MMU_RW                  (1)                                 /*  ��д                        */
#define X64_MMU_RW_NO               (0)
#define X64_MMU_RW_SHIFT            (1)

#define X64_MMU_US                  (1)                                 /*  �û�ģʽ�ɷ���              */
#define X64_MMU_US_NO               (0)
#define X64_MMU_US_SHIFT            (2)

#define X64_MMU_PWT                 (1)                                 /*  д��͸                      */
#define X64_MMU_PWT_NO              (0)
#define X64_MMU_PWT_SHIFT           (3)

#define X64_MMU_PCD                 (1)                                 /*  ���� CACHE                  */
#define X64_MMU_PCD_NO              (0)
#define X64_MMU_PCD_SHIFT           (4)

#define X64_MMU_A                   (1)                                 /*  ���ʹ�                      */
#define X64_MMU_A_NO                (0)
#define X64_MMU_A_SHIFT             (5)

#define X64_MMU_XD                  (1)                                 /*  ��ִֹ��                    */
#define X64_MMU_XD_NO               (0)
#define X64_MMU_XD_SHIFT            (63)
/*********************************************************************************************************
  ���� PDPTE PDE ��Ч
  PDPTE �� PS ��־λ��ʾ�Ƿ��ṩ 1G ������ҳ���ַ��0 Ϊ 4K �� 2M ҳ��
  PDE �� PS ��־λ��ʾ�Ƿ��ṩ 2M ������ҳ���ַ��0 Ϊ 4K ҳ��
*********************************************************************************************************/
#define X64_MMU_PS                  (1)
#define X64_MMU_PS_NO               (0)
#define X64_MMU_PS_SHIFT            (7)
/*********************************************************************************************************
  ���� PTE ��Ч
*********************************************************************************************************/
#define X64_MMU_D                   (1)                                 /*  ��λ                        */
#define X64_MMU_D_NO                (0)
#define X64_MMU_D_SHIFT             (6)

/*
 * �����ڴ�����д�ϲ�ʱ, PAT = 1, PCD = 0, PWT = 1, ���ֵΪ 0b101=5, ��ѡ�� PA5,
 * IA32_PAT MSR �� PA5 ��Ҫ����Ϊ 0x01 �� Write Combining (WC)
 * �����ڴ�����, PAT = 0 ����
 */
#define X64_MMU_PAT                 (1)                                 /*  If the PAT is supported,    */
#define X64_MMU_PAT_NO              (0)                                 /*  indirectly determines       */
#define X64_MMU_PAT_SHIFT           (7)                                 /*  the memory type             */

#define X64_MMU_G                   (1)                                 /*  ȫ��                        */
#define X64_MMU_G_NO                (0)
#define X64_MMU_G_SHIFT             (8)
/*********************************************************************************************************
  PML4E PDPTE PDE PTE �е�����
*********************************************************************************************************/
#define X64_MMU_MASK                (0xffffffffff000ul)                 /*  [51:12]                     */
/*********************************************************************************************************
  ҳ���������� 1/2 ʱ, ȫ�� TLB
*********************************************************************************************************/
#define X64_MMU_TLB_NR              (128)                               /*  Intel x86 vol 3 Table 11-1  */
/*********************************************************************************************************
  �����ַ�Ĵ�С
*********************************************************************************************************/
#define X64_MMU_VIRT_ADDR_SIZE      (48)
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  x64MmuInvalidateTLB(VOID);
extern VOID  x64MmuInvalidateTLBMVA(PVOID  pvAddr);

extern VOID  x64MmuEnable(VOID);
extern VOID  x64MmuDisable(VOID);

extern VOID  x86DCacheFlush(PVOID  pvStart, size_t  stSize);
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD ������                  */
static LW_OBJECT_HANDLE     _G_hPTSPartition;                           /*  PTS ������                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
** ��������: x64MmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� x86 MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  SylixOS Ȩ�ޱ�־
**           pucRW                   �Ƿ��д
**           pucUS                   �Ƿ���ͨ�û�
**           pucPWT                  �Ƿ�д��͸
**           pucPCD                  �Ƿ� CACHE �ر�
**           pucA                    �Ƿ��ܷ���
**           pucXD                   �Ƿ��ִֹ��
**           pucPAT                  PAT λֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x64MmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucRW,
                              UINT8  *pucUS,
                              UINT8  *pucPWT,
                              UINT8  *pucPCD,
                              UINT8  *pucA,
                              UINT8  *pucXD,
                              UINT8  *pucPAT)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucRW = X64_MMU_RW;

    } else {
        *pucRW = X64_MMU_RW_NO;
    }

    *pucUS  = X64_MMU_US_NO;                                            /*  ʼ�� supervisor             */
    *pucPAT = X64_MMU_PAT_NO;

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д CACHE                  */
        *pucPCD = X64_MMU_PCD_NO;                                       /*  ��д                        */
        *pucPWT = X64_MMU_PWT_NO;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸                      */
        *pucPCD = X64_MMU_PCD_NO;
        *pucPWT = X64_MMU_PWT;

        if (ulFlag & LW_VMM_FLAG_WRITECOMBINING) {                      /*  д�ϲ�                      */
            *pucPAT = X64_MMU_PAT;
        }

    } else {
        *pucPCD = X64_MMU_PCD;                                          /*  UNCACHE                     */
        *pucPWT = X64_MMU_PWT;
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ɷ���                  */
        *pucA = X64_MMU_A;

    } else {
        *pucA = X64_MMU_A_NO;
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {                                /*  �Ƿ��ִ��                  */
        *pucXD = X64_MMU_XD_NO;

    } else {
        *pucXD = X64_MMU_XD;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x64MmuAttr2Flags
** ��������: ���� x86 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucXD                    �Ƿ��ִֹ��
**           ucPAT                   PAT λֵ
**           pulFlag                 SylixOS Ȩ�ޱ�־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x64MmuAttr2Flags (UINT8   ucRW,
                              UINT8   ucUS,
                              UINT8   ucPWT,
                              UINT8   ucPCD,
                              UINT8   ucA,
                              UINT8   ucXD,
                              UINT8   ucPAT,
                              ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucRW == X64_MMU_RW) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }

    if ((ucPCD == X64_MMU_PCD_NO) && (ucPWT == X64_MMU_PWT_NO)) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

    } else if (ucPCD == X64_MMU_PCD_NO) {
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
    }

    if (ucPAT == X64_MMU_PAT) {
        *pulFlag |= LW_VMM_FLAG_WRITECOMBINING;
    }

    if (ucA == X64_MMU_A) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }

    if (ucXD == X64_MMU_XD_NO) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x64MmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucXD                    �Ƿ��ִֹ��
**           ucPAT                   PAT λֵ
** �䡡��  : һ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE LW_PGD_TRANSENTRY  x64MmuBuildPgdEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD,
                                                         UINT8   ucPAT)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: x64MmuBuildPmdEntry
** ��������: ����һ������������ (PMD ������)
** �䡡��  : ulBaseAddr              ����ַ     (����ҳ�����ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucXD                    �Ƿ��ִֹ��
**           ucPAT                   PAT λֵ
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PMD_TRANSENTRY  x64MmuBuildPmdEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD,
                                                         UINT8   ucPAT)
{
    LW_PMD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: x64MmuBuildPtsEntry
** ��������: ����һ������������ (PTS ������)
** �䡡��  : ulBaseAddr              ����ַ     (�ļ�ҳ�����ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucXD                    �Ƿ��ִֹ��
**           ucPAT                   PAT λֵ
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PTS_TRANSENTRY  x64MmuBuildPtsEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD,
                                                         UINT8   ucPAT)
{
    LW_PTS_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: x64MmuBuildPteEntry
** ��������: ����һ���ļ������� (PTE ������)
** �䡡��  : ulBaseAddr              ����ַ     (ҳ����ַ)
**           ucRW                    �Ƿ��д
**           ucUS                    �Ƿ���ͨ�û�
**           ucPWT                   �Ƿ�д��͸
**           ucPCD                   �Ƿ� CACHE �ر�
**           ucA                     �Ƿ��ܷ���
**           ucXD                    �Ƿ��ִֹ��
**           ucPAT                   PAT λֵ
** �䡡��  : �ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE LW_PTE_TRANSENTRY  x64MmuBuildPteEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD,
                                                         UINT8   ucPAT)
{
    LW_PTE_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT)
                 | ((X86_FEATURE_HAS_PAT ? ucPAT : 0) << X64_MMU_PAT_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** ��������: x64MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x64MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTS_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPtsTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPtsNum = bspMmuPtsMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * PMD_BLOCK_SIZE, PMD_BLOCK_SIZE);
    pvPtsTable = __KHEAP_ALLOC_ALIGN((size_t)ulPtsNum * PTS_BLOCK_SIZE, PTS_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!pvPgdTable || !pvPmdTable || !pvPtsTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPMDPartition = API_PartitionCreate("pmd_pool", pvPmdTable, ulPmdNum, PMD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTSPartition = API_PartitionCreate("pts_pool", pvPtsTable, ulPtsNum, PTS_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                           
    if (!_G_hPGDPartition || !_G_hPMDPartition || !_G_hPTSPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x64MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x64MmuGlobalInit (CPCHAR  pcMachineName)
{
    UINT64  uiMsr;

    archCacheReset(pcMachineName);

    x64MmuInvalidateTLB();
    
    x86PentiumMsrGet(X86_MSR_IA32_EFER, &uiMsr);
    uiMsr |= X86_IA32_EFER_NXE;
    x86PentiumMsrSet(X86_MSR_IA32_EFER, &uiMsr);

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
** ��������: x64MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *x64MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    if (ulAddr & (~((1ULL << X64_MMU_VIRT_ADDR_SIZE) - 1))) {           /*  ���ںϷ��������ַ�ռ���    */
        return  (LW_NULL);
    }

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: x64MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *x64MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = (LW_PGD_TRANSENTRY)(*p_pgdentry);                          /*  ���һ��ҳ��������          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  ��ö���ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  ���� PMD ��                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  ��ö���ҳ����������ַ      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** ��������: x64MmuPtsOffset
** ��������: ͨ�������ַ���� PTS ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTS �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY  *x64MmuPtsOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTS_TRANSENTRY  *p_ptsentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPtsNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  ��ö���ҳ��������          */

    p_ptsentry = (LW_PTS_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  �������ҳ�����ַ          */

    ulAddr    &= LW_CFG_VMM_PTS_MASK;
    ulPtsNum   = ulAddr >> LW_CFG_VMM_PTS_SHIFT;                        /*  ���� PTS ��                 */

    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry |
                 (ulPtsNum * sizeof(LW_PTS_TRANSENTRY)));               /*  �������ҳ����������ַ      */

    return  (p_ptsentry);
}
/*********************************************************************************************************
** ��������: x64MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_ptsentry     pts ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *x64MmuPteOffset (LW_PTS_TRANSENTRY  *p_ptsentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PTS_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PTS_TRANSENTRY)(*p_ptsentry);                          /*  �������ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  ����ļ�ҳ�����ַ          */

    ulAddr    &= 0x1fful << LW_CFG_VMM_PAGE_SHIFT;                      /*  ��Ҫʹ��LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  �������ҳ��                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: x64MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  x64MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x64MmuPmdIsOk
** ��������: �ж� PMD ����������Ƿ���ȷ
** �䡡��  : pmdentry       PMD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  x64MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  ((pmdentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x64MmuPtsIsOk
** ��������: �ж� PTS ����������Ƿ���ȷ
** �䡡��  : ptsentry       PTS ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  x64MmuPtsIsOk (LW_PTS_TRANSENTRY  ptsentry)
{
    return  ((ptsentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x64MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  x64MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: x64MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *x64MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;
    
    if (ulAddr & (~((1ULL << X64_MMU_VIRT_ADDR_SIZE) - 1))) {           /*  ���ںϷ��������ַ�ռ���    */
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
** ��������: x64MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x64MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: x64MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *x64MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
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

    *p_pgdentry = x64MmuBuildPgdEntry((addr_t)p_pmdentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO,
                                      X64_MMU_PAT_NO);                  /*  ����һ��ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pgdentry, sizeof(LW_PGD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: x64MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x64MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** ��������: x64MmuPtsAlloc
** ��������: ���� PTS ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTS ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY *x64MmuPtsAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PMD_TRANSENTRY  *p_pmdentry,
                                          addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTS_TRANSENTRY  *p_ptsentry = (LW_PTS_TRANSENTRY *)API_PartitionGet(_G_hPTSPartition);

    if (!p_ptsentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_ptsentry, PTS_BLOCK_SIZE);

    *p_pmdentry = x64MmuBuildPmdEntry((addr_t)p_ptsentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO,
                                      X64_MMU_PAT_NO);                  /*  ���ö���ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPtsOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: x64MmuPtsFree
** ��������: �ͷ� PTS ��
** �䡡��  : p_ptsentry     pts ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x64MmuPtsFree (LW_PTS_TRANSENTRY  *p_ptsentry)
{
    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry & (~(PTS_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTSPartition, (PVOID)p_ptsentry);
}
/*********************************************************************************************************
** ��������: x64MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *x64MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PTS_TRANSENTRY  *p_ptsentry,
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
    
    *p_ptsentry = x64MmuBuildPtsEntry((addr_t)p_pteentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO,
                                      X64_MMU_PAT_NO);                  /*  ��������ҳ��������          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_ptsentry, sizeof(LW_PTS_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPteOffset(p_ptsentry, ulAddr));
}
/*********************************************************************************************************
** ��������: x64MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x64MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: x64MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  x64MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(pteentry & X64_MMU_MASK);               /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x64MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  x64MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x64MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ          */

    if (p_pgdentry && x64MmuPgdIsOk(*p_pgdentry)) {                     /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = x64MmuPmdOffset(p_pgdentry,
                                                         ulAddr);       /*  ��ö�����������ַ          */

        if (x64MmuPmdIsOk(*p_pmdentry)) {                               /*  ������������Ч              */
            LW_PTS_TRANSENTRY  *p_ptsentry = x64MmuPtsOffset(p_pmdentry,
                                                             ulAddr);   /*  ���������������ַ          */

            if (x64MmuPtsIsOk(*p_ptsentry)) {                           /*  ������������Ч              */
                LW_PTE_TRANSENTRY  *p_pteentry = x64MmuPteOffset(p_ptsentry,
                                                                ulAddr);/*  ����ļ���������ַ          */
                LW_PTE_TRANSENTRY   pteentry = *p_pteentry;             /*  ����ļ�������              */

                if (x64MmuPteIsOk(pteentry)) {                          /*  �ļ���������Ч              */
                    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD, ucPAT;
                    ULONG   ulFlag;

                    ucRW  = (UINT8)((pteentry >> X64_MMU_RW_SHIFT)      & 0x01);
                    ucUS  = (UINT8)((pteentry >> X64_MMU_US_SHIFT)      & 0x01);
                    ucPWT = (UINT8)((pteentry >> X64_MMU_PWT_SHIFT)     & 0x01);
                    ucPCD = (UINT8)((pteentry >> X64_MMU_PCD_SHIFT)     & 0x01);
                    ucA   = (UINT8)((pteentry >> X64_MMU_PRESENT_SHIFT) & 0x01);
                    ucXD  = (UINT8)((pteentry >> X64_MMU_XD_SHIFT)      & 0x01);
                    ucPAT = (UINT8)((pteentry >> X64_MMU_PAT_SHIFT)     & 0x01);

                    x64MmuAttr2Flags(ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD, ucPAT, &ulFlag);

                    return  (ulFlag);
                }
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: x64MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  x64MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD, ucPAT;

    if (x64MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA,
                         &ucXD,  &ucPAT) != ERROR_NONE) {               /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = x64MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ          */

    if (p_pgdentry && x64MmuPgdIsOk(*p_pgdentry)) {                     /*  һ����������Ч              */
        LW_PMD_TRANSENTRY  *p_pmdentry = x64MmuPmdOffset(p_pgdentry,
                                                         ulAddr);       /*  ��ö�����������ַ          */

        if (x64MmuPmdIsOk(*p_pmdentry)) {                               /*  ������������Ч              */
            LW_PTS_TRANSENTRY  *p_ptsentry = x64MmuPtsOffset(p_pmdentry,
                                                             ulAddr);   /*  ���������������ַ          */

            if (x64MmuPtsIsOk(*p_ptsentry)) {                           /*  ������������Ч              */
                LW_PTE_TRANSENTRY  *p_pteentry = x64MmuPteOffset(p_ptsentry,
                                                                ulAddr);/*  ����ļ���������ַ         */
                LW_PTE_TRANSENTRY   pteentry = *p_pteentry;             /*  ����ļ�������              */

                if (x64MmuPteIsOk(pteentry)) {                          /*  �ļ���������Ч              */
                    addr_t  ulPhysicalAddr = (addr_t)(pteentry & X64_MMU_MASK);

                    *p_pteentry = x64MmuBuildPteEntry(ulPhysicalAddr,
                                                      ucRW,  ucUS, ucPWT,
                                                      ucPCD, ucA,  ucXD, ucPAT);
#if LW_CFG_CACHE_EN > 0
                    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                    return  (ERROR_NONE);
                }
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: x64MmuMakeTrans
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
static VOID  x64MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD, ucPAT;
    
    if (x64MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA,
                         &ucXD,  &ucPAT) != ERROR_NONE) {               /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = x64MmuBuildPteEntry((addr_t)paPhysicalAddr,
                                      ucRW,  ucUS, ucPWT,
                                      ucPCD, ucA,  ucXD, ucPAT);
                                                        
#if LW_CFG_CACHE_EN > 0
    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: x64MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  x64MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    X86_CR_REG  ulCr3Val;

    ulCr3Val  = ((X86_CR_REG)pmmuctx->MMUCTX_pgdEntry) & X64_MMU_MASK;

    ulCr3Val |= X64_MMU_PWT_NO << X64_MMU_PWT_SHIFT;
    ulCr3Val |= X64_MMU_PCD_NO << X64_MMU_PCD_SHIFT;

    x86Cr3Set(ulCr3Val);
}
/*********************************************************************************************************
** ��������: x64MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x64MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (X64_MMU_TLB_NR >> 1)) {
        x64MmuInvalidateTLB();                                          /*  ȫ����� TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            x64MmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  ���ҳ����� TLB            */
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

    pmmuop->MMUOP_pfuncMemInit    = x64MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = x64MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = x64MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = x64MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = x64MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = x64MmuPmdFree;
    pmmuop->MMUOP_pfuncPTSAlloc = x64MmuPtsAlloc;
    pmmuop->MMUOP_pfuncPTSFree  = x64MmuPtsFree;
    pmmuop->MMUOP_pfuncPTEAlloc = x64MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = x64MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = x64MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = x64MmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTSIsOk = x64MmuPtsIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = x64MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = x64MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = x64MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTSOffset = x64MmuPtsOffset;
    pmmuop->MMUOP_pfuncPTEOffset = x64MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = x64MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = x64MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = x64MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = x64MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = x64MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = x64MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = x64MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = x64MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
