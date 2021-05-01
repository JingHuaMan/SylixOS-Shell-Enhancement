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
** ��   ��   ��: k_api.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�ڲ� API ���������ļ�

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
2008.01.16  ������ API_ThreadForceDelete() ����.
2008.01.16  ������ API_ThreadIsSafe() ����.
2008.01.20  API_ThreadIsLock() �޸��˷�������.
*********************************************************************************************************/

#ifndef __K_API_H
#define __K_API_H

/*********************************************************************************************************
  uid gid
*********************************************************************************************************/

LW_API gid_t            getgid(void);
LW_API int              setgid(gid_t  gid);
LW_API gid_t            getegid(void);
LW_API int              setegid(gid_t  egid);
LW_API uid_t            getuid(void);
LW_API int              setuid(uid_t  uid);
LW_API uid_t            geteuid(void);
LW_API int              seteuid(uid_t  euid);

LW_API int              setgroups(int groupsun, const gid_t grlist[]);
LW_API int              getgroups(int groupsize, gid_t grlist[]);

LW_API int              getlogin_r(char *name, size_t namesize);
LW_API char            *getlogin(void);

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

LW_API INT              API_AtomicAdd(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicSub(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicInc(atomic_t  *patomic);

LW_API INT              API_AtomicDec(atomic_t  *patomic);

LW_API INT              API_AtomicAnd(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicNand(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicOr(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicXor(INT  iVal, atomic_t  *patomic);

LW_API VOID             API_AtomicSet(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicGet(atomic_t  *patomic);

LW_API INT              API_AtomicSwp(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicCas(atomic_t  *patomic, INT  iOldVal, INT  iNewVal);

LW_API INT64            API_Atomic64Add(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Sub(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Inc(atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Dec(atomic64_t  *patomic64);

LW_API INT64            API_Atomic64And(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Nand(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Or(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Xor(INT64  i64Val, atomic64_t  *patomic64);

LW_API VOID             API_Atomic64Set(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Get(atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Swp(INT64  i64Val, atomic64_t  *patomic64);

LW_API INT64            API_Atomic64Cas(atomic64_t  *patomic64, INT64  i64OldVal, INT64  i64NewVal);

/*********************************************************************************************************
  CPU
*********************************************************************************************************/

LW_API ULONG            API_CpuNum(VOID);                               /*  ��� CPU ����               */

LW_API ULONG            API_CpuUpNum(VOID);                             /*  ��������� CPU ����         */

LW_API ULONG            API_CpuCurId(VOID);                             /*  ��õ�ǰ CPU ID             */

LW_API ULONG            API_CpuPhyId(ULONG  ulCPUId, ULONG  *pulPhyId); /*  �߼� CPU ID to ���� CPU ID  */

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_SMP_EN > 0
LW_API ULONG            API_CpuUp(ULONG  ulCPUId);                      /*  ����ָ���� CPU              */

#if LW_CFG_SMP_CPU_DOWN_EN > 0
LW_API ULONG            API_CpuDown(ULONG  ulCPUId);                    /*  ָֹͣ���� CPU              */
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

LW_API BOOL             API_CpuIsUp(ULONG  ulCPUId);                    /*  �鿴ָ�� CPU �Ƿ��Ѿ������� */

LW_API BOOL             API_CpuIsRunning(ULONG  ulCPUId);               /*  �鿴ָ�� CPU �Ƿ��Ѿ�����   */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#if LW_CFG_POWERM_EN > 0
LW_API ULONG            API_CpuPowerSet(UINT  uiPowerLevel);            /*  ���� CPU Ч�ܵȼ�           */

LW_API ULONG            API_CpuPowerGet(UINT  *puiPowerLevel);          /*  ��ȡ CPU Ч�ܵȼ�           */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API ULONG            API_CpuBogoMips(ULONG  ulCPUId, ULONG  *pulKInsPerSec);
                                                                        /*  ����ָ�� CPU BogoMIPS ����  */
#if LW_CFG_SMP_EN > 0
LW_API ULONG            API_CpuSetSchedAffinity(size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
                                                                        /*  �������ȡ CPU ǿ�׺Ͷȵ��� */
LW_API ULONG            API_CpuGetSchedAffinity(size_t  stSize, PLW_CLASS_CPUSET  pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  SPINLOCK (�� API �����ں˳���ʹ��, �ڱ��������ڲ���������ں� API ����������ȱҳ�ж�)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_SpinRestrict(VOID);

LW_API INT              API_SpinInit(spinlock_t *psl);

LW_API INT              API_SpinDestory(spinlock_t *psl);

LW_API INT              API_SpinLock(spinlock_t *psl);

LW_API INT              API_SpinLockIrq(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinLockQuick(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinTryLock(spinlock_t *psl);

LW_API INT              API_SpinTryLockIrq(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinUnlock(spinlock_t *psl);

LW_API INT              API_SpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);

LW_API INT              API_SpinUnlockQuick(spinlock_t *psl, INTREG  iregInterLevel);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  OBJECT
*********************************************************************************************************/

LW_API UINT8            API_ObjectGetClass(LW_OBJECT_HANDLE  ulId);     /*  ��ö�������                */

LW_API BOOL             API_ObjectIsGlobal(LW_OBJECT_HANDLE  ulId);     /*  ��ö����Ƿ�Ϊȫ�ֶ���      */

LW_API ULONG            API_ObjectGetNode(LW_OBJECT_HANDLE  ulId);      /*  ��ö������ڵĴ�������      */

LW_API ULONG            API_ObjectGetIndex(LW_OBJECT_HANDLE  ulId);     /*  ��ö��󻺳����ڵ�ַ        */

#if LW_CFG_OBJECT_SHARE_EN > 0
LW_API ULONG            API_ObjectShareAdd(LW_OBJECT_HANDLE  ulId, UINT64  u64Key);

LW_API ULONG            API_ObjectShareDelete(UINT64  u64Key);

LW_API LW_OBJECT_HANDLE API_ObjectShareFind(UINT64  u64Key);
#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */

/*********************************************************************************************************
  THREAD
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
LW_API ULONG            API_ThreadSetAffinity(LW_OBJECT_HANDLE        ulId, 
                                              size_t                  stSize, 
                                              const PLW_CLASS_CPUSET  pcpuset);
                                                                        /*  �����߳� CPU �׺Ͷ�         */
LW_API ULONG            API_ThreadGetAffinity(LW_OBJECT_HANDLE        ulId, 
                                              size_t                  stSize, 
                                              PLW_CLASS_CPUSET        pcpuset);
                                                                        /*  ��ȡ�߳� CPU �׺Ͷ�         */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

LW_API  
LW_CLASS_THREADATTR     API_ThreadAttrGetDefault(VOID);                 /*  ���Ĭ���߳����Կ�          */

LW_API  
LW_CLASS_THREADATTR     API_ThreadAttrGet(LW_OBJECT_HANDLE  ulId);      /*  ���ָ���̵߳�ǰ���Կ�      */

LW_API ULONG            API_ThreadAttrBuild(PLW_CLASS_THREADATTR    pthreadattr,
                                            size_t                  stStackByteSize, 
                                            UINT8                   ucPriority, 
                                            ULONG                   ulOption, 
                                            PVOID                   pvArg);
                                                                        /*  ����һ���߳����Կ�          */
                                            
LW_API ULONG            API_ThreadAttrBuildEx(PLW_CLASS_THREADATTR    pthreadattr,
                                              PLW_STACK               pstkStackTop, 
                                              size_t                  stStackByteSize, 
                                              UINT8                   ucPriority, 
                                              ULONG                   ulOption, 
                                              PVOID                   pvArg,
                                              PVOID                   pvExt);
                                                                        /*  ����һ���߼��߳����Կ�      */

LW_API ULONG            API_ThreadAttrBuildFP(PLW_CLASS_THREADATTR    pthreadattr,
                                              PVOID                   pvFP);
                                                                        /*  ���ø���������������        */

LW_API ULONG            API_ThreadAttrBuildDefault(PLW_CLASS_THREADATTR    pthreadattr);
                                                                        /*  ����һ���߳�Ĭ�����Կ�      */
LW_API ULONG            API_ThreadAttrSetGuardSize(PLW_CLASS_THREADATTR    pthreadattr,
                                                   size_t                  stGuardSize);
                                                                        /*  ���ö�ջ��������С          */
LW_API ULONG            API_ThreadAttrSetStackSize(PLW_CLASS_THREADATTR    pthreadattr,
                                                   size_t                  stStackByteSize);
                                                                        /*  ���ö�ջ��С                */
LW_API ULONG            API_ThreadAttrSetArg(PLW_CLASS_THREADATTR    pthreadattr,
                                             PVOID                   pvArg);
                                                                        /*  �����߳���������            */

LW_API LW_OBJECT_HANDLE API_ThreadInit(CPCHAR                   pcName,
                                       PTHREAD_START_ROUTINE    pfuncThread,
                                       PLW_CLASS_THREADATTR     pthreadattr,
                                       LW_OBJECT_ID            *pulId); /*  �̳߳�ʼ��                  */
                                       
LW_API LW_OBJECT_HANDLE API_ThreadCreate(CPCHAR                   pcName,
                                         PTHREAD_START_ROUTINE    pfuncThread,
                                         PLW_CLASS_THREADATTR     pthreadattr,
                                         LW_OBJECT_ID            *pulId);
                                                                        /*  ����һ���߳�                */
#if LW_CFG_THREAD_DEL_EN > 0
LW_API ULONG            API_ThreadExit(PVOID  pvRetVal);                /*  �߳������˳�                */

LW_API int              atexit(void (*func)(void));

LW_API void             exit(int  iCode);

LW_API void             _exit(int  iCode);

LW_API void             _Exit(int  iCode);

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

#if LW_CFG_THREAD_DEL_EN > 0
LW_API ULONG            API_ThreadDelete(LW_OBJECT_HANDLE  *pulId, 
                                         PVOID  pvRetVal);              /*  ɾ��һ���߳�                */

LW_API ULONG            API_ThreadForceDelete(LW_OBJECT_HANDLE  *pulId, 
                                              PVOID pvRetVal);          /*  ǿ��ɾ��һ���߳�            */
#endif

#if LW_CFG_THREAD_RESTART_EN > 0
LW_API ULONG            API_ThreadRestart(LW_OBJECT_HANDLE  ulId, 
                                          PVOID  pvArg);                /*  �߳���������                */
                                          
LW_API ULONG            API_ThreadRestartEx(LW_OBJECT_HANDLE       ulId, 
                                            PTHREAD_START_ROUTINE  pfuncThread, 
                                            PVOID                  pvArg);
#endif

LW_API LW_OBJECT_HANDLE API_ThreadIdSelf(VOID);                         /*  ����߳��Լ��ľ��          */

#ifdef __SYLIXOS_KERNEL
LW_API PLW_CLASS_TCB    API_ThreadTcbSelf(VOID);                        /*  ����߳��Լ��� TCB          */

LW_API LW_OBJECT_HANDLE API_ThreadIdInter(VOID);                        /*  ��ñ��ж��߳� ID           */

LW_API PLW_CLASS_TCB    API_ThreadTcbInter(VOID);                       /*  ��ñ��ж��߳� TCB          */
#endif                                                                  /*  ֻ���ں�ģ����ʹ��          */

LW_API ULONG            API_ThreadDesc(LW_OBJECT_HANDLE     ulId, 
                                       PLW_CLASS_TCB_DESC   ptcbdesc);  /*  ����̸߳�Ҫ��Ϣ            */

LW_API ULONG            API_ThreadStart(LW_OBJECT_HANDLE    ulId);      /*  ����һ���Ѿ���ʼ�����߳�    */

LW_API ULONG            API_ThreadStartEx(LW_OBJECT_HANDLE  ulId, 
                                          BOOL              bJoin, 
                                          PVOID            *ppvRetValAddr);

LW_API ULONG            API_ThreadJoin(LW_OBJECT_HANDLE  ulId, 
                                       PVOID  *ppvRetValAddr);          /*  �̺߳ϲ�                    */

LW_API ULONG            API_ThreadDetach(LW_OBJECT_HANDLE  ulId);       /*  ��ָֹ���̺߳ϲ�            */

LW_API ULONG            API_ThreadDetachEx(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal);

LW_API ULONG            API_ThreadSafe(VOID);                           /*  ���밲ȫģʽ                */

LW_API ULONG            API_ThreadUnsafe(VOID);                         /*  �˳���ȫģʽ                */

LW_API BOOL             API_ThreadIsSafe(LW_OBJECT_HANDLE  ulId);       /*  ���Ŀ���߳��Ƿ�ȫ        */

#if LW_CFG_THREAD_SUSPEND_EN > 0
LW_API ULONG            API_ThreadSuspend(LW_OBJECT_HANDLE    ulId);    /*  �̹߳���                    */

LW_API ULONG            API_ThreadResume(LW_OBJECT_HANDLE    ulId);     /*  �̹߳���ָ�                */
#endif

#if LW_CFG_THREAD_SUSPEND_EN > 0
LW_API ULONG            API_ThreadForceResume(LW_OBJECT_HANDLE   ulId); /*  ǿ�ƻָ�������߳�          */

LW_API ULONG            API_ThreadIsSuspend(LW_OBJECT_HANDLE    ulId);  /*  ����߳��Ƿ񱻹���          */
#endif

LW_API BOOL             API_ThreadIsReady(LW_OBJECT_HANDLE    ulId);    /*  ����߳��Ƿ����            */

LW_API ULONG            API_ThreadIsRunning(LW_OBJECT_HANDLE   ulId, BOOL  *pbIsRunning);
                                                                        /*  ����߳��Ƿ���������        */
LW_API ULONG            API_ThreadSetName(LW_OBJECT_HANDLE  ulId, 
                                          CPCHAR            pcName);    /*  �����߳���                  */

LW_API ULONG            API_ThreadGetName(LW_OBJECT_HANDLE  ulId, 
                                          PCHAR             pcName);    /*  ����߳���                  */

LW_API INT              API_ThreadLock(VOID);                           /*  �߳�����(����������)        */

LW_API INT              API_ThreadUnlock(VOID);                         /*  �߳̽���(����������)        */

LW_API ULONG            API_ThreadIsLock(VOID);                         /*  ����Ƿ�����������          */

#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0
LW_API ULONG            API_ThreadSetPriority(LW_OBJECT_HANDLE  ulId, 
                                              UINT8  ucPriority);       /*  �����߳����ȼ�              */
                                              
LW_API ULONG            API_ThreadGetPriority(LW_OBJECT_HANDLE  ulId, 
                                              UINT8            *pucPriority);
                                                                        /*  ����߳����ȼ�              */
#endif

LW_API ULONG            API_ThreadSetSlice(LW_OBJECT_HANDLE  ulId, 
                                           UINT16            usSlice);  /*  �����߳�ʱ��Ƭ              */

LW_API ULONG            API_ThreadGetSlice(LW_OBJECT_HANDLE  ulId, 
                                           UINT16           *pusSlice); /*  ����߳�ʱ��Ƭ              */

LW_API ULONG            API_ThreadGetSliceEx(LW_OBJECT_HANDLE  ulId, 
                                             UINT16           *pusSlice, 
                                             UINT16           *pusCounter);
                                                                        /*  ��õ�ǰʱ��Ƭ              */

LW_API ULONG            API_ThreadSetSchedParam(LW_OBJECT_HANDLE  ulId,
                                                UINT8             ucPolicy,
                                                UINT8             ucActivatedMode);
                                                                        /*  ���õ���������              */
                                                 
LW_API ULONG            API_ThreadGetSchedParam(LW_OBJECT_HANDLE  ulId,
                                                UINT8            *pucPolicy,
                                                UINT8            *pucActivatedMode);
                                                                        /*  ��õ���������              */

#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)
LW_API ULONG            API_ThreadSetNotePad(LW_OBJECT_HANDLE  ulId,
                                             UINT8             ucNodeIndex,
                                             ULONG             ulVal);  /*  �����̼߳��±�              */
                                             
LW_API ULONG            API_ThreadGetNotePad(LW_OBJECT_HANDLE  ulId,
                                             UINT8             ucNodeIndex);
                                                                        /*  ����̼߳��±�              */
LW_API ULONG            API_ThreadCurNotePad(UINT8  ucNoteIndex);       /*  ��õ�ǰ������±�          */

#if defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0)
LW_API ULONG            API_ThreadFastNotePad(UINT8  ucNoteIndex);
#endif
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
LW_API ULONG            API_ThreadFeedWatchDog(ULONG  ulWatchDogTicks); /*  ι���߳̿��Ź�              */

LW_API VOID             API_ThreadCancelWatchDog(VOID);                 /*  �ر��߳̿��Ź�              */
#endif

LW_API ULONG            API_ThreadStackCheck(LW_OBJECT_HANDLE  ulId,
                                             size_t           *pstFreeByteSize,
                                             size_t           *pstUsedByteSize,
                                             size_t           *pstTcbByteSize);
                                                                        /*  ����̶߳�ջ                */
                                             
LW_API ULONG            API_ThreadGetStackMini(VOID);                   /*  ����߳���С��ɶ�ջ��С    */

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API VOID             API_ThreadCPUUsageOn(VOID);

LW_API VOID             API_ThreadCPUUsageOff(VOID);

LW_API BOOL             API_ThreadCPUUsageIsOn(VOID);

LW_API ULONG            API_ThreadGetCPUUsage(LW_OBJECT_HANDLE  ulId, 
                                              UINT             *puiThreadUsage,
                                              UINT             *puiCPUUsage,
                                              UINT             *puiKernelUsage);
                                                                        /*  ��� CPU ������             */
LW_API INT              API_ThreadGetCPUUsageAll(LW_OBJECT_HANDLE  ulId[], 
                                                 UINT              uiThreadUsage[],
                                                 UINT              uiKernelUsage[],
                                                 INT               iSize);
#endif

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API ULONG            API_ThreadCPUUsageRefresh(VOID);                /*  ˢ�� CPU �����ʲ�������     */
#endif

LW_API ULONG            API_ThreadWakeup(LW_OBJECT_HANDLE    ulId);     /*  ����һ��˯���߳�            */

LW_API ULONG            API_ThreadWakeupEx(LW_OBJECT_HANDLE  ulId, BOOL  bWithInfPend);

#if LW_CFG_THREAD_SCHED_YIELD_EN > 0
LW_API ULONG            API_ThreadYield(LW_OBJECT_HANDLE     ulId);     /*  POSIX �߳� yield ����       */
#endif

#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_ThreadStop(LW_OBJECT_HANDLE  ulId);         /*  �߳�ֹͣ                    */

LW_API ULONG            API_ThreadContinue(LW_OBJECT_HANDLE  ulId);     /*  �ָ�ֹͣ���߳�              */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
LW_API ULONG            API_ThreadVarAdd(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr); /*  ����˽�б���                */

LW_API ULONG            API_ThreadVarDelete(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulAddr); /*  �ͷ�˽�б���                */

LW_API ULONG            API_ThreadVarGet(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr); /*  ���˽�б���                */

LW_API ULONG            API_ThreadVarSet(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr, 
                                         ULONG                ulValue); /*  ����˽�б���                */
                                                                        /*  ����߳�˽�б�����Ϣ        */
LW_API ULONG            API_ThreadVarInfo(LW_OBJECT_HANDLE    ulId, ULONG  *pulAddr[], INT  iMaxCounter);
#endif

LW_API BOOL             API_ThreadVerify(LW_OBJECT_HANDLE  ulId);       /*  ���һ���̵߳� ID �Ƿ���ȷ  */

#if LW_CFG_THREAD_CANCEL_EN > 0
LW_API VOID             API_ThreadTestCancel(VOID);

LW_API ULONG            API_ThreadSetCancelState(INT  iNewState, INT  *piOldState);

LW_API INT              API_ThreadSetCancelType(INT  iNewType, INT  *piOldType);

LW_API ULONG            API_ThreadCancel(LW_OBJECT_HANDLE  *pulId);
#endif

/*********************************************************************************************************
  COROUTINE
*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
LW_API PVOID            API_CoroutineCreate(PCOROUTINE_START_ROUTINE pCoroutineStartAddr,
                                            size_t                   stStackByteSize,
                                            PVOID                    pvArg);
                                                                        /*  ����һ��Э��                */

LW_API ULONG            API_CoroutineDelete(PVOID      pvCrcb);
                                                                        /*  ɾ��һ��Э��(���߳���)      */

LW_API ULONG            API_CoroutineExit(VOID);                        /*  ɾ����ǰЭ��                */

LW_API VOID             API_CoroutineYield(VOID);                       /*  Э����ת����                */

LW_API ULONG            API_CoroutineResume(PVOID  pvCrcb);             /*  �ظ�ָ��Э��                */

LW_API ULONG            API_CoroutineStackCheck(PVOID      pvCrcb,
                                                size_t    *pstFreeByteSize,
                                                size_t    *pstUsedByteSize,
                                                size_t    *pstCrcbByteSize);
                                                                        /*  ���һ��Э�̵Ķ�ջʹ����    */
#endif
/*********************************************************************************************************
  SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMCBM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphorePend(LW_OBJECT_HANDLE  ulId, 
                                          ULONG             ulTimeout); /*  �ȴ������ź���              */

LW_API ULONG            API_SemaphorePost(LW_OBJECT_HANDLE  ulId);      /*  �ͷ������ź���              */

LW_API ULONG            API_SemaphoreFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  �����ȴ������ź������߳�    */
LW_API ULONG            API_SemaphoreDelete(LW_OBJECT_HANDLE  *pulId);  /*  ɾ���ź���                  */
#endif

#if ((LW_CFG_SEMB_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphorePostBPend(LW_OBJECT_HANDLE  ulIdPost, 
                                               LW_OBJECT_HANDLE  ulId,
                                               ULONG             ulTimeout);
                                                                        /*  ԭ���ͷŻ�ȡ��ֵ�ź���      */
LW_API ULONG            API_SemaphorePostCPend(LW_OBJECT_HANDLE  ulIdPost, 
                                               LW_OBJECT_HANDLE  ulId,
                                               ULONG             ulTimeout);
                                                                        /*  ԭ���ͷŻ�ȡ�������ź���    */
#endif

/*********************************************************************************************************
  COUNTING SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphoreGetName(LW_OBJECT_HANDLE  ulId, 
                                             PCHAR             pcName); /*  ����ź���������            */
#endif

#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreCCreate(CPCHAR             pcName,
                                             ULONG              ulInitCounter, 
                                             ULONG              ulMaxCounter,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  �����������ź���            */

LW_API ULONG            API_SemaphoreCDelete(LW_OBJECT_HANDLE  *pulId); /*  ɾ���������ź���            */

LW_API ULONG            API_SemaphoreCPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  �ȴ��������ź���            */

LW_API ULONG            API_SemaphoreCTryPend(LW_OBJECT_HANDLE  ulId);  /*  �������ȴ��������ź���      */

LW_API ULONG            API_SemaphoreCRelease(LW_OBJECT_HANDLE  ulId,
                                              ULONG             ulReleaseCounter, 
                                              ULONG            *pulPreviousCounter);
                                                                        /*  WIN32 �ͷ��ź���            */ 
LW_API ULONG            API_SemaphoreCPost(LW_OBJECT_HANDLE  ulId);     /*  RT �ͷ��ź���               */

LW_API ULONG            API_SemaphoreCFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  �������еȴ��߳�            */
LW_API ULONG            API_SemaphoreCClear(LW_OBJECT_HANDLE  ulId);    /*  ����ź����ź�              */

LW_API ULONG            API_SemaphoreCStatus(LW_OBJECT_HANDLE   ulId,
                                             ULONG             *pulCounter,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  ���������ź���״̬        */
LW_API ULONG            API_SemaphoreCStatusEx(LW_OBJECT_HANDLE   ulId,
                                               ULONG             *pulCounter,
                                               ULONG             *pulOption,
                                               ULONG             *pulThreadBlockNum,
                                               ULONG             *pulMaxCounter);
                                                                        /*  ���������ź���״̬        */
#endif

/*********************************************************************************************************
  BINARY SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreBCreate(CPCHAR             pcName,
                                             BOOL               bInitValue,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  �����������ź���            */
                                             
LW_API ULONG            API_SemaphoreBDelete(LW_OBJECT_HANDLE  *pulId); /*  ɾ���������ź���            */

LW_API ULONG            API_SemaphoreBPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  �ȴ��������ź���            */

LW_API ULONG            API_SemaphoreBPendEx(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout,
                                             PVOID            *ppvMsgPtr);
                                                                        /*  �ȴ��������ź�����Ϣ        */

LW_API ULONG            API_SemaphoreBTryPend(LW_OBJECT_HANDLE  ulId);  /*  �������ȴ��������ź���      */

LW_API ULONG            API_SemaphoreBRelease(LW_OBJECT_HANDLE  ulId, 
                                              ULONG             ulReleaseCounter, 
                                              BOOL             *pbPreviousValue);
                                                                        /*  WIN32 �ͷ��ź���            */

LW_API ULONG            API_SemaphoreBPost(LW_OBJECT_HANDLE  ulId);     /*  RT �ͷ��ź���               */

LW_API ULONG            API_SemaphoreBPost2(LW_OBJECT_HANDLE  ulId, LW_OBJECT_HANDLE  *pulId);

LW_API ULONG            API_SemaphoreBPostEx(LW_OBJECT_HANDLE  ulId, 
                                             PVOID      pvMsgPtr);      /*  RT �ͷ��ź�����Ϣ           */
                                             
LW_API ULONG            API_SemaphoreBPostEx2(LW_OBJECT_HANDLE  ulId, 
                                              PVOID             pvMsgPtr, 
                                              LW_OBJECT_HANDLE *pulId);

LW_API ULONG            API_SemaphoreBClear(LW_OBJECT_HANDLE  ulId);    /*  ����ź����ź�              */

LW_API ULONG            API_SemaphoreBFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  �������еȴ��߳�            */

LW_API ULONG            API_SemaphoreBStatus(LW_OBJECT_HANDLE   ulId,
                                             BOOL              *pbValue,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  ���������ź���״̬        */
#endif

/*********************************************************************************************************
  MUTEX SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreMCreate(CPCHAR             pcName,
                                             UINT8              ucCeilingPriority,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  ���������ź���              */

LW_API ULONG            API_SemaphoreMDelete(LW_OBJECT_HANDLE  *pulId); /*  ɾ�������ź���              */

LW_API ULONG            API_SemaphoreMPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  �ȴ������ź���              */

LW_API ULONG            API_SemaphoreMPost(LW_OBJECT_HANDLE  ulId);     /*  �ͷŻ����ź���              */

LW_API ULONG            API_SemaphoreMStatus(LW_OBJECT_HANDLE   ulId,
                                             BOOL              *pbValue,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  ��û����ź�����״̬        */

LW_API ULONG            API_SemaphoreMStatusEx(LW_OBJECT_HANDLE   ulId,
                                               BOOL              *pbValue,
                                               ULONG             *pulOption,
                                               ULONG             *pulThreadBlockNum,
                                               LW_OBJECT_HANDLE  *pulOwnerId);
                                                                        /*  ��û����ź�����״̬        */
#endif

/*********************************************************************************************************
  RW SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreRWCreate(CPCHAR             pcName,
                                              ULONG              ulOption,
                                              LW_OBJECT_ID      *pulId);/*  ������д�ź���              */

LW_API ULONG            API_SemaphoreRWDelete(LW_OBJECT_HANDLE  *pulId);/*  ɾ����д�ź���              */

LW_API ULONG            API_SemaphoreRWPendR(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout);
                                                                        /*  ��д�ź����ȴ���            */
LW_API ULONG            API_SemaphoreRWPendW(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout);
                                                                        /*  ��д�ź����ȴ�д            */
LW_API ULONG            API_SemaphoreRWPost(LW_OBJECT_HANDLE  ulId);
                                                                        /*  ��д�ź����ͷ�              */
LW_API ULONG            API_SemaphoreRWStatus(LW_OBJECT_HANDLE   ulId,
                                              ULONG             *pulRWCount,
                                              ULONG             *pulRPend,
                                              ULONG             *pulWPend,
                                              ULONG             *pulOption,
                                              LW_OBJECT_HANDLE  *pulOwnerId);
                                                                        /*  ��ö�д�ź�����״̬        */
#endif

/*********************************************************************************************************
  MESSAGE QUEUE
*********************************************************************************************************/

#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
LW_API LW_OBJECT_HANDLE API_MsgQueueCreate(CPCHAR             pcName,
                                           ULONG              ulMaxMsgCounter,
                                           size_t             stMaxMsgByteSize,
                                           ULONG              ulOption,
                                           LW_OBJECT_ID      *pulId);   /*  ����һ����Ϣ����            */

LW_API ULONG            API_MsgQueueDelete(LW_OBJECT_HANDLE  *pulId);   /*  ɾ��һ����Ϣ����            */

LW_API ULONG            API_MsgQueueReceive(LW_OBJECT_HANDLE  ulId,
                                            PVOID             pvMsgBuffer,
                                            size_t            stMaxByteSize,
                                            size_t           *pstMsgLen,
                                            ULONG             ulTimeout);
                                                                        /*  ����Ϣ�����л�ȡһ����Ϣ    */

LW_API ULONG            API_MsgQueueReceiveEx(LW_OBJECT_HANDLE    ulId,
                                              PVOID               pvMsgBuffer,
                                              size_t              stMaxByteSize,
                                              size_t             *pstMsgLen,
                                              ULONG               ulTimeout,
                                              ULONG               ulOption);
                                                                        /*  ����ѡ��Ļ�ȡ��Ϣ          */

LW_API ULONG            API_MsgQueueTryReceive(LW_OBJECT_HANDLE    ulId,
                                               PVOID               pvMsgBuffer,
                                               size_t              stMaxByteSize,
                                               size_t             *pstMsgLen);
                                                                        /*  �޵ȴ��Ļ�ȡһ����Ϣ        */

LW_API ULONG            API_MsgQueueSend(LW_OBJECT_HANDLE  ulId,
                                         const PVOID       pvMsgBuffer,
                                         size_t            stMsgLen);   /*  ������з���һ����Ϣ        */
                                         
LW_API ULONG            API_MsgQueueSend2(LW_OBJECT_HANDLE  ulId,
                                          const PVOID       pvMsgBuffer,
                                          size_t            stMsgLen,
                                          ULONG             ulTimeout); /*  ���г�ʱ�ķ���һ����Ϣ      */

LW_API ULONG            API_MsgQueueSendEx(LW_OBJECT_HANDLE  ulId,
                                           const PVOID       pvMsgBuffer,
                                           size_t            stMsgLen,
                                           ULONG             ulOption); /*  ������Ϣ�߼��ӿ�            */
                                           
LW_API ULONG            API_MsgQueueSendEx2(LW_OBJECT_HANDLE  ulId,
                                            const PVOID       pvMsgBuffer,
                                            size_t            stMsgLen,
                                            ULONG             ulTimeout,
                                            ULONG             ulOption);/*  ���г�ʱ�ķ�����Ϣ�߼��ӿ�  */

LW_API ULONG            API_MsgQueueClear(LW_OBJECT_HANDLE  ulId);      /*  ���������Ϣ                */

LW_API ULONG            API_MsgQueueStatus(LW_OBJECT_HANDLE   ulId,
                                           ULONG             *pulMaxMsgNum,
                                           ULONG             *pulCounter,
                                           size_t            *pstMsgLen,
                                           ULONG             *pulOption,
                                           ULONG             *pulThreadBlockNum);
                                                                        /*  ��ö��еĵ�ǰ״̬          */

LW_API ULONG            API_MsgQueueStatusEx(LW_OBJECT_HANDLE   ulId,
                                             ULONG             *pulMaxMsgNum,
                                             ULONG             *pulCounter,
                                             size_t            *pstMsgLen,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum,
                                             size_t            *pstMaxMsgLen);
                                                                        /*  ��ö��еĵ�ǰ״̬          */

LW_API ULONG            API_MsgQueueFlush(LW_OBJECT_HANDLE  ulId, 
                                          ULONG            *pulThreadUnblockNum);
                                                                        /*  �������еȴ���Ϣ���߳�      */
LW_API ULONG            API_MsgQueueFlushSend(LW_OBJECT_HANDLE  ulId, 
                                              ULONG  *pulThreadUnblockNum);
                                                                        /*  �������еȴ�д��Ϣ���߳�    */
LW_API ULONG            API_MsgQueueFlushReceive(LW_OBJECT_HANDLE  ulId, 
                                                 ULONG  *pulThreadUnblockNum);
                                                                        /*  �������еȴ�����Ϣ���߳�    */
LW_API ULONG            API_MsgQueueGetName(LW_OBJECT_HANDLE  ulId, 
                                            PCHAR             pcName);  /*  �����Ϣ��������            */
#endif

/*********************************************************************************************************
  EVENT SET
*********************************************************************************************************/

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
LW_API LW_OBJECT_HANDLE API_EventSetCreate(CPCHAR             pcName, 
                                           ULONG              ulInitEvent, 
                                           ULONG              ulOption,
                                           LW_OBJECT_ID      *pulId);   /*  �����¼���                  */

LW_API ULONG            API_EventSetDelete(LW_OBJECT_HANDLE  *pulId);   /*  ɾ���¼���                  */

LW_API ULONG            API_EventSetSet(LW_OBJECT_HANDLE  ulId, 
                                        ULONG             ulEvent,
                                        ULONG             ulOption);    /*  �����¼�                    */

LW_API ULONG            API_EventSetGet(LW_OBJECT_HANDLE  ulId, 
                                        ULONG             ulEvent,
                                        ULONG             ulOption,
                                        ULONG             ulTimeout);   /*  �����¼�                    */
                                        
LW_API ULONG            API_EventSetGetEx(LW_OBJECT_HANDLE  ulId, 
                                          ULONG             ulEvent,
                                          ULONG             ulOption,
                                          ULONG             ulTimeout,
                                          ULONG            *pulEvent);  /*  �����¼���չ�ӿ�            */

LW_API ULONG            API_EventSetTryGet(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulEvent,
                                           ULONG             ulOption); /*  �����������¼�              */
                                           
LW_API ULONG            API_EventSetTryGetEx(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulEvent,
                                             ULONG             ulOption,
                                             ULONG            *pulEvent);
                                                                        /*  �����������¼���չ�ӿ�      */
LW_API ULONG            API_EventSetStatus(LW_OBJECT_HANDLE  ulId, 
                                           ULONG            *pulEvent,
                                           ULONG            *pulOption);/*  ����¼���״̬              */

LW_API ULONG            API_EventSetGetName(LW_OBJECT_HANDLE  ulId, 
                                            PCHAR             pcName);  /*  �¼�����                    */
#endif

/*********************************************************************************************************
  TIME
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT              API_TimeNanoSleepMethod(INT  iMethod);          /*  ���� nanosleep ʹ�õ��㷨   */
#endif

LW_API VOID             API_TimeSleep(ULONG    ulTick);                 /*  ��ǰ�߳�˯��                */

LW_API ULONG            API_TimeSleepEx(ULONG   ulTick, BOOL  bSigRet); /*  ��ǰ�߳�˯��                */

LW_API ULONG            API_TimeSleepUntil(clockid_t  clockid, 
                                           const struct timespec  *tv, 
                                           BOOL  bSigRet);              /*  ��ǰ�߳�˯��ֱ��ָ����ʱ��  */

LW_API VOID             API_TimeMSleep(ULONG   ulMSeconds);             /*  �Ժ���Ϊ��λ˯��            */

LW_API VOID             API_TimeSSleep(ULONG   ulSeconds);              /*  ����Ϊ��λ˯��              */

LW_API VOID             API_TimeSet(ULONG  ulKernenlTime);              /*  ����ϵͳʱ�������          */

LW_API ULONG            API_TimeGet(VOID);                              /*  ���ϵͳʱ�������          */

LW_API INT64            API_TimeGet64(VOID);                            /*  ���ϵͳʱ������� 64bit    */

LW_API ULONG            API_TimeGetFrequency(VOID);                     /*  ���ϵͳʱ�����Ƶ��        */

LW_API VOID             API_TimeTodAdj(INT32  *piDelta, 
                                       INT32  *piOldDelta);             /*  ΢��ϵͳ TOD ʱ��           */
                                       
LW_API INT              API_TimeTodAdjEx(INT32  *piDelta, INT32  *piDeltaNs, 
                                         INT32 *piOldDelta, INT32 *piOldDeltaNs);
                                                                        /*  �߾���΢��ϵͳ TOD ʱ��     */
/*********************************************************************************************************
  TIMER
*********************************************************************************************************/

#if	(LW_CFG_HTIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
LW_API VOID             API_TimerHTicks(VOID);                          /*  ���ٶ�ʱ�������ж�          */
#endif

LW_API ULONG            API_TimerHGetFrequency(VOID);                   /*  ��ø��ٶ�ʱ��Ƶ��          */

#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
LW_API LW_OBJECT_HANDLE API_TimerCreate(CPCHAR             pcName,
                                        ULONG              ulOption,
                                        LW_OBJECT_ID      *pulId);      /*  ����һ����ʱ��              */

LW_API ULONG            API_TimerDelete(LW_OBJECT_HANDLE  *pulId);      /*  ɾ��һ����ʱ��              */

LW_API ULONG            API_TimerStart(LW_OBJECT_HANDLE         ulId,
                                       ULONG                    ulCounter,
                                       ULONG                    ulOption,
                                       PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                                       PVOID                    pvArg); /*  ����һ����ʱ��              */
                                       
LW_API ULONG            API_TimerStartEx(LW_OBJECT_HANDLE         ulId,
                                         ULONG                    ulInitCounter,
                                         ULONG                    ulCounter,
                                         ULONG                    ulOption,
                                         PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                                         PVOID                    pvArg);
                                                                        /*  ����һ����ʱ��              */

LW_API ULONG            API_TimerCancel(LW_OBJECT_HANDLE         ulId); /*  ֹͣһ����ʱ��              */

LW_API ULONG            API_TimerReset(LW_OBJECT_HANDLE          ulId); /*  ��λһ����ʱ��              */

LW_API ULONG            API_TimerStatus(LW_OBJECT_HANDLE          ulId,
                                        BOOL                     *pbTimerRunning,
                                        ULONG                    *pulOption,
                                        ULONG                    *pulCounter,
                                        ULONG                    *pulInterval);
                                                                        /*  ���һ����ʱ��״̬          */
                                                                        
LW_API ULONG            API_TimerStatusEx(LW_OBJECT_HANDLE          ulId,
                                          BOOL                     *pbTimerRunning,
                                          ULONG                    *pulOption,
                                          ULONG                    *pulCounter,
                                          ULONG                    *pulInterval,
                                          clockid_t                *pclockid);
                                                                        /*  ���һ����ʱ��״̬��չ�ӿ�  */

LW_API ULONG            API_TimerGetName(LW_OBJECT_HANDLE  ulId, 
                                         PCHAR             pcName);     /*  ��ö�ʱ������              */
#endif

/*********************************************************************************************************
  PARTITION
*********************************************************************************************************/

#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
LW_API LW_OBJECT_HANDLE API_PartitionCreate(CPCHAR             pcName,
                                            PVOID              pvLowAddr,
                                            ULONG              ulBlockCounter,
                                            size_t             stBlockByteSize,
                                            ULONG              ulOption,
                                            LW_OBJECT_ID      *pulId);  /*  ����һ�� PARTITION          */

LW_API ULONG            API_PartitionDelete(LW_OBJECT_HANDLE   *pulId); /*  ɾ��һ�� PARTITION          */

LW_API ULONG            API_PartitionDeleteEx(LW_OBJECT_HANDLE   *pulId, BOOL  bForce);
                                                                        /*  ��չ�ӿ�                    */

LW_API PVOID            API_PartitionGet(LW_OBJECT_HANDLE  ulId);       /*  ��� PARTITION һ���ֿ�     */

LW_API PVOID            API_PartitionPut(LW_OBJECT_HANDLE  ulId, 
                                         PVOID             pvBlock);    /*  ���� PARTITION һ���ֿ�     */

LW_API ULONG            API_PartitionStatus(LW_OBJECT_HANDLE    ulId,
                                            ULONG              *pulBlockCounter,
                                            ULONG              *pulFreeBlockCounter,
                                            size_t             *pstBlockByteSize);
                                                                        /*  ��� PARTITION ״̬         */
LW_API ULONG            API_PartitionGetName(LW_OBJECT_HANDLE  ulId, 
                                             PCHAR             pcName); /*  ��� PARTITION ����         */
#endif

/*********************************************************************************************************
  REGION
*********************************************************************************************************/

#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
LW_API LW_OBJECT_HANDLE API_RegionCreate(CPCHAR             pcName,
                                         PVOID              pvLowAddr,
                                         size_t             stRegionByteSize,
                                         ULONG              ulOption,
                                         LW_OBJECT_ID      *pulId);     /*  ����һ�� REGION             */

LW_API ULONG            API_RegionDelete(LW_OBJECT_HANDLE   *pulId);    /*  ɾ��һ�� REGION             */

LW_API ULONG            API_RegionDeleteEx(LW_OBJECT_HANDLE   *pulId, BOOL  bForce);

LW_API ULONG            API_RegionAddMem(LW_OBJECT_HANDLE  ulId, PVOID  pvMem, size_t  stByteSize);
                                                                        /*  �� REGION �����һ���ڴ�    */
LW_API PVOID            API_RegionGet(LW_OBJECT_HANDLE  ulId, 
                                      size_t            stByteSize);    /*  ��� REGION һ���ֶ�        */

LW_API PVOID            API_RegionGetAlign(LW_OBJECT_HANDLE  ulId, 
                                           size_t            stByteSize, 
                                           size_t            stAlign);  /*  �����ڴ�������Ե��ڴ����  */

LW_API PVOID            API_RegionReget(LW_OBJECT_HANDLE    ulId, 
                                        PVOID               pvOldMem, 
                                        size_t              stNewByteSize);
                                                                        /*  ���·���һ���ֶ�            */

LW_API PVOID            API_RegionPut(LW_OBJECT_HANDLE  ulId, 
                                      PVOID             pvSegmentData); /*  ���� REGION һ���ֶ�        */

LW_API ULONG            API_RegionGetMax(LW_OBJECT_HANDLE  ulId, size_t  *pstMaxFreeSize);
                                                                        /*  ��������зֶγ���        */
LW_API ULONG            API_RegionStatus(LW_OBJECT_HANDLE    ulId,
                                         size_t             *pstByteSize,
                                         ULONG              *pulSegmentCounter,
                                         size_t             *pstUsedByteSize,
                                         size_t             *pstFreeByteSize,
                                         size_t             *pstMaxUsedByteSize);
                                                                        /*  ��� REGION ״̬            */

LW_API ULONG            API_RegionStatusEx(LW_OBJECT_HANDLE    ulId,
                                           size_t             *pstByteSize,
                                           ULONG              *pulSegmentCounter,
                                           size_t             *pstUsedByteSize,
                                           size_t             *pstFreeByteSize,
                                           size_t             *pstMaxUsedByteSize,
                                           PLW_CLASS_SEGMENT   psegmentList[],
                                           INT                 iMaxCounter);
                                                                        /*  ��� REGION ״̬�߼��ӿ�    */

LW_API ULONG            API_RegionGetName(LW_OBJECT_HANDLE  ulId, 
                                          PCHAR             pcName);    /*  ��� REGION ����            */
#endif

/*********************************************************************************************************
  RMS
*********************************************************************************************************/

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
LW_API LW_OBJECT_HANDLE API_RmsCreate(CPCHAR             pcName,
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId);        /*  ����һ�� RMS ������         */

LW_API ULONG            API_RmsDelete(LW_OBJECT_HANDLE  *pulId);        /*  ɾ��һ�� RMS ������         */

LW_API ULONG            API_RmsDeleteEx(LW_OBJECT_HANDLE  *pulId, BOOL  bForce);
                                                                        /*  ��չ�ӿ�                    */

LW_API ULONG            API_RmsExecTimeGet(LW_OBJECT_HANDLE  ulId, 
                                           ULONG    *pulExecTime);      /*  ����߳�ִ��ʱ��            */

LW_API ULONG            API_RmsPeriod(LW_OBJECT_HANDLE   ulId, 
                                      ULONG              ulPeriod);     /*  ָ��һ�� RMS ����           */

LW_API ULONG            API_RmsCancel(LW_OBJECT_HANDLE   ulId);         /*  ֹͣһ�� RMS ������         */

LW_API ULONG            API_RmsStatus(LW_OBJECT_HANDLE   ulId,
                                      UINT8             *pucStatus,
                                      ULONG             *pulTimeLeft,
                                      LW_OBJECT_HANDLE  *pulOwnerId);   /*  ���һ�� RMS ������״̬     */

LW_API ULONG            API_RmsGetName(LW_OBJECT_HANDLE  ulId, 
                                       PCHAR             pcName);       /*  ���һ�� RMS ����������     */
#endif

/*********************************************************************************************************
  INTERRUPT
*********************************************************************************************************/

LW_API ULONG            API_InterLock(INTREG  *piregInterLevel);        /*  �ر��ж�                    */

LW_API ULONG            API_InterUnlock(INTREG  iregInterLevel);        /*  ���ж�                    */

LW_API BOOL             API_InterContext(VOID);                         /*  �Ƿ����ж���                */

LW_API ULONG            API_InterGetNesting(VOID);                      /*  ����ж�Ƕ�ײ���            */

LW_API ULONG            API_InterGetNestingById(ULONG  ulCPUId, ULONG *pulMaxNesting);
                                                                        /*  ���ָ�� CPU �ж�Ƕ�ײ���   */
#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_InterEnter(ARCH_REG_T  reg0,
                                       ARCH_REG_T  reg1,
                                       ARCH_REG_T  reg2,
                                       ARCH_REG_T  reg3);               /*  �����ж�                    */

LW_API VOID             API_InterExit(VOID);                            /*  �˳��ж�                    */

LW_API PVOID            API_InterVectorBaseGet(VOID);                   /*  ����ж��������ַ          */
                                           
LW_API ULONG            API_InterVectorConnect(ULONG                 ulVector,
                                               PINT_SVR_ROUTINE      pfuncIsr,
                                               PVOID                 pvArg,
                                               CPCHAR                pcName);
                                                                        /*  ����ָ�������ķ������      */
LW_API ULONG            API_InterVectorConnectEx(ULONG                 ulVector,
                                                 PINT_SVR_ROUTINE      pfuncIsr,
                                                 VOIDFUNCPTR           pfuncClear,
                                                 PVOID                 pvArg,
                                                 CPCHAR                pcName);
                                                                        /*  ����ָ�������ķ������      */
LW_API ULONG            API_InterVectorDisconnect(ULONG                 ulVector,
                                                  PINT_SVR_ROUTINE      pfuncIsr,
                                                  PVOID                 pvArg);
                                                                        /*  ɾ��ָ�������ķ������      */
LW_API ULONG            API_InterVectorDisconnectEx(ULONG             ulVector,
                                                    PINT_SVR_ROUTINE  pfuncIsr,
                                                    PVOID             pvArg,
                                                    ULONG             ulOption);
                                                                        /*  ɾ��ָ�������ķ������      */
LW_API ULONG            API_InterVectorServiceCnt(ULONG  ulVector, INT  *piCnt);
                                                                        /*  ���ָ���ж������ķ������  */
LW_API ULONG            API_InterVectorEnable(ULONG  ulVector);         /*  ʹ��ָ�������ж�            */

LW_API ULONG            API_InterVectorDisable(ULONG  ulVector);        /*  ����ָ�������ж�            */

LW_API ULONG            API_InterVectorDisableEx(ULONG  ulVector, INT  iMaxServCnt);
                                                                        /*  ����ָ�������ж�            */
LW_API ULONG            API_InterVectorIsEnable(ULONG  ulVector, 
                                                BOOL  *pbIsEnable);     /*  ���ָ���ж�״̬            */

#if LW_CFG_INTER_PRIO > 0
LW_API ULONG            API_InterVectorSetPriority(ULONG  ulVector, UINT  uiPrio);
                                                                        /*  �����ж����ȼ�              */
LW_API ULONG            API_InterVectorGetPriority(ULONG  ulVector, UINT  *puiPrio);
                                                                        /*  ��ȡ�ж����ȼ�              */
#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */

#if LW_CFG_INTER_TARGET > 0
LW_API ULONG            API_InterSetTarget(ULONG  ulVector, size_t  stSize, 
                                           const PLW_CLASS_CPUSET  pcpuset);
                                                                        /*  �����ж�Ŀ�� CPU            */
LW_API ULONG            API_InterGetTarget(ULONG  ulVector, size_t  stSize, 
                                           PLW_CLASS_CPUSET  pcpuset);  /*  ��ȡ�ж�Ŀ�� CPU            */
#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */

LW_API ULONG            API_InterVectorSetFlag(ULONG  ulVector, ULONG  ulFlag);
                                                                        /*  �����ж���������            */
                                                                        
LW_API ULONG            API_InterVectorGetFlag(ULONG  ulVector, ULONG  *pulFlag);
                                                                        /*  ��ȡ�ж���������            */

#if LW_CFG_INTER_MEASURE_HOOK_EN > 0
LW_API VOID             API_InterVectorMeasureHook(VOIDFUNCPTR  pfuncEnter, VOIDFUNCPTR  pfuncExit);
                                                                        /*  �жϲ��� HOOK               */
#endif

LW_API irqreturn_t      API_InterVectorIsr(ULONG    ulVector);          /*  �жϷ������BSP�жϵ���   */

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_InterVectorIpi(ULONG  ulCPUId, 
                                           ULONG  ulIPIVector);         /*  ���ú˼��ж�������          */
                                           
LW_API VOID             API_InterVectorIpiEx(ULONG    ulCPUId, 
                                             ULONG    ulIPIVector, 
                                             FUNCPTR  pfuncClear, 
                                             PVOID    pvArg);           /*  ���ú˼��ж�������          */
#endif                                                                  /*  LW_CFG_SMP_EN               */

#if LW_CFG_ISR_DEFER_EN > 0
LW_API PLW_JOB_QUEUE    API_InterDeferGet(ULONG  ulCPUId);              /*  ��ö�Ӧ CPU ���ж��ӳٶ��� */

LW_API INT              API_InterDeferContext(VOID);                    /*  �Ƿ����жϻ� defer ������   */

LW_API ULONG            API_InterDeferJobAdd(PLW_JOB_QUEUE  pjobq, VOIDFUNCPTR  pfunc, PVOID  pvArg);
                                                                        /*  ���ж��ӳٴ�����м�������  */
LW_API ULONG            API_InterDeferJobDelete(PLW_JOB_QUEUE  pjobq, BOOL  bMatchArg, 
                                                VOIDFUNCPTR  pfunc, PVOID  pvArg);
                                                                        /*  ���ж��ӳٴ������ɾ������  */
#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */

LW_API PVOID            API_InterStackBaseGet(VOID);                    /*  ��õ�ǰ�ж϶�ջ�׵�ַ      */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API VOID             API_InterStackCheck(ULONG   ulCPUId,
                                            size_t *pstFreeByteSize,
                                            size_t *pstUsedByteSize);   /*  �ж϶�ջ���                */

/*********************************************************************************************************
  LAST ERROR
*********************************************************************************************************/

LW_API ULONG            API_GetLastError(VOID);                         /*  ���ϵͳ���һ�δ���        */

LW_API ULONG            API_GetLastErrorEx(LW_OBJECT_HANDLE  ulId, 
                                           ULONG  *pulError);           /*  ���ָ����������һ�δ���  */

LW_API VOID             API_SetLastError(ULONG  ulError);               /*  ����ϵͳ���һ�δ���        */

LW_API ULONG            API_SetLastErrorEx(LW_OBJECT_HANDLE  ulId, 
                                           ULONG  ulError);             /*  ����ָ����������һ�δ���  */

/*********************************************************************************************************
  WORK QUEUE
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_WORKQUEUE_EN > 0
LW_API PVOID            API_WorkQueueCreate(CPCHAR                  pcName,
                                            UINT                    uiQSize, 
                                            BOOL                    bDelayEn, 
                                            ULONG                   ulScanPeriod, 
                                            PLW_CLASS_THREADATTR    pthreadattr);
                                                                        /*  ����һ����������            */
LW_API ULONG            API_WorkQueueDelete(PVOID  pvWQ);               /*  ɾ��һ����������            */

LW_API ULONG            API_WorkQueueInsert(PVOID           pvWQ, 
                                            ULONG           ulDelay,
                                            VOIDFUNCPTR     pfunc, 
                                            PVOID           pvArg0,
                                            PVOID           pvArg1,
                                            PVOID           pvArg2,
                                            PVOID           pvArg3,
                                            PVOID           pvArg4,
                                            PVOID           pvArg5);    /*  ��һ���������뵽��������    */
                                            
LW_API ULONG            API_WorkQueueFlush(PVOID  pvWQ);                /*  ��չ�������                */
                                            
LW_API ULONG            API_WorkQueueStatus(PVOID  pvWQ, UINT  *puiCount);
                                                                        /*  ��ù�������״̬            */

#endif                                                                  /*  LW_CFG_WORKQUEUE_EN > 0     */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  KERNEL
*********************************************************************************************************/

LW_API VOID             API_KernelNop(CPCHAR  pcArg, LONG  lArg);       /*  �ں˿ղ���                  */

LW_API BOOL             API_KernelIsCpuIdle(ULONG  ulCPUId);            /*  ָ�� CPU �Ƿ����           */

LW_API BOOL             API_KernelIsSystemIdle(VOID);                   /*  ���� CPU �Ƿ����           */

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_CPU_FPU_EN > 0
LW_API VOID             API_KernelFpuPrimaryInit(CPCHAR  pcMachineName, 
                                                 CPCHAR  pcFpuName);    /*  ��ʼ������������            */
                                                 
#define API_KernelFpuInit   API_KernelFpuPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_KernelFpuSecondaryInit(CPCHAR  pcMachineName, 
                                                   CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_CPU_DSP_EN > 0
LW_API VOID             API_KernelDspPrimaryInit(CPCHAR  pcMachineName,
                                                 CPCHAR  pcDspName);    /*  ��ʼ�� DSP                  */

#define API_KernelDspInit   API_KernelDspPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_KernelDspSecondaryInit(CPCHAR  pcMachineName,
                                                   CPCHAR  pcDspName);
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API VOID             API_KernelReboot(INT  iRebootType);             /*  ϵͳ��������                */

LW_API VOID             API_KernelRebootEx(INT  iRebootType, 
                                           addr_t  ulStartAddress);     /*  ϵͳ����������չ            */

#ifdef __SYLIXOS_KERNEL
LW_API VOID             API_KernelTicks(VOID);                          /*  ֪ͨϵͳ����һ��ʵʱʱ��    */

LW_API VOID             API_KernelTicksContext(VOID);                   /*  ����ʱ���ж�ʱ���߳̿��ƿ�  */
                                                                        /*  ϵͳʱ��΢��                */
LW_API INT              API_KernelTicksAdjust(LONG  lNs, BOOL  bRelative);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API ULONG            API_KernelVersion(VOID);                        /*  ���ϵͳ�汾��              */

LW_API PCHAR            API_KernelVerinfo(VOID);                        /*  ���ϵͳ�汾��Ϣ�ִ�        */

LW_API ULONG            API_KernelVerpatch(VOID);                       /*  ���ϵͳ��ǰ�����汾        */

LW_API LW_OBJECT_HANDLE API_KernelGetIdle(VOID);                        /*  ��ÿ����߳̾��            */

#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
LW_API LW_OBJECT_HANDLE API_KernelGetItimer(VOID);                      /*  ��ö�ʱ��ɨ���߳̾��      */
#endif

LW_API LW_OBJECT_HANDLE API_KernelGetExc(VOID);                         /*  ����쳣�ػ��߳̾��        */

LW_API UINT8            API_KernelGetPriorityMax(VOID);                 /*  ���������ȼ�              */

LW_API UINT8            API_KernelGetPriorityMin(VOID);                 /*  ���������ȼ�              */

LW_API UINT16           API_KernelGetThreadNum(VOID);                   /*  ��õ�ǰ�߳�������          */

LW_API UINT16           API_KernelGetThreadNumByPriority(UINT8  ucPriority);             
                                                                        /*  ���ָ�����ȼ��߳�����      */

LW_API BOOL             API_KernelIsRunning(VOID);                      /*  ���ϵͳ�ں��Ƿ�����        */

LW_API ULONG            API_KernelHeapInfo(ULONG   ulOption, 
                                           size_t  *pstByteSize,
                                           ULONG   *pulSegmentCounter,
                                           size_t  *pstUsedByteSize,
                                           size_t  *pstFreeByteSize,
                                           size_t  *pstMaxUsedByteSize);/*  ���ϵͳ��״̬              */
                                           
LW_API ULONG            API_KernelHeapInfoEx(ULONG                ulOption, 
                                             size_t              *pstByteSize,
                                             ULONG               *pulSegmentCounter,
                                             size_t              *pstUsedByteSize,
                                             size_t              *pstFreeByteSize,
                                             size_t              *pstMaxUsedByteSize,
                                             PLW_CLASS_SEGMENT    psegmentList[],
                                             INT                  iMaxCounter);
                                                                        /*  ���ϵͳ��״̬�߼��ӿ�      */
                                           
#ifdef __SYLIXOS_KERNEL
#if LW_CFG_POWERM_EN > 0
LW_API VOID             API_KernelSuspend(VOID);                        /*  �ں�����                    */

LW_API VOID             API_KernelResume(VOID);                         /*  �ں˴�����״̬����          */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

LW_API ULONG            API_KernelHookSet(LW_HOOK_FUNC   hookfuncPtr, ULONG  ulOpt);     
                                                                        /*  ����ϵͳ���Ӻ���            */

LW_API LW_HOOK_FUNC     API_KernelHookGet(ULONG  ulOpt);                /*  ���ϵͳ���Ӻ���            */

LW_API ULONG            API_KernelHookDelete(ULONG  ulOpt);             /*  ɾ��ϵͳ���Ӻ���            */

#if LW_CFG_SMP_EN > 0                                                   /*  �˼��жϵ���                */
LW_API INT              API_KernelSmpCall(ULONG  ulCPUId, FUNCPTR  pfunc, PVOID  pvArg,
                                          VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);

LW_API INT              API_KernelSmpCallAll(FUNCPTR  pfunc,  PVOID  pvArg, 
                                             VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);

LW_API INT              API_KernelSmpCallAllOther(FUNCPTR  pfunc, PVOID  pvArg,
                                                  VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  STARTUP
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_KernelStartParam(CPCHAR  pcParam);          /*  �����ں���������            */

LW_API ssize_t          API_KernelStartParamGet(PCHAR  pcParam, size_t  stLen);

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
LW_API VOID             API_KernelPrimaryStart(PKERNEL_START_ROUTINE  pStartHook,
                                               PVOID                  pvKernelHeapMem,
                                               size_t                 stKernelHeapSize,
                                               PVOID                  pvSystemHeapMem,
                                               size_t                 stSystemHeapSize);
                                                                        /*  ����ϵͳ�ں� (�߼�����)     */
#else
LW_API VOID             API_KernelPrimaryStart(PKERNEL_START_ROUTINE  pStartHook);
                                                                        /*  ����ϵͳ�ں� (�߼�����)     */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#define API_KernelStart API_KernelPrimaryStart

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_KernelSecondaryStart(PKERNEL_START_ROUTINE  pStartHook);
                                                                        /*  SMP ϵͳ, �߼��Ӻ�����      */
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  SHOW
*********************************************************************************************************/

#if LW_CFG_FIO_LIB_EN > 0
LW_API VOID             API_BacktraceShow(INT  iFd, INT  iMaxDepth);    /*  ��ʾ����ջ��Ϣ              */

LW_API VOID             API_BacktracePrint(PVOID  pvBuffer, size_t  stSize, INT  iMaxDepth);

LW_API VOID             API_ThreadShow(VOID);                           /*  �� STD_OUT ��ӡ�����߳���Ϣ */

LW_API VOID             API_ThreadShowEx(pid_t  pid);                   /*  ��ʾָ���������߳�          */

LW_API VOID             API_ThreadPendShow(VOID);                       /*  ��ʾ�����߳���Ϣ            */

LW_API VOID             API_ThreadPendShowEx(pid_t  pid);               /*  ��ʾ�����߳���Ϣ            */

LW_API VOID             API_ThreadWjShow(VOID);

LW_API VOID             API_ThreadWjShowEx(pid_t  pid);

LW_API VOID             API_StackShow(VOID);                            /*  ��ӡ�����̶߳�ջʹ����Ϣ    */

#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
LW_API VOID             API_RegionShow(LW_OBJECT_HANDLE  ulId);         /*  ��ӡָ���ڴ����Ϣ          */
#endif

#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
LW_API VOID             API_PartitionShow(LW_OBJECT_HANDLE  ulId);      /*  ��ӡָ���ڴ������Ϣ        */
#endif

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API VOID             API_CPUUsageShow(INT  iWaitSec, INT  iTimes);   /*  ��ʾ CPU ��������Ϣ         */
#endif

#if LW_CFG_EVENTSET_EN > 0
LW_API VOID             API_EventSetShow(LW_OBJECT_HANDLE  ulId);       /*  ��ʾ�¼����������Ϣ        */
#endif

#if LW_CFG_MSGQUEUE_EN > 0
LW_API VOID             API_MsgQueueShow(LW_OBJECT_HANDLE  ulId);       /*  ��ʾָ����Ϣ������Ϣ        */
#endif

LW_API VOID             API_InterShow(ULONG ulCPUStart, ULONG ulCPUEnd);/*  ��ʾ����ϵͳ�ж�����������  */

LW_API VOID             API_TimeShow(VOID);                             /*  ��ʾʱ��                    */

#if (LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)
LW_API VOID             API_TimerShow(LW_OBJECT_HANDLE  ulId);          /*  ��ʾ��ʱ����Ϣ              */
#endif

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
LW_API VOID             API_RmsShow(LW_OBJECT_HANDLE  ulId);            /*  ��ʾ RMS                    */
#endif

#if LW_CFG_VMM_EN > 0
LW_API VOID             API_VmmPhysicalShow(VOID);                      /*  ��ʾ vmm ����洢����Ϣ     */
LW_API VOID             API_VmmVirtualShow(VOID);                       /*  ��ʾ vmm ����洢����Ϣ     */
LW_API VOID             API_VmmAbortShow(VOID);                         /*  ��ʾ vmm ������ֹͳ����Ϣ   */
#endif

#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API VOID             API_SemaphoreShow(LW_OBJECT_HANDLE  ulId);      /*  ��ʾָ���ź�����Ϣ          */

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
#endif                                                                  /*  __K_API_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
