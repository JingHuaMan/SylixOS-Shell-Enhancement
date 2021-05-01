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
** ��   ��   ��: pciMsi.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 09 �� 10 ��
**
** ��        ��: PCI ���� MSI(Message Signaled Interrupts) ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/bitops.h"
#include "linux/log2.h"
#include "pciLib.h"
#include "pciMsi.h"
/*********************************************************************************************************
** ��������: API_PciMsixClearSet
** ��������: MSIx ���������������
** �䡡��  : iBus               ���ߺ�
**           iSlot              ���
**           iFunc              ����
**           uiMsixCapOft       ƫ�Ƶ�ַ
**           usClear            �����־
**           usSet              ���ñ�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixClearSet (INT     iBus,
                          INT     iSlot,
                          INT     iFunc,
                          UINT32  uiMsixCapOft,
                          UINT16  usClear,
                          UINT16  usSet)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    UINT16      usNew     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usNew  = usControl & (~usClear);
    usNew |= usSet;
    if (usNew != usControl) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_FLAGS, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsiMsgWrite
** ��������: д MSI ��Ϣ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           ucMultiple     ����
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgWrite (INT          iBus,
                         INT          iSlot,
                         INT          iFunc,
                         UINT32       uiMsixCapOft,
                         UINT8        ucMultiple,
                         PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    usControl &= ~PCI_MSI_FLAGS_QSIZE;
    usControl |= ucMultiple << 4;
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                 uiMsixCapOft + PCI_MSI_ADDRESS_LO, ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                     uiMsixCapOft + PCI_MSI_ADDRESS_HI, ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_DATA_64, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_DATA_32, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMsgRead
** ��������: �� MSI ��Ϣ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           ucMultiple     ����
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgRead (INT          iBus,
                        INT          iSlot,
                        INT          iFunc,
                        UINT32       uiMsixCapOft,
                        UINT8        ucMultiple,
                        PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                uiMsixCapOft + PCI_MSI_ADDRESS_LO, &ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_ADDRESS_HI, &ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   uiMsixCapOft + PCI_MSI_DATA_64, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        ppmmMsg->uiAddressHi = 0;
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   uiMsixCapOft + PCI_MSI_DATA_32, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingSet
** ��������: MSI Pending ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           uiPending      ָ��λ
**           uiFlag         �±�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingSet (INT     iBus,
                           INT     iSlot,
                           INT     iFunc,
                           UINT32  uiMsixCapOft,
                           UINT32  uiPending,
                           UINT32  uiFlag)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
    }
    uiPendinged &= ~uiPending;
    uiPendinged |=  uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iPendingPos, uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingGet
** ��������: ��ȡ MSI Pending ��ȡ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           puiPending     ��ȡ����״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, UINT32 *puiPending)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPendingPos, &uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiPending) {
        *puiPending = uiPendinged;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingPosGet
** ��������: ��ȡ MSI Pending λ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           piPendingPos   ���λ��
**                          0  ��������Ч
**                          1  ��������Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, INT *piPendingPos)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piPendingPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
        } else {
            *piPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskSet
** ��������: MSI ��������
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           uiMask         ����
**           uiFlag         �±�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskSet (INT     iBus,
                        INT     iSlot,
                        INT     iFunc,
                        UINT32  uiMsixCapOft,
                        UINT32  uiMask,
                        UINT32  uiFlag)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
    }
    uiMasked &= ~uiMask;
    uiMasked |= uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iMaskPos, uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskGet
** ��������: ��ȡ MSI ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           puiMask        ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, UINT32 *puiMask)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iMaskPos, &uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiMask) {
        *puiMask = uiMasked;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskPosGet
** ��������: ��ȡ MSI ����λ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           piMaskPos      ���λ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, INT *piMaskPos)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piMaskPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
        } else {
            *piMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskConvert
** ��������: MSI ����ת��
** �䡡��  : uiMask     ����
** �䡡��  : ת���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
UINT32  API_PciMsiMaskConvert (UINT32  uiMask)
{
    if (uiMask >= 5) {
        return  (0xffffffff);
    }

    return  ((1 << (1 << uiMask)) - 1);
}
/*********************************************************************************************************
** ��������: API_PciMsiMultipleGet
** ��������: ��ȡ MSI ��Ϣ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsixCapOft   ƫ�Ƶ�ַ
**           iNvec          �ж�����
**           piMultiple     ��Ϣ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultipleGet (INT     iBus,
                            INT     iSlot,
                            INT     iFunc,
                            UINT32  uiMsiCapOft,
                            INT     iNvec,
                            INT    *piMultiple)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiple) {
        *piMultiple = ilog2(__roundup_pow_of_two(iNvec));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiVecCountGet
** ��������: ��ȡ MSI ������
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
INT  API_PciMsiVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, UINT32 *puiVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (puiVecCount) {
        *puiVecCount = 1 << ((usControl & PCI_MSI_FLAGS_QMASK) >> 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMultiCapGet
** ��������: ��ȡ MSI ��Ϣ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           piMultiCap     ��Ϣ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultiCapGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piMultiCap)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiCap) {
        *piMultiCap = (usControl & PCI_MSI_FLAGS_QMASK) >> 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsi64BitGet
** ��������: ��ȡ MSI 64 λ��ַ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           pi64Bit        64 λ��ַ��־
**                          0  64 λ��ַ�����Ч
**                          1  64 λ��ַ�����Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsi64BitGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *pi64Bit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (pi64Bit) {
        *pi64Bit = !!(usControl & PCI_MSI_FLAGS_64BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskBitGet
** ��������: ��ȡ MSI ����λ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           uiMsiCapOft    ƫ�Ƶ�ַ
**           piMaskBit      �����־
**                          0  ��������Ч
**                          1  ��������Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskBitGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piMaskBit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMaskBit) {
        *piMaskBit = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableSet
** ��������: MSI ʹ�ܿ���
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
INT  API_PciMsiEnableSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSI_FLAGS_ENABLE));
    if (iEnable) {
        usControlNew |= PCI_MSI_FLAGS_ENABLE;
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableGet
** ��������: ��ȡ MSI ʹ�ܿ���״̬
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
INT  API_PciMsiEnableGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiRangeEnable
** ��������: ���� MSI ����ʹ��
** �䡡��  : hHandle        �豸���ƾ��
**           uiVecMin       ʹ�������ж�������Сֵ (�����ڲ����Զ�����Ϊ 1, 2, 4, 8, 16, 32)
**           uiVecMax       ʹ�������ж��������ֵ (�����ڲ����Զ�����Ϊ 1, 2, 4, 8, 16, 32)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ÿ�� PCI �豸��������ֻ��Ҫ����һ��
**
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsiRangeEnable (PCI_DEV_HANDLE  hHandle, UINT  uiVecMin, UINT  uiVecMax)
{
    INT                     i, j, iRet  = PX_ERROR;
    UINT8                   ucMsiEn     = 0;
    UINT32                  uiMsiCapOft = 0;
    UINT32                  uiVecNum    = 0;
    PCI_MSI_DESC_HANDLE     hMsgHandle  = LW_NULL;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if ((uiVecMin > 32) || (uiVecMax > 32) ||
        (uiVecMax < uiVecMin) || (uiVecMin < 1)) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if ((iRet != ERROR_NONE) ||
        (!PCI_DEV_MSI_IS_EN(hHandle))) {
        return  (PX_ERROR);
    }
    
    for (i = 0; i < 6; i++) {
        j = 1 << i;
        if (j >= uiVecMin) {
            break;
        }
    }
    uiVecMin = j;
    
    for (i = 0; i < 6; i++) {
        j = 1 << i;
        if (j >= uiVecMax) {
            break;
        }
    }
    uiVecMax = j;

    iRet = API_PciMsiVecCountGet(hHandle->PCIDEV_iDevBus,
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

    hMsgHandle = &hHandle->PCIDEV_pmdDevIrqMsiDesc;
    hMsgHandle->PCIMSI_uiNum = uiVecNum;

__reget:
    iRet = API_PciDevInterMsiGet(hHandle, hMsgHandle);
    if (iRet != ERROR_NONE) {
        hMsgHandle->PCIMSI_uiNum >>= 1;
        if (hMsgHandle->PCIMSI_uiNum < uiVecMin) {
            return  (PX_ERROR);
        }
        goto    __reget;
    }

    hHandle->PCIDEV_uiDevIrqMsiNum = hMsgHandle->PCIMSI_uiNum;
    hHandle->PCIDEV_ulDevIrqVector = hMsgHandle->PCIMSI_ulDevIrqVector;

    /*
     *  MSI can support only 1, 2, 4, 8, 16, 32 number of vectors
     */
    switch (hHandle->PCIDEV_uiDevIrqMsiNum) {
    
    case 1:
        ucMsiEn = 0;
        break;

    case 2:
        ucMsiEn = 1;
        break;

    case 4:
        ucMsiEn = 2;
        break;

    case 8:
        ucMsiEn = 3;
        break;

    case 16:
        ucMsiEn = 4;
        break;

    case 32:
        ucMsiEn = 5;
        break;

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciMsiMsgWrite(hHandle->PCIDEV_iDevBus,
                              hHandle->PCIDEV_iDevDevice,
                              hHandle->PCIDEV_iDevFunction,
                              uiMsiCapOft,
                              ucMsiEn,
                              &hHandle->PCIDEV_pmdDevIrqMsiDesc.PCIMSI_pmmMsg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiVecCountGet
** ��������: ��ȡ MSI ������
** �䡡��  : hHandle        �豸���ƾ��
**           puiVecCount    ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsiVecCountGet (PCI_DEV_HANDLE  hHandle, UINT32 *puiVecCount)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiVecCountGet(hHandle->PCIDEV_iDevBus,
                                 hHandle->PCIDEV_iDevDevice,
                                 hHandle->PCIDEV_iDevFunction,
                                 uiMsiCapOft, puiVecCount);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiEnableGet
** ��������: ��ȡ MSI ʹ�ܿ���״̬
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
INT  API_PciDevMsiEnableGet (PCI_DEV_HANDLE  hHandle, INT *piEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiEnableGet(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiMsiCapOft,
                               piEnable);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciDevMsiEnableSet
** ��������: ���� MSI ʹ�ܿ���״̬
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
INT  API_PciDevMsiEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (!iEnable) {
        iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction, 1);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiEnableSet(hHandle->PCIDEV_iDevBus,
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
