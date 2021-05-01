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
** ��   ��   ��: MsgQueueCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 06 ��
**
** ��        ��: ������Ϣ����

** BUG
2007.09.19  ���� _DebugHandle() ���ܡ�
2007.11.18  ����ע��.
2008.01.20  �ں�ȱ���ڴ�ʱ, Debug ��Ϣ����.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2009.04.08  ����� SMP ��˵�֧��.
2009.07.28  �������ĳ�ʼ�����ڳ�ʼ�����еĿ��ƿ���, ����ȥ����ز���.
2011.07.29  ������󴴽�/���ٻص�.
2014.05.31  ʹ�� ROUND_UP �������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_MsgQueueCreate
** ��������: ������Ϣ����
** �䡡��  : 
**           pcName                 ��Ϣ������������
**           ulMaxMsgCounter        �����Ϣ����
**           stMaxMsgByteSize       ÿ����Ϣ��󳤶�
**           ulOption               ��Ϣ����ѡ��
**           pulId                  ��Ϣ����IDָ��
** �䡡��  : ��Ϣ���о��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
LW_OBJECT_HANDLE  API_MsgQueueCreate (CPCHAR             pcName,
                                      ULONG              ulMaxMsgCounter,
                                      size_t             stMaxMsgByteSize,
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER ULONG                 ulI;
             PLW_CLASS_WAITQUEUE   pwqTemp[2];
    REGISTER ULONG                 ulIdTemp;
    
    REGISTER size_t                stMaxMsgByteSizeReal;
    REGISTER size_t                stHeapAllocateByteSize;
    REGISTER PVOID                 pvMemAllocate;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulMaxMsgCounter) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMaxMsgCounter invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MAX_COUNTER_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!stMaxMsgByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMaxMsgByteSize invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MAX_LEN_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pmsgqueue = _Allocate_MsgQueue_Object();                        /*  ������Ϣ����                */
    );
    
    if (!pmsgqueue) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a msgqueue.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pevent = _Allocate_Event_Object();                              /*  �����¼�                    */
    );
    
    if (!pevent) {
        __KERNEL_MODE_PROC(
            _Free_MsgQueue_Object(pmsgqueue);
        );
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a msgqueue.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    stMaxMsgByteSizeReal   = ROUND_UP(stMaxMsgByteSize, sizeof(size_t))
                           + sizeof(LW_CLASS_MSGNODE);                  /*  ÿ����Ϣ�����С            */
    
    stHeapAllocateByteSize = (size_t)ulMaxMsgCounter
                           * stMaxMsgByteSizeReal;                      /*  ��Ҫ������ڴ��ܴ�С        */
    
    pvMemAllocate = __KHEAP_ALLOC(stHeapAllocateByteSize);              /*  �����ڴ�                    */
    if (!pvMemAllocate) {
        __KERNEL_MODE_PROC(
            _Free_MsgQueue_Object(pmsgqueue);
            _Free_Event_Object(pevent);
        );
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pevent->EVENT_cEventName, pcName);
    } else {
        pevent->EVENT_cEventName[0] = PX_EOS;                           /*  �������                    */
    }
    
    pmsgqueue->MSGQUEUE_pvBuffer   = pvMemAllocate;
    pmsgqueue->MSGQUEUE_stMaxBytes = stMaxMsgByteSize;
    
    _MsgQueueClear(pmsgqueue, ulMaxMsgCounter);                         /*  ������׼����                */
    
    pevent->EVENT_ucType       = LW_TYPE_EVENT_MSGQUEUE;
    pevent->EVENT_pvTcbOwn     = LW_NULL;
    pevent->EVENT_ulCounter    = 0ul;
    pevent->EVENT_ulMaxCounter = ulMaxMsgCounter;
    pevent->EVENT_ulOption     = ulOption;
    pevent->EVENT_pvPtr        = (PVOID)pmsgqueue;
    
    pwqTemp[0] = &pevent->EVENT_wqWaitQ[0];
    pwqTemp[1] = &pevent->EVENT_wqWaitQ[1];
    pwqTemp[0]->WQ_usNum = 0;                                           /*  û���߳�                    */
    pwqTemp[1]->WQ_usNum = 0;
    
    for (ulI = 0; ulI < __EVENT_Q_SIZE; ulI++) {
        pwqTemp[0]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
        pwqTemp[1]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_MSGQUEUE, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pevent->EVENT_usIndex);                    /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_CREATE, 
                      ulIdTemp, ulMaxMsgCounter, stMaxMsgByteSize, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "msgqueue \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
