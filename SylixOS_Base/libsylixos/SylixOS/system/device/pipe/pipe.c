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
** ��   ��   ��: pipe.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: VxWorks �ܵ�ͨ�Žӿ�

** BUG
2007.06.05  LINE155 Ӧ�÷��� ERROR_NONE Ӧ���������أ��豸�����ɹ���
2007.06.06  LINE246 Ӧ�÷��� ERROR_NONE Ӧ���������أ��豸ɾ���ɹ���
2007.09.21  ����������.
2007.09.25  �������֤����.
2007.11.18  ����ע��.
2007.11.21  ���� select ����.
2007.11.21  �ھ���ɾ���豸ʱ,ɾ���������豸��ص��ļ�.
2007.12.11  ��ɾ���豸ʱ,��������ͨ�� select �ȴ����߳�.
2008.01.13  ���� _ErrorHandle(ERROR_NONE); ���.
2008.01.16  �޸����ź���������.
2008.08.12  �޸�ע��.
2009.12.09  �޸�ע��.
2010.01.04  ʹ���µ�ʱ����Ϣ.
2010.01.14  ������ abort.
2010.09.11  �����豸ʱ, ָ���豸����.
2016.07.21  ������Ҫдͬ���ź���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** ��������: API_PipeDrvInstall
** ��������: ��װ�ܵ��豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PIPE_EN > 0)

LW_API 
INT  API_PipeDrvInstall (VOID)
{
    if (_G_iPipeDrvNum <= 0) {
        _G_iPipeDrvNum  = iosDrvInstall((LONGFUNCPTR)LW_NULL,           /*  CREATE                      */
                                        (FUNCPTR)LW_NULL,               /*  DELETE                      */
                                        _PipeOpen,                      /*  OPEN                        */
                                        _PipeClose,                     /*  CLOSE                       */
                                        _PipeRead,                      /*  READ                        */
                                        _PipeWrite,                     /*  WRITE                       */
                                        _PipeIoctl);                    /*  IOCTL                       */
                                          
        DRIVER_LICENSE(_G_iPipeDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iPipeDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iPipeDrvNum, "VxWorks pipe driver.");
    }
    
    return  ((_G_iPipeDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_PipeDevCreate
** ��������: ����һ���ܵ��豸
** �䡡��  : 
**           pcName                        �ܵ�����
**           ulNMessages                   �ܵ�����Ϣ����
**           stNBytes                      ÿһ����Ϣ������ֽ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PipeDevCreate (PCHAR  pcName, 
                        ULONG  ulNMessages, 
                        size_t stNBytes)
{
    REGISTER PLW_PIPE_DEV        p_pipedev;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iPipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_pipedev = (PLW_PIPE_DEV)__SHEAP_ALLOC(sizeof(LW_PIPE_DEV));
    if (p_pipedev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(p_pipedev, sizeof(LW_PIPE_DEV));
    
    p_pipedev->PIPEDEV_iAbortFlag = 0;
    p_pipedev->PIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;             /*  ��ʼ��Ϊ���õȴ�           */
    p_pipedev->PIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;             /*  ��ʼ��Ϊ���õȴ�           */
    
    p_pipedev->PIPEDEV_hMsgQueue  = API_MsgQueueCreate("pipe_msg",
                                                       ulNMessages, stNBytes,
                                                       _G_ulPipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
    if (!p_pipedev->PIPEDEV_hMsgQueue) {
        __SHEAP_FREE(p_pipedev);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&p_pipedev->PIPEDEV_selwulList);
    
    if (iosDevAddEx(&p_pipedev->PIPEDEV_devhdrHdr, pcName, _G_iPipeDrvNum, DT_FIFO) != ERROR_NONE) {
        API_MsgQueueDelete(&p_pipedev->PIPEDEV_hMsgQueue);
        SEL_WAKE_UP_LIST_TERM(&p_pipedev->PIPEDEV_selwulList);
        __SHEAP_FREE(p_pipedev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    p_pipedev->PIPEDEV_timeCreate = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PipeDevDelete
** ��������: ɾ��һ���ܵ��豸
** �䡡��  : 
**           pcName                        �ܵ�����
**           bForce                        ǿ��ɾ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PipeDevDelete (PCHAR  pcName, BOOL  bForce)
{
    REGISTER PLW_PIPE_DEV        p_pipedev;
             PCHAR               pcTail = LW_NULL;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iPipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_pipedev = (PLW_PIPE_DEV)iosDevFind(pcName, &pcTail);
    if ((p_pipedev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&p_pipedev->PIPEDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    iosDevFileAbnormal(&p_pipedev->PIPEDEV_devhdrHdr);
    iosDevDelete(&p_pipedev->PIPEDEV_devhdrHdr);

    SEL_WAKE_UP_LIST_TERM(&p_pipedev->PIPEDEV_selwulList);
    
    API_MsgQueueDelete(&p_pipedev->PIPEDEV_hMsgQueue);

    __SHEAP_FREE(p_pipedev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
