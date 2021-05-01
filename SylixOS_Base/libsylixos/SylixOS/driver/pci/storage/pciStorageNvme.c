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
** ��   ��   ��: pciStorageNvme.c
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ����.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_NVME_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_NVME_EN > 0) && (LW_CFG_DRV_M2_NVME > 0)
#include "pci_ids.h"
#include "linux/compat.h"
#include "pciStorageNvme.h"
/*********************************************************************************************************
  ����֧�ֵ��豸 ID ��, �����������豸�����Զ�ƥ��.
*********************************************************************************************************/
static const PCI_DEV_ID_CB      pciStorageNvmeIdTbl[] = {
    { PCI_VDEVICE(INTEL, 0x0953), NVME_QUIRK_STRIPE_SIZE },
    { PCI_VDEVICE(INTEL, 0x5845), NVME_QUIRK_IDENTIFY_CNS },
    { PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_NVM_EXPRESS, 0xffffff) },
    { PCI_DEVICE(PCI_VENDOR_ID_APPLE, 0x2001) },                        /*  iPhone6S/7 ...              */
    { 0, }
};
/*********************************************************************************************************
  NVMe ����������
*********************************************************************************************************/
static UINT     pciStorageNvmeCtrlNum = 0;
/*********************************************************************************************************
** ��������: pciStorageNvmeHeaderQuirk
** ��������: �����豸ͷ������⴦��
** �䡡��  : hDevHandle         PCI �豸���ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeHeaderQuirk (PCI_DEV_HANDLE  hPciDevHandle)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciNvmeDevIdTblGet
** ��������: ��ȡ�豸�б�
** �䡡��  : phPciDevId     �豸 ID �б���������
**           puiSzie        �豸�б��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciNvmeDevIdTblGet (PCI_DEV_ID_HANDLE  *phPciDevId, UINT32  *puiSzie)
{
    if ((!phPciDevId) || (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageNvmeIdTbl;
    *puiSzie    = sizeof(pciStorageNvmeIdTbl) / sizeof(PCI_DEV_ID_CB);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeDevRemove
** ��������: �Ƴ� NVMe �豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  pciStorageNvmeDevRemove (PCI_DEV_HANDLE  hHandle)
{
}
/*********************************************************************************************************
** ��������: pciStorageNvmeDevProbe
** ��������: NVMe ����̽�⵽�豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
**           hIdEntry           ƥ��ɹ����豸 ID ��Ŀ(�������豸 ID ��)
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeDevProbe (PCI_DEV_HANDLE  hPciDevHandle, const PCI_DEV_ID_HANDLE  hIdEntry)
{
    INT                     iRet      = PX_ERROR;
    NVME_CTRL_HANDLE        hNvmeCtrl = LW_NULL;
    UINT16                  usCap;

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    pciStorageNvmeHeaderQuirk(hPciDevHandle);                           /* �����豸ͷ������⴦��       */
                                                                        /* ȷ���豸����                 */
    iRet = API_PciDevConfigRead(hPciDevHandle, PCI_CLASS_DEVICE, (UINT8 *)&usCap, 2);
    if ((iRet != ERROR_NONE)             ||
        ((usCap != PCI_CLASS_STORAGE_NVM) &&
         (usCap != PCI_CLASS_STORAGE_NVM_EXPRESS))) {                   /* �豸���ʹ���                 */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  �����豸����, �豸�����汾��Ϣ����ǰ�����豸��������
     */
    hPciDevHandle->PCIDEV_uiDevVersion = NVME_PCI_DRV_VER_NUM;          /*  ��ǰ�豸�����汾��          */
    hPciDevHandle->PCIDEV_uiUnitNumber = pciStorageNvmeCtrlNum;         /*  �����豸����                */
    pciStorageNvmeCtrlNum++;

    hNvmeCtrl = API_NvmeCtrlCreate(NVME_PCI_DRV_NAME,
                                   hPciDevHandle->PCIDEV_uiUnitNumber,
                                   (PVOID)hPciDevHandle,
                                   hIdEntry->PCIDEVID_ulData);
    if (!hNvmeCtrl) {
        pciStorageNvmeCtrlNum--;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeCtrlOpt
** ��������: ������ص�ѡ�����
** �䡡��  : hCtrl      ���������
**           uiDrive    ����������
**           iCmd       ���� (NVME_OPT_CMD_xxxx)
**           lArg       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeCtrlOpt (NVME_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iCmd, LONG  lArg)
{
    if (!hCtrl) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case NVME_OPT_CMD_DC_MSG_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = NVME_DRIVE_DISKCACHE_MSG_COUNT;
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeVendorCtrlIntEnable
** ��������: ʹ�ܿ������ж�
** �䡡��  : hCtrl         ���������
**           hQueue        �������
**           pfuncIsr      �жϷ�����
**           cpcName       �жϷ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntEnable (NVME_CTRL_HANDLE   hCtrl,
                                               NVME_QUEUE_HANDLE  hQueue,
                                               PINT_SVR_ROUTINE   pfuncIsr,
                                               CPCHAR             cpcName)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X �ж���Ϣ�����ڿ������ṹ����
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterEnable(hPciDev,
                              hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                              (PINT_SVR_ROUTINE)pfuncIsr,
                              (PVOID)hQueue);
    } else {
        /*
         *  ������ INTx �жϻ� MSI �жϣ�INTx �ж�ֻ��Ϊ 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterEnable(hPciDev,
                              hPciDev->PCIDEV_ulDevIrqVector + usVector,
                              (PINT_SVR_ROUTINE)pfuncIsr,
                              (PVOID)hQueue);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeVendorCtrlIntConnect
** ��������: ���ӿ������ж�
** �䡡��  : hCtrl      ���������
**           hQueue     �������
**           pfuncIsr   �жϷ�����
**           cpcName    �жϷ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntConnect (NVME_CTRL_HANDLE   hCtrl,
                                                NVME_QUEUE_HANDLE  hQueue,
                                                PINT_SVR_ROUTINE   pfuncIsr,
                                                CPCHAR             cpcName)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X �ж���Ϣ�����ڿ������ṹ����
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterConnect(hPciDev,
                               hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                               (PINT_SVR_ROUTINE)pfuncIsr,
                               hQueue,
                               cpcName);
    } else {
        /*
         *  ������ INTx �жϻ� MSI �жϣ�INTx �ж�ֻ��Ϊ 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterConnect(hPciDev,
                               hPciDev->PCIDEV_ulDevIrqVector + usVector,
                               (PINT_SVR_ROUTINE)pfuncIsr,
                               hQueue,
                               cpcName);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeVendorCtrlIntDisConnect
** ��������: �ͷſ������ж�
** �䡡��  : hCtrl      ���������
**           hQueue     �������
**           pfuncIsr   �жϷ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntDisConnect (NVME_CTRL_HANDLE   hCtrl,
                                                   NVME_QUEUE_HANDLE  hQueue,
                                                   PINT_SVR_ROUTINE   pfuncIsr)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X �ж���Ϣ�����ڿ������ṹ����
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterDisconnect(hPciDev,
                                  hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                                  (PINT_SVR_ROUTINE)pfuncIsr,
                                  hQueue);
    } else {
        /*
         *  ������ INTx �жϻ� MSI �жϣ�INTx �ж�ֻ��Ϊ 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterDisconnect(hPciDev,
                                  hPciDev->PCIDEV_ulDevIrqVector + usVector,
                                  (PINT_SVR_ROUTINE)pfuncIsr,
                                  hQueue);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeVendorCtrlReadyWork
** ��������: ������׼������
** �䡡��  : hCtrl    ���������
**           uiIrqNum �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlReadyWork (NVME_CTRL_HANDLE  hCtrl, UINT uiIrqNum)
{
    INT                     iRet;
    PCI_DEV_HANDLE          hPciDev;
    UINT16                  usPciDevId;
    phys_addr_t             paBaseAddr;
    PCI_RESOURCE_HANDLE     hResource;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;                    /* ��ȡ�豸���                 */

    API_PciDevConfigReadWord(hPciDev, PCI_DEVICE_ID, &usPciDevId);
    NVME_LOG(NVME_LOG_PRT, "ctrl name %s index %d unit %d for pci dev %d:%d.%d dev id 0x%04x.\r\n",
             hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiIndex, hCtrl->NVMECTRL_uiUnitIndex,
             hPciDev->PCIDEV_iDevBus,
             hPciDev->PCIDEV_iDevDevice,
             hPciDev->PCIDEV_iDevFunction, usPciDevId);
                                                                        /* ���Ҷ�Ӧ��Դ��Ϣ             */
    hResource = API_PciDevStdResourceGet(hPciDev, PCI_IORESOURCE_MEM, PCI_BAR_INDEX_0);
    if (!hResource) {
        NVME_LOG(NVME_LOG_ERR, "pci BAR index %d error.\r\n", PCI_BAR_INDEX_0);
        return  (PX_ERROR);
    }

    paBaseAddr                = (phys_addr_t)(PCI_RESOURCE_START(hResource));
    hCtrl->NVMECTRL_stRegSize = (size_t)(PCI_RESOURCE_SIZE(hResource));
    hCtrl->NVMECTRL_pvRegAddr = API_PciDevIoRemap2(paBaseAddr, hCtrl->NVMECTRL_stRegSize);
    if (hCtrl->NVMECTRL_pvRegAddr == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "pci mem resource ioremap failed addr 0x%llx 0x%llx.\r\n",
                 hCtrl->NVMECTRL_pvRegAddr,  hCtrl->NVMECTRL_stRegSize);
        return  (PX_ERROR);
    }
    NVME_LOG(NVME_LOG_PRT, "nvme reg addr 0x%llx szie %llx.\r\n",
             hCtrl->NVMECTRL_pvRegAddr, hCtrl->NVMECTRL_stRegSize);

    iRet = API_PciDevMasterEnable(hPciDev, LW_TRUE);                    /*  ʹ�� Master ģʽ            */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "%s pci master enable failed.\r\n", hPciDev->PCIDEV_cDevName);
        return  (PX_ERROR);
    }

    iRet = API_PciDevMsixEnableSet(hPciDev, LW_TRUE);                   /*  ʹ�� MSI-X �ж�             */
    if (iRet != ERROR_NONE) {
        goto    __msi_handle;
    }

    /*
     *  ����MSI-X������
     */
    hCtrl->NVMECTRL_pvIntHandle = __SHEAP_ZALLOC(sizeof(PCI_MSI_DESC) * uiIrqNum);
    if (!hCtrl->NVMECTRL_pvIntHandle) {                                 /* ����������ʧ��               */
        goto    __msi_handle;
    }

    iRet = API_PciDevMsixRangeEnable(hPciDev, (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle,
                                     1, uiIrqNum);
    if (iRet != ERROR_NONE) {
        __SHEAP_FREE(hCtrl->NVMECTRL_pvIntHandle);
        goto    __msi_handle;
    }
    hCtrl->NVMECTRL_uiIntNum = hPciDev->PCIDEV_uiDevIrqMsiNum;
    hCtrl->NVMECTRL_bMsix    = LW_TRUE;

    return  (ERROR_NONE);

__msi_handle:
    iRet = API_PciDevMsixEnableSet(hPciDev, LW_FALSE);                  /*  ���ܻ�ʧ�ܣ���������        */
    iRet = API_PciDevMsiEnableSet(hPciDev, LW_TRUE);                    /*  ʹ�� MSI �ж�               */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "pci msi enable failed dev %d:%d.%d.\r\n",
                 hPciDev->PCIDEV_iDevBus, hPciDev->PCIDEV_iDevDevice, hPciDev->PCIDEV_iDevFunction);
        goto    __intx_handle;
    }

    iRet = API_PciDevMsiRangeEnable(hPciDev, 1, uiIrqNum);
    if (iRet != ERROR_NONE) {
        goto    __intx_handle;
    }
    hCtrl->NVMECTRL_uiIntNum = hPciDev->PCIDEV_uiDevIrqMsiNum;
    hCtrl->NVMECTRL_bMsix    = LW_FALSE;

    return  (ERROR_NONE);

