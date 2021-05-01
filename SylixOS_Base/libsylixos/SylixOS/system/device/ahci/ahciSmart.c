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
** ��   ��   ��: ahciSmart.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 31 ��
**
** ��        ��: AHCI �豸 S.M.A.R.T (Self-Monitoring, Analysis and Reporting Technology)
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#include "ahci.h"
#include "ahciLib.h"
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0) && (AHCI_SMART_EN > 0)
/*********************************************************************************************************
** ��������: API_AhciSmartAttrSaveSet
** ��������: �������� SMART ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartAttrSaveSet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart auto save set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart auto save set state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_ATTR_SAVE, 0, 0, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart auto save set no data cmd send error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartAutoOffline
** ��������: ���� SMART �Զ�����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           ucSubCmd   ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartAutoOffline (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8  ucSubCmd)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart diag set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart auto offline  state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucSubCmd);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_AUTO_OFFLINE,
                                     ucSubCmd, 0, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart auto offline subcmd %d no data cmd send err.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucSubCmd);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartOfflineDiag
** ��������: ʹ�� SMART �ѻ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           ucSubCmd   ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartOfflineDiag (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8  ucSubCmd)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart diag set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart diag subcmd %d set state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucSubCmd);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_OFFLINE_DIAGS,
                                     0, ucSubCmd, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart diag set subcmd %d no data cmd send error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucSubCmd);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartEnableSet
** ��������: ʹ�� SMART �����ռ�
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartEnableSet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart enable set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart enable set state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_SMART_ENABLE, 0, 0, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart enable set no data cmd send error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartDisableSet
** ��������: ���� SMART �����ռ�
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartDisableSet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart disable set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart disable set state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_SMART_DISABLE, 0, 0, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart disable set no data cmd send error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartAttrAutoSaveSet
** ��������: ʹ���õ� SMART ����������Ч
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           uiValue    0x00        ���������Զ�����
**                      0x01 - 0xf0 �����Զ���
**                      0xf1        ʹ�������Զ�����
**                      0xf2 - 0xff �����Զ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartAttrAutoSaveSet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8  ucValue)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart attr set.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart attr set value 0x%02x state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucValue);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_AUTOSAVE_EN_DIS,
                                     ucValue, 0, 0x4f, 0xc2, 0);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart attr set value 0x%02x no data cmd send error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucValue);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartStatusGet
** ��������: ��ȡ SMART ״̬
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pucStatus  ��ȡ״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartStatusGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8 *pucStatus)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    UINT8                   ucLow;                                      /* ���ֽ�����                   */
    UINT8                   ucHigh;                                     /* ���ֽ�����                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart status get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart status get state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                     AHCI_CMD_SMART, AHCI_SMART_CMD_RETURN_STATUS, 0, 0, 0x4f, 0xc2, 0);
    if (iRet == ERROR_NONE) {
        ucLow  = hDrive->AHCIDRIVE_hRecvFis->AHCIRECVFIS_ucD2hRegisterFis[5];
        ucHigh = hDrive->AHCIDRIVE_hRecvFis->AHCIRECVFIS_ucD2hRegisterFis[6];
        if ((ucLow == 0x4f) && (ucHigh == 0xc2)) {
            if (pucStatus) {
                *pucStatus = AHCI_SMART_OK;
            }
        } else if ((ucLow == 0xc2) && (ucHigh == 0x4f)) {
            if (pucStatus) {
                *pucStatus = AHCI_SMART_EXCEEDED;
            }
        }
        return  (ERROR_NONE);
    }

    AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart status get no data cmd send error.\r\n",
             hCtrl->AHCICTRL_uiIndex, uiDrive);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_AhciSmartParamGet
** ��������: ��ȡ SMART ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           ucCmd      ����
**           uiCount    ����
**           uiLba      LBA
**           pvBuf      ���������� (��������С������ڵ��� 512 ���ֽ�)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartParamGet (AHCI_CTRL_HANDLE  hCtrl,
                            UINT              uiDrive,
                            UINT8             ucCmd,
                            UINT32            uiCount,
                            UINT32            uiLba,
                            PVOID             pvBuf)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_CMD_CB             tCtrlCmd;                                   /* ������ƿ�                   */
    AHCI_CMD_HANDLE         hCmd = &tCtrlCmd;                           /* ������                     */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart data get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) ||
        (uiCount > 1)) {
        AHCI_LOG(AHCI_LOG_ERR,
                 "ctrl %d drive %d ata smart data get cmd %d count %d lba 0x%08x state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive, ucCmd, uiCount, uiLba);
        return  (PX_ERROR);
    }

    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen  = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_SMART;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount   = uiCount;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = ucCmd;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba    = uiLba;
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
** ��������: API_AhciSmartLogSectorRead
** ��������: ��ȡ SMART Log
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           uiAddr     ָ����ַ
**           uiCount    �߼�ҳ����
**           pvBuf      ���ݻ����� (��������С������ڵ��� uiCount * 512 ���ֽ�)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartLogSectorRead (AHCI_CTRL_HANDLE  hCtrl,
                                 UINT              uiDrive,
                                 UINT32            uiAddr,
                                 UINT32            uiCount,
                                 PVOID             pvBuf)
{
    return  (API_AhciSmartParamGet(hCtrl, uiDrive, AHCI_SMART_CMD_LOG_SECTOR_READ,
                                   uiCount, 0xc24f00 | (uiAddr & 0xff), pvBuf));
}
/*********************************************************************************************************
** ��������: API_AhciSmartLogSectorWrite
** ��������: ���� SMART Log
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           hCmd       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartLogSectorWrite (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, AHCI_CMD_HANDLE  hCmd)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart log sector set.\r\n", hCtrl->AHCICTRL_uiIndex,uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) ||
        (!hCmd) ||
        (hCmd->AHCICMD_ulDataLen > hDrive->AHCIDRIVE_ulByteSector)) {
        AHCI_LOG(AHCI_LOG_ERR,
                 "ctrl %d drive %d ata smart log sector set"
                 " cmd %d count %d feature 0x%08x lba 0x%llx state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature,
                 hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba);
        return  (PX_ERROR);
    }

    lib_memcpy(hDrive->AHCIDRIVE_pucAlignDmaBuf, hCmd->AHCICMD_pucDataBuf, hCmd->AHCICMD_ulDataLen);
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;

    iRet = API_AhciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciSmartThresholdGet
** ��������: ��ȡ SMART ��ֵ
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pvBuf      ���ݻ����� (��������С������ڵ��� 512 ���ֽ�)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartThresholdGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, PVOID  pvBuf)
{
    return  (API_AhciSmartParamGet(hCtrl, uiDrive, AHCI_SMART_CMD_THRESHOLDS_READ, 0, 0xc24f00, pvBuf));
}
/*********************************************************************************************************
** ��������: API_AhciSmartDataGet
** ��������: ��ȡ SMART ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           pvBuf      ���ݻ����� (��������С������ڵ��� 512 ���ֽ�)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartDataGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, PVOID  pvBuf)
{
    return  (API_AhciSmartParamGet(hCtrl, uiDrive, AHCI_SMART_CMD_DATA_READ, 0, 0xc24f00, pvBuf));
}
/*********************************************************************************************************
** ��������: API_AhciSmartEnableGet
** ��������: ��ȡ SMART �Ƿ�Ϊʹ��
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           piEnable   �Ƿ�Ϊʹ��״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_AhciSmartEnableGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT *piEnable)
{
    INT                     iRet;                                       /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* �������                     */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d smart enable get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ������������               */
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart enable get state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    iRet = API_AhciDiskAtaParamGet(hCtrl, uiDrive, (PVOID)hParam);      /* ��ȡ����������               */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d smart enable read ata parameters failed.\r\n",
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

    if (piEnable) {
        *piEnable = (hParam->AHCIPARAM_usFeaturesEnabled0 & AHCI_SMART_SUPPORTED) ? LW_TRUE : LW_FALSE;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0) &&     */
                                                                        /*  (AHCI_SMART_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
