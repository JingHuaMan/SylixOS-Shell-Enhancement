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
** ��   ��   ��: loader_vppatch.h
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2010 �� 12 �� 08 ��
**
** ��        ��: ���̿�
*********************************************************************************************************/

#ifndef __LOADER_VPPATCH_H
#define __LOADER_VPPATCH_H

#include "SylixOS.h"

/*********************************************************************************************************
  POSIX ֧��
*********************************************************************************************************/

#if LW_CFG_POSIX_EN > 0
#include "../SylixOS/posix/include/posixLib.h"
#endif

/*********************************************************************************************************
  ��������ļ������� 
  (��Ϊ����0 1 2��׼�ļ����ں�һ��ӳ�䷽ʽ����, ����ı�׼�ļ�Ϊ��ʵ�򿪵��ļ�, ����û�� STD_UNFIX ����.
   Ϊ�˼̳��ں��ļ�������, �������Ϊ LW_CFG_MAX_FILES + 3)
*********************************************************************************************************/

#define LW_VP_MAX_FILES     (LW_CFG_MAX_FILES + 3)

/*********************************************************************************************************
  ���̶�ʱ��
*********************************************************************************************************/

typedef struct lw_vproc_timer {
    ULONG                   VPT_ulCounter;                              /*  ��ʱ����ǰ��ʱʱ��          */
    ULONG                   VPT_ulInterval;                             /*  ��ʱ���Զ�װ��ֵ            */
} LW_LD_VPROC_T;

/*********************************************************************************************************
  ���̿��ƿ�
*********************************************************************************************************/

typedef struct lw_ld_vproc {
    LW_LIST_LINE            VP_lineManage;                              /*  ��������                    */
    LW_LIST_RING_HEADER     VP_ringModules;                             /*  ģ������                    */
    
    FUNCPTR                 VP_pfuncProcess;                            /*  ���������                  */
    PVOIDFUNCPTR            VP_pfuncMalloc;                             /*  ����˽���ڴ����            */
    VOIDFUNCPTR             VP_pfuncFree;                               /*  ����˽���ڴ����            */
    
    LW_OBJECT_HANDLE        VP_ulModuleMutex;                           /*  ����ģ��������              */
    
    BOOL                    VP_bRunAtExit;                              /*  �Ƿ����� atexit ��װ�ĺ���  */
    BOOL                    VP_bImmediatelyTerm;                        /*  �Ƿ��Ǳ������˳�            */
    
    pid_t                   VP_pid;                                     /*  ���̺�                      */
    BOOL                    VP_bIssetugid;                              /*  �Ƿ����õ� ugid             */
    PCHAR                   VP_pcName;                                  /*  ��������                    */
    PCHAR                   VP_pcCmdline;                               /*  ������                      */
    
    LW_OBJECT_HANDLE        VP_ulMainThread;                            /*  ���߳̾��                  */
    PVOID                   VP_pvProcInfo;                              /*  proc �ļ�ϵͳ��Ϣ           */
    
    clock_t                 VP_clockUser;                               /*  times ��Ӧ�� utime          */
    clock_t                 VP_clockSystem;                             /*  times ��Ӧ�� stime          */
    clock_t                 VP_clockCUser;                              /*  times ��Ӧ�� cutime         */
    clock_t                 VP_clockCSystem;                            /*  times ��Ӧ�� cstime         */
    
    PLW_IO_ENV              VP_pioeIoEnv;                               /*  I/O ����                    */
    LW_OBJECT_HANDLE        VP_ulWaitForExit;                           /*  ���̵߳ȴ�����              */

#define __LW_VP_INIT        0
#define __LW_VP_RUN         1
#define __LW_VP_STOP        2
#define __LW_VP_EXIT        3
    INT                     VP_iStatus;                                 /*  ��ǰ����״̬                */
    INT                     VP_iExitCode;                               /*  ��������                    */
    INT                     VP_iSigCode;                                /*  iSigCode                    */
    
#define __LW_VP_FT_DAEMON   0x01                                        /*  daemon ����                 */
    ULONG                   VP_ulFeatrues;                              /*  ���� featrues               */
    
    struct lw_ld_vproc     *VP_pvprocFather;                            /*  ���� (NULL ��ʾ�¶�����)    */
    LW_LIST_LINE_HEADER     VP_plineChild;                              /*  ���ӽ�������ͷ              */
    LW_LIST_LINE            VP_lineBrother;                             /*  �ֵܽ���                    */
    pid_t                   VP_pidGroup;                                /*  �� id ��                    */
    LW_LIST_LINE_HEADER     VP_plineThread;                             /*  ���߳�����                  */
    
    LW_FD_DESC              VP_fddescTbl[LW_VP_MAX_FILES];              /*  ���� fd ��                  */
    
    BOOL                    VP_bKillPrepare;                            /*  �������ǰ�Ƿ���Ҫ release  */
    INT                     VP_iExitMode;                               /*  �˳�ģʽ                    */
    LW_LD_VPROC_T           VP_vptimer[3];                              /*  REAL / VIRTUAL / PROF ��ʱ��*/
    INT64                   VP_i64Tick;                                 /*  itimer tick ��������        */
    
    LW_LIST_LINE_HEADER     VP_plineMap;                                /*  �����ڴ�ռ�                */
    
#if LW_CFG_GDB_EN > 0
    INT                     VP_iDbgFlags;                               /*  ����ѡ��                    */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    __PX_VPROC_CONTEXT      VP_pvpCtx;                                  /*  POSIX ����������            */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
} LW_LD_VPROC;

