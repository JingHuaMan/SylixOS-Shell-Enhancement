/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pthread.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ���ݿ�.

** BUG:
2010.01.13  �����߳�ʹ��Ĭ������ʱ, ���ȼ�Ӧʹ�� POSIX Ĭ�����ȼ�.
2010.05.07  pthread_create() Ӧ�ڵ��� API_ThreadStart() ֮ǰ���߳� ID д�������ַ.
2010.10.04  ���� pthread_create() ���� attr Ϊ NULL ʱ���߳�������������.
            �����̲߳��л��������.
2010.10.06  ���� pthread_set_name_np() ����.
2011.06.24  pthread_create() ��û�� attr ʱ, ʹ�� POSIX Ĭ�϶�ջ��С.
2012.06.18  �� posix �̵߳����ȼ�����, ����Ҫת�����ȼ�Ϊ sylixos ��׼. 
            posix ���ȼ�����Խ��, ���ȼ�Խ��, sylixos �պ��෴.
2013.05.01  If successful, the pthread_*() function shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
2013.05.03  pthread_create() ������� attr ���� attr ָ���Ķ�ջ��СΪ 0, ��̳д����ߵĶ�ջ��С.
2013.05.07  ���� pthread_getname_np() ����.
2013.09.18  pthread_create() ����Զ�ջ�������Ĵ���.
2013.12.02  ���� pthread_yield().
2014.01.01  ���� pthread_safe_np() �� pthread_unsafe_np().
2014.07.04  ���� pthread_setaffinity_np �� pthread_getaffinity_np();
2016.04.12  ���� GJB7714 ��� API ֧��.
2017.03.10  ���� pthread_null_attr_method_np() ����, ��ָ���̴߳�������Ϊ NULL ʱ�ķ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_POSIX
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
#if LW_CFG_POSIXEX_EN > 0
#include "../include/px_pthread_np.h"
#endif
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  ����֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if (LW_CFG_GJB7714_EN > 0) && (LW_CFG_MODULELOADER_EN > 0)
#include "unistd.h"
#endif
/*********************************************************************************************************
** ��������: pthread_atfork
** ��������: �����߳��� fork() ʱ��Ҫִ�еĺ���.
** �䡡��  : prepare       prepare ����ָ��
**           parent        ������ִ�к���ָ��
**           child         �ӽ���ִ�к���ָ��
**           arg           �߳���ڲ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_atfork (void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    errno = ENOSYS;
    return  (ENOSYS);
}
/*********************************************************************************************************
** ��������: pthread_create
** ��������: ����һ�� posix �߳�.
** �䡡��  : pthread       �߳� id (����).
**           pattr         ��������
**           start_routine �߳����
**           arg           �߳���ڲ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_create (pthread_t              *pthread, 
                     const pthread_attr_t   *pattr, 
                     void                   *(*start_routine)(void *),
                     void                   *arg)
{
    LW_OBJECT_HANDLE        ulId;
    LW_CLASS_THREADATTR     lwattr;
    PLW_CLASS_TCB           ptcbCur;
    PCHAR                   pcName = "pthread";
    INT                     iErrNo;
    
#if LW_CFG_POSIXEX_EN > 0
    __PX_VPROC_CONTEXT     *pvpCtx = _posixVprocCtxGet();
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

    if (start_routine == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    lwattr = API_ThreadAttrGetDefault();                                /*  ���Ĭ���߳�����            */
    
    if (pattr) {
        pcName = pattr->PTHREADATTR_pcName;                             /*  ʹ�� attr ��Ϊ�����߳���    */
        
        if (pattr->PTHREADATTR_stStackGuard > pattr->PTHREADATTR_stStackByteSize) {
            lwattr.THREADATTR_stGuardSize = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
        } else {
            lwattr.THREADATTR_stGuardSize = pattr->PTHREADATTR_stStackGuard;
        }
        
        if (pattr->PTHREADATTR_stStackByteSize == 0) {                  /*  �̳д�����                  */
            lwattr.THREADATTR_stStackByteSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
        } else {
            lwattr.THREADATTR_pstkLowAddr     = (PLW_STACK)pattr->PTHREADATTR_pvStackAddr;
            lwattr.THREADATTR_stStackByteSize = pattr->PTHREADATTR_stStackByteSize;
        }
        
        if (pattr->PTHREADATTR_iInherit == PTHREAD_INHERIT_SCHED) {     /*  �Ƿ�̳����ȼ�              */
            lwattr.THREADATTR_ucPriority =  ptcbCur->TCB_ucPriority;
        } else {
            lwattr.THREADATTR_ucPriority = 
                (UINT8)PX_PRIORITY_CONVERT(pattr->PTHREADATTR_schedparam.sched_priority);
        }
                                                                        /*  ������������ջʹ�����  */
        lwattr.THREADATTR_ulOption = pattr->PTHREADATTR_ulOption | LW_OPTION_THREAD_STK_CHK;
    
    } else
