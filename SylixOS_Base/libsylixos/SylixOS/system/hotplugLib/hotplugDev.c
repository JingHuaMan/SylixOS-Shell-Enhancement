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
** ��   ��   ��: hotplugDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 10 �� 02 ��
**
** ��        ��: �Ȳ����Ϣ�豸, Ӧ�ó�����Զ�ȡ���豸����ȡϵͳ�Ȳ����Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_HOTPLUG_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
  �豸���ļ��ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           HOTPDEV_devhdrHdr;                             /*  �豸ͷ                      */
    LW_LIST_LINE_HEADER  HOTPDEV_plineFile;                             /*  �򿪵��ļ�����              */
    LW_OBJECT_HANDLE     HOTPDEV_ulMutex;                               /*  �������                    */
} LW_HOTPLUG_DEV;
typedef LW_HOTPLUG_DEV  *PLW_HOTPLUG_DEV;

typedef struct {
    LW_LIST_LINE         HOTPFIL_lineManage;                            /*  �ļ�����                    */
    INT                  HOTPFIL_iFlag;                                 /*  ���ļ���ѡ��              */
    INT                  HOTPFIL_iMsg;                                  /*  ���ĵ��Ȳ����Ϣ            */
    PLW_BMSG             HOTPFIL_pbmsg;                                 /*  ��Ϣ������                  */
    LW_OBJECT_HANDLE     HOTPFIL_ulReadSync;                            /*  ��ȡͬ���ź���              */
    LW_SEL_WAKEUPLIST    HOTPFIL_selwulist;
} LW_HOTPLUG_FILE;
typedef LW_HOTPLUG_FILE *PLW_HOTPLUG_FILE;
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT               _G_iHotplugDrvNum = PX_ERROR;
static LW_HOTPLUG_DEV    _G_hotplugdev;
static LW_OBJECT_HANDLE  _G_hHotplugSelMutex;
/*********************************************************************************************************
  �豸�������
*********************************************************************************************************/
#define HOTPLUG_DEV_LOCK()      API_SemaphoreMPend(_G_hotplugdev.HOTPDEV_ulMutex, LW_OPTION_WAIT_INFINITE)
#define HOTPLUG_DEV_UNLOCK()    API_SemaphoreMPost(_G_hotplugdev.HOTPDEV_ulMutex)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _hotplugOpen(PLW_HOTPLUG_DEV photplugdev, 
                             PCHAR           pcName,
                             INT             iFlags, 
                             INT             iMode);
static INT      _hotplugClose(PLW_HOTPLUG_FILE  photplugfil);
static ssize_t  _hotplugRead(PLW_HOTPLUG_FILE  photplugfil, 
                             PCHAR             pcBuffer, 
                             size_t            stMaxBytes);
static ssize_t  _hotplugWrite(PLW_HOTPLUG_FILE  photplugfil, 
                              PCHAR             pcBuffer, 
                              size_t            stNBytes);
static INT      _hotplugIoctl(PLW_HOTPLUG_FILE  photplugfil, 
                              INT               iRequest, 
                              LONG              lArg);
