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
** ��   ��   ��: monitorUpload.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 27 ��
**
** ��        ��: SylixOS �ں��¼������, ͨ���ļ��ϴ�. �������豸�ļ�, Ҳ��������ͨ�ļ�.

** BUG:
2013.09.14  �������ü�ص�Ŀ�����.
2014.10.21  ��Զ�̽ڵ������ʹ�� XML ��ʽ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)
#include "monitorBuffer.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �ϴ��������
*********************************************************************************************************/
#define MONITOR_STK_MIN_SZ      (LW_CFG_KB_SIZE * 8)
/*********************************************************************************************************
  �ϴ�����
*********************************************************************************************************/
#define MONITOR_EVENT_VERSION   "0.0.3"
#define MONITOR_EVENT_TIMEOUT   (LW_TICK_HZ * 2)
/*********************************************************************************************************
  �ϴ��ڵ�
*********************************************************************************************************/
typedef struct {
    INT                         UPLOAD_iFd;                             /*  �ļ�������                  */
    UINT64                      UPLOAD_u64SubEventAllow[MONITOR_EVENT_ID_MAX + 1];
                                                                        /*  �¼��˲���                  */
    PVOID                       UPLOAD_pvMonitorBuffer;                 /*  �¼�������                  */
    PVOID                       UPLOAD_pvMonitorTrace;                  /*  �¼����ٽڵ�                */
    LW_OBJECT_HANDLE            UPLOAD_hMonitorThread;                  /*  �ϴ�����                    */
    
    ULONG                       UPLOAD_ulOption;                        /*  ѡ��                        */
    BOOL                        UPLOAD_bNeedDelete;                     /*  ��Ҫɾ��                    */
    
    pid_t                       UPLOAD_pid;                             /*  ��ؽ���ѡ��                */
    ULONG                       UPLOAD_ulOverrun;                       /*  ��Ϣ�������                */
    
    LW_SPINLOCK_DEFINE         (UPLOAD_slLock);                         /*  spinlock                    */
} MONITOR_UPLOAD;
typedef MONITOR_UPLOAD         *PMONITOR_UPLOAD;
/*********************************************************************************************************
** ��������: __monitorUploadFilter
** ��������: ��ظ��ٽڵ��˲���
** �䡡��  : pvMonitorUpload    �ļ��ϴ���ظ��ٽڵ�
**           uiEventId          �¼���
**           uiSubEvent         ���¼���
** �䡡��  : �Ƿ��ռ��¼�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __monitorUploadFilter (PVOID  pvMonitorUpload, UINT32  uiEventId, UINT32  uiSubEvent)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    PLW_CLASS_TCB    ptcbCur;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC     *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (pmu) {
        if (pmu->UPLOAD_u64SubEventAllow[uiEventId] & ((UINT64)1 << uiSubEvent)) {
            if (uiEventId == MONITOR_EVENT_ID_SCHED) {                  /*  �����¼�������              */
                return  (LW_TRUE);
            }
            
            if (LW_CPU_GET_CUR_NESTING()) {                             /*  �ж�״̬                    */
                ptcbCur = LW_NULL;

#if LW_CFG_MODULELOADER_EN > 0
                pvproc  = LW_NULL;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            } else {                                                    /*  ������״̬                  */
                LW_TCB_GET_CUR_SAFE(ptcbCur);

#if LW_CFG_MODULELOADER_EN > 0
                pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_NO_MONITOR) {
                    if (uiEventId != MONITOR_EVENT_ID_SCHED) {
                        return  (LW_FALSE);
                    }
                }
            }
            
