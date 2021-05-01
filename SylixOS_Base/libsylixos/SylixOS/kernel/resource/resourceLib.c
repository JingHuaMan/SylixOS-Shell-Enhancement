/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: resourceLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 06 ��
**
** ��        ��: ��Դ������ (���Ĺ����������������, ��ʼ��ǰ��һЩȫ�ֶ�����Բ��ù���)
**
** ע        ��: ��Դ���յ�ԭ����: ���̺Ų�Ϊ 0, �Ҳ��� GLOBAL �Ķ�����Ҫ���л���.

** BUG:
2012.12.21  1.0.0.rc36 ���Ժ�� SylixOS ʵ���˽��̶����ļ�������, ���ﲻ��ʹ�� hook �����ļ�������.
2013.09.04  �����û�� global ���Ե� powerm �ڵ���չ���.
2014.05.20  ɾ�� __resPidCanExit() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "resourceReclaim.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  ��Դ����
  ��Ϊ: �ں˾��, I/O�ļ�, ԭʼ��Դ, ����ǰ������Դ��ϵͳ hook �Զ�����, ԭʼ��Դ��Ҫ��صĹ��������е���
*********************************************************************************************************/
typedef struct {
    LW_OBJECT_HANDLE        RESH_ulHandle;                              /*  ��Դ���                    */
    BOOL                    RESH_bIsGlobal;                             /*  �Ƿ���ȫ����Դ              */
    pid_t                   RESH_pid;                                   /*  ���̺�                      */
} LW_RESOURCE_H;
typedef LW_RESOURCE_H      *PLW_RESOURCE_H;                             /*  �ں˶�������Դ              */
/*********************************************************************************************************
  ��Դ����
*********************************************************************************************************/                                   
static LW_RESOURCE_H        _G_reshEventBuffer[LW_CFG_MAX_EVENTS];
static LW_RESOURCE_H        _G_reshEventsetBuffer[LW_CFG_MAX_EVENTSETS];
static LW_RESOURCE_H        _G_reshPartitionBuffer[LW_CFG_MAX_PARTITIONS];
static LW_RESOURCE_H        _G_reshRegionBuffer[LW_CFG_MAX_REGIONS];
static LW_RESOURCE_H        _G_reshTimerBuffer[LW_CFG_MAX_TIMERS];
static LW_RESOURCE_H        _G_reshThreadBuffer[LW_CFG_MAX_THREADS];
static LW_RESOURCE_H        _G_reshRmsBuffer[LW_CFG_MAX_RMSS];
static LW_RESOURCE_H        _G_reshThreadPoolBuffer[LW_CFG_MAX_THREAD_POOLS];
/*********************************************************************************************************
  ԭʼ��Դ����
*********************************************************************************************************/                                   
static LW_LIST_LINE_HEADER  _G_plineResRaw;
/*********************************************************************************************************
  ��Դ�������
*********************************************************************************************************/                                   
static LW_OBJECT_HANDLE     _G_ulResHLock;
static LW_OBJECT_HANDLE     _G_ulResRawLock;

#define __LW_RESH_LOCK()        API_SemaphoreMPend(_G_ulResHLock, LW_OPTION_WAIT_INFINITE)
#define __LW_RESH_UNLOCK()      API_SemaphoreMPost(_G_ulResHLock)

