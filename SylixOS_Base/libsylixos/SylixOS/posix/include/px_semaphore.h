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
** ��   ��   ��: px_semaphore.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix ���ݿ��ź�������.
*********************************************************************************************************/

#ifndef __PX_SEMAPHORE_H
#define __PX_SEMAPHORE_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  GJB7714 need some pthread defines
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0
#include "px_pthread.h"
#endif

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#define SEM_FAILED              (LW_NULL)

#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX           (__ARCH_INT_MAX)                        /*  posix sem is 'int' type     */
#endif                                                                  /*  SEM_VALUE_MAX               */

/*********************************************************************************************************
  sem handle
*********************************************************************************************************/

typedef struct {
    PVOID                   SEM_pvPxSem;                                /*  �ź����ڲ��ṹ              */
    PLW_RESOURCE_RAW        SEM_presraw;                                /*  ��Դ����ڵ�                */
    ULONG                   SEM_ulPad[5];
} sem_t;
#define SEMAPHORE_INITIALIZER   {LW_NULL, LW_NULL}

/*********************************************************************************************************
  sem GJB7714 extern
  
  sem_open_method(SEM_OPEN_METHOD_GJB, NULL);
  sem_open("sem_name", O_CREAT, 0666, 1, SEM_BINARY, PTHREAD_WAITQ_PRIO);
  
  sem_open_method(SEM_OPEN_METHOD_POSIX, NULL);
  sem_open("sem_name", O_CREAT, 0666, 1);
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0
typedef int             WaitQ_Type;                                     /*  PTHREAD_WAITQ_PRIO          */
                                                                        /*  PTHREAD_WAITQ_FIFO          */
#define SEM_BINARY      0
#define SEM_COUNTING    1
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

/*********************************************************************************************************
  sem api
*********************************************************************************************************/

LW_API int          sem_init(sem_t  *psem, int  pshared, unsigned int  value);
LW_API int          sem_destroy(sem_t  *psem);
LW_API sem_t       *sem_open(const char  *name, int  flag, ...);
LW_API int          sem_close(sem_t  *psem);
LW_API int          sem_unlink(const char *name);
LW_API int          sem_wait(sem_t  *psem);
LW_API int          sem_trywait(sem_t  *psem);
LW_API int          sem_timedwait(sem_t  *psem, const struct timespec *timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          sem_reltimedwait_np(sem_t  *psem, const struct timespec *rel_timeout);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
LW_API int          sem_post(sem_t  *psem);
LW_API int          sem_getvalue(sem_t  *psem, int  *pivalue);

/*********************************************************************************************************
  sem GJB7714 extern api
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0

#define SEM_OPEN_METHOD_POSIX   0
#define SEM_OPEN_METHOD_GJB     1
#define SEM_OPEN_METHOD_DEFAULT SEM_OPEN_METHOD_POSIX

LW_API int          sem_open_method(int  method, int *old_method);      /*  used carefully!             */

typedef struct {
    ULONG                   SEMINFO_ulCounter;
    ULONG                   SEMINFO_ulOption;
    ULONG                   SEMINFO_ulBlockNum;
    ULONG                   SEMINFO_ulPad[6];
} sem_info_t;

LW_API int          sem_flush(sem_t  *psem);
LW_API int          sem_getinfo(sem_t  *psem, sem_info_t  *info);
LW_API int          sem_show(sem_t  *psem, int  level);
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_SEMAPHORE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
