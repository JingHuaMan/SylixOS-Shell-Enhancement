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
** ��   ��   ��: px_mqueue.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix ���ݿ���Ϣ���в���. 
*********************************************************************************************************/

#ifndef __PX_MQUEUE_H
#define __PX_MQUEUE_H

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

/*********************************************************************************************************
  macro
*********************************************************************************************************/

#define MQ_PRIO_MAX     32                                              /*  ��Ϣ��������ȼ�            */

/*********************************************************************************************************
  mqueue attr
*********************************************************************************************************/

typedef struct mq_attr {
    long                mq_flags;                                       /*  message queue flags         */
    long                mq_maxmsg;                                      /*  max number of messages      */
    long                mq_msgsize;                                     /*  max message size            */
    long                mq_curmsgs;                                     /*  number of messages currently*/
#if LW_CFG_GJB7714_EN > 0
    /*
     *  mq_open() use MQ_OPEN_METHOD_GJB method, use 'mq_curmsgs' as 'mq_waitqtype'.
     *  PTHREAD_WAITQ_PRIO / PTHREAD_WAITQ_FIFO
     */
#define mq_waitqtype    mq_curmsgs                                      /*  ONLY mq_open() use this     */
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
} mq_attr_t;

extern mq_attr_t        mq_attr_default;                                /*  Ĭ������                    */

/*********************************************************************************************************
  mqueue handle
*********************************************************************************************************/

typedef long            mqd_t;

#define MQ_FAILED       (mqd_t)-1

/*********************************************************************************************************
  mqueue api
*********************************************************************************************************/

LW_API mqd_t            mq_open(const char  *name, int  flag, ...);
LW_API int              mq_close(mqd_t  mqd);
LW_API int              mq_unlink(const char  *name);
LW_API int              mq_getattr(mqd_t  mqd, struct mq_attr *pmqattr);
LW_API int              mq_setattr(mqd_t  mqd, const struct mq_attr *pmqattrNew, 
                                   struct mq_attr *pmqattrOld);
LW_API int              mq_send(mqd_t  mqd, const char  *msg, size_t  msglen, unsigned msgprio);
LW_API int              mq_timedsend(mqd_t  mqd, const char  *msg, size_t  msglen, 
                                     unsigned msgprio, const struct timespec *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int              mq_reltimedsend_np(mqd_t  mqd, const char  *msg, size_t  msglen, 
                                           unsigned msgprio, const struct timespec *rel_timeout);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
LW_API ssize_t          mq_receive(mqd_t  mqd, char  *msg, size_t  msglen, unsigned *pmsgprio);
LW_API ssize_t          mq_timedreceive(mqd_t  mqd, char  *msg, size_t  msglen, 
                                        unsigned *pmsgprio, const struct timespec *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API ssize_t          mq_reltimedreceive_np(mqd_t  mqd, char  *msg, size_t  msglen, 
                                              unsigned *pmsgprio, const struct timespec *rel_timeout);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
LW_API int              mq_notify(mqd_t  mqd, const struct sigevent  *pnotify);

/*********************************************************************************************************
  mqueue GJB7714 extern api
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0

#define MQ_OPEN_METHOD_POSIX    0
#define MQ_OPEN_METHOD_GJB      1
#define MQ_OPEN_METHOD_DEFAULT  MQ_OPEN_METHOD_POSIX

LW_API int              mq_open_method(int  method, int *old_method);   /*  used carefully!             */
LW_API mqd_t            mq_create(int  flags, int  maxmsg, int  msgsize, int  waitqtype);
LW_API int              mq_delete(mqd_t  mqd);

typedef struct {
    mq_attr_t       attr;
    mode_t          mode;
    uint32_t        priomap;
    ULONG           reservepad[12];
} mq_info_t;

LW_API int              mq_getinfo(mqd_t  mqd, mq_info_t  *info);
LW_API int              mq_show(mqd_t  mqd, int  level);

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_MQUEUE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
