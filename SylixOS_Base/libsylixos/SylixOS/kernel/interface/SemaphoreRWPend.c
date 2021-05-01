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
** ��   ��   ��: SemaphoreRWPend.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 20 ��
**
** ��        ��: �ȴ���д�ź���.
**
** BUG
2019.05.10  ��ȫģʽ���ñ������ȡ����ͬһԭ��״̬.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
ulTimeout ȡֵ��
    
    LW_OPTION_NOT_WAIT                       �����еȴ�
    LW_OPTION_WAIT_A_TICK                    �ȴ�һ��ϵͳʱ��
    LW_OPTION_WAIT_A_SECOND                  �ȴ�һ��
    LW_OPTION_WAIT_INFINITE                  ��Զ�ȴ���ֱ������Ϊֹ
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_SemaphoreRWPendR
** ��������: �ȴ���д�ź�����, ���� rw post �����������ж��н��У��ɴ�����̹��ж�ʱ��
** �䡡��  : 
**           ulId            �¼����
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreRWPendR (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ��û������              */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if ((pevent->EVENT_iStatus == EVENT_RW_STATUS_R) &&
        (_EventWaitNum(EVENT_RW_Q_W, pevent) == 0)) {                   /*  ��ǰΪ��״̬����û��д����  */
        pevent->EVENT_ulCounter++;                                      /*  ������++                    */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  ��ȫģʽ�趨                */
            LW_THREAD_SAFE_INKERN(ptcbCur);
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_RW_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SEM;                 /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  �����ȼ��ȴ�                */
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_RW_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
        
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_RW_Q_R, ppringList);                    /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  ����������                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  ���¼��㳬ʱʱ��            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            return  (ERROR_NONE);                                       /*  �ͷŲ����ѽ���������Ϊ��ȫ  */
        
        } else {
            _ErrorHandle(ERROR_EVENT_WAS_DELETED);                      /*  �Ѿ���ɾ��                  */
            return  (ERROR_EVENT_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** ��������: API_SemaphoreRWPendW
** ��������: �ȴ���д�ź���д, ���� rw post �����������ж��н��У��ɴ�����̹��ж�ʱ��
** �䡡��  : 
**           ulId            �¼����
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_SemaphoreRWPendW (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ��û������              */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter == 0) {                                 /*  ��ǰû���κβ���            */
        pevent->EVENT_ulCounter++;
        pevent->EVENT_iStatus  = EVENT_RW_STATUS_W;
        pevent->EVENT_pvTcbOwn = (PVOID)ptcbCur;                        /*  �����߳���Ϣ                */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  ��ȫģʽ�趨                */
            LW_THREAD_SAFE_INKERN(ptcbCur);
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }
    
    if (pevent->EVENT_pvTcbOwn == (PVOID)ptcbCur) {                     /*  д�ݹ�                      */
        pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr + 1);  /*  ��ʱ������++                */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }

    if (ulTimeout == LW_OPTION_NOT_WAIT) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_RW_Q_W;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SEM;                 /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  �����ȼ��ȴ�                */
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_RW_Q_W, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
        
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_RW_Q_W, ppringList);                    /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  ����������                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  ���¼��㳬ʱʱ��            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            return  (ERROR_NONE);                                       /*  �ͷŲ����ѽ���������Ϊ��ȫ  */
        
        } else {
            _ErrorHandle(ERROR_EVENT_WAS_DELETED);                      /*  �Ѿ���ɾ��                  */
            return  (ERROR_EVENT_WAS_DELETED);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
