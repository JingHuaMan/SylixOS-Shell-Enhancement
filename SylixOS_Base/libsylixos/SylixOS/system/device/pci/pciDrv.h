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
** ��   ��   ��: pciDrv.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 12 �� 23 ��
**
** ��        ��: PCI �����豸��������.
*********************************************************************************************************/

#ifndef __PCIDRV_H
#define __PCIDRV_H

#include "pciPm.h"
#include "pciDev.h"
#include "pciError.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define PCI_DRV_NAME_MAX    (24 + 1)                                    /*  �����������ֵ              */

/*********************************************************************************************************
  ����֧���豸�б���ƿ�
*********************************************************************************************************/
typedef struct {
    UINT32                  PCIDEVID_uiVendor;                          /* ���� ID                      */
    UINT32                  PCIDEVID_uiDevice;                          /* �豸 ID                      */

    UINT32                  PCIDEVID_uiSubVendor;                       /* �ӳ��� ID                    */
    UINT32                  PCIDEVID_uiSubDevice;                       /* ���豸 ID                    */

    UINT32                  PCIDEVID_uiClass;                           /* �豸��                       */
    UINT32                  PCIDEVID_uiClassMask;                       /* �豸����                     */

    ULONG                   PCIDEVID_ulData;                            /* �豸˽������                 */
} PCI_DEV_ID_CB;
typedef PCI_DEV_ID_CB      *PCI_DEV_ID_HANDLE;

/*********************************************************************************************************
  �������ƿ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PCIDRV_lineDrvNode;                         /* �����ڵ����                 */
    CHAR                    PCIDRV_cDrvName[PCI_DRV_NAME_MAX];          /* ��������                     */
    PVOID                   PCIDRV_pvPriv;                              /* ˽������                     */
    PCI_DEV_ID_HANDLE       PCIDRV_hDrvIdTable;                         /* �豸֧���б�                 */
    UINT32                  PCIDRV_uiDrvIdTableSize;                    /* �豸֧���б��С             */

    /*
     *  �������ú���, PCIDRV_pfuncDevProbe �� PCIDRV_pfuncDevRemove ����Ϊ LW_NULL, ������ѡ
     */
    INT   (*PCIDRV_pfuncDevProbe)(PCI_DEV_HANDLE hHandle, const PCI_DEV_ID_HANDLE hIdEntry);
    VOID  (*PCIDRV_pfuncDevRemove)(PCI_DEV_HANDLE hHandle);
    INT   (*PCIDRV_pfuncDevSuspend)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PCIDRV_pfuncDevSuspendLate)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PCIDRV_pfuncDevResumeEarly)(PCI_DEV_HANDLE hHandle);
    INT   (*PCIDRV_pfuncDevResume)(PCI_DEV_HANDLE hHandle);
    VOID  (*PCIDRV_pfuncDevShutdown)(PCI_DEV_HANDLE hHandle);

    PCI_ERROR_HANDLE        PCIDRV_hDrvErrHandler;                      /* ��������                 */

    INT                     PCIDRV_iDrvFlag;                            /* ������־                     */
    UINT32                  PCIDRV_uiDrvDevNum;                         /* �����豸��                   */
    LW_LIST_LINE_HEADER     PCIDRV_plineDrvDevHeader;                   /* �豸��������ͷ               */
} PCI_DRV_CB;
typedef PCI_DRV_CB         *PCI_DRV_HANDLE;

/*********************************************************************************************************
  API
  API_PciConfigInit() ������ BSP ��ʼ������ϵͳʱ������, ���ұ��뱣֤�ǵ�һ������ȷ���õ� PCI ϵͳ����.
*********************************************************************************************************/

LW_API PCI_DRV_HANDLE       API_PciDrvHandleGet(CPCHAR pcName);
LW_API INT                  API_PciDrvRegister(PCI_DRV_HANDLE hHandle);
LW_API INT                  API_PciDrvUnregister(PCI_DRV_HANDLE hDrvHandle);

/*********************************************************************************************************
  API Macro
*********************************************************************************************************/

#define pciDrvHandleGet     API_PciDrvHandleGet
#define pciDrvRegister      API_PciDrvRegister
#define pciDrvUnregister    API_PciDrvUnregister

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIDRV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
