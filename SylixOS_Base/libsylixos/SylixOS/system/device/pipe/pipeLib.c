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
** ��   ��   ��: pipeLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: VxWorks ���ݹܵ�ͨ���ڲ����ܺ���

** BUG
2007.04.09  _PipeRead()  �����޸������ж��е��õ����⡣
2007.04.09  _PipeWrite() �����޸������ж���û��д����е����⡣
2007.06.06  LINE72  Ӧ�����ж���Ч���ٴ���򿪴�����
2007.09.21  ����������.
2007.12.11  ����չܵ���Ϣʱ,��Ҫ����д�ȴ����߳�.
2008.04.01  �� FIORTIMEOUT �� FIOWTIMEOUT ���ó�ʱʱ��Ĳ�����Ϊ struct timeval ����.
            Ϊ NULL ��ʾ���õȴ�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.02.09  ioctl ����ʶ������, ����ӡ������Ϣ.
2009.05.27  ���� abort ����.
2009.07.11  ȥ��һЩ GCC ����Ϳ����� 64 λ�������ϲ���������.
2009.10.22  read write ����ֵΪ ssize_t.
2010.01.13  �Ż� write ����.
2010.01.14  ������ abort.
2010.09.09  ֧�� SELEXCEPT ����. ���ܵ��豸ɾ��ʱ���ᱻ����.
2011.08.09  st_size �ܵ�Ϊ�ڲ�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __PIPE_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** ��������: _PipeOpen
** ��������: �򿪹ܵ��豸
** �䡡��  : 
**           p_pipedev        �ܵ��豸���ƿ�
**           pcName           �ܵ�����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

