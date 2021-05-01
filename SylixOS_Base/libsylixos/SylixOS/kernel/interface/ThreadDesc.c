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
** ��   ��   ��: ThreadDesc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 03 ��
**
** ��        ��: ����̻߳�����Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: API_ThreadDesc
** ��������: ����̻߳�����Ϣ
** �䡡��  : ulId           �߳̾��
**           ptcbdesc       �߳���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadDesc (LW_OBJECT_HANDLE  ulId, PLW_CLASS_TCB_DESC  ptcbdesc)
{
             INTREG             iregInterLevel;
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
             INT                i;

#if LW_CFG_MODULELOADER_EN > 0
             LW_LD_VPROC       *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    if (!ptcbdesc) {
        return  (ERROR_NONE);
    }

    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcbdesc->TCBD_pstkStackNow = archCtxStackEnd(&ptcb->TCB_archRegCtx);
    ptcbdesc->TCBD_pvStackFP    = ptcb->TCB_pvStackFP;
    ptcbdesc->TCBD_pvStackExt   = ptcb->TCB_pvStackExt;
    
    ptcbdesc->TCBD_pstkStackTop     = ptcb->TCB_pstkStackTop;
    ptcbdesc->TCBD_pstkStackBottom  = ptcb->TCB_pstkStackBottom;
    ptcbdesc->TCBD_stStackSize      = ptcb->TCB_stStackSize;
    ptcbdesc->TCBD_pstkStackLowAddr = ptcb->TCB_pstkStackLowAddr;
    ptcbdesc->TCBD_pstkStackGuard   = ptcb->TCB_pstkStackGuard;
    
    ptcbdesc->TCBD_iSchedRet   = ptcb->TCB_iSchedRet;
    ptcbdesc->TCBD_ulOption    = ptcb->TCB_ulOption;
    ptcbdesc->TCBD_ulId        = ptcb->TCB_ulId;
    ptcbdesc->TCBD_ulLastError = ptcb->TCB_ulLastError;
    
    ptcbdesc->TCBD_bDetachFlag = ptcb->TCB_bDetachFlag;
    ptcbdesc->TCBD_ulJoinType  = ptcb->TCB_ulJoinType;
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        _WakeupStatus(&_K_wuDelay, &ptcb->TCB_wunDelay, &ptcbdesc->TCBD_ulWakeupLeft);
    } else {
        ptcbdesc->TCBD_ulWakeupLeft = 0ul;
    }
    
    ptcbdesc->TCBD_ulCPUId = ptcb->TCB_ulCPUId;
    ptcbdesc->TCBD_bIsCand = ptcb->TCB_bIsCand;
    
    ptcbdesc->TCBD_ucSchedPolicy       = ptcb->TCB_ucSchedPolicy;
    ptcbdesc->TCBD_ucSchedActivate     = ptcb->TCB_ucSchedActivate;
    ptcbdesc->TCBD_ucSchedActivateMode = ptcb->TCB_ucSchedActivateMode;
    
    ptcbdesc->TCBD_usSchedSlice   = ptcb->TCB_usSchedSlice;
    ptcbdesc->TCBD_usSchedCounter = ptcb->TCB_usSchedCounter;
    
    ptcbdesc->TCBD_ucPriority = ptcb->TCB_ucPriority;
    ptcbdesc->TCBD_usStatus   = ptcb->TCB_usStatus;
    
    ptcbdesc->TCBD_ucWaitTimeout = ptcb->TCB_ucWaitTimeout;
    
    ptcbdesc->TCBD_ulSuspendNesting  = ptcb->TCB_ulSuspendNesting;
    ptcbdesc->TCBD_iDeleteProcStatus = ptcb->TCB_iDeleteProcStatus;
    
#if LW_CFG_THREAD_CANCEL_EN > 0
    ptcbdesc->TCBD_bCancelRequest = ptcb->TCB_bCancelRequest;
    ptcbdesc->TCBD_iCancelState   = ptcb->TCB_iCancelState;
    ptcbdesc->TCBD_iCancelType    = ptcb->TCB_iCancelType;
#else
    ptcbdesc->TCBD_bCancelRequest = LW_FALSE;
    ptcbdesc->TCBD_iCancelState   = LW_THREAD_CANCEL_ENABLE;
    ptcbdesc->TCBD_iCancelType    = LW_THREAD_CANCEL_ASYNCHRONOUS;
#endif
    
    ptcbdesc->TCBD_pthreadStartAddress = ptcb->TCB_pthreadStartAddress;
    
    ptcbdesc->TCBD_ulCPUTicks       = ptcb->TCB_ulCPUTicks;
    ptcbdesc->TCBD_ulCPUKernelTicks = ptcb->TCB_ulCPUKernelTicks;
    
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    ptcbdesc->TCBD_ulCPUUsageTicks       = ptcb->TCB_ulCPUUsageTicks;
    ptcbdesc->TCBD_ulCPUUsageKernelTicks = ptcb->TCB_ulCPUUsageKernelTicks;
#else
    ptcbdesc->TCBD_ulCPUUsageTicks       = 0ul;
    ptcbdesc->TCBD_ulCPUUsageKernelTicks = 0ul;
#endif
    
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    if (ptcb->TCB_bWatchDogInQ) {
        _WakeupStatus(&_K_wuWatchDog, &ptcb->TCB_wunWatchDog, &ptcbdesc->TCBD_ulWatchDogLeft);
    } else {
        ptcbdesc->TCBD_ulWatchDogLeft = 0ul;
    }
#else
    ptcbdesc->TCBD_ulWatchDogLeft = 0ul;
#endif
    
    ptcbdesc->TCBD_ulThreadLockCounter = ptcb->TCB_ulThreadLockCounter;
    ptcbdesc->TCBD_ulThreadSafeCounter = ptcb->TCB_ulThreadSafeCounter;
    
    ptcbdesc->TCBD_ucStackAutoAllocFlag = ptcb->TCB_ucStackAutoAllocFlag;
    
    lib_strcpy(ptcbdesc->TCBD_cThreadName, ptcb->TCB_cThreadName);
    
#if LW_CFG_MODULELOADER_EN > 0
    pvproc = __LW_VP_GET_TCB_PROC(ptcb);
    ptcbdesc->TCBD_lPid = (LONG)((pvproc) ? pvproc->VP_pid : 0);
#else
    ptcbdesc->TCBD_lPid = 0;
#endif
    
    ptcbdesc->TCBD_i64PageFailCounter = ptcb->TCB_i64PageFailCounter;
    
    ptcbdesc->TCBD_uid = ptcb->TCB_uid;
    ptcbdesc->TCBD_gid = ptcb->TCB_gid;
    
    ptcbdesc->TCBD_euid = ptcb->TCB_euid;
    ptcbdesc->TCBD_egid = ptcb->TCB_egid;
    
    ptcbdesc->TCBD_suid = ptcb->TCB_suid;
    ptcbdesc->TCBD_sgid = ptcb->TCB_sgid;
    
    for (i = 0; i < 16; i++) {
        ptcbdesc->TCBD_suppgid[i] = ptcb->TCB_suppgid[i];
    }
        
    ptcbdesc->TCBD_iNumSuppGid = ptcb->TCB_iNumSuppGid;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
