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
** ��   ��   ��: MsgQueueReceive.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: �ȴ���Ϣ������Ϣ

** BUG
2007.09.19  ���� _DebugHandle() ���ܡ�
2007.10.28  �����ڵ�������������µ���.
2008.01.20  �������Ѿ������ش�ĸĽ�, �����ڵ�����������������µ��ô� API.
2008.03.30  ����ʹ���˷���Ĺرյ�������ʽ, ���Զ��߳����Կ�Ĳ���������ڹر��жϵ������.
2008.05.18  �����ź������������ restart �������������Ĵ���.
2008.05.18  ȥ���� LW_EVENT_EXIST, �����������ط�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
2009.04.15  �����˳�ʱ����, ȫ�汣����˻���.
2009.05.28  �ԴӼ�����֧��, �ر��ж�ʱ���ӳ���, �������һЩ�Ż�.
2009.06.05  �ϸ��������Ż���һ����������˺ܾ�, ���ǳ�ʱ�˳�ʱӦ�ù��ж�.
2009.06.25  pulMsgLen ����Ϊ NULL,
2009.10.11  ��TCB_ulWakeTimer��ֵ��ulTimeSave��ֵ��ǰ. ���ڵȴ������жϷ�֧ǰ��.
2010.08.03  �޸Ļ�ȡϵͳʱ�ӵķ�ʽ.
2011.02.23  ���� LW_OPTION_SIGNAL_INTER ѡ��, �¼�����ѡ���Լ��Ƿ�ɱ��жϴ��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.03.17  ���� API_MsgQueueReceiveEx �������ý���ѡ��.
2013.05.05  �жϵ���������ֵ, �������������û����˳�.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2014.05.29  �޸���ʱ��˲�䱻����ʱ����Ϣ���жϴ���.
2016.07.21  ��Ҫ����д�ȴ�������.
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
** ��������: API_MsgQueueReceive
** ��������: �ȴ���Ϣ������Ϣ
** �䡡��  : 
**           ulId            ��Ϣ���о��
**           pvMsgBuffer     ��Ϣ������
**           stMaxByteSize   ��Ϣ��������С
**           pstMsgLen       ��Ϣ����
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueReceive (LW_OBJECT_HANDLE    ulId,
                            PVOID               pvMsgBuffer,
                            size_t              stMaxByteSize,
                            size_t             *pstMsgLen,
                            ULONG               ulTimeout)

{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             size_t                stMsgLenTemp;                        /*  ��ʱ��¼����                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  ��ʱ������¼��Ϣ����        */
    }
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer || !stMaxByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    ptcbCur->TCB_ulRecvOption = LW_OPTION_NOERROR;                      /*  ���մ���Ϣ�Զ��ض�          */
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        pevent->EVENT_ulCounter--;
        _MsgQueueGet(pmsgqueue, pvMsgBuffer, 
                     stMaxByteSize, pstMsgLen);                         /*  �����Ϣ                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  �������ڵȴ�д��Ϣ          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  ʹ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  ���� TCB                    */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
        }
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  ���ȴ�                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    ptcbCur->TCB_pstMsgByteSize    = pstMsgLen;
    ptcbCur->TCB_stMaxByteSize     = stMaxByteSize;
    ptcbCur->TCB_pvMsgQueueMessage = pvMsgBuffer;                       /*  ��¼��Ϣ                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;            /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
        
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_R, ppringList);                   /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_PEND, 
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
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            return  (ERROR_NONE);
        
        } else {
            _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);                   /*  �Ѿ���ɾ��                  */
            return  (ERROR_MSGQUEUE_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** ��������: API_MsgQueueReceiveEx
** ��������: �ȴ���Ϣ������Ϣ
** �䡡��  : 
**           ulId            ��Ϣ���о��
**           pvMsgBuffer     ��Ϣ������
**           stMaxByteSize   ��Ϣ��������С
**           pstMsgLen       ��Ϣ����
**           ulTimeout       �ȴ�ʱ��
**           ulOption        ����ѡ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueReceiveEx (LW_OBJECT_HANDLE    ulId,
                              PVOID               pvMsgBuffer,
                              size_t              stMaxByteSize,
                              size_t             *pstMsgLen,
                              ULONG               ulTimeout,
                              ULONG               ulOption)
{
    
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             size_t                stMsgLenTemp;                        /*  ��ʱ��¼����                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  ��ʱ������¼��Ϣ����        */
    }
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer || !stMaxByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    ptcbCur->TCB_ulRecvOption = ulOption;
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        size_t  stMsgLenInBuffer;
        
        _MsgQueueMsgLen(pmsgqueue, &stMsgLenInBuffer);
        if ((stMsgLenInBuffer > stMaxByteSize) && 
            !(ulOption & LW_OPTION_NOERROR)) {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            _ErrorHandle(E2BIG);                                        /*  �˳�                        */
            return  (E2BIG);
        }
        
        pevent->EVENT_ulCounter--;
        _MsgQueueGet(pmsgqueue, pvMsgBuffer, 
                     stMaxByteSize, pstMsgLen);                         /*  �����Ϣ                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  �������ڵȴ�д��Ϣ          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  ʹ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  ���� TCB                    */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
        }
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  ���ȴ�                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    ptcbCur->TCB_pstMsgByteSize    = pstMsgLen;
    ptcbCur->TCB_stMaxByteSize     = stMaxByteSize;
    ptcbCur->TCB_pvMsgQueueMessage = pvMsgBuffer;                       /*  ��¼��Ϣ                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;            /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  ��¼�ȴ�����λ��            */
        _EventWaitPriority(pevent, ppringList);                         /*  �������ȼ��ȴ���            */
        
    } else {                                                            /*  �� FIFO �ȴ�                */
        _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_R, ppringList);                   /*  ȷ�� FIFO ���е�λ��        */
        _EventWaitFifo(pevent, ppringList);                             /*  ���� FIFO �ȴ���            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_PEND, 
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
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            if ((*pstMsgLen == 0) && (ptcbCur->TCB_stMaxByteSize == 0)) {
                _ErrorHandle(E2BIG);                                    /*  �˳�                        */
                return  (E2BIG);
            
            } else {
                return  (ERROR_NONE);
            }
        
        } else {
            _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);                   /*  �Ѿ���ɾ��                  */
            return  (ERROR_MSGQUEUE_WAS_DELETED);
        }
    }
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
