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
** ��   ��   ��: ahci.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 01 �� 04 ��
**
** ��        ��: AHCI ����.

** BUG:
2016.10.17  ���ж�Ӳ���Ƿ�Ϊ�ȶ�״̬ʱͳһʹ�� __ahciDriveNoBusyWait() ����.
            ��еӲ�� Seagate Desktop HDD 1000GB MODEL: ST1000DM003 �ȶ�ʱ����� 900 ms (��ȥ����ʱ��)
2016.11.01  ̽�⵽Ӳ�̺�����Ҫ�ȴ�Ӳ�̵��ȶ�״̬, �� N2600 ƽ̨ NM10 ���ϵ� AHCI ����������Ҫ�ȴ�.
2016.11.10  ����ǿ���Զ� PHY ���и�λ����, ������״̬��ȷʱ���ٶ� PHY ���г�ʼ��.
2016.11.22  �ɾ����豸�����Ƿ�� PHY ���и�λ, ͳһʹ�� __ahciDrivePhyReset() ����.
2016.11.27  ��֧�� NCQ �Ĵ���, ��ʹ�ò��й��߲���.
2016.12.27  ��֧�� NCQ �Ĵ��� (NandFlash), ��ʹ�ò�������.
2016.12.28  KINGSTON SUV400S37120G ����ʹ�� CACHE ��д.
2017.03.09  ���ӶԴ����Ȳ�ε�֧��, ����ͨ�� AHCI_HOTPLUG_EN ��������. (v1.0.6-rc0)
2017.04.24  �޸�ͬʱʹ�� NCQ �� TRIM ��ɾ�����ļ���������(ahci_slot)������. (v1.0.7-rc0)
2017.07.24  �޸��������������̲�֧����ϴ��������. (v1.0.8-rc0)
2017.08.13  �޸��� PCI �����豸�쳣������. (v1.1.0-rc0)
2017.09.03  ���ϲ� DISCACHE ��֤��д������ɺ��ٽ��� TRIM �� CACHE ����. (v1.1.1-rc0)
2018.01.27  �޸� MIPS x64 Release ����������. (v1.1.2-rc0)
2018.08.09  ���� S.M.A.R.T ���ܵ�֧��. (v1.2.0-rc0)
2018.08.20  �޸� x86 �°�װ��ϵͳ������, ����������е�̷��� RST-OFF ���ʱ����. (v1.2.1-rc0)
2018.03.01  ���� ATAPI ����֧��. (v1.2.2-rc0)
2020.08.20  ���� PHY ��λ��Դ���Ĵ������������������ǿ������. (v1.2.3-rc0)
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "linux/compat.h"
#include "pci_ids.h"
#include "ahci.h"
#include "ahciLib.h"
#include "ahciPort.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciCtrl.h"
#include "ahciPm.h"
#include "ahciSmart.h"
/*********************************************************************************************************
  λ����
*********************************************************************************************************/
#define AHCI_BIT_MASK(bit)      (1 << (bit))
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static INT  _GiAhciConfigType[AHCI_DRIVE_MAX] = {
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL
};
/*********************************************************************************************************
  ATAPI ������Ϣ
*********************************************************************************************************/
#if AHCI_LOG_EN > 0                                                     /* ʹ�ܵ���                     */
#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */

static CPCHAR   _GcpcAhciAtapiErrStrs[] = {
    "Unknown Error",                                                    /*  0                           */
    "Wait for Command Packet request time expire",                      /*  1                           */
    "Error in Command Packet Request",                                  /*  2                           */
    "Error in Command Packet Request",                                  /*  3                           */
    "Wait for Data Request time expire",                                /*  4                           */
    "Data Request for NON Data command",                                /*  5                           */
    "Error in Data Request",                                            /*  6                           */
    "Error in End of data transfer condition",                          /*  7                           */
    "Extra transfer request",                                           /*  8                           */
    "Transfer size requested exceeds desired size",                     /*  9                           */
    "Transfer direction miscompare",                                    /* 10                           */
    "No Sense",                                                         /* 11                           */
    "Recovered Error",                                                  /* 12                           */
    "Not Ready",                                                        /* 13                           */
    "Medium Error",                                                     /* 14                           */
    "Hardware Error",                                                   /* 15                           */
    "Illegal Request",                                                  /* 16                           */
    "Unit Attention",                                                   /* 17                           */
    "Data Protected",                                                   /* 18                           */
    "Aborted Command",                                                  /* 19                           */
    "Miscompare",                                                       /* 20                           */
    "\0",                                                               /* 21                           */
    "\0",                                                               /* 22                           */
    "\0",                                                               /* 23                           */
    "\0",                                                               /* 24                           */
    "\0",                                                               /* 25                           */
    "Overlapped commands are not implemented",                          /* 26                           */
    "DMA transfer Error",                                               /* 27                           */
};

#endif                                                                  /* AHCI_ATAPI_EN                */
#endif                                                                  /* AHCI_LOG_EN                  */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static PVOID            __ahciMonitorThread(PVOID pvArg);
static irqreturn_t      __ahciIsr(PVOID pvArg, ULONG ulVector);
static INT              __ahciDiskCtrlInit(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
static INT              __ahciDiskDriveInit(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
/*********************************************************************************************************
  �������Ĵ�����д����
*********************************************************************************************************/
static UINT32 __ahciCtrlRegReadLe (AHCI_CTRL_HANDLE  hCtrl, addr_t  ulReg)
{
    return  (read32_le((addr_t)((ULONG)(hCtrl)->AHCICTRL_pvRegAddr + ulReg)));
}
static UINT32 __ahciCtrlRegReadBe (AHCI_CTRL_HANDLE  hCtrl, addr_t  ulReg)
{
    return  (read32_be((addr_t)((ULONG)(hCtrl)->AHCICTRL_pvRegAddr + ulReg)));
}
static VOID __ahciCtrlRegWriteLe (AHCI_CTRL_HANDLE  hCtrl, addr_t  ulReg, UINT32  uiData)
{
    write32_le(uiData, (addr_t)((ULONG)(hCtrl)->AHCICTRL_pvRegAddr + ulReg));
}
static VOID __ahciCtrlRegWriteBe (AHCI_CTRL_HANDLE  hCtrl, addr_t  ulReg, UINT32  uiData)
{
    write32_be(uiData, (addr_t)((ULONG)(hCtrl)->AHCICTRL_pvRegAddr + ulReg));
}
/*********************************************************************************************************
  �˿ڼĴ�����д����
*********************************************************************************************************/
static UINT32 __ahciPortRegReadLe (AHCI_DRIVE_HANDLE  hDrive, addr_t  ulReg)
{
    return  (read32_le((addr_t)((ULONG)(hDrive)->AHCIDRIVE_pvRegAddr + ulReg)));
}
static UINT32 __ahciPortRegReadBe (AHCI_DRIVE_HANDLE  hDrive, addr_t  ulReg)
{
    return  (read32_be((addr_t)((ULONG)(hDrive)->AHCIDRIVE_pvRegAddr + ulReg)));
}
static VOID __ahciPortRegWriteLe (AHCI_DRIVE_HANDLE  hDrive, addr_t  ulReg, UINT32  uiData)
{
    write32_le(uiData, (addr_t)((ULONG)(hDrive)->AHCIDRIVE_pvRegAddr + ulReg));
}
static VOID __ahciPortRegWriteBe (AHCI_DRIVE_HANDLE  hDrive, addr_t  ulReg, UINT32  uiData)
{
    write32_be(uiData, (addr_t)((ULONG)(hDrive)->AHCIDRIVE_pvRegAddr + ulReg));
}
/*********************************************************************************************************
** ��������: API_AhciSwapBufLe16
** ��������: ��С������ת����λ�����ֽ���
** �䡡��  : pusBuf     ���������
**           stWords    �������ֳ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
VOID  API_AhciSwapBufLe16 (UINT16 *pusBuf, size_t  stWords)
{
    REGISTER UINT   i;

    for (i = 0; i < stWords; i++) {
        pusBuf[i] = le16_to_cpu(pusBuf[i]);
    }
}
/*********************************************************************************************************
** ��������: __ahciDiskDriveDiagnostic
** ��������: ����������������ϵĴ���
** �䡡��  : hCtrl      ���������
**           uiDrive     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskDriveDiagnostic (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                 iRet;                                           /* ���󷵻�                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    PCI_DEV_HANDLE      hPciDev;                                        /* PCI �豸���                 */
    UINT16              usVendorId;                                     /* ���� ID                      */

    /*
     *  ��ȡ������Ϣ
     */
    hDrive     = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hPciDev    = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;              /* ��ȡ�豸���                 */
    usVendorId = PCI_DEV_VENDOR_ID(hPciDev);

    switch (usVendorId) {

    case PCI_VENDOR_ID_VMWARE:
        break;

    default:
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_DIAGNOSE, 0, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
            AHCI_LOG(AHCI_LOG_ERR, "disk port diagnostic command failed ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            return  (PX_ERROR);
        }
        break;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDrivePhyReset
** ��������: PHY ��λ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDrivePhyReset (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet = PX_ERROR;
    PCI_DEV_HANDLE          hPciDevHandle;
    AHCI_DRIVE_HANDLE       hDrive;
    UINT32                  uiReg;
    UINT16                  usVendorId;
    UINT16                  usDeviceId;

    if (!hCtrl) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hDrive        = &hCtrl->AHCICTRL_hDrive[uiDrive];                   /* ��ȡ���������               */
    hPciDevHandle = (PCI_DEV_HANDLE)hCtrl->AHCICTRL_pvPciArg;
    usVendorId    = PCI_DEV_VENDOR_ID(hPciDevHandle);
    usDeviceId    = PCI_DEV_DEVICE_ID(hPciDevHandle);

    switch (usVendorId) {

    case PCI_VENDOR_ID_ATI:
        if ((usDeviceId == PCI_DEVICE_ID_ATI_IXP600_SATA) ||
            (usDeviceId == PCI_DEVICE_ID_ATI_IXP700_SATA) ||
            (usDeviceId == PCI_DEVICE_ID_AMD_HUDSON2_SATA_IDE)) {
            iRet = API_AhciDriveRegWait(hDrive,
                                        AHCI_PxSSTS, AHCI_PSSTS_DET_MSK, LW_FALSE, AHCI_PSSTS_DET_PHY,
                                        1, 50, &uiReg);
            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "port sctl reset failed ctrl %d port %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive);
                return  (PX_ERROR);
            }
            return  (ERROR_NONE);
        }
        break;

    default:
        break;
    }

    AHCI_LOG(AHCI_LOG_PRT, "port det reset, partial and slumber disable ctrl %d port %d.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, AHCI_PSCTL_DET_RESET | AHCI_PSCTL_IPM_PARSLUM_DISABLED);
    AHCI_PORT_READ(hDrive, AHCI_PxSCTL);
    API_TimeMSleep(200);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, 0);
    AHCI_PORT_READ(hDrive, AHCI_PxSCTL);

    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxSSTS, AHCI_PSSTS_DET_MSK, LW_FALSE, AHCI_PSSTS_DET_PHY,
                                1, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port sctl reset failed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    /*
     * �˿ڸ�λ���������Ĵ���
     */
    AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxSERR);
    AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDriveNoBusyWait
** ��������: �ȴ���������æ, ��еӲ�̴��ϵ絽״̬��ȷ��Ҫһ��ʱ��
** �䡡��  : hDrive    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDriveNoBusyWait (AHCI_DRIVE_HANDLE  hDrive)
{
    REGISTER INT    i;
    UINT32          uiReg;

    for (i = 0; i < hDrive->AHCIDRIVE_ulProbTimeCount; i++) {
        uiReg = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
        if (uiReg & AHCI_STAT_ACCESS) {
            API_TimeMSleep(hDrive->AHCIDRIVE_ulProbTimeUnit);
        } else {
            break;
        }
    }

    if (i >= hDrive->AHCIDRIVE_ulProbTimeCount) {
        AHCI_LOG(AHCI_LOG_ERR, "wait ctrl %d drive %d no busy failed time %d ms.\r\n",
                 hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort,
                 (hDrive->AHCIDRIVE_ulProbTimeUnit * hDrive->AHCIDRIVE_ulProbTimeCount));
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "wait ctrl %d drive %d no busy time %d ms.\r\n",
             hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort,
             (hDrive->AHCIDRIVE_ulProbTimeUnit * i));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciCmdWaitForResource
** ��������: �ȴ���Դ
** �䡡��  : hDrive     ���������
**           bQueued    �Ƿ�ʹ�ܶ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ahciCmdWaitForResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
             INTREG   iregInterLevel;
    REGISTER INT      i;                                                /* ѭ������                     */

    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* �� NCQ ģʽ                  */
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    
    } else {                                                            /* NCQ ģʽ                     */
        if (bQueued) {                                                  /* ����ģʽ                     */
            API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
            LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
            if (hDrive->AHCIDRIVE_bQueued == LW_FALSE) {                /* �Ƕ���ģʽ                   */
                hDrive->AHCIDRIVE_bQueued =  LW_TRUE;                   /* ����ģʽ                     */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* �ͷſ���Ȩ                   */
                                                                        /* ��ʼ�����вۿ���Ȩ           */
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth - 1; i++) {
                    API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
                }
            
            } else {
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
            }
        
        } else {                                                        /* �Ƕ���ģʽ                   */
            API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
            LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
            if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {                 /* ����ģʽ                     */
                hDrive->AHCIDRIVE_bQueued =  LW_FALSE;                  /* ���Ϊ�Ƕ���ģʽ             */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {
                    API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem,
                                       LW_OPTION_WAIT_INFINITE);        /* �ȴ�������Դ����Ȩ           */
                }
            
            } else {                                                    /* �Ƕ���ģʽ                   */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* �ͷſ���Ȩ                   */
                API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __ahciCmdReleaseResource
** ��������: �ͷ���Դ
** �䡡��  : hDrive     ���������
**           bQueued    �Ƿ�ʹ�ܶ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ahciCmdReleaseResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* �Ƕ���ģʽ                   */
        API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
    
    } else {                                                            /* ����ģʽ                     */
        API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
        if (bQueued == LW_FALSE) {                                      /* �Ƕ���ģʽ                   */
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
        }
    }
}
/*********************************************************************************************************
** ��������: __ahciPrdtSetup
** ��������: ���� Physical Region Descriptor Table (PRDT)
** �䡡��  : pcDataBuf      ���ݻ�����
**           ulLen          ���ݳ���
**           hPrdtHandle    PRDT ���ƿ���
** �䡡��  : ���ݿ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __ahciPrdtSetup (UINT8 *pcDataBuf, ULONG  ulLen, AHCI_PRDT_HANDLE  hPrdtHandle)
{
    ULONG       ulSize;                                                 /* ���ݳ���                     */
    UINT32      uiPrdtCount;                                            /* PRD ����                     */
    ULONG       ulByteCount;                                            /* ���ݳ��ȼ���                 */
    PVOID       pvFlush = (PVOID)hPrdtHandle;

    AHCI_CMD_LOG(AHCI_LOG_PRT, "buff 0x%x len %ld.\r\n", pcDataBuf, ulLen);

    if ((addr_t)pcDataBuf & 1) {                                        /* ��ַ�������                 */
        AHCI_CMD_LOG(AHCI_LOG_ERR, "dma buffer not word aligned %p.\r\n", pcDataBuf);
        return  (0);
    }

    ulSize      = ulLen;
    uiPrdtCount = 0;
    
    while (ulSize) {                                                    /* ���� PRDT                    */
        if (++uiPrdtCount > AHCI_PRDT_MAX) {                            /* PRDT ����������              */
            AHCI_CMD_LOG(AHCI_LOG_ERR, "dma table small [1 - %lu] %lu.\r\n", AHCI_PRDT_MAX, uiPrdtCount);
            return  (0);
        } else {                                                        /* �п��õ� PRDT ��             */
            if (ulSize > AHCI_PRDT_BYTE_MAX) {                          /* ���ݴ�С����                 */
                ulByteCount = AHCI_PRDT_BYTE_MAX;
            } else {                                                    /* ���ݴ�Сδ����               */
                ulByteCount = ulSize;
            }

            /*
             *  ���� PRDT ��ַ��Ϣ
             */
            hPrdtHandle->AHCIPRDT_uiDataAddrLow  = cpu_to_le32(AHCI_ADDR_LOW32(pcDataBuf));
            hPrdtHandle->AHCIPRDT_uiDataAddrHigh = cpu_to_le32(AHCI_ADDR_HIGH32(pcDataBuf));

            /*
             *  ���µ�ַ���С����Ϣ
             */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "table addr %p byte count %ld.\r\n", pcDataBuf, ulByteCount);
            pcDataBuf += ulByteCount;
            ulSize    -= ulByteCount;
            
            hPrdtHandle->AHCIPRDT_uiDataByteCount = cpu_to_le32((ulByteCount - 1));
            if (ulSize == 0) {
                hPrdtHandle->AHCIPRDT_uiDataByteCount |= AHCI_PRDT_I;
            }
            
            hPrdtHandle++;
        }
    }

    API_CacheDmaFlush(pvFlush, uiPrdtCount * sizeof(AHCI_PRDT_CB));     /* ��д PRDT ������Ϣ           */

    return  (uiPrdtCount);
}
/*********************************************************************************************************
** ��������: API_AhciDiskCommandSend
** ��������: �������������������
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           hCmd       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciDiskCommandSend (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, AHCI_CMD_HANDLE  hCmd)
{
    INTREG                  iregInterLevel;                             /* �жϼĴ���                   */
    
    ULONG                   ulRet;                                      /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_PRDT_HANDLE        hPrdt;                                      /* PRDT ���                    */
    UINT8                  *pucCommandFis;                              /* ����ṹ����                 */
    UINT8                  *pucPktCmd;                                  /* �����                     */
    AHCI_CMD_LIST_HANDLE    hCommandList;                               /* �������о��                 */
    UINT32                  uiFlagsPrdLength;                           /* PRDT ��־                    */
    UINT32                  uiPrdtCount;                                /* PRDT ������Ϣ                */
    UINT                    uiTag;                                      /* �����Ϣ                     */
    UINT32                  uiTagBit;                                   /* ���λ��Ϣ                   */
    ULONG                   ulWait;                                     /* ��ʱ����                     */
    BOOL                    bQueued;                                    /* ����ģʽ                     */
    INT32                   iFlags;                                     /* �����                       */

    hDrive  = &hCtrl->AHCICTRL_hDrive[uiDrive];                         /* ��ȡ���������               */
    bQueued = hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ;                 /* �Ƿ�ʹ�ܶ���ģʽ             */
    iFlags  = hCmd->AHCICMD_iFlags & (AHCI_CMD_FLAG_SRST_ON | AHCI_CMD_FLAG_SRST_OFF);
    if (iFlags == 0) {                                                  /* �ǿ�������                   */
        __ahciCmdWaitForResource(hDrive, bQueued);                      /* �ȴ�����Ȩ                   */
    }

    if (bQueued) {                                                      /* ����ģʽʹ��                 */
        for (;;) {
            LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
            uiTag = hDrive->AHCIDRIVE_uiNextTag;
            hDrive->AHCIDRIVE_uiNextTag++;
            if (hDrive->AHCIDRIVE_uiNextTag >= hDrive->AHCIDRIVE_uiQueueDepth) {
                hDrive->AHCIDRIVE_uiNextTag  = 0;
            }
            if (!(hDrive->AHCIDRIVE_uiCmdMask & AHCI_BIT_MASK(uiTag))) {
                uiTagBit = AHCI_BIT_MASK(uiTag);                        /* ��ȡ��־λ��Ϣ               */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                break;                                                  /* �м����ź�������, ����ʵ���� */
                                                                        /* ��, �������λ               */
            } else {
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
            }
        }
    
    } else {
        uiTag    = 0;                                                   /* �������                     */
        uiTagBit = 1;                                                   /* ���λ��Ϣ                   */
    }

    /*
     *  ��ȡ PRDT ���ƿ����������п��ƿ�
     */
    hPrdt         = &hDrive->AHCIDRIVE_hCmdTable[uiTag].AHCICMDTABL_tPrdt[0];
    pucCommandFis = &hDrive->AHCIDRIVE_hCmdTable[uiTag].AHCICMDTABL_ucCommandFis[0];
    pucPktCmd     = &hDrive->AHCIDRIVE_hCmdTable[uiTag].AHCICMDTABL_ucAtapiCommand[0];
    hCommandList  = &hDrive->AHCIDRIVE_hCmdList[uiTag];

    pucCommandFis[0] = 0x27;
    if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_ON) {                 /* ��λ������                   */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst on ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT | AHCI_CTL_RST;
        hCommandList->AHCICMDLIST_uiPrdtFlags =  cpu_to_le32(AHCI_CMD_LIST_C | AHCI_CMD_LIST_R | 5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        AHCI_PORT_READ(hDrive, AHCI_PxCI);
        return  (ERROR_NONE);

    } else if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_OFF) {         /* ��λ���������               */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst off ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT;
        hCommandList->AHCICMDLIST_uiPrdtFlags = cpu_to_le32(5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        AHCI_PORT_READ(hDrive, AHCI_PxCI);
        return  (ERROR_NONE);

    } else {                                                            /* ��������                     */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ����ģʽ               */
            AHCI_ATAPI_CMD_LOG(AHCI_LOG_PRT, "cmd flag atapi ctrl %d port %d.\r\n",
                               hCtrl->AHCICTRL_uiIndex,uiDrive);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = AHCI_PI_CMD_PKTCMD;
            pucCommandFis[3] = hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature;
            pucCommandFis[4] = 0;
            pucCommandFis[5] = (UINT8)hCmd->AHCICMD_ulDataLen;
            pucCommandFis[6] = (UINT8)(hCmd->AHCICMD_ulDataLen >> 8);
            pucCommandFis[7] = AHCI_SDH_LBA;
            lib_bzero(&pucCommandFis[8], 12);
            lib_memcpy(pucPktCmd, hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt, AHCI_ATAPI_CMD_LEN_MAX);
        
        } else {                                                        /* ATA ����ģʽ                 */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd flag ata ctrl %d port %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand;
            
            if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) ||
                (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NON_SEC_DATA)) {  /* LBA ģʽ������չ����         */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba true flag non sec data ctrl %d port %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, uiDrive);
                pucCommandFis[4] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba;
                pucCommandFis[5] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 8);
                pucCommandFis[6] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 16);
                pucCommandFis[7] = AHCI_SDH_LBA;
                
                if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {              /* LBA 48 ģʽ                  */
                    pucCommandFis[ 8] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24);
                    pucCommandFis[ 9] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 32);
                    pucCommandFis[10] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 40);
                
                } else {                                                /* �� LBA 48 ģʽ               */
                    pucCommandFis[ 7] = (UINT8)(pucCommandFis[7] 
                                      | ((hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24) & 0x0f));
                    pucCommandFis[ 8] = 0;
                    pucCommandFis[ 9] = 0;
                    pucCommandFis[10] = 0;
                }
            
            } else {                                                    /* �� LBA ģʽ                  */
                UINT16      usCylinder;                                 /* ����                         */
                UINT16      usHead;                                     /* ��ͷ                         */
                UINT16      usSector;                                   /* ����                         */
                
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba false flag sec data ctrl %d port %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, uiDrive);
                usCylinder = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba /
                                      (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usSector   = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba %
                                      (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usHead     = usSector / hDrive->AHCIDRIVE_uiSector;
                usSector   = usSector % hDrive->AHCIDRIVE_uiSector + 1;

                pucCommandFis[ 4] = (UINT8)usSector;
                pucCommandFis[ 5] = (UINT8)(usCylinder >> 8);
                pucCommandFis[ 6] = (UINT8)usCylinder;
                pucCommandFis[ 7] = (UINT8)(AHCI_SDH_IBM | (usHead & 0x0f));
                pucCommandFis[ 8] = 0;
                pucCommandFis[ 9] = 0;
                pucCommandFis[10] = 0;
            }

            if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {             /* ʹ�� NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag ncq ctrl %d port %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, uiDrive);
                pucCommandFis[ 3] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount;
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
                pucCommandFis[12] = (UINT8)(uiTag << 3);
                pucCommandFis[13] = 0;
                pucCommandFis[ 7] = 0x40;
            
            } else {                                                    /* ���� NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag no ncq ctrl %d port %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, uiDrive);
                pucCommandFis[ 3] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature);
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature >> 8);
                pucCommandFis[12] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount);
                pucCommandFis[13] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
            }
            
            pucCommandFis[14] = 0;
            pucCommandFis[15] = AHCI_CTL_4BIT;
            pucCommandFis[16] = 0;
            pucCommandFis[17] = 0;
            pucCommandFis[18] = 0;
            pucCommandFis[19] = 0;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                     pucCommandFis[0], pucCommandFis[1],pucCommandFis[2], pucCommandFis[3],
                     pucCommandFis[4], pucCommandFis[5],pucCommandFis[6], pucCommandFis[7]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                     pucCommandFis[ 8], pucCommandFis[ 9], pucCommandFis[10], pucCommandFis[11],
                     pucCommandFis[12], pucCommandFis[13], pucCommandFis[14], pucCommandFis[15]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                     pucCommandFis[16], pucCommandFis[17], pucCommandFis[18], pucCommandFis[19],
                     pucCommandFis[20], pucCommandFis[21], pucCommandFis[22], pucCommandFis[23]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                     pucCommandFis[24], pucCommandFis[25], pucCommandFis[26], pucCommandFis[27],
                     pucCommandFis[28], pucCommandFis[29], pucCommandFis[30], pucCommandFis[31]);

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_NONE) {           /* ��Ч����                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "data dir none ctrl %d port %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive);
            uiFlagsPrdLength = 5;
        
        } else {                                                        /* ��Ч����                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "data dir %s.\r\n",
                         hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN ? "read" : "write");
                                                                        /* ���� PRDT                    */
            uiPrdtCount = __ahciPrdtSetup(hCmd->AHCICMD_pucDataBuf, hCmd->AHCICMD_ulDataLen, hPrdt);
            if (uiPrdtCount == 0) {                                     /* ����ʧ��                     */
                goto    __error_handle;
            }
                                                                        /* ���ñ�־                     */
            uiFlagsPrdLength = (UINT32)(uiPrdtCount << AHCI_CMD_LIST_PRDTL_SHFT) | 5;
            if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {        /* ���ģʽ                     */
                uiFlagsPrdLength |= AHCI_CMD_LIST_W;                    /* д������־                   */
            }
            if (!(hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ)) {          /* �� NCQ ģʽ                  */
                uiFlagsPrdLength |= AHCI_CMD_LIST_P;                    /* ���� NCQ                     */
            }
        }
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ģʽ                   */
            uiFlagsPrdLength |= (AHCI_CMD_LIST_A | AHCI_CMD_LIST_P);
        }
        hCommandList->AHCICMDLIST_uiPrdtFlags = cpu_to_le32(uiFlagsPrdLength);
        hCommandList->AHCICMDLIST_uiByteCount = 0;

        API_CacheDmaFlush(hCommandList, sizeof(AHCI_CMD_LIST_CB));      /* ��д�����б���Ϣ             */
        API_CacheDmaFlush(pucCommandFis, 20);                           /* ��д������Ϣ                 */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ģʽ                   */
            API_CacheDmaFlush(pucPktCmd, AHCI_ATAPI_CMD_LEN_MAX);       /* ��д������Ϣ                 */
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "flag prdt length 0x%08x.\r\n", hCommandList->AHCICMDLIST_uiPrdtFlags);
        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {            /* ���ģʽ                     */
                                                                        /* ��д������                   */
            API_CacheDmaFlush(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }

        LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel); /* �ȴ�����Ȩ                   */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {                 /* NCQ ģʽ                     */
            AHCI_PORT_WRITE(hDrive, AHCI_PxSACT, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxSACT);
        }
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        AHCI_PORT_READ(hDrive, AHCI_PxCI);
        hDrive->AHCIDRIVE_uiCmdMask |= uiTagBit;                        /* �������λ                   */
        LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);/* �ͷſ���Ȩ                   */

        AHCI_CMD_LOG(AHCI_LOG_PRT, "start command ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);

        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_WAIT_SPINUP) {         /* ���³�ʱʱ�����             */
            ulWait = API_TimeGetFrequency() * 20;
        } else {                                                        /* ʹ�ó�ʼ��ʱʱ�����         */
            ulWait = hCtrl->AHCICTRL_ulSemTimeout;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "sem take tag 0x%08x wait 0x%08x.\r\n", uiTag, ulWait);
                                                                        /* �ȴ��������                 */
        ulRet = API_SemaphoreBPend(hDrive->AHCIDRIVE_hSyncBSem[uiTag], ulWait);
        AHCI_CMD_LOG(AHCI_LOG_PRT, "sem take tag 0x%08x wait 0x%08x end.\r\n", uiTag, ulWait);
        if (ulRet != ERROR_NONE) {                                      /* ����ʧ��                     */
            AHCI_CMD_LOG(AHCI_LOG_ERR, "ctrl %d drive %d sync sem timeout ata cmd %02x "
                         "tag %d tagbit 0x%08x cmdmask 0x%08x pxsact 0x%08x pxci 0x%08x intcount %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive, hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand,
                         uiTag, uiTagBit, hDrive->AHCIDRIVE_uiCmdMask,
                         AHCI_PORT_READ(hDrive, AHCI_PxSACT), AHCI_PORT_READ(hDrive, AHCI_PxCI),
                         hDrive->AHCIDRIVE_uiIntCount);
            goto    __error_handle;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "state check ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
        if (((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
             (hDrive->AHCIDRIVE_ucState != AHCI_DEV_INIT) &&
             (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) ||
            (hDrive->AHCIDRIVE_bPortError)) {                           /* ������״̬����               */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "port error cmd %02x ctrl %d drive %d state %d error %d.\r\n",
                         hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand, hCtrl->AHCICTRL_uiIndex, uiDrive,
                         hDrive->AHCIDRIVE_ucState, hDrive->AHCIDRIVE_bPortError);
            hDrive->AHCIDRIVE_bPortError = LW_FALSE;
            goto    __error_handle;
        }

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN) {             /* ����ģʽ                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "read buffer invalidate ctrl %d port %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive);
                                                                        /* ��Ч������                   */
            API_CacheDmaInvalidate(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }
        
        __ahciCmdReleaseResource(hDrive, bQueued);                      /* �ͷſ���Ȩ                   */
    }

    return  (ERROR_NONE);                                               /* ��ȷ����                     */

__error_handle:
    __ahciCmdReleaseResource(hDrive, bQueued);                          /* �ͷſ���Ȩ                   */

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_AhciDiskAtaParamGet
** ��������: ��ȡ ATA ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pvBuf      ���������� (��С�����ϲ���д���)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciDiskAtaParamGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, PVOID  pvBuf)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* ������                     */

    AHCI_LOG(AHCI_LOG_PRT, "ata parameter get ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    hCmd   = &tCtrlCmd;                                                 /* ��ȡ������                 */
    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {
        return  (PX_ERROR);
    }
    
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READP;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_NON_SEC_DATA;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuf, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDiskAtapiParamGet
** ��������: ��ȡ ATAPI ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pvBuf      ���������� (��С�����ϲ���д���)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciDiskAtapiParamGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, PVOID  pvBuf)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* ������                     */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "atapi parameter get ctrl %d port %d.\r\n",
                   hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    hCmd   = &tCtrlCmd;                                                 /* ��ȡ������                 */
    
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_PI_CMD_IDENTD;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = 0;

    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCommand = AHCI_PI_CMD_IDENTD;

    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_NON_SEC_DATA;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuf, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskFlushCache
