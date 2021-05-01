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
** ��   ��   ��: TimerHTicks.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ���Ǹ��ٶ�ʱ�������жϷ�����

** BUG
2007.11.13  ʹ���������������������ȫ��װ.
2013.12.04  ���ûص�ʱ, ��Ҫ���ں���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerHTicks
** ��������: ���Ǹ��ٶ�ʱ�������жϷ�����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if	(LW_CFG_HTIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)

LW_API
VOID  API_TimerHTicks (VOID)
{
             INTREG                     iregInterLevel;
    REGISTER PLW_CLASS_TIMER            ptmr;
             PLW_CLASS_WAKEUP_NODE      pwun;
             PTIMER_CALLBACK_ROUTINE    pfuncRoutine;
             PVOID                      pvRoutineArg;
             ULONG                      ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    __WAKEUP_PASS_FIRST(&_K_wuHTmr, pwun, ulCounter);
    
    ptmr = _LIST_ENTRY(pwun, LW_CLASS_TIMER, TIMER_wunTimer);
    
    _WakeupDel(&_K_wuHTmr, pwun, LW_FALSE);
    
    if (ptmr->TIMER_ulOption & LW_OPTION_AUTO_RESTART) {
        ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;
        _WakeupAdd(&_K_wuHTmr, pwun, LW_FALSE);
        
    } else {
        ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;                    /*  ��дֹͣ��־λ              */
    }
    
    pfuncRoutine = ptmr->TIMER_cbRoutine;
    pvRoutineArg = ptmr->TIMER_pvArg;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    
    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine(pvRoutineArg);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ����������Ӧ�ж�            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
}

#endif                                                                  /*  (LW_CFG_HTIMER_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
