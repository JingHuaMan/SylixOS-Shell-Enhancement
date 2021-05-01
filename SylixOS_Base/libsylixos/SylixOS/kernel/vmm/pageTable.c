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
** ��   ��   ��: pageTable.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ҳ�����ز�����.

** BUG:
2009.02.24  �����ڲ�ȫ��ӳ�� __vmmLibGlobalInit() ����.
2009.03.03  �޸� __vmmLibGlobalInit() ע��.
2009.03.04  MMU ȫ��ӳ���ϵ�� __vmmLibGlobalMap() ȫȨ����, 
            __VMM_MMU_GLOBAL_INIT() ��������ʼ����ϵ��عؼ���Ϣ.
2009.03.05  __vmmLibPageMap() ��ҳ�������, ���ҳ���� cache ����, ��д cache �� MMU ����������.
            ���봦�������Ϊ�����ַ���� cache ����Ҫ��д cache , ���� ARM1136.
2009.05.21  ��ʼ��ʱ�� iErrNo ���ж��д���.
2009.11.10  __vmmLibVirtualToPhysical() ����ʱ����ҳ����Ч����.
2009.12.29  __vmmLibGlobalMap() �����ӡ�����ִ�����.
2010.08.13  __vmmLibInit() �е��� __vmm_pgd_alloc() ��ַΪ 0 .
            __vmmLibPageMap() ʹ�� __vmm_pgd_alloc() ��ȡһ��ҳ���Ӧ�����.
2011.03.02  ���� __vmmLibGetFlag() ����.
2011.05.20  �� __vmmLibGetFlag() ��÷���Чӳ��ʱ, ���뷵��ӳ����Ч����.
2013.08.20  ��Ч���ʱ, ��Ҫ֪ͨ������ CPU ��Ч���.
2013.07.20  �������Ӻ˷���� MMU ��ʼ��.
2014.11.09  VMM LIB ��ʼ���������� MMU.
2016.05.01  ���������ַ�ռ��ص��ж�.
2017.07.15  ֧�� L4 ���⻯ MMU �����ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "virPage.h"
/*********************************************************************************************************
  ��ϵ�ṹ��������
*********************************************************************************************************/
extern VOID   __ARCH_MMU_INIT(CPCHAR  pcMachineName);                   /*  BSP MMU ��ʼ������          */
/*********************************************************************************************************
  �ڲ�ȫ��ӳ�亯������
*********************************************************************************************************/
static INT    __vmmLibGlobalMap(PLW_MMU_CONTEXT   pmmuctx, LW_MMU_PHYSICAL_DESC  pphydesc[]);
/*********************************************************************************************************
** ��������: __vmmGetCurCtx
** ��������: ��õ�ǰ MMU ������ (�ڲ�ʹ�û���������ʹ��)
** �䡡��  : NONE
** �䡡��  : ��ǰ����ִ�е� MMU ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_MMU_CONTEXT __vmmGetCurCtx (VOID)
{
    static LW_MMU_CONTEXT   mmuctxGlobal;                               /*  ȫ�� MMU CTX                */

    return  (&mmuctxGlobal);                                            /*  Ŀǰ������֧��, ÿһ������  */
                                                                        /*  ӵ���Լ��������ַ�ռ�      */
}
/*********************************************************************************************************
** ��������: __vmmLibVirtualOverlap
** ��������: ����ռ��ַ��ͻ���
** �䡡��  : ulAddr         ��ַ
**           stSize         ����
** �䡡��  : ��ַ�ռ��Ƿ��ͻ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  __vmmLibVirtualOverlap (addr_t  ulAddr, size_t  stSize)
{
#define __ADDR_OVERLAP(pvirdesc, addr)    \
        if (((addr) >=  pvirdesc->VIRD_ulVirAddr) && \
            ((addr) <= (pvirdesc->VIRD_ulVirAddr +   \
                        pvirdesc->VIRD_stSize - 1))) {   \
            return  (LW_TRUE);  \
        }

    INT                     i;
    PLW_MMU_VIRTUAL_DESC    pvirdescApp;
    PLW_MMU_VIRTUAL_DESC    pvirdescDev;
    addr_t                  ulEnd = ulAddr + stSize - 1;

    if (ulEnd <= ulAddr) {
        return  (LW_TRUE);
    }

    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvirdescApp = __vmmVirtualDesc(LW_VIRTUAL_MEM_APP, i, LW_NULL);
        if (pvirdescApp->VIRD_stSize) {
            __ADDR_OVERLAP(pvirdescApp, ulAddr);
            __ADDR_OVERLAP(pvirdescApp, ulEnd);
        }
    }

    pvirdescDev = __vmmVirtualDesc(LW_VIRTUAL_MEM_DEV, 0, LW_NULL);
    if (pvirdescDev->VIRD_stSize) {
        __ADDR_OVERLAP(pvirdescDev, ulAddr);
        __ADDR_OVERLAP(pvirdescDev, ulEnd);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __vmmLibPrimaryInit
** ��������: ��ʼ�� MMU ����, CPU ������ء�(���ģʽ��, Ϊ���� MMU ��ʼ��)
** �䡡��  : pphydesc          �����ڴ���������
**           pcMachineName     �������еĻ�������
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibPrimaryInit (LW_MMU_PHYSICAL_DESC  pphydesc[], CPCHAR  pcMachineName)
{
    static   BOOL             bIsInit          = LW_FALSE;
             BOOL             bIsNeedGlobalMap = LW_FALSE;

    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    REGISTER INT              iError;
    REGISTER ULONG            ulError;
    
             INT              iErrLevel = 0;

    if (bIsInit == LW_FALSE) {
        __ARCH_MMU_INIT(pcMachineName);                                 /*  ��ʼ�� MMU ����             */
        bIsInit          = LW_TRUE;
        bIsNeedGlobalMap = LW_TRUE;
    }
    
    iError = __VMM_MMU_MEM_INIT(pmmuctx);                               /*  ����ҳ���ڴ滺����          */
    if (iError < ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mmu memory init error.\r\n");
        return  (ERROR_KERNEL_MEMORY);
    }
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "mmu initialize. start memory pagination...\r\n");
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    pmmuctx->MMUCTX_iProcId = 0;                                        /*  δ����չ�ӿ�                */
    