__intx_handle:
    /*
     *  �� SylixOS ��֧�ֵ�ǰ�������ܹ��µ� Msi �� Msi-X �ж�ģʽ����֧�� INTx �жϣ�
     *  �� NVMe �豸�������֧�� INTx �ж�ģʽ�������豸ʱ��ȷ�ϡ�
     *  ��ʱ�� Msi �� Msi-X �жϵĴ���᷵�ش��󣬵������� INTx �ж�ģʽ������������
     */
    iRet = API_PciDevMsiEnableSet(hPciDev, LW_FALSE);                   /* ���ܻ�ʧ�ܣ���������         */
    hCtrl->NVMECTRL_uiIntNum = 1;
    hCtrl->NVMECTRL_bMsix    = LW_FALSE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageNvmeVendorDrvReadyWork
** ��������: ����ע��ǰ׼������
** �䡡��  : hDrv      �������ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorDrvReadyWork (NVME_DRV_HANDLE  hDrv)
{
    INT                 iRet;
    PCI_DRV_CB          tPciDrv;
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));
    iRet = pciNvmeDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], NVME_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = (PVOID)hDrv;
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageNvmeDevProbe;
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageNvmeDevRemove;

    iRet = API_PciDrvRegister(hPciDrv);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciNvmeInit
