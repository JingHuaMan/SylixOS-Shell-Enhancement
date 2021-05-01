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
** ��   ��   ��: pciAuto.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 10 �� 21 ��
**
** ��        ��: PCI �����Զ�����.
**
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciIds.h"
#include "pciLib.h"
#include "pciAuto.h"
/*********************************************************************************************************
** ��������: __pciAutoDevSkip
** ��������: ������ʼ���豸
** �䡡��  : hCtrl      ���������
**           hAutoDev   �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAutoDevSkip (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev)
{
    PCI_AUTO_HANDLE     hPciAuto;                                       /* �Զ����þ��                 */

    hPciAuto = &hCtrl->PCI_tAutoConfig;                                 /* ��ȡ�Զ����þ��             */
                                                                        /* �Ƿ�Ϊ��ʼ�豸               */
    if (hAutoDev == PCI_AUTO_BDF(hPciAuto->PCIAUTO_uiFirstBusNo, 0, 0)) {
        if (hPciAuto->PCIAUTO_iHostBridegCfgEn) {                       /* ��������ʹ��                 */
            return  (PX_ERROR);
        
        } else {                                                        /* ����������                   */
            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x dev skip.\n",
                         PCI_AUTO_BUS(hAutoDev), PCI_AUTO_DEV(hAutoDev), PCI_AUTO_FUNC(hAutoDev));
            return  (ERROR_NONE);                                       /* �����豸                     */
        }
    }

    return  (PX_ERROR);                                                 /* �������豸                   */
}
/*********************************************************************************************************
** ��������: API_PciAutoBusReset
** ��������: ��λ�����������豸
** �䡡��  : hCtrl      ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoBusReset (PCI_CTRL_HANDLE  hCtrl)
{
    PCI_AUTO_DEV_HANDLE     hAutoDev;                                   /* �Զ������豸���             */
    INT                     iBus = 0;                                   /* ���ߺ�                       */
    INT                     iDev = 0;                                   /* �豸��                       */
    INT                     iFunc = 0;                                  /* ���ܺ�                       */
    UINT32                  uiMulti;                                    /* �๦�ܱ�ʶ                   */
    UINT16                  usVendor;                                   /* ������Ϣ                     */
    UINT16                  usClass;                                    /* ����Ϣ                       */
    UINT8                   ucHeaderType;                               /* �豸ͷ����                   */

    uiMulti = 0;

    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {                        /* ������������                 */
        for (hAutoDev  = PCI_AUTO_BDF(iBus, 0, 0);
             hAutoDev <= PCI_AUTO_BDF(iBus, PCI_MAX_SLOTS - 1, PCI_MAX_FUNCTIONS - 1);
             hAutoDev += PCI_AUTO_BDF(0, 0, 1)) {                       /* �����豸�빦��               */
            iDev  = PCI_AUTO_DEV(hAutoDev);
            iFunc = PCI_AUTO_FUNC(hAutoDev);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev reset %02x:%02x.%01x.\n", iBus, iDev, iFunc);

            if ((PCI_AUTO_FUNC(hAutoDev)) && (!uiMulti)) {              /* �������豸                   */
                continue;
            }

            API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
            API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
            if ((usVendor == 0xffff) ||
                (usVendor == 0x0000)) {                                 /* �豸��Ч                     */
                continue;
            }

            if (!PCI_AUTO_FUNC(hAutoDev)) {
                uiMulti = ucHeaderType & PCI_HEADER_MULTI_FUNC;
            }

            /*
             *  ��λ�����豸
             */
            API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);
            
            switch (usClass) {

            case PCI_CLASS_BRIDGE_PCI:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0x00);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_1, 0x00000000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_BASE, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_LIMIT, 0x00);

                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_BASE, 0x0000);
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_LIMIT, 0x0000);

                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, 0x0000);
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, 0x0000);
                break;

            case PCI_CLASS_BRIDGE_CARDBUS:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0x00);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                break;

            default:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_1, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_2, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_3, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_4, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_5, 0x00000000);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_ROM_ADDRESS, 0x00000000);
                break;
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoScan
** ��������: ɨ������
** �䡡��  : hCtrl      ���������
**           puiSubBus  ������Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoScan (PCI_CTRL_HANDLE  hCtrl, UINT32 *puiSubBus)
{
    INT                 iRet;                                           /* �������                     */
    PCI_AUTO_HANDLE     hPciAuto;                                       /* �Զ����ÿ��ƾ��             */
    UINT32              uiSubBus = 0;                                   /* ������                       */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    hPciAuto = &hCtrl->PCI_tAutoConfig;
    if (!hPciAuto->PCIAUTO_iConfigEn) {                                 /* �Զ����ý���                 */
        return  (ERROR_NONE);
    }

    if (hPciAuto->PCIAUTO_uiFirstBusNo > hPciAuto->PCIAUTO_uiCurrentBusNo) {
        hPciAuto->PCIAUTO_uiCurrentBusNo = hPciAuto->PCIAUTO_uiFirstBusNo;
    }

    iRet = API_PciAutoCtrlInit(hCtrl);                                  /* ��ʼ���������Զ����ò���     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /* ɨ���������豸               */
    iRet = API_PciAutoBusScan(hCtrl, hPciAuto->PCIAUTO_uiCurrentBusNo, &uiSubBus);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiSubBus) {
        *puiSubBus = uiSubBus;                                          /* ����������                   */
    }

    hPciAuto->PCIAUTO_uiLastBusNo = uiSubBus;                           /* ���½������ߺ�               */

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "scan bus no %02x.\n", uiSubBus);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoBusScan
** ��������: ɨ������
** �䡡��  : hCtrl      ���������
**           iBus       ���ߺ�
**           puiSubBus  ������Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoBusScan (PCI_CTRL_HANDLE  hCtrl, INT  iBus, UINT32 *puiSubBus)
{
    INT                     iRet;                                       /* �������                     */
    PCI_AUTO_HANDLE         hPciAuto = &hCtrl->PCI_tAutoConfig;         /* �Զ����ÿ��ƾ��             */
    PCI_AUTO_DEV_HANDLE     hAutoDev;                                   /* �Զ������豸���             */
    INT                     iDev;                                       /* �豸��                       */
    INT                     iFunc;                                      /* ���ܺ�                       */
    UINT32                  uiSubBus;                                   /* �����ߺ�                     */
    UINT32                  uiMulti;                                    /* �๦�ܱ�ʶ                   */
    UINT16                  usVendor;                                   /* ������Ϣ                     */
    UINT16                  usDevice;                                   /* �豸��Ϣ                     */
    UINT16                  usClass;                                    /* ����Ϣ                       */
    UINT8                   ucHeaderType;                               /* �豸ͷ����                   */
    UINT32                  uiBusNum = 0;                               /* ��������                     */

    uiSubBus = iBus;
    uiMulti = 0;

    for (hAutoDev  = PCI_AUTO_BDF(iBus, 0, 0);
         hAutoDev <= PCI_AUTO_BDF(iBus, PCI_MAX_SLOTS - 1, PCI_MAX_FUNCTIONS - 1);
         hAutoDev += PCI_AUTO_BDF(0, 0, 1)) {                           /* ������ǰ�����ϵ��豸         */
        iDev  = PCI_AUTO_DEV(hAutoDev);
        iFunc = PCI_AUTO_FUNC(hAutoDev);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev scan %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        iRet = __pciAutoDevSkip(hCtrl, hAutoDev);                       /* �Ƿ�����豸                 */
        if (iRet == ERROR_NONE) {
            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev skip %02x:%02x.%01x.\n", iBus, iDev, iFunc);
            continue;
        }

        if ((PCI_AUTO_FUNC(hAutoDev)) && (!uiMulti)) {                  /* �������豸                   */
            continue;
        }

        API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
        if ((usVendor == 0xffff) ||
            (usVendor == 0x0000)) {                                     /* �豸��Ч                     */
            continue;
        }

        if (!PCI_AUTO_FUNC(hAutoDev)) {
            uiMulti = ucHeaderType & PCI_HEADER_MULTI_FUNC;
        }
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_DEVICE_ID, &usDevice);
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "dev fix up %02x:%02x.%01x device 0x%04x class 0x%04x.\n",
                     iBus, iDev, iFunc, usDevice, usClass);

        if (hPciAuto->PCIAUTO_pfuncDevFixup) {                          /* ����ָ���豸                 */
            hPciAuto->PCIAUTO_pfuncDevFixup(hCtrl, hAutoDev, usVendor, usDevice, usClass);
        }

        API_PciAutoDeviceConfig(hCtrl, hAutoDev, &uiBusNum);            /* ���������豸                 */
        uiSubBus = __MAX(uiBusNum, uiSubBus);

        if (hPciAuto->PCIAUTO_pfuncDevIrqFixup) {                       /* ����ָ���豸�ж�             */
            hPciAuto->PCIAUTO_pfuncDevIrqFixup(hCtrl, hAutoDev);
        }
    }

    if (puiSubBus) {
        *puiSubBus = uiSubBus;                                          /* �������ߺ�                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoPostScanBridgeSetup
** ��������: �Զ�����ָ���������ϵ����豸
** �䡡��  : hCtrl      ���������
**           hAutoDev   �豸���
**           iSubBus    �����ߺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoPostScanBridgeSetup (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev, INT  iSubBus)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����ÿ��ƾ��             */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* ���ߺ�                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* �豸��                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* ���ܺ�                       */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE ��Դ                     */
    UINT8                   ucAddr;                                     /* 8 λ��ַ                     */
    UINT16                  usAddr;                                     /* 16 λ��ַ                    */
    UINT32                  uiAddr;                                     /* 32 λ��ַ                    */
    UINT16                  usPrefechable64;                            /* 64 λԤȡ��ʶ                */
    UINT8                   ucPri;                                      /* PRI ����                     */
    UINT8                   ucSec;                                      /* SEC ����                     */
    UINT8                   ucSub;                                      /* SUB ����                     */


    if ((!hCtrl) || (iSubBus < 0)) {                                    /* ������Ч                     */
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ�Զ����ò���
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    /*
     *  ����·�ɲ���
     */
    API_PciConfigOutByte(iBus, iDev, iFunc,
                         PCI_SUBORDINATE_BUS, iSubBus - hPciAuto->PCIAUTO_uiFirstBusNo);

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "post scan bridge %02x:%02x.%01x first bus 0x%02x sub bus 0x%02x.\n",
                 iBus, iDev, iFunc, hPciAuto->PCIAUTO_uiFirstBusNo, iSubBus);

    API_PciConfigInByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, &ucPri);
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, &ucSec);
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, &ucSub);
    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "post scan bridge %02x:%02x.%01x pri 0x%02x sec 0x%02x sub 0x%02x.\n",
                 iBus, iDev, iFunc, ucPri, ucSec, ucSub);

    /*
     *  ������Դ����
     */
    if (hRegionMem) {
        API_PciAutoRegionAlign(hRegionMem, 0x100000);
        usAddr = (hRegionMem->PCIAUTOREG_addrBusLower - 1) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_LIMIT, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x memory limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionMem->PCIAUTOREG_addrBusLower - 1);
    }

    if (hRegionPre) {
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, &usPrefechable64);
        usPrefechable64 &= PCI_PREF_RANGE_TYPE_MASK;

        API_PciAutoRegionAlign(hRegionPre, 0x100000);
        usAddr = (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x pref memory limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionPre->PCIAUTOREG_addrBusLower - 1);

        if (usPrefechable64 == PCI_PREF_RANGE_TYPE_64) {
            uiAddr = (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 32;
            API_PciConfigOutDword(iBus, iDev, iFunc, PCI_PREF_LIMIT_UPPER32, uiAddr);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x pref memory limit upper 0x%qx.\n",
                         iBus, iDev, iFunc, (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 32);
        }
    }

    if (hRegionIo) {
        API_PciAutoRegionAlign(hRegionIo, 0x1000);
        ucAddr = ((hRegionIo->PCIAUTOREG_addrBusLower - 1) & 0x0000f000) >> 8;
        API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_LIMIT, ucAddr);
        usAddr = ((hRegionIo->PCIAUTOREG_addrBusLower - 1) & 0xffff0000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_IO_LIMIT_UPPER16, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x io limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionIo->PCIAUTOREG_addrBusLower - 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoPreScanBridgeSetup
** ��������: �Զ�����ָ���������ϵ����豸
** �䡡��  : hCtrl      ���������
**           hAutoDev   �豸���
**           iSubBus    �����ߺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoPreScanBridgeSetup (PCI_CTRL_HANDLE      hCtrl,
                                    PCI_AUTO_DEV_HANDLE  hAutoDev,
                                    INT                  iSubBus)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����ÿ��ƾ��             */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* ���ߺ�                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* �豸��                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* ���ܺ�                       */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE ��Դ                     */
    UINT16                  usCmdStatus;                                /* ���Ƽ�״̬��Ϣ               */
    UINT16                  usPrefechable;                              /* Ԥȡ��ʶ                     */
    UINT8                   ucAddr;                                     /* 8 λ��ַ                     */
    UINT16                  usAddr;                                     /* 16 λ��ַ                    */
    UINT32                  uiAddr;                                     /* 32 λ��ַ                    */

    if ((!hCtrl) || (iSubBus < 0)) {
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ�Զ����ò���
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    /*
     *  ���ÿ��Ʋ�����Ԥȡ����ַ
     */
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_COMMAND, &usCmdStatus);
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, &usPrefechable);
    usPrefechable &= PCI_PREF_RANGE_TYPE_MASK;

    /*
     *  ����·�ɲ���
     */
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, iBus - hPciAuto->PCIAUTO_uiFirstBusNo);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, iSubBus - hPciAuto->PCIAUTO_uiFirstBusNo);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0xff);

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "%02x:%02x.%01x pre scan bridge first bus 0x%02x sub bus 0x%02x prefechable 0x%02x.\n",
                 iBus, iDev, iFunc, hPciAuto->PCIAUTO_uiFirstBusNo, iSubBus, usPrefechable);

    /*
     *  ������Դ����
     */
    if (hRegionMem) {
        API_PciAutoRegionAlign(hRegionMem, 0x100000);
        usAddr = (hRegionMem->PCIAUTOREG_addrBusLower & 0xfff00000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_BASE, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x memory base 0x%qx.\n",
                     iBus, iDev, iFunc, (UINT64)hRegionMem->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_MEMORY;
    }

    if (hRegionPre) {
        API_PciAutoRegionAlign(hRegionPre, 0x100000);
        usAddr = (hRegionPre->PCIAUTOREG_addrBusLower & 0xfff00000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, usAddr);
        if (usPrefechable == PCI_PREF_RANGE_TYPE_64) {
            uiAddr = hRegionPre->PCIAUTOREG_addrBusLower >> 32;
            API_PciConfigOutDword(iBus, iDev, iFunc, PCI_PREF_BASE_UPPER32, uiAddr);
        }

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x %s pref memory base 0x%qx.\n",
                     iBus, iDev, iFunc,
                     (usPrefechable == PCI_PREF_RANGE_TYPE_64) ? "64bit" : "32bit",
                     (UINT64)hRegionPre->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_MEMORY;
    
    } else {
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, 0x1000);
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, 0x0);
        if (usPrefechable == PCI_PREF_RANGE_TYPE_64) {
            API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_BASE_UPPER32, 0x0);
            API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_LIMIT_UPPER32, 0x0);
        }
    }

    if (hRegionIo) {
        API_PciAutoRegionAlign(hRegionIo, 0x1000);
        ucAddr = (hRegionIo->PCIAUTOREG_addrBusLower & 0x0000f000) >> 8;
        API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_BASE, ucAddr);
        usAddr = (hRegionIo->PCIAUTOREG_addrBusLower & 0xffff0000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_IO_BASE_UPPER16, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x io base 0x%qx.\n",
                     iBus, iDev, iFunc, (UINT64)hRegionIo->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_IO;
    }

    API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, usCmdStatus | PCI_COMMAND_MASTER);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoDeviceSetup
