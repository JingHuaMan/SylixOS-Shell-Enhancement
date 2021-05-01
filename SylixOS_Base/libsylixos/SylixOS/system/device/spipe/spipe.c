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
** ��   ��   ��: spipe.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: �ַ����ܵ�ͨ�Žӿ� (���߳�ͬʱд���Ҷ��߳�ͬʱ��ʱ, ��ʱʱ����ܲ�׼ȷ)

** BUG
2007.06.06  LINE259 ��������ֵ����
2007.09.21  ���������������л���.
2007.09.21  ����������.
2007.09.25  �������֤����.
2007.11.18  ����ע��.
2007.11.20  ���� select ����.
2007.11.21  ���� spipe �豸ʱ, ���λ������Ļ���ַ��ȫ����, (�˴��������˼�����, �÷�˼��).
2007.11.21  �ھ���ɾ���豸ʱ,ɾ���������豸��ص��ļ�.
2007.12.11  ��ɾ���豸ʱ,��������ͨ�� select �ȴ����߳�.
2008.01.13  ���� _ErrorHandle(ERROR_NONE); ���.
2008.01.16  �޸����ź���������.
2008.01.20  ɾ������ʱ,Ҫ����豸����Ȩ��.
2008.08.12  �޸�ע��.
2009.03.06  �޸ĳ�ʼ��˳��, ȷ��׼ȷ����.
2009.12.09  �޸�ע��.
2010.09.11  �����豸ʱ, ָ���豸����.
2012.08.25  ����ܵ����˹رռ��:
            1: ������˹ر�, д���������յ� SIGPIPE �ź�, write ���᷵�� -1.
            2: ���д�˹ر�, ���������������������, Ȼ���ٴζ����� 0.
