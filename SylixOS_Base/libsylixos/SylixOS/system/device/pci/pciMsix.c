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
** ��   ��   ��: pciMsix.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2017 �� 07 �� 26 ��
**
** ��        ��: PCI ���� MSI-X ����.

** BUG:
2019.02.22  �޸���ȡ��Դ��ַ���������. Gong.YuJian (�����)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/compat.h"
#include "pciLib.h"
/*********************************************************************************************************
** ��������: API_PciMsixMsgWrite
** ��������: д MSI-X ��Ϣ
** �䡡��  : pvAddr         �ж���Ŀ�׵�ַ
**           iVector        �жϺ�
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixMsgWrite (PVOID  pvAddr, UINT  iVector, PCI_MSI_MSG  *ppmmMsg)
{
    if (!pvAddr) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    writel(ppmmMsg->uiAddressLo, (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_LOWER_ADDR);
    writel(ppmmMsg->uiAddressHi, (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_UPPER_ADDR);
    writel(ppmmMsg->uiData,      (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_DATA);
    writel(0,                    (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_VECTOR_CTRL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixMsgRead
** ��������: �� MSI-X ��Ϣ
** �䡡��  : pvAddr         �ж���Ŀ�׵�ַ
**           iVector        �жϺ�
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixMsgRead (PVOID  pvAddr, UINT  iVector, PCI_MSI_MSG  *ppmmMsg)
{
    if (!pvAddr) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    ppmmMsg->uiAddressLo = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_LOWER_ADDR);
    ppmmMsg->uiAddressHi = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_UPPER_ADDR);
    ppmmMsg->uiData      = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_DATA);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixPendingPosGet
** ��������: ��ȡ MSI-X Pending λ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           ppaPos         ���λ��
**                          0  ��������Ч
**                          1  ��������Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, phys_addr_t  *ppaPos)
{
    INT                     iRet       = PX_ERROR;
    UINT32                  uiMsiTab   = 0;
    UINT32                  iBarIndex  = 0;
    UINT32                  iOffset    = 0;
    phys_addr_t             paBaseAddr = 0;
    ULONG                   ulFlags    = 0;
    PCI_DEV_HANDLE          hDevHandle = LW_NULL;
                                                                        /*  ��ȡ MSI-X PBA �ֶ�         */
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_PBA, &uiMsiTab);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iBarIndex  = uiMsiTab & PCI_MSIX_PBA_BIR;                           /* MSI-X PBA����BAR�ռ�����     */
    iOffset    = uiMsiTab & PCI_MSIX_PBA_OFFSET;                        /* MSI-X PBA����BAR�ռ���ƫ��   */

    hDevHandle = API_PciDevHandleGet(iBus, iSlot, iFunc);               /* ��ȡ�豸���                 */
    if (!hDevHandle) {
        return  (PX_ERROR);
    }

    ulFlags = PCI_DEV_RESOURCE_FLAG(hDevHandle, iBarIndex);             /* ��ȡ��Դ����                 */
    if (!ulFlags || (ulFlags & PCI_IORESOURCE_UNSET)){
        return  (PX_ERROR);
    }
    paBaseAddr = (phys_addr_t)PCI_DEV_RESOURCE_START(hDevHandle, iBarIndex);

    if (ppaPos) {
        *ppaPos = paBaseAddr + iOffset;                                 /* MSI-X PBA ��PCI�������ַ    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixPendingGet
** ��������: ��ȡ MSI-X Pending ״̬
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           iVector        �жϺ�
**           piPending      ��ȡ����״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixPendingGet (INT      iBus,
                            INT      iSlot,
                            INT      iFunc,
                            UINT32   uiMsixCapOft,
                            INT      iVector,
                            INT     *piPending)
{
    INT          iRet       = PX_ERROR;
    phys_addr_t  paBaseAddr = 0;
    INT          iEntry     = (iVector + 64) >> 6;                      /* ÿ����Ŀ���� 64 �� pending λ*/
    UINT64       ullPended  = 0;
    UINT64      *pullAddr   = LW_NULL;

                                                                        /*  ��ȡ Pending Table λ��     */
    iRet = API_PciMsixPendingPosGet(iBus, iSlot, iFunc, uiMsixCapOft, &paBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  ӳ��Pending Table���߼��ռ� */
    pullAddr = (UINT64 *)API_PciDevIoRemap2(paBaseAddr, iEntry *  sizeof(UINT64));
    if (pullAddr == LW_NULL) {
        return  (PX_ERROR);
    }

    ullPended = readq(pullAddr + (iVector >> 6));                       /*  ��ȡ�ж�������Ŀ            */
    if (piPending) {
        *piPending = !!(ullPended & (1 << (iVector & 0x3f)));           /*  ��ȡ�ж϶�ӦPendingλ       */
    }

    API_VmmIoUnmap((PVOID)pullAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixTablePosGet
** ��������: ��ȡ MSI-X Tableλ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           ppaMaskPos     MSI-X Tableλ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixTablePosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, phys_addr_t  *ppaTablePos)
{
    INT                     iRet       = PX_ERROR;
    UINT32                  uiMsiTab   = 0;
    UINT32                  iBarIndex  = 0;
    UINT32                  iOffset    = 0;
    phys_addr_t             paBaseAddr = 0;
    ULONG                   ulFlags    = 0;
    PCI_DEV_HANDLE          hDevHandle = LW_NULL;
                                                                        /*  ��ȡ MSI-X Table �ֶ�       */
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_TABLE, &uiMsiTab);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iBarIndex  = uiMsiTab & PCI_MSIX_TABLE_BIR;                         /* MSI-X Table����BAR�ռ�����   */
    iOffset    = uiMsiTab & PCI_MSIX_TABLE_OFFSET;                      /* MSI-X Table����BAR�ռ���ƫ�� */

    hDevHandle = API_PciDevHandleGet(iBus, iSlot, iFunc);               /* ��ȡ�豸���                 */
    if (!hDevHandle) {
        return  (PX_ERROR);
    }

    ulFlags = PCI_DEV_RESOURCE_FLAG(hDevHandle, iBarIndex);             /* ��ȡ��Դ����                 */
    if (!ulFlags || (ulFlags & PCI_IORESOURCE_UNSET)){
        return  (PX_ERROR);
    }
    paBaseAddr = (phys_addr_t)PCI_DEV_RESOURCE_START(hDevHandle, iBarIndex);

    if (ppaTablePos) {
        *ppaTablePos = paBaseAddr + iOffset;                            /* MSI-X Table ��PCI�������ַ  */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixMaskSet
** ��������: ���� MSI-X ����״̬
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           iVector        �жϺ�
**           piPending      ���õ�״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixMaskSet (INT     iBus,
                         INT     iSlot,
                         INT     iFunc,
                         UINT32  uiMsixCapOft,
                         INT     iVector,
                         INT     iMask)
{
    INT         iRet       = PX_ERROR;
    PVOID       pvAddr     = LW_NULL;
    phys_addr_t paBaseAddr = 0;
    UINT32      uiControl  = 0;
                                                                        /*  ��ȡ MSI-X Table λ��       */
    iRet = API_PciMsixTablePosGet(iBus, iSlot, iFunc, uiMsixCapOft, &paBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  ӳ�� MSI-X Tabel�������ַ  */
    pvAddr = API_PciDevIoRemap2(paBaseAddr, (iVector + 1) * 16);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡMSI-X Table�жϿ����ֶ� */
    uiControl  = readl((addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);
    uiControl &= ~PCI_MSIX_ENTRY_CTRL_MASKBIT;                          /*  �������ÿ����ֶ�            */
    if (iMask) {
        uiControl |= PCI_MSIX_ENTRY_CTRL_MASKBIT;
    }
                                                                        /*  ��дMSI-X Table�жϿ����ֶ� */
    writel(uiControl, (addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);

    API_VmmIoUnmap(pvAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixMaskGet
** ��������: ��ȡ MSI-X ����״̬
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           iVector        �жϺ�
**           piMask         ��ȡ����״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixMaskGet (INT      iBus,
                         INT      iSlot,
                         INT      iFunc,
                         UINT32   uiMsixCapOft,
                         INT      iVector,
                         INT     *piMask)
{
    INT         iRet       = PX_ERROR;
    PVOID       pvAddr     = LW_NULL;
    phys_addr_t paBaseAddr = 0;
    UINT32      uiControl  = 0;

                                                                        /*  ��ȡ MSI-X Table λ��       */
    iRet = API_PciMsixTablePosGet(iBus, iSlot, iFunc, uiMsixCapOft, &paBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X Table λ��       */
    pvAddr = API_PciDevIoRemap2(paBaseAddr, (iVector + 1) * PCI_MSIX_ENTRY_SIZE);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡMSI-X Table�жϿ����ֶ� */
    uiControl = readl((addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);
    if (piMask) {
        *piMask = uiControl & PCI_MSIX_ENTRY_CTRL_MASKBIT;
    }

    API_VmmIoUnmap(pvAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixVecCountGet
** ��������: ��ȡ MSI-X ������
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           puiVecCount    ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, UINT32  *puiVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X ����״̬�ֶ�     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiVecCount) {
        *puiVecCount = (usControl & PCI_MSIX_FLAGS_QSIZE) + 1;          /*  ��ȡ MSI-X ��������         */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsixFunctionMaskSet
** ��������: ���� MSI-X ȫ�����빦��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           iEnable        ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixFunctionMaskSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X ����״̬�ֶ�     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSIX_FLAGS_MASKALL));             /*  ���� MSI-X ȫ������λ       */
    if (iEnable) {
        usControlNew |= PCI_MSIX_FLAGS_MASKALL;
    }                                                                   /*  ���»�д MSI-X ����״̬�ֶ� */
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsixFunctionMaskGet
** ��������: ��ȡ MSI-X ȫ�����빦��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           iEnable        ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixFunctionMaskGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X ����״̬�ֶ�     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSIX_FLAGS_MASKALL);             /*  ��ȡȫ������λ              */
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableSet
** ��������: MSI-X ʹ�ܿ���
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           iEnable        ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixEnableSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X ����״̬�ֶ�     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSIX_FLAGS_ENABLE));              /*  ���� MSI-X ʹ��״̬λ       */
    if (iEnable) {
        usControlNew |= PCI_MSIX_FLAGS_ENABLE;
    }                                                                   /*  �������� MSI-X ����״̬�ֶ� */
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableGet
** ��������: ��ȡ MSI-X ʹ�ܿ���״̬
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           piEnable       ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixEnableGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  ��ȡ MSI-X ����״̬�ֶ�     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSIX_FLAGS_ENABLE);              /*  ��ȡʹ�ܿ���״̬λ          */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiRangeEnable
** ��������: ���� MSI-X ����ʹ��
** �䡡��  : hHandle        �豸���ƾ��
**           uiVecMin       ʹ�������ж�������Сֵ
**           uiVecMax       ʹ�������ж��������ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ÿ�� PCI �豸��������ֻ��Ҫ����һ��
**
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixRangeEnable (PCI_DEV_HANDLE       hHandle,
                                PCI_MSI_DESC_HANDLE  hMsgHandle,
                                UINT                 uiVecMin,
                                UINT                 uiVecMax)
{
    INT         i           = 0;
    INT         iRet        = PX_ERROR;
    INT         iEnable     = 0;
    UINT32      uiMsiCapOft = 0;
    UINT32      uiVecNum    = 0;
    phys_addr_t paBaseAddr;
    PVOID       pvAddr      = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if ((uiVecMax < uiVecMin) || (uiVecMin < 1)) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  ��ȡ MSI-X �ṹ��ƫ��       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableGet(hHandle->PCIDEV_iDevBus,                /* ��ȡ MSI-X ʹ�ܿ���״̬      */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                &iEnable);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixVecCountGet(hHandle->PCIDEV_iDevBus,              /*  ��ȡ MSI-X ������           */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft, &uiVecNum);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (uiVecNum < uiVecMin) {
        return  (PX_ERROR);
    
    } else if (uiVecNum > uiVecMax) {
        uiVecNum = uiVecMax;
    }

    iRet = API_PciMsixTablePosGet(hHandle->PCIDEV_iDevBus,              /*  ��ȡ MSI-X Table λ��       */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft,
                                  &paBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  ӳ�� MSI-X Tabel�������ַ  */
    pvAddr = API_PciDevIoRemap2(paBaseAddr, uiVecNum * PCI_MSIX_ENTRY_SIZE);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }

    for (i = 0; i < uiVecNum; i++) {                                    /*  ѭ������ÿ�� MSI-X �ж�     */
        hMsgHandle[i].PCIMSI_uiNum = 1;                                 /*  ��ϵͳ����һ�� MSI �ж�     */
        
        iRet = API_PciDevInterMsiGet(hHandle, &hMsgHandle[i]);
        if (iRet != ERROR_NONE) {
            break;
        }
                                                                        /*  д MSI-X ��Ϣ               */
        iRet = API_PciMsixMsgWrite(pvAddr, i, &hMsgHandle[i].PCIMSI_pmmMsg);
        if (iRet != ERROR_NONE) {
            break;
        }

        hHandle->PCIDEV_uiDevIrqMsiNum++;                               /*  ���� MSI-X ����             */
    }

    API_VmmIoUnmap(pvAddr);                                             /*  �ͷ�ռ�õ��߼��ռ�          */

    if (i >= uiVecMin) {
        return  (ERROR_NONE);

    } else {
        return  (iRet);                                                 /*  TODO: ������Ҫ�����ж�����  */
    }
}
/*********************************************************************************************************
** ��������: API_PciDevMsixVecCountGet
** ��������: ��ȡ MSI ������
** �䡡��  : hHandle        �豸���ƾ��
**           puiVecCount    ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixVecCountGet (PCI_DEV_HANDLE  hHandle, UINT32  *puiVecCount)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  ��ȡ MSI-X �ṹ��ƫ��       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixVecCountGet(hHandle->PCIDEV_iDevBus,              /*  ��ȡ MSI-X ������           */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft, puiVecCount);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMsixEnableGet
** ��������: ��ȡ MSI-X ʹ�ܿ���״̬
** �䡡��  : hHandle        �豸���ƾ��
**           piEnable       ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixEnableGet (PCI_DEV_HANDLE  hHandle, INT  *piEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  ��ȡ MSI-X �ṹ��ƫ��       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableGet(hHandle->PCIDEV_iDevBus,                /*  ��ȡ MSI-X ʹ�ܿ���״̬     */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                piEnable);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiEnableSet
** ��������: ���� MSI-X ʹ�ܿ���״̬
**           ע�⣺����ʹ��ʱ��Ĭ�����ý���MSI��INTx״̬�����壻����ʧ��ʱ����MSI��INTx����������
** �䡡��  : hHandle        �豸���ƾ��
**           iEnable        ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (iEnable) {
        API_PciDevMsiEnableSet(hHandle, 0);                             /*  ���� MSI                    */
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  ��ȡ MSI-X �ṹ��ƫ��       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableSet(hHandle->PCIDEV_iDevBus,                /*  ���� MSI-X ʹ�ܿ���         */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                iEnable);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    hHandle->PCIDEV_iDevIrqMsiEn = iEnable;

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
