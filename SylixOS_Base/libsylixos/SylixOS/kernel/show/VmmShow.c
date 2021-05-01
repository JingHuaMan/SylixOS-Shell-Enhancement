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
** ��   ��   ��: pageLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ����ҳ������.

** BUG:
2009.09.30  ��ʾ virtuals ʱ, ������ϸ.
2009.11.13  ���� DMA ���ӡ�������ʴ�ӡ.
2011.05.17  ����ȱҳ�ж���Ϣ��ʾ.
2011.08.03  ������ҳ��û�� Link ����ҳ��ʱ, ��Ҫʹ��ҳ���ѯ��ȷ��ӳ��������ַ.
2013.06.04  ��ʾ����ռ���Ϣʱ, ������ʾ�����ַ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if LW_CFG_VMM_EN > 0
#include "../SylixOS/kernel/vmm/phyPage.h"
#include "../SylixOS/kernel/vmm/virPage.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE     PHYSICAL         SIZE     PAGESIZE       PGD        FREEPAGE  DMA  USED\n\
---- ---------------- ------------ -------- ---------------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
     VIRTUAL          SIZE        WRITE CACHE\n\
---------------- ---------------- ----- -----\n";
#else
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE PHYSICAL   SIZE   PAGESIZE    PGD   FREEPAGE  DMA  USED\n\
---- -------- -------- -------- -------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
VIRTUAL    SIZE   WRITE CACHE\n\
-------- -------- ----- -----\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
/*********************************************************************************************************
** ��������: API_VmmPhysicalShow
** ��������: ��ʾ vmm ����洢����Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmPhysicalShow (VOID)
{
    REGISTER INT                    i;
             addr_t                 ulPhysicalAddr;
             LW_MMU_PHYSICAL_DESC   phydescKernel[2];
             PCHAR                  pcDma;
             UINT                   uiAttr;
             ULONG                  ulFreePage;
             addr_t                 ulPgd;
             size_t                 stSize;
             size_t                 stUsed;
             
             size_t                 stPhyTotalSize = 0;
             size_t                 stVmmTotalSize = 0;
             size_t                 stVmmFreeSize  = 0;

    printf("vmm physical zone show >>\n");
    printf(_G_cZoneInfoHdr);                                            /*  ��ӡ��ӭ��Ϣ                */
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        if (API_VmmZoneStatus(i, &ulPhysicalAddr, &stSize, 
                              &ulPgd, &ulFreePage, &uiAttr)) {
            continue;
        }
        if (!stSize) {
            continue;
        }
        
        pcDma  = (uiAttr & LW_ZONE_ATTR_DMA) ? "true" : "false";
        stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
        stUsed = (stUsed / (stSize / 100));                             /*  ��ֹ���                    */
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        printf("%4d %16lx %12zx %8zx %16lx %8ld %-5s %3zd%%\n",
#else
        printf("%4d %08lx %8zx %8zx %08lx %8ld %-5s %3zd%%\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
               i, ulPhysicalAddr, stSize, (size_t)LW_CFG_VMM_PAGE_SIZE,
               ulPgd, ulFreePage, pcDma, stUsed);
               
        stVmmTotalSize += stSize;
        stVmmFreeSize  += (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
    }
    
    API_VmmPhysicalKernelDesc(&phydescKernel[0], &phydescKernel[1]);
    
    stPhyTotalSize = phydescKernel[0].PHYD_stSize
                   + phydescKernel[1].PHYD_stSize
                   + stVmmTotalSize;

    printf("\n"
           "ALL-Physical memory size: %5zuMB\n"
           "VMM-Physical memory size: %5zuMB\n"
           "VMM-Physical memory free: %5zuMB\n",
           stPhyTotalSize / LW_CFG_MB_SIZE,
           stVmmTotalSize / LW_CFG_MB_SIZE,
           stVmmFreeSize  / LW_CFG_MB_SIZE);
}
/*********************************************************************************************************
** ��������: __vmmVirtualPrint
** ��������: ��ӡ��Ϣ�ص�����
** �䡡��  : pvmpage  ҳ����Ϣ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmVirtualPrint (PLW_VMM_PAGE  pvmpage)
{
    addr_t  ulVirtualAddr = pvmpage->PAGE_ulPageAddr;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    printf("%16lx %16lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#else
    printf("%08lx %8lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_WRITABLE) {
        printf("true  ");
    } else {
        printf("false ");
    }
    
    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_CACHEABLE) {
        printf("true\n");
    } else {
        printf("false\n");
    }
}
/*********************************************************************************************************
** ��������: API_VmmVirtualShow
** ��������: ��ʾ vmm ����洢����Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmVirtualShow (VOID)
{
    INT     i;
    addr_t  ulVirtualAddr;
    size_t  stSize;
    ULONG   ulFreePage;
    size_t  stUsed;
    
    printf("vmm virtual area show >>\n");
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        if (API_VmmVirtualStatus(LW_VIRTUAL_MEM_APP, i, &ulVirtualAddr, &stSize, &ulFreePage)) {
            continue;
        }
        if (!stSize) {
            continue;
        }
        
        stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
        stUsed = (stUsed / (stSize / 100));
        
        printf("vmm virtual program from: 0x%08lx, size: 0x%08zx, used: %zd%%\n", 
               ulVirtualAddr, stSize, stUsed);
    }

    if (API_VmmVirtualStatus(LW_VIRTUAL_MEM_DEV, 0, &ulVirtualAddr, &stSize, &ulFreePage)) {
        return;
    }
    
    stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
    stUsed = (stUsed / (stSize / 100));
    
    printf("vmm virtual ioremap from: 0x%08lx, size: 0x%08zx, used: %zd%%\n", 
           ulVirtualAddr, stSize, stUsed);
    
    printf("vmm virtual area usage as follow:\n");
    
    printf(_G_cAreaInfoHdr);                                            /*  ��ӡ��ӭ��Ϣ                */
    
    __VMM_LOCK();
    __areaVirtualSpaceTraversal(__vmmVirtualPrint);                     /*  ��������ռ���, ��ӡ��Ϣ    */
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** ��������: API_VmmAbortShow
** ��������: ��ʾ vmm ������ֹ��Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmAbortShow (VOID)
{
    LW_VMM_STATUS  vmms;
    
    API_VmmAbortStatus(&vmms);

    printf("vmm abort statistics infomation show >>\n");
    printf("vmm abort (memory access error) counter : %lld\n", vmms.VMMS_i64AbortCounter);
    printf("vmm page fail (alloc success) counter   : %lld\n", vmms.VMMS_i64PageFailCounter);
    printf("vmm alloc physical page error counter   : %lld\n", vmms.VMMS_i64PageLackCounter);
    printf("vmm page map error counter              : %lld\n", vmms.VMMS_i64MapErrCounter);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
