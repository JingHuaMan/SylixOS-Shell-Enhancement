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
** ��   ��   ��: ThreadRestart.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: �߳���������������

** BUG
2007.04.09  ȥ���� pstkButtom �� pstkLowAddress ������
2007.07.19  ������ _DebugHandle() ����.
2007.10.22  �޸��� _DebugHandle() һЩ��׼ȷ�ĵط�.
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.13  ʹ�� join ���ܷ�����������.
2007.12.24  ����ע��.
2008.03.29  ʹ���µĵȴ�����.
2008.03.30  ʹ���µľ���������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.06.03  ����� Create Hook �� Delete Hook ��֧��.
2008.06.03  �����Լ���Ҫ exce �̰߳���.
2009.05.24  ���µ����������, ��Ҫ��Э���������Ϣ����.
2009.05.28  ���� cleanup ����.
2009.07.14  ���̱߳� join ʱ, ��������.
2011.02.24  ����ʹ�ù��ϵ� TCB_ulResumeNesting ����.
2011.02.24  ����ʹ���߳����˯�ߵķ���, �����ý���״̬���ֻ���.
2011.08.17  ����ɾ���ص���, ��Ҫ���³�ʼ�� libc reent �ṹ.
2012.03.31  ��������ʱ, ������±�.
2012.12.23  ���� API_ThreadRestart() �㷨, 
            �������������, ����Ҫ�ص��ĺ������н�������ʹ�� except �߳�����.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.08.23  ���������ص�ִ�и��Ӱ�ȫ.
            ֻ��������ͬ�����ڵ�����.
2013.09.22  ������չ�����ӿ�, �������������ں���.
2013.12.11  ������������߳��������������̸߳ı�״̬, ����Ҫ�˳��ȴ�����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** ��������: __threadRestart
** ��������: �߳������ڲ�������
** �䡡��  : ptcb                   �������� TCB
**           pfuncThread            �߳��µ���� (LW_NULL ��ʾ���ı�)
**           pvArg                  ����
**           bIsAlreadyWaitDeath    �Ƿ��Ѿ�Ϊ����״̬
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_RESTART_EN > 0

static ULONG  __threadRestart (PLW_CLASS_TCB          ptcb, 
                               PTHREAD_START_ROUTINE  pfuncThread, 
                               PVOID                  pvArg, 
                               BOOL                   bIsAlreadyWaitDeath)
{
             INTREG                iregInterLevel;
             LW_OBJECT_HANDLE      ulId;
    REGISTER PLW_CLASS_PCB         ppcb;
    
    REGISTER PLW_STACK             pstkTop;
    
#if LW_CFG_THREAD_NOTE_PAD_EN > 0
    REGISTER UINT8                 ucI;
    REGISTER ULONG                *pulNote;
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    REGISTER PLW_EVENTSETNODE      pesnPtr;
#endif

    ulId = ptcb->TCB_ulId;

    if (bIsAlreadyWaitDeath == LW_FALSE) {
        _ThreadDeleteWaitDeath(ptcb);                                   /*  ��Ҫɾ�����߳̽��뽩��״̬  */
    }
    
#if LW_CFG_COROUTINE_EN > 0
    _CoroutineFreeAll(ptcb);                                            /*  �ͷ����е�Э��              */
    {
        REGISTER PLW_CLASS_COROUTINE  pcrcb = &ptcb->TCB_crcbOrigent;   /*  ��ʣ��һ��ԭʼ����Э��      */
        _List_Ring_Add_Ahead(&pcrcb->COROUTINE_ringRoutine,
                             &ptcb->TCB_pringCoroutineHeader);          /*  ����Э�̱�                  */
    }
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    lib_nlreent_init(ulId);                                             /*  reinit libc reent           */
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    
    ppcb = _GetPcb(ptcb);                                               /*  ���»�ȡ ppcb ��ֹ���޸�    */
    
    ptcb->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_NONE;              /*  �˳�����ɾ������            */
    ptcb->TCB_bRestartReq       = LW_FALSE;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (ptcb->TCB_ptcbDeleteWait) {                                     /*  Ŀ���߳����ڵȴ���������ɾ��*/
        ptcb->TCB_ptcbDeleteWait->TCB_ptcbDeleteMe = (PLW_CLASS_TCB)1;
        ptcb->TCB_ptcbDeleteWait = LW_NULL;
    }
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (ptcb->TCB_peventPtr) {                                          /*  �ȴ��¼���                  */
        _EventUnQueue(ptcb);                                            /*  ��ȴ���                    */
    }
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    pesnPtr = ptcb->TCB_pesnPtr;
    if (pesnPtr) {
        _EventSetUnQueue(pesnPtr);                                      /*  ���¼���                    */
    }
#endif
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_ptcbWaitStatus ||
        ptcb->TCB_plineStatusReqHeader) {                               /*  �������������̸߳ı�״̬    */
        _ThreadUnwaitStatus(ptcb);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
#if LW_CFG_THREAD_NOTE_PAD_EN > 0                                       /*  ������±�                  */
    pulNote = &ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[0];
    for (ucI = 0; ucI < LW_CFG_MAX_NOTEPADS; ucI++) {
        *pulNote++ = 0ul;
    }
#endif

    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  �û��ռ�                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    pstkTop = ptcb->TCB_pstkStackTop;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (pfuncThread) {
        ptcb->TCB_pthreadStartAddress = pfuncThread;                    /*  �����µ��߳����            */
    }
    
    archTaskCtxCreate(&ptcb->TCB_archRegCtx,
                      _ThreadShell,                                     /*  ��ʼ����ջ��SHELL           */
                      (PVOID)ptcb->TCB_pthreadStartAddress,
                      ptcb, pstkTop,
                      ptcb->TCB_ulOption);
                                          
    ptcb->TCB_pvArg = pvArg;
    
    /*
     *  ���ﲻ�������Լ������Լ�.
     */
    if (!__LW_THREAD_IS_READY(ptcb)) {                                  /*  ���ھ�����ʽ��ת��Ϊ������ʽ*/
        if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
            __DEL_FROM_WAKEUP_LINE(ptcb);                               /*  �ӵȴ�����ɾ��              */
            ptcb->TCB_ulDelay = 0ul;
        }
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;             /*  �жϼ��ʽ                */
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���뵽������ȼ�������      */
    }
    
    ptcb->TCB_usStatus = LW_THREAD_STATUS_RDY;
    ptcb->TCB_ulDelay  = 0ul;
    
    ptcb->TCB_ulSuspendNesting = 0ul;
    
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    if (ptcb->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcb);                                 /*  �� watch dog ��ɾ��         */
        ptcb->TCB_ulWatchDog = 0ul;                                     /*  �رտ��Ź�                  */
    }