#if LW_CFG_MODULELOADER_EN > 0
            if (pmu->UPLOAD_pid == 0) {                                 /*  kernel                      */
                if (pvproc) {
                    return  (LW_FALSE);
                }
            
            } else if (pmu->UPLOAD_pid > 0) {                           /*  process                     */
                if ((pvproc == LW_NULL) ||
                    (pvproc->VP_pid != pmu->UPLOAD_pid)) {
                    return  (LW_FALSE);
                }
            
            } else if (pmu->UPLOAD_pid == -2) {                         /*  all process not kernel      */
                if (pvproc == LW_NULL) {
                    return  (LW_FALSE);
                }
            }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            return  (LW_TRUE);
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __monitorUploadCollector
** ��������: ��ظ��ٽڵ��ռ���
** �䡡��  : pvMonitorUpload    �ļ��ϴ���ظ��ٽڵ�
**           uiEventId          �¼���
**           uiSubEvent         ���¼���
**           pvMsg              �¼���Ϣ
**           stSize             ��Ϣ����
**           pcAddtional        �����ִ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __monitorUploadCollector (PVOID           pvMonitorUpload, 
                                       UINT32          uiEventId, 
                                       UINT32          uiSubEvent,
                                       CPVOID          pvMsg,
                                       size_t          stSize,
                                       CPCHAR          pcAddtional)
{
    INTREG           iregInterLevel;
    
    UINT16           usMsgLen;
    size_t           stOffset = 0;
    size_t           stAddStrLen;
    INT64            i64KernelTime;
    
    UCHAR            ucBuffer[MONITOR_EVENT_MAX_SIZE];
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (pcAddtional) {
        stAddStrLen = lib_strlen(pcAddtional);
    } else {
        stAddStrLen = 0;
    }
    
    usMsgLen = (UINT16)(2 + 4 + 4 + 8 + sizeof(LW_OBJECT_HANDLE) 
             + stSize + stAddStrLen);                                   /*  �¼���Ϣ�ܳ���              */
    if (usMsgLen > MONITOR_EVENT_MAX_SIZE) {
        return;
    }
    
    ucBuffer[0] = (UCHAR)(usMsgLen >> 8);                               /*  ǰ���ֽ�Ϊ��˳���          */
    ucBuffer[1] = (UCHAR)(usMsgLen & 0xFF);
    stOffset    = 2;
    
    lib_memcpy(&ucBuffer[stOffset], &uiEventId,  4);                    /*  �¼� ID                     */
    stOffset += 4;
    
    lib_memcpy(&ucBuffer[stOffset], &uiSubEvent, 4);                    /*  ���¼� ID                   */
    stOffset += 4;
    
    __KERNEL_TIME_GET(i64KernelTime, INT64);
    lib_memcpy(&ucBuffer[stOffset], &i64KernelTime, 8);                 /*  ϵͳ TICK                   */
    stOffset += 8;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        LW_OBJECT_HANDLE   hInval = LW_OBJECT_HANDLE_INVALID;
        lib_memcpy(&ucBuffer[stOffset], &hInval, sizeof(LW_OBJECT_HANDLE));
    
    } else {
        PLW_CLASS_TCB    ptcbCur;
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        lib_memcpy(&ucBuffer[stOffset], &ptcbCur->TCB_ulId, sizeof(LW_OBJECT_HANDLE));
    }
    stOffset += sizeof(LW_OBJECT_HANDLE);
    
    if (stSize) {                                                       /*  �¼���Ϣ                    */
        lib_memcpy(&ucBuffer[stOffset], pvMsg, stSize);
        stOffset += stSize;
    }
    
    if (stAddStrLen) {
        lib_memcpy(&ucBuffer[stOffset], pcAddtional, stAddStrLen);      /*  �¼������ַ���              */
    }
    
    if (__monitorBufferPut(pmu->UPLOAD_pvMonitorBuffer, 
                           ucBuffer) == 0) {                            /*  �����¼�������              */
        LW_SPIN_LOCK_QUICK(&pmu->UPLOAD_slLock, &iregInterLevel);
        pmu->UPLOAD_ulOverrun++;
        LW_SPIN_UNLOCK_QUICK(&pmu->UPLOAD_slLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** ��������: __monitorUploadTryProcProto
** ��������: ��ظ��ٽڵ㳢�Դ���Э��
** �䡡��  : pmu    �ļ��ϴ���ظ��ٽڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __monitorUploadTryProcProto (PMONITOR_UPLOAD  pmu)
{
    INT     iNRead;
    UCHAR   ucBuffer[12];
    
    while (ioctl(pmu->UPLOAD_iFd, FIONREAD, &iNRead) == 0) {
        if (iNRead >= 12) {
            if (read(pmu->UPLOAD_iFd, ucBuffer, 12) == 12) {
                UINT32  uiEvent;
                UINT64  u64Allow;
                
                lib_memcpy(&uiEvent,  &ucBuffer[0], 4);
                lib_memcpy(&u64Allow, &ucBuffer[4], 8);
                
                if (uiEvent <= MONITOR_EVENT_ID_MAX) {                  /*  �����˲���                  */
                    API_MonitorUploadSetFilter((PVOID)pmu, uiEvent, u64Allow, 
                                               LW_MONITOR_UPLOAD_SET_EVT_SET);
                
                } else if (uiEvent == MONITOR_EVENT_ID_MAX) {           /*  ����ȫ�ֿ���                */
                    if (u64Allow) {
                        API_MonitorUploadEnable((PVOID)pmu);
                    } else {
                        API_MonitorUploadDisable((PVOID)pmu);
                    }
                
                } else if (uiEvent == (MONITOR_EVENT_ID_MAX + 1)) {     /*  ���ý�����Ϣ                */
                    pid_t   pid = (pid_t)u64Allow;
                    API_MonitorUploadSetPid((PVOID)pmu, pid);
                }
            }
        } else {
            break;                                                      /*  ����������û�д����������  */
        }
    }
}
/*********************************************************************************************************
** ��������: __monitorUploadHello
** ��������: ���� upload hello ���ݰ�
** �䡡��  : iFd       ˫��
** �䡡��  : �Ƿ��ͳɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __monitorUploadHello (INT  iFd)
{
    static const CHAR       cMonitor[] = \
    "<?xml version=\"1.0\">"
    "<!-- Copyright (C) 2007-2014 SylixOS Group. -->"
    "<cpu name=\"%s\" wordlen=\"%d\" endian=\"%d\"/>"
    "<os name=\"sylixos\" version=\"%s\" tick=\"%dhz\"/>"
    "<tracer name=\"monitor\" version=\"%s\"/>\n";
                 
    CHAR            cStart[16];
    ssize_t         sstLen;
    struct timeval  tvTo = {5, 0};
                 
    fdprintf(iFd, cMonitor, bspInfoCpu(), LW_CFG_CPU_WORD_LENGHT, LW_CFG_CPU_ENDIAN,
                            __SYLIXOS_VERSTR, LW_TICK_HZ,
                            MONITOR_EVENT_VERSION);
                            
    if (waitread(iFd, &tvTo) < 1) {
        fprintf(stderr, "remote no response.\n");
        return  (PX_ERROR);
    }
    
    sstLen = read(iFd, cStart, 16);
    if ((sstLen < 5) || lib_strncmp(cStart, "start", 5)) {
        fprintf(stderr, "remote response error.\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __monitorUploadThread
** ��������: ��ظ��ٽڵ��ϴ��߳�
** �䡡��  : pvArg     ��ظ��ٽڵ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __monitorUploadThread (PVOID  pvArg)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvArg;
    UCHAR            ucEvent[MONITOR_EVENT_MAX_SIZE];
    ssize_t          sstLen;
    BOOL             bMustStop = LW_FALSE;
    
    INT64            i64ProcProtoTime;
    INT64            i64Now;
    
    __KERNEL_TIME_GET(i64ProcProtoTime, INT64);
    
    if (__monitorUploadHello(pmu->UPLOAD_iFd) < ERROR_NONE) {           /*  ���� hello ����ͷ           */
        bMustStop = LW_TRUE;
    }
    
    for (;;) {
        if (bMustStop == LW_FALSE) {
            sstLen = __monitorBufferGet(pmu->UPLOAD_pvMonitorBuffer, 
                                        ucEvent, sizeof(ucEvent), 
                                        MONITOR_EVENT_TIMEOUT);
            if (sstLen < 0) {                                           /*  �ڵ�ɾ��                    */
                fprintf(stderr, "monitor event buffer crash.\n"
                                "you must stop monitor manually.\n");
                bMustStop = LW_TRUE;
                
            } else if (sstLen > 0) {                                    /*  ���յ��¼���Ϣ              */
                sstLen = write(pmu->UPLOAD_iFd, ucEvent, (size_t)sstLen);
                if (sstLen <= 0) {
                    fprintf(stderr, "monitor can not update event to server, error: %s\n"
                                    "you must stop monitor manually.\n",
                                    lib_strerror(errno));
                    bMustStop = LW_TRUE;
                }
            }
            
            if (pmu->UPLOAD_ulOption & LW_MONITOR_UPLOAD_PROTO) {
                __KERNEL_TIME_GET(i64Now, INT64);
                if ((i64Now - i64ProcProtoTime) > MONITOR_EVENT_TIMEOUT) {
                    __monitorUploadTryProcProto(pmu);                   /*  ����Э��                    */
                    __KERNEL_TIME_GET(i64ProcProtoTime, INT64);
                }
            }
        } else {
            API_TimeSleep(MONITOR_EVENT_TIMEOUT);
        }
        
        if (pmu->UPLOAD_bNeedDelete) {                                  /*  ��Ҫɾ��                    */
            API_MonitorTraceDelete(pmu->UPLOAD_pvMonitorTrace);
            __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
            __KHEAP_FREE(pmu);
            break;
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadCreate
** ��������: ����һ����ظ����ϴ��ڵ�
** �䡡��  : iFd           �ļ�������
**           stSize        ��������С
**           ulOption      ����ѡ��
**           pthreadattr   ������񴴽�ѡ��
** �䡡��  : Upload �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorUploadCreate (INT                   iFd, 
                                size_t                stSize,
                                ULONG                 ulOption,
                                PLW_CLASS_THREADATTR  pthreadattr)
{
    INT                 i;
    PMONITOR_UPLOAD     pmu;
    LW_CLASS_THREADATTR threadattr;
    
    if (iFd < 0) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmu = (PMONITOR_UPLOAD)__KHEAP_ALLOC(sizeof(MONITOR_UPLOAD));
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    pmu->UPLOAD_ulOption    = ulOption;
    pmu->UPLOAD_bNeedDelete = LW_FALSE;
    pmu->UPLOAD_iFd         = iFd;
    pmu->UPLOAD_pid         = -1;                                       /*  kernel and process event    */
    pmu->UPLOAD_ulOverrun   = 0ul;
    
    LW_SPIN_INIT(&pmu->UPLOAD_slLock);
    
    for (i = 0; i <= MONITOR_EVENT_ID_MAX; i++) {                       /*  Ĭ�Ϲ��������¼�            */
        pmu->UPLOAD_u64SubEventAllow[i] = 0ull;
    }
    
    pmu->UPLOAD_pvMonitorBuffer = __monitorBufferCreate(stSize);
    if (!pmu->UPLOAD_pvMonitorBuffer) {
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    pmu->UPLOAD_pvMonitorTrace = API_MonitorTraceCreate(__monitorUploadFilter,
                                                        __monitorUploadCollector,
                                                        (PVOID)pmu,
                                                        LW_FALSE);
    if (!pmu->UPLOAD_pvMonitorTrace) {
        __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    if (pthreadattr) {
        threadattr = *pthreadattr;
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_pvArg     = (PVOID)pmu;
    threadattr.THREADATTR_ulOption |= LW_OPTION_OBJECT_GLOBAL
                                    | LW_OPTION_THREAD_NO_MONITOR;
    
    if (threadattr.THREADATTR_stStackByteSize < MONITOR_STK_MIN_SZ) {
        threadattr.THREADATTR_stStackByteSize = MONITOR_STK_MIN_SZ;
    }
    
    pmu->UPLOAD_hMonitorThread = API_ThreadCreate("t_monitor", __monitorUploadThread,
                                                  &threadattr, LW_NULL);
    if (pmu->UPLOAD_hMonitorThread == LW_OBJECT_HANDLE_INVALID) {
        API_MonitorTraceDelete(pmu->UPLOAD_pvMonitorTrace);
        __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    return  ((PVOID)pmu);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadDelete
** ��������: ɾ��һ����ظ����ϴ��ڵ�
** �䡡��  : pvMonitorUpload   ��ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadDelete (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    pmu->UPLOAD_bNeedDelete = LW_TRUE;
    KN_SMP_WMB();
    
    API_ThreadJoin(pmu->UPLOAD_hMonitorThread, LW_NULL);                /*  �ȴ��������                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadSetPid
** ��������: �����ϴ���ظ��ٽڵ��˲���������Ϣ
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           pid               == 0 (kernel) > 0 (process) == -1 (all) == -2 (all process no kernel)
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����˲��������ռ������¼�, �� pid �Ե����¼�������������.
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadSetPid (PVOID  pvMonitorUpload, pid_t  pid)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    pmu->UPLOAD_pid = pid;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadGetPid
** ��������: ��ȡ�ϴ���ظ��ٽڵ��˲���������Ϣ
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           pid               ��ǰ��ص����
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetPid (PVOID  pvMonitorUpload, pid_t  *pid)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pid) {
        *pid = pmu->UPLOAD_pid;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadSetFilter
** ��������: �����ϴ���ظ��ٽڵ��˲���
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           uiEventId         �¼� ID
**           u64SubEventAllow  �������¼���
**           iHow              ���÷���
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadSetFilter (PVOID    pvMonitorUpload, 
                                   UINT32   uiEventId, 
                                   UINT64   u64SubEventAllow,
                                   INT      iHow)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu || uiEventId > MONITOR_EVENT_ID_MAX) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    switch (iHow) {
    
    case LW_MONITOR_UPLOAD_SET_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] = u64SubEventAllow;
        break;
        
    case LW_MONITOR_UPLOAD_ADD_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] |= u64SubEventAllow;
        break;
        
    case LW_MONITOR_UPLOAD_SUB_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] &= ~u64SubEventAllow;
        break;
    
    default:
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadSetFilter
** ��������: ��ȡ�ϴ���ظ��ٽڵ��˲���
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           uiEventId         �¼� ID
**           pu64SubEventAllow �������¼���
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetFilter (PVOID    pvMonitorUpload, 
                                   UINT32   uiEventId, 
                                   UINT64  *pu64SubEventAllow)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu || uiEventId > MONITOR_EVENT_ID_MAX) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pu64SubEventAllow) {
        *pu64SubEventAllow = pmu->UPLOAD_u64SubEventAllow[uiEventId];
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadEnable
** ��������: ʹ���ϴ���ظ��ٽڵ�
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadEnable (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    API_MonitorTraceEnable(pmu->UPLOAD_pvMonitorTrace);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadDisable
** ��������: �����ϴ���ظ��ٽڵ�
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadDisable (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    API_MonitorTraceDisable(pmu->UPLOAD_pvMonitorTrace);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadFlush
** ��������: ����ϴ���ظ��ٽڵ������л������Ϣ
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadFlush (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    __monitorBufferFlush(pmu->UPLOAD_pvMonitorBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadClearOverrun
** ��������: ����ϴ���ظ��ٽڵ㶪ʧ��Ϣ�ĸ���������
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadClearOverrun (PVOID  pvMonitorUpload)
{
    INTREG           iregInterLevel;
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&pmu->UPLOAD_slLock, &iregInterLevel);
    pmu->UPLOAD_ulOverrun = 0;
    LW_SPIN_UNLOCK_QUICK(&pmu->UPLOAD_slLock, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadGetOverrun
** ��������: ����ϴ���ظ��ٽڵ㶪ʧ��Ϣ�ĸ���
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           pulOverRun        ��ʧ����
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetOverrun (PVOID  pvMonitorUpload, ULONG  *pulOverRun)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pulOverRun) {
        *pulOverRun = pmu->UPLOAD_ulOverrun;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MonitorUploadFd
** ��������: ��ظ��ٽڵ��ļ�������
** �䡡��  : pvMonitorUpload   �ϴ���ظ��ٽڵ�
**           piFd              �ļ�������
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadFd (PVOID  pvMonitorUpload, INT  *piFd)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (piFd) {
        *piFd = pmu->UPLOAD_iFd;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