#else
    /*
     *  __vmm_pgd_alloc() ��ַΪ 0 , ��ʾҳ���ַ + 0 ƫ����, ���Է��ص�ҳ�������ҳ���ַ.
     */
    pmmuctx->MMUCTX_pgdEntry = __vmm_pgd_alloc(pmmuctx, 0ul);           /*  ���� PGD ����               */
    if (pmmuctx->MMUCTX_pgdEntry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mmu can not allocate pgd entry.\r\n");
        iErrLevel = 1;
        ulError   = ERROR_KERNEL_MEMORY;
        goto    __error_handle;
    }
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    if (bIsNeedGlobalMap) {
        iError = __VMM_MMU_GLOBAL_INIT(pcMachineName);                  /*  ��ʼ����ϵ��عؼ�����      */
        if (iError < ERROR_NONE) {
            iErrLevel = 2;
            ulError   = errno;
            goto    __error_handle;
        }
        
        iError = __vmmLibGlobalMap(pmmuctx, pphydesc);                  /*  ȫ���ڴ��ϵӳ��            */
        if (iError < ERROR_NONE) {
            iErrLevel = 2;
            ulError   = errno;
            goto    __error_handle;
        }
    }
    
    __VMM_MMU_MAKE_CURCTX(pmmuctx);                                     /*  ����ҳ�����ַ              */
    KN_SMP_MB();
    
    return  (ERROR_NONE);
    
__error_handle:
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    if (iErrLevel > 1) {
        __vmm_pgd_free(pmmuctx->MMUCTX_pgdEntry);
    }
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: __vmmLibSecondaryInit
** ��������: ��ʼ�� MMU ����, CPU ������ء�(���ģʽ��, Ϊ���� MMU ��ʼ��)
** �䡡��  : pcMachineName     �������еĻ�������
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

ULONG  __vmmLibSecondaryInit (CPCHAR  pcMachineName)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    REGISTER INT              iError;
    REGISTER ULONG            ulError;
    
    iError = __VMM_MMU_GLOBAL_INIT(pcMachineName);                      /*  ��ʼ����ϵ��عؼ�����      */
    if (iError < ERROR_NONE) {
        ulError = errno;
        return  (ulError);
    }
    
    __VMM_MMU_MAKE_CURCTX(pmmuctx);                                     /*  ����ҳ��λ��                */
    KN_SMP_MB();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: __vmmLibGlobalMap
