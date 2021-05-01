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
** ��   ��   ��: px_sched.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix ���ȼ��ݿ�.
*********************************************************************************************************/

#ifndef __PX_SCHED_H
#define __PX_SCHED_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#include "px_sched_param.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#define PX_ID_VERIFY(id, type)  do {            \
                                    if ((id) == 0) {    \
                                        (id) = (type)API_ThreadIdSelf();    \
                                    }   \
                                } while (0)

/*********************************************************************************************************
  sched policies
*********************************************************************************************************/

#define SCHED_FIFO              LW_OPTION_SCHED_FIFO
#define SCHED_RR                LW_OPTION_SCHED_RR
#define SCHED_OTHER             LW_OPTION_SCHED_RR

/*********************************************************************************************************
  sched api
*********************************************************************************************************/

LW_API int              sched_get_priority_max(int  iPolicy);
LW_API int              sched_get_priority_min(int  iPolicy);
LW_API int              sched_yield(void);
LW_API int              sched_setparam(pid_t  pid, const struct sched_param  *pschedparam);
LW_API int              sched_getparam(pid_t  pid, struct sched_param  *pschedparam);
LW_API int              sched_setscheduler(pid_t    pid, 
                                           int      iPolicy, 
                                           const struct sched_param  *pschedparam);
LW_API int              sched_getscheduler(pid_t  pid);
LW_API int              sched_rr_get_interval(pid_t  pid, struct timespec  *interval);

/*********************************************************************************************************
  sched affinity (������Ӧ��ʱ�����㷨������ͻ, �������ﲻ֧��, ֻ��Ϊ�˼�����Ҫ��������ؽṹ)
*********************************************************************************************************/

#define CPU_SETSIZE     LW_CPU_SETSIZE
#define NCPUBITS        LW_NCPUBITS
typedef LW_CLASS_CPUSET cpu_set_t;

#define CPU_SET(n, p)   LW_CPU_SET(n, p)
#define CPU_CLR(n, p)   LW_CPU_CLR(n, p)
#define CPU_ISSET(n, p) LW_CPU_ISSET(n, p)
#define CPU_ZERO(p)     LW_CPU_ZERO(p)

LW_API int              sched_setaffinity(pid_t pid, size_t setsize, const cpu_set_t *set);
LW_API int              sched_getaffinity(pid_t pid, size_t setsize, cpu_set_t *set);

/*********************************************************************************************************
  sched cpu affinity extern api
*********************************************************************************************************/

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_POSIXEX_EN > 0)
LW_API int              sched_cpuaffinity_enable_np(size_t setsize, const cpu_set_t *set);
LW_API int              sched_cpuaffinity_disable_np(size_t setsize, const cpu_set_t *set);
LW_API int              sched_cpuaffinity_set_np(size_t setsize, const cpu_set_t *set);
LW_API int              sched_cpuaffinity_get_np(size_t setsize, cpu_set_t *set);
#endif                                                                  /*  (LW_CFG_SMP_EN > 0)         */
                                                                        /*  (LW_CFG_POSIXEX_EN > 0)     */
/*********************************************************************************************************
  sched GJB7714 extern api
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0
LW_API int              sched_settimeslice(UINT32  ticks);
LW_API unsigned int     sched_gettimeslice(void);
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_SCHED_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
