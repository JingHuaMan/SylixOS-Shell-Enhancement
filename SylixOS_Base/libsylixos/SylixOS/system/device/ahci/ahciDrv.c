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
** ��   ��   ��: ahciDrv.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 29 ��
**
** ��        ��: AHCI ��������.
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
#include "ahciCtrl.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                     _GuiAhciDrvTotalNum  = 0;
static UINT                     _GuiAhciDrvActiveNum = 0;
static LW_LIST_LINE_HEADER      _GplineAhciDrvHeader = LW_NULL;
static LW_OBJECT_HANDLE         _GulAhciDrvLock      = LW_OBJECT_HANDLE_INVALID;

#define __AHCI_DRV_LOCK()       API_SemaphoreMPend(_GulAhciDrvLock, LW_OPTION_WAIT_INFINITE)
#define __AHCI_DRV_UNLOCK()     API_SemaphoreMPost(_GulAhciDrvLock)
/*********************************************************************************************************
** ��������: __tshellAhciDrvCmdShow
** ��������: ��ӡ AHCI �豸�����б�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellAhciDrvCmdShow (VOID)
{
    static PCHAR        pcAhciDrvShowHdr = \
    " INDEX          DRVNAME                DRIVE VER      INDEX  CI         CTRL VER        TOTAL\n"
    "------- ------------------------ -------------------- ----- ----- -------------------- -------\n";

    PLW_LIST_LINE           plineDrvTemp   = LW_NULL;
    PLW_LIST_LINE           plineCtrlTemp  = LW_NULL;
    AHCI_CTRL_HANDLE        hCtrlHandle    = LW_NULL;
    AHCI_DRV_HANDLE         hDrvHandle     = LW_NULL;
    AHCI_DRV_CTRL_HANDLE    hDrvCtrlHandle = LW_NULL;
    CHAR                    cVerNum[AHCI_DRV_VER_STR_LEN] = {0};
    REGISTER INT            i;
    REGISTER INT            j;

    printf("ahci device driver total: %d active: %d.\n", _GuiAhciDrvTotalNum, _GuiAhciDrvActiveNum);

    printf(pcAhciDrvShowHdr);

    __AHCI_DRV_LOCK();                                                  /*  ���� AHCI ����              */
    i = 0;
    for (plineDrvTemp  = _GplineAhciDrvHeader;
         plineDrvTemp != LW_NULL;
         plineDrvTemp  = _list_line_get_next(plineDrvTemp)) {
        hDrvHandle = _LIST_ENTRY(plineDrvTemp, AHCI_DRV_CB, AHCIDRV_lineDrvNode);
        printf("%7d %-24s ", i, hDrvHandle->AHCIDRV_cDrvName);

        lib_bzero(&cVerNum[0], AHCI_DRV_VER_STR_LEN);
        if ((hDrvHandle) && (hDrvHandle->AHCIDRV_uiDrvVer)) {
            snprintf(cVerNum, AHCI_DRV_VER_STR_LEN, AHCI_DRV_VER_FORMAT(hDrvHandle->AHCIDRV_uiDrvVer));
        } else {
            snprintf(cVerNum, AHCI_DRV_VER_STR_LEN, "%s", "*");
        }
        printf("%-20s %-5s %-5s %-20s %7d\n", cVerNum, "", "", "", hDrvHandle->AHCIDRV_uiDrvCtrlNum);

        j = 0;
        for (plineCtrlTemp  = hDrvHandle->AHCIDRV_plineDrvCtrlHeader;
             plineCtrlTemp != LW_NULL;
             plineCtrlTemp  = _list_line_get_next(plineCtrlTemp)) {
            hDrvCtrlHandle = _LIST_ENTRY(plineCtrlTemp, AHCI_DRV_CTRL_CB, AHCIDCB_lineDrvCtrlNode);
            hCtrlHandle = hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle;
            printf("%-7s %-24s %-20s %5d ", "", "", "", j);

            if ((hCtrlHandle) && (hCtrlHandle->AHCICTRL_uiCoreVer)) {
                lib_bzero(&cVerNum[0], AHCI_DRV_VER_STR_LEN);
                snprintf(cVerNum, AHCI_DRV_VER_STR_LEN,
                         AHCI_DRV_VER_FORMAT(hCtrlHandle->AHCICTRL_uiCoreVer));
            } else {
                snprintf(cVerNum, AHCI_DRV_VER_STR_LEN, "%s", "*");
            }
            printf("%5d %-20s\n", hCtrlHandle->AHCICTRL_uiIndex, cVerNum);

            j += 1;
        }

        i += 1;
    }
    __AHCI_DRV_UNLOCK();                                                /*  ���� AHCI ����              */
}
/*********************************************************************************************************
** ��������: __tshellAhciDrvCmd
** ��������: AHCI ���� "ahcidrv"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellAhciDrvCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellAhciDrvCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDrvCtrlFind
** ��������: ��ѯһ�� AHCI �����ϵ�ָ��������
** �䡡��  : hDrvHandle     �������
**           hCtrlHandle    ���������
** �䡡��  : �����Ŀ��������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_DRV_CTRL_HANDLE  API_AhciDrvCtrlFind (AHCI_DRV_HANDLE  hDrvHandle, AHCI_CTRL_HANDLE  hCtrlHandle)
{
    PLW_LIST_LINE           plineTemp = LW_NULL;
    AHCI_DRV_CTRL_HANDLE    hDrvCtrlHandle;

    hDrvCtrlHandle = LW_NULL;
    __AHCI_DRV_LOCK();                                                  /*  ���� AHCI ����              */
    for (plineTemp  = hDrvHandle->AHCIDRV_plineDrvCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrvCtrlHandle = _LIST_ENTRY(plineTemp, AHCI_DRV_CTRL_CB, AHCIDCB_lineDrvCtrlNode);
        if (hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle == hCtrlHandle) {
            break;
        }
    }
    __AHCI_DRV_UNLOCK();                                                /*  ���� AHCI ����              */

    if (plineTemp) {
        return  (hDrvCtrlHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_AhciDrvCtrlDelete
** ��������: ָ�� AHCI ����ɾ��һ��������
** �䡡��  : hDrvHandle     �������
**           hCtrlHandle    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrvCtrlDelete (AHCI_DRV_HANDLE  hDrvHandle, AHCI_CTRL_HANDLE  hCtrlHandle)
{
    PLW_LIST_LINE           plineTemp      = LW_NULL;
    AHCI_DRV_CTRL_HANDLE    hDrvCtrlHandle = LW_NULL;

    if (hDrvHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (hCtrlHandle == LW_NULL) {
        __AHCI_DRV_UNLOCK();
        plineTemp = hDrvHandle->AHCIDRV_plineDrvCtrlHeader;
        while (plineTemp) {
            hDrvCtrlHandle = _LIST_ENTRY(plineTemp, AHCI_DRV_CTRL_CB, AHCIDCB_lineDrvCtrlNode);
            plineTemp      = _list_line_get_next(plineTemp);

            _List_Line_Del(&hDrvCtrlHandle->AHCIDCB_lineDrvCtrlNode,
                           &hDrvHandle->AHCIDRV_plineDrvCtrlHeader);
            hDrvHandle->AHCIDRV_uiDrvCtrlNum--;
            if (hDrvHandle->AHCIDRV_uiDrvCtrlNum < 1) {
                hDrvHandle->AHCIDRV_iDrvFlag &= ~(AHCI_DRV_FLAG_ACTIVE);
                hDrvHandle->AHCIDRV_plineDrvCtrlHeader = LW_NULL;
            }

            API_AhciCtrlDrvDel(hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle, hDrvHandle);
            if (hDrvHandle->AHCIDRV_pfuncCtrlRemove) {
               hDrvHandle->AHCIDRV_pfuncCtrlRemove(hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle);
            }
            hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle = LW_NULL;
            __SHEAP_FREE(hDrvCtrlHandle);
        }
        __AHCI_DRV_UNLOCK();

        API_AhciDrvDelete(hDrvHandle);

        return  (ERROR_NONE);
    }

    hDrvCtrlHandle = API_AhciDrvCtrlFind(hDrvHandle, hCtrlHandle);
    if (hDrvCtrlHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    __AHCI_DRV_LOCK();                                                  /*  ���� AHCI ����              */
    _List_Line_Del(&hDrvCtrlHandle->AHCIDCB_lineDrvCtrlNode, &hDrvHandle->AHCIDRV_plineDrvCtrlHeader);
    hDrvHandle->AHCIDRV_uiDrvCtrlNum--;
    if (hDrvHandle->AHCIDRV_uiDrvCtrlNum < 1) {
        hDrvHandle->AHCIDRV_iDrvFlag &= ~(AHCI_DRV_FLAG_ACTIVE);
        hDrvHandle->AHCIDRV_plineDrvCtrlHeader = LW_NULL;
    }

    API_AhciCtrlDrvDel(hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle, hDrvHandle);
    if (hDrvHandle->AHCIDRV_pfuncCtrlRemove) {
        hDrvHandle->AHCIDRV_pfuncCtrlRemove(hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle);
    }
    hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle = LW_NULL;
    __AHCI_DRV_UNLOCK();                                                /*  ���� AHCI ����              */

    __SHEAP_FREE(hDrvCtrlHandle);

    API_AhciDrvDelete(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDrvCtrlAdd
** ��������: ָ�� AHCI ��������һ��������
** �䡡��  : hDrvHandle     �������
**           hCtrlHandle    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrvCtrlAdd (AHCI_DRV_HANDLE  hDrvHandle, AHCI_CTRL_HANDLE  hCtrlHandle)
{
    AHCI_DRV_CTRL_HANDLE        hDrvCtrlHandle;

    if ((hDrvHandle == LW_NULL) ||
        (hCtrlHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    hDrvCtrlHandle = API_AhciDrvCtrlFind(hDrvHandle, hCtrlHandle);
    if (hDrvCtrlHandle) {
        return  (ERROR_NONE);
    }

    hDrvCtrlHandle = (AHCI_DRV_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DRV_CTRL_CB));
    if (!hDrvCtrlHandle) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    __AHCI_DRV_LOCK();                                                  /*  ���� AHCI ����              */
    hDrvCtrlHandle->AHCIDCB_hDrvCtrlHandle = hCtrlHandle;
    _List_Line_Add_Ahead(&hDrvCtrlHandle->AHCIDCB_lineDrvCtrlNode,
                         &hDrvHandle->AHCIDRV_plineDrvCtrlHeader);
    hDrvHandle->AHCIDRV_uiDrvCtrlNum++;
    if (!(hDrvHandle->AHCIDRV_iDrvFlag & AHCI_DRV_FLAG_ACTIVE)) {
        hDrvHandle->AHCIDRV_iDrvFlag |= AHCI_DRV_FLAG_ACTIVE;
        _GuiAhciDrvActiveNum++;
    }
    __AHCI_DRV_UNLOCK();                                                /*  ���� AHCI ����              */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDrvHandleGet
** ��������: ��ȡһ�������ľ��
** �䡡��  : cpcName       ��������
** �䡡��  : �������ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_DRV_HANDLE  API_AhciDrvHandleGet (CPCHAR  cpcName)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_DRV_HANDLE     hDrv = LW_NULL;

    hDrv = LW_NULL;
    __AHCI_DRV_LOCK();
    for (plineTemp  = _GplineAhciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrv = _LIST_ENTRY(plineTemp, AHCI_DRV_CB, AHCIDRV_lineDrvNode);
        if (lib_strncmp(&hDrv->AHCIDRV_cDrvName[0], cpcName, AHCI_DRV_NAME_MAX) == 0) {
            break;
        }
    }
    __AHCI_DRV_UNLOCK();

    if (plineTemp) {
        return  (hDrv);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_AhciDrvDelete
** ��������: ɾ��һ�� AHCI ����
** �䡡��  : hDrvHandle     �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrvDelete (AHCI_DRV_HANDLE  hDrvHandle)
{
    if (!hDrvHandle) {
        return  (PX_ERROR);
    }

    if ((hDrvHandle->AHCIDRV_iDrvFlag & AHCI_DRV_FLAG_ACTIVE) ||
        (hDrvHandle->AHCIDRV_uiDrvCtrlNum > 0)) {
        return  (PX_ERROR);
    }

    __AHCI_DRV_LOCK();                                                  /*  ���� AHCI ����              */
    if (!(hDrvHandle->AHCIDRV_iDrvFlag & AHCI_DRV_FLAG_ACTIVE)) {
        _GuiAhciDrvActiveNum--;
    }
    _List_Line_Del(&hDrvHandle->AHCIDRV_lineDrvNode, &_GplineAhciDrvHeader);
    _GuiAhciDrvTotalNum--;
    __AHCI_DRV_UNLOCK();                                                /*  ���� AHCI ����              */

    __SHEAP_FREE(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDrvRegister
** ��������: ע��ָ������ AHCI ����
** �䡡��  : hDrvReg       ע����ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrvRegister (AHCI_DRV_HANDLE  hDrvReg)
{
    INT                 iRet;
    AHCI_DRV_HANDLE     hDrv;

    if ((!hDrvReg) ||
        (_GulAhciDrvLock == LW_OBJECT_HANDLE_INVALID)) {
        return  (PX_ERROR);
    }

    hDrv = API_AhciDrvHandleGet(hDrvReg->AHCIDRV_cDrvName);
    if (hDrv != LW_NULL) {
        return  (PX_ERROR);
    }

    if (!hDrvReg->AHCIDRV_pfuncOptCtrl) {
        return  (PX_ERROR);
    }

    hDrv = (AHCI_DRV_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DRV_CB));
    if (!hDrv) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_bzero(hDrv, sizeof(AHCI_DRV_CB));
    lib_strlcpy(&hDrv->AHCIDRV_cDrvName[0], &hDrvReg->AHCIDRV_cDrvName[0], AHCI_DRV_NAME_MAX);

    hDrv->AHCIDRV_uiDrvVer                   = hDrvReg->AHCIDRV_uiDrvVer;
    hDrv->AHCIDRV_hCtrl                      = hDrvReg->AHCIDRV_hCtrl;
    hDrv->AHCIDRV_pfuncOptCtrl               = hDrvReg->AHCIDRV_pfuncOptCtrl;
    hDrv->AHCIDRV_pfuncVendorDriveInfoShow   = hDrvReg->AHCIDRV_pfuncVendorDriveInfoShow;
    hDrv->AHCIDRV_pfuncVendorDriveRegNameGet = hDrvReg->AHCIDRV_pfuncVendorDriveRegNameGet;
    hDrv->AHCIDRV_pfuncVendorDriveInit       = hDrvReg->AHCIDRV_pfuncVendorDriveInit;
    hDrv->AHCIDRV_pfuncVendorCtrlInfoShow    = hDrvReg->AHCIDRV_pfuncVendorCtrlInfoShow;
    hDrv->AHCIDRV_pfuncVendorCtrlRegNameGet  = hDrvReg->AHCIDRV_pfuncVendorCtrlRegNameGet;
    hDrv->AHCIDRV_pfuncVendorCtrlTypeNameGet = hDrvReg->AHCIDRV_pfuncVendorCtrlTypeNameGet;
    hDrv->AHCIDRV_pfuncVendorCtrlIntEnable   = hDrvReg->AHCIDRV_pfuncVendorCtrlIntEnable;
    hDrv->AHCIDRV_pfuncVendorCtrlIntConnect  = hDrvReg->AHCIDRV_pfuncVendorCtrlIntConnect;
    hDrv->AHCIDRV_pfuncVendorCtrlInit        = hDrvReg->AHCIDRV_pfuncVendorCtrlInit;
    hDrv->AHCIDRV_pfuncVendorCtrlReadyWork   = hDrvReg->AHCIDRV_pfuncVendorCtrlReadyWork;
    hDrv->AHCIDRV_pfuncVendorPlatformInit    = hDrvReg->AHCIDRV_pfuncVendorPlatformInit;
    hDrv->AHCIDRV_pfuncVendorDrvReadyWork    = hDrvReg->AHCIDRV_pfuncVendorDrvReadyWork;

    hDrv->AHCIDRV_pfuncCtrlProbe             = hDrvReg->AHCIDRV_pfuncCtrlProbe;
    hDrv->AHCIDRV_pfuncCtrlRemove            = hDrvReg->AHCIDRV_pfuncCtrlRemove;
    hDrv->AHCIDRV_iDrvFlag                  &= ~(AHCI_DRV_FLAG_ACTIVE);
    hDrv->AHCIDRV_uiDrvCtrlNum               = 0;
    hDrv->AHCIDRV_plineDrvCtrlHeader         = LW_NULL;

    __AHCI_DRV_LOCK();
    _List_Line_Add_Ahead(&hDrv->AHCIDRV_lineDrvNode, &_GplineAhciDrvHeader);
    _GuiAhciDrvTotalNum++;
    if (hDrv->AHCIDRV_iDrvFlag & AHCI_DRV_FLAG_ACTIVE) {
        _GuiAhciDrvActiveNum++;
    }
    __AHCI_DRV_UNLOCK();

    if (hDrv->AHCIDRV_pfuncVendorDrvReadyWork) {
        iRet = hDrv->AHCIDRV_pfuncVendorDrvReadyWork(hDrv);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:

    if (hDrv) {
        __AHCI_DRV_LOCK();
        _List_Line_Del(&hDrv->AHCIDRV_lineDrvNode, &_GplineAhciDrvHeader);
        _GuiAhciDrvTotalNum--;
        if (hDrv->AHCIDRV_iDrvFlag & AHCI_DRV_FLAG_ACTIVE) {
            _GuiAhciDrvActiveNum--;
        }
        __AHCI_DRV_UNLOCK();
        __SHEAP_FREE(hDrv);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_AhciDrvInit
** ��������: AHCI ����ע���ʼ��, ʹ��ע��ӿ�ǰӦ�ȳ�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDrvInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiAhciDrvTotalNum  = 0;
    _GuiAhciDrvActiveNum = 0;
    _GplineAhciDrvHeader = LW_NULL;

    _GulAhciDrvLock = API_SemaphoreMCreate("ahci_drvlock",
                                           LW_PRIO_DEF_CEILING,
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    if (_GulAhciDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("ahcidrv", __tshellAhciDrvCmd);
    API_TShellFormatAdd("ahcidrv", " [add | del] [driver] [name]");
    API_TShellHelpAdd("ahcidrv", "show, add, del ahci driver\n"
                                 "eg. ahcidrv\n"
                                 "    ahcidrv add imx6dq_ahci\n"
                                 "    ahcidrv del imx6dq_ahci\n");

    API_AhciDevInit();
    API_AhciCtrlInit();

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