/*********************************************************************************************************
** ��������: _hotplugDrvInstall
** ��������: ��װ hotplug ��Ϣ�豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _hotplugDrvInstall (VOID)
{
    if (_G_iHotplugDrvNum <= 0) {
        _G_iHotplugDrvNum  = iosDrvInstall(_hotplugOpen,
                                           LW_NULL,
                                           _hotplugOpen,
                                           _hotplugClose,
                                           _hotplugRead,
                                           _hotplugWrite,
                                           _hotplugIoctl);
        DRIVER_LICENSE(_G_iHotplugDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iHotplugDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iHotplugDrvNum, "hotplug message driver.");
    }
    
    if (_G_hHotplugSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hHotplugSelMutex =  API_SemaphoreMCreate("hpsel_lock", LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    }

    return  ((_G_iHotplugDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: _hotplugDevCreate
** ��������: ��װ hotplug ��Ϣ�豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
INT  _hotplugDevCreate (VOID)
{
    if (_G_iHotplugDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    _G_hotplugdev.HOTPDEV_plineFile = LW_NULL;
    _G_hotplugdev.HOTPDEV_ulMutex   = API_SemaphoreMCreate("hotplug_lock", LW_PRIO_DEF_CEILING, 
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_DELETE_SAFE | 
                                                           LW_OPTION_INHERIT_PRIORITY |
                                                           LW_OPTION_OBJECT_GLOBAL,
                                                           LW_NULL);
    if (_G_hotplugdev.HOTPDEV_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_hotplugdev.HOTPDEV_devhdrHdr, LW_HOTPLUG_DEV_PATH, 
                    _G_iHotplugDrvNum, DT_CHR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_hotplugdev.HOTPDEV_ulMutex);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _hotplugDevPutMsg
** ��������: ����һ�� hotplug ��Ϣ
** �䡡��  : iMsg      ��Ϣ����
**           pvMsg     ��Ҫ�������Ϣ
**           stSize    ��Ϣ����.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
VOID  _hotplugDevPutMsg (INT  iMsg, CPVOID pvMsg, size_t stSize)
{
    PLW_LIST_LINE       plineTemp;
    PLW_HOTPLUG_FILE    photplugfil;
    
    if ((_G_hotplugdev.HOTPDEV_ulMutex   == LW_OBJECT_HANDLE_INVALID) ||
        (_G_hotplugdev.HOTPDEV_plineFile == LW_NULL)) {
        return;
    }
    
    HOTPLUG_DEV_LOCK();
    for (plineTemp  = _G_hotplugdev.HOTPDEV_plineFile;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        photplugfil = _LIST_ENTRY(plineTemp, LW_HOTPLUG_FILE, HOTPFIL_lineManage);
        if ((photplugfil->HOTPFIL_iMsg == iMsg) ||
            (photplugfil->HOTPFIL_iMsg == LW_HOTPLUG_MSG_ALL)) {
            _bmsgPut(photplugfil->HOTPFIL_pbmsg, pvMsg, stSize);
            API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
            SEL_WAKE_UP_ALL(&photplugfil->HOTPFIL_selwulist, SELREAD);
        }
    }
    HOTPLUG_DEV_UNLOCK();
}
/*********************************************************************************************************
** ��������: _hotplugOpen
** ��������: ���Ȳ����Ϣ�豸
** �䡡��  : photplugdev      �Ȳ����Ϣ�豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _hotplugOpen (PLW_HOTPLUG_DEV photplugdev, 
                           PCHAR           pcName,
                           INT             iFlags, 
                           INT             iMode)
{
    PLW_HOTPLUG_FILE  photplugfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  �����ظ�����                */
            return  (PX_ERROR);
        }
        
        photplugfil = (PLW_HOTPLUG_FILE)__SHEAP_ALLOC(sizeof(LW_HOTPLUG_FILE));
        if (!photplugfil) {
__nomem:
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        photplugfil->HOTPFIL_iFlag = iFlags;
        photplugfil->HOTPFIL_iMsg  = LW_HOTPLUG_MSG_ALL;
        photplugfil->HOTPFIL_pbmsg = _bmsgCreate(LW_CFG_HOTPLUG_DEV_DEFAULT_BUFSIZE);
        if (photplugfil->HOTPFIL_pbmsg == LW_NULL) {
            __SHEAP_FREE(photplugfil);
            goto    __nomem;
        }
        
        photplugfil->HOTPFIL_ulReadSync = API_SemaphoreBCreate("hotplug_rsync", LW_FALSE,
                                                               LW_OPTION_OBJECT_GLOBAL,
                                                               LW_NULL);
        if (photplugfil->HOTPFIL_ulReadSync == LW_OBJECT_HANDLE_INVALID) {
            _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
            __SHEAP_FREE(photplugfil);
            return  (PX_ERROR);
        }
        
        lib_bzero(&photplugfil->HOTPFIL_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        photplugfil->HOTPFIL_selwulist.SELWUL_hListLock = _G_hHotplugSelMutex;

        HOTPLUG_DEV_LOCK();
        _List_Line_Add_Tail(&photplugfil->HOTPFIL_lineManage,
                            &_G_hotplugdev.HOTPDEV_plineFile);
        HOTPLUG_DEV_UNLOCK();
        
        LW_DEV_INC_USE_COUNT(&_G_hotplugdev.HOTPDEV_devhdrHdr);
        
        return  ((LONG)photplugfil);
    }
}
/*********************************************************************************************************
** ��������: _hotplugClose
** ��������: �ر��Ȳ����Ϣ�ļ�
** �䡡��  : photplugfil      �Ȳ����Ϣ�ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _hotplugClose (PLW_HOTPLUG_FILE  photplugfil)
{
    if (photplugfil) {
        HOTPLUG_DEV_LOCK();
        _List_Line_Del(&photplugfil->HOTPFIL_lineManage,
                       &_G_hotplugdev.HOTPDEV_plineFile);
        HOTPLUG_DEV_UNLOCK();
        
        _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
        
        LW_DEV_DEC_USE_COUNT(&_G_hotplugdev.HOTPDEV_devhdrHdr);
        
        API_SemaphoreBDelete(&photplugfil->HOTPFIL_ulReadSync);

        SEL_WAKE_UP_TERM(&photplugfil->HOTPFIL_selwulist);

        __SHEAP_FREE(photplugfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _hotplugRead
** ��������: ���Ȳ����Ϣ�ļ�
** �䡡��  : photplugfil      �Ȳ����Ϣ�ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _hotplugRead (PLW_HOTPLUG_FILE  photplugfil, 
                              PCHAR             pcBuffer, 
                              size_t            stMaxBytes)
{
    ULONG      ulErrCode;
    ULONG      ulTimeout;
    size_t     stMsgLen;
    ssize_t    sstRet;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }
    
    if (photplugfil->HOTPFIL_iFlag & O_NONBLOCK) {                      /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }

    for (;;) {
        ulErrCode = API_SemaphoreBPend(photplugfil->HOTPFIL_ulReadSync, /*  �ȴ�������Ч                */
                                       ulTimeout);
        if (ulErrCode != ERROR_NONE) {                                  /*  ��ʱ                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        HOTPLUG_DEV_LOCK();
        stMsgLen = (size_t)_bmsgNBytesNext(photplugfil->HOTPFIL_pbmsg);
        if (stMsgLen > stMaxBytes) {
            HOTPLUG_DEV_UNLOCK();
            API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
            _ErrorHandle(EMSGSIZE);                                     /*  ������̫С                  */
            return  (PX_ERROR);
        
        } else if (stMsgLen) {
            break;                                                      /*  ���ݿɶ�                    */
        }
        HOTPLUG_DEV_UNLOCK();
    }
    
    sstRet = (ssize_t)_bmsgGet(photplugfil->HOTPFIL_pbmsg, pcBuffer, stMaxBytes);
    
    if (!_bmsgIsEmpty(photplugfil->HOTPFIL_pbmsg)) {
        API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
    }
    
    HOTPLUG_DEV_UNLOCK();
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: _hotplugWrite
** ��������: д�Ȳ����Ϣ�ļ�
** �䡡��  : photplugfil      �Ȳ����Ϣ�ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _hotplugWrite (PLW_HOTPLUG_FILE  photplugfil, 
                               PCHAR             pcBuffer, 
                               size_t            stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _hotplugIoctl
** ��������: �����Ȳ����Ϣ�ļ�
** �䡡��  : photplugfil      �Ȳ����Ϣ�ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _hotplugIoctl (PLW_HOTPLUG_FILE  photplugfil, 
                           INT               iRequest, 
                           LONG              lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    struct stat         *pstatGet;
    PLW_BMSG             pbmsg;

    switch (iRequest) {
    
    case FIONREAD:
        HOTPLUG_DEV_LOCK();
        *(INT *)lArg = _bmsgNBytes(photplugfil->HOTPFIL_pbmsg);
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIONMSGS:
        HOTPLUG_DEV_LOCK();
        if (!_bmsgIsEmpty(photplugfil->HOTPFIL_pbmsg)) {
            *(INT *)lArg = 1;                                           /*  Ŀǰ�ݲ�֪ͨ������Ϣ����    */
        } else {
            *(INT *)lArg = 0;
        }
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIONBIO:
        HOTPLUG_DEV_LOCK();
        if (*(INT *)lArg) {
            photplugfil->HOTPFIL_iFlag |= O_NONBLOCK;
        } else {
            photplugfil->HOTPFIL_iFlag &= ~O_NONBLOCK;
        }
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIORFLUSH:
    case FIOFLUSH:
        HOTPLUG_DEV_LOCK();
        _bmsgFlush(photplugfil->HOTPFIL_pbmsg);
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIORBUFSET:
        if (lArg < LW_HOTPLUG_DEV_MAX_MSGSIZE) {
            _ErrorHandle(EMSGSIZE);
            return  (PX_ERROR);
        }
        pbmsg = _bmsgCreate((size_t)lArg);
        if (pbmsg) {
            HOTPLUG_DEV_LOCK();
            _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
            photplugfil->HOTPFIL_pbmsg = pbmsg;
            HOTPLUG_DEV_UNLOCK();
        } else {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        break;
        
    case LW_HOTPLUG_FIOSETMSG:
        photplugfil->HOTPFIL_iMsg = (INT)lArg;
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_hotplugdev.HOTPDEV_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0444 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)_bmsgSizeGet(photplugfil->HOTPFIL_pbmsg);
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&photplugfil->HOTPFIL_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if ((photplugfil->HOTPFIL_pbmsg == LW_NULL) ||
                _bmsgNBytes(photplugfil->HOTPFIL_pbmsg)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
        case SELEXCEPT:                                                 /*  ���˳�                      */
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&photplugfil->HOTPFIL_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