** ��������: �����д
** �䡡��  : hDev      �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if AHCI_CACHE_EN > 0

static INT  __ahciDiskFlushCache (AHCI_DEV_HANDLE  hDev)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_PARAM_HANDLE   hParam = LW_NULL;                               /* �������                     */
    UINT8               ucCmd;                                          /* ��ʹ�õ�����                 */

    AHCI_LOG(AHCI_LOG_PRT, "disk flush ctrl %d port %d start.\r\n",
             hDev->AHCIDEV_hCtrl->AHCICTRL_uiIndex, hDev->AHCIDEV_uiDrive);

    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    hParam = &hDrive->AHCIDRIVE_tParam;

    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_WCACHE_SUPPORTED) {
        if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {
            ucCmd = AHCI_CMD_FLUSH_CACHE_EXT;
        } else {
            ucCmd = AHCI_CMD_FLUSH_CACHE;
        }
        iRet = API_AhciNoDataCommandSend(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive,
                                         ucCmd, 0, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "disk flush no data command failed ctrl %d port %d.\r\n",
                     hDev->AHCIDEV_hCtrl->AHCICTRL_uiIndex, hDev->AHCIDEV_uiDrive);
            return  (PX_ERROR);
        }
    
    } else {
        return  (ERROR_NONE);
    }

    AHCI_LOG(AHCI_LOG_PRT, "disk flush ctrl %d port %d end.\r\n",
             hDev->AHCIDEV_hCtrl->AHCICTRL_uiIndex, hDev->AHCIDEV_uiDrive);

    return  (ERROR_NONE);
}

#endif                                                                  /*  AHCI_CACHE_EN > 0           */
/*********************************************************************************************************
** ��������: __ahciLbaRangeEntriesSet
** ��������: ����������Ŀ
** �䡡��  : hDev           �豸���
**           ulStartLba     ��ʼ����
**           ulSectors      ������
** �䡡��  : �ֽ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if AHCI_TRIM_EN > 0                                                    /* AHCI_TRIM_EN                 */

static ULONG  __ahciLbaRangeEntriesSet (PVOID  pvBuffer, ULONG  ulBuffSize, ULONG ulSector, ULONG ulCount)
{
    INT             i    = 0;
    INT             iCnt = (INT)(ulBuffSize / AHCI_TRIM_TCB_SIZE);
    UINT64         *pullBuffer;
    UINT64          ullEntry = 0;
    ULONG           ulBytes  = 0;

    /*
     *  6-byte LBA + 2-byte range per entry
     */
    pullBuffer = (UINT64 *)pvBuffer;
    
    while (i < iCnt) {
        ullEntry = ulSector |
                   ((UINT64)(ulCount > AHCI_TRIM_BLOCK_MAX ? AHCI_TRIM_BLOCK_MAX : ulCount) << 48);

        pullBuffer[i++] = cpu_to_le64(ullEntry);
        if (ulCount <= AHCI_TRIM_BLOCK_MAX) {
            break;
        }
        
        ulCount  -= AHCI_TRIM_BLOCK_MAX;
        ulSector += AHCI_TRIM_BLOCK_MAX;
    }

    ulBytes = ALIGN(i * AHCI_TRIM_TCB_SIZE, (AHCI_TRIM_TCB_SIZE * AHCI_TRIM_TCB_MAX));
    lib_bzero(pullBuffer + i, ulBytes - (i * AHCI_TRIM_TCB_SIZE));

    return  (ulBytes);
}
/*********************************************************************************************************
** ��������: __ahciDiskTrimSet
** ��������: TRIM ����
** �䡡��  : hDev               �豸���
**           ulStartSector      ��ʼ����
**           ulEndSector        ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskTrimSet (AHCI_DEV_HANDLE  hDev, ULONG  ulStartSector, ULONG  ulEndSector)
{
    INT                 iRet;                                           /* �������                     */
    REGISTER INT        i;                                              /* ѭ������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    ULONG               ulDataLen;                                      /* ���ݳ���                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */
    UINT32              uiCmdCount;                                     /* ������Ŀ����                 */
    UINT32              uiSector;                                       /* ����������                   */
    ULONG               ulSectors;                                      /* ��������                     */

    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    hCmd   = &tCtrlCmd;                                                 /* ��ȡ������                 */

    if (hDrive->AHCIDRIVE_bTrim != LW_TRUE) {                           /* TRIM ��֧��                  */
        AHCI_LOG(AHCI_LOG_PRT, "trim not support ctrl %d port %d.\r\n",
                 hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);
        return  (ERROR_NONE);
    }

    AHCI_LOG(AHCI_LOG_PRT, "trim ctrl %d drive %d start %lu end %lu offset %lu count %lu.\r\n",
             hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
             ulStartSector, ulEndSector, hDev->AHCIDEV_ulBlkOffset, hDev->AHCIDEV_ulBlkCount);
    /*
     *  ������������Ƿ���ȷ
     */
    if ((ulStartSector > ulEndSector) ||
        (ulStartSector < hDev->AHCIDEV_ulBlkOffset) ||
        (ulEndSector > (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1))) {
        AHCI_LOG(AHCI_LOG_ERR, "sector error start sector %lu end sector %lu [%lu - %lu].\r\n",
                 ulStartSector, ulEndSector,
                 hDev->AHCIDEV_ulBlkOffset, (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1));
        return  (PX_ERROR);
    }
    /*
     *  ������������
     */
    ulSectors  = ulEndSector - ulStartSector + 1;
    uiCmdCount = (UINT32)((ulSectors + AHCI_TRIM_CMD_BLOCK_MAX - 1) / AHCI_TRIM_CMD_BLOCK_MAX);
    AHCI_LOG(AHCI_LOG_PRT, "sectors %lu cmd count %lu.\r\n", ulSectors, uiCmdCount);
    
    for (i = 0; i < uiCmdCount; i++) {
        uiSector  = (UINT32)((ulSectors >= AHCI_TRIM_CMD_BLOCK_MAX) ? AHCI_TRIM_CMD_BLOCK_MAX : ulSectors);
        ulDataLen = __ahciLbaRangeEntriesSet(hDrive->AHCIDRIVE_pucAlignDmaBuf,
                                             hDrive->AHCIDRIVE_ulByteSector,
                                             ulStartSector, uiSector);
#if (AHCI_LOG_PRT & AHCI_LOG_LEVEL)                                     /* AHCI_LOG_LEVEL               */
        {
            REGISTER INT        j;
            
            API_CacheDmaFlush(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);

            AHCI_LOG(AHCI_LOG_PRT, "start sector %lu one sector %lu sectors %lu data len %lu.\r\n",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            AHCI_LOG(AHCI_LOG_PRT, "start sector %08x one sector %08x sectors %08x data len %08x.\r\n",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            
            for (j = 0; j < hDrive->AHCIDRIVE_ulByteSector / 8; j++) {
                AHCI_LOG(AHCI_LOG_PRT, "%02d  %02x%02x %02x%02x%02x%02x%02x%02x\r\n",
                         j,
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 7],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 6],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 5],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 4],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 3],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 2],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 1],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 0]);
            }
        }
