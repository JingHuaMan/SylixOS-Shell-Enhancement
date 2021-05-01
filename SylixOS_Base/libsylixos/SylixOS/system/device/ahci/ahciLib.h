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
** ��   ��   ��: ahciLib.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 01 �� 14 ��
**
** ��        ��: AHCI ������.
*********************************************************************************************************/

#ifndef __AHCI_LIB_H
#define __AHCI_LIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

#include "ahci.h"

/*********************************************************************************************************
  ����ģʽ
*********************************************************************************************************/
#define AHCI_LOG_RUN                        __LOGMESSAGE_LEVEL
#define AHCI_LOG_PRT                        __LOGMESSAGE_LEVEL
#define AHCI_LOG_ERR                        __ERRORMESSAGE_LEVEL
#define AHCI_LOG_BUG                        __BUGMESSAGE_LEVEL
#define AHCI_LOG_ALL                        __PRINTMESSAGE_LEVEL

#define AHCI_LOG                            _DebugFormat
#define AHCI_INT_LOG                        _DebugFormat
#define AHCI_CMD_LOG                        _DebugFormat
#define AHCI_ATAPI_LOG                      _DebugFormat
#define AHCI_ATAPI_INT_LOG                  _DebugFormat
#define AHCI_ATAPI_CMD_LOG                  _DebugFormat
/*********************************************************************************************************
  �����Ϣ
*********************************************************************************************************/
#define AHCI_FLAG(x, y)                     ((x & y) ? '+' : '-')
/*********************************************************************************************************
  �Ĵ�����Ϣ
*********************************************************************************************************/
#if AHCI_LOG_EN > 0                                                     /* ʹ�ܵ���                     */
#define AHCI_CTRL_REG_MSG(ctrl, reg)        AHCI_LOG(AHCI_LOG_PRT, "ctrl %d %s: 0x%08x.\r\n",   \
                                            ctrl->AHCICTRL_uiIndex,                         \
                                            API_AhciCtrlRegNameGet(ctrl, reg), AHCI_CTRL_READ(ctrl, reg))
#define AHCI_PORT_REG_MSG(port, reg)        AHCI_LOG(AHCI_LOG_PRT, "port %d %s: 0x%08x.\r\n",   \
                                            port->AHCIDRIVE_uiPort,                         \
                                            API_AhciDriveRegNameGet(port, reg), AHCI_PORT_READ(port, reg))
#else                                                                   /* AHCI_LOG_EN                  */
#define AHCI_CTRL_REG_MSG(ctrl, reg)
#define AHCI_PORT_REG_MSG(port, reg)
#endif                                                                  /* AHCI_LOG_EN                  */
/*********************************************************************************************************
  ��ַת��
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 32
#define AHCI_ADDR_LOW32(x)                  ((UINT32)(x))
#define AHCI_ADDR_HIGH32(x)                 0
#else
#define AHCI_ADDR_LOW32(x)                  ((UINT32)(ULONG)(x))
#define AHCI_ADDR_HIGH32(x)                 ((UINT32)((UINT64)(x) >> 32))
#endif
/*********************************************************************************************************
  �汾�ַ�����ʽ
*********************************************************************************************************/
#define AHCI_DRV_VER_STR_LEN                20
#define AHCI_DRV_VER_FORMAT(ver)            "%d.%d.%d-rc%d", (ver >> 24) & 0xff, (ver >> 16) & 0xff,    \
                                             (ver >> 8) & 0xff, ver & 0xff
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
LW_API PCHAR                API_AhciDriveSerialInfoGet(AHCI_DRIVE_HANDLE hDrive,
                                                       PCHAR pcBuf, size_t stLen);
LW_API PCHAR                API_AhciDriveFwRevInfoGet(AHCI_DRIVE_HANDLE hDrive,
                                                      PCHAR pcBuf, size_t stLen);
LW_API PCHAR                API_AhciDriveModelInfoGet(AHCI_DRIVE_HANDLE hDrive,
                                                      PCHAR pcBuf, size_t stLen);

LW_API UINT64               API_AhciDriveSectorCountGet(AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive);
LW_API PCHAR                API_AhciDriveWorkModeNameGet(UINT  uiIndex);
LW_API INT                  API_AhciDriveInfoShow(AHCI_CTRL_HANDLE   hCtrl,
                                                  UINT               uiDrive,
                                                  AHCI_PARAM_HANDLE  hParam);

LW_API INT                  API_AhciDriveRegWait(AHCI_DRIVE_HANDLE  hDrive,
                                                 UINT32             uiRegAddr,
                                                 UINT32             uiMask,
                                                 INT                iFlag,
                                                 UINT32             uiValue,
                                                 ULONG              ulInterTime,
                                                 ULONG              ulTimeout,
                                                 UINT32            *puiReg);
LW_API INT                  API_AhciDriveRecvFisStop(AHCI_DRIVE_HANDLE  hDrive);
LW_API INT                  API_AhciDriveEngineStop(AHCI_DRIVE_HANDLE  hDrive);
LW_API INT                  API_AhciDrivePowerUp(AHCI_DRIVE_HANDLE  hDrive);
LW_API PCHAR                API_AhciDriveRegNameGet(AHCI_DRIVE_HANDLE  hDrive, UINT  uiOffset);

LW_API INT                  API_AhciCtrlIntConnect(AHCI_CTRL_HANDLE  hCtrl,
                                                   PINT_SVR_ROUTINE  pfuncIsr, CPCHAR cpcName);
LW_API INT                  API_AhciCtrlReset(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlAhciModeEnable(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlSssSet(AHCI_CTRL_HANDLE  hCtrl, INT  iSet);

LW_API INT                  API_AhciCtrlInfoShow(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlImpPortGet(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlCapGet(AHCI_CTRL_HANDLE  hCtrl);
LW_API PCHAR                API_AhciCtrlRegNameGet(AHCI_CTRL_HANDLE  hCtrl, UINT  uiOffset);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_PORT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
