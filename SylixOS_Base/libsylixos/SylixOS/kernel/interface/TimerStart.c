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
** ��   ��   ��: TimerStart.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ����һ����ʱ��

** BUG
2007.10.20  ���� _DebugHandle() ���ܡ�
2008.12.17  ����ܶ���Ӷ������˸ĸ￪�� 30 ���������, ����ĸ��ܵ����������ǿ��, ��л��һ�������Ҹ���
            ����һ������ȭ�ŵĻ���, ��Ϊ 80 �������Ӧ�ü縺��ʹ��, Ϊ���л�֮����!
            ���� API_TimerStart �����ṹ.
2013.12.04  �����ʱ����������, ����Ҫ���ȸ�λ��ʱ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerStart
** ��������: ����һ����ʱ��
** �䡡��  : ulId                        ��ʱ�����
**           ulCounter                   ������ʼֵ
**           ulOption                    ����ѡ��
**           cbTimerRoutine              �ص�����
**           pvArg                       ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerStart (LW_OBJECT_HANDLE         ulId,
                       ULONG                    ulCounter,
                       ULONG                    ulOption,
                       PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                       PVOID                    pvArg)
{
    return  (API_TimerStartEx(ulId, ulCounter, ulCounter, ulOption, cbTimerRoutine, pvArg));
}
/*********************************************************************************************************
** ��������: API_TimerStartEx
** ��������: ����һ����ʱ����չ�ӿ�
** �䡡��  : ulId                        ��ʱ�����
**           ulInitCounter               ������ʼֵ
**           ulCounter                   �ظ�������ʼֵ
**           ulOption                    ����ѡ��
**           cbTimerRoutine              �ص�����
**           pvArg                       ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TimerStartEx (LW_OBJECT_HANDLE         ulId,
                         ULONG                    ulInitCounter,
                         ULONG                    ulCounter,
                         ULONG                    ulOption,
                         PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                         PVOID                    pvArg)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!cbTimerRoutine) {                                              /*  �ص����                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "cbTimerRoutine invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_CALLBACK_NULL);
        return  (ERROR_TIMER_CALLBACK_NULL);
    }
    
    if (!ulInitCounter) {                                               /*  ��ʱʱ����                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulInitCounter invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_TIME);
        return  (ERROR_TIMER_TIME);
    }
    
    if ((ulOption & LW_OPTION_AUTO_RESTART) && !ulCounter) {            /*  ��ʱʱ����                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulCounter invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_TIME);
        return  (ERROR_TIMER_TIME);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_TIMER)) {                         /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Timer_Index_Invalid(usIndex)) {                                /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Timer_Type_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_NULL);
        return  (ERROR_TIMER_NULL);
    }

    ptmr = &_K_tmrBuffer[usIndex];
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {              /*  ���ڹ���                    */
        if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {               /*  ��ɨ�����ɾ��              */
            _WakeupDel(&_K_wuITmr, &ptmr->TIMER_wunTimer, LW_TRUE);
    
        } else {
            _WakeupDel(&_K_wuHTmr, &ptmr->TIMER_wunTimer, LW_FALSE);
        }
    }
    
    ptmr->TIMER_ulCounter     = ulInitCounter;
    ptmr->TIMER_ulCounterSave = ulCounter;
    ptmr->TIMER_ulOption      = ulOption;
    ptmr->TIMER_ucStatus      = LW_TIMER_STATUS_RUNNING;                /*  ��ʱ����ʼ����              */
    ptmr->TIMER_cbRoutine     = cbTimerRoutine;
    ptmr->TIMER_pvArg         = pvArg;
    ptmr->TIMER_u64Overrun    = 0ull;
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  ����ɨ�����                */
        _WakeupAdd(&_K_wuITmr, &ptmr->TIMER_wunTimer, LW_TRUE);

    } else {
        _WakeupAdd(&_K_wuHTmr, &ptmr->TIMER_wunTimer, LW_FALSE);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