#endif
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been restart.\r\n", ptcb->TCB_cThreadName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadRestart
** ��������: �߳���������������,���ﲻ�ı�ʱ��Ƭ���ԣ�������Ϊ��������ø����ʱ��Ƭ
** �䡡��  : ulId          ���
**           pvArg         ����
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadRestart (LW_OBJECT_HANDLE  ulId, PVOID  pvArg)
{
    return  (API_ThreadRestartEx(ulId, LW_NULL, pvArg));
}
/*********************************************************************************************************
** ��������: API_ThreadRestartEx
** ��������: �߳���������������,���ﲻ�ı�ʱ��Ƭ���ԣ�������Ϊ��������ø����ʱ��Ƭ
** �䡡��  : ulId              ���
**           pfuncThread       �߳��µ���� (LW_NULL ��ʾ���ı�)
**           pvArg             ����
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadRestartEx (LW_OBJECT_HANDLE  ulId, PTHREAD_START_ROUTINE  pfuncThread, PVOID  pvArg)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcb;
             ULONG                 ulError;
             
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
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    if (ptcbCur->TCB_pvVProcessContext != 
        ptcb->TCB_pvVProcessContext) {                                  /*  ������һ������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  ����Ƿ���ɾ��������        */
        if (ptcb == ptcbCur) {
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
    
    if (ptcb->TCB_ulThreadSafeCounter) {                                /*  �ڰ�ȫģʽ�µ��߳���������  */
        ptcb->TCB_bRestartReq = LW_TRUE;
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_RESTART_DELAY);
        return  (ERROR_THREAD_RESTART_DELAY);
    }
    if (ptcb->TCB_ptcbJoin) {                                           /*  �Ƿ��Ѻ������̺߳ϲ�        */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread join.\r\n");
        _ErrorHandle(ERROR_THREAD_JOIN);
        return  (ERROR_THREAD_JOIN);
    }
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_INIT) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread is Initializing.\r\n");
        _ErrorHandle(ERROR_THREAD_INIT);
        return  (ERROR_THREAD_INIT);
    }
    
    ptcb->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_RESTART;           /*  ������������                */
    
    ptcbCur->TCB_ulThreadSafeCounter++;                                 /*  LW_THREAD_SAFE();           */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ptcb == ptcbCur) {
        _ThreadRestartProcHook(ptcb);                                   /*  ���� hook ��ز���          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESTART, 
                          ulId, pvArg, LW_NULL);
        
        ptcbCur->TCB_ulThreadSafeCounter--;                             /*  LW_THREAD_UNSAFE();         */
        
        _excJobAdd((VOIDFUNCPTR)__threadRestart,
                   ptcb, (PVOID)pfuncThread, pvArg, 
                   (PVOID)LW_FALSE, 0, 0);                              /*  ʹ���ź�ϵͳ���쳣����      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  �ȴ���ɾ��                  */
        }
        ulError = ERROR_NONE;
    
    } else {                                                            /*  ɾ����������                */
        _ThreadDeleteWaitDeath(ptcb);                                   /*  ��Ҫɾ�����߳̽��뽩��״̬  */
        
        _ThreadRestartProcHook(ptcb);                                   /*  ���� hook ��ز���          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESTART, 
                          ulId, pvArg, LW_NULL);
        
        ulError = __threadRestart(ptcb, pfuncThread, pvArg, LW_TRUE);
    
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_THREAD_RESTART_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
