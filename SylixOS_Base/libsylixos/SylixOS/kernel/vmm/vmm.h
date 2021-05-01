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
** ��   ��   ��: vmm.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����.

** BUG:
2009.11.10  LW_VMM_ZONEDESC �м��� DMA �����жϷ�, �����������Ƿ�ɹ� DMA ʹ��.
            DMA �ڴ����, ����ֵΪ�����ַ.
2011.03.18  �� LW_VMM_ZONEDESC ����Ϊ LW_VMM_ZONE_DESC.
*********************************************************************************************************/

#ifndef __VMM_H
#define __VMM_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  ZONE ����
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_ZONE_ATTR_NONE       0                                       /*  ����������                  */
#define LW_ZONE_ATTR_DMA        1                                       /*  ӳ��Ϊ DMA ƽ��ģʽʹ��     */

/*********************************************************************************************************
  �����ڴ���Ϣ����
  ע��:
  TEXT, DATA, DMA ����� PHYD_ulPhyAddr ������� PHYD_ulVirMap,
  TEXT, DATA, VECTOR, BOOTSFR, DMA ����� PHYD_ulVirMap ���䲻���������ڴ�ռ��ͻ.
*********************************************************************************************************/

typedef struct __lw_vmm_physical_desc {
    addr_t                   PHYD_ulPhyAddr;                            /*  �����ַ (ҳ�����ַ)       */
    addr_t                   PHYD_ulVirMap;                             /*  ��Ҫ��ʼ����ӳ���ϵ        */
    size_t                   PHYD_stSize;                               /*  �����ڴ������� (ҳ���볤��) */
    
#define LW_PHYSICAL_MEM_TEXT        0                                   /*  �ں˴����                  */
#define LW_PHYSICAL_MEM_DATA        1                                   /*  �ں����ݶ� (���� HEAP)      */
#define LW_PHYSICAL_MEM_VECTOR      2                                   /*  Ӳ��������                  */
#define LW_PHYSICAL_MEM_BOOTSFR     3                                   /*  ����ʱ��Ҫ�����⹦�ܼĴ���  */
#define LW_PHYSICAL_MEM_BUSPOOL     4                                   /*  ���ߵ�ַ��, ��������ǰӳ��  */
#define LW_PHYSICAL_MEM_DMA         5                                   /*  DMA �����ڴ�, ��������ǰӳ��*/
#define LW_PHYSICAL_MEM_APP         6                                   /*  װ�س����ڴ�, ��������ǰӳ��*/
    UINT32                   PHYD_uiType;                               /*  �����ڴ���������            */
    UINT32                   PHYD_uiReserve[8];
} LW_MMU_PHYSICAL_DESC;
typedef LW_MMU_PHYSICAL_DESC *PLW_MMU_PHYSICAL_DESC;

/*********************************************************************************************************
  �����ַ�ռ�����
  ע��:
  �����ڴ�ռ��ַ�����������ڴ�ռ� TEXT, DATA, VECTOR, BOOTSFR, DMA �����ͻ,
  DEV �����ڴ�������һ��.
  һ������� APP �����ڴ�����ҪԶԶ�������� APP �ڴ��С, ͬʱ���� DEV �����ڴ������С.
*********************************************************************************************************/

typedef struct __lw_mmu_virtual_desc {
    addr_t                   VIRD_ulVirAddr;                            /*  ����ռ��ַ (ҳ�����ַ)   */
    size_t                   VIRD_stSize;                               /*  ����ռ䳤�� (ҳ���볤��)   */
    
#define LW_VIRTUAL_MEM_APP      0                                       /*  װ�س��������ڴ�����        */
#define LW_VIRTUAL_MEM_DEV      1                                       /*  �豸ӳ�������ڴ�ռ�        */
    UINT32                   VIRD_uiType;                               /*  �����ڴ���������            */
    UINT32                   VIRD_uiReserve[8];
} LW_MMU_VIRTUAL_DESC;
typedef LW_MMU_VIRTUAL_DESC *PLW_MMU_VIRTUAL_DESC;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  vmm ȱҳ�ж������ڴ�����
*********************************************************************************************************/

