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
** ��   ��   ��: ahciDrv.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 29 ��
**
** ��        ��: AHCI ��������.
*********************************************************************************************************/

#ifndef __AHCI_DRV_H
#define __AHCI_DRV_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API AHCI_DRV_CTRL_HANDLE API_AhciDrvCtrlFind(AHCI_DRV_HANDLE  hDrvHandle,
                                                AHCI_CTRL_HANDLE hCtrlHandle);
LW_API INT                  API_AhciDrvCtrlDelete(AHCI_DRV_HANDLE  hDrvHandle,
                                                  AHCI_CTRL_HANDLE hCtrlHandle);
LW_API INT                  API_AhciDrvCtrlAdd(AHCI_DRV_HANDLE  hDrvHandle,
                                               AHCI_CTRL_HANDLE hCtrlHandle);
LW_API AHCI_DRV_HANDLE      API_AhciDrvHandleGet(CPCHAR  cpcName);
LW_API INT                  API_AhciDrvDelete(AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciDrvRegister(AHCI_DRV_HANDLE  hDrv);
LW_API INT                  API_AhciDrvInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_DRV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
