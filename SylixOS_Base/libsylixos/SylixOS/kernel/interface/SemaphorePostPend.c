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
** ��   ��   ��: SemaphorePostPend.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 16 ��
**
** ��        ��: �� SMP ϵͳ�½��ͬ�����ͷ�һ���ź�����ͬʱ��ʼ��ȡ����һ���ź���.

** BUG:
2010.08.03  ʹ���µĻ�ȡϵͳʱ�ӷ���.
2011.02.23  ���� LW_OPTION_SIGNAL_INTER ѡ��, �¼�����ѡ���Լ��Ƿ�ɱ��жϴ��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.05.05  �жϵ���������ֵ, �������������û����˳�.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2018.09.27  �����źŻ����ں˼�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphorePostBPend
** ��������: �ͷ�һ���ź�����������ʼ�ȴ�����һ���ź���(�м��������л�����)
** �䡡��  : ulIdPost             ��Ҫ�ͷŵ��ź���
**           ulId                 ��Ҫ�ȴ����ź��� (��ֵ�ź���)
**           ulTimeout            �ȴ���ʱʱ��            
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if ((LW_CFG_SEMB_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePostBPend (LW_OBJECT_HANDLE  ulIdPost, 
                               LW_OBJECT_HANDLE  ulId,
                               ULONG             ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER ULONG                 ulObjectClass;
             ULONG                 ulError;
    
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */

    usIndex       = _ObjectGetIndex(ulIdPost);
    ulObjectClass = _ObjectGetClass(ulIdPost);
    
    if ((ulObjectClass != _OBJECT_SEM_B) &&
        (ulObjectClass != _OBJECT_SEM_C) &&
        (ulObjectClass != _OBJECT_SEM_M)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    ulError = API_SemaphorePost(ulIdPost);                              /*  �ͷ��ź���                  */
    if (ulError) {
        __KERNEL_EXIT();
        return  (ulError);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
    if (_Event_Index_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
#endif
    pevent = &_K_eventBuffer[usIndex];

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_EVENT_TYPE;
        goto    __wait_over;
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        pevent->EVENT_ulCounter = LW_FALSE;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        ulError = ERROR_NONE;
        goto    __wait_over;
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  ���ȴ�                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  ��ʱ                        */
        goto    __wait_over;
    }
    
    ptcbCur->TCB_iPendQ         = EVENT_SEM_Q;
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
        _EVENT_PRIORITY_Q_PTR(EVENT_SEM_Q, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
    
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_SEM_Q, ppringList);                     /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_PEND, 
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
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  ��ʱ                        */
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            ulError = ERROR_NONE;                                       /*  ����                        */
        
        } else {
            ulError = ERROR_EVENT_WAS_DELETED;                          /*  �Ѿ���ɾ��                  */
        }
    }
    
__wait_over:
    _ErrorHandle(ulError);
    return  (ulError);
}

#endif                                                                  /*  ((LW_CFG_SEMB_EN > 0) ||    */
                                                                        /*   (LW_CFG_SEMM_EN > 0)) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** ��������: API_SemaphorePostCPend
** ��������: �ͷ�һ���ź�����������ʼ�ȴ�����һ���ź���(�м��������л�����)
** �䡡��  : ulIdPost             ��Ҫ�ͷŵ��ź���
**           ulId                 ��Ҫ�ȴ����ź��� (�����ź���)
**           ulTimeout            �ȴ���ʱʱ��            
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if ((LW_CFG_SEMC_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePostCPend (LW_OBJECT_HANDLE  ulIdPost, 
                               LW_OBJECT_HANDLE  ulId,
                               ULONG             ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER ULONG                 ulObjectClass;
             ULONG                 ulError;
    
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */

    usIndex       = _ObjectGetIndex(ulIdPost);
    ulObjectClass = _ObjectGetClass(ulIdPost);
    
    if ((ulObjectClass != _OBJECT_SEM_B) &&
        (ulObjectClass != _OBJECT_SEM_C) &&
        (ulObjectClass != _OBJECT_SEM_M)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    ulError = API_SemaphorePost(ulIdPost);                              /*  �ͷ��ź���                  */
    if (ulError) {
        __KERNEL_EXIT();
        return  (ulError);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_C)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
    if (_Event_Index_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
#endif
    pevent = &_K_eventBuffer[usIndex];

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMC)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_EVENT_TYPE;
        goto    __wait_over;
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        pevent->EVENT_ulCounter--;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        ulError = ERROR_NONE;
        goto    __wait_over;
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  ���ȴ�                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  ��ʱ                        */
        goto    __wait_over;
    }
    
    ptcbCur->TCB_iPendQ         = EVENT_SEM_Q;
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
        _EVENT_PRIORITY_Q_PTR(EVENT_SEM_Q, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
    
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_SEM_Q, ppringList);                     /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMC, MONITOR_EVENT_SEM_PEND, 
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
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  ��ʱ                        */
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            ulError = ERROR_NONE;                                       /*  ����                        */
        
        } else {
            ulError = ERROR_EVENT_WAS_DELETED;                          /*  �Ѿ���ɾ��                  */
        }
    }
    
__wait_over:
    _ErrorHandle(ulError);
    return  (ulError);
}

#endif                                                                  /*  ((LW_CFG_SEMC_EN > 0) ||    */
                                                                        /*   (LW_CFG_SEMM_EN > 0)) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
