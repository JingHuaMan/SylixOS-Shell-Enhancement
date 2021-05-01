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
** ��   ��   ��: pthread_mutex.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread �����ź������ݿ�. (mutex �������ᷢ�� EINTR)

** BUG:
2010.01.13  ��ʼ��������ʱ, ���û��ָ������, ������Ĭ������.
2011.05.30  pthread_mutex_init() �����ж� pmutex �л���������Ч��.
2012.06.18  Higher numerical values for the priority represent higher priorities.
2012.12.08  ���� PTHREAD_MUTEX_NORMAL �� PTHREAD_MUTEX_ERRORCHECK ��֧��.
2012.12.13  ���� SylixOS ֧�ֽ�����Դ����, ���￪ʼ֧�־�̬��ʼ��.
2013.05.01  Upon successful completion, pthread_mutexattr_*() and pthread_mutex_*() shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
2016.05.08  ���γ�ʼ��ȷ�����̰߳�ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  Ĭ������
*********************************************************************************************************/
static const pthread_mutexattr_t    _G_pmutexattrDefault = {
        1,
        PTHREAD_MUTEX_DEFAULT,                                          /*  ����ݹ����                */
        PTHREAD_MUTEX_CEILING,
        (LW_OPTION_INHERIT_PRIORITY | LW_OPTION_WAIT_PRIORITY)          /*  PTHREAD_PRIO_NONE           */
};
/*********************************************************************************************************
  ��ʼ����
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_ulPMutexInitLock;
/*********************************************************************************************************
** ��������: _posixPMutexInit
** ��������: ��ʼ�� MUTEX ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _posixPMutexInit (VOID)
{
    if (!_G_ulPMutexInitLock) {
        _G_ulPMutexInitLock = API_SemaphoreMCreate("pmutexinit", LW_PRIO_DEF_CEILING,
                                                   LW_OPTION_INHERIT_PRIORITY |
                                                   LW_OPTION_WAIT_PRIORITY |
                                                   LW_OPTION_OBJECT_GLOBAL |
                                                   LW_OPTION_DELETE_SAFE, LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __pthread_mutex_init_invisible
** ��������: ���������δ���. (��̬��ʼ��) (���ﲻ�� static ��Ϊ pthread cond Ҫʹ��)
** �䡡��  : pmutex         ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __pthread_mutex_init_invisible (pthread_mutex_t  *pmutex)
{
    if (pmutex) {
        if (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPMutexInitLock, LW_OPTION_WAIT_INFINITE);
            if (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID) {
                pthread_mutex_init(pmutex, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPMutexInitLock);
        }
    }
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_init
** ��������: ��ʼ�����������Կ�.
** �䡡��  : pmutexattr     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_init (pthread_mutexattr_t *pmutexattr)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pmutexattr = _G_pmutexattrDefault;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_destroy
** ��������: ����һ�����������Կ�.
** �䡡��  : pmutexattr     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_destroy (pthread_mutexattr_t *pmutexattr)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iIsEnable = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_setprioceiling
** ��������: ����һ�����������Կ���컨�����ȼ�.
** �䡡��  : pmutexattr     ����
**           prioceiling    ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setprioceiling (pthread_mutexattr_t *pmutexattr, int  prioceiling)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((prioceiling < __PX_PRIORITY_MIN) ||
        (prioceiling > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iPrioceiling = prioceiling;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getprioceiling
** ��������: ���һ�����������Կ���컨�����ȼ�.
** �䡡��  : pmutexattr     ����
**           prioceiling    ���ȼ�(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getprioceiling (const pthread_mutexattr_t *pmutexattr, int  *prioceiling)
{
    if ((pmutexattr == LW_NULL) || (prioceiling == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *prioceiling = pmutexattr->PMUTEXATTR_iPrioceiling;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_setprotocol
** ��������: ����һ�����������Կ���㷨����.
** �䡡��  : pmutexattr     ����
**           protocol       �㷨
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setprotocol (pthread_mutexattr_t *pmutexattr, int  protocol)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    switch (protocol) {
    
    case PTHREAD_PRIO_NONE:                                             /*  Ĭ��ʹ�����ȼ��̳��㷨      */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_INHERIT_PRIORITY;
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_WAIT_PRIORITY;    /*  fifo                        */
        break;
    
    case PTHREAD_PRIO_INHERIT:
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_INHERIT_PRIORITY; /*  ���ȼ��̳��㷨              */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_WAIT_PRIORITY;
        break;
        
    case PTHREAD_PRIO_PROTECT:
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_INHERIT_PRIORITY; /*  ���ȼ��컨��                */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_WAIT_PRIORITY;
        break;
        
    default:
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_getprotocol
** ��������: ���һ�����������Կ���㷨����.
** �䡡��  : pmutexattr     ����
**           protocol       �㷨(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getprotocol (const pthread_mutexattr_t *pmutexattr, int  *protocol)
{
    if ((pmutexattr == LW_NULL) || (protocol == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_INHERIT_PRIORITY) &&
        (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY)) {
        *protocol = PTHREAD_PRIO_INHERIT;
    } else if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY) {
        *protocol = PTHREAD_PRIO_PROTECT;
    } else {
        *protocol = PTHREAD_PRIO_NONE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_setpshared
** ��������: ����һ�����������Կ��Ƿ���̹���.
** �䡡��  : pmutexattr     ����
**           pshared        ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setpshared (pthread_mutexattr_t *pmutexattr, int  pshared)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_getpshared
** ��������: ��ȡһ�����������Կ��Ƿ���̹���.
** �䡡��  : pmutexattr     ����
**           pshared        ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getpshared (const pthread_mutexattr_t *pmutexattr, int  *pshared)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        *pshared = PTHREAD_PROCESS_PRIVATE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_settype
** ��������: ����һ�����������Կ�����.
** �䡡��  : pmutexattr     ����
**           type           ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_settype (pthread_mutexattr_t *pmutexattr, int  type)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((type != PTHREAD_MUTEX_NORMAL) &&
        (type != PTHREAD_MUTEX_ERRORCHECK) &&
        (type != PTHREAD_MUTEX_RECURSIVE)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iType = type;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_gettype
** ��������: ���һ�����������Կ�����.
** �䡡��  : pmutexattr     ����
**           type           ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_gettype (const pthread_mutexattr_t *pmutexattr, int  *type)
{
    if ((pmutexattr == LW_NULL) || (type == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *type = pmutexattr->PMUTEXATTR_iType;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_setwaitqtype
** ��������: ���û������ȴ���������.
** �䡡��  : pmutexattr     ����
**           waitq_type     �ȴ���������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setwaitqtype (pthread_mutexattr_t  *pmutexattr, int  waitq_type)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (waitq_type == PTHREAD_WAITQ_PRIO) {
        pmutexattr->PMUTEXATTR_ulOption |= LW_OPTION_WAIT_PRIORITY;
    
    } else if (waitq_type == PTHREAD_WAITQ_FIFO) {
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_WAIT_PRIORITY;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_getwaitqtype
** ��������: ��û������ȴ���������.
** �䡡��  : pmutexattr     ����
**           waitq_type     �ȴ���������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getwaitqtype (const pthread_mutexattr_t *pmutexattr, int *waitq_type)
{
    if ((pmutexattr == LW_NULL) || (waitq_type == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY) {
        *waitq_type = PTHREAD_WAITQ_PRIO;
    
    } else {
        *waitq_type = PTHREAD_WAITQ_FIFO;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_setcancelsafe
** ��������: ���û�������ȫ����.
** �䡡��  : pmutexattr     ����
**           cancel_safe    ��ȫ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setcancelsafe (pthread_mutexattr_t  *pmutexattr, int  cancel_safe)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (cancel_safe == PTHREAD_CANCEL_SAFE) {
        pmutexattr->PMUTEXATTR_ulOption |= LW_OPTION_DELETE_SAFE;
    
    } else if (cancel_safe == PTHREAD_CANCEL_UNSAFE) {
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_DELETE_SAFE;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutexattr_getcancelsafe
** ��������: ��û�������ȫ����.
** �䡡��  : pmutexattr     ����
**           cancel_safe    ��ȫ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getcancelsafe (const pthread_mutexattr_t *pmutexattr, int *cancel_safe)
{
    if ((pmutexattr == LW_NULL) || (cancel_safe == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_DELETE_SAFE) {
        *cancel_safe = PTHREAD_CANCEL_SAFE;
    
    } else {
        *cancel_safe = PTHREAD_CANCEL_UNSAFE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_init
** ��������: ����һ��������.
** �䡡��  : pmutex         ���
**           pmutexattr     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_init (pthread_mutex_t  *pmutex, const pthread_mutexattr_t *pmutexattr)
{
    pthread_mutexattr_t     mutexattr;
    UINT8                   ucPriority;

    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr) {
        mutexattr = *pmutexattr;
    } else {
        mutexattr = _G_pmutexattrDefault;                               /*  ʹ��Ĭ������                */
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(mutexattr.PMUTEXATTR_iPrioceiling);
    
    if (mutexattr.PMUTEXATTR_iType == PTHREAD_MUTEX_NORMAL) {
        mutexattr.PMUTEXATTR_ulOption |= LW_OPTION_NORMAL;              /*  ��������� (���Ƽ�)         */
    
    } else if (mutexattr.PMUTEXATTR_iType == PTHREAD_MUTEX_ERRORCHECK) {
        mutexattr.PMUTEXATTR_ulOption |= LW_OPTION_ERRORCHECK;          /*  �������                    */
    }
    
    pmutex->PMUTEX_ulMutex = API_SemaphoreMCreate("pmutex", 
                                ucPriority,
                                mutexattr.PMUTEXATTR_ulOption,
                                LW_NULL);                               /*  ����������                  */
    if (pmutex->PMUTEX_ulMutex == 0) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    pmutex->PMUTEX_iType = mutexattr.PMUTEXATTR_iType;                  /*  δ����չʹ��                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_destroy
** ��������: ɾ��һ��������.
** �䡡��  : pmutex         ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_destroy (pthread_mutex_t  *pmutex)
{
    if ((pmutex == LW_NULL) || (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SemaphoreMDelete(&pmutex->PMUTEX_ulMutex)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_lock
** ��������: lock һ��������.
** �䡡��  : pmutex         ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_lock (pthread_mutex_t  *pmutex)
{
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    if (API_SemaphoreMPend(pmutex->PMUTEX_ulMutex,
                           LW_OPTION_WAIT_INFINITE)) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_trylock
** ��������: trylock һ��������.
** �䡡��  : pmutex         ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_trylock (pthread_mutex_t  *pmutex)
{
    ULONG    ulError;
    
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, LW_OPTION_NOT_WAIT);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = EBUSY;
        return  (EBUSY);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_timedlock
** ��������: lock һ��������(���г�ʱʱ��).
** �䡡��  : pmutex         ���
**           abs_timeout    ��ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_timedlock (pthread_mutex_t  *pmutex, const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  ת����ʱʱ��                */

    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_reltimedlock_np
** ��������: lock һ��������(���г�ʱʱ��).
** �䡡��  : pmutex         ���
**           rel_timeout    ��Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_mutex_reltimedlock_np (pthread_mutex_t  *pmutex, const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  ת����ʱʱ��                */
    
    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: pthread_mutex_unlock
** ��������: unlock һ��������.
** �䡡��  : pmutex         ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_unlock (pthread_mutex_t  *pmutex)
{
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    if (API_SemaphoreMPost(pmutex->PMUTEX_ulMutex)) {
        if (errno == ERROR_EVENT_NOT_OWN) {
            errno = EPERM;
        } else {
            errno = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_setprioceiling
** ��������: ����һ�����������ȼ��컨��.
** �䡡��  : pmutex         ���
**           prioceiling    ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_setprioceiling (pthread_mutex_t  *pmutex, int  prioceiling)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((prioceiling < __PX_PRIORITY_MIN) ||
        (prioceiling > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  �����Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    pevent->EVENT_ucCeilingPriority = (UINT8)PX_PRIORITY_CONVERT(prioceiling);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_getprioceiling
** ��������: ���һ�����������ȼ��컨��.
** �䡡��  : pmutex         ���
**           prioceiling    ���ȼ�(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_getprioceiling (pthread_mutex_t  *pmutex, int  *prioceiling)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if ((pmutex == LW_NULL) || (prioceiling == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  �����Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    *prioceiling = PX_PRIORITY_CONVERT(pevent->EVENT_ucCeilingPriority);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_getinfo
** ��������: ��û�������Ϣ.
** �䡡��  : pmutex        ���������
**           info          ��������Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_mutex_getinfo (pthread_mutex_t  *pmutex, pthread_mutex_info_t  *info)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if ((pmutex == LW_NULL) || (info == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  �����Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ���ȷ                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (pevent->EVENT_ulCounter) {
        info->value = 1;
    } else {
        info->value = 0;
    }
    
    if (pevent->EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY) {
        info->inherit = 1;
    } else {
        info->inherit = 0;
    }
    
    info->prioceiling = PX_PRIORITY_CONVERT(pevent->EVENT_ucCeilingPriority);
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        info->wait_type = PTHREAD_WAITQ_PRIO;
    } else {
        info->wait_type = PTHREAD_WAITQ_FIFO;
    }
    
    if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {
        info->cancel_type = PTHREAD_CANCEL_SAFE;
    } else {
        info->cancel_type = PTHREAD_CANCEL_UNSAFE;
    }
    
    info->blocknum = _EventWaitNum(EVENT_SEM_Q, pevent);
    
    if (info->value == 0) {
        info->ownner = ((PLW_CLASS_TCB)(pevent->EVENT_pvTcbOwn))->TCB_ulId;
    } else {
        info->ownner = LW_OBJECT_HANDLE_INVALID;
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_mutex_show
** ��������: ��ʾ��������Ϣ.
** �䡡��  : pmutex        ���������
**           level         �ȼ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_mutex_show (pthread_mutex_t  *pmutex, int  level)
{
    (VOID)level;

    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    API_SemaphoreShow(pmutex->PMUTEX_ulMutex);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