#endif                                                                  /* AHCI_LOG_LEVEL               */
        /*
         *  ��������
         */
        lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
        hCmd->AHCICMD_ulDataLen  = ulDataLen;
        hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_DSM;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = 1;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0x01;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = 0;
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_TRIM;

        AHCI_LOG(AHCI_LOG_PRT, "send trim cmd ctrl %d port %d.\r\n",
                 hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);
        
        iRet = API_AhciDiskCommandSend(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive, hCmd);
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "trim failed ctrl %d port %d.\r\n",
                     hDrive->AHCIDRIVE_hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort);
            return  (PX_ERROR);
        }

        ulSectors     -= uiSector;
        ulStartSector += uiSector;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* AHCI_TRIM_EN                 */
/*********************************************************************************************************
** ��������: API_AhciDiskTrimSet
** ��������: ��ָ��������д����
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           ulStartSector  ��ʼ����
**           ulEndSector    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDiskTrimSet (AHCI_CTRL_HANDLE  hCtrl,
                          UINT              uiDrive,
                          ULONG             ulStartSector,
                          ULONG             ulEndSector)
{
#if (AHCI_TRIM_EN > 0)                                                  /* AHCI_TRIM_EN                 */
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];

    iRet = __ahciDiskTrimSet(hDrive->AHCIDRIVE_hDev, ulStartSector, ulEndSector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciReadWrite
** ��������: ��ָ��������д����
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           pvBuf          ���ݻ�����
**           ulLba          ��ʼ����
**           ulSectors      ������
**           uiDirection    ���ݴ��䷽��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  __ahciReadWrite (AHCI_CTRL_HANDLE  hCtrl,
                             UINT              uiDrive,
                             PVOID             pvBuf,
                             ULONG             ulLba,
                             ULONG             ulSectors,
                             UINT              uiDirection)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */

    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    
    /*
     *  ��������
     */
    hCmd->AHCICMD_ulDataLen  = ulSectors * hDrive->AHCIDRIVE_ulByteSector;
    hCmd->AHCICMD_pucDataBuf = (UINT8 *)pvBuf;
    hCmd->AHCICMD_iFlags     = 0;
    
    /*
     *  ��ȡ����ģʽ
     */
    if (uiDirection == O_WRONLY) {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                      (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_MULTI_EXT : AHCI_CMD_WRITE_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                  (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_EXT : AHCI_CMD_WRITE);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_WRITE_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                          (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_DMA_EXT : AHCI_CMD_WRITE_DMA);
            }
        }
    
    } else {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
        
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                        (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_MULTI_EXT : AHCI_CMD_READ_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                    (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_EXT : AHCI_CMD_READ);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READ_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                            (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_DMA_EXT : AHCI_CMD_READ_DMA);
            }
        }
    }
    
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = (UINT16)ulSectors;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = (UINT64)ulLba;

    iRet = API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciBlkReadWrite
** ��������: ���豸��д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
**           uiDirection    ��дģʽ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkReadWrite (AHCI_DEV_HANDLE  hDev,
                                PVOID            pvBuffer,
                                ULONG            ulBlkStart,
                                ULONG            ulBlkCount,
                                UINT             uiDirection)
{
    REGISTER UINT32     i;                                              /* ѭ������                     */
    INT                 iRet = PX_ERROR;                                /* �������                     */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    PLW_BLK_DEV         hBlkDev;                                        /* ���豸���                   */
    ULONG               ulLba    = 0;                                   /* LBA ����                     */
    ULONG               ulSector = 0;                                   /* ��������                     */
    AHCI_MSG_CB         tCtrlMsg;                                       /* ��Ϣ���ƿ�                   */
    AHCI_MSG_HANDLE     hCtrlMsg = LW_NULL;                             /* ��Ϣ���                     */

    hCtrl   = hDev->AHCIDEV_hCtrl;                                      /* ��ȡ���������               */
    hDrive  = hDev->AHCIDEV_hDrive;                                     /* ��ȡ���������               */
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */

    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {                   /* ״̬����                     */
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d ata block start %lu count %lu buff %p dir is %s.\r\n",
             hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, ulBlkStart, ulBlkCount, pvBuffer,
             (uiDirection == O_WRONLY) ? "write" : "read");

    iRet = API_AhciPmActive(hCtrl, hDev->AHCIDEV_uiDrive);              /* ʹ�ܵ�Դ                     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    ulSector = (ULONG)hBlkDev->BLKD_ulNSector;
    if ((ulBlkStart + ulBlkCount) > (ULONG)ulSector) {                  /* ������������                 */
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d ata block start %lu count %lu [0 - %lu].\r\n",
                 hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, ulBlkStart, ulBlkCount, ulSector);
        return  (PX_ERROR);
    }

    ulBlkStart += (ULONG)hDev->AHCIDEV_ulBlkOffset;
    for (i = 0; i < (UINT32)ulBlkCount; i += ulSector) {                /* ѭ��������������             */
        ulLba = (ULONG)ulBlkStart;

        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                ulSector = __MIN((ulBlkCount - (ULONG)i), (ULONG)hDrive->AHCIDRIVE_usMultiSector);
            } else {
                ulSector = 1;
            }
        
        } else if ((hDrive->AHCIDRIVE_bLba48) || (hDrive->AHCIDRIVE_bLba)) {
            ulSector = __MIN((UINT32)ulBlkCount - i, (UINT32)hDrive->AHCIDRIVE_ulSectorMax);
        }

        iRet = __ahciReadWrite(hCtrl, hDev->AHCIDEV_uiDrive, pvBuffer, ulLba, ulSector, uiDirection);
        if (iRet != ERROR_NONE) {                                       /* ���ִ���                     */
            goto    __error_handle;
        }

        ulBlkStart += (ULONG)ulSector;                                  /* ������ʼ����                 */
        pvBuffer    = (UINT8 *)pvBuffer
                    + (hBlkDev->BLKD_ulBytesPerSector
                    *  ulSector);                                       /* ���»�����                   */
    }

    return  (ERROR_NONE);

__error_handle:
    /*
     *  ���ʹ�����Ϣ�Ա����߳̽��д���
     */
    hDrive->AHCIDRIVE_uiTimeoutErrorCount++;
    hCtrlMsg = &tCtrlMsg;
    hCtrlMsg->AHCIMSG_uiMsgId = AHCI_MSG_TIMEOUT;
    hCtrlMsg->AHCIMSG_uiDrive = hDev->AHCIDEV_uiDrive;
    hCtrlMsg->AHCIMSG_hCtrl   = hCtrl;
    API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)hCtrlMsg, AHCI_MSG_SIZE);
    
    _ErrorHandle(EIO);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ahciBlkReset
** ��������: ���豸��λ
** �䡡��  : hDev  �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkReset (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciBlkStatusChk
** ��������: ���豸���
** �䡡��  : hDev  �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkStatusChk (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciBlkIoctl
** ��������: ���豸 ioctl
** �䡡��  : hDev   �豸���
**           iCmd   ��������
**           lArg   ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : FIOTRIM �������д������Ѿ��� disk cache ����õ���֤.
*********************************************************************************************************/
static INT  __ahciBlkIoctl (AHCI_DEV_HANDLE  hDev, INT  iCmd, LONG  lArg)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    PLW_BLK_INFO        hBlkInfo;                                       /* �豸��Ϣ                     */
#if AHCI_SMART_EN > 0                                                   /* ʹ�� SMART ����              */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������ƾ��                 */
#endif                                                                  /* AHCI_SMART_EN                */

    hCtrl  = hDev->AHCIDEV_hCtrl;                                       /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    switch (iCmd) {

    /*
     *  ����Ҫ֧�ֵ�����
     */
    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /* ������д�����               */
#if AHCI_CACHE_EN > 0                                                   /* ʹ�� cache ��д              */
    {
        INT     iRet;                                                   /* �������                     */

        if (hDev->AHCIDEV_iCacheFlush) {
            iRet = __ahciDiskFlushCache(hDev);
            if (iRet != ERROR_NONE) {
                return  (PX_ERROR);
            }
        }
    }
#endif                                                                  /* AHCI_CACHE_EN                */
    
    case FIOUNMOUNT:                                                    /* ж�ؾ�                       */
    case FIODISKINIT:                                                   /* ��ʼ������                   */
    case FIODISKCHANGE:                                                 /* ����ý�ʷ����仯             */
        break;

    case FIOTRIM:
#if AHCI_TRIM_EN > 0                                                    /* ʹ�� TRIM ����               */
    {
        INT                 iRet;                                       /* �������                     */
        PLW_BLK_RANGE       hBlkRange;                                  /* ���豸������Χ����           */

        hBlkRange = (PLW_BLK_RANGE)lArg;
        iRet = __ahciDiskTrimSet(hDev, hBlkRange->BLKR_ulStartSector, hBlkRange->BLKR_ulEndSector);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
        break;
    /*
     *  �ͼ���ʽ��
     */
    case FIODISKFORMAT:                                                 /* ��ʽ����                     */
        return  (PX_ERROR);                                             /* ��֧�ֵͼ���ʽ��             */

    /*
     *  FatFs ��չ����
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((ULONG *)lArg) = hDrive->AHCIDRIVE_ulByteSector;
        break;

    case LW_BLKD_GET_SECNUM:
        *((ULONG *)lArg) = (ULONG)API_AhciDriveSectorCountGet(hCtrl, hDev->AHCIDEV_uiDrive);
        break;

    case LW_BLKD_CTRL_INFO:
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_SATA;
        API_AhciDriveSerialInfoGet(hDrive, hBlkInfo->BLKI_cSerial,   LW_BLKD_CTRL_INFO_STR_SZ);
        API_AhciDriveFwRevInfoGet(hDrive,  hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ);
        API_AhciDriveModelInfoGet(hDrive,  hBlkInfo->BLKI_cProduct,  LW_BLKD_CTRL_INFO_STR_SZ);
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

#if AHCI_SMART_EN > 0                                                   /* ʹ�� SMART ����              */
    case LW_BLKD_SATA_CMD_CTL:
        hCmd = (AHCI_CMD_HANDLE)lArg;
        if (!hCmd) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        AHCI_LOG(AHCI_LOG_PRT,
                 "ctrl %d drive %d block ioctl cmd 0x%02x feature 0x%08x buff0 0x%02x buff1 0x%02x.\r\n",
                 hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature,
                 hCmd->AHCICMD_pucDataBuf[0], hCmd->AHCICMD_pucDataBuf[1]);

        if (hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand == AHCI_CMD_SET_FEATURE) {
            switch (hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature) {

            case AHCI_SUB_ENABLE_APM:
                return  (API_AhciApmEnable(hCtrl, hDev->AHCIDEV_uiDrive, hCmd->AHCICMD_pucDataBuf[0]));

            case AHCI_SUB_DISABLE_APM:
                return  (API_AhciApmDisable(hCtrl, hDev->AHCIDEV_uiDrive));

            case AHCI_SUB_INFO_APM:
                return  (API_AhciApmModeGet(hCtrl, hDev->AHCIDEV_uiDrive, hCmd->AHCICMD_pucDataBuf));

            default:
                _ErrorHandle(ENOSYS);
                return  (PX_ERROR);
            }
        }

        if (hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand == AHCI_CMD_CHECK_POWER_LEVEL) {
            return  (API_AhciPmPowerModeGet(hCtrl, hDev->AHCIDEV_uiDrive, hCmd->AHCICMD_pucDataBuf));
        }

        if (hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand == AHCI_CMD_SMART) {
            switch (hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature) {

            case AHCI_SMART_CMD_DATA_READ:
                return  (API_AhciSmartDataGet(hCtrl, hDev->AHCIDEV_uiDrive,
                                              (PVOID)hCmd->AHCICMD_pucDataBuf));

            case AHCI_SMART_CMD_THRESHOLDS_READ:
                return  (API_AhciSmartThresholdGet(hCtrl, hDev->AHCIDEV_uiDrive,
                                                   (PVOID)hCmd->AHCICMD_pucDataBuf));

            case AHCI_SMART_CMD_AUTOSAVE_EN_DIS:
                return  (API_AhciSmartAttrAutoSaveSet(hCtrl, hDev->AHCIDEV_uiDrive,
                                                      hCmd->AHCICMD_pucDataBuf[0]));

            case AHCI_SMART_CMD_ATTR_SAVE:
                return  (API_AhciSmartAttrSaveSet(hCtrl, hDev->AHCIDEV_uiDrive));

            case AHCI_SMART_CMD_OFFLINE_DIAGS:
                return  (API_AhciSmartOfflineDiag(hCtrl, hDev->AHCIDEV_uiDrive,
                                                  hCmd->AHCICMD_pucDataBuf[0]));

            case AHCI_SMART_CMD_LOG_SECTOR_READ:
                return  (API_AhciSmartLogSectorRead(hCtrl, hDev->AHCIDEV_uiDrive,
                                                    hCmd->AHCICMD_pucDataBuf[0],
                                                    hCmd->AHCICMD_pucDataBuf[1],
                                                    (PVOID)hCmd->AHCICMD_pucDataBuf));

            case AHCI_SMART_CMD_LOG_SECTOR_WRITE:
                return  (API_AhciSmartLogSectorWrite(hCtrl, hDev->AHCIDEV_uiDrive,
                                                     (AHCI_CMD_HANDLE)lArg));

            case AHCI_SMART_CMD_SMART_ENABLE:
                return  (API_AhciSmartEnableSet(hCtrl, hDev->AHCIDEV_uiDrive));

            case AHCI_SMART_CMD_SMART_DISABLE:
                return  (API_AhciSmartDisableSet(hCtrl, hDev->AHCIDEV_uiDrive));

            case AHCI_SMART_CMD_RETURN_STATUS:
                return  (API_AhciSmartStatusGet(hCtrl, hDev->AHCIDEV_uiDrive, hCmd->AHCICMD_pucDataBuf));

            case AHCI_SMART_CMD_AUTO_OFFLINE:
                return  (API_AhciSmartAutoOffline(hCtrl, hDev->AHCIDEV_uiDrive,
                                                  hCmd->AHCICMD_pucDataBuf[0]));

            default:
                _ErrorHandle(ENOSYS);
                return  (PX_ERROR);
            }
        }

        if (hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand == AHCI_CMD_READP) {
            return  (API_AhciDiskAtaParamGet(hCtrl, hDev->AHCIDEV_uiDrive, hCmd->AHCICMD_pucDataBuf));
        }

        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
#endif                                                                  /* AHCI_SMART_EN                */

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciBlkWr
** ��������: ���豸д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkWr (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_WRONLY));
}
/*********************************************************************************************************
** ��������: __ahciBlkRd
** ��������: ���豸������
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkRd (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_RDONLY));
}
/*********************************************************************************************************
** ��������: __ahciPiCommandSend
** ��������: �� ATAPI ��������������
** �䡡��  : hDev       �豸���
**           hCmd       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */

static INT  __ahciPiCommandSend (AHCI_DEV_HANDLE  hDev, AHCI_CMD_HANDLE  hCmd)
{
    INT                     iRet = PX_ERROR;                            /* �������                     */
    AHCI_CTRL_HANDLE        hCtrl;                                      /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    UINT8                   ucError = 0;                                /* ������Ϣ                     */
    INT                     iRetryNum = 0;                              /* ���Լ���                     */

    hCtrl  = hDev->AHCIDEV_hCtrl;                                       /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */

    do {
        ucError = 0;
        iRet = API_AhciDiskCommandSend(hCtrl, hDev->AHCIDEV_uiDrive, hCmd);
        if (iRet == PX_ERROR) {
            AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                           "ctrl %d drive %d atapi cmd send status 0x%08x error 0x%08x ret %d retry %d"
                           " cmd 0x%02x cmdbuff[0-0x%02x] cmdbuff[1-0x%02x] cmdbuff[2-0x%02x].\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                           hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError, iRet, iRetryNum,
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCommand,
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0],
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[1],
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[2]);
        }

        if ((iRet == ERROR_NONE) ||
            ((hDrive->AHCIDRIVE_uiIntStatus & AHCI_STAT_ERR) == 0)) {
            AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                           "ctrl %d drive %d atapi cmd send status 0x%08x error 0x%08x ret %d state ok"
                           " cmd 0x%02x cmdbuff[0-0x%02x] cmdbuff[1-0x%02x] cmdbuff[2-0x%02x].\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                           hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError, iRet,
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCommand,
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0],
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[1],
                           hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[2]);
            return (ERROR_NONE);
        }

        ucError = (UINT8)(hDrive->AHCIDRIVE_uiIntError & 0xff);
        switch (ucError & AHCI_ERR_SENSE_KEY) {

        case AHCI_SENSE_NO_SENSE:
            hDev->AHCIDEV_iErrNum = 11;
            break;

        case AHCI_SENSE_RECOVERED_ERROR:
            hDev->AHCIDEV_iErrNum = 12;
            break;

        case AHCI_SENSE_NOT_READY:
            hDev->AHCIDEV_iErrNum = 13;
            break;

        case AHCI_SENSE_MEDIUM_ERROR:
            hDev->AHCIDEV_iErrNum = 14;
            break;

        case AHCI_SENSE_HARDWARE_ERROR:
            hDev->AHCIDEV_iErrNum = 15;
            break;

        case AHCI_SENSE_ILLEGAL_REQUEST:
            hDev->AHCIDEV_iErrNum = 16;
            break;

        case AHCI_SENSE_UNIT_ATTENTION:
            hDev->AHCIDEV_iErrNum = 17;
            break;

        case AHCI_SENSE_DATA_PROTECT:
            hDev->AHCIDEV_iErrNum = 18;
            break;

        case AHCI_SENSE_ABBORTED_COMMAND:
            hDev->AHCIDEV_iErrNum = 19;
            break;

        case AHCI_SENSE_MISCOMPARE:
            hDev->AHCIDEV_iErrNum = 20;
            break;

        default:
            hDev->AHCIDEV_iErrNum = 0;
            break;
        }

        AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                       "ctrl %d drive %d atapi error index %d info %s "
                       "status 0x%08x error 0x%08x cmd 0x%02x retry %d\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                       hDev->AHCIDEV_iErrNum, _GcpcAhciAtapiErrStrs[hDev->AHCIDEV_iErrNum],
                       hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError,
                       hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0], iRetryNum);

        if (ucError == 0) {
            AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi error invalidate\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
            return  (PX_ERROR);
        }

        if ((ucError & AHCI_ERR_SENSE_KEY) == AHCI_SENSE_UNIT_ATTENTION) {
            AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi sense attention\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;
        }

        return  (PX_ERROR);
    } while (++iRetryNum < (AHCI_RETRY_NUM * 2));

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi retry num %d [0 - %d].\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, iRetryNum, (AHCI_RETRY_NUM * 2));

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ahciPiMediaEventGet
** ��������: ��ȡ�¼���Ϣ
** �䡡��  : hDev           �豸���
**           pucBuff        ������
**           stSize         ��������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiMediaEventGet (AHCI_DEV_HANDLE  hDev, UINT8 *pucBuff, size_t  stSize)
{
    INT                     iRet;                                       /* �������                     */
    REGISTER INT            i;                                          /* ѭ������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_CMD_CB             tCtrlCmd;                                   /* ������ƿ�                   */
    AHCI_CMD_HANDLE         hCmd;                                       /* ������                     */
    UINT8                  *pucBuf;

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi media event get start.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    hCmd   = &tCtrlCmd;                                                 /* ��ȡ������                 */

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 8;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    lib_bzero(&hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0], AHCI_ATAPI_CMD_LEN_MAX);
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0] = AHCI_CDROM_CMD_GET_EVENT_STATUS_AU;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[1] = 0x01;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[4] = 0x01 << 4;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[8] = 8;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature   = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_ATAPI;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = __ahciPiCommandSend(hDev, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi media event get cmd error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }
    pucBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi media event get buffer data:\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\nindex ");
    for (i = 0; i < 20; i++) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "  %02d ", i);
    }
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\ndata  ");
    for (i = 0; i < 20; i++) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "0x%02x ", pucBuf[i]);
    }
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\n");

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi media event get ok.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    if ((pucBuff) && (stSize > 0)) {
        lib_memcpy(pucBuff, pucBuf, __MIN(stSize, 8));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiTestUnitReady
** ��������: ̽���豸�Ƿ��Ѿ�����
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiTestUnitReady (AHCI_DEV_HANDLE  hDev)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi test unit ready state start.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 0;
    hCmd->AHCICMD_pucDataBuf = LW_NULL;
    lib_bzero(&hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0], AHCI_ATAPI_CMD_LEN_MAX);
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0] = AHCI_CDROM_CMD_TEST_UNIT_READY_CR;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_NONE;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_ATAPI;

    iRet = __ahciPiCommandSend(hDev, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi test unit ready cmd send error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi test unit ready state ok.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiStartStopUnit
** ��������: ������ֹͣ�豸
** �䡡��  : hDev       �豸���
**           iArg       �򿪹رղ��� (�� 0x02, �ر� 0x03)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiStartStopUnit (AHCI_DEV_HANDLE  hDev, INT  iArg)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi start stop unit state start.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 0;
    hCmd->AHCICMD_pucDataBuf = LW_NULL;
    lib_bzero(&hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0], AHCI_ATAPI_CMD_LEN_MAX);
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0] = AHCI_CDROM_CMD_START_STOP_UNIT;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[4] = (UINT8)iArg;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_NONE;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_ATAPI;

    iRet = __ahciPiCommandSend(hDev, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi start stop unit cmd send error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi start stop unit state ok.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiCapacityRead
** ��������: ��ȡ�豸������������Ϣ
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiCapacityRead (AHCI_DEV_HANDLE  hDev)
{
    INT                     iRet;                                       /* �������                     */
    REGISTER INT            i;                                          /* ѭ������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    PLW_BLK_DEV             hBlkDev;                                    /* ���豸���                   */
    AHCI_CMD_CB             tCtrlCmd;                                   /* ������ƿ�                   */
    AHCI_CMD_HANDLE         hCmd;                                       /* ������                     */
    UINT8                  *pucBuf;                                     /* ���ݻ���                     */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi capacity read.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    hDrive  = hDev->AHCIDEV_hDrive;                                     /* ��ȡ���������               */
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */
    hCmd    = &tCtrlCmd;                                                /* ��ȡ������                 */

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 8;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    lib_bzero(&hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0], AHCI_ATAPI_CMD_LEN_MAX);
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0] = AHCI_CDROM_CMD_READ_CAPACITY_CR;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature   = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_ATAPI;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = __ahciPiCommandSend(hDev, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi capacity read cmd send error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }
    pucBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi capacity buffer data:\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\nindex ");
    for (i = 0; i < 20; i++) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "  %02d ", i);
    }
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\ndata  ");
    for (i = 0; i < 20; i++) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "0x%02x ", pucBuf[i]);
    }
    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "\r\n");

    hBlkDev->BLKD_ulNSector        = 0;
    hBlkDev->BLKD_ulBytesPerSector = 0;
    hBlkDev->BLKD_ulBytesPerBlock  = 0;

    for (i = 0; i < 4; i++) {
        hBlkDev->BLKD_ulNSector += ((ULONG)pucBuf[i]) << (8 * (3 - i));
    }
    hBlkDev->BLKD_ulNSector++;

    for (i = 4; i < 8; i++) {
        hBlkDev->BLKD_ulBytesPerSector += ((ULONG)pucBuf[i]) << (8 * (7 - i));
    }
    hBlkDev->BLKD_ulBytesPerBlock = hBlkDev->BLKD_ulBytesPerSector;

    hDrive->AHCIDRIVE_uiSector     = hBlkDev->BLKD_ulNSector;
    hDrive->AHCIDRIVE_ulByteSector = hBlkDev->BLKD_ulBytesPerSector;

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi capacity ok"
                   " sectors %lu[0x%llx] bytes %lu[0x%llx].\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                   hBlkDev->BLKD_ulNSector, hBlkDev->BLKD_ulNSector,
                   hBlkDev->BLKD_ulBytesPerSector, hBlkDev->BLKD_ulBytesPerSector);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkReset
