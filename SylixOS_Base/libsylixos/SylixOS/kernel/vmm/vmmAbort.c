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
** ��   ��   ��: vmmAbort.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 05 ��
**
** ��        ��: ƽ̨�޹��쳣����.
**
** ע        ��: ������ȱҳ�ж�ʱ, ���е����I/O����(��������mmap���)������ vmm lock �½��е�!
                 SylixOS ����ȱҳ�쳣����������������, ͨ������������, ����, ��Ӱ���޹�����ĵ���������.
                 ����Ҳ�������������. ������������ͬһ��ַ�����쳣�ȵ�, ��������õ�һЩ�������������
                 �������.
                 
** BUG:
2012.09.15  δ����ָ����� SIGILL �ź�.
2012.12.22  �������͵Ĵ�ӡʹ���ַ���.
2012.12.22  __vmmAbortKillSignal() ��Ӧ�÷���, ���ǵȴ���ɾ��.
2013.03.29  �����쳣��, Ӧ�ûָ�֮ǰ�� errno.
2013.05.09  API_VmmAbortIsr() ��������ж�������, ֱ�Ӵ�����, ����Ҫ�������������Ĺ���.
2013.05.10  ȱ������ҳ��ʱ, Ӧ��ʹ�� SIGKILL �ź�ɱ������.
2013.09.13  �쳣�˳���, �ָ� kernel space ״̬.
2013.09.14  ֧���ļ�֮��Ĺ���ӳ��.
2013.11.21  ���� _doSigEvent() ���ò���.
2013.12.23  ֧�ֶ� LW_VMM_ABORT_TYPE_EXEC �Ĵ�����.
2014.05.17  �ϵ�̽�ⲻ�����������.
2014.05.21  ϵͳ�����δ���ʱ, ��Ҫ֪ͨ dtrace.
2016.11.24  FPU �쳣��Ҫ���쳣�����ı��� FPU ������.
2017.08.11  ������̶߳�ջ����ļ��.
2018.08.13  �ں��̳߳������ش���, �Զ���������ϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  debug support
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  loader support
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern pid_t  vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
extern pid_t  API_ModulePid(PVOID pvVProc);
#else
#define vprocGetPidByTcbNoLock(x)   0
#define API_ModulePid(x)            0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "vmmSwap.h"
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
  �ں���Ϣ��ӡ�������ں˺���
