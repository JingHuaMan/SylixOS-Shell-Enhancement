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
** ��   ��   ��: EventSetGetName.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 23 ��
**
** ��        ��: ����¼�����

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_EventSetGetName
** ��������: ����¼�����
** �䡡��  : 
**           ulId                   �¼����
**           pcName                 ���ֻ�����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API
ULONG  API_EventSetGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENTSET    pes;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_NULL);
        return  (ERROR_KERNEL_PNAME_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_EVENT_SET)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_EventSet_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    __KERNEL_ENTER();
    if (_EventSet_Type_Invalid(usIndex, LW_TYPE_EVENT_EVENTSET)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENTSET_TYPE);
        return  (ERROR_EVENTSET_TYPE);
    }
#else
    __KERNEL_ENTER();
#endif

    pes = &_K_esBuffer[usIndex];

    lib_strcpy(pcName, pes->EVENTSET_cEventSetName);
    
    __KERNEL_EXIT();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
