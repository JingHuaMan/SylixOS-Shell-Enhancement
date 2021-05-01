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
** ��   ��   ��: ThreadStart.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: ����ϵͳ����һ��������ʼ�����̡߳�

** BUG
2007.07.19  ���� _DebugHandle() ����
2008.03.30  ʹ���µľ���������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2012.03.30  ���� API_ThreadStartEx() �����������߳�ʱ, join �߳�, (ԭ�Ӳ���)
2013.08.28  �����ں��¼������.
2013.09.17  ʹ�� POSIX �涨��ȡ���㶯��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h ��Ҳ����ض���
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
** ��������: API_ThreadStartEx
** ��������: �����߳�
** �䡡��  : ulId            �߳�ID
**           bJoin           �Ƿ�ϲ��߳�
**           ppvRetValAddr   ����̷߳���ֵ�ĵ�ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStartEx (LW_OBJECT_HANDLE  ulId, BOOL  bJoin, PVOID  *ppvRetValAddr)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER PLW_CLASS_PCB         ppcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
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
    ppcb = _GetPcb(ptcb);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_START, ulId, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_INIT) {                   /*  �߳��Ƿ�Ϊ��ʼ��״̬        */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_INIT);                 /*  �����־λ                  */
        
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  ����                        */
            _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been start.\r\n",
                         ptcb->TCB_cThreadName);
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  ���뵽������ȼ�������      */

            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */
            
            if (bJoin) {
                _ThreadJoin(ptcb, LW_NULL, ppvRetValAddr);              /*  �ϲ�                        */
            }
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            return  (ERROR_NONE);

        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں˲����ж�          */
            _ErrorHandle(ERROR_THREAD_NOT_READY);
            return  (ERROR_THREAD_NOT_READY);
        }
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not has this opt.\r\n");
        _ErrorHandle(ERROR_THREAD_NOT_INIT);
        return  (ERROR_THREAD_NOT_INIT);
    }
}
/*********************************************************************************************************
** ��������: API_ThreadStart
** ��������: �����߳�
** �䡡��  : ulId            �߳�ID
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStart (LW_OBJECT_HANDLE    ulId)
{
    return  (API_ThreadStartEx(ulId, LW_FALSE, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
