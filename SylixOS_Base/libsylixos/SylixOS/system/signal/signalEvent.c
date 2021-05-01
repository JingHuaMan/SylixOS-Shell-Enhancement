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
** ��   ��   ��: signalEvent.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 01 ��
**
** ��        ��: ���� sigevent �����ź�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#include "signalPrivate.h"
/*********************************************************************************************************
  ������ش���
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#define __tcb_pid(ptcb)     vprocGetPidByTcbNoLock(ptcb)
#else
#define __tcb_pid(ptcb)     0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
extern VOID         _sigPendInit(PLW_CLASS_SIGPEND  psigpend);
extern LW_SEND_VAL  _doSignal(PLW_CLASS_TCB  ptcb, PLW_CLASS_SIGPEND   psigpend);
/*********************************************************************************************************
  �������з��Ͳ�������
*********************************************************************************************************/
typedef union {
    LW_LIST_MONO            SE_monoLink;
    struct {
        struct sigevent     SE_sigevent;
        struct siginfo      SE_siginfo;
    } SE_event;
} SIGNAL_EVENT_ARG;
typedef SIGNAL_EVENT_ARG   *PSIGNAL_EVENT_ARG;
/*********************************************************************************************************
  �������з��Ͳ�����
*********************************************************************************************************/
#define SIGNAL_EVENT_POOL_SIZE  (NSIG + LW_CFG_MAX_EXCEMSGS)
static SIGNAL_EVENT_ARG         _G_sigeventargPool[SIGNAL_EVENT_POOL_SIZE];
static LW_LIST_MONO_HEADER      _G_pmonoSigEventArg;
/*********************************************************************************************************
  ����������
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(_G_slSigeventPool);
/*********************************************************************************************************
** ��������: __sigEventArgInit
** ��������: ��ʼ�� sig event ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __sigEventArgInit (VOID)
{
    INT                 i;
    PSIGNAL_EVENT_ARG   psigea1;
    PSIGNAL_EVENT_ARG   psigea2;
    
    LW_SPIN_INIT(&_G_slSigeventPool.SLCA_sl);

    psigea1 = &_G_sigeventargPool[0];
    psigea2 = &_G_sigeventargPool[1];
    
    for (i = 0; i < (SIGNAL_EVENT_POOL_SIZE - 1); i++) {
        _LIST_MONO_LINK(&psigea1->SE_monoLink, &psigea2->SE_monoLink);
        psigea1++;
        psigea2++;
    }
    
    _INIT_LIST_MONO_HEAD(&psigea1->SE_monoLink);
    
    _G_pmonoSigEventArg = &_G_sigeventargPool[0].SE_monoLink;
}
/*********************************************************************************************************
** ��������: __sigEventArgAlloc
** ��������: ��ȡһ�� sig event ��������
** �䡡��  : NONE
** �䡡��  : sig event ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PSIGNAL_EVENT_ARG  __sigEventArgAlloc (VOID)
{
    INTREG              iregInterLevel;
    PSIGNAL_EVENT_ARG   psigea;

    LW_SPIN_LOCK_QUICK(&_G_slSigeventPool.SLCA_sl, &iregInterLevel);
    if (_G_pmonoSigEventArg) {
        psigea = (PSIGNAL_EVENT_ARG)_list_mono_allocate(&_G_pmonoSigEventArg);
    } else {
        psigea = LW_NULL;
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slSigeventPool.SLCA_sl, iregInterLevel);

    return  (psigea);
}
/*********************************************************************************************************
** ��������: __sigEventArgFree
** ��������: �ͷ�һ�� sig event ��������
** �䡡��  : sigea     sig event ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigEventArgFree (PSIGNAL_EVENT_ARG  psigea)
{
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slSigeventPool.SLCA_sl, &iregInterLevel);
    _list_mono_free(&_G_pmonoSigEventArg, &psigea->SE_monoLink);
    LW_SPIN_UNLOCK_QUICK(&_G_slSigeventPool.SLCA_sl, iregInterLevel);
}
/*********************************************************************************************************
** ��������: _doSigEventInternal
** ��������: �� sigevent ����һ���ź��ڲ��ӿ�
** �䡡��  : ulId                   Ŀ���߳� ID
**           psigea                 �ź���Ϣ
**           bNeedFree              �ź���Ϣ�Ƿ���Ҫ�ͷ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _doSigEventInternal (LW_OBJECT_HANDLE  ulId, PSIGNAL_EVENT_ARG   psigea, BOOL  bNeedFree)
{
    REGISTER UINT16              usIndex;
    REGISTER PLW_CLASS_TCB       ptcb;
             LW_CLASS_SIGPEND    sigpend;
             INT                 iNotify;
             INT                 iError = PX_ERROR;
             
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL         sendval;
#endif
#if LW_CFG_MODULELOADER_EN > 0
             pid_t               pid;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
             
    struct sigevent  *psigevent = &psigea->SE_event.SE_sigevent;
    struct siginfo   *psiginfo  = &psigea->SE_event.SE_siginfo;

#if LW_CFG_POSIX_EN > 0
    iNotify = psigevent->sigev_notify & (~SIGEV_THREAD_ID);
    if (psigevent->sigev_notify & SIGEV_THREAD_ID) {
        ulId = psigevent->sigev_notify_thread_id;                       /*  ��ָ���̷߳����ź�          */
    }
