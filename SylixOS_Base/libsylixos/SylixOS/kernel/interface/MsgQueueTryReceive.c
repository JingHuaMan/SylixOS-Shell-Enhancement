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
** ��   ��   ��: MsgQueueTryReceive.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 07 ��
**
** ��        ��: �޵ȴ������Ϣ������Ϣ

** BUG
2007.09.19  ���� _DebugHandle() ���ܡ�
2009.04.08  ����� SMP ��˵�֧��.
2009.06.25  pulMsgLen ����Ϊ NULL,
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_MsgQueueTryReceive
** ��������: �޵ȴ������Ϣ������Ϣ
** �䡡��  : 
**           ulId            ��Ϣ���о��
**           pvMsgBuffer     ��Ϣ������
**           stMaxByteSize   ��Ϣ��������С
**           pstMsgLen       ��Ϣ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueTryReceive (LW_OBJECT_HANDLE    ulId,
                               PVOID               pvMsgBuffer,
                               size_t              stMaxByteSize,
                               size_t             *pstMsgLen)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_LIST_RING        *ppringList;
            
             size_t                stMsgLenTemp;
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  ��ʱ������¼��Ϣ����        */
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    if (pevent->EVENT_ulCounter) {                                      /*  �¼���Ч                    */
        pevent->EVENT_ulCounter--;
        
        _MsgQueueGet(pmsgqueue, pvMsgBuffer, 
                     stMaxByteSize, pstMsgLen);                         /*  �����Ϣ                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  �������ڵȴ�д��Ϣ          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  ���ȼ��ȴ�����              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  �������ȼ��ȴ��߳�          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  ����FIFO�ȴ��߳�            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  ʹ���ж�                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  ���� TCB                    */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
        }
        return  (ERROR_NONE);
    
    } else {                                                            /*  �¼���Ч                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  û���¼�����                */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
