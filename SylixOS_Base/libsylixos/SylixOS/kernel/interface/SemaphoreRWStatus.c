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
** ��   ��   ��: SemaphoreRWStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 20 ��
**
** ��        ��: ��ѯ��д�ź���״̬
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreRWStatus
** ��������: ��ѯ��д�ź���״̬
** �䡡��  : 
**           ulId                   �¼����
**           pulRWCount             �ж����������ڲ���������д��  ����ΪNULL
**           pulRPend               ��ǰ����������������          ����ΪNULL
**           pulWPend               ��ǰ����д����������          ����ΪNULL
**           pulOption              �¼�ѡ��ָ��                  ����ΪNULL
**           pulThreadBlockNum      ���������߳�����              ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API 
ULONG  API_SemaphoreRWStatus (LW_OBJECT_HANDLE   ulId,
                              ULONG             *pulRWCount,
                              ULONG             *pulRPend,
                              ULONG             *pulWPend,
                              ULONG             *pulOption,
                              LW_OBJECT_HANDLE  *pulOwnerId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
             
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulRWCount) {
        *pulRWCount = pevent->EVENT_ulCounter;
    }
    
    if (pulRPend) {
        *pulRPend = (ULONG)_EventWaitNum(EVENT_RW_Q_R, pevent);
    }
    
    if (pulWPend) {
        *pulWPend = (ULONG)_EventWaitNum(EVENT_RW_Q_W, pevent);
    }
    
    if (pulOption) {
        *pulOption = pevent->EVENT_ulOption;
    }
    
    if (pevent->EVENT_iStatus == EVENT_RW_STATUS_W) {
        if (pulOwnerId) {
            *pulOwnerId = ((PLW_CLASS_TCB)(pevent->EVENT_pvTcbOwn))->TCB_ulId;
        }
    } else {
        if (pulOwnerId) {
            *pulOwnerId = 0ul;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
