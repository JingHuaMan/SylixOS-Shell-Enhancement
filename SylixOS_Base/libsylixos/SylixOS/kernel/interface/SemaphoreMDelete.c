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
** ��   ��   ��: SemaphoreMDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �����ź���ɾ��

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
2011.07.29  ������󴴽�/���ٻص�.
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
** ��������: API_SemaphoreMDelete
** ��������: �����ź���ɾ��, ��ΪMUTEX�����в����������������ж������У��������ﲻ�ù��ж�
** �䡡��  : 
**           pulId            �¼����ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API 
ULONG  API_SemaphoreMDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
    REGISTER ULONG                 ulOptionTemp;
    REGISTER UINT16                usIndex;
    REGISTER UINT8                 ucPriority;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
    
    REGISTER LW_OBJECT_HANDLE      ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_M)) {                         /*  �����Ƿ���ȷ                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MUTEX)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }

    _ObjectCloseId(pulId);                                              /*  ������                    */
    
    pevent->EVENT_ucType = LW_TYPE_EVENT_UNUSED;                        /*  �¼�����Ϊ��                */
    
    while (_EventWaitNum(EVENT_SEM_Q, pevent)) {                        /*  �Ƿ�������ڵȴ�������      */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  ���ȼ��ȴ�����              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  �������ȼ��ȴ��߳�          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  ����FIFO�ȴ��߳�            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }

        KN_INT_ENABLE(iregInterLevel);
        ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                    /*  �¼��Ѿ���ɾ��              */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  ���� TCB                    */
        iregInterLevel = KN_INT_DISABLE();
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ����ǰ CPU �ж�           */
    
    ptcb = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;                       /*  ���ӵ���� TCB              */
    pevent->EVENT_pvTcbOwn = LW_NULL;
    
    if (ptcb) {
        ucPriority = (UINT8)pevent->EVENT_ulMaxCounter;                 /*  ���ԭ�߳����ȼ�            */
        if (!LW_PRIO_IS_EQU(ucPriority, ptcb->TCB_ucPriority)) {        /*  ӵ�������ȼ������˱仯      */
            _SchedSetPrio(ptcb, ucPriority);                            /*  ��ԭ ���ȼ�                 */
        }
    }
    
    _Free_Event_Object(pevent);                                         /*  �������ƿ�                  */
    ulOptionTemp = pevent->EVENT_ulOption;                              /*  �ݴ�ѡ��                    */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_DELETE, ulId, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "semaphore \"%s\" has been delete.\r\n", pevent->EVENT_cEventName);
    
    if (ptcb) {
        if (ulOptionTemp & LW_OPTION_DELETE_SAFE) {                     /*  �˳���ȫģʽ                */
            LW_THREAD_UNSAFE_EX(ptcb);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