#else
    iNotify = psigevent->sigev_notify;
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    
    if ((iNotify != SIGEV_SIGNAL) &&
        (iNotify != SIGEV_THREAD)) {
        goto    __out;
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  ���̺�                      */
        pid   = (pid_t)ulId;
        ulId  = vprocMainThread((pid_t)ulId);
    } else {
        pid = 0;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        goto    __out;
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        goto    __out;
    }
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGEVT, 
                      ulId, 
                      psigevent->sigev_signo, 
                      psigevent->sigev_notify,
                      psigevent->sigev_value.sival_ptr,
                      LW_NULL);
                      
    if (iNotify == SIGEV_THREAD) {
        _ErrorHandle(ENOSYS);
        goto    __out;
    }
    
    _sigPendInit(&sigpend);
    
    sigpend.SIGPEND_siginfo          = *psiginfo;
    sigpend.SIGPEND_siginfo.si_signo = psigevent->sigev_signo;          /*  �� sigevent Ϊ��            */
    sigpend.SIGPEND_siginfo.si_errno = errno;
    sigpend.SIGPEND_siginfo.si_value = psigevent->sigev_value;
    sigpend.SIGPEND_iNotify          = iNotify;
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        if (API_ThreadStop(ulId)) {
            goto    __out;
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
#if LW_CFG_MODULELOADER_EN > 0
    if ((psigevent->sigev_signo == SIGKILL) && (pid > 0)) {
        vprocKillPrepare(pid, ulId);                                    /*  ���� KILL Ԥ����            */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(EINVAL);
        goto    __out;
    }
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb->TCB_iDeleteProcStatus) {
#if LW_CFG_SMP_EN > 0
        if (LW_NCPUS > 1) {                                             /*  �������� SMP ���ģʽ       */
            _ThreadContinue(ptcb, LW_FALSE);                            /*  ���ں�״̬�»��ѱ�ֹͣ�߳�  */
        }
#endif                                                                  /*  LW_CFG_SMP_EN               */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        goto    __out;
    }
        
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doSignal(ptcb, &sigpend);
#else
    _doSignal(ptcb, &sigpend);
#endif
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  ���ں�״̬�»��ѱ�ֹͣ�߳�  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, sigpend.SIGPEND_siginfo.si_signo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */

    iError = ERROR_NONE;
    
__out:
    if (bNeedFree) {
        __sigEventArgFree(psigea);
    }
    return  (iError);
}
/*********************************************************************************************************
** ��������: _doSigEvent
** ��������: �� sigevent ����һ���ź�
** �䡡��  : ulId                   Ŀ���߳� ID
**           psigevent              �ź� event
**           iSigCode               si_code
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _doSigEvent (LW_OBJECT_HANDLE  ulId, struct sigevent *psigevent, INT  iSigCode)
{
    struct siginfo     *psiginfo;
    PLW_CLASS_TCB       ptcbCur;
    SIGNAL_EVENT_ARG    sigea;
    PSIGNAL_EVENT_ARG   psigea;
    BOOL                bExc;
    
    if (!psigevent) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (psigevent->sigev_notify == SIGEV_NONE) {                        /*  �������ź�                  */
        return  (ERROR_NONE);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == ptcbCur->TCB_ulId)) {
        psigea = __sigEventArgAlloc();
        if (psigea == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        bExc   = LW_TRUE;
        
    } else {
        psigea = &sigea;
        bExc   = LW_FALSE;
    }
    
    psigea->SE_event.SE_sigevent = *psigevent;
    psiginfo = &psigea->SE_event.SE_siginfo;
    psiginfo->si_code = iSigCode;
    psiginfo->si_pid  = __tcb_pid(ptcbCur);
    psiginfo->si_uid  = ptcbCur->TCB_uid;
    
    if (bExc) {
        if (_excJobAdd((VOIDFUNCPTR)_doSigEventInternal, (PVOID)ulId, (PVOID)psigea, 
                       (PVOID)LW_TRUE, 0, 0, 0) < ERROR_NONE) {
            __sigEventArgFree(psigea);
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);
    
    } else {
        return  (_doSigEventInternal(ulId, psigea, LW_FALSE));
    }
}
/*********************************************************************************************************
** ��������: _doSigEventEx
** ��������: �� sigevent ����һ���ź���չ�ӿ�
** �䡡��  : ulId                   Ŀ���߳� ID
**           psigevent              �ź� event
**           psiginfo               �ź���Ϣ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _doSigEventEx (LW_OBJECT_HANDLE  ulId, struct sigevent *psigevent, struct siginfo *psiginfo)
{
    PLW_CLASS_TCB       ptcbCur;
    SIGNAL_EVENT_ARG    sigea;
    PSIGNAL_EVENT_ARG   psigea;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!psigevent || !psiginfo) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (psigevent->sigev_notify == SIGEV_NONE) {                        /*  �������ź�                  */
        return  (ERROR_NONE);
    }
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == ptcbCur->TCB_ulId)) {
        psigea = __sigEventArgAlloc();
        if (psigea == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        psigea->SE_event.SE_sigevent = *psigevent;
        psigea->SE_event.SE_siginfo  = *psiginfo;
        
        if (_excJobAdd((VOIDFUNCPTR)_doSigEventInternal, (PVOID)ulId, (PVOID)psigea, 
                       (PVOID)LW_TRUE, 0, 0, 0) < ERROR_NONE) {
            __sigEventArgFree(psigea);
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);
                   
    } else {
        psigea = &sigea;
        
        psigea->SE_event.SE_sigevent = *psigevent;
        psigea->SE_event.SE_siginfo  = *psiginfo;
        
        return  (_doSigEventInternal(ulId, psigea, LW_FALSE));
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
