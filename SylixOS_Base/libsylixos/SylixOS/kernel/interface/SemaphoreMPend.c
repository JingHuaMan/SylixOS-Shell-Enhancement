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
** ��   ��   ��: SemaphoreMPend.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �ȴ������ź���

** BUG
2007.05.07  ��ʹ�����ȼ��컨��ʱ��_ThreadMove() ǰ��Ӧ���ж� Mutex ӵ���ߵ����ȼ��Ƿ����컨����ͬ��
2007.07.21  ���� _DebugHandle() ���ܡ�
2007.10.28  �����ڵ��������رյ�����µ���.
2008.01.20  �������Ѿ������ش�ĸĽ�, �����ڵ�����������������µ��ô� API.
2008.03.30  ����ʹ���˷���Ĺرյ�������ʽ, ���Զ��߳����Կ�Ĳ���������ڹر��жϵ������.
2008.05.03  �����ź������������ restart �������������Ĵ���.
2008.05.18  ȥ���� LW_EVENT_EXIST, �����������ط�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.08.26  �����������, ��ʽ 25 ����, �е����˰�! 
            �� MUTEX �ź��������Ľ�, �����߳��������� pend ����ʱ, ��������, 
            ��ֻ�������ͷŵ��õĴ�����, �Ż��ͷ�.
2009.04.08  ����� SMP ��˵�֧��.
2009.05.28  �ԴӼ�����֧��, �ر��ж�ʱ���ӳ���, �������һЩ�Ż�.
2009.06.05  �ϸ��������Ż���һ����������˺ܾ�, ���ǳ�ʱ�˳�ʱӦ�ù��ж�.
2009.10.11  ��TCB_ulWakeTimer��ֵ��ulTimeSave��ֵ��ǰ. ���ڵȴ������жϷ�֧ǰ��.
2010.08.03  ʹ���µĻ�ȡϵͳʱ�ӷ���.
2011.02.23  ���� LW_OPTION_SIGNAL_INTER ѡ��, �¼�����ѡ���Լ��Ƿ�ɱ��жϴ��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2012.12.08  ����֧�ֵݹ�, ��֧�ֲ����ͼ��.
2013.05.05  �жϵ���������ֵ, �������������û����˳�.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.12.11  ϵͳû������ʱ pend ����������.
2014.05.29  �޸���ʱ��˲�伤��ʱû����������ȫ���Դ���.
2019.05.10  ��ȫģʽ���ñ������ȡ����ͬһԭ��״̬.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ע��:
       MUTEX ��ͬ�� COUNTING, BINERY, һ���̱߳���ɶ�ʹ��.
       
       API_SemaphoreMPend();
       ... (do something as fast as possible...)
       API_SemaphoreMPost();
*********************************************************************************************************/
/*********************************************************************************************************
ulTimeout ȡֵ��
    
    LW_OPTION_NOT_WAIT                       �����еȴ�
    LW_OPTION_WAIT_A_TICK                    �ȴ�һ��ϵͳʱ��
    LW_OPTION_WAIT_A_SECOND                  �ȴ�һ��
    LW_OPTION_WAIT_INFINITE                  ��Զ�ȴ���ֱ������Ϊֹ
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_SemaphoreMPend
** ��������: �ȴ������ź���            ���� mutext post �����������ж��н��У��ɴ�����̹��ж�ʱ��
** �䡡��  : 
**           ulId            �¼����
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMPend (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
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
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_M)) {
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MUTEX)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        pevent->EVENT_ulCounter    = LW_FALSE;
        pevent->EVENT_ulMaxCounter = (ULONG)ptcbCur->TCB_ucPriority;
        pevent->EVENT_pvTcbOwn     = (PVOID)ptcbCur;                    /*  �����߳���Ϣ                */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  ��ȫģʽ�趨                */
            LW_THREAD_SAFE_INKERN(ptcbCur);
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }
    
    if (!(pevent->EVENT_ulOption & LW_OPTION_NORMAL)) {                 /*  ��Ҫ�ݹ�֧�ֻ�ݹ���      */
        if (pevent->EVENT_pvTcbOwn == (PVOID)ptcbCur) {                 /*  �Ƿ����Լ���������          */
            if (pevent->EVENT_ulOption & LW_OPTION_ERRORCHECK) {
                __KERNEL_EXIT();                                        /*  �˳��ں�                    */
                _ErrorHandle(EDEADLK);                                  /*  �˳�                        */
                return  (EDEADLK);
            }
                                                                        /*  ��ʱ������++                */
            pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr + 1);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            return  (ERROR_NONE);
        }
    }

    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  ���ȴ�                      */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    _EventPrioTryBoost(pevent, ptcbCur);                                /*  �������������������ȼ�      */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
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
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_PEND, 
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

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
