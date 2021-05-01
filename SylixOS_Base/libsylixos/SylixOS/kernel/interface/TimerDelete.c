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
** ��   ��   ��: TimerDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 15 ��
**
** ��        ��: ɾ��һ����ʱ��

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2007.11.18  ucType �����Ĵ���ʱ����ִ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerDelete
** ��������: ɾ��һ����ʱ��
** �䡡��  : 
**           pulId      ��ʱ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    _ObjectCloseId(pulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "timer \"%s\" has been delete.\r\n", ptmr->TIMER_cTmrName);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_STOP) {                 /*  �ر�״̬��ɾ��              */
        goto    __delete;
    }
    
    ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  ��ɨ�����ɾ��              */
        _WakeupDel(&_K_wuITmr, &ptmr->TIMER_wunTimer, LW_TRUE);
    
    } else {
        _WakeupDel(&_K_wuHTmr, &ptmr->TIMER_wunTimer, LW_FALSE);
    }
    
    ptmr->TIMER_ulCounter = 0ul;                                        /*  ���������                  */

__delete:
    ptmr->TIMER_ucType = LW_TYPE_TIMER_UNUSED;                          /*  ɾ����־                    */
    
    KN_INT_ENABLE(iregInterLevel);
    
    _Free_Timer_Object(ptmr);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_TIMER, MONITOR_EVENT_TIMER_DELETE, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
