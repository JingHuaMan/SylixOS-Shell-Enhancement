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
** ��   ��   ��: pciLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 28 ��
**
** ��        ��: PCI ��������ģ��.
**
** BUG:
2015.08.26  ֧�� PCI_MECHANISM_0 ģʽ.
2016.04.25  ���� �豸����ƥ�� ֧�ֺ���.
2018.08.28  ��� PCI_MECHANISM_1 2 ģʽ���� mm config ֧��.
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
#include "pciIds.h"
#include "pciDev.h"
#include "pciDrv.h"
#include "pciLib.h"
#include "pciProc.h"
/*********************************************************************************************************
  PCI ������
*********************************************************************************************************/
PCI_CTRL_HANDLE     _G_hPciCtrlHandle = LW_NULL;
#define PCI_CTRL    _G_hPciCtrlHandle
/*********************************************************************************************************
  PCI ���������� IO ���� PCI_MECHANISM_0
*********************************************************************************************************/
#define PCI_CFG_READ(iBus, iSlot, iFunc, iOft, iLen, pvRet)                                             \
        PCI_CTRL->PCI_pDrvFuncs0->cfgRead(iBus, iSlot, iFunc, iOft, iLen, pvRet)
#define PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, iLen, uiData)                                           \
        PCI_CTRL->PCI_pDrvFuncs0->cfgWrite(iBus, iSlot, iFunc, iOft, iLen, uiData)
#define PCI_VPD_READ(iBus, iSlot, iFunc, iPos, pucBuf, iLen)                                            \
        PCI_CTRL->PCI_pDrvFuncs0->vpdRead(iBus, iSlot, iFunc, iPos, pucBuf, iLen)
#define PCI_IRQ_GET(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pvIrq)                                     \
        PCI_CTRL->PCI_pDrvFuncs0->irqGet(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pvIrq)
#define PCI_CFG_SPCL(iBus, uiMsg)                                                                       \
        PCI_CTRL->PCI_pDrvFuncs0->cfgSpcl(iBus, uiMsg)
#define PCI_CFG_SPCL_IS_EN()                                                                            \
        PCI_CTRL->PCI_pDrvFuncs0->cfgSpcl
/*********************************************************************************************************
  PCI ���������õ�ַ PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_CONFIG_ADDR0()          PCI_CTRL->PCI_ulConfigAddr
#define PCI_CONFIG_ADDR1()          PCI_CTRL->PCI_ulConfigData
#define PCI_CONFIG_ADDR2()          PCI_CTRL->PCI_ulConfigBase
/*********************************************************************************************************
  PCI ���������� IO ���� PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_IN_BYTE(addr)           PCI_CTRL->PCI_pDrvFuncs12->ioInByte((addr))
#define PCI_IN_WORD(addr)           PCI_CTRL->PCI_pDrvFuncs12->ioInWord((addr))
#define PCI_IN_DWORD(addr)          PCI_CTRL->PCI_pDrvFuncs12->ioInDword((addr))
#define PCI_OUT_BYTE(addr, data)    PCI_CTRL->PCI_pDrvFuncs12->ioOutByte((UINT8)(data), (addr))
#define PCI_OUT_WORD(addr, data)    PCI_CTRL->PCI_pDrvFuncs12->ioOutWord((UINT16)(data), (addr))
#define PCI_OUT_DWORD(addr, data)   PCI_CTRL->PCI_pDrvFuncs12->ioOutDword((UINT32)(data), (addr))
/*********************************************************************************************************
  PCI �����������жϲ��� PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_IRQ_GET12(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pvIrq)  \
        PCI_CTRL->PCI_pDrvFuncs12->irqGet(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pvIrq)
#define PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, iLen, pvRet)                                        \
        PCI_CTRL->PCI_pDrvFuncs12->mmCfgRead(iBus, iSlot, iFunc, iOft, iLen, pvRet)
#define PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, iLen, uiData)                                      \
        PCI_CTRL->PCI_pDrvFuncs12->mmCfgWrite(iBus, iSlot, iFunc, iOft, iLen, uiData)
/*********************************************************************************************************
  PCI ������ MM Config ̽��
*********************************************************************************************************/
#define PCI_MM_CFG_READ_CHECK()                                                                         \
        _BugHandle(!PCI_CTRL->PCI_pDrvFuncs12->mmCfgRead, LW_TRUE, "PCI no 'mmCfgRead' for offset >= 255.")
#define PCI_MM_CFG_WRITE_CHECK()                                                                        \
        _BugHandle(!PCI_CTRL->PCI_pDrvFuncs12->mmCfgWrite, LW_TRUE, "PCI no 'mmCfgWrite' for offset >= 255.")
