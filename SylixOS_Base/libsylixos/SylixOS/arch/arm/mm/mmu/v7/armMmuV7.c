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
** ��   ��   ��: armMmuV7.c
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv7 ��ϵ���� MMU ����.
**
** BUG:
2014.05.24  ARMv7 ���� L2 CACHE ����һ�� CACHE ��ҪΪд��ģʽ.
2014.09.04  �ع� CACHE Ȩ��, L1, L2 �ɷֱ���ƻ�д��д��ģʽ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#if LW_CFG_CPU_PHYS_ADDR_64BIT == 0
#include "../armMmuCommon.h"
#include "../../cache/armCacheCommon.h"
#include "../../../common/cp15/armCp15.h"
#include "../../../param/armParam.h"
/*********************************************************************************************************
  һ�����������Ͷ���
*********************************************************************************************************/
#define COARSE_TBASE        (1)                                         /*  ����������ҳ�����ַ        */
#define SEGMENT_BASE        (2)                                         /*  ��ӳ�����ַ                */
/*********************************************************************************************************
  �������������Ͷ���
*********************************************************************************************************/
#define FAIL_DESC           (0)                                         /*  �任ʧЧ                    */
#define SMALLPAGE_DESC      (2)                                         /*  Сҳ����ַ                  */
/*********************************************************************************************************
  ������λ
*********************************************************************************************************/
#define DOMAIN_FAIL         (0)                                         /*  ��������ʧЧ                */
#define DOMAIN_CHECK        (1)                                         /*  ����Ȩ�޼��                */
#define DOMAIN_NOCHK        (3)                                         /*  ������Ȩ�޼��              */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define DOMAIN_ATTR         ((DOMAIN_CHECK)      |  \
                             (DOMAIN_NOCHK << 2) |  \
                             (DOMAIN_FAIL  << 4))
