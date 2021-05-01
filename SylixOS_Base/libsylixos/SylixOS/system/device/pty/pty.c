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
** ��   ��   ��: pty.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 15 ��
**
** ��        ��: �����ն˽ӿڲ���.
                 �����ն˷�Ϊ�����˿�: �豸�˺����ض�! 
                 �豸�������������һ��Ӳ������.
                 ���ض˿��Կ��ɾ���һ�� TTY �豸.
                 
** BUG:
2009.09.05  ��ɾ�� pty �豸ʱ, �����д��豸���ļ�����Ϊ�쳣ģʽ(Ԥ�ر�ģʽ), ���ȴ��ر�.
2009.10.22  read write ����ֵ��Ϊ ssize_t.
2010.09.11  �����豸ʱ, ָ���豸����.
2012.08.06  �ж��в���ɾ�����ߴ����豸.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_PTY_DEVICE_EN > 0)
#include "ptyLib.h"
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT      _G_iPtyDeviceDrvNum = PX_ERROR;                         /*  �豸���������豸��          */
static INT      _G_iPtyHostDrvNum   = PX_ERROR;                         /*  ���ض��������豸��          */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
LONG    _PtyHostOpen(P_PTY_DEV      p_ptydev,
                     PCHAR          pcName, 
                     INT            iFlags, 
                     INT            iMode);
INT     _PtyHostClose(P_PTY_DEV    p_ptydev);
ssize_t _PtyHostRead(P_PTY_DEV     p_ptydev, 
                     PCHAR         pcBuffer, 
                     size_t        stMaxBytes);
ssize_t _PtyHostWrite(P_PTY_DEV     p_ptydev, 
                      PCHAR         pcBuffer, 
                      size_t        stNBytes);
INT     _PtyHostIoctl(P_PTY_DEV     p_ptydev, 
                      INT           iRequest,
                      LONG          lArg);
LONG    _PtyDeviceOpen(P_PTY_D_DEV  p_ptyddev,
                       PCHAR        pcName,   
                       INT          iFlags, 
                       INT          iMode);
INT     _PtyDeviceClose(P_PTY_DEV    p_ptydev);
ssize_t _PtyDeviceRead(P_PTY_DEV     p_ptydev, 
                       PCHAR         pcBuffer, 
                       size_t        stMaxBytes);
ssize_t _PtyDeviceWrite(P_PTY_DEV     p_ptydev, 
                        PCHAR         pcBuffer, 
                        size_t        stNBytes);
INT     _PtyDeviceIoctl(P_PTY_DEV     p_ptydev, 
                        INT           iRequest,
                        LONG          lArg);
VOID    _PtyDeviceStartup(P_PTY_DEV   p_ptydev);
/*********************************************************************************************************
** ��������: API_PtyDrvInstall
** ��������: ��װPTY�豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PtyDrvInstall (VOID)
{
    if (_G_iPtyDeviceDrvNum > 0) {
        return  (ERROR_NONE);                                           /*  �����Ѿ���װ                */
    }
    
    _G_iPtyDeviceDrvNum = iosDrvInstall(_PtyDeviceOpen,
                                        (FUNCPTR)LW_NULL,
                                        _PtyDeviceOpen,
                                        _PtyDeviceClose,
                                        _PtyDeviceRead,
                                        _PtyDeviceWrite,
                                        _PtyDeviceIoctl);
    if (_G_iPtyDeviceDrvNum == PX_ERROR) {
        return  (PX_ERROR);
    }
    
    _G_iPtyHostDrvNum = iosDrvInstall(_PtyHostOpen,
                                      (FUNCPTR)LW_NULL,
                                      _PtyHostOpen,
                                      _PtyHostClose,
                                      _PtyHostRead,
                                      _PtyHostWrite,
                                      _PtyHostIoctl);
    if (_G_iPtyHostDrvNum == PX_ERROR) {
        iosDrvRemove(_G_iPtyDeviceDrvNum, LW_TRUE);
        return  (PX_ERROR);
    }
    
    DRIVER_LICENSE(_G_iPtyDeviceDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iPtyDeviceDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iPtyDeviceDrvNum, "pty driver (device node).");
    
    DRIVER_LICENSE(_G_iPtyHostDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iPtyHostDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iPtyHostDrvNum, "pty driver (host node).");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PtyDevCreate
