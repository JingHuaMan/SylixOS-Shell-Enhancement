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
** ��   ��   ��: tty.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 02 �� 04 ��
**
** ��        ��: TTY �豸�ӿ�.

** BUG
2008.03.24  ���򿪴����ļ����������豸ͷ��.
2008.06.15  ���ļ��رյ�ʱ��, ���� select() �ȴ��쳣���߳�.
2008.09.27  API_TtyDevCreate() �� SIO_CHAN ����Ч���ж��д�.
2009.10.02  _ttyOpen() ֻ���ڵ�һ�δ�ʱ���� SIO_OPEN ����.
2010.02.03  ����� SMP ��֧��.
2010.07.12  _ttyClose() ����ֵΪ ERROR_NONE.
2010.09.11  �����豸ʱ, ָ���豸����.
2012.08.06  ������ tty �豸ɾ���Ĳ���.
2012.08.10  ��һ�δ��豸ʱ��Ҫ���������.
2012.12.17  tty �豸�������ظ���, �豸����ͨ�� SIO_OPEN �� SIO_HUP ���жϽӿڴ򿪺͹ر�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
#include "termios.h"
/*********************************************************************************************************
  NCCS �������ں˶����Сһ��
*********************************************************************************************************/
#if NCCS != 19
#error "Kernel NCCS size not same as termios.h!"
#endif
/*********************************************************************************************************
  �Ƿ������ظ���
*********************************************************************************************************/
#define __LW_TTY_OPEN_REPEATEDLY_EN     1
/*********************************************************************************************************
  LOCAL FUNC
*********************************************************************************************************/
static LONG   _ttyOpen(TYCO_DEV    *ptycoDev,
                       PCHAR        pcName,
                       INT          iFlags,
                       INT          iMode);
static INT    _ttyClose(TYCO_DEV   *ptycoDev);
static INT    _ttyIoctl(TYCO_DEV   *ptycoDev,
                        INT         iRequest,
                        PVOID       pvArg);
