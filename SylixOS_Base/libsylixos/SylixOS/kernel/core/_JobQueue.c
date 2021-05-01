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
** ��   ��   ��: _JobQueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 01 ��
**
** ��        ��: ����ϵͳ�첽��������.
**
** BUG:
2018.01.05  _jobQueueDel() ��� pfunc ����Ϊ NULL ���ʾ���⺯��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  JOB MSG COPY
*********************************************************************************************************/
#define LW_JOB_MSG_COPY(t, f)                               \
        {                                                   \
            (t)->JOBM_pfuncFunc = (f)->JOBM_pfuncFunc;      \
            (t)->JOBM_pvArg[0]  = (f)->JOBM_pvArg[0];       \
            (t)->JOBM_pvArg[1]  = (f)->JOBM_pvArg[1];       \
            (t)->JOBM_pvArg[2]  = (f)->JOBM_pvArg[2];       \
            (t)->JOBM_pvArg[3]  = (f)->JOBM_pvArg[3];       \
            (t)->JOBM_pvArg[4]  = (f)->JOBM_pvArg[4];       \
            (t)->JOBM_pvArg[5]  = (f)->JOBM_pvArg[5];       \
        }
/*********************************************************************************************************
** ��������: _JobQueueCreate
** ��������: ����һ����������
** �䡡��  : uiQueueSize       ���д�С
**           bNonBlock         ִ�к����Ƿ�Ϊ��������ʽ
** �䡡��  : �������п��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_JOB_QUEUE  _jobQueueCreate (UINT uiQueueSize, BOOL bNonBlock)
{
    PLW_JOB_QUEUE pjobq;
    
    pjobq = (PLW_JOB_QUEUE)__KHEAP_ALLOC((size_t)(sizeof(LW_JOB_QUEUE) + 
                                         (uiQueueSize * sizeof(LW_JOB_MSG))));
    if (pjobq == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pjobq->JOBQ_pjobmsgQueue = (PLW_JOB_MSG)(pjobq + 1);
    pjobq->JOBQ_uiIn         = 0;
    pjobq->JOBQ_uiOut        = 0;
    pjobq->JOBQ_uiCnt        = 0;
    pjobq->JOBQ_uiSize       = uiQueueSize;
    pjobq->JOBQ_ulLost       = 0;
    
    if (bNonBlock == LW_FALSE) {
        pjobq->JOBQ_ulSync = API_SemaphoreBCreate("job_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pjobq->JOBQ_ulSync == LW_OBJECT_HANDLE_INVALID) {
            __KHEAP_FREE(pjobq);
            return  (LW_NULL);
        }
    } else {
        pjobq->JOBQ_ulSync = LW_OBJECT_HANDLE_INVALID;
    }
    
    LW_SPIN_INIT(&pjobq->JOBQ_slLock);
    
    return  (pjobq);
}
/*********************************************************************************************************
** ��������: _JobQueueDelete
** ��������: ɾ��һ����������
** �䡡��  : pjobq         �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _jobQueueDelete (PLW_JOB_QUEUE pjobq)
{
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBDelete(&pjobq->JOBQ_ulSync);
    }
    
    __KHEAP_FREE(pjobq);
}
/*********************************************************************************************************
** ��������: _JobQueueInit
** ��������: ��ʼ��һ���������� (��̬����)
** �䡡��  : pjobq             ��Ҫ��ʼ���Ĺ������п��ƿ�
**           pjobmsg           ��Ϣ������
**           uiQueueSize       ���д�С
**           bNonBlock         ִ�к����Ƿ�Ϊ��������ʽ
** �䡡��  : �������п��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _jobQueueInit (PLW_JOB_QUEUE pjobq, PLW_JOB_MSG  pjobmsg, UINT uiQueueSize, BOOL bNonBlock)
{
    pjobq->JOBQ_pjobmsgQueue = pjobmsg;
    pjobq->JOBQ_uiIn         = 0;
    pjobq->JOBQ_uiOut        = 0;
    pjobq->JOBQ_uiCnt        = 0;
    pjobq->JOBQ_uiSize       = uiQueueSize;
    pjobq->JOBQ_ulLost       = 0;
    
    if (bNonBlock == LW_FALSE) {
        pjobq->JOBQ_ulSync = API_SemaphoreBCreate("job_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pjobq->JOBQ_ulSync == LW_OBJECT_HANDLE_INVALID) {
            return  (errno);
        }
    }
    
    LW_SPIN_INIT(&pjobq->JOBQ_slLock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _jobQueueFinit
** ��������: ����һ���������� (��̬����)
** �䡡��  : pjobq         �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _jobQueueFinit (PLW_JOB_QUEUE pjobq)
{
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBDelete(&pjobq->JOBQ_ulSync);
    }
}
/*********************************************************************************************************
** ��������: _jobQueueFlush
** ��������: ��չ�������
** �䡡��  : pjobq         �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _jobQueueFlush (PLW_JOB_QUEUE pjobq)
{
    INTREG   iregInterLevel;
    
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBClear(pjobq->JOBQ_ulSync);
    }
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    
    pjobq->JOBQ_uiIn  = 0;
    pjobq->JOBQ_uiOut = 0;
    pjobq->JOBQ_uiCnt = 0;
    
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: _JobQueueAdd
** ��������: ���һ����������������
** �䡡��  : pjobq         �������п��ƿ�
**           pfunc         Ҫִ�еĺ���
**           pvArg0 ~ 5    ��������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _jobQueueAdd (PLW_JOB_QUEUE pjobq,
                     VOIDFUNCPTR   pfunc,
                     PVOID         pvArg0,
                     PVOID         pvArg1,
                     PVOID         pvArg2,
                     PVOID         pvArg3,
                     PVOID         pvArg4,
                     PVOID         pvArg5)
{
    INTREG           iregInterLevel;
    PLW_JOB_MSG      pjobmsg;
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    if (pjobq->JOBQ_uiCnt == pjobq->JOBQ_uiSize) {
        pjobq->JOBQ_ulLost++;
        LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "job message lost.\r\n");
        return  (ENOSPC);
    }
    
    pjobmsg                 = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiIn];
    pjobmsg->JOBM_pfuncFunc = pfunc;
    pjobmsg->JOBM_pvArg[0]  = pvArg0;
    pjobmsg->JOBM_pvArg[1]  = pvArg1;
    pjobmsg->JOBM_pvArg[2]  = pvArg2;
    pjobmsg->JOBM_pvArg[3]  = pvArg3;
    pjobmsg->JOBM_pvArg[4]  = pvArg4;
    pjobmsg->JOBM_pvArg[5]  = pvArg5;
    
    if (pjobq->JOBQ_uiIn == (pjobq->JOBQ_uiSize - 1)) {
        pjobq->JOBQ_uiIn =  0;
    } else {
        pjobq->JOBQ_uiIn++;
    }
    
    pjobq->JOBQ_uiCnt++;
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
    
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBPost(pjobq->JOBQ_ulSync);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _JobQueueDel
** ��������: �ӹ���������ɾ��һ������ (����ɾ��)
** �䡡��  : pjobq         �������п��ƿ�
**           uiMatchArgNum ƥ������ĸ���
**           pfunc         Ҫɾ���ĺ���
**           pvArg0 ~ 5    ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _jobQueueDel (PLW_JOB_QUEUE pjobq,
                    UINT          uiMatchArgNum,
                    VOIDFUNCPTR   pfunc, 
                    PVOID         pvArg0,
                    PVOID         pvArg1,
                    PVOID         pvArg2,
                    PVOID         pvArg3,
                    PVOID         pvArg4,
                    PVOID         pvArg5)
{
    INTREG          iregInterLevel;
    UINT            i = 0;
    PLW_JOB_MSG     pjobmsg;
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    pjobmsg = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiOut];
    for (i = 0; i < pjobq->JOBQ_uiCnt; i++) {
        switch (uiMatchArgNum) {
        
        case 0:
            if (pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc) {
                pjobmsg->JOBM_pfuncFunc =  LW_NULL;
            }
            break;
            
        case 1:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc) &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 2:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 3:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 4:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 5:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3) &&
                (pjobmsg->JOBM_pvArg[4]  == pvArg4)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 6:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3) &&
                (pjobmsg->JOBM_pvArg[4]  == pvArg4) &&
                (pjobmsg->JOBM_pvArg[5]  == pvArg5)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
        }
        pjobmsg++;
        if (pjobmsg > &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiSize - 1]) {
            pjobmsg = &pjobq->JOBQ_pjobmsgQueue[0];
        }
    }
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: _jobQueueLost
** ��������: ��ù������ж�ʧ��Ϣ����
** �䡡��  : pjobq         �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _jobQueueLost (PLW_JOB_QUEUE pjobq)
{
    return  (pjobq->JOBQ_ulLost);
}
/*********************************************************************************************************
** ��������: _jobQueueExec
** ��������: ִ�й��������еĹ���
** �䡡��  : pjobq         �������п��ƿ�
**           ulTimeout     �ȴ���ʱʱ��
** �䡡��  : ��Ϊ ERROR_NONE ��ʾ��ʱ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _jobQueueExec (PLW_JOB_QUEUE pjobq, ULONG  ulTimeout)
{
    INTREG          iregInterLevel;
    PLW_JOB_MSG     pjobmsg;
    LW_JOB_MSG      jobmsgRun;
    
    if (pjobq->JOBQ_ulSync) {
        for (;;) {
            if (API_SemaphoreBPend(pjobq->JOBQ_ulSync, ulTimeout)) {
                return  (ERROR_THREAD_WAIT_TIMEOUT);
            }
            LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
            if (pjobq->JOBQ_uiCnt) {
                break;
            }
            LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        }
    } else {
        LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    }
    
    while (pjobq->JOBQ_uiCnt) {
        pjobmsg = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiOut];
        LW_JOB_MSG_COPY(&jobmsgRun, pjobmsg);
        if (pjobq->JOBQ_uiOut == (pjobq->JOBQ_uiSize - 1)) {
            pjobq->JOBQ_uiOut =  0;
        } else {
            pjobq->JOBQ_uiOut++;
        }
        pjobq->JOBQ_uiCnt--;
        LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        
        if (jobmsgRun.JOBM_pfuncFunc) {
            jobmsgRun.JOBM_pfuncFunc(jobmsgRun.JOBM_pvArg[0],
                                     jobmsgRun.JOBM_pvArg[1],
                                     jobmsgRun.JOBM_pvArg[2],
                                     jobmsgRun.JOBM_pvArg[3],
                                     jobmsgRun.JOBM_pvArg[4],
                                     jobmsgRun.JOBM_pvArg[5]);
        }
        
        LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    }
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
