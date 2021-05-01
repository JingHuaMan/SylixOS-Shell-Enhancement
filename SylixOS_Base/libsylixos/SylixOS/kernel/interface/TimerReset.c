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
** ��   ��   ��: TimerReset.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: �ڶ�ʱ������״̬�¸�λһ����ʱ��

** BUG
2007.10.20  ���� _DebugHandle() ���ܡ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerReset
** ��������: �ڶ�ʱ������״̬�¸�λһ����ʱ��
** �䡡��  : 
**           ulId                        ��ʱ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerReset (LW_OBJECT_HANDLE  ulId)
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
    
    if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_STOP) {                 /*  û�й���                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        _ErrorHandle(ERROR_TIMER_TIME);
        return  (ERROR_TIMER_TIME);
    }
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  ��ɨ�����ɾ��              */
        _WakeupDel(&_K_wuITmr, &ptmr->TIMER_wunTimer, LW_TRUE);
    
    } else {
        _WakeupDel(&_K_wuHTmr, &ptmr->TIMER_wunTimer, LW_FALSE);
    }
    
    ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;                  /*  �ָ�����ֵ                  */
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  ��ӵ�ɨ�����              */
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
