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
** ��   ��   ��: SemaphoreCStatusEx.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 04 ��
**
** ��        ��: ��ѯ�������ź���״̬:�߼��ӿ�.

** BUG:
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreCStatusEx
** ��������: ��ѯ�������ź���״̬
** �䡡��  : 
**           ulId                   �¼����
**           pulCounter             �¼�����ֵ         ����ΪNULL
**           pulOption              �¼�ѡ��ָ��       ����ΪNULL
**           pulThreadBlockNum      ���������߳�����   ����ΪNULL
**           pulMaxCounter          ������ֵ         ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreCStatusEx (LW_OBJECT_HANDLE   ulId,
                               ULONG             *pulCounter,
                               ULONG             *pulOption,
                               ULONG             *pulThreadBlockNum,
                               ULONG             *pulMaxCounter)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_C)) {                         /*  �����Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMC)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulCounter) {
        *pulCounter = pevent->EVENT_ulCounter;                          /*  ��ǰ������ֵ                */
    }
    if (pulOption) {
        *pulOption  = pevent->EVENT_ulOption;                           /*  ѡ��                        */
    }
    if (pulThreadBlockNum) {
        *pulThreadBlockNum = _EventWaitNum(EVENT_SEM_Q, pevent);
    }
    
    if (pulMaxCounter) {
        *pulMaxCounter = pevent->EVENT_ulMaxCounter;                    /*  ������ֵ                  */
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMC_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