** ��������: ATAPI ���豸��λ
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkReset (AHCI_DEV_HANDLE  hDev)
{

    INT                     iRet;                                       /* �������                     */
    REGISTER INT            i;                                          /* ѭ������                     */
    AHCI_CTRL_HANDLE        hCtrl;                                      /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    PLW_BLK_DEV             hBlkDev;                                    /* ���豸���                   */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev reset.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    hCtrl   = hDev->AHCIDEV_hCtrl;                                      /* ��ȡ���������               */
    hDrive  = hDev->AHCIDEV_hDrive;                                     /* ��ȡ���������               */
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */

    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi driver not install name %s.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, hBlkDev->BLKD_pcName);
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    } else {                                                            /* ֻҪ������װ����ȷ����       */
        return  (ERROR_NONE);                                           /* ��ȷ����                     */
    }

    if (hDrive->AHCIDRIVE_ucState == AHCI_DEV_OK) {                     /* �豸״̬�������踴λ         */
        return  (ERROR_NONE);
    }

    if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH) {
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
        if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_INIT) {
            iRet = hDrive->AHCIDRIVE_pfuncReset(hCtrl, hDev->AHCIDEV_uiDrive);
            if (iRet != ERROR_NONE) {
                API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
                AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi dev reset error.\r\n",
                               hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
                return  (PX_ERROR);
            }
       }

        iRet = __ahciDiskDriveInit(hCtrl, hDev->AHCIDEV_uiDrive);
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
            AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi dev init error.\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
            return  (PX_ERROR);
        }

        API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
    }

    for (i = 0; i < (AHCI_RETRY_NUM * 2); i++) {
        iRet = __ahciPiTestUnitReady(hDev);
        if (iRet == ERROR_NONE) {
            break;
        } else {
            AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi dev test unit ready retry %d.\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, i);
            __ahciPiStartStopUnit(hDev, 0x03);
        }
    }
    if (i >= (AHCI_RETRY_NUM * 2)) {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi test unit ready or start stop unit err.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, i);
        return  (PX_ERROR);
    }

    iRet = __ahciPiCapacityRead(hDev);
    if (iRet != ERROR_NONE)  {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi dev capacity read error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    } else {
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi dev reset ok.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciPiBlkStatusChk
** ��������: ATAPI ���豸״̬���������
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkStatusChk (AHCI_DEV_HANDLE  hDev)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_CTRL_HANDLE        hCtrl;                                      /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    PLW_BLK_DEV             hBlkDev;                                    /* ���豸���                   */

    hCtrl   = hDev->AHCIDEV_hCtrl;                                      /* ��ȡ���������               */
    hDrive  = hDev->AHCIDEV_hDrive;                                     /* ��ȡ���������               */
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi status check state 0x%02x change %d.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                   hDrive->AHCIDRIVE_ucState, hBlkDev->BLKD_bDiskChange);

    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi driver not install.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* ״̬����                     */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi state error 0x%02x.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, hDrive->AHCIDRIVE_ucState);
        return  (PX_ERROR);
    }

    iRet = __ahciPiTestUnitReady(hDev);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi test unit ready err.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }

    if ((hDrive->AHCIDRIVE_ucState == AHCI_DEV_MED_CH) ||
        (hBlkDev->BLKD_bDiskChange == LW_TRUE)) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi status check capacity read.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        iRet = __ahciPiCapacityRead(hDev);
        if (iRet != ERROR_NONE)  {
            AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d status check capacity read error.\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
            return  (PX_ERROR);
        }
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi status check ok state 0x%02x change %d.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                   hDrive->AHCIDRIVE_ucState, hBlkDev->BLKD_bDiskChange);

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciPiBlkIoctl
** ��������: ATAPI ���豸 ioctl
** �䡡��  : hDev       �豸���
**           iCmd       ��������
**           lArg       ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : FIOTRIM �������д������Ѿ��� disk cache ����õ���֤.
*********************************************************************************************************/
static INT  __ahciPiBlkIoctl (AHCI_DEV_HANDLE  hDev, INT  iCmd, LONG  lArg)
{
    AHCI_CTRL_HANDLE        hCtrl;                                      /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    PLW_BLK_INFO            hBlkInfo;                                   /* �豸��Ϣ                     */

    hCtrl  = hDev->AHCIDEV_hCtrl;                                       /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    switch (iCmd) {

    /*
     *  ����Ҫ֧�ֵ�����
     */
    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  ������д�����              */
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ������                  */
    case FIODISKCHANGE:                                                 /*  ����ý�ʷ����仯            */
    case FIOTRIM:
        break;
    /*
     *  �ͼ���ʽ��
     */
    case FIODISKFORMAT:                                                 /*  ��ʽ����                    */
        return  (PX_ERROR);                                             /*  ��֧�ֵͼ���ʽ��            */

    /*
     *  FatFs ��չ����
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((ULONG *)lArg) = hDrive->AHCIDRIVE_ulByteSector;
        break;

    case LW_BLKD_GET_SECNUM:
        *((ULONG *)lArg) = (ULONG)hDrive->AHCIDRIVE_uiSector;
        break;

    case LW_BLKD_CTRL_INFO:
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_SATA;
        API_AhciDriveSerialInfoGet(hDrive, hBlkInfo->BLKI_cSerial,   LW_BLKD_CTRL_INFO_STR_SZ);
        API_AhciDriveFwRevInfoGet(hDrive,  hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ);
        API_AhciDriveModelInfoGet(hDrive,  hBlkInfo->BLKI_cProduct,  LW_BLKD_CTRL_INFO_STR_SZ);
        AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                       "ctrl %d drive %d atapi ioctl serial %s firmware %s product %s.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                       hBlkInfo->BLKI_cSerial, hBlkInfo->BLKI_cFirmware, hBlkInfo->BLKI_cProduct);
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkWr
** ��������: ATAPI ���豸д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkWr (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkRd
** ��������: ATAPI ���豸������
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkRd (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    INT                     iRet;                                       /* �������                     */
    REGISTER INT            i;                                          /* ѭ������                     */
    AHCI_CTRL_HANDLE        hCtrl;                                      /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    PLW_BLK_DEV             hBlkDev;                                    /* ���豸���                   */
    AHCI_CMD_CB             tCtrlCmd;                                   /* ������ƿ�                   */
    AHCI_CMD_HANDLE         hCmd;                                       /* ������                     */
    ULONG                   ulSector;                                   /* ��������                     */

    hCtrl   = hDev->AHCIDEV_hCtrl;                                      /* ��ȡ���������               */
    hDrive  = hDev->AHCIDEV_hDrive;                                     /* ��ȡ���������               */
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */
    hCmd    = &tCtrlCmd;                                                /* ��ȡ������                 */

    AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                   "ctrl %d drive %d atapi block read start %lu[0x%llx] count %lu[0x%llx] buff %p"
                   " offset[%lu][0x%llx] sectors[%lu][0x%llx] sector bytes[%lu][0x%llx].\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                   ulBlkStart, ulBlkStart, ulBlkCount, ulBlkCount, pvBuffer,
                   hDev->AHCIDEV_ulBlkOffset, hDev->AHCIDEV_ulBlkOffset,
                   hBlkDev->BLKD_ulNSector, hBlkDev->BLKD_ulNSector,
                   hBlkDev->BLKD_ulBytesPerSector, hBlkDev->BLKD_ulBytesPerSector);

    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {                   /* ״̬����                     */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi driver not installed or state not ok.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        iRet = __ahciPiBlkStatusChk(hDev);
        if (iRet != ERROR_NONE) {
            AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi read blk status check fail.\r\n",
                           hCtrl->AHCICTRL_uiIndex, hDev->AHCIDEV_uiDrive);
            return  (PX_ERROR);
        } else {
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;                    /* ����������״̬               */
        }
    }

    iRet = API_AhciPmActive(hCtrl, hDev->AHCIDEV_uiDrive);              /* ʹ�ܵ�Դ                     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    ulSector = (ULONG)hBlkDev->BLKD_ulNSector;
    if ((ulBlkStart + ulBlkCount) > (ULONG)ulSector) {                  /* ������������                 */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR,
                       "ctrl %d drive %d atapi block read start %lu count %lu [0 - %lu].\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, ulBlkStart, ulBlkCount, ulSector);
        return  (PX_ERROR);
    }

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[0] = AHCI_CDROM_CMD_READ_12_CR;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[1] = 0;
    for (i = 2; i < AHCI_ATAPI_CMD_LEN_MAX; i++) {
        hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[i] = 0;
    }

    for (i = 2; i < 6; i++) {
        hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[i] =
              (UINT8)(hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[i] + (ulBlkStart >> (8 * (5 - i))));
    }

    for (i = 6; i < 10; i++) {
        hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[i] =
              (UINT8)(hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[i] + (ulBlkCount >> (8 * (9 - i))));
    }

    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[10] = 0;
    hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt[11] = 0;

    if (hDrive->AHCIDRIVE_usRwMode > AHCI_PIO_4) {
        hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature = AHCI_FEAT_DMA | 0x04;
    } else {
        hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature = 0;
    }

    hCmd->AHCICMD_ulDataLen  = ulBlkCount * hBlkDev->BLKD_ulBytesPerSector;
    hCmd->AHCICMD_pucDataBuf = (UINT8 *)pvBuffer;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags     = AHCI_CMD_FLAG_ATAPI;

    iRet = __ahciPiCommandSend(hDev, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d atapi cmd send error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return  (PX_ERROR);
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi block read ok.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkDevDelete
** ��������: ATAPI ���豸ɾ��
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkDevDelete (AHCI_DEV_HANDLE  hDev)
{
    INT     iRet;                                                       /* �������                     */

    if (hDev->AHCIDEV_pvOemdisk) {                                      /* �Ѿ����ع�                   */
        iRet = API_OemDiskUnmount((PLW_OEMDISK_CB)hDev->AHCIDEV_pvOemdisk);
        if (iRet != ERROR_NONE) {                                       /* ж���豸                     */
            AHCI_ATAPI_LOG(AHCI_LOG_ERR, "ahci dev unmount error ctrl %d port %d.\r\n",
                           hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        }
        hDev->AHCIDEV_pvOemdisk = LW_NULL;                              /* ���¹����豸���             */
    }

    if (API_AhciDevHandleGet(hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive)) {
        API_AhciDevDelete(hDev);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkDevCreate
** ��������: ATAPI ���豸����
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciPiBlkDevCreate (AHCI_DEV_HANDLE  hDev)
{
    INT                     iRet          = PX_ERROR;                   /* �������                     */
    AHCI_CTRL_HANDLE        hCtrl         = LW_NULL;                    /* ���������                   */
    AHCI_DRIVE_HANDLE       hDrive        = LW_NULL;                    /* ���������                   */
    AHCI_DRV_HANDLE         hDrv          = LW_NULL;                    /* �������                     */
    PLW_BLK_DEV             hBlkDev       = LW_NULL;                    /* ���豸���                   */
    ULONG                   ulPl          = AHCI_CACHE_PL;              /* ���������߳�����             */
    ULONG                   ulCacheSize   = AHCI_CACHE_SIZE;            /* �����С                     */
    ULONG                   ulBurstSizeRd = AHCI_CACHE_BURST_RD;        /* ⧷�����С                   */
    ULONG                   ulBurstSizeWr = AHCI_CACHE_BURST_WR;        /* ⧷�д��С                   */
    LW_DISKCACHE_ATTR       dcattrl;                                    /* CACHE ����                   */
    ULONG                   ulDcMsgCount  = 0;                          /* CACHE ��Ϣ����               */
    ULONG                   ulDcParallel  = LW_FALSE;                   /* CACHE �Ƿ�ʹ�ܲ��в���       */

    hCtrl   = hDev->AHCIDEV_hCtrl;
    hDrv    = hCtrl->AHCICTRL_hDrv;                                     /* ����������                 */
    hDrive  = hDev->AHCIDEV_hDrive;
    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ÿ��豸���               */

    iRet = __ahciPiBlkDevDelete(hDev);                                  /* ɾ���Ѿ����ڵ��豸           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = __ahciPiBlkStatusChk(hDev);                                  /* ��ȡ״̬��Ϣ                 */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;                            /* ����������״̬               */

    /*
     *  ���ÿ��豸����
     */
    hBlkDev->BLKD_pcName            = hDrive->AHCIDRIVE_cDevName;
    hBlkDev->BLKD_ulBytesPerSector  = hDrive->AHCIDRIVE_ulByteSector;
    hBlkDev->BLKD_ulBytesPerBlock   = hDrive->AHCIDRIVE_ulByteSector;
    hBlkDev->BLKD_bRemovable        = LW_TRUE;
    hBlkDev->BLKD_iRetry            = AHCI_RETRY_NUM;
    hBlkDev->BLKD_iFlag             = O_RDONLY;
    hBlkDev->BLKD_bDiskChange       = LW_FALSE;
    hBlkDev->BLKD_pfuncBlkRd        = __ahciPiBlkRd;
    hBlkDev->BLKD_pfuncBlkWrt       = __ahciPiBlkWr;
    hBlkDev->BLKD_pfuncBlkIoctl     = __ahciPiBlkIoctl;
    hBlkDev->BLKD_pfuncBlkReset     = __ahciPiBlkReset;
    hBlkDev->BLKD_pfuncBlkStatusChk = __ahciPiBlkStatusChk;

    hBlkDev->BLKD_iLogic            = 0;                                /*  �����豸                    */
    hBlkDev->BLKD_uiLinkCounter     = 0;
    hBlkDev->BLKD_pvLink            = LW_NULL;

    hBlkDev->BLKD_uiPowerCounter    = 0;
    hBlkDev->BLKD_uiInitCounter     = 0;

    /*
     *  ��ȡ���������豸������Ϣ
     */
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_CACHE_PL_GET,
                                      (LONG)((ULONG *)&ulPl));
    if (iRet != ERROR_NONE) {
        ulPl = AHCI_CACHE_PL;
    }
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_CACHE_SIZE_GET,
                                      (LONG)((ULONG *)&ulCacheSize));
    if (iRet != ERROR_NONE) {
        ulCacheSize = AHCI_CACHE_SIZE;
    }
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_CACHE_BURST_RD_GET,
                                      (LONG)((ULONG *)&ulBurstSizeRd));
    if (iRet != ERROR_NONE) {
        ulBurstSizeRd = AHCI_CACHE_BURST_RD;
    }
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_CACHE_BURST_WR_GET,
                                      (LONG)((ULONG *)&ulBurstSizeWr));
    if (iRet != ERROR_NONE) {
        ulBurstSizeWr = AHCI_CACHE_BURST_WR;
    }
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_DC_MSG_COUNT_GET,
                                      (LONG)((ULONG *)&ulDcMsgCount));
    if (iRet != ERROR_NONE) {
        ulDcMsgCount = AHCI_DRIVE_DISKCACHE_MSG_COUNT;
    }
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, hDev->AHCIDEV_uiDrive, AHCI_OPT_CMD_DC_PARALLEL_EN_GET,
                                      (LONG)((ULONG *)&ulDcParallel));
    if (iRet != ERROR_NONE) {
        ulDcParallel = AHCI_DRIVE_DISKCACHE_PARALLEL_EN;
    }
    ulDcParallel = ulDcParallel ? LW_TRUE : LW_FALSE;

    dcattrl.DCATTR_pvCacheMem       = LW_NULL;
    dcattrl.DCATTR_stMemSize        = (size_t)ulCacheSize;              /* ��֤ CACHE һ����            */
    dcattrl.DCATTR_iBurstOpt        = LW_DCATTR_BOPT_CACHE_COHERENCE;
    dcattrl.DCATTR_iMaxRBurstSector = (INT)ulBurstSizeRd;
    dcattrl.DCATTR_iMaxWBurstSector = (INT)ulBurstSizeWr;
    dcattrl.DCATTR_iMsgCount        = (INT)ulDcMsgCount;
    dcattrl.DCATTR_bParallel        = (BOOL)(ulDcParallel);             /* ��֧�ֲ��в���               */

    if (hDrive->AHCIDRIVE_bNcq) {                                       /* �Ƿ�֧���������             */
        dcattrl.DCATTR_iPipeline = (INT)((ulPl > LW_NCPUS) ? LW_NCPUS : ulPl);
    } else {
        dcattrl.DCATTR_iPipeline = 1;
    }

    hDev->AHCIDEV_ulBlkCount = hDrive->AHCIDRIVE_uiSector;
    if (!hDev->AHCIDEV_pvOemdisk) {
        hDev->AHCIDEV_pvOemdisk = (PVOID)API_OemDiskMount2(AHCI_ATAPI_MEDIA_NAME, hBlkDev, &dcattrl);
    }
    if (!hDev->AHCIDEV_pvOemdisk) {                                     /* ����ʧ��                     */
        AHCI_ATAPI_LOG(AHCI_LOG_ERR, "oem disk mount failed ctrl %d port %d.\r\n",
                       hCtrl->AHCICTRL_uiIndex, hDev->AHCIDEV_uiDrive);
    }

    API_AhciDevAdd(hCtrl, hDev->AHCIDEV_uiDrive);                       /* ����豸                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciPiBlkDevManage
** ��������: ATAPI ���豸��̬����
** �䡡��  : hDev       �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ahciPiBlkDevManage (AHCI_DEV_HANDLE  hDev)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    UINT8                   ucEventBuff[8] = {0};                       /* �¼���Ϣ                     */

    hDrive = hDev->AHCIDEV_hDrive;

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange start.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);

    if (hDrive->AHCIDRIVE_ucState == AHCI_DEV_INIT) {                   /* ���쳣�жϽ��д���           */
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange dev init.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        __ahciPiBlkDevDelete(hDev);
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;
    }

    iRet = __ahciPiMediaEventGet(hDev, ucEventBuff, 8);                 /* ��ȡ����״̬�͹���״̬       */
    if ((iRet == ERROR_NONE) &&
        ((ucEventBuff[4] == 0x00)) &&
        ((ucEventBuff[5] == 0x02))) {                                   /* ���Źرղ����й���           */
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi the door is closed and there is media.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
    } else {                                                            /* ��ȡ״̬ʧ��                 */
        AHCI_ATAPI_LOG(AHCI_LOG_PRT,
                       "ctrl %d drive %d atapi blk dev mange get event fail "
                       "buff[4] 0x%02x buff[5] 0x%02x.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive, ucEventBuff[4], ucEventBuff[5]);
        __ahciPiBlkDevDelete(hDev);
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;
        return;
    }

    if (hDrive->AHCIDRIVE_ucState == AHCI_DEV_OK) {                     /* �豸״̬����                 */
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange drive state ok.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        return;
    }

    iRet = __ahciPiBlkStatusChk(hDev);                                  /* ��ȡ����״̬��������Ϣ       */
    if ((iRet == ERROR_NONE) &&
        (hDrive->AHCIDRIVE_ucState == AHCI_DEV_MED_CH)) {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange status check ok.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        __ahciPiBlkDevCreate(hDrive->AHCIDRIVE_hDev);
    } else {
        AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange status check error.\r\n",
                       hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
        __ahciPiBlkDevDelete(hDev);
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;
    }

    AHCI_ATAPI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d atapi blk dev mange status check error.\r\n",
                   hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
}

