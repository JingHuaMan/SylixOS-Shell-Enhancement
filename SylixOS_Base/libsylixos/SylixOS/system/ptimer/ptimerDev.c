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
** ��   ��   ��: ptimerDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 20 ��
**
** ��        ��: Linux ���� timerfd ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_PTIMER_EN > 0) && (LW_CFG_TIMERFD_EN > 0)
#include "sys/timerfd.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iTmrfdDrvNum = PX_ERROR;
static LW_TMRFD_DEV     _G_tmrfddev;
static LW_OBJECT_HANDLE _G_hTmrfdSelMutex;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _tmrfdOpen(PLW_TMRFD_DEV    ptmrfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _tmrfdClose(PLW_TMRFD_FILE  ptmrfdfil);
static ssize_t  _tmrfdRead(PLW_TMRFD_FILE   ptmrfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _tmrfdIoctl(PLW_TMRFD_FILE  ptmrfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
** ��������: API_TimerfdDrvInstall
** ��������: ��װ timerfd �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TimerfdDrvInstall (VOID)
{
    if (_G_iTmrfdDrvNum <= 0) {
        _G_iTmrfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _tmrfdOpen,
                                         _tmrfdClose,
                                         _tmrfdRead,
                                         LW_NULL,
                                         _tmrfdIoctl);
        DRIVER_LICENSE(_G_iTmrfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iTmrfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iTmrfdDrvNum, "timerfd driver.");
    }
    
    if (_G_hTmrfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hTmrfdSelMutex =  API_SemaphoreMCreate("tmrfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    
    return  ((_G_iTmrfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_TimerfdDevCreate
** ��������: ��װ timerfd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TimerfdDevCreate (VOID)
{
    if (_G_iTmrfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_tmrfddev.TD_devhdrHdr, LW_TMRFD_DEV_PATH, 
                    _G_iTmrfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: timerfd_create
** ��������: �� timerfd �ļ�
** �䡡��  : clockid       ʱ��Դ CLOCK_REALTIME / CLOCK_MONOTONIC
**           flags         �򿪱�־ TFD_NONBLOCK / TFD_CLOEXEC
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  timerfd_create (int clockid, int flags)
{
    INT             iFd;
    INT             iError;
    PLW_TMRFD_FILE  ptmrfdfil;
    
    flags &= (TFD_NONBLOCK | TFD_CLOEXEC);
    
    iFd = open(LW_TMRFD_DEV_PATH, O_RDONLY | flags);
    if (iFd >= 0) {
        ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(iFd);
        iError = timer_create_internal(clockid, LW_NULL, 
                                       &ptmrfdfil->TF_timer, 
                                       LW_OPTION_OBJECT_GLOBAL);        /*  timer ����Ϊȫ�ֶ���        */
        if (iError) {
            close(iFd);
            return  (PX_ERROR);
        }
        timer_setfile(ptmrfdfil->TF_timer, (PVOID)ptmrfdfil);
        ptmrfdfil->TF_uiMagic = LW_TIMER_FILE_MAGIC;                    /*  ��ʼ�����                  */
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: timerfd_settime
** ��������: ���� timerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           flags         ���ñ�־ 0 / TFD_TIMER_ABSTIME
**           ntmr          �µĶ�ʱʱ��
**           otmr          ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  timerfd_settime (int fd, int flags, const struct itimerspec *ntmr, struct itimerspec *otmr)
{
    PLW_TMRFD_FILE  ptmrfdfil;
    
    if (!ntmr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((flags != 0) && (flags != TFD_TIMER_ABSTIME)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(fd);
    if (!ptmrfdfil || (ptmrfdfil->TF_uiMagic != LW_TIMER_FILE_MAGIC)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(ptmrfdfil->TF_ulReadLock);
    
    return  (timer_settime(ptmrfdfil->TF_timer, flags, ntmr, otmr));
}
/*********************************************************************************************************
** ��������: timerfd_gettime
** ��������: ��ȡ timerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           currvalue     ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  timerfd_gettime (int fd, struct itimerspec *currvalue)
{
    PLW_TMRFD_FILE  ptmrfdfil;
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(fd);
    if (!ptmrfdfil || (ptmrfdfil->TF_uiMagic != LW_TIMER_FILE_MAGIC)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (timer_gettime(ptmrfdfil->TF_timer, currvalue));
}
/*********************************************************************************************************
** ��������: _tmrfdCallback
** ��������: tmrfd �ļ���ʱ�ص�
** �䡡��  : ptmr             ��ʱ���ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _tmrfdCallback (PLW_CLASS_TIMER   ptmr)
{
    PLW_TMRFD_FILE  ptmrfdfil = (PLW_TMRFD_FILE)ptmr->TIMER_pvTimerfd;
    
    API_SemaphoreBPost(ptmrfdfil->TF_ulReadLock);
    SEL_WAKE_UP_ALL(&ptmrfdfil->TF_selwulist, SELREAD);
}
/*********************************************************************************************************
** ��������: _tmrfdOpen
** ��������: �� timerfd �豸
** �䡡��  : ptmrfddev        timerfd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _tmrfdOpen (PLW_TMRFD_DEV ptmrfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_TMRFD_FILE  ptmrfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        ptmrfdfil = (PLW_TMRFD_FILE)__SHEAP_ALLOC(sizeof(LW_TMRFD_FILE));
        if (!ptmrfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        ptmrfdfil->TF_uiMagic    = 0;
        ptmrfdfil->TF_iFlag      = iFlags;
        ptmrfdfil->TF_timer      = (timer_t)0ul;
        ptmrfdfil->TF_ulReadLock = API_SemaphoreBCreate("tmrfd_rlock", LW_FALSE, 
                                                        LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (ptmrfdfil->TF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(ptmrfdfil);
            return  (PX_ERROR);
        }
        
        lib_bzero(&ptmrfdfil->TF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        ptmrfdfil->TF_selwulist.SELWUL_hListLock = _G_hTmrfdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_tmrfddev.TD_devhdrHdr);
        
        return  ((LONG)ptmrfdfil);
    }
}
/*********************************************************************************************************
** ��������: _tmrfdClose
** ��������: �ر� timerfd �ļ�
** �䡡��  : ptmrfdfil         timerfd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _tmrfdClose (PLW_TMRFD_FILE  ptmrfdfil)
{
    if (ptmrfdfil) {
        SEL_WAKE_UP_TERM(&ptmrfdfil->TF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_tmrfddev.TD_devhdrHdr);
        
        if (ptmrfdfil->TF_timer) {
            timer_delete(ptmrfdfil->TF_timer);
            ptmrfdfil->TF_timer = (timer_t)0ul;
        }
        
        API_SemaphoreBDelete(&ptmrfdfil->TF_ulReadLock);
        __SHEAP_FREE(ptmrfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _tmrfdRead
** ��������: �� timerfd �豸
** �䡡��  : ptmrfdfil        timerfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _tmrfdRead (PLW_TMRFD_FILE  ptmrfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    ULONG   ulLwErrCode;
    UINT64  u64Overruns;
    ULONG   ulTimeout;

    if (!pcBuffer || (stMaxBytes < sizeof(UINT64))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �Ƿ����ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  �������ж��е���            */
    }
    
    if (ptmrfdfil->TF_timer == (timer_t)0ul) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (ptmrfdfil->TF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    ulLwErrCode = API_SemaphoreBPend(ptmrfdfil->TF_ulReadLock, ulTimeout);
    if (ulLwErrCode != ERROR_NONE) {                                    /*  ��ʱ                        */
        _ErrorHandle(EAGAIN);
        return  (0);
    }
    
    if (timer_getoverrun_64(ptmrfdfil->TF_timer, &u64Overruns, LW_TRUE)) {
        return  (PX_ERROR);
    }
    
    lib_memcpy(pcBuffer, &u64Overruns, sizeof(UINT64));
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** ��������: _tmrfdIoctl
** ��������: ���� timerfd �ļ�
** �䡡��  : ptmrfdfil        timerfd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _tmrfdIoctl (PLW_TMRFD_FILE  ptmrfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    BOOL                 bExpired = LW_FALSE;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            ptmrfdfil->TF_iFlag |= O_NONBLOCK;
        } else {
            ptmrfdfil->TF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_tmrfddev.TD_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0444 | S_IFCHR;
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
        SEL_WAKE_NODE_ADD(&ptmrfdfil->TF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            API_SemaphoreBStatus(ptmrfdfil->TF_ulReadLock, &bExpired, LW_NULL, LW_NULL);
            if (bExpired) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
        case SELEXCEPT:
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&ptmrfdfil->TF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
                                                                        /*  LW_CFG_TIMERFD_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
