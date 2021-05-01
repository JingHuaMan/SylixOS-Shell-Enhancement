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
** ��   ��   ��: signalDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 21 ��
**
** ��        ��: Linux ���� signalfd ʵ��.
**
** BUG
2020.03.08  ����ͬʱ select �����ͬ���� signalfd �ǶԵȴ�����Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_SIGNAL_EN > 0) && (LW_CFG_SIGNALFD_EN > 0)
#include "sys/signalfd.h"
#include "signalPrivate.h"
/*********************************************************************************************************
  �ź��ڲ�����
*********************************************************************************************************/
extern PLW_CLASS_SIGCONTEXT _signalGetCtx(PLW_CLASS_TCB  ptcb);
extern INT                  _sigPendGet(PLW_CLASS_SIGCONTEXT  psigctx, 
                                        const sigset_t       *psigset, 
                                        struct siginfo       *psiginfo);
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iSigfdDrvNum = PX_ERROR;
static LW_SIGFD_DEV     _G_sigfddev;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _sigfdOpen(PLW_SIGFD_DEV    psigfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _sigfdClose(PLW_SIGFD_FILE  psigfdfil);
static ssize_t  _sigfdRead(PLW_SIGFD_FILE   psigfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _sigfdIoctl(PLW_SIGFD_FILE  psigfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
** ��������: API_SignalfdDrvInstall
** ��������: ��װ signalfd �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SignalfdDrvInstall (VOID)
{
    if (_G_iSigfdDrvNum <= 0) {
        _G_iSigfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _sigfdOpen,
                                         _sigfdClose,
                                         _sigfdRead,
                                         LW_NULL,
                                         _sigfdIoctl);
        DRIVER_LICENSE(_G_iSigfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iSigfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iSigfdDrvNum, "signalfd driver.");
    }
    
    return  ((_G_iSigfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_SignalfdDevCreate
** ��������: ��װ signalfd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SignalfdDevCreate (VOID)
{
    if (_G_iSigfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_sigfddev.SD_devhdrHdr, LW_SIGFD_DEV_PATH, 
                    _G_iSigfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: signalfd
** ��������: ��/�޸� signalfd �ļ�
** �䡡��  : fd    < 0 ��ʾ�½� signal �ļ�
**                 >=0 ��ʾ�޸�֮ǰ���� signal �ļ�������
**           mask  �źŵȴ�����
**           flags ������־ SFD_CLOEXEC, SFD_NONBLOCK
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  signalfd (int fd, const sigset_t *mask, int flags)
{
    INT             iFd;
    PLW_SIGFD_FILE  psigfdfil;
    
    flags &= (SFD_CLOEXEC | SFD_NONBLOCK);
    
    if (fd < 0) {
        iFd = open(LW_SIGFD_DEV_PATH, O_RDONLY | flags);
    } else {
        iFd = fd;
    }
    
    if (iFd >= 0) {
        psigfdfil = (PLW_SIGFD_FILE)API_IosFdValue(iFd);
        if (!psigfdfil || (psigfdfil->SF_uiMagic != LW_SIGNAL_FILE_MAGIC)) {
            _ErrorHandle(EBADF);
            return  (PX_ERROR);
        }
        if (mask) {
            psigfdfil->SF_sigsetMask = *mask;
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: _sigfdOpen
** ��������: �� signalfd �豸
** �䡡��  : psigfddev        signalfd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _sigfdOpen (PLW_SIGFD_DEV psigfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_SIGFD_FILE  psigfdfil;
    
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        psigfdfil = (PLW_SIGFD_FILE)__SHEAP_ALLOC(sizeof(LW_SIGFD_FILE));
        if (!psigfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        psigfdfil->SF_uiMagic    = LW_SIGNAL_FILE_MAGIC;
        psigfdfil->SF_iFlag      = iFlags;
        psigfdfil->SF_sigsetMask = 0ull;
        
        LW_DEV_INC_USE_COUNT(&_G_sigfddev.SD_devhdrHdr);
        
        return  ((LONG)psigfdfil);
    }
}
/*********************************************************************************************************
** ��������: _sigfdClose
** ��������: �ر� signalfd �ļ�
** �䡡��  : psigfdfil         signalfd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _sigfdClose (PLW_SIGFD_FILE  psigfdfil)
{
    if (psigfdfil) {
        LW_DEV_DEC_USE_COUNT(&_G_sigfddev.SD_devhdrHdr);
        
        __SHEAP_FREE(psigfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _sigfdInfo2FdInfor
** ��������: �� siginfo ת��Ϊ signalfd_siginfo
** �䡡��  : iSigNo           �ź�
**           psiginfo         siginfo
**           psigfdinfo       signalfd_siginfo
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _sigfdInfo2FdInfor (INT                      iSigNo, 
                                 const struct siginfo     *psiginfo, 
                                 struct signalfd_siginfo  *psigfdinfo)
{
    lib_bzero(psigfdinfo, sizeof(struct signalfd_siginfo));

    psigfdinfo->ssi_signo   = (uint32_t)iSigNo;
    psigfdinfo->ssi_errno   = (int32_t) psiginfo->si_errno;
    psigfdinfo->ssi_code    = (int32_t) psiginfo->si_code;
    psigfdinfo->ssi_pid     = (uint32_t)psiginfo->si_pid;
    psigfdinfo->ssi_uid     = (uint32_t)psiginfo->si_uid;
    psigfdinfo->ssi_fd      = (int32_t) psiginfo->si_fd;
    psigfdinfo->ssi_tid     = (uint32_t)psiginfo->si_timerid;
    psigfdinfo->ssi_band    = (uint32_t)psiginfo->si_band;
    psigfdinfo->ssi_overrun = (uint32_t)psiginfo->si_overrun;
    psigfdinfo->ssi_trapno  = 0;
    psigfdinfo->ssi_status  = (int32_t) psiginfo->si_status;
    psigfdinfo->ssi_int     = (int32_t) psiginfo->si_int;
    psigfdinfo->ssi_ptr     = (uint64_t)(size_t)psiginfo->si_ptr;
    psigfdinfo->ssi_utime   = (uint64_t)psiginfo->si_utime;
    psigfdinfo->ssi_stime   = (uint64_t)psiginfo->si_stime;
    psigfdinfo->ssi_addr    = (uint64_t)(size_t)psiginfo->si_addr;
}
/*********************************************************************************************************
** ��������: _sigfdReadBlock
** ��������: �� signalfd �豸����
** �䡡��  : psigfdfil        signalfd �ļ�
**           psigctx          �ź�����������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _sigfdReadBlock (PLW_CLASS_TCB  ptcbCur, PLW_CLASS_SIGCONTEXT  psigctx)
{
    INTREG             iregInterLevel;
    PLW_CLASS_PCB      ppcb;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    __KERNEL_EXIT_IRQ(iregInterLevel);
}
/*********************************************************************************************************
** ��������: _sigfdReadUnblock
** ��������: ����� select signalfd �豸����
** �䡡��  : ulId             �߳� ID
**           iSigNo           �ź�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _sigfdReadUnblock (LW_OBJECT_HANDLE  ulId, INT  iSigNo)
{
    INTREG                  iregInterLevel;
    UINT16                  usIndex;
    PLW_CLASS_TCB           ptcb;
    PLW_CLASS_PCB           ppcb;
    PLW_CLASS_SIGCONTEXT    psigctx;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        return;
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return;
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        return;
    }
    
    ptcb    = __GET_TCB_FROM_INDEX(usIndex);
    psigctx = _signalGetCtx(ptcb);
    if (psigctx->SIGCTX_bRead) {                                        /*  �Ƿ����ڶ�                  */
        ppcb = _GetPcb(ptcb);
        if (ptcb->TCB_usStatus) {
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SIGNAL);
            if (__LW_THREAD_IS_READY(ptcb)) {
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  ���������                  */
            }
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
    
    if (psigctx->SIGCTX_sigsetFdw & __sigmask(iSigNo)) {
        SEL_WAKE_UP_ALL(&psigctx->SIGCTX_selwulist, SELREAD);           /*  ������������ select       */
    }
}
/*********************************************************************************************************
** ��������: _sigfdPendGet
** ��������: ��� pend �ź���Ϣ
** �䡡��  : psigfdfil        signalfd �ļ�
**           psigctx          �ź�����������
**           psigfdinfo       signalfd �ź���Ϣ
**           stMaxBytes       ��������С�ֽ���
**           pstGetNum        ���ػ�ȡ���ź���Ϣ����
** �䡡��  : ���ػ�ȡ�����ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _sigfdPendGet (PLW_SIGFD_FILE           psigfdfil,
                               PLW_CLASS_SIGCONTEXT     psigctx, 
                               struct signalfd_siginfo *psigfdinfo,
                               size_t                   stMaxBytes,
                               size_t                  *pstGetNum)
{
    INT             iSigNo;
    ssize_t         sstRetVal = 0;
    struct siginfo  siginfo;
    
    *pstGetNum = 0;

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (;;) {
        iSigNo = _sigPendGet(psigctx, &psigfdfil->SF_sigsetMask, &siginfo);
        if (__issig(iSigNo)) {                                          /*  ���ڱ��������ź�            */
            _sigfdInfo2FdInfor(iSigNo, &siginfo, psigfdinfo);
            sstRetVal += sizeof(struct signalfd_siginfo);
            psigfdinfo++;
            (*pstGetNum)++;
            if ((stMaxBytes - sstRetVal) < sizeof(struct signalfd_siginfo)) {
                break;
            }
        } else {
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (sstRetVal);
}
/*********************************************************************************************************
** ��������: _sigfdRead
** ��������: �� signalfd �豸
** �䡡��  : psigfdfil        signalfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _sigfdRead (PLW_SIGFD_FILE  psigfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    PLW_CLASS_TCB               ptcbCur;
    PLW_CLASS_SIGCONTEXT        psigctx;
    struct signalfd_siginfo    *psigfdinfo = (struct signalfd_siginfo *)pcBuffer;
    
    size_t                      stGetNum;
    ssize_t                     sstRetVal = 0;
    
    if (!pcBuffer || (stMaxBytes < sizeof(struct signalfd_siginfo))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    psigctx->SIGCTX_bRead = LW_TRUE;                                    /*  ���� read ����              */
    
    for (;;) {
        sstRetVal += _sigfdPendGet(psigfdfil, psigctx, psigfdinfo, stMaxBytes, &stGetNum);
        if (stGetNum) {
            psigfdinfo += stGetNum;
            stMaxBytes -= (stGetNum * sizeof(struct signalfd_siginfo));
        }
        
        if (sstRetVal) {
            break;
        
        } else {
            if (psigfdfil->SF_iFlag & O_NONBLOCK) {
                _ErrorHandle(EAGAIN);
                break;
            
            } else {
                _sigfdReadBlock(ptcbCur, psigctx);
            }
        }
    }
    
    psigctx->SIGCTX_bRead = LW_FALSE;                                   /*  �˳� read ����              */

    return  (sstRetVal);
}
/*********************************************************************************************************
** ��������: _sigfdSelect
** ��������: signalfd FIOSELECT
** �䡡��  : psigfdfil        signalfd �ļ�
**           pselwunNode      select �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _sigfdSelect (PLW_SIGFD_FILE  psigfdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    BOOL                 bHaveSigPend = LW_FALSE;
    PLW_CLASS_TCB        ptcbCur;
    PLW_CLASS_SIGCONTEXT psigctx;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    SEL_WAKE_NODE_ADD(&psigctx->SIGCTX_selwulist, pselwunNode);
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:
        psigctx->SIGCTX_sigsetFdw |= psigfdfil->SF_sigsetMask;          /*  ���û�������                */
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (psigctx->SIGCTX_sigsetPending & psigfdfil->SF_sigsetMask) {
            bHaveSigPend = LW_TRUE;
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        if (bHaveSigPend) {
            SEL_WAKE_UP(pselwunNode);
            psigctx->SIGCTX_sigsetFdw = 0ull;                           /*  �Ѿ���������, ����Ҫ����    */
        }
        break;
        
    case SELWRITE:
    case SELEXCEPT:
        break;
    }
}
/*********************************************************************************************************
** ��������: _sigfdUnselect
** ��������: signalfd FIOUNSELECT
** �䡡��  : psigfdfil        signalfd �ļ�
**           pselwunNode      select �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _sigfdUnselect (PLW_SIGFD_FILE  psigfdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    PLW_CLASS_TCB        ptcbCur;
    PLW_CLASS_SIGCONTEXT psigctx;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    SEL_WAKE_NODE_DELETE(&psigctx->SIGCTX_selwulist, pselwunNode);
    
    psigctx->SIGCTX_sigsetFdw = 0ull;                                   /*  ����Ҫ select ����          */
}
/*********************************************************************************************************
** ��������: _sigfdIoctl
** ��������: ���� signalfd �ļ�
** �䡡��  : psigfdfil        signalfd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _sigfdIoctl (PLW_SIGFD_FILE  psigfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            psigfdfil->SF_iFlag |= O_NONBLOCK;
        } else {
            psigfdfil->SF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_sigfddev.SD_devhdrHdr);
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
        _sigfdSelect(psigfdfil, pselwunNode);
        break;
        
    case FIOUNSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        _sigfdUnselect(psigfdfil, pselwunNode);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
                                                                        /*  LW_CFG_SIGNALFD_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
