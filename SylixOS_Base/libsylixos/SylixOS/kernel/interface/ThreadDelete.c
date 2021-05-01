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
** ��   ��   ��: ThreadDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: �߳�ɾ��������

** BUG
2007.01.08  �ڴ�����ͷţ������ж�����ʱ��
2007.07.18  ���� _DebugHandle() ����
2007.11.13  ���� join �̺߳�������߳̿��ƿ����.
2007.11.18  ����ע��.
2007.12.22  �Ķ���ɾ������, ���ȵ��ûص�(��������,���жϵ����), ����ɾ��������ȹ���. 
2007.12.24  ����ϵͳ��û�н��������ģʽʱ, ����ɾ���߳�.
2008.01.04  �Կ����жϵ�ʱ���������޸�, ��ִ���� HOOK ��, ���»�ȡ PCB, ��ʹ�޸������ȼ�, Ҳ������ִ���.
2008.03.29  ������µ� wake up �� watch dog ���ƵĴ���.
2008.03.30  ʹ���µľ���������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.06.21  ����Э�̵Ŀռ��ͷź���.
2009.02.03  �������� TCB ��չ��.
2009.05.08  ���� _CleanupPopTCBExt().
2009.05.24  �� TCB_bIsInDeleteProc ������ǰ.
2011.02.24  ����ʹ���߳����˯�ߵķ���, �����ý���״̬���ֻ���.
2011.08.05  ���� exit ����, ����ʹ�� #define exit API_ThreadExit
2012.12.08  ������Դ��������̬�ص�.
2012.12.23  ���� API_ThreadDelete() �㷨, 
            �����ɾ������, ����Ҫ�ص��ĺ������н�������ʹ�� except �߳�ɾ��.
2013.01.15  _exit() �� 1.0.0.rc37 ���Ϊϵͳ api. ���������ṩ�˺���.
2013.05.07  TCB ���ٷ��ڶ�ջ��, ���Ǵ� TCB ���з���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.08.23  ����ɾ���ص�ִ�и��Ӱ�ȫ.
            ֻ��ɾ����ͬ�����ڵ�����. �������̵�������Ҫʹ���ź�ɾ��.
2013.08.28  �����ں��¼������.
2013.09.10  �����ڰ�ȫģʽ��ɾ���Լ�, �߳̽������˳���ȫģʽ���Զ�ɾ��.
2013.09.21  �����ɾ���������̵߳����⴦��.
2013.12.11  �����ɾ�����߳��������������̸߳ı�״̬, ����Ҫ�˳��ȴ�����.
2014.01.01  ����ɾ�����ڵȴ�ɾ���������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  VMM ȱҳ�жϾ������
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
extern VOID __vmmPhysicalPageFaultClear(LW_OBJECT_HANDLE  ulId);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: __threadDelete
** ��������: �߳�ɾ���ڲ�������
** �䡡��  : 
**           ptcbDel                ɾ������ TCB
**           bIsInSafe              �Ƿ��ڰ�ȫģʽ�±�ɾ��
**           ulRetVal               ����ֵ   (���ظ� JOIN ���߳�)
**           bIsAlreadyWaitDeath    �Ƿ��Ѿ�Ϊ����״̬
** �䡡��  : ɾ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

ULONG  __threadDelete (PLW_CLASS_TCB  ptcbDel, BOOL  bIsInSafe, 
                       PVOID  pvRetVal, BOOL  bIsAlreadyWaitDeath)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_PCB         ppcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
    REGISTER PLW_STACK             pstkFree;                            /*  Ҫ�ͷŵ��ڴ��ַ            */
             BOOL                  bDetachFlag;
             INT                   iDetachCnt;

#if LW_CFG_MODULELOADER_EN > 0
             PVOID                 pvVProc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
             
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    REGISTER PLW_EVENTSETNODE      pesnPtr;
#endif

    ulId = ptcbDel->TCB_ulId;

