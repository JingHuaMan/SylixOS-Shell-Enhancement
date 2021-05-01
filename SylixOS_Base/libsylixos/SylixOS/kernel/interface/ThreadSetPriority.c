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
** ��   ��   ��: ThreadSetPriority.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �����߳����ȼ���

** BUG
2007.05.11  û�м�����ȼ�����Ч��
2007.07.19  ���� _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadSetPriority
** ��������: �����߳����ȼ���
** �䡡��  : 
**           ulId            �߳�ID
**           ucPriority      ���ȼ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0

LW_API
ULONG  API_ThreadSetPriority (LW_OBJECT_HANDLE    ulId, UINT8    ucPriority)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  ���ȼ�����                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (ptcb->TCB_iDeleteProcStatus) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (ERROR_THREAD_OTHER_DELETE);
    }

    if (LW_PRIO_IS_EQU(ptcb->TCB_ucPriority, ucPriority)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
    }
    
    _SchedSetPrio(ptcb, ucPriority);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CHANGE_PRIO_EN*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
