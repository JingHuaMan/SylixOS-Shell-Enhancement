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
** ��   ��   ��: nvmeDev.c
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe �豸����.
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
static UINT32                   _GuiNvmeDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulNvmeDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineNvmeDevHeader = LW_NULL;

#define __NVME_DEV_LOCK()       API_SemaphoreMPend(_GulNvmeDevLock, LW_OPTION_WAIT_INFINITE)
#define __NVME_DEV_UNLOCK()     API_SemaphoreMPost(_GulNvmeDevLock)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID     __tshellNvmeDevCmdShow(NVME_DEV_HANDLE  hDev);
static INT      __tshellNvmeDevCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** ��������: API_NvmeDevDelete
** ��������: ɾ��һ�� NVMe �豸
** �䡡��  : hDev      �豸���ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeDevDelete (NVME_DEV_HANDLE  hDev)
{
    NVME_DEV_HANDLE     hDevNvme = LW_NULL;

    hDevNvme = API_NvmeDevHandleGet(hDev->NVMEDEV_hCtrl->NVMECTRL_uiIndex,
                                    hDev->NVMEDEV_uiNameSpaceId);
    if ((!hDevNvme) ||
        (hDevNvme != hDev)) {
        return  (PX_ERROR);
    }

    __NVME_DEV_LOCK();
    _List_Line_Del(&hDev->NVMEDEV_lineDevNode, &_GplineNvmeDevHeader);
    _GuiNvmeDevTotalNum--;
    __NVME_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeDevAdd
** ��������: ����һ���豸
** �䡡��  : hCtrl      ���������
**           uiDrive    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeDevAdd (NVME_DEV_HANDLE    hDev)
{
    NVME_DEV_HANDLE    hDevTmep = LW_NULL;

    hDevTmep = API_NvmeDevHandleGet(hDev->NVMEDEV_uiCtrl,
                                    hDev->NVMEDEV_uiNameSpaceId);
    if (hDevTmep != LW_NULL) {
        return  (ERROR_NONE);
    }

    __NVME_DEV_LOCK();
    _List_Line_Add_Ahead(&hDev->NVMEDEV_lineDevNode, &_GplineNvmeDevHeader);
    _GuiNvmeDevTotalNum++;
    __NVME_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeDevHandleGet
** ��������: ��ȡһ���豸�ľ��
** �䡡��  : uiCtrl         ����������
**           NamespaceId    �����ռ�ID
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_DEV_HANDLE  API_NvmeDevHandleGet (UINT  uiCtrl, UINT  NamespaceId)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DEV_HANDLE     hDev      = LW_NULL;

    hDev = LW_NULL;
    __NVME_DEV_LOCK();
    for (plineTemp  = _GplineNvmeDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
        if ((hDev->NVMEDEV_uiCtrl  == uiCtrl ) &&
            (hDev->NVMEDEV_uiNameSpaceId == NamespaceId)) {
            break;
        }
    }
    __NVME_DEV_UNLOCK();

    if (plineTemp) {
        return  (hDev);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __nvmeMonitorThread
** ��������: �豸�������߳�
** �䡡��  : pvArg             ��������
** �䡡��  : NULL
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
PVOID  __nvmeMonitorThread (PVOID   pvArg)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DEV_HANDLE     hDev      = LW_NULL;
    VOID              (*fpCallBack)(NVME_DEV_HANDLE) = (VOID (*)(NVME_DEV_HANDLE))pvArg;

    for (;;) {
        API_TimeSleep(LW_TICK_HZ);
        __NVME_DEV_LOCK();
        for (plineTemp  = _GplineNvmeDevHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
            fpCallBack(hDev);
        }
        __NVME_DEV_UNLOCK();
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_NvmeDevCountGet
** ��������: ��ȡ NVMe �豸����
** �䡡��  : NONE
** �䡡��  : NVMe �豸����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
UINT32  API_NvmeDevCountGet (VOID)
{
    UINT32      uiCount = 0;

    __NVME_DEV_LOCK();
    uiCount = _GuiNvmeDevTotalNum;
    __NVME_DEV_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** ��������: API_NvmeDevInit
** ��������: NVMe �豸�����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeDevInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiNvmeDevTotalNum  = 0;
    _GplineNvmeDevHeader = LW_NULL;

    _GulNvmeDevLock = API_SemaphoreMCreate("nvme_devlock",
                                           LW_PRIO_DEF_CEILING,
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    if (_GulNvmeDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("nvmedev", __tshellNvmeDevCmd);
    API_TShellFormatAdd("nvmedev", " [[-c] 1 ...]");
    API_TShellHelpAdd("nvmedev", "show, add, del nvme device\n"
                                 "eg. nvmedev\n"
                                 "    nvmedev 0 0\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNvmeDevCmdShow
** ��������: ��ӡ NVMe �豸�б�
** �䡡��  : hDev       �豸���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellNvmeDevCmdShow (NVME_DEV_HANDLE  hDev)
{
    static PCHAR        pcNvmeDevShowHdr = \
    "INDEX CTRL DRIVER  START    SECTOR COUNT       DRVNAME         DEVNAME         BLKNAME\n"
    "----- ---- ------ ------- ---------------- --------------- --------------- ---------------\n";

    REGISTER INT        i;
    PLW_BLK_DEV         hBlkDev   = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;

    i = 0;
    if (hDev) {
        hBlkDev = &hDev->NVMEDEV_tBlkDev;
        printf("\n");
        printf(pcNvmeDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->NVMEDEV_uiCtrl,
               hDev->NVMEDEV_uiNameSpaceId,
               (UINT32)hDev->NVMEDEV_ulBlkOffset,
               (UINT32)hDev->NVMEDEV_ulBlkCount,
               hDev->NVMEDEV_hCtrl->NVMECTRL_hDrv->NVMEDRV_cDrvName,
               hDev->NVMEDEV_tBlkDev.BLKD_pcName,
               hBlkDev->BLKD_pcName);
        return;
    }

    printf("nvme dev number total: %d\n", _GuiNvmeDevTotalNum);

    __NVME_DEV_LOCK();
    i = 0;
    for (plineTemp  = _GplineNvmeDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
        __NVME_DEV_UNLOCK();
        hBlkDev = &hDev->NVMEDEV_tBlkDev;
        printf("\n");
        printf(pcNvmeDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->NVMEDEV_uiCtrl,
               hDev->NVMEDEV_uiNameSpaceId,
               (UINT32)hDev->NVMEDEV_ulBlkOffset,
               (UINT32)hDev->NVMEDEV_ulBlkCount,
               hDev->NVMEDEV_hCtrl->NVMECTRL_hDrv->NVMEDRV_cDrvName,
               hDev->NVMEDEV_tBlkDev.BLKD_pcName,
               hBlkDev->BLKD_pcName);
        __NVME_DEV_LOCK();
        i += 1;
    }
    __NVME_DEV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __tshellNvmeDevCmd
** ��������: NVMe ���� "nvmedev"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNvmeDevCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    NVME_DEV_HANDLE     hNvmeDev;                                       /* �豸���ƾ��                 */
    UINT                uiCtrl;                                         /* ����������                   */
    UINT                uiDrive;                                        /* ��������                     */

    if (iArgC == 1) {
        __tshellNvmeDevCmdShow(LW_NULL);
        return  (ERROR_NONE);
    }

    if ((iArgC != 3) ||
        (sscanf(ppcArgV[1], "%d", &uiCtrl) != 1) ||
        (sscanf(ppcArgV[2], "%d", &uiDrive) != 1)) {
        goto    __arg_error;
    
    } else {
        hNvmeDev = API_NvmeDevHandleGet(uiCtrl, uiDrive);
        if (!hNvmeDev) {
            goto    __arg_error;
        }
    }

    __tshellNvmeDevCmdShow(hNvmeDev);

    return  (ERROR_NONE);

__arg_error:
    fprintf(stderr, "argument error.\n");
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