/*********************************************************************************************************
  PCI �Ƿ�Ϊ��Ч VENDOR_ID
*********************************************************************************************************/
#define PCI_VENDOR_ID_IS_INVALIDATE(vendor)     \
        (((vendor) == 0xffff) || ((vendor) == 0x0000))
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static PCI_DEV_ID_HANDLE __pciDevMatchId(PCI_DEV_HANDLE hDevHandle, PCI_DEV_ID_HANDLE hId);
/*********************************************************************************************************
  PCI �ߴ絥λ����
*********************************************************************************************************/
static CPCHAR  _G_pcPciSizeNameSuffix[5] = {"", "K", "M", "G", "T"};
/*********************************************************************************************************
** ��������: __tshellPciCmdCtrl
** ��������: PCI ���� "pcictrl"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciCmdCtrl (INT  iArgC, PCHAR  ppcArgV[])
{
    static PCHAR        pcPciCtrlShowHdr = \
    " INDEX   METHOD   AUTOEN   HOSTEN   FIRST   LAST       I/O-BUS            I/O-SIZE      "
    "     I/O-PHY            MEM-BUS            MEM-SIZE           MEM-PHY            PRE-BUS       "
    "     PRE-SIZE           PRE-PHY\n"
    "------- -------- -------- -------- ------- ------ ------------------ ------------------ "
    "------------------ ------------------ ------------------ ------------------ ------------------ "
    "------------------ ------------------\n";

    PCI_CTRL_HANDLE     hPciCtrl = PCI_CTRL;
    PCI_AUTO_HANDLE     hPciAuto;

    if (!hPciCtrl) {
        return  (ERROR_NONE);
    }

    hPciAuto = &hPciCtrl->PCI_tAutoConfig;

    printf(pcPciCtrlShowHdr);
    printf("%7d %8d %8x %8x %7x %6x %18qx %18qx %18qx %18qx %18qx %18qx %18qx %18qx %18qx\n",
           hPciCtrl->PCI_iIndex,
           hPciCtrl->PCI_ucMechanism,
           hPciAuto->PCIAUTO_iConfigEn,
           hPciAuto->PCIAUTO_iHostBridegCfgEn,
           hPciAuto->PCIAUTO_uiFirstBusNo,
           hPciAuto->PCIAUTO_uiLastBusNo,
           hPciAuto->PCIAUTO_hRegionIo ? hPciAuto->PCIAUTO_hRegionIo->PCIAUTOREG_addrBusStart : 0,
           hPciAuto->PCIAUTO_hRegionIo ? hPciAuto->PCIAUTO_hRegionIo->PCIAUTOREG_stSize : 0,
           hPciAuto->PCIAUTO_hRegionIo ? hPciAuto->PCIAUTO_hRegionIo->PCIAUTOREG_addrPhyStart : 0,
           hPciAuto->PCIAUTO_hRegionMem ? hPciAuto->PCIAUTO_hRegionMem->PCIAUTOREG_addrBusStart : 0,
           hPciAuto->PCIAUTO_hRegionMem ? hPciAuto->PCIAUTO_hRegionMem->PCIAUTOREG_stSize : 0,
           hPciAuto->PCIAUTO_hRegionMem ? hPciAuto->PCIAUTO_hRegionMem->PCIAUTOREG_addrPhyStart : 0,
           hPciAuto->PCIAUTO_hRegionPre ? hPciAuto->PCIAUTO_hRegionPre->PCIAUTOREG_addrBusStart : 0,
           hPciAuto->PCIAUTO_hRegionPre ? hPciAuto->PCIAUTO_hRegionPre->PCIAUTOREG_stSize : 0,
           hPciAuto->PCIAUTO_hRegionPre ? hPciAuto->PCIAUTO_hRegionPre->PCIAUTOREG_addrPhyStart : 0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciSizeNameGet
** ��������: ��� PCI �ߴ絥λ����
** �䡡��  : stSize    ��С
** �䡡��  : �ߴ絥λ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
CPCHAR  API_PciSizeNameGet (pci_size_t stSize)
{
    UINT  i;

    i = 0;
    if (!stSize) {
        return  (_G_pcPciSizeNameSuffix[i]);
    }

    for (i = 0; i < (sizeof(_G_pcPciSizeNameSuffix) / sizeof(_G_pcPciSizeNameSuffix) - 1); i++) {
        if (stSize % 1024) {
            break;
        }

        stSize /= 1024;
    }

    return  (_G_pcPciSizeNameSuffix[i]);
}
/*********************************************************************************************************
** ��������: API_PciSizeNumGet
** ��������: ��� PCI ����λ�Ĵ�С
** �䡡��  : stSize    ��С
** �䡡��  : �����Ĵ�С
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
pci_size_t  API_PciSizeNumGet (pci_size_t stSize)
{
    UINT  i;

    if (!stSize) {
        return  (0);
    }

    for (i = 0; i < (sizeof(_G_pcPciSizeNameSuffix) / sizeof(_G_pcPciSizeNameSuffix[0]) - 1); i++) {
        if (stSize % 1024) {
            break;
        }

        stSize /= 1024;
    }

    return  (stSize);
}
/*********************************************************************************************************
** ��������: API_PciCtrlCreate
** ��������: ��װ pci ��������������
** �䡡��  : hCtrl  pci ��������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
PCI_CTRL_HANDLE  API_PciCtrlCreate (PCI_CTRL_HANDLE hCtrl)
{
    PCI_AUTO_HANDLE     hPciAuto;                                       /* �Զ����ÿ��ƾ��             */

    if (PCI_CTRL == LW_NULL) {
        PCI_CTRL = hCtrl;
        
        LW_SPIN_INIT(&PCI_CTRL->PCI_slLock);
        
        API_PciDevInit();
        API_PciDevListCreate();

        /*
         *  ��Ҫ���豸�б������֮���ٽ����Զ�����
         *  ��Ϊ����ƽ̨���üĴ����Ķ�д��Ҫ�������ڵ����������
         *
         *  ��Ҫ���Զ����ú����¸����豸�б�
         *  ��Ϊ�������������޸�ö���豸���ͽṹ
         */
        API_PciAutoScan(hCtrl, (UINT32 *)&hCtrl->PCI_iBusMax);
        hPciAuto = &hCtrl->PCI_tAutoConfig;
        if (hPciAuto->PCIAUTO_iConfigEn) {                              /* �Զ�����ʹ��                 */
            API_PciDevDelete(LW_NULL);
            API_PciDevListCreate();
        }

#if LW_CFG_PROCFS_EN > 0
        __procFsPciInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

        API_PciDevSetupAll();
        API_PciDrvInit();

        API_TShellKeywordAdd("pcictrl", __tshellPciCmdCtrl);
        API_TShellHelpAdd("pcictrl", "show control table\n"
                                     "eg. pcictrl\n");
    }
    
    return  (PCI_CTRL) ? (PCI_CTRL) : (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciCtrlReset
** ��������: ֹͣ pci �����豸 (ֻ�������ϲ������豸)
** �䡡��  : iRebootType   ϵͳ��λ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
VOID  API_PciCtrlReset (INT  iRebootType)
{
    (VOID)iRebootType;
    
    if (PCI_CTRL == LW_NULL) {
        return;
    }
    
    API_PciTraversalDev(0, LW_FALSE, (INT (*)())API_PciFuncDisable, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciConfigInByte
** ��������: �� pci ���ÿռ��ȡһ���ֽ�
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           pucValue  ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 *pucValue)
{
    INTREG  iregInterLevel;
    UINT8   ucRet = 0;
    UINT32  uiDword;
    INT     iRetVal = PX_ERROR;
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);
    
    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 1, (PVOID)&ucRet) < 0) {
            ucRet = 0xff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;
    
    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 1, (PVOID)&ucRet) < 0) {
                ucRet = 0xff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            ucRet = PCI_IN_BYTE(PCI_CONFIG_ADDR1() + (iOft & 0x3));
            iRetVal = ERROR_NONE;
        }
        break;
        
    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 1, (PVOID)&ucRet) < 0) {
                ucRet = 0xff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            uiDword = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
            uiDword >>= (iOft & 0x03) * 8;
            ucRet   = (UINT8)(uiDword & 0xff);
            iRetVal = ERROR_NONE;
        }
        break;
        
    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    if (pucValue) {
        *pucValue = ucRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciConfigInWord
** ��������: �� pci ���ÿռ��ȡһ���� (16 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           pusValue  ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 *pusValue)
{
    INTREG  iregInterLevel;
    UINT16  usRet = 0;
    UINT32  uiDword;
    INT     iRetVal = PX_ERROR;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 2, (PVOID)&usRet) < 0) {
            usRet = 0xffff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 2, (PVOID)&usRet) < 0) {
                usRet = 0xffff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            usRet = PCI_IN_WORD(PCI_CONFIG_ADDR1() + (iOft & 0x2));
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 2, (PVOID)&usRet) < 0) {
                usRet = 0xffff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            uiDword = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
            uiDword >>= (iOft & 0x02) * 8;
            usRet   = (UINT16)(uiDword & 0xffff);
            iRetVal = ERROR_NONE;
        }
        break;

    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    if (pusValue) {
        *pusValue = usRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciConfigInDword
** ��������: �� pci ���ÿռ��ȡһ��˫�� (32 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           puiValue  ��ȡ�Ľ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 *puiValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet   = 0;
    INT     iRetVal = PX_ERROR;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 4, (PVOID)&uiRet) < 0) {
            uiRet = 0xffffffff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 4, (PVOID)&uiRet) < 0) {
                uiRet = 0xffffffff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR1());
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_READ_CHECK();
            if (PCI_MM_CFG_READ12(iBus, iSlot, iFunc, iOft, 4, (PVOID)&uiRet) < 0) {
                uiRet = 0xffffffff;
            } else {
                iRetVal = ERROR_NONE;
            }
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
            iRetVal = ERROR_NONE;
        }
        break;

    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    if (puiValue) {
        *puiValue = uiRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciConfigOutByte
** ��������: �� pci ���ÿռ�д��һ���ֽ�
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           ucValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet;
    UINT32  uiMask = 0x000000ff;
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 1, ucValue);
        break;

    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 1, ucValue);
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            PCI_OUT_BYTE((PCI_CONFIG_ADDR1() + (iOft & 0x3)), ucValue);
        }
        break;

    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 1, ucValue);
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            uiRet    = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
            ucValue  = (ucValue & uiMask) << ((iOft & 0x03) * 8);
            uiMask <<= (iOft & 0x03) * 8;
            uiRet    = (uiRet & ~uiMask) | ucValue;
            PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiRet);
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        }
        break;

    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciConfigOutWord
** ��������: �� pci ���ÿռ�д��һ���� (16 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           usValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet;
    UINT32  uiMask = 0x0000ffff;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 2, usValue);
        break;

    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 2, usValue);
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            PCI_OUT_WORD((PCI_CONFIG_ADDR1() + (iOft & 0x2)), usValue);
        }
        break;

    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 2, usValue);
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            uiRet    = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
            usValue  = (usValue & uiMask) << ((iOft & 0x02) * 8);
            uiMask <<= (iOft & 0x02) * 8;
            uiRet    = (uiRet & ~uiMask) | usValue;
            PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiRet);
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        }
        break;

    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciConfigOutDword
