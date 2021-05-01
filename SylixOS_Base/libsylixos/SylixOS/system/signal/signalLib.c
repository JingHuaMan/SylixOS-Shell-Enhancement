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
** ��   ��   ��: signalLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 06 �� 03 ��
**
** ��        ��: ϵͳ�źŴ����ڲ�������.
**
** ע        ��: ϵͳ��ֲ��ʱ��, ���뽫�����жϿ��Ʒ���������������. 
                 
** BUG
2007.09.08  һ������ _SignalShell �͹��ж�,Ȼ�������µ�������.
2008.01.16  ��ʼ�������Ϊ SIG_IGN ������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.08.11  _doPendKill ����Ծ�����͵��ж�, ���� shell ��һ����������·����źŽ� idle ɾ����, ������Ϊ
            û�м��������.
2009.03.05  ����ں˲ü����봦���.
2009.04.14  ���� SMP ֧��.
2009.05.26  �߳�������ʱ�յ��ź�, ���߳���ִ���źž��ʱ�� resume, ��ô�źŷ���ʱ��Ӧ���� suspend.
2010.01.22  __sigMakeReady() ��Ҫ�����ں�.
2011.02.22  �ع����� signal ϵͳ, ʵ�� POSIX Ҫ����źŹ���.
2011.02.24  __sigMakeReady() �� suspend ״̬���߳�����Ϊ��! ������źŷ���;�б�ǿռ, ��ǰ�����ջ�������
            Ŀǰֻ�ܵ� resume �����ִ���źž��.
2011.02.26  ����� SIGEV_NONE SIGEV_SIGNAL SIGEV_THREAD ��֧��.
2011.05.17  ���յ���Ҫ�����̵߳��ź�ʱ, ���Ͻ����߳�.
2011.08.16  __sigRunHandle() ��Ϊ default ����ʱ, ��Ҫ�ж��Ƿ�Ϊ��ֹ�ź�, �������Ҫ��ֹ����.
2011.11.17  ���� SIGSTOP SIGTSTP ������.
2012.04.09  ϵͳ��׼�źŷ��������� psiginfo ����.
2012.04.22  SIGKILL Ĭ�ϴ���ɵ����̱߳���ֹ.
2012.05.07  ʹ�� OSStkSetFp() ������ͬ�� fp ָ��, ��֤ unwind ��ջ����ȷ��.
2012.09.14  _doSigEvent() ֧�ֽ��̺�.
2012.10.23  ��� SMP ϵͳ��, �����ź�ʱ, ���Ŀ���߳�����һ�� CPU ִ��ʱ�Ĵ���.
2012.12.09  ������ͣʱ, ��Ҫ�Ը��׽���֪ͨ.
2012.12.25  ��� SIGSEGV �ź�, Ĭ���źž����ʾ������������Ϣ.
            �źŽ���֮ǰ��¼����Ŀ��������ں�״̬, Ȼ���źž������Ҫ���û�����������, �˳��ź�ʱ��Ҫ�ظ�
            ֮ǰ��״̬.
2013.01.15  �����쳣�ź��˳�ʱ��Ҫʹ�� _exit() api.
            __sigRunHandle() �����ǿ������ź�, �����Ƿ����û�����ִ�ж�����ɱ���Լ�.
2013.05.05  ֧�� SA_RESTART �ź�.
2013.06.17  �����ź������Ĳ����޸ĵ�ǰ���� errno
2013.09.04  ������������� SA_NOCLDSTOP ��ʾ, �򲻽����ӽ�����ͣ�ź�.
2013.09.17  ����� SIGCNCL �źŵĴ���.
2013.11.24  ����� signalfd ���ܵ�֧��.
2013.12.12  _sigGetLsb() ʹ�� archFindLsb() Ѱ������Ҫ���͵��źű����С���ź�.
2014.06.01  ��������ֹͣ�ź�, ���̽��ᱻǿ��ֹͣ.
2014.09.30  SA_SIGINFO ������ź������Ĳ����Ĵ���, ���ڽ���������е��ź�, ���ź������Ĳ���Ϊ LW_NULL.
2014.10.31  ֧�� POSIX �����ָ����ջ���ź������Ĳ���.
2014.11.04  ֧�� SA_NOCLDWAIT �Զ������ӽ���.
2015.11.16  __sigMakeReady() ����Ҫ�ر��жϼ���.
            _sigPendAlloc() �� _sigPendFree() ����Ҫ�ر��ж�.
2016.04.15  �ź���������Ҫ���� FPU ������.
2016.07.21  ������������.
2016.08.11  �½��̼߳������߳��ź�����.
2017.08.17  __sigCtlCreate() ��Ҫ�����ջ����.
2017.08.18  ���� __sigReturn() ����������ֵ���ܳ���Ĵ���.
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
#include "sys/vproc.h"
#include "sys/wait.h"
#include "unistd.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#define __tcb_pid(ptcb)     vprocGetPidByTcbNoLock(ptcb)
#else
#define __tcb_pid(ptcb)     0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  LW_CFG_MAX_SIGQUEUE_NODES ������� 2
*********************************************************************************************************/
#if     LW_CFG_MAX_SIGQUEUE_NODES < 2
#undef  LW_CFG_MAX_SIGQUEUE_NODES
#define LW_CFG_MAX_SIGQUEUE_NODES   2
#endif                                                                  /*  LW_CFG_MAX_SIGQUEUE_NODES   */
/*********************************************************************************************************
  �ź��ڲ�ȫ�ֱ���
*********************************************************************************************************/
static LW_CLASS_SIGCONTEXT      _K_sigctxTable[LW_CFG_MAX_THREADS];
static LW_CLASS_SIGPEND         _K_sigpendBuffer[LW_CFG_MAX_SIGQUEUE_NODES];
static LW_LIST_RING_HEADER      _K_pringSigPendFreeHeader;
#if LW_CFG_SIGNALFD_EN > 0
static LW_OBJECT_HANDLE         _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
/*********************************************************************************************************
  �ź��ڲ�����
*********************************************************************************************************/
extern VOID                     __sigEventArgInit(VOID);
static VOID                     __sigShell(PLW_CLASS_SIGCTLMSG  psigctlmsg);
       PLW_CLASS_SIGCONTEXT     _signalGetCtx(PLW_CLASS_TCB  ptcb);
static PLW_CLASS_SIGPEND        _sigPendAlloc(VOID);
       VOID                     _sigPendFree(PLW_CLASS_SIGPEND  psigpendFree);
       BOOL                     _sigPendRun(PLW_CLASS_TCB  ptcb);
