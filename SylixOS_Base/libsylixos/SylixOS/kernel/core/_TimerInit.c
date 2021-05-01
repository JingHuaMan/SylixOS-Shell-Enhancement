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
** ��   ��   ��: _TimerInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 10 ��
**
** ��        ��: ��ʱ����ʼ�������⡣
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _TimerInit
** ��������: ��ʱ����ʼ��
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TimerInit (VOID)
{
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
    
#if  LW_CFG_MAX_TIMERS == 1

    REGISTER PLW_CLASS_TIMER        ptmrTemp1;
    
    _K_resrcTmr.RESRC_pmonoFreeHeader = &_K_tmrBuffer[0].TIMER_monoResrcList;
    
    ptmrTemp1 = &_K_tmrBuffer[0];
    
    ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
    ptmrTemp1->TIMER_usIndex = 0;
    LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);

    _INIT_LIST_MONO_HEAD(_K_resrcTmr.RESRC_pmonoFreeHeader);
    
    _K_resrcTmr.RESRC_pmonoFreeTail = _K_resrcTmr.RESRC_pmonoFreeHeader;
#else

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_TIMER        ptmrTemp1;
    REGISTER PLW_CLASS_TIMER        ptmrTemp2;
    
    _K_resrcTmr.RESRC_pmonoFreeHeader = &_K_tmrBuffer[0].TIMER_monoResrcList;
    
    ptmrTemp1 = &_K_tmrBuffer[0];                                       /*  ָ�򻺳���׵�ַ            */
    ptmrTemp2 = &_K_tmrBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_TIMERS - 1); ulI++) {
        ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
        ptmrTemp1->TIMER_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &ptmrTemp1->TIMER_monoResrcList;
        pmonoTemp2 = &ptmrTemp2->TIMER_monoResrcList;
        LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        ptmrTemp1++;
        ptmrTemp2++;
    }
    
    ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
    ptmrTemp1->TIMER_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);
    
    pmonoTemp1 = &ptmrTemp1->TIMER_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcTmr.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                  /*  LW_CFG_MAX_TIMERS == 1      */

    _K_resrcTmr.RESRC_uiUsed    = 0;
    _K_resrcTmr.RESRC_uiMaxUsed = 0;

#if (LW_CFG_HTIMER_EN > 0)
    __WAKEUP_INIT(&_K_wuHTmr, LW_NULL, LW_NULL);
#endif

#if (LW_CFG_ITIMER_EN > 0)
    __WAKEUP_INIT(&_K_wuITmr, _ITimerWakeup, LW_NULL);
#endif

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