** ��������: �� pci ���ÿռ�д��һ��˫�� (32 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           uiValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiValue)
{
    INTREG  iregInterLevel;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);
    
    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 4, uiValue);
        break;

    case PCI_MECHANISM_1:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 4, uiValue);
        } else {
            PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
            PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiValue);
        }
        break;

    case PCI_MECHANISM_2:
        if (iOft >= 255) {
            PCI_MM_CFG_WRITE_CHECK();
            PCI_MM_CFG_WRITE12(iBus, iSlot, iFunc, iOft, 4, uiValue);
        } else {
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
            PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
            PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiValue);
            PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        }
        break;

    default:
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciConfigModifyByte
** ��������: �� pci ���ÿռ��������λ���޸�һ���ֽ�
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           ucMask    ����
**           ucValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucMask, UINT8 ucValue)
{
    INTREG  iregInterLevel;
    UINT8   ucTemp;
    INT     iRet = PX_ERROR;
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInByte(iBus, iSlot, iFunc, iOft, &ucTemp) == ERROR_NONE) {
        ucTemp = (ucTemp & ~ucMask) | (ucValue & ucMask);
        iRet   = API_PciConfigOutByte(iBus, iSlot, iFunc, iOft, ucTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciConfigModifyWord
** ��������: �� pci ���ÿռ��������λ���޸�һ���� (16 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           usMask    ����
**           usValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usMask, UINT16 usValue)
{
    INTREG  iregInterLevel;
    UINT16  usTemp;
    INT     iRet = PX_ERROR;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInWord(iBus, iSlot, iFunc, iOft, &usTemp) == ERROR_NONE) {
        usTemp = (usTemp & ~usMask) | (usValue & usMask);
        iRet   = API_PciConfigOutWord(iBus, iSlot, iFunc, iOft, usTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciConfigModifyDword
** ��������: �� pci ���ÿռ��������λ���޸�һ��˫�� (32 bit)
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iOft      ���ÿռ��ַƫ��
**           uiMask    ����
**           uiValue   д�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiMask, UINT32 uiValue)
{
    INTREG  iregInterLevel;
    UINT32  uiTemp;
    INT     iRet = PX_ERROR;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInDword(iBus, iSlot, iFunc, iOft, &uiTemp) == ERROR_NONE) {
        uiTemp = (uiTemp & ~uiMask) | (uiValue & uiMask);
        iRet   = API_PciConfigOutDword(iBus, iSlot, iFunc, iOft, uiTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciSpecialCycle
** ��������: �� pci �����Ϲ㲥һ����Ϣ
** �䡡��  : iBus      ���ߺ�
**           uiMsg     ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciSpecialCycle (INT iBus, UINT32 uiMsg)
{
    INTREG  iregInterLevel;
    INT     iRet  = ERROR_NONE;
    INT     iSlot = 0x1f;
    INT     iFunc = 0x07;
    
    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);
    
    switch (PCI_CTRL->PCI_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_SPCL_IS_EN()) {
            PCI_CFG_SPCL(iBus, uiMsg);
        }
        break;
    
    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | 0x80000000));
        PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiMsg);
        break;
    
    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xff);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), 0x00);
        PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8)), uiMsg);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        break;
    
    default:
        iRet = PX_ERROR;
        break;
    }
    
    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciFindDev
