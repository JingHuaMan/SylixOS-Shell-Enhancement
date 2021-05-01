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
** ��   ��   ��: threadpool.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 03 �� 20 ��
**
** ��        ��: ϵͳ�̳߳ع��ܽӿں�����

** BUG
2007.09.21  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2008.05.31  ֻ��ʹ�ö�̬�ڴ����.
2013.09.22  �����̳߳� pthreadattr Ϊ NULL ��ʹ��Ĭ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
#include "threadpoolLib.h"
/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define __OPT_MUTEX_LOCK   (LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | LW_OPTION_INHERIT_PRIORITY)
/*********************************************************************************************************
  threadpool lock
*********************************************************************************************************/
#define __THREADPOOL_LOCK(pthreadpool)      \
        API_SemaphoreMPend(pthreadpool->TPCB_hMutexLock, LW_OPTION_WAIT_INFINITE)
#define __THREADPOOL_UNLOCK(pthreadpool)    \
        API_SemaphoreMPost(pthreadpool->TPCB_hMutexLock)
/*********************************************************************************************************
** ��������: API_ThreadPoolCreate
** ��������: ����һ�� ThreadPool
** �䡡��  : pcName                        �̳߳�����
**           pfuncThread                   �������
**           pthreadattr                   �߳����Կ��ƿ�
**           usMaxThreadCounter            ����߳�����
**           pulId                         Idָ��
** �䡡��  : �½������̳߳ؾ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadPoolCreate (PCHAR                    pcName,
                                        PTHREAD_START_ROUTINE    pfuncThread,
                                        PLW_CLASS_THREADATTR     pthreadattr,
                                        UINT16                   usMaxThreadCounter,
                                        LW_OBJECT_ID            *pulId)
{
    REGISTER PLW_CLASS_THREADPOOL   pthreadpool;
    REGISTER ULONG                  ulIdTemp;
    REGISTER ULONG                  ulOption;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pfuncThread) {                                                 /*  û���߳�ִ�к���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pfuncThread invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (usMaxThreadCounter >= LW_CFG_MAX_THREADS || 
        usMaxThreadCounter == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "usMaxThreadCounter invalidate.\r\n");
        _ErrorHandle(ERROR_THREADPOOL_MAX_COUNTER);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pthreadpool = _Allocate_ThreadPool_Object();                    /*  ���һ���������ƿ�          */
    );
    
    if (!pthreadpool) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "threadpool full.\r\n");
        _ErrorHandle(ERROR_THREADPOOL_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pthreadpool->TPCB_hMutexLock = API_SemaphoreMCreate("tp_lock",
                                                        LW_PRIO_DEF_CEILING,
                                                        __OPT_MUTEX_LOCK,
                                                        LW_NULL);       /*  ��������������              */
    if (!pthreadpool->TPCB_hMutexLock) {                                /*  ����ʧ��                    */
        __KERNEL_MODE_PROC(
            _Free_ThreadPool_Object(pthreadpool);
        );
        _ErrorHandle(ERROR_EVENT_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pthreadpool->TPCB_cThreadPoolName, pcName);
    } else {
        pthreadpool->TPCB_cThreadPoolName[0] = PX_EOS;                  /*  �������                    */
    }
    
    if (pthreadattr) {
        pthreadpool->TPCB_threakattrAttr = *pthreadattr;                /*  �������Կ�                  */
    
    } else {
        pthreadpool->TPCB_threakattrAttr = API_ThreadAttrGetDefault();
    }
    
    pthreadpool->TPCB_threakattrAttr.THREADATTR_pstkLowAddr = LW_NULL;  /*  ������ö�̬�����ջ        */
    
    pthreadpool->TPCB_pthreadStartAddress = pfuncThread;                /*  �̴߳������                */
    pthreadpool->TPCB_pringFirstThread    = LW_NULL;                    /*  û���߳�                    */
    pthreadpool->TPCB_usMaxThreadCounter  = usMaxThreadCounter;         /*  ����߳�����                */   
    pthreadpool->TPCB_usThreadCounter     = 0;                          /*  ��ǰ�߳�����                */
    
    ulOption = pthreadattr->THREADATTR_ulOption;
    
    ulIdTemp = _MakeObjectId(_OBJECT_THREAD_POOL, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pthreadpool->TPCB_usIndex);                /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread pool \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolDelete
** ��������: ɾ��һ�� ThreadPool
** �䡡��  : pulId                         �̳߳ؾ��ָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolDelete (LW_OBJECT_HANDLE   *pulId)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER UINT16                    usI;
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    for (usI = pthreadpool->TPCB_usThreadCounter;                       /*  ��ʼɾ���������߳�          */
         usI > 0;
         usI--) {
         
        pringTcb = pthreadpool->TPCB_pringFirstThread;
        
        ptcb = _LIST_ENTRY(pringTcb, LW_CLASS_TCB, TCB_ringThreadPool);
        
        hThread = ptcb->TCB_ulId;
        
        API_ThreadDelete(&hThread, LW_NULL);
        if (hThread) {                                                  /*  �ͷ��ź���                  */
            __THREADPOOL_UNLOCK(pthreadpool);
            ulErrCode = API_GetLastError();                             /*  �� API_ThreadDelete ����    */
            return  (ulErrCode);                                        /*   _ErrorHandle()             */
        }
        
        pthreadpool->TPCB_usThreadCounter--;
        
        _List_Ring_Del(pringTcb, &pthreadpool->TPCB_pringFirstThread);  /*  �ӹ���������ɾ��            */
        
    }
    
    API_SemaphoreMDelete(&pthreadpool->TPCB_hMutexLock);                /*  ɾ����                      */
    
    _ObjectCloseId(pulId);

    _DebugFormat(__LOGMESSAGE_LEVEL, "thread pool \"%s\" has been delete.\r\n", 
                 pthreadpool->TPCB_cThreadPoolName);
    
    __KERNEL_MODE_PROC(
        _Free_ThreadPool_Object(pthreadpool);                           /*  �ͷſ��ƿ�                  */
    );
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolAddThread
** ��������: ָ��һ�� ThreadPool ����һ���߳�
** �䡡��  : 
**           ulId                          �̳߳ؾ��
**           pcArg                         �߳���ڲ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolAddThread (LW_OBJECT_HANDLE   ulId, PVOID  pvArg)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if ((pthreadpool->TPCB_usThreadCounter) == 
        (pthreadpool->TPCB_usMaxThreadCounter)) {                       /*  �Ƿ�Ӧ������                */
        __THREADPOOL_UNLOCK(pthreadpool);
        _ErrorHandle(ERROR_THREADPOOL_FULL);
        return  (ERROR_THREADPOOL_FULL);
    }
    
    pthreadpool->TPCB_threakattrAttr.THREADATTR_pvArg = pvArg;          /*  �����²���                  */
    
    hThread = API_ThreadCreate(pthreadpool->TPCB_cThreadPoolName,
                               pthreadpool->TPCB_pthreadStartAddress,
                               &pthreadpool->TPCB_threakattrAttr,
                               LW_NULL);                                /*  �����߳�                    */
                     
    if (!hThread) {
        __THREADPOOL_UNLOCK(pthreadpool);
        ulErrCode = API_GetLastError();
        return  (ulErrCode);
    }
    
    pthreadpool->TPCB_usThreadCounter++;
    
    ptcb = _K_ptcbTCBIdTable[_ObjectGetIndex(hThread)];                 /*  ����߳̿��ƿ�              */
    
    pringTcb = &ptcb->TCB_ringThreadPool;
    
    _List_Ring_Add_Last(pringTcb, &pthreadpool->TPCB_pringFirstThread); /*  �����������                */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolDeleteThread
** ��������: ָ��һ�� ThreadPool ɾ��һ���߳�
** �䡡��  : 
**           ulId                          �̳߳ؾ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolDeleteThread (LW_OBJECT_HANDLE   ulId)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (pthreadpool->TPCB_usThreadCounter == 0) {                       /*  �Ƿ��Ѿ�����                */
        __THREADPOOL_UNLOCK(pthreadpool);
        _ErrorHandle(ERROR_THREADPOOL_NULL);
        return  (ERROR_THREADPOOL_NULL);
    }
    
    pringTcb = pthreadpool->TPCB_pringFirstThread;
        
    ptcb = _LIST_ENTRY(pringTcb, LW_CLASS_TCB, TCB_ringThreadPool);
        
    hThread = ptcb->TCB_ulId;
    
    API_ThreadDelete(&hThread, LW_NULL);
    if (hThread) {                                                      /*  �ͷ��ź���                  */
        __THREADPOOL_UNLOCK(pthreadpool);
        ulErrCode = API_GetLastError();                                 /*  �� API_ThreadDelete ����    */
        return  (ulErrCode);                                            /*  _ErrorHandle()              */
    }
    
    pthreadpool->TPCB_usThreadCounter--;
        
    _List_Ring_Del(pringTcb, &pthreadpool->TPCB_pringFirstThread);      /*  �ӹ���������ɾ��            */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolSetAttr
** ��������: ָ��һ�� ThreadPool �����߳̽������Կ�
** �䡡��  : ulId                          �̳߳ؾ��
**           pthreadattr                   �߳����Կ��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolSetAttr (LW_OBJECT_HANDLE  ulId, PLW_CLASS_THREADATTR  pthreadattr)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ȱ�����Կ�                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pthreadattr invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    pthreadpool->TPCB_threakattrAttr = *pthreadattr;                    /*  �������Կ�                  */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolGetAttr
** ��������: ָ��һ�� ThreadPool ����߳̽������Կ�
** �䡡��  : 
**           ulId                          �̳߳ؾ��
**           pthreadattr                   �߳����Կ��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolGetAttr (LW_OBJECT_HANDLE  ulId, PLW_CLASS_THREADATTR  pthreadattr)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ȱ�����Կ�                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pthreadattr invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    *pthreadattr = pthreadpool->TPCB_threakattrAttr;
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadPoolStatus
** ��������: ��� ThreadPool ״̬
** �䡡��  : 
**           ulId                          �̳߳ؾ��
**           ppthreadStartAddr             �߳���ڵ�ַ    ����Ϊ NULL
**           pusMaxThreadCounter           �߳��������ֵ  ����Ϊ NULL
**           pusThreadCounter              ��ǰ�߳�����    ����Ϊ NULL
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolStatus (LW_OBJECT_HANDLE         ulId,
                             PTHREAD_START_ROUTINE   *ppthreadStartAddr,
                             UINT16                  *pusMaxThreadCounter,
                             UINT16                  *pusThreadCounter)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  ��                          */
    if (ulErrCode) {                                                    /*  ��ɾ���ˣ�                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (ppthreadStartAddr) {
        *ppthreadStartAddr = pthreadpool->TPCB_pthreadStartAddress;
    }
    
    if (pusMaxThreadCounter) {
        *pusMaxThreadCounter = pthreadpool->TPCB_usMaxThreadCounter;
    }
    
    if (pusThreadCounter) {
        *pusThreadCounter = pthreadpool->TPCB_usThreadCounter;
    }
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
                                                                        /*  LW_CFG_MAX_THREAD_POOLS > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