** ��������: ȫ��ҳ��ӳ���ϵ����
** �䡡��  : pmmuctx        MMU ������
**           pphydesc       �����ڴ���������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __vmmLibGlobalMap (PLW_MMU_CONTEXT   pmmuctx, LW_MMU_PHYSICAL_DESC  pphydesc[]) 
{
    INT     i;
    ULONG   ulError;
    ULONG   ulPageNum;
    ULONG   ulTextFlags;

    if (LW_KERN_TEXT_RO_GET()) {
        ulTextFlags = LW_VMM_FLAG_EXEC | LW_VMM_FLAG_READ;
    } else {
        ulTextFlags = LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR;
    }

    for (i = 0; pphydesc[i].PHYD_stSize; i++) {
        if ((pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_BUSPOOL) ||
            (pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_APP)     ||
            (pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_DMA)) {
            continue;
        }
        
        ulPageNum = (ULONG)(pphydesc[i].PHYD_stSize >> LW_CFG_VMM_PAGE_SHIFT);
        if (pphydesc[i].PHYD_stSize & ~LW_CFG_VMM_PAGE_MASK) {
            ulPageNum++;
        }
        
        _BugFormat(__vmmLibVirtualOverlap(pphydesc[i].PHYD_ulVirMap, 
                                          pphydesc[i].PHYD_stSize), LW_TRUE,
                   "global map vaddr 0x%08lx size: 0x%08zx overlap with virtual space.\r\n",
                   pphydesc[i].PHYD_ulVirMap, pphydesc[i].PHYD_stSize);
    
        switch (pphydesc[i].PHYD_uiType) {
        
        case LW_PHYSICAL_MEM_TEXT:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, ulTextFlags);
            break;
            
        case LW_PHYSICAL_MEM_DATA:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_RDWR);
            break;
            
        case LW_PHYSICAL_MEM_VECTOR:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_EXEC);
            break;
            
        case LW_PHYSICAL_MEM_BOOTSFR:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_DMA);
            break;
            
        default:
            ulError = ERROR_NONE;
            break;
        }
        
        if (ulError) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm global map fail.\r\n");
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  TextUpdate Parameter
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

#if LW_CFG_SMP_EN > 0

typedef struct {
    PLW_MMU_CONTEXT     FTLB_pmmuctx;
    addr_t              FTLB_ulPageAddr;
    ULONG               FTLB_ulPageNum;
} LW_VMM_FTLB_ARG;
/*********************************************************************************************************
** ��������: __vmmLibFlushTblTlb
** ��������: ˢ�µ�ǰ CPU TLB (IPI ִ��)
** �䡡��  : pftlb     ˢ�²���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmLibFlushTlbIpi (LW_VMM_FTLB_ARG  *pftlb)
{
    INTREG      iregInterLevel;

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    __VMM_MMU_INV_TLB(pftlb->FTLB_pmmuctx, 
                      pftlb->FTLB_ulPageAddr, 
                      pftlb->FTLB_ulPageNum);                           /*  ��Ч���                    */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __vmmLibFlushTlb
