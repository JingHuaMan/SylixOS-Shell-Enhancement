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
** ��   ��   ��: phyPage.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ����ҳ�����.
*********************************************************************************************************/

#ifndef __PHYPAGE_H
#define __PHYPAGE_H

/*********************************************************************************************************
  ZONE
*********************************************************************************************************/

typedef struct __lw_vmm_zone_desc {
    addr_t                   ZONED_ulAddr;                              /*  ��ʼ��ַ                    */
    size_t                   ZONED_stSize;                              /*  ���򳤶�                    */
    UINT                     ZONED_uiAttr;                              /*  ��������                    */
} LW_VMM_ZONE_DESC;
typedef LW_VMM_ZONE_DESC    *PLW_VMM_ZONE_DESC;

/*********************************************************************************************************
  ����ռ����
*********************************************************************************************************/

ULONG           __vmmPhysicalCreate(LW_MMU_PHYSICAL_DESC  pphydesc[]);
PLW_VMM_PAGE    __vmmPhysicalPageAlloc(ULONG  ulPageNum, UINT  uiAttr, ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageAllocZone(ULONG  ulZoneIndex, ULONG  ulPageNum, UINT  uiAttr);
PLW_VMM_PAGE    __vmmPhysicalPageAllocAlign(ULONG   ulPageNum, 
                                            size_t  stAlign, 
                                            UINT    uiAttr, 
                                            ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageClone(PLW_VMM_PAGE  pvmpage);
PLW_VMM_PAGE    __vmmPhysicalPageRef(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFree(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFreeAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageSetFlag(PLW_VMM_PAGE  pvmpage, ULONG  ulFlag, BOOL  bFlushTlb);
VOID            __vmmPhysicalPageSetFlagAll(PLW_VMM_PAGE  pvmpageVirtual, ULONG  ulFlag);

#if LW_CFG_CACHE_EN > 0
VOID            __vmmPhysicalPageFlush(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFlushAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageInvalidate(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageInvalidateAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageClear(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageClearAll(PLW_VMM_PAGE  pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

ULONG           __vmmPhysicalGetZone(addr_t  ulAddr);
ULONG           __vmmPhysicalPageGetMinContinue(ULONG  *pulZoneIndex, UINT  uiAttr);

VOID            __vmmPhysicalGetKernelDesc(PLW_MMU_PHYSICAL_DESC  pphydescText,
                                           PLW_MMU_PHYSICAL_DESC  pphydescData);

/*********************************************************************************************************
  �����ڴ�ȱҳ���Ƽ��
*********************************************************************************************************/

#if LW_CFG_THREAD_DEL_EN > 0
VOID            __vmmPhysicalPageFaultClear(LW_OBJECT_HANDLE  ulId);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

INT             __vmmPhysicalPageFaultLimit(PLW_VMM_PAGE_FAULT_LIMIT  pvpflNew,
                                            PLW_VMM_PAGE_FAULT_LIMIT  pvpflOld);
BOOL            __vmmPhysicalPageFaultCheck(ULONG  ulPageNum,
                                            PLW_CLASS_TCB  ptcbCur, LW_OBJECT_HANDLE  *pulWarn);
INT             __vmmPhysicalPageFaultGuarder(LW_OBJECT_HANDLE  ulGuarder);

#endif                                                                  /*  __PHYPAGE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
