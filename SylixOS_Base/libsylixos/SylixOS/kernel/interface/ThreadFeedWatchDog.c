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
** ��   ��   ��: ThreadFeedWatchDog.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 02 ��
**
** ��        ��: �����߳��߳��Լ��Ŀ��Ź���ʱ��

** BUG
2008.03.29  ��ʼ�����µ�ɨ��������.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadFeedWatchDog
** ��������: �����߳��Լ��Ŀ��Ź���ʱ��
** �䡡��  : 
**           ulWatchDogTicks               ���Ź���ʱ����ʱ����ֵ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

LW_API  
ULONG  API_ThreadFeedWatchDog (ULONG  ulWatchDogTicks)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (ulWatchDogTicks == 0) {                                         /*  ��Ҫֹͣ���Ź�              */
        API_ThreadCancelWatchDog();
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    if (ptcbCur->TCB_bWatchDogInQ) {                                    /*  �Ѿ���ɨ������              */
        __DEL_FROM_WATCHDOG_LINE(ptcbCur);
    }
    
    ptcbCur->TCB_ulWatchDog = ulWatchDogTicks;
    
    __ADD_TO_WATCHDOG_LINE(ptcbCur);                                    /*  ����ɨ������                */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_FEEDWD, 
                      ptcbCur->TCB_ulId, ulWatchDogTicks, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
  END
*********************************************************************************************************/
