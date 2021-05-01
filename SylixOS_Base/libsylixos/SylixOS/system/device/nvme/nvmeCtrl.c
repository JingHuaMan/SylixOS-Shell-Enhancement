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
** ��   ��   ��: nvmeCtrl.c
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ����������.
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
static UINT32                   _GuiNvmeCtrlTotalNum  = 0;
static UINT8                    _GucNvmeCtrlIndexMap  = 0;
static LW_OBJECT_HANDLE         _GulNvmeCtrlLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineNvmeCtrlHeader = LW_NULL;

#define __NVME_CTRL_LOCK()      API_SemaphoreMPend(_GulNvmeCtrlLock, LW_OPTION_WAIT_INFINITE)
#define __NVME_CTRL_UNLOCK()    API_SemaphoreMPost(_GulNvmeCtrlLock)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID     __tshellNvmeCtrlCmdShow(VOID);
static INT      __tshellNvmeCtrlCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** ��������: API_NvmeCtrlDelete
** ��������: ɾ��һ�� NVMe ������
** �䡡��  : hCtrl     �豸���ƾ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlDelete (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_NvmeCtrlHandleGetFromIndex(hCtrl->NVMECTRL_uiIndex);
    if ((!hCtrlTmep) ||
        (hCtrlTmep != hCtrl)) {
        return  (PX_ERROR);
    }

    __NVME_CTRL_LOCK();
    _List_Line_Del(&hCtrl->NVMECTRL_lineCtrlNode, &_GplineNvmeCtrlHeader);
    _GucNvmeCtrlIndexMap &= ~(0x01 << hCtrl->NVMECTRL_uiIndex);
    _GuiNvmeCtrlTotalNum--;
    __NVME_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlAdd
** ��������: ����һ��������
** �䡡��  : hCtrl      ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlAdd (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_NvmeCtrlHandleGetFromIndex(hCtrl->NVMECTRL_uiIndex);
    if (hCtrlTmep != LW_NULL) {
        return  (ERROR_NONE);
    }

    __NVME_CTRL_LOCK();
    _List_Line_Add_Ahead(&hCtrl->NVMECTRL_lineCtrlNode, &_GplineNvmeCtrlHeader);
    _GucNvmeCtrlIndexMap |= 0x01 << hCtrl->NVMECTRL_uiIndex;
    _GuiNvmeCtrlTotalNum++;
    __NVME_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlHandleGetFromPciArg
** ��������: ͨ�� PCI �豸�����ȡһ���������ľ��
** �䡡��  : pvCtrlPciArg      ����������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromPciArg (PVOID  pvCtrlPciArg)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if (hCtrl->NVMECTRL_pvArg == pvCtrlPciArg) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlHandleGetFromName
** ��������: ͨ�����ֻ�ȡһ���������ľ��
** �䡡��  : cpcName    ����������
**           uiUnit     �������������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromName (CPCHAR  cpcName, UINT  uiUnit)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if ((lib_strncmp(&hCtrl->NVMECTRL_cCtrlName[0], cpcName, NVME_CTRL_NAME_MAX) == 0) &&
            (hCtrl->NVMECTRL_uiUnitIndex == uiUnit)) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlHandleGetFromIndex
** ��������: ͨ��������ȡһ���������ľ��
** �䡡��  : uiIndex     ����������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromIndex (UINT  uiIndex)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if (hCtrl->NVMECTRL_uiIndex == uiIndex) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlIndexGet
** ��������: ��ȡ NVMe ����������
** �䡡��  : NONE
** �䡡��  : NVMe ����������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlIndexGet (VOID)
{
    REGISTER INT    i;

    __NVME_CTRL_LOCK();
    for (i = 0; i < NVME_CTRL_MAX; i++) {
        if (!((_GucNvmeCtrlIndexMap >> i) & 0x01)) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (i >= NVME_CTRL_MAX) {
        return  (PX_ERROR);
    }

    return  (i);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlCountGet
** ��������: ��ȡ NVMe ����������
** �䡡��  : NONE
** �䡡��  : NVMe ����������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
UINT32  API_NvmeCtrlCountGet (VOID)
{
    UINT32      uiCount = 0;

    __NVME_CTRL_LOCK();
    uiCount = _GuiNvmeCtrlTotalNum;
    __NVME_CTRL_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlInit
** ��������: NVMe �����������ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiNvmeCtrlTotalNum  = 0;
    _GplineNvmeCtrlHeader = LW_NULL;

    _GulNvmeCtrlLock = API_SemaphoreMCreate("nvme_ctrllock",
                                            LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    if (_GulNvmeCtrlLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("nvmectrl", __tshellNvmeCtrlCmd);
    API_TShellFormatAdd("nvmectrl", " [add | del] [0:1]");
    API_TShellHelpAdd("nvmectrl", "show, add, del nvme control\n"
                                  "eg. nvmectrl\n"
                                  "    nvmectrl add imx6dq_nvme 0\n"
                                  "    nvmectrl del imx6dq_nvme 0\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNvmeCtrlCmdShow
** ��������: ��ӡ NVMe �������б�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellNvmeCtrlCmdShow (VOID)
{
    static PCHAR        pcNvmeCtrlShowHdr = \
    "INDEX CTRL UNIT     CTRLNAME           CTRL VER             DRIVE VER\n"
    "----- ---- ---- ---------------- -------------------- --------------------\n";

    REGISTER INT        i;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;
    CHAR                cVerNum[NVME_DRV_VER_STR_LEN]  = {0};

    printf("nvme control number total: %d\n", _GuiNvmeCtrlTotalNum);
    printf(pcNvmeCtrlShowHdr);

    __NVME_CTRL_LOCK();
    i = 0;
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        printf("%5d %4d %4d %-16s",
               i,
               hCtrl->NVMECTRL_uiIndex,
               hCtrl->NVMECTRL_uiUnitIndex,
               hCtrl->NVMECTRL_cCtrlName);

        if ((hCtrl) && (hCtrl->NVMECTRL_uiCoreVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN, NVME_DRV_VER_FORMAT(hCtrl->NVMECTRL_uiCoreVer));
            printf(" %-20s", cVerNum);
        } else {
            printf(" %-20s", "*");
        }

        if ((hCtrl) && (hCtrl->NVMECTRL_hDrv) && (hCtrl->NVMECTRL_hDrv->NVMEDRV_uiDrvVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN,
                     NVME_DRV_VER_FORMAT(hCtrl->NVMECTRL_hDrv->NVMEDRV_uiDrvVer));
            printf(" %-20s\n", cVerNum);
        } else {
            printf(" %-20s\n", "*");
        }

        i += 1;
    }
    __NVME_CTRL_UNLOCK();
}
/*********************************************************************************************************
** ��������: __tshellNvmeCtrlCmd
** ��������: NVMe ���� "nvmectrl"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNvmeCtrlCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellNvmeCtrlCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
