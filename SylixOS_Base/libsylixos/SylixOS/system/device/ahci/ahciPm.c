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
** 文   件   名: ahciPm.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 31 日
**
** 描        述: AHCI 设备电源管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciPm.h"
/*********************************************************************************************************
** 函数名称: API_AhciPmPowerModeGet
** 功能描述: 获取当前电源管理模式
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           pucMode    电源管理模式
** 输　出  : ERROR or AHCI_PM_ACTIVE_IDLE or AHCI_PM_STANDBY or AHCI_PM_SLEEP
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_AhciPmPowerModeGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8 *pucMode)
{
    INT                     iRet;                                       /* 操作结果                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* 驱动器句柄                   */
    UINT8                   ucPower;

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d power mode get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* 获得驱动器句柄               */
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
** 函数名称: API_AhciApmModeGet
** 功能描述: 获取当前电源管理模式级别
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           pucMode    是否为使能状态 (0x01 - 0xfe)
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_AhciApmModeGet (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, UINT8 *pucMode)
{
    INT                     iRet;                                       /* 操作结果                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* 驱动器句柄                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* 参数句柄                     */
    UINT16                  usCurrentAPM;

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d apm mode get.\r\n", hCtrl->AHCICTRL_uiIndex, uiDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* 获得驱动器句柄               */
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((!hCtrl->AHCICTRL_bInstalled) ||
        (!hCtrl->AHCICTRL_bDrvInstalled) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d apm mode get state error.\r\n",
                 hCtrl->AHCICTRL_uiIndex, uiDrive);
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesEnabled1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciDiskAtaParamGet(hCtrl, uiDrive, (PVOID)hParam);  /* 获取驱动器参数               */
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
** 函数名称: API_AhciApmDisable
** 功能描述: 禁能设备高级电源管理
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciApmDisable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                     iRet;                                       /* 操作结果                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* 驱动器句柄                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* 参数句柄                     */

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
** 函数名称: API_AhciApmEnable
** 功能描述: 使能设备高级电源管理
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           iApm       电源级别
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciApmEnable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iApm)
{
    INT                     iRet;                                       /* 操作结果                     */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* 驱动器句柄                   */
    AHCI_PARAM_HANDLE       hParam;                                     /* 参数句柄                     */

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
** 函数名称: API_AhciPmActive
** 功能描述: 设备电源是否使能
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
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
