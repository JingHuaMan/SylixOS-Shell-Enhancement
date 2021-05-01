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
** ��   ��   ��: k_internal.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 15 ��
**
** ��        ��: ����ϵͳ�ڲ�����������

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
2007.09.28  �궨����˷ֺ�.
2007.11.07  ���û�������Ϊ�ں˶�.
2008.01.24  ���������ڴ����.
2011.02.24  ȥ�� kernel exit int ��������.
            �������õ���������ֵר�ú���.
2011.03.05  �����ڴ�ѷ�����ͼ��Ϣ, ����ʹ���ڴ���������и���.
*********************************************************************************************************/

#ifndef __K_INTERNAL_H
#define __K_INTERNAL_H

/*********************************************************************************************************
  ��������ǿ�����Ŷ���
*********************************************************************************************************/

#ifdef __GNUC__
#define weak_alias(name, aliasname)     _weak_alias(name, aliasname)
#define strong_alias(name, aliasname)   _strong_alias(name, aliasname)

#define _weak_alias(name, aliasname)    \
extern  __typeof (name) aliasname __attribute__((weak, alias(#name)));

#define _strong_alias(name, aliasname)  \
extern  __typeof (name) aliasname __attribute__((alias(#name)));
#else 
#define weak_alias(name, aliasname)
#define strong_alias(name, aliasname)
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  �ڴ�Ѻ����
*********************************************************************************************************/

#ifdef __GNUC__
#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2
#define __func__ __FUNCTION__
#else
#define __func__ "<unknown>"
#endif                                                                  /*  __GNUC__ >= 2               */
#endif                                                                  /*  __STDC_VERSION__ < 199901L  */
#endif                                                                  /*  __GNUC__                    */

#define __KHEAP_ALLOC(stNBytes)         _HeapAllocate(_K_pheapKernel, stNBytes, __func__)
#define __SHEAP_ALLOC(stNBytes)         _HeapAllocate(_K_pheapSystem, stNBytes, __func__)

#define __KHEAP_ZALLOC(stNBytes)        _HeapZallocate(_K_pheapKernel, stNBytes, __func__)
#define __SHEAP_ZALLOC(stNBytes)        _HeapZallocate(_K_pheapSystem, stNBytes, __func__)

#define __KHEAP_REALLOC(pvMemory, stNBytes)     \
        _HeapRealloc(_K_pheapKernel, pvMemory, stNBytes, LW_FALSE, __func__)
         
#define __SHEAP_REALLOC(pvMemory, stNBytes)     \
        _HeapRealloc(_K_pheapSystem, pvMemory, stNBytes, LW_FALSE, __func__)

#define __KHEAP_FREE(pvMemory)          _HeapFree(_K_pheapKernel, pvMemory, LW_FALSE, __func__)
#define __SHEAP_FREE(pvMemory)          _HeapFree(_K_pheapSystem, pvMemory, LW_FALSE, __func__)

/*********************************************************************************************************
  �����ڴ�Ѻ����
*********************************************************************************************************/

#define __KHEAP_ALLOC_ALIGN(stNBytes, stAlgin)  \
        _HeapAllocateAlign(_K_pheapKernel, stNBytes, stAlgin, __func__)
         
#define __SHEAP_ALLOC_ALIGN(stNBytes, stAlgin)  \
        _HeapAllocateAlign(_K_pheapSystem, stNBytes, stAlgin, __func__)
         
/*********************************************************************************************************
  �ں���ϵͳ���ڴ�Ѻ���� (���������������ں�ģ�����ʹ�� sys_malloc �����ڴ�)
      
  ע��: ��ֲ linux ϵͳ����ʱ, kmalloc �� kfree ����ʹ��   sys_malloc �� sys_free �滻.
                               vmalloc �� vfree Ҳ����ʹ�� sys_malloc �� sys_free �滻.
*********************************************************************************************************/

#define ker_malloc(size)                __KHEAP_ALLOC((size_t)(size))
#define ker_zalloc(size)                __KHEAP_ZALLOC((size_t)(size))
#define ker_free(p)                     __KHEAP_FREE((p))
#define ker_realloc(p, new_size)        __KHEAP_REALLOC((p), (size_t)(new_size))
#define ker_malloc_align(size, align)   __KHEAP_ALLOC_ALIGN((size_t)(size), (size_t)(align))

#define sys_malloc(size)                __SHEAP_ALLOC((size_t)(size))
#define sys_zalloc(size)                __SHEAP_ZALLOC((size_t)(size))
#define sys_free(p)                     __SHEAP_FREE((p))
#define sys_realloc(p, new_size)        __SHEAP_REALLOC((p), (size_t)(new_size))
#define sys_malloc_align(size, align)   __SHEAP_ALLOC_ALIGN((size_t)(size), (size_t)(align))

/*********************************************************************************************************
  ������
*********************************************************************************************************/

VOID  _List_Ring_Add_Ahead(PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader);
VOID  _List_Ring_Add_Front(PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader);
VOID  _List_Ring_Add_Last(PLW_LIST_RING   pringNew, LW_LIST_RING_HEADER  *ppringHeader);
VOID  _List_Ring_Del(PLW_LIST_RING        pringDel, LW_LIST_RING_HEADER  *ppringHeader);

/*********************************************************************************************************
  ������
*********************************************************************************************************/

VOID  _List_Line_Add_Ahead(PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
VOID  _List_Line_Add_Tail( PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
VOID  _List_Line_Add_Left(PLW_LIST_LINE   plineNew, PLW_LIST_LINE  plineRight);
VOID  _List_Line_Add_Right(PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineLeft);
VOID  _List_Line_Del(PLW_LIST_LINE        plineDel, LW_LIST_LINE_HEADER  *pplineHeader);

/*********************************************************************************************************
  �����
*********************************************************************************************************/

VOID  _Tree_Rb_Insert_Color(PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr);
VOID  _Tree_Rb_Erase(PLW_TREE_RB_NODE  ptrbn, PLW_TREE_RB_ROOT  ptrbr);

/*********************************************************************************************************
  ϵͳ�����߳�
*********************************************************************************************************/

PVOID _IdleThread(PVOID    pvArg);

/*********************************************************************************************************
  ��ʱ���߳�
*********************************************************************************************************/

PVOID _ITimerThread(PVOID  pvArg);
VOID  _ITimerWakeup(VOID);

/*********************************************************************************************************
  ��ǰ������Ϣ��ȡ
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define LW_NONSCHED_MODE_PROC(code)                     \
        do {                                            \
            INTREG  iregInterLevel = KN_INT_DISABLE();  \
            { code }                                    \
            KN_INT_ENABLE(iregInterLevel);              \
        } while (0)
#else
#define LW_NONSCHED_MODE_PROC(code)             { code }
#endif                                                                  /*  LW_CFG_SMP_EN               */

#define LW_TCB_GET_CUR(ptcb)                    (ptcb = LW_CPU_GET_CUR()->CPU_ptcbTCBCur)
#define LW_TCB_GET_CUR_SAFE(ptcb)               LW_NONSCHED_MODE_PROC(LW_TCB_GET_CUR(ptcb);)

#define LW_TCB_GET_HIGH(ptcb)                   (ptcb = LW_CPU_GET_CUR()->CPU_ptcbTCBHigh)
#define LW_TCB_GET_HIGH_SAFE(ptcb)              LW_NONSCHED_MODE_PROC(LW_TCB_GET_HIGH(ptcb);)

#define LW_CRCB_GET_CUR(pcrcb)                  (pcrcb = LW_CPU_GET_CUR()->CPU_pcrcbCur)
#define LW_CRCB_GET_CUR_SAFE(ptcb)              LW_NONSCHED_MODE_PROC(LW_CRCB_GET_CUR(pcrcb);)

#define LW_CRCB_GET_NEXT(pcrcb)                 (pcrcb = LW_CPU_GET_CUR()->CPU_pcrcbNext)
#define LW_CRCB_GET_NEXT_SAFE(ptcb)             LW_NONSCHED_MODE_PROC(LW_CRCB_GET_NEXT(pcrcb);)

/*********************************************************************************************************
  �߳������ʲ��� (__LW_TICK_CPUUSAGE_UPDATE ֻ�����ж��������б�����)
*********************************************************************************************************/

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0

#define __LW_TICK_CPUUSAGE_ENABLE()             { _K_ulCPUUsageEnable = LW_TRUE;  }
#define __LW_TICK_CPUUSAGE_DISABLE()            { _K_ulCPUUsageEnable = LW_FALSE; }
#define __LW_TICK_CPUUSAGE_ISENABLE()           ( _K_ulCPUUsageEnable )

#define __LW_TICK_CPUUSAGE_UPDATE(ptcb, pcpu)           \
        do {                                            \
            if (__LW_TICK_CPUUSAGE_ISENABLE()) {        \
                _K_ulCPUUsageTicks++;                   \
                ptcb->TCB_ulCPUUsageTicks++;            \
                if (pcpu->CPU_iKernelCounter) {         \
                    _K_ulCPUUsageKernelTicks++;         \
                    ptcb->TCB_ulCPUUsageKernelTicks++;  \
                }                                       \
            }                                           \
        } while (0)
#else
#define __LW_TICK_CPUUSAGE_UPDATE(ptcb, pcpu)
#endif                                                                  /*  LW_CFG_THREAD_CPU_USAGE_... */

/*********************************************************************************************************
  �ں���������
*********************************************************************************************************/

VOID                __kernelEnter(CPCHAR  pcFunc);
INT                 __kernelExit(VOID);
BOOL                __kernelIsEnter(VOID);

#if LW_CFG_SMP_EN > 0
BOOL                __kernelIsLockByMe(BOOL  bIntLock);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

INTREG              __kernelEnterIrq(CPCHAR  pcFunc);
INT                 __kernelExitIrq(INTREG  iregInterLevel);

#define __KERNEL_ENTER()                    __kernelEnter(__func__)
#define __KERNEL_EXIT()                     __kernelExit()
#define __KERNEL_ISENTER()                  __kernelIsEnter()

#if LW_CFG_SMP_EN > 0
#define __KERNEL_ISLOCKBYME(bIntLock)       __kernelIsLockByMe(bIntLock)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#define __KERNEL_ENTER_IRQ()                __kernelEnterIrq(__func__)
#define __KERNEL_EXIT_IRQ(iregInterLevel)   __kernelExitIrq(iregInterLevel)

LW_OBJECT_HANDLE    __kernelOwner(VOID);
CPCHAR              __kernelEnterFunc(VOID);

#define __KERNEL_OWNER()            __kernelOwner()
#define __KERNEL_ENTERFUNC()        __kernelEnterFunc()

#define __KERNEL_MODE_PROC(code)    do {                            \
                                        __kernelEnter(__func__);    \
                                        {   code    }               \
                                        __kernelExit();             \
                                    } while (0)
                                            
/*********************************************************************************************************
  �ں˵���
*********************************************************************************************************/

INT                 __kernelSched(VOID);
VOID                __kernelSchedInt(PLW_CLASS_CPU  pcpuCur);

#define __KERNEL_SCHED()                __kernelSched()
#define __KERNEL_SCHED_INT(pcpu)        __kernelSchedInt(pcpu)

#if defined(__SYLIXOS_ARM_ARCH_M__)

BOOL                __kernelSchedIntCheck(PLW_CLASS_CPU  pcpuCur);

#define __KERNEL_SCHED_INT_CHECK(pcpu)  __kernelSchedIntCheck(pcpu)

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
                                            
/*********************************************************************************************************
  �ں� ticks
*********************************************************************************************************/

#if LW_CFG_CPU_ATOMIC64_EN > 0
#define __KERNEL_TIME_GET_NO_SPINLOCK(time, type)                   \
        {                                                           \
            time = (type)__LW_ATOMIC64_GET(&_K_atomic64KernelTime); \
        }
#define __KERNEL_TIME_GET(time, type)   \
        __KERNEL_TIME_GET_NO_SPINLOCK(time, type)

#define __KERNEL_TIME_GET_IGNIRQ(time, type)    \
        __KERNEL_TIME_GET_NO_SPINLOCK(time, type)

#else                                                                   /*  LW_CFG_CPU_ATOMIC64_EN      */
#define __KERNEL_TIME_GET_NO_SPINLOCK(time, type)       \
        {                                               \
            time = (type)_K_atomic64KernelTime.counter; \
        }
#define __KERNEL_TIME_GET(time, type)                       \
        {                                                   \
            INTREG  iregInterLevel;                         \
            LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);  \
            time = (type)_K_atomic64KernelTime.counter;     \
            LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel); \
        }

#define __KERNEL_TIME_GET_IGNIRQ(time, type)                \
        {                                                   \
            LW_SPIN_KERN_TIME_LOCK_IGNIRQ();                \
            time = (type)_K_atomic64KernelTime.counter;     \
            LW_SPIN_KERN_TIME_UNLOCK_IGNIRQ();              \
        }
#endif                                                                  /*  !LW_CFG_CPU_ATOMIC64_EN     */

/*********************************************************************************************************
  �ں˿ռ����
*********************************************************************************************************/

VOID  __kernelSpaceEnter(VOID);
VOID  __kernelSpaceExit(VOID);
INT   __kernelSpaceGet(VOID);
INT   __kernelSpaceGet2(PLW_CLASS_TCB  ptcb);
VOID  __kernelSpaceSet(INT  iNesting);
VOID  __kernelSpaceSet2(PLW_CLASS_TCB  ptcb, INT  iNesting);

#define __KERNEL_SPACE_ENTER()              __kernelSpaceEnter()
#define __KERNEL_SPACE_EXIT()               __kernelSpaceExit()
#define __KERNEL_SPACE_ISENTER()            __kernelSpaceGet()
#define __KERNEL_SPACE_GET()                __kernelSpaceGet()
#define __KERNEL_SPACE_GET2(ptcb)           __kernelSpaceGet2(ptcb)
#define __KERNEL_SPACE_SET(iNesting)        __kernelSpaceSet(iNesting)
#define __KERNEL_SPACE_SET2(ptcb, iNesting) __kernelSpaceSet2(ptcb, iNesting)

/*********************************************************************************************************
  ������ (��������ǰû�����еĻ���)
*********************************************************************************************************/

VOID  _AddTCBToReadyRing(PLW_CLASS_TCB    ptcb, PLW_CLASS_PCB  ppcb, BOOL  bIsHeader);
VOID  _DelTCBFromReadyRing(PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb);

/*********************************************************************************************************
  ���Ѷ���
*********************************************************************************************************/

VOID  _WakeupAdd(PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime);
VOID  _WakeupDel(PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime);
VOID  _WakeupStatus(PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, ULONG  *pulLeft);

/*********************************************************************************************************
  ���Ź����ӳٻ��ѷ���
*********************************************************************************************************/

VOID  _ThreadTick(VOID);
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
VOID  _WatchDogTick(VOID);
#endif

/*********************************************************************************************************
  �¼��ȴ�����
*********************************************************************************************************/

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

VOID  _AddTCBToEventFifo(PLW_CLASS_TCB       ptcb, PLW_CLASS_EVENT pevent, PLW_LIST_RING *ppringList);
VOID  _AddTCBToEventPriority(PLW_CLASS_TCB   ptcb, PLW_CLASS_EVENT pevent, PLW_LIST_RING *ppringList);
VOID  _DelTCBFromEventFifo(PLW_CLASS_TCB     ptcb, PLW_CLASS_EVENT pevent, PLW_LIST_RING *ppringList);
VOID  _DelTCBFromEventPriority(PLW_CLASS_TCB ptcb, PLW_CLASS_EVENT pevent, PLW_LIST_RING *ppringList);

/*********************************************************************************************************
  �¼��ȴ�����
*********************************************************************************************************/

VOID  _EventWaitFifo(PLW_CLASS_EVENT         pevent, PLW_LIST_RING  *ppringList);
VOID  _EventWaitPriority(PLW_CLASS_EVENT     pevent, PLW_LIST_RING  *ppringList);

/*********************************************************************************************************
  �����ź������ȼ��̳�
*********************************************************************************************************/

VOID  _EventPrioTryBoost(PLW_CLASS_EVENT   pevent, PLW_CLASS_TCB   ptcbCur);
VOID  _EventPrioTryResume(PLW_CLASS_EVENT  pevent, PLW_CLASS_TCB   ptcbCur);

/*********************************************************************************************************
  �¼�����
*********************************************************************************************************/

PLW_CLASS_TCB    _EventReadyFifoLowLevel(PLW_CLASS_EVENT   pevent, 
                                         PVOID             pvMsgBoxMessage, 
                                         PLW_LIST_RING    *ppringList);
PLW_CLASS_TCB    _EventReadyPriorityLowLevel(PLW_CLASS_EVENT   pevent, 
                                             PVOID             pvMsgBoxMessage, 
                                             PLW_LIST_RING    *ppringList);

VOID             _EventReadyHighLevel(PLW_CLASS_TCB  ptcb, UINT16  usWaitType);

PLW_CLASS_EVENT  _EventUnQueue(PLW_CLASS_TCB  ptcb);

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  ��Ϣ����
*********************************************************************************************************/

#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
VOID           _MsgQueueClear(PLW_CLASS_MSGQUEUE    pmsgqueue,
                              ULONG                 ulMsgTotal);
VOID           _MsgQueueGet(PLW_CLASS_MSGQUEUE    pmsgqueue,
                            PVOID                 pvMsgBuffer,
                            size_t                stMaxByteSize,
                            size_t               *pstMsgLen);
VOID           _MsgQueuePut(PLW_CLASS_MSGQUEUE    pmsgqueue,
                            PVOID                 pvMsgBuffer,
                            size_t                stMsgLen, 
                            UINT                  uiPrio);
VOID           _MsgQueueMsgLen(PLW_CLASS_MSGQUEUE  pmsgqueue, size_t  *pstMsgLen);
#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  �������
*********************************************************************************************************/

LW_OBJECT_ID   _MakeObjectId(UINT8  ucCls, UINT16  usNode, UINT16  usIndex);

/*********************************************************************************************************
  CPU
*********************************************************************************************************/

INT            _CpuActive(PLW_CLASS_CPU   pcpu);

#if LW_CFG_SMP_CPU_DOWN_EN > 0
INT            _CpuInactive(PLW_CLASS_CPU   pcpu);
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

#if LW_CFG_SMP_EN > 0
VOID           _CpuSetSchedAffinity(ULONG  ulCPUId, BOOL  bEnable);
VOID           _CpuGetSchedAffinity(ULONG  ulCPUId, BOOL  *pbEnable);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

VOID           _CpuBogoMipsClear(ULONG  ulCPUId);

/*********************************************************************************************************
  ������
*********************************************************************************************************/

BOOL           _SchedIsLock(ULONG  ulCurMaxLock);                       /*  �������Ƿ�����            */
PLW_CLASS_TCB  _SchedGetCand(PLW_CLASS_CPU  pcpuCur, ULONG  ulCurMaxLock);
VOID           _SchedTick(VOID);

#if LW_CFG_THREAD_SCHED_YIELD_EN > 0
VOID           _SchedYield(PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb);
#endif

VOID           _SchedSwp(PLW_CLASS_CPU pcpuCur);                        /*  �����л��ص�                */
#if LW_CFG_COROUTINE_EN > 0
VOID           _SchedCrSwp(PLW_CLASS_CPU pcpuCur);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

#if LW_CFG_SMP_EN > 0
PVOID          _SchedSafeStack(PLW_CLASS_CPU pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

INT            _Schedule(VOID);
VOID           _ScheduleInt(PLW_CLASS_CPU  pcpuCur);
VOID           _ScheduleInit(VOID);

#if defined(__SYLIXOS_ARM_ARCH_M__)
BOOL           _ScheduleIntCheck(PLW_CLASS_CPU  pcpuCur);
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */

VOID           _SchedSetRet(INT  iSchedSetRet);
VOID           _SchedSetPrio(PLW_CLASS_TCB  ptcb, UINT8  ucPriority);   /*  �������ȼ�                  */

/*********************************************************************************************************
  SMP �˼��ж�
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#if LW_CFG_SYSPERF_EN > 0
VOIDFUNCPTR    _SmpPerfIpi(VOIDFUNCPTR  pfuncHook);
#endif                                                                  /* LW_CFG_SYSPERF_EN > 0        */
VOID           _SmpSendIpi(ULONG  ulCPUId, ULONG  ulIPIVec, INT  iWait, BOOL  bIntLock);
VOID           _SmpSendIpiAllOther(ULONG  ulIPIVec, INT  iWait);
INT            _SmpCallFunc(ULONG  ulCPUId, FUNCPTR  pfunc, PVOID  pvArg,
                            VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);
VOID           _SmpCallFuncAllOther(FUNCPTR  pfunc, PVOID  pvArg,
                                    VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);
VOID           _SmpProcIpi(PLW_CLASS_CPU  pcpuCur);
VOID           _SmpTryProcIpi(PLW_CLASS_CPU  pcpuCur);
VOID           _SmpUpdateIpi(PLW_CLASS_CPU  pcpu);
#endif                                                                  /* LW_CFG_SMP_EN                */

/*********************************************************************************************************
  �߳����
*********************************************************************************************************/

PVOID          _ThreadShell(PVOID  pvThreadStartAddress);

/*********************************************************************************************************
  Э�����
*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
VOID           _CoroutineShell(PVOID   pvCoroutineStartAddress);
VOID           _CoroutineReclaim(PLW_CLASS_TCB  ptcb);
VOID           _CoroutineFreeAll(PLW_CLASS_TCB  ptcb);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

/*********************************************************************************************************
  �߳� JOIN & DETACH
*********************************************************************************************************/

VOID           _ThreadJoin(PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  *ppvRetValSave);
VOID           _ThreadDisjoin(PLW_CLASS_TCB  ptcbDes, PLW_CLASS_TCB  ptcbDisjoin, BOOL  bWakeup, PVOID  pvArg);
INT            _ThreadDetach(PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal);

VOID           _ThreadWjAdd(PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal);
VOID           _ThreadWjClear(PVOID  pvVProc);

/*********************************************************************************************************
  �߳� CPU ����������
*********************************************************************************************************/

VOID            _ThreadLock(VOID);
VOID            _ThreadUnlock(VOID);

#define LW_THREAD_LOCK()            _ThreadLock()
#define LW_THREAD_UNLOCK()          _ThreadUnlock()

/*********************************************************************************************************
  ��ȫģʽ����
*********************************************************************************************************/

VOID           _ThreadSafeSuspend(PLW_CLASS_TCB  ptcbCur);
VOID           _ThreadSafeResume(PLW_CLASS_TCB  ptcb);
VOID           _ThreadSafeInternal(VOID);
VOID           _ThreadSafeInKern(PLW_CLASS_TCB  ptcbDes);
VOID           _ThreadUnsafeInternal(VOID);
VOID           _ThreadUnsafeInternalEx(PLW_CLASS_TCB   ptcbDes);

#define LW_THREAD_SAFE()            _ThreadSafeInternal()
#define LW_THREAD_SAFE_INKERN(ptcb) _ThreadSafeInKern(ptcb)
#define LW_THREAD_UNSAFE()          _ThreadUnsafeInternal()
#define LW_THREAD_UNSAFE_EX(ptcb)   _ThreadUnsafeInternalEx(ptcb)

/*********************************************************************************************************
  ���ȼ��Ƚ�
*********************************************************************************************************/

#define LW_PRIO_IS_EQU(prio1, prio2)            (prio1 == prio2)
#define LW_PRIO_IS_HIGH_OR_EQU(prio1, prio2)    (prio1 <= prio2)
#define LW_PRIO_IS_HIGH(prio1, prio2)           (prio1 <  prio2)

/*********************************************************************************************************
  �߳�ɾ���ڲ�����
*********************************************************************************************************/

VOID           _ThreadDeleteWaitDeath(PLW_CLASS_TCB  ptcbDel);
VOID           _ThreadRestartProcHook(PLW_CLASS_TCB  ptcb);
VOID           _ThreadDeleteProcHook(PLW_CLASS_TCB   ptcb, PVOID  pvRetVal);

/*********************************************************************************************************
  �߳��û���Ϣ
*********************************************************************************************************/

ULONG          _ThreadUserGet(LW_HANDLE  ulId, uid_t  *puid, gid_t  *pgid);

/*********************************************************************************************************
  ��ǰ�̳߳���һ�ε���
*********************************************************************************************************/

INT            _ThreadSched(PLW_CLASS_TCB  ptcbCur);

/*********************************************************************************************************
  �߳����� CPU ����
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
ULONG          _ThreadSetAffinity(PLW_CLASS_TCB  ptcb, size_t stSize, const PLW_CLASS_CPUSET  pcpuset);
VOID           _ThreadGetAffinity(PLW_CLASS_TCB  ptcb, size_t stSize, PLW_CLASS_CPUSET  pcpuset);
VOID           _ThreadOffAffinity(PLW_CLASS_CPU  pcpu);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  ǿ�Ƹı��߳�״̬
*********************************************************************************************************/

ULONG          _ThreadStatusChange(PLW_CLASS_TCB  ptcb, UINT  uiStatusReq);

#if LW_CFG_SMP_EN > 0
VOID           _ThreadStatusChangeCur(PLW_CLASS_CPU  pcpuCur);
VOID           _ThreadUnwaitStatus(PLW_CLASS_TCB  ptcb);                /*  �߳�ɾ��/����ʱ��Ҫ�˳����� */
#endif                                                                  /*  LW_CFG_SMP_EN               */

ULONG          _ThreadStop(PLW_CLASS_TCB  ptcb);
ULONG          _ThreadContinue(PLW_CLASS_TCB  ptcb, BOOL  bForce);

#if LW_CFG_GDB_EN > 0
VOID           _ThreadDebugUnpendSem(PLW_CLASS_TCB  ptcb);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  �߳̽������
*********************************************************************************************************/

#if LW_CFG_MODULELOADER_EN > 0
ULONG          _ThreadMigrateToProc(LW_HANDLE  ulId, PVOID   pvVProc, BOOL  bMain);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

/*********************************************************************************************************
  ���������߳�
*********************************************************************************************************/

VOID           _ThreadTraversal(VOIDFUNCPTR    pfunc, 
                                PVOID          pvArg0,
                                PVOID          pvArg1,
                                PVOID          pvArg2,
                                PVOID          pvArg3,
                                PVOID          pvArg4,
                                PVOID          pvArg5);

/*********************************************************************************************************
  ���� TCB ����չ������
*********************************************************************************************************/

VOID           _TCBBuild(UINT8                    ucPriority,
                         PLW_STACK                pstkStackTop,
                         PLW_STACK                pstkStackButtom,
                         PLW_STACK                pstkStackGuard,
                         PVOID                    pvStackExt,
                         PLW_STACK                pstkStackLowAddr,
                         size_t                   stStackSize,
                         LW_OBJECT_ID             ulId,
                         ULONG                    ulOption,
                         PTHREAD_START_ROUTINE    pThread,
                         PLW_CLASS_TCB            ptcb,
                         PVOID                    pvArg);
VOID           _TCBDestroy(PLW_CLASS_TCB  ptcb);
ULONG          _TCBBuildExt(PLW_CLASS_TCB  ptcb);
VOID           _TCBDestroyExt(PLW_CLASS_TCB  ptcb);

VOID           _TCBCleanupPopExt(PLW_CLASS_TCB  ptcb);
VOID           _TCBTryRun(PLW_CLASS_TCB  ptcb);

/*********************************************************************************************************
  ��ջ���
*********************************************************************************************************/

VOID           _StackCheckGuard(PLW_CLASS_TCB  ptcb);
PLW_STACK      _StackAllocate(PLW_CLASS_TCB  ptcb, ULONG  ulOption, size_t  stSize);
VOID           _StackFree(PLW_CLASS_TCB  ptcb, PLW_STACK  pstk);

/*********************************************************************************************************
  �߳�˽�б����л�
*********************************************************************************************************/

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
VOID           _ThreadVarDelete(PLW_CLASS_TCB  ptcb);
VOID           _ThreadVarSwitch(PLW_CLASS_TCB  ptcbOld, PLW_CLASS_TCB  ptcbNew);
#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID           _ThreadVarSave(PLW_CLASS_TCB  ptcbCur);
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB...   */
/*********************************************************************************************************
  �߳� FPU ���
*********************************************************************************************************/

#if LW_CFG_CPU_FPU_EN > 0
VOID           _ThreadFpuSwitch(BOOL bIntSwitch);
#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID           _ThreadFpuSave(PLW_CLASS_TCB   ptcbCur, BOOL bIntSwitch);
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

/*********************************************************************************************************
  �߳� DSP ���
*********************************************************************************************************/

#if LW_CFG_CPU_DSP_EN > 0
VOID           _ThreadDspSwitch(BOOL bIntSwitch);
#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID           _ThreadDspSave(PLW_CLASS_TCB   ptcbCur, BOOL bIntSwitch);
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

/*********************************************************************************************************
  �¼����ڲ�����
*********************************************************************************************************/

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
VOID           _EventSetUnQueue(PLW_CLASS_EVENTSETNODE  pesn);
VOID           _EventSetBlock(PLW_CLASS_EVENTSET        pes,
                              PLW_CLASS_EVENTSETNODE    pesn,
                              ULONG                     ulEvents,
                              UINT8                     ucWaitType,
                              ULONG                     ulWaitTime);
BOOL           _EventSetThreadReady(PLW_CLASS_EVENTSETNODE    pesn,
                                    ULONG                     ulEventsReady);
BOOL           _EventSetDeleteReady(PLW_CLASS_EVENTSETNODE    pesn);
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  ����
*********************************************************************************************************/

#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
PVOID          _PartitionAllocate(PLW_CLASS_PARTITION  p_part);
VOID           _PartitionFree(PLW_CLASS_PARTITION  p_part, PVOID  pvFree);
#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  ���ڲ�����
*********************************************************************************************************/

VOID           _HeapTraceAlloc(PLW_CLASS_HEAP  pheap, PVOID  pvMem, size_t  stByteSize, CPCHAR  cpcPurpose);
VOID           _HeapTraceFree(PLW_CLASS_HEAP  pheap, PVOID  pvMem);

PLW_CLASS_HEAP _HeapCtor(PLW_CLASS_HEAP    pheapToBuild,
                         PVOID             pvStartAddress, 
                         size_t            stByteSize);
PLW_CLASS_HEAP _HeapCtorEx(PLW_CLASS_HEAP    pheapToBuild,
                           PVOID             pvStartAddress, 
                           size_t            stByteSize, 
                           BOOL              bIsMosHeap);
PLW_CLASS_HEAP _HeapDtor(PLW_CLASS_HEAP  pheap, BOOL  bIsCheckUsed);
PLW_CLASS_HEAP _HeapCreate(PVOID               pvStartAddress, 
                           size_t              stByteSize);
PLW_CLASS_HEAP _HeapDelete(PLW_CLASS_HEAP     pheap, BOOL  bIsCheckUsed);
ULONG          _HeapAddMemory(PLW_CLASS_HEAP  pheap, PVOID pvMemory, size_t  stSize);

PVOID          _HeapAllocate(PLW_CLASS_HEAP   pheap, size_t  stByteSize, CPCHAR  pcPurpose);
PVOID          _HeapAllocateAlign(PLW_CLASS_HEAP   pheap, size_t  stByteSize, 
                                  size_t  stAlign, CPCHAR  pcPurpose);
PVOID          _HeapZallocate(PLW_CLASS_HEAP  pheap, size_t  stByteSize, CPCHAR  pcPurpose);
PVOID          _HeapFree(PLW_CLASS_HEAP       pheap, PVOID pvStartAddress, 
                         BOOL bIsNeedVerify, CPCHAR  pcPurpose);
ULONG          _HeapGetInfo(PLW_CLASS_HEAP    pheap, PLW_CLASS_SEGMENT  psegmentList[], INT iMaxCounter);
size_t         _HeapGetMax(PLW_CLASS_HEAP  pheap);
PVOID          _HeapRealloc(PLW_CLASS_HEAP    pheap, 
                            PVOID             pvStartAddress, 
                            size_t            stNewByteSize,
                            BOOL              bIsNeedVerify,
                            CPCHAR            pcPurpose);
                            
/*********************************************************************************************************
  RMS
*********************************************************************************************************/

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
VOID           _RmsActive(PLW_CLASS_RMS  prms);
ULONG          _RmsGetExecTime(PLW_CLASS_RMS  prms);
ULONG          _RmsInitExpire(PLW_CLASS_RMS  prms, ULONG  ulPeriod, ULONG  *pulWaitTick);
ULONG          _RmsEndExpire(PLW_CLASS_RMS  prms);
#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  SIGNAL
*********************************************************************************************************/

#if LW_CFG_SIGNAL_EN > 0
ULONG          _sigTimeoutRecalc(ULONG  ulOrgKernelTime, 
                                 ULONG  ulOrgTimeout);
INT            _doSigEvent(LW_OBJECT_HANDLE  ulId, 
                           struct sigevent  *psigevent, 
                           INT               iSigCode);
INT            _doSigEventEx(LW_OBJECT_HANDLE  ulId, 
                             struct sigevent  *psigevent, 
                             struct siginfo   *psiginfo);
#else
#define _sigTimeoutRecalc(a, b) LW_OPTION_NOT_WAIT
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

/*********************************************************************************************************
  KERNEL JOB QUEUE
*********************************************************************************************************/

#define LW_KERNEL_JOB_INIT()    \
        _jobQueueInit(&_K_jobqKernel, &_K_jobmsgKernel[0], LW_CFG_MAX_EXCEMSGS, LW_TRUE)

#define LW_KERNEL_JOB_ADD(pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)  \
        _jobQueueAdd(&_K_jobqKernel, pfunc, \
                     pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)
        
#define LW_KERNEL_JOB_DEL(uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5) \
        _jobQueueDel(&_K_jobqKernel, uiMatchArgNum, pfunc, \
                     pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)
                     
#define LW_KERNEL_JOB_LOST()    \
        _jobQueueLost(&_K_jobqKernel)
                     
#define LW_KERNEL_JOB_EXEC()    \
        _jobQueueExec(&_K_jobqKernel, LW_OPTION_NOT_WAIT)

/*********************************************************************************************************
  KERNEL HOOK
*********************************************************************************************************/

#define __LW_THREAD_CREATE_HOOK(ulId, ulOption) \
        if (_K_hookKernel.HOOK_ThreadCreate) {  \
            _K_hookKernel.HOOK_ThreadCreate(ulId, ulOption);    \
        }
        
#define __LW_THREAD_DELETE_HOOK(ulId, pvRetVal, ptcbDel) \
        if (_K_hookKernel.HOOK_ThreadDelete) {  \
            _K_hookKernel.HOOK_ThreadDelete(ulId, pvRetVal, ptcbDel);   \
        }
        
#define __LW_THREAD_SWAP_HOOK(hOldThread, hNewThread) \
        if (_K_hookKernel.HOOK_ThreadSwap) {    \
            _K_hookKernel.HOOK_ThreadSwap(hOldThread, hNewThread);  \
        }
        
#define __LW_THREAD_TICK_HOOK(i64Tick) \
        if (_K_hookKernel.HOOK_ThreadTick) {    \
            _K_hookKernel.HOOK_ThreadTick(i64Tick); \
        }
        
#define __LW_THREAD_INIT_HOOK(ulId, ptcb) \
        if (_K_hookKernel.HOOK_ThreadInit) {    \
            _K_hookKernel.HOOK_ThreadInit(ulId, ptcb);  \
        }
        
#define __LW_THREAD_IDLE_HOOK(ulCPUId) \
        if (_K_hookKernel.HOOK_ThreadIdle) {    \
            _K_hookKernel.HOOK_ThreadIdle(ulCPUId); \
        }
        
#define __LW_KERNEL_INIT_BEGIN_HOOK() \
        if (_K_hookKernel.HOOK_KernelInitBegin) {   \
            _K_hookKernel.HOOK_KernelInitBegin();   \
        }
        
#define __LW_KERNEL_INIT_END_HOOK(iError) \
        if (_K_hookKernel.HOOK_KernelInitEnd) { \
            _K_hookKernel.HOOK_KernelInitEnd(iError);   \
        }
        
#define __LW_KERNEL_REBOOT_HOOK(iRebootType)    \
        if (_K_hookKernel.HOOK_KernelReboot) {  \
            _K_hookKernel.HOOK_KernelReboot(iRebootType);   \
        }
        
#define __LW_WATCHDOG_TIMER_HOOK(ulId)  \
        if (_K_hookKernel.HOOK_WatchDogTimer) {  \
            _K_hookKernel.HOOK_WatchDogTimer(ulId); \
        }
        
/*********************************************************************************************************
  OBJECT & FILE DESCRIPTOR HOOK
*********************************************************************************************************/

#define __LW_OBJECT_CREATE_HOOK(ulId, ulOption)   \
        if (_K_hookKernel.HOOK_ObjectCreate) {  \
            _K_hookKernel.HOOK_ObjectCreate(ulId, ulOption);  \
        }
        
#define __LW_OBJECT_DELETE_HOOK(ulId)   \
        if (_K_hookKernel.HOOK_ObjectDelete) {  \
            _K_hookKernel.HOOK_ObjectDelete(ulId);  \
        }
        
#define __LW_FD_CREATE_HOOK(iFd, pid)   \
        if (_K_hookKernel.HOOK_FdCreate) {  \
            _K_hookKernel.HOOK_FdCreate(iFd, pid);  \
        }
        
#define __LW_FD_DELETE_HOOK(iFd, pid)   \
        if (_K_hookKernel.HOOK_FdDelete) {  \
            _K_hookKernel.HOOK_FdDelete(iFd, pid);  \
        }
        
/*********************************************************************************************************
  ������ CPU ���Ĺ������� HOOK
*********************************************************************************************************/

#define __LW_CPU_IDLE_ENTER_HOOK(ulIdEnterFrom) \
        if (_K_hookKernel.HOOK_CpuIdleEnter) {  \
            _K_hookKernel.HOOK_CpuIdleEnter(ulIdEnterFrom); \
        }
        
#define __LW_CPU_IDLE_EXIT_HOOK(ulIdExitTo) \
        if (_K_hookKernel.HOOK_CpuIdleExit) {  \
            _K_hookKernel.HOOK_CpuIdleExit(ulIdExitTo); \
        }
        
#define __LW_CPU_INT_ENTER_HOOK(ulVector, ulNesting)    \
        if (_K_hookKernel.HOOK_CpuIntEnter) {   \
            _K_hookKernel.HOOK_CpuIntEnter(ulVector, ulNesting);   \
        }

#define __LW_CPU_INT_EXIT_HOOK(ulVector, ulNesting)     \
        if (_K_hookKernel.HOOK_CpuIntExit) {    \
            _K_hookKernel.HOOK_CpuIntExit(ulVector, ulNesting); \
        }

/*********************************************************************************************************
  ϵͳ������� HOOK
*********************************************************************************************************/

#define __LW_STACK_OVERFLOW_HOOK(pid, ulId) \
        if (_K_hookKernel.HOOK_StkOverflow) {  \
            _K_hookKernel.HOOK_StkOverflow(pid, ulId); \
        }
        
#define __LW_FATAL_ERROR_HOOK(pid, ulId, psiginfo) \
        if (_K_hookKernel.HOOK_FatalError) {  \
            _K_hookKernel.HOOK_FatalError(pid, ulId, psiginfo); \
        }

/*********************************************************************************************************
  ���̻ص�
*********************************************************************************************************/

#define __LW_VPROC_CREATE_HOOK(pid) \
        if (_K_hookKernel.HOOK_VpCreate) {  \
            _K_hookKernel.HOOK_VpCreate(pid);    \
        }
        
#define __LW_VPROC_DELETE_HOOK(pid, iExitCode) \
        if (_K_hookKernel.HOOK_VpDelete) {  \
            _K_hookKernel.HOOK_VpDelete(pid, iExitCode);   \
        }

#endif                                                                  /*  __K_INTERNAL_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