** ��������: ���� PTY �豸. һ�δ����Զ����������ļ�, һ�������ض�, һ�����豸��
**           ���ض��ļ���Ϊ: ???.hst
**           �豸���ļ���Ϊ: ???.dev
**           ����: ����һ�������ն��豸 /dev/pty/0 ���ض���Ϊ: /dev/pty/0.hst �豸����Ϊ: /dev/pty/0.dev
** �䡡��  : pcName,                       �豸��
**           stRdBufSize,                  ���뻺������С
**           stWrtBufSize                  �����������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PtyDevCreate (PCHAR   pcName, 
                       size_t  stRdBufSize,
                       size_t  stWrtBufSize)
{
    REGISTER INT            iTemp;
    REGISTER P_PTY_DEV      p_ptydev;
    REGISTER P_PTY_D_DEV    p_ptyddev;
    
             CHAR           cNameBuffer[MAX_FILENAME_LENGTH];
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (!pcName || (lib_strlen(pcName) > (PATH_MAX - 4))) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if ((_G_iPtyDeviceDrvNum <= 0) ||
        (_G_iPtyHostDrvNum   <= 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_ptydev = (P_PTY_DEV)__SHEAP_ALLOC(sizeof(PTY_DEV));
    if (p_ptydev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    iTemp = _TyDevInit(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev, 
                       stRdBufSize, 
                       stWrtBufSize,
                       (FUNCPTR)_PtyDeviceStartup);                     /*  ��ʼ���豸���ƿ�            */
    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(p_ptydev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    lib_bzero(&p_ptyddev->PTYDDEV_devhdrDevice, sizeof(LW_DEV_HDR));
    
    p_ptyddev->PTYDDEV_iAbortFlag = 0;
    p_ptyddev->PTYDDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;            /*  ��ʼ��Ϊ���õȴ�            */
    SEL_WAKE_UP_LIST_INIT(&p_ptyddev->PTYDDEV_selwulList);              /*  ��ʼ���豸�� sel �ṹ       */
    
    p_ptyddev->PTYDDEV_hRdSyncSemB = API_SemaphoreBCreate("ptyd_rsync",
                                     LW_FALSE, 
                                     LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                     LW_NULL);
    if (!p_ptyddev->PTYDDEV_hRdSyncSemB) {                              /*  �Ƿ�ʧ��                    */
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  ж�� sel �ṹ               */
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  ж�� ty  �ṹ               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".hst");                                    /*  ���ض�                      */
    
    iTemp = (INT)iosDevAddEx(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr, 
                             cNameBuffer, 
                             _G_iPtyHostDrvNum,
                             DT_CHR);
    if (iTemp) {
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  ж�� sel �ṹ               */
        API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  ж�� ty  �ṹ               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".dev");                                    /*  �豸��                      */
    
    iTemp = (INT)iosDevAddEx(&p_ptyddev->PTYDDEV_devhdrDevice, 
                             cNameBuffer, 
                             _G_iPtyDeviceDrvNum,
                             DT_CHR);
    if (iTemp) {
        iosDevDelete(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  ж�� sel �ṹ               */
        API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  ж�� ty  �ṹ               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    /*
     *  ��¼����ʱ�� (UTC ʱ��)
     */
    p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_timeCreate = lib_time(LW_NULL);
    p_ptydev->PTYDEV_ptyddev.PTYDDEV_timeCreate = 
        p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_timeCreate;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PtyDevRemove
** ��������: ɾ�� PTY �豸.
** �䡡��  : pcName,                       �豸��, ��ͬ�� ����ʱ���豸��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PtyDevRemove (PCHAR   pcName)
{
    REGISTER DEV_HDR     *pdevhdrHost;
    REGISTER DEV_HDR     *pdevhdrDevice;
    REGISTER P_PTY_D_DEV  p_ptyddev;
    REGISTER P_PTY_DEV    p_ptydev;
    
             CHAR         cNameBuffer[MAX_FILENAME_LENGTH];
             PCHAR        pcTail;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pcName) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if ((_G_iPtyDeviceDrvNum <= 0) ||
        (_G_iPtyHostDrvNum   <= 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".hst");                                    /*  ���ض�                      */
    
    pdevhdrHost = iosDevFind(cNameBuffer, &pcTail);
    if ((pdevhdrHost == LW_NULL) || (pcTail == cNameBuffer)) {
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".dev");                                    /*  �豸��                      */
    
    pdevhdrDevice = iosDevFind(cNameBuffer, &pcTail);
    if ((pdevhdrDevice == LW_NULL) || (pcTail == cNameBuffer)) {
        return  (PX_ERROR);
    }
    
    p_ptyddev = (P_PTY_D_DEV)pdevhdrDevice;
    
    iosDevFileAbnormal(&p_ptyddev->PTYDDEV_devhdrDevice);               /*  �����д��豸��ǰ�ļ���Ϊ����*/
    iosDevDelete(pdevhdrDevice);                                        /*  �� IO ϵͳж���豸          */
    
    SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);              /*  ж�� sel �ṹ               */
    
    p_ptydev = (P_PTY_DEV)pdevhdrHost;
                                                                        /*  �����д򿪵��ļ���Ϊ�쳣    */
    iosDevFileAbnormal(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);
    iosDevDelete(pdevhdrHost);                                          /*  �� IO ϵͳж���豸          */
    
    _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);         /*  ж�� ty  �ṹ               */
    
    API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);              /*  ɾ��ͬ���ź���              */
                                                                        /*  ��Щ�����������ļ��쳣֮��  */
    __SHEAP_FREE(p_ptydev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