#endif                                                                  /* AHCI_ATAPI_EN                */
/*********************************************************************************************************
** ��������: __ahciBlkDevRemove
** ��������: �Ƴ����豸
** �䡡��  : hCtrl      ���������
**           uiDrive    ���������
** �䡡��  : ���豸���ƿ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if AHCI_HOTPLUG_EN > 0                                                 /* �Ƿ�ʹ���Ȳ��               */

static INT  __ahciBlkDevRemove (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    ULONG               ulRet  = PX_ERROR;                              /* �������                     */
    REGISTER INT        i      = 0;                                     /* ѭ������                     */
    AHCI_DEV_HANDLE     hDev   = LW_NULL;                               /* �豸���                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "blk device remove ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDev   = API_AhciDevHandleGet(hCtrl->AHCICTRL_uiIndex, uiDrive);    /* ��ȡ AHCI �豸���           */
    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    if ((!hDev) || (!hDrive)) {                                         /* �豸�������������Ч         */
        AHCI_LOG(AHCI_LOG_ERR, "ahci dev handle error ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    if (hDev->AHCIDEV_pvOemdisk) {                                      /* �Ѿ����ع�                   */
        iRet = API_OemDiskUnmount((PLW_OEMDISK_CB)hDev->AHCIDEV_pvOemdisk);
        if (iRet != ERROR_NONE) {                                       /* ж���豸                     */
            AHCI_LOG(AHCI_LOG_ERR, "ahci dev unmount error ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            return  (PX_ERROR);
        }
        hDev->AHCIDEV_pvOemdisk = LW_NULL;                              /* ���¹����豸���             */
    }

    if (hDrive->AHCIDRIVE_pucAlignDmaBuf) {                             /* �Ѿ����������� DMA ����      */
        API_CacheDmaFree(hDrive->AHCIDRIVE_pucAlignDmaBuf);             /* �ͷ������� DMA ����          */
        hDrive->AHCIDRIVE_pucAlignDmaBuf = LW_NULL;
        hDrive->AHCIDRIVE_ulByteSector   = 0;
        hDrive->AHCIDRIVE_uiAlignSize    = 0;
    }

    for (i = 0; i < AHCI_CMD_SLOT_MAX; i++) {                           /* ����ģʽ                     */
        if (hDrive->AHCIDRIVE_hSyncBSem[i]) {                           /* �Ѿ������ͬ����             */
                                                                        /* ����ͬ������Դ               */
            ulRet = API_SemaphoreBDelete(&hDrive->AHCIDRIVE_hSyncBSem[i]);
            if (ulRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "ahci dev queue sync sem del error ctrl %d port %d index %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, uiDrive, i);
            } else {
                hDrive->AHCIDRIVE_hSyncBSem[i] = LW_OBJECT_HANDLE_INVALID;
            }
        }
    }

    if (hDev) {                                                         /* �Ѿ������豸���             */
        iRet = API_AhciDevDelete(hDev);                                 /* ���豸���������Ƴ��豸       */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ahci dev remove error ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* AHCI_HOTPLUG_EN              */
/*********************************************************************************************************
** ��������: __ahciBlkDevCreate
** ��������: �������豸
** �䡡��  : hCtrl          ���������
**           uiDrive        ���������
**           ulBlkOffset    ƫ��
**           ulBlkCount     ����
** �䡡��  : ���豸���ƿ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_BLK_DEV  __ahciBlkDevCreate (AHCI_CTRL_HANDLE  hCtrl,
                                        UINT              uiDrive,
                                        ULONG             ulBlkOffset,
                                        ULONG             ulBlkCount)
{
    INT                 iRet          = PX_ERROR;                       /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive        = LW_NULL;                        /* ���������                   */
    AHCI_DRV_HANDLE     hDrv          = LW_NULL;                        /* �������                     */
    PLW_BLK_DEV         hBlkDev       = LW_NULL;                        /* ���豸���                   */
    AHCI_DEV_HANDLE     hDev          = LW_NULL;                        /* �豸���                     */
    UINT64              ullBlkMax     = 0;                              /* ���������                   */
    ULONG               ulBlkMax      = 0;                              /* ���������                   */
    ULONG               ulPl          = AHCI_CACHE_PL;                  /* ���������߳�����             */
    ULONG               ulCacheSize   = AHCI_CACHE_SIZE;                /* �����С                     */
    ULONG               ulBurstSizeRd = AHCI_CACHE_BURST_RD;            /* ⧷�����С                   */
    ULONG               ulBurstSizeWr = AHCI_CACHE_BURST_WR;            /* ⧷�д��С                   */
    LW_DISKCACHE_ATTR   dcattrl;                                        /* CACHE ����                   */
    ULONG               ulDcMsgCount  = 0;                              /* CACHE ��Ϣ����               */
    ULONG               ulDcParallel  = LW_FALSE;                       /* CACHE �Ƿ�ʹ�ܲ��в���       */

    if (!hCtrl) {                                                       /* �����������Ч               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* ����������δ��װ             */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver invalid ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl is not installed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (uiDrive >= hCtrl->AHCICTRL_uiImpPortNum) {                      /* ��������������               */
        AHCI_LOG(AHCI_LOG_ERR, "drive %d is out of range (0-%d).\r\n",
                 uiDrive, (hCtrl->AHCICTRL_uiImpPortNum - 1));
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    hDev   = hDrive->AHCIDRIVE_hDev;                                    /* ����豸���                 */
    if (!hDev) {                                                        /* �豸�����Ч                 */
        hDev = (AHCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DEV_CB));    /* �����豸���ƿ�               */
        if (!hDev) {                                                    /* ������ƿ�ʧ��               */
            AHCI_LOG(AHCI_LOG_ERR, "alloc ahci dev tcb failed ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            return  (LW_NULL);
        }
    }
    hDrive->AHCIDRIVE_hDev = hDev;                                      /* �������������豸���         */
    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */

    hDev->AHCIDEV_hCtrl       = hCtrl;                                  /* ���¿��������               */
    hDev->AHCIDEV_hDrive      = hDrive;                                 /* �������������               */
    hDev->AHCIDEV_uiCtrl      = hCtrl->AHCICTRL_uiIndex;                /* ���¿�����ȫ������           */
    hDev->AHCIDEV_uiDrive     = uiDrive;                                /* ��������������               */
    hDev->AHCIDEV_ulBlkOffset = ulBlkOffset;                            /* ��������ƫ��                 */

    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ÿ��豸���               */
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* ������״̬����               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;                      /* ��λ������״̬               */
        AHCI_LOG(AHCI_LOG_ERR, "drive state error ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (LW_NULL);
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;                        /* �����豸״̬                 */
    
    switch (hDrive->AHCIDRIVE_ucType) {                                 /* ����豸����                 */

    case AHCI_TYPE_ATA:                                                 /* ATA �豸                     */
        if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {                      /* LBA 48 ģʽ                  */
            ullBlkMax = ((hCtrl->AHCICTRL_ullLba48TotalSecs[uiDrive]) - (UINT64)ulBlkOffset);
            ulBlkMax = (ULONG)ullBlkMax;
        
        } else if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] != 0) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] >
                   (UINT32)(hDrive->AHCIDRIVE_uiCylinder *
                            hDrive->AHCIDRIVE_uiHead *
                            hDrive->AHCIDRIVE_uiSector))) {             /* LBA ģʽ                     */
            ulBlkMax = (ULONG)(hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] - (UINT32)ulBlkOffset);
        
        } else {                                                        /* CHS ģʽ                     */
            ulBlkMax = (ULONG)((hDrive->AHCIDRIVE_uiCylinder *
                                hDrive->AHCIDRIVE_uiHead *
                                hDrive->AHCIDRIVE_uiSector) - ulBlkOffset);
        }
        if ((ulBlkCount == 0) ||
            (ulBlkCount > ulBlkMax)) {                                  /* ȫ����������               */
            hDev->AHCIDEV_ulBlkCount = (ULONG)ulBlkMax;
        }
        /*
         *  ���ÿ��豸����
         */
        hBlkDev->BLKD_pcName            = hDrive->AHCIDRIVE_cDevName;
        hBlkDev->BLKD_ulNSector         = hDev->AHCIDEV_ulBlkCount;
        hBlkDev->BLKD_ulBytesPerSector  = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_ulBytesPerBlock   = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_bRemovable        = LW_TRUE;
        hBlkDev->BLKD_iRetry            = 1;
        hBlkDev->BLKD_iFlag             = O_RDWR;
        hBlkDev->BLKD_bDiskChange       = LW_FALSE;
        hBlkDev->BLKD_pfuncBlkRd        = __ahciBlkRd;
        hBlkDev->BLKD_pfuncBlkWrt       = __ahciBlkWr;
        hBlkDev->BLKD_pfuncBlkIoctl     = __ahciBlkIoctl;
        hBlkDev->BLKD_pfuncBlkReset     = __ahciBlkReset;
        hBlkDev->BLKD_pfuncBlkStatusChk = __ahciBlkStatusChk;

        hBlkDev->BLKD_iLogic            = 0;                            /*  �����豸                    */
        hBlkDev->BLKD_uiLinkCounter     = 0;
        hBlkDev->BLKD_pvLink            = LW_NULL;

        hBlkDev->BLKD_uiPowerCounter    = 0;
        hBlkDev->BLKD_uiInitCounter     = 0;

        hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;                        /* ����������״̬               */
        /*
         *  ��ȡ���������豸������Ϣ
         */
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_CACHE_PL_GET,
                                          (LONG)((ULONG *)&ulPl));
        if (iRet != ERROR_NONE) {
            ulPl = AHCI_CACHE_PL;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_CACHE_SIZE_GET,
                                          (LONG)((ULONG *)&ulCacheSize));
        if (iRet != ERROR_NONE) {
            ulCacheSize = AHCI_CACHE_SIZE;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_CACHE_BURST_RD_GET,
                                          (LONG)((ULONG *)&ulBurstSizeRd));
        if (iRet != ERROR_NONE) {
            ulBurstSizeRd = AHCI_CACHE_BURST_RD;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_CACHE_BURST_WR_GET,
                                          (LONG)((ULONG *)&ulBurstSizeWr));
        if (iRet != ERROR_NONE) {
            ulBurstSizeWr = AHCI_CACHE_BURST_WR;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_DC_MSG_COUNT_GET,
                                          (LONG)((ULONG *)&ulDcMsgCount));
        if (iRet != ERROR_NONE) {
            ulDcMsgCount = AHCI_DRIVE_DISKCACHE_MSG_COUNT;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_DC_PARALLEL_EN_GET,
                                          (LONG)((ULONG *)&ulDcParallel));
        if (iRet != ERROR_NONE) {
            ulDcParallel = AHCI_DRIVE_DISKCACHE_PARALLEL_EN;
        }
        ulDcParallel = ulDcParallel ? LW_TRUE : LW_FALSE;
        
        dcattrl.DCATTR_pvCacheMem       = LW_NULL;
        dcattrl.DCATTR_stMemSize        = (size_t)ulCacheSize;          /* ��֤ CACHE һ����            */
        dcattrl.DCATTR_iBurstOpt        = LW_DCATTR_BOPT_CACHE_COHERENCE;
        dcattrl.DCATTR_iMaxRBurstSector = (INT)ulBurstSizeRd;
        dcattrl.DCATTR_iMaxWBurstSector = (INT)ulBurstSizeWr;
        dcattrl.DCATTR_iMsgCount        = (INT)ulDcMsgCount;
        dcattrl.DCATTR_bParallel        = (BOOL)(ulDcParallel);         /* ��֧�ֲ��в���               */
        
        if (hDrive->AHCIDRIVE_bNcq) {                                   /* �Ƿ�֧���������             */
            dcattrl.DCATTR_iPipeline = (INT)((ulPl > LW_NCPUS) ? LW_NCPUS : ulPl);

        } else {
            dcattrl.DCATTR_iPipeline = 1;
        }
                                                                        /* �����豸                     */
        if (!hDev->AHCIDEV_pvOemdisk) {
            hDev->AHCIDEV_pvOemdisk = (PVOID)API_OemDiskMount2(AHCI_MEDIA_NAME, hBlkDev, &dcattrl);
        }
        if (!hDev->AHCIDEV_pvOemdisk) {                                 /* ����ʧ��                     */
            AHCI_LOG(AHCI_LOG_ERR, "oem disk mount failed ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
        }
        return  (hBlkDev);                                              /* ���ؿ��豸���               */

#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */
    case AHCI_TYPE_ATAPI:                                               /* ATAPI �豸                   */
        API_HotplugPollAdd((VOIDFUNCPTR)__ahciPiBlkDevManage, (PVOID)hDev);
        return  (LW_NULL);                                              /* ���ؿ��豸���               */
#endif                                                                  /* AHCI_ATAPI_EN                */

    default:                                                            /* �豸���ʹ���                 */
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciDiskConfig
** ��������: ���ô�������
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           cpcDevName     �豸����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskConfig (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, CPCHAR  cpcDevName)
{
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* ���豸���                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */

    if (!hCtrl) {                                                       /* �����������Ч               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!cpcDevName) || (cpcDevName[0] == PX_EOS)) {                   /* �豸������Ч                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid device name ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE)) {                  /* �����������δ��װ           */
        AHCI_LOG(AHCI_LOG_ERR, "ctrl or driver is not installed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    lib_strlcpy(hDrive->AHCIDRIVE_cDevName, cpcDevName, AHCI_DEV_NAME_MAX);
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* �豸״̬����                 */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        return  (ERROR_NONE);
    }
                                                                        /* �������豸                   */
    hBlkDev = __ahciBlkDevCreate(hCtrl, uiDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
    if (!hBlkDev) {                                                     /* �������豸ʧ��               */
        AHCI_LOG(AHCI_LOG_PRT, "create blk dev error %s.\r\n", cpcDevName);
        return  (PX_ERROR);
    }
    API_AhciDevAdd(hCtrl, uiDrive);                                     /* ����豸                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskCtrlInit
** ��������: ���̿�������ʼ��
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskCtrlInit (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    REGISTER INT        i;                                              /* ѭ������                     */
    INT                 iRet;                                           /* �������                     */
    AHCI_DRV_HANDLE     hDrv;                                           /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */
    UINT32              uiReg;                                          /* �Ĵ���                       */
    ULONG               ulInterTime;                                    /* ��ʱʱ����                   */
    ULONG               ulInterCount;                                   /* ��ʱʱ��������             */

    AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.\r\n",
             hCtrl->AHCICTRL_uiIndex, hCtrl->AHCICTRL_cCtrlName, hCtrl->AHCICTRL_uiUnitIndex,
             hCtrl->AHCICTRL_pvRegAddr);

    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d disk ctrl init.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    hCmd   = &tCtrlCmd;                                                 /* ������                     */

    __ahciCmdWaitForResource(hDrive, LW_FALSE);                         /* �Ƕ���ģʽ�ȴ�               */
    
    hDrive->AHCIDRIVE_iInitActive = LW_TRUE;                            /* �����������״̬           */

    iRet = API_AhciDriveEngineStop(hDrive);                             /* ֹͣ DMA                     */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port engine stop failed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        goto    __out;
    }

    iRet = API_AhciDriveRecvFisStop(hDrive);                            /* ֹͣ���մ���                 */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port recv fis stop failed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        goto    __out;
    }

    API_AhciDrivePowerUp(hDrive);                                       /* ��Դʹ��                     */

    __ahciDrivePhyReset(hCtrl, uiDrive);                                /* �ɾ����豸������λ��Ϊ       */

    AHCI_LOG(AHCI_LOG_PRT, "restart ctrl %d drive %d port %d.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive, hDrive->AHCIDRIVE_uiPort);
    AHCI_LOG(AHCI_LOG_PRT, "port : active, recv fis start, power on, spin up.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive);
    AHCI_PORT_WRITE(hDrive,
                    AHCI_PxCMD, AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_POD | AHCI_PCMD_SUD);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "wait link reactivate failed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        goto    __out;
    }

    iRet = __ahciDriveNoBusyWait(hDrive);
    if (iRet != ERROR_NONE) {
        goto    __out;
    }
    
    AHCI_LOG(AHCI_LOG_PRT, "port start ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD) | AHCI_PCMD_ST;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    AHCI_PORT_READ(hDrive, AHCI_PxCMD);
    iRet = __ahciDriveNoBusyWait(hDrive);
    if (iRet != ERROR_NONE) {
        goto    __out;
    }

    ulInterTime = 0;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0,
                                      AHCI_OPT_CMD_RSTON_INTER_TIME_GET,
                                      (LONG)((ULONG *)&ulInterTime));
    if ((iRet != ERROR_NONE) || (ulInterTime == 0)) {
        ulInterTime = AHCI_DRIVE_RSTON_INTER_TIME_UNIT;
    }
    ulInterCount = 0;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0,
                                      AHCI_OPT_CMD_RSTON_INTER_COUNT_GET,
                                      (LONG)((ULONG *)&ulInterCount));
    if ((iRet != ERROR_NONE) || (ulInterCount == 0)) {
        ulInterCount = AHCI_DRIVE_RSTON_INTER_TIME_COUNT;
    }
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_ON;
    API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01,
                                ulInterTime, ulInterCount, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "srst on failed ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
        goto    __out;
    }

    ulInterTime = 0;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0,
                                      AHCI_OPT_CMD_RSTOFF_INTER_TIME_GET,
                                      (LONG)((ULONG *)&ulInterTime));
    if ((iRet != ERROR_NONE) || (ulInterTime == 0)) {
        ulInterTime = AHCI_DRIVE_RSTOFF_INTER_TIME_UNIT;
    }
    ulInterCount = 0;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0,
                                      AHCI_OPT_CMD_RSTOFF_INTER_COUNT_GET,
                                      (LONG)((ULONG *)&ulInterCount));
    if ((iRet != ERROR_NONE) || (ulInterCount == 0)) {
        ulInterCount = AHCI_DRIVE_RSTOFF_INTER_TIME_COUNT;
    }
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_OFF;
    API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01,
                                ulInterTime, ulInterCount, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "srst off failed ctrl %d port %d.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
        goto    __out;
    }

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d port %d stat 0x%08x.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive, hDrive->AHCIDRIVE_uiPort,
             AHCI_PORT_READ(hDrive, AHCI_PxTFD));
             
    for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {              /* ����ģʽ                     */
        if (hDrive->AHCIDRIVE_hSyncBSem[i]) {
            continue;
        } else {
            hDrive->AHCIDRIVE_hSyncBSem[i] = API_SemaphoreBCreate("ahci_sync",
                                                                  LW_FALSE,
                                                                  (LW_OPTION_WAIT_FIFO |
                                                                   LW_OPTION_OBJECT_GLOBAL),
                                                                   LW_NULL);
        }
    }

    hDrive->AHCIDRIVE_uiCmdMask   = 0;
    hDrive->AHCIDRIVE_bQueued     = LW_FALSE;
    hDrive->AHCIDRIVE_iInitActive = LW_FALSE;

