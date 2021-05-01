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
** ��   ��   ��: sched.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix ���ȼ��ݿ�.

** BUG:
2011.03.26  Higher numerical values for the priority represent higher priorities.
2014.07.04  ���Ȳ�����һ��Ϊ���� ID.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_sched.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: sched_get_priority_max
** ��������: ��õ����������������ȼ� (pthread �̲߳����� idle ͬ���ȼ�!)
** �䡡��  : iPolicy       ���Ȳ��� (Ŀǰ����)
** �䡡��  : ������ȼ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_get_priority_max (int  iPolicy)
{
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    return  (__PX_PRIORITY_MAX);                                        /*  254                         */
}
/*********************************************************************************************************
** ��������: sched_get_priority_min
** ��������: ��õ������������С���ȼ�
** �䡡��  : iPolicy       ���Ȳ��� (Ŀǰ����)
** �䡡��  : ��С���ȼ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_get_priority_min (int  iPolicy)
{
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (__PX_PRIORITY_MIN);                                        /*  1                           */
}
/*********************************************************************************************************
** ��������: sched_yield
** ��������: ����ǰ������뵽ͬ���ȼ���������������, �����ó�һ�� CPU ����.
** �䡡��  : NONE
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_yield (void)
{
    API_ThreadYield(API_ThreadIdSelf());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sched_set
** ��������: ����ָ��������������� (�����ں˺󱻵���)
** �䡡��  : ptcb               ������ƿ�
**           pucPolicy          ���Ȳ���
**           pucPriority        ���ȼ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __sched_set (PLW_CLASS_TCB  ptcb, UINT8  *pucPolicy, UINT8  *pucPriority)
{
    if (LW_LIKELY(ptcb)) {
        if (pucPriority) {
            if (!LW_PRIO_IS_EQU(ptcb->TCB_ucPriority, *pucPriority)) {
                _SchedSetPrio(ptcb, *pucPriority);
            }
        }

        if (pucPolicy) {
            ptcb->TCB_ucSchedPolicy = *pucPolicy;
            if (*pucPolicy == LW_OPTION_SCHED_FIFO) {
                ptcb->TCB_usSchedCounter = ptcb->TCB_usSchedSlice;      /*  ��Ϊ��                      */
            }
        }
    }
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: sched_setparam
** ��������: ����ָ���������������
** �䡡��  : pid           ���� / �߳� ID
**           pschedparam   ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_setparam (pid_t  pid, const struct sched_param  *pschedparam)
{
    UINT8  ucPriority;
    
    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid = getpid();
    }
    if (pid == 0) {
        errno = ESRCH;
        return  (PX_ERROR);
    }

    if (vprocThreadTraversal(pid, __sched_set, LW_NULL, &ucPriority,
                             LW_NULL, LW_NULL, LW_NULL, LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    LW_OBJECT_HANDLE  ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);

    if (API_ThreadSetPriority(ulThread, ucPriority)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_getparam
** ��������: ���ָ���������������
** �䡡��  : pid           ���� / �߳� ID
**           pschedparam   ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_getparam (pid_t  pid, struct sched_param  *pschedparam)
{
    UINT8               ucPriority;
    ULONG               ulError;
    LW_OBJECT_HANDLE    ulThread;
    
    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    ulError = API_ThreadGetPriority(ulThread, &ucPriority);
    if (ulError) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        pschedparam->sched_priority = PX_PRIORITY_CONVERT(ucPriority);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: sched_setscheduler
** ��������: ����ָ�����������
** �䡡��  : pid           ���� / �߳� ID
**           iPolicy       ���Ȳ���
**           pschedparam   ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_setscheduler (pid_t                      pid, 
                         int                        iPolicy, 
                         const struct sched_param  *pschedparam)
{
    UINT8  ucPriority;

    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid = getpid();
    }
    if (pid == 0) {
        errno = ESRCH;
        return  (PX_ERROR);
    }

    if (vprocThreadTraversal(pid, __sched_set, &iPolicy, &ucPriority,
                             LW_NULL, LW_NULL, LW_NULL, LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    UINT8             ucActivatedMode;
    LW_OBJECT_HANDLE  ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);

    if (API_ThreadGetSchedParam(ulThread,
                                LW_NULL,
                                &ucActivatedMode)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }

    API_ThreadSetSchedParam(ulThread, (UINT8)iPolicy, ucActivatedMode);

    if (API_ThreadSetPriority(ulThread, ucPriority)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_getscheduler
** ��������: ���ָ�����������
** �䡡��  : pid           ���� / �߳� ID
** �䡡��  : ���Ȳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_getscheduler (pid_t  pid)
{
    UINT8               ucPolicy;
    LW_OBJECT_HANDLE    ulThread;
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (API_ThreadGetSchedParam(ulThread,
                                &ucPolicy,
                                LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        return  ((int)ucPolicy);
    }
}
/*********************************************************************************************************
** ��������: sched_rr_get_interval
** ��������: ���ָ�����������
** �䡡��  : pid           ���� / �߳� ID
**           interval      current execution time limit.
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_rr_get_interval (pid_t  pid, struct timespec  *interval)
{
    UINT8               ucPolicy;
    UINT16              usCounter = 0;
    LW_OBJECT_HANDLE    ulThread;

    if (!interval) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (API_ThreadGetSchedParam(ulThread,
                                &ucPolicy,
                                LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    if (ucPolicy != LW_OPTION_SCHED_RR) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ThreadGetSliceEx(ulThread, LW_NULL, &usCounter)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    
    __tickToTimespec((ULONG)usCounter, interval);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_setaffinity
** ��������: ���ý��̵��ȵ� CPU ����
** �䡡��  : pid           ���� / �߳� ID
**           setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_setaffinity (pid_t pid, size_t setsize, const cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    if (vprocSetAffinity(pid, setsize, (PLW_CLASS_CPUSET)set)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    errno = ESRCH;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_getaffinity
** ��������: ��ȡ���̵��ȵ� CPU ����
** �䡡��  : pid           ���� / �߳� ID
**           setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_getaffinity (pid_t pid, size_t setsize, cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if ((setsize < sizeof(cpu_set_t)) || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    if (vprocGetAffinity(pid, setsize, set)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    errno = ESRCH;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_settimeslice
** ��������: ����ϵͳ����ʱ��Ƭ
** �䡡��  : ticks         ʱ��Ƭ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API  
int  sched_settimeslice (UINT32  ticks)
{
    INT             i;
    PLW_CLASS_TCB   ptcb;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    if (ticks > 100) {
        errno = ERANGE;
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  ��� TCB ���ƿ�             */
        if (ptcb == LW_NULL) {                                          /*  �̲߳�����                  */
            continue;
        }
        
        ptcb->TCB_usSchedSlice = (UINT16)ticks;
    }
    
    LW_SCHED_SLICE = (UINT16)ticks;
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_gettimeslice
** ��������: ���ϵͳ����ʱ��Ƭ
** �䡡��  : NONE
** �䡡��  : ʱ��Ƭ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
unsigned int  sched_gettimeslice (void)
{
    unsigned int  ticks;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    ticks = (unsigned int)LW_SCHED_SLICE;
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ticks);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
