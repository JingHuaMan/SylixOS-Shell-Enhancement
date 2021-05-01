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
** ��   ��   ��: ahciLib.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 01 �� 04 ��
**
** ��        ��: AHCI ������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "linux/compat.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
/*********************************************************************************************************
  �Ĵ�����Ϣ
*********************************************************************************************************/
typedef struct {
    UINT8       AHR_ucOffset;
    PCHAR       AHR_pcName;
} AHCI_HOST_REG;

typedef struct {
    UINT8       APR_ucOffset;
    PCHAR       APR_pcName;
} AHCI_PORT_REG;

static AHCI_HOST_REG    _G_pahrAhciHostRegTable[] = {
    {AHCI_CAP,          "Host Capabilities (AHCI_CAP)"                                          },
    {AHCI_GHC,          "Global Host Control (AHCI_GHC)"                                        },
    {AHCI_IS,           "Interrupt Status (AHCI_IS)"                                            },
    {AHCI_PI,           "Ports Implemented (AHCI_PI)"                                           },
    {AHCI_VS,           "Version (AHCI_VS)",                                                    },
    {AHCI_CCC_CTL,      "Command Completion Coalescing Control (AHCI_CCC_CTL)"                  },
    {AHCI_CCC_PORTS,    "Command Completion Coalsecing Ports (AHCI_CCC_PORTS)"                  },
    {AHCI_EM_LOC,       "Enclosure Management Location (AHCI_EM_LOC)"                           },
    {AHCI_EM_CTL,       "Enclosure Management Control (AHCI_EM_CTL)"                            },
    {AHCI_CAP2,         "Host Capabilities Extended (AHCI_CAP2)"                                },
    {AHCI_BOHC,         "BIOS/OS Handoff Control and Status (AHCI_BOHC)"                        },
};

static AHCI_PORT_REG    _G_pahrAhciPortRegTable[] = {
    {AHCI_PxCLB,        "Port x Command List Base Address (AHCI_PxCLB)"                         },
    {AHCI_PxCLBU,       "Port x Command List Base Address Upper 32-Bits (AHCI_PxCLBU)"          },
    {AHCI_PxFB,         "Port x FIS Base Address (AHCI_PxFB)"                                   },
    {AHCI_PxFBU,        "Port x FIS Base Address Upper 32-Bits (AHCI_PxFBU)"                    },
    {AHCI_PxIS,         "Port x Interrupt Status (AHCI_PxIS)"                                   },
    {AHCI_PxIE,         "Port x Interrupt Enable (AHCI_PxIE)"                                   },
    {AHCI_PxCMD,        "Port x Command and Status (AHCI_PxCMD)"                                },
    {AHCI_PxTFD,        "Port x Task File Data (AHCI_PxTFD)"                                    },
    {AHCI_PxSIG,        "Port x Signature (AHCI_PxSIG)"                                         },
    {AHCI_PxSSTS,       "Port x Serial ATA Status (SCR0: SStatus) (AHCI_PxSSTS)"                },
    {AHCI_PxSCTL,       "Port x Serial ATA Control (SCR2: SControl) (AHCI_PxSCTL)"              },
    {AHCI_PxSERR,       "Port x Serial ATA Error (SCR1: SError) (AHCI_PxSERR)"                  },
    {AHCI_PxSACT,       "Port x Serial ATA Active (SCR3: SActive) (AHCI_PxSACT)"                },
    {AHCI_PxCI,         "Port x Command Issue (AHCI_PxCI)"                                      },
    {AHCI_PxSNTF,       "Port x Serial ATA Notification (SCR4: SNotification) (AHCI_PxSNTF)"    },
    {AHCI_PxFBS,        "Port x FIS-based Switching Control (AHCI_PxFBS)"                       },
    {AHCI_PxDEVSLP,     "Port x Device Sleep (AHCI_PxDEVSLP)"                                   },
    {AHCI_PxVS,         "Port x Vendor Specific (AHCI_PxVS)"                                    },
};
/*********************************************************************************************************
  ����ģʽ��Ϣ
*********************************************************************************************************/
typedef struct {
    UINT8       ADWM_ucIndex;
    PCHAR       ADWM_pcName;
} AHCI_DRIVE_WORK_MODE;