static BOOL                     _sigPendRunSelf(VOID);
/*********************************************************************************************************
** ��������: __signalCnclHandle
** ��������: SIGCNCL �źŵķ�����
** �䡡��  : ptcbCur       ��ǰ����
**           iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalCnclHandle (PLW_CLASS_TCB  ptcbCur, INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE  ulId = ptcbCur->TCB_ulId;
    
    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
    } else {
        ptcbCur->TCB_bCancelRequest = LW_TRUE;
    }
}
/*********************************************************************************************************
** ��������: __signalExitHandle
** ��������: ��Ҫ�����˳����źŵķ�����
** �䡡��  : ptcbCur       ��ǰ����
**           iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalExitHandle (PLW_CLASS_TCB  ptcbCur, INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId = ptcbCur->TCB_ulId;
#if LW_CFG_MODULELOADER_EN > 0
    pid_t               pid  = vprocGetPidByTcb(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if ((iSigNo == SIGBUS)  ||
        (iSigNo == SIGABRT) ||
        (iSigNo == SIGSEGV) ||
        (iSigNo == SIGILL)  ||
        (iSigNo == SIGFPE)  ||
        (iSigNo == SIGSYS)) {                                           /*  ����������Ҫ�˳�            */
#if LW_CFG_MODULELOADER_EN > 0
        if (pid > 0) {
            vprocExitModeSet(pid, LW_VPROC_EXIT_FORCE);                 /*  ǿ�ƽ����˳�                */
            vprocSetImmediatelyTerm(pid);                               /*  �����˳�ģʽ                */
        }
        __LW_FATAL_ERROR_HOOK(pid, ulId, psiginfo);                     /*  �ؼ����쳣                  */
#else
        __LW_FATAL_ERROR_HOOK(0, ulId, psiginfo);                       /*  �ؼ����쳣                  */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        _exit(psiginfo->si_int);
    
#if LW_CFG_MODULELOADER_EN > 0
    } else if (iSigNo != SIGTERM) {                                     /*  ��ɾ����ǰ�߳�              */
        if (pid > 0 && vprocIsMainThread()) {
            vprocExitModeSet(pid, LW_VPROC_EXIT_FORCE);                 /*  ǿ�ƽ����˳�                */
            vprocSetImmediatelyTerm(pid);                               /*  �����˳�ģʽ                */
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    }
                                                                        /*  ɾ���Լ�                    */
    API_ThreadDelete(&ulId, (PVOID)(LONG)psiginfo->si_int);             /*  ����ڰ�ȫģʽ, ���˳���ȫ  */
}                                                                       /*  ģʽ��, �Զ���ɾ��          */
/*********************************************************************************************************
** ��������: __signalKillHandle
** ��������: SIGKILL �źŵķ�����
** �䡡��  : ptcbCur       ��ǰ����
**           iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __signalKillHandle (PLW_CLASS_TCB  ptcbCur, INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId = ptcbCur->TCB_ulId;
    pid_t               pid  = vprocGetPidByTcb(ptcbCur);

    if (pid > 0 && vprocIsMainThread()) {
        vprocExitModeSet(pid, LW_VPROC_EXIT_FORCE);                     /*  ǿ�ƽ����˳�                */
        vprocSetImmediatelyTerm(pid);                                   /*  �����˳�ģʽ                */
    }

    API_ThreadDelete(&ulId, (PVOID)(LONG)psiginfo->si_int);             /*  ɾ���Լ�                    */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: __signalWaitHandle
