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
** ��   ��   ��: ThreadResume.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: ����ϵͳ�ָ̻߳���������

** BUG
2007.07.19  ���� _DebugHandle() ����
2007.09.26  ȥ���˾���û������������ʱ�Ĵ����ӡ.
2008.03.30  ʹ���µľ���������.
2009.05.26  ��Ŀ���߳�û�б�����ʱ, �� TCB_ulResumeNesting++;
            �߳���ɾ��������, ��������ô˺��������߳�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadResume
** ��������: �ָ������߳�
** �䡡��  : ulId            �߳�ID
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_SUSPEND_EN > 0

LW_API
ULONG  API_ThreadResume (LW_OBJECT_HANDLE    ulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER PLW_CLASS_PCB         ppcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    if (ptcb->TCB_iDeleteProcStatus) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread has been deleted.\r\n");
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (ERROR_THREAD_OTHER_DELETE);
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESUME,
                      ulId, ptcb->TCB_ulSuspendNesting - 1, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (ptcb->TCB_ulSuspendNesting) {
        ptcb->TCB_ulSuspendNesting--;
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        return  (ERROR_NONE);
    }
    
    if (!ptcb->TCB_ulSuspendNesting) {                                  /*  ���һ��������            */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SUSPEND);
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  ���� ?                      */
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;         /*  �жϼ��ʽ                */
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  ���뵽������ȼ�������      */
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        return  (ERROR_NONE);
        
    } else {                                                            /*  ��Ƕ��                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
