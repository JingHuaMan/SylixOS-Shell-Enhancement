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
** ��   ��   ��: ptyDevice.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 13 ��
**
** ��        ��: �����ն��豸��(����Ӳ��))�ڲ���.
                 �����ն˷�Ϊ�����˿�: �豸�˺����ض�! 
                 �豸�������������һ��Ӳ������.
                 ���ض˿��Կ��ɾ���һ�� TTY �豸.

** BUG:
2009.05.27  ���� abort ����.
2009.08.27  �򿪹ر�ʱ���Ӷ��豸���õļ�������.
2009.10.22  read write ����ֵΪ ssize_t.
2010.01.14  ���� abort.
2012.03.26  ������ȷ����Ŀ.
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
  ��������
*********************************************************************************************************/
VOID    __selPtyAdd(P_PTY_D_DEV   p_ptyddev, LONG  lArg);
VOID    __selPtyDelete(P_PTY_D_DEV   p_ptyddev, LONG  lArg);
/*********************************************************************************************************
** ��������: _PtyDeviceOpen
** ��������: �����ն��豸�˴�
** �䡡��  : p_ptydev      �Ѿ��������豸���ƿ�
**           pcName        �򿪵��ļ���
**           iFlags        ��ʽ         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode         UNIX MODE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LONG  _PtyDeviceOpen (P_PTY_D_DEV    p_ptyddev,
                      PCHAR          pcName,   
                      INT            iFlags, 
                      INT            iMode)
{
    REGISTER P_PTY_DEV     p_ptydev = _LIST_ENTRY(p_ptyddev, PTY_DEV, PTYDEV_ptyddev);

    p_ptyddev->PTYDDEV_bIsClose = LW_FALSE;                             /*  û�йر�                    */
    
    LW_DEV_INC_USE_COUNT(&p_ptyddev->PTYDDEV_devhdrDevice);
    
    return  ((LONG)(p_ptydev));                                         /*  �����豸�˿��Ƶ�ַ          */
}
/*********************************************************************************************************
** ��������: _PtyDeviceClose
** ��������: �����ն��豸�˹ر�
** �䡡��  : p_ptydev       �����ն�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PtyDeviceClose (P_PTY_DEV     p_ptydev)
{
    if (LW_DEV_DEC_USE_COUNT(&p_ptydev->PTYDEV_ptyddev.PTYDDEV_devhdrDevice) == 0) {
        return  (_TyIoctl(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev, 
                          FIOCANCEL, 0));                               /*  ֹͣ���ض�                  */
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: _PtyDeviceRead
** ��������: �����ն��豸�˴����ض˵ķ��ͻ�������ȡ����
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ���ջ�����
**           stMaxBytes     ��������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PtyDeviceRead (P_PTY_DEV     p_ptydev, 
                         PCHAR         pcBuffer, 
                         size_t        stMaxBytes)
{
             CHAR           cGet;
    REGISTER INT            iTemp = 0;
    REGISTER INT            iError;
    REGISTER ULONG          ulError;
    
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;

    if (p_ptyddev->PTYDDEV_bIsClose || !stMaxBytes) {                   /*  �Ƿ񱻹ر���                */
        return  (0);
    }
    
    do {
        p_ptyddev->PTYDDEV_iAbortFlag &= ~OPT_RABORT;                   /*  ��� abort                  */
    
        while (iTemp < stMaxBytes) {
            iError = _TyITx(&p_ptyhdev->PTYHDEV_tydevTyDev, &cGet);     /*  ģ�⴮�ڵ� TxD �ж�         */
            if (iError == PX_ERROR) {
                break;
            
            } else {
                pcBuffer[iTemp++] = cGet;
            }
        }
        
        if (p_ptyddev->PTYDDEV_bIsClose) {                              /*  �Ƿ񱻹ر���                */
            break;
        }
        
        if (iTemp == 0) {
            ulError = API_SemaphoreBPend(p_ptyddev->PTYDDEV_hRdSyncSemB, 
                                         p_ptyddev->PTYDDEV_ulRTimeout);
            if (ulError) {                                              /*  ������ʱ����������          */
                return  ((ssize_t)iTemp);
            }
            if (p_ptyddev->PTYDDEV_iAbortFlag & OPT_RABORT) {
                if (iTemp <= 0) {
                    _ErrorHandle(ERROR_IO_ABORT);
                }
                return  ((ssize_t)iTemp);
            }
        }
    } while (iTemp == 0);
     
    return  ((ssize_t)iTemp);
}
/*********************************************************************************************************
** ��������: _PtyDeviceWrite
** ��������: �����ն��豸�������ض˵Ľ��ջ�����д������
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ��Ҫд���ն˵�����
**           stNBytes       ��Ҫд������ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PtyDeviceWrite (P_PTY_DEV     p_ptydev, 
                          PCHAR         pcBuffer, 
                          size_t        stNBytes)
{
    REGISTER INT            i;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    for (i = 0; i < stNBytes; i++) {
        if (_TyIRd(&p_ptyhdev->PTYHDEV_tydevTyDev, pcBuffer[i]) < ERROR_NONE) {
            break;
        }
    }
    
    return  ((ssize_t)i);
}
/*********************************************************************************************************
** ��������: _PtyDeviceIoctl
** ��������: �����ն��豸��ִ�п�������
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ��Ҫд���ն˵�����
**           iNBytes        ��Ҫд������ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PtyDeviceIoctl (P_PTY_DEV     p_ptydev, 
                      INT           iRequest,
                      LONG          lArg)
{
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
             struct stat   *pstatGet;

    switch (iRequest) {
    
    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                p_ptyddev->PTYDDEV_ulRTimeout = ulTick;
            } else {
                p_ptyddev->PTYDDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOFSTATGET:                                                   /*  ��ȡ�ļ�����                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&p_ptydev->PTYDEV_ptyddev.PTYDDEV_devhdrDevice);
            pstatGet->st_ino     = (ino_t)0;                            /*  �൱��Ψһ�ڵ�              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = p_ptyddev->PTYDDEV_timeCreate;
            pstatGet->st_mtime   = p_ptyddev->PTYDDEV_timeCreate;
            pstatGet->st_ctime   = p_ptyddev->PTYDDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:
        __selPtyAdd(p_ptyddev, lArg);
        break;
        
    case FIOUNSELECT:
        __selPtyDelete(p_ptyddev, lArg);
        break;
        
    case FIOWAITABORT:                                                  /*  ֹͣ��ǰ�ȴ� IO �߳�        */
        if ((INT)lArg & OPT_RABORT) {                                   /*  ��֧�� read abort           */
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(p_ptyddev->PTYDDEV_hRdSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                p_ptyddev->PTYDDEV_iAbortFlag |= OPT_RABORT;
                API_SemaphoreBPost(p_ptyddev->PTYDDEV_hRdSyncSemB);     /*  ����д�ȴ��߳�              */
            }
        }
        break;
        
    default:
        return  (_TyIoctl(&p_ptyhdev->PTYHDEV_tydevTyDev,
                          iRequest, lArg));                             /*  ת�������豸                */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _PtyDeviceStartup
** ��������: �����ն��豸���������� (ģ���豸��׼���ý�������)
** �䡡��  : p_ptydev       �����ն�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _PtyDeviceStartup (P_PTY_DEV     p_ptydev)
{
    API_SemaphoreBPost(p_ptydev->PTYDEV_ptyddev.PTYDDEV_hRdSyncSemB);   /*  ���� read() ����            */
    
    /*
     *  ��������� ty �Ļ��Բ���, ������ȫ�� host �˵� write ����(���� wake up ����)
     *  ������Ҫһ������� wake up ����, �����Ϳ���ȷ����Ҫ��������ʱ, select() �Ķ�
     *  �ȴ����ܱ���ȷ�ļ���.
     */
    SEL_WAKE_UP_ALL(&p_ptydev->PTYDEV_ptyddev.PTYDDEV_selwulList, 
                    SELREAD);                                           /*  select() ����               */
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
