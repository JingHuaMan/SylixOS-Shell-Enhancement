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
** ��   ��   ��: hstimerfd.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 03 ��
**
** ��        ��: ��Ƶ�ʶ�ʱ���豸�û����ʽӿ�.
*********************************************************************************************************/

#ifndef __SYS_HSTIMERFD_H
#define __SYS_HSTIMERFD_H

#include <unistd.h>

/*********************************************************************************************************
 hstimerfd_create() flags
*********************************************************************************************************/

#define HSTFD_CLOEXEC       O_CLOEXEC
#define HSTFD_NONBLOCK      O_NONBLOCK

/*********************************************************************************************************
 hstimerfd_settime2() and hstimerfd_gettime2 arguments type
*********************************************************************************************************/

typedef struct hstimer_cnt {
    unsigned long   value;
    unsigned long   interval;
} hstimer_cnt_t;

/*********************************************************************************************************
 hstimerfd api
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int hstimerfd_create(int flags);
int hstimerfd_settime(int fd, const struct itimerspec *ntmr, struct itimerspec *otmr);
int hstimerfd_settime2(int fd, hstimer_cnt_t *ncnt, hstimer_cnt_t *ocnt);
int hstimerfd_gettime(int fd, struct itimerspec *currvalue);
int hstimerfd_gettime2(int fd, hstimer_cnt_t *currvalue);
int hstimerfd_hz(void);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_HSTIMERFD_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
