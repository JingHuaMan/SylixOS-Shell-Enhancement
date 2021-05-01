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
** ��   ��   ��: monitorNetUpload.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼������, ͨ�������ϴ� (��������ͨ����������Ȳ��ɿ����紫��).
*********************************************************************************************************/

#ifndef __MONITORNETUPLOAD_H
#define __MONITORNETUPLOAD_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0 && LW_CFG_NET_EN > 0

/*********************************************************************************************************
  �����ϴ��ڵ� API
*********************************************************************************************************/

LW_API PVOID    API_MonitorNetUploadCreate(CPCHAR                pcIp, 
                                           UINT16                usPort,
                                           size_t                stSize,
                                           PLW_CLASS_THREADATTR  pthreadattr);
                                           
LW_API PVOID    API_MonitorNet6UploadCreate(CPCHAR                pcIp, 
                                            UINT16                usPort,
                                            size_t                stSize,
                                            PLW_CLASS_THREADATTR  pthreadattr);
                                           
LW_API ULONG    API_MonitorNetUploadDelete(PVOID  pvMonitorUpload);

#define monitorNetUploadCreate      API_MonitorNetUploadCreate
#define monitorNet6UploadCreate     API_MonitorNet6UploadCreate
#define monitorNetUploadDelete      API_MonitorNetUploadDelete

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_NET_EN > 0           */
#endif                                                                  /*  __MONITORNETUPLOAD_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
