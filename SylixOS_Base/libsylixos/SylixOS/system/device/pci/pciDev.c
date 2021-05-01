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
** ��   ��   ��: pciDev.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 12 �� 23 ��
**
** ��        ��: PCI �����豸����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "endian.h"
#include "pciDev.h"
#include "pciDrv.h"
#include "pciLib.h"
/*********************************************************************************************************
  ���� (����֮ǰ��ƴд����)
*********************************************************************************************************/
#ifndef LW_CFG_CPU_ARCH_C6X
#include "sys/cdefs.h"
__weak_alias(API_PciDevInterDisonnect, API_PciDevInterDisconnect)
#endif                                                                  /*  !LW_CFG_CPU_ARCH_C6X        */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                     _GuiPciDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulPciDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplinePciDevHeader = LW_NULL;
/*********************************************************************************************************
  PCI �豸��
*********************************************************************************************************/
#define __PCI_DEV_LOCK()        API_SemaphoreMPend(_GulPciDevLock, LW_OPTION_WAIT_INFINITE)
#define __PCI_DEV_UNLOCK()      API_SemaphoreMPost(_GulPciDevLock)
/*********************************************************************************************************
** ��������: API_PciDevIntxEnableSet
** ��������: �豸 INTx ʹ�������
** �䡡��  : hHandle    �豸���
**           iEnable    ʹ�ܱ�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevIntxEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                iEnable);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciDevInterDisable
** ��������: ���� PCI �豸�ж�
** �䡡��  : hHandle        �豸���
**           ulVector       �ж�����
**           pfuncIsr       �жϷ�����
**           pvArg          �жϷ���������
**           iMaxServCnt    ������������ > ��ֵʱ, ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisableEx (PCI_DEV_HANDLE   hHandle,
                               ULONG            ulVector,
                               PINT_SVR_ROUTINE pfuncIsr,
                               PVOID            pvArg,
                               INT              iMaxServCnt)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorDisableEx(ulVector, iMaxServCnt)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterDisable
** ��������: ���� PCI �豸�ж�
** �䡡��  : hHandle    �豸���
**           ulVector   �ж�����
**           pfuncIsr   �жϷ�����
**           pvArg      �жϷ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisable (PCI_DEV_HANDLE   hHandle,
                             ULONG            ulVector,
                             PINT_SVR_ROUTINE pfuncIsr,
                             PVOID            pvArg)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorDisable(ulVector)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterEnable
** ��������: ʹ�� PCI �豸�ж�
** �䡡��  : hHandle    �豸���
**           ulVector   �ж�����
**           pfuncIsr   �жϷ�����
**           pvArg      �жϷ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterEnable (PCI_DEV_HANDLE   hHandle,
                            ULONG            ulVector,
                            PINT_SVR_ROUTINE pfuncIsr,
                            PVOID            pvArg)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorEnable(ulVector)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterDisconnect
** ��������: ���� PCI �豸����ж�����
** �䡡��  : hHandle    �豸���
**           ulVector   �ж�����
**           pfuncIsr   �жϷ�����
**           pvArg      �жϷ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisconnect (PCI_DEV_HANDLE    hHandle,
                                ULONG             ulVector,
                                PINT_SVR_ROUTINE  pfuncIsr,
                                PVOID             pvArg)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterDisconnect(ulVector, pfuncIsr, pvArg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bzero(hHandle->PCIDEV_cDevIrqName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PCIDEV_pfuncDevIrqHandle = LW_NULL;
    hHandle->PCIDEV_pvDevIrqArg = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterConnect
** ��������: ���� PCI �豸�ж�����
** �䡡��  : hHandle    �豸���
**           ulVector   �ж�����
**           pfuncIsr   �жϷ�����
**           pvArg      �жϷ���������
**           pcName     �жϷ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterConnect (PCI_DEV_HANDLE    hHandle,
                             ULONG             ulVector,
                             PINT_SVR_ROUTINE  pfuncIsr,
                             PVOID             pvArg,
                             CPCHAR            pcName)
{
    INT     iRet = PX_ERROR;

    if ((hHandle  == LW_NULL) ||
        (pfuncIsr == LW_NULL)) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterConnect(ulVector, pfuncIsr, pvArg, pcName);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(hHandle->PCIDEV_cDevIrqName, pcName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PCIDEV_pfuncDevIrqHandle = pfuncIsr;
    hHandle->PCIDEV_pvDevIrqArg       = pvArg;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterServiceCnt
** ��������: ��ȡ PCI �豸�ж���������������
** �䡡��  : hHandle    �豸���
**           ulVector   �ж�����
**           piCnt      �жϷ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterServiceCnt (PCI_DEV_HANDLE    hHandle,
                                ULONG             ulVector,
                                INT              *piCnt)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorServiceCnt(ulVector, piCnt)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInterVectorGet
** ��������: ��ȡ INTx �ж�����
** �䡡��  : hHandle        �豸���
**           pulVector      �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterVectorGet (PCI_DEV_HANDLE  hHandle, ULONG *pulVector)
{
    INT     iRet     = PX_ERROR;
    INT     iLine    = 0;
    INT     iPin     = 0;
    INT     iHdrType = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iHdrType = hHandle->PCIDEV_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (iHdrType) {

    case PCI_HEADER_TYPE_NORMAL:
        iLine = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        iPin  = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        return  (PX_ERROR);

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciIrqGet(hHandle->PCIDEV_iDevBus,
                         hHandle->PCIDEV_iDevDevice,
                         hHandle->PCIDEV_iDevFunction,
                         hHandle->PCIDEV_iDevIrqMsiEn, iLine, iPin, pulVector);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciDevInterMsiGet
** ��������: ��ȡ MSI MSI-X �ж�����
** �䡡��  : hHandle        �豸���
**           pmsidesc       MSI �ж���Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInterMsiGet (PCI_DEV_HANDLE  hHandle, PCI_MSI_DESC *pmsidesc)
{
    INT     iRet     = PX_ERROR;
    INT     iLine    = 0;
    INT     iPin     = 0;
    INT     iHdrType = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iHdrType = hHandle->PCIDEV_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (iHdrType) {

    case PCI_HEADER_TYPE_NORMAL:
        iLine = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        iPin  = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        return  (PX_ERROR);

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciIrqMsi(hHandle->PCIDEV_iDevBus,
                         hHandle->PCIDEV_iDevDevice,
                         hHandle->PCIDEV_iDevFunction,
                         hHandle->PCIDEV_iDevIrqMsiEn, iLine, iPin, pmsidesc);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __pciDevConfigBlockOp
** ��������: ������� PCI �豸���ÿռ�
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pucBuf     ���ݻ�����
**           uiLen      ���ݳ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciDevConfigBlockOp (PCI_DEV_HANDLE  hHandle, INT  uiPos, UINT8 *pucBuf, INT  uiLen,
                                   INT (*pfuncOpt)(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT8 *pucBuf, UINT uiLen))
{
    INT     iRet = PX_ERROR;

    if ((uiPos & 1) && uiLen >= 1) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 1);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos++; pucBuf++; uiLen--;
    }

    if ((uiPos & 3) && uiLen >= 2) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 2; pucBuf += 2; uiLen -= 2;
    }

    while (uiLen >= 4) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 4);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 4; pucBuf += 4; uiLen -= 4;
    }

    if (uiLen >= 2) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 2; pucBuf += 2; uiLen -= 2;
    }

    iRet = pfuncOpt(hHandle, uiPos, pucBuf, 1);
    if ((uiLen) &&
        (iRet != ERROR_NONE)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pciDevConfigBlockRead
** ��������: �����ȡ PCI �豸���ÿռ�
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pucBuf     ���ݻ�����
**           uiLen      ���ݳ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciDevConfigBlockRead (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    return  (__pciDevConfigBlockOp(hHandle, uiPos, pucBuf, uiLen, API_PciDevConfigRead));
}
/*********************************************************************************************************
** ��������: __pciDevConfigBlockWrite
** ��������: ����д�� PCI �豸���ÿռ�
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos     ƫ��λ��
**           pucBuf     ���ݻ�����
**           uiLen      ���ݳ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciDevConfigBlockWrite (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    return  (__pciDevConfigBlockOp(hHandle, uiPos, pucBuf, uiLen, API_PciDevConfigWrite));
}
/*********************************************************************************************************
** ��������: API_PciDevConfigRead
** ��������: ��ȡ PCI �豸���ÿռ�
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pucBuf     ���ݻ�����
**           uiLen      ���ݳ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigRead (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = (UINT8)PX_ERROR;
    UINT16      usData = (UINT16)PX_ERROR;
    UINT32      uiData = (UINT32)PX_ERROR;

    if (!((uiLen == 1) || (uiLen == 2) || (uiLen == 4))) {
        return  (__pciDevConfigBlockRead(hHandle, uiPos, pucBuf, uiLen));
    }

    if ((hHandle == LW_NULL) ||
        (pucBuf == LW_NULL) ||
        (uiPos >= PCI_CONFIG_LEN_MAX) ||
        ((uiPos + uiLen) > PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    switch (uiLen) {

    case 1:
        iRet = API_PciConfigInByte(hHandle->PCIDEV_iDevBus,
                                   hHandle->PCIDEV_iDevDevice,
                                   hHandle->PCIDEV_iDevFunction,
                                   uiPos, &ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        pucBuf[0] = (UINT8)ucData;
        break;

    case 2:
        iRet = API_PciConfigInWord(hHandle->PCIDEV_iDevBus,
                                   hHandle->PCIDEV_iDevDevice,
                                   hHandle->PCIDEV_iDevFunction,
                                   uiPos, &usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT16 *)pucBuf)[0] = usData;
        break;

    case 4:
        iRet = API_PciConfigInDword(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, &uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT32 *)pucBuf)[0] = uiData;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigWrite
** ��������: д�� PCI �豸���ÿռ�
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pucBuf     ���ݻ�����
**           uiLen       ���ݳ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWrite (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = (UINT8)PX_ERROR;
    UINT16      usData = (UINT16)PX_ERROR;
    UINT32      uiData = (UINT32)PX_ERROR;

    if (!((uiLen == 1) || (uiLen == 2) || (uiLen == 4))) {
        return  (__pciDevConfigBlockWrite(hHandle, uiPos, pucBuf, uiLen));
    }

    if ((hHandle == LW_NULL) ||
        (pucBuf == LW_NULL) ||
        (uiPos >= PCI_CONFIG_LEN_MAX) ||
        ((uiPos + uiLen) > PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    switch (uiLen) {

    case 1:
        ucData = (UINT8)pucBuf[0];
        iRet = API_PciConfigOutByte(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 2:
        usData = ((UINT16 *)pucBuf)[0];
        iRet = API_PciConfigOutWord(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 4:
        uiData = ((UINT32 *)pucBuf)[0];
        iRet = API_PciConfigOutDword(hHandle->PCIDEV_iDevBus,
                                     hHandle->PCIDEV_iDevDevice,
                                     hHandle->PCIDEV_iDevFunction,
                                     uiPos, uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigReadByte
** ��������: ���ֽڶ�ȡ PCI �豸���ÿռ� (8 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pucValue   ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadByte (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInByte(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiPos, pucValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigReadWord
** ��������: ���ֶ�ȡ PCI �豸���ÿռ� (16 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           pusValue   ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadWord (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT16 *pusValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiPos, pusValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigReadDword
** ��������: ��˫�ֶ�ȡ PCI �豸���ÿռ� (32 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           puiValue   ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadDword (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT32 *puiValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, puiValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigWriteByte
** ��������: ���ֽ�д PCI �豸���ÿռ� (8 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           ucValue    ���õ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteByte (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 ucValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutByte(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, ucValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigWriteWord
** ��������: ����д PCI �豸���ÿռ� (16 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           usValue    ���õ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteWord (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT16  usValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutWord(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, usValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevConfigWriteDword
** ��������: ��˫��д PCI �豸���ÿռ� (32 bit)
** �䡡��  : hHandle    �豸���ƾ��
**           uiPos      ƫ��λ��
**           uiValue    ���õ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteDword (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT32 uiValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(hHandle->PCIDEV_iDevBus,
                                 hHandle->PCIDEV_iDevDevice,
                                 hHandle->PCIDEV_iDevFunction,
                                 uiPos, uiValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPciCmdDevParent
** ��������: PCI ��������豸�����ڵ�
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciCmdDevParent (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                 iRet;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    INT                 iBus;
    INT                 iDevice;
    INT                 iFunction;

    if (iArgC == 2) {
        iRet = sscanf(ppcArgV[1], "%x:%x.%x", &iBus, &iDevice, &iFunction);
        if (iRet != 3) {
            fprintf(stderr, "pci device address format error.\n");
            goto  __error_handle;
        }

        hDevHandle = API_PciDevParentHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle != LW_NULL) {
            printf("pci dev parent: %x:%x.%x\n",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction);

            return  (ERROR_NONE);
        }
    }

__error_handle:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: __tshellPciDevAddDel
** ��������: ��ӻ�ɾ���豸
** �䡡��  : iAdd           �Ƿ�Ϊ��Ӳ���
**           iAll           �Ƿ�Ϊ����ȫ���豸
**           iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciDevAddDel (INT  iAdd, INT  iAll, INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    INT                 iRet = PX_ERROR;

    if ((iAdd == LW_TRUE) && (iAll == LW_TRUE)) {
        API_PciDevListCreate();
    
    } else if ((iAdd == LW_TRUE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevAdd(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
    }

    if ((iAdd == LW_FALSE) && (iAll == LW_TRUE)) {
        API_PciDevDelete(LW_NULL);
    
    } else if ((iAdd == LW_FALSE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
        iRet = API_PciDevDelete(hDevHandle);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "probe pci device error.\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __tshellPciCmdDevShow
** ��������: ��ӡ PCI ��ͨ�豸�б�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellPciCmdDevShow (VOID)
{
    static PCHAR        pcPciDevShowHdr = \
    " INDEX    TYPE    BUS   DEV  FUNC VENDOR DEVICE SUBV(PRI) SUBD(SEC) (SUB) LINE PIN  "
    "        DRVNAME\n"
    "------- -------- ----- ----- ---- ------ ------ --------- --------- ----- ---- ---- "
    "------------------------\n";

    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;
    REGISTER INT        i;

    printf("pci dev number total: %d\n", _GuiPciDevTotalNum);
    printf(pcPciDevShowHdr);

    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    i = 0;
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        hDrvHandle = (PCI_DRV_HANDLE)hDevHandle->PCIDEV_pvDevDriver;

        switch (hDevHandle->PCIDEV_iType) {

        case PCI_HEADER_TYPE_NORMAL:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x %-5s 0x%02x 0x%02x %-24s\n",
                   i,
                   "NORMAL",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId,
                   "",
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        case PCI_HEADER_TYPE_BRIDGE:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x 0x%03x 0x%02x 0x%02x %-24s\n",
                   i,
                   "BRIDGE",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucPriBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus,
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        case PCI_HEADER_TYPE_CARDBUS:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x 0x%03x 0x%02x 0x%02x %-24s\n",
                   i,
                   "CARDBUS",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucPriBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucSecBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucSubBus,
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        default:
            break;
        }

        i += 1;
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */
}
/*********************************************************************************************************
** ��������: __tshellPciCmdDev
** ��������: PCI ���� "pcidev"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciCmdDev (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iRet;
    INT     iBus;
    INT     iDevice;
    INT     iFunction;
    INT     iAddFlag = LW_FALSE;
    INT     iAllFlag = LW_FALSE;

    if (iArgC == 1) {
        __tshellPciCmdDevShow();
        return  (ERROR_NONE);
    }

    if ((lib_strcmp(ppcArgV[1], "add") == 0) && (iArgC == 3)) {
        iAddFlag = LW_TRUE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 3)) {
        iAddFlag = LW_FALSE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: API_PciDevSetupAll
** ��������: ���� PCI �����ϵ������豸
** �䡡��  : hDevHandle     �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevSetupAll (VOID)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    hDevHandle = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        __PCI_DEV_UNLOCK();                                            /*  ���� PCI ����               */

        API_PciDevSetup(hDevHandle);

        __PCI_DEV_LOCK();                                               /*  ���� PCI ����               */
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */


    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevParentHandleGet
