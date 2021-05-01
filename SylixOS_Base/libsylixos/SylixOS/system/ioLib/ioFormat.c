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
** ��   ��   ��: ioFormat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 19 ��
**
** ��        ��: ���̸�ʽ������.

** BUG
2008.12.02  ������ĸ�׵�����, ףĸ�����տ���! ����ز������� FIOFLUSH ����.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** ��������: diskformat
** ��������: ��ʽ��ָ�����豸.
** �䡡��  : pcDevName     �豸��, NULL ��ʾ��ʽ����ǰ�豸
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  diskformat (CPCHAR  pcDevName)
{
    REGISTER INT         iError;
    REGISTER INT         iFd;
    REGISTER CPCHAR      pcName;
    
    pcName = (pcDevName == LW_NULL) ? "." : pcDevName;
    
    if (geteuid() != 0) {                                               /*  ������� root Ȩ��          */
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    iFd = open(pcName, O_WRONLY);                                       /*  ���豸                    */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKFORMAT, LW_OSIOD_LARG(0));               /*  ��ʽ���ļ�                  */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOSYNC);                                       /*  ��ջ���                    */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKINIT, LW_OSIOD_LARG(0));                 /*  ���³�ʼ���豸              */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);                                                         /*  �ر��豸                    */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" format ok.\r\n", pcDevName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: diskinit
** ��������: ��ʼ��ָ���Ĵ����豸.
** �䡡��  : pcDevName     �豸��, NULL ��ʾ��ʼ����ǰ����
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  diskinit (CPCHAR  pcDevName)
{
    REGISTER INT         iError;
    REGISTER INT         iFd;
             CHAR        cName[MAX_FILENAME_LENGTH];
    
    if (pcDevName == LW_NULL) {
        ioDefPathGet(cName);
    } else {
        lib_strlcpy(cName, pcDevName, MAX_FILENAME_LENGTH);
    }
    
    iFd = open(cName, O_WRONLY, 0);                                     /*  ���豸                    */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOFLUSH);                                      /*  ��ջ���                    */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIODISKINIT, LW_OSIOD_LARG(0));                 /*  ���³�ʼ���豸              */
    if (iError < 0) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);                                                         /*  �ر��豸                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
