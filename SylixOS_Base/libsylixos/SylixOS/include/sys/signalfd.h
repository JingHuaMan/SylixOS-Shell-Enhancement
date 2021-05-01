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
** ��   ��   ��: signalfd.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 21 ��
**
** ��        ��: Linux ���� signalfd ʵ��.
*********************************************************************************************************/

#ifndef __SYS_SIGNALFD_H
#define __SYS_SIGNALFD_H

#include <stdint.h>

/*********************************************************************************************************
 signalfd() flags
*********************************************************************************************************/

#define SFD_CLOEXEC         O_CLOEXEC
#define SFD_NONBLOCK        O_NONBLOCK

/*********************************************************************************************************
 signalfd_siginfo
*********************************************************************************************************/

struct signalfd_siginfo {
    uint32_t ssi_signo;                                         /* Signal number                        */
    int32_t  ssi_errno;                                         /* Error number (unused)                */
    int32_t  ssi_code;                                          /* Signal code                          */
    uint32_t ssi_pid;                                           /* PID of sender                        */
    uint32_t ssi_uid;                                           /* Real UID of sender                   */
    int32_t  ssi_fd;                                            /* File descriptor (SIGIO)              */
    uint32_t ssi_tid;                                           /* Kernel timer ID (POSIX timers)       */
    uint32_t ssi_band;                                          /* Band event (SIGIO)                   */
    uint32_t ssi_overrun;                                       /* POSIX timer overrun count            */
    uint32_t ssi_trapno;                                        /* Trap number that caused signal       */
    int32_t  ssi_status;                                        /* Exit status or signal (SIGCHLD)      */
    int32_t  ssi_int;                                           /* Integer sent by sigqueue(3)          */
    uint64_t ssi_ptr;                                           /* Pointer sent by sigqueue(3)          */
    uint64_t ssi_utime;                                         /* User CPU time consumed (SIGCHLD)     */
    uint64_t ssi_stime;                                         /* System CPU time consumed (SIGCHLD)   */
    uint64_t ssi_addr;                                          /* Address that generated signal        */
                                                                /* (for hardware-generated signals)     */
    uint8_t  pad[48];
};

/*********************************************************************************************************
 signalfd api
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                          /*  __cplusplus                         */

int signalfd(int fd, const sigset_t *mask, int flags);

#ifdef __cplusplus
}
#endif                                                          /*  __cplusplus                         */

#endif                                                          /*  __SYS_SIGNALFD_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
