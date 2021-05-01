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
** ��   ��   ��: hwrtc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 01 �� 04 ��
**
** ��        ��: Ӳ�� RTC �豸�������. (ע��: Ӳ�� RTC �ӿ�Ӧ��Ϊ UTC ʱ��)

** BUG:
2010.09.11  �����豸ʱ, ָ���豸����.
2011.06.11  ���� API_RtcToRoot() ����.
2012.03.11  __rootFsTimeSet() Ϊ UTC ʱ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_RTC_EN > 0)
/*********************************************************************************************************
  RTC �豸��
*********************************************************************************************************/
#define __LW_RTC_DEV_NAME               "/dev/rtc"
/*********************************************************************************************************
  RTC �豸
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           RTCDEV_devhdr;                                 /*  �豸ͷ                      */
    PLW_RTC_FUNCS        RTCDEV_prtcfuncs;                              /*  ����������                  */
} LW_RTC_DEV;
typedef LW_RTC_DEV      *PLW_RTC_DEV;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     __rtcOpen(PLW_RTC_DEV    prtcdev, 
                          PCHAR          pcName,   
                          INT            iFlags, 
                          INT            iMode);
static INT      __rtcClose(PLW_RTC_DEV    prtcdev);
static INT      __rtcIoctl(PLW_RTC_DEV    prtcdev, INT  iCmd, PVOID  pvArg);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static INT      _G_iRtcDrvNum = PX_ERROR;
/*********************************************************************************************************
** ��������: API_RtcDrvInstall
** ��������: ��װ RTC �豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcDrvInstall (VOID)
{
    if (_G_iRtcDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iRtcDrvNum = iosDrvInstall(__rtcOpen, 
                                  (FUNCPTR)LW_NULL, 
                                  __rtcOpen,
                                  __rtcClose,
                                  LW_NULL,
                                  LW_NULL,
                                  __rtcIoctl);
    DRIVER_LICENSE(_G_iRtcDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRtcDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRtcDrvNum, "hardware rtc.");
    
    return  ((_G_iRtcDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_RtcDevCreate
** ��������: ����һ�� RTC �豸
** �䡡��  : prtcfuncs     rtc ����������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcDevCreate (PLW_RTC_FUNCS    prtcfuncs)
{
    PLW_RTC_DEV     prtcdev;
    
    if (prtcfuncs == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iRtcDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    prtcdev = (PLW_RTC_DEV)__SHEAP_ALLOC(sizeof(LW_RTC_DEV));
    if (prtcdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(prtcdev, sizeof(LW_RTC_DEV));
    
    prtcdev->RTCDEV_prtcfuncs = prtcfuncs;
    
    if (iosDevAddEx(&prtcdev->RTCDEV_devhdr, __LW_RTC_DEV_NAME, _G_iRtcDrvNum, DT_CHR) != ERROR_NONE) {
        __SHEAP_FREE(prtcdev);
        return  (PX_ERROR);
    }
    
    if (prtcfuncs->RTC_pfuncInit) {
        prtcfuncs->RTC_pfuncInit();                                     /*  ��ʼ��Ӳ�� RTC              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtcOpen
** ��������: �� RTC �豸
** �䡡��  : prtcdev          rtc �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : �豸
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __rtcOpen (PLW_RTC_DEV    prtcdev, 
                        PCHAR          pcName,   
                        INT            iFlags, 
                        INT            iMode)
{
    LW_DEV_INC_USE_COUNT(&prtcdev->RTCDEV_devhdr);
    
    return  ((LONG)prtcdev);
}
/*********************************************************************************************************
** ��������: __rtcClose
** ��������: �ر� RTC �豸
** �䡡��  : prtcdev          rtc �豸
** �䡡��  : �豸
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtcClose (PLW_RTC_DEV    prtcdev)
{
    if (prtcdev) {
        LW_DEV_DEC_USE_COUNT(&prtcdev->RTCDEV_devhdr);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __rtcIoctl
** ��������: ���� RTC �豸
** �䡡��  : prtcdev          rtc �豸
**           iCmd             ��������
**           pvArg            ����
** �䡡��  : �豸
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtcIoctl (PLW_RTC_DEV    prtcdev, INT  iCmd, PVOID  pvArg)
{
    struct stat *pstat;

    switch (iCmd) {
    
    case FIOGETTIME:                                                    /*  ��� rtc ʱ��               */
        if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncGet) {
            return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncGet(prtcdev->RTCDEV_prtcfuncs,
                                                            (time_t *)pvArg);
        }
        break;
        
    case FIOSETTIME:                                                    /*  ���� rtc ʱ��               */
        if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncSet) {
            return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncSet(prtcdev->RTCDEV_prtcfuncs,
                                                            (time_t *)pvArg);
        }
        break;
        
    case FIOFSTATGET:                                                   /*  ����ļ�����                */
        pstat = (struct stat *)pvArg;
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&prtcdev->RTCDEV_devhdr);
        pstat->st_ino     = (ino_t)0;                                   /*  �൱��Ψһ�ڵ�              */
        pstat->st_mode    = 0644 | S_IFCHR;                             /*  Ĭ������                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        return  (ERROR_NONE);
        
    default:
        break;
    }
    
    if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncIoctl) {
        return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncIoctl(prtcdev->RTCDEV_prtcfuncs,
                                                          iCmd,
                                                          pvArg);
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_RtcSet
** ��������: ����Ӳ�� RTC �豸ʱ��: UTC ʱ��
** �䡡��  : time          ʱ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcSet (time_t  time)
{
    INT     iFd = open(__LW_RTC_DEV_NAME, O_WRONLY);
    INT     iError;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOSETTIME, &time);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_RtcGet
** ��������: ��ȡӲ�� RTC �豸ʱ��: UTC ʱ��
** �䡡��  : ptime          ʱ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcGet (time_t  *ptime)
{
    INT     iFd = open(__LW_RTC_DEV_NAME, O_RDONLY);
    INT     iError;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOGETTIME, ptime);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SysToRtc
** ��������: ��ϵͳʱ��ͬ���� RTC �豸
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SysToRtc (VOID)
{
    struct timespec   tv;
    
    if (lib_clock_gettime(CLOCK_REALTIME, &tv) < 0) {                   /*  ���ϵͳʱ��                */
        return  (PX_ERROR);
    }
    
    return  (API_RtcSet(tv.tv_sec));                                    /*  ����Ӳ�� RTC                */
}
/*********************************************************************************************************
** ��������: API_RtcToSys
** ��������: �� RTC �豸ͬ����ϵͳʱ��
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcToSys (VOID)
{
    struct timespec   tv;
    
    if (API_RtcGet(&tv.tv_sec) < 0) {
        return  (PX_ERROR);
    }
    tv.tv_nsec = 0;
    
    return  (lib_clock_settime(CLOCK_REALTIME, &tv));                   /*  ����ϵͳʱ��                */
}
/*********************************************************************************************************
** ��������: API_RtcToRoot
** ��������: ���õ�ǰ RTC ʱ��Ϊ���ļ�ϵͳ��׼ʱ��
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RtcToRoot (VOID)
{
VOID  __rootFsTimeSet(time_t  *time);

    time_t      timeNow;
    
    if (API_RtcGet(&timeNow) < 0) {
        return  (PX_ERROR);
    }
    
    __rootFsTimeSet(&timeNow);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_RTC_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
