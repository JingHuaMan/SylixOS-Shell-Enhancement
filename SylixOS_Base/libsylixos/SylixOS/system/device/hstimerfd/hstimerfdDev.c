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
** ��   ��   ��: hstimerfdDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 03 ��
**
** ��        ��: ��Ƶ�ʶ�ʱ���豸.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_PTIMER_EN > 0) && (LW_CFG_TIMERFD_EN > 0)
#include "sys/hstimerfd.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iHstmrfdDrvNum = PX_ERROR;
static LW_HSTMRFD_DEV   _G_hstmrfddev;
static LW_OBJECT_HANDLE _G_hHstmrfdSelMutex;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID     _hstmrfdCallback(PLW_HSTMRFD_FILE  phstmrfdfil);
static LONG     _hstmrfdOpen(PLW_HSTMRFD_DEV    phstmrfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _hstmrfdClose(PLW_HSTMRFD_FILE  phstmrfdfil);
static ssize_t  _hstmrfdRead(PLW_HSTMRFD_FILE   phstmrfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _hstmrfdIoctl(PLW_HSTMRFD_FILE  phstmrfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
  ʱ��任
*********************************************************************************************************/
static LW_INLINE  VOID   __hstickToTimespec (ULONG  ulHsticks, struct timespec  *ptv)
{
    ptv->tv_sec  = (time_t)(ulHsticks / LW_HTIMER_HZ);
    ptv->tv_nsec = (LONG)(ulHsticks % LW_HTIMER_HZ) 
                 * ((1000 * 1000 * 1000) / LW_HTIMER_HZ);
}
static LW_INLINE  ULONG  __timespecToHstick (const struct timespec  *ptv)
{
    REGISTER ULONG     ulHsticks;
    
    ulHsticks  = (ULONG)(ptv->tv_sec * LW_HTIMER_HZ);
    ulHsticks += (((((ptv->tv_nsec / 1000) * LW_HTIMER_HZ) / 100) / 100) / 100);
    
    return  (ulHsticks);
}
/*********************************************************************************************************
** ��������: API_HstimerfdDrvInstall
** ��������: ��װ hstimerfd �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_HstimerfdDrvInstall (VOID)
{
    if (_G_iHstmrfdDrvNum <= 0) {
        _G_iHstmrfdDrvNum  = iosDrvInstall(LW_NULL,
                                           LW_NULL,
                                           _hstmrfdOpen,
                                           _hstmrfdClose,
                                           _hstmrfdRead,
                                           LW_NULL,
                                           _hstmrfdIoctl);
        DRIVER_LICENSE(_G_iHstmrfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iHstmrfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iHstmrfdDrvNum, "hstimerfd driver.");
    }
    
    if (_G_hHstmrfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hHstmrfdSelMutex =  API_SemaphoreMCreate("hstmrfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                    LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    }
    
    return  ((_G_iHstmrfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_HstimerfdDevCreate
** ��������: ��װ hstimerfd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_HstimerfdDevCreate (VOID)
{
    if (_G_iHstmrfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_hstmrfddev.HD_devhdrHdr, LW_HSTMRFD_DEV_PATH, 
                    _G_iHstmrfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hstimerfd_hz
** ��������: ���ϵͳ���ٶ�ʱ������Ƶ��
** �䡡��  : NONE
** �䡡��  : Ƶ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_hz (void)
{
    return  (LW_HTIMER_HZ);
}
/*********************************************************************************************************
** ��������: hstimerfd_create
** ��������: �� hstimerfd �ļ�
** �䡡��  : flags         �򿪱�־ HSTFD_NONBLOCK / HSTFD_CLOEXEC
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_create (int flags)
{
    flags &= (HSTFD_NONBLOCK | HSTFD_CLOEXEC);
    
    return  (open(LW_HSTMRFD_DEV_PATH, O_RDONLY | flags));
}
/*********************************************************************************************************
** ��������: hstimerfd_settime
** ��������: ���� hstimerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           ntmr          �µĶ�ʱʱ��
**           otmr          ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_settime (int fd, const struct itimerspec *ntmr, struct itimerspec *otmr)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    ULONG             ulOption;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!ntmr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(phstmrfdfil->HF_ulReadLock);
    
    if (otmr) {
        ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                                  LW_NULL, &ulCounter, &ulInterval);
        if (ulError) {
            return  (PX_ERROR);
        }
        if (bIsRunning == LW_FALSE) {
            otmr->it_interval.tv_sec  = 0;
            otmr->it_interval.tv_nsec = 0;
            otmr->it_value.tv_sec     = 0;
            otmr->it_value.tv_nsec    = 0;
        
        } else {
            __hstickToTimespec(ulInterval, &otmr->it_interval);
            __hstickToTimespec(ulCounter, &otmr->it_value);
        }
    }
    
    if (ntmr->it_value.tv_sec  == 0 &&
        ntmr->it_value.tv_nsec == 0) {
        API_TimerCancel(phstmrfdfil->HF_ulTimer);
        return  (ERROR_NONE);
    }
    
    if (ntmr->it_interval.tv_sec  == 0 &&
        ntmr->it_interval.tv_nsec == 0) {                               /*  ONE SHOT                    */
        ulOption = LW_OPTION_MANUAL_RESTART;
    } else {
        ulOption = LW_OPTION_AUTO_RESTART;                              /*  AUTO RELOAD                 */
    }
    
    ulCounter  = __timespecToHstick(&ntmr->it_value);
    ulInterval = __timespecToHstick(&ntmr->it_interval);
    
    ulCounter  = (ulCounter  == 0) ? 1 : ulCounter;
    ulInterval = (ulInterval == 0) ? 1 : ulInterval;
    
    ulError = API_TimerStartEx(phstmrfdfil->HF_ulTimer, 
                               ulCounter,
                               ulInterval,
                               ulOption,
                               (PTIMER_CALLBACK_ROUTINE)_hstmrfdCallback,
                               (PVOID)phstmrfdfil);
    if (ulError) {
        return  (PX_ERROR);
    }
                     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hstimerfd_settime2
** ��������: ���� hstimerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           ncnt          �µĶ�ʱʱ��
**           ocnt          ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_settime2 (int fd, hstimer_cnt_t *ncnt, hstimer_cnt_t *ocnt)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    ULONG             ulOption;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!ncnt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(phstmrfdfil->HF_ulReadLock);
    
    if (ocnt) {
        ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                                  LW_NULL, &ulCounter, &ulInterval);
        if (ulError) {
            return  (PX_ERROR);
        }
        if (bIsRunning == LW_FALSE) {
            ocnt->interval = 0ul;
            ocnt->value    = 0ul;
        
        } else {
            ocnt->interval = ulInterval;
            ocnt->value    = ulCounter;
        }
    }
    
    if (ncnt->value == 0) {
        API_TimerCancel(phstmrfdfil->HF_ulTimer);
        return  (ERROR_NONE);
    }
    
    if (ncnt->interval == 0) {                                          /*  ONE SHOT                    */
        ulOption = LW_OPTION_MANUAL_RESTART;
    } else {
        ulOption = LW_OPTION_AUTO_RESTART;                              /*  AUTO RELOAD                 */
    }
    
    ulError = API_TimerStartEx(phstmrfdfil->HF_ulTimer, 
                               ncnt->value,
                               ncnt->interval,
                               ulOption,
                               (PTIMER_CALLBACK_ROUTINE)_hstmrfdCallback,
                               (PVOID)phstmrfdfil);
    if (ulError) {
        return  (PX_ERROR);
    }
                     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hstimerfd_gettime
** ��������: ��ȡ hstimerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           currvalue     ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_gettime (int fd, struct itimerspec *currvalue)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!currvalue) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                              LW_NULL, &ulCounter, &ulInterval);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    if (bIsRunning == LW_FALSE) {
        currvalue->it_interval.tv_sec  = 0;
        currvalue->it_interval.tv_nsec = 0;
        currvalue->it_value.tv_sec     = 0;
        currvalue->it_value.tv_nsec    = 0;
        return  (ERROR_NONE);
    }
    
    __hstickToTimespec(ulInterval, &currvalue->it_interval);
    __hstickToTimespec(ulCounter, &currvalue->it_value);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: hstimerfd_gettime2
