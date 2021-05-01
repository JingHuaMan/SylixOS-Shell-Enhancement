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
** ��   ��   ��: hotplugLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 09 ��
**
** ��        ��: �Ȳ��֧��.

** BUG:
2010.09.18  ֧���Ȳ����Ϣ��ʽ.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2012.09.01  ����� hotplug �߳̾���Ļ�ȡ.
2012.12.08  ������Դ���յĹ���.
2012.12.13  �Ȳ����Ϣ�����ظ���װ.
2013.10.03  ϵͳɾ�� message ��ʽ���Ȳ�λص�, ֧���µ� /dev/hotplug �Ȳ��֪ͨӦ�õķ���.
2013.12.01  ����ʹ����Ϣ����, ʹ���ں��ṩ�Ĺ�������ģ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_HOTPLUG_EN > 0
#if LW_CFG_DEVICE_EN > 0
#include "hotplugDev.h"
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �¼��ص��ڵ�
*********************************************************************************************************/
static LW_JOB_QUEUE            _G_jobqHotplug;
static LW_JOB_MSG              _G_jobmsgHotplug[LW_CFG_HOTPLUG_MAX_MSGS];
/*********************************************************************************************************
  �¼���ѯ�ڵ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE               HPPN_lineManage;                         /*  �ڵ�����                    */
    VOIDFUNCPTR                HPPN_pfunc;                              /*  ����ָ��                    */
    PVOID                      HPPN_pvArg;                              /*  ��������                    */
    LW_RESOURCE_RAW            HPPN_resraw;                             /*  ��Դ����ڵ�                */
} LW_HOTPLUG_POLLNODE;
typedef LW_HOTPLUG_POLLNODE   *PLW_HOTPLUG_POLLNODE;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE        _G_hHotplugLock     = 0;                 /*  ѭ���������                */
static LW_OBJECT_HANDLE        _G_hHotplug         = 0;                 /*  �߳̾��                    */
static LW_LIST_LINE_HEADER     _G_plineHotplugPoll = LW_NULL;           /*  �Ȳ��ѭ�������            */
/*********************************************************************************************************
  �Ȳ����
*********************************************************************************************************/
#define __HOTPLUG_LOCK()       API_SemaphoreMPend(_G_hHotplugLock, LW_OPTION_WAIT_INFINITE)
#define __HOTPLUG_UNLOCK()     API_SemaphoreMPost(_G_hHotplugLock)
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID  _hotplugThread(VOID);                                      /*  ��ҵ�������                */
/*********************************************************************************************************
** ��������: _hotplugInit
** ��������: ��ʼ�� hotplug ��
** �䡡��  : NONE
** �䡡��  : �Ƿ��ʼ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _hotplugInit (VOID)
{
    LW_OBJECT_HANDLE    hHotplugThread;
    LW_CLASS_THREADATTR threadattr;
    
    if (_jobQueueInit(&_G_jobqHotplug, &_G_jobmsgHotplug[0], 
                      LW_CFG_HOTPLUG_MAX_MSGS, LW_FALSE)) {
        return  (PX_ERROR);
    }

    _G_hHotplugLock = API_SemaphoreMCreate("hotplug_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_INHERIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_WAIT_PRIORITY | 
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);                    /*  ���� poll ��������          */
    if (!_G_hHotplugLock) {
        _jobQueueFinit(&_G_jobqHotplug);
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, LW_CFG_THREAD_HOTPLUG_STK_SIZE, 
                        LW_PRIO_T_SYSMSG, 
                        (LW_CFG_HOTPLUG_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
                        
    hHotplugThread = API_ThreadCreate("t_hotplug",
                                      (PTHREAD_START_ROUTINE)_hotplugThread,
                                      (PLW_CLASS_THREADATTR)&threadattr,
                                      LW_NULL);                         /*  ���� job �����߳�           */
    if (!hHotplugThread) {
        API_SemaphoreMDelete(&_G_hHotplugLock);
        _jobQueueFinit(&_G_jobqHotplug);
        return  (PX_ERROR);
    }
    
    _G_hHotplug = hHotplugThread;
    
#if LW_CFG_DEVICE_EN > 0
    _hotplugDrvInstall();                                               /*  ��ʼ���Ȳ����Ϣ�豸        */
    _hotplugDevCreate();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_HotplugContext
** ��������: �Ƿ��� hotplug �����߳���
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
BOOL  API_HotplugContext (VOID)
{
    if (API_ThreadIdSelf() == _G_hHotplug) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: API_HotplugEvent
** ��������: ����Ҫ����� hotplug �¼����봦�����
** �䡡��  : pfunc                      ����ָ��
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_HotplugEvent (VOIDFUNCPTR  pfunc, 
                       PVOID        pvArg0,
                       PVOID        pvArg1,
                       PVOID        pvArg2,
                       PVOID        pvArg3,
                       PVOID        pvArg4,
                       PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqHotplug, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_HotplugEventMessage
** ��������: ����һ�� hotplug ��Ϣ�¼�, �� t_hotplug ����ִ�еĺ���. (��˸�ʽ)
** �䡡��  : iMsg                       ��Ϣ��
**           bInsert                    �Ƿ�Ϊ����, ����Ϊ�γ�
**           pcPath                     �豸·��������
**           uiArg0                     ���Ӳ���
**           uiArg1                     ���Ӳ���
**           uiArg2                     ���Ӳ���
**           uiArg3                     ���Ӳ���
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

LW_API  
INT      API_HotplugEventMessage (INT       iMsg,
                                  BOOL      bInsert,
                                  CPCHAR    pcPath,
                                  UINT32    uiArg0,
                                  UINT32    uiArg1,
                                  UINT32    uiArg2,
                                  UINT32    uiArg3)
{
    size_t  i;
    UCHAR   ucBuffer[LW_HOTPLUG_DEV_MAX_MSGSIZE];
    
    if (pcPath == LW_NULL) {
        i = 0;
    
    } else {
        i = lib_strlen(pcPath);
        if (i > PATH_MAX) {
            _ErrorHandle(ENAMETOOLONG);
            return  (PX_ERROR);
        }
    }
    
    ucBuffer[0] = (UCHAR)((iMsg >> 24) & 0xff);
    ucBuffer[1] = (UCHAR)((iMsg >> 16) & 0xff);
    ucBuffer[2] = (UCHAR)((iMsg >>  8) & 0xff);
    ucBuffer[3] = (UCHAR)((iMsg)       & 0xff);
    ucBuffer[4] = (UCHAR)bInsert;
    
    if (pcPath) {
        lib_strcpy((PCHAR)&ucBuffer[5], pcPath);
    } else {
        ucBuffer[5] = PX_EOS;
    }
    
    i += 6;                                                             /*  �������� \0               */
    
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 24) & 0xff);                     /*  MSB                         */
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3)       & 0xff);
    
    _hotplugDevPutMsg(iMsg, ucBuffer, i);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: API_HotplugPollAdd
** ��������: �� hotplug �¼�������������, ����һ��ѭ����⺯��.
** �䡡��  : pfunc                      ����ָ��
**           pvArg                      ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_HotplugPollAdd (VOIDFUNCPTR   pfunc, PVOID  pvArg)
{
    PLW_HOTPLUG_POLLNODE        phppn;
    
    phppn = (PLW_HOTPLUG_POLLNODE)__SHEAP_ALLOC(sizeof(LW_HOTPLUG_POLLNODE));
                                                                        /*  ������ƿ��ڴ�              */
    if (!phppn) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  ȱ���ڴ�                    */
        return  (PX_ERROR);
    }
    
    phppn->HPPN_pfunc = pfunc;
    phppn->HPPN_pvArg = pvArg;
    
    __HOTPLUG_LOCK();                                                   /*  lock hotplug poll list      */
    _List_Line_Add_Ahead(&phppn->HPPN_lineManage, &_G_plineHotplugPoll);
    __HOTPLUG_UNLOCK();                                                 /*  unlock hotplug poll list    */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (__PROC_GET_PID_CUR() && vprocFindProc((PVOID)pfunc)) {
        __resAddRawHook(&phppn->HPPN_resraw, (VOIDFUNCPTR)API_HotplugPollDelete,
                        (PVOID)pfunc, pvArg, 0, 0, 0, 0);
    } else {
        phppn->HPPN_resraw.RESRAW_bIsInstall = LW_FALSE;                /*  ����Ҫ���ղ���              */
    }
#else
    __resAddRawHook(&phppn->HPPN_resraw, (VOIDFUNCPTR)API_HotplugPollDelete,
                    (PVOID)pfunc, pvArg, 0, 0, 0, 0);
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_HotplugPollDelete
** ��������: �� hotplug �¼�������������, ɾ��һ��ѭ����⺯��.
** �䡡��  : pfunc                      ����ָ��
**           pvArg                      ��������
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_HotplugPollDelete (VOIDFUNCPTR   pfunc, PVOID  pvArg)
{
    PLW_HOTPLUG_POLLNODE        phppn = LW_NULL;
    PLW_LIST_LINE               plineTemp;
    
    __HOTPLUG_LOCK();                                                   /*  lock hotplug poll list      */
    for (plineTemp  = _G_plineHotplugPoll;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phppn = _LIST_ENTRY(plineTemp, LW_HOTPLUG_POLLNODE, HPPN_lineManage);
        if ((phppn->HPPN_pfunc == pfunc) &&
            (phppn->HPPN_pvArg == pvArg)) {
            _List_Line_Del(&phppn->HPPN_lineManage, &_G_plineHotplugPoll);
            break;
        }
    }
    __HOTPLUG_UNLOCK();                                                 /*  unlock hotplug poll list    */
    
    if (plineTemp != LW_NULL) {
        __resDelRawHook(&phppn->HPPN_resraw);
        __SHEAP_FREE(phppn);
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(ERROR_HOTPLUG_POLL_NODE_NULL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _hotplugThread
** ��������: hotplug �¼������߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _hotplugThread (VOID)
{
    ULONG  ulError;
    
    for (;;) {
        ulError = _jobQueueExec(&_G_jobqHotplug, LW_HOTPLUG_SEC * LW_TICK_HZ);
        if (ulError) {                                                  /*  ��Ҫ������ѯ�¼�            */
            PLW_HOTPLUG_POLLNODE    phppn;
            PLW_LIST_LINE           plineTemp;
            
            __HOTPLUG_LOCK();                                           /*  lock hotplug poll list      */
            for (plineTemp  = _G_plineHotplugPoll;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                phppn = _LIST_ENTRY(plineTemp, LW_HOTPLUG_POLLNODE, HPPN_lineManage);
                if (phppn->HPPN_pfunc) {
                    phppn->HPPN_pfunc(phppn->HPPN_pvArg);               /*  ������ǰ��װ��ѭ����⺯��  */
                }
            }
            __HOTPLUG_UNLOCK();                                         /*  unlock hotplug poll list    */
        }
    }
}
/*********************************************************************************************************
** ��������: API_HotplugGetLost
** ��������: ��� hotplug ��Ϣ��ʧ������
** �䡡��  : NONE
** �䡡��  : ��Ϣ��ʧ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_HotplugGetLost (VOID)
{
    return  (_jobQueueLost(&_G_jobqHotplug));
}

#endif                                                                  /*  LW_CFG_HOTPLUG_EN           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
