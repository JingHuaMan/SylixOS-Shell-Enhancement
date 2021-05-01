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
** ��   ��   ��: monitorNetUpload.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼������, ͨ�������ϴ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_NET_EN > 0)
#include "socket.h"
#include "netinet/in.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
** ��������: API_MonitorNetUploadCreate
** ��������: ����һ�����������ϴ���ʽ�ļ�ظ��ٽڵ�
** �䡡��  : pcIp          IP ��ַ
**           usPort        �˿� (�����ֽ���)
**           stSize        ��������С
**           pthreadattr   ������񴴽�ѡ��
** �䡡��  : Upload �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorNetUploadCreate (CPCHAR                pcIp, 
                                   UINT16                usPort,
                                   size_t                stSize,
                                   PLW_CLASS_THREADATTR  pthreadattr)
{
    PVOID               pvMonitorUpload;
    INT                 iSock;
    struct sockaddr_in  sockaddrinDest;
    
    if (!pcIp || !usPort) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                  /*  use tcp connect             */
    if (iSock < 0) {
        return  (LW_NULL);
    }
    
    sockaddrinDest.sin_len         = sizeof(struct sockaddr_in);
    sockaddrinDest.sin_family      = AF_INET;
    sockaddrinDest.sin_port        = htons(usPort);
    sockaddrinDest.sin_addr.s_addr = inet_addr(pcIp);
    
    if (connect(iSock, (const struct sockaddr *)&sockaddrinDest, sizeof(struct sockaddr_in)) < 0) {
        close(iSock);
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iSock, stSize, LW_MONITOR_UPLOAD_PROTO, pthreadattr);
    if (!pvMonitorUpload) {
        close(iSock);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** ��������: API_MonitorNet6UploadCreate
** ��������: ����һ�����������ϴ���ʽ�ļ�ظ��ٽڵ� (IPv6)
** �䡡��  : pcIp          IP ��ַ
**           usPort        �˿� (�����ֽ���)
**           stSize        ��������С
**           pthreadattr   ������񴴽�ѡ��
** �䡡��  : Upload �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorNet6UploadCreate (CPCHAR                pcIp, 
                                    UINT16                usPort,
                                    size_t                stSize,
                                    PLW_CLASS_THREADATTR  pthreadattr)
{
    PVOID               pvMonitorUpload;
    INT                 iSock;
    struct sockaddr_in6 sockaddrin6Dest;
    
    if (!pcIp || !usPort) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);                 /*  use tcp connect             */
    if (iSock < 0) {
        return  (LW_NULL);
    }
    
    sockaddrin6Dest.sin6_len      = sizeof(struct sockaddr_in6);
    sockaddrin6Dest.sin6_family   = AF_INET6;
    sockaddrin6Dest.sin6_port     = htons(usPort);
    sockaddrin6Dest.sin6_flowinfo = 0;
    sockaddrin6Dest.sin6_scope_id = 0;
    inet6_aton(pcIp, &sockaddrin6Dest.sin6_addr);
    
    if (connect(iSock, (const struct sockaddr *)&sockaddrin6Dest, sizeof(struct sockaddr_in6)) < 0) {
        close(iSock);
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iSock, stSize, LW_MONITOR_UPLOAD_PROTO, pthreadattr);
    if (!pvMonitorUpload) {
        close(iSock);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** ��������: API_MonitorNetUploadDelete
** ��������: ɾ��һ�����������ϴ���ʽ�ļ�ظ��ٽڵ�
** �䡡��  : pvMonitorUpload   �ļ��ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorNetUploadDelete (PVOID  pvMonitorUpload)
{
    INT     iSock;
    ULONG   ulError;
    
    ulError = API_MonitorUploadFd(pvMonitorUpload, &iSock);
    if (ulError) {
        return  (ulError);
    }
    
    ulError = API_MonitorUploadDelete(pvMonitorUpload);
    if (ulError) {
        return  (ulError);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