** ��������: PCI ���Ϳ�����������س�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciStorageNvmeInit (VOID)
{
    NVME_DRV_CB         tDrvReg;
    NVME_DRV_HANDLE     hDrvReg = &tDrvReg;

    API_NvmeDrvInit();

    lib_bzero(hDrvReg, sizeof(NVME_DRV_CB));
    lib_strlcpy(&hDrvReg->NVMEDRV_cDrvName[0], NVME_PCI_DRV_NAME, NVME_DRV_NAME_MAX);

    hDrvReg->NVMEDRV_uiDrvVer                      = NVME_PCI_DRV_VER_NUM;
    hDrvReg->NVMEDRV_hCtrl                         = LW_NULL;
    hDrvReg->NVMEDRV_pfuncOptCtrl                  = pciStorageNvmeCtrlOpt;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntEnable      = pciStorageNvmeVendorCtrlIntEnable;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntConnect     = pciStorageNvmeVendorCtrlIntConnect;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntDisConnect  = pciStorageNvmeVendorCtrlIntDisConnect;
    hDrvReg->NVMEDRV_pfuncVendorCtrlReadyWork      = pciStorageNvmeVendorCtrlReadyWork;
    hDrvReg->NVMEDRV_pfuncVendorDrvReadyWork       = pciStorageNvmeVendorDrvReadyWork;

    API_NvmeDrvRegister(hDrvReg);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
                                                                        /*  (LW_CFG_DRV_M2_NVME > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
