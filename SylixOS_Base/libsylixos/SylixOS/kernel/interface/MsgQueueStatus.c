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
** ��   ��   ��: MsgQueueStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 06 ��
**
** ��        ��: ��ѯ��Ϣ����״̬

** BUG
2007.09.19  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_MsgQueueStatus
** ��������: ��ѯ��Ϣ����״̬
** �䡡��  : 
**           ulId                   ��Ϣ���о��
**           pulMaxMsgNum           ��Ϣ������Ϣ������        ����ΪNULL
**           pulCounter             ��Ϣ������Ϣ����          ����ΪNULL
**           pstMsgLen              ��Ϣ�������һ����Ϣ��С  ����ΪNULL
**           pulOption              ��Ϣ����ѡ��ָ��          ����ΪNULL
**           pulThreadBlockNum      ���������߳�����          ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueStatus (LW_OBJECT_HANDLE   ulId,
                           ULONG             *pulMaxMsgNum,
                           ULONG             *pulCounter,
                           size_t            *pstMsgLen,
                           ULONG             *pulOption,
                           ULONG             *pulThreadBlockNum)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {                      /*  �����Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
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
    
    if (pulMaxMsgNum) {
        *pulMaxMsgNum = pevent->EVENT_ulMaxCounter;                     /*  ������Ϣ����              */
    }
    
    if (pulCounter) {
        *pulCounter = pevent->EVENT_ulCounter;                          /*  �����Ϣ����                */
    }
    
    if (pevent->EVENT_ulCounter) {
        if (pstMsgLen) {
            _MsgQueueMsgLen(pmsgqueue, pstMsgLen);                      /*  ����������Ϣ����          */
        }
    } else {
        if (pstMsgLen) {
            *pstMsgLen = 0;
        }
    }
    
    if (pulOption) {
        *pulOption  = pevent->EVENT_ulOption;                           /*  OPTION ѡ��                 */
    }
    
    if (pulThreadBlockNum) {
        *pulThreadBlockNum  = _EventWaitNum(EVENT_MSG_Q_R, pevent);     /*  �̵߳ȴ�����                */
        *pulThreadBlockNum += _EventWaitNum(EVENT_MSG_Q_S, pevent);     /*  �̵߳ȴ�����                */
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MSGQUEUE_EN > 0      */
                                                                        /*  LW_CFG_MAX_MSGQUEUES > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
