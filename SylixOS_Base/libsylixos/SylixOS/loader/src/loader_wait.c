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
** ��   ��   ��: loader_wait.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2012 �� 08 �� 24 ��
**
** ��        ��: posix ���̼��� api.

** BUG:
2012.12.06  �� getpid ����.
2012.12.17  waitpid ����ǵȴ�ָ�����ӽ���, ������Ӧ�õ�ǰ�����Ƿ����ָ�����ӽ���.
2013.01.10  ���� waitid ����.
2013.06.07  ���� detach ����, ���ڽ���ӽ����븸���̵Ĺ�ϵ.
2014.05.04  ���� daemon ����.
2014.11.04  ���� reclaimchild ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "paths.h"
#include "sys/wait.h"
#include "sys/resource.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
/*********************************************************************************************************
** ��������: fork
** ��������: posix fork
** �䡡��  : NONE
** �䡡��  : ���̺�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t fork (void)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: getpid
** ��������: ��õ�ǰ���̺� (�˺������ü���, ��Ϊ�����������߳��˳����ɾ�����̿��ƿ�)
** �䡡��  : NONE
** �䡡��  : ���̺�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t getpid (void)
{
    LW_LD_VPROC    *pvproc;
    pid_t           pid = 0;
    
    pvproc = __LW_VP_GET_CUR_PROC();
    if (pvproc) {
        pid = pvproc->VP_pid;
    }
    
    return  (pid);
}
/*********************************************************************************************************
** ��������: setpgid
** ��������: set process group ID for job control
** �䡡��  : pid       ���̺�
**           pgid      �������
** �䡡��  : 
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int setpgid (pid_t pid, pid_t pgid)
{
    if (pid == 0) {
        pid = getpid();
    }
    
    if (pgid == 0) {
        pgid = getpid();
    }

    return  (vprocSetGroup(pid, pgid));
}
/*********************************************************************************************************
** ��������: getpgid
** ��������: get the process group ID for a process
** �䡡��  : pid       ���̺�
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t getpgid (pid_t pid)
{
    if (pid == 0) {
        pid = getpid();
    }

    return  (vprocGetGroup(pid));
}
/*********************************************************************************************************
** ��������: setpgrp
** ��������: set the process group ID of the calling process
** �䡡��  : NONE
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t setpgrp (void)
{
    if (setpgid(0, 0) == 0) {
        return  (getpgid(0));
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: getpgrp
** ��������: get the process group ID of the calling process
** �䡡��  : NONE
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t getpgrp (void)
{
    return  getpgid(0);
}
/*********************************************************************************************************
** ��������: getpgrp
** ��������: get the parent process ID
** �䡡��  : NONE
** �䡡��  : ���̺�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t getppid (void)
{
    return  (vprocGetFather(getpid()));
}
/*********************************************************************************************************
** ��������: setsid
** ��������: create session and set process group ID
** �䡡��  : NONE
** �䡡��  : ���̺�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t setsid (void)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: issetugid
** ��������: ����������ʱʹ���� setuid �� setgid λ�Ļ����ͷ����档
** �䡡��  : 
** �䡡��  : �Ƿ������� setuid �� setgid λ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int issetugid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_LD_VPROC    *pvproc;
    int             ret = 0;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
    if (pvproc) {
        ret = (int)pvproc->VP_bIssetugid;
    }
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: __haveThisChild
** ��������: ��鵱ǰ�����Ƿ����һ��ָ�� pid �ŵ��ӽ���
** �䡡��  : pvproc        ��ǰ����
**           pidChild      �ӽ���
** �䡡��  : �л���û��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL __haveThisChild (LW_LD_VPROC  *pvproc, pid_t  pidChild)
{
    PLW_LIST_LINE   plineList;
    LW_LD_VPROC    *pvprocChild;
    BOOL            bRet = LW_FALSE;

    LW_LD_LOCK();
    for (plineList  = pvproc->VP_plineChild;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
        pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        if (pvprocChild->VP_pid == pidChild) {
            bRet = LW_TRUE;
            break;
        }
    }
    LW_LD_UNLOCK();
    
    return  (bRet);
}
/*********************************************************************************************************
** ��������: __reclaimAChild
** ��������: ����һ���ӽ���
** �䡡��  : pvproc        ��ǰ����
**           pidChild      ��Ҫ���յ��ӽ���
**           pid           �ӽ��̺�
**           stat_loc      ����ֵ
**           option        ѡ��
**           prusage       ������Ϣ
** �䡡��  : ���ս��̵����� 1: ����һ�� 0: û�л��� -1: û���ӽ���
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� pidChild ����������:
             pidChild >  0      �ȴ�ָ�� PID ���ӽ���
             pidChild == 0      �ӽ������� ID ��ͬ��
             pidChild == -1     �κ��ӽ���
             pidChild <  -1     �ӽ������� ID Ϊ pid ����ֵ��

             �˺���Ϊ�ź���Ӧ����, ����֮ǰ��ռ���й���Դ��, �������������ӽ�����Դ, �����������,
             ���������ڻ�ȡ�����̷�����Ϣ��, ֪ͨ�ں˻����߳�ȥ������Դ.
*********************************************************************************************************/
static INT __reclaimAChild (LW_LD_VPROC   *pvproc, 
                            pid_t          pidChild, 
                            pid_t         *pid, 
                            int           *stat_loc, 
                            int            option,
                            struct rusage *prusage)
{
    PLW_LIST_LINE   plineList;
    LW_LD_VPROC    *pvprocChild;
    BOOL            bHasEvent    = LW_FALSE;
    BOOL            bNeedReclaim = LW_FALSE;
    pid_t           gpid         = 0;
    
