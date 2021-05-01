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
** ��   ��   ��: monitorTrace.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼������.
*********************************************************************************************************/

#ifndef __MONITORTRACE_H
#define __MONITORTRACE_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

/*********************************************************************************************************
  ��ؽڵ㺯������
*********************************************************************************************************/

typedef BOOL  (*MONITOR_FILTER_ROUTINE)(PVOID           pvArg, 
                                        UINT32          uiEventId, 
                                        UINT32          uiSubEvent);
                                        
typedef VOID  (*MONITOR_COLLECTOR_ROUTINE)(PVOID           pvArg, 
                                           UINT32          uiEventId, 
                                           UINT32          uiSubEvent,
                                           CPVOID          pvMsg,
                                           size_t          stSize,
                                           CPCHAR          pcAddtional);

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID  API_MonitorTraceCreate(MONITOR_FILTER_ROUTINE       pfuncFilter,
                                     MONITOR_COLLECTOR_ROUTINE    pfuncCollector,
                                     PVOID                        pvArg,
                                     BOOL                         bEnable);

LW_API ULONG  API_MonitorTraceDelete(PVOID  pvMonitor);

LW_API ULONG  API_MonitorTraceEnable(PVOID  pvMonitor);

LW_API ULONG  API_MonitorTraceDisable(PVOID  pvMonitor);

LW_API VOID   API_MonitorTraceEvent(UINT32         uiEventId, 
                                    UINT32         uiSubEvent, 
                                    CPVOID         pvMsg, 
                                    size_t         stSize,
                                    CPCHAR         pcAddtional);

#define MONITOR_TRACE_EVENT(a, b, c, d, e)  API_MonitorTraceEvent(a, b, c, d, e)

#else

#define MONITOR_TRACE_EVENT(a, b, c, d, e)

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

#endif                                                                  /*  __MONITORTRACE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
