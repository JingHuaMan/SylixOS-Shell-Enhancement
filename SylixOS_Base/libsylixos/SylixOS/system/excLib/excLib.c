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
** ��   ��   ��: excLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 03 �� 30 ��
**
** ��        ��: ϵͳ�쳣�����ڲ�������

** BUG
2007.06.25  ��ϵͳ�߳����ָ�Ϊ t_??? ����ʽ��
2008.01.13  ϵͳʹ�õ���Ϣ������������ LW_CFG_MAX_EXCEMSGS.
2008.01.16  �޸�����Ϣ������.
2010.09.18  _excJobAdd() NULL �������ܼ��� job ����.
2012.03.12  sigthread ʹ�ö�̬ attr
2013.07.14  ����ע��.
2013.08.28  ���������ʵ�����.
2013.12.01  ����ʹ����Ϣ����, ʹ���ں��ṩ�Ĺ�������ģ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#define MAX_EXC_MSG_NUM     (NSIG + LW_CFG_MAX_EXCEMSGS + LW_CFG_MAX_THREADS)
#else
#define MAX_EXC_MSG_NUM     LW_CFG_MAX_EXCEMSGS
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

static LW_JOB_QUEUE         _G_jobqExc;
static LW_JOB_MSG           _G_jobmsgExc[MAX_EXC_MSG_NUM];
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID  _ExcThread(VOID);                                          /*  �쳣�������                */
/*********************************************************************************************************
** ��������: _excInit
** ��������: ��ʼ�� �쳣���� ����
** �䡡��  : NONE
** �䡡��  : �Ƿ��ʼ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _excInit (VOID)
{
    LW_CLASS_THREADATTR threadattr;

    if (_jobQueueInit(&_G_jobqExc, &_G_jobmsgExc[0], MAX_EXC_MSG_NUM, LW_FALSE)) {
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_SIG_STK_SIZE, 
                        LW_PRIO_T_EXCPT,                                /*  ������ȼ�                  */
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL), 
                        LW_NULL);
                        
    _S_ulThreadExceId = API_ThreadCreate("t_except",
                                         (PTHREAD_START_ROUTINE)_ExcThread,
                                         &threadattr,
                                         LW_NULL);                      /*  �����쳣�����߳�            */
    if (!_S_ulThreadExceId) {
        _jobQueueFinit(&_G_jobqExc);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _excJobAdd
** ��������: ����������쳣��Ϣ
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
*********************************************************************************************************/
INT  _excJobAdd (VOIDFUNCPTR  pfunc, 
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
    
    if (_jobQueueAdd(&_G_jobqExc, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _excJobDel
** ��������: ɾ���������쳣��Ϣ
** �䡡��  : uiMatchArgNum              ƥ���������
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
*********************************************************************************************************/
VOID  _excJobDel (UINT         uiMatchArgNum,
                  VOIDFUNCPTR  pfunc, 
                  PVOID        pvArg0, 
                  PVOID        pvArg1, 
                  PVOID        pvArg2, 
                  PVOID        pvArg3, 
                  PVOID        pvArg4, 
                  PVOID        pvArg5)
{
    _jobQueueDel(&_G_jobqExc, uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
}
/*********************************************************************************************************
** ��������: _ExcThread
** ��������: �����쳣��Ϣ���߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _ExcThread (VOID)
{
    for (;;) {
        _jobQueueExec(&_G_jobqExc, LW_OPTION_WAIT_INFINITE);
    }
}
/*********************************************************************************************************
** ��������: _ExcGetLost
** ��������: ����쳣��Ϣ��ʧ������
** �䡡��  : NONE
** �䡡��  : �쳣��Ϣ��ʧ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _ExcGetLost (VOID)
{
    return  (_jobQueueLost(&_G_jobqExc));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
