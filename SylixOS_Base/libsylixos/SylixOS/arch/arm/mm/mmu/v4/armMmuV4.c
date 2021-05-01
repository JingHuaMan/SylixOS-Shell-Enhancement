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
** ��   ��   ��: armMmuV4.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv4/5/6 ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "../armMmuCommon.h"
#include "../../cache/armCacheCommon.h"
#include "../../../param/armParam.h"
/*********************************************************************************************************
  һ�����������Ͷ���
*********************************************************************************************************/
#define COARSE_TBASE        (1)                                         /*  ����������ҳ�����ַ        */
#define TINY_TBASE          (3)                                         /*  ����ϸ����ҳ�����ַ        */
#define SEGMENT_BASE        (2)                                         /*  ��ӳ�����ַ                */
/*********************************************************************************************************
  �������������Ͷ���
*********************************************************************************************************/
#define FAIL_DESC           (0)                                         /*  �任ʧЧ                    */
#define BIGPAGE_DESC        (1)                                         /*  ��ҳ����ַ                  */
#define SMALLPAGE_DESC      (2)                                         /*  Сҳ����ַ                  */
#define TINYPAGE_DESC       (3)                                         /*  ��Сҳ����ַ                */
/*********************************************************************************************************
  �������е� C B λ (C: Cache B: WriteBuffer)
*********************************************************************************************************/
#define NC_NB               (0)                                         /*  UNCACHE UNBUFFER            */
#define NC_B                (1)                                         /*  UNCACHE BUFFER              */
#define C_NB                (2)                                         /*  CACHE   BUFFER WRITE THROUGH*/
#define C_B                 (3)                                         /*  CACHE   BUFFER WRITE BACK   */
/*********************************************************************************************************
  �������е� A P λ (����Ȩ��λ, C1 sys=0 rom=1)
*********************************************************************************************************/
#define AP_RW               (3)                                         /*  ��Ȩģʽ��READ / WRITE      */
                                                                        /*  �û�ģʽ��READ / WRITE      */