** ��������: ��ѯһ��ָ���� pci �豸
** �䡡��  : usVendorId    ��Ӧ�̺�
**           usDeviceId    �豸 ID
**           iInstance     �ڼ����豸ʵ��
**           piBus         ��ȡ�����ߺ�
**           piSlot        ��ȡ�Ĳ�ۺ�
**           piFunc        ��ȡ�Ĺ��ܺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciFindDev (UINT16  usVendorId, 
                     UINT16  usDeviceId, 
                     INT     iInstance,
                     INT    *piBus, 
                     INT    *piSlot, 
                     INT    *piFunc)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    INT         iRet = PX_ERROR;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    UINT16      usDeviceTemp;
    
    if (iInstance < 0) {
        return  (PX_ERROR);
    }
    
    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID, &usDeviceTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  û���豸����                */
                    if (iFunc == 0) {
                        break;
                    }
                    continue;                                           /*  next function               */
                }
                
                if ((usVendorTemp == usVendorId) && 
                    (usDeviceTemp == usDeviceId)) {                     /*  ƥ���ѯ����                */
                    
                    if (iInstance == 0) {
                        if (piBus) {
                            *piBus = iBus;
                        }
                        if (piSlot) {
                            *piSlot = iSlot;
                        }
                        if (piFunc) {
                            *piFunc = iFunc;
                        }
                        iRet = ERROR_NONE;
                        goto    __out;
                    }
                    iInstance--;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciFindClass
** ��������: ��ѯָ���豸���͵� pci �豸
** �䡡��  : usClassCode   ���ͱ��� (PCI_CLASS_STORAGE_SCSI, PCI_CLASS_DISPLAY_XGA ...)
**           iInstance     �ڼ����豸ʵ��
**           piBus         ��ȡ�����ߺ�
**           piSlot        ��ȡ�Ĳ�ۺ�
**           piFunc        ��ȡ�Ĺ��ܺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciFindClass (UINT16  usClassCode, 
                       INT     iInstance,
                       INT    *piBus, 
                       INT    *piSlot, 
                       INT    *piFunc)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    INT         iRet = PX_ERROR;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    UINT16      usClassTemp;
    
    if (iInstance < 0) {
        return  (PX_ERROR);
    }
    
    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  û���豸����                */
                    if (iFunc == 0) {
                        break;
                    }
                    continue;                                           /*  next function               */
                }
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usClassTemp);
                
                if (usClassTemp == usClassCode) {
                
                    if (iInstance == 0) {
                        if (piBus) {
                            *piBus = iBus;
                        }
                        if (piSlot) {
                            *piSlot = iSlot;
                        }
                        if (piFunc) {
                            *piFunc = iFunc;
                        }
                        iRet = ERROR_NONE;
                        goto    __out;
                    }
                    iInstance--;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciTraversal
** ��������: pci ���߱���
** �䡡��  : pfuncCall     �����ص�����
**           pvArg         �ص���������
**           iMaxBusNum    ������ߺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciTraversal (INT (*pfuncCall)(), PVOID pvArg, INT iMaxBusNum)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    
    if (!pfuncCall || (iMaxBusNum < 0)) {
        return  (PX_ERROR);
    }
    
    iMaxBusNum = (iMaxBusNum > (PCI_MAX_BUS - 1)) ? (PCI_MAX_BUS - 1) : iMaxBusNum;
    
    for (iBus = 0; iBus <= iMaxBusNum; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  û���豸����                */
                    if (iFunc == 0) {
                        break;                                          /*  next slot                   */
                    }
                    continue;                                           /*  next function               */
                }
                
                if (pfuncCall(iBus, iSlot, iFunc, pvArg) != ERROR_NONE) {
                    goto    __out;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciTraversalDev
** ��������: pci ���߱��������е��豸
** �䡡��  : iBusStart     ��ʼ���ߺ�
**           bSubBus       �Ƿ�����Ž�����
**           pfuncCall     �����ص�����
**           pvArg         �ص���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciTraversalDev (INT   iBusStart, 
                          BOOL  bSubBus,
                          INT (*pfuncCall)(), 
                          PVOID pvArg)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    UINT16      usVendorId;
    UINT16      usSubClass;
    
    UINT8       ucHeader;
    UINT8       ucSecBus;
    
    INT         iRet;
    
    if (!pfuncCall) {
        return  (PX_ERROR);
    }
    
    iBus = iBusStart;
    
    for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
        for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorId);
            
            if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorId)) {
                if (iFunc == 0) {
                    break;                                              /*  next slot                   */
                }
                continue;                                               /*  next function               */
            }
            
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usSubClass);
            
            if ((usSubClass & 0xff00) != (PCI_BASE_CLASS_BRIDGE << 8)) {
                iRet = pfuncCall(iBus, iSlot, iFunc, pvArg);
                if (iRet != ERROR_NONE) {
                    return  (iRet);
                }
            } 
            
            if (bSubBus) {
                if ((usSubClass == PCI_CLASS_BRIDGE_PCI) ||
                    (usSubClass == PCI_CLASS_BRIDGE_CARDBUS)) {         /*  PCI to PCI or PCI to cardbus*/
                    
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS, &ucSecBus);
                    
                    if (ucSecBus > 0) {
                        iRet = API_PciTraversalDev(ucSecBus, bSubBus,
                                                   pfuncCall, pvArg);
                    
                    } else {
                        iRet = ERROR_NONE;
                    }
                    
                    if (iRet != ERROR_NONE) {
                        return  (iRet);
                    }
                }
            }
            
            if (iFunc == 0) {
                API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                
                if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                    break;
                }
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMemSizeGet
** ��������: ��ȡ�ռ��С
** �䡡��  : hDevHandle     �豸���
**           pstIoSize      IO �ռ��С
**           pstMemSize     �ڴ�ռ��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMemSizeGet (PCI_DEV_HANDLE   hDevHandle,
                           pci_size_t      *pstIoSize,
                           pci_size_t      *pstMemSize)
{
    INT             iBar          = 0;
    UINT32          uiBarResponse = 0;
    pci_size_t      stIoSize      = 0;
    pci_size_t      stMemSize     = 0;

    for (iBar = PCI_BASE_ADDRESS_0; iBar <= PCI_BASE_ADDRESS_5; iBar += 4) {

        API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                              hDevHandle->PCIDEV_iDevDevice,
                              hDevHandle->PCIDEV_iDevFunction,
                              iBar, 0xffffffff);
        API_PciConfigInDword(hDevHandle->PCIDEV_iDevBus,
                             hDevHandle->PCIDEV_iDevDevice,
                             hDevHandle->PCIDEV_iDevFunction,
                             iBar, &uiBarResponse);
        if (uiBarResponse == 0) {
            continue;
        }

        if (uiBarResponse & PCI_BASE_ADDRESS_SPACE) {
            stIoSize += ((~(uiBarResponse & PCI_BASE_ADDRESS_IO_MASK)) & 0xffff) + 1;
        } else {
            if ((uiBarResponse & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) {
                UINT32  uiBarResponseUpper;
                UINT64  ullBar64;

                API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                                      hDevHandle->PCIDEV_iDevDevice,
                                      hDevHandle->PCIDEV_iDevFunction,
                                      iBar + 4, 0xffffffff);
                API_PciConfigInDword(hDevHandle->PCIDEV_iDevBus,
                                     hDevHandle->PCIDEV_iDevDevice,
                                     hDevHandle->PCIDEV_iDevFunction,
                                     iBar + 4, &uiBarResponseUpper);

                ullBar64   = ((UINT64)uiBarResponseUpper << 32) | uiBarResponse;
                stMemSize += ~(ullBar64 & PCI_BASE_ADDRESS_MEM_MASK) + 1;
            } else {
                stMemSize += (UINT32)(~(uiBarResponse & PCI_BASE_ADDRESS_MEM_MASK) + 1);
            }
        }
    }

    if (pstIoSize) {
        *pstIoSize = stIoSize;
    }

    if (pstMemSize) {
        *pstMemSize = stMemSize;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMemConfig
** ��������: ���� pci �����ϵ�һ���豸��Դ
**           ���Ƚ�����豸����, Ȼ������ I/O ���� �ڴ��ַ, Ȼ�����ø��ٻ��������ӳټĴ���,
**           ����µ�ָ��д��ָ��Ĵ���.
** �䡡��  : hDevHandle     �豸���
**           ulIoBase       IO ����ַ
**           ulMemBase      �ڴ����ַ
**           ucLatency      �ӳ�ʱ�� (PCI clocks max 255)
**           uiCommand      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciDevMemConfig (PCI_DEV_HANDLE  hDevHandle,
                          ULONG           ulIoBase,
                          pci_addr_t      ulMemBase,
                          UINT8           ucLatency,
                          UINT32          uiCommand)
{
    INT         iBar;
    INT         iFoundMem64;
    pci_addr_t  ulBarValue;
    pci_size_t  ulBarSize;
    UINT32      uiBarResponse;
    UINT8       ucCacheLine;
    UINT8       ucPin;
    UINT32      uiCommandTemp;

    if (!hDevHandle) {
        return  (PX_ERROR);
    }

    API_PciConfigInDword(hDevHandle->PCIDEV_iDevBus,
                         hDevHandle->PCIDEV_iDevDevice,
                         hDevHandle->PCIDEV_iDevFunction,
                         PCI_COMMAND, &uiCommandTemp);
    uiCommandTemp = (uiCommandTemp & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) | PCI_COMMAND_MASTER;
    
    for (iBar = PCI_BASE_ADDRESS_0; iBar <= PCI_BASE_ADDRESS_5; iBar += 4) {
    
        API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                              hDevHandle->PCIDEV_iDevDevice,
                              hDevHandle->PCIDEV_iDevFunction,
                              iBar, 0xffffffff);
        API_PciConfigInDword(hDevHandle->PCIDEV_iDevBus,
                             hDevHandle->PCIDEV_iDevDevice,
                             hDevHandle->PCIDEV_iDevFunction,
                             iBar, &uiBarResponse);
        if (uiBarResponse == 0) {
            continue;
        }
        
        iFoundMem64 = 0;
        
        if (uiBarResponse & PCI_BASE_ADDRESS_SPACE) {
            ulBarSize  = ((~(uiBarResponse & PCI_BASE_ADDRESS_IO_MASK)) & 0xffff) + 1;
            ulIoBase   = ((ulIoBase - 1) | (ulBarSize - 1)) + 1;
            ulBarValue = ulIoBase;
            ulIoBase   = ulIoBase + ulBarSize;
        } else {
            if ((uiBarResponse & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) {
                UINT32  uiBarResponseUpper;
                UINT64  ullBar64;
                
                API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                                      hDevHandle->PCIDEV_iDevDevice,
                                      hDevHandle->PCIDEV_iDevFunction,
                                      iBar + 4, 0xffffffff);
                API_PciConfigInDword(hDevHandle->PCIDEV_iDevBus,
                                     hDevHandle->PCIDEV_iDevDevice,
                                     hDevHandle->PCIDEV_iDevFunction,
                                     iBar + 4, &uiBarResponseUpper);
                
                ullBar64  = ((UINT64)uiBarResponseUpper << 32) | uiBarResponse;
                ulBarSize = ~(ullBar64 & PCI_BASE_ADDRESS_MEM_MASK) + 1;
                
                iFoundMem64 = 1;
            
            } else {
                ulBarSize = (UINT32)(~(uiBarResponse & PCI_BASE_ADDRESS_MEM_MASK) + 1);
            }
            
            ulMemBase  = ((ulMemBase - 1) | (ulBarSize - 1)) + 1;
            ulBarValue = ulMemBase;
            ulMemBase  = ulMemBase + ulBarSize;
        }

        API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                              hDevHandle->PCIDEV_iDevDevice,
                              hDevHandle->PCIDEV_iDevFunction,
                              iBar, (UINT32)ulBarValue);
        
        if (iFoundMem64) {
            iBar += 4;
#if LW_CFG_PCI_64 > 0
            API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                                  hDevHandle->PCIDEV_iDevDevice,
                                  hDevHandle->PCIDEV_iDevFunction,
                                  iBar, (UINT32)(ulBarValue >> 32));
#else
            API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                                  hDevHandle->PCIDEV_iDevDevice,
                                  hDevHandle->PCIDEV_iDevFunction,
                                  iBar, 0x00000000);
#endif
        }

        uiCommandTemp |= (uiBarResponse & PCI_BASE_ADDRESS_SPACE) ? PCI_COMMAND_IO : PCI_COMMAND_MEMORY;
    }
    
#if LW_CFG_CACHE_EN > 0
    ucCacheLine = (API_CacheLine(DATA_CACHE) >> 2);
#else
    ucCacheLine = (32 >> 2);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    API_PciConfigOutByte(hDevHandle->PCIDEV_iDevBus,
                         hDevHandle->PCIDEV_iDevDevice,
                         hDevHandle->PCIDEV_iDevFunction,
                         PCI_CACHE_LINE_SIZE, ucCacheLine);
    API_PciConfigOutByte(hDevHandle->PCIDEV_iDevBus,
                         hDevHandle->PCIDEV_iDevDevice,
                         hDevHandle->PCIDEV_iDevFunction,
                         PCI_LATENCY_TIMER, ucLatency);
    
    /* 
     * Disable interrupt line, if device says it wants to use interrupts
     */
    API_PciConfigInByte(hDevHandle->PCIDEV_iDevBus,
                        hDevHandle->PCIDEV_iDevDevice,
                        hDevHandle->PCIDEV_iDevFunction,
                        PCI_INTERRUPT_PIN, &ucPin);
    if (ucPin != 0) {
        API_PciConfigOutByte(hDevHandle->PCIDEV_iDevBus,
                             hDevHandle->PCIDEV_iDevDevice,
                             hDevHandle->PCIDEV_iDevFunction,
                             PCI_INTERRUPT_LINE, 0xff);
    }

    API_PciConfigOutDword(hDevHandle->PCIDEV_iDevBus,
                          hDevHandle->PCIDEV_iDevDevice,
                          hDevHandle->PCIDEV_iDevFunction,
                          PCI_COMMAND, uiCommandTemp | uiCommand);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pciDevBarSize
