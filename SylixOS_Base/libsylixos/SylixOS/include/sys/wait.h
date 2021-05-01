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
** ��   ��   ��: wait.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 09 ��
**
** ��        ��: posix wait. (BSD)
*********************************************************************************************************/

#ifndef __SYS_WAIT_H
#define __SYS_WAIT_H

#include "types.h"

#ifndef __PX_RESOURCE_H
#include <posix/include/px_resource.h>
#endif                                                                  /*  __PX_RESOURCE_H             */

#define WIFEXITED(status)       ((int)((status) & 0xFF00) == 0)
#define WIFSIGNALED(status)     ((int)((status) & 0xFF00) != 0)
#define WIFSTOPPED(status)      ((int)((status) & 0xFF0000) != 0)

#define WEXITSTATUS(status)     ((int)((status) & 0xFF))
#define WTERMSIG(status)        ((int)(((status) >> 8)  & 0xFF))
#define WSTOPSIG(status)        ((int)(((status) >> 16) & 0xFF))

/*********************************************************************************************************
  wait ���÷���ֵ״̬
*********************************************************************************************************/

#define SET_EXITSTATUS(status)  ((int)((status) & 0xFF))
#define SET_TERMSIG(sig)        ((int)(((sig) <<  8) & 0xFF00))
#define SET_STOPSIG(sig)        ((int)(((sig) << 16) & 0xFF0000))

/*********************************************************************************************************
  option.
*********************************************************************************************************/

#define WNOHANG                 1                                       /* Don't hang in wait.          */
#define WUNTRACED               2                                       /* Tell about stopped,          */
                                                                        /* untraced children.           */

/*********************************************************************************************************
  id type.
*********************************************************************************************************/

#ifndef __idtype_t_defined
typedef enum {
    P_PID,                                                              /* A process identifier.        */
    P_PGID,                                                             /* A process group identifier.  */
    P_ALL                                                               /* All processes.               */
} idtype_t;
#define __idtype_t_defined      1
#endif

/*********************************************************************************************************
  POSIX API.
*********************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

pid_t wait(int *stat_loc);
int   waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
pid_t waitpid(pid_t pid, int *stat_loc, int options);
pid_t wait3(int *stat_loc, int options, struct rusage *prusage);
pid_t wait4(pid_t pid, int *stat_loc, int options, struct rusage *prusage);

/*********************************************************************************************************
  SylixOS extern API.
*********************************************************************************************************/

BOOL  ischild(pid_t pid);
BOOL  isbrother(pid_t pid);
void  reclaimchild(pid_t pid);
int   detach(pid_t pid);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_WAIT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
