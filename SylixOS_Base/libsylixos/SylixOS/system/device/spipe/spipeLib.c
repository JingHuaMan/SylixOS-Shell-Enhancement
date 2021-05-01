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
** ��   ��   ��: spipeLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: �ַ����ܵ�ͨ���ڲ����ܺ��� (���߳�ͬʱд���Ҷ��߳�ͬʱ��ʱ, ��ʱʱ����ܲ�׼ȷ)

** BUG
2007.06.06  LINE71 Ӧ�����ж���Ч���ٴ���򿪴���
2007.09.21  �����˻��������ơ�
2007.09.21  ����������.
2007.11.20  ���� select ����.
2007.12.11  ����չܵ���Ϣʱ,��Ҫ����д�ȴ����߳�.
2008.04.01  �� FIORTIMEOUT �� FIOWTIMEOUT ���ó�ʱʱ��Ĳ�����Ϊ struct timeval ����.
            Ϊ NULL ��ʾ���õȴ�.
2009.02.09  ioctl ����ʶ������, ����ӡ������Ϣ.
2009.05.27  ���� abort ����.
2010.01.14  ���� abort.
2010.09.09  ֧�� SELEXCEPT ����. ���ܵ��豸ɾ��ʱ���ᱻ����.
2011.03.27  ���� _SpipeRemove ����.
2011.08.09  st_size �ܵ�Ϊ�ڲ�������.
2011.12.13  ʹ write ֧�� PIPE_BUF ��С�ڵ�ԭ�Ӳ���. read ����ԭ�Ӳ�����.
2012.08.25  ����ܵ����˹رռ��:
            1: ������˹ر�, д���������յ� SIGPIPE �ź�, write ���᷵�� -1.
            2: ���д�˹ر�, ���������������������, Ȼ���ٴζ����� 0.
2012.12.29  �������뵱дһ���ܵ�ʱ, ���û�ж���, ���յ� SIGPIPE �ź�.
2013.06.12  select read û��д��, write û�ж��˶���Ҫ����.
            ������������ PIPE_BUF �ֽ�ʱ�ż���д��.
2014.03.03  �Ż�����.
2016.10.25  �����ź����ȴ�����.
2017.07.27  ��߹ܵ�����Ч�� (������ν���ź�������).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __SPIPE_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
#include "limits.h"
/*********************************************************************************************************
  EXT MODE
*********************************************************************************************************/
#define LW_SPIPE_EXT_MODE_NOSIG     0x1
/*********************************************************************************************************
  ����������
*********************************************************************************************************/
#define LW_SPIPE_LOCK(pspipedev, code) \
        if (API_SemaphoreMPend(pspipedev->SPIPEDEV_hOpLock, LW_OPTION_WAIT_INFINITE)) { \
            code; \
        }
#define LW_SPIPE_UNLOCK(pspipedev) \
        API_SemaphoreMPost(pspipedev->SPIPEDEV_hOpLock)
/*********************************************************************************************************
  ��ͬ��������
*********************************************************************************************************/
#define LW_SPIPE_WAIT_ROPEN(pspipedev, code) \
        if (API_SemaphorePostBPend(pspipedev->SPIPEDEV_hOpLock, \
                                   _G_ulSpipeReadOpenLock, LW_OPTION_WAIT_INFINITE)) { \
            code; \
        }
#define LW_SPIPE_WAKEUP_ROPEN() \
        API_SemaphoreBFlush(_G_ulSpipeReadOpenLock, LW_NULL)
        
#define LW_SPIPE_WAIT_WOPEN(pspipedev, code) \
        if (API_SemaphorePostBPend(pspipedev->SPIPEDEV_hOpLock, \
                                   _G_ulSpipeWriteOpenLock, LW_OPTION_WAIT_INFINITE)) { \
            code; \
        }
#define LW_SPIPE_WAKEUP_WOPEN() \
        API_SemaphoreBFlush(_G_ulSpipeWriteOpenLock, LW_NULL)
        