/*********************************************************************************************************
  ����װ�����ֹͣ���� (sigval.sival_int == 0 ����װ����ɲ�ֹͣ, < 0 ��ʾװ���쳣, ���������˳�)
*********************************************************************************************************/

typedef struct {
    INT                     VPS_iSigNo;                                 /*  װ����ɺ��͵��ź�        */
    LW_OBJECT_HANDLE        VPS_ulId;                                   /*  Ŀ���߳� (�����)           */
} LW_LD_VPROC_STOP;

/*********************************************************************************************************
  �ں˽��̿��ƿ�
*********************************************************************************************************/

extern LW_LD_VPROC          _G_vprocKernel;

/*********************************************************************************************************
  vprocess ������
*********************************************************************************************************/

#define LW_VP_LOCK(a)       API_SemaphoreMPend(a->VP_ulModuleMutex, LW_OPTION_WAIT_INFINITE)
#define LW_VP_UNLOCK(a)     API_SemaphoreMPost(a->VP_ulModuleMutex)

/*********************************************************************************************************
  vprocess ���ƿ����
*********************************************************************************************************/

#define __LW_VP_GET_TCB_PROC(ptcb)      ((LW_LD_VPROC *)(ptcb->TCB_pvVProcessContext))
#define __LW_VP_GET_CUR_PROC()          vprocGetCur()
#define __LW_VP_SET_CUR_PROC(pvproc)    vprocSetCur(pvproc)

/*********************************************************************************************************
  vprocess �ڲ�����
*********************************************************************************************************/

VOID                vprocThreadExitHook(PVOID  pvVProc, LW_OBJECT_HANDLE  ulId);
INT                 vprocSetGroup(pid_t  pid, pid_t  pgid);
pid_t               vprocGetGroup(pid_t  pid);
pid_t               vprocGetFather(pid_t  pid);
INT                 vprocDetach(pid_t  pid);
LW_LD_VPROC        *vprocCreate(CPCHAR  pcFile, ULONG  ulExts);
INT                 vprocDestroy(LW_LD_VPROC *pvproc);
LW_LD_VPROC        *vprocGet(pid_t  pid);
LW_LD_VPROC        *vprocGetCur(VOID);
VOID                vprocSetCur(LW_LD_VPROC  *pvproc);
pid_t               vprocGetPidByTcb(PLW_CLASS_TCB ptcb);
pid_t               vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
pid_t               vprocGetPidByThread(LW_OBJECT_HANDLE  ulId);
VOID                vprocKillPrepare(pid_t pid, LW_OBJECT_HANDLE  ulId);
LW_OBJECT_HANDLE    vprocMainThread(pid_t pid);
BOOL                vprocIsMainThread(VOID);
INT                 vprocNotifyParent(LW_LD_VPROC *pvproc, INT  iSigCode, BOOL  bUpDateStat);
VOID                vprocReclaim(LW_LD_VPROC *pvproc, BOOL  bFreeVproc);
INT                 vprocSetImmediatelyTerm(pid_t  pid);
VOID                vprocExit(LW_LD_VPROC *pvproc, LW_OBJECT_HANDLE  ulId, INT  iCode);
VOID                vprocExitNotDestroy(LW_LD_VPROC *pvproc);
INT                 vprocRun(LW_LD_VPROC      *pvproc, 
                             LW_LD_VPROC_STOP *pvpstop,
                             CPCHAR            pcFile, 
                             CPCHAR            pcEntry, 
                             INT              *piRet,
                             INT               iArgC, 
                             CPCHAR            ppcArgV[],
                             CPCHAR            ppcEnv[]);
VOID                vprocTickHook(VOID);
PLW_IO_ENV          vprocIoEnvGet(PLW_CLASS_TCB  ptcb);
FUNCPTR             vprocGetMain(VOID);
pid_t               vprocFindProc(PVOID  pvAddr);
INT                 vprocGetPath(pid_t  pid, PCHAR  pcPath, size_t stMaxLen);

#if LW_CFG_GDB_EN > 0
ssize_t             vprocGetModsInfo(pid_t  pid, PCHAR  pcBuff, size_t stMaxLen);
#if defined(LW_CFG_CPU_ARCH_CSKY)
ssize_t             vprocGetModsSvr4Info(pid_t  pid, PCHAR  pcBuff, size_t stMaxLen);
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  �����ڴ���Ϣ
*********************************************************************************************************/

INT                 vprocMemInfoNoLock(LW_LD_VPROC  *pvproc,
                                       size_t  *pstStatic, size_t  *pstHeap, size_t  *pstMmap);

