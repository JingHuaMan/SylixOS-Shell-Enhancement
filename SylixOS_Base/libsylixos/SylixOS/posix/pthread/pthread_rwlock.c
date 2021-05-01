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
** ��   ��   ��: pthread_rwlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ��д����.

** BUG:
2010.01.12  ����ʱ����ʱ, ֻ�轫 pend ������һ������һ�μ���.
2010.01.13  ʹ�ü����ź����䵱ͬ����, ÿ��״̬�ĸı伤�����е��߳�, ���ǽ��Լ�������ռ.
2012.12.13  ���� SylixOS ֧�ֽ�����Դ����, ���￪ʼ֧�־�̬��ʼ��.
2013.05.01  If successful, the pthread_rwlockattr_*() and pthread_rwlock_*() functions shall return zero;
            otherwise, an error number shall be returned to indicate the error.
2016.04.13  ���� GJB7714 ��� API ֧��.
2016.05.09  ���γ�ʼ��ȷ�����̰߳�ȫ.
2016.07.21  ʹ���ں˶�д��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  ��
*********************************************************************************************************/
#define __PX_RWLOCKATTR_SHARED  1
#define __PX_RWLOCKATTR_WRITER  2
/*********************************************************************************************************
  ��ʼ����
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulPRWLockInitLock;
/*********************************************************************************************************
** ��������: _posixPRWLockInit
** ��������: ��ʼ�� ��д�� ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _posixPRWLockInit (VOID)
{
    _G_ulPRWLockInitLock = API_SemaphoreMCreate("prwinit", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_INHERIT_PRIORITY | 
                                                LW_OPTION_WAIT_PRIORITY | 
                                                LW_OPTION_OBJECT_GLOBAL |
                                                LW_OPTION_DELETE_SAFE, LW_NULL);
}
/*********************************************************************************************************
** ��������: __pthread_rwlock_init_invisible
** ��������: ��д�����δ���. (��̬��ʼ��)
** �䡡��  : prwlock        ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __pthread_rwlock_init_invisible (pthread_rwlock_t  *prwlock)
{
    if (prwlock) {
        if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPRWLockInitLock, LW_OPTION_WAIT_INFINITE);
            if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
                pthread_rwlock_init(prwlock, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPRWLockInitLock);
        }
    }
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_init
** ��������: ��ʼ��һ����д�����Կ�.
** �䡡��  : prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_init (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    *prwlockattr = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_destroy
** ��������: ����һ����д�����Կ�.
** �䡡��  : prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_destroy (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *prwlockattr = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_setpshared
** ��������: ����һ����д�����Կ��Ƿ���̹���.
** �䡡��  : prwlockattr    ���Կ�
**           pshared        ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_setpshared (pthread_rwlockattr_t *prwlockattr, int  pshared)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        *prwlockattr |= __PX_RWLOCKATTR_SHARED;
    
    } else {
        *prwlockattr |= ~__PX_RWLOCKATTR_SHARED;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_getpshared
** ��������: ��ȡһ����д�����Կ��Ƿ���̹���.
** �䡡��  : prwlockattr    ���Կ�
**           pshared        ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_getpshared (const pthread_rwlockattr_t *prwlockattr, int  *pshared)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        if (*prwlockattr & __PX_RWLOCKATTR_SHARED) {
            *pshared = PTHREAD_PROCESS_SHARED;
        
        } else {
            *pshared = PTHREAD_PROCESS_PRIVATE;
        }
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_setkind_np
** ��������: ����һ����д�����Կ��д����Ȩ.
** �䡡��  : prwlockattr    ���Կ�
**           pref           ����Ȩѡ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_setkind_np (pthread_rwlockattr_t *prwlockattr, int pref)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pref == PTHREAD_RWLOCK_PREFER_READER_NP) {
        *prwlockattr &= ~__PX_RWLOCKATTR_WRITER;
    
    } else if (pref == PTHREAD_RWLOCK_PREFER_WRITER_NP) {
        *prwlockattr |= __PX_RWLOCKATTR_WRITER;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_setkind_np
** ��������: ����һ����д�����Կ��д����Ȩ.
** �䡡��  : prwlockattr    ���Կ�
**           pref           ����Ȩѡ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_getkind_np (const pthread_rwlockattr_t *prwlockattr, int *pref)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pref) {
        if (*prwlockattr & __PX_RWLOCKATTR_WRITER) {
            *pref = PTHREAD_RWLOCK_PREFER_WRITER_NP;
        
        } else {
            *pref = PTHREAD_RWLOCK_PREFER_READER_NP;
        }
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_init
** ��������: ����һ����д��.
** �䡡��  : prwlock        ���
**           prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_init (pthread_rwlock_t  *prwlock, const pthread_rwlockattr_t  *prwlockattr)
{
    ULONG   ulOption = LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (prwlockattr) {
        if (*prwlockattr & __PX_RWLOCKATTR_WRITER) {
            ulOption |= LW_OPTION_RW_PREFER_WRITER;
        }
    }
    
    prwlock->PRWLOCK_ulRwLock = API_SemaphoreRWCreate("prwrlock", ulOption, LW_NULL);
    if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_destroy
** ��������: ����һ����д��.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_destroy (pthread_rwlock_t  *prwlock)
{
    if ((prwlock == LW_NULL) || (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SemaphoreRWDelete(&prwlock->PRWLOCK_ulRwLock)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_rdlock
** ��������: �ȴ�һ����д���ɶ�.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_rdlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, 
                             LW_OPTION_WAIT_INFINITE)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_tryrdlock
** ��������: �������ȴ�һ����д���ɶ�.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_tryrdlock (pthread_rwlock_t  *prwlock)
{
    ULONG    ulError;

    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, LW_OPTION_NOT_WAIT);
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
** ��������: pthread_rwlock_timedrdlock
** ��������: �ȴ�һ����д���ɶ� (���г�ʱ������).
** �䡡��  : prwlock        ���
**           abs_timeout    ���Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedrdlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  ת����ʱʱ��                */

    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** ��������: pthread_rwlock_reltimedrdlock_np