*********************************************************************************************************/
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
/*********************************************************************************************************
  �����ҳ�ռ����� (ȱҳ�жϲ��ұ�)
*********************************************************************************************************/
LW_LIST_LINE_HEADER       _K_plineVmmVAddrSpaceHeader = LW_NULL;
/*********************************************************************************************************
  ͳ�Ʊ���
*********************************************************************************************************/
static LW_VMM_STATUS      _K_vmmStatus;
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
static VOID   __vmmWarnGuarder(LW_OBJECT_HANDLE  ulGuarder);
static VOID   __vmmAbortKill(PLW_VMM_ABORT_CTX  pabtctx);
static VOID   __vmmAbortAccess(PLW_VMM_ABORT_CTX  pabtctx);
static PCHAR  __vmmAbortTypeStr(PLW_VMM_ABORT  pabtInfo);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define __PAGEFAIL_ALLOC_PAGE_NUM   1                                   /*  ȱҳ�жϷ���ҳ����, ����Ϊ1 */
#define __PAGEFAIL_CUR_PID          API_ModulePid(ptcbCur->TCB_pvVProcessContext)
/*********************************************************************************************************
** ��������: __vmmAbortFakeSymbol
** ��������: �쳣ģʽ��ʹ��ջ���ݲ���ʱ, ��ʾ�˷���.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmAbortFakeSymbol (VOID)
{
}
/*********************************************************************************************************
** ��������: __vmmAbortCacheRefresh
** ��������: �� MMU ȱҳ������ɺ�, ���ﴦ�� cache ��ˢ�²���.
** �䡡��  : bSwapNeedLoad         �Ƿ��Ǵ� swap �ж�������
**           ulVirtualPageAlign    ��Ӧ������ҳ���ڴ��ַ
**           ulSwitchPhysicalPage  ������������ҳ���ַ
**           ulAllocPageNum        ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __vmmAbortCacheRefresh (BOOL    bSwapNeedLoad,
                                    addr_t  ulVirtualPageAlign,
                                    addr_t  ulSwitchPhysicalPage,
                                    ULONG   ulAllocPageNum)
{
#if LW_CFG_CACHE_EN > 0
    addr_t      ulSwitchAddr = __vmmVirtualSwitch();
    BOOL        bFlush       = LW_FALSE;
    size_t      stSize       = (size_t)(ulAllocPageNum << LW_CFG_VMM_PAGE_SHIFT);
    
    if (API_CacheAliasProb()) {                                         /*  ����п��ܲ��� cache ����   */
        API_CacheClearPage(DATA_CACHE, 
                           (PVOID)ulSwitchAddr,
                           (PVOID)ulSwitchPhysicalPage,
                           stSize);                                     /*  ������д���ڴ沢��������    */
        API_CacheInvalidatePage(DATA_CACHE, 
                                (PVOID)ulVirtualPageAlign, 
                                (PVOID)ulSwitchPhysicalPage,
                                stSize);                                /*  ��Ч�������ڴ�ռ�          */
        bFlush = LW_TRUE;
    }
    
    if (bSwapNeedLoad) {                                                /*  swap load �п����Ǵ���      */
        if (bFlush && (LW_NCPUS == 1)) {
            API_CacheInvalidate(INSTRUCTION_CACHE, (PVOID)ulVirtualPageAlign, stSize);
                                                                        /*  �Ѿ���д, ֻ��Ҫ��Ч I-CACHE*/
        } else {
            API_CacheTextUpdate((PVOID)ulVirtualPageAlign, stSize);     /*  ��Ч I-CACHE ��д D-CACHE   */
        }
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: __vmmAbortPageGet
** ��������: �����쳣��ַ, ��ȡ����ҳ����ƿ�
** �䡡��  : ulAbortAddr       �쳣��ַ
** �䡡��  : ����ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VMM_PAGE  __vmmAbortPageGet (addr_t  ulAbortAddr)
{
             PLW_LIST_LINE          plineTemp;
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    for (plineTemp  = _K_plineVmmVAddrSpaceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ��ѯ���ʵĵ�ַ�Ƿ�Ϊ�Ϸ���ַ*/
         
        pvmpagep       = _LIST_ENTRY(plineTemp, LW_VMM_PAGE_PRIVATE, PAGEP_lineManage);
        pvmpageVirtual = pvmpagep->PAGEP_pvmpageVirtual;
        
        if ((ulAbortAddr >= pvmpageVirtual->PAGE_ulPageAddr) &&
            (ulAbortAddr <  (pvmpageVirtual->PAGE_ulPageAddr + 
                             (pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT)))) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pvmpageVirtual);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __vmmAbortCopyOnWrite
** ��������: ���������ϸı�ʱ, ��Ҫ���ı��ҳ���Ϊ private, ���ٱ�����.
** �䡡��  : pabtctx               page fail ������
**           pvmpageVirtual        ����ռ�
**           ulAbortAddrAlign      �쳣�ĵ�ַ
**           ptcbCur               ��ǰ������ƿ�
**           pulGuarder            �Ƿ���Ҫ����ػ��߲��� SIGLOWMEM WARNING ��Ϣ
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 1: ����ǹ���仯�Ķ�, ������Ϊ�����ַ cache ����, �� mmap �϶��Ѿ�����Ӧ����� cache ����
                ����, ���ﲻ��Ҫ���� cache
             2: __vmmPhysicalPageClone() ����һ��Ϊ pvmpagePhysical, ��Ϊ pvmpagePhysical ��Ӧ�������ַ
                ����������Ҫ copy ������Դ, ��Ϊ pvmpageReal ��Ӧ�������ַ�����Ѿ������ڻ��߱���д��.
*********************************************************************************************************/
static INT  __vmmAbortCopyOnWrite (PLW_VMM_ABORT_CTX  pabtctx,
                                   PLW_VMM_PAGE       pvmpageVirtual,
                                   addr_t             ulAbortAddrAlign,
                                   PLW_CLASS_TCB      ptcbCur,
                                   LW_OBJECT_HANDLE  *pulGuarder)
{
    PLW_VMM_PAGE            pvmpagePhysical = __pageFindLink(pvmpageVirtual, ulAbortAddrAlign);
    PLW_VMM_PAGE            pvmpageReal;
    PLW_VMM_PAGE            pvmpageNew;
    PLW_VMM_PAGE_PRIVATE    pvmpagep;

    BOOL                    bNoLimit;
    ULONG                   ulError;
    
    if (pvmpagePhysical == LW_NULL) {
        return  (PX_ERROR);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;

    if (LW_VMM_PAGE_IS_FAKE(pvmpagePhysical)) {
        pvmpageReal = LW_VMM_PAGE_GET_REAL(pvmpagePhysical);
    
    } else {
        pvmpageReal = pvmpagePhysical;
    }
    
    if ((pvmpageReal->PAGE_ulRef == 1) ||                               /*  ��ǰ����ҳ����Ψһ����      */
        (pvmpagep->PAGEP_iFlags & LW_VMM_SHARED_CHANGE)) {              /*  ����仯                    */
        
        pvmpageReal->PAGE_iChange     = 1;
        pvmpagePhysical->PAGE_iChange = 1;
        pvmpageReal->PAGE_ulFlags     = pvmpageVirtual->PAGE_ulFlags;
        pvmpagePhysical->PAGE_ulFlags = pvmpageVirtual->PAGE_ulFlags;
        
        __vmmLibSetFlag(ulAbortAddrAlign, 1, 
                        pvmpageVirtual->PAGE_ulFlags, LW_TRUE);         /*  ��д                        */
        return  (ERROR_NONE);                                           /*  �������Լ���                */
    
    } else {                                                            /*  �������Ĺ������            */
        bNoLimit = __vmmPhysicalPageFaultCheck(1, ptcbCur, pulGuarder); /*  ��������ڴ��Ƿ���        */
        if (!bNoLimit) {
            __ABTCTX_ABORT_TYPE(pabtctx) = LW_VMM_ABORT_TYPE_NOINFO;    /*  ȱ������ҳ��                */
            printk(KERN_CRIT "kernel no more physical page.\n");        /*  ϵͳ�޷���������ҳ��        */
            return  (PX_ERROR);
        }
                                                                        /*  ִ�� copy-on-write ����     */
        pvmpageNew = __vmmPhysicalPageClone(pvmpagePhysical);
        if (pvmpageNew == LW_NULL) {
            if (API_GetLastError() == ERROR_VMM_LOW_PHYSICAL_PAGE) {
                printk(KERN_CRIT "kernel no more physical page.\n");    /*  ϵͳ�޷���������ҳ��        */
            }
            return  (PX_ERROR);
        }
        
        __pageUnlink(pvmpageVirtual, pvmpagePhysical);                  /*  ������ӹ�ϵ                */
        
        __vmmPhysicalPageFree(pvmpagePhysical);                         /*  ����ʹ��������ҳ��          */
        
        ulError = __vmmLibPageMap(pvmpageNew->PAGE_ulPageAddr,          
                                  ulAbortAddrAlign, 1,
                                  pvmpageVirtual->PAGE_ulFlags);        /*  ��д                        */
        if (ulError) {
            __vmmPhysicalPageFree(pvmpageNew);
            return  (PX_ERROR);
        }
        
        pvmpageNew->PAGE_ulMapPageAddr = ulAbortAddrAlign;
        pvmpageNew->PAGE_ulFlags       = pvmpageVirtual->PAGE_ulFlags;
        pvmpageNew->PAGE_iChange       = 1;
                                                                        
        __pageLink(pvmpageVirtual, pvmpageNew);                         /*  ��������                    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmAbortWriteProtect
** ��������: ����д������ֹ
** �䡡��  : pabtctx               page fail ������
**           pvmpageVirtual        ����ռ�
**           ulAbortAddr           �쳣�ĵ�ַ
**           ptcbCur               ��ǰ������ƿ�
**           pulGuarder            �Ƿ���Ҫ����ػ��߲��� SIGLOWMEM WARNING ��Ϣ
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static INT  __vmmAbortWriteProtect (PLW_VMM_ABORT_CTX  pabtctx,
                                    PLW_VMM_PAGE       pvmpageVirtual,
                                    addr_t             ulAbortAddr,
                                    PLW_CLASS_TCB      ptcbCur,
                                    LW_OBJECT_HANDLE  *pulGuarder)
{
    addr_t  ulAbortAddrAlign = ulAbortAddr & LW_CFG_VMM_PAGE_MASK;

#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    PLW_VMM_PAGE_PRIVATE   pvmpagep;
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

    if (pvmpageVirtual->PAGE_ulFlags & LW_VMM_FLAG_WRITABLE) {          /*  ����ռ�����д����          */
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
        pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
        if (pvmpagep && pvmpagep->PAGEP_stPtSize) {
            if ((ulAbortAddr >= pvmpagep->PAGEP_ulPtStart) ||
                (ulAbortAddr < (pvmpagep->PAGEP_ulPtStart + pvmpagep->PAGEP_stPtSize))) {
                if (ptcbCur->TCB_pvVProcessContext) {                   /*  �������޸ı�����            */
#if LW_CFG_GDB_EN > 0
                    if (ptcbCur->TCB_ulAbortPointAddr == LW_GDB_ADDR_INVAL)
#endif                                                                  /*  LW_CFG_GDB_EN               */
                    {
                        return  (PX_ERROR);                             /*  ɱ������, �ڴ治��д        */
                    }
                }
            }
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

        return  (__vmmAbortCopyOnWrite(pabtctx,
                                       pvmpageVirtual,
                                       ulAbortAddrAlign,
                                       ptcbCur, pulGuarder));           /*  copy-on-write               */
    }
    
    return  (PX_ERROR);                                                 /*  ɱ������, �ڴ治��д        */
}
/*********************************************************************************************************
** ��������: __vmmAbortNoPage
** ��������: ȱҳ��ֹʱ�����µ�ҳ��
** �䡡��  : pvmpagePhysical       ����ҳ��
**           pvmpagep              ����ռ�����
**           ulVirtualPageAlign    �����ַ
**           ulAllocPageNum        ����ҳ������
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static INT  __vmmAbortNoPage (PLW_VMM_PAGE           pvmpagePhysical, 
                              PLW_VMM_PAGE_PRIVATE   pvmpagep,
                              addr_t                 ulVirtualPageAlign, 
                              ULONG                  ulAllocPageNum)
{
    addr_t  ulSwitchAddr = __vmmVirtualSwitch();
    ULONG   ulError;

    if (pvmpagep->PAGEP_pfuncFiller) {                                  /*  ��Ҫ���                    */
        ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,     /*  ʹ�� CACHE ����             */
                                  ulSwitchAddr,                         /*  �����������ַ              */
                                  ulAllocPageNum, 
                                  LW_VMM_FLAG_RDWR);                    /*  ӳ��ָ���������ַ          */
        if (ulError) {
            _K_vmmStatus.VMMS_i64MapErrCounter++;
            printk(KERN_CRIT "kernel physical page map error.\n");      /*  ϵͳ�޷�ӳ������ҳ��        */
            return  (PX_ERROR);
        }

        pvmpagep->PAGEP_pfuncFiller(pvmpagep->PAGEP_pvArg,          
                                    ulSwitchAddr,                       /*  ��Ҫ�����Ļ���Ŀ�������ַ  */
                                    ulVirtualPageAlign,                 /*  ��������л���Ŀ�������ַ  */
                                    ulAllocPageNum);                    /*  ������ҳ�����              */
                                                                        /*  cache ˢ��                  */
        __vmmAbortCacheRefresh(LW_FALSE, 
                               ulVirtualPageAlign, 
                               pvmpagePhysical->PAGE_ulPageAddr, 
                               ulAllocPageNum);
    
        __vmmLibSetFlag(ulSwitchAddr, 1, LW_VMM_FLAG_FAIL, LW_TRUE);    /*  VIRTUAL_SWITCH ���������   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmAbortSwapPage
** ��������: ҳ�潻��
** �䡡��  : pvmpagePhysical       ����ҳ��
**           ulVirtualPageAlign    �����ַ
**           ulAllocPageNum        ҳ�����
**           ptcbCur               ��ǰ������ƿ�
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��������������, �������ʱ, ������ vmm lock �½���, ����� vmm ���������� vmm �ϵ�����
             �������п��ܻ��������! ����Ҳ����ֱ�ӽ�������䵽ָ���������ַ��, ��Ϊ���״̬��û���κ���
             ������ס�����������ָ������ڴ�! ���ֻ���Ƚ�����ڴ�ת������л�������ҳ��.
             (���е���䶼���� vmm lock �´���ִ�е�)
*********************************************************************************************************/
static INT  __vmmAbortSwapPage (PLW_VMM_PAGE  pvmpagePhysical, 
                                addr_t        ulVirtualPageAlign, 
                                ULONG         ulAllocPageNum,
                                PLW_CLASS_TCB ptcbCur)
{
    addr_t          ulSwitchAddr = __vmmVirtualSwitch();
    ULONG           ulError;

    ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,         /*  ʹ�� CACHE ����             */
                              ulSwitchAddr,                             /*  �����������ַ              */
                              ulAllocPageNum, 
                              LW_VMM_FLAG_RDWR);                        /*  ӳ��ָ���������ַ          */
    if (ulError) {
        _K_vmmStatus.VMMS_i64MapErrCounter++;
        printk(KERN_CRIT "kernel physical page map error.\n");          /*  ϵͳ�޷�ӳ������ҳ��        */
        return  (PX_ERROR);
    }
    
    ulError = __vmmPageSwapLoad(__PAGEFAIL_CUR_PID,
                                ulSwitchAddr, 
                                ulVirtualPageAlign);                    /*  �ӽ������ж�ȡҳ��ԭʼ����  */
    if (ulError) {
        printk(KERN_CRIT "kernel swap load page error.\n");             /*  ϵͳ�޷����� swap ҳ��      */
        return  (PX_ERROR);                                             /*  ������� swap �����ͳ�Ʊ���*/
    }
    
    __vmmAbortCacheRefresh(LW_TRUE, 
                           ulVirtualPageAlign, 
                           pvmpagePhysical->PAGE_ulPageAddr, 
                           ulAllocPageNum);                             /*  cache ˢ��                  */
    
    __vmmLibSetFlag(ulSwitchAddr, 1, LW_VMM_FLAG_FAIL, LW_TRUE);        /*  VIRTUAL_SWITCH ���������   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmAbortNewPage
** ��������: ����һ���µ�ҳ��, ���ȱ������ҳ���򽻻�һ���Ͼɵ�����ҳ��
** �䡡��  : ulAllocPageNum        ��Ҫ���ڴ�ҳ�����
**           ptcbCur               ��ǰ������ƿ�
**           pulGuarder            �Ƿ���Ҫ����ػ��߲��� SIGLOWMEM WARNING ��Ϣ
** �䡡��  : ����ҳ��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static PLW_VMM_PAGE  __vmmAbortNewPage (ULONG              ulAllocPageNum,
                                        PLW_CLASS_TCB      ptcbCur,
                                        LW_OBJECT_HANDLE  *pulGuarder)
{
    PLW_VMM_PAGE    pvmpagePhysical;
    ULONG           ulZoneIndex;
    BOOL            bNoLimit;
    
    bNoLimit = __vmmPhysicalPageFaultCheck(1, ptcbCur, pulGuarder);     /*  ��������ڴ��Ƿ���        */
    if (!bNoLimit) {
        return  (LW_NULL);
    }
    
    pvmpagePhysical = __vmmPhysicalPageAlloc(ulAllocPageNum, 
                                             LW_ZONE_ATTR_NONE,
                                             &ulZoneIndex);             /*  ���������ڴ�(1��ҳ��)       */
    if (pvmpagePhysical == LW_NULL) {                                   /*  �������ʧ���񽻻���һ��ҳ��*/
        _K_vmmStatus.VMMS_i64PageLackCounter++;
        pvmpagePhysical = __vmmPageSwapSwitch(__PAGEFAIL_CUR_PID,
                                              ulAllocPageNum, 
                                              LW_ZONE_ATTR_NONE);       /*  ����ҳ�潻��                */
        return  (pvmpagePhysical);
    }
    
    return  (pvmpagePhysical);
}
/*********************************************************************************************************
** ��������: __vmmAbortShareCallback
** ��������: __vmmAbortShare �ص�����
** �䡡��  : pvStartAddr       ���ҵ��ɹ���������������ʵ�����ַ
**           stOffset          �ɹ���ҳ��ƫ��
** �䡡��  : ����ҳ��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static PVOID __vmmAbortShareCallback (PVOID  pvStartAddr, size_t  stOffset)
{
    PLW_VMM_PAGE         pvmpageVirtual;
    PLW_VMM_PAGE         pvmpagePhysical;
    PLW_VMM_PAGE_PRIVATE pvmpagep;
    addr_t               ulVirAddr = (addr_t)pvStartAddr;
    
    pvmpageVirtual = __areaVirtualSearchPage(ulVirAddr);
    if (pvmpageVirtual == LW_NULL) {
        return  (LW_NULL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (pvmpagep == LW_NULL) {
        return  (LW_NULL);
    }
    
    if (pvmpagep->PAGEP_iFlags & LW_VMM_PRIVATE_CHANGE) {               /*  Ŀ�������������          */
        return  (LW_NULL);
    }
    
    ulVirAddr += stOffset;
    
    pvmpagePhysical = __pageFindLink(pvmpageVirtual, ulVirAddr);
    
    return  ((PVOID)pvmpagePhysical);
}
/*********************************************************************************************************
** ��������: __vmmAbortShare
** ��������: ��ͼ�� mmap �����ҵ����Թ�����ʵ�ҳ��
** �䡡��  : pvmpageVirtual        �����쳣������ҳ��
**           pvmpagep              ����ҳ����Ϣ
**           ulVirtualPageAlign    �����쳣��ҳ������ַ
** �䡡��  : �Ƿ��Ѿ����ù���
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static INT  __vmmAbortShare (PLW_VMM_PAGE         pvmpageVirtual, 
                             PLW_VMM_PAGE_PRIVATE pvmpagep,
                             addr_t               ulVirtualPageAlign)
{
    PLW_VMM_PAGE    pvmpagePhysical;
    PLW_VMM_PAGE    pvmpageRef;
    ULONG           ulError;
    
    if (pvmpagep->PAGEP_pfuncFindShare == LW_NULL) {
        return  (PX_ERROR);
    }
    
    pvmpagePhysical = (PLW_VMM_PAGE)pvmpagep->PAGEP_pfuncFindShare(pvmpagep->PAGEP_pvFindArg, 
                                                                   ulVirtualPageAlign,
                                                                   __vmmAbortShareCallback);
    if (pvmpagePhysical == LW_NULL) {
        return  (PX_ERROR);
    }
    
    pvmpageRef = __vmmPhysicalPageRef(pvmpagePhysical);                 /*  ��������ҳ��                */
    if (pvmpageRef == LW_NULL) {
        return  (PX_ERROR);
    }
    
    ulError = __vmmLibPageMap(pvmpageRef->PAGE_ulPageAddr,
                              ulVirtualPageAlign, 1, 
                              pvmpageRef->PAGE_ulFlags);
    if (ulError) {
        __vmmPhysicalPageFree(pvmpageRef);
        return  (PX_ERROR);
    }
    
    pvmpageRef->PAGE_ulMapPageAddr = ulVirtualPageAlign;
    
    __pageLink(pvmpageVirtual, pvmpageRef);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmAbortDump
** ��������: �� __vmmAbortShell() �޷�ִ��ʱ, ��ӡ������Ϣ
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmAbortDump (PLW_VMM_ABORT_CTX  pabtctx)
{
             addr_t                 ulAbortAddr = pabtctx->ABTCTX_ulAbortAddr;
             LW_OBJECT_HANDLE       ulOwner;
             PCHAR                  pcTail;
             CPCHAR                 pcKernelFunc;
             CHAR                   cMmapMsg[128] = "<unknown>.";
             
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    ulOwner      = __KERNEL_OWNER();
    pcKernelFunc = __KERNEL_ENTERFUNC();
    
    pvmpageVirtual = __vmmAbortPageGet(ulAbortAddr);                    /*  ��ö�Ӧ�����ڴ���ƿ�      */
    
    if (pvmpageVirtual) {
        pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
        if (pvmpagep->PAGEP_pfuncFiller) {
            PLW_VMM_MAP_NODE  pmapn = (PLW_VMM_MAP_NODE)pvmpagep->PAGEP_pvArg;
            
            snprintf(cMmapMsg, 128, "address in mmap, fdesc %d pid %d\n", 
                     pmapn->MAPN_iFd, pmapn->MAPN_pid);
            pcTail = cMmapMsg;
            
        } else {
            pcTail = "address in vmm.";
        }
    } else {
        pcTail = "address invalidate.";
    }
    
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
    _CrashDumpAbortKernel(ulOwner, pcKernelFunc, pabtctx,
                          __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo),
                          pcTail);