static VOID   _ttyStartup(TYCO_DEV *ptycoDev);
/*********************************************************************************************************
** ��������: API_TtyDrvInstall
** ��������: ��װTTY�豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_TtyDrvInstall (VOID)
{
    if (_G_iTycoDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iTycoDrvNum = iosDrvInstall(_ttyOpen, 
                                   (FUNCPTR)LW_NULL, 
                                   _ttyOpen,
                                   _ttyClose,
                                   _TyRead,
                                   _TyWrite,
                                   _ttyIoctl);
                                    
    DRIVER_LICENSE(_G_iTycoDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iTycoDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iTycoDrvNum, "tty driver.");
    
    return  ((_G_iTycoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_TtyDevCreate
** ��������: ����һ�� TTY �豸
** �䡡��  : 
**           pcName,                       �豸��
**           psiochan,                     ͬ�� I/O ������
**           stRdBufSize,                  ���뻺������С
**           stWrtBufSize                  �����������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_TtyDevCreate (PCHAR     pcName,
                       SIO_CHAN *psiochan,
                       size_t    stRdBufSize,
                       size_t    stWrtBufSize)
{
    REGISTER INT           iTemp;
    REGISTER TYCO_DEV     *ptycoDev;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iTycoDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (psiochan == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "SIO channel invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if ((pcName == LW_NULL) || (*pcName == PX_EOS)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    ptycoDev = (TYCO_DEV *)__SHEAP_ALLOC(sizeof(TYCO_DEV));             /*  �����ڴ�                    */
    if (ptycoDev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    iTemp = _TyDevInit(&ptycoDev->TYCODEV_tydevTyDev, 
                       stRdBufSize, 
                       stWrtBufSize,
                       (FUNCPTR)_ttyStartup);                           /*  ��ʼ���豸���ƿ�            */
    
    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(ptycoDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    ptycoDev->TYCODEV_psiochan = psiochan;
    
    sioCallbackInstall(psiochan, SIO_CALLBACK_GET_TX_CHAR,
            (VX_SIO_CALLBACK)_TyITx, (PVOID)ptycoDev);
    sioCallbackInstall(psiochan, SIO_CALLBACK_PUT_RCV_CHAR,
            (VX_SIO_CALLBACK)_TyIRd, (PVOID)ptycoDev);
    
    sioIoctl(psiochan, SIO_MODE_SET, (PVOID)SIO_MODE_INT);
    
    iTemp = (INT)iosDevAddEx(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr, 
                             pcName, 
                             _G_iTycoDrvNum,
                             DT_CHR);
    if (iTemp) {
        _TyDevRemove(&ptycoDev->TYCODEV_tydevTyDev);
        __SHEAP_FREE(ptycoDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    
    } else {
        ptycoDev->TYCODEV_tydevTyDev.TYDEV_timeCreate = lib_time(LW_NULL);
                                                                        /*  ��¼����ʱ�� (UTC ʱ��)     */
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_TtyDevRemove
** ��������: ɾ�� TTY �豸.
** �䡡��  : pcName,                       �豸��, ��ͬ�� ����ʱ���豸��
**           bForce                        �Ƿ�ǿ��ɾ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� tty �豸��ж��ǿ�ҵĲ�����ʹ�� bForce �ӿ�, �п��ܻ����ϵͳ����, ����, �����һ�ιر��ļ�
             ϵͳ�� ioctl(SIO_HUP), ���ʱ��ſ���ж���豸 

                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TtyDevRemove (PCHAR   pcName, BOOL  bForce)
{
    REGISTER TYCO_DEV     *ptycoDev;
             TY_DEV_ID     ptyDev;
             PCHAR         pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pcName) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (_G_iTycoDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    ptycoDev = (TYCO_DEV *)iosDevFind(pcName, &pcTail);
    if ((ptycoDev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    ptyDev = &ptycoDev->TYCODEV_tydevTyDev;
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&ptyDev->TYDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&ptyDev->TYDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    iosDevFileAbnormal(&ptyDev->TYDEV_devhdrHdr);                       /*  �����д򿪵��ļ���Ϊ�쳣    */
    
    iosDevDelete(&ptyDev->TYDEV_devhdrHdr);
    
    _TyDevRemove(&ptycoDev->TYCODEV_tydevTyDev);
    
    __SHEAP_FREE(ptycoDev);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _ttyOpen
** ��������: ��һ�� TTY �豸 (ֻ�ܴ�һ��)
** �䡡��  : 
**           ptycoDev,                     tty �豸���ƿ�
**           pcName,                       �豸��
**           iFlags,                       O_RDONLY O_WRONLY O_RDWR O_CREAT ...
**           iMode                         ����
** �䡡��  : ���ƿ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG   _ttyOpen (TYCO_DEV    *ptycoDev,
                        PCHAR        pcName,
                        INT          iFlags,
                        INT          iMode)
{
    INT     iError;

    if (LW_DEV_INC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr) == 1) {
        iError = sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_OPEN, LW_NULL);
        if (iError < 0) {                                               /*  �򿪶˿�                    */
            LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr);
            return  (PX_ERROR);                                         /*  �޷��򿪶˿�                */
        }
        sioIoctl(ptycoDev->TYCODEV_psiochan, FIOFLUSH, LW_NULL);        /*  ���������                  */
    
        if (iFlags & O_NOCTTY) {                                        /*  ��֧�ֿ���ָ��              */
            INT   iOpt = OPT_TERMINAL;
            _TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, FIOGETOPTIONS, (LONG)&iOpt);
            iOpt &= ~(OPT_ABORT | OPT_MON_TRAP);
            _TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, FIOSETOPTIONS, iOpt);
        }
        
        if (iFlags & O_NONBLOCK) {                                      /*  ��д������                  */
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulRTimeout = LW_OPTION_NOT_WAIT;
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulWTimeout = LW_OPTION_NOT_WAIT;
        
        } else {
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
        }
    }
#if __LW_TTY_OPEN_REPEATEDLY_EN == 0
      else {
        LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr);
        _ErrorHandle(EBUSY);                                            /*  ֻ�����һ��              */
        return  (PX_ERROR);
    }
#endif                                                                  /*  __LW_TTY_OPEN_REPEATEDLY_EN */
    
    return  ((LONG)ptycoDev);
}
/*********************************************************************************************************
** ��������: _ttyClose
** ��������: �ر�һ�� TTY �豸 (SIO_HUP ϵͳ����˿ں�, �豸���ܻᱻ��������ɾ��)
** �䡡��  : 
**           ptycoDev,                     tty �豸���ƿ�
** �䡡��  : ���ƿ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT    _ttyClose (TYCO_DEV   *ptycoDev)
{
    if (LW_DEV_GET_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr)) {
            SEL_WAKE_UP_ALL(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_selwulList, 
                            SELEXCEPT);                                 /*  �����쳣�ȴ�                */
            sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_HUP, LW_NULL);     /*  ����˿�                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _ttyClose
** ��������: �ر�һ�� TTY �豸
** �䡡��  : 
**           ptycoDev,                     tty �豸���ƿ�
**           iRequest                      ��������
**           pvArg                         �������
** �䡡��  : �����ֵ.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT    _ttyIoctl (TYCO_DEV   *ptycoDev,
                         INT         iRequest,
                         PVOID       pvArg)
{
    REGISTER INT       iStatus;
    
    if (iRequest == SIO_HUP) {
        _ErrorHandle(EBUSY);                                            /*  ���ڴ򿪵��豸���������    */
        return  (PX_ERROR);
    }
    
    if (iRequest == FIOBAUDRATE) {                                      /*  ���ò�����                  */
        iStatus = sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_BAUD_SET, pvArg);
        if (iStatus == ERROR_NONE) { 
            return  (ERROR_NONE);
        } else {
            return  (PX_ERROR);
        }
    }
    
    iStatus = sioIoctl(ptycoDev->TYCODEV_psiochan, iRequest, pvArg);    /*  ִ������                    */
    
    if ((iStatus == ENOSYS) || 
        ((iStatus == PX_ERROR) && (errno == ENOSYS))) {                 /*  ���������޷�ʶ�������      */
        return  (_TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, 
                          iRequest, 
                          (LONG)pvArg));                                /*  ִ�� TY ������              */
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** ��������: _ttyStartup
** ��������: ���� TTY �豸���͹���
** �䡡��  : 
**           ptycoDev,                     tty �豸���ƿ�
** �䡡��  : NONE.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID   _ttyStartup (TYCO_DEV  *ptycoDev)
{
    sioTxStartup(ptycoDev->TYCODEV_psiochan);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
