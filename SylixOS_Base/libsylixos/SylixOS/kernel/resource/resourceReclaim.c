/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: resourceReclaim.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 09 ��
**
** ��        ��: Ϊ�˱��⽩ʬ�߳�, SylixOS ʹ��ͳһ����Դ������������Դ����.
**
** ע        ��: ��Դ���յ�ԭ����: ���̺Ų�Ϊ 0, �Ҳ��� GLOBAL �Ķ�����Ҫ���л���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  ��Դ���ն���
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulResReclaimQueue;
/*********************************************************************************************************
** ��������: __resReclaimReq
** ��������: ������ս�����Դ
** �䡡��  : pvVProc       ���̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __resReclaimReq (PVOID  pvVProc)
{
    ULONG        ulError;
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        ulError = API_MsgQueueSend(_G_ulResReclaimQueue, (PVOID)&pvproc, sizeof(PVOID));
        if (ulError) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not req reclaim.\r\n");
        }
    }
}
/*********************************************************************************************************
** ��������: __resReclaimThread
** ��������: ��Դ������ϵͳ�߳�
** �䡡��  : pvArg         ��ʱû��ʹ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __resReclaimThread (PVOID  pvArg)
{
    LW_LD_VPROC *pvproc;
    ULONG        ulError;

    (VOID)pvArg;
    
    for (;;) {
        ulError = API_MsgQueueReceive(_G_ulResReclaimQueue, (PVOID)&pvproc, sizeof(PVOID), 
                                      LW_NULL, LW_OPTION_WAIT_INFINITE);
        if (ulError == ERROR_NONE) {
            vprocReclaim(pvproc, LW_TRUE);                              /*  ���ս�����Դ                */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __resReclaimInit
** ��������: ��Դ��������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __resReclaimInit (VOID)
{
    LW_CLASS_THREADATTR     threadattr;

    _G_ulResReclaimQueue = API_MsgQueueCreate("res_reclaim", LW_CFG_MAX_THREADS, 
                                              sizeof(PVOID), LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_ulResReclaimQueue == LW_OBJECT_HANDLE_INVALID) {
        return;
    }
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_THREAD_RECLAIM_STK_SIZE,
                        LW_PRIO_T_RECLAIM,
                        LW_CFG_RECLAIM_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    
    API_ThreadCreate("t_reclaim", __resReclaimThread, &threadattr, LW_NULL);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