#if LW_CFG_VMM_EN > 0
    __vmmPhysicalPageFaultClear(ulId);                                  /*  ���������Ϊ���������      */
#endif

    usIndex = _ObjectGetIndex(ulId);
    if (bIsAlreadyWaitDeath == LW_FALSE) {
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  ��Ҫɾ�����߳̽��뽩��״̬  */
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    
    ppcbDel = _GetPcb(ptcbDel);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (ptcbDel->TCB_ptcbDeleteWait) {                                  /*  Ŀ���߳����ڵȴ���������ɾ��*/
        ptcbDel->TCB_ptcbDeleteWait->TCB_ptcbDeleteMe = (PLW_CLASS_TCB)1;
        ptcbDel->TCB_ptcbDeleteWait = LW_NULL;
    }
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (ptcbDel->TCB_peventPtr) {                                       /*  �ȴ��¼���                  */
        _EventUnQueue(ptcbDel);                                         /*  ��ȴ���                    */
    }
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    pesnPtr = ptcbDel->TCB_pesnPtr;
    if (pesnPtr) {
        _EventSetUnQueue(pesnPtr);                                      /*  ���¼���                    */
    }
#endif

#if LW_CFG_SMP_EN > 0
    if (ptcbDel->TCB_ptcbWaitStatus ||
        ptcbDel->TCB_plineStatusReqHeader) {                            /*  �������������̸߳ı�״̬    */
        _ThreadUnwaitStatus(ptcbDel);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (__LW_THREAD_IS_READY(ptcbDel)) {                                /*  �Ƿ����                    */
        __DEL_FROM_READY_RING(ptcbDel, ppcbDel);                        /*  �Ӿ���������ɾ��            */
        
    } else {
        if (ptcbDel->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
            __DEL_FROM_WAKEUP_LINE(ptcbDel);                            /*  �ӵȴ�����ɾ��              */
            ptcbDel->TCB_ulDelay = 0ul;
        }
        ptcbDel->TCB_usStatus = LW_THREAD_STATUS_RDY;                   /*  ��ֹ Tick �жϼ���          */
    }
    
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    if (ptcbDel->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcbDel);                              /*  �� watch dog ��ɾ��         */
        ptcbDel->TCB_ulWatchDog = 0ul;
    }
#endif
    KN_INT_ENABLE(iregInterLevel);
    
    pstkFree = ptcbDel->TCB_pstkStackLowAddr;                           /*  ��¼��ַ                    */
    
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    _ThreadVarDelete(ptcbDel);                                          /*  ɾ�����ָ�˽�л���ȫ�ֱ���  */
#endif
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */ 
    
    _K_usThreadCounter--;
    _K_ptcbTCBIdTable[usIndex] = LW_NULL;                               /*  TCB ����0                   */
    
    _List_Line_Del(&ptcbDel->TCB_lineManage, &_K_plineTCBHeader);       /*  �ӹ���������ɾ��            */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    if (ptcbDel->TCB_ptcbJoin) {                                        /*  �˳� join ״̬, ������������*/
        _ThreadDisjoin(ptcbDel->TCB_ptcbJoin, ptcbDel, LW_FALSE, LW_NULL);
    }
    
    bDetachFlag = ptcbDel->TCB_bDetachFlag;                             /*  save detach flag            */
    iDetachCnt  = _ThreadDetach(ptcbDel, LW_NULL, pvRetVal);            /*  DETACH                      */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_COROUTINE_EN > 0
    _CoroutineFreeAll(ptcbDel);                                         /*  ɾ��Э���ڴ�ռ�            */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
    
    if (bIsInSafe) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, 
                     "thread \"%s\" has been delete in SAFE mode.\r\n",
                     ptcbDel->TCB_cThreadName);
    } else {
        _DebugFormat(__LOGMESSAGE_LEVEL, 
                     "thread \"%s\" has been delete.\r\n",
                     ptcbDel->TCB_cThreadName);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    pvVProc = ptcbDel->TCB_pvVProcessContext;                           /*  ������Ϣ                    */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (ptcbDel->TCB_ucStackAutoAllocFlag) {                            /*  �Ƿ����Զ�����              */
        _StackFree(ptcbDel, pstkFree);                                  /*  �ͷŶ�ջ�ռ�                */
    }
    
    _TCBDestroy(ptcbDel);                                               /*  ���� TCB                    */
    
    if (!bDetachFlag && !iDetachCnt &&                                  /*  û������ detach ��־        */
        !LW_KERN_AUTO_REC_TCB_GET() &&
        (ptcbDel->TCB_ulOption & LW_OPTION_THREAD_POSIX)) {             /*  POSIX �߳�                  */
        __KERNEL_MODE_PROC(
            _ThreadWjAdd(ptcbDel, &_K_twjTable[usIndex], pvRetVal);     /*  ���� TCB ֱ�� Join          */
        );

    } else {
        __KERNEL_MODE_PROC(
            _Free_Tcb_Object(ptcbDel);                                  /*  �ͷ� ID                     */
        );
    }
    
    __LW_OBJECT_DELETE_HOOK(ulId);

#if LW_CFG_MODULELOADER_EN > 0
    __resThreadDelHook(pvVProc, ulId);                                  /*  ��Դ���������þ�̬�ص�      */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadDelete
** ��������: �߳�ɾ��������
** �䡡��  : 
**           pulId         ���
**           pvRetVal      ����ֵ   (���ظ� JOIN ���߳�)
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadDelete (LW_OBJECT_HANDLE  *pulId, PVOID  pvRetVal)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
             ULONG                 ulError;
             
#if LW_CFG_MODULELOADER_EN > 0
             LW_LD_VPROC          *pvprocCur;
             LW_LD_VPROC          *pvprocDel;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳû���������ܵ���        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    pvprocCur = __LW_VP_GET_TCB_PROC(ptcbCur);
    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocCur != pvprocDel) {                                       /*  ������һ������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {
        if (ptcbCur != ptcbDel) {                                       /*  ���߳�ֻ���Լ�ɾ���Լ�      */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not delete main thread.\r\n");
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    if (ptcbDel->TCB_iDeleteProcStatus) {                               /*  ����Ƿ���ɾ��������        */
        if (ptcbDel == ptcbCur) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            for (;;) {
                API_TimeSleep(__ARCH_ULONG_MAX);                        /*  �ȴ���ɾ��                  */
            }
            return  (ERROR_NONE);                                       /*  ��Զ���в�������            */
        
        } else {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
    }
    
    if (ptcbDel->TCB_ulThreadSafeCounter) {                             /*  �ڰ�ȫģʽ��                */
        if (ptcbDel->TCB_ptcbDeleteMe) {                                /*  �Ѿ����߳�ɾ������          */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "another thread kill this thread now.\r\n");
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
        
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        
        ptcbDel->TCB_ptcbDeleteMe = ptcbCur;                            /*  ����ɾ���߳�                */
        ptcbDel->TCB_pvRetValue   = pvRetVal;                           /*  ��¼����ֵ                  */
        
        if (ptcbDel != ptcbCur) {
            ptcbCur->TCB_ptcbDeleteWait = ptcbDel;                      /*  ��¼�Լ��ȴ�ɾ��������      */
            _ThreadSafeSuspend(ptcbCur);                                /*  �����Լ��ȴ��Է�ɾ��        */
        }
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        return  (ERROR_NONE);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {            /*  ���߳��Լ�ɾ���Լ�          */
        if (pvprocDel->VP_iStatus != __LW_VP_EXIT) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            vprocExit(pvprocDel, ulId, (INT)(LONG)pvRetVal);            /*  ���̽���                    */
            return  (ERROR_NONE);                                       /*  �������е�����              */
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ptcbDel->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_DEL;            /*  ����ɾ������                */
    
    _ObjectCloseId(pulId);                                              /*  �ر� ID                     */
    
    ptcbCur->TCB_ulThreadSafeCounter++;                                 /*  LW_THREAD_SAFE();           */
    
    __KERNEL_SPACE_SET2(ptcbDel, 0);                                    /*  Ŀ�������˳��ں˻���        */

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ptcbDel == ptcbCur) {
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  ���� hook ��ز���          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        ptcbCur->TCB_ulThreadSafeCounter--;                             /*  LW_THREAD_UNSAFE();         */
        
        _excJobAdd((VOIDFUNCPTR)__threadDelete,
                   ptcbDel, (PVOID)LW_FALSE, pvRetVal, 
                   (PVOID)LW_FALSE, 0, 0);                              /*  ʹ���ź�ϵͳ���쳣����      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  �ȴ���ɾ��                  */
        }
        ulError = ERROR_NONE;
    
    } else {                                                            /*  ɾ����������                */
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  ��Ҫɾ�����߳̽��뽩��״̬  */
        
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  ���� hook ��ز���          */
    
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        ulError = __threadDelete(ptcbDel, LW_FALSE, pvRetVal, LW_TRUE);
        
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadExit
** ��������: �߳������˳���
** �䡡��  : 
**           pvRetVal       ����ֵ   (���ظ� JOIN ���߳�)
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadExit (PVOID  pvRetVal)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();
    
    return  (API_ThreadDelete(&ulId, pvRetVal));
}
/*********************************************************************************************************
** ��������: exit
** ��������: �ں��̻߳��߽����˳�
** �䡡��  : 
**           iCode         ����ֵ
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
void  exit (int  iCode)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvprocCur = __LW_VP_GET_CUR_PROC();
    
    if (pvprocCur) {
        if (pvprocCur->VP_ulMainThread == ulId) {
            vprocExit(pvprocCur, ulId, iCode);                          /*  ���̽���                    */
        }
