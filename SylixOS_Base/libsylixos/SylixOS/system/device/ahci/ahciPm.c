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
** ��   ��   ��: ahciPm.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 31 ��
**
** ��        ��: AHCI �豸��Դ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciPm.h"
/*********************************************************************************************************
** ��������: API_AhciPmPowerModeGet
** ��������: ��ȡ��ǰ��Դ����ģʽ
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pucMode    ��Դ����ģʽ
** �䡡��  : ERROR or AHCI_PM_ACTIVE_IDLE or AHCI_PM_STANDBY or AHCI_PM_SLEEP
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciPmPowerModeGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8 *pucMode)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    UINT8                   ucPower;

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d power mode get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d power mode get state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    if (hDrive->AHCIDRIVE_iPmState == AHCI_PM_SLEEP) {
        if (pucMode) {
            *pucMode = AHCI_PM_SLEEP;
        }
        return  (ERROR_NONE);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive, AHCI_CMD_CHECK_POWER_LEVEL, 0, 0, 0, 0, 0, 0);
    if (iRet == ERROR_NONE) {
        ucPower  = hDrive->AHCIDRIVE_hRecvFis->AHCIRECVFIS_ucD2hRegisterFis[12];
        switch (ucPower) {

        case 0x00:
            if (pucMode) {
                *pucMode = AHCI_PM_STANDBY;
            }
            return  (ERROR_NONE);

        case 0x80:
        case 0xff:
            if (pucMode) {
                *pucMode = AHCI_PM_ACTIVE_IDLE;
            }
            return  (ERROR_NONE);

        default:
            return  (PX_ERROR);
        }
    }

    AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d power mode get no data cmd send error.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_AhciApmModeGet
** ��������: ��ȡ��ǰ��Դ����ģʽ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pucMode    �Ƿ�Ϊʹ��״̬ (0x01 - 0xfe)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciApmModeGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8 *pucMode)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* �������                     */
    UINT16                  usCurrentAPM;

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d apm mode get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d apm mode get state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesEnabled1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciDiskAtaParamGet(hCtrl, uiDrive, (PVOID)hParam);  /* ��ȡ����������               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d apm mode read ata parameters failed.\r\n",
                     hCtrl->AHCICTRL_uiIndex, uiDrive);
            return  (PX_ERROR);
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

        usCurrentAPM = hParam->AHCIPARAM_usCurrentAPM & 0xff;
    } else {
        usCurrentAPM = 0;
    }

    if (pucMode) {
        *pucMode = (UINT8)(usCurrentAPM & 0xff);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciApmDisable
** ��������: �����豸�߼���Դ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciApmDisable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* �������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_DISABLE_APM, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciApmEnable
** ��������: ʹ���豸�߼���Դ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           iApm       ��Դ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciApmEnable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iApm)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* �������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_APM, iApm, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciPmActive
** ��������: �豸��Դ�Ƿ�ʹ��
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciPmActive (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
