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
** ��   ��   ��: ahciSmart.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 31 ��
**
** ��        ��: AHCI �豸 S.M.A.R.T (Self-Monitoring, Analysis and Reporting Technology)
*********************************************************************************************************/

#ifndef __AHCI_SMART_H
#define __AHCI_SMART_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0) && (AHCI_SMART_EN > 0)

#ifndef LW_BLKD_SATA_CMD_CTL                                            /* ��ȡ��Ӧ���� SMART ��Ϣ����  */
#define LW_BLKD_SATA_CMD_CTL         LW_OSIOR('b', 220, AHCI_CMD_CB)    /* ��ȡ��Ӧ���� SMART ��Ϣ      */
#endif                                                                  /* LW_BLKD_SATA_CMD_CTL         */

/*********************************************************************************************************
  ��������
*********************************************************************************************************/
LW_API INT              API_AhciSmartAttrSaveSet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
LW_API INT              API_AhciSmartAutoOffline(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 ucSubCmd);
LW_API INT              API_AhciSmartOfflineDiag(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 ucSubCmd);
LW_API INT              API_AhciSmartEnableSet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
LW_API INT              API_AhciSmartDisableSet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
LW_API INT              API_AhciSmartAttrAutoSaveSet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 ucValue);
LW_API INT              API_AhciSmartStatusGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 *pucStatus);
LW_API INT              API_AhciSmartParamGet(AHCI_CTRL_HANDLE hCtrl,
                                              UINT             uiDrive,
                                              UINT8            ucCmd,
                                              UINT32           uiCount,
                                              UINT32           uiLba,
                                              PVOID            pvBuf);
LW_API INT              API_AhciSmartLogSectorRead(AHCI_CTRL_HANDLE hCtrl,
                                                   UINT             uiDrive,
                                                   UINT32           uiAddr,
                                                   UINT32           uiCount,
                                                   PVOID            pvBuf);
LW_API INT              API_AhciSmartLogSectorWrite(AHCI_CTRL_HANDLE hCtrl,
                                                    UINT             uiDrive,
                                                    AHCI_CMD_HANDLE  hCmd);
LW_API INT              API_AhciSmartThresholdGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, PVOID pvBuf);
LW_API INT              API_AhciSmartDataGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, PVOID pvBuf);
LW_API INT              API_AhciSmartEnableGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, INT *piEnable);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_SMART_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