#define __LW_RESRAW_LOCK()      API_SemaphoreMPend(_G_ulResRawLock, LW_OPTION_WAIT_INFINITE)
#define __LW_RESRAW_UNLOCK()    API_SemaphoreMPost(_G_ulResRawLock)
/*********************************************************************************************************
** ��������: __resGetHandleBuffer
** ��������: ͨ�������ö�Ӧ��Դ��¼����λ��
** �䡡��  : ulHandle      ��Դ���
** �䡡��  : ��Դ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE PLW_RESOURCE_H  __resGetHandleBuffer (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh = LW_NULL;
    UINT16          usIndex = _ObjectGetIndex(ulHandle);
    ULONG           ulClass = _ObjectGetClass(ulHandle);
    
    switch (ulClass) {
    
    case _OBJECT_THREAD:
        if (usIndex < LW_CFG_MAX_THREADS) {
            presh = &_G_reshThreadBuffer[usIndex];
        }
        break;
        
    case _OBJECT_THREAD_POOL:
        if (usIndex < LW_CFG_MAX_THREAD_POOLS) {
            presh = &_G_reshThreadPoolBuffer[usIndex];
        }
        break;
        
    case _OBJECT_SEM_C:
    case _OBJECT_SEM_B:
    case _OBJECT_SEM_M:
    case _OBJECT_SEM_RW:
    case _OBJECT_MSGQUEUE:
        if (usIndex < LW_CFG_MAX_EVENTS) {
            presh = &_G_reshEventBuffer[usIndex];
        }
        break;
    
    case _OBJECT_EVENT_SET:
        if (usIndex < LW_CFG_MAX_EVENTSETS) {
            presh = &_G_reshEventsetBuffer[usIndex];
        }
        break;
    
    case _OBJECT_TIMER:
        if (usIndex < LW_CFG_MAX_TIMERS) {
            presh = &_G_reshTimerBuffer[usIndex];
        }
        break;
    
    case _OBJECT_PARTITION:
        if (usIndex < LW_CFG_MAX_PARTITIONS) {
            presh = &_G_reshPartitionBuffer[usIndex];
        }
        break;
    
    case _OBJECT_REGION:
        if (usIndex < LW_CFG_MAX_REGIONS) {
            presh = &_G_reshRegionBuffer[usIndex];
        }
        break;
    
    case _OBJECT_RMS:
        if (usIndex < LW_CFG_MAX_RMSS) {
            presh = &_G_reshRmsBuffer[usIndex];
        }
        break;
        
    default:
        break;
    }
    
    return  (presh);
}
/*********************************************************************************************************
** ��������: __resAddHandleHook
** ��������: ��Դ����������һ���ں���Դ
** �䡡��  : ulHandle      ��Դ���
**           ulOption      ����ѡ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __resAddHandleHook (LW_OBJECT_HANDLE  ulHandle, ULONG  ulOption)
{
    PLW_RESOURCE_H  presh;
    pid_t           pid = getpid();
    
    if (!_ObjectClassOK(ulHandle, _OBJECT_THREAD)) {                    /*  �����߳�                    */
        if ((ulOption & LW_OPTION_OBJECT_GLOBAL) || (pid == 0)) {       /*  ȫ�ֶ�����ں�������      */
            return;                                                     /*  ����¼                      */
        }
    }

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle = ulHandle;
        presh->RESH_pid      = pid;
        if (ulOption & LW_OPTION_OBJECT_GLOBAL) {
            presh->RESH_bIsGlobal = LW_TRUE;
        } else {
            presh->RESH_bIsGlobal = LW_FALSE;
        }
    }
    __LW_RESH_UNLOCK();
}
/*********************************************************************************************************
** ��������: __resDelHandleHook
** ��������: ����Դ������ɾ��һ���ں���Դ
** �䡡��  : ulHandle      ��Դ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __resDelHandleHook (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;
    
    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle  = LW_OBJECT_HANDLE_INVALID;
        presh->RESH_bIsGlobal = LW_FALSE;
        presh->RESH_pid       = 0;
    }
    __LW_RESH_UNLOCK();
}
/*********************************************************************************************************
** ��������: __resAddRawHook
** ��������: ��Դ����������һ��ԭʼ��Դ
** �䡡��  : presraw       ԭʼ��Դ����
**           pvfunc        �ͷź���
**           pvArg0~5      �ͷź�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __resAddRawHook (PLW_RESOURCE_RAW    presraw, 
                       VOIDFUNCPTR         pvfunc,
                       PVOID               pvArg0,
                       PVOID               pvArg1,
                       PVOID               pvArg2,
                       PVOID               pvArg3,
                       PVOID               pvArg4,
                       PVOID               pvArg5)
{
    if (!presraw) {
        return;
    }
    
    presraw->RESRAW_pid = getpid();
    if (presraw->RESRAW_pid == 0) {                                     /*  �������ں�������Դ          */
        presraw->RESRAW_bIsInstall = LW_FALSE;                          /*  û�а�װ�ɹ�                */
        return;
    }
    
    presraw->RESRAW_pvArg[0] = pvArg0;
    presraw->RESRAW_pvArg[1] = pvArg1;
    presraw->RESRAW_pvArg[2] = pvArg2;
    presraw->RESRAW_pvArg[3] = pvArg3;
    presraw->RESRAW_pvArg[4] = pvArg4;
    presraw->RESRAW_pvArg[5] = pvArg5;
    
    presraw->RESRAW_pfuncFree = pvfunc;
    
    __LW_RESRAW_LOCK();
    _List_Line_Add_Ahead(&presraw->RESRAW_lineManage, &_G_plineResRaw);
    presraw->RESRAW_bIsInstall = LW_TRUE;                               /*  ��װ�ɹ�                    */
    __LW_RESRAW_UNLOCK();
}
/*********************************************************************************************************
** ��������: __resDelRawHook
** ��������: ����Դ������ȥ��һ��ԭʼ��Դ
** �䡡��  : presraw       ԭʼ��Դ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __resDelRawHook (PLW_RESOURCE_RAW    presraw)
{
    if (!presraw) {
        return;
    }
    
    if (presraw->RESRAW_bIsInstall == LW_FALSE) {                       /*  �����ǰ�װ�ɹ���            */
        return;
    }
    
    __LW_RESRAW_LOCK();
    _List_Line_Del(&presraw->RESRAW_lineManage, &_G_plineResRaw);
    presraw->RESRAW_bIsInstall = LW_FALSE;
    __LW_RESRAW_UNLOCK();
}
/*********************************************************************************************************
** ��������: __resThreadDelHook
** ��������: �߳�ɾ�����ʱ�����ô˺���
** �䡡��  : pvVProc       �̱߳���Ľ�����Ϣ
**           ulId          ��ɾ�����߳� ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __resThreadDelHook (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
    if (pvVProc) {
        vprocThreadExitHook(pvVProc, ulId);
    }
}
/*********************************************************************************************************
** ��������: __resPidReclaim
** ��������: ����Դ������ɾ����Ӧ���̵�ȫ����Դ (��Ҫ�ȴ����н������߳̽���)
** �䡡��  : pid           ���̺�
** �䡡��  : �Ƿ�ɹ�, ������ɹ�֤��������̻����߳�������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __resPidReclaim (pid_t  pid)
{
    INT                 i;
    PLW_RESOURCE_H      presh;
    PLW_RESOURCE_RAW    presraw;
    PLW_LIST_LINE       plineTemp;
    
    if (pid == 0) {
        return  (ERROR_NONE);                                           /*  �ں�, ��ִ�л��ղ���        */
    }
    
    __LW_RESRAW_LOCK();
    plineTemp = _G_plineResRaw;
    while (plineTemp) {
        presraw   = (PLW_RESOURCE_RAW)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        if (presraw->RESRAW_pid == pid) {
            presraw->RESRAW_pfuncFree(presraw->RESRAW_pvArg[0],
                                      presraw->RESRAW_pvArg[1],
                                      presraw->RESRAW_pvArg[2],
                                      presraw->RESRAW_pvArg[3],
                                      presraw->RESRAW_pvArg[4],
                                      presraw->RESRAW_pvArg[5]);        /*  �ͷ�ԭʼ��Դ                */
        }
    }
    __LW_RESRAW_UNLOCK();
    
    vprocIoReclaim(pid, LW_FALSE);                                      /*  ���ս������д򿪵��ļ�      */
    
