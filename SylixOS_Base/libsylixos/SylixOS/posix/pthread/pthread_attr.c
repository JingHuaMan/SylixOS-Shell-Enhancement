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
** ��   ��   ��: pthread_attr.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ���Լ��ݿ�.

** BUG:
2012.09.11  �����߳��ǲ���ǿ��ָ���Ƿ�ʹ�� FPU, ��Ӧ�ɲ���ϵͳ�����ж�.
2013.05.01  Upon successful completion, pthread_attr_*() shall return a value of 0; 
            otherwise, an error number shall be returned to indicate the error.
2013.05.03  pthread_attr_init() ��ջ��С����Ϊ 0, ��ʾ�̳��̴߳����߶�ջ��С.
2013.05.04  ����һЩ���õ� UNIX ��չ�ӿ�.
2013.09.17  ����Զ�ջ��������֧��.
2016.04.12  ���� GJB7714 ��� API ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_POSIX
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#include "limits.h"
/*********************************************************************************************************
** ��������: pthread_attr_init
** ��������: ��ʼ���߳����Կ�.
** �䡡��  : pattr         ��Ҫ��ʼ���� attr ָ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_init (pthread_attr_t  *pattr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pcName          = "pthread";
    pattr->PTHREADATTR_pvStackAddr     = LW_NULL;                       /*  �Զ������ջ                */
    pattr->PTHREADATTR_stStackGuard    = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pattr->PTHREADATTR_stStackByteSize = 0;                             /*  0 ��ʾ�̳д��������ȼ�      */
    pattr->PTHREADATTR_iSchedPolicy    = LW_OPTION_SCHED_RR;            /*  ���Ȳ���                    */
    pattr->PTHREADATTR_iInherit        = PTHREAD_EXPLICIT_SCHED;        /*  �̳���                      */
    pattr->PTHREADATTR_ulOption        = LW_OPTION_THREAD_STK_CHK;      /*  SylixOS �̴߳���ѡ��        */
    pattr->PTHREADATTR_schedparam.sched_priority = PX_PRIORITY_CONVERT(LW_PRIO_NORMAL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_destroy
** ��������: ����һ���߳����Կ�.
** �䡡��  : pattr         ��Ҫ���ٵ� attr ָ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_destroy (pthread_attr_t  *pattr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_ulOption = 0ul;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setstack
** ��������: ���ö�ջ����ز���.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           pvStackAddr   ��ջ��ַ
**           stSize        ��ջ��С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstack (pthread_attr_t *pattr, void *pvStackAddr, size_t stSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (_StackSizeCheck(stSize) || (stSize < PTHREAD_STACK_MIN)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pvStackAddr     = pvStackAddr;
    pattr->PTHREADATTR_stStackByteSize = stSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getstack
** ��������: ��ö�ջ����ز���.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           ppvStackAddr  ��ջ��ַ
**           pstSize       ��ջ��С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstack (const pthread_attr_t *pattr, void **ppvStackAddr, size_t *pstSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (ppvStackAddr) {
        *ppvStackAddr = pattr->PTHREADATTR_pvStackAddr;
    }
    
    if (pstSize) {
        *pstSize = pattr->PTHREADATTR_stStackByteSize;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setguardsize
** ��������: ����һ���߳����Կ�Ķ�ջ��������С.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           stGuard       ��ջ��������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setguardsize (pthread_attr_t  *pattr, size_t  stGuard)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_stStackGuard = stGuard;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getguardsize
** ��������: ��ȡһ���߳����Կ�Ķ�ջ��������С.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           pstSize       ��ȡ��ջ��������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getguardsize (const pthread_attr_t  *pattr, size_t  *pstGuard)
{
    if ((pattr == LW_NULL) || (pstGuard == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pstGuard = pattr->PTHREADATTR_stStackGuard;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setstacksize
** ��������: ����һ���߳����Կ�Ķ�ջ��С.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           stSize        ��ջ��С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstacksize (pthread_attr_t  *pattr, size_t  stSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (stSize) {
        if (_StackSizeCheck(stSize) || (stSize < PTHREAD_STACK_MIN)) {
            errno = EINVAL;
            return  (EINVAL);
        }
    }
    
    pattr->PTHREADATTR_stStackByteSize = stSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getstacksize
** ��������: ��ȡһ���߳����Կ�Ķ�ջ��С.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           pstSize       ��ջ��С�����ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstacksize (const pthread_attr_t  *pattr, size_t  *pstSize)
{
    if ((pattr == LW_NULL) || (pstSize == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pstSize = pattr->PTHREADATTR_stStackByteSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setstackaddr
** ��������: ָ��һ���߳����Կ�Ķ�ջ��ַ.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           pvStackAddr   ��ջ��ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstackaddr (pthread_attr_t  *pattr, void  *pvStackAddr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pvStackAddr = pvStackAddr;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getstackaddr
** ��������: ��ȡһ���߳����Կ�Ķ�ջ��ַ.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           ppvStackAddr  �����ջ��ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstackaddr (const pthread_attr_t  *pattr, void  **ppvStackAddr)
{
    if ((pattr == LW_NULL) || (ppvStackAddr == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppvStackAddr = pattr->PTHREADATTR_pvStackAddr;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setdetachstate
** ��������: ����һ���߳����Կ�� detach ״̬.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           iDetachState  detach ״̬
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setdetachstate (pthread_attr_t  *pattr, int  iDetachState)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (iDetachState == PTHREAD_CREATE_DETACHED) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_DETACHED;
    
    } else if (iDetachState == PTHREAD_CREATE_JOINABLE) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_DETACHED;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getdetachstate
** ��������: ��ȡһ���߳����Կ�� detach ״̬.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           piDetachState ���� detach ״̬
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getdetachstate (const pthread_attr_t  *pattr, int  *piDetachState)
{
    if ((pattr == LW_NULL) || (piDetachState == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_DETACHED) {
        *piDetachState = PTHREAD_CREATE_DETACHED;
    
    } else {
        *piDetachState = PTHREAD_CREATE_JOINABLE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setschedpolicy
** ��������: ����һ���߳����Կ�ĵ��Ȳ���.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           iPolicy       ���Ȳ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setschedpolicy (pthread_attr_t  *pattr, int  iPolicy)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    pattr->PTHREADATTR_iSchedPolicy = iPolicy;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getschedpolicy
** ��������: ��ȡһ���߳����Կ�ĵ��Ȳ���.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           piPolicy      ���Ȳ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getschedpolicy (const pthread_attr_t  *pattr, int  *piPolicy)
{
    if ((pattr == LW_NULL) || (piPolicy == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *piPolicy = pattr->PTHREADATTR_iSchedPolicy;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setschedparam
** ��������: ����һ���߳����Կ�ĵ���������.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           pschedparam   ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setschedparam (pthread_attr_t            *pattr, 
                                 const struct sched_param  *pschedparam)
{
    if ((pattr == LW_NULL) || (pschedparam == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_schedparam = *pschedparam;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getschedparam
** ��������: ���һ���߳����Կ�ĵ���������.
** �䡡��  : pattr         ��Ҫ��õ� attr ָ��.
**           pschedparam   ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getschedparam (const pthread_attr_t      *pattr, 
                                 struct sched_param  *pschedparam)
{
    if ((pattr == LW_NULL) || (pschedparam == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pschedparam = pattr->PTHREADATTR_schedparam;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setinheritsched
** ��������: ����һ���߳����Կ��Ƿ�̳е��Ȳ��Եķ�ʽ.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           iInherit      �����̵߳ļ̳�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setinheritsched (pthread_attr_t  *pattr, int  iInherit)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iInherit != PTHREAD_INHERIT_SCHED) &&
        (iInherit != PTHREAD_EXPLICIT_SCHED)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_iInherit = iInherit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getinheritsched
** ��������: ��ȡһ���߳����Կ��Ƿ�̳е��Ȳ��Եķ�ʽ.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           piInherit     �����̵߳ļ̳�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getinheritsched (const pthread_attr_t  *pattr, int  *piInherit)
{
    if ((pattr == LW_NULL) || (piInherit == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *piInherit = pattr->PTHREADATTR_iInherit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setscope
** ��������: ����һ���߳����Կ鴴�����߳�������.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           iScope        ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setscope (pthread_attr_t  *pattr, int  iScope)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (iScope == PTHREAD_SCOPE_PROCESS) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_SCOPE_PROCESS;
    
    } else if (iScope == PTHREAD_SCOPE_SYSTEM) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_SCOPE_PROCESS;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getscope
** ��������: ��ȡһ���߳����Կ鴴�����߳�������.
** �䡡��  : pattr         ��Ҫ��ȡ�� attr ָ��.
**           piScope       ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getscope (const pthread_attr_t  *pattr, int  *piScope)
{
    if ((pattr == LW_NULL) || (piScope == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_SCOPE_PROCESS) {
        *piScope = PTHREAD_SCOPE_PROCESS;
    
    } else {
        *piScope = PTHREAD_SCOPE_SYSTEM;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setname
** ��������: ����һ���߳����Կ������, �������߳̽�ʹ��ͬ��������.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           pcName        ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setname (pthread_attr_t  *pattr, const char  *pcName)
{
    if ((pattr == LW_NULL) || (pcName == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pcName = (char *)pcName;                         /*  pcName �ڴ治�ᱻ�޸�       */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getname
** ��������: ���һ���߳����Կ������, �������߳̽�ʹ��ͬ��������.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           ppcName       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_getname (const pthread_attr_t  *pattr, char  **ppcName)
{
    if ((pattr == LW_NULL) || (ppcName == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppcName = pattr->PTHREADATTR_pcName;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setinitonly_np
** ��������: ����һ���߳����Կ��Ƿ����ʼ���߳�.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           init          �Ƿ����ʼ���߳�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API
int  pthread_attr_setinitonly_np (pthread_attr_t  *pattr, int  init)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (init) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_INIT;
    } else {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_INIT;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getinitonly_np
** ��������: ��ȡһ���߳����Կ��Ƿ����ʼ���߳�.
** �䡡��  : pattr         ��Ҫ���õ� attr ָ��.
**           pinit         �Ƿ����ʼ���߳�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int  pthread_attr_getinitonly_np (const pthread_attr_t  *pattr, int  *pinit)
{
    if ((pattr == LW_NULL) || !pinit) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_INIT) {
        *pinit = 1;
    } else {
        *pinit = 0;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: pthread_attr_get_np
** ��������: ��ȡ�߳����Կ��ƿ� (FreeBSD ��չ�ӿ�)
** �䡡��  : thread        �߳� ID
**           pattr         ��Ҫ���õ� attr ָ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_get_np (pthread_t  thread, pthread_attr_t *pattr)
{
    LW_CLASS_THREADATTR     lwattr;
    UINT8                   ucPolicy = LW_OPTION_SCHED_RR;
    
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lwattr = API_ThreadAttrGet(thread);
    
    if (lwattr.THREADATTR_stStackByteSize == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    API_ThreadGetSchedParam(thread, &ucPolicy, LW_NULL);
    
    pattr->PTHREADATTR_pcName          = LW_NULL;
    pattr->PTHREADATTR_pvStackAddr     = (PVOID)lwattr.THREADATTR_pstkLowAddr;
    pattr->PTHREADATTR_stStackGuard    = lwattr.THREADATTR_stGuardSize;
    pattr->PTHREADATTR_stStackByteSize = lwattr.THREADATTR_stStackByteSize;
    pattr->PTHREADATTR_iSchedPolicy    = (INT)ucPolicy;
    pattr->PTHREADATTR_iInherit        = PTHREAD_EXPLICIT_SCHED;        /*  Ŀǰ����ȷ��                */
    pattr->PTHREADATTR_ulOption        = lwattr.THREADATTR_ulOption;
    pattr->PTHREADATTR_schedparam.sched_priority = PX_PRIORITY_CONVERT(lwattr.THREADATTR_ucPriority);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_getattr_np
** ��������: ��ȡ�߳����Կ��ƿ� (Linux ��չ�ӿ�)
** �䡡��  : thread        �߳� ID
**           pattr         ��Ҫ���õ� attr ָ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_getattr_np (pthread_t thread, pthread_attr_t *pattr)
{
    return  (pthread_attr_get_np(thread, pattr));
}
/*********************************************************************************************************
** ��������: pthread_attr_getstackfilled
** ��������: ����߳����Կ�ջ�������
** �䡡��  : pattr         �߳�����
**           stackfilled   ��ջ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_attr_getstackfilled (const pthread_attr_t *pattr, int *stackfilled)
{
    if (!pattr || !stackfilled) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_STK_CLR) {
        *stackfilled = PTHREAD_STACK_FILLED;
    
    } else {
        *stackfilled = PTHREAD_NO_STACK_FILLED;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setstackfilled
** ��������: �����߳����Կ�ջ�������
** �䡡��  : pattr         �߳�����
**           stackfilled   ��ջ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstackfilled (pthread_attr_t *pattr, int stackfilled)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (stackfilled == PTHREAD_NO_STACK_FILLED) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_STK_CHK;
    
    } else if (stackfilled == PTHREAD_STACK_FILLED) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_STK_CHK;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getbreakallowed
** ��������: ����߳����Կ��Ƿ�����ϵ�
** �䡡��  : pattr         �߳�����
**           breakallowed  �Ƿ�����ϵ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  pthread_attr_getbreakallowed (const pthread_attr_t *pattr, int *breakallowed)
{
    if (!pattr || !breakallowed) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *breakallowed = PTHREAD_BREAK_ALLOWED;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setbreakallowed
** ��������: �����߳����Կ��Ƿ�����ϵ�
** �䡡��  : pattr         �߳�����
**           breakallowed  �Ƿ�����ϵ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  pthread_attr_setbreakallowed (pthread_attr_t *pattr, int breakallowed)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (breakallowed == PTHREAD_BREAK_DISALLOWED) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_getfpallowed
** ��������: ����߳����Կ��Ƿ�������
** �䡡��  : pattr         �߳�����
**           fpallowed     �Ƿ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  pthread_attr_getfpallowed (const pthread_attr_t *pattr, int *fpallowed)
{
    if (!pattr || !fpallowed) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *fpallowed = PTHREAD_FP_ALLOWED;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_attr_setfpallowed
** ��������: �����߳����Կ��Ƿ�������
** �䡡��  : pattr         �߳�����
**           fpallowed     �Ƿ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  pthread_attr_setfpallowed (pthread_attr_t *pattr, int fpallowed)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (fpallowed == PTHREAD_FP_DISALLOWED) {
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
