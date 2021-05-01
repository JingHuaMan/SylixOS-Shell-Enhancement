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
** ��   ��   ��: ThreadCancelWatchDog.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 02 ��
**
** ��        ��: ȡ���߳��Լ��Ŀ��Ź�

** BUG
2008.03.29  ��ʼ�����µ�ɨ��������.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadCancelWatchDog
** ��������: ȡ���߳��Լ��Ŀ��Ź�
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

LW_API  
VOID  API_ThreadCancelWatchDog (VOID)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    if (ptcbCur->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcbCur);                              /*  ��ɨ��������ɾ��            */
        ptcbCur->TCB_ulWatchDog = 0ul;
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_CANCELWD, 
                      ptcbCur->TCB_ulId, LW_NULL);
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
  END
*********************************************************************************************************/
