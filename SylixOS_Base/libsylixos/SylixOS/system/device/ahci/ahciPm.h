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
** ��   ��   ��: ahciPm.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 31 ��
**
** ��        ��: AHCI ��Դ����.
*********************************************************************************************************/

#ifndef __AHCI_PM_H
#define __AHCI_PM_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API INT              API_AhciPmPowerModeGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 *pucMode);
LW_API INT              API_AhciApmModeGet(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, UINT8 *pucMode);
LW_API INT              API_AhciApmDisable(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);
LW_API INT              API_AhciApmEnable(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive, INT iApm);

LW_API INT              API_AhciPmActive(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_PM_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