#if LW_CFG_POSIXEX_EN > 0
           if (pvpCtx->PVPCTX_iThreadDefMethod == PTHREAD_NULL_ATTR_METHOD_USE_INHERIT)
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
    {                                                                   /*  ��ջ��С�����ȼ�ȫ���̳�    */
        lwattr.THREADATTR_stStackByteSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
        lwattr.THREADATTR_ucPriority      = ptcbCur->TCB_ucPriority;
    }
    
    lwattr.THREADATTR_ulOption |= LW_OPTION_THREAD_POSIX;               /*  POSIX �߳�                  */
    lwattr.THREADATTR_pvArg     = arg;                                  /*  ��¼����                    */
    
    /*
     *  ��ʼ���߳�.
     */
    ulId = API_ThreadInit(pcName, (PTHREAD_START_ROUTINE)start_routine, &lwattr, LW_NULL);
    if (ulId == 0) {
        iErrNo = errno;
        switch (iErrNo) {
        
        case ERROR_THREAD_FULL:
            errno = EAGAIN;
            return  (EAGAIN);
            
        case ERROR_KERNEL_LOW_MEMORY:
            errno = ENOMEM;
            return  (ENOMEM);
        
        default:
            return  (iErrNo);
        }
    }
    
    if (pattr) {
        UINT8       ucPolicy        = (UINT8)pattr->PTHREADATTR_iSchedPolicy;
        UINT8       ucActivatedMode = LW_OPTION_RESPOND_AUTO;           /*  Ĭ����Ӧ��ʽ                */
        
        if (pattr->PTHREADATTR_iInherit == PTHREAD_INHERIT_SCHED) {     /*  �̳е��Ȳ���                */
            API_ThreadGetSchedParam(ptcbCur->TCB_ulId, &ucPolicy, &ucActivatedMode);
        }
        
        API_ThreadSetSchedParam(ulId, ucPolicy, ucActivatedMode);       /*  ���õ��Ȳ���                */
    }
    
    if (pthread) {
        *pthread = ulId;                                                /*  �����߳̾��                */
    }
    
    if (!(lwattr.THREADATTR_ulOption & LW_OPTION_THREAD_INIT)) {
        API_ThreadStart(ulId);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_start_np
** ��������: ����һ���Ѿ�����ʼ���� posix �߳�.
** �䡡��  : thread       �߳� id.
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API
int  pthread_start_np (pthread_t  thread)
{
    ULONG   ulError;

    PX_ID_VERIFY(thread, pthread_t);

    ulError = API_ThreadStart(thread);
    switch (ulError) {

    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);

    default:
        return  ((int)ulError);
    }
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: pthread_cancel
** ��������: cancel һ�� posix �߳�.
** �䡡��  : thread       �߳� id.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_cancel (pthread_t  thread)
{
    ULONG   ulError;

    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadCancel(&thread);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** ��������: pthread_testcancel
** ��������: testcancel ��ǰ posix �߳�.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_testcancel (void)
{
    API_ThreadTestCancel();
}
/*********************************************************************************************************
** ��������: pthread_join
** ��������: join һ�� posix �߳�.
** �䡡��  : thread       �߳� id.
**           ppstatus     ��ȡ�̷߳���ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_join (pthread_t  thread, void **ppstatus)
{
    ULONG   ulError;
    
    ulError = API_ThreadJoin(thread, ppstatus);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    case ERROR_THREAD_JOIN_SELF:
        errno = EDEADLK;
        return  (EDEADLK);
        
    case ERROR_THREAD_DETACHED:
        errno = EINVAL;
        return  (EINVAL);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** ��������: pthread_detach
** ��������: detach һ�� posix �߳�.
** �䡡��  : thread       �߳� id.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_detach (pthread_t  thread)
{
    ULONG   ulError;

    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadDetach(thread);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    case ERROR_THREAD_DETACHED:
        errno = EINVAL;
        return  (EINVAL);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** ��������: pthread_equal
** ��������: �ж����� posix �߳��Ƿ���ͬ.
** �䡡��  : thread1       �߳� id.
**           thread2       �߳� id.
** �䡡��  : true or false
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_equal (pthread_t  thread1, pthread_t  thread2)
{
    PX_ID_VERIFY(thread1, pthread_t);
    PX_ID_VERIFY(thread2, pthread_t);

    return  (thread1 == thread2);
}
/*********************************************************************************************************
** ��������: pthread_exit
** ��������: ɾ����ǰ posix �߳�.
** �䡡��  : status        exit ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_exit (void *status)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();

    API_ThreadDelete(&ulId, status);
}
/*********************************************************************************************************
** ��������: pthread_self
** ��������: ��õ�ǰ posix �߳̾��.
** �䡡��  : NONE
** �䡡��  : ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
pthread_t  pthread_self (void)
{
    return  (API_ThreadIdSelf());
}
/*********************************************************************************************************
** ��������: pthread_yield
** ��������: ����ǰ������뵽ͬ���ȼ���������������, �����ó�һ�� CPU ����.
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_yield (void)
{
    API_ThreadYield(API_ThreadIdSelf());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_kill
** ��������: ��ָ�� posix �̷߳���һ���ź�.
** �䡡��  : thread        �߳̾��
**           signo         �ź�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

LW_API 
int  pthread_kill (pthread_t  thread, int signo)
{
    int  error, err_no;

    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread < LW_CFG_MAX_THREADS) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    error = kill(thread, signo);
    if (error < ERROR_NONE) {
        err_no = errno;
        switch (err_no) {
    
        case ERROR_KERNEL_HANDLE_NULL:
        case ERROR_THREAD_NULL:
            errno = ESRCH;
            return  (ESRCH);
            
        default:
            return  (err_no);
        }
    
    } else {
        return  (error);
    }
}
/*********************************************************************************************************
** ��������: pthread_sigmask
** ��������: �޸� posix �߳��ź�����.
** �䡡��  : how           ����
**           newmask       �µ��ź�����
**           oldmask       �ɵ��ź�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_sigmask (int  how, const sigset_t  *newmask, sigset_t *oldmask)
{
    int  error, err_no;
    
    error = sigprocmask(how, newmask, oldmask);
    if (error < ERROR_NONE) {
        err_no = errno;
        switch (err_no) {
    
        case ERROR_KERNEL_HANDLE_NULL:
        case ERROR_THREAD_NULL:
            errno = ESRCH;
            return  (ESRCH);
            
        default:
            return  (err_no);
        }
    
    } else {
        return  (error);
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: pthread_cleanup_pop
** ��������: ��һ��ѹջ�������в��ͷ�
** �䡡��  : iNeedRun          �Ƿ�ִ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_cleanup_pop (int  iNeedRun)
{
    API_ThreadCleanupPop((BOOL)iNeedRun);
}
/*********************************************************************************************************
** ��������: pthread_cleanup_push
** ��������: ��һ���������ѹ�뺯����ջ
** �䡡��  : pfunc         ����ָ��
**           arg           ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_cleanup_push (void (*pfunc)(void *), void *arg)
{
    API_ThreadCleanupPush(pfunc, arg);
}
/*********************************************************************************************************
** ��������: pthread_getschedparam
** ��������: ��õ���������
** �䡡��  : thread        �߳̾��
**           piPolicy      ���Ȳ���(����)
**           pschedparam   ����������(����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getschedparam (pthread_t            thread, 
                            int                 *piPolicy, 
                            struct sched_param  *pschedparam)
{
    UINT8       ucPriority;
    UINT8       ucPolicy;
    
    PX_ID_VERIFY(thread, pthread_t);
    
    if (pschedparam) {
        if (API_ThreadGetPriority(thread, &ucPriority)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        pschedparam->sched_priority = PX_PRIORITY_CONVERT(ucPriority);
    }
    
    if (piPolicy) {
        if (API_ThreadGetSchedParam(thread, &ucPolicy, LW_NULL)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        *piPolicy = (int)ucPolicy;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getschedparam
** ��������: ���õ���������
** �䡡��  : thread        �߳̾��
**           iPolicy       ���Ȳ���
**           pschedparam   ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setschedparam (pthread_t                  thread, 
                            int                        iPolicy, 
                            const struct sched_param  *pschedparam)
{
    UINT8       ucPriority;
    UINT8       ucActivatedMode;

    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
    PX_ID_VERIFY(thread, pid_t);
    
    if (API_ThreadGetSchedParam(thread,
                                LW_NULL,
                                &ucActivatedMode)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    API_ThreadSetSchedParam(thread, (UINT8)iPolicy, ucActivatedMode);
    
    if (API_ThreadSetPriority(thread, ucPriority)) {
        errno = ESRCH;
        return  (ESRCH);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: pthread_onec
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������
** �䡡��  : thread        �߳̾��
**           once          onec_t����
**           pfunc         ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_once (pthread_once_t  *once, void (*pfunc)(void))
{
    int  error;
    
    error = API_ThreadOnce(once, pfunc);
    if (error < ERROR_NONE) {
        return  (errno);
    } else {
        return  (error);
    }
}
/*********************************************************************************************************
** ��������: pthread_setschedprio
** ��������: �����߳����ȼ�
** �䡡��  : thread        �߳̾��
**           prio          ���ȼ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setschedprio (pthread_t  thread, int  prio)
{
    UINT8       ucPriority;

    if ((prio < __PX_PRIORITY_MIN) ||
        (prio > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(prio);
    
    if (API_ThreadSetPriority(thread, ucPriority)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getschedprio
** ��������: ����߳����ȼ�
** �䡡��  : thread        �߳̾��
**           prio          ���ȼ�(����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getschedprio (pthread_t  thread, int  *prio)
{
    UINT8       ucPriority;

    PX_ID_VERIFY(thread, pthread_t);

    if (prio) {
        if (API_ThreadGetPriority(thread, &ucPriority)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        *prio = PX_PRIORITY_CONVERT(ucPriority);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setcancelstate
** ��������: ����ȡ���߳��Ƿ�ʹ��
** �䡡��  : newstate      �µ�״̬
**           poldstate     ��ǰ��״̬(����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setcancelstate (int  newstate, int  *poldstate)
{
    ULONG   ulError = API_ThreadSetCancelState(newstate, poldstate);
    
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setcanceltype
** ��������: ���õ�ǰ�̱߳���ȡ��ʱ�Ķ���
** �䡡��  : newtype      �µĶ���
**           poldtype     ��ǰ�Ķ���(����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setcanceltype (int  newtype, int  *poldtype)
{
    ULONG   ulError = API_ThreadSetCancelType(newtype, poldtype);
    
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setconcurrency
** ��������: �����߳��µĲ��м���
** �䡡��  : newlevel      �²��м���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setconcurrency (int  newlevel)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getconcurrency
** ��������: ��õ�ǰ�̵߳Ĳ��м���
** �䡡��  : NONE
** �䡡��  : ��ǰ�̵߳Ĳ��м���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getconcurrency (void)
{
    return  (LW_CFG_MAX_THREADS);
}
/*********************************************************************************************************
** ��������: pthread_getcpuclockid
** ��������: ����߳� CPU ʱ�� clock id.
** �䡡��  : thread    �߳̾��
**           clock_id  ʱ�� id
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getcpuclockid (pthread_t thread, clockid_t *clock_id)
{
    if (!clock_id) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *clock_id = CLOCK_THREAD_CPUTIME_ID;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_null_attr_method_np
** ��������: ���ô����߳�ʱ���̵߳�����Ϊ NULL ʱ�Ĵ�����
** �䡡��  : method        �µĲ�������
**           old_method    ֮ǰ�Ĳ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API int  pthread_null_attr_method_np (int  method, int *old_method)
{
    __PX_VPROC_CONTEXT  *pvpCtx = _posixVprocCtxGet();

    if (old_method) {
        *old_method = pvpCtx->PVPCTX_iThreadDefMethod;
    }

    if ((method == PTHREAD_NULL_ATTR_METHOD_USE_INHERIT) ||
        (method == PTHREAD_NULL_ATTR_METHOD_USE_DEFSETTING)) {
        pvpCtx->PVPCTX_iThreadDefMethod = method;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_verify_np
** ��������: ���ָ���߳��Ƿ����
** �䡡��  : pthread       �߳̾��
** �䡡��  : 0: ������� -1: ���񲻴���
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int  pthread_verify_np (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);

    if (API_ThreadVerify(thread)) {
        return  (0);

    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: pthread_setname_np
** ��������: �����߳�����
** �䡡��  : thread    �߳̾��
**           name      �߳�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setname_np (pthread_t  thread, const char  *name)
{
    ULONG   ulError;
    
    PX_ID_VERIFY(thread, pthread_t);

    ulError = API_ThreadSetName(thread, name);
    if (ulError == ERROR_KERNEL_PNAME_TOO_LONG) {
        errno = ERANGE;
        return  (ERANGE);
    
    } else if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getname_np
** ��������: ����߳�����
** �䡡��  : thread    �߳̾��
**           name      �߳�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getname_np (pthread_t  thread, char  *name, size_t len)
{
    ULONG   ulError;
    CHAR    cNameBuffer[LW_CFG_OBJECT_NAME_SIZE];
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadGetName(thread, cNameBuffer);
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (lib_strlen(cNameBuffer) >= len) {
        errno = ERANGE;
        return  (ERANGE);
    }
    
    lib_strcpy(name, cNameBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_wakeup_np
** ��������: ����һ���������߳�
** �䡡��  : thread        �߳̾��
**           timeout_only  �Ƿ�����Ѵ��г�ʱ���õ������߳�.
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int  pthread_wakeup_np (pthread_t  thread, int  timeout_only)
{
    ULONG   ulError;
    BOOL    bWithInfPend = timeout_only ? LW_FALSE : LW_TRUE;

    PX_ID_VERIFY(thread, pthread_t);

    ulError = API_ThreadWakeupEx(thread, bWithInfPend);
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_list_callback
** ��������: ��ȡ��ǰ�����߳��б�ص�
** �䡡��  : ptcb          ������ƿ�
**           list          �߳��б�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static void  pthread_list_callback (PLW_CLASS_TCB  ptcb, struct pthread_list *list)
{
    struct pthread_info *info;

    list->total++;
    if (list->get_cnt < list->pool_cnt) {
        info = &list->pool[list->get_cnt];
        info->tid = ptcb->TCB_ulId;
        if (info->name && info->size) {
            lib_strlcpy(info->name, ptcb->TCB_cThreadName, info->size);
        }
        list->get_cnt++;
    }
}
/*********************************************************************************************************
** ��������: pthread_list_np
** ��������: ��ȡ��ǰ�����߳��б�
** �䡡��  : list          �߳��б�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int  pthread_list_np (struct pthread_list *list)
{
    struct pthread_info *info;
    volatile char        byte;

    UINT   i;
    pid_t  pid = getpid();

    (void)byte;

    if (!list) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pid <= 0) {
        errno = ESRCH;
        return  (ESRCH);
    }

    if (list->pool && list->pool_cnt) {
        for (i = 0; i < list->pool_cnt; i++) {
            info = &list->pool[i];
            if (info->name && info->size) {
                byte = info->name[0];                                   /*  ͨ����ѭ���ж��ڴ���Ч��    */
            }
        }
    }

    list->total   = 0;
    list->get_cnt = 0;

    vprocThreadTraversal(pid, pthread_list_callback, list,
                         LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: pthread_safe_np
** ��������: �߳̽��밲ȫģʽ, �κζԱ��̵߳�ɾ�����������Ƴٵ��߳��˳���ȫģʽʱ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_safe_np (void)
{
    API_ThreadSafe();
}
/*********************************************************************************************************
** ��������: pthread_unsafe_np
** ��������: �߳��˳���ȫģʽ.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  pthread_unsafe_np (void)
{
    API_ThreadUnsafe();
}
/*********************************************************************************************************
** ��������: pthread_int_lock_np
** ��������: �߳�������ǰ���ں��ж�, ������Ӧ�ж�.
** �䡡��  : irqctx        ��ϵ�ṹ����ж�״̬����ṹ (�û�����Ҫ���ľ�������)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_int_lock_np (pthread_int_t *irqctx)
{
    if (!irqctx) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *irqctx = KN_INT_DISABLE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_int_unlock_np
** ��������: �߳̽�����ǰ���ں��ж�, ��ʼ��Ӧ�ж�.
** �䡡��  : irqctx        ��ϵ�ṹ����ж�״̬����ṹ (�û�����Ҫ���ľ�������)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_int_unlock_np (pthread_int_t irqctx)
{
    KN_INT_ENABLE(irqctx);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setaffinity_np
** ��������: �����̵߳��ȵ� CPU ����
** �䡡��  : pid           ���� / �߳� ID
**           setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : SylixOS Ŀǰ��֧�ֽ�����������һ�� CPU, ��������ֻ�ܽ��߳�������ָ������С CPU ����.
**
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setaffinity_np (pthread_t  thread, size_t setsize, const cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if (!setsize || !set) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_ThreadSetAffinity(thread, setsize, (PLW_CLASS_CPUSET)set)) {
        errno = ESRCH;
        return  (ESRCH);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getaffinity_np
** ��������: ��ȡ�̵߳��ȵ� CPU ����
** �䡡��  : pid           ���� / �߳� ID
**           setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getaffinity_np (pthread_t  thread, size_t setsize, cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if ((setsize < sizeof(cpu_set_t)) || !set) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_ThreadGetAffinity(thread, setsize, set)) {
        errno = ESRCH;
        return  (ESRCH);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: pthread_getid
** ��������: ͨ���߳�������߳̾��
** �䡡��  : name          �߳���
**           pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_getid (const char *name, pthread_t *pthread)
{
    INT     i;

#if LW_CFG_MODULELOADER_EN > 0
    pid_t   pid = getpid();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    PLW_CLASS_TCB      ptcb;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!name || !pthread) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  ��� TCB ���ƿ�             */
        if (ptcb == LW_NULL) {                                          /*  �̲߳�����                  */
            continue;
        }
        
        if (API_ThreadDesc(ptcb->TCB_ulId, &tcbdesc)) {
            continue;
        }
        
#if LW_CFG_MODULELOADER_EN > 0
        if (pid != (pid_t)tcbdesc.TCBD_lPid) {
            continue;
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        if (lib_strcmp(name, tcbdesc.TCBD_cThreadName)) {
            continue;
        }
        
        break;
    }
    
    if (i < LW_CFG_MAX_THREADS) {
        *pthread = tcbdesc.TCBD_ulId;
        return  (ERROR_NONE);
        
    } else {
        errno = ESRCH;
        return  (ESRCH);
    }
}
/*********************************************************************************************************
** ��������: pthread_getname
** ��������: ����߳���
** �䡡��  : pthread       �߳̾��
**           name          �߳���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getname (pthread_t thread, char *name)
{
    CHAR    cNameBuffer[LW_CFG_OBJECT_NAME_SIZE];
    
    if (!name) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadGetName(thread, cNameBuffer)) {
        errno = ESRCH;
        return  (ESRCH);
    }

    lib_strcpy(name, cNameBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_suspend
** ��������: �̹߳���
** �䡡��  : pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_suspend (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadSuspend(thread)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_resume
** ��������: �߳̽������
** �䡡��  : pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_resume (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadResume(thread)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_delay
** ��������: �̵߳ȴ�
** �䡡��  : ticks       �ӳ�ʱ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_delay (int  ticks)
{
    ULONG   ulTick;
    
    if (ticks < 0) {
        return  (ERROR_NONE);
    }
    
    if (ticks == 0) {
        API_ThreadYield(API_ThreadIdSelf());
        return  (ERROR_NONE);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    ulTick = (ULONG)ticks;
    
    return  ((int)API_TimeSleepEx(ulTick, LW_TRUE));
}
/*********************************************************************************************************
** ��������: pthread_lock
** ��������: �߳�������ǰ CPU ����
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_lock (void)
{
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        errno = ECALLEDINISR;
        return  (ECALLEDINISR);
    }

    API_ThreadLock();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_unlock
** ��������: �߳̽�����ǰ CPU ����
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_unlock (void)
{
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        errno = ECALLEDINISR;
        return  (ECALLEDINISR);
    }
    
    API_ThreadUnlock();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_is_ready
** ��������: �߳��Ƿ����
** �䡡��  : pthread       �߳̾��
** �䡡��  : �Ƿ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
boolean  pthread_is_ready (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (!API_ThreadVerify(thread)) {
        errno = ESRCH;
        return  (LW_FALSE);
    }
    
    return  (API_ThreadIsReady(thread));
}
/*********************************************************************************************************
** ��������: pthread_is_suspend
** ��������: �߳��Ƿ����
** �䡡��  : pthread       �߳̾��
** �䡡��  : �Ƿ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
boolean  pthread_is_suspend (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (!API_ThreadVerify(thread)) {
        errno = ESRCH;
        return  (LW_FALSE);
    }
    
    if (API_ThreadIsSuspend(thread)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: pthread_verifyid
** ��������: ���ָ���߳��Ƿ����
** �䡡��  : pthread       �߳̾��
** �䡡��  : 0: ������� -1: ���񲻴���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_verifyid (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadVerify(thread)) {
        return  (0);
    
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: pthread_cancelforce
** ��������: ǿ���߳�ɾ��
** �䡡��  : pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_cancelforce (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadForceDelete(&thread, LW_NULL)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getinfo
** ��������: ����߳���Ϣ
** �䡡��  : pthread       �߳̾��
**           info          �߳���Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getinfo (pthread_t thread, pthread_info_t *info)
{
    if (!info) {
        errno = EINVAL;
        return  (EINVAL);
    }

    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadDesc(thread, info)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getregs
** ��������: ����̼߳Ĵ�����
** �䡡��  : pthread       �߳̾��
**           pregs         �̼߳Ĵ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getregs (pthread_t thread, REG_SET *pregs)
{
    UINT16         usIndex;
    PLW_CLASS_TCB  ptcb;
    
    if (!pregs) {
        errno = EINVAL;
        return  (EINVAL);
    }

    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread == API_ThreadIdSelf()) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    usIndex = _ObjectGetIndex(thread);
    
    if (!_ObjectClassOK(thread, _OBJECT_THREAD)) {                      /*  ��� ID ������Ч��          */
        errno = ESRCH;
        return  (ESRCH);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        errno = ESRCH;
        return  (ESRCH);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        errno = ESRCH;
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb   = _K_ptcbTCBIdTable[usIndex];
    *pregs = ptcb->TCB_archRegCtx;
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_show
** ��������: ��ʾ�߳���Ϣ
** �䡡��  : pthread       �߳̾��
**           level         ��ʾ�ȼ� (δʹ��)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_show (pthread_t thread, int level)
{
#if LW_CFG_MODULELOADER_EN > 0
    pid_t   pid = getpid();
#else
    pid_t   pid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    (VOID)thread;

    API_ThreadShowEx(pid);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_showstack
** ��������: ��ʾ�̶߳�ջ��Ϣ
** �䡡��  : pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_showstack (pthread_t thread)
{
    (VOID)thread;
    
    API_StackShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_showstackframe
** ��������: ��ʾ�̵߳���ջ��Ϣ
** �䡡��  : pthread       �߳̾��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_showstackframe (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread != pthread_self()) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }

    API_BacktraceShow(STD_OUT, 100);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_addvar
** ��������: �̼߳���˽�б���
** �䡡��  : pthread       �߳̾��
**           pvar          ˽�б�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_CPU_WORD_LENGHT == 32)

