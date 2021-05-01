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
** ��   ��   ��: ptyHost.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 15 ��
**
** ��        ��: �����ն����ض��ڲ���(�������豸�˵���һ������һ������).
                 �����ն˷�Ϊ�����˿�: �豸�˺����ض�! 
                 �豸�������������һ��Ӳ������.
                 ���ض˿��Կ��ɾ���һ�� TTY �豸.
                 
** BUG:
2009.08.27  �򿪹ر�ʱ���Ӷ��豸���õļ�������.
2009.10.22  read write ����ֵΪ ssize_t.
2012.01.11  _PtyHostIoctl() ����� SIO Ӳ����������Ͳ������������Ӧ.
2019.01.21  _PtyHostWrite() ����Ҫ���� sel list.
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
  ȫ�ֱ���(���Ʒ�ֵ)
*********************************************************************************************************/
static CHAR             _G_cPtyWrtThreshold = 20;                       /*  ����������������ֽ�������  */
                                                                        /*  ���ֵ, ����ȴ�д���߳�    */
/*********************************************************************************************************
** ��������: _PtyHostOpen
** ��������: �����ն����ض˴�
** �䡡��  : p_ptydev      �Ѿ��������豸���ƿ�
**           pcName        �򿪵��ļ���
**           iFlags        ��ʽ         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode         UNIX MODE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LONG  _PtyHostOpen (P_PTY_DEV      p_ptydev,
                    PCHAR          pcName,   
                    INT            iFlags, 
                    INT            iMode)
{
    P_PTY_D_DEV  p_ptyddev = &p_ptydev->PTYDEV_ptyddev;

    p_ptyddev->PTYDDEV_bIsClose = LW_FALSE;                             /*  û�йر�                    */

    LW_DEV_INC_USE_COUNT(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);

    return  ((LONG)(p_ptydev));                                         /*  �����ն˿��ƿ��ַ          */
}
/*********************************************************************************************************
** ��������: _PtyHostClose
** ��������: �����ն����ض˹ر�
** �䡡��  : p_ptydev       �����ն˿��ƿ��ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PtyHostClose (P_PTY_DEV      p_ptydev)
{
    P_PTY_D_DEV  p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    
    if (LW_DEV_DEC_USE_COUNT(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr) == 0) {
        p_ptyddev->PTYDDEV_bIsClose = LW_TRUE;                          /*  �ر�                        */
        
        API_SemaphoreBPost(p_ptyddev->PTYDDEV_hRdSyncSemB);             /*  ����ȴ����߳�              */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELREAD);       /*  select() ����               */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELEXCEPT);     /*  select() ����               */
    }
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _PtyHostRead
** ��������: �����ն����ض˶�ȡ����
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ���ջ�����
**           stMaxBytes     ��������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PtyHostRead (P_PTY_DEV     p_ptydev, 
                       PCHAR         pcBuffer, 
                       size_t        stMaxBytes)
{
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    REGISTER ssize_t        sstRead   = _TyRead(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                                                pcBuffer, 
                                                stMaxBytes);            /*  �ӽ��ջ�������ȡ����        */

    if (rngNBytes(p_ptyhdev->PTYHDEV_tydevTyDev.TYDEV_vxringidRdBuf)
        < _G_cPtyWrtThreshold) {                                        /*  �ɶ�����С�� 20 ��          */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELWRITE);      /*  select() ����               */
    }
    
    return  (sstRead);
}
/*********************************************************************************************************
** ��������: _PtyHostWrite
** ��������: �����ն����ض�д������
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ��Ҫд���ն˵�����
**           stNBytes       ��Ҫд������ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  _PtyHostWrite (P_PTY_DEV     p_ptydev, 
                        PCHAR         pcBuffer, 
                        size_t        stNBytes)
{
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    REGISTER ssize_t        sstWrite  = _TyWrite(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                                                 pcBuffer, 
                                                 stNBytes);             /*  ���ͻ�����д������        */
    return  (sstWrite);
}
/*********************************************************************************************************
** ��������: _PtyHostIoctl
** ��������: �����ն����ض�ִ�п�������
** �䡡��  : p_ptydev       �����ն�
**           pcBuffer       ��Ҫд���ն˵�����
**           iNBytes        ��Ҫд������ݴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PtyHostIoctl (P_PTY_DEV     p_ptydev, 
                    INT           iRequest,
                    LONG          lArg)
{
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    switch (iRequest) {
    
    case SIO_HW_OPTS_GET:
        *(INT *)lArg = (CREAD | CS8);
        return  (ERROR_NONE);
        
    case SIO_HW_OPTS_SET:
        return  (ERROR_NONE);
        
    case SIO_BAUD_GET:
        *(LONG *)lArg = SIO_BAUD_115200;
        return  (ERROR_NONE);
        
    case FIOBAUDRATE:
    case SIO_BAUD_SET:
        return  (ERROR_NONE);
    }
    
    return  (_TyIoctl(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                      iRequest, lArg));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
