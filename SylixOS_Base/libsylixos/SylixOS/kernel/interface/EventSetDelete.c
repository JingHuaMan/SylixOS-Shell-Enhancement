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
** ��   ��   ��: EventSetDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: ɾ���¼���

** BUG
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock(); 
2008.05.31  ʹ�� __KERNEL_MODE_...().
2009.04.08  ����� SMP ��˵�֧��.
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_EventSetDelete
** ��������: ɾ���¼���
** �䡡��  : 
**           pulId                         �¼����ָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
ULONG  API_EventSetDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_EVENTSET        pes;
    REGISTER PLW_CLASS_EVENTSETNODE    pesn;
             PLW_LIST_LINE             plineList;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
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
#endif
    pes = &_K_esBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_EventSet_Type_Invalid(usIndex, LW_TYPE_EVENT_EVENTSET)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENTSET_TYPE);
        return  (ERROR_EVENTSET_TYPE);
    }
    
    _ObjectCloseId(pulId);                                              /*  ������                    */
    
    for (plineList  = pes->EVENTSET_plineWaitList;                      /*  ��ͷָ��                    */
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  �ͷ�������Ч�߳�            */
                  
        pesn = _LIST_ENTRY(plineList, LW_CLASS_EVENTSETNODE, EVENTSETNODE_lineManage);
        _EventSetDeleteReady(pesn);
    }
    
    pes->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pes->EVENTSET_plineWaitList = LW_NULL;
    
    _Free_EventSet_Object(pes);                                         /*  ����������                  */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_DELETE, ulId, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "eventset \"%s\" has been delete.\r\n", pes->EVENTSET_cEventSetName);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