LONG  _PipeOpen (PLW_PIPE_DEV  p_pipedev, 
                 PCHAR         pcName,   
                 INT           iFlags, 
                 INT           iMode)
{
    if (pcName == LW_NULL) {
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
        
        p_pipedev->PIPEDEV_iFlags = iFlags;
        p_pipedev->PIPEDEV_iMode  = iMode;
        LW_DEV_INC_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr);
        
        return  ((LONG)p_pipedev);
    }
}
/*********************************************************************************************************
** ��������: _PipeClose
** ��������: �رչܵ��豸
** �䡡��  : 
**           p_pipedev        �ܵ��豸���ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PipeClose (PLW_PIPE_DEV  p_pipedev)
{
    if (p_pipedev) {
        LW_DEV_DEC_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _PipeRead
** ��������: ���ܵ��豸
** �䡡��  : 
**           p_pipedev        �ܵ��豸���ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PipeRead (PLW_PIPE_DEV  p_pipedev, 
                    PCHAR         pcBuffer, 
                    size_t        stMaxBytes)
{
    REGISTER ULONG      ulError;
             size_t     stTemp = 0;
             ssize_t    sstNBytes;
    
    p_pipedev->PIPEDEV_iAbortFlag &= ~OPT_RABORT;                       /*  ��� abort ��־             */
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �жϵ���                    */
        ulError = API_MsgQueueTryReceive(p_pipedev->PIPEDEV_hMsgQueue,
                                         (PVOID)pcBuffer,
                                         stMaxBytes,
                                         &stTemp);
    } else {                                                            /*  ��ͨ����                    */
        ulError = API_MsgQueueReceive(p_pipedev->PIPEDEV_hMsgQueue,
                                      (PVOID)pcBuffer,
                                      stMaxBytes,
                                      &stTemp,
                                      p_pipedev->PIPEDEV_ulRTimeout);
    }

    sstNBytes = (ssize_t)stTemp;
    if (ulError) {                                                      /*  �����˳�                    */
        return  (0);
    }
    
    if (p_pipedev->PIPEDEV_iAbortFlag & OPT_RABORT) {                   /*  abort                       */
        _ErrorHandle(ERROR_IO_ABORT);
        return  (0);
    }
    
    SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELWRITE);
    
    return  (sstNBytes);
}
/*********************************************************************************************************
** ��������: _SpipeWrite
** ��������: д�ܵ��豸
** �䡡��  : 
**           p_pipedev        �ܵ��豸���ƿ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PipeWrite (PLW_PIPE_DEV  p_pipedev, 
                     PCHAR         pcBuffer, 
                     size_t        stNBytes)
{
    REGISTER ULONG  ulError;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж��е���                */
        ulError = API_MsgQueueSend(p_pipedev->PIPEDEV_hMsgQueue,
                                   (PVOID)pcBuffer, stNBytes);          /*  ������Ϣ                    */
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);
            return  (0);
        }
        
    } else {
        p_pipedev->PIPEDEV_iAbortFlag &= ~OPT_WABORT;                   /*  ��� abort ��־             */
        
        ulError = API_MsgQueueSend2(p_pipedev->PIPEDEV_hMsgQueue,
                                    (PVOID)pcBuffer, stNBytes,
                                    p_pipedev->PIPEDEV_ulWTimeout);     /*  ������Ϣ                    */
        if (ulError) {                                                  /*  ����д                      */
            if (p_pipedev->PIPEDEV_iAbortFlag & OPT_WABORT) {           /*  abort                       */
                _ErrorHandle(ERROR_IO_ABORT);
            
            } else if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
                _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);
            }
            return  (0);
        }
    }
    
    SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELREAD);           /*  ���Զ���                    */
    
    return  ((ssize_t)stNBytes);
}
/*********************************************************************************************************
** ��������: _PipeIoctl
** ��������: ���ƹܵ��豸
** �䡡��  : 
**           p_pipedev        �ܵ��豸���ƿ�
**           iRequest         ����
**           piArgPtr         ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PipeIoctl (PLW_PIPE_DEV  p_pipedev, 
                 INT           iRequest, 
                 INT          *piArgPtr)
{
    REGISTER INT                  iErrCode = ERROR_NONE;
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode;
             ULONG                ulMsgNum;
             ULONG                ulMsgMax;
             ULONG                ulTemp = 0;
             
             struct stat         *pstatGet;
    
    switch (iRequest) {
    
    case FIOSEEK:
    case FIOWHERE:
        iErrCode = PX_ERROR;
        _ErrorHandle(ESPIPE);
        break;
        
    case FIONREAD:                                                      /*  ��ùܵ������ݵ��ֽڸ���    */
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           LW_NULL,
                           (size_t *)&ulTemp,
                           LW_NULL,
                           LW_NULL);
        *piArgPtr = (INT)ulTemp;
        break;
        
    case FIONMSGS:                                                      /*  ��ùܵ������ݵĸ���        */
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           &ulTemp,
                           LW_NULL,
                           LW_NULL,
                           LW_NULL);
        *piArgPtr = (INT)ulTemp;
        break;
        
    case FIOPIPEBLOCK:                                                  /*  ��������                    */
        break;
        
    case FIOFLUSH:                                                      /*  �������                    */
        API_MsgQueueClear(p_pipedev->PIPEDEV_hMsgQueue);                /*  �����Ϣ������Ϣ            */
        SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELWRITE);      /*  ���ݿ���д����              */
        break;
        
    case FIOFSTATGET: {                                                 /*  ��ȡ�ļ�����                */
        ULONG   ulNMsg;
        size_t  stNRead;
        
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           &ulNMsg,
                           &stNRead,
                           LW_NULL,
                           LW_NULL);
        
        pstatGet = (struct stat *)piArgPtr;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&p_pipedev->PIPEDEV_devhdrHdr);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0666 | S_IFIFO;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)(ulNMsg * stNRead);
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = p_pipedev->PIPEDEV_timeCreate;
            pstatGet->st_mtime   = p_pipedev->PIPEDEV_timeCreate;
            pstatGet->st_ctime   = p_pipedev->PIPEDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
    }
        break;
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)piArgPtr;
        SEL_WAKE_NODE_ADD(&p_pipedev->PIPEDEV_selwulList, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:                                                   /*  �ȴ����ݿɶ�                */
            API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                               LW_NULL,
                               &ulMsgNum,
                               LW_NULL,
                               LW_NULL,
                               LW_NULL);
            if (ulMsgNum > 0) {
                SEL_WAKE_UP(pselwunNode);                               /*  ���ѽڵ�                    */
            }
            break;
            
        case SELWRITE:
            API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                               &ulMsgMax,
                               &ulMsgNum,
                               LW_NULL,
                               LW_NULL,
                               LW_NULL);
            if (ulMsgNum < ulMsgMax) {
                SEL_WAKE_UP(pselwunNode);                               /*  ���ѽڵ�                    */
            }
            break;
            
        case SELEXCEPT:                                                 /*  �豸ɾ��ʱ���ᱻ����        */
            break;
        }
        break;

    case FIOUNSELECT:
	    SEL_WAKE_NODE_DELETE(&p_pipedev->PIPEDEV_selwulList, (PLW_SEL_WAKEUPNODE)piArgPtr);
        break;
        
    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                p_pipedev->PIPEDEV_ulRTimeout = ulTick;
            } else {
                p_pipedev->PIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWTIMEOUT:                                                   /*  ����д��ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                p_pipedev->PIPEDEV_ulWTimeout = ulTick;
            } else {
                p_pipedev->PIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOWAITABORT:                                                  /*  ֹͣ��ǰ�ȴ� IO �߳�        */
        if ((INT)(LONG)piArgPtr & OPT_RABORT) {
            p_pipedev->PIPEDEV_iAbortFlag |= OPT_RABORT;
            API_MsgQueueFlushReceive(p_pipedev->PIPEDEV_hMsgQueue, LW_NULL);
        }
        if ((INT)(LONG)piArgPtr & OPT_WABORT) {
            p_pipedev->PIPEDEV_iAbortFlag |= OPT_WABORT;
            API_MsgQueueFlushSend(p_pipedev->PIPEDEV_hMsgQueue, LW_NULL);
        }
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
