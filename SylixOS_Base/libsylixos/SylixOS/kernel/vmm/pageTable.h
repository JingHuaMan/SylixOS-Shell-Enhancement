/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pageTable.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ҳ�����ز�����.

** BUG:
2009.06.18  ���������ַ�������ַ�Ĳ�ѯ API �� BSP �ӿ�.
2013.12.21  �����Ƿ��ִ��ѡ��.
*********************************************************************************************************/

#ifndef __PAGETABLE_H
#define __PAGETABLE_H

/*********************************************************************************************************
  mmu ҳ���־
*********************************************************************************************************/

#define LW_VMM_FLAG_VALID               0x01                            /*  ӳ����Ч                    */
#define LW_VMM_FLAG_UNVALID             0x00                            /*  ӳ����Ч                    */

#define LW_VMM_FLAG_ACCESS              0x02                            /*  ���Է���                    */
#define LW_VMM_FLAG_UNACCESS            0x00                            /*  ���ܷ���                    */

#define LW_VMM_FLAG_WRITABLE            0x04                            /*  ����д����                  */
#define LW_VMM_FLAG_UNWRITABLE          0x00                            /*  ������д����                */

#define LW_VMM_FLAG_EXECABLE            0x08                            /*  ����ִ�д���                */
#define LW_VMM_FLAG_UNEXECABLE          0x00                            /*  ������ִ�д���              */

#define LW_VMM_FLAG_CACHEABLE           0x10                            /*  ���� CACHE Writeback        */
#define LW_VMM_FLAG_UNCACHEABLE         0x00                            /*  ������ CACHE Writeback      */

#define LW_VMM_FLAG_WRITETHROUGH        0x20                            /*  ���� CACHE Writethrough     */
#define LW_VMM_FLAG_UNWRITETHROUGH      0x00                            /*  ������ CACHE Writethrough   */

#define LW_VMM_FLAG_GUARDED             0x40                            /*  �����ϸ��Ȩ�޼��          */
#define LW_VMM_FLAG_UNGUARDED           0x00                            /*  �������ϸ��Ȩ�޼��        */

#define LW_VMM_FLAG_WRITECOMBINING      0x80                            /*  ����д�ϲ�                  */
#define LW_VMM_FLAG_UNWRITECOMBINING    0x00                            /*  ������д�ϲ�                */

/*********************************************************************************************************
  ע��: LW_VMM_FLAG_PHY_CONTINUOUS ��־, Ϊ CETC 14 Institute ��������,
        ������ API_VmmMallocEx() �� API_VmmMallocAlign().
*********************************************************************************************************/

#define LW_VMM_FLAG_PHY_CONTINUOUS      0x80000000                      /*  �����ַ�ռ�����            */
#define LW_VMM_FLAG_PHY_OPTIMIZE        0x00000000                      /*  ����ʹ�������ַ�ռ�        */

/*********************************************************************************************************
  mmu �����Խӿ� (�൱��д��͸)
*********************************************************************************************************/

#define LW_VMM_FLAG_BUFFERABLE          LW_VMM_FLAG_WRITETHROUGH
#define LW_VMM_FLAG_UNBUFFERABLE        LW_VMM_FLAG_UNWRITETHROUGH

/*********************************************************************************************************
  Ĭ��ҳ���־
*********************************************************************************************************/

#define LW_VMM_FLAG_EXEC                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_EXECABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_GUARDED)           /*  ��ִ������                  */

#define LW_VMM_FLAG_READ                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_GUARDED)           /*  ֻ������                    */
                                         
#define LW_VMM_FLAG_RDWR                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_GUARDED)           /*  ��д����                    */

#define LW_VMM_FLAG_DMA                 (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_GUARDED)           /*  ����Ӳ��ӳ�� (CACHE һ�µ�) */
                                         
#define LW_VMM_FLAG_FAIL                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_UNACCESS |     \
                                         LW_VMM_FLAG_GUARDED)           /*  �������������              */

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  mmu ��Ϣ
*********************************************************************************************************/

typedef struct __lw_mmu_context {
    LW_VMM_AREA              MMUCTX_vmareaVirSpace;                     /*  �����ַ�ռ䷴���          */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    INT                      MMUCTX_iProcId;
#else
    LW_PGD_TRANSENTRY       *MMUCTX_pgdEntry;                           /*  PGD ����ڵ�ַ              */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
} LW_MMU_CONTEXT;
typedef LW_MMU_CONTEXT      *PLW_MMU_CONTEXT;

