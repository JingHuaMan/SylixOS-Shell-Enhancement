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
** ��   ��   ��: nvmeLib.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
/*********************************************************************************************************
** ��������: API_NvmeCtrlIntConnect
** ��������: ���ӿ������ж�
** �䡡��  : hCtrl      ���������
**           hQueue     �������
**           pfuncIsr   �жϷ�����
**           cpcName    �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlIntConnect (NVME_CTRL_HANDLE   hCtrl,
                             NVME_QUEUE_HANDLE  hQueue,
                             PINT_SVR_ROUTINE   pfuncIsr,
                             CPCHAR             cpcName)
{
    INT                 iRet;
    NVME_DRV_HANDLE     hDrv;

    hDrv = hCtrl->NVMECTRL_hDrv;

    if (hDrv->NVMEDRV_pfuncVendorCtrlIntConnect) {
        iRet = hDrv->NVMEDRV_pfuncVendorCtrlIntConnect(hCtrl, hQueue, pfuncIsr, cpcName);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (hDrv->NVMEDRV_pfuncVendorCtrlIntEnable) {
        hDrv->NVMEDRV_pfuncVendorCtrlIntEnable(hCtrl, hQueue, pfuncIsr, cpcName);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* (LW_CFG_DEVICE_EN > 0) &&    */
                                                                        /* (LW_CFG_NVME_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
