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
** ��   ��   ��: TimerCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ����һ����ʱ��

** BUG
2007.10.20  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2008.06.09  ��ʼ��Ϊ�� posix ��ʱ��.
2008.12.15  ���� _INIT_LIST_LINE_HEAD Ϊ _LIST_LINE_INIT_IN_CODE �������ļ�ͳһ.
2011.02.26  ʹ�� TIMER_u64Overrun ��¼ overrun ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimerCreate
** ��������: ����һ����ʱ��
** �䡡��  : 
**           pcName                        ����
**           ulOption                      ��ʱ�����ͣ����١���ͨ
**           pulId                         Id ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
LW_OBJECT_HANDLE  API_TimerCreate (CPCHAR             pcName,
                                   ULONG              ulOption,
                                   LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_TIMER       ptmr;
    REGISTER ULONG                 ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        ptmr = _Allocate_Timer_Object();                                /*  ���һ����ʱ�����ƿ�        */
    );
    
    if (!ptmr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a timer.\r\n");
        _ErrorHandle(ERROR_TIMER_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(ptmr->TIMER_cTmrName, pcName);
    } else {
        ptmr->TIMER_cTmrName[0] = PX_EOS;                               /*  �������                    */
    }
    
    if (ulOption & LW_OPTION_ITIMER) {                                  /*  Ӧ�ü���ʱ��                */
        ptmr->TIMER_ucType = LW_TYPE_TIMER_ITIMER;                      /*  ��ʱ������                  */
    } else {
        ptmr->TIMER_ucType = LW_TYPE_TIMER_HTIMER;
    }
    
    __WAKEUP_NODE_INIT(&ptmr->TIMER_wunTimer);

    ptmr->TIMER_ulCounterSave = 0ul;                                    /*  ��ʱ������ֵ����ֵ          */
    ptmr->TIMER_ulOption      = 0ul;                                    /*  ��ʱ������ѡ��              */
    ptmr->TIMER_ucStatus      = LW_TIMER_STATUS_STOP;                   /*  ��ʱ��״̬                  */
    ptmr->TIMER_cbRoutine     = LW_NULL;                                /*  ִ�к���                    */
    ptmr->TIMER_pvArg         = LW_NULL;
    
    ptmr->TIMER_ulThreadId    = 0ul;                                    /*  û���߳�ӵ��                */
    ptmr->TIMER_u64Overrun    = 0ull;
    ptmr->TIMER_clockid       = CLOCK_REALTIME;

#if LW_CFG_TIMERFD_EN > 0
    ptmr->TIMER_pvTimerfd     = LW_NULL;
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */

    ulIdTemp = _MakeObjectId(_OBJECT_TIMER, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             ptmr->TIMER_usIndex);                      /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }

#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    ptmr->TIMER_ulTimer = ulIdTemp;
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_TIMER, MONITOR_EVENT_TIMER_CREATE, 
                      ulIdTemp, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "timer \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