__out:
    __ahciCmdReleaseResource(hDrive, LW_FALSE);                         /* �ԷǶ���ģʽ�ͷ�             */

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskDriveInit
** ��������: ������������ʼ��
** �䡡��  : hCtrl      ���������
**           uiDrive     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskDriveInit (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                 iRet;                                           /* ���󷵻�                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_PARAM_HANDLE   hParam;                                         /* �������                     */
    AHCI_DRV_HANDLE     hDrv;                                           /* �������                     */
    INT                 iConfigType;                                    /* ��������                     */
    UINT32              uiReg;                                          /* �Ĵ���                       */
    UINT16              usDma;                                          /* DMA ģʽ                     */
    PVOID               pvBuff;                                         /* ������                       */
    INT                 iFlag;                                          /* ���                         */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d port %d disk drive init.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);
    /*
     *  ��ȡ������Ϣ
     */
    hDrive      = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam      = &hDrive->AHCIDRIVE_tParam;
    iConfigType = hCtrl->AHCICTRL_piConfigType[uiDrive];

    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {                                /* �����������δ��װ           */
        return  (PX_ERROR);
    }
                                                                        /* �ȴ�����Ȩ                   */
    API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    /*
     *  ��ȡ����������
     */
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSIG);
    if (uiReg == AHCI_PSIG_ATAPI) {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATAPI;
    } else {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATA;
    }
    /*
     *  ��ȡ��������
     */
    hDrv = hCtrl->AHCICTRL_hDrv;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, uiDrive, AHCI_OPT_CMD_SECTOR_SIZE_GET,
                                      (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulByteSector));
    if (iRet != ERROR_NONE) {
        if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {
            hDrive->AHCIDRIVE_ulByteSector = AHCI_PI_SECTOR_SIZE;
        } else {
            hDrive->AHCIDRIVE_ulByteSector = AHCI_SECTOR_SIZE;
        }
    }
    hDrive->AHCIDRIVE_uiAlignSize = (size_t)API_CacheLine(DATA_CACHE);
    if (!hDrive->AHCIDRIVE_pucAlignDmaBuf) {
        pvBuff = API_CacheDmaMallocAlign((size_t)hDrive->AHCIDRIVE_ulByteSector,
                                         (size_t)hDrive->AHCIDRIVE_uiAlignSize);
        hDrive->AHCIDRIVE_pucAlignDmaBuf = (UINT8 *)pvBuff;
    }
    if (!hDrive->AHCIDRIVE_pucAlignDmaBuf) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc aligned vmm dma buffer failed ctrl %d port %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);

    } else {
        AHCI_LOG(AHCI_LOG_PRT, "align dma buf addr %p size %lu align size %lu.\r\n",
                 hDrive->AHCIDRIVE_pucAlignDmaBuf,
                 hDrive->AHCIDRIVE_ulByteSector, hDrive->AHCIDRIVE_uiAlignSize);
    }

    if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA) {                    /* ATA ģʽ                     */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* ��λ����                     */

        iRet = API_AhciDiskAtaParamGet(hCtrl, uiDrive, (PVOID)hParam);  /* ��ȡ����������               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d read ata parameters failed.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto    __error_handle;
        }

#if LW_CFG_CPU_ENDIAN > 0
        if (hDrive->AHCIDRIVE_iParamEndianType == AHCI_ENDIAN_TYPE_LITTEL) {
            API_AhciSwapBufLe16((UINT16 *)hParam, (size_t)(512 / 2));
        }
#else
        if (hDrive->AHCIDRIVE_iParamEndianType == AHCI_ENDIAN_TYPE_BIG) {
            API_AhciSwapBufLe16((UINT16 *)hParam, (size_t)(512 / 2));
        }
#endif

        iRet = __ahciDiskDriveDiagnostic(hCtrl, uiDrive);
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "disk port diagnostic failed ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
        }

        /*
         *  ��ȡ����������Ϣ
         */
        hDrive->AHCIDRIVE_uiCylinder = hParam->AHCIPARAM_usCylinders - 1;
        hDrive->AHCIDRIVE_uiHead     = hParam->AHCIPARAM_usHeads;
        hDrive->AHCIDRIVE_uiSector   = hParam->AHCIPARAM_usSectors;
        if (hParam->AHCIPARAM_usCapabilities & 0x0200) {
            hCtrl->AHCICTRL_uiLbaTotalSecs[uiDrive] =
                                (UINT32)((((UINT32)((hParam->AHCIPARAM_usSectors0) & 0x0000ffff)) <<  0) |
                                         (((UINT32)((hParam->AHCIPARAM_usSectors1) & 0x0000ffff)) << 16));
        }
    
    } else if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {           /* ATAPI ����                   */
#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* ��������ʼ��                 */

        iRet = API_AhciDiskAtapiParamGet(hCtrl, uiDrive, (PVOID)hParam);/* ��ȡ ATAPI ����              */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "read atapi parameters failed ctrl %d port %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto    __error_handle;
        }

#if LW_CFG_CPU_ENDIAN > 0                                               /* ���ģʽ                     */
        if (hDrive->AHCIDRIVE_iParamEndianType == AHCI_ENDIAN_TYPE_LITTEL) {
            API_AhciSwapBufLe16((UINT16 *)hParam, (size_t)(512 / 2));
        }
#else                                                                   /* С��ģʽ                     */
        if (hDrive->AHCIDRIVE_iParamEndianType == AHCI_ENDIAN_TYPE_BIG) {
            API_AhciSwapBufLe16((UINT16 *)hParam, (size_t)(512 / 2));
        }
#endif                                                                  /* LW_CFG_CPU_ENDIAN            */
#endif                                                                  /* AHCI_ATAPI_EN                */
    }
    /*
     *  ���²�����Ϣ
     */
    hDrive->AHCIDRIVE_usMultiSector = hParam->AHCIPARAM_usMultiSecs & 0x00ff;
    hDrive->AHCIDRIVE_bMulti        = (hDrive->AHCIDRIVE_usMultiSector != 0) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bIordy        = (hParam->AHCIPARAM_usCapabilities & 0x0800) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bLba          = (hParam->AHCIPARAM_usCapabilities & 0x0200) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bDma          = (hParam->AHCIPARAM_usCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;

    if ((hCtrl->AHCICTRL_bNcq == LW_TRUE) &&
        (iConfigType & AHCI_NCQ_MODE)) {
        hDrive->AHCIDRIVE_bNcq = (hParam->AHCIPARAM_usSataCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;
    } else {
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;
    }
    
    if (hDrive->AHCIDRIVE_bNcq) {
        hDrive->AHCIDRIVE_uiQueueDepth = __MIN((hParam->AHCIPARAM_usQueueDepth + 1),
                                               hCtrl->AHCICTRL_uiCmdSlotNum);
    } else {
        hDrive->AHCIDRIVE_uiQueueDepth = 1;
    }

    if (hParam->AHCIPARAM_usFeaturesEnabled1 & 0x0400) {
        hDrive->AHCIDRIVE_bLba48 = LW_TRUE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[uiDrive] =
                            (UINT64)((((UINT64)((hParam->AHCIPARAM_usLba48Size[0]) & 0x0000ffff)) <<  0) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[1]) & 0x0000ffff)) << 16) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[2]) & 0x0000ffff)) << 24) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[3]) & 0x0000ffff)) << 32));
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_48LBA_SECTORS;
    
    } else {
        hDrive->AHCIDRIVE_bLba48 = LW_FALSE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[uiDrive] = (UINT64)0;
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_SECTORS;
    }
    /*
     *  ��ȡ����ģʽ
     */
    hDrive->AHCIDRIVE_usPioMode = (UINT16)((hParam->AHCIPARAM_usPioMode >> 8) & 0x03);
    if (hDrive->AHCIDRIVE_usPioMode > 2) {
        hDrive->AHCIDRIVE_usPioMode = 0;
    }
    if ((hDrive->AHCIDRIVE_bIordy) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        if (hParam->AHCIPARAM_usAdvancedPio & 0x01) {
            hDrive->AHCIDRIVE_usPioMode = 3;
        }
        if (hParam->AHCIPARAM_usAdvancedPio & 0x02) {
            hDrive->AHCIDRIVE_usPioMode = 4;
        }
    }

    if ((hDrive->AHCIDRIVE_bDma == LW_TRUE) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        hDrive->AHCIDRIVE_usSingleDmaMode = (UINT16)((hParam->AHCIPARAM_usDmaMode >> 8) & 0x03);

        if (hDrive->AHCIDRIVE_usSingleDmaMode >= 2) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }
        hDrive->AHCIDRIVE_usMultiDmaMode = 0;

        if (hParam->AHCIPARAM_usSingleDma & 0x04) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 2;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x02) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 1;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x01) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usMultiDma & 0x04) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 2;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x02) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 1;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x01) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usUltraDma & 0x4000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 6;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x2000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 5;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x1000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 4;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0800) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 3;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0400) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 2;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0200) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 1;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0100) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 0;
        }
    }

    hDrive->AHCIDRIVE_usRwPio = (UINT16)(iConfigType & AHCI_PIO_MASK);
    hDrive->AHCIDRIVE_usRwDma = (UINT16)(iConfigType & AHCI_DMA_MASK);
    hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_DEF_W;

    switch (iConfigType & AHCI_MODE_MASK) {

    case AHCI_PIO_0:
    case AHCI_PIO_1:
    case AHCI_PIO_2:
    case AHCI_PIO_3:
    case AHCI_PIO_4:
    case AHCI_PIO_DEF_0:
    case AHCI_PIO_DEF_1:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(iConfigType & AHCI_MODE_MASK);
        break;

    case AHCI_PIO_AUTO:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode);
        break;

    case AHCI_DMA_0:
    case AHCI_DMA_1:
    case AHCI_DMA_2:
    case AHCI_DMA_3:
    case AHCI_DMA_4:
    case AHCI_DMA_5:
    case AHCI_DMA_6:
        if (hDrive->AHCIDRIVE_bDma) {
            usDma = (UINT16)((iConfigType & AHCI_MODE_MASK) - AHCI_DMA_0);
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                if (usDma > hDrive->AHCIDRIVE_usSingleDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usSingleDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma == 0) {
                    if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
                } else {
                    if (usDma > hDrive->AHCIDRIVE_usUltraDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usUltraDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + usDma;
                }
            }
        } else {
            hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode;
        }
        break;

    case AHCI_DMA_AUTO:
        if (hDrive->AHCIDRIVE_bDma) {
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + hDrive->AHCIDRIVE_usSingleDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma != 0) {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + hDrive->AHCIDRIVE_usUltraDmaMode;
                } else {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
                    hDrive->AHCIDRIVE_usRwDma = AHCI_DMA_MULTI;
                }
            }
        }
        break;

    default:
        break;
    }

    hDrive->AHCIDRIVE_bTrim = (hParam->AHCIPARAM_usDataSetManagement & 0x01) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_usTrimBlockNumMax = hParam->AHCIPARAM_usTrimBlockNumMax;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 14) & 0x01;
    hDrive->AHCIDRIVE_bDrat = iFlag ? LW_TRUE : LW_FALSE;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 5) & 0x01;
    hDrive->AHCIDRIVE_bRzat = iFlag ? LW_TRUE : LW_FALSE;

    if (hDrive->AHCIDRIVE_ucState == AHCI_DEV_INIT) {                   /* �豸��ʼ״̬ʱ��ӡ��ϸ��Ϣ   */
        API_AhciDriveInfoShow(hCtrl, uiDrive, hParam);                  /* ��ӡ��������Ϣ               */
    }
                                                                        /* ���ô���ģʽ                 */
    API_AhciNoDataCommandSend(hCtrl, uiDrive,
                              AHCI_CMD_SET_FEATURE, AHCI_SUB_SET_RWMODE,
                              (UINT8)hDrive->AHCIDRIVE_usRwMode, 0, 0, 0, 0);
    /*
     *  �������Բ���
     */
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_LOOK_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_SET_FEATURE, 
                                  AHCI_SUB_ENABLE_LOOK, 0, 0, 0, 0, 0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_WCACHE_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_SET_FEATURE, 
                                  AHCI_SUB_ENABLE_WCACHE, 0, 0, 0, 0, 0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_SET_FEATURE, 
                                  AHCI_SUB_ENABLE_APM, 0xfe, 0, 0, 0, 0);
    }

    if ((hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) &&
        (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA)) {
        if (hDrive->AHCIDRIVE_bMulti) {
            API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_SET_MULTI, 0, 
                                      hDrive->AHCIDRIVE_usMultiSector, 0, 0, 0, 0);
        } else {
            hDrive->AHCIDRIVE_usRwPio = AHCI_PIO_SINGLE;
        }
    }

    hDrive->AHCIDRIVE_ucState  = AHCI_DEV_OK;
    hDrive->AHCIDRIVE_iPmState = AHCI_PM_ACTIVE_IDLE;

__error_handle:
    API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);

    if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d state %d status 0x%x error 0x%x.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive,
                 hDrive->AHCIDRIVE_ucState, hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDrvInit
** ��������: ��ʼ�� AHCI ����
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDrvInit (AHCI_CTRL_HANDLE  hCtrl)
{
    REGISTER UINT           i;                                          /* ѭ������                     */
    REGISTER UINT           j;                                          /* ѭ������                     */
    INT                     iRet;                                       /* �������                     */
    LW_CLASS_THREADATTR     threadattr;                                 /* �߳̿��ƿ�                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_DRV_HANDLE         hDrv;                                       /* �������                     */
    size_t                  stSizeTemp;                                 /* �ڴ��С                     */
    size_t                  stMemSize;                                  /* �ڴ��С                     */
    UINT8                  *pucCmdList;                                 /* �����б�                     */
    UINT8                  *pucRecvFis;                                 /* �����б�                     */
    INT                     iCurrPort;                                  /* ��ǰ�˿�                     */
    UINT32                  uiPortMap;                                  /* �˿�ӳ����Ϣ                 */
    UINT32                  uiReg;                                      /* �Ĵ���                       */
    UINT32                  uiPortNum;                                  /* ��ǰ�˿�����                 */

    AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.\r\n",
             hCtrl->AHCICTRL_uiIndex, hCtrl->AHCICTRL_cCtrlName, hCtrl->AHCICTRL_uiUnitIndex,
             hCtrl->AHCICTRL_pvRegAddr);

    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* ����δ��װ                   */
        if (hDrv->AHCIDRV_pfuncVendorPlatformInit) {
            iRet = hDrv->AHCIDRV_pfuncVendorPlatformInit(hCtrl);
        } else {
            iRet = ERROR_NONE;
        }

        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor platform init failed.\r\n", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bDrvInstalled = LW_TRUE;                        /* ��ʶ������װ                 */
    }

    if (hCtrl->AHCICTRL_bMonitorStarted == LW_FALSE) {                  /* ����߳�δ����               */
        hCtrl->AHCICTRL_hMsgQueue = API_MsgQueueCreate("ahci_msg",
                                                       AHCI_MSG_QUEUE_SIZE, AHCI_MSG_SIZE,
                                                       LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
        if (hCtrl->AHCICTRL_hMsgQueue == LW_OBJECT_HANDLE_INVALID) {    /* ���������Ϣʧ��             */
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci msg queue failed.\r\n", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }

        /*
         * ��������߳�
         */
        API_ThreadAttrBuild(&threadattr,
                            AHCI_MONITOR_STK_SIZE, AHCI_MONITOR_PRIORITY,
                            LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                            LW_NULL);
        threadattr.THREADATTR_pvArg = (PVOID)hCtrl;
        hCtrl->AHCICTRL_hMonitorThread = API_ThreadCreate("t_ahcimsg",
                                                          (PTHREAD_START_ROUTINE)__ahciMonitorThread,
                                                          (PLW_CLASS_THREADATTR)&threadattr,
                                                          LW_NULL);
        if (hCtrl->AHCICTRL_hMonitorThread == LW_OBJECT_HANDLE_INVALID) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci monitor thread failed.\r\n",
                     hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bMonitorStarted = LW_TRUE;
    }

    if (hCtrl->AHCICTRL_bInstalled != LW_FALSE) {                       /* �������Ѿ�����װ             */
        return  (ERROR_NONE);
    }

    /*
     *  ��ʼ��������
     */
    hCtrl->AHCICTRL_bInstalled   = LW_TRUE;
    hCtrl->AHCICTRL_ulSemTimeout = AHCI_SEM_TIMEOUT_DEF;
    hCtrl->AHCICTRL_piConfigType = &_GiAhciConfigType[0];

    iRet = API_AhciCtrlReset(hCtrl);                                    /* ��λ������                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control reset failed.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hDrv->AHCIDRV_pfuncVendorCtrlInit) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlInit(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor control init failed.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hCtrl->AHCICTRL_bIntConnect == LW_FALSE) {                      /* �ж�δ����                   */
        iRet = API_AhciCtrlIntConnect(hCtrl, __ahciIsr, "ahci_isr");    /* ���ӿ������ж�               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control int connect failed.\r\n", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bIntConnect = LW_TRUE;                          /* ��ʶ�ж��Ѿ�����             */
    }

    iRet = API_AhciCtrlAhciModeEnable(hCtrl);                           /* ʹ�ܿ����� AHCI ģʽ         */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable ahci mode failed.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    iRet = API_AhciCtrlSssSet(hCtrl, LW_TRUE);                          /* ʹ�� Staggered Spin-up       */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable Staggered Spin-up failed.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    API_AhciCtrlCapGet(hCtrl);                                          /* ��ȡ������������Ϣ           */
    API_AhciCtrlImpPortGet(hCtrl);                                      /* ��ȡ�˿ڲ���                 */
    API_AhciCtrlInfoShow(hCtrl);                                        /* չʾ��������ϸ��Ϣ           */

    if (hCtrl->AHCICTRL_uiImpPortNum < 1) {
        AHCI_LOG(AHCI_LOG_ERR, "drive imp port failed ctrl %d.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }
    
    /*
     *  �������������ƿ�
     */
    stSizeTemp = sizeof(AHCI_DRIVE_CB) * (size_t)hCtrl->AHCICTRL_uiImpPortNum;
    hCtrl->AHCICTRL_hDrive = (AHCI_DRIVE_HANDLE)__SHEAP_ZALLOC(stSizeTemp);
    if (!hCtrl->AHCICTRL_hDrive) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc drive tcb failed ctrl %d.\r\n", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    /*
     *  �˿���������Ϊ 0x1F ���Ϊ 32 ��, ���Ƕ˿ڷֲ���Ϣ�;���������й�
     *  �� VendorID 8086 DevieceID 8c02 �ж˿������� 4, ���Ƕ˿ڷֲ�ʽ Bit5 Bit1 Bit0
     */
    uiPortMap = hCtrl->AHCICTRL_uiImpPortMap;                           /* �˿ڷֲ���Ϣ                 */
    uiPortNum = __MAX(hCtrl->AHCICTRL_uiPortNum, fls(uiPortMap));       /* ������Դ����˿�����         */

    /*
     *  �������������������ڴ�ṹ
     */
    stMemSize  = (AHCI_CMD_LIST_SIZE)
               + (AHCI_RECV_FIS_SIZE)
               + (AHCI_CMD_TABLE_SIZE * hCtrl->AHCICTRL_uiCmdSlotNum);
    stMemSize *= uiPortNum;

    pucCmdList = (UINT8 *)API_CacheDmaMallocAlign(stMemSize, AHCI_CMD_LIST_ALIGN);
    if (!pucCmdList) {                                                  /* ���������ڴ�                 */
        AHCI_LOG(AHCI_LOG_ERR, "alloc dma buf size 0x%08x failed.\r\n", stMemSize);
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ�����������ڴ�����
     */
    pucRecvFis = pucCmdList + (uiPortNum * AHCI_CMD_LIST_SIZE);

    AHCI_LOG(AHCI_LOG_PRT, "alloc cmd list addr %p size 0x%08x fis addr %p size 0x%08x.\r\n",
             pucCmdList, AHCI_CMD_LIST_ALIGN, pucRecvFis, sizeof(AHCI_RECV_FIS_CB));

    AHCI_LOG(AHCI_LOG_PRT, "port num %d active %d map 0x%08x.\r\n",
             hCtrl->AHCICTRL_uiPortNum, hCtrl->AHCICTRL_uiImpPortNum, hCtrl->AHCICTRL_uiImpPortMap);

    iCurrPort = 0;                                                      /* ��õ�ǰ�˿�                 */

    for (i = 0; i < uiPortNum; i++) {                                   /* ��������˿�                 */
        if (uiPortMap & 1) {                                            /* �˿���Ч                     */
            AHCI_LOG(AHCI_LOG_PRT, "port %d current port %d.\r\n", i, iCurrPort);

            hDrive = &hCtrl->AHCICTRL_hDrive[iCurrPort];                /* ��ȡ���������ƾ��           */
            LW_SPIN_INIT(&hDrive->AHCIDRIVE_slLock);                    /* ��ʼ��������������           */
            hDrive->AHCIDRIVE_hCtrl  = hCtrl;                           /* ������������               */
            hDrive->AHCIDRIVE_uiPort = i;                               /* ��ȡ��ǰ�˿�����             */
                                                                        /* ��ȡ����������ַ             */
            hDrive->AHCIDRIVE_pvRegAddr = (PVOID)((ULONG)hCtrl->AHCICTRL_pvRegAddr 
                                        + AHCI_DRIVE_BASE(i));
            hDrive->AHCIDRIVE_uiIntCount           = 0;                 /* ��ʼ���жϼ���               */
            hDrive->AHCIDRIVE_uiTimeoutErrorCount  = 0;                 /* ��ʼ����ʱ�������           */
            hDrive->AHCIDRIVE_uiTaskFileErrorCount = 0;                 /* ��ʼ��TF �������            */
            hDrive->AHCIDRIVE_uiNextTag            = 0;                 /* ��ʼ���������               */
            hDrive->AHCIDRIVE_uiBuffNextTag        = 0;                 /* ��ʼ���������������         */
            iCurrPort++;                                                /* ���µ�ǰ�˿�                 */

            /*
             *  ��ʼ���˿ڲ���
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, i, AHCI_OPT_CMD_PORT_ENDIAN_TYPE_GET,
                                              (LONG)((INT *)&hDrive->AHCIDRIVE_iPortEndianType));
            if (iRet != ERROR_NONE) {
                hDrive->AHCIDRIVE_iPortEndianType = AHCI_ENDIAN_TYPE_LITTEL;
                hDrive->AHCIDRIVE_pfuncDriveRead  = __ahciPortRegReadLe;
                hDrive->AHCIDRIVE_pfuncDriveWrite = __ahciPortRegWriteLe;
            
            } else {
                switch (hDrive->AHCIDRIVE_iPortEndianType) {

                case AHCI_ENDIAN_TYPE_BIG:
                    hDrive->AHCIDRIVE_iPortEndianType = AHCI_ENDIAN_TYPE_BIG;
                    hDrive->AHCIDRIVE_pfuncDriveRead  = __ahciPortRegReadBe;
                    hDrive->AHCIDRIVE_pfuncDriveWrite = __ahciPortRegWriteBe;
                    break;

                case AHCI_ENDIAN_TYPE_LITTEL:
                default:
                    hDrive->AHCIDRIVE_iPortEndianType = AHCI_ENDIAN_TYPE_LITTEL;
                    hDrive->AHCIDRIVE_pfuncDriveRead  = __ahciPortRegReadLe;
                    hDrive->AHCIDRIVE_pfuncDriveWrite = __ahciPortRegWriteLe;
                    break;
                }
            }

            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, i, AHCI_OPT_CMD_PARAM_ENDIAN_TYPE_GET,
                                              (LONG)((INT *)&hDrive->AHCIDRIVE_iParamEndianType));
            if (iRet != ERROR_NONE) {
                hDrive->AHCIDRIVE_iParamEndianType = AHCI_ENDIAN_TYPE_LITTEL;
            
            } else {
                switch (hDrive->AHCIDRIVE_iParamEndianType) {

                case AHCI_ENDIAN_TYPE_BIG:
                    hDrive->AHCIDRIVE_iParamEndianType = AHCI_ENDIAN_TYPE_BIG;
                    break;

                case AHCI_ENDIAN_TYPE_LITTEL:
                default:
                    hDrive->AHCIDRIVE_iParamEndianType = AHCI_ENDIAN_TYPE_LITTEL;
                    break;
                }
            }

            /*
             *  ��ȡ��������ʼ����
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_SECTOR_START_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulStartSector));
            if (iRet != ERROR_NONE) {
                hDrive->AHCIDRIVE_ulStartSector = 0;
            }
            
            /*
             *  ��ȡ����������̽��ʱ��
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_UNIT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulProbTimeUnit));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_ulProbTimeUnit == 0)) {
                hDrive->AHCIDRIVE_ulProbTimeUnit = AHCI_DRIVE_PROB_TIME_UNIT;
            }
            
            /*
             *  ��ȡ̽�����
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_COUNT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulProbTimeCount));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_ulProbTimeCount == 0)) {
                hDrive->AHCIDRIVE_ulProbTimeCount = AHCI_DRIVE_PROB_TIME_COUNT;
            }

            if (hDrv->AHCIDRV_pfuncVendorDriveInit) {
                iRet = hDrv->AHCIDRV_pfuncVendorDriveInit(hDrive);
            } else {
                iRet = ERROR_NONE;
            }

            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "port %d vendor init failed.\r\n", hDrive->AHCIDRIVE_uiPort);

                return  (PX_ERROR);
            }
            
            /*
             *  ���������б��ַ��Ϣ
             */
            hDrive->AHCIDRIVE_hCmdList = (AHCI_CMD_LIST_HANDLE)pucCmdList;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLB, AHCI_ADDR_LOW32(pucCmdList));
            AHCI_PORT_READ(hDrive, AHCI_PxCLB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLBU, AHCI_ADDR_HIGH32(pucCmdList));
            AHCI_PORT_READ(hDrive, AHCI_PxCLBU);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            pucCmdList += AHCI_CMD_LIST_SIZE;
            
            /*
             *  ���½��� FIS ��ַ��Ϣ
             */
            hDrive->AHCIDRIVE_hRecvFis = (AHCI_RECV_FIS_HANDLE)pucRecvFis;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFB, AHCI_ADDR_LOW32(pucRecvFis));
            AHCI_PORT_READ(hDrive, AHCI_PxFB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFBU, AHCI_ADDR_HIGH32(pucRecvFis));
            AHCI_PORT_READ(hDrive, AHCI_PxFBU);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            pucRecvFis += AHCI_RECV_FIS_SIZE;
            
            /*
             *  ��������������ַ�������б�
             */
            hDrive->AHCIDRIVE_hCmdTable = (AHCI_CMD_TABLE_HANDLE)pucRecvFis;
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                addr_t   addr = (addr_t)&hDrive->AHCIDRIVE_hCmdTable[j];
            
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCTAddrLow =
                        cpu_to_le32(AHCI_ADDR_LOW32(addr));
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCTAddrHigh =
                        cpu_to_le32(AHCI_ADDR_HIGH32(addr));
                    
                AHCI_LOG(AHCI_LOG_PRT, "cmd list %2d addr %p low 0x%08x high 0x%08x.\r\n",
                         j, &hDrive->AHCIDRIVE_hCmdTable[j],
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCTAddrLow,
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCTAddrHigh);
            }
            pucRecvFis += (AHCI_CMD_TABLE_SIZE * hCtrl->AHCICTRL_uiCmdSlotNum);
            API_CacheDmaFlush(hDrive->AHCIDRIVE_hCmdList,
                              (size_t)hCtrl->AHCICTRL_uiCmdSlotNum * sizeof(AHCI_CMD_LIST_CB));
            
            /*
             *  ���������ͬ���ź���
             */
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                hDrive->AHCIDRIVE_hSyncBSem[j] = API_SemaphoreBCreate("ahci_sync",
                                                                      LW_FALSE,
                                                                      (LW_OPTION_WAIT_FIFO |
                                                                       LW_OPTION_OBJECT_GLOBAL),
                                                                      LW_NULL);
                AHCI_LOG(AHCI_LOG_PRT, "slot %2d sync sem %p.\r\n", j, hDrive->AHCIDRIVE_hSyncBSem[j]);
            }

            hDrive->AHCIDRIVE_uiCmdMask    = 0;                         /* ��ʼ��������ʼ״̬           */
                                                                        /* ��ʼ���������               */
            hDrive->AHCIDRIVE_uiQueueDepth = hCtrl->AHCICTRL_uiCmdSlotNum;
            hDrive->AHCIDRIVE_bQueued      = LW_FALSE;                  /* ��ʼ���Ƿ�ʹ�ܶ���ģʽ       */
            
            /*
             *  ��ʼ��ͬ����
             */
            hDrive->AHCIDRIVE_hLockMSem = API_SemaphoreMCreate("ahci_dlock",
                                                               LW_PRIO_DEF_CEILING,
                                                               (LW_OPTION_WAIT_PRIORITY |
                                                                LW_OPTION_DELETE_SAFE |
                                                                LW_OPTION_INHERIT_PRIORITY |
                                                                LW_OPTION_OBJECT_GLOBAL),
                                                               LW_NULL);

            hDrive->AHCIDRIVE_hQueueSlotCSem = API_SemaphoreCCreate("ahci_slot",
                                                                    1,
                                                                    AHCI_CMD_SLOT_MAX,
                                                                    (LW_OPTION_WAIT_PRIORITY |
                                                                     LW_OPTION_OBJECT_GLOBAL),
                                                                    LW_NULL);
            /*
             *  �������
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
            AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiReg);
            AHCI_PORT_READ(hDrive, AHCI_PxSERR);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            /*
             *  ����ж�״̬
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxIS);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiReg);
            AHCI_PORT_READ(hDrive, AHCI_PxIS);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            /*
             *  ʹ���ж�
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIE,
                            AHCI_PIE_PRCE | AHCI_PIE_PCE | AHCI_PIE_PSE | AHCI_PIE_DSE | AHCI_PIE_DPE |
                            AHCI_PIE_DHRE | AHCI_PIS_SDBS);
            AHCI_PORT_READ(hDrive, AHCI_PxIE);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            /*
             *  ��λ������״̬
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
            if (uiReg & (AHCI_PCMD_CR | AHCI_PCMD_FR | AHCI_PCMD_FRE | AHCI_PCMD_ST)) {
                uiReg &= ~AHCI_PCMD_ST;
                AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
                AHCI_PORT_READ(hDrive, AHCI_PxCMD);
                API_AhciDriveRegWait(hDrive,
                                     AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
            }
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCMD,
                            (AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_CLO | AHCI_PCMD_POD |
                             AHCI_PCMD_SUD));
            AHCI_PORT_READ(hDrive, AHCI_PxCMD);
            API_AhciDriveRegWait(hDrive,
                                 AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
        }

        uiPortMap >>= 1;
    }
    
    /*
     *  ʹ�ܿ������ж�
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg | AHCI_GHC_IE);
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);

    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum ; i++) {               /* ��ʼ��ָ��������             */
        hDrive = &hCtrl->AHCICTRL_hDrive[i];                            /* ��ȡ���������               */
        hDrive->AHCIDRIVE_hDev = LW_NULL;                               /* ��ʼ���豸���               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;                      /* ��ʼ��������״̬             */
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;                      /* ��ʼ���豸����               */
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;                              /* ��ʼ�� NCQ ��־              */
        hDrive->AHCIDRIVE_bPortError = LW_FALSE;                        /* ��ʼ���˿ڴ���״̬           */
        hDrive->AHCIDRIVE_iInitActive = LW_FALSE;                       /* ��ʼ���״̬               */

        for (j = 0; j < AHCI_RETRY_NUM; j++) {                          /* ̽���豸�Ƿ��Ѿ�����         */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSSTS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSSTS) & AHCI_PSSTS_DET_MSK;
            if (uiReg == AHCI_PSSTS_DET_PHY) {
                break;
            }
            API_TimeMSleep(50);                                         /* ���µȴ�̽����             */
        }
        if ((j < AHCI_RETRY_NUM) &&
            (uiReg == AHCI_PSSTS_DET_PHY)) {                            /* �Ѿ�̽�⵽�豸               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy det.\r\n", hCtrl->AHCICTRL_uiIndex, i);
            __ahciDiskCtrlInit(hCtrl, i);                               /* ��ʼ�����̿�����             */
            __ahciDiskDriveInit(hCtrl, i);                              /* ��ʼ����������               */
        
        } else {                                                        /* û��̽�⵽�豸               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy not det.\r\n", hCtrl->AHCICTRL_uiIndex, i);
            hDrive->AHCIDRIVE_ucType  = AHCI_TYPE_NONE;
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciNoDataCommandSend
** ��������: ���������ݴ��������
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           ucCmd          ����
**           uiFeature      �ض�����
**           usSector       ������
**           ucLbaLow       LBA ����
**           ucLbaMid       LBA ����
**           ucLbaHigh      LBA ����
**           iFlags         ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciNoDataCommandSend (AHCI_CTRL_HANDLE  hCtrl,
                                UINT              uiDrive,
                                UINT8             ucCmd,
                                UINT32            uiFeature,
                                UINT16            usSector,
                                UINT8             ucLbaLow,
                                UINT8             ucLbaMid,
                                UINT8             ucLbaHigh,
                                INT               iFlags)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = ucCmd;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = usSector;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = uiFeature;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = (UINT64)((ucLbaHigh << 16) | (ucLbaMid << 8) | ucLbaLow);
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_NONE;
    hCmd->AHCICMD_iFlags     = (INT)(iFlags | AHCI_CMD_FLAG_NON_SEC_DATA);
    if ((ucCmd == AHCI_CMD_FLUSH_CACHE) ||
        (ucCmd == AHCI_CMD_FLUSH_CACHE_EXT)) {
        hCmd->AHCICMD_iFlags    |= AHCI_CMD_FLAG_CACHE;
        hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
        hCmd->AHCICMD_ulDataLen  = 0;
    }

    iRet = API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        AHCI_CMD_LOG(AHCI_LOG_ERR, "no data cmd error ctrl %d drive %d cmd %02x feature %08x "
                     "sector %04x lba low %02x lba mid %02x lba high %02x flag %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive, ucCmd, uiFeature,
                     usSector, ucLbaLow, ucLbaMid, ucLbaHigh, iFlags);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlFree
** ��������: �ͷ�һ�� AHCI ������
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlFree (AHCI_CTRL_HANDLE  hCtrl)
{
    API_AhciCtrlDelete(hCtrl);                                          /* ɾ��������                   */

    if (hCtrl != LW_NULL) {
        __SHEAP_FREE(hCtrl);                                            /* �ͷſ�����                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlCreate
** ��������: ���� AHCI ������
** �䡡��  : pcName     ����������
**           uiUnit     �������������
**           pvArg      ��չ����
** �䡡��  : AHCI ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlCreate (CPCHAR  pcName, UINT  uiUnit, PVOID  pvArg)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    REGISTER INT        i      = 0;                                     /* ѭ������                     */
    AHCI_CTRL_HANDLE    hCtrl  = LW_NULL;                               /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_DRV_HANDLE     hDrv   = LW_NULL;                               /* �������                     */
    CHAR                cDriveName[AHCI_DEV_NAME_MAX] = {0};            /* �������豸����               */

    if (!pcName) {                                                      /* ��������������               */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hCtrl = (AHCI_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_CTRL_CB));     /* ������������ƿ�             */
    if (!hCtrl) {                                                       /* ������ƿ�ʧ��               */
        AHCI_LOG(AHCI_LOG_ERR, "alloc ctrl %s unit %d tcb failed.\r\n", pcName, uiUnit);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }

    hDrv = API_AhciDrvHandleGet(pcName);                                /* ͨ�����ֻ���������         */
    if (!hDrv) {                                                        /* ����δע��                   */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver %s not register.\r\n", pcName);
        goto    __error_handle;
    }

    hDrv->AHCIDRV_hCtrl         = hCtrl;                                /* �������´����Ŀ�����         */
    hCtrl->AHCICTRL_hDrv        = hDrv;                                 /* ����������                   */
    lib_strlcpy(&hCtrl->AHCICTRL_cCtrlName[0], &hDrv->AHCIDRV_cDrvName[0], AHCI_CTRL_NAME_MAX);
    hCtrl->AHCICTRL_uiCoreVer   = AHCI_CTRL_DRV_VER_NUM;                /* ���������İ汾               */
    hCtrl->AHCICTRL_uiUnitIndex = uiUnit;                               /* �������������               */
    hCtrl->AHCICTRL_uiIndex     = API_AhciCtrlIndexGet();               /* ����������                   */
    hCtrl->AHCICTRL_pvPciArg    = pvArg;                                /* ����������                   */
    API_AhciCtrlAdd(hCtrl);                                             /* ��ӿ�����                   */
    API_AhciDrvCtrlAdd(hDrv, hCtrl);                                    /* ����������Ӧ�Ŀ�����         */

    /*
     *  ��ʼ������������
     */
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_CTRL_ENDIAN_TYPE_GET,
                                      (LONG)((INT *)&hCtrl->AHCICTRL_iEndianType));
    if (iRet != ERROR_NONE) {
        hCtrl->AHCICTRL_iEndianType = AHCI_ENDIAN_TYPE_LITTEL;
        hCtrl->AHCICTRL_pfuncCtrlRead  = __ahciCtrlRegReadLe;
        hCtrl->AHCICTRL_pfuncCtrlWrite = __ahciCtrlRegWriteLe;
    
    } else {
        switch (hCtrl->AHCICTRL_iEndianType) {

        case AHCI_ENDIAN_TYPE_BIG:
            hCtrl->AHCICTRL_iEndianType    = AHCI_ENDIAN_TYPE_BIG;
            hCtrl->AHCICTRL_pfuncCtrlRead  = __ahciCtrlRegReadBe;
            hCtrl->AHCICTRL_pfuncCtrlWrite = __ahciCtrlRegWriteBe;
            break;

        case AHCI_ENDIAN_TYPE_LITTEL:
        default:
            hCtrl->AHCICTRL_iEndianType = AHCI_ENDIAN_TYPE_LITTEL;
            hCtrl->AHCICTRL_pfuncCtrlRead  = __ahciCtrlRegReadLe;
            hCtrl->AHCICTRL_pfuncCtrlWrite = __ahciCtrlRegWriteLe;
            break;
        }
    }

    if (hDrv->AHCIDRV_pfuncVendorCtrlReadyWork) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlReadyWork(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d vendor ready work failed.\r\n", pcName, uiUnit);
        goto    __error_handle;
    }

    if (!hCtrl->AHCICTRL_pvRegAddr) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d reg addr null ctrl %d unit %d.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiUnit);
        goto    __error_handle;
    }

    iRet = __ahciDrvInit(hCtrl);                                        /* ������ʼ��                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d driver init failed.\r\n", pcName, uiUnit);
        goto    __error_handle;
    }
    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {                /* ��������ʼ��                 */
        hDrive = &hCtrl->AHCICTRL_hDrive[i];
        if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {
#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */
            snprintf(cDriveName, AHCI_DEV_NAME_MAX, AHCI_DRV_NAME "-%d:%d", hCtrl->AHCICTRL_uiIndex, i);
            __ahciDiskConfig(hCtrl, i, &cDriveName[0]);                 /* ��ʼ��ָ��������             */
#endif                                                                  /* AHCI_ATAPI_EN                */
        } else {
            snprintf(cDriveName, AHCI_DEV_NAME_MAX, AHCI_DRV_NAME "-%d:%d", hCtrl->AHCICTRL_uiIndex, i);
            __ahciDiskConfig(hCtrl, i, &cDriveName[0]);                 /* ��ʼ��ָ��������             */
        }
    }

    return  (hCtrl);                                                    /* ���ؿ��������               */