#if LW_CFG_VMM_EN > 0
    API_VmmMmapReclaim(pid);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
    for (i = 0; i < LW_CFG_MAX_THREAD_POOLS; i++) {
        presh = &_G_reshThreadPoolBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_ThreadPoolDelete(&presh->RESH_ulHandle);
        }
    }
#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
                                                                        /*  LW_CFG_MAX_THREAD_POOLS > 0 */
    for (i = 0; i < LW_CFG_MAX_EVENTS; i++) {                           /*  �����¼�                    */
        presh = &_G_reshEventBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            if (_ObjectGetClass(presh->RESH_ulHandle) == _OBJECT_MSGQUEUE) {
                API_MsgQueueDelete(&presh->RESH_ulHandle);
            } else {
                API_SemaphoreDelete(&presh->RESH_ulHandle);
            }
        }
    }
    
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    for (i = 0; i < LW_CFG_MAX_EVENTSETS; i++) {                        /*  �����¼���                  */
        presh = &_G_reshEventsetBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_EventSetDelete(&presh->RESH_ulHandle);
        }
    }
#endif
    
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
    for (i = 0; i < LW_CFG_MAX_TIMERS; i++) {                           /*  ����ʱ��                  */
        presh = &_G_reshTimerBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_TimerDelete(&presh->RESH_ulHandle);
        }
    }