static AHCI_DRIVE_WORK_MODE     _G_padwmAhciDriveWorkModeTable[] = {
    {AHCI_PIO_DEF_W,    "Default PIO"                                                           },
    {AHCI_PIO_DEF_WO,   "Default PIO No IORDY"                                                  },
    {AHCI_PIO_W_0,      "PIO 0"                                                                 },
    {AHCI_PIO_W_1,      "PIO 1"                                                                 },
    {AHCI_PIO_W_2,      "PIO 2"                                                                 },
    {AHCI_PIO_W_3,      "PIO 3",                                                                },
    {AHCI_PIO_W_4,      "PIO 4"                                                                 },
    {AHCI_DMA_SINGLE_0, "Single DMA 0"                                                          },
    {AHCI_DMA_SINGLE_1, "Single DMA 1"                                                          },
    {AHCI_DMA_SINGLE_2, "Single DMA 2"                                                          },
    {AHCI_DMA_MULTI_0,  "Multi DMA 0"                                                           },
    {AHCI_DMA_MULTI_1,  "Multi DMA 1"                                                           },
    {AHCI_DMA_MULTI_2,  "Multi DMA 2"                                                           },
    {AHCI_DMA_ULTRA_0,  "Ultra DMA 0"                                                           },
    {AHCI_DMA_ULTRA_1,  "Ultra DMA 1"                                                           },
    {AHCI_DMA_ULTRA_2,  "Ultra DMA 2"                                                           },
    {AHCI_DMA_ULTRA_3,  "Ultra DMA 3"                                                           },
    {AHCI_DMA_ULTRA_4,  "Ultra DMA 4"                                                           },
    {AHCI_DMA_ULTRA_5,  "Ultra DMA 5"                                                           },
    {AHCI_DMA_ULTRA_6,  "Ultra DMA 6"                                                           },
};
/*********************************************************************************************************
** ��������: ahciIdString
** ��������: �� ID ֵת��Ϊ�ַ�����Ϣ
** �䡡��  : pusId      ID ����
**           pcBuff     �ַ�������
**           uiLen      �����С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PCHAR  ahciIdString (const UINT16 *pusId, PCHAR  pcBuff, UINT32  uiLen)
{
    REGISTER INT    i;
    UINT8           ucChar   = 0;                                       /* �ֽ�����                     */
    INT             iOffset  = 0;                                       /* ��ƫ��                       */
    PCHAR           pcString = pcBuff;                                  /* �ַ���������                 */

    /*
     *  ��������ת��Ϊ�ַ�������
     */
    iOffset  = 0;
    pcString = pcBuff;
    
    for (i = 0; i < (uiLen - 1); i += 2) {
        ucChar = (UINT8)(pusId[iOffset] >> 8);
        *pcString = ucChar;
        pcString += 1;

        ucChar = (UINT8)(pusId[iOffset] & 0xff);
        *pcString = ucChar;
        pcString += 1;
        iOffset  += 1;
    }

    /*
     *  ɾ����Ч�ո���Ϣ
     */
    pcString = pcBuff + lib_strnlen(pcBuff, uiLen - 1);
    while ((pcString > pcBuff) && (pcString[-1] == ' ')) {
        pcString--;
    }
    *pcString = '\0';

    iOffset  = 0;
    pcString = pcBuff;
    
    for (i = 0; i < uiLen; i++) {
        if (pcString[iOffset] == ' ') {
            pcString += 1;
        } else {
            break;
        }
    }

    return  (pcString);
}
/*********************************************************************************************************
** ��������: API_AhciDriveSerialInfoGet
** ��������: ���к���Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciDriveSerialInfoGet (AHCI_DRIVE_HANDLE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    AHCI_PARAM_HANDLE   hParam;                                         /* �������                     */
    PCHAR               pcSerial;                                       /* �豸����                     */
    CHAR                cSerial[21] = { 0 };                            /* ���кŻ�����                 */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->AHCIDRIVE_tParam;
    lib_bzero(&cSerial[0], 21);
    pcSerial = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucSerial[0], &cSerial[0], 21);
    lib_strncpy(pcBuf, pcSerial, __MIN(stLen, lib_strlen(pcSerial) + 1));

    return  (pcSerial);
}
/*********************************************************************************************************
** ��������: API_AhciDriveFwRevInfoGet
** ��������: �̼��汾����Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciDriveFwRevInfoGet (AHCI_DRIVE_HANDLE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    AHCI_PARAM_HANDLE   hParam;                                         /* �������                     */
    PCHAR               pcFirmware;                                     /* �̼��汾                     */
    CHAR                cFirmware[9] = { 0 };                           /* �̼��汾������               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->AHCIDRIVE_tParam;
    lib_bzero(&cFirmware[0], 9);
    pcFirmware = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucFwRev[0], &cFirmware[0], 9);
    lib_strncpy(pcBuf, pcFirmware, __MIN(stLen, lib_strlen(pcFirmware) + 1));

    return  (pcFirmware);
}
/*********************************************************************************************************
** ��������: API_AhciDriveModelInfoGet
** ��������: �豸��ϸ��Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciDriveModelInfoGet (AHCI_DRIVE_HANDLE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    AHCI_PARAM_HANDLE   hParam;                                         /* �������                     */
    PCHAR               pcProduct;                                      /* ��Ʒ��Ϣ                     */
    CHAR                cProduct[41] = { 0 };                           /* ��Ʒ��Ϣ������               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->AHCIDRIVE_tParam;
    lib_bzero(&cProduct[0], 41);
    pcProduct = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucModel[0], &cProduct[0], 41);
    lib_strncpy(pcBuf, pcProduct, __MIN(stLen, lib_strlen(pcProduct) + 1));

    return  (pcProduct);
}
/*********************************************************************************************************
** ��������: API_AhciDriveSectorCountGet
** ��������: ��ȡ��������������
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
UINT64  API_AhciDriveSectorCountGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    AHCI_DRIVE_HANDLE   hDrive     = LW_NULL;                           /* ���������                   */
    UINT64              ullSectors = 0;                                 /* ��������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ͨ������������������       */
    if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {                          /* LBA 48 λģʽ                */
        ullSectors = hCtrl->AHCICTRL_ullLba48TotalSecs[uiDrive];
    
    } else if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) &&
               (hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] != 0) &&
               (hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] >
               (UINT32)(hDrive->AHCIDRIVE_uiCylinder *
                        hDrive->AHCIDRIVE_uiHead *
                        hDrive->AHCIDRIVE_uiSector))) {                 /* LBA ģʽ                     */
        ullSectors = hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive];
    
    } else {                                                            /* CHS ģʽ                     */
        ullSectors = hDrive->AHCIDRIVE_uiCylinder * hDrive->AHCIDRIVE_uiHead * hDrive->AHCIDRIVE_uiSector;
    }

    return  (ullSectors);
}
/*********************************************************************************************************
** ��������: API_AhciDriveWorkModeNameGet
** ��������: ��ȡ����������ģʽ����
** �䡡��  : uiIndex    ����ģʽ����
** �䡡��  : ����ģʽ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciDriveWorkModeNameGet (UINT  uiIndex)
{
    REGISTER INT    i = 0;
    UINT8           ucSize = sizeof(_G_padwmAhciDriveWorkModeTable) / sizeof(AHCI_DRIVE_WORK_MODE);

    for (i = 0; i < ucSize; i++) {
        if (_G_padwmAhciDriveWorkModeTable[i].ADWM_ucIndex == uiIndex) {
            break;
        }
    }
    if (i >= ucSize) {
        return  (LW_NULL);
    }

    return  (_G_padwmAhciDriveWorkModeTable[i].ADWM_pcName);
}
/*********************************************************************************************************
** ��������: API_AhciDriveInfoShow
** ��������: SATA ��������Ϣ
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           hParam     �������ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDriveInfoShow (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, AHCI_PARAM_HANDLE  hParam)
{
    INT                 iRet = PX_ERROR;                                /* �������                     */
    AHCI_DRV_HANDLE     hDrv;                                           /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_DEV_HANDLE     hDev = LW_NULL;                                 /* �豸���                     */
    PCHAR               pcSerial;                                       /* �豸����                     */
    PCHAR               pcFirmware;                                     /* �̼��汾                     */
    PCHAR               pcProduct;                                      /* ��Ʒ��Ϣ                     */
    CHAR                cSerial[21] = { 0 };                            /* ���кŻ�����                 */
    CHAR                cFirmware[9] = { 0 };                           /* �̼��汾������               */
    CHAR                cProduct[41] = { 0 };                           /* ��Ʒ��Ϣ������               */
    UINT64              ullValue;

    if ((!hCtrl) || (!hParam)) {
        return  (PX_ERROR);
    }

    hDrv = hCtrl->AHCICTRL_hDrv;
    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ͨ��������ÿ��������       */
    hDev = hDrive->AHCIDRIVE_hDev;
    /*
     *  ��������Ϣת��Ϊ�ַ�����Ϣ
     */
    lib_bzero(&cSerial[0], 21);
    pcSerial = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucSerial[0], &cSerial[0], 21);
    lib_bzero(&cFirmware[0], 9);
    pcFirmware = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucFwRev[0], &cFirmware[0], 9);
    lib_bzero(&cProduct[0], 41);
    pcProduct = ahciIdString((const UINT16 *)&hParam->AHCIPARAM_ucModel[0], &cProduct[0], 41);

    iRet = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT);
    if (iRet < 0) {                                                     /* ��׼�����Ч                 */
        return  (ERROR_NONE);                                           /* ֱ����ȷ����                 */
    }

    /*
     *  ��ӡ������Ϣ
     */
    printf("\nAHCI Control %d Drive %d Information >>\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
    printf("Control Name          : %s\n", hCtrl->AHCICTRL_cCtrlName);
    printf("Control Unit Index    : %d\n", hCtrl->AHCICTRL_uiUnitIndex);
    if ((hCtrl) && (hCtrl->AHCICTRL_uiCoreVer)) {
        printf("Control Core Version  : " AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_uiCoreVer));
    } else {
        printf("Control Core Version  : " "*");
    }
    printf("\n");
    printf("Driver Name           : %s\n", hCtrl->AHCICTRL_hDrv->AHCIDRV_cDrvName);
    if ((hCtrl) && (hCtrl->AHCICTRL_hDrv) && (hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer)) {
        printf("Driver Version        : " AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer));
    } else {
        printf("Driver Version        : " "*");
    }
    printf("\n");
    printf("Drive Base Addr       : %p\n", hDrive->AHCIDRIVE_pvRegAddr);
    printf("Cylinders Number      : %d\n", hDrive->AHCIDRIVE_uiCylinder);
    printf("Heads Number          : %d\n", hDrive->AHCIDRIVE_uiHead);
    printf("Sector Number         : %d\n", hDrive->AHCIDRIVE_uiSector);
    printf("MULTI                 : %s\n", hDrive->AHCIDRIVE_bMulti == LW_TRUE ? "Enable" : "Disable");
    printf("Multiple Sectors      : %d\n", hDrive->AHCIDRIVE_usMultiSector);
    printf("IORDY                 : %s\n", hDrive->AHCIDRIVE_bIordy == LW_TRUE ? "Enable" : "Disable");
    printf("LBA                   : %s\n", hDrive->AHCIDRIVE_bLba == LW_TRUE ? "Enable" : "Disable");
    printf("LBA Sectors           : %qu\n", (UINT64)hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive]);
    printf("SATA Capabilities     : 0x%04x\n", hParam->AHCIPARAM_usSataCapabilities);
    printf("SATA Add Capabilities : 0x%04x\n", hParam->AHCIPARAM_usSataAddCapabilities);
    printf("SATA Features Support : 0x%04x\n", hParam->AHCIPARAM_usSataFeaturesSupported);
    printf("SATA Features Enabled : 0x%04x\n", hParam->AHCIPARAM_usSataFeaturesEnabled);
    printf("Major Version Number  : 0x%04x\n", hParam->AHCIPARAM_usMajorRevNum);
    printf("Minor Version Number  : 0x%04x\n", hParam->AHCIPARAM_usMinorVersNum);
    printf("Ultra DMA Modes       : 0x%04x\n", hParam->AHCIPARAM_usUltraDma);
    printf("HW Reset Results      : 0x%04x\n", hParam->AHCIPARAM_usResetResult);
    printf("Physical/Logical Size : 0x%04x\n", hParam->AHCIPARAM_usPhysicalLogicalSector);
    printf("Data Set Management   : 0x%04x\n", hParam->AHCIPARAM_usDataSetManagement);
    printf("Additional Supported  : 0x%04x\n", hParam->AHCIPARAM_usAdditionalSupported);
    printf("Features Supported 0  : 0x%04x\n", hParam->AHCIPARAM_usFeaturesSupported0);
    printf("Features Supported 1  : 0x%04x\n", hParam->AHCIPARAM_usFeaturesSupported1);
    printf("Features Supported 2  : 0x%04x\n", hParam->AHCIPARAM_usFeaturesSupported2);
    printf("Features Enabled 0    : 0x%04x\n", hParam->AHCIPARAM_usFeaturesEnabled0);
    printf("Features Enabled 1    : 0x%04x\n", hParam->AHCIPARAM_usFeaturesEnabled1);
    printf("Features Enabled 2    : 0x%04x\n", hParam->AHCIPARAM_usFeaturesEnabled2);
    printf("\n");
    if (hDev) {
        printf("Cache Flush Enabled   : %s\n", (hDev->AHCIDEV_iCacheFlush) ? "Enable" : "Disable");
    } else {
        printf("Cache Flush Enabled   : %s\n", "*");
    }
    printf("\n");
    printf("S/N                   : %s\n", pcSerial);
    printf("Firmware Version      : %s\n", pcFirmware);
    printf("Product Model Number  : %s\n", pcProduct);
    printf("Device Type           : %s\n", hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA ? "ATA" : "ATAPI");
    printf("Work Mode             : %s\n", API_AhciDriveWorkModeNameGet(hDrive->AHCIDRIVE_usRwMode));
    printf("TRIM Support          : %s\n", hDrive->AHCIDRIVE_bTrim == LW_TRUE ? "Enable" : "Disable");
    printf("Max Blocks Trim       : %d\n", hDrive->AHCIDRIVE_usTrimBlockNumMax);
    printf("DRAT Support          : %s\n", hDrive->AHCIDRIVE_bDrat == LW_TRUE ? "Enable" : "Disable");
    printf("RZAT Support          : %s\n", hDrive->AHCIDRIVE_bRzat == LW_TRUE ? "Enable" : "Disable");
    printf("DMA                   : %s\n", hDrive->AHCIDRIVE_bDma == LW_TRUE ? "Enable" : "Disable");
    printf("NCQ                   : %s\n", hDrive->AHCIDRIVE_bNcq == LW_TRUE ? "Enable" : "Disable");
    printf("NCQ Depth             : %d\n", hDrive->AHCIDRIVE_uiQueueDepth);
    printf("LBA 48-bit            : %s\n", hDrive->AHCIDRIVE_bLba48 == LW_TRUE ? "Enable" : "Disable");
    printf("Queue Depth           : %d\n", hParam->AHCIPARAM_usQueueDepth + 1);
    printf("Sector Start          : %qu\n", (UINT64)hDrive->AHCIDRIVE_ulStartSector);
    printf("Sector Size           : %qu Bytes\n", (UINT64)hDrive->AHCIDRIVE_ulByteSector);

    if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {
        ullValue = (UINT64)hDrive->AHCIDRIVE_uiSector;
    } else {
        ullValue = (UINT64)API_AhciDriveSectorCountGet(hCtrl, uiDrive);
    }
    printf("Sector Count          : %qu\n", ullValue);

    ullValue  = (UINT64)(ullValue * hDrive->AHCIDRIVE_ulByteSector);
    if (ullValue > LW_CFG_GB_SIZE) {
        ullValue = (ullValue >> 20);
        printf("Disk Size             : %qu.%qu GB\n", (ullValue >> 10), (ullValue & 0x3ff) / 102);
    } else if (ullValue > LW_CFG_MB_SIZE) {
        ullValue = (ullValue >> 10);
        printf("Disk Size             : %qu.%qu MB\n", (ullValue >> 10), (ullValue & 0x3ff) / 102);
    } else if (ullValue > LW_CFG_KB_SIZE) {
        printf("Disk Size             : %qu.%qu KB\n", (ullValue >> 10), (ullValue & 0x3ff) / 102);
    } else {
        printf("Disk Size             : %qu.0 B\n", ullValue);
    }

    printf("Media Serial Number   : %s\n", (PCHAR)&hParam->AHCIPARAM_usCurrentMediaSN[0]);
    printf("\n");
    printf("Hotplug Attach Number : %d\n", hDrive->AHCIDRIVE_uiAttachNum);
    printf("Hotplug Remove Number : %d\n", hDrive->AHCIDRIVE_uiRemoveNum);
    printf("\n");
    if (hDrv->AHCIDRV_pfuncVendorDriveInfoShow) {
        iRet = hDrv->AHCIDRV_pfuncVendorDriveInfoShow(hCtrl, uiDrive, hParam);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDriveRegWait
** ��������: �ȴ�ֱ���Ĵ���Ϊ��������ֵ
** �䡡��  : hDrive         ���������
**           uiRegAddr      �Ĵ�ƫ�Ƶ�ַ
**           uiMask         ����
**           iFlag          ѭ����־
**                              LW_TRUE  reg & mask == value
**                              LW_FALSE reg & mask != value
**           uiValue        ֵ
**           ulInterTime    ��λʱ��
**           ulTimeout      ��ʱʱ��
**           puiReg         ��ȡ�ļĴ���ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDriveRegWait (AHCI_DRIVE_HANDLE  hDrive,
                           UINT32             uiRegAddr,
                           UINT32             uiMask,
                           INT                iFlag,
                           UINT32             uiValue,
                           ULONG              ulInterTime,
                           ULONG              ulTimeout,
                           UINT32            *puiReg)
{
    UINT32      uiReg;                                                  /* �Ĵ���ֵ                     */
    ULONG       ulTime;                                                 /* ��ǰʱ��                     */

    /*
     *  ѭ���ȴ���ʱ��Ĵ���ֵ����Ԥ�ڵ�ֵ
     */
    ulTime = 0;
    do {
        uiReg = AHCI_PORT_READ(hDrive, uiRegAddr);

        switch (iFlag) {

        case LW_TRUE:
            if ((uiReg & uiMask) != uiValue) {
                goto  __timeout_handle;
            }
            break;

        case LW_FALSE:
            if ((uiReg & uiMask) == uiValue) {
                goto  __timeout_handle;
            }
            break;

        default:
            return  (PX_ERROR);
        }

        API_TimeMSleep(ulInterTime);
        ulTime += ulInterTime;
        if (ulTime > ulTimeout) {
            goto  __timeout_handle;
        }
    } while (1);

__timeout_handle:
    if (puiReg) {
        *puiReg = uiReg;
    }

    AHCI_LOG(AHCI_LOG_PRT,
             "port reg %s offset 0x%08x mask 0x%08x flag %d "
             "value 0x%08x reg 0x%08x time %d timeout %d.\r\n",
             API_AhciDriveRegNameGet(hDrive, uiRegAddr), uiRegAddr, uiMask, iFlag, uiValue, uiReg,
             ulTime, ulTimeout);

    if (ulTime > ulTimeout) {                                           /* ��ʱ                         */
        AHCI_LOG(AHCI_LOG_ERR,
                 "port reg %s offset 0x%08x mask 0x%08x flag %d "
                 "value 0x%08x reg 0x%08x time %d timeout %d.\r\n",
                 API_AhciDriveRegNameGet(hDrive, uiRegAddr), uiRegAddr, uiMask, iFlag, uiValue, uiReg,
                 ulTime, ulTimeout);
        return  (PX_ERROR);                                             /* ���ش���                     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDriveRecvFisStop
** ��������: ֹͣ���� FIS
** �䡡��  : hDrive   ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDriveRecvFisStop (AHCI_DRIVE_HANDLE  hDrive)
{
    INT         iRet;                                                   /* ����ֵ                       */
    UINT32      uiReg;                                                  /* �Ĵ���ֵ                     */

    AHCI_LOG(AHCI_LOG_PRT, "drive recv fis stop ctrl %d port %d.\r\n",
             hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);

    /*
     *  ��ֹ FIS ���ȴ��������
     */
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    uiReg &= ~(AHCI_PCMD_FRE);
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxCMD, AHCI_PCMD_FR, LW_TRUE, AHCI_PCMD_FR, 10, 1000, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCI);
    AHCI_LOG(AHCI_LOG_PRT, "drive state 0x%08x ctrl %d port %d.\r\n",
             uiReg, hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDriveEngineStop
** ��������: ֹͣ DMA
** �䡡��  : hDrive   ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDriveEngineStop (AHCI_DRIVE_HANDLE  hDrive)
{
    INT         iRet;                                                   /* ����ֵ                       */
    UINT32      uiReg;                                                  /* �Ĵ���ֵ                     */

    AHCI_LOG(AHCI_LOG_PRT, "drive engine stop ctrl %d port %d\r\n",
             hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);

    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    if ((uiReg & ((AHCI_PCMD_ST) | (AHCI_PCMD_CR))) == 0) {             /* DMA δ����                   */
        return  (ERROR_NONE);
    }

    /*
     *  ֹͣ DMA ���ȴ��������
     */
    uiReg &= ~(AHCI_PCMD_ST);
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 1, 500, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    uiReg |= AHCI_PCMD_CLO;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxCMD, AHCI_PCMD_CLO, LW_TRUE, AHCI_PCMD_CLO, 1, 500, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDrivePowerUp
** ��������: �˿ڵ�Դʹ��
** �䡡��  : hDrive   ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrivePowerUp (AHCI_DRIVE_HANDLE  hDrive)
{
    UINT32  uiReg;

    AHCI_LOG(AHCI_LOG_PRT, "drive power up ctrl %d port %d.\r\n",
             hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);

    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    uiReg &= ~(AHCI_PCMD_ICC);
    if (hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiCap & AHCI_CAP_SSS) {
        uiReg |= AHCI_PCMD_SUD;
        AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
        AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    }

    uiReg |= AHCI_PCMD_ICC_ACTIVE;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDriveRegNameGet
** ��������: ��ȡ�������˿ڼĴ�����
** �䡡��  : hDrive     ���������
**           uiOffset   �˿ڼĴ���ƫ��
** �䡡��  : �Ĵ������ƻ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciDriveRegNameGet (AHCI_DRIVE_HANDLE  hDrive, UINT  uiOffset)
{
    REGISTER INT        i = 0;
    AHCI_DRV_HANDLE     hDrv;
    UINT8               ucSize = sizeof(_G_pahrAhciPortRegTable) / sizeof(AHCI_PORT_REG);

    hDrv = hDrive->AHCIDRIVE_hCtrl->AHCICTRL_hDrv;                      /* ����������                 */

    for (i = 0; i < ucSize; i++) {
        if (_G_pahrAhciPortRegTable[i].APR_ucOffset == uiOffset) {
            return  (_G_pahrAhciPortRegTable[i].APR_pcName);
        }
    }

    if (hDrv->AHCIDRV_pfuncVendorDriveRegNameGet) {
        return  (hDrv->AHCIDRV_pfuncVendorDriveRegNameGet(hDrive, uiOffset));
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlRegisterWait
** ��������: �ȴ�ֱ���Ĵ���Ϊ��������ֵ
** �䡡��  : hCtrl          ���������
**           uiRegAddr      �Ĵ�ƫ�Ƶ�ַ
**           uiMask         ����
**           iFlag          ѭ����־
**                              LW_TRUE  reg & mask == value
**                              LW_FALSE reg & mask != value
**           uiValue        ֵ
**           ulInterTime    ��λʱ��
**           ulTimeout      ��ʱʱ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlRegisterWait (AHCI_CTRL_HANDLE  hCtrl,
                               UINT32            uiRegAddr,
                               UINT32            uiMask,
                               INT               iFlag,
                               UINT32            uiValue,
                               ULONG             ulInterTime,
                               ULONG             ulTimeout,
                               UINT32           *puiReg)
{
    UINT32      uiReg;                                                  /* �Ĵ���ֵ                     */
    ULONG       ulTime;                                                 /* ��ǰʱ��                     */

    /*
     *  ѭ���ȴ���ʱ��Ĵ���ֵ����Ԥ�ڵ�ֵ
     */
    ulTime = 0;
    do {
        uiReg = AHCI_CTRL_READ(hCtrl, uiRegAddr);

        switch (iFlag) {

        case LW_TRUE:
            if ((uiReg & uiMask) != uiValue) {
                goto  __timeout_handle;
            }
            break;

        case LW_FALSE:
            if ((uiReg & uiMask) == uiValue) {
                goto  __timeout_handle;
            }
            break;

        default:
            return  (PX_ERROR);
        }

        API_TimeMSleep(ulInterTime);
        ulTime += ulInterTime;
        if (ulTime > ulTimeout) {
            goto  __timeout_handle;
        }
    } while (1);

__timeout_handle:
    if (puiReg) {
        *puiReg = uiReg;
    }

    AHCI_LOG(AHCI_LOG_PRT,
             "ctrl reg %s offset 0x%08x mask 0x%08x flag %d "
             "value 0x%08x reg 0x%08x time %d timeout %d.\r\n",
             API_AhciCtrlRegNameGet(hCtrl, uiRegAddr), uiRegAddr, uiMask, iFlag, uiValue, uiReg,
             ulTime, ulTimeout);

    if (ulTime > ulTimeout) {
        AHCI_LOG(AHCI_LOG_ERR,
                 "ctrl reg %s offset 0x%08x mask 0x%08x flag %d "
                 "value 0x%08x reg 0x%08x time %d timeout %d.\r\n",
                 API_AhciCtrlRegNameGet(hCtrl, uiRegAddr), uiRegAddr, uiMask, iFlag, uiValue, uiReg,
                 ulTime, ulTimeout);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlIntConnect
** ��������: ���ӿ������ж�
** �䡡��  : hCtrl      ���������
**           pfuncIsr   �жϷ�����
**           cpcName    �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlIntConnect (AHCI_CTRL_HANDLE  hCtrl, PINT_SVR_ROUTINE pfuncIsr, CPCHAR cpcName)
{
    INT                 iRet;
    AHCI_DRV_HANDLE     hDrv;

    hDrv = hCtrl->AHCICTRL_hDrv;

    if (hDrv->AHCIDRV_pfuncVendorCtrlIntConnect) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlIntConnect(hCtrl, pfuncIsr, cpcName);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(&hCtrl->AHCICTRL_cIrqName[0], cpcName, AHCI_CTRL_IRQ_NAME_MAX);
    hCtrl->AHCICTRL_pfuncIrq = pfuncIsr;

    if (hDrv->AHCIDRV_pfuncVendorCtrlIntEnable) {
        hDrv->AHCIDRV_pfuncVendorCtrlIntEnable(hCtrl);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlReset
** ��������: ��������λ
** �䡡��  : hCtrl    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlReset (AHCI_CTRL_HANDLE  hCtrl)
{
    INT     iRet;
    UINT32  uiReg;

    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    if ((uiReg & AHCI_GHC_HR) == 0) {
        uiReg |= AHCI_GHC_HR;
        AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg);
        API_TimeMSleep(50);
        uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    }

    iRet = API_AhciCtrlRegisterWait(hCtrl, AHCI_GHC, AHCI_GHC_HR, LW_TRUE, AHCI_GHC_HR, 10, 1000, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlAhciModeEnable
** ��������: ʹ�� AHCI ģʽ
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlAhciModeEnable (AHCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT        i;
    UINT32              uiReg;

    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    if (uiReg & AHCI_GHC_AE) {
        AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
        return  (ERROR_NONE);
    }

    for (i = 0; i < AHCI_MODE_EN_RETRY_NUM; i++) {                      /* �����豸��Ҫ��β���         */
        uiReg |= AHCI_GHC_AE;
        AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg);
        uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
        if (uiReg & AHCI_GHC_AE) {
            break;
        }
        API_TimeMSleep(10);
    }
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);

    if (i >= AHCI_MODE_EN_RETRY_NUM) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlSssSet
** ��������: Staggered Spin-up ����
** �䡡��  : hCtrl      ���������
**           iSet       ʹ�ܿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlSssSet (AHCI_CTRL_HANDLE  hCtrl, INT  iSet)
{
    UINT32      uiReg;

    AHCI_CTRL_REG_MSG(hCtrl, AHCI_CAP);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_CAP);
    if ((iSet == LW_TRUE) &&
        (!(uiReg & AHCI_CAP_SSS))) {
        uiReg |= AHCI_CAP_SSS;
        AHCI_CTRL_WRITE(hCtrl, AHCI_CAP, uiReg);
    
    } else if ((iSet == LW_FALSE) &&
               (uiReg & AHCI_CAP_SSS)) {
        uiReg &= ~(AHCI_CAP_SSS);
        AHCI_CTRL_WRITE(hCtrl, AHCI_CAP, uiReg);
    }
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_CAP);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlInfoShow
** ��������: ��ӡ��������Ϣ
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlInfoShow (AHCI_CTRL_HANDLE  hCtrl)
{
    INT                 iRet;                                           /* �������                     */
    REGISTER INT        i;                                              /* ѭ������                     */
    AHCI_DRV_HANDLE     hDrv;
    UINT32              uiCap;                                          /* ��������������               */
    UINT32              uiCap2;                                         /* ��������չ��������           */
    UINT32              uiVer;                                          /* �汾��Ϣ                     */
    UINT32              uiSpeed;                                        /* ͨ������                     */
    CPCHAR              cpcType = "SATA";                               /* ����������                   */
    CPCHAR              cpcSpeed;                                       /* ͨ�������ַ�����Ϣ           */
    UINT32              uiImpPortMap;                                   /* �˿�λͼ                     */
    UINT32              uiImpPortNum;                                   /* ��Ч�˿�����                 */
    UINT32              uiReg;                                          /* �Ĵ���ֵ                     */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    hDrv = hCtrl->AHCICTRL_hDrv;

    if (hDrv->AHCIDRV_pfuncVendorCtrlTypeNameGet) {
        cpcType = (CPCHAR)hDrv->AHCIDRV_pfuncVendorCtrlTypeNameGet(hCtrl);
    }

    /*
     *  ͨ���汾��Ϣ�ж��Ƿ�֧����չ����
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_VS);
    uiVer = AHCI_CTRL_READ(hCtrl, AHCI_VS);
    if (((uiVer >> 16) > 1) ||
        ((uiVer >> 16) == 1 && (uiVer & 0xFFFF) >= 0x0200)) {
        AHCI_CTRL_REG_MSG(hCtrl, AHCI_CAP2);
        uiCap2 = AHCI_CTRL_READ(hCtrl, AHCI_CAP2);
    
    } else {
        uiCap2 = 0;
    }

    /*
     * ��ȡ������������Ϣ
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_CAP);
    uiCap = AHCI_CTRL_READ(hCtrl, AHCI_CAP);
    uiSpeed = (uiCap & AHCI_CAP_ISS) >> AHCI_CAP_ISS_SHFT;
    switch (uiSpeed) {

    case 0x01:
        cpcSpeed = "Gen 1 (1.5 Gbps)";
        break;

    case 0x02:
        cpcSpeed = "Gen 2 (3.0 Gbps)";
        break;

    case 0x03:
        cpcSpeed = "Gen 3 (6.0 Gbps)";
        break;

    default:
        cpcSpeed = "Reserved";
        break;
    }

    /*
     *  ��ȡ�˿�λͼ����Ч�˿ڵ���Ϣ
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_PI);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_PI);
    uiImpPortMap = 0;
    uiImpPortNum = 0;
    for (i = 0; i < AHCI_PI_WIDTH; i++) {
        if (uiReg & 1) {
            uiImpPortMap |= 0x01 << i;
            uiImpPortNum += 1;
        }
        uiReg >>= 1;
    }

    iRet = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT);
    if (iRet < 0) {                                                     /* ��׼��������Ч             */
        return  (ERROR_NONE);                                           /* ֱ����ȷ����                 */
    }

    /*
     *  ��ӡ��������Ϣ
     */
    printf("\nAHCI Control %d Information >>\n", hCtrl->AHCICTRL_uiIndex);
    printf("Build Date Time      : %s %s\n", __DATE__, __TIME__);
    printf("Debug Level          : %s\n", (AHCI_LEVEL == AHCI_LEVEL_RELEASE) ? "Release" : "Debug");
    printf("Log Message          : %s\n", (AHCI_LOG_EN) ? "Enable" : "Disable");
    printf("Int Log Message      : %s\n", (AHCI_INT_LOG_EN) ? "Enable" : "Disable");
    printf("Log Message Level    : Running%c Error%c Bug%c Print%c\n",
           AHCI_FLAG(AHCI_LOG_LEVEL, AHCI_LOG_RUN), AHCI_FLAG(AHCI_LOG_LEVEL, AHCI_LOG_ERR),
           AHCI_FLAG(AHCI_LOG_LEVEL, AHCI_LOG_BUG), AHCI_FLAG(AHCI_LOG_LEVEL, AHCI_LOG_PRT));
    printf("Control Name         : %s\n", hCtrl->AHCICTRL_cCtrlName);
    printf("Control Unit Index   : %d\n", hCtrl->AHCICTRL_uiUnitIndex);
    if ((hCtrl) && (hCtrl->AHCICTRL_uiCoreVer)) {
        printf("Control Core Version : " AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_uiCoreVer));
    } else {
        printf("Control Core Version : " "*");
    }
    printf("\n");
    printf("Driver Name          : %s\n", hCtrl->AHCICTRL_hDrv->AHCIDRV_cDrvName);
    if ((hCtrl) && (hCtrl->AHCICTRL_hDrv) && (hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer)) {
        printf("Driver Version       : " AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer));
    } else {
        printf("Driver Version       : " "*");
    }
    printf("\n");
    printf("Control Base Addr    : %p\n", hCtrl->AHCICTRL_pvRegAddr);
    printf("Control Irq Number   : %qu\n", (UINT64)hCtrl->AHCICTRL_ulIrqVector);
    printf("Type                 : %s\n", cpcType);
    printf("Version              : %02x%02x.%02x%02x\n",
           (uiVer >> 24) & 0xff, (uiVer >> 16) & 0xff, (uiVer >> 8) & 0xff, uiVer & 0xff);
    printf("Interface Speed      : %s\n", cpcSpeed);
    printf("Cmd Slot Num         : %d\n",((uiCap & AHCI_CAP_NCS) >> AHCI_CAP_NCS_SHFT) + 1);
    printf("Port Num             : %d\n", (uiCap & AHCI_CAP_NP) + 1);
    printf("Active Port Num      : %d\n", uiImpPortNum);
    printf("Active Port Map      : 0x%08x\n", uiImpPortMap);
    printf("\n");
    printf("Capabilities :\n");
    printf("  S64A(Supports 64-bit Addressing)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_S64A));
    printf("  SNCQ(Supports Native Command Queuing)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SNCQ));
    printf("  SSNTF(Supports SNotification Register)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SSNTF));
    printf("  SMPS(Supports Mechanical Presence Switch)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SMPS));
    printf("  SSS(Supports Staggered Spin-up)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SSS));
    printf("  SALP(Supports Aggressive Link Power Management)%c\n",AHCI_FLAG(uiCap, AHCI_CAP_SALP));
    printf("  SAL(Supports Activity LED)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SAL));
    printf("  SCLO(Supports Command List Override)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SCLO));
    printf("  SAM(Supports AHCI mode only)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SAM));
    printf("  SPM(Supports Port Multiplier)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SPM));
    printf("  FBSS(FIS-based Switching Supported)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_FBSS));
    printf("  PMD(PIO Multiple DRQ Block)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_PMD));
    printf("  SSC(Slumber State Capable)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SSC));
    printf("  PSC(Partial State Capable)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_PSC));
    printf("  CCCS(Command Completion Coalescing Supported)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_CCCS));
    printf("  EMS(Enclosure Management Supported)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_EMS));
    printf("  SXS(Supports External SATA)%c\n", AHCI_FLAG(uiCap, AHCI_CAP_SXS));
    printf("Capabilities 2 :\n");
    printf("  DESO(DevSleep Entrance from Slumber Only)%c\n", AHCI_FLAG(uiCap2, AHCI_CAP2_DESO));
    printf("  SADM(Supports Aggressive Device Sleep Management)%c\n", AHCI_FLAG(uiCap2,AHCI_CAP2_SADM));
    printf("  SDS(Supports Device Sleep)%c\n", AHCI_FLAG(uiCap2, AHCI_CAP2_SDS));
    printf("  APST(Automatic Partial to Slumber Transitions)%c\n", AHCI_FLAG(uiCap2, AHCI_CAP2_APST));
    printf("  NVMP(NVMHCI Present)%c\n", AHCI_FLAG(uiCap2, AHCI_CAP2_NVMP));
    printf("  BOH(BIOS/OS Handoff)%c\n", AHCI_FLAG(uiCap2, AHCI_CAP2_BOH));

    if (hDrv->AHCIDRV_pfuncVendorCtrlInfoShow) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlInfoShow(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlImpPortGet
** ��������: ��ȡ�˿ڲ���
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlImpPortGet (AHCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT    i;
    UINT32          uiReg;

    hCtrl->AHCICTRL_uiImpPortMap = 0;
    hCtrl->AHCICTRL_uiImpPortNum = 0;
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_PI);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_PI);
    for (i = 0; i < AHCI_PI_WIDTH; i++) {
        if (uiReg & 1) {
            hCtrl->AHCICTRL_uiImpPortMap |= 0x01 << i;
            hCtrl->AHCICTRL_uiImpPortNum += 1;
        }
        uiReg >>= 1;
    }

    AHCI_LOG(AHCI_LOG_PRT, "active port %d, index \r\n", hCtrl->AHCICTRL_uiImpPortNum);
    for (i = 0; i < AHCI_PI_WIDTH; i++) {
        if (hCtrl->AHCICTRL_uiImpPortMap & (0x01 << i)) {
            AHCI_LOG(AHCI_LOG_PRT, "%d \r\n", i);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlCapGet
** ��������: ��ȡ��Ӧ����
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlCapGet (AHCI_CTRL_HANDLE  hCtrl)
{
    UINT32      uiReg;

    AHCI_CTRL_REG_MSG(hCtrl, AHCI_CAP);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_CAP);
    hCtrl->AHCICTRL_uiCap = uiReg;
    hCtrl->AHCICTRL_uiPortNum = (uiReg & AHCI_CAP_NP) + 1;
    hCtrl->AHCICTRL_uiCmdSlotNum = ((uiReg & AHCI_CAP_NCS) >> AHCI_CAP_NCS_SHFT) + 1;
    if (uiReg & AHCI_CAP_S64A) {
        hCtrl->AHCICTRL_bAddr64 = LW_TRUE;
    } else {
        hCtrl->AHCICTRL_bAddr64 = LW_FALSE;
    }
    if (uiReg & AHCI_CAP_SNCQ) {
        hCtrl->AHCICTRL_bNcq = LW_TRUE;
    } else {
        hCtrl->AHCICTRL_bNcq = LW_FALSE;
    }
    if (uiReg & AHCI_CAP_SPM) {
        hCtrl->AHCICTRL_bPmp = LW_TRUE;
    } else {
        hCtrl->AHCICTRL_bPmp = LW_FALSE;
    }
    if (uiReg & AHCI_CAP_EMS) {
        hCtrl->AHCICTRL_bEms = LW_TRUE;
    } else {
        hCtrl->AHCICTRL_bEms = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlRegNameGet
** ��������: ��ȡȫ�ּĴ�����
** �䡡��  : hCtrl      ���������
**           uiOffset   ȫ�ּĴ���ƫ��
** �䡡��  : �Ĵ������ƻ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCHAR  API_AhciCtrlRegNameGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiOffset)
{
    REGISTER INT        i = 0;
    AHCI_DRV_HANDLE     hDrv;
    UINT8               ucSize = sizeof(_G_pahrAhciHostRegTable) / sizeof(AHCI_HOST_REG);

    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */

    for (i = 0; i < ucSize; i++) {
        if (_G_pahrAhciHostRegTable[i].AHR_ucOffset == uiOffset) {
            return  (_G_pahrAhciHostRegTable[i].AHR_pcName);
        }
    }

    if (hDrv->AHCIDRV_pfuncVendorCtrlRegNameGet) {
        return  (hDrv->AHCIDRV_pfuncVendorCtrlRegNameGet(hCtrl, uiOffset));
    }

    return  (LW_NULL);
}

#endif                                                                  /* (LW_CFG_DEVICE_EN > 0) &&    */
                                                                        /* (LW_CFG_AHCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