/*********************************************************************************************************
  �����ڲ��̲߳���
*********************************************************************************************************/

VOID                vprocThreadAdd(PVOID   pvVProc, PLW_CLASS_TCB  ptcb);
VOID                vprocThreadDelete(PVOID   pvVProc, PLW_CLASS_TCB  ptcb);
VOID                vprocThreadKill(PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp);

INT                 vprocThreadNum(pid_t  pid, ULONG  *pulNum);
INT                 vprocThreadTraversal(pid_t          pid,
                                         VOIDFUNCPTR    pfunc,
                                         PVOID          pvArg0,
                                         PVOID          pvArg1,
                                         PVOID          pvArg2,
                                         PVOID          pvArg3,
                                         PVOID          pvArg4,
                                         PVOID          pvArg5);

#if LW_CFG_SIGNAL_EN > 0
INT                 vprocThreadSigaction(PVOID  pvVProc, VOIDFUNCPTR  pfunc, INT  iSigIndex, 
                                         const struct sigaction  *psigactionNew);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

#if LW_CFG_SMP_EN > 0
INT                 vprocThreadAffinity(PVOID  pvVProc, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  ���̵���֧��
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
VOID                vprocDebugStop(PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp);
VOID                vprocDebugContinue(PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp);
VOID                vprocDebugThreadStop(PVOID  pvVProc, LW_OBJECT_HANDLE  ulId);
VOID                vprocDebugThreadContinue(PVOID  pvVProc, LW_OBJECT_HANDLE  ulId);
UINT                vprocDebugThreadGet(PVOID  pvVProc, LW_OBJECT_HANDLE  ulId[], UINT   uiTableNum);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  ���� CPU �׺Ͷ�
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
INT                 vprocSetAffinity(pid_t  pid, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
INT                 vprocGetAffinity(pid_t  pid, size_t  stSize, PLW_CLASS_CPUSET  pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  vprocess �ļ�����������
*********************************************************************************************************/

VOID                vprocIoFileInit(LW_LD_VPROC *pvproc, ULONG ulExts); /*  vprocCreate  �ڱ�����       */
VOID                vprocIoFileDeinit(LW_LD_VPROC *pvproc);             /*  vprocDestroy �ڱ�����       */
PLW_FD_ENTRY        vprocIoFileGet(INT  iFd, BOOL  bIsIgnAbn);
PLW_FD_ENTRY        vprocIoFileGetEx(LW_LD_VPROC *pvproc, INT  iFd, BOOL  bIsIgnAbn);
PLW_FD_DESC         vprocIoFileDescGet(INT  iFd, BOOL  bIsIgnAbn);
INT                 vprocIoFileDup(PLW_FD_ENTRY pfdentry, INT  iMinFd);
INT                 vprocIoFileDup2(PLW_FD_ENTRY pfdentry, INT  iNewFd);
INT                 vprocIoFileRefInc(INT  iFd);
INT                 vprocIoFileRefDec(INT  iFd);
INT                 vprocIoFileRefGet(INT  iFd);

/*********************************************************************************************************
  �ļ����������ݲ���
*********************************************************************************************************/

INT                 vprocIoFileDupFrom(pid_t  pidSrc, INT  iFd);
INT                 vprocIoFileRefGetByPid(pid_t  pid, INT  iFd);
INT                 vprocIoFileRefIncByPid(pid_t  pid, INT  iFd);
INT                 vprocIoFileRefDecByPid(pid_t  pid, INT  iFd);
INT                 vprocIoFileRefIncArryByPid(pid_t  pid, INT  iFd[], INT  iNum);
INT                 vprocIoFileRefDecArryByPid(pid_t  pid, INT  iFd[], INT  iNum);

/*********************************************************************************************************
  ��Դ�������������º���
*********************************************************************************************************/

VOID                vprocIoReclaim(pid_t  pid, BOOL  bIsExec);

/*********************************************************************************************************
  �����̶߳�ջ
*********************************************************************************************************/

PVOID               vprocStackAlloc(PLW_CLASS_TCB  ptcbNew, ULONG  ulOption, size_t  stSize);
VOID                vprocStackFree(PLW_CLASS_TCB  ptcbDel, PVOID  pvStack);

/*********************************************************************************************************
  ���̶�ʱ��
*********************************************************************************************************/

#if LW_CFG_PTIMER_EN > 0
VOID                vprocItimerMainHook(VOID);
VOID                vprocItimerEachHook(PLW_CLASS_CPU  pcpu, LW_LD_VPROC  *pvproc);

INT                 vprocSetitimer(INT        iWhich, 
                                   ULONG      ulCounter,
                                   ULONG      ulInterval,
                                   ULONG     *pulCounter,
                                   ULONG     *pulInterval);
INT                 vprocGetitimer(INT        iWhich, 
                                   ULONG     *pulCounter,
                                   ULONG     *pulInterval);
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
#endif                                                                  /*  __LOADER_SYMBOL_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
