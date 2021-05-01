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
** ��   ��   ��: monitorUpload.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 27 ��
**
** ��        ��: SylixOS �ں��¼������, ͨ���ļ��ϴ�. �������豸�ļ�, Ҳ��������ͨ�ļ�.
*********************************************************************************************************/


#ifndef __MONITORUPLOAD_H
#define __MONITORUPLOAD_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)

/*********************************************************************************************************
  Upload ����ѡ��
*********************************************************************************************************/

#define LW_MONITOR_UPLOAD_PROTO         0x1                             /*  ����ʹ��Э������˲���      */
                                                                        /*  ��ѡ���ֹ������ͨ�ļ��ϴ�  */
                                                                        
/*********************************************************************************************************
  Upload SetFilter ��ʽ
*********************************************************************************************************/

#define LW_MONITOR_UPLOAD_SET_EVT_SET   0                               /*  ����                        */
#define LW_MONITOR_UPLOAD_ADD_EVT_SET   1                               /*  ���                        */
#define LW_MONITOR_UPLOAD_SUB_EVT_SET   2                               /*  ɾ��                        */

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID    API_MonitorUploadCreate(INT                   iFd, 
                                        size_t                stSize,
                                        ULONG                 ulOption,
                                        PLW_CLASS_THREADATTR  pthreadattr);
                                        
LW_API ULONG    API_MonitorUploadDelete(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadSetPid(PVOID  pvMonitorUpload, pid_t  pid);

LW_API ULONG    API_MonitorUploadGetPid(PVOID  pvMonitorUpload, pid_t  *pid);

LW_API ULONG    API_MonitorUploadSetFilter(PVOID    pvMonitorUpload, 
                                           UINT32   uiEventId, 
                                           UINT64   u64SubEventAllow,
                                           INT      iHow);
                                           
LW_API ULONG    API_MonitorUploadGetFilter(PVOID    pvMonitorUpload, 
                                           UINT32   uiEventId, 
                                           UINT64  *pu64SubEventAllow);
                                           
LW_API ULONG    API_MonitorUploadEnable(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadDisable(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadFlush(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadClearOverrun(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadGetOverrun(PVOID  pvMonitorUpload, ULONG  *pulOverRun);

LW_API ULONG    API_MonitorUploadFd(PVOID  pvMonitorUpload, INT  *piFd);

#define monitorUploadCreate         API_MonitorUploadCreate
#define monitorUploadDelete         API_MonitorUploadDelete
#define monitorUploadSetPid         API_MonitorUploadSetPid
#define monitorUploadGetPid         API_MonitorUploadGetPid
#define monitorUploadSetFilter      API_MonitorUploadSetFilter
#define monitorUploadGetFilter      API_MonitorUploadGetFilter
#define monitorUploadEnable         API_MonitorUploadEnable
#define monitorUploadDisable        API_MonitorUploadDisable
#define monitorUploadFlush          API_MonitorUploadFlush
#define monitorUploadClearOverrun   API_MonitorUploadClearOverrun
#define monitorUploadGetOverrun     API_MonitorUploadGetOverrun
#define monitorUploadFd             API_MonitorUploadFd

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#endif                                                                  /*  __MONITORFILEUPLOAD_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