    if (pidChild < -1) {
        gpid = 0 - pidChild;                                            /*  ����ֵ                      */
    }
    
    LW_LD_LOCK();
    if (pvproc->VP_plineChild == LW_NULL) {                             /*  û���ӽ���                  */
        LW_LD_UNLOCK();
        errno = ECHILD;
        return  (PX_ERROR);
    }
    plineList = pvproc->VP_plineChild;
    while (plineList) {                                                 /*  �����ӽ���                  */
        pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        plineList = _list_line_get_next(plineList);
        
        if (pidChild > 0) {                                             /*  �ȴ�ָ�� PID ���ӽ���       */
            if (pidChild != pvprocChild->VP_pid) {
                continue;
            }
        
        } else if (pidChild == 0) {                                     /*  �ӽ������� ID ��ͬ��        */
            if (pvproc->VP_pidGroup != pvprocChild->VP_pidGroup) {
                continue;
            }
            
        } else if (pidChild < -1) {                                     /*  ָ�����ӽ�����              */
            if (gpid != pvprocChild->VP_pidGroup) {
                continue;
            }
        }                                                               /*  -1 ��ʾ�κ��ӽ���           */
        
        /*
         *  ��������������ӽ����Ƿ���ָ����״̬
         */
        if (pvprocChild->VP_iStatus == __LW_VP_EXIT) {                  /*  �ӽ����Ѿ�����              */
            if (pid) {
                *pid = pvprocChild->VP_pid;
            }
            if (stat_loc) {
                *stat_loc = SET_EXITSTATUS(pvprocChild->VP_iExitCode);  /*  �ӽ��������˳�              */
            }
            pvprocChild->VP_pvprocFather = LW_NULL;
            _List_Line_Del(&pvprocChild->VP_lineBrother,
                           &pvproc->VP_plineChild);                     /*  ���ӽ��̴��������˳�        */
            bHasEvent    = LW_TRUE;
            bNeedReclaim = LW_TRUE;
            break;
        
        } else if (pvprocChild->VP_iStatus == __LW_VP_STOP) {           /*  �ӽ���ֹͣ                  */
            if (option & WUNTRACED) {
                if (pid) {
                    *pid = 0;                                           /*  ����ֵΪ 0                  */
                }
                if (stat_loc) {
                    *stat_loc = SET_STOPSIG(pvprocChild->VP_iSigCode);
                }
                bHasEvent = LW_TRUE;
                break;
            }
        
        } else if (pvprocChild->VP_ulFeatrues & __LW_VP_FT_DAEMON) {    /*  �ӽ��̽��� deamon ״̬      */
            if (pid) {
                *pid = pvprocChild->VP_pid;
            }
            if (stat_loc) {
                *stat_loc = 0;                                          /*  �ӽ��������˳�              */
            }
            bHasEvent = LW_TRUE;
            break;
        }
    }
    LW_LD_UNLOCK();
    
