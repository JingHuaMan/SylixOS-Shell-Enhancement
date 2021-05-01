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
** ��   ��   ��: monitorTrace.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0
/*********************************************************************************************************
  ��ص�ṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                MONITOR_lineManage;
    BOOL                        MONITOR_bEnable;
    MONITOR_FILTER_ROUTINE      MONITOR_pfuncFilter;
    MONITOR_COLLECTOR_ROUTINE   MONITOR_pfuncCollector;
    PVOID                       MONITOR_pvArg;
} MONITOR_TRACE;
typedef MONITOR_TRACE          *PMONITOR_TRACE;
/*********************************************************************************************************
  ��ص�ȫ�ֱ���
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE      (_G_slMonitor);
static LW_LIST_LINE_HEADER      _G_plineMonitor;
static LW_LIST_LINE_HEADER      _G_plineMonitorWalk;
/*********************************************************************************************************
** ��������: API_MonitorTraceCreate
** ��������: ����һ����ظ��ٽڵ�
** �䡡��  : pfuncFilter       �¼��˲���
**           pfuncCollector    �¼��ռ���
**           pvArg             �ص��ײ���
**           bEnable           �Ƿ�ʹ��
** �䡡��  : ��ظ��ٽڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorTraceCreate (MONITOR_FILTER_ROUTINE       pfuncFilter,
                               MONITOR_COLLECTOR_ROUTINE    pfuncCollector,
                               PVOID                        pvArg,
                               BOOL                         bEnable)
{
    static BOOL     bIsInit = LW_FALSE;
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt;
    
    if (bIsInit == LW_FALSE) {
        bIsInit =  LW_TRUE;
        LW_SPIN_INIT(&_G_slMonitor);
    }
    
    if (!pfuncCollector) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmt = (PMONITOR_TRACE)__KHEAP_ALLOC(sizeof(MONITOR_TRACE));
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    _LIST_LINE_INIT_IN_CODE(pmt->MONITOR_lineManage);
    pmt->MONITOR_bEnable        = bEnable;
    pmt->MONITOR_pfuncFilter    = pfuncFilter;
    pmt->MONITOR_pfuncCollector = pfuncCollector;
    pmt->MONITOR_pvArg          = pvArg;
    
    if (bEnable) {
        LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
        _List_Line_Add_Ahead(&pmt->MONITOR_lineManage, &_G_plineMonitor);
        LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    }
    
    return  ((PVOID)pmt);
}
/*********************************************************************************************************
** ��������: API_MonitorTraceDelete
** ��������: ɾ��һ����ظ��ٽڵ�
** �䡡��  : pvMonitor     ��ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceDelete (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (pmt->MONITOR_bEnable) {
        _List_Line_Del(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    __KHEAP_FREE(pmt);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorTraceEnable
** ��������: ʹ��һ����ظ��ٽڵ�
** �䡡��  : pvMonitor     ��ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceEnable (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (!pmt->MONITOR_bEnable) {
        pmt->MONITOR_bEnable = LW_TRUE;
        _List_Line_Add_Ahead(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorTraceDisable
** ��������: ����һ����ظ��ٽڵ�
** �䡡��  : pvMonitor     ��ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceDisable (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (pmt->MONITOR_bEnable) {
        pmt->MONITOR_bEnable = LW_FALSE;
        if (_G_plineMonitorWalk == &pmt->MONITOR_lineManage) {
            _G_plineMonitorWalk =  _list_line_get_next(_G_plineMonitorWalk);
        }
        _List_Line_Del(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __monitorTraceEvent
** ��������: ����һ������¼�
** �䡡��  : uiEventId         �¼� ID
**           uiSubEvent        ���¼�
**           pvMsg             ��Ӧ�¼���Ϣ
**           stSize            �¼���Ϣ����
**           pcAddtional       ׷���ִ���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_MonitorTraceEvent (UINT32         uiEventId, 
                             UINT32         uiSubEvent, 
                             CPVOID         pvMsg, 
                             size_t         stSize,
                             CPCHAR         pcAddtional)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt;
    
    if (!_G_plineMonitor) {                                             /*  �����ڼ�ؽڵ�              */
        return;
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    _G_plineMonitorWalk = _G_plineMonitor;
    while (_G_plineMonitorWalk) {
        pmt = _LIST_ENTRY(_G_plineMonitorWalk, MONITOR_TRACE, MONITOR_lineManage);
        _G_plineMonitorWalk = _list_line_get_next(_G_plineMonitorWalk);
        
        LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
        if (pmt->MONITOR_pfuncFilter == LW_NULL ||
            pmt->MONITOR_pfuncFilter(pmt->MONITOR_pvArg, uiEventId, uiSubEvent)) {
            pmt->MONITOR_pfuncCollector(pmt->MONITOR_pvArg, uiEventId, uiSubEvent, 
                                        pvMsg, stSize, pcAddtional);
        }
        LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