LW_API 
int  pthread_addvar (pthread_t thread, int *pvar)
{
    ULONG   ulError;

    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadVarAdd(thread, (ULONG *)pvar);
    if (ulError == ERROR_THREAD_VAR_FULL) {
        errno = ENOSPC;
        return  (ENOSPC);
    
    } else if (ulError) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_delvar
** ��������: �߳�ɾ��˽�б���
** �䡡��  : pthread       �߳̾��
**           pvar          ˽�б�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_delvar (pthread_t thread, int *pvar)
{
    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    if (API_ThreadVarDelete(thread, (ULONG *)pvar)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setvar
** ��������: �����߳�˽�б���
** �䡡��  : pthread       �߳̾��
**           pvar          ˽�б�����ַ
**           value         ����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setvar (pthread_t thread, int *pvar, int value)
{
    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    if (API_ThreadVarSet(thread, (ULONG *)pvar, (ULONG)value)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getvar
** ��������: ����߳�˽�б���
** �䡡��  : pthread       �߳̾��
**           pvar          ˽�б�����ַ
**           value         ����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getvar (pthread_t thread, int *pvar, int *value)
{
    if (!pvar || !value) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    *value = (int)API_ThreadVarGet(thread, (ULONG *)pvar);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
/*********************************************************************************************************
** ��������: pthread_create_hook_add
** ��������: ���һ�����񴴽� HOOK
** �䡡��  : create_hook   ���񴴽� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_create_hook_add (OS_STATUS (*create_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!create_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)create_hook, 
                          LW_OPTION_THREAD_CREATE_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_create_hook_delete
** ��������: ɾ��һ�����񴴽� HOOK
** �䡡��  : create_hook   ���񴴽� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_create_hook_delete (OS_STATUS (*create_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!create_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)create_hook, 
                             LW_OPTION_THREAD_CREATE_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_switch_hook_add
** ��������: ���һ�������л� HOOK
** �䡡��  : switch_hook   �����л� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_switch_hook_add (OS_STATUS (*switch_hook)(pthread_t, pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!switch_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)switch_hook, 
                          LW_OPTION_THREAD_SWAP_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_switch_hook_delete
** ��������: ɾ��һ�������л� HOOK
** �䡡��  : switch_hook   �����л� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_switch_hook_delete (OS_STATUS (*switch_hook)(pthread_t, pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!switch_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)switch_hook, 
                             LW_OPTION_THREAD_SWAP_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_close_hook_add
** ��������: ���һ������ɾ�� HOOK
** �䡡��  : close_hook    ����ɾ�� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_close_hook_add (OS_STATUS (*close_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!close_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)close_hook, 
                          LW_OPTION_THREAD_DELETE_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_close_hook_delete
** ��������: ɾ��һ������ɾ�� HOOK
** �䡡��  : close_hook   ����ɾ�� HOOK
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_close_hook_delete (OS_STATUS (*close_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!close_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)close_hook, 
                             LW_OPTION_THREAD_DELETE_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
