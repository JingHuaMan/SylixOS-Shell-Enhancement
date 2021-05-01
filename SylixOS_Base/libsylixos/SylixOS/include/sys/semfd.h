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
** ��   ��   ��: semfd.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 23 ��
**
** ��        ��: �ź����豸ʵ��.
*********************************************************************************************************/

#ifndef __SYS_SEMFD_H
#define __SYS_SEMFD_H

#include <unistd.h>
#include <fcntl.h>

/*********************************************************************************************************
 semfd() type
*********************************************************************************************************/

typedef enum {
    SEMFD_TYPE_BINARY = 0,
    SEMFD_TYPE_COUNTING,
    SEMFD_TYPE_MUTEX
} semfd_type;

/*********************************************************************************************************
 semfd() options (Currently, only wait queue type and delete safe options can be set)
*********************************************************************************************************/

#define SEMFD_OPT_WAIT_PRIO     LW_OPTION_WAIT_PRIORITY
#define SEMFD_OPT_DELETE_SAFE   LW_OPTION_DELETE_SAFE                   /*  Only for mutex              */

/*********************************************************************************************************
 set/get parameter (Initial value and Max value <= INT32_MAX)
*********************************************************************************************************/

struct semfd_param {
    semfd_type      sem_type;                                           /*  Type                        */
    UINT32          sem_opts;                                           /*  Options                     */
    UINT32          sem_value;                                          /*  Initial value               */
    UINT32          sem_max;                                            /*  Max value                   */
    UINT32          auto_unlink;                                        /*  unlink on last close        */
    UINT32          reserved[3];
};

/*********************************************************************************************************
 semfd() api

 NOTICE:
 The ... arguments is: mode_t mode, struct semfd_param *param.
 If (flags & O_CREAT) You must add 'mode' 'param' argument.
 If semfd file already exist, semfd() did not change the type value and options.
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int semfd(const char *name, int flags, ...);
int semfd_timeout(int fd, unsigned long *ms);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_SEMFD_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