typedef struct __lw_vmm_page_fault_limit {
    ULONG                    VPFL_ulRootWarnPages;                      /*  ��Ȩ�û�ȱҳ���䱣���ڴ�    */
    ULONG                    VPFL_ulRootMinPages;                       /*  ��Ȩ�û�ȱҳ��С�����ڴ�    */
    ULONG                    VPFL_ulUserWarnPages;                      /*  ��ͨ�û�ȱҳ���䱣���ڴ�    */
    ULONG                    VPFL_ulUserMinPages;                       /*  ��ͨ�û�ȱҳ��С�����ڴ�    */
    ULONG                    VPFL_ulReserve[8];
} LW_VMM_PAGE_FAULT_LIMIT;
typedef LW_VMM_PAGE_FAULT_LIMIT *PLW_VMM_PAGE_FAULT_LIMIT;

/*********************************************************************************************************
  vmm ��ǰ״̬
*********************************************************************************************************/

typedef struct __lw_vmm_status {
    INT64                    VMMS_i64AbortCounter;                      /*  �쳣��ֹ����                */
    INT64                    VMMS_i64PageFailCounter;                   /*  ȱҳ�ж������������        */
    INT64                    VMMS_i64PageLackCounter;                   /*  ϵͳȱ������ҳ�����        */
    INT64                    VMMS_i64MapErrCounter;                     /*  ӳ��������                */
    INT64                    VMMS_i64SwpCounter;                        /*  ��������                    */
    INT64                    VMMS_i64Reseve[8];
} LW_VMM_STATUS;
typedef LW_VMM_STATUS       *PLW_VMM_STATUS;

/*********************************************************************************************************
  VMM ��ʼ��, ֻ�ܷ��� API_KernelStart �ص���
  
  ��Ϊ SMP ϵͳʱ, API_KernelPrimaryStart    ��Ӧ�����ص����� API_VmmLibPrimaryInit
                   API_KernelSecondaryStart  ��Ӧ�����ص����� API_VmmLibSecondaryInit
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG        API_VmmLibPrimaryInit(LW_MMU_PHYSICAL_DESC  pphydesc[], 
                                          LW_MMU_VIRTUAL_DESC   pvirdes[],
                                          CPCHAR                pcMachineName);
                                                                        /*  ��ʼ�� VMM �������������   */
#define API_VmmLibInit      API_VmmLibPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API ULONG        API_VmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

LW_API ULONG        API_VmmLibAddPhyRam(addr_t  ulPhyRam, size_t  stSize);
                                                                        /*  ��������ڴ����� APP        */
/*********************************************************************************************************
  MMU ������ֹͣ
*********************************************************************************************************/

LW_API VOID         API_VmmMmuEnable(VOID);                             /*  MMU ����                    */

LW_API VOID         API_VmmMmuDisable(VOID);                            /*  MMU ֹͣ                    */

/*********************************************************************************************************
  VMM API (���·��亯�����Է����ȷ����, �ɹ�ֱ�ӷ��ʵ��ڴ�ռ�)
*********************************************************************************************************/

LW_API PVOID        API_VmmMalloc(size_t stSize);                       /*  �����߼������ڴ�, �����ַ  */
LW_API PVOID        API_VmmMallocEx(size_t stSize, ULONG ulFlag);       /*  �����߼������ڴ�, �����ַ  */
LW_API PVOID        API_VmmMallocAlign(size_t stSize, 
                                       size_t stAlign, 
                                       ULONG  ulFlag);                  /*  �����߼������ڴ�, �����ַ  */
LW_API VOID         API_VmmFree(PVOID  pvVirtualMem);                   /*  �������������ڴ�            */

LW_API ULONG        API_VmmVirtualToPhysical(addr_t  ulVirtualAddr,     /*  ͨ�������ַ��ȡ�����ַ    */
                                             phys_addr_t  *ppaPhysicalAddr);

LW_API BOOL         API_VmmVirtualIsInside(addr_t  ulAddr);             /*  ָ����ַ�Ƿ��ڹ��������ռ�*/

