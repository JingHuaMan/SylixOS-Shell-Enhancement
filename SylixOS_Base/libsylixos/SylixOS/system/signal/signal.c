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
** ��   ��   ��: signal.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 06 �� 03 ��
**
** ��        ��: ϵͳ�źŴ������⡣ 

** BUG
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.08.11  API_kill ����Ծ�����͵��ж�, ���� shell ��һ����������·����źŽ� idle ɾ����, ������Ϊ
            û�м��������.
2009.01.13  �޸����ע��.
2009.05.24  ������ TCB_bIsInDeleteProc Ϊ TRUE ���̷߳����ź�.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2012.08.24  �����źź���֧�ֽ��̺�, ��Ӧ�źŽ����͵��Է����߳�.
2012.12.12  sigprocmask �����ź�����ʱ, ��Щ�ź��ǲ������ε�.
2013.01.15  sigaction ��װ���ź�������, Ҫ���������ε��ź�.
2014.05.21  �� killTrap ��Ϊ sigTrap ���Է��Ͳ���.
2014.10.31  ������� POSIX �涨���źź���.
2015.11.16  trap �ź�ʹ�� kill ����.
2016.08.11  �������߳������źŴ�����, ��Ҫͬ�������н����ڵ��߳�.
2017.06.06  ���� sigaltstack() ����.
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
  �ڲ���������
*********************************************************************************************************/
extern PLW_CLASS_SIGCONTEXT  _signalGetCtx(PLW_CLASS_TCB  ptcb);
extern VOID                  _sigGetAction(PLW_CLASS_TCB  ptcb, INT  iSigNo,
                                           struct sigaction *psigaction);
extern VOID                  _sigPendFree(PLW_CLASS_SIGPEND  psigpendFree);
extern BOOL                  _sigPendRun(PLW_CLASS_TCB  ptcb);
extern INT                   _sigPendGet(PLW_CLASS_SIGCONTEXT  psigctx, 
                                         const sigset_t  *psigset, struct siginfo *psiginfo);
/*********************************************************************************************************
  �ڲ����ͺ�������
*********************************************************************************************************/
extern LW_SEND_VAL           _doKill(PLW_CLASS_TCB  ptcb, INT  iSigNo);
extern LW_SEND_VAL           _doSigQueue(PLW_CLASS_TCB  ptcb, INT  iSigNo, const union sigval  sigvalue);
/*********************************************************************************************************
  �ڲ�������غ�������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: sigemptyset
** ��������: ��ʼ��һ���յ��źż�
** �䡡��  : 
**           psigset                 �źż�
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigemptyset (sigset_t    *psigset)
{
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigfillset
** ��������: ��ʼ��һ�������źż�
** �䡡��  : psigset                 �źż�
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigfillset (sigset_t	*psigset)
{
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset = ~0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigaddset
** ��������: ��һ���źż����һ���ź�
** �䡡��  : psigset                 �źż�
**           iSigNo                  �ź�
** �䡡��  : ERROR_NONE , EINVAL
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigaddset (sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset |= __sigmask(iSigNo);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigdelset
** ��������: ��һ���źż���ɾ��һ���ź�
** �䡡��  : psigset                 �źż�
**           iSigNo                  �ź�
** �䡡��  : ERROR_NONE , EINVAL
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigdelset (sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset &= ~__sigmask(iSigNo);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigismember
** ��������: ���һ���ź��Ƿ�����һ���źż�
** �䡡��  : 
**           psigset                 �źż�
**           iSigNo                  �ź�
** �䡡��  : 0 or 1 or -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigismember (const sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (*psigset & __sigmask(iSigNo)) {
        return  (1);
    
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: __sigaction
** ��������: ���������߳�����һ��ָ���źŵķ�������
** �䡡��  : ptcb          �߳̿��ƿ�
**           iSigIndex     TCB_sigaction �±�
**           psigactionNew �µĴ���ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __sigaction (PLW_CLASS_TCB  ptcb, INT  iSigIndex, const struct sigaction  *psigactionNew)
{
    struct sigaction       *psigaction;
    PLW_CLASS_SIGCONTEXT    psigctx;
    
    psigctx    = _signalGetCtx(ptcb);
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];
    
    *psigaction = *psigactionNew;
    psigaction->sa_mask &= ~__SIGNO_UNMASK;                             /*  ��Щ�źŲ�������            */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: sigaction
