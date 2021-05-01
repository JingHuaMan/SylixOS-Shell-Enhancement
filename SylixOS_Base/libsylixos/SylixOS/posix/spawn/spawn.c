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
** ��   ��   ��: spawn.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 12 ��
**
** ��        ��: posix ʵʱ spawn ��.

** BUG:
2013.05.01  ���� posix ���ڷ���ֵ�Ĺ涨.
2013.06.07  ������������ӽ��̹���Ŀ¼����չ�ӿ�.
2013.06.12  posix_spawn �� posix_spawnp ��������ɹ�����ֵΪ 0.
2014.05.13  ����� sigstop ���ԵĲ���.
*********************************************************************************************************/
#define  __SYLIXOS_SPAWN
#define  __SYLIXOS_KERNEL
#include "../include/px_spawn.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_MODULELOADER_EN > 0)
#include "../SylixOS/loader/include/loader_vppatch.h"
#include "../SylixOS/loader/include/loader_exec.h"
#include "process.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT  __ldGetFilePath(CPCHAR  pcParam, PCHAR  pcPathBuffer, size_t  stMaxLen);
/*********************************************************************************************************
** ��������: posix_spawn
** ��������: spawn a process (ADVANCED REALTIME)
** �䡡��  : path              program file
**           file_actions      file actions
**           attrp             spawn attribute
**           argv, envp        args and envs
** �䡡��  : child pid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#define __POSIX_SPAWN_SET_PID(pid, num)  { if (pid) { *pid = num; } }

LW_API  
int   posix_spawn (pid_t *pid, const char *path,
                   const posix_spawn_file_actions_t *file_actions,
                   const posix_spawnattr_t *attrp, 
                   char *const argv[],
                   char *const envp[])
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    pid_t               pidChild;
    
    if (!path) {
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = EINVAL;
        return  (EINVAL);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = ENOMEM;
        return  (ENOMEM);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                __POSIX_SPAWN_SET_PID(pid, -1);
                errno = ENOMEM;
                return  (ENOMEM);
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
                __POSIX_SPAWN_SET_PID(pid, -1);
                errno = ENOMEM;
                return  (ENOMEM);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    if (attrp && attrp->SPA_pcWd && lib_strlen(attrp->SPA_pcWd)) {
        psarg->SA_pcWd = lib_strdup(attrp->SPA_pcWd);
        if (!psarg->SA_pcWd) {
            __spawnArgFree(psarg);
            __POSIX_SPAWN_SET_PID(pid, -1);
            errno = ENOMEM;
            return  (ENOMEM);
        }
    }
    
    if (attrp) {
        psarg->SA_spawnattr = *attrp;
    }
    
    if (file_actions && 
        (file_actions->SPA_iInited == 12345) &&
        *file_actions->SPA_pplineActions) {
        psarg->SA_plineActions = __spawnArgActionDup(*file_actions->SPA_pplineActions);
        if (psarg->SA_plineActions == LW_NULL) {
            __spawnArgFree(psarg);
            __POSIX_SPAWN_SET_PID(pid, -1);
            errno = ENOMEM;
            return  (ENOMEM);
        }
    }
    
    pidChild = __processStart(_P_NOWAIT, psarg);
    if (pidChild > 0) {
        __POSIX_SPAWN_SET_PID(pid, pidChild);
        return  (ERROR_NONE);
        
    } else {
        __POSIX_SPAWN_SET_PID(pid, -1);
        if (errno) {
            return  (errno);
        } else {
            return  (ENOMEM);                                           /*  �����ϲ������е�����        */
        }
    }
}
/*********************************************************************************************************
** ��������: posix_spawnp
** ��������: spawn a process (ADVANCED REALTIME)
** �䡡��  : file              program file
**           file_actions      file actions
**           attrp             spawn attribute
**           argv, envp        args and envs
** �䡡��  : child pid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnp (pid_t *pid, const char *file,
                    const posix_spawn_file_actions_t *file_actions,
                    const posix_spawnattr_t *attrp, 
                    char *const argv[],
                    char *const envp[])
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    pid_t               pidChild;
    
    if (!file) {
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = EINVAL;
        return  (EINVAL);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = ENOENT;
        return  (ENOENT);                                               /*  �޷��ҵ�����                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = ENOMEM;
        return  (ENOMEM);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                __POSIX_SPAWN_SET_PID(pid, -1);
                errno = ENOMEM;
                return  (ENOMEM);
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
                __POSIX_SPAWN_SET_PID(pid, -1);
                errno = ENOMEM;
                return  (ENOMEM);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        __POSIX_SPAWN_SET_PID(pid, -1);
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    if (attrp && attrp->SPA_pcWd && lib_strlen(attrp->SPA_pcWd)) {
        psarg->SA_pcWd = lib_strdup(attrp->SPA_pcWd);
        if (!psarg->SA_pcWd) {
            __spawnArgFree(psarg);
            __POSIX_SPAWN_SET_PID(pid, -1);
            errno = ENOMEM;
            return  (ENOMEM);
        }
    }
    
    if (attrp) {
        psarg->SA_spawnattr = *attrp;
    }
    
    if (file_actions && 
        (file_actions->SPA_iInited == 12345) &&
        *file_actions->SPA_pplineActions) {
        psarg->SA_plineActions = __spawnArgActionDup(*file_actions->SPA_pplineActions);
        if (psarg->SA_plineActions == LW_NULL) {
            __spawnArgFree(psarg);
            __POSIX_SPAWN_SET_PID(pid, -1);
            errno = ENOMEM;
            return  (ENOMEM);
        }
    }
    
    pidChild = __processStart(_P_NOWAIT, psarg);
    if (pidChild > 0) {
        __POSIX_SPAWN_SET_PID(pid, pidChild);
        return  (ERROR_NONE);
        
    } else {
        __POSIX_SPAWN_SET_PID(pid, -1);
        if (errno) {
            return  (errno);
        } else {
            return  (ENOMEM);                                           /*  �����ϲ������е�����        */
        }
    }
}
/*********************************************************************************************************
** ��������: posix_spawnattr_free
** ��������: free data structure for posix_spawnattr_t for `spawn' call.
** �䡡��  : presraw  posix_spawnattr_t.SPA_presraw
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  posix_spawnattr_free (PLW_RESOURCE_RAW  presraw)
{
    __resDelRawHook(presraw);                                           /*  �����Դ����                */
    
    __SHEAP_FREE(presraw);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_init
