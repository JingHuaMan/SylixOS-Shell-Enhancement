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
** ��   ��   ��: _ThreadSafe.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: �̰߳�ȫģʽ������

** BUG
2007.05.08  _ThreadUnsafeInternal() �� _ThreadUnsafeInternalEx() ���ܳ���û�д��жϵ����
2008.03.30  ʹ���µľ���������.
2008.03.30  ����ɾ��ʱ, ��Ҫ������� TCB_ptcbDeleteThread ��־, ��������� Thread Delete �� heap 
            ������Ӱ��.
2010.01.22  ֧�� SMP.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.09.10  �������ɾ���Լ�, ���˳���ȫģʽʱ����Ҫ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadSafeSuspend
** ��������: �߳������ȴ��Է��Ӱ�ȫģʽ�˳� (�ڽ����ں˲��ر��ж��е���)
** �䡡��  : ptcbCur       ��ǰ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadSafeSuspend (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_CLASS_PCB   ppcbMe;
             
    ptcbCur->TCB_ulSuspendNesting++;
    ppcbMe = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcbMe);                             /*  �Ӿ�������ɾ��              */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
}
/*********************************************************************************************************
** ��������: _ThreadSafeResume
** ��������: �߳̽��밲ȫģʽ (�ڽ����ں˲��ر��ж��е���)
** �䡡��  : ptcb          ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadSafeResume (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_PCB  ppcb;
    
    if (ptcb->TCB_ulSuspendNesting) {
        ptcb->TCB_ulSuspendNesting--;
    } else {
        return;
    }
    
    if (ptcb->TCB_ulSuspendNesting) {                                   /*  ����Ƕ���û������������    */
        return;
    }
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SUSPEND);
    
    if (__LW_THREAD_IS_READY(ptcb)) {
       ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;              /*  �жϼ��ʽ                */
       ppcb = _GetPcb(ptcb);
       __ADD_TO_READY_RING(ptcb, ppcb);                                 /*  ���������                  */
    }
}
/*********************************************************************************************************
** ��������: _ThreadSafeInternal
** ��������: ��ǰ�߳̽��밲ȫģʽ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadSafeInternal (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TCB   ptcbCur;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    LW_TCB_GET_CUR(ptcbCur);
    
    ptcbCur->TCB_ulThreadSafeCounter++;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SAFE, 
                      ptcbCur->TCB_ulId, ptcbCur->TCB_ulThreadSafeCounter, LW_NULL);
}
/*********************************************************************************************************
** ��������: _ThreadSafeInKern
** ��������: ���ں�ģʽָ���߳̽��밲ȫģʽ (�ں�����״̬������)
** �䡡��  : ptcbDes       Ŀ���߳�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _ThreadSafeInKern (PLW_CLASS_TCB  ptcbDes)
{
    ptcbDes->TCB_ulThreadSafeCounter++;

    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SAFE,
                      ptcbDes->TCB_ulId, ptcbDes->TCB_ulThreadSafeCounter, LW_NULL);
}
/*********************************************************************************************************
** ��������: _ThreadUnsafeInternal
** ��������: ��ǰ�߳̽��밲ȫģʽ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadUnsafeInternal (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    _ThreadUnsafeInternalEx(ptcbCur);
}
/*********************************************************************************************************
** ��������: _ThreadUnsafeInternalEx
** ��������: ָ���߳��˳���ȫģʽ
** �䡡��  : ptcbDes       Ŀ���߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadUnsafeInternalEx (PLW_CLASS_TCB   ptcbDes)
{
             INTREG             iregInterLevel;
    REGISTER PLW_CLASS_TCB      ptcbResume;
    
#if LW_CFG_THREAD_DEL_EN > 0
             LW_OBJECT_HANDLE   ulIdMe;
    REGISTER PVOID              pvRetValue;
#endif

    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_UNSAFE, 
                      ptcbDes->TCB_ulId, ptcbDes->TCB_ulThreadSafeCounter - 1, LW_NULL);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    if (ptcbDes->TCB_ulThreadSafeCounter) {
        ptcbDes->TCB_ulThreadSafeCounter--;
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
        return;
    }

    if (ptcbDes->TCB_ulThreadSafeCounter) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
        return;
    }
    
    if (ptcbDes->TCB_ptcbDeleteMe) {
        ptcbResume = ptcbDes->TCB_ptcbDeleteMe;
        ptcbDes->TCB_ptcbDeleteMe = LW_NULL;                            /*  �����ʶ                    */
        
        if ((ptcbResume != ptcbDes) && 
            ((addr_t)ptcbResume != 1ul) &&
            (!_Thread_Invalid(_ObjectGetIndex(ptcbResume->TCB_ulId)))) {
            ptcbResume->TCB_ptcbDeleteWait = LW_NULL;                   /*  �˳��ȴ�ģʽ                */
            _ThreadSafeResume(ptcbResume);                              /*  ����ȴ��߳�                */
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
                                                                        /*  ���ܲ���һ�ε���            */

#if LW_CFG_THREAD_DEL_EN > 0
        ulIdMe     = ptcbDes->TCB_ulId;
        pvRetValue = ptcbDes->TCB_pvRetValue;
        
        API_ThreadDelete(&ulIdMe, pvRetValue);                          /*  ɾ���Լ�                    */
#endif
    
#if LW_CFG_THREAD_RESTART_EN > 0
    } else if (ptcbDes->TCB_bRestartReq) {                              /*  ��Ҫ����                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */

        API_ThreadRestart(ptcbDes->TCB_ulId, LW_NULL);                  /*  �����Լ�                    */
#endif

    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
