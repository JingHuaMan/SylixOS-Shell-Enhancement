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
** ��   ��   ��: epollDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 18 ��
**
** ��        ��: Linux epoll ��ϵͳ�����豸 (����֧�� epoll ������Ҫ����).
**
** ע        ��: SylixOS epoll ������ϵͳ���� select ��ϵͳģ�������, ����Ч��û�� select ��.

** BUG:
2017.08.31  _epollFiniFdset() ���ؾ�ȷ���ļ�����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0) && (LW_CFG_EPOLL_EN > 0)
#include "epollDev.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iEpollDrvNum = PX_ERROR;
static LW_EPOLL_DEV     _G_epolldev;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _epollOpen(PLW_EPOLL_DEV    pepolldev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _epollClose(PLW_EPOLL_FILE  pepollfil);
static INT      _epollIoctl(PLW_EPOLL_FILE  pepollfil, INT  iRequest, LONG  lArg);
/*********************************************************************************************************
** ��������: _epollDrvInstall
** ��������: ��װ epoll �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollDrvInstall (VOID)
{
    if (_G_iEpollDrvNum <= 0) {
        _G_iEpollDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _epollOpen,
                                         _epollClose,
                                         LW_NULL,
                                         LW_NULL,
                                         _epollIoctl);
        DRIVER_LICENSE(_G_iEpollDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iEpollDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iEpollDrvNum, "epoll driver.");
    }
    
    return  ((_G_iEpollDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: _epollDevCreate
** ��������: ��װ epoll �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
INT  _epollDevCreate (VOID)
{
    if (_G_iEpollDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_epolldev.EPD_devhdrHdr, LW_EPOLL_DEV_PATH, 
                    _G_iEpollDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _epollOpen
** ��������: �� epoll �豸
** �䡡��  : pepolldev        epoll �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _epollOpen (PLW_EPOLL_DEV pepolldev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_EPOLL_FILE  pepollfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        pepollfil = (PLW_EPOLL_FILE)__SHEAP_ALLOC(sizeof(LW_EPOLL_FILE));
        if (!pepollfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_bzero(pepollfil, sizeof(LW_EPOLL_FILE));
        
        pepollfil->EPF_uiMagic = LW_EPOLL_FILE_MAGIC;
        pepollfil->EPF_iFlag   = iFlags;
        pepollfil->EPF_ulMutex = API_SemaphoreMCreate("epoll_mutex", LW_PRIO_DEF_CEILING,
                                                      LW_OPTION_WAIT_PRIORITY |
                                                      LW_OPTION_DELETE_SAFE | 
                                                      LW_OPTION_INHERIT_PRIORITY |
                                                      LW_OPTION_OBJECT_GLOBAL,
                                                      LW_NULL);
        if (pepollfil->EPF_ulMutex == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(pepollfil);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&_G_epolldev.EPD_devhdrHdr);
        
        return  ((LONG)pepollfil);
    }
}
/*********************************************************************************************************
** ��������: _epollClose
** ��������: �ر� epoll �ļ�
** �䡡��  : pepollfil         epoll �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _epollClose (PLW_EPOLL_FILE  pepollfil)
{
    INT             i;
    PLW_EPOLL_EVENT pepollevent;

    if (pepollfil) {
        LW_EPOLL_FILE_LOCK(pepollfil);
        for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
            while (pepollfil->EPF_plineEvent[i]) {
                pepollevent = _LIST_ENTRY(pepollfil->EPF_plineEvent[i], 
                                          LW_EPOLL_EVENT, EPE_lineManage);
                _List_Line_Del(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[i]);
                __SHEAP_FREE(pepollevent);
            }
        }
        LW_EPOLL_FILE_UNLOCK(pepollfil);
        
        LW_DEV_DEC_USE_COUNT(&_G_epolldev.EPD_devhdrHdr);
        
        API_SemaphoreMDelete(&pepollfil->EPF_ulMutex);
        __SHEAP_FREE(pepollfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _epollIoctl
** ��������: ���� epoll �ļ�
** �䡡��  : pepollfil        epoll �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _epollIoctl (PLW_EPOLL_FILE  pepollfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat *pstatGet;
    
    switch (iRequest) {
    
    case FIONBIO:
        LW_EPOLL_FILE_LOCK(pepollfil);
        if (*(INT *)lArg) {
            pepollfil->EPF_iFlag |= O_NONBLOCK;
        } else {
            pepollfil->EPF_iFlag &= ~O_NONBLOCK;
        }
        LW_EPOLL_FILE_UNLOCK(pepollfil);
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_epolldev.EPD_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _epollFindEvent
** ��������: epoll �ļ�Ѱ��һ��ƥ����¼�
** �䡡��  : pepollfil        epoll �ļ�
**           iFd              �ļ�������
** �䡡��  : Ѱ�ҵ����¼�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_EPOLL_EVENT _epollFindEvent (PLW_EPOLL_FILE pepollfil, INT  iFd)
{
    INT             iHash = (iFd & LW_EPOLL_HASH_MASK);
    PLW_LIST_LINE   plineTemp;
    PLW_EPOLL_EVENT pepollevent;
    
    for (plineTemp  = pepollfil->EPF_plineEvent[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
        if (pepollevent->EPE_iFd == iFd) {
            return  (pepollevent);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _epollAddEvent
** ��������: epoll �ļ����һ���¼�
** �䡡��  : pepollfil        epoll �ļ�
**           iFd              �ļ�������
**           event            �¼�
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollAddEvent (PLW_EPOLL_FILE pepollfil, INT  iFd, struct epoll_event *event)
{
    INT             iHash = (iFd & LW_EPOLL_HASH_MASK);
    PLW_EPOLL_EVENT pepollevent;
    
    pepollevent = (PLW_EPOLL_EVENT)__SHEAP_ALLOC(sizeof(LW_EPOLL_EVENT));
    if (pepollevent == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pepollevent->EPE_iFd             = iFd;
    pepollevent->EPE_epEvent         = *event;
    pepollevent->EPE_epEvent.events |= EPOLLERR | EPOLLHUP;             /*  Linux �������Ĳ���          */
    
    _List_Line_Add_Tail(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[iHash]);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _epollDelEvent
** ��������: epoll �ļ�ɾ��һ���¼�
** �䡡��  : pepollfil        epoll �ļ�
**           pepollevent      epoll �¼�
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollDelEvent (PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent)
{
    INT  iHash = (pepollevent->EPE_iFd & LW_EPOLL_HASH_MASK);

    _List_Line_Del(&pepollevent->EPE_lineManage, &pepollfil->EPF_plineEvent[iHash]);
    
    __SHEAP_FREE(pepollevent);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _epollModEvent
** ��������: epoll �ļ��޸�һ���¼�
** �䡡��  : pepollfil        epoll �ļ�
**           pepollevent      epoll �¼�
**           event            ���¼�
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollModEvent (PLW_EPOLL_FILE  pepollfil, PLW_EPOLL_EVENT pepollevent, struct epoll_event *event)
{
    pepollevent->EPE_epEvent         = *event;
    pepollevent->EPE_epEvent.events |= EPOLLERR | EPOLLHUP;             /*  Linux �������Ĳ���          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _epollInitFdset
** ��������: epoll �ļ������¼�, ���� select �ļ�����������
** �䡡��  : pepollfil        epoll �ļ�
**           pfdsetRead       ���ļ�����������
**           pfdsetWrite      д�ļ�����������
**           pfdsetExcept     �쳣�ļ�����������
** �䡡��  : ����ļ������� + 1
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollInitFdset (PLW_EPOLL_FILE  pepollfil, 
                      fd_set         *pfdsetRead,
                      fd_set         *pfdsetWrite,
                      fd_set         *pfdsetExcept)
{
#define LW_EPOLL_INMASK    (EPOLLIN)
#define LW_EPOLL_OUTMASK   (EPOLLOUT)
#define LW_EPOLL_EXCMASK   (EPOLLERR | EPOLLHUP)
    
    INT             i;
    INT             iWidth = 0;
    PLW_LIST_LINE   plineTemp;
    PLW_EPOLL_EVENT pepollevent;
    
    for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
        for (plineTemp  = pepollfil->EPF_plineEvent[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_INMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetRead);
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_OUTMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetWrite);
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_EXCMASK) {
                FD_SET(pepollevent->EPE_iFd, pfdsetExcept);
            }
            
            if (iWidth < pepollevent->EPE_iFd) {
                iWidth = pepollevent->EPE_iFd;
            }
        }
    }
    
    return  (iWidth + 1);
}
/*********************************************************************************************************
** ��������: _epollInitFdset
** ��������: epoll �ļ����� select ���, ��д��Ӧ���¼�.
** �䡡��  : pepollfil        epoll �ļ�
**           iWidth           ����ļ��� + 1
**           pfdsetRead       ���ļ�����������
**           pfdsetWrite      д�ļ�����������
**           pfdsetExcept     �쳣�ļ�����������
**           events           ��д�¼�����
**           maxevents        ��д�¼������С
** �䡡��  : ��Ч fd ������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _epollFiniFdset (PLW_EPOLL_FILE      pepollfil, 
                      INT                 iWidth,
                      fd_set             *pfdsetRead,
                      fd_set             *pfdsetWrite,
                      fd_set             *pfdsetExcept,
                      struct epoll_event *events, 
                      int                 maxevents)
{
    BOOL                bOne;
    INT                 i, iCnt = 0;
    UINT32              uiEvents;
    PLW_LIST_LINE       plineTemp;
    PLW_EPOLL_EVENT     pepollevent;
    
    for (i = 0; i < LW_EPOLL_HASH_SIZE; i++) {
        for (plineTemp  = pepollfil->EPF_plineEvent[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            bOne        = LW_FALSE;
            uiEvents    = 0;
            pepollevent = _LIST_ENTRY(plineTemp, LW_EPOLL_EVENT, EPE_lineManage);
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_INMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetRead)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_INMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_OUTMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetWrite)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_OUTMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (pepollevent->EPE_epEvent.events & LW_EPOLL_EXCMASK) {
                if (FD_ISSET(pepollevent->EPE_iFd, pfdsetExcept)) {
                    bOne      = LW_TRUE;
                    uiEvents |= LW_EPOLL_EXCMASK;
                    if (pepollevent->EPE_epEvent.events & EPOLLONESHOT) {
                        pepollevent->EPE_epEvent.events = 0;
                    }
                }
            }
            
            if (bOne) {
                if (iCnt < maxevents) {
                    events[iCnt].events = uiEvents;
                    events[iCnt].data   = pepollevent->EPE_epEvent.data;
                }
                iCnt++;
            }
        }
    }
    
    return  (iCnt);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
                                                                        /*  LW_CFG_EPOLL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
