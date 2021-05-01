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
** ��   ��   ��: lwip_jobqueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 03 �� 30 ��
**
** ��        ��: �����첽����������. (��������Ӧ�ð���Ҫ�Ĵ������� job queue ��, 
                                        ���ݰ��ڴ洦�����������ж��������д���.)

** BUG:
2009.05.20  netjob �߳�Ӧ�þ��а�ȫ����.
2009.12.09  �޸�ע��.
2013.12.01  ����ʹ����Ϣ����, ʹ���ں��ṩ�Ĺ�������ģ��.
2016.11.04  ֧�ֶ������д������.
2018.08.01  ֧�ֶ��кϲ��Զ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_LWIP_JOBQUEUE_NUM != 1) && \
    ((LW_CFG_LWIP_JOBQUEUE_NUM & (LW_CFG_LWIP_JOBQUEUE_NUM - 1)) != 0)
#error "N must be equal to 1 or Pow of 2!"
#endif
/*********************************************************************************************************
  ���繤������
*********************************************************************************************************/
static LW_JOB_QUEUE         _G_jobqNet[LW_CFG_LWIP_JOBQUEUE_NUM];
static LW_JOB_MSG           _G_jobmsgNet[LW_CFG_LWIP_JOBQUEUE_NUM][LW_CFG_LWIP_JOBQUEUE_SIZE];
static UINT                 _G_uiJobqNum;
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID    _NetJobThread(PLW_JOB_QUEUE  pjobq);                     /*  ��ҵ�������                */
/*********************************************************************************************************
** ��������: _netJobqueueInit
** ��������: ��ʼ�� Net jobqueue ���� ����
** �䡡��  : NONE
** �䡡��  : �Ƿ��ʼ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _netJobqueueInit (VOID)
{
    INT                 i;
    LW_OBJECT_HANDLE    hNetJobThread[LW_CFG_LWIP_JOBQUEUE_NUM];
    LW_CLASS_THREADATTR threadattr;
    
    _G_uiJobqNum = LW_CFG_LWIP_JOBQUEUE_NUM;
    
    while (_G_uiJobqNum > LW_NCPUS) {
        _G_uiJobqNum >>= 1;
    }
    
#if LW_CFG_LWIP_JOBQUEUE_MERGE > 0
    if (_jobQueueInit(&_G_jobqNet[0], &_G_jobmsgNet[0][0], 
                      LW_CFG_LWIP_JOBQUEUE_SIZE * LW_CFG_LWIP_JOBQUEUE_NUM, LW_FALSE)) {
        return  (PX_ERROR);
    }
#else
    for (i = 0; i < _G_uiJobqNum; i++) {
        if (_jobQueueInit(&_G_jobqNet[i], &_G_jobmsgNet[i][0], 
                          LW_CFG_LWIP_JOBQUEUE_SIZE, LW_FALSE)) {
            break;
        }
    }
    if (i < _G_uiJobqNum) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create netjob queue.\r\n");
        for (; i >= 0; i--) {
            _jobQueueFinit(&_G_jobqNet[i]);
        }
        return  (PX_ERROR);
    }
#endif
    
    API_ThreadAttrBuild(&threadattr, LW_CFG_LWIP_JOBQUEUE_STK_SIZE, 
                        LW_PRIO_T_NETJOB, 
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
    
    for (i = 0; i < _G_uiJobqNum; i++) {
#if LW_CFG_LWIP_JOBQUEUE_MERGE > 0
        threadattr.THREADATTR_pvArg = (PVOID)&_G_jobqNet[0];
#else
        threadattr.THREADATTR_pvArg = (PVOID)&_G_jobqNet[i];
#endif
        hNetJobThread[i] = API_ThreadCreate("t_netjob",
                                            (PTHREAD_START_ROUTINE)_NetJobThread,
                                            (PLW_CLASS_THREADATTR)&threadattr,
                                            LW_NULL);                   /*  ���� job �����߳�           */
        if (hNetJobThread[i] == LW_OBJECT_HANDLE_INVALID) {
            break;
        }
    }
    if (i < _G_uiJobqNum) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create netjob task.\r\n");
        for (; i >= 0; i--) {
            API_ThreadDelete(&hNetJobThread[i], LW_NULL);
        }
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NetJobAdd
** ��������: ���������첽������ҵ����
** �䡡��  : pfunc                      ����ָ��
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_NetJobAdd (VOIDFUNCPTR  pfunc, 
                    PVOID        pvArg0,
                    PVOID        pvArg1,
                    PVOID        pvArg2,
                    PVOID        pvArg3,
                    PVOID        pvArg4,
                    PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqNet[0], pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NetJobAddEx
** ��������: ���������첽������ҵ����
** �䡡��  : uiQ                        ����ʶ���
**           pfunc                      ����ָ��
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_NetJobAddEx (UINT         uiQ,
                      VOIDFUNCPTR  pfunc, 
                      PVOID        pvArg0,
                      PVOID        pvArg1,
                      PVOID        pvArg2,
                      PVOID        pvArg3,
                      PVOID        pvArg4,
                      PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_LWIP_JOBQUEUE_MERGE > 0
    if (_jobQueueAdd(&_G_jobqNet[0], 
                     pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) 
#else
    if (_jobQueueAdd(&_G_jobqNet[uiQ & (_G_uiJobqNum - 1)], 
                     pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) 
#endif
                     
    {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NetJobDelete
** ��������: �������첽������ҵ������ɾ��
** �䡡��  : uiMatchArgNum              ƥ������ĸ���
**           pfunc                      ����ָ��
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_NetJobDelete (UINT         uiMatchArgNum,
                        VOIDFUNCPTR  pfunc, 
                        PVOID        pvArg0,
                        PVOID        pvArg1,
                        PVOID        pvArg2,
                        PVOID        pvArg3,
                        PVOID        pvArg4,
                        PVOID        pvArg5)
{
    if (!pfunc) {
        return;
    }
    
    _jobQueueDel(&_G_jobqNet[0], uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
}
/*********************************************************************************************************
** ��������: API_NetJobDeleteEx
** ��������: �������첽������ҵ������ɾ��
** �䡡��  : uiQ                        ����ʶ���
**           uiMatchArgNum              ƥ������ĸ���
**           pfunc                      ����ָ��
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_NetJobDeleteEx (UINT         uiQ,
                          UINT         uiMatchArgNum,
                          VOIDFUNCPTR  pfunc, 
                          PVOID        pvArg0,
                          PVOID        pvArg1,
                          PVOID        pvArg2,
                          PVOID        pvArg3,
                          PVOID        pvArg4,
                          PVOID        pvArg5)
{
    if (!pfunc) {
        return;
    }
    
#if LW_CFG_LWIP_JOBQUEUE_MERGE > 0
    _jobQueueDel(&_G_jobqNet[0], uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    
#else
    if (uiQ == LW_NETJOB_Q_ALL) {
        UINT  i;
    
        for (i = 0; i < _G_uiJobqNum; i++) {
            _jobQueueDel(&_G_jobqNet[i], uiMatchArgNum, 
                         pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
        }
    
    } else {
        _jobQueueDel(&_G_jobqNet[uiQ & (_G_uiJobqNum - 1)], uiMatchArgNum, 
                     pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
#endif
}
/*********************************************************************************************************
** ��������: _NetJobThread
** ��������: ���繤�����д����߳�
** �䡡��  : pjobq     �������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _NetJobThread (PLW_JOB_QUEUE  pjobq)
{
    for (;;) {
        _jobQueueExec(pjobq, LW_OPTION_WAIT_INFINITE);
    }
}
/*********************************************************************************************************
** ��������: API_NetJobGetLost
** ��������: ���������Ϣ��ʧ������
** �䡡��  : NONE
** �䡡��  : ��Ϣ��ʧ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_NetJobGetLost (VOID)
{
    ULONG   ulTotal = 0;
    
#if LW_CFG_LWIP_JOBQUEUE_MERGE > 0
    ulTotal = _jobQueueLost(&_G_jobqNet[0]);
    
#else
    INT     i;
    
    for (i = 0; i < _G_uiJobqNum; i++) {
        ulTotal += _jobQueueLost(&_G_jobqNet[i]);
    }
#endif
    
    return  (ulTotal);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
