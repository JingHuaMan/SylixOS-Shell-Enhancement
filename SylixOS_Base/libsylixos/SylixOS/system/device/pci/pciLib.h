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
** ��   ��   ��: pciLib.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 05 �� 22 ��
**
** ��        ��: PCI ���������ڲ�����.
*********************************************************************************************************/

#ifndef __PCILIB_H
#define __PCILIB_H

#include "pciDev.h"
#include "pciDrv.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  ���������豸���ƿ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PDDCB_lineDrvDevNode;                       /* �豸�ڵ����                 */
    PCI_DEV_HANDLE          PDDCB_hDrvDevHandle;                        /* �豸���                     */
} PCI_DRV_DEV_CB;
typedef PCI_DRV_DEV_CB     *PCI_DRV_DEV_HANDLE;

/*********************************************************************************************************
  PCI ���������ڲ�ʹ��
*********************************************************************************************************/
LW_API CPCHAR               API_PciSizeNameGet(pci_size_t stSize);
LW_API pci_size_t           API_PciSizeNumGet(pci_size_t stSize);

LW_API INT                  API_PciSpecialCycle(INT iBus, UINT32 uiMsg);

LW_API INT                  API_PciTraversal(INT (*pfuncCall)(), PVOID pvArg, INT iMaxBusNum);
LW_API INT                  API_PciTraversalDev(INT iBusStart, BOOL bSubBus, INT (*pfuncCall)(), PVOID pvArg);

LW_API INT                  API_PciDevMemSizeGet(PCI_DEV_HANDLE   hDevHandle,
                                                 pci_size_t      *pstIoSize,
                                                 pci_size_t      *pstMemSize);
LW_API INT                  API_PciDevMemConfig(PCI_DEV_HANDLE  hDevHandle,
                                                ULONG ulIoBase, pci_addr_t ulMemBase,
                                                UINT8 ucLatency, UINT32 uiCommand);
LW_API INT                  API_PciFuncDisable(INT iBus, INT iSlot, INT iFunc);
                                 
LW_API INT                  API_PciInterConnect(ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr,
                                                PVOID pvArg, CPCHAR pcName);
LW_API INT                  API_PciInterDisconnect(ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr,
                                                   PVOID pvArg);

LW_API INT                  API_PciGetHeader(INT iBus, INT iSlot, INT iFunc, PCI_HDR *p_pcihdr);
LW_API INT                  API_PciHeaderTypeGet(INT iBus, INT iSlot, INT iFunc, UINT8 *ucType);

LW_API INT                  API_PciVpdRead(INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen);
LW_API INT                  API_PciIrqGet(INT iBus, INT iSlot, INT iFunc,
                                          INT iMsiEn, INT iLine, INT iPin, ULONG *pulVector);
                                                                        /*  ֻ������ INTx �жϻ�ȡ      */
LW_API INT                  API_PciIrqMsi(INT iBus, INT iSlot, INT iFunc, 
                                          INT iMsiEn, INT iLine, INT iPin, PCI_MSI_DESC *pmsidesc);
                                                                        /*  ֻ������ MSI MSI-X �жϻ�ȡ */
LW_API INT                  API_PciConfigFetch(INT iBus, INT iSlot, INT iFunc, UINT uiPos, UINT uiLen);
LW_API INT                  API_PciConfigBusMaxSet(INT iIndex, UINT32 uiBusMax);
LW_API INT                  API_PciConfigBusMaxGet(INT iIndex);

LW_API INT                  API_PciIntxEnableSet(INT iBus, INT iSlot, INT iFunc, INT iEnable);
LW_API INT                  API_PciIntxMaskSupported(INT iBus, INT iSlot, INT iFunc, INT *piSupported);

LW_API VOID                 API_PciDrvBindEachDev(PCI_DRV_HANDLE hDrvHandle);
LW_API INT                  API_PciDrvLoad(PCI_DRV_HANDLE       hDrvHandle,
                                           PCI_DEV_HANDLE       hDevHandle,
                                           PCI_DEV_ID_HANDLE    hIdEntry);
