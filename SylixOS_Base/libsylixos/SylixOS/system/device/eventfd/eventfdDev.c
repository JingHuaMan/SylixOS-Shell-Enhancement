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
** ��   ��   ��: eventfdDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 20 ��
**
** ��        ��: Linux ���� eventfd ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_EVENTFD_EN > 0)
#include "sys/eventfd.h"
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define LW_EVENTFD_MAX_CNT  0xffffffffffffffffull
/*********************************************************************************************************
  check can read/write
*********************************************************************************************************/
static LW_INLINE BOOL  __evtfd_can_read (PLW_EVTFD_FILE  pevtfdfil)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
    if (pevtfdfil->EF_u64Counter) {
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
        return  (LW_TRUE);
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    return  (LW_FALSE);
}

static LW_INLINE BOOL  __evtfd_can_write (PLW_EVTFD_FILE  pevtfdfil)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
    if ((LW_EVENTFD_MAX_CNT - pevtfdfil->EF_u64Counter) > 1) {
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
        return  (LW_TRUE);
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iEvtfdDrvNum = PX_ERROR;
static LW_EVTFD_DEV     _G_evtfddev;
static LW_OBJECT_HANDLE _G_hEvtfdSelMutex;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _evtfdOpen(PLW_EVTFD_DEV    pevtfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _evtfdClose(PLW_EVTFD_FILE  pevtfdfil);
static ssize_t  _evtfdRead(PLW_EVTFD_FILE   pevtfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _evtfdWrite(PLW_EVTFD_FILE  pevtfdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _evtfdIoctl(PLW_EVTFD_FILE  pevtfdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
** ��������: API_EventfdDrvInstall
** ��������: ��װ eventfd �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_EventfdDrvInstall (VOID)
{
    if (_G_iEvtfdDrvNum <= 0) {
        _G_iEvtfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _evtfdOpen,
                                         _evtfdClose,
                                         _evtfdRead,
                                         _evtfdWrite,
                                         _evtfdIoctl);
        DRIVER_LICENSE(_G_iEvtfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iEvtfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iEvtfdDrvNum, "eventfd driver.");
    }
    
    if (_G_hEvtfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hEvtfdSelMutex =  API_SemaphoreMCreate("evtfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    
    return  ((_G_iEvtfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_EventfdDevCreate
** ��������: ��װ eventfd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_EventfdDevCreate (VOID)
{
    if (_G_iEvtfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_evtfddev.ED_devhdrHdr, LW_EVTFD_DEV_PATH, 
                    _G_iEvtfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: eventfd
** ��������: �� eventfd �ļ�
** �䡡��  : initval   ��ʼ��ֵ
**           flags     �򿪱�־ EFD_SEMAPHORE / EFD_CLOEXEC / EFD_NONBLOCK
** �䡡��  : eventfd �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  eventfd (unsigned int initval, int flags)
{
    INT             iFd;
    PLW_EVTFD_FILE  pevtfdfil;
    
    flags &= (EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK);
    
    iFd = open(LW_EVTFD_DEV_PATH, O_RDWR | flags);
    if (iFd >= 0) {
        pevtfdfil = (PLW_EVTFD_FILE)API_IosFdValue(iFd);
        pevtfdfil->EF_u64Counter = (UINT64)initval;
        if (pevtfdfil->EF_u64Counter) {
            API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: eventfd_read
** ��������: ��ȡ eventfd �ļ�
** �䡡��  : fd        �ļ�������
**           value     ��ȡ����
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  eventfd_read (int fd, eventfd_t *value)
{
    return  (read(fd, value, sizeof(eventfd_t)) != sizeof(eventfd_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** ��������: eventfd_write
** ��������: д eventfd �ļ�
** �䡡��  : fd        �ļ�������
**           value     д������
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  eventfd_write (int fd, eventfd_t value)
{
    return  (write(fd, &value, sizeof(eventfd_t)) != sizeof(eventfd_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _evtfdOpen
** ��������: �� eventfd �豸
** �䡡��  : pevtfddev        eventfd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _evtfdOpen (PLW_EVTFD_DEV pevtfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_EVTFD_FILE  pevtfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        pevtfdfil = (PLW_EVTFD_FILE)__SHEAP_ALLOC(sizeof(LW_EVTFD_FILE));
        if (!pevtfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pevtfdfil->EF_iFlag      = iFlags;
        pevtfdfil->EF_ulReadLock = API_SemaphoreBCreate("evtfd_rlock", LW_FALSE, 
                                                        LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pevtfdfil->EF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(pevtfdfil);
            return  (PX_ERROR);
        }
        pevtfdfil->EF_ulWriteLock = API_SemaphoreBCreate("evtfd_wlock", LW_TRUE, 
                                                         LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pevtfdfil->EF_ulWriteLock == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreBDelete(&pevtfdfil->EF_ulReadLock);
            __SHEAP_FREE(pevtfdfil);
            return  (PX_ERROR);
        }
        
        pevtfdfil->EF_u64Counter = 0ull;
        
        lib_bzero(&pevtfdfil->EF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        pevtfdfil->EF_selwulist.SELWUL_hListLock = _G_hEvtfdSelMutex;
        
        LW_SPIN_INIT(&pevtfdfil->EF_slLock);
        
        LW_DEV_INC_USE_COUNT(&_G_evtfddev.ED_devhdrHdr);
        
        return  ((LONG)pevtfdfil);
    }
}
/*********************************************************************************************************
** ��������: _evtfdClose
** ��������: �ر� eventfd �ļ�
** �䡡��  : pevtfdfil         eventfd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _evtfdClose (PLW_EVTFD_FILE  pevtfdfil)
{
    if (pevtfdfil) {
        SEL_WAKE_UP_TERM(&pevtfdfil->EF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_evtfddev.ED_devhdrHdr);
        
        API_SemaphoreBDelete(&pevtfdfil->EF_ulReadLock);
        API_SemaphoreBDelete(&pevtfdfil->EF_ulWriteLock);
        __SHEAP_FREE(pevtfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _evtfdRead
** ��������: �� eventfd �豸
** �䡡��  : pevtfdfil        eventfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _evtfdRead (PLW_EVTFD_FILE  pevtfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    INTREG  iregInterLevel;
    ULONG   ulLwErrCode;
    ULONG   ulTimeout;
    BOOL    bRelease = LW_FALSE;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes < sizeof(UINT64)) {
        return  (0);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �Ƿ����ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  �������ж��е���            */
    }
    
    if (pevtfdfil->EF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    for (;;) {
        ulLwErrCode = API_SemaphoreBPend(pevtfdfil->EF_ulReadLock, ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  ��ʱ                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
        if (pevtfdfil->EF_u64Counter) {
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    }
    
    if (pevtfdfil->EF_iFlag & EFD_SEMAPHORE) {                          /*  EFD_SEMAPHORE               */
        UINT64  u64One = 1;
        lib_memcpy(pcBuffer, &u64One, sizeof(UINT64));                  /*  host byte order             */
        pevtfdfil->EF_u64Counter--;
    
    } else {
        lib_memcpy(pcBuffer, &pevtfdfil->EF_u64Counter, sizeof(UINT64));
        pevtfdfil->EF_u64Counter = 0;
    }
    
    if (pevtfdfil->EF_u64Counter) {
        bRelease = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    if (bRelease) {
        API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELREAD);
    }
    
    API_SemaphoreBPost(pevtfdfil->EF_ulWriteLock);
    SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELWRITE);
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** ��������: _evtfdWrite
** ��������: д eventfd �豸
** �䡡��  : pevtfdfil        eventfd �ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _evtfdWrite (PLW_EVTFD_FILE pevtfdfil, 
                             PCHAR          pcBuffer, 
                             size_t         stNBytes)
{
    INTREG  iregInterLevel;
    UINT64  u64Add;
    ULONG   ulLwErrCode;
    ULONG   ulTimeout;
    BOOL    bRelease = LW_FALSE;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stNBytes < sizeof(UINT64)) {
        return  (0);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �Ƿ����ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  �������ж��е���            */
    }
    
    lib_memcpy(&u64Add, pcBuffer, sizeof(UINT64));
    
    if (u64Add == LW_EVENTFD_MAX_CNT) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pevtfdfil->EF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    for (;;) {
        ulLwErrCode = API_SemaphoreBPend(pevtfdfil->EF_ulWriteLock, ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  ��ʱ                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
        if ((LW_EVENTFD_MAX_CNT - u64Add) > pevtfdfil->EF_u64Counter) { /*  ���ܲ������                */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    }
    
    pevtfdfil->EF_u64Counter += u64Add;
    if (pevtfdfil->EF_u64Counter) {
        bRelease = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    if (bRelease) {
        API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELREAD);
    }
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** ��������: _evtfdIoctl
** ��������: ���� eventfd �ļ�
** �䡡��  : pevtfdfil        eventfd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _evtfdIoctl (PLW_EVTFD_FILE  pevtfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pevtfdfil->EF_iFlag |= O_NONBLOCK;
        } else {
            pevtfdfil->EF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_evtfddev.ED_devhdrHdr);
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
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&pevtfdfil->EF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if (__evtfd_can_read(pevtfdfil)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
            if (__evtfd_can_write(pevtfdfil)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELEXCEPT:
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pevtfdfil->EF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_EVENTFD_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