    if (bHasEvent) {
        if (prusage) {
            __tickToTimeval(pvprocChild->VP_clockUser,   &prusage->ru_utime);
            __tickToTimeval(pvprocChild->VP_clockSystem, &prusage->ru_stime);
        }
        if (bNeedReclaim) {
            __resReclaimReq((PVOID)pvprocChild);                        /*  �����ں��ͷŽ�����Դ        */
        }
        return  (1);
    
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: wait
** ��������: wait for a child process to stop or terminate
** �䡡��  : stat_loc
** �䡡��  : child pid
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t wait (int *stat_loc)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    do {
        iError = __reclaimAChild(pvproc, -1, &pidChld, 
                                 stat_loc, 0, LW_NULL);                 /*  ��ͼ����һ���ӽ���          */
        if (iError < 0) {
            return  (iError);                                           /*  û�����߳�                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  ���߳��Ѿ�������            */
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  �ȴ��ӽ����ź�              */
        if (iError < 0) {
            return  (iError);                                           /*  �������źŴ��              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: waitid
** ��������: wait for a child process to change state
** �䡡��  : idtype        ���� (P_PID  P_PGID  P_ALL)
**           id            P_PID: will wait for the child with a process ID equal to (pid_t)id.
**                         P_PGID: will wait for any child with a process group ID equal to (pid_t)id.
**                         P_ALL: waitid() will wait for any children and id is ignored.
**           options       ѡ��
** �䡡��  : If waitid() returns due to the change of state of one of its children, 0 is returned. 
**           Otherwise, -1 is returned and errno is set to indicate the error.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int  waitid (idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
    INT               iError;
    INT               iStat;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    pid_t             pid;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  (PX_ERROR);
    }
    
    if (infop == LW_NULL) {
        infop =  &siginfoChld;
    }
    
    switch (idtype) {
    
    case P_PID:
        pid = (pid_t)id;
        break;
        
    case P_PGID:
        pid = (pid_t)(-id);
        if (pid == -1) {
            errno = EINVAL;
            return  (PX_ERROR);
        }
        break;
        
    case P_ALL:
        pid = (pid_t)(-1);
        break;
        
    default:
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    if (pid > 0) {                                                      /*  �ȴ�ָ�����ӽ���            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  (PX_ERROR);                                         /*  ��ǰ���̲�����ָ�����ӽ���  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 &iStat, options, LW_NULL);             /*  ��ͼ����һ���ӽ���          */
        if (iError < 0) {
            return  (PX_ERROR);                                         /*  û�����߳�                  */
        
        } else if (iError > 0) {
            return  (ERROR_NONE);                                       /*  ���߳��Ѿ�������            */
        }
        
        if (options & WNOHANG) {                                        /*  ���ȴ�                      */
            return  (ERROR_NONE);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, infop);                    /*  �ȴ��ӽ����ź�              */
        if (iError < 0) {
            return  (iError);                                           /*  �������źŴ��              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: waitpid
** ��������: wait for a child process to stop or terminate
** �䡡��  : pid           ָ�������߳�
**           stat_loc      ����ֵ
**           options       ѡ��
** �䡡��  : child pid 
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t waitpid (pid_t pid, int *stat_loc, int options)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    if (pid > 0) {                                                      /*  �ȴ�ָ�����ӽ���            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  ((pid_t)-1);                                        /*  ��ǰ���̲�����ָ�����ӽ���  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 stat_loc, options, LW_NULL);           /*  ��ͼ����һ���ӽ���          */
        if (iError < 0) {
            return  (iError);                                           /*  û�����߳�                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  ���߳��Ѿ�������            */
        }
        
        if (options & WNOHANG) {                                        /*  ���ȴ�                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  �ȴ��ӽ����ź�              */
        if (iError < 0) {
            return  (iError);                                           /*  �������źŴ��              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: wait3
** ��������: wait for a child process to stop or terminate
** �䡡��  : stat_loc      ����ֵ
**           options       ѡ��
**           prusage       �����ս�����Դʹ�����
** �䡡��  : child pid 
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t wait3 (int *stat_loc, int options, struct rusage *prusage)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    do {
        iError = __reclaimAChild(pvproc, -1, &pidChld, 
                                 stat_loc, options, prusage);           /*  ��ͼ����һ���ӽ���          */
        if (iError < 0) {
            return  (iError);                                           /*  û�����߳�                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  ���߳��Ѿ�������            */
        }
        
        if (options & WNOHANG) {                                        /*  ���ȴ�                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  �ȴ��ӽ����ź�              */
        if (iError < 0) {
            return  (iError);                                           /*  �������źŴ��              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: wait4
** ��������: wait for a child process to stop or terminate
** �䡡��  : pid           ָ�������߳�
**           stat_loc      ����ֵ
**           options       ѡ��
**           prusage       �����ս�����Դʹ�����
** �䡡��  : child pid 
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t wait4 (pid_t pid, int *stat_loc, int options, struct rusage *prusage)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    if (pid > 0) {                                                      /*  �ȴ�ָ�����ӽ���            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  ((pid_t)-1);                                        /*  ��ǰ���̲�����ָ�����ӽ���  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 stat_loc, options, prusage);           /*  ��ͼ����һ���ӽ���          */
        if (iError < 0) {
            return  (iError);                                           /*  û�����߳�                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  ���߳��Ѿ�������            */
        }
        
        if (options & WNOHANG) {                                        /*  ���ȴ�                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  �ȴ��ӽ����ź�              */
        if (iError < 0) {
            return  (iError);                                           /*  �������źŴ��              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: reclaimchild
** ��������: reclaim child process
** �䡡��  : pid     child process
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
void reclaimchild (pid_t pid)
{
    INT             iError;
    pid_t           pidChld;
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        return;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld,
                                 LW_NULL, 0, LW_NULL);                  /*  ��ͼ����һ���ӽ���          */
        if (iError <= 0) {
            break;
        }
    } while (1);
}
/*********************************************************************************************************
** ��������: ischild
** ��������: �Ƿ�Ϊ��ǰ�����ӽ���
** �䡡��  : pid           ���� ID
** �䡡��  : LW_TRUE: �ǵ�ǰ�����ӽ���, LW_FALSE ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
BOOL ischild (pid_t pid)
{
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();

    if (pvproc == LW_NULL) {
        return  (LW_FALSE);
    }

    return  (__haveThisChild(pvproc, pid));
}
/*********************************************************************************************************
** ��������: isbrother
** ��������: �Ƿ�Ϊ��ǰ�����ֵܽ���
** �䡡��  : pid           ���� ID
** �䡡��  : LW_TRUE: �ǵ�ǰ�����ֵܽ���, LW_FALSE ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
BOOL isbrother (pid_t pid)
{
    BOOL            bRet = LW_FALSE;
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();
    LW_LD_VPROC    *pvprocChild;
    LW_LD_VPROC    *pvprocFather;
    PLW_LIST_LINE   plineList;

    if (pvproc == LW_NULL) {
        return  (LW_FALSE);
    }

    LW_LD_LOCK();
    pvprocFather = pvproc->VP_pvprocFather;
    if (!pvprocFather) {
        LW_LD_UNLOCK();
        return  (LW_FALSE);
    }

    for (plineList  = pvprocFather->VP_plineChild;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {

        pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        if (pvprocChild->VP_pid == pid) {
            bRet = LW_TRUE;
            break;
        }
    }
    LW_LD_UNLOCK();

    return  (bRet);
}
/*********************************************************************************************************
** ��������: detach
** ��������: �����븸���̹�ϵ
** �䡡��  : pid           ָ�������߳�
** �䡡��  : ���
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int detach (pid_t pid)
{
    return  (vprocDetach(pid));
}
/*********************************************************************************************************
** ��������: daemon
** ��������: ������ת�����ػ�����
** �䡡��  : nochdir       0 chdir root
**           noclose       0 change stdin stdout stderr to /dev/null
** �䡡��  : 0: �ɹ�  -1: ʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int daemon (int nochdir, int noclose)
{
    INT                 iFd;
    pid_t               pid = getpid();
    LW_OBJECT_HANDLE    ulId;
    
    if (pid <= 0) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    if (!nochdir) {
        chdir("/");
    }
    
    if (!noclose) {
        iFd = open(_PATH_DEVNULL, O_RDWR);
        if (iFd < 0) {
            return  (PX_ERROR);
        }
        dup2(iFd, STD_IN);
        dup2(iFd, STD_OUT);
        dup2(iFd, STD_ERR);
        close(iFd);
    }
    
    detach(pid);                                                        /*  �Ӹ���������                */
    setsid();
    
    ulId = vprocMainThread(pid);
    if (ulId != LW_OBJECT_HANDLE_INVALID) {
        API_ThreadDetach(ulId);                                         /*  ���߳� detach               */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: getpid
** ��������: ��õ�ǰ���̺�
** �䡡��  : 
** �䡡��  : ���̺�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#else                                                                   /*  NO MODULELOADER             */

LW_API
pid_t getpid (void)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