LW_API PCI_DRV_DEV_HANDLE   API_PciDrvDevFind(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDrvDevDel(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDrvDevAdd(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDrvInit(VOID);

LW_API INT                  API_PciDevInterVectorGet(PCI_DEV_HANDLE  hHandle, ULONG *pulVector);
                                                                        /*  ֻ�ܻ�ȡ INTx �ж�����      */
LW_API INT                  API_PciDevInterMsiGet(PCI_DEV_HANDLE  hHandle, PCI_MSI_DESC *pmsidesc);
                                                                        /*  ֻ�ܻ�ȡ MSI MSI-X �ж����� */
LW_API PCI_DEV_ID_HANDLE    API_PciDevMatchDrv(PCI_DEV_HANDLE hDevHandle, PCI_DRV_HANDLE hDrvHandle);
LW_API VOID                 API_PciDevBindEachDrv(PCI_DEV_HANDLE hDevHandle);

LW_API INT                  API_PciDevSetup(PCI_DEV_HANDLE hHandle);

LW_API PCI_DEV_HANDLE       API_PciDevAdd(INT iBus, INT iDevice, INT iFunction);
LW_API INT                  API_PciDevDelete(PCI_DEV_HANDLE hHandle);
LW_API INT                  API_PciDevDrvDel(PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_PciDevDrvUpdate(PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_PciDevListCreate(VOID);
LW_API INT                  API_PciDevInit(VOID);

LW_API INT                  API_PciDevMsiEnableGet(PCI_DEV_HANDLE  hHandle, INT *piEnable);

/*********************************************************************************************************
  �Զ�����
*********************************************************************************************************/
LW_API INT                  API_PciAutoBusReset(PCI_CTRL_HANDLE hCtrl);
LW_API INT                  API_PciAutoScan(PCI_CTRL_HANDLE hCtrl, UINT32 *puiSubBus);
LW_API INT                  API_PciAutoBusScan(PCI_CTRL_HANDLE hCtrl, INT iBus, UINT32 *puiSubBus);
LW_API INT                  API_PciAutoPostScanBridgeSetup(PCI_CTRL_HANDLE     hCtrl,
                                                           PCI_AUTO_DEV_HANDLE hAutoDev,
                                                           INT                 iSubBus);
LW_API INT                  API_PciAutoPreScanBridgeSetup(PCI_CTRL_HANDLE     hCtrl,
                                                          PCI_AUTO_DEV_HANDLE hAutoDev,
                                                          INT                 iSubBus);
LW_API INT                  API_PciAutoDeviceSetup(PCI_CTRL_HANDLE        hCtrl,
                                                   PCI_AUTO_DEV_HANDLE    hAutoDev,
                                                   UINT32                 uiBarNum,
                                                   PCI_AUTO_REGION_HANDLE hRegionIo,
                                                   PCI_AUTO_REGION_HANDLE hRegionMem,
                                                   PCI_AUTO_REGION_HANDLE hRegionPrefetch);
LW_API INT                  API_PciAutoRegionAllocate(PCI_AUTO_REGION_HANDLE hRegion,
                                                      pci_size_t             stSize,
                                                      pci_addr_t            *paddrBar);
LW_API VOID                 API_PciAutoRegionAlign(PCI_AUTO_REGION_HANDLE hRegion, pci_size_t stSize);


LW_API VOID                 API_PciAutoRegionInit(PCI_AUTO_REGION_HANDLE hRegion);

LW_API INT                  API_PciAutoDeviceConfig(PCI_CTRL_HANDLE     hCtrl,
                                                    PCI_AUTO_DEV_HANDLE hAutoDev,
                                                    UINT32             *puiSubBus);
LW_API INT                  API_PciAutoCtrlInit(PCI_CTRL_HANDLE hCtrl);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCILIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
