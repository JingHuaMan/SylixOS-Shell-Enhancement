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
** ��   ��   ��: nvmeDrv.c
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ��������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "nvme.h"
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
#include "nvmeCtrl.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                     _GuiNvmeDrvTotalNum  = 0;
static LW_LIST_LINE_HEADER      _GplineNvmeDrvHeader = LW_NULL;
static LW_OBJECT_HANDLE         _GulNvmeDrvLock      = LW_OBJECT_HANDLE_INVALID;

#define __NVME_DRV_LOCK()       API_SemaphoreMPend(_GulNvmeDrvLock, LW_OPTION_WAIT_INFINITE)
#define __NVME_DRV_UNLOCK()     API_SemaphoreMPost(_GulNvmeDrvLock)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID     __tshellNvmeDrvCmdShow(VOID);
static INT      __tshellNvmeDrvCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** ��������: API_NvmeDrvHandleGet
** ��������: ��ȡһ�������ľ��
** �䡡��  : cpcName       ��������
** �䡡��  : �������ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_DRV_HANDLE  API_NvmeDrvHandleGet (CPCHAR  cpcName)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DRV_HANDLE     hDrv      = LW_NULL;

    hDrv = LW_NULL;
    __NVME_DRV_LOCK();
    for (plineTemp  = _GplineNvmeDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrv = _LIST_ENTRY(plineTemp, NVME_DRV_CB, NVMEDRV_lineDrvNode);
        if (lib_strncmp(&hDrv->NVMEDRV_cDrvName[0], cpcName, NVME_DRV_NAME_MAX) == 0) {
            break;
        }
    }
    __NVME_DRV_UNLOCK();

    if (plineTemp) {
        return  (hDrv);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_NvmeDrvRegister
** ��������: ע��ָ������ NVMe ����
** �䡡��  : hDrvReg       ע����ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeDrvRegister (NVME_DRV_HANDLE  hDrvReg)
{
    INT                 iRet;
    NVME_DRV_HANDLE     hDrv;

    if ((!hDrvReg) ||
        (_GulNvmeDrvLock == LW_OBJECT_HANDLE_INVALID)) {
        return  (PX_ERROR);
    }

    hDrv = API_NvmeDrvHandleGet(hDrvReg->NVMEDRV_cDrvName);
    if (hDrv != LW_NULL) {
        return  (PX_ERROR);
    }

    if (!hDrvReg->NVMEDRV_pfuncOptCtrl) {
        return  (PX_ERROR);
    }

    hDrv = (NVME_DRV_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_DRV_CB));
    if (!hDrv) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_strlcpy(&hDrv->NVMEDRV_cDrvName[0], &hDrvReg->NVMEDRV_cDrvName[0], NVME_DRV_NAME_MAX);

    __NVME_DRV_LOCK();
    _List_Line_Add_Ahead(&hDrv->NVMEDRV_lineDrvNode, &_GplineNvmeDrvHeader);
    _GuiNvmeDrvTotalNum += 1;
    __NVME_DRV_UNLOCK();

    hDrv->NVMEDRV_uiDrvVer                      = hDrvReg->NVMEDRV_uiDrvVer;
    hDrv->NVMEDRV_hCtrl                         = hDrvReg->NVMEDRV_hCtrl;
    hDrv->NVMEDRV_pfuncOptCtrl                  = hDrvReg->NVMEDRV_pfuncOptCtrl;
    hDrv->NVMEDRV_pfuncVendorCtrlIntEnable      = hDrvReg->NVMEDRV_pfuncVendorCtrlIntEnable;
    hDrv->NVMEDRV_pfuncVendorCtrlIntConnect     = hDrvReg->NVMEDRV_pfuncVendorCtrlIntConnect;
    hDrv->NVMEDRV_pfuncVendorCtrlIntDisConnect  = hDrvReg->NVMEDRV_pfuncVendorCtrlIntDisConnect;
    hDrv->NVMEDRV_pfuncVendorDrvReadyWork       = hDrvReg->NVMEDRV_pfuncVendorDrvReadyWork;
    hDrv->NVMEDRV_pfuncVendorCtrlReadyWork      = hDrvReg->NVMEDRV_pfuncVendorCtrlReadyWork;

    if (hDrv->NVMEDRV_pfuncVendorDrvReadyWork) {
        iRet = hDrv->NVMEDRV_pfuncVendorDrvReadyWork(hDrv);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        goto    __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    if (hDrv) {
        __NVME_DRV_LOCK();
        _List_Line_Del(&hDrv->NVMEDRV_lineDrvNode, &_GplineNvmeDrvHeader);
        _GuiNvmeDrvTotalNum -= 1;
        __NVME_DRV_UNLOCK();
        __SHEAP_FREE(hDrv);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_NvmeDrvInit
** ��������: NVMe ����ע���ʼ��, ʹ��ע��ӿ�ǰӦ�ȳ�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeDrvInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiNvmeDrvTotalNum  = 0;
    _GplineNvmeDrvHeader = LW_NULL;

    _GulNvmeDrvLock = API_SemaphoreMCreate("nvme_drvlock",
                                           LW_PRIO_DEF_CEILING,
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    if (_GulNvmeDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("nvmedrv", __tshellNvmeDrvCmd);
    API_TShellFormatAdd("nvmedrv", " [add | del] [driver] [name]");
    API_TShellHelpAdd("nvmedrv", "show, add, del nvme driver\n"
                                 "eg. nvmedrv\n"
                                 "    nvmedrv add imx6dq_nvme\n"
                                 "    nvmedrv del imx6dq_nvme\n");

    API_NvmeDevInit();
    API_NvmeCtrlInit();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNvmeDrvCmdShow
** ��������: ��ӡ NVMe �豸�����б�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellNvmeDrvCmdShow (VOID)
{
    static PCHAR        pcNvmeDrvShowHdr = \
    "INDEX     DRVNAME           CTRL VER             DRIVE VER\n"
    "----- --------------- -------------------- --------------------\n";
    REGISTER INT        i;
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DRV_HANDLE     hDrv      = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;
    CHAR                cVerNum[NVME_DRV_VER_STR_LEN]  = {0};

    printf("nvme drv number total: %d\n", _GuiNvmeDrvTotalNum);
    printf(pcNvmeDrvShowHdr);

    __NVME_DRV_LOCK();
    i = 0;
    for (plineTemp  = _GplineNvmeDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrv = _LIST_ENTRY(plineTemp, NVME_DRV_CB, NVMEDRV_lineDrvNode);
        hCtrl = hDrv->NVMEDRV_hCtrl;
        printf("%5d %-15s", i, hDrv->NVMEDRV_cDrvName);

        if ((hCtrl) && (hCtrl->NVMECTRL_uiCoreVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN, NVME_DRV_VER_FORMAT(hCtrl->NVMECTRL_uiCoreVer));
            printf(" %-20s", cVerNum);
        } else {
            printf(" %-20s", "*");
        }

        if ((hDrv) && (hDrv->NVMEDRV_uiDrvVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN, NVME_DRV_VER_FORMAT(hDrv->NVMEDRV_uiDrvVer));
            printf(" %-20s\n", cVerNum);
        } else {
            printf(" %-20s\n", "*");
        }

        i += 1;
    }
    __NVME_DRV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __tshellNvmeDrvCmd
** ��������: NVMe ���� "nvmedrv"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNvmeDrvCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellNvmeDrvCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
