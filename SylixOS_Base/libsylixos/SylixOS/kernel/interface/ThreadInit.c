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
** ��   ��   ��: ThreadInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����ϵͳ�����߳����Կ麯����

** BUG
2007.01.08  ���ڴ濪�ٿ��Բ�ʹ�� Lock() �� Unlok()
2007.07.18  ������ _DebugHandle() ����
2007.11.08  ���û�������Ϊ�ں˶�.
2007.12.24  �ڽ��� BuildTcb ʱ��Ҫ���밲ȫģʽ, ������˳���ȫģʽ.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2008.08.17  ���� TCB ����.
2009.02.03  ���� TCB ��չ�ṹ�Ĵ���
2009.04.06  ����ջ����ֽ�ȷ��Ϊ LW_CFG_STK_EMPTY_FLAG. ����߶�ջ����׼ȷ��.
2009.05.26  pthreadattr Ϊ��ʱ, ʹ��Ĭ�����Դ����߳�.
2009.06.02  �޸��˶� pthreadattr �жϵ�λ��. ȥ���˶Ը����ջ���жϸ��� BSP ���.
2010.01.22  _K_usThreadCounter �����ں˺��޸�.
2012.03.21  ʹ���µ� thread attr ��ȡ��ʽ.
2013.05.07  TCB ���ٷ��ڶ�ջ��, ���Ǵ� TCB ���п�ʼ����.
2013.08.27  �����ں��¼������.
2013.09.17  �����̶߳�ջ����ָ�봦��.
2015.11.24  �����ڵ��߳�ʹ�� vmm ��ջ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadInit
** ��������: ��ʼ��һ���߳�
** �䡡��  : pcName                 �߳���
**           pfuncThread            ָ�̴߳������ʼ��ַ
**           pthreadattr            �߳����Լ���ָ��
**           pulId                  �߳����ɵ�IDָ��     ����Ϊ NULL
** �䡡��  : pulId                  �߳̾��             ͬ ID һ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadInit (CPCHAR                   pcName,
                                  PTHREAD_START_ROUTINE    pfuncThread,
                                  PLW_CLASS_THREADATTR     pthreadattr,
                                  LW_OBJECT_ID            *pulId)
{
    static LW_CLASS_THREADATTR     threadattrDefault;

    REGISTER PLW_STACK             pstkTop;
    REGISTER PLW_STACK             pstkButtom;
    REGISTER PLW_STACK             pstkGuard;
    REGISTER PLW_STACK             pstkLowAddress;
    
             size_t                stStackSize;                         /*  ��ջ��С(��λ����)          */
             size_t                stGuardSize;
                         
             PLW_CLASS_TCB         ptcb;
             ULONG                 ulIdTemp;
             
             ULONG                 ulError;
             INT                   iErrLevel = 0;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (threadattrDefault.THREADATTR_stStackByteSize == 0) {
        threadattrDefault = API_ThreadAttrGetDefault();                 /*  ��ʼ��Ĭ������              */
    }
    
    if (pthreadattr == LW_NULL) {
        pthreadattr = &threadattrDefault;                               /*  ʹ��Ĭ������                */
    }                                                                   /*  Ĭ����������ʹ���Զ������ջ*/
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pfuncThread) {                                                 /*  ָ�̴߳������ʼ��ַΪ��    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread code segment not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_StackSizeCheck(pthreadattr->THREADATTR_stStackByteSize)) {     /*  ��ջ��С����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size low.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pfuncThread != _IdleThread) {                                   /*  idle ����Ϊ�Ϸ�������ȼ�   */
        if (_PriorityCheck(pthreadattr->THREADATTR_ucPriority)) {       /*  ���ȼ�����                  */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
            _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
            return  (LW_OBJECT_HANDLE_INVALID);
        }
    }
#endif

    stGuardSize = pthreadattr->THREADATTR_stGuardSize;

    if (stGuardSize < (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))) {
        stGuardSize = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    }

    if (stGuardSize > pthreadattr->THREADATTR_stStackByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack guard size invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        ptcb = _Allocate_Tcb_Object();                                  /*  ���һ�� TCB                */
    );
    
    if (!ptcb) {                                                        /*  ����Ƿ���Խ����߳�        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a thread.\r\n");
        _ErrorHandle(ERROR_THREAD_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        LW_THREAD_SAFE();                                               /*  ���밲ȫģʽ                */
    }
    
    lib_bzero(&ptcb->TCB_pstkStackTop, 
              sizeof(LW_CLASS_TCB) - 
              _LIST_OFFSETOF(LW_CLASS_TCB, TCB_pstkStackTop));          /*  TCB ����                    */
    
    if (!pthreadattr->THREADATTR_pstkLowAddr) {                         /*  �Ƿ���ں˶��п��ٶ�ջ      */
        pstkLowAddress = _StackAllocate(ptcb,
                                        pthreadattr->THREADATTR_ulOption,
                                        pthreadattr->THREADATTR_stStackByteSize);
        if (!pstkLowAddress) {                                          /*  ����ʧ��                    */
            iErrLevel = 1;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
            goto    __error_handle;
        }
    
    } else {
        pstkLowAddress = pthreadattr->THREADATTR_pstkLowAddr;           /*  ��ջ�ĵ͵�ַ                */
    }
    
    stStackSize = _CalWordAlign(pthreadattr->THREADATTR_stStackByteSize);
    stGuardSize = _CalWordAlign(stGuardSize);

