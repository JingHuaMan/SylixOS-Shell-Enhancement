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
** ��   ��   ��: px_spawn.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 12 ��
**
** ��        ��: posix ʵʱ spawn ��.
*********************************************************************************************************/

#ifndef __PX_SPAWN_H
#define __PX_SPAWN_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */
#include "signal.h"

/*********************************************************************************************************
  posix schedule parameters
*********************************************************************************************************/

#include "px_sched_param.h"

/*********************************************************************************************************
  Flags to be set in the `posix_spawnattr_t'.
*********************************************************************************************************/

#define POSIX_SPAWN_RESETIDS                0x01
#define POSIX_SPAWN_SETPGROUP               0x02
#define POSIX_SPAWN_SETSIGDEF               0x04
#define POSIX_SPAWN_SETSIGMASK              0x08
#define POSIX_SPAWN_SETSCHEDPARAM           0x10
#define POSIX_SPAWN_SETSCHEDULER            0x20

/*********************************************************************************************************
  Data structure to contain attributes for process option.
*********************************************************************************************************/

typedef struct {
    INT                 SPO_iSigNo;                                     /*  �ź�                        */
    LW_OBJECT_HANDLE    SPO_ulId;                                       /*  Ŀ���߳� (�����)           */
    ULONG               SPO_ulMainOption;                               /*  ���߳� OPTION               */
    size_t              SPO_stStackSize;                                /*  ���̶߳�ջ��С (0 Ϊ�̳�)   */
} posix_spawnopt_t;

/*********************************************************************************************************
  Data structure to contain attributes for process creation.
*********************************************************************************************************/

typedef struct {
    short               SPA_sFlags;
    pid_t               SPA_pidGroup;
    sigset_t            SPA_sigsetDefault;
    sigset_t            SPA_sigsetMask;
    struct sched_param  SPA_schedparam;
    INT                 SPA_iPolicy;
    PCHAR               SPA_pcWd;
    PLW_RESOURCE_RAW    SPA_presraw;                                    /*  ��Դ����ڵ�                */
    posix_spawnopt_t    SPA_opt;
    ULONG               SPA_ulExts;                                     /*  �����Ǳ�ѡ��                */
    ULONG               SPA_ulPad[5];
} posix_spawnattr_t;

/*********************************************************************************************************
  posix_spawnattr_t extend flags.
*********************************************************************************************************/

#define POSIX_SPAWN_EXT_NO_FILE_INHERIT         0x00000001              /*  No file inherit             */
#define POSIX_SPAWN_EXT_NO_FILE_INHERIT_EXC_STD 0x00000002              /*  No file inherit except 012  */
#define POSIX_SPAWN_EXT_NO_FILE_INHERIT_CLOEXEC 0x00000004              /*  No file with cloexec inherit*/

/*********************************************************************************************************
  Data structure to contain information about the actions to be
  performed in the new process with respect to file descriptors.
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
typedef struct {
    LW_LIST_LINE        SFA_lineManage;
    INT                 SFA_iType;
#define __FILE_ACTIONS_DO_CLOSE 0
#define __FILE_ACTIONS_DO_OPEN  1
#define __FILE_ACTIONS_DO_DUP2  2
    union {
        struct {
            INT         SFAC_iFd;
        } SFA_closeaction;
        struct {
            INT         SFAO_iFd;
            PCHAR       SFAO_pcPath;
            INT         SFAO_iFlag;
            mode_t      SFAO_mode;
        } SFA_openaction;
        struct {
            INT         SFAD_iFd;
            INT         SFAD_iFdNew;
        } SFA_dup2action;
    } SFA_action;
#define SFA_iCloseFd    SFA_action.SFA_closeaction.SFAC_iFd
#define SFA_iOpenFd     SFA_action.SFA_openaction.SFAO_iFd
#define SFA_pcPath      SFA_action.SFA_openaction.SFAO_pcPath
#define SFA_iFlag       SFA_action.SFA_openaction.SFAO_iFlag
#define SFA_mode        SFA_action.SFA_openaction.SFAO_mode
#define SFA_iDup2Fd     SFA_action.SFA_dup2action.SFAD_iFd
#define SFA_iDup2NewFd  SFA_action.SFA_dup2action.SFAD_iFdNew
} __spawn_action;
#endif                                                                  /*  __SYLIXOS_KERNEL            */

