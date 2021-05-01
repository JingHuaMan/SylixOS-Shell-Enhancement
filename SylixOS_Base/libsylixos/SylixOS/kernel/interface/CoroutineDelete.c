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
** ��   ��   ��: CoroutineDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 19 ��
**
** ��        ��: ����Э�̹����(Э����һ���������Ĳ���ִ�е�λ). 
                 �ڵ�ǰ�߳���ɾ��һ��Э��. (����ɾ�������߳��е�Э��, ���������ϵͳ������������)
** BUG:
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.12.14  ʹ������������ȷ������Э�����������ȫ��.
2016.12.27  ʹ��Э���ӳ�ɾ������, ȷ����ջ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** ��������: API_CoroutineExit
** ��������: �ڵ�ǰ�߳�����ִ�е�Э��ɾ��
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineExit (VOID)
{
             INTREG                 iregInterLevel;
             
             PLW_CLASS_CPU          pcpuCur;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_COROUTINE    pcrcbExit;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNext;
    REGISTER PLW_LIST_RING          pringNext;
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pcrcbExit = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader, 
                            LW_CLASS_COROUTINE, 
                            COROUTINE_ringRoutine);                     /*  ��õ�ǰЭ��                */
    
    if (&pcrcbExit->COROUTINE_ringRoutine == 
        _list_ring_get_next(&pcrcbExit->COROUTINE_ringRoutine)) {       /*  ������һ��Э��              */
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadExit(LW_NULL);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
        return  (ERROR_NONE);
    }
    
    pringNext = _list_ring_get_next(&pcrcbExit->COROUTINE_ringRoutine);

    pcrcbNext = _LIST_ENTRY(pringNext, LW_CLASS_COROUTINE, 
                            COROUTINE_ringRoutine);                     /*  �����һ��Э��              */

    pcrcbExit->COROUTINE_ulFlags |= LW_COROUTINE_FLAG_DELETE;           /*  ��ǰЭ������ɾ��          */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_DELETE, 
                      ptcbCur->TCB_ulId, pcrcbExit, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcbCur->TCB_pringCoroutineHeader = pringNext;                      /*  ѡ����һ��Э��              */
    
    pcpuCur                = LW_CPU_GET_CUR();
    pcpuCur->CPU_pcrcbCur  = pcrcbExit;
    pcpuCur->CPU_pcrcbNext = pcrcbNext;
    archCrtCtxSwitch(pcpuCur);                                          /*  Э���л�                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    return  (ERROR_NONE);                                               /*  ���������޷����е������    */
}
/*********************************************************************************************************
** ��������: API_CoroutineDelete
** ��������: ɾ��һ��ָ����Э��.
** �䡡��  : pvCrcb                        Э�̾��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineDelete (PVOID  pvCrcb)
{
             INTREG                 iregInterLevel;

    REGISTER PLW_CLASS_COROUTINE    pcrcbDel = (PLW_CLASS_COROUTINE)pvCrcb;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNow;
             PLW_CLASS_TCB          ptcbCur;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!pcrcbDel) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (pcrcbDel->COROUTINE_ulThread != ptcbCur->TCB_ulId) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pcrcbNow = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader, 
                           LW_CLASS_COROUTINE, 
                           COROUTINE_ringRoutine);                      /*  ��õ�ǰЭ��                */
    
    if (pcrcbNow == pcrcbDel) {                                         /*  ɾ����ǰЭ��                */
        return  (API_CoroutineExit());
    }
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */

    LW_SPIN_LOCK_QUICK(&ptcbCur->TCB_slLock, &iregInterLevel);
    _List_Ring_Del(&pcrcbDel->COROUTINE_ringRoutine,
                   &ptcbCur->TCB_pringCoroutineHeader);                 /*  ��Э�̱���ɾ��              */
    LW_SPIN_UNLOCK_QUICK(&ptcbCur->TCB_slLock, iregInterLevel);
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_DELETE, 
                      ptcbCur->TCB_ulId, pcrcbDel, LW_NULL);
    
    if (pcrcbDel->COROUTINE_ulFlags & LW_COROUTINE_FLAG_DYNSTK) {
        _StackFree(ptcbCur, pcrcbDel->COROUTINE_pstkStackLowAddr);      /*  �ͷ��ڴ�                    */
    }
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
