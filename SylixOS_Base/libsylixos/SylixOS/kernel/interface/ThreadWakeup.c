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
** ��   ��   ��: ThreadWakeup.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �̴߳�˯��ģʽ����

** BUG
2007.07.19  ���� _DebugHandle() ����
2008.03.29  ʹ���µĵȴ�����.
2008.03.30  ʹ���µľ���������.
2010.01.22  ֧�� SMP.
2016.07.21  ��ʱ������ȴ��¼�, ��ֱ�Ӵ��¼�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadWakeupEx
** ��������: �̴߳�˯��ģʽ����
** �䡡��  : 
**           ulId               �߳̾��
**           bWithInfPend       �Ƿ񼤻����ȴ�����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadWakeupEx (LW_OBJECT_HANDLE  ulId, BOOL  bWithInfPend)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_PCB         ppcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                         /*  ��� ID ������Ч��         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                                /*  ����߳���Ч��             */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        _ErrorHandle(ERROR_THREAD_NOT_SLEEP);
        return  (ERROR_THREAD_NOT_SLEEP);
    }
    
    if (!bWithInfPend) {
        if (!(ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY)) {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں˲����ж�          */
            _ErrorHandle(ERROR_THREAD_NOT_SLEEP);
            return  (ERROR_THREAD_NOT_SLEEP);
        }
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  ��ʱ�ȴ�                    */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
        ptcb->TCB_ulDelay = 0ul;
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  �ȴ��¼�                    */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                     /*  �ȴ���ʱ                    */
        
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        if (ptcb->TCB_peventPtr) {
            _EventUnQueue(ptcb);
        } else 
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
        {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
            if (ptcb->TCB_pesnPtr) {
                _EventSetUnQueue(ptcb->TCB_pesnPtr);
            }
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0) && */
        }
    }
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ����Ƿ����                */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_WAKEUP, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadWakeup
** ��������: �̴߳�˯��ģʽ����
** �䡡��  : 
**           ulId    �߳̾��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadWakeup (LW_OBJECT_HANDLE  ulId)
{
    return  (API_ThreadWakeupEx(ulId, LW_FALSE));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