/*********************************************************************************************************
  3 ������
*********************************************************************************************************/
#define ACCESS_AND_CHK      0                                           /*  0 ����                      */
#define ACCESS_NOT_CHK      1                                           /*  1 ����                      */
#define ACCESS_FAIL         2                                           /*  2 ����                      */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
/*********************************************************************************************************
  �ڴ�ӳ����������
*********************************************************************************************************/
static BOOL                 _G_bVMSAForceShare = LW_FALSE;              /*  ǿ��Ϊ����ģʽ              */
static UINT                 _G_uiVMSANonSec    = 0;                     /*  Ĭ��Ϊ��ȫģʽ              */
static UINT                 _G_uiVMSAShare;                             /*  ����λֵ                    */
static UINT                 _G_uiVMSADevType   = ARM_MMU_V7_DEV_SHAREABLE;
/*********************************************************************************************************
  ȫ�ֶ���
*********************************************************************************************************/
#define VMSA_FORCE_S        _G_bVMSAForceShare                          /*  ǿ��Ϊ����ģʽ              */
#define VMSA_S              _G_uiVMSAShare                              /*  ����λֵ                    */
#define VMSA_NS             _G_uiVMSANonSec                             /*  �ǰ�ȫλֵ                  */
#define VMSA_nG             0ul                                         /*  ��ȫ��λֵ                  */
/*********************************************************************************************************
  ��ຯ��
*********************************************************************************************************/
extern UINT32 armMmuV7GetTTBCR(VOID);
extern VOID   armMmuV7SetTTBCR(UINT32 uiTTBCR);
/*********************************************************************************************************
** ��������: armMmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� ARM MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  �ڴ����Ȩ��
**           pucAP                   ����Ȩ��
**           pucDomain               ����������
**           pucCB                   CACHE ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucAP,
                              UINT8  *pucAP2,
                              UINT8  *pucDomain,
                              UINT8  *pucCB,
                              UINT8  *pucTEX,
                              UINT8  *pucXN)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        *pucDomain = ACCESS_FAIL;
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ӵ�з���Ȩ��            */
        if (ulFlag & LW_VMM_FLAG_GUARDED) {
            *pucDomain = ACCESS_AND_CHK;                                /*  ����Ȩ�޼��                */
        } else {
            *pucDomain = ACCESS_NOT_CHK;                                /*  ������Ȩ�޼��              */
        }
    } else {
        *pucDomain = ACCESS_FAIL;                                       /*  ����ʧЧ                    */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucAP2 = 0x0;                                                  /*  ��д                        */
        *pucAP  = 0x3;
    } else {
        *pucAP2 = 0x1;                                                  /*  ֻ��                        */
        *pucAP  = 0x3;
    }

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  ��д                        */
        if (LW_NCPUS > 1) {                                             /*  SMP д����                  */
            *pucTEX = 0x1;                                              /*  Outer: ��д, д����         */
        } else {
            *pucTEX = 0x0;                                              /*  Outer: ��д, ���߱�д����   */
        }
        *pucCB = 0x3;                                                   /*  Inner: ��д, д����         */

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  д��͸                      */
        *pucTEX = 0x0;                                                  /*  Outer: д��͸, ���߱�д���� */
        *pucCB  = 0x2;                                                  /*  Inner: д��͸, ���߱�д���� */

    } else {
        switch (_G_uiVMSADevType) {
        
        case ARM_MMU_V7_DEV_SHAREABLE:
            *pucTEX = 0x0;
            *pucCB  = 0x1;
            break;
        
        case ARM_MMU_V7_DEV_NON_SHAREABLE:
            *pucTEX = 0x2;
            *pucCB  = 0x0;
            break;
            
        default:                                                        /*  ǿ�������豸�ڴ�          */
            *pucTEX = 0x0;
            *pucCB  = 0x0;
            break;
        }
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        *pucXN = 0x0;
    } else {
        *pucXN = 0x1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuAttr2Flags
** ��������: ���� ARM MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucAP                    ����Ȩ��
**           ucAP2                   ����Ȩ��
**           ucDomain                ����������
**           ucCB                    CACHE ���Ʋ���
**           ucTEX                   CACHE ���Ʋ���
**           ucXN                    ��ִ��Ȩ��
**           pulFlag                 �ڴ����Ȩ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuAttr2Flags (UINT8  ucAP,
                              UINT8  ucAP2,
                              UINT8  ucDomain,
                              UINT8  ucCB,
                              UINT8  ucTEX,
                              UINT8  ucXN,
                              ULONG *pulFlag)
{
    (VOID)ucAP;
    (VOID)ucTEX;

    *pulFlag = LW_VMM_FLAG_VALID;
    
    if (ucDomain == ACCESS_AND_CHK) {
        *pulFlag |= LW_VMM_FLAG_GUARDED;
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    
    } else if (ucDomain == ACCESS_NOT_CHK) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }
    
    if (ucAP2 == 0) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }
    
    switch (ucCB) {
    
    case 0x3:
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
        break;
        
    case 0x2:
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
        break;

    default:
        break;
    }
    
    if (ucXN == 0x0) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuBuildPgdesc
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ַ     (�λ���ַ������ҳ�����ַ)
**           ucAP                    ����Ȩ��
**           ucDomain                ��
**           ucCB                    CACHE �� WRITEBUFFER ����
**           ucType                  һ������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  armMmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucAP,
                                             UINT8   ucAP2,
                                             UINT8   ucDomain,
                                             UINT8   ucCB,
                                             UINT8   ucTEX,
                                             UINT8   ucXN,
                                             UINT8   ucType)
{
    LW_PGD_TRANSENTRY   uiDescriptor;

    switch (ucType) {
    
    case COARSE_TBASE:                                                  /*  �����ȶ���ҳ��������        */
        uiDescriptor = (uiBaseAddr & 0xFFFFFC00)
                     | (ucDomain <<  5)
                     | (VMSA_NS  <<  3)
                     | ucType;
        break;
        
    case SEGMENT_BASE:                                                  /*  ��������                    */
        uiDescriptor = (uiBaseAddr & 0xFFF00000)
                     | (VMSA_NS  << 19)
                     | (VMSA_nG  << 17)
                     | (VMSA_S   << 16)
                     | (ucAP2    << 15)
                     | (ucTEX    << 12)
                     | (ucAP     << 10)
                     | (ucDomain <<  5)
                     | (ucXN     <<  4)
                     | (ucCB     <<  2)
                     | ucType;
        break;
        
    default:
        uiDescriptor = 0;                                               /*  ����ʧЧ                    */
        break;
    }
    
    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: armMmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ַ     (ҳ��ַ)
**           ucAP                    ����Ȩ��
**           ucDomain                ��
**           ucCB                    CACHE �� WRITEBUFFER ����
**           ucType                  ��������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  armMmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucAP,
                                              UINT8   ucAP2,
                                              UINT8   ucDomain,
                                              UINT8   ucCB,
                                              UINT8   ucTEX,
                                              UINT8   ucXN,
                                              UINT8   ucType)
{
    LW_PTE_TRANSENTRY   uiDescriptor;

    switch (ucType) {
    
    case SMALLPAGE_DESC:                                                /*  Сҳ������                  */
        uiDescriptor = (uiBaseAddr & 0xFFFFF000)
                     | (VMSA_nG  << 11)
                     | (VMSA_S   << 10)
                     | (ucAP2    <<  9)
                     | (ucTEX    <<  6)
                     | (ucAP     <<  4)
                     | (ucCB     <<  2)
                     | (ucXN     <<  0)
                     | ucType;
        break;

    default:
        uiDescriptor = 0;                                               /*  ����ʧЧ                    */
        break;
    }
    
    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: armMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ARM ��ϵ�ṹҪ��: һ��ҳ�����ַ��Ҫ���� 16 KByte ����, ����Ŀӳ�� 1 MByte �ռ�.
                               ����ҳ�����ַ��Ҫ����  1 KByte ����, ����Ŀӳ�� 4 KByte �ռ�.
*********************************************************************************************************/
static INT  armMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 1 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();
    
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
** ��������: armMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuGlobalInit (CPCHAR  pcMachineName)
{
    ARM_PARAM   *param = archKernelParamGet();
    
    archCacheReset(pcMachineName);

    armMmuInvalidateTLB();

    armMmuSetDomain(DOMAIN_ATTR);
    armMmuSetProcessId(0);

    /*
     *  ��ַ���ѡ�� (Qt �����õ��˷Ƕ���ָ��)
     *  ע ��: ���ʹ�ܵ�ַ������, GCC ���������� -mno-unaligned-access ѡ�� (�����ɷǶ������ָ��)
     */
    if (param->AP_bUnalign) {
        armMmuDisableAlignFault();
        
    } else {
        armMmuEnableAlignFault();                                       /*  -mno-unaligned-access       */
    }
    
    armControlFeatureDisable(CP15_CONTROL_TEXREMAP);                    /*  Disable TEX remapping       */

    armMmuV7SetTTBCR(0);                                                /*  Use the 32-bit translation  */
                                                                        /*  system, Always use TTBR0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  ARM �� PMD ��               */
}
/*********************************************************************************************************
** ��������: armMmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *armMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    
    uiTemp = (UINT32)(*p_pmdentry);                                     /*  ��ö���ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & (~(LW_CFG_KB_SIZE - 1)));
                                                                        /*  ��ô����ȶ���ҳ�����ַ    */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | ((ulAddr >> 10) & 0x3FC));                             /*  ��ö�Ӧ�����ַҳ��������  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: armMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  armMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  armMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    /*
     * ע��, SMALLPAGE ���������λ�� XN λ, �������� 0x02, ������ 0x03
     */
    return  (((pteentry & 0x02) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  �µ� PGD ����Ч��ҳ����     */
        
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  pgd offset                  */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~((16 * LW_CFG_KB_SIZE) - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: armMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *armMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
                                           addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG              iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    UINT8               ucAP;                                           /*  �洢Ȩ��                    */
    UINT8               ucDomain;                                       /*  ��                          */
    UINT8               ucCB;                                           /*  CACHE �뻺��������          */
    UINT8               ucAP2;                                          /*  �洢Ȩ��                    */
    UINT8               ucTEX;                                          /*  CACHE �뻺��������          */
    UINT8               ucXN;                                           /*  ����ִ��λ                  */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    armMmuFlags2Attr(LW_VMM_FLAG_VALID   |
                     LW_VMM_FLAG_ACCESS  |
                     LW_VMM_FLAG_GUARDED |
                     LW_VMM_FLAG_WRITABLE|
                     LW_VMM_FLAG_EXECABLE,
                     &ucAP, &ucAP2,
                     &ucDomain,
                     &ucCB, &ucTEX,
                     &ucXN);

    *p_pmdentry = (LW_PMD_TRANSENTRY)armMmuBuildPgdesc((UINT32)p_pteentry,
                                                       ucAP, ucAP2,
                                                       ucDomain,
                                                       ucCB, ucTEX,
                                                       ucXN,
                                                       COARSE_TBASE);   /*  ���ö���ҳ�����ַ          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    armDCacheFlush((PVOID)p_pmdentry, (PVOID)p_pmdentry, 32);           /*  ������������Ӱ��            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (armMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: armMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(LW_CFG_KB_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: armMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           ppaPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, phys_addr_t  *ppaPhysicalAddr)
{
    *ppaPhysicalAddr = (addr_t)(pteentry & (UINT32)0xFFFFF000);         /*  ��������ַ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  armMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucAP;                                           /*  �洢Ȩ��                    */
    UINT8               ucDomain;                                       /*  ��                          */
    UINT8               ucCB;                                           /*  CACHE �뻺��������          */
    UINT8               ucAP2;                                          /*  �洢Ȩ��                    */
    UINT8               ucTEX;                                          /*  CACHE �뻺��������          */
    UINT8               ucXN;                                           /*  ����ִ��λ                  */
    ULONG               ulFlag = 0;
    
    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == SEGMENT_BASE) {                                    /*  ���ڶε�ӳ��                */
        UINT32  uiDescriptor = (UINT32)(*p_pgdentry);
        
        ucCB     = (UINT8)((uiDescriptor >>  2) & 0x03);
        ucDomain = (UINT8)((uiDescriptor >>  5) & 0x0F);
        ucAP     = (UINT8)((uiDescriptor >> 10) & 0x02);

        ucAP2    = (UINT8)((uiDescriptor >> 15) & 0x01);
        ucTEX    = (UINT8)((uiDescriptor >> 12) & 0x07);

        ucXN     = (UINT8)((uiDescriptor >>  4) & 0x01);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  ���ڴ����ȶ���ҳ��ӳ��      */
        UINT32              uiDescriptor = (UINT32)(*p_pgdentry);
        LW_PTE_TRANSENTRY  *p_pteentry   = armMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);
        
        if (armMmuPteIsOk(*p_pteentry)) {
            ucDomain     = (UINT8)((uiDescriptor >>  5) & 0x0F);
            
            uiDescriptor = (UINT32)(*p_pteentry);
            ucCB         = (UINT8)((uiDescriptor >>  2) & 0x03);
            ucAP         = (UINT8)((uiDescriptor >>  4) & 0x03);

            ucAP2        = (UINT8)((uiDescriptor >>  9) & 0x01);
            ucTEX        = (UINT8)((uiDescriptor >>  6) & 0x07);

            ucXN         = (UINT8)((uiDescriptor >>  0) & 0x01);
        
        } else {
            return  (LW_VMM_FLAG_UNVALID);
        }
    } else {
        return  (LW_VMM_FLAG_UNVALID);
    }
    
    armMmuAttr2Flags(ucAP, ucAP2, ucDomain, ucCB, ucTEX, ucXN, &ulFlag);
    
    return  (ulFlag);
}
/*********************************************************************************************************
** ��������: armMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  armMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    
    UINT8               ucAP;                                           /*  �洢Ȩ��                    */
    UINT8               ucDomain;                                       /*  ��                          */
    UINT8               ucCB;                                           /*  CACHE �뻺��������          */
    UINT8               ucAP2;                                          /*  �洢Ȩ��                    */
    UINT8               ucTEX;                                          /*  CACHE �뻺��������          */
    UINT8               ucXN;                                           /*  ����ִ��λ                  */
    UINT8               ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }
    
    if (armMmuFlags2Attr(ulFlag,
                         &ucAP, &ucAP2,
                         &ucDomain,
                         &ucCB, &ucTEX,
                         &ucXN) < 0) {                                  /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }
    
    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == SEGMENT_BASE) {                                    /*  ���ڶε�ӳ��                */
        return  (ERROR_NONE);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  ���ڴ����ȶ���ҳ��ӳ��      */
        REGISTER LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                                  ulAddr);
        if (armMmuPteIsOk(*p_pteentry)) {
            addr_t   ulPhysicalAddr = (addr_t)(*p_pteentry & 0xFFFFF000);
            *p_pteentry = armMmuBuildPtentry((UINT32)ulPhysicalAddr, ucAP, ucAP2,
                                             ucDomain, ucCB, ucTEX, ucXN, ucType);
#if LW_CFG_CACHE_EN > 0
            armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);   /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armMmuMakeTrans
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
static VOID  armMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
    UINT8               ucAP;                                           /*  �洢Ȩ��                    */
    UINT8               ucDomain;                                       /*  ��                          */
    UINT8               ucCB;                                           /*  CACHE �뻺��������          */
    UINT8               ucAP2;                                          /*  �洢Ȩ��                    */
    UINT8               ucTEX;                                          /*  CACHE �뻺��������          */
    UINT8               ucXN;                                           /*  ����ִ��λ                  */
    UINT8               ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }
    
    if (armMmuFlags2Attr(ulFlag,
                         &ucAP, &ucAP2,
                         &ucDomain,
                         &ucCB, &ucTEX,
                         &ucXN) < 0) {                                  /*  ��Ч��ӳ���ϵ              */
        return;
    }
    
    *p_pteentry = armMmuBuildPtentry((UINT32)paPhysicalAddr,
                                     ucAP, ucAP2, ucDomain,
                                     ucCB, ucTEX, ucXN, ucType);
                                                        