#define AP_RO               (0)                                         /*  ��Ȩģʽ��READ              */
                                                                        /*  �û�ģʽ��READ              */
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
static BOOL                 _G_bInitWriteBuffer = LW_FALSE;             /*  ��ʼ�� writebuffer          */
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
static INT  armMmuFlags2Attr (ULONG ulFlag, UINT8  *pucAP, UINT8  *pucDomain, UINT8  *pucCB)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        *pucDomain = ACCESS_FAIL;
        return  (PX_ERROR);
    }
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ӵ�з���Ȩ��            */
        if (ulFlag & LW_VMM_FLAG_GUARDED) {
            *pucDomain = ACCESS_AND_CHK;                                /*  Ȩ�޼��                    */
        
        } else {
            *pucDomain = ACCESS_NOT_CHK;                                /*  ��Ȩ�޼��                  */
        }
    } else {
        *pucDomain = ACCESS_FAIL;                                       /*  ����ʧЧ                    */
    }
    
    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  �Ƿ��д                    */
        *pucAP = AP_RW;
    
    } else {
        *pucAP = AP_RO;
    }
    
    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  WRITE BACK                  */
        *pucCB = C_B;

    } else if (ulFlag & LW_VMM_FLAG_WRITETHROUGH) {                     /*  WRITE THROUGH               */
        *pucCB = C_NB;

    } else if (ulFlag & LW_VMM_FLAG_WRITECOMBINING) {                   /*  UNCACHE BUFFERED            */
        *pucCB = NC_B;

    } else {
        *pucCB = NC_NB;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armMmuAttr2Flags
** ��������: ���� ARM MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucAP                    ����Ȩ��
**           ucDomain                ����������
**           ucCB                    CACHE ���Ʋ���
**           pulFlag                 �ڴ����Ȩ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armMmuAttr2Flags (UINT8  ucAP, UINT8  ucDomain, UINT8  ucCB, ULONG *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;
    
    if (ucDomain == ACCESS_AND_CHK) {
        *pulFlag |= LW_VMM_FLAG_GUARDED;
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    
    } else if (ucDomain == ACCESS_NOT_CHK) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }
    
    if (ucAP == AP_RW) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }
    
    switch (ucCB) {
    
    case C_B:
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
        break;
        
    case C_NB:
        *pulFlag |= LW_VMM_FLAG_WRITETHROUGH;
        break;

    case NC_B:
        *pulFlag |= LW_VMM_FLAG_WRITECOMBINING;
        break;
    }
    
    if (*pulFlag & LW_VMM_FLAG_ACCESS) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;                               /*  ARMv4/v5/v6 MMU ��֧�ִ�λ  */
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
                                             UINT8   ucDomain,
                                             UINT8   ucCB,
                                             UINT8   ucType)
{
    LW_PGD_TRANSENTRY   uiDescriptor;
    
    switch (ucType) {
    
    case COARSE_TBASE:                                                  /*  �����ȶ���ҳ��������        */
        uiDescriptor = (uiBaseAddr & 0xFFFFFC00)
                     | (ucDomain << 5)
                     | ucType;
        break;
        
    case TINY_TBASE:                                                    /*  ϸ���ȶ���ҳ��������        */
        uiDescriptor = (uiBaseAddr & 0xFFFFF000)
                     | (ucDomain << 5)
                     | ucType;
        break;
        
    case SEGMENT_BASE:                                                  /*  ��������                    */
        uiDescriptor = (uiBaseAddr & 0xFFF00000)
                     | (ucAP << 10)
                     | (ucDomain << 5)
                     | (ucCB << 2)
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
                                              UINT8   ucDomain,
                                              UINT8   ucCB,
                                              UINT8   ucType)
{
    LW_PTE_TRANSENTRY   uiDescriptor;
    
    switch (ucType) {
    
    case BIGPAGE_DESC:                                                  /*  ��ҳ������                  */
        uiDescriptor = (uiBaseAddr & 0xFFFF0000)
                     | (ucAP << 10)
                     | (ucAP <<  8)                                     /*  4 ����ҳ����Ȩ��            */
                     | (ucAP <<  6)
                     | (ucAP <<  4)
                     | (ucCB <<  2)
                     | ucType;
        break;
        
    case SMALLPAGE_DESC:                                                /*  Сҳ������                  */
        uiDescriptor = (uiBaseAddr & 0xFFFFF000)
                     | (ucAP << 10)
                     | (ucAP <<  8)                                     /*  4 ����ҳ����Ȩ��            */
                     | (ucAP <<  6)
                     | (ucAP <<  4)
                     | (ucCB <<  2)
                     | ucType;
        break;
        
    case TINYPAGE_DESC:                                                 /*  ��Сҳ������                */
        uiDescriptor = (uiBaseAddr & 0xFFFFFC00)
                     | (ucAP <<  4)
                     | (ucCB <<  2)
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
    
    armMmuInitSysRom();
    
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
    
    if (_G_bInitWriteBuffer) {
        armMmuEnableWriteBuffer();
    }

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
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
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
    return  (((pteentry & 0x03) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
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
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    *p_pmdentry = (LW_PMD_TRANSENTRY)armMmuBuildPgdesc((UINT32)p_pteentry,
                                                       AP_RW, 
                                                       ACCESS_AND_CHK, 
                                                       NC_NB, 
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
    ULONG               ulFlag = 0;
    
    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == SEGMENT_BASE) {                                    /*  ���ڶε�ӳ��                */
        UINT32  uiDescriptor = (UINT32)(*p_pgdentry);
        
        ucCB     = (UINT8)((uiDescriptor >>  2) & 0x03);
        ucDomain = (UINT8)((uiDescriptor >>  5) & 0x0F);
        ucAP     = (UINT8)((uiDescriptor >> 10) & 0x02);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  ���ڴ����ȶ���ҳ��ӳ��      */
        UINT32              uiDescriptor = (UINT32)(*p_pgdentry);
        LW_PTE_TRANSENTRY  *p_pteentry   = armMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);
        
        if (armMmuPteIsOk(*p_pteentry)) {
            ucDomain     = (UINT8)((uiDescriptor >>  5) & 0x0F);
            
            uiDescriptor = (UINT32)(*p_pteentry);
            ucCB         = (UINT8)((uiDescriptor >>  2) & 0x03);
            ucAP         = (UINT8)((uiDescriptor >>  4) & 0x03);
        
        } else {
            return  (LW_VMM_FLAG_UNVALID);
        }
    } else {
        return  (LW_VMM_FLAG_UNVALID);
    }
    
    armMmuAttr2Flags(ucAP, ucDomain, ucCB, &ulFlag);
    
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
    UINT8               ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    
    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }
    
    if (armMmuFlags2Attr(ulFlag, &ucAP, &ucDomain, &ucCB) < 0) {        /*  ��Ч��ӳ���ϵ              */
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
            *p_pteentry = armMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                             ucAP, ucDomain, ucCB, ucType);
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
    UINT8   ucAP;                                                       /*  �洢Ȩ��                    */
    UINT8   ucDomain;                                                   /*  ��                          */
    UINT8   ucCB;                                                       /*  CACHE �뻺��������          */
    UINT8   ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    
    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }
    
    if (armMmuFlags2Attr(ulFlag, &ucAP, &ucDomain, &ucCB) < 0) {        /*  ��Ч��ӳ���ϵ              */
        return;
    }
    
    *p_pteentry = armMmuBuildPtentry((UINT32)paPhysicalAddr, ucAP,
                                     ucDomain, ucCB, ucType);
                                                        
#if LW_CFG_CACHE_EN > 0
    armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);           /*  ������������Ӱ��            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: armMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    armMmuSetTTBase(pmmuctx->MMUCTX_pgdEntry);
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
** ��������: armMmuV4Init
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armMmuV4Init (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, ARM_MACHINE_920) == 0) || 
        (lib_strcmp(pcMachineName, ARM_MACHINE_926) == 0)) {
        _G_bInitWriteBuffer = LW_TRUE;
    }

    pmmuop->MMUOP_ulOption = 0lu;
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
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