/*********************************************************************************************************
  mmu ִ�й���
*********************************************************************************************************/

#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

typedef struct {
    FUNCPTR                  MMUOP_pfuncMemInit;                        /*  ��ʼ���ڴ�, (ҳ���Ŀ¼��)  */
    FUNCPTR                  MMUOP_pfuncGlobalInit;                     /*  ��ʼ��ȫ��ӳ���ϵ          */
    
    ULONGFUNCPTR             MMUOP_pfuncFlagGet;                        /*  ���ҳ���־                */
    FUNCPTR                  MMUOP_pfuncFlagSet;                        /*  ����ҳ���־ (��ǰδʹ��)   */
    
    FUNCPTR                  MMUOP_pfuncPageMap;                        /*  ӳ���ڴ�ҳ��                */
    FUNCPTR                  MMUOP_pfuncPageUnmap;                      /*  �ͷ�ӳ���ϵ                */
    FUNCPTR                  MMUOP_pfuncVirToPhy;                       /*  ��������ַ                */
} LW_MMU_OP;

#else                                                                   /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

#ifdef __cplusplus
typedef LW_PGD_TRANSENTRY  *(*PGDFUNCPTR)(...);
typedef LW_PMD_TRANSENTRY  *(*PMDFUNCPTR)(...);
#if LW_CFG_VMM_PAGE_4L_EN > 0
typedef LW_PTS_TRANSENTRY  *(*PTSFUNCPTR)(...);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
typedef LW_PTE_TRANSENTRY  *(*PTEFUNCPTR)(...);

#else
typedef LW_PGD_TRANSENTRY  *(*PGDFUNCPTR)();
typedef LW_PMD_TRANSENTRY  *(*PMDFUNCPTR)();
#if LW_CFG_VMM_PAGE_4L_EN > 0
typedef LW_PTS_TRANSENTRY  *(*PTSFUNCPTR)();
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
typedef LW_PTE_TRANSENTRY  *(*PTEFUNCPTR)();
#endif                                                                  /*  __cplusplus                 */

typedef VOID                (*MAKETRANSFUNCPTR)(PLW_MMU_CONTEXT, 
                                                LW_PTE_TRANSENTRY *, 
                                                addr_t, phys_addr_t, ULONG);

typedef struct {
    ULONG                    MMUOP_ulOption;                            /*  MMU ѡ��                    */
#define LW_VMM_MMU_FLUSH_TLB_MP     0x01                                /*  ÿһ�����Ƿ�Ҫ����      */

    FUNCPTR                  MMUOP_pfuncMemInit;                        /*  ��ʼ���ڴ�, (ҳ���Ŀ¼��)  */
    FUNCPTR                  MMUOP_pfuncGlobalInit;                     /*  ��ʼ��ȫ��ӳ���ϵ          */
    
    PGDFUNCPTR               MMUOP_pfuncPGDAlloc;                       /*  ���� PGD �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPGDFree;                        /*  �ͷ� PGD �ռ�               */
    PMDFUNCPTR               MMUOP_pfuncPMDAlloc;                       /*  ���� PMD �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPMDFree;                        /*  �ͷ� PMD �ռ�               */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    PTSFUNCPTR               MMUOP_pfuncPTSAlloc;                       /*  ���� PTS �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPTSFree;                        /*  �ͷ� PTS �ռ�               */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    PTEFUNCPTR               MMUOP_pfuncPTEAlloc;                       /*  ���� PTE �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPTEFree;                        /*  �ͷ� PTE �ռ�               */

    BOOLFUNCPTR              MMUOP_pfuncPGDIsOk;                        /*  PGD ������Ƿ���ȷ          */
    BOOLFUNCPTR              MMUOP_pfuncPMDIsOk;                        /*  PMD ������Ƿ���ȷ          */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    BOOLFUNCPTR              MMUOP_pfuncPTSIsOk;                        /*  PTS ������Ƿ���ȷ          */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    BOOLFUNCPTR              MMUOP_pfuncPTEIsOk;                        /*  PTE ������Ƿ���ȷ          */

    PGDFUNCPTR               MMUOP_pfuncPGDOffset;                      /*  ͨ����ַ���ָ�� PGD ����   */
    PMDFUNCPTR               MMUOP_pfuncPMDOffset;                      /*  ͨ����ַ���ָ�� PMD ����   */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    PTSFUNCPTR               MMUOP_pfuncPTSOffset;                      /*  ͨ����ַ���ָ�� PTS ����   */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    PTEFUNCPTR               MMUOP_pfuncPTEOffset;                      /*  ͨ����ַ���ָ�� PTE ����   */
    
    FUNCPTR                  MMUOP_pfuncPTEPhysGet;                     /*  ͨ�� PTE ��Ŀ��ȡ�����ַ   */
    
    ULONGFUNCPTR             MMUOP_pfuncFlagGet;                        /*  ���ҳ���־                */
    FUNCPTR                  MMUOP_pfuncFlagSet;                        /*  ����ҳ���־ (��ǰδʹ��)   */
    
    MAKETRANSFUNCPTR         MMUOP_pfuncMakeTrans;                      /*  ����ҳ��ת����ϵ������      */
    VOIDFUNCPTR              MMUOP_pfuncMakeCurCtx;                     /*  ���ǰ��ҳ��ת����ϵ      */
    VOIDFUNCPTR              MMUOP_pfuncInvalidateTLB;                  /*  ��Ч TLB ��                 */
    
    VOIDFUNCPTR              MMUOP_pfuncSetEnable;                      /*  ���� MMU                    */
    VOIDFUNCPTR              MMUOP_pfuncSetDisable;                     /*  �ر� MMU                    */
} LW_MMU_OP;

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