2013.06.12  �ܵ���С����С�� PIPE_BUF.
2013.10.03  �����ܵ��豸����ʧ�ܺ���ڴ���ͷŴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "limits.h"
/*********************************************************************************************************
  ȫ���ź���
*********************************************************************************************************/
LW_OBJECT_HANDLE     _G_ulSpipeReadOpenLock;
LW_OBJECT_HANDLE     _G_ulSpipeWriteOpenLock;
/*********************************************************************************************************
** ��������: API_SpipeDrvInstall
** ��������: ��װ�ַ����ܵ��豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

LW_API 
INT  API_SpipeDrvInstall (VOID)
{
    if (_G_iSpipeDrvNum <= 0) {
        _G_iSpipeDrvNum  = iosDrvInstall(LW_NULL,                       /*  CREATE                      */
                                         _SpipeRemove,                  /*  DELETE                      */
                                         _SpipeOpen,                    /*  OPEN                        */
                                         _SpipeClose,                   /*  CLOSE                       */
                                         _SpipeRead,                    /*  READ                        */
                                         _SpipeWrite,                   /*  WRITE                       */
                                         _SpipeIoctl);                  /*  IOCTL                       */
        
        DRIVER_LICENSE(_G_iSpipeDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iSpipeDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iSpipeDrvNum, "stream pipe driver.");
    }
    
    if (!_G_ulSpipeReadOpenLock) {
        _G_ulSpipeReadOpenLock = API_SemaphoreBCreate("pipe_ropen",
                                                      LW_FALSE, 
                                                      LW_OPTION_OBJECT_GLOBAL, 
                                                      LW_NULL);
        _BugHandle(!_G_ulSpipeReadOpenLock, LW_TRUE, "can not create pipe open lock!\r\n");
    }
    
    if (!_G_ulSpipeWriteOpenLock) {
        _G_ulSpipeWriteOpenLock = API_SemaphoreBCreate("pipe_wopen",
                                                       LW_FALSE, 
                                                       LW_OPTION_OBJECT_GLOBAL, 
                                                       LW_NULL);
        _BugHandle(!_G_ulSpipeWriteOpenLock, LW_TRUE, "can not create pipe open lock!\r\n");
    }
    
    return  ((_G_iSpipeDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_SpipeDevCreate
** ��������: ����һ���ַ����ܵ��豸
** �䡡��  : 
**           pcName                        �ַ����ܵ�����
**           stBufferByteSize              ��������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SpipeDevCreate (PCHAR  pcName, size_t  stBufferByteSize)
{
    REGISTER PLW_SPIPE_DEV       pspipedev;
    REGISTER PLW_RING_BUFFER     pringbuffer;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (stBufferByteSize < PIPE_BUF) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iSpipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pspipedev = (PLW_SPIPE_DEV)__SHEAP_ALLOC(sizeof(LW_SPIPE_DEV) + stBufferByteSize);
    if (pspipedev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pspipedev, sizeof(LW_SPIPE_DEV));
    
    pspipedev->SPIPEDEV_uiReadCnt  = 0;
    pspipedev->SPIPEDEV_uiWriteCnt = 0;
    pspipedev->SPIPEDEV_bUnlinkReq = LW_FALSE;
    pspipedev->SPIPEDEV_iAbortFlag = 0;
    pspipedev->SPIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;            /*  ��ʼ��Ϊ���õȴ�           */
    pspipedev->SPIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;            /*  ��ʼ��Ϊ���õȴ�           */
    
    /*
     *  ���ڲ��� test-pend ����, ȫ����ʼ��Ϊ��Ч״̬.
     */
    pspipedev->SPIPEDEV_hReadLock = API_SemaphoreBCreate("pipe_rsync",   /*  create lock                */
                                                         LW_FALSE, 
                                                         _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL, 
                                                         LW_NULL);
    if (!pspipedev->SPIPEDEV_hReadLock) {
        __SHEAP_FREE(pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_hWriteLock = API_SemaphoreBCreate("pipe_wsync", 
                                                          LW_FALSE,
                                                          _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
                                                        
    if (!pspipedev->SPIPEDEV_hWriteLock) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        __SHEAP_FREE(pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_hOpLock = API_SemaphoreMCreate("pipe_lock", 
                                                        LW_PRIO_DEF_CEILING,
                                                        _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                        LW_NULL);
    if (!pspipedev->SPIPEDEV_hOpLock) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
        __SHEAP_FREE(pspipedev);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&pspipedev->SPIPEDEV_selwulList);
    
    pringbuffer = &pspipedev->SPIPEDEV_ringbufferBuffer;
    
    pringbuffer->RINGBUFFER_pcBuffer = (PCHAR)((UINT8 *)(pspipedev) + sizeof(LW_SPIPE_DEV));
    pringbuffer->RINGBUFFER_pcInPtr  = pringbuffer->RINGBUFFER_pcBuffer;
    pringbuffer->RINGBUFFER_pcOutPtr = pringbuffer->RINGBUFFER_pcBuffer;
    
    pringbuffer->RINGBUFFER_stTotalBytes = stBufferByteSize;
    pringbuffer->RINGBUFFER_stMsgBytes   = 0;
    
    if (iosDevAddEx(&pspipedev->SPIPEDEV_devhdrHdr, pcName, _G_iSpipeDrvNum, DT_FIFO) != ERROR_NONE) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
        API_SemaphoreMDelete(&pspipedev->SPIPEDEV_hOpLock);
        SEL_WAKE_UP_LIST_TERM(&pspipedev->SPIPEDEV_selwulList);
        __SHEAP_FREE(pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_timeCreate = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpipeDevDelete
** ��������: ɾ��һ���ַ����ܵ��豸
** �䡡��  : 
**           pcName                        �ַ����ܵ�����
**           bForce                        ǿ��ɾ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SpipeDevDelete (PCHAR  pcName, BOOL  bForce)
{
    REGISTER PLW_SPIPE_DEV       pspipedev;
             PCHAR               pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iSpipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pspipedev = (PLW_SPIPE_DEV)iosDevFind(pcName, &pcTail);
    if ((pspipedev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&pspipedev->SPIPEDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&pspipedev->SPIPEDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    if (API_SemaphoreMPend(pspipedev->SPIPEDEV_hOpLock, LW_OPTION_WAIT_INFINITE)) {
        return  (PX_ERROR);
    }
    
    iosDevFileAbnormal(&pspipedev->SPIPEDEV_devhdrHdr);
    iosDevDelete(&pspipedev->SPIPEDEV_devhdrHdr);
    
    SEL_WAKE_UP_LIST_TERM(&pspipedev->SPIPEDEV_selwulList);
    
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
    API_SemaphoreMDelete(&pspipedev->SPIPEDEV_hOpLock);
    
    __SHEAP_FREE(pspipedev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SPIPE_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