** ��������: �����ӽ�����Դ
** �䡡��  : ptcbCur       ��ǰ����
**           iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalWaitHandle (PLW_CLASS_TCB  ptcbCur, INT  iSigNo, struct siginfo *psiginfo)
{
#if LW_CFG_MODULELOADER_EN > 0
    reclaimchild(psiginfo->si_pid);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** ��������: __signalStopHandle
** ��������: SIGSTOP / SIGTSTP �źŵķ�����
** �䡡��  : ptcbCur       ��ǰ����
**           iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalStopHandle (PLW_CLASS_TCB  ptcbCur, INT  iSigNo, struct siginfo *psiginfo)
{
    sigset_t            sigsetMask;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsetMask = ~__SIGNO_UNMASK;
    
    sigdelset(&sigsetMask, SIGCONT);                                    /*  unmask SIGCONT              */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_STOPPED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsuspend(&sigsetMask);                                            /*  �����߳�                    */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_CONTINUED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** ��������: __signalStkShowHandle
** ��������: ��ӡ�����ķ�����
** �䡡��  : ptcbCur       ��ǰ����
**           psigctlmsg    �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalStkShowHandle (PLW_CLASS_TCB  ptcbCur, PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
#if LW_CFG_ABORT_CALLSTACK_INFO_EN > 0
    API_BacktraceShow(STD_OUT, 100);
#endif                                                                  /*  LW_CFG_ABORT_CALLSTACK_IN...*/

#if LW_CFG_DEVICE_EN > 0
    if (psigctlmsg) {
        archTaskCtxShow(STD_OUT, &psigctlmsg->SIGCTLMSG_archRegCtx);
    }
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
}
/*********************************************************************************************************
** ��������: __sigTaskCreateHook
** ��������: �߳̽���ʱ����ʼ���߳̿��ƿ��е��źſ��Ʋ���
** �䡡��  : ulId                  �߳� ID ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigTaskCreateHook (LW_OBJECT_HANDLE  ulId)
{
    PLW_CLASS_TCB          ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    PLW_CLASS_SIGCONTEXT   psigctx = _signalGetCtx(ptcb);
    
#if LW_CFG_MODULELOADER_EN > 0
    INT                    i;
    PLW_CLASS_TCB          ptcbCur;
    PLW_CLASS_SIGCONTEXT   psigctxCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    lib_bzero(psigctx, sizeof(LW_CLASS_SIGCONTEXT));                    /*  �����ź� DEFAULT ����       */

#if LW_CFG_MODULELOADER_EN > 0
    if (LW_SYS_STATUS_IS_RUNNING()) {                                   /*  ����ϵͳ��������            */
        LW_TCB_GET_CUR_SAFE(ptcbCur);                                   /*  ͬһ������                  */
        if (__LW_VP_GET_TCB_PROC(ptcb) == __LW_VP_GET_TCB_PROC(ptcbCur)) {
            psigctxCur = _signalGetCtx(ptcbCur);
            psigctx->SIGCTX_sigsetMask = psigctxCur->SIGCTX_sigsetMask; /*  �̳��ź�����                */
            
            for (i = 0; i < NSIG; i++) {
                psigctx->SIGCTX_sigaction[i] = psigctxCur->SIGCTX_sigaction[i];
            }
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    psigctx->SIGCTX_stack.ss_flags = SS_DISABLE;                        /*  ��ʹ���Զ����ջ            */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (_K_hSigfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _K_hSigfdSelMutex =  API_SemaphoreMCreate("sigfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    psigctx->SIGCTX_selwulist.SELWUL_hListLock = _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}
/*********************************************************************************************************
** ��������: __sigTaskDeleteHook
** ��������: �߳�ɾ��ʱ���õĻص�����
** �䡡��  : ulId                   �߳� ID ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

static VOID    __sigTaskDeleteHook (LW_OBJECT_HANDLE  ulId)
{
    REGISTER INT                    iI;
             PLW_CLASS_SIGCONTEXT   psigctx;
             PLW_CLASS_TCB          ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    REGISTER PLW_CLASS_SIGPEND      psigpend;
    
    psigctx = _signalGetCtx(ptcb);                                      /*  ��� sig context            */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (iI = 0; iI < NSIG; iI++) {
        if (psigctx->SIGCTX_pringSigQ[iI]) {                            /*  ����û�д�����Ŷ��ź�      */
                     PLW_LIST_RING  pringHead = psigctx->SIGCTX_pringSigQ[iI];
            REGISTER PLW_LIST_RING  pringSigP = pringHead;
            
            do {
                psigpend  = _LIST_ENTRY(pringSigP, 
                                        LW_CLASS_SIGPEND, 
                                        SIGPEND_ringSigQ);              /*  ��� sigpend ���ƿ��ַ     */
                pringSigP = _list_ring_get_next(pringSigP);             /*  ��һ���ڵ�                  */
                
                if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
                    (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {
                    _sigPendFree(psigpend);                             /*  ��Ҫ�������ж���            */
                }
            } while (pringSigP != pringHead);
        
            psigctx->SIGCTX_pringSigQ[iI] = LW_NULL;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    psigctx->SIGCTX_bRead     = LW_FALSE;
    psigctx->SIGCTX_sigsetFdw = 0ull;
    SEL_WAKE_UP_TERM(&psigctx->SIGCTX_selwulist);
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
/*********************************************************************************************************
** ��������: __sigMakeReady
** ��������: ��ָ���߳�����Ϊ����״̬�������õ������ķ���ֵΪ LW_SIGNAL_RESTART (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�ֵ
**           piSchedRet             �˳��źž����, ����������ֵ.
**           iSaType                �ź����� (LW_SIGNAL_EINTR or LW_SIGNAL_RESTART)
** �䡡��  : ���ڵȴ�ʱ��ʣ��ʱ��.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��Ŀ���̴߳��ڵȴ��¼�,�����ӳ�ʱ,��Ҫ���̵߳ĵ���������ֵ����Ϊ iSaType.
*********************************************************************************************************/
static VOID  __sigMakeReady (PLW_CLASS_TCB  ptcb, 
                             INT            iSigNo,
                             INT           *piSchedRet,
                             INT            iSaType)
{
             INTREG                  iregInterLevel;
    REGISTER PLW_CLASS_PCB           ppcb;
             PLW_CLASS_SIGCONTEXT    psigctx;

    *piSchedRet = ERROR_NONE;                                           /*  Ĭ��Ϊ����״̬              */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ���ھ���״̬, ֱ���˳�      */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return;
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_INIT) {                   /*  �߳̽�����ʼ��, ���߳̾���  */
        ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_INIT;                   /*  ȥ�� init ��־              */
    }

    ppcb = _GetPcb(ptcb);                                               /*  ������ȼ����ƿ�            */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  �����ڻ��Ѷ�����            */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
        ptcb->TCB_ulDelay = 0ul;
        *piSchedRet = iSaType;                                          /*  ���õ���������ֵ            */
    }
    
    if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                        /*  �����˳��ź�                */
        if (ptcb->TCB_ptcbJoin) {                                       /*  �˳� join ״̬, ������������*/
            _ThreadDisjoin(ptcb->TCB_ptcbJoin, ptcb, LW_FALSE, LW_NULL);
        }                                                               /*  ������ܲ����ظ��Ĳ���������*/
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  ����Ƿ��ڵȴ��¼�          */
        *piSchedRet = iSaType;                                          /*  ���õ���������ֵ            */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);             /*  �ȴ���ʱ����¼��ȴ�λ      */
        
        psigctx = _signalGetCtx(ptcb);
        if (psigctx->SIGCTX_sigwait &&
            psigctx->SIGCTX_sigwait->SIGWT_sigset & __sigmask(iSigNo)) {/*  sigwait                     */
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;
            
        } else {
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                 /*  �ȴ���ʱ                    */
        }

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        if (ptcb->TCB_peventPtr) {
            _EventUnQueue(ptcb);
        } else 
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
        {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
            if (ptcb->TCB_pesnPtr) {
                _EventSetUnQueue(ptcb->TCB_pesnPtr);
            }
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0) && */
        }
    } else {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  û�еȴ��¼�                */
    }
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
}
/*********************************************************************************************************
** ��������: __sigCtlCreate
** ��������: �ڶ�ջ�д���һ������ִ���źź��ź��˳��Ļ���
** �䡡��  : ptcb                   ������ƿ�
**           psigctx                �ź����������Ϣ
**           psiginfo               �ź���Ϣ
**           iSchedRet              �����ĵ���������ֵ
**           psigsetMask            ִ�����źž������Ҫ�������õ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  VOID  __sigCtlCreate (PLW_CLASS_TCB         ptcb,
                              PLW_CLASS_SIGCONTEXT  psigctx,
                              struct siginfo       *psiginfo,
                              INT                   iSchedRet,
                              sigset_t             *psigsetMask)
{
    PLW_CLASS_SIGCTLMSG  psigctlmsg;
    PLW_STACK            pstkSignalShell;                               /*  ����signalshell�Ķ�ջ��     */
    BYTE                *pucStkNow;
    stack_t             *pstack;

    if (psigctx && (psigctx->SIGCTX_stack.ss_flags == 0)) {             /*  ʹ���û�ָ�����źŶ�ջ      */
        PLW_STACK  pstkStackNow = archCtxStackEnd(&ptcb->TCB_archRegCtx);

        pstack = &psigctx->SIGCTX_stack;
        if ((pstkStackNow >= (PLW_STACK)pstack->ss_sp) &&
            (pstkStackNow < (PLW_STACK)((size_t)pstack->ss_sp + pstack->ss_size))) {
            pucStkNow = (BYTE *)pstkStackNow;                           /*  �Ѿ����û�ָ�����źŶ�ջ��  */
        
        } else {
#if	CPU_STK_GROWTH == 0
            pucStkNow = (BYTE *)pstack->ss_sp;
#else
            pucStkNow = (BYTE *)pstack->ss_sp + pstack->ss_size;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */
        }
    } else {
        pucStkNow = (BYTE *)archCtxStackEnd(&ptcb->TCB_archRegCtx);
    }

#if	CPU_STK_GROWTH == 0                                                 /*  CPU_STK_GROWTH == 0         */
    pucStkNow  += sizeof(LW_STACK);                                     /*  ���ջ�����ƶ�һ����ջ�ռ�  */
    pucStkNow   = (BYTE *)ROUND_UP(pucStkNow, ARCH_STK_ALIGN_SIZE);
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  ��¼ signal contrl msg λ�� */
    pucStkNow  += __SIGCTLMSG_SIZE_ALIGN;                               /*  �ó� signal contrl msg �ռ� */

#if LW_CFG_CPU_FPU_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        pucStkNow = (BYTE *)ROUND_UP(pucStkNow, ARCH_FPU_CTX_ALIGN);    /*  ���뱣֤���������Ķ���Ҫ��  */
        psigctlmsg->SIGCTLMSG_pfpuctx = (LW_FPU_CONTEXT *)pucStkNow;
        pucStkNow  += __SIGFPUCTX_SIZE_ALIGN;                           /*  �ó� signal fpu ctx �ռ�    */
        
    } else {
        psigctlmsg->SIGCTLMSG_pfpuctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
        pucStkNow = (BYTE *)ROUND_UP(pucStkNow, ARCH_DSP_CTX_ALIGN);    /*  ���뱣֤DSP�����Ķ���Ҫ��   */
        psigctlmsg->SIGCTLMSG_pdspctx = (LW_DSP_CONTEXT *)pucStkNow;
        pucStkNow  += __SIGDSPCTX_SIZE_ALIGN;                           /*  �ó� signal dsp ctx �ռ�    */

    } else {
        psigctlmsg->SIGCTLMSG_pdspctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

#else                                                                   /*  CPU_STK_GROWTH == 1         */
    pucStkNow  -= __SIGCTLMSG_SIZE_ALIGN;                               /*  �ó� signal contrl msg �ռ� */
    pucStkNow   = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_STK_ALIGN_SIZE);
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  ��¼ signal contrl msg λ�� */
    pucStkNow  -= sizeof(LW_STACK);                                     /*  ���ջ�����ƶ�һ����ջ�ռ�  */
    
#if LW_CFG_CPU_FPU_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        pucStkNow  -= __SIGFPUCTX_SIZE_ALIGN;                           /*  �ó� signal fpu ctx �ռ�    */
        pucStkNow   = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_FPU_CTX_ALIGN);/*  ���뱣֤���������Ķ���Ҫ��  */
        psigctlmsg->SIGCTLMSG_pfpuctx = (LW_FPU_CONTEXT *)pucStkNow;
        pucStkNow  -= sizeof(LW_STACK);                                 /*  ���ջ�����ƶ�һ����ջ�ռ�  */
        
    } else {
        psigctlmsg->SIGCTLMSG_pfpuctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
        pucStkNow  -= __SIGDSPCTX_SIZE_ALIGN;                           /*  �ó� signal dsp ctx �ռ�    */
        pucStkNow   = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_DSP_CTX_ALIGN);/*  ���뱣֤DSP�����Ķ���Ҫ��   */
        psigctlmsg->SIGCTLMSG_pdspctx = (LW_DSP_CONTEXT *)pucStkNow;
        pucStkNow  -= sizeof(LW_STACK);                                 /*  ���ջ�����ƶ�һ����ջ�ռ�  */

    } else {
        psigctlmsg->SIGCTLMSG_pdspctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
#endif                                                                  /*  CPU_STK_GROWTH              */

    psigctlmsg->SIGCTLMSG_iSchedRet       = iSchedRet;
    psigctlmsg->SIGCTLMSG_iKernelSpace    = __KERNEL_SPACE_GET2(ptcb);
    psigctlmsg->SIGCTLMSG_archRegCtx      = ptcb->TCB_archRegCtx;
    psigctlmsg->SIGCTLMSG_siginfo         = *psiginfo;
    psigctlmsg->SIGCTLMSG_sigsetMask      = *psigsetMask;
    psigctlmsg->SIGCTLMSG_ulLastError     = ptcb->TCB_ulLastError;      /*  ��¼��ǰ�����              */
    psigctlmsg->SIGCTLMSG_ucWaitTimeout   = ptcb->TCB_ucWaitTimeout;    /*  ��¼ timeout ��־           */
    psigctlmsg->SIGCTLMSG_ucIsEventDelete = ptcb->TCB_ucIsEventDelete;
    
    pstkSignalShell = archTaskCtxCreate(&ptcb->TCB_archRegCtx,
                                        (PTHREAD_START_ROUTINE)__sigShell,
                                        (PVOID)psigctlmsg,
                                        ptcb, (PLW_STACK)pucStkNow,
                                        ptcb->TCB_ulOption);            /*  �����ź���ǻ���            */
    
    archTaskCtxSetFp(pstkSignalShell,
                     &ptcb->TCB_archRegCtx,
                     &psigctlmsg->SIGCTLMSG_archRegCtx);                /*  ���� fp, ʹ callstack ����  */
    
    _StackCheckGuard(ptcb);                                             /*  ��ջ������                */
    
    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  �źž������������״̬��    */
}
/*********************************************************************************************************
** ��������: __sigReturn
** ��������: �źž������Ǻ������ô˺������ź��������з�������������.
** �䡡��  : psigctx                 �ź����������Ϣ
**           ptcbCur                 ��ǰ������ƿ�
**           psigctlmsg              �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigReturn (PLW_CLASS_SIGCONTEXT  psigctx, 
                          PLW_CLASS_TCB         ptcbCur, 
                          PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
    INTREG   iregInterLevel;

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx->SIGCTX_sigsetMask = psigctlmsg->SIGCTLMSG_sigsetMask;
                                                                        /*  �ָ�ԭ�ȵ�����              */
    _sigPendRunSelf();                                                  /*  ��鲢������Ҫ���е��ź�    */
    __KERNEL_SPACE_SET(psigctlmsg->SIGCTLMSG_iKernelSpace);             /*  �ָ��ɽ����ź�ǰ��״̬      */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    _SchedSetRet(psigctlmsg->SIGCTLMSG_iSchedRet);                      /*  ֪ͨ���������ص����        */
