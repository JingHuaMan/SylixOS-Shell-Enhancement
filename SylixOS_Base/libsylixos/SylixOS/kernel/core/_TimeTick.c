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
** ��   ��   ��: _TimeTick.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 29 ��
**
** ��        ��: ����ϵͳ TICK �жϺ�����.

** BUG
2008.03.30  ʹ���µľ���������.
2009.03.15  ʹ ppcb �Ļ�ȡ��Ϊ�Ż�.
2009.11.20  WatchDogTimerHook ��Ϊ OSTaskWatchDogTimerHook.
2010.01.22  ʹ���ں�ģʽ���б���. ��֧�� SMP.
2016.07.21  ��ʱ������ȴ��¼�, ��ֱ�Ӵ��¼�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _WatchDogTick
** ��������: ɨ�迴�Ź�����, tick ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

VOID  _WatchDogTick (VOID)
{
             INTREG                 iregInterLevel;
    REGISTER PLW_CLASS_TCB          ptcb;
             PLW_CLASS_WAKEUP_NODE  pwun;
             ULONG                  ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    __WAKEUP_PASS_FIRST(&_K_wuWatchDog, pwun, ulCounter);
    
    ptcb = _LIST_ENTRY(pwun, LW_CLASS_TCB, TCB_wunWatchDog);
    
    __DEL_FROM_WATCHDOG_LINE(ptcb);                                     /*  ��ɨ��������ɾ��            */
    
    KN_INT_ENABLE(iregInterLevel);
    
    bspWdTimerHook(ptcb->TCB_ulId);
    __LW_WATCHDOG_TIMER_HOOK(ptcb->TCB_ulId);                           /*  ִ�лص�����                */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ����������Ӧ�ж�            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
** ��������: _ThreadTick
** ��������: ɨ��ȴ���������, tick ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadTick (VOID)
{
             INTREG                 iregInterLevel;
    REGISTER PLW_CLASS_TCB          ptcb;
    REGISTER PLW_CLASS_PCB          ppcb;
             PLW_CLASS_WAKEUP_NODE  pwun;
             ULONG                  ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    __WAKEUP_PASS_FIRST(&_K_wuDelay, pwun, ulCounter);
    
    ptcb = _LIST_ENTRY(pwun, LW_CLASS_TCB, TCB_wunDelay);
    
    __DEL_FROM_WAKEUP_LINE(ptcb);                                       /*  �ӵȴ�����ɾ��              */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  ����Ƿ��ڵȴ��¼�          */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);             /*  �ȴ���ʱ����¼��ȴ�λ      */
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
    } else {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  û�еȴ��¼�                */
    }
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ����Ƿ����                */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
        ppcb = _GetPcb(ptcb);                                           /*  ������ȼ����ƿ�            */
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
    }
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ����������Ӧ�ж�            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