** ��������: �ȴ�һ����д���ɶ� (���г�ʱ������).
** �䡡��  : prwlock        ���
**           rel_timeout    ��Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_rwlock_reltimedrdlock_np (pthread_rwlock_t *prwlock,
                                       const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;

    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  ת����ʱʱ��                */
    
    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** ��������: pthread_rwlock_wrlock
** ��������: �ȴ�һ����д����д.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_wrlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    if (API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, 
                             LW_OPTION_WAIT_INFINITE)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_trywrlock
** ��������: �������ȴ�һ����д����д.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_trywrlock (pthread_rwlock_t  *prwlock)
{
    ULONG   ulError;

    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, LW_OPTION_NOT_WAIT);
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
** ��������: pthread_rwlock_wrlock
** ��������: �ȴ�һ����д����д (���г�ʱ������).
** �䡡��  : prwlock        ���
**           abs_timeout    ���Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedwrlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  ת����ʱʱ��                */

    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** ��������: pthread_rwlock_reltimedwrlock_np
** ��������: �ȴ�һ����д����д (���г�ʱ������).
** �䡡��  : prwlock        ���
**           rel_timeout    ��Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����೬�� ulong tick ��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.

                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_rwlock_reltimedwrlock_np (pthread_rwlock_t *prwlock,
                                       const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  ת����ʱʱ��                */
    
    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** ��������: pthread_rwlock_unlock
** ��������: �ͷ�һ����д��.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_unlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWPost(prwlock->PRWLOCK_ulRwLock)) {
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
** ��������: pthread_rwlock_getinfo
** ��������: ��ö�д����Ϣ
** �䡡��  : prwlock       ��д�����ƿ�
**           info          ��д����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_rwlock_getinfo (pthread_rwlock_t  *prwlock, pthread_rwlock_info_t  *info)
{
    ULONG   ulRWCount;
    ULONG   ulRPend;
    ULONG   ulWPend;

    if ((prwlock == LW_NULL) || (info == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWStatus(prwlock->PRWLOCK_ulRwLock,
                              &ulRWCount,
                              &ulRPend,
                              &ulWPend,
                              LW_NULL,
                              &info->owner)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    info->opcnt = (unsigned int)ulRWCount;
    info->rpend = (unsigned int)ulRPend;
    info->wpend = (unsigned int)ulWPend;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
