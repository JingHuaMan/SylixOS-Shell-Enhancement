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
** ��   ��   ��: resource.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 09 ��
**
** ��        ��: posix resource ���ݿ�.

** BUG:
2013.06.13  ʵ�� setpriority getpriority �� nice. 
            setpriority �� getpriority ���ȼ����������� PRIO_MIN PRIO_MAX ֮��. ��ֵԽС, ���ȼ�Խ��.
2014.07.04  ���� getpriority �� setpriority �����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_resource.h"                                     /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/px_times.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: getrusage
** ��������: shall provide measures of the resources used by the current process or its terminated 
             and waited-for child processes.
** �䡡��  : who           RUSAGE_SELF or RUSAGE_CHILDREN
** �䡡��  : ������ȼ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  getrusage (int  who, struct rusage *r_usage)
{
    struct tms tmsBuf;

    if (!r_usage) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_bzero(r_usage, sizeof(struct rusage));
    
    times(&tmsBuf);
    
    if (who == RUSAGE_SELF) {
        __tickToTimeval(tmsBuf.tms_utime, &r_usage->ru_utime);
        __tickToTimeval(tmsBuf.tms_stime, &r_usage->ru_stime);
    
    } else if (who == RUSAGE_CHILDREN) {
        __tickToTimeval(tmsBuf.tms_cutime, &r_usage->ru_utime);
        __tickToTimeval(tmsBuf.tms_cstime, &r_usage->ru_stime);
    
    } else {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: getrlimit
** ��������: get maximum resource consumption
** �䡡��  : resource      resource type
**           rlp           resource limite
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  getrlimit (int resource, struct rlimit *rlp)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: setrlimit
** ��������: set maximum resource consumption
** �䡡��  : resource      resource type
**           rlp           resource limite
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  setrlimit (int resource, const struct rlimit *rlp)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __gprio_hook
** ��������: set or set the nice value
** �䡡��  : ret           return value
**           which         PRIO_PROCESS / PRIO_PGRP / PRIO_USER
**           who           process ID / process group ID / effective user ID
                           who == 0 specifies the current process, process group or user.
**           pucPriority   return sylixos highest priority (lowest priority)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static void  __gprio_hook (PLW_CLASS_TCB  ptcb, int  *ret, int which, id_t  who, UINT8  *pucPriority)
{
    LW_LD_VPROC     *pvproc = __LW_VP_GET_TCB_PROC(ptcb);
    
    if (pvproc) {
        switch (which) {
        
        case PRIO_PROCESS:
            if (pvproc->VP_pid == (pid_t)who) {
                if (LW_PRIO_IS_HIGH(*pucPriority, ptcb->TCB_ucPriority)) {
                    *pucPriority = ptcb->TCB_ucPriority;
                }
                *ret = ERROR_NONE;
            }
            break;
            
        case PRIO_PGRP:
            if (pvproc->VP_pidGroup == (pid_t)who) {
                if (LW_PRIO_IS_HIGH(*pucPriority, ptcb->TCB_ucPriority)) {
                    *pucPriority = ptcb->TCB_ucPriority;
                }
                *ret = ERROR_NONE;
            }
            break;
            
        case PRIO_USER:
            if (ptcb->TCB_euid == (uid_t)who) {
                if (LW_PRIO_IS_HIGH(*pucPriority, ptcb->TCB_ucPriority)) {
                    *pucPriority = ptcb->TCB_ucPriority;
                }
                *ret = ERROR_NONE;
            }
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: __sprio_hook
** ��������: set or set the nice value
** �䡡��  : ret           return value
**           which         PRIO_PROCESS / PRIO_PGRP / PRIO_USER
**           who           process ID / process group ID / effective user ID
**           pucPriority   sylixos priority
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __sprio_hook (PLW_CLASS_TCB  ptcb, int  *ret, int which, id_t  who, UINT8  *pucPriority)
{
    LW_LD_VPROC     *pvproc = __LW_VP_GET_TCB_PROC(ptcb);
    
    if (pvproc) {
        switch (which) {
        
        case PRIO_PROCESS:
            if (pvproc->VP_pid == (pid_t)who) {
                if (!LW_PRIO_IS_EQU(*pucPriority, ptcb->TCB_ucPriority)) {
                    _SchedSetPrio(ptcb, *pucPriority);
                }
                *ret = ERROR_NONE;
            }
            break;
            
        case PRIO_PGRP:
            if (pvproc->VP_pidGroup == (pid_t)who) {
                if (!LW_PRIO_IS_EQU(*pucPriority, ptcb->TCB_ucPriority)) {
                    _SchedSetPrio(ptcb, *pucPriority);
                }
                *ret = ERROR_NONE;
            }
            break;
            
        case PRIO_USER:
            if (ptcb->TCB_euid == (uid_t)who) {
                if (!LW_PRIO_IS_EQU(*pucPriority, ptcb->TCB_ucPriority)) {
                    _SchedSetPrio(ptcb, *pucPriority);
                }
                *ret = ERROR_NONE;
            }
            break;
        }
    }
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: getpriority
** ��������: get or set the nice value
** �䡡��  : which         PRIO_PROCESS / PRIO_PGRP / PRIO_USER
**           who           process ID / process group ID / effective user ID
                           who == 0 specifies the current process, process group or user.
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  getpriority (int which, id_t who)
{
#if LW_CFG_MODULELOADER_EN > 0
    INT    iRet       = PX_ERROR;
    UINT8  ucPriority = PRIO_MIN;
    
    if (which == PRIO_PROCESS) {
        if (who == 0) {
            who =  (id_t)getpid();
        }
    } else if (which == PRIO_PGRP) {
        if (who == 0) {
            who =  (id_t)getpgid(getpid());
        }
    } else if (PRIO_USER) {
        if (who == 0) {
            who =  (id_t)geteuid();
        }
    } else {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();
    _ThreadTraversal(__gprio_hook, 
                     (PVOID)&iRet, (PVOID)(LONG)which, (PVOID)(LONG)who, (PVOID)&ucPriority,
                     LW_NULL, LW_NULL);
    __KERNEL_EXIT();
    
    if (iRet == ERROR_NONE) {
        return  ((int)ucPriority);
    
    } else {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: setpriority
** ��������: set or set the nice value
** �䡡��  : which         PRIO_PROCESS / PRIO_PGRP / PRIO_USER
**           who           process ID / process group ID / effective user ID
                           who == 0 specifies the current process, process group or user.
**           value         priority (PRIO_MIN ~ PRIO_MAX) 
                           PRIO_MIN -> highest priorioty
                           PRIO_MAX -> lowest priorioty
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  setpriority (int which, id_t who, int value)
{
#if LW_CFG_MODULELOADER_EN > 0
    INT    iRet = PX_ERROR;
    UINT8  ucPriority;
    
    if ((value < PRIO_MIN) ||
        (value > PRIO_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (which == PRIO_PROCESS) {
        if (who == 0) {
            who =  (id_t)getpid();
        }
    } else if (which == PRIO_PGRP) {
        if (who == 0) {
            who =  (id_t)getpgid(getpid());
        }
    } else if (PRIO_USER) {
        if (who == 0) {
            who =  (id_t)geteuid();
        }
    } else {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ucPriority = (UINT8)value;
    
    __KERNEL_ENTER();
    _ThreadTraversal(__sprio_hook, 
                     (PVOID)&iRet, (PVOID)(LONG)which, (PVOID)(LONG)who, (PVOID)&ucPriority,
                     LW_NULL, LW_NULL);
    __KERNEL_EXIT();

    if (iRet == ERROR_NONE) {
        return  (ERROR_NONE);
    
    } else {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: nice
** ��������: set the nice value
** �䡡��  : incr          add the value of incr to the nice value of the calling process
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int nice (int incr)
{
#if LW_CFG_MODULELOADER_EN > 0
    int  oldpro = getpriority(PRIO_PROCESS, getpid());
    return  (setpriority(PRIO_PROCESS, getpid(), oldpro + incr));
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
