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
** ��   ��   ��: WorkQueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 09 �� 07 ��
**
** ��        ��: �ں˹�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_WORKQUEUE_EN > 0
/*********************************************************************************************************
  ����ҵ���п��ƽṹ
*********************************************************************************************************/
typedef struct {
    PLW_JOB_QUEUE        SWQ_pjobQ;                                     /*  ���ӳ���ҵ����              */
} LW_WORK_SQUEUE;
/*********************************************************************************************************
  �����ӳ�������ҵ���п��ƽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_MONO         DWQN_monoList;
    LW_CLASS_WAKEUP_NODE DWQN_wun;
    VOIDFUNCPTR          DWQN_pfunc;
    PVOID                DWQN_pvArg[LW_JOB_ARGS];
} LW_WORK_DNODE;

typedef struct {
    LW_CLASS_WAKEUP      DWQ_wakeup;                                    /*  �ӳ���ҵ����                */
    PLW_LIST_MONO        DWQ_pmonoPool;                                 /*  �ڵ��                      */
    LW_WORK_DNODE       *DWQ_pwdnPool;
    
    LW_OBJECT_HANDLE     DWQ_ulLock;                                    /*  ������                      */
    LW_OBJECT_HANDLE     DWQ_ulSem;                                     /*  ֪ͨ�ź���                  */
    
    UINT                 DWQ_uiCount;                                   /*  �ȴ�����ҵ����              */
    UINT64               DWQ_u64Time;                                   /*  ���һ��ִ��ʱ���          */
    ULONG                DWQ_ulScanPeriod;                              /*  ѭ��ɨ������                */
} LW_WORK_DQUEUE;
/*********************************************************************************************************
  ��ҵ���п��ƽṹ
*********************************************************************************************************/
typedef struct {
    union {
        LW_WORK_SQUEUE   WQ_sq;
        LW_WORK_DQUEUE   WQ_dq;
    } q;
    
#define LW_WQ_TYPE_S     0                                              /*  ���������                */
#define LW_WQ_TYPE_D     1                                              /*  �����ӳ����Ե��������      */
    INT                  WQ_iType;                                      /*  ��������                    */
    LW_OBJECT_HANDLE     WQ_ulTask;                                     /*  �����߳�                    */
    BOOL                 WQ_bDelReq;
} LW_WORK_QUEUE;
typedef LW_WORK_QUEUE   *PLW_WORK_QUEUE;
/*********************************************************************************************************
** ��������: __wqSCreate
** ��������: ����һ���򵥹�������
** �䡡��  : pwq           �������п��ƿ�
**           uiQSize       ���д�С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_WORK_QUEUE  __wqSCreate (PLW_WORK_QUEUE  pwq, UINT  uiQSize)
{
    pwq->WQ_iType = LW_WQ_TYPE_S;
    pwq->q.WQ_sq.SWQ_pjobQ = _jobQueueCreate(uiQSize, LW_FALSE);
    if (pwq->q.WQ_sq.SWQ_pjobQ == LW_NULL) {
        return  (LW_NULL);
    }
    
    return  (pwq);
}
/*********************************************************************************************************
** ��������: __wqDCreate
** ��������: ����һ�������ӳٹ��ܵĹ�������
** �䡡��  : pwq           �������п��ƿ�
**           uiQSize       ���д�С
**           ulScanPeriod  �����߳�ɨ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_WORK_QUEUE  __wqDCreate (PLW_WORK_QUEUE  pwq, UINT  uiQSize, ULONG  ulScanPeriod)
{
    UINT            i;
    LW_WORK_DNODE  *pwdn;
    
    pwq->WQ_iType = LW_WQ_TYPE_D;
    pwdn = (LW_WORK_DNODE *)__KHEAP_ALLOC(sizeof(LW_WORK_DNODE) * uiQSize);
    if (pwdn == LW_NULL) {
        return  (LW_NULL);
    }
    pwq->q.WQ_dq.DWQ_pwdnPool = pwdn;
    
    pwq->q.WQ_dq.DWQ_ulLock = API_SemaphoreMCreate("wqd_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE | 
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pwq->q.WQ_dq.DWQ_ulLock == LW_OBJECT_HANDLE_INVALID) {
        __KHEAP_FREE(pwdn);
        return  (LW_NULL);
    }
    
    pwq->q.WQ_dq.DWQ_ulSem = API_SemaphoreBCreate("wqd_sem", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pwq->q.WQ_dq.DWQ_ulSem == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreMDelete(&pwq->q.WQ_dq.DWQ_ulLock);
        __KHEAP_FREE(pwdn);
        return  (LW_NULL);
    }
    
    for (i = 0; i < uiQSize; i++) {
        _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
        pwdn++;
    }
    
    pwq->q.WQ_dq.DWQ_u64Time      = API_TimeGet64();
    pwq->q.WQ_dq.DWQ_ulScanPeriod = ulScanPeriod;
    
    return  (pwq);
}
/*********************************************************************************************************
** ��������: __wqSDelete
** ��������: ɾ��һ���򵥹�������
** �䡡��  : pwq           �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqSDelete (PLW_WORK_QUEUE  pwq)
{
    _jobQueueDelete(pwq->q.WQ_sq.SWQ_pjobQ);
}
/*********************************************************************************************************
** ��������: __wqDDelete
** ��������: ɾ��һ�������ӳٹ��ܵĹ�������
** �䡡��  : pwq           �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqDDelete (PLW_WORK_QUEUE  pwq)
{
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    API_SemaphoreMDelete(&pwq->q.WQ_dq.DWQ_ulLock);
    API_SemaphoreBDelete(&pwq->q.WQ_dq.DWQ_ulSem);
    
    __KHEAP_FREE(pwq->q.WQ_dq.DWQ_pwdnPool);
}
/*********************************************************************************************************
** ��������: __wqSFlush
** ��������: ���һ���򵥹�������
** �䡡��  : pwq           �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqSFlush (PLW_WORK_QUEUE  pwq)
{
    _jobQueueFlush(pwq->q.WQ_sq.SWQ_pjobQ);
}
/*********************************************************************************************************
** ��������: __wqDFlush
** ��������: ���һ�������ӳٹ��ܵĹ�������
** �䡡��  : pwq           �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqDFlush (PLW_WORK_QUEUE  pwq)
{
    PLW_CLASS_WAKEUP_NODE   pwun;
    LW_WORK_DNODE          *pwdn;
    
    API_SemaphoreBClear(pwq->q.WQ_dq.DWQ_ulSem);
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    while (pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader) {
        pwun = _LIST_ENTRY(pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader, 
                           LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        pwdn = _LIST_ENTRY(pwun, LW_WORK_DNODE, DWQN_wun);
    
        _List_Line_Del(&pwun->WUN_lineManage, 
                       &pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader);
        _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
    }
    
    pwq->q.WQ_dq.DWQ_uiCount = 0;
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
}
/*********************************************************************************************************
** ��������: __wqSInsert
** ��������: ��һ���������뵽��������
** �䡡��  : pwq          �������п��ƿ�
**           pfunc        ִ�к���
**           pvArg0 ~ 5   ִ�в���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __wqSInsert (PLW_WORK_QUEUE  pwq, 
                           VOIDFUNCPTR     pfunc, 
                           PVOID           pvArg0,
                           PVOID           pvArg1,
                           PVOID           pvArg2,
                           PVOID           pvArg3,
                           PVOID           pvArg4,
                           PVOID           pvArg5)
{
    return  (_jobQueueAdd(pwq->q.WQ_sq.SWQ_pjobQ,
                          pfunc, pvArg0, pvArg1, pvArg2,
                          pvArg3, pvArg4, pvArg5));
}
/*********************************************************************************************************
** ��������: __wqDInsert
** ��������: ��һ���������뵽��������
** �䡡��  : pwq          �������п��ƿ�
**           ulDelay      ��С�ӳ�ִ��ʱ��
**           pfunc        ִ�к���
**           pvArg0 ~ 5   ִ�в���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __wqDInsert (PLW_WORK_QUEUE  pwq, 
                           ULONG           ulDelay,
                           VOIDFUNCPTR     pfunc, 
                           PVOID           pvArg0,
                           PVOID           pvArg1,
                           PVOID           pvArg2,
                           PVOID           pvArg3,
                           PVOID           pvArg4,
                           PVOID           pvArg5)
{
    PLW_LIST_MONO   pmonoWDN;
    LW_WORK_DNODE  *pwdn;
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    if (!pwq->q.WQ_dq.DWQ_pmonoPool) {
        API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
        _ErrorHandle(ENOSPC);
        return  (ENOSPC);

    } else {
        pmonoWDN = _list_mono_allocate(&pwq->q.WQ_dq.DWQ_pmonoPool);
    }

    pwdn = _LIST_ENTRY(pmonoWDN, LW_WORK_DNODE, DWQN_monoList);
    pwdn->DWQN_pfunc    = pfunc;
    pwdn->DWQN_pvArg[0] = pvArg0;
    pwdn->DWQN_pvArg[1] = pvArg1;
    pwdn->DWQN_pvArg[2] = pvArg2;
    pwdn->DWQN_pvArg[3] = pvArg3;
    pwdn->DWQN_pvArg[4] = pvArg4;
    pwdn->DWQN_pvArg[5] = pvArg5;
    
    pwdn->DWQN_wun.WUN_ulCounter = ulDelay;
    _WakeupAdd(&pwq->q.WQ_dq.DWQ_wakeup, &pwdn->DWQN_wun, LW_FALSE);
    pwq->q.WQ_dq.DWQ_uiCount++;
    
    API_SemaphoreBPost(pwq->q.WQ_dq.DWQ_ulSem);
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __wqSStatus
** ��������: ��ȡ��������״̬
** �䡡��  : pwq          �������п��ƿ�
**           pulCount     ��ǰ��������ҵ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqSStatus (PLW_WORK_QUEUE  pwq, UINT  *puiCount)
{
    *puiCount = pwq->q.WQ_sq.SWQ_pjobQ->JOBQ_uiCnt;
}
/*********************************************************************************************************
** ��������: __wqDStatus
** ��������: ��ȡ��������״̬
** �䡡��  : pwq          �������п��ƿ�
**           pulCount     ��ǰ��������ҵ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqDStatus (PLW_WORK_QUEUE  pwq, UINT  *puiCount)
{
    *puiCount = pwq->q.WQ_dq.DWQ_uiCount;
}
/*********************************************************************************************************
** ��������: __wqSExec
** ��������: ִ��һ�ι�������
** �䡡��  : pwq          �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqSExec (PLW_WORK_QUEUE  pwq)
{
    _jobQueueExec(pwq->q.WQ_sq.SWQ_pjobQ, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
** ��������: __wqDExec
** ��������: ִ��һ�ι�������
** �䡡��  : pwq          �������п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __wqDExec (PLW_WORK_QUEUE  pwq)
{
    UINT                   i;
    PLW_CLASS_WAKEUP_NODE  pwun;
    LW_WORK_DNODE         *pwdn;
    
    VOIDFUNCPTR            pfunc;
    PVOID                  pvArg[LW_JOB_ARGS];
    
    UINT64                 u64Now;
    ULONG                  ulCounter;

    API_SemaphoreBPend(pwq->q.WQ_dq.DWQ_ulSem, pwq->q.WQ_dq.DWQ_ulScanPeriod);
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    u64Now = API_TimeGet64();
    ulCounter = (ULONG)(u64Now - pwq->q.WQ_dq.DWQ_u64Time);
    pwq->q.WQ_dq.DWQ_u64Time = u64Now;
    
    __WAKEUP_PASS_FIRST(&pwq->q.WQ_dq.DWQ_wakeup, pwun, ulCounter);
    
    pwdn = _LIST_ENTRY(pwun, LW_WORK_DNODE, DWQN_wun);
    
    _WakeupDel(&pwq->q.WQ_dq.DWQ_wakeup, pwun, LW_FALSE);
    pwq->q.WQ_dq.DWQ_uiCount--;
    
    pfunc = pwdn->DWQN_pfunc;
    for (i = 0; i < LW_JOB_ARGS; i++) {
        pvArg[i] = pwdn->DWQN_pvArg[i];
    }
    
    _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
    
    if (pfunc) {
        pfunc(pvArg[0], pvArg[1], pvArg[2], pvArg[3], pvArg[4], pvArg[5]);
    }
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    __WAKEUP_PASS_SECOND();
    
    __WAKEUP_PASS_END();
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
}
/*********************************************************************************************************
** ��������: __wqTask
** ��������: �������з����߳�
** �䡡��  : pvArg        �������п��ƿ�
** �䡡��  : NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __wqTask (PVOID  pvArg)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvArg;
    
    for (;;) {
        switch (pwq->WQ_iType) {
        
        case LW_WQ_TYPE_S:
            __wqSExec(pwq);
            break;
            
        case LW_WQ_TYPE_D:
            __wqDExec(pwq);
            break;
            
        default:
            break;
        }
        
        if (pwq->WQ_bDelReq) {
            break;
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_WorkQueueCreate
** ��������: ����һ����������
** �䡡��  : pcName            ��������
**           uiQSize           ���д�С
**           bDelayEn          �Ƿ񴴽������ӳ�ִ�й��ܵĹ�������
**           ulScanPeriod      ��������ӳ�ѡ��, �˲���ָ�������߳�ɨ������.
**           pthreadattr       ���з����߳�ѡ��
** �䡡��  : �������о��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������Ҫ�ӳ�ִ�й���, ��ϵͳ�Զ�����Ϊ�򵥹�������, �ڴ�ռ����С, ִ��Ч�ʸ�.

                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_WorkQueueCreate (CPCHAR                  pcName,
                            UINT                    uiQSize, 
                            BOOL                    bDelayEn, 
                            ULONG                   ulScanPeriod, 
                            PLW_CLASS_THREADATTR    pthreadattr)
{
    PLW_WORK_QUEUE         pwq;
    LW_CLASS_THREADATTR    threadattr;
    
    if (!uiQSize || (bDelayEn && !ulScanPeriod)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pwq = (PLW_WORK_QUEUE)__KHEAP_ALLOC(sizeof(LW_WORK_QUEUE));
    if (pwq == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pwq, sizeof(LW_WORK_QUEUE));
    
    if (bDelayEn) {
        if (__wqDCreate(pwq, uiQSize, ulScanPeriod) == LW_NULL) {
            __KHEAP_FREE(pwq);
            return  (LW_NULL);
        }
    
    } else {
        if (__wqSCreate(pwq, uiQSize) == LW_NULL) {
            __KHEAP_FREE(pwq);
            return  (LW_NULL);
        }
    }
    
    if (pthreadattr) {
        threadattr = *pthreadattr;
    
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_ulOption &= ~LW_OPTION_THREAD_DETACHED;
    threadattr.THREADATTR_ulOption |= LW_OPTION_OBJECT_GLOBAL;
    threadattr.THREADATTR_pvArg     = pwq;
    
    pwq->WQ_ulTask = API_ThreadCreate(pcName, __wqTask, &threadattr, LW_NULL);
    if (pwq->WQ_ulTask == LW_OBJECT_HANDLE_INVALID) {
        if (bDelayEn) {
            __wqDDelete(pwq);
        
        } else {
            __wqSDelete(pwq);
        }
        __KHEAP_FREE(pwq);
        return  (LW_NULL);
    }
    
    return  ((PVOID)pwq);
}
/*********************************************************************************************************
** ��������: API_WorkQueueDelete
** ��������: ɾ��һ����������
** �䡡��  : pvWQ      �������о��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueDelete (PVOID  pvWQ)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    KN_SMP_MB();
    if (pwq->WQ_bDelReq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pwq->WQ_bDelReq = LW_TRUE;
    KN_SMP_MB();
    
    switch (pwq->WQ_iType) {
    
    case LW_WQ_TYPE_S:
        __wqSFlush(pwq);
        __wqSInsert(pwq, LW_NULL, 0, 0, 0, 0, 0, 0);
        API_ThreadJoin(pwq->WQ_ulTask, LW_NULL);
        __wqSDelete(pwq);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDFlush(pwq);
        __wqDInsert(pwq, 0, LW_NULL, 0, 0, 0, 0, 0, 0);
        API_ThreadJoin(pwq->WQ_ulTask, LW_NULL);
        __wqDDelete(pwq);
        break;

    default:
        break;
    }
    
    __KHEAP_FREE(pwq);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_WorkQueueInsert
** ��������: ��һ���������뵽��������
** �䡡��  : pvWQ         �������о��
**           ulDelay      ��С�ӳ�ִ��ʱ��
**           pfunc        ִ�к���
**           pvArg0 ~ 5   ִ�в���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueInsert (PVOID           pvWQ, 
                            ULONG           ulDelay,
                            VOIDFUNCPTR     pfunc, 
                            PVOID           pvArg0,
                            PVOID           pvArg1,
                            PVOID           pvArg2,
                            PVOID           pvArg3,
                            PVOID           pvArg4,
                            PVOID           pvArg5)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    ULONG           ulRet;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        ulRet = __wqSInsert(pwq, pfunc, pvArg0, pvArg1, pvArg2,
                            pvArg3, pvArg4, pvArg5);
        break;
        
    case LW_WQ_TYPE_D:
        ulRet = __wqDInsert(pwq, ulDelay, pfunc, pvArg0, pvArg1, pvArg2,
                            pvArg3, pvArg4, pvArg5);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        ulRet = ENXIO;
        break;
    }
    
    return  (ulRet);
}
/*********************************************************************************************************
** ��������: API_WorkQueueFlush
** ��������: ��չ�������
** �䡡��  : pvWQ         �������о��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueFlush (PVOID  pvWQ)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        __wqSFlush(pwq);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDFlush(pwq);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        return  (ENXIO);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_WorkQueueStatus
** ��������: ��ȡ��������״̬
** �䡡��  : pvWQ         �������о��
**           puiCount     ��ǰ��������ҵ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueStatus (PVOID  pvWQ, UINT  *puiCount)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq || !puiCount) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        __wqSStatus(pwq, puiCount);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDStatus(pwq, puiCount);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        return  (ENXIO);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_WORKQUEUE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
