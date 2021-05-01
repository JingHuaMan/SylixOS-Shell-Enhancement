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
** ��   ��   ��: epollDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 18 ��
**
** ��        ��: Linux epoll ��ϵͳ�����豸 (����֧�� epoll ������Ҫ����).
**
** ע        ��: SylixOS epoll ������ϵͳ���� select ��ϵͳģ�������, ����Ч��û�� select ��.
*********************************************************************************************************/

#ifndef __EPOLLDEV_H
#define __EPOLLDEV_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)

#include "sys/epoll.h"

/*********************************************************************************************************
  �豸·��
*********************************************************************************************************/

#define LW_EPOLL_DEV_PATH   "/dev/epollfd"

/*********************************************************************************************************
  �豸���ļ��ṹ
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          EPD_devhdrHdr;                                  /*  �豸ͷ                      */
} LW_EPOLL_DEV;
typedef LW_EPOLL_DEV   *PLW_EPOLL_DEV;

#define LW_EPOLL_HASH_SIZE  16
#define LW_EPOLL_HASH_MASK  (LW_EPOLL_HASH_SIZE - 1)

typedef struct {
#define LW_EPOLL_FILE_MAGIC 0x35ac796b
    UINT32              EPF_uiMagic;                                    /*  ħ��                        */
    INT                 EPF_iFlag;                                      /*  ���ļ���ѡ��              */
    LW_OBJECT_HANDLE    EPF_ulMutex;                                    /*  �������                    */
    LW_LIST_LINE_HEADER EPF_plineEvent[LW_EPOLL_HASH_SIZE];             /*  �¼�����                    */
} LW_EPOLL_FILE;
typedef LW_EPOLL_FILE  *PLW_EPOLL_FILE;

/*********************************************************************************************************
  �ļ��������
*********************************************************************************************************/

#define LW_EPOLL_FILE_LOCK(pepollfil)       \
        API_SemaphoreMPend(pepollfil->EPF_ulMutex, LW_OPTION_WAIT_INFINITE)
        
#define LW_EPOLL_FILE_UNLOCK(pepollfil)     \
        API_SemaphoreMPost(pepollfil->EPF_ulMutex)

/*********************************************************************************************************
  �ļ�������¼��ṹ
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE        EPE_lineManage;
    INT                 EPE_iFd;
    struct epoll_event  EPE_epEvent;
} LW_EPOLL_EVENT;
typedef LW_EPOLL_EVENT *PLW_EPOLL_EVENT;

/*********************************************************************************************************
  ��ʼ������
*********************************************************************************************************/

INT  _epollDrvInstall(VOID);
INT  _epollDevCreate(VOID);

/*********************************************************************************************************
  �����¼�����
*********************************************************************************************************/

PLW_EPOLL_EVENT _epollFindEvent(PLW_EPOLL_FILE pepollfil, INT  iFd);
INT             _epollAddEvent(PLW_EPOLL_FILE  pepollfil, INT  iFd, struct epoll_event *event);
INT             _epollDelEvent(PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent);
INT             _epollModEvent(PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent, 
                               struct epoll_event *event);

/*********************************************************************************************************
  select �¼����ϲ���
*********************************************************************************************************/

INT   _epollInitFdset(PLW_EPOLL_FILE  pepollfil, 
                      fd_set         *pfdsetRead,
                      fd_set         *pfdsetWrite,
                      fd_set         *pfdsetExcept);
INT   _epollFiniFdset(PLW_EPOLL_FILE      pepollfil, 
                      INT                 iWidth,
                      fd_set             *pfdsetRead,
                      fd_set             *pfdsetWrite,
                      fd_set             *pfdsetExcept,
                      struct epoll_event *events, 
                      int                 maxevents);

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
#endif                                                                  /*  __EPOLLDEV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
