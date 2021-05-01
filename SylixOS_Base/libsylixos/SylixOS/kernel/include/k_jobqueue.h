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
** ��   ��   ��: k_jobqueue.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 01 ��
**
** ��        ��: �ں˹�������ģ��.
*********************************************************************************************************/

#ifndef __K_JOBQUEUE_H
#define __K_JOBQUEUE_H

/*********************************************************************************************************
  ��������һ�����
*********************************************************************************************************/

#define LW_JOB_ARGS     6

/*********************************************************************************************************
  �������нṹ
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR             JOBM_pfuncFunc;                             /*  �ص�����                    */
    PVOID                   JOBM_pvArg[LW_JOB_ARGS];                    /*  �ص�����                    */
} LW_JOB_MSG;
typedef LW_JOB_MSG         *PLW_JOB_MSG;

typedef struct {
    PLW_JOB_MSG             JOBQ_pjobmsgQueue;                          /*  ����������Ϣ                */
    UINT                    JOBQ_uiIn;
    UINT                    JOBQ_uiOut;
    UINT                    JOBQ_uiCnt;
    UINT                    JOBQ_uiSize;
    ULONG                   JOBQ_ulLost;                                /*  ��ʧ��Ϣ����                */
    LW_OBJECT_HANDLE        JOBQ_ulSync;                                /*  ͬ���ȴ�                    */
    LW_SPINLOCK_DEFINE     (JOBQ_slLock);
} LW_JOB_QUEUE;
typedef LW_JOB_QUEUE       *PLW_JOB_QUEUE;

/*********************************************************************************************************
  �������� NOP 
*********************************************************************************************************/

#define LW_JOB_NOPFUNC      ((VOIDFUNCPTR)0)

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

PLW_JOB_QUEUE    _jobQueueCreate(UINT  uiQueueSize, BOOL  bNonBlock);
VOID             _jobQueueDelete(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueInit(PLW_JOB_QUEUE pjobq, PLW_JOB_MSG  pjobmsg, 
                               UINT uiQueueSize, BOOL bNonBlock);
VOID             _jobQueueFinit(PLW_JOB_QUEUE pjobq);
VOID             _jobQueueFlush(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueAdd(PLW_JOB_QUEUE pjobq,
                              VOIDFUNCPTR   pfunc,
                              PVOID         pvArg0,
                              PVOID         pvArg1,
                              PVOID         pvArg2,
                              PVOID         pvArg3,
                              PVOID         pvArg4,
                              PVOID         pvArg5);
VOID             _jobQueueDel(PLW_JOB_QUEUE pjobq,
                              UINT          uiMatchArgNum,
                              VOIDFUNCPTR   pfunc, 
                              PVOID         pvArg0,
                              PVOID         pvArg1,
                              PVOID         pvArg2,
                              PVOID         pvArg3,
                              PVOID         pvArg4,
                              PVOID         pvArg5);
ULONG            _jobQueueLost(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueExec(PLW_JOB_QUEUE pjobq, ULONG  ulTimeout);

#endif                                                                  /*  __K_JOBQUEUE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