#if CPU_STK_GROWTH == 0                                                 /*  Ѱ�Ҷ�ջͷβ                */
    pstkTop    = pstkLowAddress;
    pstkButtom = pstkLowAddress + stStackSize - 1;
    pstkGuard  = pstkButtom - stGuardSize - 1;
#else
    pstkTop    = pstkLowAddress + stStackSize - 1;
    pstkButtom = pstkLowAddress;
    pstkGuard  = pstkButtom + stGuardSize - 1;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */

    ulIdTemp = _MakeObjectId(_OBJECT_THREAD, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             ptcb->TCB_usIndex);                        /*  �������� id                 */
    
    if (pthreadattr->THREADATTR_ulOption & LW_OPTION_THREAD_STK_CLR) {
        lib_memset(pstkLowAddress, LW_CFG_STK_EMPTY_FLAG, 
                   pthreadattr->THREADATTR_stStackByteSize);            /*  ��Ҫ�����ջ                */
    }
                                                                        /*  ��ʼ����ջ��SHELL           */
    archTaskCtxCreate(&ptcb->TCB_archRegCtx,
                      (PTHREAD_START_ROUTINE)_ThreadShell,
                      (PVOID)pfuncThread,                               /*  �����Ŀ�ִ�д�����          */
                      ptcb, pstkTop,
                      pthreadattr->THREADATTR_ulOption);
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(ptcb->TCB_cThreadName, pcName);
    } else {
        ptcb->TCB_cThreadName[0] = PX_EOS;
    }
    
    ulError = _TCBBuildExt(ptcb);                                       /*  �����ȳ�ʼ����չ�ṹ        */
    if (ulError) {
        iErrLevel = 2;
        _ErrorHandle(ulError);
        goto    __error_handle;
    }
    
    _TCBBuild(pthreadattr->THREADATTR_ucPriority,                       /*  ���� TCB                    */
              pstkTop,                                                  /*  ��ջ����ַ                  */
              pstkButtom,                                               /*  ջ��                        */
              pstkGuard,
              pthreadattr->THREADATTR_pvExt,
              pstkLowAddress,
              stStackSize,                                              /*  ������ֶ���Ķ�ջ��С      */
              ulIdTemp,
              (pthreadattr->THREADATTR_ulOption | 
               LW_OPTION_THREAD_INIT),                                  /*  ��ʼ��                      */
              pfuncThread,
              ptcb,
              pthreadattr->THREADATTR_pvArg);

    if (pthreadattr->THREADATTR_pstkLowAddr) {                          /*  �Զ����ټ��                */
        ptcb->TCB_ucStackAutoAllocFlag = LW_STACK_MANUAL_ALLOC;
    
    } else {
        ptcb->TCB_ucStackAutoAllocFlag = LW_STACK_AUTO_ALLOC;
    }
    
    __KERNEL_MODE_PROC(_K_usThreadCounter++;);                          /*  �߳�����++                  */
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, pthreadattr->THREADATTR_ulOption);
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_INIT, 
                      ulIdTemp, 
                      pthreadattr->THREADATTR_ulOption, 
                      pthreadattr->THREADATTR_stStackByteSize,
                      pthreadattr->THREADATTR_ucPriority,
                      pthreadattr->THREADATTR_pvArg,
                      pcName);
    
    if (pulId) {
        *pulId = ulIdTemp;                                              /*  ��¼ ID                     */
    }
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been initialized.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
    
__error_handle:
    if (iErrLevel > 1) {
        if (!pthreadattr->THREADATTR_pstkLowAddr) {
            _StackFree(ptcb, pstkLowAddress);                           /*  �ͷŶ�ջ�ռ�                */
        }
    }
    if (iErrLevel > 0) {
        __KERNEL_MODE_PROC(
            _Free_Tcb_Object(ptcb);                                     /*  �ͷ� ID                     */
        );
    }
    if (LW_SYS_STATUS_IS_RUNNING()) {
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
    }
    
    return  (LW_OBJECT_HANDLE_INVALID);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
