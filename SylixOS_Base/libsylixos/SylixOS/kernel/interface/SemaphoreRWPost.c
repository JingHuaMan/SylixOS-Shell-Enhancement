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
** ��   ��   ��: SemaphoreRWPost.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 20 ��
**
** ��        ��: �ͷŶ�д�ź���, �������ж��в���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreRWPost
** ��������: �ͷŶ�д�ź���, �������ж��в���
** �䡡��  : 
**           ulId                   �¼����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreRWPost (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  �ȴ����е�ַ                */
             BOOL                  bIgnWrite = LW_FALSE;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {                        /*  �����Ƿ���ȷ                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter == 0) {                                 /*  û�м���                    */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_EVENT_NOT_OWN);                              /*  û���¼�����Ȩ              */
        return  (ERROR_EVENT_NOT_OWN);
    }
    
    if (pevent->EVENT_iStatus == EVENT_RW_STATUS_R) {                   /*  ��ǰΪ������                */
        if (pevent->EVENT_ulCounter > 1) {                              /*  ������������������ִ��      */
            pevent->EVENT_ulCounter--;
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  �˳���ȫģʽ                */
                LW_THREAD_UNSAFE();
            }
            return  (ERROR_NONE);
        
        } else {                                                        /*  ����������������            */
            if (!(pevent->EVENT_ulOption & LW_OPTION_RW_PREFER_WRITER)) {
                if (_EventWaitNum(EVENT_RW_Q_R, pevent)) {
                    bIgnWrite = LW_TRUE;                                /*  ���Լ���д��, ���ȼ������  */
                }
            }
            goto    __release_pend;
        }
        
    } else {                                                            /*  д����                      */
        if (pevent->EVENT_pvTcbOwn != (PVOID)ptcbCur) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_EVENT_NOT_OWN);                          /*  û���¼�����Ȩ              */
            return  (ERROR_EVENT_NOT_OWN);
        }
        
        if (pevent->EVENT_pvPtr) {                                      /*  ����Ƿ��������������      */
            pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr - 1);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            return  (ERROR_NONE);
        }
        
__release_pend:
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */

        if (_EventWaitNum(EVENT_RW_Q_W, pevent) && !bIgnWrite) {        /*  ���ڵȴ�д������            */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_W, ppringList);        /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_RW_Q_W, ppringList);            /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);           /*  ���� TCB                    */
            
            pevent->EVENT_pvTcbOwn = (PVOID)ptcb;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_W;
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  ��������������Ϊ��ȫ        */
                LW_THREAD_SAFE_INKERN(ptcb);
            }

            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_POST, 
                              ulId, ptcb->TCB_ulId, LW_NULL);
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  �˳���ȫģʽ                */
                LW_THREAD_UNSAFE();
            }
            return  (ERROR_NONE);
        
        } else if (_EventWaitNum(EVENT_RW_Q_R, pevent)) {               /*  ���ڵȴ���������            */
            pevent->EVENT_ulCounter--;
            
            while (_EventWaitNum(EVENT_RW_Q_R, pevent)) {               /*  ����ȫ��������              */
                if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                    _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_R, ppringList);    /*  �������ȼ��ȴ��߳�          */
                    ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
                    
                } else {
                    _EVENT_DEL_Q_FIFO(EVENT_RW_Q_R, ppringList);        /*  ����FIFO�ȴ��߳�            */
                    ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
                }
                
                pevent->EVENT_ulCounter++;                              /*  ����ʹ���߼���              */
                
                KN_INT_ENABLE(iregInterLevel);                          /*  ���ж�                    */
                _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);       /*  ���� TCB                    */
                
                if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {   /*  ��������������Ϊ��ȫ        */
                    LW_THREAD_SAFE_INKERN(ptcb);
                }

                MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_POST, 
                                  ulId, ptcb->TCB_ulId, LW_NULL);
                
                iregInterLevel = KN_INT_DISABLE();                      /*  �ر��ж�                    */
            }
            
            KN_INT_ENABLE(iregInterLevel);
            
            pevent->EVENT_pvTcbOwn = LW_NULL;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_R;                 /*  �ָ�Ϊ��״̬                */
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  �˳���ȫģʽ                */
                LW_THREAD_UNSAFE();
            }
            return  (ERROR_NONE);
        
        } else {
            KN_INT_ENABLE(iregInterLevel);
        
            pevent->EVENT_ulCounter--;
            pevent->EVENT_pvTcbOwn = LW_NULL;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_R;                 /*  �ָ�Ϊ��״̬                */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  �˳���ȫģʽ                */
                LW_THREAD_UNSAFE();
            }
            return  (ERROR_NONE);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
