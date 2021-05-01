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
** ��   ��   ��: TimerStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 15 ��
**
** ��        ��: ��ö�ʱ�����״̬

** BUG
2007.10.20  ���� _DebugHandle() ����.
2015.04.07  ���� API_TimerStatusEx() ��չ�ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerStatusEx
** ��������: ��ö�ʱ�����״̬��չ�ӿ�, ���ṩ POSIX �ӿ��ڲ�ʹ��
** �䡡��  : 
**           ulId                        ��ʱ�����
**           pbTimerRunning              ��ʱ���Ƿ�������
**           pulOption                   ��ʱ��ѡ��
**           pulCounter                  ��ʱ����ǰ����ֵ
**           pulInterval                 ���ʱ��, Ϊ 0 ��ʾ��������
**           pclockid                    POSIX ʱ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerStatusEx (LW_OBJECT_HANDLE          ulId,
                          BOOL                     *pbTimerRunning,
                          ULONG                    *pulOption,
                          ULONG                    *pulCounter,
                          ULONG                    *pulInterval,
                          clockid_t                *pclockid)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (pbTimerRunning) {
        if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {
            *pbTimerRunning = LW_TRUE;
        } else {
            *pbTimerRunning = LW_FALSE;
        }
    }
    
    if (pulOption) {
        *pulOption = ptmr->TIMER_ulOption;
    }
    
    if (pulCounter) {
        if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {
            if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {
                _WakeupStatus(&_K_wuITmr, &ptmr->TIMER_wunTimer, pulCounter);
            
            } else {
                _WakeupStatus(&_K_wuHTmr, &ptmr->TIMER_wunTimer, pulCounter);
            }
        } else {
            *pulCounter = 0ul;
        }
    }
    
    if (pulInterval) {
        *pulInterval = ptmr->TIMER_ulCounterSave;
    }
    
    if (pclockid) {
        *pclockid = ptmr->TIMER_clockid;
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TimerStatus
** ��������: ��ö�ʱ�����״̬
** �䡡��  : 
**           ulId                        ��ʱ�����
**           pbTimerRunning              ��ʱ���Ƿ�������
**           pulOption                   ��ʱ��ѡ��
**           pulCounter                  ��ʱ����ǰ����ֵ
**           pulInterval                 ���ʱ��, Ϊ 0 ��ʾ��������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TimerStatus (LW_OBJECT_HANDLE          ulId,
                        BOOL                     *pbTimerRunning,
                        ULONG                    *pulOption,
                        ULONG                    *pulCounter,
                        ULONG                    *pulInterval)
{
    return  (API_TimerStatusEx(ulId, pbTimerRunning, pulOption,
                               pulCounter, pulInterval, LW_NULL));
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
