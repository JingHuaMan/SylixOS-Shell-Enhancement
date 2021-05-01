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
** ��   ��   ��: poll.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 11 ��
**
** ��        ��: ���� posix poll ��. (����ʹ�� select ʵ��)

** BUG:
2013.11.18  fd ����ͬʱ���ڶ�д�쳣�ȴ�.
            ���� ppoll ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_poll.h"                                         /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  MASK (POLLNVAL not supported)
*********************************************************************************************************/
#define __PX_POLL_INMASK                (POLLIN  | POLLPRI)
#define __PX_POLL_OUTMASK               (POLLOUT | POLLWRBAND)
/*********************************************************************************************************
** ��������: ppoll
** ��������: input/output multiplexing
** �䡡��  : fds           pecifies the file descriptors to be examined and the events of interest for 
                           each file descriptor.
**           nfds          The array's members are pollfd structures within which fd specifies
**           timeout_ts    specifies an upper limit on the amount of
                           time that ppoll() will block
             sigmask       signal mask.
** �䡡��  : A positive value indicates the total number of file descriptors that have been selected.
             A value of 0 indicates that the call timed out and no file descriptors have been selected. 
             Upon failure, poll() returns -1 and sets errno
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int   ppoll (struct pollfd fds[], nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
{
    INT                 i;
    INT                 iMaxFd = PX_ERROR;
    INT                 iRet;
    fd_set              fdsetRead, fdsetWrite, fdsetExcept;
    
    if (!fds || (nfds < 1)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    FD_ZERO(&fdsetRead);
    FD_ZERO(&fdsetWrite);
    FD_ZERO(&fdsetExcept);
    
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd < 0) {
            continue;
        }
        
        fds[i].revents = 0;
        if (fds[i].events & __PX_POLL_INMASK) {
            FD_SET(fds[i].fd, &fdsetRead);
        }
        
        if (fds[i].events & __PX_POLL_OUTMASK) {
            FD_SET(fds[i].fd, &fdsetWrite);
        }
        
        if (fds[i].events) {
            FD_SET(fds[i].fd, &fdsetExcept);
        }
        
        if (iMaxFd < fds[i].fd) {
            iMaxFd = fds[i].fd;
        }
    }
    
    iRet = pselect(iMaxFd + 1, &fdsetRead, &fdsetWrite, &fdsetExcept, timeout_ts, sigmask);
    if (iRet <= 0) {
        if (errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER) {
            errno = EIO;
        } else if (errno == ERROR_IO_SELECT_CONTEXT) {
            errno = ENOMEM;
        } else if (errno == ERROR_IO_SELECT_WIDTH) {
            errno = EBADF;
        } else if (errno == ERROR_THREAD_WAIT_TIMEOUT) {
            errno = ETIMEDOUT;
        }
        return  (iRet);
    }
    
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd < 0) {
            fds[i].revents |= POLLNVAL;
            continue;
        }
        
        if (FD_ISSET(fds[i].fd, &fdsetRead)) {
            fds[i].revents |= (short int)(fds[i].events & __PX_POLL_INMASK);
        }
        
        if (FD_ISSET(fds[i].fd, &fdsetWrite)) {
            fds[i].revents |= (short int)(fds[i].events & __PX_POLL_OUTMASK);
        }
        
        if (FD_ISSET(fds[i].fd, &fdsetExcept)) {
            fds[i].revents |= POLLERR;
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: poll
** ��������: input/output multiplexing
** �䡡��  : fds           pecifies the file descriptors to be examined and the events of interest for 
                           each file descriptor.
**           nfds          The array's members are pollfd structures within which fd specifies
**           timeout       wait (timeout) milliseconds for an event to occur, on any of the selected file 
                           descriptors.
                           0:  poll() returns immediately
                           -1: poll() blocks until a requested event occurs.
** �䡡��  : A positive value indicates the total number of file descriptors that have been selected.
             A value of 0 indicates that the call timed out and no file descriptors have been selected. 
             Upon failure, poll() returns -1 and sets errno
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int   poll (struct pollfd fds[], nfds_t nfds, int timeout)
{
    struct timespec ts;
    INT             iRet;
    
    if (timeout >= 0) {
        ts.tv_sec  = (time_t)(timeout / 1000);
        ts.tv_nsec = (LONG)(timeout % 1000) * (1000 * 1000);
    }
    
    if (timeout < 0) {
        iRet = ppoll(fds, nfds, LW_NULL, LW_NULL);
    
    } else {
        iRet = ppoll(fds, nfds, &ts, LW_NULL);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