LW_API ULONG        API_VmmZoneStatus(ULONG     ulZoneIndex,
                                      addr_t   *pulPhysicalAddr,        /*  0 ~ LW_CFG_VMM_ZONE_NUM - 1 */
                                      size_t   *pstSize,
                                      addr_t   *pulPgd,
                                      ULONG    *pulFreePage,
                                      UINT     *puiAttr);               /*  ��������������Ϣ          */

LW_API ULONG        API_VmmVirtualStatus(UINT32   uiType,               /*  LW_VIRTUAL_MEM_APP / DEV    */
                                         ULONG    ulZoneIndex,          /*  0 ~ LW_CFG_VMM_VIR_NUM - 1  */
                                         addr_t  *pulVirtualAddr,
                                         size_t  *pulSize,
                                         ULONG   *pulFreePage);         /*  �������ռ���Ϣ            */

LW_API VOID         API_VmmPhysicalKernelDesc(PLW_MMU_PHYSICAL_DESC  pphydescText, 
                                              PLW_MMU_PHYSICAL_DESC  pphydescData);
                                                                        /*  ����ں� TEXT DATA ������   */
/*********************************************************************************************************
  VMM ��չ����
  
  ����������ռ�, �����ֵ�һ�η���ʱ, ��ͨ��ȱҳ�жϷ��������ڴ�, ��ȱҳ�ж����޷���������ҳ��ʱ, ���յ�
  SIGSEGV �źŲ������߳�. 
  
  API_VmmRemapArea() ������������ʹ��, ���������� linux remap_pfn_range() ����.
*********************************************************************************************************/