typedef struct {
    LW_LIST_LINE_HEADER    *SPA_pplineActions;
    PLW_RESOURCE_RAW        SPA_presraw;                                /*  ��Դ����ڵ�                */
    INT                     SPA_iInited;
    ULONG                   SPA_ulPad[16];
} posix_spawn_file_actions_t;

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

__BEGIN_NAMESPACE_STD

/*********************************************************************************************************
  api.
*********************************************************************************************************/

LW_API int   posix_spawn(pid_t *pid, const char *path,
                         const posix_spawn_file_actions_t *file_actions,
                         const posix_spawnattr_t *attrp, 
                         char *const argv[],
                         char *const envp[]);
                         
LW_API int   posix_spawnp(pid_t *pid, const char *file,
                          const posix_spawn_file_actions_t *file_actions,
                          const posix_spawnattr_t *attrp,
                          char *const argv[], 
                          char *const envp[]);
                          
LW_API int   posix_spawnattr_init(posix_spawnattr_t *attrp);
LW_API int   posix_spawnattr_destroy(posix_spawnattr_t *attrp);

LW_API int   posix_spawnattr_getwd(const posix_spawnattr_t *attrp, char *pwd, size_t size);
LW_API int   posix_spawnattr_setwd(posix_spawnattr_t *attrp, const char *pwd);

LW_API int   posix_spawnattr_getsigdefault(const posix_spawnattr_t *attrp,
                                           sigset_t *sigdefault);
LW_API int   posix_spawnattr_setsigdefault(posix_spawnattr_t *attrp,
                                           const sigset_t *sigdefault);
                                           
LW_API int   posix_spawnattr_getsigmask(const posix_spawnattr_t *attrp,
                                        sigset_t *sigmask);
LW_API int   posix_spawnattr_setsigmask(posix_spawnattr_t *attrp,
                                        const sigset_t *sigmask);
                                        
LW_API int   posix_spawnattr_getflags(const posix_spawnattr_t *attrp,
                                      short *flags);
LW_API int   posix_spawnattr_setflags(posix_spawnattr_t *attrp,
                                      short flags);

LW_API int   posix_spawnattr_getextflags(const posix_spawnattr_t *attrp,
                                         unsigned long *flags);
LW_API int   posix_spawnattr_setextflags(posix_spawnattr_t *attrp,
                                         unsigned long flags);

LW_API int   posix_spawnattr_getpgroup(const posix_spawnattr_t *attrp, 
                                       pid_t *pgroup);
LW_API int   posix_spawnattr_setpgroup(posix_spawnattr_t *attrp, 
                                       pid_t pgroup);
                                       
LW_API int   posix_spawnattr_getschedpolicy(const posix_spawnattr_t *attrp,
                                            int *schedpolicy);
LW_API int   posix_spawnattr_setschedpolicy(posix_spawnattr_t *attrp,
                                            int schedpolicy);
                                            
LW_API int   posix_spawnattr_getschedparam(const posix_spawnattr_t *attrp,
                                           struct sched_param *schedparam);
LW_API int   posix_spawnattr_setschedparam(posix_spawnattr_t *attrp,
                                           const struct sched_param *schedparam);
                                           
LW_API int   posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions);
LW_API int   posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions);

LW_API int   posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *file_actions,
					     int fd, const char *path, int oflag, mode_t mode);
LW_API int   posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
                         int fd);
LW_API int   posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
					     int fd, int newfd);

__END_NAMESPACE_STD

/*********************************************************************************************************
  sylixos extern (debuger use only!)
*********************************************************************************************************/

LW_API int   posix_spawnattr_getopt(const posix_spawnattr_t *attrp, posix_spawnopt_t *popt);
LW_API int   posix_spawnattr_setopt(posix_spawnattr_t *attrp, const posix_spawnopt_t *popt);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_SPAWN_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
