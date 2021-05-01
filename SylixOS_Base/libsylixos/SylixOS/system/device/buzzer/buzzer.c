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
** ��   ��   ��: buzzer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 23 ��
**
** ��        ��: ��׼����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BUZZER_EN > 0)
/*********************************************************************************************************
  BUZZER �豸
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           BUZZER_devhdr;                                 /*  �豸ͷ                      */
    PLW_BUZZER_FUNCS     BUZZER_pbuzzerfuncs;                           /*  ����������                  */
    LW_OBJECT_HANDLE     BUZZER_ulThread;
    LW_OBJECT_HANDLE     BUZZER_ulMsgQ;
    time_t               BUZZER_timeCreate;
} LW_BUZZER_DEV;
typedef LW_BUZZER_DEV   *PLW_BUZZER_DEV;
/*********************************************************************************************************
  BUZZER �ļ�
*********************************************************************************************************/
typedef struct {
    PLW_BUZZER_DEV       BUZFIL_pbuzzer;
    INT                  BUZFIL_iFlags;
} LW_BUZZER_FILE;
typedef LW_BUZZER_FILE  *PLW_BUZZER_FILE;
/*********************************************************************************************************
  ����ȫ�ֱ���, ���ڱ��� buzzer ������
*********************************************************************************************************/
static INT _G_iBuzzerDrvNum = PX_ERROR;
/*********************************************************************************************************
** ��������: __buzzerOpen
** ��������: BUZZER �豸��
** �䡡��  : pbuzzer          �豸
**           pcName           �豸����
**           iFlags           ���豸ʱʹ�õı�־
**           iMode            �򿪵ķ�ʽ������
** �䡡��  : BUZZER �豸ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __buzzerOpen (PLW_BUZZER_DEV   pbuzzer,
                           PCHAR            pcName,
                           INT              iFlags,
                           INT              iMode)
{
    PLW_BUZZER_FILE  pbuzfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  �����ظ�����                */
            return  (PX_ERROR);
        }
    
        pbuzfil = (PLW_BUZZER_FILE)__SHEAP_ALLOC(sizeof(LW_BUZZER_FILE));
        if (!pbuzfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pbuzfil->BUZFIL_iFlags  = iFlags;
        pbuzfil->BUZFIL_pbuzzer = pbuzzer;
        
        LW_DEV_INC_USE_COUNT(&pbuzzer->BUZZER_devhdr);
        
        return  ((LONG)pbuzfil);
    }
}
/*********************************************************************************************************
** ��������: __buzzerClose
** ��������: BUZZER �豸�ر�
** �䡡��  : pbuzfil          �ļ�
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __buzzerClose (PLW_BUZZER_FILE   pbuzfil)
{
    PLW_BUZZER_DEV   pbuzzer;

    if (pbuzfil) {
        pbuzzer = pbuzfil->BUZFIL_pbuzzer;
    
        LW_DEV_DEC_USE_COUNT(&pbuzzer->BUZZER_devhdr);
            
        __SHEAP_FREE(pbuzfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __buzzerWrite
** ��������: д BUZZER �豸
** �䡡��  : pbuzfil          �ļ�
**           pbmsg            д������ָ��
**           stNBytes         ���ͻ������ֽ���
** �䡡��  : ����ʵ��д��ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __buzzerWrite (PLW_BUZZER_FILE   pbuzfil,
                              PBUZZER_MSG       pbmsg,
                              size_t            stNBytes)
{
    INT              iCnt;
    ssize_t          sstRet = 0;
    ULONG            ulTimeout;
    PLW_BUZZER_DEV   pbuzzer;
    
    if (!pbmsg || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iCnt = stNBytes / sizeof(BUZZER_MSG);
    if (iCnt % sizeof(BUZZER_MSG)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    if (pbuzfil->BUZFIL_iFlags & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    pbuzzer = pbuzfil->BUZFIL_pbuzzer;
    
    for (; iCnt > 0; iCnt--) {
        if (API_MsgQueueSend2(pbuzzer->BUZZER_ulThread,
                              pbmsg, sizeof(BUZZER_MSG),
                              ulTimeout)) {
            break;
        }
        sstRet += sizeof(BUZZER_MSG);
    }
    
    if (sstRet == 0) {
        _ErrorHandle(EAGAIN);
    }
    
    return  ((ssize_t)sstRet);
}
/*********************************************************************************************************
** ��������: __buzzerIoctl
** ��������: BUZZER �豸����
** �䡡��  : pbuzfil          �ļ�
**           iCmd             ��������
**           lArg             ����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __buzzerIoctl (PLW_BUZZER_FILE   pbuzfil, INT  iCmd, LONG  lArg)
{
    INT             iError = ERROR_NONE;
    struct stat    *pstatGet;
    PLW_BUZZER_DEV  pbuzzer = pbuzfil->BUZFIL_pbuzzer;

    switch (iCmd) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pbuzfil->BUZFIL_iFlags |= O_NONBLOCK;
        } else {
            pbuzfil->BUZFIL_iFlags &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFLUSH:
        API_MsgQueueClear(pbuzzer->BUZZER_ulMsgQ);
        API_ThreadWakeup(pbuzzer->BUZZER_ulThread);
        break;
    
    case FIOFSTATGET:                                                   /*  ��ȡ�ļ�����                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&pbuzzer->BUZZER_devhdr);
            pstatGet->st_ino     = (ino_t)pbuzfil;                      /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0222 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = pbuzzer->BUZZER_timeCreate;
            pstatGet->st_mtime   = pbuzzer->BUZZER_timeCreate;
            pstatGet->st_ctime   = pbuzzer->BUZZER_timeCreate;
        
        } else {
            _ErrorHandle(EINVAL);
            iError = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
        break;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __buzzerThread
** ��������: BUZZER �����߳�
** �䡡��  : pbuzzer       �豸
** �䡡��  : LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID __buzzerThread (PVOID  pvArg)
{
    PLW_BUZZER_DEV    pbuzzer = (PLW_BUZZER_DEV)pvArg;
    ULONG             ulError;
    ULONG             ulCounter;
    BUZZER_MSG        bmsg;
    
    for (;;) {
        ulError = API_MsgQueueReceive(pbuzzer->BUZZER_ulMsgQ, &bmsg,
                                      sizeof(BUZZER_MSG), LW_NULL,
                                      LW_OPTION_WAIT_INFINITE);
        if (ulError) {
            continue;
        }
        
        if (bmsg.BUZZER_uiOn) {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOn(pbuzzer->BUZZER_pbuzzerfuncs, bmsg.BUZZER_uiHz);
        } else {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOff(pbuzzer->BUZZER_pbuzzerfuncs);
        }
        
        API_TimeMSleep(bmsg.BUZZER_uiMs);
        
        ulError = API_MsgQueueStatus(pbuzzer->BUZZER_ulMsgQ, LW_NULL, &ulCounter,
                                     LW_NULL, LW_NULL, LW_NULL);
        if (ulError) {
            continue;
        }
        
        if (!ulCounter) {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOff(pbuzzer->BUZZER_pbuzzerfuncs);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_BuzzerDrvInstall
** ��������: ��װ Buzzer ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_BuzzerDrvInstall (void)
{
    if (_G_iBuzzerDrvNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iBuzzerDrvNum = iosDrvInstall(__buzzerOpen, LW_NULL, __buzzerOpen, __buzzerClose,
                                     LW_NULL, __buzzerWrite, __buzzerIoctl);

    DRIVER_LICENSE(_G_iBuzzerDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iBuzzerDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iBuzzerDrvNum, "Buzzer driver.");

    return  (_G_iBuzzerDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_BuzzerDevCreate
** ��������: ���� Buzzer �豸
** �䡡��  : pcName           �豸��
**           ulMaxQSize       �豸������Ϣ����
**           pbuzzerfuncs     �豸����
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_BuzzerDevCreate (CPCHAR            pcName, 
                          ULONG             ulMaxQSize,
                          PLW_BUZZER_FUNCS  pbuzzerfuncs)
{
    PLW_BUZZER_DEV      pbuzzerdev;
    LW_CLASS_THREADATTR threadattr;
    
    if (!pcName || !pbuzzerfuncs || (ulMaxQSize < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iBuzzerDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pbuzzerdev = (PLW_BUZZER_DEV)__SHEAP_ALLOC(sizeof(LW_BUZZER_DEV));
    if (pbuzzerdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pbuzzerdev, sizeof(LW_BUZZER_DEV));
    
    pbuzzerdev->BUZZER_pbuzzerfuncs = pbuzzerfuncs;
    pbuzzerdev->BUZZER_ulMsgQ = API_MsgQueueCreate("buzzer_q",
                                                   ulMaxQSize, sizeof(BUZZER_MSG),
                                                   LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);
    if (pbuzzerdev->BUZZER_ulMsgQ == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFAULT_STK_SIZE, 
                        LW_PRIO_T_BUS, 
                        (LW_OPTION_THREAD_STK_CHK | 
                         LW_OPTION_OBJECT_GLOBAL |
                         LW_OPTION_THREAD_DETACHED),
                        (PVOID)pbuzzerdev);
    
    pbuzzerdev->BUZZER_ulThread = API_ThreadInit("t_buzzer",
                                                 __buzzerThread,
                                                 &threadattr,
                                                 LW_NULL);
    if (pbuzzerdev->BUZZER_ulThread == LW_OBJECT_HANDLE_INVALID) {
        API_MsgQueueDelete(&pbuzzerdev->BUZZER_ulMsgQ);
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&pbuzzerdev->BUZZER_devhdr, pcName, _G_iBuzzerDrvNum, DT_CHR) != ERROR_NONE) {
        API_ThreadDelete(&pbuzzerdev->BUZZER_ulThread, LW_NULL);
        API_MsgQueueDelete(&pbuzzerdev->BUZZER_ulMsgQ);
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    pbuzzerdev->BUZZER_timeCreate = lib_time(LW_NULL);
    
    API_ThreadStart(pbuzzerdev->BUZZER_ulThread);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_BUZZER_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