#endif
    
    _DebugFormat(__ERRORMESSAGE_LEVEL, 
                 "FATAL ERROR: abort in kernel status. "
                 "kowner: 0x%08lx, kfunc: %s, "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s, %s.\r\n",
                 ulOwner, pcKernelFunc, 
                 pabtctx->ABTCTX_ulRetAddr,
                 pabtctx->ABTCTX_ulAbortAddr,
                 __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo),
                 pcTail);
}
/*********************************************************************************************************
** ��������: __vmmAbortShell
** ��������: �� MMU ��������ʧЧʱ, �߳�ִ�����庯��.
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmAbortShell (PLW_VMM_ABORT_CTX  pabtctx)
{
             INTREG                 iregInterLevel;
             addr_t                 ulAbortAddr = pabtctx->ABTCTX_ulAbortAddr;
             ULONG                  ulFlag;
             
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             
             ULONG                  ulAllocPageNum;
             BOOL                   bSwapNeedLoad;
             LW_OBJECT_HANDLE       ulGuarder = LW_OBJECT_HANDLE_INVALID;
             
             addr_t                 ulVirtualPageAlign;
             ULONG                  ulError;
             INT                    iRet;
             PLW_CLASS_TCB          ptcbCur;                            /*  ��ǰ������ƿ�              */

    if (__KERNEL_ISENTER()) {
        __vmmAbortDump(pabtctx);                                        /*  ��ӡ�ؼ���Ϣ                */
        __vmmAbortKill(pabtctx);
        goto    __abort_return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __VMM_LOCK();
    _K_vmmStatus.VMMS_i64AbortCounter++;
    
    if ((__ABTCTX_ABORT_TYPE(pabtctx) != LW_VMM_ABORT_TYPE_MAP) &&
        (__ABTCTX_ABORT_TYPE(pabtctx) != LW_VMM_ABORT_TYPE_PERM)) {
        __VMM_UNLOCK();
        __vmmAbortAccess(pabtctx);                                      /*  �����쳣���                */
        goto    __abort_return;                                         /*  �������е�����              */
    }
    
    pvmpageVirtual = __vmmAbortPageGet(ulAbortAddr);                    /*  ��ö�Ӧ�����ڴ���ƿ�      */
    if (pvmpageVirtual) {
        pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    } else {
        __VMM_UNLOCK();
        __vmmAbortAccess(pabtctx);                                      /*  �����쳣                    */
        goto    __abort_return;                                         /*  �������е�����              */
    }
    
    _K_vmmStatus.VMMS_i64PageFailCounter++;                             /*  ȱҳ�жϴ���++              */
    ptcbCur->TCB_i64PageFailCounter++;                                  /*  ȱҳ�жϴ���++              */
    
    ulVirtualPageAlign = ulAbortAddr & LW_CFG_VMM_PAGE_MASK;            /*  ��÷��ʵ�ַҳ�߽�          */
    
    ulError = __vmmLibGetFlag(ulVirtualPageAlign, &ulFlag);             /*  ����쳣��ַ������ҳ������  */
    if (ulError == ERROR_NONE) {                                        /*  ��������ҳ������            */
        if (__ABTCTX_ABORT_TYPE(pabtctx) == LW_VMM_ABORT_TYPE_MAP) {    /*  MAP ���ʹ���                */
            __VMM_UNLOCK();
            goto    __abort_return;                                     /*  ҳ���Ѿ�����, ���Է���      */
        }
                                                                        /*  д���쳣                    */
        if (__ABTCTX_ABORT_METHOD(pabtctx) == LW_VMM_ABORT_METHOD_WRITE) {
            if (ulFlag & LW_VMM_FLAG_WRITABLE) {                        /*  ������������ͬʱ���ʴ˵�ַ  */
                __VMM_UNLOCK();
                goto    __abort_return;
            }
                                                                        /*  ����д��������              */
            if (__vmmAbortWriteProtect(pabtctx,
                                       pvmpageVirtual,
                                       ulAbortAddr,
                                       ptcbCur,
                                       &ulGuarder) == ERROR_NONE) {     /*  ������ copy-on-write ����   */
                __VMM_UNLOCK();
                goto    __abort_return;
            }
        }
        
        __VMM_UNLOCK();
        __vmmAbortAccess(pabtctx);                                      /*  �Ƿ��ڴ����                */
        goto    __abort_return;                                         /*  �������е�����              */
    
    } else {                                                            /*  ӳ�����, û������ҳ�����  */
        if (pvmpagep->PAGEP_iFlags & LW_VMM_SHARED_CHANGE) {            /*  ��������                    */
            if (__vmmAbortShare(pvmpageVirtual, 
                                pvmpagep,
                                ulVirtualPageAlign) == ERROR_NONE) {    /*  ���Թ���                    */
                __VMM_UNLOCK();
                goto    __abort_return;
            }
        }
        
        ulAllocPageNum  = __PAGEFAIL_ALLOC_PAGE_NUM;                    /*  ȱҳ�жϷ�����ڴ�ҳ�����  */
        
        pvmpagePhysical = __vmmAbortNewPage(ulAllocPageNum,
                                            ptcbCur, &ulGuarder);       /*  ��������ҳ��                */
        if (pvmpagePhysical == LW_NULL) {
            __VMM_UNLOCK();

            __ABTCTX_ABORT_TYPE(pabtctx) = LW_VMM_ABORT_TYPE_NOINFO;    /*  ȱ������ҳ��                */
            printk(KERN_CRIT "kernel no more physical page.\n");        /*  ϵͳ�޷���������ҳ��        */
            __vmmAbortKill(pabtctx);
            goto    __abort_return;
        }
        
        bSwapNeedLoad = __vmmPageSwapIsNeedLoad(__PAGEFAIL_CUR_PID,
                                                ulVirtualPageAlign);    /*  ����Ƿ���Ҫ load swap ���� */
        if (bSwapNeedLoad) {
            iRet = __vmmAbortSwapPage(pvmpagePhysical, 
                                      ulVirtualPageAlign,
                                      ulAllocPageNum,
                                      ptcbCur);                         /*  ����ҳ�潻������            */
        
        } else {
            iRet = __vmmAbortNoPage(pvmpagePhysical, 
                                    pvmpagep,
                                    ulVirtualPageAlign, 
                                    ulAllocPageNum);                    /*  ����ҳ����䴦��            */
        }
        
        if (iRet != ERROR_NONE) {
            __vmmPhysicalPageFree(pvmpagePhysical);
            __VMM_UNLOCK();
            __vmmAbortKill(pabtctx);
            goto    __abort_return;
        }
    }
    
    ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,         /*  �����ʱ��ͬ������          */
                              ulVirtualPageAlign,
                              ulAllocPageNum, 
                              pvmpageVirtual->PAGE_ulFlags);            /*  ӳ��ָ���������ַ          */
    if (ulError) {
        _K_vmmStatus.VMMS_i64MapErrCounter++;
        __vmmPhysicalPageFree(pvmpagePhysical);
        __VMM_UNLOCK();

        printk(KERN_CRIT "kernel physical page map error.\n");          /*  ϵͳ�޷�ӳ������ҳ��        */
        __vmmAbortKill(pabtctx);
        goto    __abort_return;
    }
    
    pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualPageAlign;
    pvmpagePhysical->PAGE_ulFlags       = pvmpageVirtual->PAGE_ulFlags;
    
    __pageLink(pvmpageVirtual, pvmpagePhysical);                        /*  �������ӹ�ϵ                */
    __VMM_UNLOCK();
    
