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
** ��   ��   ��: MsgQueueSendEx.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 07 ��
**
** ��        ��: ����Ϣ���з�����Ϣ(�߼��ӿں���)

** BUG
2007.09.19  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
2010.01.22  ���������ں˵�ʱ��.
2013.03.17  ������Զ��ض���Ϣʱ���ж�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_MsgQueueSendEx2
** ��������: ����Ϣ���з�����Ϣ
** �䡡��  : 
**           ulId                   ��Ϣ���о��
**           pvMsgBuffer            ��Ϣ������
**           stMsgLen               ��Ϣ����
**           ulTimeout              ��ʱʱ��
**           ulOption               ��Ϣѡ��       LW_OPTION_DEFAULT or LW_OPTION_URGENT or 
**                                                 LW_OPTION_BROADCAST
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueSendEx2 (LW_OBJECT_HANDLE  ulId,
                            const PVOID       pvMsgBuffer,
                            size_t            stMsgLen,
                            ULONG             ulTimeout,
                            ULONG             ulOption)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  �¼�����ѡ��                */
             size_t                stRealLen;
    
    if (ulOption == LW_OPTION_DEFAULT) {                                /*  ��ͨ����                    */
        return  (API_MsgQueueSend2(ulId, pvMsgBuffer, stMsgLen, ulTimeout));
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
__re_send:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!stMsgLen) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMsgLen invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_LEN);
        return  (ERROR_MSGQUEUE_MSG_LEN);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {                      /*  �����Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
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
    
    if (stMsgLen > pmsgqueue->MSGQUEUE_stMaxBytes) {                    /*  ����̫��                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMsgLen invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_LEN);
        return  (ERROR_MSGQUEUE_MSG_LEN);
    }
    
    if (ulOption & LW_OPTION_URGENT) {
        if (_EventWaitNum(EVENT_MSG_Q_R, pevent)) {
            BOOL    bSendOk = LW_TRUE;
            
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_R, ppringList);       /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_R, ppringList);           /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
        
            if ((stMsgLen > ptcb->TCB_stMaxByteSize) && 
                !(ptcb->TCB_ulRecvOption & LW_OPTION_NOERROR)) {        /*  �Ƿ������Զ��ض�            */
                *ptcb->TCB_pstMsgByteSize = 0;
                ptcb->TCB_stMaxByteSize = 0;
                bSendOk = LW_FALSE;
            
            } else {
                stRealLen = (stMsgLen < ptcb->TCB_stMaxByteSize) ?
                            (stMsgLen) : (ptcb->TCB_stMaxByteSize);     /*  ȷ����Ϣ��������            */
                
                *ptcb->TCB_pstMsgByteSize = stRealLen;                  /*  ���泤��                    */
                lib_memcpy(ptcb->TCB_pvMsgQueueMessage,                 /*  ������Ϣ                    */
                           pvMsgBuffer, 
                           stRealLen);
            }
        
            KN_INT_ENABLE(iregInterLevel);                              /*  ʹ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  ���� TCB                    */
            
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_POST, 
                              ulId, ptcb->TCB_ulId, LW_NULL);
                             
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            if (bSendOk == LW_FALSE) {
                goto    __re_send;                                      /*  ���·���                    */
            }
            
            return  (ERROR_NONE);
        
        } else {                                                        /*  û���̵߳ȴ�                */
            UINT  uiPrio = (UINT)((ulOption >> 4) & 0xf);
            
            if (uiPrio > EVENT_MSG_Q_PRIO_LOW) {
                __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  �˳��ں�                    */
                _ErrorHandle(ERROR_MSGQUEUE_OPTION);
                return  (ERROR_MSGQUEUE_OPTION);
            }
            
            if (pevent->EVENT_ulCounter < pevent->EVENT_ulMaxCounter) { /*  ����Ƿ��пռ��          */
                pevent->EVENT_ulCounter++;                              /*  ������Ϣ                    */
                _MsgQueuePut(pmsgqueue, pvMsgBuffer, stMsgLen, uiPrio);
                __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  �˳��ں�                    */
                return  (ERROR_NONE);
            
            } else {                                                    /*  �Ѿ�����                    */
                if ((ulTimeout == LW_OPTION_NOT_WAIT) || 
                    LW_CPU_GET_CUR_NESTING()) {                         /*  ����Ҫ�ȴ�                  */
                    __KERNEL_EXIT_IRQ(iregInterLevel);                  /*  �˳��ں�                    */
                    _ErrorHandle(ERROR_MSGQUEUE_FULL);
                    return  (ERROR_MSGQUEUE_FULL);
                }
                
                LW_TCB_GET_CUR(ptcbCur);                                /*  ��ǰ������ƿ�              */
            
                ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_S;
                ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;/*  д״̬λ����ʼ�ȴ�          */
                ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;       /*  ��յȴ�ʱ��                */
                
                if (ulTimeout == LW_OPTION_WAIT_INFINITE) {             /*  �Ƿ�������ȴ�              */
                    ptcbCur->TCB_ulDelay = 0ul;
                } else {
                    ptcbCur->TCB_ulDelay = ulTimeout;                   /*  ���ó�ʱʱ��                */
                }
                __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);            /*  ��¼ϵͳʱ��                */
        
                if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                    _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
                    _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_S, ppringList, ucPriorityIndex);
                    ptcbCur->TCB_ppringPriorityQueue = ppringList;      /*  ��¼�ȴ�����λ��            */
                    _EventWaitPriority(pevent, ppringList);             /*  �������ȼ��ȴ���            */
                    
                } else {                                                /*  �� FIFO �ȴ�                */
                    _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_S, ppringList);       /*  ȷ�� FIFO ���е�λ��        */
                    _EventWaitFifo(pevent, ppringList);                 /*  ���� FIFO �ȴ���            */
                }
                
                KN_INT_ENABLE(iregInterLevel);                          /*  ʹ���ж�                    */
                
                ulEventOption = pevent->EVENT_ulOption;
                
                iSchedRet = __KERNEL_EXIT();                            /*  ����������                  */
                if (iSchedRet) {
                    if ((iSchedRet == LW_SIGNAL_EINTR) && 
                        (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
                        _ErrorHandle(EINTR);
                        return  (EINTR);
                    }                                                   /*  ���¼��㳬ʱʱ��            */
                    ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);   
                    if (ulTimeout == LW_OPTION_NOT_WAIT) {
                        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
                        return  (ERROR_THREAD_WAIT_TIMEOUT);
                    }
                    goto    __re_send;
                }
                
                if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
                    _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);            /*  ��ʱ                        */
                    return  (ERROR_THREAD_WAIT_TIMEOUT);
                    
                } else {
                    if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {
                        goto    __re_send;                              /*  ���³��Է���                */
                    
                    } else {
                        _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);       /*  �Ѿ���ɾ��                  */
                        return  (ERROR_MSGQUEUE_WAS_DELETED);
                    }
                }
            }
        }
        return  (ERROR_NONE);
        
    } else if (ulOption & LW_OPTION_BROADCAST) {                        /*  �㲥����, ֻ����ȴ��߳�    */
        while (_EventWaitNum(EVENT_MSG_Q_R, pevent)) {
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_R, ppringList);       /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_R, ppringList);           /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            if ((stMsgLen > ptcb->TCB_stMaxByteSize) && 
                !(ptcb->TCB_ulRecvOption & LW_OPTION_NOERROR)) {        /*  �Ƿ������Զ��ض�            */
                *ptcb->TCB_pstMsgByteSize = 0;
                ptcb->TCB_stMaxByteSize = 0;
            
            } else {
                stRealLen = (stMsgLen < ptcb->TCB_stMaxByteSize) ?
                            (stMsgLen) : (ptcb->TCB_stMaxByteSize);     /*  ȷ����Ϣ��������            */
                
                *ptcb->TCB_pstMsgByteSize = stRealLen;                  /*  ���泤��                    */
                lib_memcpy(ptcb->TCB_pvMsgQueueMessage,                 /*  ������Ϣ                    */
                           pvMsgBuffer, 
                           stRealLen);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  ���� TCB                    */
            
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_POST, 
                              ulId, ptcb->TCB_ulId, LW_NULL);
                         
            iregInterLevel = KN_INT_DISABLE();
        }
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        return  (ERROR_NONE);
        
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulOption invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_OPTION);
        return  (ERROR_MSGQUEUE_OPTION);
    }
}
/*********************************************************************************************************
** ��������: API_MsgQueueSendEx
** ��������: ����Ϣ���з�����Ϣ
** �䡡��  : 
**           ulId                   ��Ϣ���о��
**           pvMsgBuffer            ��Ϣ������
**           stMsgLen               ��Ϣ����
**           ulOption               ��Ϣѡ��       LW_OPTION_DEFAULT or LW_OPTION_URGENT or 
**                                                 LW_OPTION_BROADCAST
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueSendEx (LW_OBJECT_HANDLE  ulId,
                           const PVOID       pvMsgBuffer,
                           size_t            stMsgLen,
                           ULONG             ulOption)
{
    return  (API_MsgQueueSendEx2(ulId, pvMsgBuffer, stMsgLen, LW_OPTION_NOT_WAIT, ulOption));
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