** ��������: Initialize data structure for file attribute for `spawn' call.
** �䡡��  : attrp     spawn attr
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_init (posix_spawnattr_t *attrp)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_bzero(attrp, sizeof(posix_spawnattr_t));
    
    attrp->SPA_presraw = (PLW_RESOURCE_RAW)__SHEAP_ALLOC(sizeof(LW_RESOURCE_RAW) + MAX_FILENAME_LENGTH);
    if (!attrp->SPA_presraw) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    attrp->SPA_pcWd    = (PCHAR)attrp->SPA_presraw + sizeof(LW_RESOURCE_RAW);
    attrp->SPA_pcWd[0] = PX_EOS;
    
    __resAddRawHook(attrp->SPA_presraw, (VOIDFUNCPTR)posix_spawnattr_free,
                    attrp->SPA_presraw, 0, 0, 0, 0, 0);                 /*  ���̽���ʱ��Ҫ������Դ      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_destroy
** ��������: Deinitialize data structure for file attribute for `spawn' call.
** �䡡��  : attrp     spawn attr
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_destroy (posix_spawnattr_t *attrp)
{
    if (!attrp || !attrp->SPA_presraw) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __resDelRawHook(attrp->SPA_presraw);                                /*  �����Դ����                */
    
    __SHEAP_FREE(attrp->SPA_presraw);
    
    attrp->SPA_presraw = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getopt
** ��������: Get spawn option from ATTR
** �䡡��  : attrp         spawn attr
**           popt          spawn option
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getopt (const posix_spawnattr_t *attrp, posix_spawnopt_t *popt)
{
    if (!attrp || !popt) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *popt = attrp->SPA_opt;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setopt
** ��������: Set spawn option setting for ATTR
** �䡡��  : attrp         spawn attr
**           popt          spawn option
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setopt (posix_spawnattr_t *attrp, const posix_spawnopt_t *popt)
{
    if (!attrp || !popt) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    attrp->SPA_opt = *popt;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getwd
** ��������: Get working directory from ATTR
** �䡡��  : attrp         spawn attr
**           pwd           buffer
**           size          buffer size
** �䡡��  : ok or not (if strlen(ppwd) == 0 the new process working directory will inherit creator's)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getwd (const posix_spawnattr_t *attrp, char *pwd, size_t size)
{
    if (!attrp || !pwd || !size) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_strlcpy(pwd, attrp->SPA_pcWd, size);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setwd
** ��������: Set working directory to ATTR
** �䡡��  : attrp         spawn attr
**           pwd           new working directory
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setwd (posix_spawnattr_t *attrp, const char *pwd)
{
    if (!attrp || !pwd) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (lib_strnlen(pwd, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        errno = ENAMETOOLONG;
        return  (ENAMETOOLONG);
    }
    
    lib_strlcpy(attrp->SPA_pcWd, pwd, MAX_FILENAME_LENGTH);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getsigdefault
** ��������: Store signal mask for signals with default handling from ATTR in
             SIGDEFAULT
** �䡡��  : attrp         spawn attr
**           sigdefault    signal defaut mask
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getsigdefault (const posix_spawnattr_t *attrp, sigset_t *sigdefault)
{
    if (!attrp || !sigdefault) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_memcpy(sigdefault, &attrp->SPA_sigsetDefault, sizeof(sigset_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setsigdefault
** ��������: Set signal mask for signals with default handling from ATTR in
             SIGDEFAULT
** �䡡��  : attrp         spawn attr
**           sigdefault    signal defaut mask
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setsigdefault (posix_spawnattr_t *attrp, const sigset_t *sigdefault)
{
    if (!attrp || !sigdefault) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_memcpy(&attrp->SPA_sigsetDefault, sigdefault, sizeof(sigset_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getsigmask
** ��������: Store signal mask for the new process from ATTR in SIGMASK.
** �䡡��  : attrp         spawn attr
**           sigmask       signal mask
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getsigmask (const posix_spawnattr_t *attrp, sigset_t *sigmask)
{
    if (!attrp || !sigmask) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_memcpy(sigmask, &attrp->SPA_sigsetMask, sizeof(sigset_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setsigmask
** ��������: Set signal mask for the new process from ATTR in SIGMASK.
** �䡡��  : attrp         spawn attr
**           sigmask       signal mask
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setsigmask (posix_spawnattr_t *attrp, const sigset_t *sigmask)
{
    if (!attrp || !sigmask) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lib_memcpy(&attrp->SPA_sigsetMask, sigmask, sizeof(sigset_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getflags
** ��������: Get flag word from the attribute structure.
** �䡡��  : attrp         spawn attr
**           flags         spawn flag
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getflags (const posix_spawnattr_t *attrp, short *flags)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (flags) {
        *flags = attrp->SPA_sFlags;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setflags
** ��������: Set flag word from the attribute structure.
** �䡡��  : attrp         spawn attr
**           flags         spawn flag
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setflags (posix_spawnattr_t *attrp, short flags)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    attrp->SPA_sFlags = flags;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getextflags
** ��������: Get flag word from the attribute structure.
** �䡡��  : attrp         spawn attr
**           flags         spawn ext flag
** �䡡��  : ok or not
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int   posix_spawnattr_getextflags (const posix_spawnattr_t *attrp, unsigned long *flags)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (flags) {
        *flags = attrp->SPA_ulExts;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setextflags
** ��������: Set flag word from the attribute structure.
** �䡡��  : attrp         spawn attr
**           flags         spawn ext flag
** �䡡��  : ok or not
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
int   posix_spawnattr_setextflags (posix_spawnattr_t *attrp, unsigned long flags)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }

    attrp->SPA_ulExts = flags;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getpgroup
** ��������: Get process group ID from the attribute structure. 
** �䡡��  : attrp         spawn attr
**           pgroup        group ID
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getpgroup (const posix_spawnattr_t *attrp, pid_t *pgroup)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pgroup) {
        *pgroup = attrp->SPA_pidGroup;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setpgroup
** ��������: Set process group ID from the attribute structure. 
** �䡡��  : attrp         spawn attr
**           pgroup        group ID
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setpgroup (posix_spawnattr_t *attrp, pid_t pgroup)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    attrp->SPA_pidGroup = pgroup;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getschedpolicy
** ��������: Get scheduling policy from the attribute structure.
** �䡡��  : attrp         spawn attr
**           schedpolicy   scheduling policy
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getschedpolicy (const posix_spawnattr_t *attrp, int *schedpolicy)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (schedpolicy) {
        *schedpolicy = attrp->SPA_iPolicy;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setschedpolicy
** ��������: Set scheduling policy from the attribute structure.
** �䡡��  : attrp         spawn attr
**           schedpolicy   scheduling policy
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setschedpolicy (posix_spawnattr_t *attrp, int schedpolicy)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    attrp->SPA_iPolicy = schedpolicy;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_getschedparam
** ��������: Get scheduling parameters from the attribute structure.
** �䡡��  : attrp         spawn attr
**           schedparam    scheduling parameters
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_getschedparam (const posix_spawnattr_t *attrp, struct sched_param *schedparam)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (schedparam) {
        *schedparam = attrp->SPA_schedparam;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawnattr_setschedparam
** ��������: Set scheduling parameters from the attribute structure.
** �䡡��  : attrp         spawn attr
**           schedparam    scheduling parameters
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawnattr_setschedparam (posix_spawnattr_t *attrp, const struct sched_param *schedparam)
{
    if (!attrp) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (schedparam) {
        attrp->SPA_schedparam = *schedparam;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __posix_spawn_file_actions_free
** ��������: free data structure for file attribute for `spawn' call.
** �䡡��  : file_actions  file attribute
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  posix_spawn_file_actions_free (LW_LIST_LINE_HEADER *pplineActions)
{
    PLW_RESOURCE_RAW    presraw = (PLW_RESOURCE_RAW)((PCHAR)pplineActions
                                + sizeof(LW_LIST_LINE_HEADER));
    PLW_LIST_LINE       plineTemp;
    __spawn_action     *pspawnactFree;
    
    plineTemp = *pplineActions;
    while (plineTemp) {
        pspawnactFree = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        __SHEAP_FREE(pspawnactFree);
    }
    
    __resDelRawHook(presraw);                                           /*  �����Դ����                */
    
    __SHEAP_FREE(pplineActions);
}
/*********************************************************************************************************
** ��������: posix_spawn_file_actions_init
** ��������: Initialize data structure for file attribute for `spawn' call.
** �䡡��  : file_actions  file attribute
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawn_file_actions_init (posix_spawn_file_actions_t *file_actions)
{
    if (!file_actions) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    file_actions->SPA_pplineActions = (LW_LIST_LINE_HEADER *)__SHEAP_ALLOC(sizeof(LW_LIST_LINE_HEADER) +
                                                                           sizeof(LW_RESOURCE_RAW));
    if (!file_actions->SPA_pplineActions) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    *file_actions->SPA_pplineActions = LW_NULL;                         /*  Ŀǰû���ļ�                */
    
    file_actions->SPA_presraw = (PLW_RESOURCE_RAW)((PCHAR)file_actions->SPA_pplineActions 
                              + sizeof(LW_LIST_LINE_HEADER));
    
    __resAddRawHook(file_actions->SPA_presraw, (VOIDFUNCPTR)posix_spawn_file_actions_free,
                    file_actions->SPA_pplineActions, 0, 0, 0, 0, 0);    /*  ���̽���ʱ��Ҫ������Դ      */
                    
    file_actions->SPA_iInited = 12345;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawn_file_actions_destroy
** ��������: Deinitialize data structure for file attribute for `spawn' call.
** �䡡��  : file_actions  file attribute
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawn_file_actions_destroy (posix_spawn_file_actions_t *file_actions)
{
    PLW_LIST_LINE   plineTemp;
    __spawn_action *pspawnactFree;

    if (!file_actions || (file_actions->SPA_iInited != 12345)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    plineTemp = *file_actions->SPA_pplineActions;
    while (plineTemp) {
        pspawnactFree = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        __SHEAP_FREE(pspawnactFree);
    }
    
    *file_actions->SPA_pplineActions = LW_NULL;
    
    __resDelRawHook(file_actions->SPA_presraw);                         /*  �����Դ����                */
    
    __SHEAP_FREE(file_actions->SPA_pplineActions);
    
    file_actions->SPA_iInited = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawn_file_actions_addopen
** ��������: Add an action to FILE-ACTIONS which tells the implementation to call
             `open' for the given file during the `spawn' call.
**           file_actions   file attribute
**           fd             file descriptor
**           path, oflag, mode like open(path, oflag, mode)
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawn_file_actions_addopen (posix_spawn_file_actions_t *file_actions,
					                    int fd, const char *path, int oflag, mode_t mode)
{
    __spawn_action *pspawnactNew;
    size_t          stPathLen;
    
    if (!file_actions || (file_actions->SPA_iInited != 12345)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((fd < 0) || (fd >= (LW_CFG_MAX_FILES + 3))) {
        errno = EBADF;
        return  (EBADF);
    }
    
    if (!path) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    stPathLen = lib_strnlen(path, PATH_MAX);
    if (stPathLen >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return  (ENAMETOOLONG);
    }
    
    pspawnactNew = (__spawn_action *)__SHEAP_ALLOC(sizeof(__spawn_action) + stPathLen + 1);
    if (pspawnactNew == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    pspawnactNew->SFA_iType   = __FILE_ACTIONS_DO_OPEN;
    pspawnactNew->SFA_iOpenFd = fd;
    pspawnactNew->SFA_pcPath  = ((PCHAR)pspawnactNew + sizeof(__spawn_action));
    pspawnactNew->SFA_iFlag   = oflag;
    pspawnactNew->SFA_mode    = mode;
    
    lib_strcpy(pspawnactNew->SFA_pcPath, path);
    
    _List_Line_Add_Ahead(&pspawnactNew->SFA_lineManage,
                         file_actions->SPA_pplineActions);
                         
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawn_file_actions_addclose
** ��������: Add an action to FILE-ACTIONS which tells the implementation to call
             `close' for the given file descriptor during the `spawn' call.
**           file_actions   file attribute
**           fd             file descriptor
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawn_file_actions_addclose (posix_spawn_file_actions_t *file_actions, int fd)
{
    __spawn_action *pspawnactNew;
    
    if (!file_actions || (file_actions->SPA_iInited != 12345)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((fd < 0) || (fd >= (LW_CFG_MAX_FILES + 3))) {
        errno = EBADF;
        return  (EBADF);
    }
    
    pspawnactNew = (__spawn_action *)__SHEAP_ALLOC(sizeof(__spawn_action));
    if (pspawnactNew == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    pspawnactNew->SFA_iType    = __FILE_ACTIONS_DO_CLOSE;
    pspawnactNew->SFA_iCloseFd = fd;
    
    _List_Line_Add_Ahead(&pspawnactNew->SFA_lineManage,
                         file_actions->SPA_pplineActions);
                         
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: posix_spawn_file_actions_adddup2
** ��������: Add an action to FILE-ACTIONS which tells the implementation to call
             `dup2' for the given file descriptors during the `spawn' call.
**           file_actions   file attribute
**           fd             file descriptor
**           newfd          to be duplicated as newfildes
** �䡡��  : ok or not
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int   posix_spawn_file_actions_adddup2 (posix_spawn_file_actions_t *file_actions, int fd, int newfd)
{
    __spawn_action *pspawnactNew;
    
    if (!file_actions || (file_actions->SPA_iInited != 12345)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((fd < 0) || (fd >= (LW_CFG_MAX_FILES + 3))) {
        errno = EBADF;
        return  (EBADF);
    }
    
    if ((newfd < 0) || (newfd >= (LW_CFG_MAX_FILES + 3))) {
        errno = EBADF;
        return  (EBADF);
    }
    
    pspawnactNew = (__spawn_action *)__SHEAP_ALLOC(sizeof(__spawn_action));
    if (pspawnactNew == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    pspawnactNew->SFA_iType      = __FILE_ACTIONS_DO_DUP2;
    pspawnactNew->SFA_iDup2Fd    = fd;
    pspawnactNew->SFA_iDup2NewFd = newfd;
    
    _List_Line_Add_Ahead(&pspawnactNew->SFA_lineManage,
                         file_actions->SPA_pplineActions);
                         
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