** ��������: ��ȡ hstimerfd �ļ���ʱʱ��
** �䡡��  : fd            �ļ�������
**           currvalue     ��ȡ��ǰ�Ķ�ʱʱ��
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  hstimerfd_gettime2 (int fd, hstimer_cnt_t *currvalue)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!currvalue) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                              LW_NULL, &ulCounter, &ulInterval);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    if (bIsRunning == LW_FALSE) {
        currvalue->interval = 0ul;
        currvalue->value    = 0ul;
        return  (ERROR_NONE);
    }
    
    currvalue->interval = ulInterval;
    currvalue->value    = ulCounter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _hstmrfdCallback
** ��������: hstmrfd �ļ���ʱ�ص�
** �䡡��  : phstmrfdfil      hstimerfd �ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _hstmrfdCallback (PLW_HSTMRFD_FILE  phstmrfdfil)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TIMER ptmr = (PLW_CLASS_TIMER)phstmrfdfil->HF_pvTimer;

    LW_SPIN_LOCK_QUICK(&ptmr->TIMER_slLock, &iregInterLevel);
    ptmr->TIMER_u64Overrun++;
    LW_SPIN_UNLOCK_QUICK(&ptmr->TIMER_slLock, iregInterLevel);
    
    API_SemaphoreBPost(phstmrfdfil->HF_ulReadLock);
    SEL_WAKE_UP_ALL(&phstmrfdfil->HF_selwulist, SELREAD);
}
/*********************************************************************************************************
** ��������: _hstmrfdOpen
** ��������: �� hstimerfd �豸
** �䡡��  : phstmrfddev      hstimerfd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _hstmrfdOpen (PLW_HSTMRFD_DEV phstmrfddev, 
                           PCHAR           pcName,
                           INT             iFlags, 
                           INT             iMode)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        phstmrfdfil = (PLW_HSTMRFD_FILE)__SHEAP_ALLOC(sizeof(LW_HSTMRFD_FILE));
        if (!phstmrfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        phstmrfdfil->HF_uiMagic = LW_HSTIMER_FILE_MAGIC;
        phstmrfdfil->HF_iFlag   = iFlags;
        
        phstmrfdfil->HF_ulTimer = API_TimerCreate("hstmrfd", 
                                                  LW_OPTION_HTIMER | 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (phstmrfdfil->HF_ulTimer == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(phstmrfdfil);
            return  (PX_ERROR);
        }
        
        phstmrfdfil->HF_pvTimer = (PVOID)&_K_tmrBuffer[_ObjectGetIndex(phstmrfdfil->HF_ulTimer)];

        phstmrfdfil->HF_ulReadLock = API_SemaphoreBCreate("hstmrfd_rlock", LW_FALSE, 
                                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (phstmrfdfil->HF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            API_TimerDelete(&phstmrfdfil->HF_ulTimer);
            __SHEAP_FREE(phstmrfdfil);
            return  (PX_ERROR);
        }
        
        lib_bzero(&phstmrfdfil->HF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        phstmrfdfil->HF_selwulist.SELWUL_hListLock = _G_hHstmrfdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_hstmrfddev.HD_devhdrHdr);
        
        return  ((LONG)phstmrfdfil);
    }
}
/*********************************************************************************************************
** ��������: _hstmrfdClose
** ��������: �ر� hstimerfd �ļ�
** �䡡��  : phstmrfdfil       hstimerfd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _hstmrfdClose (PLW_HSTMRFD_FILE  phstmrfdfil)
{
    if (phstmrfdfil) {
        SEL_WAKE_UP_TERM(&phstmrfdfil->HF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_hstmrfddev.HD_devhdrHdr);
        
        API_TimerDelete(&phstmrfdfil->HF_ulTimer);
        API_SemaphoreBDelete(&phstmrfdfil->HF_ulReadLock);
        __SHEAP_FREE(phstmrfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _hstmrfdRead
** ��������: �� hstimerfd �豸
** �䡡��  : phstmrfdfil      hstimerfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �����Ǹ��ٶ�ʱ��, ���������ȡ��ʱ���ṹʱû�м����ں���, 
             ���Ա��뱣֤�ļ��ڶ���ʱ���ܱ��ر�.
*********************************************************************************************************/
static ssize_t  _hstmrfdRead (PLW_HSTMRFD_FILE  phstmrfdfil, 
                              PCHAR             pcBuffer, 
                              size_t            stMaxBytes)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TIMER ptmr;
    ULONG           ulLwErrCode;
    UINT64          u64Overruns;
    ULONG           ulTimeout;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes < sizeof(UINT64)) {
        return  (0);
    }

    if (phstmrfdfil->HF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    ulLwErrCode = API_SemaphoreBPend(phstmrfdfil->HF_ulReadLock, ulTimeout);
    if (ulLwErrCode != ERROR_NONE) {                                    /*  ��ʱ                        */
        _ErrorHandle(EAGAIN);
        return  (0);
    }
    
    ptmr = (PLW_CLASS_TIMER)phstmrfdfil->HF_pvTimer;
    
    LW_SPIN_LOCK_QUICK(&ptmr->TIMER_slLock, &iregInterLevel);
    u64Overruns = ptmr->TIMER_u64Overrun;
    ptmr->TIMER_u64Overrun = 0ull;
    LW_SPIN_UNLOCK_QUICK(&ptmr->TIMER_slLock, iregInterLevel);
    
    lib_memcpy(pcBuffer, &u64Overruns, sizeof(UINT64));
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** ��������: _hstmrfdIoctl
** ��������: ���� hstimerfd �ļ�
** �䡡��  : phstmrfdfil      hstimerfd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _hstmrfdIoctl (PLW_HSTMRFD_FILE  phstmrfdfil, 
                           INT               iRequest, 
                           LONG              lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    BOOL                 bExpired = LW_FALSE;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            phstmrfdfil->HF_iFlag |= O_NONBLOCK;
        } else {
            phstmrfdfil->HF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_hstmrfddev.HD_devhdrHdr);
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
        SEL_WAKE_NODE_ADD(&phstmrfdfil->HF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            API_SemaphoreBStatus(phstmrfdfil->HF_ulReadLock, &bExpired, LW_NULL, LW_NULL);
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
        SEL_WAKE_NODE_DELETE(&phstmrfdfil->HF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
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