typedef LW_MMU_OP           *PLW_MMU_OP;
extern  LW_MMU_OP            _G_mmuOpLib;                               /*  MMU ����������              */

/*********************************************************************************************************
  MMU ��
*********************************************************************************************************/
#ifndef __VMM_MAIN_FILE
extern LW_OBJECT_HANDLE     _G_ulVmmLock;
#endif

#define __VMM_LOCK()        API_SemaphoreMPend(_G_ulVmmLock, LW_OPTION_WAIT_INFINITE)
#define __VMM_UNLOCK()      API_SemaphoreMPost(_G_ulVmmLock)

/*********************************************************************************************************
  MMU ���ѡ����Ϣ
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

#define __VMM_MMU_OPTION()                      _G_mmuOpLib.MMUOP_ulOption

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
  MMU ������
*********************************************************************************************************/

#define __VMM_MMU_MEM_INIT(pmmuctx)             (_G_mmuOpLib.MMUOP_pfuncMemInit) ?  \
            _G_mmuOpLib.MMUOP_pfuncMemInit(pmmuctx) : (PX_ERROR)
#define __VMM_MMU_GLOBAL_INIT(pcmachine)        (_G_mmuOpLib.MMUOP_pfuncGlobalInit) ?   \
            _G_mmuOpLib.MMUOP_pfuncGlobalInit(pcmachine) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU ҳ�濪�����ͷ�
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

#define __VMM_MMU_PAGE_MAP(pmmuctx, paPhyAddr, ulVirAddr, ulFlag)   (_G_mmuOpLib.MMUOP_pfuncPageMap) ? \
            _G_mmuOpLib.MMUOP_pfuncPageMap(pmmuctx, paPhyAddr, ulVirAddr, ulFlag) : (PX_ERROR)

#define __VMM_MMU_PAGE_UNMAP(pmmuctx, ulVirAddr)    (_G_mmuOpLib.MMUOP_pfuncPageUnmap) ? \
            _G_mmuOpLib.MMUOP_pfuncPageUnmap(pmmuctx, ulVirAddr) : (PX_ERROR)

#define __VMM_MMU_PHYS_GET(ulVirAddr, pulPhysicalAddr)  (_G_mmuOpLib.MMUOP_pfuncVirToPhy) ? \
            _G_mmuOpLib.MMUOP_pfuncVirToPhy(ulVirAddr, pulPhysicalAddr) : (PX_ERROR)

#else                                                                   /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

#define __VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr)                (_G_mmuOpLib.MMUOP_pfuncPGDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPGDAlloc(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PGD_FREE(p_pgdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPGDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPGDFree(p_pgdentry);                         \
        }