__abort_return:
    if (ulGuarder) {
        __vmmWarnGuarder(ulGuarder);                                    /*  ���;���֪ͨ                */
    }
    __KERNEL_SPACE_SET(pabtctx->ABTCTX_iKernelSpace);                   /*  �ָ��ɽ���֮ǰ��״̬        */
    errno = pabtctx->ABTCTX_iLastErrno;                                 /*  �ָ�֮ǰ�� errno            */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    KN_SMP_MB();
    archSigCtxLoad(&pabtctx->ABTCTX_archRegCtx);                        /*  �� page fail �������з���   */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���в�������                */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
   ���º���û�� MMU ʱҲ��ʹ��, ��Ҫ������������쳣
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __vmmAbortTypeStr
** ��������: �� __vmmAbortShell() �޷�ִ��ʱ, ��ӡ������Ϣ
** �䡡��  : pabtInfo       �쳣����
** �䡡��  : �쳣�����ִ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PCHAR  __vmmAbortTypeStr (PLW_VMM_ABORT  pabtInfo)
{
    switch (pabtInfo->VMABT_uiType) {
    
    case LW_VMM_ABORT_TYPE_TERMINAL:
        return  ("cpu extremity error");
        
    case LW_VMM_ABORT_TYPE_MAP:
        return  ("memory map");
    
    case LW_VMM_ABORT_TYPE_PERM:
        switch (pabtInfo->VMABT_uiMethod) {
        
        case LW_VMM_ABORT_METHOD_READ:
            return  ("can not read");
            
        case LW_VMM_ABORT_METHOD_WRITE:
            return  ("can not write");
            
        case LW_VMM_ABORT_METHOD_EXEC:
            return  ("can not execute");
            
        default:
            return  ("unknown");
        }
    
    case LW_VMM_ABORT_TYPE_FPE:
        return  ("float points");

    case LW_VMM_ABORT_TYPE_DSPE:
        return  ("dsp error");
    
    case LW_VMM_ABORT_TYPE_BUS:
        return  ("bus error");
    
    case LW_VMM_ABORT_TYPE_BREAK:
        return  ("break points");
    
    case LW_VMM_ABORT_TYPE_SYS:
        return  ("syscall");
    
    case LW_VMM_ABORT_TYPE_UNDEF:
        return  ("undefined instruction");
    
    case LW_VMM_ABORT_TYPE_FATAL_ERROR:
        return  ("fata error");
    
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** ��������: __vmmWarnGuarder
** ��������: ���ڴ��ػ��̷߳���һ��������Ϣ
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  __vmmWarnGuarder (LW_OBJECT_HANDLE  ulGuarder)
{
#if LW_CFG_SIGNAL_EN > 0
    struct sigevent  sigeventWarn;

    sigeventWarn.sigev_signo  = SIGLOWMEM;
    sigeventWarn.sigev_notify = SIGEV_SIGNAL;
    sigeventWarn.sigev_value.sival_ptr = LW_NULL;

    _doSigEvent(ulGuarder, &sigeventWarn, SI_KILL);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: __vmmAbortKill
** ��������: ��ǰ�̲߳���һ���ź�
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmAbortKill (PLW_VMM_ABORT_CTX  pabtctx)
{
#if LW_CFG_SIGNAL_EN > 0
    struct sigevent  sigeventAbort;
    INT              iSigCode;
    BOOL             bSerious;
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

#if LW_CFG_GDB_EN > 0
    if (!__KERNEL_ISENTER()) {
        if (API_DtraceAbortTrap(pabtctx->ABTCTX_ulRetAddr) ==
            ERROR_NONE) {                                               /*  ����֪ͨ������              */
            return;                                                     /*  ���غ󽫻�����ϵ�          */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    _BugHandle((_ObjectGetIndex(pabtctx->ABTCTX_ptcb->TCB_ulId) < LW_NCPUS),
               LW_TRUE, "idle thread serious error!\r\n");
    
#if LW_CFG_SIGNAL_EN > 0
    if (__KERNEL_ISENTER()) {                                           /*  �ں�״̬�³������ش���      */
        API_KernelReboot(LW_REBOOT_FORCE);                              /*  ֱ��������������ϵͳ        */
    }

    switch (__ABTCTX_ABORT_TYPE(pabtctx)) {                             /*  �����쳣����                */
    
    case 0:
        sigeventAbort.sigev_signo = SIGKILL;                            /*  ͨ�� SIGKILL �ź�ɱ������   */
        iSigCode = SI_KERNEL;
        bSerious = LW_FALSE;
        break;
    
    case LW_VMM_ABORT_TYPE_FPE:
        sigeventAbort.sigev_signo = SIGFPE;
        iSigCode = __ABTCTX_ABORT_METHOD(pabtctx);
        bSerious = LW_TRUE;
        break;
        
    case LW_VMM_ABORT_TYPE_BUS:
        sigeventAbort.sigev_signo = SIGBUS;
        iSigCode = __ABTCTX_ABORT_METHOD(pabtctx);
        bSerious = LW_TRUE;
        break;
        
    case LW_VMM_ABORT_TYPE_SYS:
        sigeventAbort.sigev_signo = SIGSYS;
        iSigCode = SI_KERNEL;
        bSerious = LW_TRUE;
        break;
        
    case LW_VMM_ABORT_TYPE_UNDEF:
        sigeventAbort.sigev_signo = SIGILL;
        iSigCode = ILL_ILLOPC;
        bSerious = LW_TRUE;
        break;
        
    case LW_VMM_ABORT_TYPE_MAP:
        sigeventAbort.sigev_signo = SIGSEGV;
        iSigCode = SEGV_MAPERR;
        bSerious = LW_TRUE;
        break;
        
    case LW_VMM_ABORT_TYPE_PERM:                                        /*  �� default �����֧         */
    default:
        sigeventAbort.sigev_signo = SIGSEGV;
        iSigCode = SEGV_ACCERR;
        bSerious = LW_TRUE;
        break;
    }

    sigeventAbort.sigev_value.sival_ptr = (PVOID)pabtctx->ABTCTX_ulAbortAddr;
    sigeventAbort.sigev_notify          = SIGEV_SIGNAL;
    
    _doSigEvent(pabtctx->ABTCTX_ptcb->TCB_ulId, &sigeventAbort, iSigCode);

    if (LW_KERN_BUG_REBOOT_EN_GET()) {
        if (bSerious && !vprocGetPidByTcbNoLock(pabtctx->ABTCTX_ptcb)) {
            API_KernelReboot(LW_REBOOT_FORCE);                          /*  �ں��߳����ش���            */
        }
    }

#else
    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    for (;;) {                                                          /*  1.0.0-rc36 ��ʼ���ﲻ�˳�   */
        API_TimeSleep(__ARCH_ULONG_MAX);                                /*  wait-to-die                 */
    }
}
/*********************************************************************************************************
** ��������: __vmmAbortAccess
** ��������: �����ʹ����� (�˺������᷵��)
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmAbortAccess (PLW_VMM_ABORT_CTX  pabtctx)
{
    INTREG  iregInterLevel;
    
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
    _CrashDumpAbortAccess(pabtctx, __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo));
#endif

#if LW_CFG_ABORT_CALLSTACK_INFO_EN > 0
    API_BacktraceShow(ioGlobalStdGet(STD_ERR), 100);
#endif                                                                  /*  LW_CFG_ABORT_CALLSTACK_IN...*/

#if LW_CFG_DEVICE_EN > 0
    archTaskCtxShow(ioGlobalStdGet(STD_ERR), &pabtctx->ABTCTX_archRegCtx);
#endif

#if LW_CFG_ABORT_BASIC_INFO_EN > 0
    switch (__ABTCTX_ABORT_TYPE(pabtctx)) {
    
    case LW_VMM_ABORT_TYPE_UNDEF:
        printk(KERN_EMERG "UNDEF ERROR: abort in thread %lx[%s]. \n"
               "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
               pabtctx->ABTCTX_ptcb->TCB_ulId,
               pabtctx->ABTCTX_ptcb->TCB_cThreadName,
               pabtctx->ABTCTX_ulRetAddr,
               pabtctx->ABTCTX_ulAbortAddr,
               __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo));            /*  �����쳣                    */
        break;
        
    case LW_VMM_ABORT_TYPE_FPE:
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_DEVICE_EN > 0)
        {
            PLW_CLASS_TCB   ptcbCur;
            LW_TCB_GET_CUR_SAFE(ptcbCur);
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
                __ARCH_FPU_CTX_SHOW(ioGlobalStdGet(STD_ERR), ptcbCur->TCB_pvStackFP);
            }
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        printk(KERN_EMERG "FPU ERROR: abort in thread %lx[%s]. "
               "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
               pabtctx->ABTCTX_ptcb->TCB_ulId,
               pabtctx->ABTCTX_ptcb->TCB_cThreadName,
               pabtctx->ABTCTX_ulRetAddr,
               pabtctx->ABTCTX_ulAbortAddr,
               __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo));            /*  �����쳣                    */
        break;
        
    case LW_VMM_ABORT_TYPE_DSPE:
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
        {
            PLW_CLASS_TCB   ptcbCur;
            LW_TCB_GET_CUR_SAFE(ptcbCur);
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
                __ARCH_DSP_CTX_SHOW(ioGlobalStdGet(STD_ERR), ptcbCur->TCB_pvStackDSP);
            }
        }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
        printk(KERN_EMERG "DSP ERROR: abort in thread %lx[%s]. "
               "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
               pabtctx->ABTCTX_ptcb->TCB_ulId,
               pabtctx->ABTCTX_ptcb->TCB_cThreadName,
               pabtctx->ABTCTX_ulRetAddr,
               pabtctx->ABTCTX_ulAbortAddr,
               __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo));            /*  �����쳣                    */
        break;

    default:
        printk(KERN_EMERG "ACCESS ERROR: abort in thread %lx[%s]. "
               "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
               pabtctx->ABTCTX_ptcb->TCB_ulId,
               pabtctx->ABTCTX_ptcb->TCB_cThreadName,
               pabtctx->ABTCTX_ulRetAddr,
               pabtctx->ABTCTX_ulAbortAddr,
               __vmmAbortTypeStr(&pabtctx->ABTCTX_abtInfo));            /*  �����쳣                    */
        break;
    }