#if LW_CFG_CPU_FPU_EN > 0
    if (psigctlmsg->SIGCTLMSG_pfpuctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP)) {
        __ARCH_FPU_RESTORE((PVOID)psigctlmsg->SIGCTLMSG_pfpuctx);       /*  �ָ����ź��ж�ǰ FPU ������ */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#if LW_CFG_CPU_DSP_EN > 0
    if (psigctlmsg->SIGCTLMSG_pdspctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP)) {
        __ARCH_DSP_RESTORE((PVOID)psigctlmsg->SIGCTLMSG_pdspctx);       /*  �ָ����ź��ж�ǰ DSP ������ */
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
    
    ptcbCur->TCB_ulLastError     = psigctlmsg->SIGCTLMSG_ulLastError;   /*  �ָ������                  */
    ptcbCur->TCB_ucWaitTimeout   = psigctlmsg->SIGCTLMSG_ucWaitTimeout; /*  �ָ� timeout ��־           */
    ptcbCur->TCB_ucIsEventDelete = psigctlmsg->SIGCTLMSG_ucIsEventDelete;
    
    KN_SMP_MB();
    archSigCtxLoad(&psigctlmsg->SIGCTLMSG_archRegCtx);                  /*  ���ź��������з���          */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���в�������                */
}
/*********************************************************************************************************
** ��������: __sigRunHandle
** ��������: �ź������Ѿ���װ�ľ��
** �䡡��  : psigctx               �ź����������Ϣ
**           iSigNo                �źŵ�ֵ
**           psiginfo              ��Ҫ���е��ź���Ϣ
**           psigctlmsg            �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigRunHandle (PLW_CLASS_SIGCONTEXT  psigctx, 
                             INT                   iSigNo, 
                             struct siginfo       *psiginfo, 
                             PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
    REGISTER struct sigaction     *psigaction;
             PLW_CLASS_TCB         ptcbCur;
    REGISTER VOIDFUNCPTR           pfuncHandle;
             PVOID                 pvCtx;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��õ�ǰ TCB                */

    psigaction  = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
    pfuncHandle = (VOIDFUNCPTR)psigaction->sa_handler;                  /*  ����ź�ִ�к������        */
    
    if (psigaction->sa_flags & SA_ONESHOT) {                            /*  �����ػ���һ���ź�          */
        psigaction->sa_handler = SIG_DFL;                               /*  ����Ĭ�ϴ���                */
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_MODULELOADER_EN > 0                                          /*  ���� KILL ��ִ�а�װ���    */
    if (iSigNo == SIGKILL && __LW_VP_GET_TCB_PROC(ptcbCur)) {
        __signalKillHandle(ptcbCur, iSigNo, psiginfo);                  /*  �����˳�                    */

    } else
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    {
        if ((pfuncHandle != SIG_IGN)   &&
            (pfuncHandle != SIG_ERR)   &&
            (pfuncHandle != SIG_DFL)   &&
            (pfuncHandle != SIG_CATCH) &&
            (pfuncHandle != SIG_HOLD)) {                                /*  ��Ҫִ���û����            */
            pvCtx = (psigctlmsg)
                  ? &psigctlmsg->SIGCTLMSG_archRegCtx
                  : LW_NULL;

            if (psigaction->sa_flags & SA_SIGINFO) {                    /*  ��Ҫ siginfo_t ��Ϣ         */
                LW_SOFUNC_PREPARE(pfuncHandle);
                pfuncHandle(iSigNo, psiginfo, pvCtx);                   /*  ִ���źž��                */

            } else {
                LW_SOFUNC_PREPARE(pfuncHandle);
                pfuncHandle(iSigNo, pvCtx);                             /*  XXX �Ƿ��� pvCtx ���� ?   */
            }
        
            if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                /*  �����˳�                    */
                __signalExitHandle(ptcbCur, iSigNo, psiginfo);

            } else if (iSigNo == SIGCNCL) {                             /*  �߳�ȡ���ź�                */
                __signalCnclHandle(ptcbCur, iSigNo, psiginfo);
            }
        
        } else {                                                        /*  ��������                    */
            switch (iSigNo) {                                           /*  Ĭ�ϴ�����                */
            
            case SIGINT:
            case SIGQUIT:
            case SIGFPE:
            case SIGKILL:
            case SIGBUS:
            case SIGTERM:
            case SIGABRT:
            case SIGILL:
            case SIGSEGV:
            case SIGSYS:
                __signalExitHandle(ptcbCur, iSigNo, psiginfo);
                break;

            case SIGSTOP:
                __signalStopHandle(ptcbCur, iSigNo, psiginfo);
                break;

            case SIGTSTP:
                if (pfuncHandle == SIG_DFL) {
                    __signalStopHandle(ptcbCur, iSigNo, psiginfo);
                }
                break;

            case SIGCHLD:
                if (pfuncHandle == SIG_IGN) {                           /*  IGN ʱ�Զ�����              */
                    if ((psiginfo->si_code == CLD_EXITED) ||
                        (psiginfo->si_code == CLD_KILLED) ||
                        (psiginfo->si_code == CLD_DUMPED)) {            /*  �����ӽ�����Դ              */
                        __signalWaitHandle(ptcbCur, iSigNo, psiginfo);
                    }
                }
                break;

            case SIGCNCL:
                __signalCnclHandle(ptcbCur, iSigNo, psiginfo);
                break;

            case SIGSTKSHOW:
                if (pfuncHandle == SIG_DFL) {
                    __signalStkShowHandle(ptcbCur, psigctlmsg);
                }
                break;

            default:
                break;
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __sigShell
** ��������: �ź����е���Ǻ���
** �䡡��  : psigctlmsg              �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigShell (PLW_CLASS_SIGCTLMSG  psigctlmsg)
{
             INTREG                iregInterLevel;
             PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_SIGCONTEXT  psigctx;
    REGISTER struct siginfo       *psiginfo = &psigctlmsg->SIGCTLMSG_siginfo;
    REGISTER INT                   iSigNo   = psiginfo->si_signo;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
#if LW_CFG_CPU_FPU_EN > 0
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    if (psigctlmsg->SIGCTLMSG_pfpuctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP)) {           /*  ��ǰ�߳�ʹ�� FPU            */
        *psigctlmsg->SIGCTLMSG_pfpuctx = ptcbCur->TCB_fpuctxContext;
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  �򿪵�ǰ CPU �ж�           */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    if (psigctlmsg->SIGCTLMSG_pdspctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP)) {          /*  ��ǰ�߳�ʹ�� DSP            */
        *psigctlmsg->SIGCTLMSG_pdspctx = ptcbCur->TCB_dspctxContext;
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  �򿪵�ǰ CPU �ж�           */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGRUN, 
                      ptcbCur->TCB_ulId, iSigNo, psiginfo->si_code, LW_NULL);
    
    __sigRunHandle(psigctx, iSigNo, psiginfo, psigctlmsg);              /*  �����źž��                */
    
    __sigReturn(psigctx, ptcbCur, psigctlmsg);                          /*  �źŷ���                    */
}
/*********************************************************************************************************
** ��������: _signalInit
** ��������: ȫ�ֳ�ʼ���źŶ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _signalInit (VOID)
{
    REGISTER INT    i;
    
    __sigEventArgInit();
    
    for (i = 0; i < LW_CFG_MAX_SIGQUEUE_NODES; i++) {
        _List_Ring_Add_Last(&_K_sigpendBuffer[i].SIGPEND_ringSigQ, 
                            &_K_pringSigPendFreeHeader);                /*  �����������                */
    }
    
    API_SystemHookAdd(__sigTaskCreateHook, 
                      LW_OPTION_THREAD_CREATE_HOOK);                    /*  ��Ӵ����ص�����            */

#if LW_CFG_THREAD_DEL_EN > 0
    API_SystemHookAdd(__sigTaskDeleteHook, 
                      LW_OPTION_THREAD_DELETE_HOOK);                    /*  ���ɾ���ص�����            */
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
}
/*********************************************************************************************************
** ��������: _sigGetMsb
** ��������: ��һ���źż��л�ȡ�ź���ֵ. (���ȵ����ź�ֵС���ź�)
** �䡡��  : psigset        �źż�
** �䡡��  : �ź���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _sigGetLsb (sigset_t  *psigset)
{
    UINT32  uiHigh = (UINT32)((*psigset) >> 32);
    UINT32  uiLow  = (UINT32)((*psigset) & 0xffffffff);
    
    if (uiHigh) {
        return  (archFindLsb(uiHigh) + 32);
    
    } else {
        return  (archFindLsb(uiLow));
    }
}
/*********************************************************************************************************
** ��������: _signalGetCtx
** ��������: ���ָ���̵߳� sig context �ṹ
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
** �䡡��  : sig context �ṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_SIGCONTEXT  _signalGetCtx (PLW_CLASS_TCB  ptcb)
{
    return  (&_K_sigctxTable[_ObjectGetIndex(ptcb->TCB_ulId)]);
}
/*********************************************************************************************************
** ��������: _sigGetAction
** ��������: ���ָ���̵߳��ź� action
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź���ֵ
**           psigaction             �źŷ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _sigGetAction (PLW_CLASS_TCB  ptcb, INT  iSigNo, struct sigaction *psigaction)
{
    PLW_CLASS_SIGCONTEXT  psigctx;

    psigctx = _signalGetCtx(ptcb);

    psigaction->sa_handler  = psigctx->SIGCTX_sigaction[iSigNo].sa_handler;
    psigaction->sa_flags    = psigctx->SIGCTX_sigaction[iSigNo].sa_flags;
    psigaction->sa_mask     = psigctx->SIGCTX_sigaction[iSigNo].sa_mask;
    psigaction->sa_restorer = psigctx->SIGCTX_sigaction[iSigNo].sa_restorer;
}
/*********************************************************************************************************
** ��������: _sigPendAlloc
** ��������: ���һ�����е��źŶ��нڵ� (�˺����ڽ����ں˺����)
** �䡡��  : NONE
** �䡡��  : ������ڿ��нڵ�, �򷵻ص�ַ, ���򷵻� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_CLASS_SIGPEND   _sigPendAlloc (VOID)
{
    PLW_CLASS_SIGPEND   psigpendNew = LW_NULL;
    
    if (_K_pringSigPendFreeHeader) {
        psigpendNew = _LIST_ENTRY(_K_pringSigPendFreeHeader, LW_CLASS_SIGPEND, 
                                  SIGPEND_ringSigQ);                    /*  ��ÿ��п��ƿ�ĵ�ַ        */
        _List_Ring_Del(_K_pringSigPendFreeHeader, 
                       &_K_pringSigPendFreeHeader);                     /*  �ӿ��ж�����ɾ��            */
    }
    
    return  (psigpendNew);
}
/*********************************************************************************************************
** ��������: _sigPendFree
** ��������: �ͷ�һ���źŶ��нڵ�. (�˺����ڽ����ں˺����)
** �䡡��  : psigpendFree      ��Ҫ�ͷŵĽڵ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _sigPendFree (PLW_CLASS_SIGPEND  psigpendFree)
{
    _List_Ring_Add_Last(&psigpendFree->SIGPEND_ringSigQ, 
                        &_K_pringSigPendFreeHeader);                    /*  �黹�����ж�����            */
}
/*********************************************************************************************************
** ��������: _sigPendInit
** ��������: ��ʼ��һ���źŶ��нڵ�.
** �䡡��  : psigpend               ��Ҫ��ʼ���Ľڵ��ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _sigPendInit (PLW_CLASS_SIGPEND  psigpend)
{
    _LIST_RING_INIT_IN_CODE(psigpend->SIGPEND_ringSigQ);
    
    psigpend->SIGPEND_uiTimes = 0;
    psigpend->SIGPEND_iNotify = SIGEV_SIGNAL;
    psigpend->SIGPEND_psigctx = LW_NULL;
}
/*********************************************************************************************************
** ��������: _sigPendGet
** ��������: ��ȡһ����Ҫ�����е��ź�, (�˺����ڽ����ں˺����)
** �䡡��  : psigctx               ������ƿ����ź�������
**           psigset               ��Ҫ�����źż�
**           psiginfo              ��Ҫ���е��ź���Ϣ
** �䡡��  : �ź���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _sigPendGet (PLW_CLASS_SIGCONTEXT  psigctx, const sigset_t  *psigset, struct siginfo *psiginfo)
{
    INT                 iSigNo;
    INT                 iSigIndex;
    sigset_t            sigsetNeedRun;
    PLW_CLASS_SIGPEND   psigpend;
    
    sigsetNeedRun = *psigset & psigctx->SIGCTX_sigsetPending;
    if (sigsetNeedRun == 0ull) {                                        /*  ��Ҫ�����źż�������Ҫ����*/
        return  (0);
    }
    
    sigsetNeedRun &= (-sigsetNeedRun);                                  /*  ȡ sigsetNeedRun �����λ   */
    iSigNo         = _sigGetLsb(&sigsetNeedRun);
    iSigIndex      = __sigindex(iSigNo);
    
    if (sigsetNeedRun & psigctx->SIGCTX_sigsetKill) {                   /*  �� kill ���ź���Ҫ������    */
        psigctx->SIGCTX_sigsetKill     &= ~sigsetNeedRun;
        psigctx->SIGCTX_sigsetPending  &= ~sigsetNeedRun;
        
        psiginfo->si_signo           = iSigNo;
        psiginfo->si_errno           = ERROR_NONE;
        psiginfo->si_code            = SI_KILL;
        psiginfo->si_value.sival_int = iSigNo;
    
    } else {                                                            /*  û�� kill, ��һ�����Ŷ��ź� */
        psigpend = _LIST_ENTRY(psigctx->SIGCTX_pringSigQ[iSigIndex], 
                               LW_CLASS_SIGPEND, SIGPEND_ringSigQ);

        if (psigpend->SIGPEND_uiTimes == 0) {                           /*  �� pend ������ɾ��          */
            _List_Ring_Del(&psigpend->SIGPEND_ringSigQ, 
                           &psigctx->SIGCTX_pringSigQ[iSigIndex]);
        } else {
            psigpend->SIGPEND_uiTimes--;
        }
        
        *psiginfo = psigpend->SIGPEND_siginfo;
        
        if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
            (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {      /*  ʹ�� queue �����ź�         */
            _sigPendFree(psigpend);                                     /*  ��Ҫ�������ж���            */
        }

        if (psigctx->SIGCTX_pringSigQ[iSigIndex] == LW_NULL) {          /*  ���źŶ��������� pend       */
            psigctx->SIGCTX_sigsetPending &= ~sigsetNeedRun;
        }
    }
    
    return  (iSigNo);
}
/*********************************************************************************************************
** ��������: _sigPendRunSelf
** ��������: ��ǰ�߳��������еȴ����ź�. (�˺����ڽ����ں˺����)
** �䡡��  : NONE
** �䡡��  : �Ƿ��������źŴ�����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static BOOL _sigPendRunSelf (VOID)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  �����Ҫ�����źż�        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  �ź�ִ�������Ҫ�ظ�������  */
    BOOL                    bIsRun = LW_FALSE;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    psigctx = _signalGetCtx(ptcbCur);
    sigset  = ~psigctx->SIGCTX_sigsetMask;                              /*  û�б����ε��ź�            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  û����Ҫ�����е��ź�        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  ��¼��ǰ������              */
    
    do {
        iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);               /*  �����Ҫ���е��ź�          */
        if (__issig(iSigNo)) {
            struct sigaction     *psigaction;
                
            psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
            
            psigctx->SIGCTX_sigsetMask |= psigaction->sa_mask;
            if ((psigaction->sa_flags & SA_NOMASK) == 0) {              /*  ��ֹ��ͬ�ź�Ƕ��            */
                psigctx->SIGCTX_sigsetMask |= __sigmask(iSigNo);
            }
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            __sigRunHandle(psigctx, iSigNo, &siginfo, LW_NULL);         /*  ֱ�������źž��            */
            __KERNEL_ENTER();                                           /*  ���½����ں�                */
            
            psigctx->SIGCTX_sigsetMask = sigsetOld;
            bIsRun = LW_TRUE;
        
        } else {
            break;
        }
    } while (1);
    
    return  (bIsRun);
}
/*********************************************************************************************************
** ��������: _sigPendRun
** ��������: ���еȴ����ź�. (�˺����ڽ����ں˺����)
** �䡡��  : ptcb                  ������ƿ�
** �䡡��  : �Ƿ��������źž��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� ptcb == CPU_ptcbTCBCur ʱ, ϵͳ��ֱ�������źž��, ���ҽ����п������е� pend �ź�, ȫ������
                ptcb != CPU_ptcbTCBCur ʱ, ϵͳ����ָ�����߳��ڹ����źŻ���, �����ź�ִ��ʱ, β�����Զ�ִ��
                                           ���п������е� pend �ź�.
*********************************************************************************************************/
BOOL  _sigPendRun (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  �����Ҫ�����źż�        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  �ź�ִ�������Ҫ�ظ�������  */
    INT                     iSchedRet;                                  /*  ��Ҫ���µȴ��¼�            */

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if (ptcb == ptcbCur) {
        return  (_sigPendRunSelf());
    }

    psigctx = _signalGetCtx(ptcb);
    sigset  = ~psigctx->SIGCTX_sigsetMask;                              /*  û�б����ε��ź�            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  û����Ҫ�����е��ź�        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  ��¼��ǰ������              */
    
    iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);                   /*  �����Ҫ���е��ź�          */
    if (__issig(iSigNo)) {
        struct sigaction *psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
        INT iSaType;
        
        if (psigaction->sa_flags & SA_RESTART) {
            iSaType = LW_SIGNAL_RESTART;                                /*  ��������                    */
        } else {
            iSaType = LW_SIGNAL_EINTR;                                  /*  ���� EINTR                  */
        }
        
        __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);              /*  ǿ�ƽ������״̬            */
        __sigCtlCreate(ptcb, psigctx, &siginfo, iSchedRet, &sigsetOld); /*  �����ź������Ļ���          */
        
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: _sigTimeoutRecalc
** ��������: ��Ҫ���µȴ��¼�ʱ, ���¼���ȴ�ʱ��.
** �䡡��  : ulOrgKernelTime        ��ʼ�ȴ�ʱ��ϵͳʱ��
**           ulOrgTimeout           ��ʼ�ȴ�ʱ�ĳ�ʱѡ��
** �䡡��  : �µĵȴ�ʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _sigTimeoutRecalc (ULONG  ulOrgKernelTime, ULONG  ulOrgTimeout)
{
    REGISTER ULONG      ulTimeRun;
             ULONG      ulKernelTime;
    
    if (ulOrgTimeout == LW_OPTION_WAIT_INFINITE) {                      /*  ���޵ȴ�                    */
        return  (ulOrgTimeout);
    }
    
    __KERNEL_TIME_GET(ulKernelTime, ULONG);                             /*  ��õ�ǰϵͳʱ��            */
    ulTimeRun = (ulKernelTime >= ulOrgKernelTime) ?
                (ulKernelTime -  ulOrgKernelTime) :
                (ulKernelTime + (__ARCH_ULONG_MAX - ulOrgKernelTime) + 1);
    
    if (ulTimeRun >= ulOrgTimeout) {                                    /*  �Ѿ������˳�ʱ              */
        return  (0);
    }
    
    return  (ulOrgTimeout - ulTimeRun);
}
/*********************************************************************************************************
** ��������: _doSignal
** ��������: ��ָ���źž����Ƕ��ָ�����̣߳��������Լ������ź�. (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           psigpend               �źŵȴ�������Ϣ
** �䡡��  : ���ͽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doSignal (PLW_CLASS_TCB  ptcb, PLW_CLASS_SIGPEND   psigpend)
{
    REGISTER struct siginfo      *psiginfo = &psigpend->SIGPEND_siginfo;
    REGISTER INT                  iSigNo   = psiginfo->si_signo;

    REGISTER struct sigaction    *psigaction;
    REGISTER sigset_t             sigsetSigMaskBit  = __sigmask(iSigNo);/*  �ź�����                    */
    REGISTER INT                  iSigIndex = __sigindex(iSigNo);       /*  TCB_sigaction �±�          */
    
             PLW_CLASS_SIGCONTEXT psigctx;
             sigset_t             sigsetOld;                            /*  �ź�ִ�������Ҫ�ظ�������  */
             INT                  iSchedRet;                            /*  ��Ҫ���µȴ��¼�            */
             INT                  iSaType;
             
    if (psigpend->SIGPEND_iNotify == SIGEV_NONE) {                      /*  �������ź�                  */
        return  (SEND_IGN);
    }
    
    psigctx = _signalGetCtx(ptcb);
    if (psigctx->SIGCTX_sigwait) {                                      /*  Ŀ���߳��ڵȴ��ź�          */
        if (psigctx->SIGCTX_sigwait->SIGWT_sigset & __sigmask(iSigNo)) {/*  ���ڹ��ĵ��ź�              */
            psigctx->SIGCTX_sigwait->SIGWT_siginfo = psigpend->SIGPEND_siginfo;
            __sigMakeReady(ptcb, iSigNo, &iSchedRet, LW_SIGNAL_EINTR);  /*  ��������                    */
            psigctx->SIGCTX_sigwait = LW_NULL;                          /*  ɾ���ȴ���Ϣ                */
            return  (SEND_INFO);
        }
    }
    
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];                 /*  ���Ŀ���̵߳�����źſ��ƿ�*/
    if ((psigaction->sa_handler == SIG_ERR) ||
        (psigaction->sa_handler == SIG_IGN)) {                          /*  �������Ч��              */
        return  (SEND_IGN);
    }
    
    if (sigsetSigMaskBit & psigctx->SIGCTX_sigsetMask) {                /*  ��������                    */
        if (psiginfo->si_code == SI_KILL) {                             /*  kill �������ź�, �����Ŷ�   */
            psigctx->SIGCTX_sigsetKill    |= sigsetSigMaskBit;          /*  �� kill ���źű�������      */
            psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;          /*  iSigNo �������εȴ�����     */
        
        } else if (psigpend->SIGPEND_ringSigQ.RING_plistNext) {         /*  ���� kill �������ź�, ���  */
            psigpend->SIGPEND_uiTimes++;                                /*  �ڶ�����, ����Ҫ���л����ź�*/
                                                                        /*  �Ѿ������ڶ�����            */
        } else {
            if (psigpend->SIGPEND_iNotify == SIGEV_SIGNAL) {            /*  ��Ҫ�Ŷ��ź�                */
                PLW_CLASS_SIGPEND   psigpendNew = _sigPendAlloc();      /*  �ӻ������л�ȡһ�����е�    */
                LW_LIST_RING_HEADER *ppringHeader = 
                                    &psigctx->SIGCTX_pringSigQ[iSigIndex];
                
                if (psigpendNew == LW_NULL) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, 
                    "no node can allocate from free sigqueue.\r\n");
                    _ErrorHandle(ERROR_SIGNAL_SIGQUEUE_NODES_NULL);
                    return  (SEND_ERROR);
                }
                
                *psigpendNew = *psigpend;                               /*  ������Ϣ                    */
                _List_Ring_Add_Last(&psigpendNew->SIGPEND_ringSigQ, 
                                    ppringHeader);                      /*  �����������                */
                psigpendNew->SIGPEND_psigctx   = psigctx;
                psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;      /*  iSigNo �������εȴ�����     */
            }
        }
        return  (SEND_BLOCK);                                           /*  �� mask �Ķ���ִ��          */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  �ź�ִ�������Ҫ��������Ϊ  */
                                                                        /*  �������                    */
    psigctx->SIGCTX_sigsetMask |= psigaction->sa_mask;
    if ((psigaction->sa_flags & SA_NOMASK) == 0) {                      /*  ��ֹ��ͬ�ź�Ƕ��            */
        psigctx->SIGCTX_sigsetMask |= sigsetSigMaskBit;
    }
    if (psigaction->sa_flags & SA_RESTART) {
        iSaType = LW_SIGNAL_RESTART;                                    /*  ��Ҫ��������                */
    } else {
        iSaType = LW_SIGNAL_EINTR;                                      /*  ���� EINTR                  */
    }
    
    __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);                  /*  ǿ�ƽ������״̬            */
    __sigCtlCreate(ptcb, psigctx, psiginfo, iSchedRet, &sigsetOld);     /*  �����ź������Ļ���          */
    
    return  (SEND_OK);
}
/*********************************************************************************************************
** ��������: _doKill
** ��������: kill ����������������������һ�� kill ���ź�, Ȼ�󽫻���� doSignal ��������ź�.
             (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�
** �䡡��  : �����źŽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doKill (PLW_CLASS_TCB  ptcb, INT  iSigNo)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;                                         /*  ������ kill ����, ���Ծ���  */
                                                                        /*  �����������, ���ﲻ�ó�ʼ��*/
                                                                        /*  ��ص�����                  */
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_KILL;                                       /*  �����Ŷ�                    */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_int   = EXIT_FAILURE;                                  /*  Ĭ���źŲ���                */

    sigpend.SIGPEND_iNotify = SIGEV_SIGNAL;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_KILL, 
                      ptcb->TCB_ulId, iSigNo, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  �����ź�                    */
}
/*********************************************************************************************************
** ��������: _doSigQueue
** ��������: sigqueue ����������������������һ�� queue ���ź�, Ȼ�󽫻���� doSignal ��������ź�.
             (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�
**           sigvalue               �ź� value
** �䡡��  : �����źŽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doSigQueue (PLW_CLASS_TCB  ptcb, INT  iSigNo, const union sigval sigvalue)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;

    _sigPendInit(&sigpend);                                             /*  ��ʼ���ɲ����Ŷ��ź�        */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_QUEUE;                                       /*  �Ŷ��ź�                   */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_value = sigvalue;
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGQUEUE, 
                      ptcb->TCB_ulId, iSigNo, sigvalue.sival_ptr, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  �����ź�                    */
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