** ��������: ��ȡһ���豸�ĸ��ڵ���
** �䡡��  : iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevParentHandleGet (INT  iBus, INT  iDevice, INT  iFunction)
{
    PLW_LIST_LINE       plineTemp      = LW_NULL;
    PCI_DEV_HANDLE      hDevHandleTemp = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle     = LW_NULL;
    UINT8               ucSecBus       = 0;
    UINT8               ucSubBus       = 0;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
    if (hDevHandle == LW_NULL) {
        return  (LW_NULL);
    }

    hDevHandleTemp = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandleTemp = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if (hDevHandleTemp->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) {
            ucSecBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus;
            ucSubBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus;

            if ((hDevHandle->PCIDEV_iType   == PCI_HEADER_TYPE_NORMAL) &&
                (hDevHandle->PCIDEV_iDevBus == ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus == ucSubBus)) {
                break;
            }

            if ((hDevHandle->PCIDEV_iType   == PCI_HEADER_TYPE_BRIDGE) &&
                (hDevHandle->PCIDEV_iDevBus >= ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus <= ucSubBus)) {
                break;
            }
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */

    if (plineTemp) {
        return  (hDevHandleTemp);
    }

    hDevHandleTemp = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandleTemp = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if (hDevHandleTemp->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) {
            ucSecBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus;
            ucSubBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus;

            if ((hDevHandle->PCIDEV_iType == PCI_HEADER_TYPE_NORMAL) &&
                (hDevHandle->PCIDEV_iDevBus >= ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus <= ucSubBus)) {
                break;
            }

            if ((hDevHandle->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) &&
                (hDevHandle->PCIDEV_iDevBus == hDevHandleTemp->PCIDEV_iDevBus)) {
                hDevHandleTemp = hDevHandle;
                break;
            }
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */

    if (plineTemp) {
        return  (hDevHandleTemp);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciDevHandleGet
** ��������: ��ȡһ���豸�ľ��
** �䡡��  : iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevHandleGet (INT  iBus, INT  iDevice, INT  iFunction)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    hDevHandle = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if ((hDevHandle->PCIDEV_iDevBus      == iBus     ) &&
            (hDevHandle->PCIDEV_iDevDevice   == iDevice  ) &&
            (hDevHandle->PCIDEV_iDevFunction == iFunction)) {
            break;
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */

    if (plineTemp) {
        return  (hDevHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_PciDevAdd
** ��������: ����һ���豸
** �䡡��  : iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevAdd (INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_HDR             phPciHdr;
    INT                 iType;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);         /* ��ȡ�豸���                 */
    if (hDevHandle != LW_NULL) {
        return  (hDevHandle);
    }

    lib_bzero(&phPciHdr, sizeof(PCI_HDR));
    API_PciGetHeader(iBus, iDevice, iFunction, &phPciHdr);              /* ��ȡ�豸ͷ��Ϣ               */
    iType = phPciHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;                /* ��ȡ�豸����                 */
    if ((iType != PCI_HEADER_TYPE_NORMAL) &&
        (iType != PCI_HEADER_TYPE_BRIDGE) &&
        (iType != PCI_HEADER_TYPE_CARDBUS)) {                           /* �豸���ʹ���                 */
        return  (LW_NULL);
    }

    hDevHandle = (PCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DEV_CB));    /* �����豸���ƿ�               */
    if (hDevHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    /*
     *  �����豸����
     */
    hDevHandle->PCIDEV_iDevBus      = iBus;
    hDevHandle->PCIDEV_iDevDevice   = iDevice;
    hDevHandle->PCIDEV_iDevFunction = iFunction;
    lib_memcpy(&hDevHandle->PCIDEV_phDevHdr, &phPciHdr, sizeof(PCI_HDR));

    switch (iType) {

    case PCI_HEADER_TYPE_NORMAL:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_NORMAL;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_BRIDGE;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntLine;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_CARDBUS;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntLine;
        break;

    default:
        __SHEAP_FREE(hDevHandle);
        return  (LW_NULL);
    }

    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    _List_Line_Add_Tail(&hDevHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
    _GuiPciDevTotalNum += 1;
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */

    return  (hDevHandle);
}
/*********************************************************************************************************
** ��������: API_PciDevDelete
** ��������: ɾ��һ�� PCI �豸
** �䡡��  : hHandle       �豸���ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevDelete (PCI_DEV_HANDLE  hHandle)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    if (hHandle == LW_NULL) {
        __PCI_DEV_LOCK();                                               /*  ���� PCI ����               */
        plineTemp = _GplinePciDevHeader;
        while (plineTemp) {
            hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
            plineTemp  = _list_line_get_next(plineTemp);
            
            _List_Line_Del(&hDevHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
            _GuiPciDevTotalNum -= 1;
            __SHEAP_FREE(hDevHandle);
        }
        __PCI_DEV_UNLOCK();                                             /*  ���� PCI ����               */

        return  (ERROR_NONE);
    }

    hDevHandle = API_PciDevHandleGet(hHandle->PCIDEV_iDevBus,
                                     hHandle->PCIDEV_iDevDevice,
                                     hHandle->PCIDEV_iDevFunction);
    if ((hDevHandle == LW_NULL) ||
        (hDevHandle != hHandle)) {
        return  (PX_ERROR);
    }

    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    _List_Line_Del(&hHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
    _GuiPciDevTotalNum -= 1;
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */

    __SHEAP_FREE(hHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevIoRemapEx2
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�
** �䡡��  : paPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlags            �ڴ����� LW_VMM_FLAG_DMA / LW_VMM_FLAG_RDWR / LW_VMM_FLAG_READ
** �䡡��  : ӳ���ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemapEx2 (phys_addr_t  paPhysicalAddr, size_t  stSize, ULONG  ulFlags)
{
#if LW_CFG_VMM_EN > 0
    size_t       stSizeAlign = ROUND_UP(stSize, LW_CFG_VMM_PAGE_SIZE);
    phys_addr_t  paBaseAlign = PHY_ROUND_DOWN(paPhysicalAddr, LW_CFG_VMM_PAGE_SIZE);
    addr_t       ulOffset    = (addr_t)(paPhysicalAddr - paBaseAlign);
    PVOID        pvRet;
    
    pvRet = API_VmmIoRemapEx2(paBaseAlign, stSizeAlign, ulFlags);
    if (!pvRet) {
        return  (LW_NULL);
    }
    
    return  ((PVOID)((addr_t)pvRet + ulOffset));
#else
    return  ((PVOID)paPhysicalAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: API_PciDevIoRemapEx
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlags            �ڴ����� LW_VMM_FLAG_DMA / LW_VMM_FLAG_RDWR / LW_VMM_FLAG_READ
** �䡡��  : ӳ���ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemapEx (PVOID  pvPhysicalAddr, size_t  stSize, ULONG  ulFlags)
{
#if LW_CFG_VMM_EN > 0
    size_t  stSizeAlign = ROUND_UP(stSize, LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulBaseAlign = ROUND_DOWN(((addr_t)pvPhysicalAddr), LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulOffset    = (addr_t)pvPhysicalAddr - ulBaseAlign;
    PVOID   pvRet;
    
    pvRet = API_VmmIoRemapEx((PVOID)ulBaseAlign, stSizeAlign, ulFlags);
    if (!pvRet) {
        return  (LW_NULL);
    }
    
    return  ((PVOID)((addr_t)pvRet + ulOffset));
#else
    return  (pvPhysicalAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: API_PciDevIoRemap
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ���ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemap (PVOID  pvPhysicalAddr, size_t  stSize)
{
    return  (API_PciDevIoRemapEx(pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_PciDevIoRemap2
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�
** �䡡��  : paPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ���ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemap2 (phys_addr_t  paPhysicalAddr, size_t  stSize)
{
    return  (API_PciDevIoRemapEx2(paPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_PciDevIoUnmap
** ��������: �ͷ� ioremap ռ�õ��߼��ռ�
** �䡡��  : pvVirtualMem      �����ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_PciDevIoUnmap (PVOID  pvVirtualAddr)
{
    return  (API_VmmIoUnmap(pvVirtualAddr));
}
/*********************************************************************************************************
** ��������: API_PciDevDrvDel
** ��������: ɾ��һ�� PCI �豸������
** �䡡��  : hDevHandle     �豸���ƾ��
**           hDrvHandle     �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvDel (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (hDevHandle->PCIDEV_pvDevDriver != hDrvHandle) {
        return  (PX_ERROR);
    }

    hDevHandle->PCIDEV_pvDevDriver = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevDrvUpdate
** ��������: ����һ�� PCI �豸������
** �䡡��  : hDevHandle     �豸���ƾ��
**           hDrvHandle     �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvUpdate (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if ((hDevHandle->PCIDEV_pvDevDriver == LW_NULL) ||
        (hDevHandle->PCIDEV_pvDevDriver != hDrvHandle)) {
        hDevHandle->PCIDEV_pvDevDriver = (PVOID)hDrvHandle;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDrvBindEachDev
** ��������: ��һ��PCI���������������ϵ�ÿ���豸(δ��������)���а�
** �䡡��  : hDrvHandle     �������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� API_PciDrvLoad �ڲ������ DRV_LOCK ����������, �˴����ᷢ������.

                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_PciDrvBindEachDev (PCI_DRV_HANDLE hDrvHandle)
{
    PCI_DEV_HANDLE        hDevCurr;
    PLW_LIST_LINE         plineTemp;
    PCI_DEV_ID_HANDLE     hId;

    __PCI_DEV_LOCK();                                                   /*  ���� PCI ����               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        hDevCurr = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);

        if (hDevCurr->PCIDEV_pvDevDriver) {                             /*  �Ѿ���������              */
            continue;
        }

        hId = API_PciDevMatchDrv(hDevCurr, hDrvHandle);
        if (!hId) {
            continue;                                                   /*  ID ��ƥ��                   */
        }

        API_PciDrvLoad(hDrvHandle, hDevCurr, hId);                      /*  ���豸����                */
    }
    __PCI_DEV_UNLOCK();                                                 /*  ���� PCI ����               */
}
/*********************************************************************************************************
** ��������: API_PciDevMasterEnable
** ��������: ʹ�� PCI �豸 Master ģʽ
** �䡡��  : hDevHandle   �豸���ƾ��
**           bEnable      �Ƿ�ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMasterEnable (PCI_DEV_HANDLE  hDevHandle, BOOL bEnable)
{
    UINT16 usCmdOld;
    UINT16 usCmd;
    INT    iRet;

    iRet = API_PciDevConfigRead(hDevHandle, PCI_COMMAND, (UINT8 *)&usCmdOld, sizeof(UINT16));
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (bEnable) {
        usCmd = usCmdOld | PCI_COMMAND_MASTER;
    
    } else {
        usCmd = usCmdOld & ~PCI_COMMAND_MASTER;
    }

    if (usCmd != usCmdOld) {
        iRet = API_PciDevConfigWrite(hDevHandle, PCI_COMMAND, (UINT8 *)&usCmd, sizeof(UINT16));
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pciDevListCreate
** ��������: �����豸�б�ص�, �ϲ��Ѿ�����, ����Ҫ�ٽ��м�������
** �䡡��  : iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
**           pvArg          ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciDevListCreate (INT  iBus, INT  iDevice, INT  iFunction, PVOID pvArg)
{
    API_PciDevAdd(iBus, iDevice, iFunction);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevListCreate
** ��������: ���� PCI �豸�б�
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevListCreate (VOID)
{
    API_PciTraversal(__pciDevListCreate, LW_NULL, PCI_MAX_BUS - 1);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevInit
** ��������: PCI �豸�����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevInit (VOID)
{
    _GuiPciDevTotalNum  = 0;
    _GplinePciDevHeader = LW_NULL;
    
    _GulPciDevLock = API_SemaphoreMCreate("pci_dev_lock",
                                          LW_PRIO_DEF_CEILING,
                                          LW_OPTION_WAIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);
    if (_GulPciDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("pcidev", __tshellPciCmdDev);
    API_TShellFormatAdd("pcidev", " [add | del] [[all] | 1:0.1]");
    API_TShellHelpAdd("pcidev", "show, add, del pci device table\n"
                                "eg. pcidev\n"
                                "    pcidev add all\n"
                                "    pcidev add 1:0.1\n"
                                "    pcidev del 1:0.1\n");

    API_TShellKeywordAdd("pciparent", __tshellPciCmdDevParent);
    API_TShellFormatAdd("pciparent", " [1:0.1]");
    API_TShellHelpAdd("pciparent", "show pci device parent node\n"
                                   "eg. pciparent 1:0.1\n");

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