** ��������: ���� BAR SIZE
** �䡡��  : ulBase     ����ַ
**           ulBaseMax  ���ֵ
**           ulMask     ����
** �䡡��  : BAR SIZE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
static UINT64  __pciDevBarSize (UINT64  ulBase, UINT64  ulBaseMax, UINT64  ulMask)
{
    UINT64      ulSize;

    ulSize = ulBaseMax & ulMask;
    if (!ulSize) {
        return  (0);
    }

    ulSize = (ulSize & ~(ulSize - 1)) - 1;
    if (ulBase == ulBaseMax && ((ulBase | ulSize) & ulMask) != ulMask) {
        return  (0);
    }

    return  (ulSize);
}
/*********************************************************************************************************
** ��������: __pciDevBarDecode
** ��������: ���� BAR ��Ϣ
** �䡡��  : hHandle    �豸���
**           uiBar      BAR ֵ
** �䡡��  : BAR ��־��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __pciDevBarDecode (PCI_DEV_HANDLE hHandle, UINT32  uiBar)
{
    UINT32      uiMemType;
    ULONG       ulFlags;

    if ((uiBar & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
        ulFlags  = uiBar & ~PCI_BASE_ADDRESS_IO_MASK;
        ulFlags |= PCI_IORESOURCE_IO;

        return  (ulFlags);
    }

    ulFlags = uiBar & ~PCI_BASE_ADDRESS_MEM_MASK;
    ulFlags |= PCI_IORESOURCE_MEM;
    if (ulFlags & PCI_BASE_ADDRESS_MEM_PREFETCH) {
        ulFlags |= PCI_IORESOURCE_PREFETCH;
    }

    uiMemType = uiBar & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
    switch (uiMemType) {

    case PCI_BASE_ADDRESS_MEM_TYPE_32:
        break;

    case PCI_BASE_ADDRESS_MEM_TYPE_1M:                                  /* 1M mem BAR as 32-bit BAR     */
        break;

    case PCI_BASE_ADDRESS_MEM_TYPE_64:
        ulFlags |= PCI_IORESOURCE_MEM_64;
        break;

    default:                                                            /* mem unknown type 32-bit BAR  */
        break;
    }

    return  (ulFlags);
}
/*********************************************************************************************************
** ��������: __pciDevReadBase
** ��������: ��ȡָ����Դ��Ϣ
** �䡡��  : hHandle    �豸���
**           iType      ��Դ���� (PCI_BAR_TYPE_xxxx)
**           hRes       ��Դ��ַ
**           uiPos      ƫ�Ƶ�ַ
** �䡡��  : 0      32 λ���� BAR
**           1      64 λ���� BAR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciDevReadBase (PCI_DEV_HANDLE hHandle, INT iType, PCI_RESOURCE_HANDLE hRes, UINT uiPos)
{
    UINT32      uiLen;
    UINT32      uiSize;
    UINT32      uiMask;
    UINT64      ulLen;
    UINT64      ulSize;
    UINT64      ulMask;
    UINT16      usCmd;

    uiMask = iType ? PCI_ROM_ADDRESS_MASK : ~0;

    API_PciDevConfigReadWord(hHandle, PCI_COMMAND, &usCmd);
    if (usCmd & (PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) {
        API_PciDevConfigWriteWord(hHandle, PCI_COMMAND, usCmd & (~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)));
    }

    hRes->PCIRS_pcName = &hHandle->PCIDEV_cDevName[0];

    API_PciDevConfigReadDword(hHandle, uiPos, &uiLen);
    API_PciDevConfigWriteDword(hHandle, uiPos, uiLen | uiMask);
    API_PciDevConfigReadDword(hHandle, uiPos, &uiSize);
    API_PciDevConfigWriteDword(hHandle, uiPos, uiLen);

    if (uiSize == 0xffffffff) {
        uiSize = 0;
    }

    if (uiLen == 0xffffffff) {
        uiLen = 0;
    }

    if (iType == PCI_BAR_TYPE_UNKNOWN) {
        hRes->PCIRS_ulFlags  = __pciDevBarDecode(hHandle, uiLen);
        hRes->PCIRS_ulFlags |= PCI_IORESOURCE_SIZEALIGN;
        if (hRes->PCIRS_ulFlags & PCI_IORESOURCE_IO) {
            ulLen  = uiLen & PCI_BASE_ADDRESS_IO_MASK;
            ulSize = uiSize & PCI_BASE_ADDRESS_IO_MASK;
            ulMask = PCI_BASE_ADDRESS_IO_MASK & (UINT32)PCI_IO_SPACE_LIMIT;
        } else {
            ulLen  = uiLen & PCI_BASE_ADDRESS_MEM_MASK;
            ulSize = uiSize & PCI_BASE_ADDRESS_MEM_MASK;
            ulMask = (UINT32)PCI_BASE_ADDRESS_MEM_MASK;
        }
    } else {
        hRes->PCIRS_ulFlags |= (uiLen & PCI_IORESOURCE_ROM_ENABLE);
        ulLen  = uiLen & PCI_ROM_ADDRESS_MASK;
        ulSize = uiSize & PCI_ROM_ADDRESS_MASK;
        ulMask = (UINT32)PCI_ROM_ADDRESS_MASK;
    }

    if (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) {
        API_PciDevConfigReadDword(hHandle, uiPos + 4, &uiLen);
        API_PciDevConfigWriteDword(hHandle, uiPos + 4, ~0);
        API_PciDevConfigReadDword(hHandle, uiPos + 4, &uiSize);
        API_PciDevConfigWriteDword(hHandle, uiPos + 4, uiLen);

        ulLen  |= ((UINT64)uiLen << 32);
        ulSize |= ((UINT64)uiSize << 32);
        ulMask |= ((UINT64)~0 << 32);
    }

    if (usCmd & (PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) {
        API_PciDevConfigWriteWord(hHandle, PCI_COMMAND, usCmd);
    }

    if (!ulSize) {
        hRes->PCIRS_ulFlags = 0;

        return  (0);
    }

    ulSize = __pciDevBarSize(ulLen, ulSize, ulMask);
    if (!ulSize) {
        hRes->PCIRS_ulFlags = 0;

        return  (0);
    }

    if (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) {
        if ((sizeof(pci_bus_addr_t) < 8 || sizeof(pci_resource_size_t) < 8) &&
            (ulSize > 0x100000000ULL)) {
            hRes->PCIRS_ulFlags |= PCI_IORESOURCE_UNSET | PCI_IORESOURCE_DISABLED;
            hRes->PCIRS_stStart  = 0;
            hRes->PCIRS_stEnd    = 0;

            return  (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) ? 1 : 0;
        }

        if ((sizeof(pci_bus_addr_t) < 8) && uiLen) {
            hRes->PCIRS_ulFlags |= PCI_IORESOURCE_UNSET;
            hRes->PCIRS_stStart  = 0;
            hRes->PCIRS_stEnd    = ulSize;

            return  (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) ? 1 : 0;
        }
    }

    hRes->PCIRS_stStart = ulLen;
    hRes->PCIRS_stEnd   = ulLen + ulSize;

    return  (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) ? 1 : 0;
}
/*********************************************************************************************************
** ��������: __pciDevReadBases
** ��������: ��ȡ��Դ��Ϣ
** �䡡��  : hHandle    �豸���
**           uiHowMany  ��Դ��Ŀ
**           iRom       ROM ƫ�Ƶ�ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pciDevReadBases (PCI_DEV_HANDLE hHandle, UINT uiHowMany, INT iRom)
{
    UINT                    uiPos;
    UINT                    uiReg;
    PCI_RESOURCE_HANDLE     hRes;

    for (uiPos = 0; uiPos < uiHowMany; uiPos++) {
        hRes = &hHandle->PCIDEV_tResource[uiPos];
        uiReg = PCI_BASE_ADDRESS_0 + (uiPos << 2);
        uiPos += __pciDevReadBase(hHandle, PCI_BAR_TYPE_UNKNOWN, hRes, uiReg);
    }

    if (iRom) {
        hRes = &hHandle->PCIDEV_tResource[PCI_ROM_RESOURCE];
        hHandle->PCIDEV_ucRomBaseReg = (UINT8)iRom;
        hRes->PCIRS_ulFlags = PCI_IORESOURCE_MEM | PCI_IORESOURCE_PREFETCH |
                              PCI_IORESOURCE_READONLY | PCI_IORESOURCE_SIZEALIGN;
        __pciDevReadBase(hHandle, PCI_BAR_TYPE_MEM32, hRes, iRom);
    }
}
/*********************************************************************************************************
** ��������: __pciDevReadIrq
** ��������: ��ȡ IRQ ��Դ��Ϣ
** �䡡��  : hHandle    �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pciDevReadIrq (PCI_DEV_HANDLE hHandle)
{
    INT                     iRet;
    PCI_RESOURCE_HANDLE     hResource;

    if (!hHandle) {
        return;
    }

    hResource = &hHandle->PCIDEV_tResource[PCI_IRQ_RESOURCE];

    hHandle->PCIDEV_iDevIrqMsiEn = LW_FALSE;
    iRet = API_PciDevInterVectorGet(hHandle, &hHandle->PCIDEV_ulDevIrqVector);
    if (iRet != ERROR_NONE) {
        hResource->PCIRS_pcName  = LW_NULL;
        hResource->PCIRS_stStart = 0;
        hResource->PCIRS_stEnd   = 0;
        hResource->PCIRS_ulFlags = 0;
        return;
    }

    hResource->PCIRS_pcName  = hHandle->PCIDEV_cDevName;
    hResource->PCIRS_stStart = hHandle->PCIDEV_ulDevIrqVector;
    hResource->PCIRS_stEnd   = hHandle->PCIDEV_ulDevIrqVector;
    hResource->PCIRS_ulFlags = PCI_IORESOURCE_IRQ;
}
/*********************************************************************************************************
** ��������: API_PciDevSetup
** ��������: ���� PCI �����ϵ�һ���豸
** �䡡��  : hDevHandle     �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevSetup (PCI_DEV_HANDLE  hDevHandle)
{
    if (!hDevHandle) {
        return  (PX_ERROR);
    }

    /*
     *  ��ִ���Զ����ú�, ��Ҫ���������豸��Ϣ
     */
    API_PciGetHeader(hDevHandle->PCIDEV_iDevBus,
                     hDevHandle->PCIDEV_iDevDevice,
                     hDevHandle->PCIDEV_iDevFunction,
                     &hDevHandle->PCIDEV_phDevHdr);                     /* �����豸��Ϣ                 */

    hDevHandle->PCIDEV_iType = hDevHandle->PCIDEV_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (hDevHandle->PCIDEV_iType) {

    case PCI_HEADER_TYPE_NORMAL:
        __pciDevReadIrq(hDevHandle);
        __pciDevReadBases(hDevHandle, PCI_DEV_BAR_MAX, PCI_ROM_ADDRESS);
        hDevHandle->PCIDEV_uiResourceNum = PCI_NUM_RESOURCES;
        hDevHandle->PCIDEV_ucPin         = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;
        hDevHandle->PCIDEV_ucLine        = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntLine;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntLine;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciResourceGet
** ��������: ��ȡ PCI ������Դ��Ϣ
** �䡡��  : iBus       ���ߺ�
**           iDevice    �豸
**           iFunc      ����
**           uiType     ��Դ����
**           uiNum      ��Դ����
** �䡡��  : ��Դ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_RESOURCE_HANDLE  API_PciResourceGet (INT iBus, INT iDevice, INT iFunc, UINT uiType, UINT uiNum)
{
    PCI_DEV_HANDLE      hDevHandle;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunc);

    return  (API_PciDevResourceGet(hDevHandle, uiType, uiNum));
}
/*********************************************************************************************************
** ��������: API_PciDevResourceGet
** ��������: ��ȡ PCI �豸��Դ
** �䡡��  : hDevHandle     �豸���
**           uiType         ��Դ����
**           uiNum          ��Դ����
** �䡡��  : ��Դ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_RESOURCE_HANDLE  API_PciDevResourceGet (PCI_DEV_HANDLE  hDevHandle, UINT uiType, UINT uiNum)
{
    INT                     i;
    PCI_RESOURCE_HANDLE     hResource;

    if ((!hDevHandle) || (uiNum >= PCI_NUM_RESOURCES)) {
        return  (LW_NULL);
    }

    for (i = 0; i < hDevHandle->PCIDEV_uiResourceNum; i++) {
        hResource = &hDevHandle->PCIDEV_tResource[i];
        if ((uiType == PCI_RESOURCE_TYPE(hResource)) &&
            (uiNum-- == 0)) {
            return  (hResource);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciDevStdResourceGet
** ��������: ��ȡ PCI �豸 STD BAR ��Դ
** �䡡��  : hDevHandle     �豸���
**           uiType         ��Դ����
**           uiNum          ��Դ����
** �䡡��  : ��Դ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_RESOURCE_HANDLE  API_PciDevStdResourceGet (PCI_DEV_HANDLE  hDevHandle, UINT uiType, UINT uiNum)
{
    INT                     i;
    PCI_RESOURCE_HANDLE     hResource;

    if ((!hDevHandle) || (uiNum > PCI_STD_RESOURCE_END)) {
        return  (LW_NULL);
    }

    for (i = 0; i <= PCI_STD_RESOURCE_END; i++) {
        hResource = &hDevHandle->PCIDEV_tResource[i];
        if ((uiType == PCI_RESOURCE_TYPE(hResource)) &&
            (uiNum-- == 0)) {
            return  (hResource);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciDevStdResourceFind
** ��������: ��ѯ PCI �豸 STD BAR ��Դ
** �䡡��  : hDevHandle     �豸���
**           uiType         ��Դ����
**           uiNum          ��Դ����
** �䡡��  : ��Դ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_RESOURCE_HANDLE  API_PciDevStdResourceFind (PCI_DEV_HANDLE  hDevHandle, UINT uiType, 
                                                pci_resource_size_t  stStart)
{
    INT                     i;
    PCI_RESOURCE_HANDLE     hResource;
    
    if (!hDevHandle) {
        return  (LW_NULL);
    }
    
    for (i = 0; i <= PCI_STD_RESOURCE_END; i++) {
        hResource = &hDevHandle->PCIDEV_tResource[i];
        if ((uiType == PCI_RESOURCE_TYPE(hResource)) &&
            (stStart == PCI_RESOURCE_START(hResource))) {
            return  (hResource);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciFuncDisable
** ��������: ֹͣ pci �豸����
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciFuncDisable (INT iBus, INT iSlot, INT iFunc)
{
    return  (API_PciConfigModifyDword(iBus, iSlot, iFunc, PCI_COMMAND, 
                                      PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, 0));
}
/*********************************************************************************************************
** ��������: API_PciInterConnect
** ��������: ���� pci �ж�����
** �䡡��  : ulVector  CPU �ж����� (pci ���忨���ܻṲ�� CPU ĳһ�ж�����)
**           pfuncIsr  �жϷ�����
**           pvArg     �жϷ���������
**           pcName    �жϷ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciInterConnect (ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr, PVOID pvArg, CPCHAR pcName)
{
    ULONG   ulFlag = 0ul;

    API_InterVectorGetFlag(ulVector, &ulFlag);
    if (!(ulFlag & LW_IRQ_FLAG_QUEUE)) {
        ulFlag |= LW_IRQ_FLAG_QUEUE;
        API_InterVectorSetFlag(ulVector, ulFlag);                       /*  �������༶�ж�              */
    }
    
    if (API_InterVectorConnect(ulVector, pfuncIsr, pvArg, pcName) == ERROR_NONE) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_PciInterDisonnect
