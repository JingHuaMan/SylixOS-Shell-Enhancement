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
** ��   ��   ��: csky_support.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 10 ��
**
** ��        ��: C-SKY ��ϵ�ܹ�֧��.
*********************************************************************************************************/

#ifndef __ARCH_CSKY_SUPPORT_H
#define __ARCH_CSKY_SUPPORT_H

#define __LW_SCHEDULER_BUG_TRACE_EN                                     /*  ���Զ�˵�����              */
#define __LW_KERNLOCK_BUG_TRACE_EN                                      /*  �����ں���                  */

/*********************************************************************************************************
  ������ͷ�ļ�
*********************************************************************************************************/

#include "arch/assembler.h"

/*********************************************************************************************************
  �洢������ (CPU ջ������)
*********************************************************************************************************/

#define CPU_STK_GROWTH              1                                   /*  1����ջ�Ӹߵ�ַ��͵�ַ     */
                                                                        /*  0����ջ�ӵ͵�ַ��ߵ�ַ     */
/*********************************************************************************************************
  arch �Ѿ��ṩ�Ľӿ�����:
*********************************************************************************************************/

#define __ARCH_KERNEL_PARAM         archKernelParam

VOID    archKernelParam(CPCHAR  pcParam);

/*********************************************************************************************************
  C-SKY ����������
*********************************************************************************************************/

VOID    archAssert(CPCHAR  pcCond, CPCHAR  pcFunc, CPCHAR  pcFile, INT  iLine);

/*********************************************************************************************************
  C-SKY �������߳���������ؽӿ�
*********************************************************************************************************/

PLW_STACK       archTaskCtxCreate(ARCH_REG_CTX          *pregctx,
                                  PTHREAD_START_ROUTINE  pfuncTask,
                                  PVOID                  pvArg,
                                  PLW_CLASS_TCB          ptcb,
                                  PLW_STACK              pstkTop,
                                  ULONG                  ulOpt);
VOID            archTaskCtxSetFp(PLW_STACK               pstkDest,
                                 ARCH_REG_CTX           *pregctxDest,
                                 const ARCH_REG_CTX     *pregctxSrc);
ARCH_REG_CTX   *archTaskRegsGet(ARCH_REG_CTX  *pregctx, ARCH_REG_T  *pregSp);
VOID            archTaskRegsSet(ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc);

#if LW_CFG_DEVICE_EN > 0
VOID        archTaskCtxShow(INT  iFd, const ARCH_REG_CTX  *pregctx);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
VOID        archTaskCtxPrint(PVOID  pvBuffer, size_t  stSize, const ARCH_REG_CTX  *pregctx);

VOID        archTaskCtxStart(PLW_CLASS_CPU  pcpuSw);
VOID        archTaskCtxSwitch(PLW_CLASS_CPU  pcpuSw);

VOID        archTaskCtxCopy(ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc);

#if LW_CFG_COROUTINE_EN > 0
VOID        archCrtCtxSwitch(PLW_CLASS_CPU  pcpuSw);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

VOID        archIntCtxLoad(PLW_CLASS_CPU  pcpuSw);
VOID        archSigCtxLoad(const ARCH_REG_CTX  *pregctx);

VOID        archIntCtxSaveReg(PLW_CLASS_CPU  pcpu,
                              ARCH_REG_T     reg0,
                              ARCH_REG_T     reg1,
                              ARCH_REG_T     reg2,
                              ARCH_REG_T     reg3);

PLW_STACK   archCtxStackEnd(const ARCH_REG_CTX  *pregctx);

/*********************************************************************************************************
  C-SKY ���������Խӿ�
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
VOID    archDbgBpInsert(addr_t   ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal);
VOID    archDbgAbInsert(addr_t   ulAddr, ULONG  *pulIns);
VOID    archDbgBpRemove(addr_t   ulAddr, size_t stSize, ULONG   ulIns, BOOL  bLocal);
VOID    archDbgApRemove(addr_t   ulAddr, ULONG   ulIns);
VOID    archDbgBpPrefetch(addr_t ulAddr);
UINT    archDbgTrapType(addr_t   ulAddr, PVOID   pvArch);
VOID    archDbgBpAdjust(PVOID  pvDtrace, PVOID   pvtm);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  C-SKY �������쳣�ӿ�
*********************************************************************************************************/

VOID    archIntHandle(ULONG  ulVector, BOOL  bPreemptive);              /*  bspIntHandle ��Ҫ���ô˺��� */

/*********************************************************************************************************
  C-SKY ͨ�ÿ�
*********************************************************************************************************/