#define LW_SPIPE_WAKEUP_OPEN(pspipedev, flag) \
        {   \
            if ((flag & O_ACCMODE) == O_RDONLY) { \
                LW_SPIPE_WAKEUP_WOPEN(); \
            } else if ((flag & O_ACCMODE) == O_WRONLY) { \
                LW_SPIPE_WAKEUP_ROPEN(); \
            } else { \
                LW_SPIPE_WAKEUP_WOPEN(); \
                LW_SPIPE_WAKEUP_ROPEN(); \
            } \
        }
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define LW_SPIPE_INC_CNT(pspipedev, flag) \
        {   \
            if ((flag & O_ACCMODE) == O_RDONLY) { \
                pspipedev->SPIPEDEV_uiReadCnt++; \
            } else if ((flag & O_ACCMODE) == O_WRONLY) { \
                pspipedev->SPIPEDEV_uiWriteCnt++; \
            } else { \
                pspipedev->SPIPEDEV_uiReadCnt++; \
                pspipedev->SPIPEDEV_uiWriteCnt++; \
            } \
        }
        
#define LW_SPIPE_DEC_CNT(pspipedev, flag) \
        {   \
            if ((flag & O_ACCMODE) == O_RDONLY) { \
                pspipedev->SPIPEDEV_uiReadCnt--; \
            } else if ((flag & O_ACCMODE) == O_WRONLY) { \
                pspipedev->SPIPEDEV_uiWriteCnt--; \
            } else { \
                pspipedev->SPIPEDEV_uiReadCnt--; \
                pspipedev->SPIPEDEV_uiWriteCnt--; \
            } \
        }
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define LW_SPIPE_BLOCK(pspipedev, flag) \
        {   \
            if ((flag & O_ACCMODE) == O_RDONLY) { \
                while (!pspipedev->SPIPEDEV_uiWriteCnt) { \
                    LW_SPIPE_WAIT_ROPEN(pspipedev, return (PX_ERROR)); \
                    LW_SPIPE_LOCK(pspipedev, return (PX_ERROR)); \
                } \
            } else if ((flag & O_ACCMODE) == O_WRONLY) { \
                while (!pspipedev->SPIPEDEV_uiReadCnt) { \
                    LW_SPIPE_WAIT_WOPEN(pspipedev, return (PX_ERROR)); \
                    LW_SPIPE_LOCK(pspipedev, return (PX_ERROR)); \
                } \
            } \
        }
