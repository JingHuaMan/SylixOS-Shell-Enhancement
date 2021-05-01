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
** ��   ��   ��: loader_exec.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 26 ��
**
** ��        ��: unistd exec family of functions.
**
** ע        ��: sylixos ���л���������Ϊȫ������, ���Ե�ʹ�� envp ʱ, ����ϵͳ�Ļ�������������֮�ı�.
*********************************************************************************************************/

#ifndef __LOADER_EXEC_H
#define __LOADER_EXEC_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

/*********************************************************************************************************
  ������������
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) && defined(__SYLIXOS_SPAWN)

#include "spawn.h"

typedef struct {
    LW_LD_VPROC            *SA_pvproc;
    INT                     SA_iArgs;
    INT                     SA_iEvns;
    PCHAR                   SA_pcPath;
    PCHAR                   SA_pcWd;
    PCHAR                   SA_pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];
    PCHAR                   SA_pcpcEvn[LW_CFG_SHELL_MAX_PARAMNUM];
    LW_LIST_LINE_HEADER     SA_plineActions;
    posix_spawnattr_t       SA_spawnattr;                               /*  posix spawn attribute       */
} __SPAWN_ARG;
typedef __SPAWN_ARG        *__PSPAWN_ARG;

__PSPAWN_ARG        __spawnArgCreate(VOID);
VOID                __spawnArgFree(__PSPAWN_ARG  psarg);
LW_LIST_LINE_HEADER __spawnArgActionDup(LW_LIST_LINE_HEADER  plineAction);
INT                 __processStart(INT  mode, __PSPAWN_ARG  psarg);

#endif                                                                  /*  KERNEL && SPAWN             */
/*********************************************************************************************************
  api
*********************************************************************************************************/

LW_API int execl(  const char *path, const char *argv0, ...);
LW_API int execle( const char *path, const char *argv0, ... /*, char * const *envp */);
LW_API int execlp( const char *file, const char *argv0, ...);
LW_API int execv(  const char *path, char * const *argv);
LW_API int execve( const char *path, char * const *argv, char * const *envp);
LW_API int execvp( const char *file, char * const *argv);
LW_API int execvpe(const char *file, char * const *argv, char * const *envp);

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  __LOADER_EXEC_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
