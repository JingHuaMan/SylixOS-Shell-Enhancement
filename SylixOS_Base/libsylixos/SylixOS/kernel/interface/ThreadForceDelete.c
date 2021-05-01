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
** ��   ��   ��: ThreadForceDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 16 ��
**
** ��        ��: �߳�ǿ��ɾ������������ʹ��, ����Ŀ���߳��Ƿ������˰�ȫ��ʶ, ����ɾ��.

** BUG
2008.01.16  ��ɾ����ȫģʽ����ʱ, ��ӡ��� LOG ��Ϣ.
2008.03.29  ������µ� wake up �� watch dog ���ƵĴ���.
2008.03.30  ʹ���µľ���������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.02.03  �������� TCB ��չ��.
2009.05.08  ���� _CleanupPopTCBExt().
2009.05.24  �� TCB_bIsInDeleteProc ������ǰ.
2009.07.31  _excJobAdd() Ӧ��ǿ��ɾ��.
2011.02.24  ����ʹ���߳����˯�ߵķ���, �����ý���״̬���ֻ���.
2012.12.08  ������Դ��������̬�ص�.
2012.12.23  ���� API_ThreadForceDelete() �㷨, 
            �����ɾ������, ����Ҫ�ص��ĺ������н�������ʹ�� except �߳�ɾ��.
2013.08.23  ����ɾ���ص�ִ�и��Ӱ�ȫ.
            ֻ��ɾ����ͬ�����ڵ�����. �������̵�������Ҫʹ���ź�ɾ��.
2013.08.28  �����ں��¼������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern ULONG  __threadDelete(PLW_CLASS_TCB  ptcbDel, BOOL  bIsInSafe, 
                             PVOID  pvRetVal, BOOL  bIsAlreadyWaitDeath);
/*********************************************************************************************************
** ��������: API_ThreadForceDelete
** ��������: �߳�ǿ��ɾ������
** �䡡��  : 
**           pulId         ���
**           pvRetVal      ����ֵ   (���ظ� JOIN ���߳�)
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadForceDelete (LW_OBJECT_HANDLE  *pulId, PVOID  pvRetVal)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
    REGISTER BOOL                  bIsInSafeMode;                       /*  ����߳��Ƿ��ڰ�ȫģʽ      */
             ULONG                 ulError;
             
#if LW_CFG_MODULELOADER_EN > 0
             LW_LD_VPROC          *pvprocCur;
             LW_LD_VPROC          *pvprocDel;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳû���������ܵ���        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    pvprocCur = __LW_VP_GET_TCB_PROC(ptcbCur);
    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocCur != pvprocDel) {                                       /*  ������һ������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {
        if (ptcbCur != ptcbDel) {                                       /*  ���߳�ֻ���Լ�ɾ���Լ�      */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not delete main thread.\r\n");
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (ptcbDel->TCB_iDeleteProcStatus) {                               /*  ����Ƿ���ɾ��������        */
        if (ptcbDel == ptcbCur) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            for (;;) {
                API_TimeSleep(__ARCH_ULONG_MAX);                        /*  �ȴ���ɾ��                  */
            }
            return  (ERROR_NONE);                                       /*  ��Զ���в�������            */
        
        } else {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
    }
    
    if (ptcbDel->TCB_ulThreadSafeCounter) {                             /*  ����Ƿ��ڰ�ȫģʽ        */
        bIsInSafeMode = LW_TRUE;
    } else {
        bIsInSafeMode = LW_FALSE;
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {            /*  ���߳��Լ�ɾ���Լ�          */
        if (pvprocDel->VP_iStatus != __LW_VP_EXIT) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            vprocExit(pvprocDel, ulId, (INT)(LONG)pvRetVal);            /*  �����˳�, �˺���������      */
            return  (ERROR_NONE);                                       /*  �������е�����              */
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    ptcbDel->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_DEL;            /*  ����ɾ������                */
    
    _ObjectCloseId(pulId);                                              /*  �ر� ID                     */
    
    ptcbCur->TCB_ulThreadSafeCounter++;                                 /*  LW_THREAD_SAFE();           */
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ptcbDel == ptcbCur) {
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  ���� hook ��ز���          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        ptcbCur->TCB_ulThreadSafeCounter--;                             /*  LW_THREAD_UNSAFE();         */
        
        _excJobAdd((VOIDFUNCPTR)__threadDelete,
                   ptcbDel, (PVOID)(LONG)bIsInSafeMode, pvRetVal,
                   (PVOID)LW_FALSE, 0, 0);                              /*  ʹ���ź�ϵͳ���쳣����      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  �ȴ���ɾ��                  */
        }
        ulError = ERROR_NONE;
    
    } else {
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  ��Ҫɾ�����߳̽��뽩��״̬  */
        
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  ���� hook ��ز���          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        ulError = __threadDelete(ptcbDel, bIsInSafeMode, pvRetVal, LW_TRUE);
    
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
