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
** ��   ��   ��: ahciDev.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 29 ��
**
** ��        ��: AHCI �豸����.
*********************************************************************************************************/

#ifndef __AHCI_CTRL_H
#define __AHCI_CTRL_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API INT                  API_AhciCtrlDrvDel(AHCI_CTRL_HANDLE hCtrlHandle,
                                               AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciCtrlDrvDel(AHCI_CTRL_HANDLE hCtrlHandle,
                                               AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciCtrlDelete(AHCI_CTRL_HANDLE hCtrl);
LW_API INT                  API_AhciCtrlAdd(AHCI_CTRL_HANDLE hCtrl);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromPciArg(PVOID pvCtrlPciArg);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromName(CPCHAR cpcName, UINT uiUnit);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromIndex(UINT uiIndex);
LW_API INT                  API_AhciCtrlIndexGet(VOID);
LW_API UINT32               API_AhciCtrlCountGet(VOID);
LW_API INT                  API_AhciCtrlInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_CTRL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