INT     archFindLsb(UINT32 ui32);
INT     archFindMsb(UINT32 ui32);
INTREG  archGetPSR(VOID);

/*********************************************************************************************************
  C-SKY ��������׼�ײ��
*********************************************************************************************************/

#define KN_INT_DISABLE()            archIntDisable()
#define KN_INT_ENABLE(intLevel)     archIntEnable(intLevel)
#define KN_INT_ENABLE_FORCE()       archIntEnableForce()

INTREG  archIntDisable(VOID);
VOID    archIntEnable(INTREG  iregInterLevel);
VOID    archIntEnableForce(VOID);

VOID    archPageCopy(PVOID pvTo, PVOID pvFrom);

#define KN_COPY_PAGE(to, from)      archPageCopy(to, from)

VOID    archReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  C-SKY ������ BogoMIPS ѭ��
*********************************************************************************************************/

VOID    archBogoMipsLoop(ULONG  ulLoop);

#define __ARCH_BOGOMIPS_LOOP            archBogoMipsLoop
#define __ARCH_BOGOMIPS_INS_PER_LOOP    8

/*********************************************************************************************************
  C-SKY ������ CACHE ����
*********************************************************************************************************/

#define CSKY_MACHINE_AUTO   "auto"
#define CSKY_MACHINE_510    "ck510"
#define CSKY_MACHINE_610    "ck610"
#define CSKY_MACHINE_801    "ck801"
#define CSKY_MACHINE_802    "ck802"
#define CSKY_MACHINE_803    "ck803"
#define CSKY_MACHINE_807    "ck807"
#define CSKY_MACHINE_810    "ck810"
#define CSKY_MACHINE_860    "ck860"

#if LW_CFG_CACHE_EN > 0
VOID    archCacheReset(CPCHAR     pcMachineName);
VOID    archCacheInit(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);

#define __ARCH_CACHE_INIT   archCacheInit
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

/*********************************************************************************************************
  C-SKY ������ MMU ���� (armMmu*() �ɹ��� bsp ʹ��)
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
VOID    archMmuInit(CPCHAR  pcMachineName);

#define __ARCH_MMU_INIT     archMmuInit
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  C-SKY ������ MPU ���� (bsp ��ʼ���׶�ֱ�ӵ������溯����ʼ�� MPU)
*********************************************************************************************************/

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_CSKY_MPU > 0)
VOID    archMpuInit(CPCHAR  pcMachineName, const CSKY_MPU_REGION  mpuregion[]);
#endif

/*********************************************************************************************************
  C-SKY �������������������
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    archSpinInit(spinlock_t  *psl);
VOID    archSpinDelay(VOID);
VOID    archSpinNotify(VOID);

#define __ARCH_SPIN_INIT    archSpinInit
#define __ARCH_SPIN_DELAY   archSpinDelay
#define __ARCH_SPIN_NOTIFY  archSpinNotify

INT     archSpinLock(spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg);
INT     archSpinTryLock(spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur);
INT     archSpinUnlock(spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur);

#define __ARCH_SPIN_LOCK    archSpinLock
#define __ARCH_SPIN_TRYLOCK archSpinTryLock
#define __ARCH_SPIN_UNLOCK  archSpinUnlock

INT     archSpinLockRaw(spinlock_t  *psl);
INT     archSpinTryLockRaw(spinlock_t  *psl);
INT     archSpinUnlockRaw(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK_RAW    archSpinLockRaw
#define __ARCH_SPIN_TRYLOCK_RAW archSpinTryLockRaw
#define __ARCH_SPIN_UNLOCK_RAW  archSpinUnlockRaw

ULONG   archMpCur(VOID);
VOID    archMpInt(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  C-SKY �ڴ�����
*********************************************************************************************************/

#define KN_BARRIER()            __asm__ __volatile__ ("" : : : "memory")

#if LW_CFG_CSKY_CACHE_V2 > 0

#define KN_MB()                 __asm__ __volatile__ ("bar.brwarw" : : : "memory")
#define KN_RMB()                __asm__ __volatile__ ("bar.brar"   : : : "memory")
#define KN_WMB()                __asm__ __volatile__ ("bar.bwaw"   : : : "memory")

#define KN_DMA_RMB()            KN_RMB()
#define KN_DMA_WMB()            KN_WMB()

#if LW_CFG_SMP_EN > 0
#define KN_SMP_MB()             __asm__ __volatile__ ("bar.brwarws" : : : "memory")
#define KN_SMP_RMB()            __asm__ __volatile__ ("bar.brars"   : : : "memory")
#define KN_SMP_WMB()            __asm__ __volatile__ ("bar.bwaws"   : : : "memory")