#endif

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
    for (i = 0; i < LW_CFG_MAX_RMSS; i++) {                             /*  ���� RMS                    */
        presh = &_G_reshRmsBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_RmsDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
    for (i = 0; i < LW_CFG_MAX_PARTITIONS; i++) {                       /*  ���� PATITIONS              */
        presh = &_G_reshPartitionBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_PartitionDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
    for (i = 0; i < LW_CFG_MAX_REGIONS; i++) {                          /*  �����ɱ��ڴ�            */
        presh = &_G_reshRegionBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_RegionDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __resPidReclaimOnlyRaw
** ��������: ����Դ������ɾ����Ӧ���̵�ԭʼ��Դ (�ں˶�����ļ�������)
** �䡡��  : pid           ���̺�
** �䡡��  : �Ƿ�ɹ�, ������ɹ�֤��������̻����߳�������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __resPidReclaimOnlyRaw (pid_t  pid)
{
    PLW_RESOURCE_RAW    presraw;
    PLW_LIST_LINE       plineTemp;
    
    if (pid == 0) {
        return  (ERROR_NONE);                                           /*  �ں�, ��ִ�л��ղ���        */
    }
    
    __LW_RESRAW_LOCK();
    plineTemp = _G_plineResRaw;
    while (plineTemp) {
        presraw   = (PLW_RESOURCE_RAW)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        if (presraw->RESRAW_pid == pid) {
            presraw->RESRAW_pfuncFree(presraw->RESRAW_pvArg[0],
                                      presraw->RESRAW_pvArg[1],
                                      presraw->RESRAW_pvArg[2],
                                      presraw->RESRAW_pvArg[3],
                                      presraw->RESRAW_pvArg[4],
                                      presraw->RESRAW_pvArg[5]);        /*  �ͷ�ԭʼ��Դ                */
        }
    }
    __LW_RESRAW_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __resHandleIsGlobal
** ��������: �ж�һ���ں���Դ�Ƿ�Ϊȫ����Դ
** �䡡��  : ulHandle      ���
** �䡡��  : �Ƿ���ȫ����Դ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  __resHandleIsGlobal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        if ((presh->RESH_ulHandle == ulHandle) && !presh->RESH_bIsGlobal) {
            __LW_RESH_UNLOCK();
            return  (LW_FALSE);
        }
    }
    __LW_RESH_UNLOCK();
    
    return  (LW_TRUE);                                                  /*  Ĭ�϶���ȫ�ֶ���            */
}
/*********************************************************************************************************
** ��������: __resHandleMakeGlobal
** ��������: ��һ����Դ����Ϊ�ں�ȫ����Դ
** �䡡��  : ulHandle      ���
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __resHandleMakeGlobal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        if (presh->RESH_ulHandle == ulHandle) {
            presh->RESH_bIsGlobal = LW_TRUE;
            __LW_RESH_UNLOCK();
            return  (ERROR_NONE);
        }
    }
    __LW_RESH_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __resHandleMakeLocal
** ��������: ��һ����Դ����Ϊ��������Դ (��������Ѿ��ı��˽��̺�)
** �䡡��  : ulHandle      ���
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __resHandleMakeLocal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;
    pid_t           pid = getpid();

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle  = ulHandle;
        presh->RESH_pid       = pid;                                    /*  ����ȷ�� pid                */
        presh->RESH_bIsGlobal = LW_FALSE;
        __LW_RESH_UNLOCK();
        return  (ERROR_NONE);
    }
    __LW_RESH_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _resInit
** ��������: ��Դ��������ʼ�� (�˳�ʼ�������ں˳�ʼ����Ϻ�, �ں˵�һЩ������Դ���ﲻ��Ҫ��¼)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _resInit (VOID)
{
    __resReclaimInit();                                                 /*  ��Դ��������ʼ��            */

    _G_ulResHLock   = API_SemaphoreMCreate("resh_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_ulResRawLock = API_SemaphoreMCreate("resraw_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    
    API_SystemHookAdd(__resAddHandleHook, LW_OPTION_OBJECT_CREATE_HOOK);
    API_SystemHookAdd(__resDelHandleHook, LW_OPTION_OBJECT_DELETE_HOOK);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