** ��������: ˢ�� TLB
** �䡡��  : pmmuctx       MMU ������
**           ulPageAddr    ҳ�������ַ
**           ulPageNum     ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmLibFlushTlb (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    INTREG  iregInterLevel;
    
    ulPageAddr &= LW_CFG_VMM_PAGE_MASK;                                 /*  ��ַ����                    */
    
#if LW_CFG_SMP_EN > 0
    if (LW_SYS_STATUS_IS_RUNNING() && 
        (__VMM_MMU_OPTION() & LW_VMM_MMU_FLUSH_TLB_MP) &&
        (LW_NCPUS > 1)) {                                               /*  ��Ҫ֪ͨ���� CPU            */
        LW_VMM_FTLB_ARG  ftlb;
        BOOL             bLock;

        ftlb.FTLB_pmmuctx    = pmmuctx;
        ftlb.FTLB_ulPageAddr = ulPageAddr;
        ftlb.FTLB_ulPageNum  = ulPageNum;
        
        bLock = __SMP_CPU_LOCK();                                       /*  ������ǰ CPU ִ��           */

        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        __VMM_MMU_INV_TLB(pmmuctx, ulPageAddr, ulPageNum);              /*  ��Ч���                    */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */

        _SmpCallFuncAllOther((FUNCPTR)__vmmLibFlushTlbIpi, &ftlb,
                             LW_NULL, LW_NULL, IPIM_OPT_NORMAL);        /*  ֪ͨ������ CPU              */

        __SMP_CPU_UNLOCK(bLock);                                        /*  ������ǰ CPU ִ��           */

    } else
#endif                                                                  /*  LW_CFG_SMP_EN               */
    {
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        __VMM_MMU_INV_TLB(pmmuctx, ulPageAddr, ulPageNum);              /*  ��Ч���                    */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
}

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
** ��������: __vmmLibPageMap2
** ��������: ������ҳ������ӳ�� (for ioremap2 )
** �䡡��  : paPhysicalAddr        ����ҳ���ַ
**           ulVirtualAddr         ��Ҫӳ��������ַ
**           ulPageNum             ��Ҫӳ���ҳ�����
**           ulFlag                ҳ���־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibPageMap2 (phys_addr_t paPhysicalAddr, addr_t ulVirtualAddr, ULONG ulPageNum, ULONG ulFlag)
{
    ULONG                    i;
    PLW_MMU_CONTEXT          pmmuctx = __vmmGetCurCtx();
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    INTREG                   iregInterLevel;
    addr_t                   ulVirtualTlb = ulVirtualAddr;
    
    LW_PGD_TRANSENTRY       *p_pgdentry;
    LW_PMD_TRANSENTRY       *p_pmdentry;
#if LW_CFG_VMM_PAGE_4L_EN > 0
    LW_PTS_TRANSENTRY       *p_ptsentry;
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    LW_PTE_TRANSENTRY       *p_pteentry;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        for (i = 0; i < ulPageNum; i++) {
            if (__VMM_MMU_PAGE_MAP(pmmuctx, paPhysicalAddr, 
                                   ulVirtualAddr, ulFlag) < ERROR_NONE) {
                return  (ERROR_VMM_LOW_LEVEL);
            }
            paPhysicalAddr += LW_CFG_VMM_PAGE_SIZE;
            ulVirtualAddr  += LW_CFG_VMM_PAGE_SIZE;
        }
    
    } else {
        for (i = 0; i < ulPageNum; i++) {
            if (__VMM_MMU_PAGE_UNMAP(pmmuctx, ulVirtualAddr) < ERROR_NONE) {
                return  (ERROR_VMM_LOW_LEVEL);
            }
            ulVirtualAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
    
#else
    for (i = 0; i < ulPageNum; i++) {
        p_pgdentry = __vmm_pgd_alloc(pmmuctx, ulVirtualAddr);
        if (p_pgdentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        p_pmdentry = __vmm_pmd_alloc(pmmuctx, p_pgdentry, ulVirtualAddr);
        if (p_pmdentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
#if LW_CFG_VMM_PAGE_4L_EN > 0
        p_ptsentry = __vmm_pts_alloc(pmmuctx, p_pmdentry, ulVirtualAddr);
        if (p_ptsentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        p_pteentry = __vmm_pte_alloc(pmmuctx, p_ptsentry, ulVirtualAddr);
#else
        p_pteentry = __vmm_pte_alloc(pmmuctx, p_pmdentry, ulVirtualAddr);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */

        if (p_pteentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        __VMM_MMU_MAKE_TRANS(pmmuctx, p_pteentry,
                             ulVirtualAddr,
                             paPhysicalAddr, ulFlag);                   /*  ����ӳ���ϵ                */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        
        paPhysicalAddr += LW_CFG_VMM_PAGE_SIZE;
        ulVirtualAddr  += LW_CFG_VMM_PAGE_SIZE;
    }
    
    __vmmLibFlushTlb(pmmuctx, ulVirtualTlb, ulPageNum);                 /*  ͬ��ˢ������ CPU TLB        */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmLibPageMap
** ��������: ������ҳ������ӳ��
** �䡡��  : ulPhysicalAddr        ����ҳ���ַ
**           ulVirtualAddr         ��Ҫӳ��������ַ
**           ulPageNum             ��Ҫӳ���ҳ�����
**           ulFlag                ҳ���־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibPageMap (addr_t  ulPhysicalAddr, addr_t  ulVirtualAddr, ULONG   ulPageNum, ULONG   ulFlag)
{
    return  (__vmmLibPageMap2((phys_addr_t)ulPhysicalAddr, ulVirtualAddr, ulPageNum, ulFlag));
}
/*********************************************************************************************************
** ��������: __vmmLibGetFlag
** ��������: ��ȡָ���߼���ַ�ķ���Ȩ��
** �䡡��  : ulVirtualAddr         ��Ҫӳ��������ַ
**           pulFlag               ҳ���־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibGetFlag (addr_t  ulVirtualAddr, ULONG  *pulFlag)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    ULONG               ulFlag;

    ulFlag = __VMM_MMU_FLAG_GET(pmmuctx, ulVirtualAddr);
    if (pulFlag) {
        *pulFlag = ulFlag;
    }
    
    if (ulFlag & LW_VMM_FLAG_VALID) {
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
}
/*********************************************************************************************************
** ��������: __vmmLibSetFlag
** ��������: ����ָ���߼���ַ�ķ���Ȩ��
** �䡡��  : ulVirtualAddr         ��Ҫӳ��������ַ
**           ulPageNum             ҳ������
**           ulFlag                ҳ���־
**           bFlushTlb             �Ƿ���� TLB.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibSetFlag (addr_t  ulVirtualAddr, ULONG   ulPageNum, ULONG  ulFlag, BOOL  bFlushTlb)
{
    INTREG              iregInterLevel;
    INT                 i;
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    INT                 iError;
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    addr_t              ulVirtualTlb = ulVirtualAddr;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    for (i = 0; i < ulPageNum; i++) {                                   /*  ����ӳ����Щҳ��            */
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        iError = __VMM_MMU_FLAG_SET(pmmuctx, ulVirtualAddr, ulFlag);
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        
        _BugHandle((iError < 0), LW_FALSE, "set page flag error,\r\n");
        ulVirtualAddr += LW_CFG_VMM_PAGE_SIZE;
    }
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    if (bFlushTlb) {
        __vmmLibFlushTlb(pmmuctx, ulVirtualTlb, ulPageNum);             /*  ͬ��ˢ������ CPU TLB        */
    }
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmLibVirtualToPhysical
** ��������: ͨ�������ַ��ѯ��Ӧ�������ַ
** �䡡��  : ulVirtualAddr      �����ַ
**           pulPhysicalAddr    ���ص������ַ
** �䡡��  : �����ַ, ���󷵻� (void *)-1 ��ַ.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmLibVirtualToPhysical (addr_t  ulVirtualAddr, phys_addr_t  *ppaPhysicalAddr)
{
    INT                      iError;
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    PLW_MMU_CONTEXT          pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY       *p_pgdentry;
    LW_PMD_TRANSENTRY       *p_pmdentry;
#if LW_CFG_VMM_PAGE_4L_EN > 0
    LW_PTS_TRANSENTRY       *p_ptsentry;
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    LW_PTE_TRANSENTRY       *p_pteentry;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    iError = __VMM_MMU_PHYS_GET(ulVirtualAddr, ppaPhysicalAddr);
    if (iError < 0) {
        return  (ERROR_VMM_LOW_LEVEL);
    }
    
    return  (ERROR_NONE);

#else
    p_pgdentry = __VMM_MMU_PGD_OFFSET(pmmuctx, ulVirtualAddr);
    if (__VMM_MMU_PGD_NONE((*p_pgdentry))) {                            /*  �ж� PGD ��Ŀ��ȷ��         */
        goto    __error_handle;
    }
    
    p_pmdentry = __VMM_MMU_PMD_OFFSET(p_pgdentry, ulVirtualAddr);
    if (__VMM_MMU_PMD_NONE((*p_pmdentry))) {                            /*  �ж� PMD ��Ŀ��ȷ��         */
        goto    __error_handle;
    }
    
#if LW_CFG_VMM_PAGE_4L_EN > 0
    p_ptsentry = __VMM_MMU_PTS_OFFSET(p_pmdentry, ulVirtualAddr);
    if (__VMM_MMU_PTS_NONE((*p_ptsentry))) {                            /*  �ж� PTS ��Ŀ��ȷ��         */
        goto    __error_handle;
    }
    p_pteentry = __VMM_MMU_PTE_OFFSET(p_ptsentry, ulVirtualAddr);
#else
    p_pteentry = __VMM_MMU_PTE_OFFSET(p_pmdentry, ulVirtualAddr);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    
    if (__VMM_MMU_PTE_NONE((*p_pteentry))) {                            /*  �ж� PTE ��Ŀ��ȷ��         */
        goto    __error_handle;
    }
    
    iError = __VMM_MMU_PHYS_GET((*p_pteentry), ppaPhysicalAddr);        /*  ��ѯҳ�����ַ              */
    if (iError < 0) {
        return  (ERROR_VMM_LOW_LEVEL);
    
    } else {
        *ppaPhysicalAddr = (ulVirtualAddr & (LW_CFG_VMM_PAGE_SIZE - 1))
                         + (*ppaPhysicalAddr);                          /*  ����ҳ��ƫ����              */
        return  (ERROR_NONE);
    }
    
__error_handle:
    _ErrorHandle(ERROR_VMM_PAGE_INVAL);
    return  (ERROR_VMM_PAGE_INVAL);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
