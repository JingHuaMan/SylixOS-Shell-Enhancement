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
** ��   ��   ��: virPage.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: �����ڴ����.

** BUG:
2013.05.24  ��������ռ���뿪��.
*********************************************************************************************************/

#ifndef __VIRPAGE_H
#define __VIRPAGE_H

/*********************************************************************************************************
  ����ռ����
*********************************************************************************************************/

PLW_MMU_VIRTUAL_DESC    __vmmVirtualDesc(UINT32  uiType, ULONG  ulZoneIndex, ULONG  *pulFreePage);
addr_t                  __vmmVirtualSwitch(VOID);

BOOL                    __vmmVirtualIsInside(addr_t  ulAddr);
ULONG                   __vmmVirtualCreate(LW_MMU_VIRTUAL_DESC   pvirdes[]);

PLW_VMM_PAGE            __vmmVirtualPageAlloc(ULONG  ulPageNum);
PLW_VMM_PAGE            __vmmVirDevPageAlloc(ULONG  ulPageNum);

PLW_VMM_PAGE            __vmmVirtualPageAllocAlign(ULONG  ulPageNum, size_t  stAlign);
PLW_VMM_PAGE            __vmmVirDevPageAllocAlign(ULONG  ulPageNum, size_t  stAlign);

VOID                    __vmmVirtualPageFree(PLW_VMM_PAGE  pvmpage);
VOID                    __vmmVirDevPageFree(PLW_VMM_PAGE  pvmpage);

#endif                                                                  /*  __VIRPAGE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
