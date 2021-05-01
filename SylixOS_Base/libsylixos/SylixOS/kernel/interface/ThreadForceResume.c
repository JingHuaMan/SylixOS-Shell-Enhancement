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
** ��   ��   ��: ThreadForceResume.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����ϵͳ�ָ̻߳���������

** BUG
2007.07.18  ���� _DebugHandle() ����
2008.03.30  ʹ���µľ���������.
2009.05.26  �߳���ɾ��������, ��������ô˺��������߳�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadForceResume
** ��������: �ָ������߳� (����Ƕ�׶��ٲ�)
** �䡡��  : ulId            �߳�ID
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_SUSPEND_EN > 0

LW_API
ULONG  API_ThreadForceResume (LW_OBJECT_HANDLE    ulId)
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
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    if (ptcb->TCB_iDeleteProcStatus) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread has been deleted.\r\n");
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (ERROR_THREAD_OTHER_DELETE);
    }
    
    if (ptcb->TCB_ulSuspendNesting) {
        ptcb->TCB_ulSuspendNesting = 0;                                 /*  ֱ�Ӽ���                    */
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        return  (ERROR_NONE);
    }
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SUSPEND);
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;             /*  �жϼ��ʽ                */
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
    }
        
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
