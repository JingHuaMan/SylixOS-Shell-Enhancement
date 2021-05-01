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
** ��   ��   ��: vmmArea.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ��ַ�ռ����. �ṩ��ַ���鹦��.
                 ��ַ����ʹ�ù�ϣ�����.
*********************************************************************************************************/

#ifndef __VMMAREA_H
#define __VMMAREA_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  ��ַ�ռ�
*********************************************************************************************************/

typedef struct __vmm_area {
    PLW_TREE_RB_ROOT     AREA_ptrbrHash;                                /*  hash rb tree ��             */
    ULONG                AREA_ulHashSize;                               /*  hash ���С                 */
    addr_t               AREA_ulAreaAddr;                               /*  �ռ���ʼ��ַ                */
    size_t               AREA_stAreaSize;                               /*  �ռ��С                    */
} LW_VMM_AREA;
typedef LW_VMM_AREA     *PLW_VMM_AREA;

/*********************************************************************************************************
  service
*********************************************************************************************************/
ULONG  __areaVirtualSpaceInit(LW_MMU_VIRTUAL_DESC   pvirdes[]);
ULONG  __areaPhysicalSpaceInit(LW_MMU_PHYSICAL_DESC  pphydesc[]);

VOID  __areaVirtualSpaceTraversal(VOIDFUNCPTR  pfuncCallback);
VOID  __areaPhysicalSpaceTraversal(ULONG  ulZoneIndex, VOIDFUNCPTR  pfuncCallback);

PLW_VMM_PAGE  __areaVirtualSearchPage(addr_t  ulAddr);
PLW_VMM_PAGE  __areaPhysicalSearchPage(ULONG  ulZoneIndex, addr_t  ulAddr);

VOID  __areaVirtualInsertPage(ULONG  ulAddr, PLW_VMM_PAGE  pvmpage);
VOID  __areaPhysicalInsertPage(ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage);

VOID  __areaVirtualUnlinkPage(ULONG  ulAddr, PLW_VMM_PAGE  pvmpage);
VOID  __areaPhysicalUnlinkPage(ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage);

#endif                                                                  /*  __VMM_H                     */
#endif                                                                  /*  __VMMAREA_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