#endif                                                                  /*  LW_CFG_ABORT_BASIC_INFO_EN  */
    
    __vmmAbortKill(pabtctx);                                            /*  �����쳣�ź�                */
    
    __KERNEL_SPACE_SET(pabtctx->ABTCTX_iKernelSpace);                   /*  �ָ��ɽ���֮ǰ��״̬        */
    errno = pabtctx->ABTCTX_iLastErrno;                                 /*  �ָ�֮ǰ�� errno            */
    
    /*
     * iRet == 0 or iRet > 0 ����Ҫ��������ִ��ָ��.
     */
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    KN_SMP_MB();
    archSigCtxLoad(&pabtctx->ABTCTX_archRegCtx);                        /*  �� page fail �������з���   */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���в�������                */
}
/*********************************************************************************************************
** ��������: __vmmAbortFatalDetected
** ��������: ����������
** �䡡��  : ulRetAddr     �쳣���ص�ַ
**           ulAbortAddr   �쳣��ַ (�쳣�������)
**           pabtInfo      �쳣����
**           ptcb          �����쳣���߳̿��ƿ� (����Ϊ NULL)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmAbortFatalDetected (addr_t          ulRetAddr, 
                                      addr_t          ulAbortAddr, 
                                      PLW_VMM_ABORT   pabtInfo,
                                      PLW_CLASS_TCB   ptcb)
{
    struct siginfo  si;

    if ((pabtInfo->VMABT_uiType == LW_VMM_ABORT_TYPE_FATAL_ERROR) ||
        (LW_CPU_GET_CUR_NESTING() > 1)) {
        
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
        _CrashDumpAbortFatal(ulRetAddr, ulAbortAddr, __vmmAbortTypeStr(pabtInfo));
#endif
        
#if LW_CFG_SIGNAL_EN > 0
        si.si_signo = SIGSEGV;
#else
        si.si_signo = 11;
#endif
        si.si_errno = 0;
        si.si_code  = -1;
        si.si_addr  = (PVOID)ulAbortAddr;
        
        __LW_FATAL_ERROR_HOOK(vprocGetPidByTcbNoLock(ptcb), ptcb->TCB_ulId, &si);
        
        if (pabtInfo->VMABT_uiType == LW_VMM_ABORT_TYPE_FATAL_ERROR) {
            _DebugFormat(__ERRORMESSAGE_LEVEL,                          /*  �ؼ��Դ���                  */
                         "FATAL ERROR: exception. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                         "rebooting...\r\n",
                         ulRetAddr, ulAbortAddr, __vmmAbortTypeStr(pabtInfo));
            API_KernelReboot(LW_REBOOT_FORCE);                          /*  ֱ��������������ϵͳ        */
        
        } else {
            _DebugFormat(__ERRORMESSAGE_LEVEL, 
                         "FATAL ERROR: abort occur in exception mode. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                         "rebooting...\r\n",
                         ulRetAddr, ulAbortAddr, __vmmAbortTypeStr(pabtInfo));
            API_KernelReboot(LW_REBOOT_FORCE);                          /*  ֱ��������������ϵͳ        */
        }
    }
}
/*********************************************************************************************************
** ��������: __vmmAbortStkOfDetected
** ��������: ��ջ���������
** �䡡��  : ulRetAddr     �쳣���ص�ַ
**           ulAbortAddr   �쳣��ַ (�쳣�������)
**           pabtInfo      �쳣����
**           ptcb          �����쳣���߳̿��ƿ� (����Ϊ NULL)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  __vmmAbortStkOfDetected (addr_t          ulRetAddr, 
                                      addr_t          ulAbortAddr, 
                                      PLW_VMM_ABORT   pabtInfo,
                                      PLW_CLASS_TCB   ptcb)
{
    addr_t          ulStkPtStart;
    struct siginfo  si;
    
#if LW_CFG_COROUTINE_EN > 0
    PLW_CLASS_COROUTINE  pcrcbCur = _LIST_ENTRY(ptcb->TCB_pringCoroutineHeader, 
                                                LW_CLASS_COROUTINE, 
                                                COROUTINE_ringRoutine);
    ulStkPtStart = (addr_t)pcrcbCur->COROUTINE_pstkStackBottom;
#else
    ulStkPtStart = (addr_t)ptcb->TCB_pstkStackBottom;
#endif

#if CPU_STK_GROWTH == 0
    if ((ulAbortAddr > ulStkPtStart) && 
        (ulAbortAddr <= (ulStkPtStart + LW_CFG_VMM_PAGE_SIZE))) 
#else
    if ((ulAbortAddr < ulStkPtStart) && 
        (ulAbortAddr >= (ulStkPtStart - LW_CFG_VMM_PAGE_SIZE))) 
#endif
    
    {
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
        _CrashDumpAbortStkOf(ulRetAddr, ulAbortAddr, __vmmAbortTypeStr(pabtInfo), ptcb);
#endif
        
#if LW_CFG_SIGNAL_EN > 0
        si.si_signo = SIGSEGV;
#else
        si.si_signo = 11;
#endif
        si.si_errno = 0;
        si.si_code  = SEGV_MAPERR;
        si.si_addr  = (PVOID)ulAbortAddr;
        
        __LW_FATAL_ERROR_HOOK(vprocGetPidByTcbNoLock(ptcb), ptcb->TCB_ulId, &si);
        
        _DebugFormat(__ERRORMESSAGE_LEVEL, 
                     "FATAL ERROR: thread %lx[%s] stack overflow. "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                     "rebooting...\r\n",
                     ptcb->TCB_ulId, ptcb->TCB_cThreadName,
                     ulRetAddr, ulAbortAddr, __vmmAbortTypeStr(pabtInfo));
        API_KernelReboot(LW_REBOOT_FORCE);                              /*  ֱ��������������ϵͳ        */
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: API_VmmAbortReturn
** ��������: API_VmmAbortIsrEx ���� pfuncHandler ����ʱ����
** �䡡��  : pabtctx    page fail ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
VOID  API_VmmAbortReturn (PLW_VMM_ABORT_CTX  pabtctx)
{
    INTREG  iregInterLevel;

    if (pabtctx->ABTCTX_abtInfo.VMABT_uiType) {                         /*  �����޷�������쳣          */
        __vmmAbortAccess(pabtctx);
    }

    __KERNEL_SPACE_SET(pabtctx->ABTCTX_iKernelSpace);                   /*  �ָ��ɽ���֮ǰ��״̬        */
    errno = pabtctx->ABTCTX_iLastErrno;                                 /*  �ָ�֮ǰ�� errno            */

    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    KN_SMP_MB();
    archSigCtxLoad(&pabtctx->ABTCTX_archRegCtx);                        /*  �� page fail �������з���   */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���в�������                */
}
/*********************************************************************************************************
** ��������: API_VmmAbortIsrEx
** ��������: �� MMU ��������ʧЧʱ, ���ô˺���(�������жϷ�����)
** �䡡��  : ulRetAddr     �쳣���ص�ַ
**           ulAbortAddr   �쳣��ַ (�쳣�������)
**           pabtInfo      �쳣����
**           ptcb          �����쳣���߳̿��ƿ� (����Ϊ NULL)
**           pfuncHandler  �쳣������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_API 
VOID  API_VmmAbortIsrEx (addr_t          ulRetAddr, 
                         addr_t          ulAbortAddr, 
                         PLW_VMM_ABORT   pabtInfo, 
                         PLW_CLASS_TCB   ptcb,
                         VOIDFUNCPTR     pfuncHandler)
{
    PLW_VMM_ABORT_CTX    pabtctx;
    PLW_STACK            pstkFailShell;                                 /*  ���� fail shell �Ķ�ջ��    */
    BYTE                *pucStkNow;                                     /*  ��¼��ԭ��ջ��              */
    
    __vmmAbortFatalDetected(ulRetAddr, ulAbortAddr, pabtInfo, ptcb);    /*  ��������̽��                */
    
