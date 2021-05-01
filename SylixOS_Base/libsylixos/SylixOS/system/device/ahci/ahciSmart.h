/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ahciSmart.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 31 日
**
** 描        述: AHCI 设备 S.M.A.R.T (Self-Monitoring, Analysis and Reporting Technology)
*********************************************************************************************************/

#ifndef __AHCI_SMART_H
#define __AHCI_SMART_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0) && (AHCI_SMART_EN > 0)

#ifndef LW_BLKD_SATA_CMD_CTL                                            /* 获取对应磁盘 SMART 信息命令  */
#define LW_BLKD_SATA_CMD_CTL         LW_OSIOR('b', 220, AHCI_CMD_CB)    /* 获取对应磁盘 SMART 信息      */
#endif                                                                  /* LW_BLKD_SATA_CMD_CTL         */

/*********************************************************************************************************
  函数声明
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