#if LW_CFG_SIGNAL_EN > 0
          else {
            union sigval    sigvalue;
            sigvalue.sival_int = iCode;
            sigqueue(pvprocCur->VP_ulMainThread, SIGTERM, sigvalue);    /*  �����̷߳��ź��˳�          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    API_ThreadForceDelete(&ulId, (PVOID)(LONG)iCode);
    
    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);
    }
}
/*********************************************************************************************************
** ��������: _exit
** ��������: �ں��̻߳��߽����˳�, ��ִ�� atexit ��װ�ĺ���
** �䡡��  : 
**           iCode         ����ֵ
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
void  _exit (int  iCode)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvprocCur = __LW_VP_GET_CUR_PROC();
    
    if (pvprocCur) {
        pvprocCur->VP_bRunAtExit = LW_FALSE;                            /*  ��ִ�� atexit ��װ����      */
        if (pvprocCur->VP_ulMainThread == ulId) {
            vprocExit(pvprocCur, ulId, iCode);                          /*  ���̽���                    */
        }
#if LW_CFG_SIGNAL_EN > 0
          else {
            union sigval    sigvalue;
            sigvalue.sival_int = iCode;
            sigqueue(pvprocCur->VP_ulMainThread, SIGTERM, sigvalue);    /*  �����̷߳��ź��˳�          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    API_ThreadForceDelete(&ulId, (PVOID)(LONG)iCode);
    
    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);
    }
}

#ifdef __GNUC__
weak_alias(_exit, _Exit);
#else
void  _Exit (int  iCode)
{
    _exit(iCode);
}
#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
** ��������: atexit
** ��������: �����˳�ʱִ�еĲ���. (��ʹ�ö����ģʽʱ, ����ʹ�� vp patch �ض���)
** �䡡��  : 
**           iCode         ����ֵ   (���ظ� JOIN ���߳�)
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
int  atexit (void (*func)(void))
{
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_MODULELOADER_ATEXIT_EN > 0)
    return  (API_ModuleAtExit(func));

#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