** ��������: �Զ�����ָ���������ϵ��豸
** �䡡��  : hCtrl              ���������
**           hAutoDev           �豸���
**           uiBarNum           ��Դ��Ŀ
**           hRegionIo          IO ��Դ
**           hRegionMem         MMEM ��Դ
**           hRegionPrefetch    ��Ԥȡ��Դ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoDeviceSetup (PCI_CTRL_HANDLE         hCtrl,
                             PCI_AUTO_DEV_HANDLE     hAutoDev,
                             UINT32                  uiBarNum,
                             PCI_AUTO_REGION_HANDLE  hRegionIo,
                             PCI_AUTO_REGION_HANDLE  hRegionMem,
                             PCI_AUTO_REGION_HANDLE  hRegionPrefetch)
{
    INT                     iRet;                                       /* �������                     */
    REGISTER INT            uiBar;                                      /* BAR ����                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����þ��                 */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* ���ߺ�                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* �豸��                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* ���ܺ�                       */
    UINT32                  uiBarResponse;                              /* ��Դ��Ϣ                     */
    UINT32                  uiBarResponseUpper;                         /* ��Դ��Ϣ�ߵ�ַ��Ϣ           */
    pci_addr_t              addrBarAddr;                                /* BAR ��ַ                     */
    pci_size_t              stBarSize;                                  /* BAR ��С                     */
    UINT16                  usClass;                                    /* ����Ϣ                       */
    UINT16                  usCmdStatus;                                /* ������״̬                   */
    UINT8                   ucHeaderType;                               /* �豸ͷ����                   */
    INT                     iBarIndex = 0;                              /* BAR ����                     */
    INT                     iIndex = 0;                                 /* ��������                     */
    UINT                    uiRomAddrIndex;                             /* ROM ��ַ����                 */
    pci_addr_t              addrBarValue;                               /* BAR ֵ                       */
    PCI_AUTO_REGION_HANDLE  hBarRegion;                                 /* ��Դ���                     */
    INT                     iMem64En;                                   /* 64 λ��ַʹ�ܱ�ʶ            */
    UINT8                   ucCacheLineSize;                            /* ���ٻ����д�С               */
    UINT8                   ucLatencyTimer;                             /* ʱ�����                     */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    hPciAuto = &hCtrl->PCI_tAutoConfig;

    API_PciConfigInWord(iBus, iDev, iFunc, PCI_COMMAND, &usCmdStatus);  /* ��ȡ��ʼ������Ϣ             */
    usCmdStatus = (usCmdStatus & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) | PCI_COMMAND_MASTER;

    /*
     *  ����������Դ����������
     */
    iIndex = 0;
    iBarIndex = 0;
    for (uiBar = PCI_BASE_ADDRESS_0; uiBar < (PCI_BASE_ADDRESS_0 + (uiBarNum * 4)); uiBar += 4) {
        API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, 0xffffffff);
        API_PciConfigInDword(iBus, iDev, iFunc, uiBar, &uiBarResponse);
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x BAR %d response=0x%x.\n", iBus, iDev, iFunc, iIndex, uiBarResponse);
        iIndex++;
        if (!uiBarResponse) {
            continue;
        }

        iMem64En = 0;

        if (uiBarResponse & PCI_BASE_ADDRESS_SPACE) {
            stBarSize  = (pci_size_t)(((~(uiBarResponse & PCI_BASE_ADDRESS_IO_MASK)) & 0xffff) + 1);
            hBarRegion = hRegionIo;

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x BAR %d, I/O, size=0x%qx.\n",
                         iBus, iDev, iFunc, iBarIndex, (UINT64)stBarSize);
        
        } else {
            if ((uiBarResponse & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) {
                API_PciConfigOutDword(iBus, iDev, iFunc, uiBar + 4, 0xffffffff);
                API_PciConfigInDword(iBus, iDev, iFunc, uiBar + 4, &uiBarResponseUpper);
                addrBarAddr = ((pci_addr_t)uiBarResponseUpper << 32) | uiBarResponse;
                stBarSize = ~(addrBarAddr & PCI_BASE_ADDRESS_MEM_MASK) + 1;
                iMem64En = 1;
            
            } else {
                stBarSize = (UINT32)(~(uiBarResponse & PCI_BASE_ADDRESS_MEM_MASK) + 1);
            }

            if ((hRegionPrefetch) &&
                (uiBarResponse & PCI_BASE_ADDRESS_MEM_PREFETCH)) {
                hBarRegion = hRegionPrefetch;
            
            } else {
                hBarRegion = hRegionMem;
            }

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x BAR %d, type %s %s, size=0x%qx.\n",
                         iBus, iDev, iFunc, iBarIndex,
                         iMem64En ? "64bit" : "32bit",
                         (hBarRegion == hRegionPrefetch) ? "Prf" : "Mem", (UINT64)stBarSize);
        }

        iRet = API_PciAutoRegionAllocate(hBarRegion, stBarSize, &addrBarValue);
        if (iRet != PX_ERROR) {
            API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, (UINT32)addrBarValue);
            if (iMem64En) {
                uiBar += 4;
                API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, (UINT32)(addrBarValue >> 32));
            }
        }

        usCmdStatus |= (uiBarResponse & PCI_BASE_ADDRESS_SPACE) ? PCI_COMMAND_IO : PCI_COMMAND_MEMORY;

        iBarIndex++;
    }

    /*
     *  ���� ROM ����
     */
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
    ucHeaderType &= 0x7f;
    if (ucHeaderType != PCI_HEADER_TYPE_CARDBUS) {
        uiRomAddrIndex = (ucHeaderType == PCI_HEADER_TYPE_NORMAL) ? PCI_ROM_ADDRESS : PCI_ROM_ADDRESS1;
        API_PciConfigOutDword(iBus, iDev, iFunc, uiRomAddrIndex, 0xfffffffe);
        API_PciConfigInDword(iBus, iDev, iFunc, uiRomAddrIndex, &uiBarResponse);
        if (uiBarResponse) {
            stBarSize = -(uiBarResponse & ~1);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "ROM, size = 0x%qx.\n", (UINT64)stBarSize);

            iRet = API_PciAutoRegionAllocate(hRegionMem, stBarSize, &addrBarValue);
            if (iRet != PX_ERROR) {
                API_PciConfigOutDword(iBus, iDev, iFunc, uiRomAddrIndex, addrBarValue);
            }
            usCmdStatus |= PCI_COMMAND_MEMORY;
        }
    }

    API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);
    if (usClass == PCI_CLASS_DISPLAY_VGA) {
        usCmdStatus |= PCI_COMMAND_IO;
    }

    API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, usCmdStatus);
    if (!hPciAuto->PCIAUTO_ucCacheLineSize) {
        ucCacheLineSize = PCI_AUTO_CACHE_LINE_SIZE;
    
    } else {
        ucCacheLineSize = hPciAuto->PCIAUTO_ucCacheLineSize;
    }
    if (!hPciAuto->PCIAUTO_ucLatencyTimer) {
        ucLatencyTimer = PCI_AUTO_LATENCY_TIMER;
    
    } else {
        ucLatencyTimer = hPciAuto->PCIAUTO_ucLatencyTimer;
    }
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_CACHE_LINE_SIZE, ucCacheLineSize);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_LATENCY_TIMER, ucLatencyTimer);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoRegionAllocate
** ��������: ������Դ����
** �䡡��  : hRegion    ��Դ�������
**           stSize     ��Դ��С
**           addrBar    ��Դ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoRegionAllocate (PCI_AUTO_REGION_HANDLE  hRegion, pci_size_t stSize, pci_addr_t *paddrBar)
{
    pci_addr_t      addrAddr;

    if (!hRegion) {
        if (paddrBar) {
            *paddrBar = (pci_addr_t)-1;
        }
        return  (PX_ERROR);
    }

    /*
     *  ���ж��봦��
     */
    addrAddr = ((hRegion->PCIAUTOREG_addrBusLower - 1) | (stSize - 1)) + 1;
    if ((addrAddr - hRegion->PCIAUTOREG_addrBusStart + stSize) > hRegion->PCIAUTOREG_stSize) {
        if (paddrBar) {
            *paddrBar = (pci_addr_t)-1;
        }
        return  (PX_ERROR);
    }

    hRegion->PCIAUTOREG_addrBusLower = addrAddr + stSize;               /* ���µ�ǰ��ַ��Ϣ             */
    if (paddrBar) {
        *paddrBar = addrAddr;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "region allocate address = 0x%qx bus_lower = 0x%qx.\n",
                 (UINT64)addrAddr, (UINT64)hRegion->PCIAUTOREG_addrBusLower);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoRegionAlign
** ��������: �Զ����õ���Դ��������
** �䡡��  : hRegion    ��Դ�������
**           stSize     �����С
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_PciAutoRegionAlign (PCI_AUTO_REGION_HANDLE  hRegion, pci_size_t stSize)
{
    hRegion->PCIAUTOREG_addrBusLower = ((hRegion->PCIAUTOREG_addrBusLower - 1) | (stSize - 1)) + 1;
}
/*********************************************************************************************************
** ��������: API_PciAutoRegionInit
** ��������: �Զ����õ���Դ������ʼ��
** �䡡��  : hRegion    ��Դ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_PciAutoRegionInit (PCI_AUTO_REGION_HANDLE  hRegion)
{
    PCHAR       pcRegionName = LW_NULL;

    if (!hRegion) {
        return;
    }

    /*
     *  Ϊ�˱��� 0 ��ַ�Ĵ���, ʹ�� 0x1000 ����.
     */
    if (hRegion->PCIAUTOREG_addrBusStart) {
        hRegion->PCIAUTOREG_addrBusLower = hRegion->PCIAUTOREG_addrBusStart;
    
    } else {
        hRegion->PCIAUTOREG_addrBusLower = 0x1000;
    }

    /*
     *  ��ȡ��Դ����
     */
    switch (hRegion->PCIAUTOREG_ulFlags) {

    case PCI_AUTO_REGION_IO:
        pcRegionName = "I/O             ";
        break;

    case PCI_AUTO_REGION_MEM:
        pcRegionName = "Memory          ";
        break;

    case PCI_AUTO_REGION_MEM | PCI_AUTO_REGION_PREFETCH:
        pcRegionName = "Prefetchable Mem";
        break;

    default:
        pcRegionName = "xxxx";
        break;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "Bus %s region [0x%qx-0x%qx] Physical Memory [0x%qx-0x%qx].\n",
                 pcRegionName,
                 (UINT64)hRegion->PCIAUTOREG_addrBusStart,
                 (UINT64)(hRegion->PCIAUTOREG_addrBusStart + hRegion->PCIAUTOREG_stSize - 1),
                 (UINT64)hRegion->PCIAUTOREG_addrPhyStart,
                 (UINT64)(hRegion->PCIAUTOREG_addrPhyStart + hRegion->PCIAUTOREG_stSize - 1));
}
/*********************************************************************************************************
** ��������: API_PciAutoDeviceConfig
** ��������: �Զ�����ָ���������ϵ��豸
** �䡡��  : hCtrl      ���������
**           hAutoDev   �豸���
**           puiSubBus  ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoDeviceConfig (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev, UINT32 *puiSubBus)
{
    INT                     iRet;                                       /* �������                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����ÿ��ƾ��             */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM ��Դ                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE ��Դ                     */
    UINT32                  uiSubBus = PCI_AUTO_BUS(hAutoDev);          /* �����ߺ�                     */
    UINT32                  uiBusNum;                                   /* ������Ŀ                     */
    UINT16                  usVendor;                                   /* ������Ϣ                     */
    UINT16                  usClass;                                    /* ����Ϣ                       */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* ���ߺ�                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* �豸��                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* ���ܺ�                       */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ�Զ����ò���
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    iRet = API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
    if ((iRet != ERROR_NONE) ||
        (usVendor == 0xffff) ||
        (usVendor == 0x0000)) {                                         /* �豸��Ч                     */
        return  (PX_ERROR);
    }

    /*
     *  ͨ���豸���ͽ����豸����
     */
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);

    switch (usClass) {

    case PCI_CLASS_BRIDGE_PCI:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found P2P bridge, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 2, hRegionIo, hRegionMem, hRegionPre);

        hPciAuto->PCIAUTO_uiCurrentBusNo++;
        API_PciAutoPreScanBridgeSetup(hCtrl, hAutoDev, hPciAuto->PCIAUTO_uiCurrentBusNo);
        API_PciAutoBusScan(hCtrl, hPciAuto->PCIAUTO_uiCurrentBusNo, &uiBusNum);
        uiSubBus = __MAX(uiBusNum, uiSubBus);
        API_PciAutoPostScanBridgeSetup(hCtrl, hAutoDev, uiSubBus);
        uiSubBus = hPciAuto->PCIAUTO_uiCurrentBusNo;
        break;

    case PCI_CLASS_BRIDGE_CARDBUS:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found P2CardBus bridge, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 0, hRegionIo, hRegionMem, hRegionPre);
        hPciAuto->PCIAUTO_uiCurrentBusNo++;
        break;

    default:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found Normal devices, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 6, hRegionIo, hRegionMem, hRegionPre);
        break;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "auto config sub bus 0x%02x.\n", uiSubBus);

    if (puiSubBus) {
        *puiSubBus = uiSubBus;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoCtrlInit
** ��������: �������Զ����õĳ�ʼ��
** �䡡��  : hCtrl      ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoCtrlInit (PCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT            i;                                          /* ѭ������                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����ÿ��ƾ��             */
    PCI_AUTO_REGION_HANDLE  hRegion = LW_NULL;                          /* ��Դ���ƾ��                 */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ�Զ����ò���
     */
    hPciAuto = &hCtrl->PCI_tAutoConfig;
    hPciAuto->PCIAUTO_hRegionIo  = LW_NULL;
    hPciAuto->PCIAUTO_hRegionMem = LW_NULL;
    hPciAuto->PCIAUTO_hRegionPre = LW_NULL;
    hPciAuto->PCIAUTO_pvPriv     = (PVOID)hCtrl;

    if (!hPciAuto->PCIAUTO_iConfigEn) {
        return  (ERROR_NONE);
    }

    /*
     *  ��ȡ��Դ��Ϣ
     */
    for (i = 0; i < hPciAuto->PCIAUTO_uiRegionCount; i++) {
        hRegion = &hPciAuto->PCIAUTO_tRegion[i];

        switch (hRegion->PCIAUTOREG_ulFlags) {

        case PCI_AUTO_REGION_IO:
            if ((!hPciAuto->PCIAUTO_hRegionIo) ||
                (hPciAuto->PCIAUTO_hRegionIo->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionIo = hRegion;
            }
            break;

        case PCI_AUTO_REGION_MEM:
            if ((!hPciAuto->PCIAUTO_hRegionMem) ||
                (hPciAuto->PCIAUTO_hRegionMem->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionMem = hRegion;
            }
            break;

        case (PCI_AUTO_REGION_MEM | PCI_AUTO_REGION_PREFETCH):
            if ((!hPciAuto->PCIAUTO_hRegionPre) ||
                (hPciAuto->PCIAUTO_hRegionPre->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionPre = hRegion;
            }
            break;

        default:
            break;
        }
    }

    /*
     *  ������Դ��Ϣ
     */
    if (hPciAuto->PCIAUTO_hRegionIo) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionIo);
    }
    if (hPciAuto->PCIAUTO_hRegionMem) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionMem);
    }
    if (hPciAuto->PCIAUTO_hRegionPre) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionPre);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciAutoCtrlRegionSet
** ��������: ���ÿ������Զ����õ���Դ
** �䡡��  : hCtrl          ���������
**           uiIndex        ��Դ��������
**           addrBusStart   PCI ��������ʼ��ַ
**           addrPhyStart   ��ʼ�����ַ
**           stSize         �ռ��С
**           ulFlags        ��Դ���� PCI_AUTO_REGION_MEM  PCI_AUTO_REGION_IO  PCI_AUTO_REGION_PREFETCH
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciAutoCtrlRegionSet (PCI_CTRL_HANDLE  hCtrl,
                               UINT             uiIndex,
                               pci_bus_addr_t   addrBusStart,
                               pci_addr_t       addrPhyStart,
                               pci_size_t       stSize,
                               ULONG            ulFlags)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* �Զ����ÿ��ƾ��             */
    PCI_AUTO_REGION_HANDLE  hRegion = LW_NULL;                          /* ��Դ���                     */

    if ((!hCtrl) ||
        (uiIndex >= PCI_AUTO_REGION_MAX)) {                             /* �������������Դ������Ч     */
        PCI_AUTO_LOG(PCI_AUTO_LOG_ERR,
                     "set region failed ctrl handle %px index %d [%d-%d].\n",
                     hCtrl, uiIndex, PCI_AUTO_REGION_INDEX_0, (PCI_AUTO_REGION_MAX - 1));

        return  (PX_ERROR);
    }

    /*
     *  ����ָ����Դ
     */
    hPciAuto = &hCtrl->PCI_tAutoConfig;
    hRegion  = &hPciAuto->PCIAUTO_tRegion[uiIndex];
    hRegion->PCIAUTOREG_addrBusStart = addrBusStart;
    hRegion->PCIAUTOREG_ulFlags      = ulFlags;
    hRegion->PCIAUTOREG_stSize       = stSize;
    hRegion->PCIAUTOREG_addrPhyStart = addrPhyStart;

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