#else
#define KN_SMP_MB()             KN_BARRIER()
#define KN_SMP_RMB()            KN_BARRIER()
#define KN_SMP_WMB()            KN_BARRIER()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#else                                                                   /*  LW_CFG_CSKY_CACHE_V2 > 0    */

#define KN_MB()                 __asm__ __volatile__ ("sync" : : : "memory")

#define KN_RMB()                KN_MB()
#define KN_WMB()                KN_MB()

#define KN_DMA_RMB()            KN_RMB()
#define KN_DMA_WMB()            KN_WMB()

#if LW_CFG_SMP_EN > 0
#define KN_SMP_MB()             KN_MB()
#define KN_SMP_RMB()            KN_RMB()
#define KN_SMP_WMB()            KN_WMB()

#else
#define KN_SMP_MB()             KN_BARRIER()
#define KN_SMP_RMB()            KN_BARRIER()
#define KN_SMP_WMB()            KN_BARRIER()
#endif

#endif                                                                  /*  LW_CFG_CSKY_CACHE_V2 == 0   */

/*********************************************************************************************************
  C-SKY ����������������
*********************************************************************************************************/

#define CSKY_FPU_FPUV1    "fpuv1"
#define CSKY_FPU_FPUV2    "fpuv2"
#define CSKY_FPU_NONE     "none"

#if LW_CFG_CPU_FPU_EN > 0
VOID    archFpuPrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#if LW_CFG_SMP_EN > 0
VOID    archFpuSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID    archFpuCtxInit(PVOID pvFpuCtx);
VOID    archFpuEnable(VOID);
VOID    archFpuDisable(VOID);
VOID    archFpuSave(PVOID pvFpuCtx);
VOID    archFpuRestore(PVOID pvFpuCtx);

#define __ARCH_FPU_CTX_INIT     archFpuCtxInit
#define __ARCH_FPU_ENABLE       archFpuEnable
#define __ARCH_FPU_DISABLE      archFpuDisable
#define __ARCH_FPU_SAVE         archFpuSave
#define __ARCH_FPU_RESTORE      archFpuRestore

#if LW_CFG_DEVICE_EN > 0
VOID    archFpuCtxShow(INT  iFd, PVOID pvFpuCtx);
#define __ARCH_FPU_CTX_SHOW     archFpuCtxShow
#endif                                                                  /*  LW_CFG_DEVICE_EN            */

INT     archFpuUndHandle(PLW_CLASS_TCB  ptcbCur);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN           */

/*********************************************************************************************************
  bsp ��Ҫ�ṩ�Ľӿ�����:
*********************************************************************************************************/
/*********************************************************************************************************
  C-SKY �������쳣�ص� (�ɲ�����)
*********************************************************************************************************/

#if LW_CFG_CPU_EXC_HOOK_EN > 0
#define ARCH_INSTRUCTION_EXCEPTION  0                                   /*  iExcType                    */
#define ARCH_DATA_EXCEPTION         1
#define ARCH_MACHINE_EXCEPTION      2
#define ARCH_BUS_EXCEPTION          3

INT     bspCpuExcHook(PLW_CLASS_TCB   ptcb,
                      addr_t          ulRetAddr,
                      addr_t          ulExcAddr,
                      INT             iExcType,
                      INT             iExcInfo);
#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN      */

/*********************************************************************************************************
  C-SKY �������ж������ж�
*********************************************************************************************************/

VOID    bspIntInit(VOID);
VOID    bspIntHandle(ULONG  ulVector);

UINT32  bspIntInitEnableStatus(VOID);

VOID    bspIntVectorEnable(ULONG  ulVector);
VOID    bspIntVectorDisable(ULONG  ulVector);
BOOL    bspIntVectorIsEnable(ULONG  ulVector);

#define __ARCH_INT_VECTOR_ENABLE    bspIntVectorEnable
#define __ARCH_INT_VECTOR_DISABLE   bspIntVectorDisable
#define __ARCH_INT_VECTOR_ISENABLE  bspIntVectorIsEnable

#if LW_CFG_INTER_PRIO > 0
ULONG   bspIntVectorSetPriority(ULONG  ulVector, UINT  uiPrio);
ULONG   bspIntVectorGetPriority(ULONG  ulVector, UINT *puiPrio);

#define __ARCH_INT_VECTOR_SETPRIO   bspIntVectorSetPriority
#define __ARCH_INT_VECTOR_GETPRIO   bspIntVectorGetPriority
#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */

#if LW_CFG_INTER_TARGET > 0
ULONG   bspIntVectorSetTarget(ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
ULONG   bspIntVectorGetTarget(ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset);

#define __ARCH_INT_VECTOR_SETTARGET bspIntVectorSetTarget
#define __ARCH_INT_VECTOR_GETTARGET bspIntVectorGetTarget
#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */

/*********************************************************************************************************
  CPU ��ʱ��ʱ��
*********************************************************************************************************/

VOID    bspTickInit(VOID);
VOID    bspDelayUs(ULONG ulUs);
VOID    bspDelayNs(ULONG ulNs);

/*********************************************************************************************************
  TICK �߾���ʱ������
  ��δʵ����ɶ���Ϊ�պ���, ͬʱϵͳ��ʼ��ǰ����� API_TimeNanoSleepMethod(LW_TIME_NANOSLEEP_METHOD_TICK)
*********************************************************************************************************/

#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
VOID    bspTickHighResolution(struct timespec *ptv);
#endif                                                                  /*  LW_CFG_TIME_HIGH_...        */

/*********************************************************************************************************
  �ں˹ؼ�λ�ûص�����
*********************************************************************************************************/

VOID    bspTaskCreateHook(LW_OBJECT_ID  ulId);
VOID    bspTaskDeleteHook(LW_OBJECT_ID  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
VOID    bspTaskSwapHook(LW_OBJECT_HANDLE  hOldThread, LW_OBJECT_HANDLE  hNewThread);
VOID    bspTaskIdleHook(VOID);
VOID    bspTickHook(INT64  i64Tick);
VOID    bspWdTimerHook(LW_OBJECT_ID  ulId);
VOID    bspTCBInitHook(LW_OBJECT_ID  ulId, PLW_CLASS_TCB   ptcb);
VOID    bspKernelInitHook(VOID);

/*********************************************************************************************************
  ϵͳ�������� (ulStartAddress �������ڵ���, BSP �ɺ���)
*********************************************************************************************************/

VOID    bspReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  ϵͳ�ؼ���Ϣ��ӡ (��ӡ��Ϣ���������κβ���ϵͳ api)
*********************************************************************************************************/

VOID    bspDebugMsg(CPCHAR pcMsg);

/*********************************************************************************************************
  BSP ��Ϣ
*********************************************************************************************************/

CPCHAR  bspInfoCpu(VOID);
CPCHAR  bspInfoCache(VOID);
CPCHAR  bspInfoPacket(VOID);
CPCHAR  bspInfoVersion(VOID);
ULONG   bspInfoHwcap(VOID);
addr_t  bspInfoRomBase(VOID);
size_t  bspInfoRomSize(VOID);
addr_t  bspInfoRamBase(VOID);
size_t  bspInfoRamSize(VOID);

/*********************************************************************************************************
  C-SKY ������ MMU ����
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
ULONG   bspMmuPgdMaxNum(VOID);
ULONG   bspMmuPteMaxNum(VOID);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  C-SKY ��������˲���
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    bspMpInt(ULONG  ulCPUId);
VOID    bspCpuUp(ULONG  ulCPUId);                                       /*  ����һ�� CPU                */

#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID    bspCpuDown(ULONG  ulCPUId);                                     /*  ֹͣһ�� CPU                */
#endif

#if LW_CFG_CPU_ARCH_SMT > 0
ULONG   bspCpuLogic2Physical(ULONG  ulCPUId);
#endif
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  C-SKY ������ CPU ���� (������Ƶ)
*********************************************************************************************************/

#if LW_CFG_POWERM_EN > 0
VOID    bspSuspend(VOID);                                               /*  ϵͳ����                    */
VOID    bspCpuPowerSet(UINT  uiPowerLevel);                             /*  ���� CPU ��Ƶ�ȼ�           */
VOID    bspCpuPowerGet(UINT *puiPowerLevel);                            /*  ��ȡ CPU ��Ƶ�ȼ�           */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

/*********************************************************************************************************
  Trusted computing
*********************************************************************************************************/

#if LW_CFG_TRUSTED_COMPUTING_EN > 0
VOID    bspTrustedModuleInit(VOID);
INT     bspTrustedModuleCheck(const PCHAR  pcPath);
VOID    bspTrustedModuleLoad(PVOID  pvModule);
VOID    bspTrustedModuleUnload(PVOID  pvModule);
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */

#endif                                                                  /*  __ARCH_CSKY_SUPPORT_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