#if LW_CFG_VMM_EN > 0
    __vmmAbortStkOfDetected(ulRetAddr, ulAbortAddr, pabtInfo, ptcb);    /*  �Ƿ��ջ���                */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
                                                                        /*  �����쳣                    */
    pucStkNow = (BYTE *)archCtxStackEnd(&ptcb->TCB_archRegCtx);         /*  ��¼��ԭ��ջ��              */
#if	CPU_STK_GROWTH == 0
    pucStkNow += sizeof(LW_STACK);                                      /*  ���ջ�����ƶ�һ����ջ�ռ�  */
    pucStkNow  = (BYTE *)ROUND_UP(pucStkNow, ARCH_STK_ALIGN_SIZE);
    pabtctx    = (PLW_VMM_ABORT_CTX)pucStkNow;                          /*  ��¼ PAGE_FAIL_CTX λ��     */
    pucStkNow += __ABTCTX_SIZE_ALIGN;                                   /*  �ó� PAGE_FAIL_CTX �ռ�     */
#else
    pucStkNow -= __ABTCTX_SIZE_ALIGN;                                   /*  �ó� PAGE_FAIL_CTX �ռ�     */
    pucStkNow  = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_STK_ALIGN_SIZE);
    pabtctx    = (PLW_VMM_ABORT_CTX)pucStkNow;                          /*  ��¼ PAGE_FAIL_CTX λ��     */
    pucStkNow -= sizeof(LW_STACK);                                      /*  ���ջ�����ƶ�һ����ջ�ռ�  */
