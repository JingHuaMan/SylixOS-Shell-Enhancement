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
** ��   ��   ��: SemaphoreMPost.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �ͷŻ����ź���, �������ж��в���

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.08.26  �� MUTEX �ź��������Ľ�, �����߳��������� pend ����ʱ, ��������, 
            ��ֻ�������ͷŵ��õĴ�����, �Ż��ͷ�.
2009.04.08  ����� SMP ��˵�֧��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.12.11  ϵͳû������ʱ post ����������.
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
** ��������: API_SemaphoreMPost
** ��������: �ͷŻ����ź���, �������ж��в���
** �䡡��  : 
**           ulId                   �¼����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMPost (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
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
    
    if (ptcbCur != (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn) {             /*  �Ƿ���ӵ����                */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_EVENT_NOT_OWN);                              /*  û���¼�����Ȩ              */
        return  (ERROR_EVENT_NOT_OWN);
    }
    
    if (pevent->EVENT_pvPtr) {                                          /*  ����Ƿ�����˵ݹ����      */
        pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr - 1);  /*  ��ʱ������--                */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (_EventWaitNum(EVENT_SEM_Q, pevent)) {
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  ���ȼ��ȴ�����              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  �������ȼ��ȴ��߳�          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  ����FIFO�ȴ��߳�            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);
        
        _EventPrioTryResume(pevent, ptcbCur);                           /*  ���Է���֮ǰ�����ȼ�        */
        
        pevent->EVENT_ulMaxCounter = (ULONG)ptcb->TCB_ucPriority;
        pevent->EVENT_pvTcbOwn     = (PVOID)ptcb;                       /*  �����߳���Ϣ                */

        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  ���� TCB                    */
        
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {
            LW_THREAD_SAFE_INKERN(ptcb);                                /*  ��������������Ϊ��ȫ        */
        }

        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_POST, 
                          ulId, ptcb->TCB_ulId, LW_NULL);
        
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  �������˳���ȫģʽ          */
            LW_THREAD_UNSAFE();
        }
        return  (ERROR_NONE);
    
    } else {                                                            /*  û���̵߳ȴ�                */
        KN_INT_ENABLE(iregInterLevel);
        
        if (pevent->EVENT_ulCounter == LW_FALSE) {                      /*  ����Ƿ��пռ��          */
            pevent->EVENT_ulCounter = (ULONG)LW_TRUE;
            
            pevent->EVENT_ulMaxCounter = LW_PRIO_LOWEST;                /*  ��ձ�����Ϣ                */
            pevent->EVENT_pvTcbOwn     = LW_NULL;
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  �������˳���ȫģʽ          */
                LW_THREAD_UNSAFE();
            }
            return  (ERROR_NONE);
        
        } else {                                                        /*  �Ѿ�����                    */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_EVENT_FULL);
            return  (ERROR_EVENT_FULL);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
