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
** ��   ��   ��: SemaphoreCCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: �������ź�������

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2009.04.08  ����� SMP ��˵�֧��.
2009.07.28  �������ĳ�ʼ�����ڳ�ʼ�����еĿ��ƿ���, ����ȥ����ز���.
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreCCreate
** ��������: �������ź�������
** �䡡��  : 
**           pcName          �¼���������
**           ulInitCounter   ��ʼ������ֵ
**           ulMaxCounter    ������ֵ
**           ulOption        �¼�ѡ��
**           pulId           �¼�IDָ��
** �䡡��  : �¼����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
LW_OBJECT_HANDLE  API_SemaphoreCCreate (CPCHAR             pcName,
                                        ULONG              ulInitCounter, 
                                        ULONG              ulMaxCounter,
                                        ULONG              ulOption,
                                        LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER ULONG                 ulI;
             PLW_CLASS_WAITQUEUE   pwqTemp[2];
    REGISTER ULONG                 ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulMaxCounter) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "max counter invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_MAX_COUNTER_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (ulInitCounter > ulMaxCounter) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "initialize counter invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_INIT_COUNTER);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    __KERNEL_MODE_PROC(
        pevent = _Allocate_Event_Object();                              /*  ���һ���¼����ƿ�          */
    );
    
    if (!pevent) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a semaphore.\r\n");
        _ErrorHandle(ERROR_EVENT_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pevent->EVENT_cEventName, pcName);
    } else {
        pevent->EVENT_cEventName[0] = PX_EOS;                           /*  �������                    */
    }
                                                                        /*  ��ʼ��ʼ��                  */
    pevent->EVENT_ucType       = LW_TYPE_EVENT_SEMC;
    pevent->EVENT_ulCounter    = ulInitCounter;
    pevent->EVENT_ulMaxCounter = ulMaxCounter;
    pevent->EVENT_ulOption     = ulOption;
    pevent->EVENT_pvPtr        = LW_NULL;
    pevent->EVENT_pvTcbOwn     = LW_NULL;
    
    pwqTemp[0] = &pevent->EVENT_wqWaitQ[0];
    pwqTemp[1] = &pevent->EVENT_wqWaitQ[1];
    pwqTemp[0]->WQ_usNum = 0;                                           /*  û���߳�                    */
    pwqTemp[1]->WQ_usNum = 0;
    
    for (ulI = 0; ulI < __EVENT_Q_SIZE; ulI++) {
        pwqTemp[0]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
        pwqTemp[1]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_SEM_C, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pevent->EVENT_usIndex);                    /*  �������� id                 */

    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_SEMC, MONITOR_EVENT_SEM_CREATE, 
                      ulIdTemp, ulInitCounter, ulMaxCounter, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "semaphore \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_SEMC_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