/*********************************************************************************************************
  check can read/write
*********************************************************************************************************/
static LW_INLINE BOOL  __spipe_can_read (size_t stMsgLen, size_t stTotal)
{
    return  (stMsgLen ? LW_TRUE : LW_FALSE);
}
static LW_INLINE BOOL  __spipe_can_write (size_t stMsgLen, size_t stTotal)
{
    if ((stTotal - stMsgLen) >= PIPE_BUF) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
  ȫ���ź���
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulSpipeReadOpenLock;
extern LW_OBJECT_HANDLE     _G_ulSpipeWriteOpenLock;
/*********************************************************************************************************
** ��������: _SpipeOpen
** ��������: ���ַ����ܵ��豸
** �䡡��  : 
**           pspipedev        �ַ����ܵ��豸���ƿ�
**           pcName           �ַ����ܵ�����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

LONG  _SpipeOpen (PLW_SPIPE_DEV  pspipedev, 
                  PCHAR          pcName,
                  INT            iFlags, 
                  INT            iMode)
{
    PLW_SPIPE_FILE  pspipefil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  �����ظ�����                */
            return  (PX_ERROR);
        }
        if (iFlags & O_DIRECTORY) {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                    /*  �����ܵ��豸                */
        
        if (iFlags & O_NONBLOCK) {                                      /*  ������                      */
            if ((iFlags & O_ACCMODE) == O_WRONLY) {                     /*  ֻд��ʽ                    */
                if (!pspipedev->SPIPEDEV_uiReadCnt) {                   /*  û�ж���                    */
                    LW_SPIPE_UNLOCK(pspipedev);                         /*  �ͷ��豸ʹ��Ȩ              */
                    _ErrorHandle(ENXIO);
                    return  (PX_ERROR);
                }
            }
        }
        
        pspipefil = (PLW_SPIPE_FILE)__SHEAP_ALLOC(sizeof(LW_SPIPE_FILE));
        if (!pspipefil) {
            LW_SPIPE_UNLOCK(pspipedev);                                 /*  �ͷ��豸ʹ��Ȩ              */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pspipefil->SPIPEFIL_iFlags    = iFlags;
        pspipefil->SPIPEFIL_iMode     = iMode;
        pspipefil->SPIPEFIL_iExtMode  = 0;
        pspipefil->SPIPEFIL_pspipedev = pspipedev;
        
        if (!(iFlags & O_PEEKONLY)) {
            LW_SPIPE_INC_CNT(pspipedev, iFlags);                        /*  ���Ӽ���                    */
        }
        
        LW_SPIPE_UNLOCK(pspipedev);                                     /*  �ͷ��豸ʹ��Ȩ              */
        
        LW_DEV_INC_USE_COUNT(&pspipedev->SPIPEDEV_devhdrHdr);
        
        return  ((LONG)pspipefil);
    }
}
/*********************************************************************************************************
** ��������: _SpipeRemove
** ��������: ɾ���ַ����ܵ��豸
** �䡡��  : 
**           pspipedev        �ַ����ܵ��豸���ƿ�
**           pcName           �ַ����ܵ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����Ѿ�ȷ��û���ļ���, ���Բ�����Ҫ iosDevFileAbnormal() ����.
*********************************************************************************************************/
INT  _SpipeRemove (PLW_SPIPE_DEV  pspipedev, PCHAR  pcName)
{
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (LW_DEV_GET_USE_COUNT(&pspipedev->SPIPEDEV_devhdrHdr)) {
        _ErrorHandle(EBUSY);                                            /*  ���ﲻ��ӡ����              */
        return  (PX_ERROR);
    }

    LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                        /*  ����豸����Ȩ��            */
    
    LW_SPIPE_WAKEUP_WOPEN();                                            /*  �������� open ��������      */
    LW_SPIPE_WAKEUP_ROPEN();
    
    iosDevDelete(&pspipedev->SPIPEDEV_devhdrHdr);                       /*  device no longer in system  */
    
    SEL_WAKE_UP_LIST_TERM(&pspipedev->SPIPEDEV_selwulList);
    
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);               /*  terminate binary semaphore  */
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
    API_SemaphoreMDelete(&pspipedev->SPIPEDEV_hOpLock);
    
    __SHEAP_FREE(pspipedev);                                            /*  free pipe memory            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _SpipeBlock
** ��������: �ȴ��¼�����
** �䡡��  : 
**           pspipefil        �ַ����ܵ��ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _SpipeBlock (PLW_SPIPE_FILE  pspipefil)
{
    PLW_SPIPE_DEV  pspipedev;
    
    if (pspipefil && !(pspipefil->SPIPEFIL_iFlags & O_PEEKONLY)) {
        pspipedev = pspipefil->SPIPEFIL_pspipedev;
        
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                    /*  ��ȡ�豸ʹ��Ȩ              */
        
        if (!(pspipefil->SPIPEFIL_iFlags & O_NONBLOCK)) {
            LW_SPIPE_BLOCK(pspipedev, pspipefil->SPIPEFIL_iFlags);
        }
        
        LW_SPIPE_WAKEUP_OPEN(pspipedev, pspipefil->SPIPEFIL_iFlags);    /*  ���� Open ����              */
        
        LW_SPIPE_UNLOCK(pspipedev);                                     /*  �ͷ��豸ʹ��Ȩ              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _SpipeClose
** ��������: �ر��ַ����ܵ��ļ�
** �䡡��  : 
**           pspipefil        �ַ����ܵ��ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SpipeClose (PLW_SPIPE_FILE  pspipefil)
{
    PLW_SPIPE_DEV  pspipedev;

    if (pspipefil) {
        pspipedev = pspipefil->SPIPEFIL_pspipedev;
        
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                    /*  ��ȡ�豸ʹ��Ȩ              */
              
        if (!(pspipefil->SPIPEFIL_iFlags & O_PEEKONLY)) {
            LW_SPIPE_DEC_CNT(pspipedev, pspipefil->SPIPEFIL_iFlags);
            
            if (pspipedev->SPIPEDEV_uiWriteCnt == 0) {                  /*  û��д��                    */
                API_SemaphoreBPost(pspipedev->SPIPEDEV_hReadLock);
            }
            if (pspipedev->SPIPEDEV_uiReadCnt == 0) {                   /*  û�ж���                    */
                API_SemaphoreBPost(pspipedev->SPIPEDEV_hWriteLock);
            }
        }
        
        LW_SPIPE_UNLOCK(pspipedev);                                     /*  �ͷ��豸ʹ��Ȩ              */
    
        __SHEAP_FREE(pspipefil);
    
        if (!LW_DEV_DEC_USE_COUNT(&pspipedev->SPIPEDEV_devhdrHdr)) {
            if (pspipedev->SPIPEDEV_bUnlinkReq) {
                _SpipeRemove(pspipedev, "");                            /*  ɾ���豸                    */
            }
        }
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _SpipeRead
** ��������: ���ַ����ܵ��豸
** �䡡��  : 
**           pspipefil        �ַ����ܵ��ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _SpipeRead (PLW_SPIPE_FILE  pspipefil, 
                     PCHAR           pcBuffer, 
                     size_t          stMaxBytes)
{
    REGISTER size_t     stNBytes;
    REGISTER ssize_t    sstRetVal;
    
    REGISTER PCHAR      pcBase;                                         /*  ����������ַ                */
    REGISTER PCHAR      pcEnd;                                          /*  ������������ַ              */
    REGISTER PCHAR      pcOut;                                          /*  ���ָ��                    */
    
    REGISTER ULONG      ulLwErrCode;
             ULONG      ulTimeout;
             BOOL       bNonblock;
             BOOL       bOrgWriteEn;
    
             PLW_SPIPE_DEV  pspipedev = pspipefil->SPIPEFIL_pspipedev;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �Ƿ����ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  �������ж��е���            */
    }
    
    pspipedev->SPIPEDEV_iAbortFlag &= ~OPT_RABORT;                      /*  ��� abort                  */
    
    if (pspipedev->SPIPEDEV_uiWriteCnt == 0) {                          /*  û��д����û������          */
        if (pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes == 0) {
            if (!(pspipefil->SPIPEFIL_iExtMode & LW_SPIPE_EXT_MODE_NOSIG)) {
                return  (0);
            }
        }
    }
    
    if (pspipefil->SPIPEFIL_iFlags & O_NONBLOCK) {                      /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
        bNonblock = LW_TRUE;
    
    } else {
        ulTimeout = pspipedev->SPIPEDEV_ulRTimeout;
        bNonblock = LW_FALSE;
    }
    
    for (;;) {
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                    /*  ��ȡ�豸ʹ��Ȩ              */

        if (pspipedev->SPIPEDEV_iAbortFlag & OPT_RABORT) {
            LW_SPIPE_UNLOCK(pspipedev);
            _ErrorHandle(ERROR_IO_ABORT);
            return  (0);
        }

        if (pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes) {
            break;                                                      /*  �����ݿɶ�                  */
        
        } else {
#if LW_CFG_SPIPE_MULTI_EN > 0
            API_SemaphoreBClear(pspipedev->SPIPEDEV_hReadLock);         /*  ���ݲ��ɶ�                  */
#endif
            if (pspipedev->SPIPEDEV_uiWriteCnt == 0) {                  /*  �Ѿ�������д��              */
                LW_SPIPE_UNLOCK(pspipedev);
                return  (0);
            }
        }
        
        LW_SPIPE_UNLOCK(pspipedev);                                     /*  �ͷ��豸ʹ��Ȩ              */
        
        ulLwErrCode = API_SemaphoreBPend(pspipedev->SPIPEDEV_hReadLock, /*  �ȴ�������Ч                */
                                         ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  ��ʱ                        */
            if (bNonblock) {
                _ErrorHandle(EAGAIN);
            } else {
                _ErrorHandle(ETIMEDOUT);
            }
            return  (0);
        }
    }
    
    stNBytes  = ((stMaxBytes < pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes) ?
                (stMaxBytes) : (pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes));
    
    sstRetVal = (ssize_t)stNBytes;                                      /*  ���㷵��ֵ                  */
              
    pcBase = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcBuffer;
    pcEnd  = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcBuffer + 
             pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes;
    pcOut  = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcOutPtr;

    {
        REGISTER size_t stLen = pcEnd - pcOut;                          /*  ʣ�ಿ�ֳ���                */
        
        if (stNBytes > stLen) {
            lib_memcpy(pcBuffer, pcOut, stLen);                         /*  ��벿��                    */
            pcBuffer += stLen;
            lib_memcpy(pcBuffer, pcBase, (stNBytes - stLen));           /*  ǰ�벿��                    */
            pcOut = pcBase + (stNBytes - stLen);
        } else if (stNBytes < stLen) {
            lib_memcpy(pcBuffer, pcOut, stNBytes);                      /*  ֱ�ӿ���                    */
            pcOut += stNBytes;
        } else {
            lib_memcpy(pcBuffer, pcOut, stNBytes);
            pcOut = pcBase;                                             /*  ��벿��ȫ������            */
        }
    }
    
    if (__spipe_can_write(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                          pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
        bOrgWriteEn = LW_TRUE;                                          /*  �ж�֮ǰ�Ƿ����д������    */
    
    } else {
        bOrgWriteEn = LW_FALSE;
    }
    
    pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes -= (size_t)sstRetVal;
    pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcOutPtr    = pcOut;
    
    if (__spipe_can_write(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                          pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes) && !bOrgWriteEn) {
        SEL_WAKE_UP_ALL(&pspipedev->SPIPEDEV_selwulList, SELWRITE);
        API_SemaphoreBPost(pspipedev->SPIPEDEV_hWriteLock);             /*  ֪ͨ����д������            */
    }
    
#if LW_CFG_SPIPE_MULTI_EN > 0
    if (__spipe_can_read(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                         pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
        API_SemaphoreBPost(pspipedev->SPIPEDEV_hReadLock);              /*  ֪ͨ�������ݿɶ�            */
    }
#endif
    
    LW_SPIPE_UNLOCK(pspipedev);                                         /*  �ͷ��豸ʹ��Ȩ              */
    
    return  (sstRetVal);
}
/*********************************************************************************************************
** ��������: _SpipeWrite
** ��������: д�ַ����ܵ��豸
** �䡡��  : 
**           pspipefil        �ַ����ܵ��ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _SpipeWrite (PLW_SPIPE_FILE  pspipefil, 
                      PCHAR           pcBuffer, 
                      size_t          stNBytes)
{
    REGISTER size_t     stNBytesToWrite;
    REGISTER ssize_t    sstNBytes = (ssize_t)stNBytes;
    REGISTER size_t     stRetVal;
    REGISTER size_t     stFreeByteSize;
    
    REGISTER PCHAR      pcBase;                                         /*  ����������ַ                */
    REGISTER PCHAR      pcEnd;                                          /*  ������������ַ              */
    REGISTER PCHAR      pcIn;                                           /*  ����ָ��                    */
    
    REGISTER ULONG      ulLwErrCode;
             ULONG      ulTimeout;
             BOOL       bNonblock;
             BOOL       bOrgReadEn;
    
             PLW_SPIPE_DEV  pspipedev = pspipefil->SPIPEFIL_pspipedev;
    
    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �Ƿ����ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
__continue_write:
    pspipedev->SPIPEDEV_iAbortFlag &= ~OPT_WABORT;                      /*  ��� abort                  */
    
    if (pspipedev->SPIPEDEV_uiReadCnt == 0) {                           /*  û�ж���                    */
#if LW_CFG_SIGNAL_EN > 0
        if (!(pspipefil->SPIPEFIL_iExtMode & LW_SPIPE_EXT_MODE_NOSIG)) {
            sigevent_t  sigeventPipe;

            sigeventPipe.sigev_signo           = SIGPIPE;
            sigeventPipe.sigev_value.sival_ptr = LW_NULL;
            sigeventPipe.sigev_notify          = SIGEV_SIGNAL;
            _doSigEvent(API_ThreadIdSelf(), &sigeventPipe, SI_MESGQ);   /*  ���� SIGPIPE �ź�           */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        _ErrorHandle(EPIPE);
        return  (PX_ERROR);
    }
    
    if (pspipefil->SPIPEFIL_iFlags & O_NONBLOCK) {                      /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
        bNonblock = LW_TRUE;

    } else {
        ulTimeout = pspipedev->SPIPEDEV_ulWTimeout;
        bNonblock = LW_FALSE;
    }
    
    for (;;) {
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));                    /*  ��ȡ�豸ʹ��Ȩ              */

        if (pspipedev->SPIPEDEV_iAbortFlag & OPT_WABORT) {
            LW_SPIPE_UNLOCK(pspipedev);
            _ErrorHandle(ERROR_IO_ABORT);
            return  (0);
        }
        
        if (pspipedev->SPIPEDEV_uiReadCnt == 0) {                       /*  û�ж���                    */
            LW_SPIPE_UNLOCK(pspipedev);
            _ErrorHandle(EPIPE);
            return  (PX_ERROR);
        }
        
        stFreeByteSize = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes - 
                         pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes;
        
        if (stFreeByteSize >= __MIN(stNBytes, PIPE_BUF)) {              /*  �жϿ��пռ��Ƿ�������Ҫ    */
            break;
        
        } 
#if LW_CFG_SPIPE_MULTI_EN > 0
          else {
            API_SemaphoreBClear(pspipedev->SPIPEDEV_hWriteLock);        /*  ����д��                    */
        }
#endif
        
        LW_SPIPE_UNLOCK(pspipedev);                                     /*  �ͷ��豸ʹ��Ȩ              */
    
        ulLwErrCode = API_SemaphoreBPend(pspipedev->SPIPEDEV_hWriteLock,/*  �ȴ��ռ�д��                */
                                         ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  ��ʱ                        */
            if (bNonblock) {
                _ErrorHandle(EAGAIN);
            } else {
                _ErrorHandle(ETIMEDOUT);
            }
            return  (sstNBytes - stNBytes);
        }
    }
    
    stNBytesToWrite = ((stNBytes < stFreeByteSize) ?
                      (stNBytes) : (stFreeByteSize));
                     
    stRetVal = (size_t)stNBytesToWrite;                                 /*  ���㷵��ֵ                  */
                     
    pcBase = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcBuffer;
    pcEnd  = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcBuffer + 
             pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes;
    pcIn   = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcInPtr;

    {
        REGISTER size_t stLen = pcEnd - pcIn;                           /*  ʣ�ಿ�ֳ���                */
        
        if (stNBytesToWrite > stLen) {
            lib_memcpy(pcIn, pcBuffer, stLen);                          /*  ��벿��                    */
            pcBuffer += stLen;
            lib_memcpy(pcBase, pcBuffer, (stNBytesToWrite - stLen));    /*  ǰ�벿��                    */
            pcIn      = pcBase + (stNBytesToWrite - stLen);
            pcBuffer += stNBytesToWrite - stLen;
        } else if (stNBytesToWrite < stLen) {
            lib_memcpy(pcIn, pcBuffer, stNBytesToWrite);                /*  ֱ�ӿ���                    */
            pcIn     += stNBytesToWrite;
            pcBuffer += stNBytesToWrite;
        } else {
            lib_memcpy(pcIn, pcBuffer, stNBytesToWrite);
            pcIn      = pcBase;                                         /*  ��벿��ȫ������            */
            pcBuffer += stNBytesToWrite;
        }
    }
    
    if (__spipe_can_read(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                         pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
        bOrgReadEn = LW_TRUE;
        
    } else {
        bOrgReadEn = LW_FALSE;
    }
    
    pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes += (size_t)stRetVal;
    pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcInPtr     = pcIn;
    
    if (__spipe_can_read(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                         pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes) && !bOrgReadEn) {
        SEL_WAKE_UP_ALL(&pspipedev->SPIPEDEV_selwulList, SELREAD);
        API_SemaphoreBPost(pspipedev->SPIPEDEV_hReadLock);              /*  ֪ͨ�������ݿɶ�            */
    }
    
#if LW_CFG_SPIPE_MULTI_EN > 0
    if (__spipe_can_write(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                          pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
        API_SemaphoreBPost(pspipedev->SPIPEDEV_hWriteLock);             /*  ֪ͨ����д������            */
    }
#endif
    
    LW_SPIPE_UNLOCK(pspipedev);
    
    stNBytes -= stRetVal;
    if (stNBytes) {
        goto    __continue_write;
    }
    
    return  (sstNBytes);
}
/*********************************************************************************************************
** ��������: _SpipeIoctl
** ��������: �����ַ����ܵ��豸
** �䡡��  : 
**           pspipefil        �ַ����ܵ��ļ�
**           iRequest         ����
**           piArgPtr         ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : FIONMSGS �� VxWorks ������.
*********************************************************************************************************/
INT  _SpipeIoctl (PLW_SPIPE_FILE pspipefil, 
                  INT            iRequest, 
                  INT           *piArgPtr)
{
    REGISTER INT                  iErrCode = ERROR_NONE;
    REGISTER PCHAR                pcBufferBase;
    
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode;
             struct stat         *pstatGet;
             PLW_SPIPE_DEV        pspipedev = pspipefil->SPIPEFIL_pspipedev;
    
    switch (iRequest) {
    
    case FIOSEEK:
    case FIOWHERE:
        iErrCode = PX_ERROR;
        _ErrorHandle(ESPIPE);
        break;
    
    case FIONREAD:                                                      /*  ��ùܵ������ݵ��ֽڸ���    */
        *(INT *)piArgPtr = (INT)pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes;
        break;
        
    case FIONMSGS:                                                      /*  ��ùܵ������ݵĸ���        */
        if (pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes) {
            *piArgPtr = 1;
        } else {
            *piArgPtr = 0;
        }
        break;
        
    case FIONFREE:                                                      /*  ���пռ��С                */
        *(INT *)piArgPtr = (INT)(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes
                         - pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes);
        break;

    case FIONBIO:
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        if (*piArgPtr) {
            pspipefil->SPIPEFIL_iFlags |= O_NONBLOCK;
        } else {
            pspipefil->SPIPEFIL_iFlags &= ~O_NONBLOCK;
        }
        LW_SPIPE_UNLOCK(pspipedev);
        break;
        
    case FIOPIPEBLOCK:                                                  /*  ��������                    */
        iErrCode = _SpipeBlock(pspipefil);
        break;
        
    case FIOPIPERDONLY:                                                 /*  �� pipe ����ʹ��            */
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        if ((pspipefil->SPIPEFIL_iFlags & O_ACCMODE) == O_RDWR) {
            pspipefil->SPIPEFIL_iFlags &= ~O_ACCMODE;                   /*  RDONLY == 0                 */
            pspipedev->SPIPEDEV_uiWriteCnt--;
        } else {
            iErrCode = PX_ERROR;
            _ErrorHandle(ENOTSUP);
        }
        LW_SPIPE_UNLOCK(pspipedev);
        break;
        
    case FIOFLUSH:                                                      /*  �������                    */
        pcBufferBase = pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcBuffer;
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcInPtr    = pcBufferBase;
        pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_pcOutPtr   = pcBufferBase;
        pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes = 0;
        LW_SPIPE_UNLOCK(pspipedev);
        
        SEL_WAKE_UP_ALL(&pspipedev->SPIPEDEV_selwulList, SELWRITE);     /*  ֪ͨ���ݿ�д                */
        API_SemaphoreBPost(pspipedev->SPIPEDEV_hWriteLock);
        break;
        
    case FIOFSTATGET:                                                   /*  ��ȡ�ļ�����                */
        pstatGet = (struct stat *)piArgPtr;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&pspipedev->SPIPEDEV_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0666 | S_IFIFO;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = pspipedev->SPIPEDEV_timeCreate;
            pstatGet->st_mtime   = pspipedev->SPIPEDEV_timeCreate;
            pstatGet->st_ctime   = pspipedev->SPIPEDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
        break;
    
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)piArgPtr;
        SEL_WAKE_NODE_ADD(&pspipedev->SPIPEDEV_selwulList, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:                                                   /*  �ȴ����ݿɶ�                */
            if (__spipe_can_read(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                                 pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
                SEL_WAKE_UP(pselwunNode);                               /*  ���ѽڵ�                    */
            } else if (pspipedev->SPIPEDEV_uiWriteCnt == 0) {
                SEL_WAKE_UP(pselwunNode);                               /*  û��д��Ҳ��Ҫ���ѽڵ�      */
            }
            break;
            
        case SELWRITE:
            if (__spipe_can_write(pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stMsgBytes,
                                  pspipedev->SPIPEDEV_ringbufferBuffer.RINGBUFFER_stTotalBytes)) {
                SEL_WAKE_UP(pselwunNode);                               /*  ���ѽڵ�                    */
            } else if (pspipedev->SPIPEDEV_uiReadCnt == 0) {
                SEL_WAKE_UP(pselwunNode);                               /*  û�ж���Ҳ��Ҫ���ѽڵ�      */
            }
            break;
            
        case SELEXCEPT:                                                 /*  �豸ɾ��ʱ���ᱻ����        */
            break;
        }
        break;

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pspipedev->SPIPEDEV_selwulList, (PLW_SEL_WAKEUPNODE)piArgPtr);
        break;
        
    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                pspipedev->SPIPEDEV_ulRTimeout = ulTick;
            } else {
                pspipedev->SPIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWTIMEOUT:
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                pspipedev->SPIPEDEV_ulWTimeout = ulTick;
            } else {
                pspipedev->SPIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
    
    case FIOWAITABORT:                                                  /*  ֹͣ��ǰ�ȴ� IO �߳�        */
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        if ((INT)(LONG)piArgPtr & OPT_RABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(pspipedev->SPIPEDEV_hReadLock, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                pspipedev->SPIPEDEV_iAbortFlag |= OPT_RABORT;
                API_SemaphoreBPost(pspipedev->SPIPEDEV_hReadLock);      /*  ������ȴ��߳�              */
            }
        }
        if ((INT)(LONG)piArgPtr & OPT_WABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(pspipedev->SPIPEDEV_hWriteLock, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                pspipedev->SPIPEDEV_iAbortFlag |= OPT_WABORT;
                API_SemaphoreBPost(pspipedev->SPIPEDEV_hWriteLock);     /*  ������ȴ��߳�              */
            }
        }
        LW_SPIPE_UNLOCK(pspipedev);
        break;
        
    case FIOUNMOUNT:                                                    /*  ���һ�ιر�ʱɾ���豸      */
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        pspipedev->SPIPEDEV_bUnlinkReq = LW_TRUE;
        LW_SPIPE_UNLOCK(pspipedev);
        break;
        
    case FIOPIPENOSIG:                                                  /*  ����Ҫ�ź�                  */
        LW_SPIPE_LOCK(pspipedev, return (PX_ERROR));
        if ((INT)(LONG)piArgPtr) {
            pspipefil->SPIPEFIL_iExtMode |= LW_SPIPE_EXT_MODE_NOSIG;
        } else {
            pspipefil->SPIPEFIL_iExtMode &= ~LW_SPIPE_EXT_MODE_NOSIG;
        }
        LW_SPIPE_UNLOCK(pspipedev);
        break;

    default:
        iErrCode = PX_ERROR;
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        break;
    }
    
    return  (iErrCode);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
