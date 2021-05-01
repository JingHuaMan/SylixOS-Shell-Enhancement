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
** ��   ��   ��: epollLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 18 ��
**
** ��        ��: Linux epoll ��ϵͳ (����֧�� epoll ������Ҫ����).
**
** ע        ��: SylixOS epoll ������ϵͳ���� select ��ϵͳģ�������, ����Ч��û�� select ��.

** BUG:
2017.08.31  epoll_pwait() ���ؾ�ȷ���ļ�����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)
#include "epollDev.h"
/*********************************************************************************************************
  ��ȫ�Ļ�ȡ epoll �ļ��ṹ
*********************************************************************************************************/
#define LW_EPOLL_FILE_GET(fd)   \
        pepollfil = (PLW_EPOLL_FILE)API_IosFdValue((fd));   \
        if (pepollfil == (PLW_EPOLL_FILE)PX_ERROR) {    \
            return  (PX_ERROR); \
        }   \
        if (pepollfil->EPF_uiMagic != LW_EPOLL_FILE_MAGIC) {    \
            _ErrorHandle(EBADF);    \
            return  (PX_ERROR); \
        }
/*********************************************************************************************************
** ��������: _EpollInit
** ��������: ��ʼ�� epoll ��ϵͳ
** �䡡��  : NONE
** �䡡��  : ��ʼ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _EpollInit (VOID)
{
    INT     iError;
    
    iError = _epollDrvInstall();
    if (iError == ERROR_NONE) {
        iError =  _epollDevCreate();
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: epoll_create
** ��������: ����һ�� epoll �ļ�������
** �䡡��  : size      ��ʱδʹ��
** �䡡��  : epoll �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  epoll_create (int size)
{
    INT iFd;

    (VOID)size;
    
    iFd = open(LW_EPOLL_DEV_PATH, O_RDWR);
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: epoll_create1
** ��������: ����һ�� epoll �ļ�������
** �䡡��  : flags 
** �䡡��  : epoll �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  epoll_create1 (int flags)
{
    INT iFd;
    
    flags &= (EPOLL_CLOEXEC | EPOLL_NONBLOCK);
    
    iFd = open(LW_EPOLL_DEV_PATH, O_RDWR | flags);
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: epoll_ctl
** ��������: ����һ�� epoll �ļ�������
** �䡡��  : epfd          �ļ�������
**           op            �������� EPOLL_CTL_ADD / EPOLL_CTL_DEL / EPOLL_CTL_MOD
**           fd            Ŀ���ļ�������
**           event         �¼�
** �䡡��  : 0 ��ʾ�ɹ� -1 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  epoll_ctl (int epfd, int op, int fd, struct epoll_event *event)
{
    INT             iError;
    PLW_EPOLL_FILE  pepollfil;
    PLW_EPOLL_EVENT pepollevent;
    
    if (!event && (op != EPOLL_CTL_DEL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_EPOLL_FILE_GET(epfd);
    
    if (pepollfil == (PLW_EPOLL_FILE)API_IosFdValue(fd)) {              /*  ���ܲ����Լ�                */
        _ErrorHandle(ELOOP);
        return  (PX_ERROR);
    }
    
    iError = PX_ERROR;
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    pepollevent = _epollFindEvent(pepollfil, fd);
    
    switch (op) {
    
    case EPOLL_CTL_ADD:
        if (!pepollevent) {
            iError = _epollAddEvent(pepollfil, fd, event);
        } else {
            _ErrorHandle(EEXIST);
        }
        break;
        
    case EPOLL_CTL_DEL:
        if (pepollevent) {
            iError = _epollDelEvent(pepollfil, pepollevent);
        } else {
            _ErrorHandle(ENOENT);
        }
        break;
        
    case EPOLL_CTL_MOD:
        if (pepollevent) {
            iError = _epollModEvent(pepollfil, pepollevent, event);
        } else {
            _ErrorHandle(ENOENT);
        }
        break;
        
    default:
        _ErrorHandle(EINVAL);
        break;
    }
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: epoll_wait
** ��������: �ȴ�ָ�� epoll �ļ��������¼���Ч
** �䡡��  : epfd          �ļ�������
**           event         ��Ч�¼�������
**           maxevents     ��Ч�¼���������С
**           timeout       ��ʱʱ�� (����, -1 ��ʾ���õȴ�)
** �䡡��  : ��Ч���¼�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  epoll_wait (int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    return  (epoll_pwait(epfd, events, maxevents, timeout, LW_NULL));
}
/*********************************************************************************************************
** ��������: epoll_pwait
** ��������: �ȴ�ָ�� epoll �ļ��������¼���Ч
** �䡡��  : epfd          �ļ�������
**           event         ��Ч�¼�������
**           maxevents     ��Ч�¼���������С
**           timeout       ��ʱʱ�� (����, -1 ��ʾ���õȴ�)
**           sigmask       �ȴ�ʱ�������ź�
** �䡡��  : ��Ч���¼�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  epoll_pwait (int epfd, struct epoll_event *events, int maxevents, int timeout, 
                  const sigset_t *sigmask)
{
    struct timespec ts;
    fd_set          fdsetRead, fdsetWrite, fdsetExcept;
    PLW_EPOLL_FILE  pepollfil;
    INT             iWidth;
    INT             iNum;
    
    LW_EPOLL_FILE_GET(epfd);
    
    FD_ZERO(&fdsetRead);
    FD_ZERO(&fdsetWrite);
    FD_ZERO(&fdsetExcept);
    
    if (timeout >= 0) {
        ts.tv_sec  = (time_t)(timeout / 1000);
        ts.tv_nsec = (LONG)(timeout % 1000) * (1000 * 1000);
    }
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    iWidth = _epollInitFdset(pepollfil, &fdsetRead, &fdsetWrite, &fdsetExcept);
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    if (timeout < 0) {
        iNum = pselect(iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, LW_NULL, sigmask);
    
    } else {
        iNum = pselect(iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, &ts, sigmask);
    }
    
    if (iNum <= 0) {
        return  (iNum);
    }
    
    LW_EPOLL_FILE_GET(epfd);
    
    LW_EPOLL_FILE_LOCK(pepollfil);
    iNum = _epollFiniFdset(pepollfil, iWidth, &fdsetRead, &fdsetWrite, &fdsetExcept, events, maxevents);
    LW_EPOLL_FILE_UNLOCK(pepollfil);
    
    return  (iNum);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