** ��������: ����һ��ָ���źŵķ�������, ͬʱ�ɻ�ȡԭʼ��������. 
**           (������ struct sigaction ����, ��������ֱ��ʹ�� sigaction ������)
** �䡡��  : iSigNo        �ź�
**           psigactionNew �µĴ���ṹ
**           psigactionOld ����Ĵ���ṹ
** �䡡��  : ERROR_NONE , EINVAL
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : SIGKILL ���� gdbserver ���в������.

                                           API ����
*********************************************************************************************************/
LW_API  
INT   sigaction (INT                      iSigNo, 
                 const struct sigaction  *psigactionNew,
                 struct sigaction        *psigactionOld)
{
             struct sigaction      *psigaction;
             PLW_CLASS_SIGCONTEXT   psigctx;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_SIGPEND      psigpend;
    REGISTER INT                    iSigIndex = __sigindex(iSigNo);     /*  TCB_sigaction �±�          */
    
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (__SIGNO_UNCATCH & __sigmask(iSigNo)) {                          /*  ���̲��ܲ���ͺ���          */
        if (__LW_VP_GET_CUR_PROC()) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx    = _signalGetCtx(ptcbCur);
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];
    
    if (psigactionOld) {
        *psigactionOld = *psigaction;                                   /*  ����������Ϣ                */
    }
    