#define __VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPMDAlloc(pmmuctx, p_pgdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_FREE(p_pmdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPMDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPMDFree(p_pmdentry); \
        }
        
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_ALLOC(pmmuctx, p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTSAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTSAlloc(pmmuctx, p_pmdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTS_FREE(p_ptsentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPTSFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPTSFree(p_ptsentry); \
        }
#define __VMM_MMU_PTE_ALLOC(pmmuctx, p_ptsentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTEAlloc(pmmuctx, p_ptsentry, ulAddr) : (LW_NULL)
#else                                                                   /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTEAlloc(pmmuctx, p_pmdentry, ulAddr) : (LW_NULL)
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

#define __VMM_MMU_PTE_FREE(p_pteentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPTEFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPTEFree(p_pteentry); \
        }

/*********************************************************************************************************
  MMU ҳ���������ж�
*********************************************************************************************************/

#define __VMM_MMU_PGD_NONE(pgdentry)    (_G_mmuOpLib.MMUOP_pfuncPGDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPGDIsOk(pgdentry)) : (LW_TRUE)
#define __VMM_MMU_PMD_NONE(pmdentry)    (_G_mmuOpLib.MMUOP_pfuncPMDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPMDIsOk(pmdentry)) : (LW_TRUE)
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_NONE(ptsentry)    (_G_mmuOpLib.MMUOP_pfuncPTSIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPTSIsOk(ptsentry)) : (LW_TRUE)
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_NONE(pteentry)    (_G_mmuOpLib.MMUOP_pfuncPTEIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPTEIsOk(pteentry)) : (LW_TRUE)
            
/*********************************************************************************************************
  MMU ҳ����������ȡ
*********************************************************************************************************/

#define __VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr)       (_G_mmuOpLib.MMUOP_pfuncPGDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPGDOffset(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPMDOffset(p_pgdentry, ulAddr) : (LW_NULL)
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_OFFSET(p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTSOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTSOffset(p_pmdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTE_OFFSET(p_ptsentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEOffset(p_ptsentry, ulAddr) : (LW_NULL)
#else                                                                   /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEOffset(p_pmdentry, ulAddr) : (LW_NULL)
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

/*********************************************************************************************************
  MMU ��ȡ�����ַ
*********************************************************************************************************/

#define __VMM_MMU_PHYS_GET(pteentry, paPhysicalAddr)   (_G_mmuOpLib.MMUOP_pfuncPTEPhysGet) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEPhysGet(pteentry, paPhysicalAddr) : (PX_ERROR)

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

/*********************************************************************************************************
  MMU ҳ����������־
*********************************************************************************************************/

#define __VMM_MMU_FLAG_GET(pmmuctx, ulAddr)         (_G_mmuOpLib.MMUOP_pfuncFlagGet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagGet(pmmuctx, ulAddr) : 0ul
#define __VMM_MMU_FLAG_SET(pmmuctx, ulAddr, ulFlag) (_G_mmuOpLib.MMUOP_pfuncFlagSet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagSet(pmmuctx, ulAddr, (ulFlag)) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU �ڲ�����
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

#define __VMM_MMU_MAKE_CURCTX(pmmuctx)      (VOID)pmmuctx               /*  δ����չ                    */

#else

#define __VMM_MMU_MAKE_TRANS(pmmuctx, p_pteentry, ulVirtualAddr, paPhysicalAddr, ulFlag)  \
        if (_G_mmuOpLib.MMUOP_pfuncMakeTrans) { \
            _G_mmuOpLib.MMUOP_pfuncMakeTrans(pmmuctx, p_pteentry,   \
                                             ulVirtualAddr, paPhysicalAddr, (ulFlag));  \
        }
#define __VMM_MMU_MAKE_CURCTX(pmmuctx)  \
        if (_G_mmuOpLib.MMUOP_pfuncMakeCurCtx) {    \
            _G_mmuOpLib.MMUOP_pfuncMakeCurCtx(pmmuctx); \
        }
#define __VMM_MMU_INV_TLB(pmmuctx, ulPageAddr, ulPageNum)  \
        if (_G_mmuOpLib.MMUOP_pfuncInvalidateTLB) { \
            _G_mmuOpLib.MMUOP_pfuncInvalidateTLB(pmmuctx, ulPageAddr, ulPageNum); \
        }
#define __VMM_MMU_ENABLE()  \
        if (_G_mmuOpLib.MMUOP_pfuncSetEnable) { \
            _G_mmuOpLib.MMUOP_pfuncSetEnable(); \
        }
#define __VMM_MMU_DISABLE() \
        if (_G_mmuOpLib.MMUOP_pfuncSetDisable) { \
            _G_mmuOpLib.MMUOP_pfuncSetDisable(); \
        }
        
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

/*********************************************************************************************************
  VMM �ڲ�ƥ��
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

static LW_INLINE  LW_PGD_TRANSENTRY  *__vmm_pgd_alloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    if (pmmuctx->MMUCTX_pgdEntry == LW_NULL) {
        return  (__VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr));
    } else {
        return  (__VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr));
    }
}

static LW_INLINE  VOID  __vmm_pgd_free (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    __VMM_MMU_PGD_FREE(p_pgdentry);
}

static LW_INLINE LW_PMD_TRANSENTRY   *__vmm_pmd_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PGD_TRANSENTRY *p_pgdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PGD_NONE(*p_pgdentry)) {
        return  (__VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pmd_free (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    __VMM_MMU_PMD_FREE(p_pmdentry);
}

#if LW_CFG_VMM_PAGE_4L_EN > 0                                           /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
static LW_INLINE LW_PTS_TRANSENTRY   *__vmm_pts_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PMD_TRANSENTRY *p_pmdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PMD_NONE(*p_pmdentry)) {
        return  (__VMM_MMU_PTS_ALLOC(pmmuctx, p_pmdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTS_OFFSET(p_pmdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pts_free (LW_PTS_TRANSENTRY  *p_ptsentry)
{
    __VMM_MMU_PTS_FREE(p_ptsentry);
}

static LW_INLINE LW_PTE_TRANSENTRY   *__vmm_pte_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PTS_TRANSENTRY *p_ptsentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PTS_NONE(*p_ptsentry)) {
        return  (__VMM_MMU_PTE_ALLOC(pmmuctx, p_ptsentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTE_OFFSET(p_ptsentry, ulAddr));
    }
}

#else                                                                   /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */
static LW_INLINE LW_PTE_TRANSENTRY   *__vmm_pte_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PMD_TRANSENTRY *p_pmdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PMD_NONE(*p_pmdentry)) {
        return  (__VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr));
    }
}
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

static LW_INLINE VOID  __vmm_pte_free (LW_PTE_TRANSENTRY  *p_pteentry)
{
    __VMM_MMU_PTE_FREE(p_pteentry);
}

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

PLW_MMU_CONTEXT __vmmGetCurCtx(VOID);                                   /*  get current mmu context     */
ULONG           __vmmLibPrimaryInit(LW_MMU_PHYSICAL_DESC  pphydesc[],
                                    CPCHAR                pcMachineName);
                                                                        /*  init current mmu context    */
#if LW_CFG_SMP_EN > 0
ULONG           __vmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID            __vmmLibFlushTlb(PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum);

ULONG           __vmmLibPageMap(addr_t ulPhysicalAddr, addr_t ulVirtualAddr, 
                                ULONG  ulPageNum, ULONG  ulFlag);       /*  mmu map                     */
ULONG           __vmmLibPageMap2(phys_addr_t paPhysicalAddr, addr_t ulVirtualAddr, 
                                 ULONG  ulPageNum, ULONG  ulFlag);      /*  mmu map for ioremap2        */
                                
ULONG           __vmmLibGetFlag(addr_t  ulVirtualAddr, ULONG  *pulFlag);
ULONG           __vmmLibSetFlag(addr_t  ulVirtualAddr, ULONG   ulPageNum, ULONG  ulFlag, BOOL  bFlushTlb);

ULONG           __vmmLibVirtualToPhysical(addr_t  ulVirtualAddr, phys_addr_t  *ppaPhysicalAddr);

/*********************************************************************************************************
  bsp api
*********************************************************************************************************/

LW_API LW_MMU_OP       *API_VmmGetLibBlock(VOID);                       /*  BSP get mmu op lib          */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __PAGETABLE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
