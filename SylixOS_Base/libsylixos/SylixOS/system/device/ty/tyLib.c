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
** ��   ��   ��: tyLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 02 �� 04 ��
**
** ��        ��: TTY �豸�ڲ���.
**
** ע        ��: ��ʼ��ʱ VTIME=0 �� VMIN=1, ������������˴˲���, ����� FIORTIMEOUT FIOWTIMEOUT ����Ч.

** BUG
2008.04.01  �� FIORTIMEOUT �� FIOWTIMEOUT ���ó�ʱʱ��Ĳ�����Ϊ struct timeval ����.
            Ϊ NULL ��ʾ���õȴ�.
2008.05.31  ioctl ʹ�� long ��, ֧�� 64 λϵͳָ�봫��.
2008.06.06  �޸���һЩ��������.
2009.02.09  ioctl ����ʶ������, ����ӡ������Ϣ.
2009.03.10  FIOISATTY ����ʹ�� lArg ��Ϊ����.
2009.05.21  _TyDevRemove û��ɾ�����ź���.
2009.05.27  ���� abort ����.
2009.06.09  ����һ�� BUG, ���ܵ��´ӱ��� XOFF�޷��ص� XON ģʽ(�������޷��ͷ�).
2009.07.15  OPT_CRMOD ģʽ��, �� \r\n ��������ʱ, ���� \r ��Ϊ \n, ����� \n ����.
2009.08.15  _TyITx() ���񼤻����޼��뻺�����Ĵ�С�о�.
2009.09.04  �ж��շ��ص���Ҫ�ڴ��� ring buffer ʱ�ر�����ж�, ��Ϊ���������������� PTY ����, ������ر�
            ���ܻᵼ�»���������.
2009.09.04  _TyDevRemove() ����� select �ṹ�����, ͬʱ���ѵȴ�����.
2009.09.05  �Ƴ� ty �豸ʱ��Ҫ�����д򿪵��ļ�����Ϊ�쳣(Ԥ�ر�ģʽ).
2009.10.22  read write ����������ֵ���͵���.
2009.12.01  �ն�ģʽ�¶� 0x00 �ַ�����Ӧ.
2010.01.14  ���� abort.
2010.02.03  ����� SMP ��֧��.
2012.01.10  ����� termios ������֧��.
2012.03.04  _TyDrain() ��Ҫ���жϷ��ͻ������Ƿ�Ϊ��, �����Ϊ��, �ڵȴ� drain �ź���.
2012.08.10  pend(..., LW_OPTION_NOT_WAIT) ��Ϊ clear() ����.
2012.10.17  ����� TIOCGWINSZ �� TIOCSWINSZ ��֧��.
2012.12.14  ���� Tx �� Rx �жϻص��� LOCK ��ʱ��, ȷ���������.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2013.06.09  _TyIRd ����ϵͳʱ����Ҫ _execJob ����.
2013.10.03  ioctl() ����ȫ�����û���������.
            ����һЩ��־, ���� SMP �ڴ����ϲ���.
2014.03.03  �Ż�����.
2014.05.20  tty �豸ɾ�����Ӱ�ȫ.
2014.08.03  FIOWBUFSET ʱ��Ҫ����ȴ�д���߳�.
2014.12.08  ���� _TyIRd() �����ͷ� spinlock ����.
2015.05.07  �Ż��жϳ������.
2015.12.04  ���� SMP ������������.
2020.06.23  ��� RAW ģʽЧ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __TYCO_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  termios header file
*********************************************************************************************************/
#include "limits.h"
#include "termios.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
  �������������Ϣ
*********************************************************************************************************/
#define __XON           0x11                                            /*  ctrl-Q XON  ����            */
#define __XOFF          0x13                                            /*  ctrl-S XOFF ����            */
/*********************************************************************************************************
  ȫ�ֱ���(������ __LINUX_BACKSPACE_CHAR ֱ�Ӱ� backspace ����)
*********************************************************************************************************/
#define __LINUX_BACKSPACE_CHAR      127                                 /*  linux �µ��˸� control-?    */
#define __TTY_BACKSPACE(ptyDev, ch) ((ch) == __LINUX_BACKSPACE_CHAR || (ch) == __TTY_CC(ptyDev, VERASE))

static CHAR             _G_cTyBackspaceChar  = 0x08;                    /*  Ĭ��ֵ     control-H        */
static CHAR             _G_cTyDeleteLineChar = 0x15;                    /*  Ĭ��ֵ     control-U        */
static CHAR             _G_cTyEofChar        = 0x04;                    /*  Ĭ��ֵ     control-D        */
static CHAR             _G_cTyAbortChar      = 0x03;                    /*  Ĭ��ֵ     control-C        */
static CHAR             _G_cTyMonTrapChar    = 0x18;                    /*  Ĭ��ֵ     control-X        */
/*********************************************************************************************************
  ȫ�ֱ���(���Ʒ�ֵ)
*********************************************************************************************************/
static CHAR             _G_cTyXoffThreshold  = 10;                      /*  ʹ�� OPT_TANDEM ģʽʱ, ��  */
                                                                        /*  ���뻺���������ֽ���С����  */
                                                                        /*  ��ֵʱ���� XOFF ������֡    */
static CHAR             _G_cTyXonThreshold   = 30;                      /*  ʹ�� OPT_TANDEM ģʽʱ, ��  */
                                                                        /*  ���뻺���������ֽ���������  */
                                                                        /*  ��ֵʱ���� XON ������֡     */
static CHAR             _G_cTyWrtThreshold   = 64;                      /*  ����������������ֽ�������  */
                                                                        /*  ���ֵ, ����ȴ�д���߳�    */
/*********************************************************************************************************
  ȫ�ֱ���(����ָ��)
*********************************************************************************************************/
static FUNCPTR          _G_pfuncTyAbortFunc  = LW_NULL;                 /*  �յ� control-C ִ�еĲ���   */
/*********************************************************************************************************
  �ڲ�ͳ���ñ���
*********************************************************************************************************/
static INT              _G_iTyXoffChars = 0;
static INT              _G_iTyXoffMax   = 0;
/*********************************************************************************************************
  ��������ַ�
*********************************************************************************************************/
#define __TTY_CC(ptyDev, c)         (ptyDev)->TYDEV_cCtlChars[c]
/*********************************************************************************************************
  select ֧�ֺ���
*********************************************************************************************************/
VOID            __selTyAdd(   TY_DEV_ID   ptyDev, LONG  lArg);          /*  ���һ���ȴ��Ľڵ�          */
VOID            __selTyDelete(TY_DEV_ID   ptyDev, LONG  lArg);          /*  ɾ��һ���ȴ��Ľڵ�          */
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static VOID     _TyFlush(    TY_DEV_ID  ptyDev);                        /*  ����豸������              */
static VOID     _TyFlushRd(  TY_DEV_ID  ptyDev);                        /*  �����������                */
static VOID     _TyFlushWrt( TY_DEV_ID  ptyDev);                        /*  ���д������                */
static VOID     _TyTxStartup(TY_DEV_ID  ptyDev);                        /*  �����豸���͹���            */
static VOID     _TyRdXoff(   TY_DEV_ID  ptyDev, BOOL  bXoff);           /*  ���ý��ն�����״̬          */
static VOID     _TyWrtXoff(  TY_DEV_ID  ptyDev, BOOL  bXoff);           /*  ���÷��Ͷ�����״̬          */
/*********************************************************************************************************
  ��
*********************************************************************************************************/
#define TYDEV_LOCK(ptyDev, code)    \
        if (API_SemaphoreMPend((ptyDev)->TYDEV_hMutexSemM, LW_OPTION_WAIT_INFINITE)) {  \
            code;   \
        }