** ��������: ���� pci ����ж�����
** �䡡��  : ulVector  CPU �ж����� (pci ���忨���ܻṲ�� CPU ĳһ�ж�����)
**           pfuncIsr  �жϷ�����
**           pvArg     �жϷ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciInterDisconnect (ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr, PVOID pvArg)
{
    if (API_InterVectorDisconnect(ulVector, pfuncIsr, pvArg) == ERROR_NONE) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_PciGetHader
** ��������: ���ָ���豸�� pci ͷ��Ϣ
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           p_pcihdr  ͷ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciGetHeader (INT iBus, INT iSlot, INT iFunc, PCI_HDR *p_pcihdr)
{
#define PCI_D  p_pcihdr->PCIH_pcidHdr
#define PCI_B  p_pcihdr->PCIH_pcibHdr
#define PCI_CB p_pcihdr->PCIH_pcicbHdr

    if (p_pcihdr == LW_NULL) {
        return  (PX_ERROR);
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &p_pcihdr->PCIH_ucType);
    p_pcihdr->PCIH_ucType &= PCI_HEADER_TYPE_MASK;

    if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_NORMAL) {              /* PCI iSlot                   */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID,       &PCI_D.PCID_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID,       &PCI_D.PCID_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,         &PCI_D.PCID_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,          &PCI_D.PCID_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION,  &PCI_D.PCID_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,      &PCI_D.PCID_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,    &PCI_D.PCID_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,           &PCI_D.PCID_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE, &PCI_D.PCID_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,   &PCI_D.PCID_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,     &PCI_D.PCID_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,            &PCI_D.PCID_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_0]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_1, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_1]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_2, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_2]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_3, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_3]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_4, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_4]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_5, &PCI_D.PCID_uiBase[PCI_BAR_INDEX_5]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CARDBUS_CIS,         &PCI_D.PCID_uiCis);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SUBSYSTEM_VENDOR_ID, &PCI_D.PCID_usSubVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SUBSYSTEM_ID,        &PCI_D.PCID_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS,         &PCI_D.PCID_uiRomBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,      &PCI_D.PCID_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,       &PCI_D.PCID_ucIntPin);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_MIN_GNT,             &PCI_D.PCID_ucMinGrant);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_MAX_LAT,             &PCI_D.PCID_ucMaxLatency);

    } else if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_BRIDGE) {       /* PCI to PCI bridge            */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID,          &PCI_B.PCIB_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID,          &PCI_B.PCIB_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,            &PCI_B.PCIB_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,             &PCI_B.PCIB_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION,     &PCI_B.PCIB_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,         &PCI_B.PCIB_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,       &PCI_B.PCIB_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,              &PCI_B.PCIB_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,    &PCI_B.PCIB_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,      &PCI_B.PCIB_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,        &PCI_B.PCIB_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,               &PCI_B.PCIB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_IO_BASE,            &PCI_B.PCIB_uiBase[0]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_MEMORY_BASE,        &PCI_B.PCIB_uiBase[1]);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_PRIMARY_BUS,        &PCI_B.PCIB_ucPriBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SECONDARY_BUS,      &PCI_B.PCIB_ucSecBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SUBORDINATE_BUS,    &PCI_B.PCIB_ucSubBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SEC_LATENCY_TIMER,  &PCI_B.PCIB_ucSecLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_IO_BASE,            &PCI_B.PCIB_ucIoBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_IO_LIMIT,           &PCI_B.PCIB_ucIoLimit);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SEC_STATUS,         &PCI_B.PCIB_usSecStatus);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_MEMORY_BASE,        &PCI_B.PCIB_usMemBase);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_MEMORY_LIMIT,       &PCI_B.PCIB_usMemLimit);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_PREF_MEMORY_BASE,   &PCI_B.PCIB_usPreBase);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_PREF_MEMORY_LIMIT,  &PCI_B.PCIB_usPreLimit);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_BASE_UPPER32,  &PCI_B.PCIB_uiPreBaseUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_LIMIT_UPPER32, &PCI_B.PCIB_uiPreLimitUpper);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_IO_BASE_UPPER16,    &PCI_B.PCIB_usIoBaseUpper);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_IO_LIMIT_UPPER16,   &PCI_B.PCIB_usIoLimitUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS1,       &PCI_B.PCIB_uiRomBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,     &PCI_B.PCIB_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,      &PCI_B.PCIB_ucIntPin);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_BRIDGE_CONTROL,     &PCI_B.PCIB_usControl);

    } else if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_CARDBUS) {      /* PCI card iBus bridge         */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID,       &PCI_CB.PCICB_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID,       &PCI_CB.PCICB_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,         &PCI_CB.PCICB_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,          &PCI_CB.PCICB_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION,  &PCI_CB.PCICB_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,      &PCI_CB.PCICB_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,    &PCI_CB.PCICB_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,           &PCI_CB.PCICB_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE, &PCI_CB.PCICB_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,   &PCI_CB.PCICB_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,     &PCI_CB.PCICB_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,            &PCI_CB.PCICB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc,PCI_BASE_ADDRESS_0,&PCI_CB.PCICB_uiBase[PCI_BAR_INDEX_0]);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CAPABILITY_LIST,        &PCI_CB.PCICB_ucCapPtr);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SEC_STATUS,          &PCI_CB.PCICB_usSecStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_PRIMARY_BUS,         &PCI_CB.PCICB_ucPriBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_CARD_BUS,            &PCI_CB.PCICB_ucSecBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_SUBORDINATE_BUS,     &PCI_CB.PCICB_ucSubBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_LATENCY_TIMER,       &PCI_CB.PCICB_ucSecLatency);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_0,       &PCI_CB.PCICB_uiMemBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_0,      &PCI_CB.PCICB_uiMemLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_1,       &PCI_CB.PCICB_uiMemBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_1,      &PCI_CB.PCICB_uiMemLimit1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_0,           &PCI_CB.PCICB_uiIoBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_0,          &PCI_CB.PCICB_uiIoLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_1,           &PCI_CB.PCICB_uiIoBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_1,          &PCI_CB.PCICB_uiIoLimit1);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,         &PCI_CB.PCICB_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,          &PCI_CB.PCICB_ucIntPin);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_BRIDGE_CONTROL,         &PCI_CB.PCICB_usControl);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_VENDOR_ID, &PCI_CB.PCICB_usSubVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_ID,        &PCI_CB.PCICB_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_LEGACY_MODE_BASE,    &PCI_CB.PCICB_uiLegacyBase);
    
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciHeaderTypeGet
** ��������: ��ȡ�豸ͷ����.
** �䡡��  : iBus        ���ߺ�
**           iSlot       ���
**           iFunc       ����
**           ucType      �豸����(PCI_HEADER_TYPE_NORMAL, PCI_HEADER_TYPE_BRIDGE, PCI_HEADER_TYPE_CARDBUS)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciHeaderTypeGet (INT iBus, INT iSlot, INT iFunc, UINT8 *ucType)
{
    UINT8       ucHeaderType = 0xFF;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, (UINT8 *)&ucHeaderType);
    if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE) {
        ucHeaderType = PCI_HEADER_TYPE_BRIDGE;
    } else if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_CARDBUS) {
        ucHeaderType = PCI_HEADER_TYPE_CARDBUS;
    } else if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_NORMAL) {
        ucHeaderType = PCI_HEADER_TYPE_NORMAL;
    } else {
        return  (PX_ERROR);
    }

    if (ucType) {
        *ucType = ucHeaderType;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciVpdRead
** ��������: ��ȡ VPD (Vital Product Data) ����
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iPos      ���ݵ�ַ
**           pucBuf    ���������
**           iLen      �����������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciVpdRead (INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen)
{
    INTREG  iregInterLevel;
    INT     iRetVal = PX_ERROR;

    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {

    case PCI_MECHANISM_0:
        iRetVal = PCI_VPD_READ(iBus, iSlot, iFunc, iPos, pucBuf, iLen);
        break;

    case PCI_MECHANISM_1:
        iRetVal = PX_ERROR;
        break;

    case PCI_MECHANISM_2:
        iRetVal = PX_ERROR;
        break;

    default:
        break;
    }

    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciIrqGet
** ��������: ��ȡ INTx ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiEn         �Ƿ�ʹ�� MSI
**           iLine          �ж���
**           iPin           �ж�����
**           pulVector      �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciIrqGet (INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, ULONG *pulVector)
{
    INTREG  iregInterLevel;
    INT     iRetVal = PX_ERROR;

    if (iMsiEn) {
        if (pulVector) {
            *pulVector = (ULONG)PX_ERROR;
        }
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {

    case PCI_MECHANISM_0:
        iRetVal = PCI_IRQ_GET(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, (PVOID)pulVector);
        break;

    case PCI_MECHANISM_1:
    case PCI_MECHANISM_2:
        iRetVal = PCI_IRQ_GET12(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, (PVOID)pulVector);
        break;

    default:
        break;
    }

    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciIrqMsi
** ��������: ��ȡ MSI MSI-X ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiEn         �Ƿ�ʹ�� MSI
**           iLine          �ж���
**           iPin           �ж�����
**           pmsidesc       MSI �ж���Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciIrqMsi (INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, PCI_MSI_DESC *pmsidesc)
{
    INTREG  iregInterLevel;
    INT     iRetVal = PX_ERROR;

    if (iMsiEn == 0) {
        if (pmsidesc) {
            pmsidesc->PCIMSI_uiNum          = 0;
            pmsidesc->PCIMSI_ulDevIrqVector = (ULONG)PX_ERROR;
        }
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&PCI_CTRL->PCI_slLock, &iregInterLevel);

    switch (PCI_CTRL->PCI_ucMechanism) {

    case PCI_MECHANISM_0:
        iRetVal = PCI_IRQ_GET(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, (PVOID)pmsidesc);
        break;

    case PCI_MECHANISM_1:
    case PCI_MECHANISM_2:
        iRetVal = PCI_IRQ_GET12(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, (PVOID)pmsidesc);
        break;

    default:
        break;
    }

    LW_SPIN_UNLOCK_QUICK(&PCI_CTRL->PCI_slLock, iregInterLevel);

    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: API_PciConfigFetch
** ��������: ��ȡ VPD (Vital Product Data) ����
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           uiPos     ���ݵ�ַ
**           uiLen     �����������С
** �䡡��  : 1
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciConfigFetch (INT iBus, INT iSlot, INT iFunc, UINT uiPos, UINT uiLen)
{
    /*
     *  TODO
     */
    return  (1);
}
/*********************************************************************************************************
** ��������: API_PciConfigHandleGet
** ��������: ��ȡָ�� PCI ���þ��
** �䡡��  : iIndex    PCI ������ (PCI_CTRL_HANDLE hCtrl) ����
** �䡡��  : LW_NULL or ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_CTRL_HANDLE  API_PciConfigHandleGet (INT iIndex)
{
    if (PCI_CTRL == LW_NULL) {
        return  (LW_NULL);
    }

    if (PCI_CTRL->PCI_iIndex == iIndex) {
        return  (PCI_CTRL);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciConfigIndexGet
** ��������: ͨ�� PCI ���þ����ȡ����
** �䡡��  : hCtrl      PCI ������ (PCI_CTRL_HANDLE) ���
** �䡡��  : ERROR or ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciConfigIndexGet (PCI_CTRL_HANDLE hCtrl)
{
    if (!hCtrl) {
        return  (PX_ERROR);
    }

    return  (hCtrl->PCI_iIndex);
}
/*********************************************************************************************************
** ��������: __procFsPciGetCnt
** ��������: ��� PCI �豸����
** �䡡��  : iBus          ���ߺ�
**           iSlot         ���
**           iFunc         ���ܺ�
**           pstBufferSize ��������С
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciBusCntGet (INT iBus, INT iSlot, INT iFunc, UINT *puiCount)
{
    (VOID)iBus;
    (VOID)iSlot;
    (VOID)iFunc;

    if (puiCount) {
        *puiCount += 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciConfigBusMaxSet
** ��������: ����ָ�� PCI ���������
** �䡡��  : iIndex    PCI ���� (PCI_CONFIG) ����
**           uiBusMax  PCI ���������
** �䡡��  : ERROR or ���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciConfigBusMaxSet (INT  iIndex, UINT32  uiBusMax)
{
    if (PCI_CTRL == LW_NULL) {
        return  (PX_ERROR);
    }

    if (PCI_CTRL->PCI_iIndex == iIndex) {
        PCI_CTRL->PCI_iBusMax = uiBusMax;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PciConfigBusMaxGet
** ��������: ��ȡָ�� PCI ���������
** �䡡��  : iIndex    PCI ���� (PCI_CONFIG) ����
** �䡡��  : ERROR or ���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciConfigBusMaxGet (INT iIndex)
{
    UINT        uiBusNumber = 0;

    if (PCI_CTRL == LW_NULL) {
        return  (PX_ERROR);
    }

    if (PCI_CTRL->PCI_iIndex == iIndex) {
        API_PciTraversal(__pciBusCntGet, &uiBusNumber, PCI_MAX_BUS - 1);

        PCI_CTRL->PCI_iBusMax = uiBusNumber;
        return  (PCI_CTRL->PCI_iBusMax);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PciIntxEnableSet
** ��������: ���� INTx ʹ�������
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           iEnable   ʹ�ܱ�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciIntxEnableSet (INT iBus, INT iSlot, INT iFunc, INT iEnable)
{
    INT         iRet = PX_ERROR;
    UINT16      usCommand, usNew;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usCommand);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (iEnable) {
        usNew = usCommand & ~PCI_COMMAND_INTX_DISABLE;
    } else {
        usNew = usCommand | PCI_COMMAND_INTX_DISABLE;
    }

    if (usNew != usCommand) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciIntxMaskSupported
** ��������: ̽�� INTx �Ƿ�֧������
** �䡡��  : iBus               ���ߺ�
**           iSlot              ���
**           iFunc              ����
**           piSupported        �Ƿ�֧��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciIntxMaskSupported (INT iBus, INT iSlot, INT iFunc, INT *piSupported)
{
    INT         iMaskSupported = LW_FALSE;
    INT         iRet           = PX_ERROR;
    UINT16      usOrig         = 0;
    UINT16      usNew          = 0;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usOrig);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usOrig ^ PCI_COMMAND_INTX_DISABLE);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usNew);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if ((usNew ^ usOrig) & ~PCI_COMMAND_INTX_DISABLE) {
        return  (PX_ERROR);
    } else if ((usNew ^ usOrig) & PCI_COMMAND_INTX_DISABLE) {
        iMaskSupported = LW_TRUE;
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usOrig);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    if (piSupported) {
        *piSupported = iMaskSupported;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevMatchDrv
** ��������: PCI �豸����ƥ��
** �䡡��  : hDevHandle   �豸���
**           hDrvHandle   �������
** �䡡��  : �ɹ�: ����ƥ���һ�� PCI ID ��Ϣ, ʧ��: ���� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PCI_DEV_ID_HANDLE API_PciDevMatchDrv (PCI_DEV_HANDLE hDevHandle, PCI_DRV_HANDLE hDrvHandle)
{
    PCI_DEV_ID_HANDLE   hId;
    INT                 i;

    if (!hDevHandle || !hDrvHandle) {
        return  (LW_NULL);
    }

    if (hDevHandle->PCIDEV_phDevHdr.PCIH_ucType != PCI_HEADER_TYPE_NORMAL) {
        return  (LW_NULL);
    }

    for (hId = hDrvHandle->PCIDRV_hDrvIdTable, i = 0; i < hDrvHandle->PCIDRV_uiDrvIdTableSize; i++) {
        if (__pciDevMatchId(hDevHandle, hId)) {
            return  (hId);
        }
        hId++;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PciDevMatchDrv
** ��������: PCI �豸����ƥ��
** �䡡��  : hDevHandle   �豸���
**           hId          һ�� ID ����
** �䡡��  : �ɹ�: ���ظ� ID����, ʧ��: ���� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
static PCI_DEV_ID_HANDLE __pciDevMatchId (PCI_DEV_HANDLE hDevHandle, PCI_DEV_ID_HANDLE hId)
{
    PCI_DEV_HDR    *phdr       = &hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr;
    UINT32          uiDevClass = (UINT32)((phdr->PCID_ucClassCode << 16) +
                                          (phdr->PCID_ucSubClass  <<  8) +
                                          phdr->PCID_ucProgIf);

    if (((hId->PCIDEVID_uiVendor    == PCI_ANY_ID) || (hId->PCIDEVID_uiVendor    == phdr->PCID_usVendorId))    &&
        ((hId->PCIDEVID_uiDevice    == PCI_ANY_ID) || (hId->PCIDEVID_uiDevice    == phdr->PCID_usDeviceId))    &&
        ((hId->PCIDEVID_uiSubVendor == PCI_ANY_ID) || (hId->PCIDEVID_uiSubVendor == phdr->PCID_usSubVendorId)) &&
        ((hId->PCIDEVID_uiSubDevice == PCI_ANY_ID) || (hId->PCIDEVID_uiSubDevice == phdr->PCID_usSubSystemId)) &&
        !((hId->PCIDEVID_uiClass ^ uiDevClass) & hId->PCIDEVID_uiClassMask)) {
        return  (hId);
    }

    return  (LW_NULL);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
