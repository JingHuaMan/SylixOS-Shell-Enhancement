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
** ��   ��   ��: CoroutineCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 19 ��
**
** ��        ��: ����Э�̹����(Э����һ���������Ĳ���ִ�е�λ). 
                 �ڵ�ǰ�߳��д���һ��Э��.
** BUG:
2009.03.05  ������ѧ�׷���, �׷���Զ������ѧϰ�İ���.
            ����ջ��С��Ϊ ULONG ����.
2009.04.06  ����ջ����ֽ�ȷ��Ϊ LW_CFG_STK_EMPTY_FLAG. ����߶�ջ����׼ȷ��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.12.14  ʹ������������ȷ������Э�����������ȫ��.
2013.12.17  ��ջ��С��Ϊ size_t ����.
2016.05.03  �����ڵ�Э��ʹ�� vmm ��ջ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** ��������: API_CoroutineCreate
** ��������: �ڵ�ǰ�߳��д���һ��Э��.
** �䡡��  : pCoroutineStartAddr       Э��������ַ
**           stStackByteSize           ��ջ��С
**           pvArg                     ��ڲ���
** �䡡��  : Э�̿��ƾ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID   API_CoroutineCreate (PCOROUTINE_START_ROUTINE pCoroutineStartAddr,
                             size_t                   stStackByteSize,
                             PVOID                    pvArg)
{
             INTREG                iregInterLevel;
             PLW_CLASS_TCB         ptcbCur;

    REGISTER PLW_STACK             pstkTop;
    REGISTER PLW_STACK             pstkButtom;
    REGISTER PLW_STACK             pstkLowAddress;
    REGISTER size_t                stStackSizeWordAlign;                /*  ��ջ��С(��λ����)          */
    
             PLW_CLASS_COROUTINE   pcrcbNew;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (LW_NULL);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pCoroutineStartAddr) {                                         /*  ָЭ�̴������ʼ��ַΪ��    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine code segment not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    if (_StackSizeCheck(stStackByteSize)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine stack size low.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (LW_NULL);
    }
#endif

    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */

    pstkLowAddress = _StackAllocate(ptcbCur, 0ul, stStackByteSize);     /*  �����ڴ�                    */
    if (!pstkLowAddress) {
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    stStackSizeWordAlign = _CalWordAlign(stStackByteSize);              /*  ��������ջ��С            */

#if CPU_STK_GROWTH == 0                                                 /*  Ѱ�Ҷ�ջͷβ                */
    pstkTop    = pstkLowAddress;
    pstkButtom = pstkLowAddress + stStackSizeWordAlign - 1;
    
    pstkTop    = (PLW_STACK)ROUND_UP(pstkTop, ARCH_STK_ALIGN_SIZE);
    pcrcbNew   = (PLW_CLASS_COROUTINE)pstkTop;                          /*  ��¼ CRCB λ��              */
    pstkTop    = (PLW_STACK)((BYTE *)pstkTop + __CRCB_SIZE_ALIGN + sizeof(LW_STACK));    
                                                                        /*  Ѱ������ջ��                */
    stStackSizeWordAlign = pstkButtom - pstkTop + 1;

#else
    pstkTop    = pstkLowAddress + stStackSizeWordAlign - 1;
    pstkButtom = pstkLowAddress;
    
    pstkTop    = (PLW_STACK)((BYTE *)pstkTop - __CRCB_SIZE_ALIGN);      /*  �ó� CRCB �ռ�              */
    pstkTop    = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);
    pcrcbNew   = (PLW_CLASS_COROUTINE)pstkTop;                          /*  ��¼ CRCB λ��              */
    pstkTop--;                                                          /*  ���ջ�����ƶ�һ����ջ�ռ�  */

    stStackSizeWordAlign = pstkTop - pstkButtom + 1;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */
    
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_STK_CLR) {
        lib_memset((BYTE *)pstkLowAddress,                              /*  ��Ҫ�����ջ                */
                   LW_CFG_STK_EMPTY_FLAG, 
                   stStackSizeWordAlign * sizeof(LW_STACK));            /*  ��λ���ֽ�                  */
    }
    
    archTaskCtxCreate(&pcrcbNew->COROUTINE_archRegCtx,
                      (PTHREAD_START_ROUTINE)_CoroutineShell,
                      (PVOID)pCoroutineStartAddr,                       /*  �����Ŀ�ִ�д�����          */
                      ptcbCur, pstkTop,
                      ptcbCur->TCB_ulOption);

    pcrcbNew->COROUTINE_pstkStackTop     = pstkTop;                     /*  �߳�����ջջ��              */
    pcrcbNew->COROUTINE_pstkStackBottom  = pstkButtom;                  /*  �߳�����ջջ��              */
    pcrcbNew->COROUTINE_stStackSize      = stStackSizeWordAlign;        /*  �̶߳�ջ��С(��λ����)      */
    pcrcbNew->COROUTINE_pstkStackLowAddr = pstkLowAddress;              /*  �ܶ�ջ��͵�ַ              */
	
	pcrcbNew->COROUTINE_pvArg    = pvArg;
	pcrcbNew->COROUTINE_ulThread = ptcbCur->TCB_ulId;
	pcrcbNew->COROUTINE_ulFlags  = LW_COROUTINE_FLAG_DYNSTK;            /*  ��Ҫɾ����ջ                */
	
    LW_SPIN_LOCK_QUICK(&ptcbCur->TCB_slLock, &iregInterLevel);
    _List_Ring_Add_Last(&pcrcbNew->COROUTINE_ringRoutine,
                        &ptcbCur->TCB_pringCoroutineHeader);            /*  ����Э�̱�                  */
    LW_SPIN_UNLOCK_QUICK(&ptcbCur->TCB_slLock, iregInterLevel);
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_CREATE, 
                      ptcbCur->TCB_ulId, pcrcbNew, stStackByteSize, LW_NULL);
    
    return  ((PVOID)pcrcbNew);                                          /*  �����ɹ�                    */
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