#endif
    
    pabtctx->ABTCTX_ptcb          = ptcb;
    pabtctx->ABTCTX_ulRetAddr     = ulRetAddr;                          /*  �쳣���ص�ַ                */
    pabtctx->ABTCTX_ulAbortAddr   = ulAbortAddr;                        /*  �쳣��ַ (�쳣�������)     */
    pabtctx->ABTCTX_abtInfo       = *pabtInfo;                          /*  �쳣����                    */
    pabtctx->ABTCTX_archRegCtx    = ptcb->TCB_archRegCtx;
    pabtctx->ABTCTX_iLastErrno    = (errno_t)ptcb->TCB_ulLastError;
    pabtctx->ABTCTX_iKernelSpace  = __KERNEL_SPACE_GET2(ptcb);

    pstkFailShell = archTaskCtxCreate(&ptcb->TCB_archRegCtx,
                                      (PTHREAD_START_ROUTINE)pfuncHandler,
                                      (PVOID)pabtctx,
                                      ptcb, (PLW_STACK)pucStkNow, 0);   /*  ���������쳣������ǻ���    */

    archTaskCtxSetFp(pstkFailShell,
                     &ptcb->TCB_archRegCtx,
                     &pabtctx->ABTCTX_archRegCtx);                      /*  ���� fp, ʹ callstack ����  */

    _StackCheckGuard(ptcb);                                             /*  ��ջ������                */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

#if LW_CFG_CPU_FPU_EN > 0
    if (__ABTCTX_ABORT_TYPE(pabtctx) == LW_VMM_ABORT_TYPE_FPE) {
        if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {            /*  ���Ϊ FPU �쳣             */
            __ARCH_FPU_SAVE(ptcb->TCB_pvStackFP);                       /*  ��Ҫ���浱ǰ FPU CTX        */
        }
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    if (__ABTCTX_ABORT_TYPE(pabtctx) == LW_VMM_ABORT_TYPE_DSPE) {
        if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {           /*  ���Ϊ DSP �쳣             */
            __ARCH_DSP_SAVE(ptcb->TCB_pvStackDSP);                      /*  ��Ҫ���浱ǰ DSP CTX        */
        }
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
}
/*********************************************************************************************************
** ��������: API_VmmAbortIsr
** ��������: �� MMU ��������ʧЧʱ, ���ô˺���(�������жϷ�����)
** �䡡��  : ulRetAddr     �쳣���ص�ַ
**           ulAbortAddr   �쳣��ַ (�쳣�������)
**           pabtInfo      �쳣����
**           ptcb          �����쳣���߳̿��ƿ� (����Ϊ NULL)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �� 1.0.0.rc20 �汾��, �˺����������� VMM ����.
**
**           1. ����ȱҳ�ж��߳�ִ������, ��������������е�ҳ�����, ���ʧ��, �߳���������.
**           2. ע��, ����ִ����Ϻ�, ϵͳ�����ܹ��ص��ոշ����ڴ沢�����쳣������ָ��.
**           3. ���ڲ���ȱҳ�ж�ʱ, ����߳�һ���Ǿ�����, �������ﲻ�ü������������.

                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_VmmAbortIsr (addr_t          ulRetAddr, 
                       addr_t          ulAbortAddr, 
                       PLW_VMM_ABORT   pabtInfo, 
                       PLW_CLASS_TCB   ptcb)
{
#if LW_CFG_VMM_EN > 0
    API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, pabtInfo, ptcb, __vmmAbortShell);
#else
    API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, pabtInfo, ptcb, __vmmAbortAccess);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: API_VmmAbortStatus
** ��������: ϵͳ������ֹ״̬. 
** �䡡��  : pvmms         ϵͳ״̬����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

LW_API 
VOID  API_VmmAbortStatus (PLW_VMM_STATUS  pvmms)
{
    if (!pvmms) {
        return;
    }

    __VMM_LOCK();
    *pvmms = _K_vmmStatus;
    __VMM_UNLOCK();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