LW_API PVOID        API_VmmMallocArea(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API PVOID        API_VmmMallocAreaEx(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                        INT  iFlags, ULONG ulFlag);     /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API PVOID        API_VmmMallocAreaAlign(size_t stSize, size_t stAlign, 
                                           FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                           INT  iFlags, ULONG  ulFlag); /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API VOID         API_VmmFreeArea(PVOID  pvVirtualMem);               /*  �������������ڴ�            */

LW_API ULONG        API_VmmExpandArea(PVOID  pvVirtualMem, size_t  stExpSize);
                                                                        /*  ��չ�����ڴ����            */
LW_API PVOID        API_VmmSplitArea(PVOID  pvVirtualMem, size_t  stSize);
                                                                        /*  ��������ڴ����            */
LW_API ULONG        API_VmmMergeArea(PVOID  pvVirtualMem1, PVOID  pvVirtualMem2);
                                                                        /*  �ϲ������ڴ����            */
LW_API ULONG        API_VmmMoveArea(PVOID  pvVirtualTo, PVOID  pvVirtualFrom);
                                                                        /*  �ƶ������ڴ����            */
LW_API ULONG        API_VmmPCountInArea(PVOID  pvVirtualMem, ULONG  *pulPageNum);
                                                                        /*  ͳ��ȱҳ�жϷ�����ڴ�ҳ��  */

LW_API ULONG        API_VmmInvalidateArea(PVOID  pvVirtualMem, 
                                          PVOID  pvSubMem, 
                                          size_t stSize);               /*  �ͷ������ڴ�, ��������ռ�  */
                                          
LW_API VOID         API_VmmAbortStatus(PLW_VMM_STATUS  pvmms);          /*  ��÷�����ֹͳ����Ϣ        */

/*********************************************************************************************************
  ��������ʹ�����º���ʵ�� mmap �ӿ�, (�Ƽ�ʹ�õڶ��׽ӿ�)
*********************************************************************************************************/
                                                                        /*  �����µ�ӳ���ϵ            */
LW_API ULONG        API_VmmRemapArea(PVOID  pvVirtualAddr, PVOID  pvPhysicalAddr, 
                                     size_t  stSize, ULONG  ulFlag,
                                     FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  �����µ�ӳ���ϵ            */
LW_API ULONG        API_VmmRemapArea2(PVOID  pvVirtualAddr, phys_addr_t  paPhysicalAddr, 
                                      size_t  stSize, ULONG  ulFlag,
                                      FUNCPTR  pfuncFiller, PVOID  pvArg);

/*********************************************************************************************************
  VMM ���� loader ���������ں�ģ���ṩ�Ĺ����֧�� (���� loader ������ SylixOS �ں˷����Լ�ʹ��)
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFiller(PVOID  pvVirtualMem, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  ������亯��                */
LW_API ULONG        API_VmmSetFindShare(PVOID  pvVirtualMem, PVOIDFUNCPTR  pfuncFindShare, PVOID  pvArg);
                                                                        /*  ���ò�ѯ������            */
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
LW_API ULONG        API_VmmSetProtect(PVOID  pvVirtualMem, PVOID  pvSubMem, size_t  stSize);
#endif                                                                  /*  ���ý�ֹ copy-on-write ��   */

LW_API ULONG        API_VmmPreallocArea(PVOID       pvVirtualMem, 
                                        PVOID       pvSubMem, 
                                        size_t      stSize, 
                                        FUNCPTR     pfuncTempFiller, 
                                        PVOID       pvTempArg,
                                        ULONG       ulFlag);            /*  Ԥ���������ڴ�ҳ��          */
                                        
LW_API ULONG        API_VmmShareArea(PVOID      pvVirtualMem1, 
                                     PVOID      pvVirtualMem2,
                                     size_t     stStartOft1, 
                                     size_t     stStartOft2, 
                                     size_t     stSize,
                                     BOOL       bExecEn,
                                     FUNCPTR    pfuncTempFiller, 
                                     PVOID      pvTempArg);             /*  ���ù�������                */

/*********************************************************************************************************
  �ں�ר�� API �û����� 
  
  �������޸������ڴ����������ҳ��, �޸Ĵ�С�ѷ���ʱ�Ĵ�СΪ��׼
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFlag(PVOID  pvVirtualAddr, 
                                   ULONG  ulFlag);                      /*  ���������ַȨ��            */
                                   
LW_API ULONG        API_VmmGetFlag(PVOID  pvVirtualAddr, 
                                   ULONG *pulFlag);                     /*  ��ȡ�����ַȨ��            */

/*********************************************************************************************************
  Ӧ�ó����ջ����ר�� API (������һҳ��Ϊ��ջ����ҳ��)
*********************************************************************************************************/

LW_API PVOID        API_VmmStackAlloc(size_t  stSize);                  /*  �����ջ                    */

LW_API VOID         API_VmmStackFree(PVOID  pvVirtualMem);              /*  �ͷŶ�ջ                    */

/*********************************************************************************************************
  ���� API ֻ������������ڴ�, ��û�в���ӳ���ϵ. ����ֱ��ʹ��, ����ͨ�������ڴ�ӳ�����ʹ��.
*********************************************************************************************************/

LW_API PVOID        API_VmmPhyAlloc(size_t stSize);                     /*  ���������ڴ�                */
LW_API PVOID        API_VmmPhyAllocEx(size_t  stSize, UINT  uiAttr);    /*  ������ͬ, ������ָ���ڴ�����*/
LW_API PVOID        API_VmmPhyAllocAlign(size_t stSize, 
                                         size_t stAlign,
                                         UINT   uiAttr);                /*  ���������ڴ�, ָ�������ϵ  */
LW_API VOID         API_VmmPhyFree(PVOID  pvPhyMem);                    /*  �ͷ������ڴ�                */

/*********************************************************************************************************
  ���� API ֻ������������ʹ��
  
  no cache ������� (dma �������ص���ֱ�ӵ������ַ, ��ƽ��ӳ��, ������ LW_ZONE_ATTR_DMA ����)
*********************************************************************************************************/

LW_API PVOID        API_VmmDmaAlloc(size_t  stSize);                    /*  �������������ڴ�, �����ַ  */
LW_API PVOID        API_VmmDmaAllocAlign(size_t stSize, size_t stAlign);/*  ������ͬ, ������ָ�������ϵ*/
LW_API PVOID        API_VmmDmaAllocAlignWithFlags(size_t  stSize, size_t  stAlign, ULONG  ulFlags);
                                                                        /*  ������ͬ, ������ָ���ڴ�����*/
LW_API VOID         API_VmmDmaFree(PVOID  pvDmaMem);                    /*  ���� DMA �ڴ滺����         */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  �� VMM ֧��
*********************************************************************************************************/
#else

#ifdef __SYLIXOS_KERNEL
static LW_INLINE ULONG  API_VmmVirtualToPhysical (addr_t  ulVirtualAddr, phys_addr_t *ppaPhysicalAddr)
{
    if (ppaPhysicalAddr) {
        *ppaPhysicalAddr = ulVirtualAddr;
    }
    
    return  (ERROR_NONE);
}
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  API_VmmAbortIsr() Ϊ�쳣������, ֻ���� ARCH ����ʹ��. 
  û��ʹ�� VMM SylixOS ��Ȼ���Դ����쳣���
  
  ���쳣����Ϊ LW_VMM_ABORT_TYPE_FPE �����쳣ʱ: 
  VMABT_uiMethod ȡֵΪ:
  
  FPE_INTDIV            1  Integer divide by zero
  FPE_INTOVF            2  Integer overflow
  FPE_FLTDIV            3  Floating-point divide by zero
  FPE_FLTOVF            4  Floating-point overflow
  FPE_FLTUND            5  Floating-point underflow
  FPE_FLTRES            6  Floating-point inexact result
  FPE_FLTINV            7  inval floating point operate
  FPE_FLTSUB            8  Subscript out of range
  
  ���쳣����Ϊ LW_VMM_ABORT_TYPE_BUS �����쳣ʱ: 
  
  BUS_ADRALN            1  Invalid address alignment
  BUS_ADRERR            2  Nonexistent physical memory
  BUS_OBJERR            3  Object-specific hardware err
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct __lw_vmm_abort {

#define LW_VMM_ABORT_TYPE_NOINFO            0                           /*  �ڲ�ʹ��                    */
#define LW_VMM_ABORT_TYPE_TERMINAL          1                           /*  ��ϵ�ṹ��ش���            */
#define LW_VMM_ABORT_TYPE_MAP               2                           /*  ҳ��ӳ����� (MMU ����)     */
                                                                        /*  ��Ҫ uiMethod ��Ƿ�������  */
#define LW_VMM_ABORT_TYPE_PERM              3                           /*  ����Ȩ�޴��� (MMU ����)     */
                                                                        /*  ��Ҫ uiMethod ��Ƿ�������  */
#define LW_VMM_ABORT_TYPE_FPE               4                           /*  �����������쳣              */
#define LW_VMM_ABORT_TYPE_BUS               5                           /*  ���߷����쳣                */
#define LW_VMM_ABORT_TYPE_BREAK             6                           /*  �ϵ��쳣                    */
#define LW_VMM_ABORT_TYPE_SYS               7                           /*  ϵͳ�����쳣                */
#define LW_VMM_ABORT_TYPE_UNDEF             8                           /*  δ����ָ��, ������ SIGILL   */
#define LW_VMM_ABORT_TYPE_DSPE              9                           /*  DSP �쳣                    */

#define LW_VMM_ABORT_TYPE_FATAL_ERROR       0xffffffff                  /*  ��������, ��Ҫ��������      */

    UINT32               VMABT_uiType;
    
#define LW_VMM_ABORT_METHOD_READ            1                           /*  ������                      */
#define LW_VMM_ABORT_METHOD_WRITE           2                           /*  д����                      */
#define LW_VMM_ABORT_METHOD_EXEC            3                           /*  ִ�з���                    */
    
    UINT32               VMABT_uiMethod;
} LW_VMM_ABORT;
typedef LW_VMM_ABORT    *PLW_VMM_ABORT;

LW_API VOID         API_VmmAbortIsr(addr_t          ulRetAddr,
                                    addr_t          ulAbortAddr, 
                                    PLW_VMM_ABORT   pabtInfo,
                                    PLW_CLASS_TCB   ptcb);              /*  �쳣������                */

LW_API VOID         API_VmmAbortIsrEx(addr_t          ulRetAddr,
                                      addr_t          ulAbortAddr,
                                      PLW_VMM_ABORT   pabtInfo,
                                      PLW_CLASS_TCB   ptcb,
                                      VOIDFUNCPTR     pfuncHandler);    /*  �Զ����쳣������          */

/*********************************************************************************************************
  �쳣����
*********************************************************************************************************/

typedef struct {
    ARCH_REG_CTX        ABTCTX_archRegCtx;                              /*  �Ĵ���������                */
    addr_t              ABTCTX_ulRetAddr;                               /*  �쳣���ص�ַ                */
    addr_t              ABTCTX_ulAbortAddr;                             /*  �쳣��ַ                    */
    LW_VMM_ABORT        ABTCTX_abtInfo;                                 /*  �쳣����                    */
    PLW_CLASS_TCB       ABTCTX_ptcb;                                    /*  �����쳣���߳�              */

    errno_t             ABTCTX_iLastErrno;                              /*  ����ʱ��Ҫ�ָ�����Ϣ        */
    INT                 ABTCTX_iKernelSpace;
} LW_VMM_ABORT_CTX;
typedef LW_VMM_ABORT_CTX    *PLW_VMM_ABORT_CTX;

LW_API VOID         API_VmmAbortReturn(PLW_VMM_ABORT_CTX  pabtctx);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  vmm ȱҳ�ж������ڴ�����
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

LW_API INT          API_VmmPageFaultLimit(PLW_VMM_PAGE_FAULT_LIMIT  pvpflNew,
                                          PLW_VMM_PAGE_FAULT_LIMIT  pvpflOld);
LW_API INT          API_VmmPageFaultGuarder(LW_OBJECT_HANDLE  ulGuarder);

/*********************************************************************************************************
  vmm api macro
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#define vmmMalloc                   API_VmmMalloc
#define vmmMallocEx                 API_VmmMallocEx
#define vmmMallocAlign              API_VmmMallocAlign
#define vmmFree                     API_VmmFree

#define vmmMallocArea               API_VmmMallocArea
#define vmmMallocAreaEx             API_VmmMallocAreaEx
#define vmmMallocAreaAlign          API_VmmMallocAreaAlign
#define vmmFreeArea                 API_VmmFreeArea
#define vmmPCountInArea             API_VmmPCountInArea
#define vmmInvalidateArea           API_VmmInvalidateArea
#define vmmAbortStatus              API_VmmAbortStatus

#define vmmRemapArea                API_VmmRemapArea
#define vmmRemapArea2               API_VmmRemapArea2

#define vmmStackAlloc               API_VmmStackAlloc
#define vmmStackFree                API_VmmStackFree

#define vmmPhyAlloc                 API_VmmPhyAlloc
#define vmmPhyAllocEx               API_VmmPhyAllocEx
#define vmmPhyAllocAlign            API_VmmPhyAllocAlign
#define vmmPhyFree                  API_VmmPhyFree

#define vmmDmaAlloc                 API_VmmDmaAlloc                     /*  ����ֵΪ �����ַ           */
#define vmmDmaAllocAlign            API_VmmDmaAllocAlign                /*  ����ֵΪ �����ַ           */
#define vmmDmaAllocAlignWithFlags   API_VmmDmaAllocAlignWithFlags
#define vmmDmaFree                  API_VmmDmaFree

#define vmmMap                      API_VmmMap
#define vmmVirtualToPhysical        API_VmmVirtualToPhysical
#define vmmPhysicalToVirtual        API_VmmPhysicalToVirtual            /*  ��֧��VMM����������ڴ��ѯ */
#define vmmVirtualIsInside          API_VmmVirtualIsInside

#define vmmSetFlag                  API_VmmSetFlag
#define vmmGetFlag                  API_VmmGetFlag

#define vmmZoneStatus               API_VmmZoneStatus
#define vmmVirtualStatus            API_VmmVirtualStatus
#define vmmPhysicalKernelDesc       API_VmmPhysicalKernelDesc
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#define vmmPageFaultLimit           API_VmmPageFaultLimit
#define vmmPageFaultGuarder         API_VmmPageFaultGuarder

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMM_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
