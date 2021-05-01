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
** ��   ��   ��: loader_exec.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 26 ��
**
** ��        ��: unistd exec family of functions.
**
** ע        ��: sylixos ���л���������Ϊȫ������, ���Ե�ʹ�� envp ʱ, ����ϵͳ�Ļ�������������֮�ı�.

** BUG:
2012.03.30  ���� spawn ϵ�к����ӿ�.
2012.08.24  spawn �ӿ�, P_WAIT ʱ�����½��̵ķ���ֵ, ������Ƿ��ؽ��� id.
2012.08.25  �����������̶Բ����Ĳ�������.
2012.12.07  __processShell() P_OVERLAY ģʽ�������˳�.
2012.12.10  ��ʼ֧�ֽ����ڻ�������, �����������.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2013.06.07  ���̴����������빤��Ŀ¼ѡ��.
2013.09.21  exec �ڵ�ǰ�����������������µ��ļ������л����߳�.
2015.07.24  ���� spawnle execle �����Ի����������������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_SPAWN
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_exec.h"
#include "process.h"
#include "sys/wait.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT  __ldGetFilePath(CPCHAR  pcParam, PCHAR  pcPathBuffer, size_t  stMaxLen);
/*********************************************************************************************************
** ��������: __spawnArgFree
** ��������: free a spawn args (����ʹ�� lib_strdup ����, ��������ʹ�� lib_free �ͷ�)
** �䡡��  : psarg
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
VOID  __spawnArgFree (__PSPAWN_ARG  psarg)
{
    INT             i;
    PLW_LIST_LINE   plineTemp;
    __spawn_action *pspawnactFree;
    
    if (!psarg) {
        return;
    }
    
    plineTemp = psarg->SA_plineActions;
    while (plineTemp) {
        pspawnactFree = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        lib_free(pspawnactFree);
    }
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (psarg->SA_pcParamList[i]) {
            lib_free(psarg->SA_pcParamList[i]);
        }
        if (psarg->SA_pcpcEvn[i]) {
            lib_free(psarg->SA_pcpcEvn[i]);
        }
    }
    
    if (psarg->SA_pcWd) {
        lib_free(psarg->SA_pcWd);
    }
    
    if (psarg->SA_pcPath) {
        lib_free(psarg->SA_pcPath);
    }
    
    lib_free(psarg);
}
/*********************************************************************************************************
** ��������: __spawnArgCreate
** ��������: create a spawn args
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
__PSPAWN_ARG  __spawnArgCreate (VOID)
{
    __PSPAWN_ARG        psarg;

    psarg = (__PSPAWN_ARG)lib_malloc(sizeof(__SPAWN_ARG));
    if (!psarg) {
        return  (LW_NULL);
    }
    lib_bzero(psarg, sizeof(__SPAWN_ARG));
    
    return  (psarg);
}
/*********************************************************************************************************
** ��������: __spawnArgActionDup
** ��������: ����һ�� file action
** �䡡��  : plineAction     action ��ͷ
** �䡡��  : �µ� action ��
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
LW_LIST_LINE_HEADER  __spawnArgActionDup (LW_LIST_LINE_HEADER  plineAction)
{
    PLW_LIST_LINE           plineTemp;
    LW_LIST_LINE_HEADER     plineNew = LW_NULL;
    __spawn_action         *pspawnactOrg;
    __spawn_action         *pspawnactNew;
    
    if (plineAction == LW_NULL) {
        return  (LW_NULL);
    }
    
    plineTemp = plineAction;
    while (plineTemp) {
        pspawnactOrg = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        
        switch (pspawnactOrg->SFA_iType) {
        
        case __FILE_ACTIONS_DO_CLOSE:
        case __FILE_ACTIONS_DO_DUP2:
            pspawnactNew = (__spawn_action *)lib_malloc(sizeof(__spawn_action));
            if (pspawnactNew == LW_NULL) {
                goto    __fail;
            }
            *pspawnactNew = *pspawnactOrg;
            break;
            
        case __FILE_ACTIONS_DO_OPEN:
            pspawnactNew = (__spawn_action *)lib_malloc(sizeof(__spawn_action) + 
                                lib_strlen(pspawnactOrg->SFA_pcPath) + 1);
            if (pspawnactNew == LW_NULL) {
                goto    __fail;
            }
            *pspawnactNew = *pspawnactOrg;
            pspawnactNew->SFA_pcPath = (PCHAR)pspawnactNew + sizeof(__spawn_action);
            lib_strcpy(pspawnactNew->SFA_pcPath, pspawnactOrg->SFA_pcPath);
            break;
            
        default:
            goto    __fail;
        }
        
        _List_Line_Add_Ahead(&pspawnactNew->SFA_lineManage,
                             &plineNew);
    }
    
    return  (plineNew);
    
__fail:
    plineTemp = plineNew;
    while (plineTemp) {
        pspawnactNew = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        lib_free(pspawnactNew);
    }
    
    _ErrorHandle(ENOMEM);
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __spawnArgProc
** ��������: ����һ�����̲���
** �䡡��  : psarg     spawn args
** �䡡��  : INT
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
INT  __spawnArgProc (__PSPAWN_ARG  psarg)
{
    LW_LD_VPROC     *pvproc = psarg->SA_pvproc;
    PLW_LIST_LINE    plineTemp;
    __spawn_action  *pspawnact;
    INT              iFd, iFdNew;
    
    /*
     *  TODO: POSIX_SPAWN_RESETIDS �� POSIX_SPAWN_SETSIGDEF ��ʱ��֧��
     */
    if (psarg->SA_pcWd) {
        chdir(psarg->SA_pcWd);                                          /*  ���ù���Ŀ¼                */
    }
     
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETPGROUP) {       /*  ���� group                  */
        if (psarg->SA_spawnattr.SPA_pidGroup == 0) {
            pvproc->VP_pidGroup = pvproc->VP_pid;
        } else if (psarg->SA_spawnattr.SPA_pidGroup > 0) {
            pvproc->VP_pidGroup = psarg->SA_spawnattr.SPA_pidGroup;
        }
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSIGMASK) {      /*  �����ź�������              */
        sigprocmask(SIG_BLOCK, &psarg->SA_spawnattr.SPA_sigsetMask, LW_NULL);
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDULER) {    /*  ���õ���������              */
        API_ThreadSetSchedParam(API_ThreadIdSelf(), 
                                (UINT8)psarg->SA_spawnattr.SPA_iPolicy,
                                LW_OPTION_RESPOND_AUTO);
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDPARAM) {   /*  ���õ��������ȼ�            */
        UINT8               ucPriority;
        struct sched_param *pschedparam = &psarg->SA_spawnattr.SPA_schedparam;
        if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
            (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
        API_ThreadSetPriority(API_ThreadIdSelf(), ucPriority);
    }
    
    for (plineTemp  = psarg->SA_plineActions;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���� file action            */
        
        pspawnact = (__spawn_action *)plineTemp;
        
        switch (pspawnact->SFA_iType) {
        
        case __FILE_ACTIONS_DO_CLOSE:
            close(pspawnact->SFA_iCloseFd);
            break;
            
        case __FILE_ACTIONS_DO_OPEN:
            iFd = open(pspawnact->SFA_pcPath, pspawnact->SFA_iFlag, pspawnact->SFA_mode);
            if (iFd < 0) {
                return  (PX_ERROR);
            }
            iFdNew = dup2(iFd, pspawnact->SFA_iOpenFd);
            close(iFd);                                                 /*  ��Ҫ�رմ򿪵��ļ�          */
            if (iFdNew != pspawnact->SFA_iOpenFd) {
                return  (PX_ERROR);
            }
            break;
            
        case __FILE_ACTIONS_DO_DUP2:
            iFdNew = dup2(pspawnact->SFA_iDup2Fd, pspawnact->SFA_iDup2NewFd);
            if (iFdNew != pspawnact->SFA_iDup2NewFd) {
                return  (PX_ERROR);
            }
            break;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spawnStopProc
** ��������: ����һ������ stop ����
** �䡡��  : psarg     spawn args
**           pvpstop   stop ���滺��
** �䡡��  : ���صĴ�����
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
static LW_LD_VPROC_STOP *__spawnStopGet (__PSPAWN_ARG  psarg, LW_LD_VPROC_STOP *pvpstop)
{
    posix_spawnopt_t *popt = &psarg->SA_spawnattr.SPA_opt;
    
    if (!__issig(popt->SPO_iSigNo)) {
        return  (LW_NULL);
    }
    
    pvpstop->VPS_iSigNo   = popt->SPO_iSigNo;
    pvpstop->VPS_ulId     = popt->SPO_ulId;
    
    return  (pvpstop);
}
/*********************************************************************************************************
** ��������: __execShell
** ��������: exec shell (ע��, __execShell ������ǽ������߳�, ����װ�سɹ������Ҫͨ�� vprocExit �˳�)
** �䡡��  : pvArg
** �䡡��  : return code
** ȫ�ֱ���:
** ����ģ��: 
** ע  ��  : vprocExitNotDestroy() ���ý�����, ���ܰ�װ�µ� cleanup ����.
*********************************************************************************************************/
static INT  __execShell (PVOID  pvArg)
{
    __PSPAWN_ARG        psarg = (__PSPAWN_ARG)pvArg;
    LW_LD_VPROC_STOP    vpstop;
    LW_LD_VPROC_STOP   *pvpstop;
    INT                 iError;
    INT                 iRet = PX_ERROR;
    
    vprocExitNotDestroy(psarg->SA_pvproc);                              /*  �ȴ����̽�����ǰ����        */
    
    vprocReclaim(psarg->SA_pvproc, LW_FALSE);                           /*  ���ս��̵����ͷŽ��̿��ƿ�  */
    
    API_ThreadCleanupPush(__spawnArgFree, (PVOID)psarg);                /*  ���߳��˳�ʱ�ͷ� pvArg      */
    
    pvpstop = __spawnStopGet(psarg, &vpstop);                           /*  ���ֹͣ����                */
    
    iError = vprocRun(psarg->SA_pvproc, pvpstop,
                      psarg->SA_pcPath, LW_LD_DEFAULT_ENTRY, 
                      &iRet, psarg->SA_iArgs, 
                      (CPCHAR *)psarg->SA_pcParamList,
                      (CPCHAR *)psarg->SA_pcpcEvn);                     /*  ���߳̽���ɽ��������߳�    */
    if (iError) {
        iRet = iError;                                                  /*  ����ʧ��                    */
    }
    
    vprocExit(psarg->SA_pvproc, API_ThreadIdSelf(), iRet);              /*  �����˳�, ���߳̾������߳�  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __processShell
** ��������: �µĽ���ִ�����
** �䡡��  : pvArg
** �䡡��  : return code
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
static INT  __processShell (PVOID  pvArg)
{
    __PSPAWN_ARG        psarg  = (__PSPAWN_ARG)pvArg;
    LW_OBJECT_HANDLE    ulSelf = API_ThreadIdSelf();
    LW_LD_VPROC_STOP    vpstop;
    LW_LD_VPROC_STOP   *pvpstop;
    INT                 iError;
    INT                 iRet = PX_ERROR;
    CHAR                cName[LW_CFG_OBJECT_NAME_SIZE];

    API_ThreadCleanupPush(__spawnArgFree, pvArg);                       /*  ���߳��˳�ʱ�ͷ� pvArg      */
    
    __LW_VP_SET_CUR_PROC(psarg->SA_pvproc);                             /*  ���̼߳�¼���̿��ƿ�        */
                                                                        /*  ��֤����� file actions ����*/
    iError = __spawnArgProc(psarg);                                     /*  ��ʼ�����̲���              */
    if (iError < ERROR_NONE) {
        vprocDestroy(psarg->SA_pvproc);
        return  (iError);
    }
    
    lib_strlcpy(cName, _PathLastNamePtr(psarg->SA_pcPath), LW_CFG_OBJECT_NAME_SIZE);
    
    API_ThreadSetName(ulSelf, cName);                                   /*  ����������                  */
    
    pvpstop = __spawnStopGet(psarg, &vpstop);                           /*  ���ֹͣ����                */
    
    iError = vprocRun(psarg->SA_pvproc, pvpstop, 
                      psarg->SA_pcPath, LW_LD_DEFAULT_ENTRY, 
                      &iRet, psarg->SA_iArgs, 
                      (CPCHAR *)psarg->SA_pcParamList,
                      (CPCHAR *)psarg->SA_pcpcEvn);                     /*  ���߳̽���ɽ��������߳�    */
    if (iError) {
        iRet = iError;                                                  /*  ����ʧ��                    */
    }
    
    vprocExit(psarg->SA_pvproc, ulSelf, iRet);                          /*  �����˳�, ���߳̾������߳�  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __processStart
** ��������: start a process shell (�µļ̳м̳е�ǰ�߳����ȼ��Ͷ�ջ��С)
** �䡡��  : mode          run mode
**           psarg
** �䡡��  : return code
** ȫ�ֱ���:
** ����ģ��: 
** ע  ��  : �����½����߳�Ϊ�ں��߳�, �� vprocRun ʱ, �Զ�תΪ���������߳�.
*********************************************************************************************************/
INT  __processStart (INT  mode, __PSPAWN_ARG  psarg)
{
    LW_CLASS_THREADATTR threadattr;
    LW_OBJECT_HANDLE    hHandle;
    PLW_CLASS_TCB       ptcbCur;
    INT                 iRetValue = ERROR_NONE;
    pid_t               pid;
    ULONG               ulOption;
    size_t              stStackSize;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulOption = LW_OPTION_THREAD_STK_CHK
             | LW_OPTION_OBJECT_GLOBAL
             | LW_OPTION_THREAD_STK_MAIN
             | psarg->SA_spawnattr.SPA_opt.SPO_ulMainOption;
             
    if (psarg->SA_spawnattr.SPA_opt.SPO_stStackSize) {
        stStackSize = psarg->SA_spawnattr.SPA_opt.SPO_stStackSize;
    } else {
        stStackSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
    }
    
    API_ThreadAttrBuild(&threadattr,
                        stStackSize,
                        ptcbCur->TCB_ucPriority,
                        ulOption,
                        (PVOID)psarg);
                        
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDPARAM) {   /*  �������ȼ�                  */
        threadattr.THREADATTR_ucPriority = 
            (UINT8)PX_PRIORITY_CONVERT(psarg->SA_spawnattr.SPA_schedparam.sched_priority);
    }

    if (mode == P_OVERLAY) {                                            /*  �滻��ǰ���̿ռ�            */
        psarg->SA_pvproc = __LW_VP_GET_CUR_PROC();                      /*  ��õ�ǰ���̿��ƿ�          */
        if (psarg->SA_pvproc == LW_NULL) {
            __spawnArgFree(psarg);
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        if (psarg->SA_pvproc->VP_ulMainThread != API_ThreadIdSelf()) {  /*  ���������߳�                */
            __spawnArgFree(psarg);
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        if (API_ThreadRestartEx(psarg->SA_pvproc->VP_ulMainThread,
                                (PTHREAD_START_ROUTINE)__execShell, 
                                (PVOID)psarg)) {                        /*  �������߳�                  */
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        
        return  (PX_ERROR);                                             /*  ���������в�������          */
        
    } else {                                                            /*  ����һ���µĽ���            */
        psarg->SA_pvproc = vprocCreate(psarg->SA_pcPath,
                                       psarg->SA_spawnattr.SPA_ulExts);
        if (psarg->SA_pvproc == LW_NULL) {
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        pid = psarg->SA_pvproc->VP_pid;                                 /*  �ӽ��� id                   */
                            
        hHandle = API_ThreadInit("t_spawn", (PTHREAD_START_ROUTINE)__processShell, 
                                 &threadattr, LW_NULL);
        if (!hHandle) {
            vprocDestroy(psarg->SA_pvproc);
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        
        API_ThreadStart(hHandle);
        
        if (mode == P_WAIT) {
            if (waitpid(pid, &iRetValue, 0) < 0) {                      /*  �ȴ����̽���                */
                return  (PX_ERROR);
            
            } else {
                return  (iRetValue);
            }
            
        } else {
            return  (pid);                                              /*  �ӽ��� id                   */
        }
    }
}
/*********************************************************************************************************
** ��������: spawnl
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv0         args...
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnl (int mode, const char *path, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  ѭ����ȡ����                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
    
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnle
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv0         args...
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnle (int mode, const char *path, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    char * const       *envp;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  ѭ����ȡ����                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
        
        envp = (char * const *)va_arg(valist, char * const *);
        
        for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  ѭ����ȡ��������            */
            if (envp[i]) {
                psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
                if (!psarg->SA_pcpcEvn[i]) {
                    __spawnArgFree(psarg);
                    errno = ENOMEM;
                    return  (PX_ERROR);
                }
            } else {
                break;
            }
        }
        psarg->SA_iEvns = i;
    
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnlp
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv0         args...
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnlp (int mode, const char *file, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  �޷��ҵ�����                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  ѭ����ȡ����                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
        
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnv
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv          args...
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnv (int mode, const char *path, char * const *argv)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnve
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv          args...
**           envp[]        evar array need to set.
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnve (int mode, const char *path, char * const *argv, char * const *envp)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (envp[i]) {
            psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
            if (!psarg->SA_pcpcEvn[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnvp
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv          args...
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnvp (int mode, const char *file, char * const *argv)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  �޷��ҵ�����                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: spawnvpe
** ��������: spawn family of functions
** �䡡��  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv          args...
**           envp[]        evar array need to set.
** �䡡��  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int spawnvpe (int mode, const char *file, char * const *argv, char * const *envp)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  �޷��ҵ�����                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (envp[i]) {
            psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
            if (!psarg->SA_pcpcEvn[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** ��������: execl
** ��������: exec family of functions
** �䡡��  : path          pathname that identifies the new module image file.
**           arg0          args...
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execl (const char *path, const char *arg0, ...)
{
    INT         i;
    va_list     valist;
    CPCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];                 /*  �����б�                    */
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  ѭ����ȡ����                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    va_end(valist);

    return  (spawnv(P_OVERLAY, path, (char * const *)pcParamList));     /*  �滻��ǰ�������н���        */
}
/*********************************************************************************************************
** ��������: execl
** ��������: exec family of functions
** �䡡��  : path          pathname that identifies the new module image file.
**           arg0          args...
**           ...
**           envp[]        evar array need to set.
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execle (const char *path, const char *arg0, ...)
{
    INT            i;
    va_list        valist;
    CPCHAR         pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];              /*  �����б�                    */
    char * const  *envp;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  ѭ����ȡ����                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    envp = (char * const *)va_arg(valist, char * const *);              /*  ��ȡ������������            */
    
    va_end(valist);
                                                                        /*  �滻��ǰ�������н���        */
    return  (spawnve(P_OVERLAY, path, (char * const *)pcParamList, envp));
}
/*********************************************************************************************************
** ��������: execlp
** ��������: exec family of functions
** �䡡��  : file          execlp() ��� PATH ����������ָ��Ŀ¼�в��ҷ��ϲ���file���ļ���.
**           arg0          args...
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execlp (const char *file, const char *arg0, ...)
{
    INT         i;
    va_list     valist;
    CPCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];                 /*  �����б�                    */
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  ѭ����ȡ����                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    va_end(valist);
    
    return  (spawnvp(P_OVERLAY, file, (char * const *)pcParamList));    /*  �滻��ǰ�������н���        */
}
/*********************************************************************************************************
** ��������: execv
** ��������: exec family of functions
** �䡡��  : path          pathname that identifies the new module image file.
**           argv          arglist
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execv (const char *path, char * const *argv)
{
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnv(P_OVERLAY, path, argv));                            /*  �滻��ǰ�������н���        */
}
/*********************************************************************************************************
** ��������: execv
** ��������: exec family of functions
** �䡡��  : path          pathname that identifies the new module image file.
**           argv          arglist
**           envp[]        evar array need to set.
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execve (const char *path, char *const argv[], char *const envp[])
{
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnve(P_OVERLAY, path, argv, envp));                     /*  �滻��ǰ�������н���        */
}
/*********************************************************************************************************
** ��������: execvp
** ��������: exec family of functions
** �䡡��  : file          execlp() ��� PATH ����������ָ��Ŀ¼�в��ҷ��ϲ���file���ļ���.
**           argv          arglist
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execvp (const char *file, char *const argv[])
{
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnvp(P_OVERLAY, file, argv));                           /*  �滻��ǰ�������н���        */
}
/*********************************************************************************************************
** ��������: execvpe
** ��������: exec family of functions
** �䡡��  : file          execlp() ��� PATH ����������ָ��Ŀ¼�в��ҷ��ϲ���file���ļ���.
**           argv          arglist
**           envp          ��������
** �䡡��  : module return
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int execvpe (const char *file, char * const *argv, char * const *envp)
{
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnvpe(P_OVERLAY, file, argv, envp));                    /*  �滻��ǰ�������н���        */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
