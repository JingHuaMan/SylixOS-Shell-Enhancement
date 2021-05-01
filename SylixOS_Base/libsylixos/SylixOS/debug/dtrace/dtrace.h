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
** ��   ��   ��: dtrace.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 11 �� 18 ��
**
** ��        ��: SylixOS ���Ը�����, GDB server ����ʹ�ô˵��Խ���.
*********************************************************************************************************/

#ifndef __DTRACE_H
#define __DTRACE_H

#include "signal.h"

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

/*********************************************************************************************************
  �Ƿ�ʹ��Ӳ������
*********************************************************************************************************/

#if defined(LW_CFG_CPU_ARCH_X86) || \
    (defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_HW_STEP_EN > 0))
#define LW_DTRACE_HW_ISTEP  1                                           /*  ֧��Ӳ�������ϵ�            */
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */

/*********************************************************************************************************
  ���� dtrace ������
*********************************************************************************************************/

#define LW_DTRACE_THREAD    0                                           /*  ����ͣ�߳�                  */
#define LW_DTRACE_PROCESS   1                                           /*  ����ͣ����                  */
#define LW_DTRACE_SYSTEM    2                                           /*  ��ͣ����ϵͳ (��ǰδ֧��)   */

/*********************************************************************************************************
  ���� dtrace flags
*********************************************************************************************************/

#define LW_DTRACE_F_DEF     0x00
#define LW_DTRACE_F_KBP     0x01                                        /*  �ں˶ϵ�ʹ��                */

/*********************************************************************************************************
  LW_DTRACE_MSG ���� (LW_TRAP_RETRY Ϊ�ڲ�ʹ��)
*********************************************************************************************************/

#define LW_TRAP_INVAL       0                                           /*  ��Ч                        */
#define LW_TRAP_RETRY       1                                           /*  ��ٶϵ�                    */

#define LW_TRAP_BRKPT       2                                           /*  ����ϵ�                    */
#define LW_TRAP_ABORT       3                                           /*  �����쳣��ֹ                */
#define LW_TRAP_ISTEP       4                                           /*  Ӳ��ָ����ϵ�            */
#define LW_TRAP_QUIT        5                                           /*  �����˳�                    */
#define LW_TRAP_WATCH       LW_TRAP_BRKPT                               /*  �۲�� (�ݲ�֧��)           */

/*********************************************************************************************************
  �ϵ���Ϣ
*********************************************************************************************************/

typedef struct {
    addr_t              DTM_ulAddr;                                     /*  �ϵ��ַ                    */
    UINT                DTM_uiType;                                     /*  ֹͣ����                    */
    LW_OBJECT_HANDLE    DTM_ulThread;                                   /*  ִ�е��ϵ���߳�            */
} LW_DTRACE_MSG;
typedef LW_DTRACE_MSG  *PLW_DTRACE_MSG;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID    API_DtraceCreate(UINT  uiType, UINT  uiFlag, LW_OBJECT_HANDLE  ulDbger);
LW_API ULONG    API_DtraceDelete(PVOID  pvDtrace);
LW_API BOOL     API_DtraceIsValid(VOID);
LW_API ULONG    API_DtraceSetPid(PVOID  pvDtrace, pid_t  pid);
LW_API ULONG    API_DtraceGetRegs(PVOID  pvDtrace, 
                                  LW_OBJECT_HANDLE  ulThread, 
                                  ARCH_REG_CTX  *pregctx,
                                  ARCH_REG_T *pregSp);
LW_API ULONG    API_DtraceSetRegs(PVOID  pvDtrace, 
                                  LW_OBJECT_HANDLE  ulThread, 
                                  const ARCH_REG_CTX  *pregctx);
                                  
#if LW_CFG_CPU_FPU_EN > 0
LW_API ULONG    API_DtraceGetFpuRegs(PVOID  pvDtrace, 
                                     LW_OBJECT_HANDLE  ulThread,
                                     ARCH_FPU_CTX  *pfpuctx);
LW_API ULONG    API_DtraceSetFpuRegs(PVOID  pvDtrace, 
                                     LW_OBJECT_HANDLE  ulThread, 
                                     const ARCH_FPU_CTX  *pfpuctx);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
LW_API ULONG    API_DtraceGetDspRegs(PVOID  pvDtrace,
                                     LW_OBJECT_HANDLE  ulThread,
                                     ARCH_DSP_CTX  *pdspctx);
LW_API ULONG    API_DtraceSetDspRegs(PVOID  pvDtrace,
                                     LW_OBJECT_HANDLE  ulThread,
                                     const ARCH_DSP_CTX  *pdspctx);
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                     
LW_API ULONG    API_DtraceGetMems(PVOID  pvDtrace, addr_t  ulAddr, PVOID  pvBuffer, size_t  stSize);
LW_API ULONG    API_DtraceSetMems(PVOID  pvDtrace, addr_t  ulAddr, const PVOID  pvBuffer, size_t  stSize);
LW_API ULONG    API_DtraceBreakpointInsert(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  *pulIns);
LW_API ULONG    API_DtraceBreakpointRemove(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  ulIns);
LW_API ULONG    API_DtraceWatchpointInsert(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize);
LW_API ULONG    API_DtraceWatchpointRemove(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize);
LW_API ULONG    API_DtraceStopThread(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceContinueThread(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceStopProcess(PVOID  pvDtrace);
LW_API ULONG    API_DtraceContinueProcess(PVOID  pvDtrace);
LW_API ULONG    API_DtraceProcessThread(PVOID  pvDtrace, LW_OBJECT_HANDLE ulThread[], 
                                        UINT   uiTableNum, UINT *puiThreadNum);
LW_API ULONG    API_DtraceGetBreakInfo(PVOID  pvDtrace, PLW_DTRACE_MSG  pdtm, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceDelBreakInfo(PVOID             pvDtrace, 
                                       LW_OBJECT_HANDLE  ulThread, 
                                       addr_t            ulBreakAddr,
                                       BOOL              bContinue);
LW_API ULONG    API_DtraceThreadExtraInfo(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread,
                                          PCHAR  pcExtraInfo, size_t  stSize);
                                          
#ifndef LW_DTRACE_HW_ISTEP
LW_API ULONG    API_DtraceThreadStepSet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  ulAddr);
LW_API ULONG    API_DtraceThreadStepGet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  *pulAddr);
LW_API VOID     API_DtraceSchedHook(LW_OBJECT_HANDLE  ulThreadOld, LW_OBJECT_HANDLE  ulThreadNew);
#else
LW_API ULONG    API_DtraceThreadStepSet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, BOOL bEnable);
#endif                                                                  /*  !LW_DTRACE_HW_ISTEP         */

/*********************************************************************************************************
  API (SylixOS internal use only!)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT      API_DtraceBreakTrap(addr_t  ulAddr, UINT  uiBpType);
LW_API INT      API_DtraceAbortTrap(addr_t  ulAddr);
LW_API INT      API_DtraceChildSig(pid_t pid, struct sigevent *psigevent, struct siginfo *psiginfo);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
#endif                                                                  /*  __DTRACE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
