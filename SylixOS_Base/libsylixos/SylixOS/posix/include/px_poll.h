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
** ��   ��   ��: px_poll.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 11 ��
**
** ��        ��: ���� posix poll ��. (����ʹ�� select ʵ��)
*********************************************************************************************************/

#ifndef __PX_POLL_H
#define __PX_POLL_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  pollfd structure 
*********************************************************************************************************/

struct pollfd {
    int         fd;                                                     /*  file descriptor being polled*/
    short int   events;                                                 /*  the input event flags       */
    short int   revents;                                                /*  the output event flags      */
};

/*********************************************************************************************************
  nfds_t type used for the number of file descriptors.
*********************************************************************************************************/

typedef unsigned int        nfds_t;

/*********************************************************************************************************
  event type POLLERR POLLHUP POLLNVAL are revents only
  
  warning : because use select() internal, so do not support POLLNVAL
*********************************************************************************************************/

#define POLLIN      (POLLRDNORM | POLLRDBAND)                           /* There is data to read        */
#define POLLRDNORM  0x0001
#define POLLRDBAND  0x0002
#define POLLPRI     0x0004                                              /* There is urgent data to read */

#define POLLOUT     0x0008                                              /* Writing now will not block   */
#define POLLWRNORM  POLLOUT
#define POLLWRBAND  0x0010

#define POLLERR     0x0020                                              /* Error condition              */
#define POLLHUP     0x0040                                              /* Hung up                      */
#define POLLNVAL    0x0080                                              /* Invalid request: fd not open */

/*********************************************************************************************************
  api
*********************************************************************************************************/

LW_API int   poll(struct pollfd fds[], nfds_t nfds, int timeout);
LW_API int   ppoll(struct pollfd fds[], nfds_t nfds, const struct timespec *timeout_ts,
                   const sigset_t *sigmask);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_POLL_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