#define TYDEV_UNLOCK(ptyDev)        \
        API_SemaphoreMPost((ptyDev)->TYDEV_hMutexSemM)
/*********************************************************************************************************
  �ȴ��жϽ���
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define TYDEV_WAIT_ISR(ptyDev)      \
        do {    \
            INTREG  iregInterLevel; \
            LW_SPIN_LOCK_QUICK(&((ptyDev)->TYDEV_slLock), &iregInterLevel); \
            LW_SPIN_UNLOCK_QUICK(&((ptyDev)->TYDEV_slLock), iregInterLevel);    \
        } while (0)
#else
#define TYDEV_WAIT_ISR(ptyDev)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: _TyDevInit
** ��������: ��ʼ��һ�� TY �豸
** �䡡��  : 
**           ptyDev,                  ��Ҫ��ʼ���� ty �豸
**           stRdBufSize,             ���������Ĵ�С
**           stWrtBufSize,            д�������Ĵ�С
**           pfunctxStartup           �豸������������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _TyDevInit (TY_DEV_ID  ptyDev, 
                 size_t     stRdBufSize,
                 size_t     stWrtBufSize,
                 FUNCPTR    pfunctxStartup)
{
    REGISTER INT    iErrLevel = 0;

    lib_bzero((PVOID)ptyDev, sizeof(TY_DEV));                           /*  �����ڴ�                    */

    ptyDev->TYDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;                 /*  ��ʼ��Ϊ���õȴ�            */
    ptyDev->TYDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;                 /*  ��ʼ��Ϊ���õȴ�            */
    
    ptyDev->TYDEV_vxringidWrBuf = rngCreate((INT)stWrtBufSize);         /*  ����д������                */
    if (ptyDev->TYDEV_vxringidWrBuf == LW_NULL) {                       /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    ptyDev->TYDEV_vxringidRdBuf = rngCreate((INT)stRdBufSize);          /*  ������������                */
    if (ptyDev->TYDEV_vxringidRdBuf == LW_NULL) {                       /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_iWrtThreshold = (INT)(stWrtBufSize > (INT)_G_cTyWrtThreshold)
                                ? ((INT)_G_cTyWrtThreshold) 
                                : (stWrtBufSize);                       /*  ȷ��д��������              */
    
    ptyDev->TYDEV_pfuncTxStartup = pfunctxStartup;                      /*  ��������                    */
    
    ptyDev->TYDEV_hRdSyncSemB = API_SemaphoreBCreate("ty_rsync", 
                                                     LW_FALSE, 
                                                     LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, 
                                                     LW_NULL);          /*  ��ͬ��                      */
    if (!ptyDev->TYDEV_hRdSyncSemB) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hWrtSyncSemB = API_SemaphoreBCreate("ty_wsync", 
                                                      LW_FALSE, 
                                                      LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, 
                                                      LW_NULL);         /*  дͬ��                      */
    if (!ptyDev->TYDEV_hWrtSyncSemB) {
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hDrainSyncSemB = API_SemaphoreBCreate("ty_drain",
                                                        LW_FALSE,
                                                        LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                        LW_NULL);       /*  �������ͬ���ź���          */
    if (!ptyDev->TYDEV_hDrainSyncSemB) {
        iErrLevel = 4;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hMutexSemM = API_SemaphoreMCreate("ty_lock", 
                                                    LW_PRIO_DEF_CEILING, 
                                                    _G_ulMutexOptionsTyLib |
                                                    LW_OPTION_OBJECT_DEBUG_UNPEND |
                                                    LW_OPTION_OBJECT_GLOBAL, 
                                                    LW_NULL);           /*  ������ʿ����ź���          */
    if (!ptyDev->TYDEV_hMutexSemM) {
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    SEL_WAKE_UP_LIST_INIT(&ptyDev->TYDEV_selwulList);                   /*  ��ʼ�� select �ȴ���        */
    
    _TyFlush(ptyDev);                                                   /*  ����豸������              */

    __TTY_CC(ptyDev, VINTR)  = _G_cTyAbortChar;                         /*  ��ʼ�����������            */
    __TTY_CC(ptyDev, VQUIT)  = _G_cTyMonTrapChar;
    __TTY_CC(ptyDev, VERASE) = _G_cTyBackspaceChar;
    __TTY_CC(ptyDev, VKILL)  = _G_cTyDeleteLineChar;
    __TTY_CC(ptyDev, VEOF)   = _G_cTyEofChar;
    __TTY_CC(ptyDev, VSTART) = __XON;
    __TTY_CC(ptyDev, VSTOP)  = __XOFF;
    
    __TTY_CC(ptyDev, VTIME) = 0;                                        /*  sioLib Ĭ�ϳ�ʱ����         */
    __TTY_CC(ptyDev, VMIN)  = 1;

    ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow = 24;                       /*  Ĭ��Ϊ 80 * 24              */
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol = 80;
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel = 80 * 8;                /*  Ĭ��Ϊ 16 * 8 ����          */
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel = 24 * 16;

    LW_SPIN_INIT(&ptyDev->TYDEV_slLock);                                /*  ��ʼ��������                */

    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hDrainSyncSemB);
    }
    if (iErrLevel > 3) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hWrtSyncSemB);              /*  ɾ��дͬ��                  */
    }
    if (iErrLevel > 2) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hRdSyncSemB);               /*  ɾ����ͬ��                  */
    }
    if (iErrLevel > 1) {
        rngDelete(ptyDev->TYDEV_vxringidRdBuf);                         /*  ɾ����������                */
    }
    if (iErrLevel > 0) {
        rngDelete(ptyDev->TYDEV_vxringidWrBuf);                         /*  ɾ��д������                */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _TyDevRemove
** ��������: �Ƴ�һ�� TY �豸
** �䡡��  : 
**           pstrDev,           ��Ҫ�Ƴ��� ty �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _TyDevRemove (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;

    TYDEV_LOCK(ptyDev, return (PX_ERROR));                              /*  �ȴ��豸ʹ��Ȩ              */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_TRUE;          /*  read cancel                 */
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_TRUE;          /*  write cancel                */
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf  = LW_TRUE;
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock �����ж�    */
    
    if (ptyDev->TYDEV_vxringidRdBuf) {
        rngDelete(ptyDev->TYDEV_vxringidRdBuf);                         /*  ɾ����������                */
        ptyDev->TYDEV_vxringidRdBuf = LW_NULL;
    }
    if (ptyDev->TYDEV_vxringidWrBuf) {
        rngDelete(ptyDev->TYDEV_vxringidWrBuf);                         /*  ɾ��д������                */
        ptyDev->TYDEV_vxringidWrBuf = LW_NULL;
    }
    
    SEL_WAKE_UP_LIST_TERM(&ptyDev->TYDEV_selwulList);                   /*  ж�� sel �ṹ               */
    
    API_SemaphoreBDelete(&ptyDev->TYDEV_hWrtSyncSemB);                  /*  ɾ��дͬ��                  */
    API_SemaphoreBDelete(&ptyDev->TYDEV_hRdSyncSemB);                   /*  ɾ����ͬ��                  */
    API_SemaphoreBDelete(&ptyDev->TYDEV_hDrainSyncSemB);                /*  ɾ����������ź�            */
    API_SemaphoreMDelete(&ptyDev->TYDEV_hMutexSemM);                    /*  ɾ�������ź���              */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _TyFlush
** ��������: ��� TY �豸�Ľ��շ��ͻ�����
** �䡡��  : 
**           pstrDev,           ty �豸
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyFlush (TY_DEV_ID  ptyDev)
{
    _TyFlushRd(ptyDev);
    _TyFlushWrt(ptyDev);
}
/*********************************************************************************************************
** ��������: _TyFlushRd
** ��������: ��� TY �豸��������
** �䡡��  : 
**           pstrDev,           ty �豸
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyFlushRd (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;
    
    TYDEV_LOCK(ptyDev, return);                                         /*  �ȴ��豸ʹ��Ȩ              */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_TRUE;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    
    rngFlush(ptyDev->TYDEV_vxringidRdBuf);                              /*  ���������                  */
    
    ptyDev->TYDEV_ucInNBytes    = 0;
    ptyDev->TYDEV_ucInBytesLeft = 0;
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock �����ж�    */
    
    _TyRdXoff(ptyDev, LW_FALSE);                                        /*  ��������Է�����            */
    
    API_SemaphoreBClear(ptyDev->TYDEV_hRdSyncSemB);                     /*  �����ͬ��                  */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_FALSE;
    KN_SMP_MB();
    
    TYDEV_UNLOCK(ptyDev);                                               /*  �ͷ��豸ʹ��Ȩ              */
}
/*********************************************************************************************************
** ��������: _TyFlushWrt
** ��������: ��� TY �豸д������
** �䡡��  : 
**           pstrDev,           ty �豸
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyFlushWrt (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;
    
    TYDEV_LOCK(ptyDev, return);                                         /*  �ȴ��豸ʹ��Ȩ              */
    
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    
    rngFlush(ptyDev->TYDEV_vxringidWrBuf);
    
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR = LW_FALSE;

    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock �����ж�    */
    
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_FALSE;
    KN_SMP_MB();
    
    API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);                     /*  ֪ͨ�߳̿�д                */
    API_SemaphoreBPost(ptyDev->TYDEV_hDrainSyncSemB);                   /*  drain                       */
    
    SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);               /*  ֪ͨ select �߳̿�д        */
    
    TYDEV_UNLOCK(ptyDev);                                               /*  �ͷ��豸ʹ��Ȩ              */
}
/*********************************************************************************************************
** ��������: API_TyAbortFuncSet
** ��������: TY �豸������ OPT_ABORT ʱ���յ� ABORT ����ʱִ�еĶ���.
** �䡡��  : 
**           pfuncAbort,        ���յ� ABORT �����ִ�еĺ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyAbortFuncSet (FUNCPTR  pfuncAbort)
{
    _G_pfuncTyAbortFunc = pfuncAbort;
}
/*********************************************************************************************************
** ��������: API_TyAbortSet
** ��������: TY �豸������ OPT_ABORT ʱ, ���� ABORT �ַ�
** �䡡��  : 
**           cAbort,            ABORT �ַ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyAbortSet (CHAR  cAbort)
{
    _G_cTyAbortChar = cAbort;
}
/*********************************************************************************************************
** ��������: API_TyBackspaceSet
** ��������: ���� BACKSPACE �ַ�
** �䡡��  : 
**           cBackspace,        BACKSPACE �ַ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyBackspaceSet (CHAR  cBackspace)
{
    _G_cTyBackspaceChar = cBackspace;
}
/*********************************************************************************************************
** ��������: API_TyDeleteLineSet
** ��������: ���� cDeleteLine �ַ�
** �䡡��  : 
**           cDeleteLine,       DeleteLine �ַ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyDeleteLineSet (CHAR  cDeleteLine)
{
    _G_cTyDeleteLineChar = cDeleteLine;
}
/*********************************************************************************************************
** ��������: API_TyEOFSet
** ��������: ���� cEOF �ַ�
** �䡡��  : 
**           cEOF,              EOF �ַ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyEOFSet (CHAR  cEOF)
{
    _G_cTyEofChar = cEOF;
}
/*********************************************************************************************************
** ��������: API_TyMonitorTrapSet
** ��������: ���� MonitorTrap �ַ�
** �䡡��  : 
**           cMonitorTrap,      MonitorTrap �ַ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TyMonitorTrapSet (CHAR  cMonitorTrap)
{
    _G_cTyMonTrapChar = cMonitorTrap;
}
/*********************************************************************************************************
** ��������: _TyDrain
** ��������: TY �豸�ȴ��������
** �䡡��  : pstrDev,           TY �豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ������ֶ�����ͬʱ drain ���п��ܷ����޷�����������������, ����ÿһ�� tick ����� ring ����
*********************************************************************************************************/
static INT  _TyDrain (TY_DEV_ID  ptyDev)
{
             INTREG     iregInterLevel;

    REGISTER ULONG      ulError;
             INT        iFree;

    do {
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);
        iFree = rngNBytes(ptyDev->TYDEV_vxringidWrBuf);
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
        
        if (iFree == 0) {
            return  (ERROR_NONE);
        }
                                                                        /*  ���ȴ�һ��ʱ������          */
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hDrainSyncSemB, LW_OPTION_WAIT_A_TICK);
    } while ((ulError == ERROR_NONE) || 
             (ulError == ERROR_THREAD_WAIT_TIMEOUT));
    
    _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                              /*  ��ʱ                        */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _TyIoctl
