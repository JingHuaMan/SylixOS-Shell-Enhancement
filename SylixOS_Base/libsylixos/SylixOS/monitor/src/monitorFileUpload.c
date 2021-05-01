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
** ��   ��   ��: monitorFileUpload.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼������, ͨ���ļ��ϴ�. �������豸�ļ�, Ҳ��������ͨ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** ��������: API_MonitorFileUploadCreate
** ��������: ����һ�������ļ��ϴ���ʽ�ļ�ظ��ٽڵ�
** �䡡��  : pcFileName    �ļ���
**           iFlags        �� open ���� 2 ��ͬ
**           mode          �� open ���� 3 ��ͬ
**           stSize        ��������С
**           ulOption      Upload ѡ��, �� MonitorUpload.h
**           pthreadattr   ������񴴽�ѡ��
** �䡡��  : Upload �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorFileUploadCreate (CPCHAR                pcFileName, 
                                    INT                   iFlags, 
                                    mode_t                mode,
                                    size_t                stSize,
                                    ULONG                 ulOption,
                                    PLW_CLASS_THREADATTR  pthreadattr)
{
    INT     iFd;
    PVOID   pvMonitorUpload;
    
    if (!pcFileName) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    if (((iFlags & O_ACCMODE) != O_WRONLY) &&
        ((iFlags & O_ACCMODE) != O_RDWR)) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iFd = open(pcFileName, iFlags, mode);
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iFd, stSize, ulOption, pthreadattr);
    if (!pvMonitorUpload) {
        close(iFd);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** ��������: API_MonitorFileUploadDelete
** ��������: ɾ��һ�������ļ��ϴ���ʽ�ļ�ظ��ٽڵ�
** �䡡��  : pvMonitorUpload   �ļ��ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorFileUploadDelete (PVOID  pvMonitorUpload)
{
    INT     iFd;
    ULONG   ulError;
    
    ulError = API_MonitorUploadFd(pvMonitorUpload, &iFd);
    if (ulError) {
        return  (ulError);
    }
    
    ulError = API_MonitorUploadDelete(pvMonitorUpload);
    if (ulError) {
        return  (ulError);
    }
    
    close(iFd);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