    if (psigactionNew == LW_NULL) {
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();
    *psigaction = *psigactionNew;                                       /*  �����µĴ�����ƿ�          */
    psigaction->sa_mask &= ~__SIGNO_UNMASK;                             /*  ��Щ�źŲ�������            */
    __KERNEL_EXIT();

#if LW_CFG_MODULELOADER_EN > 0                                          /*  ���������߳������µĴ�����*/
    vprocThreadSigaction(vprocGetCur(), __sigaction, iSigIndex, psigactionNew);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (psigaction->sa_handler == SIG_IGN) {                            /*  ����Ϊ���Ը��ź�            */
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        psigctx->SIGCTX_sigsetPending &= ~__sigmask(iSigNo);            /*  û�еȴ� unmask ��ִ�е��ź�*/
        psigctx->SIGCTX_sigsetKill    &= ~__sigmask(iSigNo);            /*  û��������״̬ kill ����ź�*/
        
        {                                                               /*  ɾ�������е�����źŽڵ�    */
                     PLW_LIST_RING  pringHead = psigctx->SIGCTX_pringSigQ[iSigIndex];
            REGISTER PLW_LIST_RING  pringSigP = pringHead;
            
            if (pringHead) {                                            /*  ���Ѷ����д��ڽڵ�          */
                do {
                    psigpend  = _LIST_ENTRY(pringSigP, 
                                            LW_CLASS_SIGPEND, 
                                            SIGPEND_ringSigQ);          /*  ��� sigpend ���ƿ��ַ     */
                    
                    pringSigP = _list_ring_get_next(pringSigP);         /*  ��һ���ڵ�                  */
                    
                    if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
                        (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {
                        _sigPendFree(psigpend);                         /*  ��Ҫ�������ж���            */
                    }
                } while (pringSigP != pringHead);
            }
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bsd_signal
** ��������: ����һ���źŵĴ�����
** �䡡��  : iSigNo        �ź�
**           pfuncHandler  ������
** �䡡��  : ����Ĵ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
sighandler_t bsd_signal (INT   iSigNo, PSIGNAL_HANDLE  pfuncHandler)
{
    return  (signal(iSigNo, pfuncHandler));
}
/*********************************************************************************************************
** ��������: signal
** ��������: ����һ���źŵĴ�����
** �䡡��  : iSigNo        �ź�
**           pfuncHandler  ������
** �䡡��  : ����Ĵ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PSIGNAL_HANDLE signal (INT  iSigNo, PSIGNAL_HANDLE  pfuncHandler)
{
    INT               iError;
    struct sigaction  sigactionNew;
    struct sigaction  sigactionOld;
    
    sigactionNew.sa_handler = pfuncHandler;
    sigactionNew.sa_flags   = 0;                                        /*  ������ͬ�����ź�Ƕ��        */
    
    sigemptyset(&sigactionNew.sa_mask);
    
    iError = sigaction(iSigNo, &sigactionNew, &sigactionOld);
    if (iError) {
        return  (SIG_ERR);
    
    } else {
        return  (sigactionOld.sa_handler);
    }
}
/*********************************************************************************************************
** ��������: sigvec
** ��������: ��װ�ź�������� (BSD ����)
** �䡡��  : iBlock                   �µ������ź�����
** �䡡��  : ���������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigvec (INT  iSigNo, const struct sigvec *pvec, struct sigvec *pvecOld)
{
    struct sigaction sigactionNew;
    struct sigaction sigactionOld;
    
    INT              iSave  = 0;
    INT              iError = PX_ERROR;
    
    if (pvec) {                                                         /*  �����µ��ź�����            */
        sigactionNew.sa_handler = pvec->sv_handler;
        sigactionNew.sa_mask    = pvec->sv_mask;
        sigactionNew.sa_flags   = pvec->sv_flags;
        if (pvecOld) {
            iError = sigaction(iSigNo, &sigactionNew, &sigactionOld);
            iSave  = 1;
        
        } else {
            iError = sigaction(iSigNo, &sigactionNew, LW_NULL);
        }
    
    } else if (pvecOld) {
        iError = sigaction(iSigNo, LW_NULL, &sigactionOld);             /*  ��ȡ                        */
        iSave  = 1;
    }
    
    if (iSave) {
        pvecOld->sv_handler = sigactionOld.sa_handler;
        pvecOld->sv_mask    = sigactionOld.sa_mask;
        pvecOld->sv_flags   = sigactionOld.sa_flags;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: sigpending
** ��������: ��õ�ǰ����ʱ����δ������ź� (�д�����. ���Ǳ�������)
** �䡡��  : psigset                 �źż�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigpending (sigset_t  *psigset)
{
    PLW_CLASS_SIGCONTEXT  psigctx;
    PLW_CLASS_TCB         ptcbCur;
    
    if (psigset) {
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        psigctx  = _signalGetCtx(ptcbCur);
        *psigset = psigctx->SIGCTX_sigsetPending;
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: sigprocmask
** ��������: ���Ի�ı䵱ǰ�̵߳��ź�����
** �䡡��  : iCmd                    ����
**           psigset                 ���źż�
**           psigsetOld              �����źż�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigprocmask (INT              iCmd, 
                  const sigset_t  *psigset, 
                        sigset_t  *psigsetOld)
{
    PLW_CLASS_TCB         ptcbCur;
    PLW_CLASS_SIGCONTEXT  psigctx;
    sigset_t              sigsetBlock;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    if (psigsetOld) {                                                   /*  ������ϵ�                  */
        *psigsetOld = psigctx->SIGCTX_sigsetMask;
    }
    
    if (!psigset) {                                                     /*  �µ��Ƿ���Ч                */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    
    switch (iCmd) {
    
    case SIG_BLOCK:                                                     /*  �������                    */
        sigsetBlock  = *psigset;
        sigsetBlock &= ~__SIGNO_UNMASK;                                 /*  ��Щ�ź��ǲ������ε�        */
        psigctx->SIGCTX_sigsetMask |= sigsetBlock;
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
        
    case SIG_UNBLOCK:                                                   /*  ɾ������                    */
        psigctx->SIGCTX_sigsetMask &= ~(*psigset);
        break;
        
    case SIG_SETMASK:                                                   /*  ��������                    */
        sigsetBlock  = *psigset;
        sigsetBlock &= ~__SIGNO_UNMASK;                                 /*  ��Щ�ź��ǲ������ε�        */
        psigctx->SIGCTX_sigsetMask  = sigsetBlock;
        break;
    
    default:                                                            /*  ����                        */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "command invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _sigPendRun(ptcbCur);                                               /*  ��������ǰ�������ź���Ҫ����*/
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigmask
** ��������: ͨ���źŵ�ֵ��ȡһ���ź�����
** �䡡��  : iSigNo                  �ź�
** �䡡��  : �ź�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigmask (INT  iSigNo)
{
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    return  ((INT)__sigmask(iSigNo));
}
/*********************************************************************************************************
** ��������: siggetmask
** ��������: ��õ�ǰ�߳��ź�����
** �䡡��  : NONE
** �䡡��  : �ź�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  siggetmask (VOID)
{
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigprocmask(SIG_BLOCK, LW_NULL, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** ��������: sigsetmask
** ��������: ���õ�ǰ�߳��µ����� (BSD ����)
** �䡡��  : iMask                   �µ�����
** �䡡��  : ���������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigsetmask (INT  iMask)
{
    sigset_t    sigsetNew;
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigsetNew = (sigset_t)iMask;                                        /*  ���� BSD �ӿڽ� 32 ���ź�   */
    sigprocmask(SIG_SETMASK, &sigsetNew, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** ��������: sigsetblock
** ��������: ���µ���Ҫ�������ź���ӵ���ǰ�߳� (BSD ����)
** �䡡��  : iBlock                   �µ������ź�����
** �䡡��  : ���������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigblock (INT  iMask)
{
    sigset_t    sigsetNew;
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigsetNew = (sigset_t)iMask;                                        /*  ���� BSD �ӿڽ� 32 ���ź�   */
    sigprocmask(SIG_BLOCK, &sigsetNew, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** ��������: sighold
** ��������: ���µ���Ҫ�������ź���ӵ���ǰ�߳�
** �䡡��  : iSigNo                   ��Ҫ�������ź�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sighold (INT  iSigNo)
{
    sigset_t    sigset = 0;

    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    sigset = __sigmask(iSigNo);
    if (__SIGNO_UNMASK & sigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (sigprocmask(SIG_BLOCK, &sigset, LW_NULL));
}
/*********************************************************************************************************
** ��������: sigignore
** ��������: ��ָ�����ź�����Ϊ SIG_IGN
** �䡡��  : iSigNo                   ��Ҫ SIG_IGN ���ź�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  sigignore (INT  iSigNo)
{
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SIGNO_UNMASK & __sigmask(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (signal(iSigNo, SIG_IGN) == SIG_ERR) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigrelse
** ��������: ��ָ�����źŴ�������ɾ��
** �䡡��  : iSigNo                   ��Ҫɾ��������ź�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  sigrelse (INT  iSigNo)
{
    sigset_t    sigset;
    
    if (sigprocmask(SIG_SETMASK, LW_NULL, &sigset) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (sigdelset(&sigset, iSigNo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (sigprocmask(SIG_SETMASK, &sigset, LW_NULL));
}
/*********************************************************************************************************
** ��������: sigpause
** ��������: �ڵ�ǰ���������ָ�����źŵȴ��źŵĵ���, Ȼ�󷵻���ǰ���ź�����. 
**           �� API �Ա� sigsuspend ���.
** �䡡��  : iSigMask                 ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigpause (INT  iSigMask)
{
    sigset_t    sigset;
    
    sigset = ~(sigset_t)iSigMask;
    
    return  (sigsuspend(&sigset));
}
/*********************************************************************************************************
** ��������: sigset
** ��������: �˺����ݲ�֧��
** �䡡��  : iSigNo                   ��Ҫɾ��������ź�
**           disp
** �䡡��  : disp
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
sighandler_t  sigset (INT  iSigNo, sighandler_t  disp)
{
    _ErrorHandle(ENOSYS);
    return  ((sighandler_t)PX_ERROR);
}
/*********************************************************************************************************
** ��������: siginterrupt
** ��������: �ı��ź� SA_RESTART ѡ��
** �䡡��  : iSigNo                   �ź�
**           iFlag                    �Ƿ�ʹ�� SA_RESTART ѡ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  siginterrupt (INT  iSigNo, INT  iFlag)
{
    struct sigaction    sigact;
    
    if (sigaction(iSigNo, LW_NULL, &sigact)) {
        return  (PX_ERROR);
    }
    
    if (iFlag) {
        sigact.sa_flags &= ~SA_RESTART;
    } else {
        sigact.sa_flags |= SA_RESTART;
    }
    
    return  (sigaction(iSigNo, &sigact, LW_NULL));
}
/*********************************************************************************************************
** ��������: sigstack
** ��������: �����ź������ĵĶ�ջ
** �䡡��  : ss                       �µĶ�ջ��Ϣ
**           oss                      �ϵĶ�ջ��Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  sigstack (struct sigstack *ss, struct sigstack *oss)
{
    stack_t     stackNew, stackOld;
    
    if (ss) {
        stackNew.ss_sp = ss->ss_sp;
        if (ss->ss_sp) {
            stackNew.ss_flags = 0;
            stackNew.ss_size  = SIGSTKSZ;                               /*  Ĭ�ϴ�С                    */
        
        } else {
            stackNew.ss_flags = SS_DISABLE;
            stackNew.ss_size  = 0;
        }
        if (sigaltstack(&stackNew, &stackOld)) {
            return  (PX_ERROR);
        }
    
    } else if (oss) {
        sigaltstack(LW_NULL, &stackOld);
    }
        
    if (oss) {
        oss->ss_sp = stackOld.ss_sp;
        if (stackOld.ss_flags == SS_ONSTACK) {
            oss->ss_onstack = 1;
        } else {
            oss->ss_onstack = 0;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigaltstack
** ��������: �����ź������ĵĶ�ջ
** �䡡��  : ss                       �µĶ�ջ��Ϣ
**           oss                      �ϵĶ�ջ��Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  sigaltstack (const stack_t *ss, stack_t *oss)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    stack_t                *pstack;
    PLW_STACK               pstkStackNow = (PLW_STACK)&pstkStackNow;

    if (ss) {
        if ((ss->ss_flags != 0) && 
            (ss->ss_flags != SS_DISABLE)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        if (ss->ss_flags == 0) {
            if (!ss->ss_sp || 
                !ALIGNED(ss->ss_sp, LW_CFG_HEAP_ALIGNMENT)) {           /*  ������������ϵ            */
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            if (ss->ss_size < MINSIGSTKSZ) {
                _ErrorHandle(ENOMEM);
                return  (PX_ERROR);
            }
        }
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx = _signalGetCtx(ptcbCur);
    pstack  = &psigctx->SIGCTX_stack;
    
    if (oss) {
        if (pstack->ss_flags == 0) {
            if ((pstkStackNow >= (PLW_STACK)pstack->ss_sp) &&
                (pstkStackNow <  (PLW_STACK)((size_t)pstack->ss_sp + pstack->ss_size))) {
                oss->ss_flags = SS_ONSTACK;
            } else {
                oss->ss_flags = 0;
            }
            oss->ss_sp   = pstack->ss_sp;
            oss->ss_size = pstack->ss_size;
        
        } else {
            oss->ss_sp    = LW_NULL;
            oss->ss_size  = 0;
            oss->ss_flags = SS_DISABLE;
        }
    }
    
    if (ss) {
        if ((pstkStackNow >= (PLW_STACK)pstack->ss_sp) &&
            (pstkStackNow <  (PLW_STACK)((size_t)pstack->ss_sp + pstack->ss_size))) {
            _ErrorHandle(EPERM);                                        /*  �����ź���������ʹ�ô˶�ջ  */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            return  (PX_ERROR);
        }
        if (ss->ss_flags == 0) {
            pstack->ss_sp    = ss->ss_sp;
            pstack->ss_size  = ROUND_DOWN(ss->ss_size, LW_CFG_HEAP_ALIGNMENT);
            pstack->ss_flags = 0;
            
        } else {
            pstack->ss_sp    = LW_NULL;
            pstack->ss_size  = 0;
            pstack->ss_flags = SS_DISABLE;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: kill
** ��������: ��ָ�����̷߳����ź�, ����ǽ���, �����͸������߳�.
** �䡡��  : ulId                    �߳� id ���� ���̺�
**           iSigNo                  �ź�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  kill (LW_OBJECT_HANDLE  ulId, INT  iSigNo)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL        sendval;
#endif
    
#if LW_CFG_MODULELOADER_EN > 0
    pid_t    pid;

    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  ���̺�                      */
        pid   = (pid_t)ulId;
        ulId  = vprocMainThread((pid_t)ulId);
    } else {
        pid = 0;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (iSigNo == 0) {                                                  /*  ����Ŀ���Ƿ����            */
        return  (ERROR_NONE);
    
    } else if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == API_ThreadIdSelf())) {
        _excJobAdd((VOIDFUNCPTR)kill, (PVOID)ulId, (PVOID)(LONG)iSigNo, 0, 0, 0, 0);
        return  (ERROR_NONE);
    }

#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        if (API_ThreadStop(ulId)) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

#if LW_CFG_MODULELOADER_EN > 0
    if ((iSigNo == SIGKILL) && (pid > 0)) {
        vprocKillPrepare(pid, ulId);                                    /*  ���� KILL Ԥ����            */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
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
        return  (PX_ERROR);
    }
    
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doKill(ptcb, iSigNo);                                    /*  �����ź�                    */
#else
    _doKill(ptcb, iSigNo);                                              /*  �����ź�                    */
#endif
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  ���ں�״̬�»��ѱ�ֹͣ�߳�  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, iSigNo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: raise
** ��������: ���Լ������ź�
** �䡡��  : iSigNo                  �ź�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  raise (INT  iSigNo)
{
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (kill(API_ThreadIdSelf(), iSigNo));
}
/*********************************************************************************************************
** ��������: __sigqueue
** ��������: ���Ͷ��������ź�, ����ǽ���, �����͸������߳�.
** �䡡��  : ulId                    �߳� id ���� ���̺�
**           iSigNo                  �ź�
**           psigvalue               �ź� value
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __sigqueue (LW_OBJECT_HANDLE  ulId, INT   iSigNo, PVOID  psigvalue)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    union    sigval             sigvalue;
    
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL        sendval;
#endif
             
    sigvalue.sival_ptr = psigvalue;

#if LW_CFG_MODULELOADER_EN > 0
    pid_t    pid;

    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  ���̺�                      */
        pid   = (pid_t)ulId;
        ulId  = vprocMainThread((pid_t)ulId);
    } else {
        pid = 0;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (iSigNo == 0) {                                                  /*  ����Ŀ���Ƿ����            */
        return  (ERROR_NONE);
    
    } else if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        if (API_ThreadStop(ulId)) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

#if LW_CFG_MODULELOADER_EN > 0
    if ((iSigNo == SIGKILL) && (pid > 0)) {
        vprocKillPrepare(pid, ulId);                                    /*  ���� KILL Ԥ����            */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
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
        return  (PX_ERROR);
    }
    
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doSigQueue(ptcb, iSigNo, sigvalue);
#else
    _doSigQueue(ptcb, iSigNo, sigvalue);
#endif

#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  �������� SMP ���ģʽ       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  ���ں�״̬�»��ѱ�ֹͣ�߳�  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, iSigNo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigqueue
** ��������: ���Ͷ��������ź�, ����ǽ���, �����͸������߳�.
** �䡡��  : ulId                    �߳� id ���� ���̺�
**           iSigNo                  �ź�
**           sigvalue                �ź� value
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigqueue (LW_OBJECT_HANDLE  ulId, INT   iSigNo, const union sigval  sigvalue)
{
    REGISTER UINT16  usIndex;

#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  ���̺�                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if ((iSigNo != SIGUSR1) && (iSigNo != SIGUSR2)) {
        if (sigvalue.sival_int == 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    if (LW_CPU_GET_CUR_NESTING() || (ulId == API_ThreadIdSelf())) {
        _excJobAdd((VOIDFUNCPTR)__sigqueue, (PVOID)ulId, (PVOID)(LONG)iSigNo, sigvalue.sival_ptr, 0, 0, 0);
        return  (ERROR_NONE);
    }
    
    return  (__sigqueue(ulId, iSigNo, sigvalue.sival_ptr));
}
/*********************************************************************************************************
** ��������: sigTrap
** ��������: ��ָ���������ź�, ͬʱֹͣ�Լ�. (���������쳣��������ִ��)
** �䡡��  : ulIdSig                  �߳� id (������Ϊ���̺�)
**           ulIdStop                 ��Ҫ�ȴ��������߳�
**           pvSigValue               �źŲ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sig_trap (LW_OBJECT_HANDLE  ulIdSig, LW_OBJECT_HANDLE  ulIdStop, PVOID  pvSigValue)
{
#if LW_CFG_SMP_EN > 0
    BOOL  bIsRunning;
    
    for (;;) {
        if (API_ThreadIsRunning(ulIdStop, &bIsRunning)) {
            return;
        }
        if (!bIsRunning) {
            break;
        }
        LW_SPINLOCK_DELAY();
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    kill(ulIdSig, SIGTRAP);
}
/*********************************************************************************************************
** ��������: sigTrap
** ��������: ��ָ���������ź�, ͬʱֹͣ�Լ�. (���������쳣��������ִ��)
** �䡡��  : ulId                     �߳� id (������Ϊ���̺�)
**           sigvalue                 �źŲ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigTrap (LW_OBJECT_HANDLE  ulId, const union sigval  sigvalue)
{
    REGISTER PLW_CLASS_TCB  ptcbCur;
    
    if (!LW_CPU_GET_CUR_NESTING()) {                                    /*  �������쳣��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "not in exception mode.\r\n");
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��ǰ������ƿ�              */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    _ThreadStop(ptcbCur);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ulId) {                                                         /*  ����Ŀ��                    */
        _excJobAdd(__sig_trap, (PVOID)ulId, (PVOID)ptcbCur->TCB_ulId, sigvalue.sival_ptr, 0, 0, 0);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigGetAction
** ��������: ��ȡָ������� Notify action
** �䡡��  : ulId          ���� ID
**           iSigNo        �ź�
**           psigaction    ��ȡ�� action
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  sigGetAction (LW_OBJECT_HANDLE  ulId, INT  iSigNo, struct sigaction *psigaction)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;

    if (!__issig(iSigNo) || !psigaction) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  ���̺�                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);

    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    _sigGetAction(ptcb, iSigNo, psigaction);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pause
** ��������: �ȴ�һ���źŵĵ���
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  pause (VOID)
{
             INTREG         iregInterLevel;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */

    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_PAUSE, 
                      ptcbCur->TCB_ulId, LW_NULL);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  �ȴ��ź�                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    _ErrorHandle(EINTR);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sigsuspend
** ��������: ʹ��ָ��������ȴ�һ����Ч�źŵĵ���, Ȼ�󷵻���ǰ���ź�����.
** �䡡��  : psigsetMask        ָ�����ź�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigsuspend (const sigset_t  *psigsetMask)
{
             INTREG         iregInterLevel;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER PLW_CLASS_PCB  ppcb;
             BOOL           bIsRun;
    
             PLW_CLASS_SIGCONTEXT   psigctx;
             sigset_t               sigsetOld;
             
    if (!psigsetMask) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGSUSPEND, 
                   ptcbCur->TCB_ulId, *psigsetMask, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  ��¼��ǰ������              */
    psigctx->SIGCTX_sigsetMask = *psigsetMask & (~__SIGNO_UNMASK);
    
    bIsRun = _sigPendRun(ptcbCur);
    if (bIsRun) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        sigprocmask(SIG_SETMASK, &sigsetOld, LW_NULL);                  /*  ����Ϊԭ�ȵ� mask           */
        
        _ErrorHandle(EINTR);
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  �ȴ��ź�                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    sigprocmask(SIG_SETMASK, &sigsetOld, NULL);
    
    _ErrorHandle(EINTR);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sigwait
** ��������: �ȴ� sigset ���źŵĵ������Դ��еķ�ʽ���źŶ�����ȡ���źŽ��д���, �źŽ����ٱ�ִ��.
** �䡡��  : psigset       ָ�����źż�
**           psiginfo      ��ȡ���ź���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigwait (const sigset_t  *psigset, INT  *piSig)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
    
    if (!psigset || !piSig) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  ��鵱ǰ�Ƿ��еȴ����ź�    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        if (piSig) {
            *piSig = iSigNo;
        }
        return  (ERROR_NONE);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  �ȴ��ź�                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  ����ȴ���Ϣ                */
    
    if (__KERNEL_EXIT()) {                                              /*  �Ƿ������źż���            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART Ҳ�˳�           */
        return  (PX_ERROR);
    }
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (piSig) {
        *piSig = sigwt.SIGWT_siginfo.si_signo;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sigwaitinfo
** ��������: �ȴ� sigset ���źŵĵ������Դ��еķ�ʽ���źŶ�����ȡ���źŽ��д���, �źŽ����ٱ�ִ��.
** �䡡��  : psigset       ָ�����źż�
**           psiginfo      ��ȡ���ź���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigwaitinfo (const sigset_t *psigset, struct  siginfo  *psiginfo)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
    
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  ��鵱ǰ�Ƿ��еȴ����ź�    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        if (psiginfo) {
            *psiginfo = siginfo;
        }
        return  (siginfo.si_signo);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  �ȴ��ź�                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  ����ȴ���Ϣ                */
    
    if (__KERNEL_EXIT()) {                                              /*  �Ƿ������źż���            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART Ҳ�˳�           */
        return  (PX_ERROR);
    }
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (psiginfo) {
        *psiginfo = sigwt.SIGWT_siginfo;
    }
    
    return  (sigwt.SIGWT_siginfo.si_signo);
}
/*********************************************************************************************************
** ��������: sigtimedwait
** ��������: �ȴ� sigset ���źŵĵ������Դ��еķ�ʽ���źŶ�����ȡ���źŽ��д���, �źŽ����ٱ�ִ��.
** �䡡��  : psigset        ָ�����źż�
**           psiginfo      ��ȡ���ź���Ϣ
**           ptv           ��ʱʱ�� (NULL ��ʾһֱ�ȴ�)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  sigtimedwait (const sigset_t *psigset, struct  siginfo  *psiginfo, const struct timespec *ptv)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
             
             ULONG                  ulTimeout;
    
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (ptv == LW_NULL) {                                               /*  ���õȴ�                    */
        ulTimeout = LW_OPTION_WAIT_INFINITE;

    } else {
        if ((ptv->tv_nsec < 0) || (ptv->tv_nsec >= 1000000000)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        ulTimeout = __timespecToTick(ptv);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  ��鵱ǰ�Ƿ��еȴ����ź�    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        if (psiginfo) {
            *psiginfo = siginfo;
        }
        return  (siginfo.si_signo);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  �����еȴ�                  */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(EAGAIN);
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SIGNAL;              /*  �ȴ��ź�                    */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */
    
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    
    if (ulTimeout != LW_OPTION_WAIT_INFINITE) {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
    } else {
        ptcbCur->TCB_ulDelay = 0ul;
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  ����ȴ���Ϣ                */
    
    if (__KERNEL_EXIT()) {                                              /*  �Ƿ������źż���            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART Ҳ�˳�           */
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EAGAIN);
        return  (PX_ERROR);
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (psiginfo) {
        *psiginfo = sigwt.SIGWT_siginfo;
    }
    
    return  (sigwt.SIGWT_siginfo.si_signo);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
