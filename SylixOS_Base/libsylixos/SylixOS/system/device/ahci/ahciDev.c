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
** ��   ��   ��: ahciDev.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 29 ��
**
** ��        ��: AHCI �豸����.
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
static UINT32                   _GuiAhciDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulAhciDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineAhciDevHeader = LW_NULL;

#define __AHCI_DEV_LOCK()       API_SemaphoreMPend(_GulAhciDevLock, LW_OPTION_WAIT_INFINITE)
#define __AHCI_DEV_UNLOCK()     API_SemaphoreMPost(_GulAhciDevLock)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID     __tshellAhciDevCmdShow(AHCI_DEV_HANDLE  hDev);
static INT      __tshellAhciDevCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** ��������: API_AhciDevIoctl
** ��������: ����һ�� AHCI �豸
** �䡡��  : hDev      �豸���ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDevIoctl (AHCI_DEV_HANDLE  hDev, INT  iCmd, LONG  lArg)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */

    if (!hDev) {
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }
    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hDrive->AHCIDRIVE_ucType != AHCI_TYPE_ATA)) {                  /* ������δ��װ�����ʹ���       */
        return  (PX_ERROR);                                             /* ���󷵻�                     */
    }

    __AHCI_DEV_LOCK();
    switch (iCmd) {

    case AHCI_DEV_IOCTL_CACHE_FLUSH_GET:
        if (lArg) {
            *(INT *)lArg = hDev->AHCIDEV_iCacheFlush;
        }
        break;

    case AHCI_DEV_IOCTL_CACHE_FLUSH_SET:
        hDev->AHCIDEV_iCacheFlush = (INT)lArg;
        break;

    default:
        __AHCI_DEV_UNLOCK();
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    __AHCI_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDevDelete
** ��������: ɾ��һ�� AHCI �豸
** �䡡��  : hDev      �豸���ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDevDelete (AHCI_DEV_HANDLE  hDev)
{
    AHCI_DEV_HANDLE     hDevAhci = LW_NULL;

    hDevAhci = API_AhciDevHandleGet(hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
    if ((!hDevAhci) ||
        (hDevAhci != hDev)) {
        return  (PX_ERROR);
    }

    __AHCI_DEV_LOCK();
    _List_Line_Del(&hDev->AHCIDEV_lineDevNode, &_GplineAhciDevHeader);
    _GuiAhciDevTotalNum--;
    __AHCI_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciDevAdd
** ��������: ����һ���豸
** �䡡��  : hCtrl      ���������
**           uiDrive    ���������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_DEV_HANDLE  API_AhciDevAdd (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;
    AHCI_DEV_HANDLE     hDev   = LW_NULL;

    if (uiDrive >= hCtrl->AHCICTRL_uiImpPortNum) {
        return  (LW_NULL);
    }

    hDev = API_AhciDevHandleGet(hCtrl->AHCICTRL_uiIndex, uiDrive);
    if (hDev != LW_NULL) {
        return  (hDev);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hDev = hDrive->AHCIDRIVE_hDev;
    __AHCI_DEV_LOCK();
    _List_Line_Add_Ahead(&hDev->AHCIDEV_lineDevNode, &_GplineAhciDevHeader);
    _GuiAhciDevTotalNum++;
    __AHCI_DEV_UNLOCK();

    return  (hDev);
}
/*********************************************************************************************************
** ��������: API_AhciDevHandleGet
** ��������: ��ȡһ���豸�ľ��
** �䡡��  : uiCtrl     ����������
**           uiDrive    ����������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_DEV_HANDLE  API_AhciDevHandleGet (UINT  uiCtrl, UINT  uiDrive)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_DEV_HANDLE     hDev      = LW_NULL;

    hDev = LW_NULL;
    __AHCI_DEV_LOCK();
    for (plineTemp  = _GplineAhciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, AHCI_DEV_CB, AHCIDEV_lineDevNode);
        if ((hDev->AHCIDEV_uiCtrl  == uiCtrl ) &&
            (hDev->AHCIDEV_uiDrive == uiDrive)) {
            break;
        }
    }
    __AHCI_DEV_UNLOCK();

    if (plineTemp) {
        return  (hDev);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_AhciDevCountGet
** ��������: ��ȡ AHCI �豸����
** �䡡��  : NONE
** �䡡��  : AHCI �豸����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
UINT32  API_AhciDevCountGet (VOID)
{
    UINT32      uiCount = 0;

    __AHCI_DEV_LOCK();
    uiCount = _GuiAhciDevTotalNum;
    __AHCI_DEV_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** ��������: API_AhciDevInit
** ��������: AHCI �豸�����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDevInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiAhciDevTotalNum  = 0;
    _GplineAhciDevHeader = LW_NULL;

    _GulAhciDevLock = API_SemaphoreMCreate("ahci_devlock",
                                           LW_PRIO_DEF_CEILING,
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    if (_GulAhciDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("ahcidev", __tshellAhciDevCmd);
    API_TShellFormatAdd("ahcidev", " [[-c] 1 ...]");
    API_TShellHelpAdd("ahcidev", "show, add, del, set ahci device\n"
                                 "eg. ahcidev\n"
                                 "    ahcidev 0 0 -c 1\n"
                                 "    ahcidev 0 0 -c 0\n");
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellAhciDevCmdShow
** ��������: ��ӡ AHCI �豸�б�
** �䡡��  : hDev       �豸���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellAhciDevCmdShow (AHCI_DEV_HANDLE  hDev)
{
    static PCHAR        pcAhciDevShowHdr = \
    "INDEX CTRL DRIVER  START    SECTOR COUNT       DRVNAME         DEVNAME         BLKNAME\n"
    "----- ---- ------ ------- ---------------- --------------- --------------- ---------------\n";

    REGISTER INT        i;
    PLW_BLK_DEV         hBlkDev = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;

    i = 0;
    if (hDev) {
        hBlkDev = &hDev->AHCIDEV_tBlkDev;
        printf("\n");
        printf(pcAhciDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->AHCIDEV_uiCtrl,
               hDev->AHCIDEV_uiDrive,
               (UINT32)hDev->AHCIDEV_ulBlkOffset,
               (UINT32)hDev->AHCIDEV_ulBlkCount,
               hDev->AHCIDEV_hCtrl->AHCICTRL_hDrv->AHCIDRV_cDrvName,
               hDev->AHCIDEV_hDrive->AHCIDRIVE_cDevName,
               hBlkDev->BLKD_pcName);

        API_AhciDriveInfoShow(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive,
                              &hDev->AHCIDEV_hDrive->AHCIDRIVE_tParam);
        return;
    }

    printf("\nahci dev number total: %d\n", _GuiAhciDevTotalNum);

    __AHCI_DEV_LOCK();
    i = 0;
    for (plineTemp  = _GplineAhciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, AHCI_DEV_CB, AHCIDEV_lineDevNode);
        __AHCI_DEV_UNLOCK();
        hBlkDev = &hDev->AHCIDEV_tBlkDev;
        printf("\n");
        printf(pcAhciDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->AHCIDEV_uiCtrl,
               hDev->AHCIDEV_uiDrive,
               (UINT32)hDev->AHCIDEV_ulBlkOffset,
               (UINT32)hDev->AHCIDEV_ulBlkCount,
               hDev->AHCIDEV_hCtrl->AHCICTRL_hDrv->AHCIDRV_cDrvName,
               hDev->AHCIDEV_hDrive->AHCIDRIVE_cDevName,
               hBlkDev->BLKD_pcName);

        API_AhciDriveInfoShow(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive,
                              &hDev->AHCIDEV_hDrive->AHCIDRIVE_tParam);
        __AHCI_DEV_LOCK();
        i += 1;
    }
    __AHCI_DEV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __tshellAhciDevCmd
** ��������: AHCI ���� "ahcidev"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellAhciDevCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                 iRet;                                           /* ִ�н��                     */
    AHCI_DEV_HANDLE     hAhciDev;                                       /* �豸���ƾ��                 */
    UINT                uiCtrl;                                         /* ����������                   */
    UINT                uiDrive;                                        /* ��������                     */
    INT                 iCacheFlush;                                    /* CACHE ��д����               */

    if (iArgC == 1) {
        __tshellAhciDevCmdShow(LW_NULL);
        return  (ERROR_NONE);
    }

    if ((iArgC < 3) ||
        (sscanf(ppcArgV[1], "%d", &uiCtrl) != 1) ||
        (sscanf(ppcArgV[2], "%d", &uiDrive) != 1)) {
        goto  __arg_error;
    } else {
        hAhciDev = API_AhciDevHandleGet(uiCtrl, uiDrive);
        if (!hAhciDev) {
            goto  __arg_error;
        }
    }

    if (iArgC == 3) {
        __tshellAhciDevCmdShow(hAhciDev);
        return  (ERROR_NONE);
    }

    if (lib_strcmp(ppcArgV[3], "-c") == 0) {                            /* CACHE ����                   */
        if (iArgC < 5) {
            goto  __arg_error;
        }
        if (sscanf(ppcArgV[4], "%d", &iCacheFlush) != 1) {
            goto  __arg_error;
        }

        iCacheFlush = (iCacheFlush) ? 1 : 0;
        iRet = API_AhciDevIoctl(hAhciDev, AHCI_DEV_IOCTL_CACHE_FLUSH_SET, iCacheFlush);
        if (iRet != ERROR_NONE) {
            printf("ahci dev cache flush %s error.\n", (iCacheFlush) ? "enable" : "disable");
        }
        printf("ahci dev cache flush %s ok.\n", (iCacheFlush) ? "enable" : "disable");
    } else {
        goto  __arg_error;
    }

    return  (ERROR_NONE);

__arg_error:
    fprintf(stderr, "argument error.\n");
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
