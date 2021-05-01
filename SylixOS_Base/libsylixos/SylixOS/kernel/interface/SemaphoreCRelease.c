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
** ��   ��   ��: SemaphoreCRelease.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �ͷż������ź���(�߼��������� WIN32 API �ǳ�����)

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreCRelease
** ��������: �ͷż������ź���
** �䡡��  : 
**           ulId                   �¼����
**           ulReleaseCounter       �ͷŵ��ź�������
**           pulPreviousCounter     ����ԭ�ȵ��ź�������������Ϊ NULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreCRelease (LW_OBJECT_HANDLE  ulId, ULONG  ulReleaseCounter, ULONG  *pulPreviousCounter)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulReleaseCounter) {                                            /*  ���Ҫ�ͷŵ��ź���          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "release counter invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_NULL);
        return  (ERROR_EVENT_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_C)) {                         /*  �����Ƿ���ȷ                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMC)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulPreviousCounter) {                                           /*  ������ǰ����ֵ              */
        *pulPreviousCounter = pevent->EVENT_ulCounter;
    }
    
    for (; ulReleaseCounter > 0; ulReleaseCounter--) {                  /*  ���͵��ź�������            */
        if (_EventWaitNum(EVENT_SEM_Q, pevent)) {                       /*  �Ƿ����̵߳ȴ�              */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);         /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);             /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }

            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);           /*  ���� TCB                    */
            
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMC, MONITOR_EVENT_SEM_POST, 
                              ulId, ptcb->TCB_ulId, LW_NULL);
            
            iregInterLevel = KN_INT_DISABLE();                          /*  �ر��ж�                    */
        
        } else {                                                        /*  û���̵߳ȴ�                */
            if (pevent->EVENT_ulCounter < pevent->EVENT_ulMaxCounter) { /*  ����Ƿ��пռ��          */
                pevent->EVENT_ulCounter++;
                
                KN_INT_ENABLE(iregInterLevel);                          /*  ���ж�                    */
                iregInterLevel = KN_INT_DISABLE();                      /*  �ر��ж�                    */
            
            } else {                                                    /*  �Ѿ�����                    */
                __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  �˳��ں�                    */
                _ErrorHandle(ERROR_EVENT_FULL);
                return  (ERROR_EVENT_FULL);
            }
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMC_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