** ��������: TY �豸�����
** �䡡��  : 
**           pstrDev,           TY �豸
**           request,           ����
**           arg                �������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _TyIoctl (TY_DEV_ID  ptyDev,
               INT        iRequest,
               LONG       lArg)
{
             INTREG          iregInterLevel;
             VX_RING_ID      ringId;
             
    REGISTER INT             iStatus = ERROR_NONE;
    REGISTER INT             iOldOption;
    
             struct stat    *pstatGet;
    
    switch (iRequest) {
    
    case FIONREAD:                                                      /*  ����������Ч��������        */
        *((INT *)lArg) = rngNBytes(ptyDev->TYDEV_vxringidRdBuf);
        break;
    
    case FIONWRITE:                                                     /*  д��������Ч��������        */
        *((INT *)lArg) = rngNBytes(ptyDev->TYDEV_vxringidWrBuf);
        break;
    
    case FIONFREE:
        *((INT *)lArg) = rngFreeBytes(ptyDev->TYDEV_vxringidWrBuf);     /*  ��д������ֽ�              */
        break;

    case FIOFLUSH:                                                      /*  ����豸������              */
        _TyFlush(ptyDev);
        break;
        
    case FIOWFLUSH:                                                     /*  ���д������                */
        _TyFlushWrt(ptyDev);
        break;
        
    case FIORFLUSH:                                                     /*  ��ն�������                */
        _TyFlushRd(ptyDev);
        break;
    
    case FIOSYNC:                                                       /*  �ȴ��������                */
    case FIODATASYNC:
        iStatus = _TyDrain(ptyDev);
        break;
    
    case FIOGETOPTIONS:                                                 /*  ��� TY ����                */
        *(INT *)lArg = ptyDev->TYDEV_iOpt;
        break;
        
    case FIOSETOPTIONS:                                                 /*  ���� TY ����                */
        iOldOption = ptyDev->TYDEV_iOpt;
        ptyDev->TYDEV_iOpt = (INT)lArg;
        if ((iOldOption & OPT_LINE) != 
            (ptyDev->TYDEV_iOpt & OPT_LINE)) {                          /*  �� OPT_LINE ��־�ı仯      */
            _TyFlushRd(ptyDev);
        }
        if ((iOldOption & OPT_TANDEM) && 
            !(ptyDev->TYDEV_iOpt & OPT_TANDEM)) {                       /*  ����������                  */
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  �ȴ��豸ʹ��Ȩ              */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  XON                         */
            _TyWrtXoff(ptyDev, LW_FALSE);
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
        }
        break;
        
    case FIOCANCEL:                                                     /*  ��ʱ���� FIO ��             */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_TRUE;
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_TRUE;
        API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        break;
    
    case FIOISATTY:                                                     /*  �Ƿ�Ϊ�ն��豸              */
        if (lArg) {
            *(BOOL *)lArg = LW_TRUE;
        }
        break;
    
    case FIOPROTOHOOK:                                                  /*  ��������Э��ջ�ص�����      */
        ptyDev->TYDEV_pfuncProtoHook = (FUNCPTR)lArg;
        break;
        
    case FIOPROTOARG:                                                   /*  ���ûص���������            */
        ptyDev->TYDEV_iProtoArg = (INT)lArg;
        break;
    
    case FIORBUFSET:                                                    /*  �������ö�����Ĵ�С        */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_TRUE;
        KN_SMP_MB();
        TYDEV_WAIT_ISR(ptyDev);                                         /*  �ȴ�һ�����жϽ���          */
        ringId = rngCreate((INT)lArg);                                  /*  ���½���������              */
        if (ringId) {
            if (ptyDev->TYDEV_vxringidRdBuf) {
                rngDelete(ptyDev->TYDEV_vxringidRdBuf);
            }
            ptyDev->TYDEV_vxringidRdBuf = ringId;
        } else {
            iStatus = PX_ERROR;
        }
        KN_SMP_MB();
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_FALSE;
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        break;
        
    case FIOWBUFSET:                                                    /*  ��������д������            */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
        KN_SMP_MB();
        TYDEV_WAIT_ISR(ptyDev);                                         /*  �ȴ�һ���жϽ���            */
        ringId = rngCreate((INT)lArg);                                  /*  ���½���������              */
        if (ringId) {
            if (ptyDev->TYDEV_vxringidWrBuf) {
                rngDelete(ptyDev->TYDEV_vxringidWrBuf);
            }
            ptyDev->TYDEV_vxringidWrBuf = ringId;
        } else {
            iStatus = PX_ERROR;
        }
        if (iStatus == ERROR_NONE) {
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);
            ptyDev->TYDEV_iWrtThreshold = ((INT)lArg > (INT)_G_cTyWrtThreshold)
                                        ? ((INT)_G_cTyWrtThreshold) 
                                        : ((INT)lArg);                  /*  ȷ��д��������              */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
        }
        KN_SMP_MB();
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_FALSE;
        
        API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);                 /*  �ͷ��ź���                  */
        SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);           /*  �ͷ����еȴ�д���߳�        */
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        break;
        
    case FIOFSTATGET:                                                   /*  ��ȡ�ļ�����                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&ptyDev->TYDEV_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = ptyDev->TYDEV_timeCreate;
            pstatGet->st_mtime   = ptyDev->TYDEV_timeCreate;
            pstatGet->st_ctime   = ptyDev->TYDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:                                                     /*  ty FIOSELECT ����           */
        __selTyAdd(ptyDev, lArg);
        break;
        
    case FIOUNSELECT:                                                   /*  ty FIOUNSELECT ����         */
        __selTyDelete(ptyDev, lArg);
        break;
        
    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                ptyDev->TYDEV_ulRTimeout = ulTick;
            } else {
                ptyDev->TYDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWTIMEOUT:                                                   /*  ����д��ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                ptyDev->TYDEV_ulWTimeout = ulTick;
            } else {
                ptyDev->TYDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWAITABORT:                                                  /*  ֹͣ��ǰ�ȴ� IO �߳�        */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        if ((INT)lArg & OPT_RABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(ptyDev->TYDEV_hRdSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                ptyDev->TYDEV_iAbortFlag |= OPT_RABORT;
                API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);          /*  ������ȴ��߳�              */
            }
        }
        if ((INT)lArg & OPT_WABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(ptyDev->TYDEV_hWrtSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                ptyDev->TYDEV_iAbortFlag |= OPT_WABORT;
                API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);         /*  ����д�ȴ��߳�              */
            }
        }
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        break;
        
    case FIOABORTFUNC:                                                  /*  ���� control-C ��Ϊ         */
        ptyDev->TYDEV_pfuncCtrlC = (FUNCPTR)lArg;
        break;
        
    case FIOABORTARG:                                                   /*  ���� control-C ����         */
        ptyDev->TYDEV_pvArgCtrlC = (PVOID)lArg;
        break;
        
    case FIOGETCC: {                                                    /*  ��ȡ��������ַ���          */
        PCHAR   pcCtlChars = (PCHAR)lArg;
        if (pcCtlChars) {
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  �ȴ��豸ʹ��Ȩ              */
            lib_memcpy(pcCtlChars, ptyDev->TYDEV_cCtlChars, NCCS);
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    case FIOSETCC: {                                                    /*  ������������ַ���          */
        PCHAR   pcCtlChars = (PCHAR)lArg;
        if (pcCtlChars) {
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  �ȴ��豸ʹ��Ȩ              */
            lib_memcpy(ptyDev->TYDEV_cCtlChars, pcCtlChars, NCCS);
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    case TIOCGWINSZ: {
        struct winsize  *pwin = (struct winsize *)lArg;
        if (pwin) {
            pwin->ws_row = ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow;
            pwin->ws_col = ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol;
            pwin->ws_xpixel = ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel;
            pwin->ws_ypixel = ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel;
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
        
    case TIOCSWINSZ: {
        struct winsize  *pwin = (struct winsize *)lArg;
        if (pwin) {
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow = pwin->ws_row;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol = pwin->ws_col;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel = pwin->ws_xpixel;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel = pwin->ws_ypixel;
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        iStatus = PX_ERROR;
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** ��������: _TyWrite
** ��������: ���ն�д������
** �䡡��  : 
**           ptyDev             TY �豸
**           pcBuffer,          ��Ҫд���ն˵�����
**           stNBytes           ��Ҫд������ݴ�С
** �䡡��  : ʵ��д����ֽ���, С�� 1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������ڻ�ȡдͬ���ź������ȡ�����ź���ǰ��ɾ��, �� tty �ᶪʧдͬ��, ����������ǰ���밲ȫ
             ģʽ.
*********************************************************************************************************/
ssize_t  _TyWrite (TY_DEV_ID  ptyDev, 
                   PCHAR      pcBuffer, 
                   size_t     stNBytes)
{
             INTREG     iregInterLevel;
             
    REGISTER INT        iBytesput;
    REGISTER ssize_t    sstNbStart = stNBytes;
    
    REGISTER ULONG      ulError;
    
    if (pcBuffer == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stNBytes == 0) {
        return  (0);
    }
    
    LW_THREAD_SAFE();                                                   /*  ������ǰ���밲ȫ״̬        */
    
    while (stNBytes > 0) {
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hWrtSyncSemB, ptyDev->TYDEV_ulWTimeout);
        if (ulError) {
            LW_THREAD_UNSAFE();
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*   ��ʱ                       */
            return  (sstNbStart - stNBytes);
        }
        
        TYDEV_LOCK(ptyDev, LW_THREAD_UNSAFE(); return (PX_ERROR));      /*  �ȴ��豸ʹ��Ȩ              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_WABORT) {                    /*  is abort?                   */
            ptyDev->TYDEV_iAbortFlag &= ~OPT_WABORT;                    /*  ��� abort                  */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            LW_THREAD_UNSAFE();
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNbStart - stNBytes);
        }
        
        if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled) {          /*  ����Ƿ񱻽�ֹ�����        */
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_FALSE; /*  ��� canceled               */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            LW_THREAD_UNSAFE();
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNbStart - stNBytes);
        }
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  ���� spinlock ���ر��ж�    */
        iBytesput = rngBufPut(ptyDev->TYDEV_vxringidWrBuf, 
                              pcBuffer, 
                              (INT)stNBytes);                           /*  ������д�뻺����            */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        
        _TyTxStartup(ptyDev);                                           /*  �����豸����                */
        
        stNBytes -= (size_t)iBytesput;                                  /*  ʣ����Ҫ���͵�����          */
        pcBuffer += iBytesput;                                          /*  �µĻ�������ʼ��ַ          */
    
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  ���� spinlock ���ر��ж�    */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
            API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);             /*  ���������пռ�              */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
        }
        
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
    }
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    return  (sstNbStart);
}
/*********************************************************************************************************
** ��������: _TyReadVtime
** ��������: ���ն˶������� (�� termios VTIME!=0 ʱʹ��)
** �䡡��  : 
**           ptyDev,            TY �豸
**           pcBuffer,          �������ݻ�����
**           stMaxBytes         ��������С
** �䡡��  : ʵ�ʽ��յ��ֽ���, С�� 1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������ڻ�ȡ��ͬ���ź������ȡ�����ź���ǰ��ɾ��, �� tty �ᶪʧ��ͬ��!
*********************************************************************************************************/
static ssize_t  _TyReadVtime (TY_DEV_ID  ptyDev, 
                              PCHAR      pcBuffer, 
                              size_t     stMaxBytes)
{
             INTREG        iregInterLevel;
         
             ssize_t       sstNBytes;
             ssize_t       sstNTotalBytes = 0;
    
    REGISTER VX_RING_ID    ringId;
    REGISTER INT           iNTemp;
    REGISTER INT           iRetTemp;
    REGISTER INT           iFreeBytes;
    
    REGISTER ULONG         ulError;
             ULONG         ulTimeout;                                   /*  ���ʱ��                    */
    
    if (__TTY_CC(ptyDev, VMIN) == 0) {                                  /*  �״εȴ�ʱ��                */
        ulTimeout = ((ULONG)(__TTY_CC(ptyDev, VTIME) * 100) * LW_TICK_HZ / 1000);
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
__re_read:
    for (;;) {
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hRdSyncSemB, ulTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  ��ʱ                        */
            return  (sstNTotalBytes);
        }
        
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_RABORT) {                    /*  is abort                    */
            ptyDev->TYDEV_iAbortFlag &= ~OPT_RABORT;                    /*  ��� abort                  */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNTotalBytes);
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled) {          /*  ����豸�Ƿ񱻶���ֹ��      */
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_FALSE; /*  ��� canceled               */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNTotalBytes);
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;                           /*  ���뻺����                  */
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  ���� spinlock ���ر��ж�    */
        if (!rngIsEmpty(ringId)) {                                      /*  ����Ƿ�������              */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
    }
        
    if (__TTY_CC(ptyDev, VMIN)) {                                       /*  VMIN ��Ϊ 0 ���ü���ȴ�    */
        ulTimeout = ((ULONG)(__TTY_CC(ptyDev, VTIME) * 100) * LW_TICK_HZ / 1000);
    }
    
    if (ptyDev->TYDEV_iOpt & OPT_LINE) {                                /*  ��ģʽ                      */
        if (ptyDev->TYDEV_ucInBytesLeft == 0) {                         /*  ����Ч����ʣ������          */
            iRetTemp = __RNG_ELEM_GET(ringId, 
                           (PCHAR)&ptyDev->TYDEV_ucInBytesLeft, 
                           iNTemp);                                     /*  ���ʣ�����ݴ�С            */
            (VOID)iRetTemp;                                             /*  ����һ���ɹ�, �ݲ��жϷ���ֵ*/
        }
        sstNBytes = (ssize_t)__MIN((ssize_t)ptyDev->TYDEV_ucInBytesLeft, 
                                   (ssize_t)stMaxBytes);                /*  ������յ����ֵ            */
        rngBufGet(ringId, pcBuffer, (INT)sstNBytes);                    /*  ��������                    */
        ptyDev->TYDEV_ucInBytesLeft = (UCHAR)
                                      (ptyDev->TYDEV_ucInBytesLeft
                                    - sstNBytes);                       /*  ���¼�����ʣ������          */
    } else {                                                            /*  ����ģʽ                    */
        sstNBytes = (ssize_t)rngBufGet(ringId, pcBuffer, 
                                       (INT)stMaxBytes);                /*  ֱ�ӽ���                    */
    }
    
    if ((ptyDev->TYDEV_iOpt & OPT_TANDEM) &&                            /*  ʹ�� XON XOFF ������        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {                  /*  ���ҹض϶�                  */
        iFreeBytes = rngFreeBytes(ringId);                              /*  ��ÿ�������                */
        if (ptyDev->TYDEV_iOpt & OPT_LINE) {                            /*  ��ģʽ                      */
            iFreeBytes -= ptyDev->TYDEV_ucInNBytes + 1;
        }
        if (iFreeBytes > _G_cTyXonThreshold) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  �����Է�����                */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel); /*  ���� spinlock ���ر��ж�    */
        }
    }
    
    if (!rngIsEmpty(ringId)) {                                          /*  �Ƿ�������                */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);                  /*  ֪ͨ�����ȴ������߳�        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
    }
    
    sstNTotalBytes += sstNBytes;                                        /*  �Զ�ȡ���ݵ�����            */
    
    if (sstNTotalBytes < __TTY_CC(ptyDev, VMIN)) {                      /*  �Ƿ���յ�ָ������������    */
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        pcBuffer   += sstNBytes;
        stMaxBytes -= (size_t)sstNBytes;
        goto    __re_read;
        
    } else {
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
    }
    
    return  (sstNTotalBytes);
}
/*********************************************************************************************************
** ��������: _TyRead
** ��������: ���ն˶�������
** �䡡��  : 
**           ptyDev,            TY �豸
**           pcBuffer,          �������ݻ�����
**           stMaxBytes         ��������С
** �䡡��  : ʵ�ʽ��յ��ֽ���, С�� 1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������ڻ�ȡ��ͬ���ź������ȡ�����ź���ǰ��ɾ��, �� tty �ᶪʧ��ͬ��!
*********************************************************************************************************/
ssize_t  _TyRead (TY_DEV_ID  ptyDev, 
                  PCHAR      pcBuffer, 
                  size_t     stMaxBytes)
{
             INTREG        iregInterLevel;
         
             ssize_t       sstNBytes;
             ssize_t       sstNTotalBytes = 0;
    
    REGISTER VX_RING_ID    ringId;
    REGISTER INT           iNTemp;
    REGISTER INT           iRetTemp;
    REGISTER INT           iFreeBytes;
    
    REGISTER ULONG         ulError;
    
    if (pcBuffer == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes == 0) {
        return  (0);
    }
    
    if (__TTY_CC(ptyDev, VMIN) > stMaxBytes) {                          /*  stMaxBytes ̫С             */
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__TTY_CC(ptyDev, VTIME) != 0) {
        return  (_TyReadVtime(ptyDev, pcBuffer, stMaxBytes));           /*  ���� VTIME ��ʱ�Ķ�ȡ       */
    }

__re_read:
    for (;;) {
        if (__TTY_CC(ptyDev, VMIN) == 0) {                              /*  ����ȴ�                    */
            ulError = API_SemaphoreBTryPend(ptyDev->TYDEV_hRdSyncSemB);
        } else {                                                        /*  ��ͨ����                    */
            ulError = API_SemaphoreBPend(ptyDev->TYDEV_hRdSyncSemB, ptyDev->TYDEV_ulRTimeout);
        }
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  ��ʱ                        */
            return  (sstNTotalBytes);
        }
        
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  �ȴ��豸ʹ��Ȩ              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_RABORT) {                    /*  is abort                    */
            ptyDev->TYDEV_iAbortFlag &= ~OPT_RABORT;                    /*  ��� abort                  */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNTotalBytes);
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled) {          /*  ����豸�Ƿ񱻶���ֹ��      */
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_FALSE; /*  ��� canceled               */
            TYDEV_UNLOCK(ptyDev);                                       /*  �ͷ��豸ʹ��Ȩ              */
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNTotalBytes);
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;                           /*  ���뻺����                  */
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  ���� spinlock ���ر��ж�    */
        if (!rngIsEmpty(ringId)) {                                      /*  ����Ƿ�������              */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
    }
    
    if (ptyDev->TYDEV_iOpt & OPT_LINE) {                                /*  ��ģʽ                      */
        if (ptyDev->TYDEV_ucInBytesLeft == 0) {                         /*  ����Ч����ʣ������          */
            iRetTemp = __RNG_ELEM_GET(ringId, 
                           (PCHAR)&ptyDev->TYDEV_ucInBytesLeft, 
                           iNTemp);                                     /*  ���ʣ�����ݴ�С            */
            (VOID)iRetTemp;                                             /*  ����һ���ɹ�, �ݲ��жϷ���ֵ*/
        }
        sstNBytes = (ssize_t)__MIN((ssize_t)ptyDev->TYDEV_ucInBytesLeft, 
                                   (ssize_t)stMaxBytes);                /*  ������յ����ֵ            */
        rngBufGet(ringId, pcBuffer, (INT)sstNBytes);                    /*  ��������                    */
        ptyDev->TYDEV_ucInBytesLeft = (UCHAR)
                                      (ptyDev->TYDEV_ucInBytesLeft
                                    - sstNBytes);                       /*  ���¼�����ʣ������          */
    } else {                                                            /*  ����ģʽ                    */
        sstNBytes = (ssize_t)rngBufGet(ringId, pcBuffer, 
                                       (INT)stMaxBytes);                /*  ֱ�ӽ���                    */
    }
    
    if ((ptyDev->TYDEV_iOpt & OPT_TANDEM) &&                            /*  ʹ�� XON XOFF ������        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {                  /*  ���ҹض϶�                  */
        iFreeBytes = rngFreeBytes(ringId);                              /*  ��ÿ�������                */
        if (ptyDev->TYDEV_iOpt & OPT_LINE) {                            /*  ��ģʽ                      */
            iFreeBytes -= ptyDev->TYDEV_ucInNBytes + 1;
        }
        if (iFreeBytes > _G_cTyXonThreshold) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  �����Է�����                */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);   
                                                                        /*  ���� spinlock ���ر��ж�    */
        }
    }
    
    if (!rngIsEmpty(ringId)) {                                          /*  �Ƿ�������                */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);                  /*  ֪ͨ�����ȴ������߳�        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
    }
    
    sstNTotalBytes += sstNBytes;                                        /*  �Զ�ȡ���ݵ�����            */
    
    if (sstNTotalBytes < __TTY_CC(ptyDev, VMIN)) {                      /*  �Ƿ���յ�ָ������������    */
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
        pcBuffer   += sstNBytes;
        stMaxBytes -= (size_t)sstNBytes;
        goto    __re_read;
        
    } else {
        TYDEV_UNLOCK(ptyDev);                                           /*  �ͷ��豸ʹ��Ȩ              */
    }
    
    return  (sstNTotalBytes);
}
/*********************************************************************************************************
** ��������: _TyITx
** ��������: ���ն˵ķ��ͻ������ж���һ���ֽڵĴ�����, FIFO
** �䡡��  : 
**           ptyDev,            TY �豸
**           pcChar,            �����͵�����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _TyITx (TY_DEV_ID  ptyDev, PCHAR  pcChar)
{
             INTREG        iregInterLevel;

    REGISTER VX_RING_ID    ringId = ptyDev->TYDEV_vxringidWrBuf;
    REGISTER INT           iNTemp;
             INT           iRet;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending) {               /*  �Ƿ���Ҫ������������        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending = LW_FALSE;
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {
            if (_G_iTyXoffChars > _G_iTyXoffMax) {                      /*  ͳ�Ʊ�������                */
                _G_iTyXoffMax   = _G_iTyXoffChars;                      /*  ��¼���ֵ                  */
                _G_iTyXoffChars = 0;
            }
            *pcChar = __TTY_CC(ptyDev, VSTOP);
        } else {
            *pcChar = __TTY_CC(ptyDev, VSTART);
        }

    } else if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff || 
               ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf) { /*  �Է���ֹ���ջ��߻�����ռ��  */
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;
        
    } else if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR) {             /*  �Ƿ���Ҫ׷�� CR �ַ�        */
        *pcChar = '\n';
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR   = LW_FALSE;
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
        goto    __release_wrt;                                          /*  ����д����                  */
        
    } else {
        if (__RNG_ELEM_GET(ringId, pcChar, iNTemp) == 0) {              /*  �ӻ�����ȡ������            */
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;     /*  û��ʣ�����ݵȴ�����        */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
            API_SemaphoreBPost(ptyDev->TYDEV_hDrainSyncSemB);           /*  ������������ź���          */
            
            return  (PX_ERROR);                                         /*  û��������Ҫ����            */

        } else {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
            
            if ((ptyDev->TYDEV_iOpt & OPT_CRMOD) && (*pcChar == '\n')) {/*  ��Ҫ�� LF ǰ��� CR �ַ�    */
                                                                        /*  ���Ҵ����͵�����Ϊ LF       */
                *pcChar = '\r';
                ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR = LW_TRUE;    /*  ��һ�η��͵�����Ϊ LF       */
            }
            
__release_wrt:
            if (rngFreeBytes(ringId) >= ptyDev->TYDEV_iWrtThreshold) {  /*  ���Լ�����һ����Ҫд���߳�  */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
                API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);         /*  �ͷ��ź���                  */
                SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);   /*  �ͷ����еȴ�д���߳�        */
            
                return  (ERROR_NONE);                                   /*  ���Է�������                */
            }
        }
    }
    
    iRet = (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) ? (ERROR_NONE) : (PX_ERROR);
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock ���ж�      */
        
    return  (iRet);                                                     /*  ����                        */
}
/*********************************************************************************************************
** ��������: _TyIRx
** ��������: ���ն˵Ľ��ջ�������д��һ����Ӳ�����յ�������, FIFO
** �䡡��  : 
**           ptyDev,            TY �豸
**           cInChar,           ���յ�������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _TyIRd (TY_DEV_ID  ptyDev, CHAR   cInchar)
{
             INTREG      iregInterLevel;

    REGISTER VX_RING_ID  ringId;
    REGISTER INT         iNTemp;
             BOOL        bReleaseTaskLevel;
             
    REGISTER INT         iOpt          = ptyDev->TYDEV_iOpt;
             BOOL        bCharEchoed   = LW_FALSE;
             BOOL        bNeedBsOrKill = LW_FALSE;
             INT         iStatus       = ERROR_NONE;
             
    REGISTER INT         iFreeBytes;

    
    if (LW_UNLIKELY(ptyDev->TYDEV_pfuncProtoHook)) {
        if (ptyDev->TYDEV_pfuncProtoHook(ptyDev->TYDEV_iProtoArg,
                                         cInchar) == ERROR_NONE) {      /*  �����ӵ�Э��ʱ����Э��ջ    */
            return  (ERROR_NONE);
        }
    }
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf) {         /*  ���뻺�����Ƿ� FLUSH      */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        return  (PX_ERROR);
    }

    if (iOpt == OPT_RAW) {                                              /*  ԭʼģʽ���ٲ���            */
        if (RNG_ELEM_PUT(ptyDev->TYDEV_vxringidRdBuf, cInchar, iNTemp) == 0) {
            iStatus = PX_ERROR;                                         /*  д���������                */
        }
        if (rngNBytes(ptyDev->TYDEV_vxringidRdBuf) == 1) {
            bReleaseTaskLevel = LW_TRUE;                                /*  ����ȴ�����                */
        } else {
            bReleaseTaskLevel = LW_FALSE;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */

        if (bReleaseTaskLevel) {
            API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);              /*  ����ȴ�����                */
            SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELREAD);        /*  select() ����               */
        }
        return  (iStatus);

    } else if (iOpt & OPT_7_BIT) {
        cInchar &= 0x7f;                                                /*  ������ 7 λ����             */
    }
    
    if ((cInchar == __TTY_CC(ptyDev, VINTR)) && 
        (iOpt & OPT_ABORT) && 
        ((_G_pfuncTyAbortFunc != LW_NULL) || 
        (ptyDev->TYDEV_pfuncCtrlC != LW_NULL))) {                       /*  �Ƿ���Ҫ���� ABORT ����     */
        
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        
        if (ptyDev->TYDEV_pfuncCtrlC) {
            ptyDev->TYDEV_pfuncCtrlC(ptyDev->TYDEV_pvArgCtrlC);
        }
        if (_G_pfuncTyAbortFunc) {
            _G_pfuncTyAbortFunc();
        }
    
    } else if ((cInchar == __TTY_CC(ptyDev, VQUIT)) && 
               (iOpt & OPT_MON_TRAP)) {                                 /*  ��Ҫ���� CONTORL+X ����     */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        API_KernelReboot(LW_REBOOT_WARM);                               /*  ������������ϵͳ            */
    
    } else if (((cInchar == __TTY_CC(ptyDev, VSTOP)) || (cInchar == __TTY_CC(ptyDev, VSTART))) && 
               (iOpt & OPT_TANDEM)) {                                   /*  ��Ҫ����������              */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
        
        if (cInchar == __TTY_CC(ptyDev, VSTOP)) {
            _TyWrtXoff(ptyDev, LW_TRUE);
        } else {
            _TyWrtXoff(ptyDev, LW_FALSE);
        }
    
    } else {
        if ((iOpt & OPT_CRMOD) && (iOpt & OPT_LINE)) {
            if (cInchar == PX_EOS) {                                    /*  ��ģʽ�¶� 0x00 �ַ�����Ӧ  */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
                return  (iStatus);                                      /*  ���Դ� \0                   */
            }
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {              /*  ��������������              */
            _G_iTyXoffChars++;                                          /*  ͳ�Ʊ���++                  */
        }
        
        if ((iOpt & OPT_CRMOD) && 
            (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv == '\r') &&
            (cInchar == '\n')) {                                        /*  ������ \r\n ����, ��ʶ��һ��*/
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv = cInchar;
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
            return  (iStatus);                                          /*  ���Դ� \n                   */
        
        } else {
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv = cInchar;  /*  ���汾������                */
        }
        
        if ((iOpt & OPT_CRMOD) && (cInchar == '\r')) {                  /*  ���յ� CR �ַ�              */
            cInchar = '\n';                                             /*  ����ת��                    */
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;
        bReleaseTaskLevel = LW_FALSE;                                   /*  ��ʼ�����뻺��������        */
        
        if (!(iOpt & OPT_LINE)) {                                       /*  ������ģʽ                  */
            if (RNG_ELEM_PUT(ringId, cInchar, iNTemp) == 0) {           /*  д���������                */
                iStatus = PX_ERROR;
            }
            if (rngNBytes(ringId) == 1) {
                bReleaseTaskLevel = LW_TRUE;                            /*  ��Ҫ����ȴ��߳�            */
            }
            
        } else {                                                        /*  ������ģʽ                  */
            iFreeBytes = rngFreeBytes(ringId);                          /*  ��ÿ����ֽڵĸ���          */
            
            if (__TTY_BACKSPACE(ptyDev, cInchar)) {                     /*  �˸��                      */
                if (ptyDev->TYDEV_ucInNBytes) {
                    ptyDev->TYDEV_ucInNBytes--;
                    bNeedBsOrKill = LW_TRUE;
                
                } else {
                    bNeedBsOrKill = LW_FALSE;
                }
            
            } else if (cInchar == __TTY_CC(ptyDev, VKILL)) {            /*  ɾ��һ��                    */
                if (ptyDev->TYDEV_ucInNBytes) {
                    ptyDev->TYDEV_ucInNBytes = 0;
                    bNeedBsOrKill = LW_TRUE;
                
                } else {
                    bNeedBsOrKill = LW_FALSE;
                }
            
            } else if (cInchar == __TTY_CC(ptyDev, VEOF)) {             /*  ������                      */
                if (iFreeBytes > 0) {
                    bReleaseTaskLevel = LW_TRUE;                        /*  ������������                */
                }
            
            } else {                                                    /*  ������                      */
                if (iFreeBytes >= 2) {                                  /*  ���������� 2 ���ַ�         */
                    if ((iFreeBytes >= (ptyDev->TYDEV_ucInNBytes + 3)) &&
                        (ptyDev->TYDEV_ucInNBytes < (MAX_CANON - 1))) {
                        ptyDev->TYDEV_ucInNBytes++;
                    } else {
                        iStatus = PX_ERROR;                             /*  û��ʣ��ռ�                */
                    }
                    
                    rngPutAhead(ringId, cInchar, 
                                (INT)ptyDev->TYDEV_ucInNBytes);
                                
                    if (cInchar == '\n') {
                        bReleaseTaskLevel = LW_TRUE;                    /*  ������������                */
                    }
                
                } else {                                                /*  ��ȫû�пռ�                */
                    iStatus = PX_ERROR;                                 /*  û��ʣ��ռ�                */
                }
            }
            
            if (bReleaseTaskLevel) {
                rngPutAhead( ringId, (CHAR)ptyDev->TYDEV_ucInNBytes, 0);
                rngMoveAhead(ringId, (INT) ptyDev->TYDEV_ucInNBytes + 1);
                ptyDev->TYDEV_ucInNBytes = 0;
            }
        }
        
        if ((iOpt & OPT_ECHO) && (iStatus != PX_ERROR) &&               /*  ��Ҫ�����ն˻���            */
            !ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf) {   /*  �������ۺ���������          */
            
            ringId = ptyDev->TYDEV_vxringidWrBuf;                       /*  TTY ���������              */
            
            if (iOpt & OPT_LINE) {                                      /*  ��ģʽ                      */
                if (cInchar == __TTY_CC(ptyDev, VKILL)) {               /*  ɾ����                      */
                    if (bNeedBsOrKill) {
                        iNTemp = RNG_ELEM_PUT(ringId, '\n', iNTemp);    /*  ��� LF                     */
                        bCharEchoed = LW_TRUE;                          /*  ���Գɹ�                    */
                    }
                
                } else if (__TTY_BACKSPACE(ptyDev, cInchar)) {          /*  �˸��                      */
                    if (bNeedBsOrKill) {                                /*  ��һ�����Ѿ����ַ�          */
                        CHAR    cBsCharList[3];
                        
                        cBsCharList[0] = _G_cTyBackspaceChar;           /*  �˸�����                    */
                        cBsCharList[1] = ' ';
                        cBsCharList[2] = _G_cTyBackspaceChar;
                        
                        rngBufPut(ringId, cBsCharList, 3);              /*  BS ����                     */
                        bCharEchoed = LW_TRUE;                          /*  ���Գɹ�                    */
                    }
                
                } else if ((cInchar < 0x20) && (cInchar != '\n')) {     /*  �� LF �Ŀ����ַ�            */
                    iNTemp = RNG_ELEM_PUT(ringId, '^', iNTemp);
                    iNTemp = RNG_ELEM_PUT(ringId, 
                                          (CHAR)(cInchar + '@'), 
                                          iNTemp);                      /*  ���Ϳ����������            */
                    bCharEchoed = LW_TRUE;                              /*  ���Գɹ�                    */
                
                } else {                                                /*  �����ַ�                    */
                    iNTemp = RNG_ELEM_PUT(ringId, cInchar, iNTemp);     /*  ֱ�ӻ���                    */
                    bCharEchoed = LW_TRUE;                              /*  ���Գɹ�                    */
                }
            
            } else {                                                    /*  ����ģʽ                    */
                iNTemp = RNG_ELEM_PUT(ringId, cInchar, iNTemp);         /*  ֱ�ӻظ�����                */
                bCharEchoed = LW_TRUE;                                  /*  ���Գɹ�                    */
            }
            
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
            if (bCharEchoed) {                                          /*  ����ظ��ɹ�                */
                _TyTxStartup(ptyDev);                                   /*  ��Ҫ��������                */
            }
        } else {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
        }
        
        if ((iOpt & OPT_TANDEM) && !(iOpt & OPT_LINE)) {                /*  ������ģʽ                  */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel); /*  ���� spinlock ���ر��ж�    */
            if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf) { /*  ���뻺�����Ƿ� FLUSH      */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
                return  (PX_ERROR);
            }
            ringId     = ptyDev->TYDEV_vxringidRdBuf;
            iFreeBytes = rngFreeBytes(ringId);
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  ���� spinlock ���ж�      */
            
            if (!ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {         /*  ���������������            */
                if (iFreeBytes < _G_cTyXoffThreshold) {                 /*  ������뻺������Ҫ����      */
                    _TyRdXoff(ptyDev, LW_TRUE);                         /*  ���� XOFF                   */
                    bReleaseTaskLevel = LW_TRUE;                        /*  ������Ҫ������������߳�    */
                }
            } else {
                if (iFreeBytes > _G_cTyXonThreshold) {
                    _TyRdXoff(ptyDev, LW_FALSE);                        /*  ���� XON                    */
                }
            }
        }
        
        if (bReleaseTaskLevel) {
            API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);              /*  ����ȴ�����                */
            SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELREAD);        /*  select() ����               */
        }
    }
    
    if (bCharEchoed) {
        iStatus = ERROR_NONE;
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** ��������: _TyRdXoff
** ��������: ���ý��ն�����״̬
** �䡡��  : 
**           ptyDev,            TY �豸
**           bXoff,             �Ƿ�رս���
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyRdXoff (TY_DEV_ID  ptyDev, BOOL  bXoff)
{
    INTREG      iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff != bXoff) {         /*  ��Ҫ�ı�״̬                */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff    = bXoff;
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending = LW_TRUE;       /*  ��Ҫ��������������          */
        if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy == LW_FALSE) {  /*  û����������                */
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy  = LW_TRUE;     /*  ��������                    */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
            ptyDev->TYDEV_pfuncTxStartup(ptyDev);                       /*  ��������                    */
            return;
        }
    }
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock ���ж�      */
}
/*********************************************************************************************************
** ��������: _TyWrtXoff
** ��������: ���÷��Ͷ�����״̬
** �䡡��  : 
**           ptyDev,            TY �豸
**           bXoff,             �Ƿ�رշ���
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyWrtXoff (TY_DEV_ID  ptyDev, BOOL  bXoff)
{
    INTREG      iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  ���� spinlock ���ر��ж�    */
    if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff != bXoff) {
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff  = bXoff;
        if (bXoff == LW_FALSE) {                                        /*  ��������                    */
            if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy == 
                LW_FALSE) {                                             /*  û����������                */
                ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;  /*  ��������                    */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
                ptyDev->TYDEV_pfuncTxStartup(ptyDev);                   /*  ��������                    */
                return;
            }
        }
    }
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  ���� spinlock ���ж�      */
}
/*********************************************************************************************************
** ��������: _TyTxStartup
** ��������: �������ͺ���
** �䡡��  : 
**           ptyDev,            TY �豸
**           bXoff,             �Ƿ�رշ���
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TyTxStartup (TY_DEV_ID  ptyDev)
{
    INTREG    iregInterLevel;
    
    if (!ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) {
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  ���� spinlock ���ر��ж�    */
        if (!ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  ���� spinlock ���ж�      */
            ptyDev->TYDEV_pfuncTxStartup(ptyDev);                       /*  ��������                    */
            return;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  ���� spinlock ���ж�      */
    }
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
