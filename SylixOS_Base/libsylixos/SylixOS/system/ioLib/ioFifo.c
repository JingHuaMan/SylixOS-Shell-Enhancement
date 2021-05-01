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
** ��   ��   ��: ioFifo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 27 ��
**
** ��        ��: ϵͳ FIFO ������.

** BUG:
2012.08.25  pipe() �� uiPipCnt++; ��Ҫ�б���.
2012.09.25  mkfifo() �� pipe() ��Ҫ�� O_EXCL ����.
2012.11.21  �Ѿ�֧�� unlink �ӳ�, ������Ҫ O_TEMP.
2013.01.17  ����� pipe2 ��֧��.
2013.05.08  ���� pipe ����� /dev/pipe Ŀ¼��.
2013.08.14  pipe2 ʹ�� atomic lock ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  pipe Ĭ��·����
*********************************************************************************************************/
#define __LW_PIPE_PATH      "/dev/pipe"
/*********************************************************************************************************
** ��������: mkfifo
** ��������: ����һ���µ� FIFO
** �䡡��  : pcFifoName    Ŀ¼��
**           mode          ��ʽ (Ŀǰδʹ��)
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  mkfifo (CPCHAR  pcFifoName, mode_t  mode)
{
    REGISTER INT    iFd;
    
    (VOID)mode;
    
    iFd = open(pcFifoName, O_RDWR | O_CREAT | O_EXCL, S_IFIFO | DEFAULT_DIR_PERM);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pipe2
** ��������: ����һ�� "����" �ܵ�
** �䡡��  : iFd           ���ص��ļ����������� 0:�� 1:д
**           iFlag         ��ʽ  0 / O_NONBLOCK / O_CLOEXEC
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  pipe2 (INT  iFd[2], INT  iFlag)
{
    static UINT     uiPipeIndex = 0;
    
    INTREG          iregInterLevel;
    CHAR            cName[64];
    INT             iFifo;
    INT             iFdR, iFdW;
    UINT            uiPipe;
    
    if (!iFd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iFlag &= (O_NONBLOCK | O_CLOEXEC);                                  /*  ֻ������������־            */
    
    do {
        __LW_ATOMIC_LOCK(iregInterLevel);
        uiPipe = uiPipeIndex;
        uiPipeIndex++;
        __LW_ATOMIC_UNLOCK(iregInterLevel);
    
        snprintf(cName, sizeof(cName), "%s/%d", __LW_PIPE_PATH, uiPipe);
        
        iFifo = open(cName, iFlag | O_RDWR | O_CREAT | O_EXCL, S_IFIFO | DEFAULT_DIR_PERM);
        if (iFifo < 0) {
            if (errno != ERROR_IO_FILE_EXIST) {                         /*  �����ظ��豸����������      */
                return  (PX_ERROR);
            }
        
        } else {
            close(iFifo);
            break;                                                      /*  ���� FIFO �ɹ�              */
        }
    } while (1);
    
    iFdR = open(cName, iFlag | O_RDWR);
    if (iFdR < 0) {
        return  (PX_ERROR);
    }

    ioctl(iFdR, FIOUNMOUNT);                                            /*  ���һ�ιر�ʱɾ���豸      */
    ioctl(iFdR, FIOPIPERDONLY);                                         /*  ��Ϊֻ��ģʽ                */
    
    iFdW = open(cName, iFlag | O_WRONLY);
    if (iFdW < 0) {
        close(iFdR);                                                    /*  �޷�����д��, �رն���      */
        return  (PX_ERROR);
    }

    iFd[0] = iFdR;
    iFd[1] = iFdW;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pipe
** ��������: ����һ�� "����" �ܵ�
** �䡡��  : iFd           ���ص��ļ����������� 0:�� 1:д
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  pipe (INT  iFd[2])
{
    return  (pipe2(iFd, 0));
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
