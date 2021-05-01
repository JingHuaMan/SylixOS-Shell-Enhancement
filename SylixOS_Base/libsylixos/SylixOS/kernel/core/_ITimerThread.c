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
** ��   ��   ��: _ITimerThread.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: ����ϵͳ��ͨ��ʱ�����ڷ����̡߳�

** BUG
2007.05.11  ɾ���˴����ע����Ϣ
2007.11.13  ʹ���������������������ȫ��װ.
2008.10.17  ��ʹ�þ��ȵ������Ƚ����ӳ�.
2011.11.29  ���ûص�ʱ, ���������ں�.
2019.02.23  ����ʱ��ȴ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************

*********************************************************************************************************/
#define LW_ITIMER_IDLE_TICK     (LW_TICK_HZ * 100)
/*********************************************************************************************************
** ��������: _ITimerThread
** ��������: ����ϵͳ��ͨ��ʱ�����ڷ����̡߳�
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)

PVOID  _ITimerThread (PVOID  pvArg)
{
             INTREG                     iregInterLevel;
    REGISTER PLW_CLASS_TIMER            ptmr;
             PTIMER_CALLBACK_ROUTINE    pfuncRoutine;
             PVOID                      pvRoutineArg;

             PLW_CLASS_TCB              ptcbCur;
             PLW_CLASS_PCB              ppcb;

             PLW_CLASS_WAKEUP_NODE      pwun;
             INT64                      i64CurTime;
             ULONG                      ulCounter;
             BOOL                       bNoTimer;
    
    (VOID)pvArg;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */

    for (;;) {
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  �����ں�ͬʱ�ر��ж�        */

        __KERNEL_TIME_GET_IGNIRQ(_K_wuITmr.WU_i64LastTime, INT64);      /*  ԭʼʱ��                    */

        __WAKEUP_GET_FIRST(&_K_wuITmr, pwun);                           /*  ��õ�һ���ڵ�              */

        if (pwun) {
            ulCounter = pwun->WUN_ulCounter;                            /*  �ѵ�һ���ڵ�ȴ�ʱ�� Sleep  */
            bNoTimer  = LW_FALSE;

        } else {
            ulCounter = LW_ITIMER_IDLE_TICK;                            /*  û���κνڵ� (�ᱻ�Զ�����) */
            bNoTimer  = LW_TRUE;
        }

        ppcb = _GetPcb(ptcbCur);
        __DEL_FROM_READY_RING(ptcbCur, ppcb);                           /*  �Ӿ�������ɾ��              */

        ptcbCur->TCB_ulDelay = ulCounter;
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */

        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */

        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  �����ں�ͬʱ�ر��ж�        */
        
        if (bNoTimer) {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�ͬʱ���ж�        */
            continue;
        }

        __KERNEL_TIME_GET_IGNIRQ(i64CurTime, INT64);                    /*  ��� Sleep ��ʱ��           */
        ulCounter = (ULONG)(i64CurTime - _K_wuITmr.WU_i64LastTime);     /*  ����˯��ʱ��                */
        _K_wuITmr.WU_i64LastTime = i64CurTime;

        __WAKEUP_PASS_FIRST(&_K_wuITmr, pwun, ulCounter);
        
        ptmr = _LIST_ENTRY(pwun, LW_CLASS_TIMER, TIMER_wunTimer);
        
        _WakeupDel(&_K_wuITmr, pwun, LW_FALSE);
        
        if (ptmr->TIMER_ulOption & LW_OPTION_AUTO_RESTART) {
            ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;
            _WakeupAdd(&_K_wuITmr, pwun, LW_FALSE);
            
        } else {
            ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;                /*  ��дֹͣ��־λ              */
        }
        
        pfuncRoutine = ptmr->TIMER_cbRoutine;
        pvRoutineArg = ptmr->TIMER_pvArg;
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
        
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine(pvRoutineArg);
        
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  �����ں�ͬʱ�ر��ж�        */
        
        __WAKEUP_PASS_SECOND();
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  ����������Ӧ�ж�            */
    
        iregInterLevel = KN_INT_DISABLE();
        
        __WAKEUP_PASS_END();
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _ITimerWakeup
** ��������: ���� ITimer �̡߳�
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _ITimerWakeup (VOID)
{
    API_ThreadWakeup(_K_ulThreadItimerId);
}

#endif                                                                  /*  (LW_CFG_ITIMER_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
