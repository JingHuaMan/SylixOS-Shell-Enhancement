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
** ��   ��   ��: SemaphoreBFlush.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �ͷŵȴ����������ź����������߳�

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.03.30  �������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreBFlush
** ��������: �ͷŵȴ����������ź����������߳�
** �䡡��  : 
**           ulId                   �¼����
**           pulThreadUnblockNum    ���������߳�����   ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreBFlush (LW_OBJECT_HANDLE  ulId, ULONG  *pulThreadUnblockNum)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {                         /*  �����Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulThreadUnblockNum) {                                          /*  ���潫Ҫ�������߳�����      */
        *pulThreadUnblockNum = _EventWaitNum(EVENT_SEM_Q, pevent);
    }

    while (_EventWaitNum(EVENT_SEM_Q, pevent)) {                        /*  �Ƿ�������ڵȴ�������      */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  ���ȼ��ȴ�����              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  �������ȼ��ȴ��߳�          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  ����FIFO�ȴ��߳�            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  ���� TCB                    */
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
    }

    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_FLUSH, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMB_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
