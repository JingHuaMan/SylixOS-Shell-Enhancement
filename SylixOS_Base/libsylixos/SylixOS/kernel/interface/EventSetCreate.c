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
** ��   ��   ��: EventSetCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: �����¼���

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock(); 
2008.05.31  ʹ�� __KERNEL_MODE_...().
2009.04.08  ����� SMP ��˵�֧��.
2009.07.28  �������ĳ�ʼ�����ڳ�ʼ�����еĿ��ƿ���, ����ȥ����ز���.
2011.02.23  ���봴��ѡ���.
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_EventSetCreate
** ��������: �����¼���
** �䡡��  : 
**           pcName          �¼�����������
**           ulInitEvent     ��ʼ���¼�
**           ulOption        �¼���ѡ��
**           pulId           �¼���IDָ��
** �䡡��  : �¼����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
LW_OBJECT_HANDLE  API_EventSetCreate (CPCHAR             pcName, 
                                      ULONG              ulInitEvent, 
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_EVENTSET    pes;
    REGISTER ULONG                 ulIdTemp;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    __KERNEL_MODE_PROC(
        pes = _Allocate_EventSet_Object();                              /*  ���һ���¼����ƿ�          */
    );
    
    if (!pes) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a eventset.\r\n");
        _ErrorHandle(ERROR_EVENTSET_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pes->EVENTSET_cEventSetName, pcName);
    } else {
        pes->EVENTSET_cEventSetName[0] = PX_EOS;                        /*  �������                    */
    }
    
    pes->EVENTSET_ucType        = LW_TYPE_EVENT_EVENTSET;
    pes->EVENTSET_plineWaitList = LW_NULL;
    pes->EVENTSET_ulEventSets   = ulInitEvent;
    pes->EVENTSET_ulOption      = ulOption;                             /*  ��¼ѡ��                    */
    
    ulIdTemp = _MakeObjectId(_OBJECT_EVENT_SET, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pes->EVENTSET_usIndex);                    /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_CREATE, 
                      ulIdTemp, ulInitEvent, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "eventset \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