#if LW_CFG_CACHE_EN > 0
    armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);           /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: armMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������, ����ʹ�� TTBR1 ҳ���ַ�Ĵ���.
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;

    if (LW_NCPUS > 1) {
        /*
         *  Set location of level 1 page table
         * ------------------------------------
         *  31:14 - Base addr
         *  13:7  - 0x0
         *  6     - IRGN[0]     0x1 (Normal memory, Inner Write-Back Write-Allocate Cacheable)
         *  5     - NOS         0x0 (Outer Shareable)
         *  4:3   - RGN         0x1 (Normal memory, Outer Write-Back Write-Allocate Cacheable)
         *  2     - IMP         0x0
         *  1     - S           0x1 (Shareable)
         *  0     - IRGN[1]     0x0 (Normal memory, Inner Write-Back Write-Allocate Cacheable)
         */
        p_pgdentry = (LW_PGD_TRANSENTRY *)((ULONG)pmmuctx->MMUCTX_pgdEntry
                   | (1 << 6)
                   | ((!VMSA_S) << 5)
                   | (1 << 3)
                   | (0 << 2)
                   | (VMSA_S << 1)
                   | (0 << 0));
    } else {
        /*
         *  Set location of level 1 page table
         * ------------------------------------
         *  31:14 - Base addr
         *  13:7  - 0x0
         *  5     - NOS         0x1 (Outer NonShareable)
         *  4:3   - RGN         0x1 (Normal memory, Outer Write-Back Write-Allocate Cacheable)
         *  2     - IMP         0x0
         *  1     - S           0x0 (NonShareable)
         *  0     - C           0x1 (Inner cacheable)
         */
        p_pgdentry = (LW_PGD_TRANSENTRY *)((ULONG)pmmuctx->MMUCTX_pgdEntry
                   | ((!VMSA_S) << 5)
                   | (1 << 3)
                   | (0 << 2)
                   | (VMSA_S << 1)
                   | (1 << 0));
    }
    
    armMmuSetTTBase(p_pgdentry);
    armMmuSetTTBase1(p_pgdentry);
}
/*********************************************************************************************************
** ��������: armMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        armMmuInvalidateTLB();                                          /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            armMmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: armMmuV7Init
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armMmuV7Init (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit    = armMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = armMmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = armMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = armMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = armMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = armMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = armMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = armMmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = armMmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = armMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = armMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = armMmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = armMmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = armMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = armMmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = armMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = armMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = armMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = armMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = armMmuDisable;
    
    VMSA_S = ((LW_NCPUS > 1) || VMSA_FORCE_S) ? 1 : 0;                  /*  ����λ����                  */
}
/*********************************************************************************************************
** ��������: armMmuV7ForceShare
** ��������: MMU ϵͳǿ��Ϊ share ģʽ
** �䡡��  : bEnOrDis      �Ƿ�ʹ��ǿ�� share ģʽ, �˺��������� bspKernelInitHook() �б�����.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armMmuV7ForceShare (BOOL  bEnOrDis)
{
    VMSA_FORCE_S = bEnOrDis;
}
/*********************************************************************************************************
** ��������: armMmuV7ForceNonSecure
** ��������: MMU ϵͳǿ��Ϊ Non-Secure ģʽ
** �䡡��  : bEnOrDis      �Ƿ�ʹ��ǿ�� Non-Secure ģʽ, �˺��������� bspKernelInitHook() �б�����.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armMmuV7ForceNonSecure (BOOL  bEnOrDis)
{
    VMSA_NS = (bEnOrDis) ? 1 : 0;
}
/*********************************************************************************************************
** ��������: armMmuV7ForceDevType
** ��������: MMU ϵͳǿ������ Dev �ڴ�����
** �䡡��  : uiType        �豸�ڴ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armMmuV7ForceDevType (UINT  uiType)
{
    _G_uiVMSADevType = uiType;
}

#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT  */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
