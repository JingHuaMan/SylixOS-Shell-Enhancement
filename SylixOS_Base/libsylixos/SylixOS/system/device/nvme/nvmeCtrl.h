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
** ��   ��   ��: nvmeCtrl.h
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ����������.
*********************************************************************************************************/

#ifndef __NVME_CTRL_H
#define __NVME_CTRL_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)

LW_API INT                  API_NvmeCtrlDelete(NVME_CTRL_HANDLE  hCtrl);
LW_API INT                  API_NvmeCtrlAdd(NVME_CTRL_HANDLE  hCtrl);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromPciArg(PVOID  pvCtrlPciArg);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromName(CPCHAR  cpcName, UINT  uiUnit);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromIndex(UINT  uiIndex);
LW_API INT                  API_NvmeCtrlIndexGet(VOID);
LW_API UINT32               API_NvmeCtrlCountGet(VOID);
LW_API INT                  API_NvmeCtrlInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
#endif                                                                  /*  __NVME_CTRL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