__error_handle:                                                         /* ������                     */
    API_AhciCtrlFree(hCtrl);                                            /* �ͷſ�����                   */
    API_AhciDrvCtrlDelete(hDrv, hCtrl);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciMonitorThread
** ��������: AHCI ����߳�
** �䡡��  : pvArg     �̲߳���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __ahciMonitorThread (PVOID  pvArg)
{
    ULONG               ulRet;                                          /* �������                     */
    AHCI_MSG_CB         tCtrlMsg;                                       /* ��Ϣ���ƿ�                   */
    size_t              stTemp = 0;                                     /* ��ʱ��С                     */
    INT                 iMsgId;                                         /* ��Ϣ��ʶ                     */
    INT                 iDrive;                                         /* ����������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */

#if AHCI_HOTPLUG_EN > 0                                                 /* �Ƿ�ʹ���Ȳ��               */
    AHCI_DEV_HANDLE     hDev    = LW_NULL;                              /* �豸���                     */
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* ���豸���                   */
    INT                 iRetry;                                         /* ���Բ���                     */
    INT                 iRet;                                           /* �������                     */
#endif                                                                  /* AHCI_HOTPLUG_EN              */

    for (;;) {
        hCtrl = (AHCI_CTRL_HANDLE)pvArg;                                /* ��ȡ���������               */
        ulRet = API_MsgQueueReceive(hCtrl->AHCICTRL_hMsgQueue,
                                    (PVOID)&tCtrlMsg,
                                    AHCI_MSG_SIZE,
                                    &stTemp,
                                    LW_OPTION_WAIT_INFINITE);           /* �ȴ���Ϣ                     */
        if (ulRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ahci msg queue recv error ctrl %d.\r\n", hCtrl->AHCICTRL_uiIndex);
            continue;                                                   /* ���յ�������Ϣ������ȴ�     */
        }

        if (hCtrl != tCtrlMsg.AHCIMSG_hCtrl) {                          /* �����������Ч               */
            AHCI_LOG(AHCI_LOG_ERR, "msg handle error ctrl %d.\r\n", hCtrl->AHCICTRL_uiIndex);
            continue;
        }
        hCtrl  = tCtrlMsg.AHCIMSG_hCtrl;
        iDrive = tCtrlMsg.AHCIMSG_uiDrive;
        if ((iDrive < 0) ||
            (iDrive >= hCtrl->AHCICTRL_uiImpPortNum)) {                 /* ��������������               */
            AHCI_LOG(AHCI_LOG_ERR, "drive %d is out of range (0-%d).\r\n", iDrive, (AHCI_DRIVE_MAX - 1));
            continue;
        }
        iMsgId = tCtrlMsg.AHCIMSG_uiMsgId;
        hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
        /*
         *  ����ָ����Ϣ
         */
        switch (iMsgId) {

        case AHCI_MSG_ATTACH:                                           /* �豸����                     */
            AHCI_LOG(AHCI_LOG_PRT, "recv attach msg ctrl %d drive %d.\r\n",
                     hCtrl->AHCICTRL_uiIndex, iDrive);
            hDrive->AHCIDRIVE_uiAttachNum += 1;                         /* �豸�������                 */
#if AHCI_HOTPLUG_EN > 0                                                 /* �Ƿ�ʹ���Ȳ��               */
            if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_NONE) {
                continue;
            }

            hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;
            AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d drive %d.\r\n", hCtrl->AHCICTRL_uiIndex, iDrive);

            iRet = __ahciDriveNoBusyWait(hDrive);
            if (iRet != ERROR_NONE) {
                break;
            }

            iRetry = 0;
            iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "ctrl init err ctrl %d drive %d retry %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }
                iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            }

            iRetry = 0;
            iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "drive init err ctrl %d drive %d retry %d.\r\n",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }
                iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            }

            hBlkDev = __ahciBlkDevCreate(hCtrl, iDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
            if (!hBlkDev) {
                AHCI_LOG(AHCI_LOG_ERR, "create blk dev error %s.\r\n", hDrive->AHCIDRIVE_cDevName);
                break;
            }

            hDev = API_AhciDevHandleGet(hCtrl->AHCICTRL_uiIndex, iDrive);
            if (!hDev) {
                API_AhciDevAdd(hCtrl, iDrive);
            }
#endif                                                                  /* AHCI_HOTPLUG_EN              */
            break;

        case AHCI_MSG_REMOVE:                                           /* �豸�Ƴ�                     */
            AHCI_LOG(AHCI_LOG_PRT, "remove ctrl %d drive %d.\r\n", hCtrl->AHCICTRL_uiIndex, iDrive);
            hDrive->AHCIDRIVE_uiRemoveNum += 1;
#if AHCI_HOTPLUG_EN > 0                                                 /* �Ƿ�ʹ���Ȳ��               */
            if (hDrive->AHCIDRIVE_hDev != LW_NULL) {
                __ahciBlkDevRemove(hCtrl, iDrive);
            }
#endif                                                                  /* AHCI_HOTPLUG_EN              */
            break;

        case AHCI_MSG_ERROR:                                            /* �豸����                     */
            AHCI_LOG(AHCI_LOG_PRT, "error ctrl %d drive %d error 0x%02x status 0x%02x.\r\n",
                     hCtrl->AHCICTRL_uiIndex, iDrive,
                     hDrive->AHCIDRIVE_uiIntError, hDrive->AHCIDRIVE_uiIntStatus);

            if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA) {
                UINT  uiReg;

                AHCI_LOG(AHCI_LOG_ERR, "error ctrl %d drive %d ata drive status init.\r\n",
                         hCtrl->AHCICTRL_uiIndex, iDrive);

                uiReg  = AHCI_PORT_READ(hDrive, AHCI_PxSCTL);
                uiReg &= ~0x0ff;
                uiReg |= AHCI_PSCTL_DET_RESET | AHCI_PSCTL_IPM_PARSLUM_DISABLED | AHCI_PSSTS_SPD_GEN2;
                AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, uiReg);
                AHCI_PORT_READ(hDrive, AHCI_PxSCTL);

                API_TimeMSleep(1);

                uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSCTL);

                iRetry = 5;
                do {
                    uiReg = (uiReg & 0x0f0) | 0x300;
                    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, uiReg);
                    API_TimeMSleep(200);

                    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSCTL);
                } while (((uiReg & 0xf0f) != 0x300) && (--iRetry > 0));

                iRet = API_AhciDriveRegWait(hDrive,
                                            AHCI_PxSSTS, AHCI_PSSTS_DET_MSK, LW_FALSE, AHCI_PSSTS_DET_PHY,
                                            1, 50, &uiReg);
                if (iRet != ERROR_NONE) {
                    break;
                }

                AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, 0xffffffff);

            } else if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {
#if AHCI_ATAPI_EN > 0                                                   /* �Ƿ�ʹ�� ATAPI               */
                AHCI_LOG(AHCI_LOG_PRT, "error ctrl %d drive %d atapi drive status init.\r\n",
                         hCtrl->AHCICTRL_uiIndex, iDrive);

                iRetry = 0;
                iRet = __ahciDriveNoBusyWait(hDrive);
                while (iRet != ERROR_NONE) {
                    AHCI_LOG(AHCI_LOG_ERR, "ctrl nobusy err ctrl %d drive %d drive no busy retry %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                    iRetry += 1;
                    if (iRetry >= AHCI_RETRY_NUM) {
                        break;
                    }
                    iRet = __ahciDriveNoBusyWait(hDrive);
                }

                iRetry = 0;
                iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
                while (iRet != ERROR_NONE) {
                    AHCI_LOG(AHCI_LOG_ERR, "ctrl init err ctrl %d drive %d retry %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                    iRetry += 1;
                    if (iRetry >= AHCI_RETRY_NUM) {
                        break;
                    }
                    iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
                }

                iRetry = 0;
                iRet = __ahciDiskDriveInit(hCtrl, iDrive);
                while (iRet != ERROR_NONE) {
                    AHCI_LOG(AHCI_LOG_ERR, "drive init err ctrl %d drive %d retry %d.\r\n",
                             hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                    iRetry += 1;
                    if (iRetry >= AHCI_RETRY_NUM) {
                        break;
                    }
                    iRet = __ahciDiskDriveInit(hCtrl, iDrive);
                }

                hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;
#endif                                                                  /* AHCI_ATAPI_EN                */
            }
            hDrive->AHCIDRIVE_bPortError = LW_FALSE;
            break;

        case AHCI_MSG_TIMEOUT:                                          /* ��ʱ����                     */
            AHCI_LOG(AHCI_LOG_ERR, "timeout ctrl %d drive %d error 0x%02x status 0x%02x.\r\n",
                     hCtrl->AHCICTRL_uiIndex, iDrive,
                     hDrive->AHCIDRIVE_uiIntError, hDrive->AHCIDRIVE_uiIntStatus);
            break;

        default:
            break;
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciIsr
** ��������: �жϷ���
** �䡡��  : hCtrl    ���������
**           ulVector       �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t __ahciIsr (PVOID  pvArg, ULONG  ulVector)
{
#define AHCI_RELEASE_SEM(x, mask)  \
        for ((x) = 0; (x) < hDrive->AHCIDRIVE_uiQueueDepth; (x)++) {    \
            if (mask & AHCI_BIT_MASK(x)) {  \
                API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[(x)]);   \
            }   \
        }

    AHCI_CTRL_HANDLE    hCtrl = (AHCI_CTRL_HANDLE)pvArg;
    REGISTER INT        i, j;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_MSG_CB         tCtrlMsg;
    UINT32              uiSataIntr;
    UINT32              uiPortIntr;
    UINT32              uiTaskStatus;
    UINT32              uiSataStatus;
    UINT32              uiActive;
    UINT32              uiReg;
    UINT32              uiSlotBit;
    UINT32              uiMask;
    INTREG              iregInterLevel = 0;                             /* �жϼĴ���                   */

    tCtrlMsg.AHCIMSG_hCtrl = hCtrl;
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_IS);
    if (!uiReg) {
        return  (LW_IRQ_NONE);
    }

    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {
        hDrive = &hCtrl->AHCICTRL_hDrive[i];
        LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
        if (uiReg & AHCI_BIT_MASK(hDrive->AHCIDRIVE_uiPort)) {
            hDrive->AHCIDRIVE_uiIntCount++;

            uiPortIntr = AHCI_PORT_READ(hDrive, AHCI_PxIS);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiPortIntr);
            uiSataIntr = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
            AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiSataIntr);

            uiTaskStatus = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
            uiSataStatus = AHCI_PORT_READ(hDrive, AHCI_PxSSTS);
            hDrive->AHCIDRIVE_uiIntStatus = uiTaskStatus & 0xff;
            hDrive->AHCIDRIVE_uiIntError  = (uiTaskStatus & 0xff00) >> 8;

            if ((uiPortIntr & AHCI_PIS_PRCS) &&
                ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_ACTIVE)) {
                if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                    tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ATTACH;
                    tCtrlMsg.AHCIMSG_uiDrive = i;
                    LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                    API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                    continue;
                }
            }

            if ((uiPortIntr & AHCI_PIS_PRCS) &&
                ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_DEVICE_NONE)) {
                if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                    hDrive->AHCIDRIVE_ucType  = AHCI_TYPE_NONE;
                    hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
                    hDrive->AHCIDRIVE_hDev->AHCIDEV_tBlkDev.BLKD_bDiskChange = LW_TRUE;
                    
                    uiMask = hDrive->AHCIDRIVE_uiCmdMask;
                    hDrive->AHCIDRIVE_uiCmdMask = 0;
                    
                    tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_REMOVE;
                    tCtrlMsg.AHCIMSG_uiDrive = i;
                    LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                    AHCI_RELEASE_SEM(j, uiMask);
                    API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                
                } else {
                    LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                }
                continue;
            }

            if (uiPortIntr & AHCI_PIS_TFES) {
                hDrive->AHCIDRIVE_bPortError = LW_TRUE;
                hDrive->AHCIDRIVE_uiTaskFileErrorCount++;
                
                uiMask = hDrive->AHCIDRIVE_uiCmdMask;
                hDrive->AHCIDRIVE_uiCmdMask = 0;
                
                tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ERROR;
                tCtrlMsg.AHCIMSG_uiDrive = i;
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                AHCI_RELEASE_SEM(j, uiMask);
                API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                continue;
            }

            if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {
                uiActive = AHCI_PORT_READ(hDrive, AHCI_PxSACT);
            } else {
                uiActive = AHCI_PORT_READ(hDrive, AHCI_PxCI);
            }

            uiMask = 0;
            for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                uiSlotBit = AHCI_BIT_MASK(j);
                if ((hDrive->AHCIDRIVE_uiCmdMask & uiSlotBit) && (!(uiActive & uiSlotBit))) {
                    uiMask                      |=  uiSlotBit;
                    hDrive->AHCIDRIVE_uiCmdMask &= ~uiSlotBit;
                }
            }
            LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
            AHCI_RELEASE_SEM(j, uiMask);
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
        }
    }

    AHCI_CTRL_WRITE(hCtrl, AHCI_IS, uiReg);

    return  (LW_IRQ_HANDLED);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
