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
** ��   ��   ��: k_class.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 12 ��
**
** ��        ��: ����ϵͳ�ں˻����ؼ����Ͷ����ļ���

** BUG
2007.11.04  ������ posix �������ת�Ƶ� k_ptype.h �ļ�.
2007.11.07  ���� select() ��������ļ���ṹ����.
2007.11.13  tcb �м���ȴ����߳� tcb ָ��.
2007.11.21  ������ָ�����Ͷ������ k_functype.h ��.
2007.12.22  tcb �м��� bIsInDeleteProc ���ж��߳��Ƿ����ڱ�ɾ��.
2007.12.24  tcb �м��뼸����ѡ�˾��, ��Ҫ��������ϵͳ.
2008.01.24  ���� HEAP ���������.
2008.03.02  LW_CLASS_HOOK �м����ں������ص�����.
2008.03.04  �ж��������м�������, �������.
2008.03.28  �� sigContext �Ƴ� TCB , TCBͨ������Ѱ�� sigContext, ���ٶ�ջ��ʹ����.
2008.03.28  TCB �м��� Delay �� Wdtimer ר�õ�����, ���� tick �ı���ʱ��.
2008.03.28  ��ϵͳ�����źŹ���ʱ, TCB ����ɶ� eventset node �ı���.
2008.04.12  ��ʱ���м���struct sigevent�ṹ���� posix ϵͳ������.
2009.04.08  ��Ҫ��˻�����ʵĶ����м��� spinlock �ṹ.
2009.05.21  �ѿ��ƿ�������ʹ����ͳ���ֶ�.
2009.07.11  ������������ͱ�֤�˻������, ���Բ���Ҫ������.
2009.09.29  TCB �м��� FPU ������.
2009.11.21  �����߳�˽�е� io ����ָ��.
2009.12.14  ����ÿһ���߳� kernel �����ʼ�����.
2011.02.23  �¼�������ѡ���¼.
2011.03.31  ���� vector queue �����ж�����֧��.
2011.08.17  �߳������Ĳ��ٰ��� Clib �е� reent ��Ϣ, ��������Χͳһ����.
2011.11.03  heap ���зֶ�ʹ�û����������, ����ʹ�� free ���ڴ�, �������½��ķֶ�.
            �˷�����Ҫ��Ϊ����Ͻ�������ռ��ʹ�õ�ȱҳ�жϻ���.
2013.05.07  ȥ���߳� ID ���ƿ�, TCB ����Ϊ�����ڶ�ջ�Ŀ��ƿ��.
2013.11.14  ���������Դ����ṹ.
2013.12.12  �޸��ж�������ṹ.
2015.04.18  ���� timing ������.
*********************************************************************************************************/

#ifndef __K_CLASS_H
#define __K_CLASS_H

/*********************************************************************************************************
  �������
*********************************************************************************************************/

typedef ULONG             LW_ERROR;                                     /*  ϵͳ�������                */

/*********************************************************************************************************
  ϵͳ��ʱ������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    ULONG                TIMING_ulTickHz;                               /*  ϵͳ tick Ƶ��              */
    ULONG                TIMING_ulNsecPerTick;                          /*  ÿһ�� tick ��������      */
    ULONG                TIMING_ulHTimerHz;                             /*  ���ٶ�ʱ��Ƶ��              */
    ULONG                TIMING_ulITimerRate;                           /*  Ӧ�ö�ʱ���ֱ���            */
    ULONG                TIMING_ulHotplugSec;                           /*  �Ȳ�μ��ʱ��              */
    ULONG                TIMING_ulRebootToSec;                          /*  ������ʱʱ��                */
    UINT16               TIMING_usSlice;                                /*  Ĭ��ʱ��Ƭ tick ��          */
} LW_CLASS_TIMING;

/*********************************************************************************************************
  ϵͳ������Դ�ṹ
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO_HEADER   RESRC_pmonoFreeHeader;                        /*  ��������ͷ                  */
    LW_LIST_MONO_HEADER   RESRC_pmonoFreeTail;                          /*  ��������β                  */
    
    UINT                  RESRC_uiUsed;                                 /*  ��ǰʹ����                  */
    UINT                  RESRC_uiMaxUsed;                              /*  ���ʹ����                  */
} LW_CLASS_OBJECT_RESRC;

/*********************************************************************************************************
  HOOK ����
*********************************************************************************************************/

typedef struct {
    LW_HOOK_FUNC          HOOK_ThreadCreate;                            /*  �߳̽�������                */
    LW_HOOK_FUNC          HOOK_ThreadDelete;                            /*  �߳�ɾ������                */
    LW_HOOK_FUNC          HOOK_ThreadSwap;                              /*  �߳��л�����                */
    LW_HOOK_FUNC          HOOK_ThreadTick;                              /*  ϵͳʱ���жϹ���            */
    LW_HOOK_FUNC          HOOK_ThreadInit;                              /*  �̳߳�ʼ������              */
    LW_HOOK_FUNC          HOOK_ThreadIdle;                              /*  �����̹߳���                */
    LW_HOOK_FUNC          HOOK_KernelInitBegin;                         /*  �ں˳�ʼ����ʼ����          */
    LW_HOOK_FUNC          HOOK_KernelInitEnd;                           /*  �ں˳�ʼ����������          */
    LW_HOOK_FUNC          HOOK_KernelReboot;                            /*  �ں���������                */
    LW_HOOK_FUNC          HOOK_WatchDogTimer;                           /*  ���Ź���ʱ������            */
    
    LW_HOOK_FUNC          HOOK_ObjectCreate;                            /*  �����ں˶�����            */
    LW_HOOK_FUNC          HOOK_ObjectDelete;                            /*  ɾ���ں˶�����            */
    LW_HOOK_FUNC          HOOK_FdCreate;                                /*  �ļ���������������          */
    LW_HOOK_FUNC          HOOK_FdDelete;                                /*  �ļ�������ɾ������          */
    
    LW_HOOK_FUNC          HOOK_CpuIdleEnter;                            /*  CPU �������ģʽ            */
    LW_HOOK_FUNC          HOOK_CpuIdleExit;                             /*  CPU �˳�����ģʽ            */
    LW_HOOK_FUNC          HOOK_CpuIntEnter;                             /*  CPU �����ж�(�쳣)ģʽ      */
    LW_HOOK_FUNC          HOOK_CpuIntExit;                              /*  CPU �˳��ж�(�쳣)ģʽ      */
    
    LW_HOOK_FUNC          HOOK_VpCreate;                                /*  ���̴�������                */
    LW_HOOK_FUNC          HOOK_VpDelete;                                /*  ����ɾ������                */
    
    LW_HOOK_FUNC          HOOK_StkOverflow;                             /*  �̶߳�ջ���                */
    LW_HOOK_FUNC          HOOK_FatalError;                              /*  �δ���                      */
} LW_CLASS_HOOK;

/*********************************************************************************************************
  �ж�������ṹ (���ﲻ�����ж��������ȼ��� target cpu ʹ��ʱ���� bsp �����ȡ)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE          IACT_plineManage;                             /*  ��������                    */
    INT64                 IACT_iIntCnt[LW_CFG_MAX_PROCESSORS];          /*  �жϼ�����                  */
    PINT_SVR_ROUTINE      IACT_pfuncIsr;                                /*  �жϷ�����                */
    VOIDFUNCPTR           IACT_pfuncClear;                              /*  �ж�������                */
    PVOID                 IACT_pvArg;                                   /*  �жϷ���������            */
    CHAR                  IACT_cInterName[LW_CFG_OBJECT_NAME_SIZE];
} LW_CLASS_INTACT;                                                      /*  �ж�������                  */
typedef LW_CLASS_INTACT  *PLW_CLASS_INTACT;

typedef struct {
    LW_LIST_LINE_HEADER   IDESC_plineAction;                            /*  �ж��жϷ������б�        */
    ULONG                 IDESC_ulFlag;                                 /*  �ж�����ѡ��                */
    LW_SPINLOCK_DEFINE   (IDESC_slLock);                                /*  ������                      */
} LW_CLASS_INTDESC;
typedef LW_CLASS_INTDESC *PLW_CLASS_INTDESC;

/*********************************************************************************************************
  ���ѱ� (���ʱ������)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE          WUN_lineManage;                               /*  ɨ������                    */
    BOOL                  WUN_bInQ;                                     /*  �Ƿ���ɨ������              */
    ULONG                 WUN_ulCounter;                                /*  ��Եȴ�ʱ��                */
} LW_CLASS_WAKEUP_NODE;
typedef LW_CLASS_WAKEUP_NODE   *PLW_CLASS_WAKEUP_NODE;

typedef struct {
    LW_LIST_LINE_HEADER   WU_plineHeader;
    PLW_LIST_LINE         WU_plineOp;
    INT64                 WU_i64LastTime;                               /*  ���һ�εȴ�ʱ��            */
    VOIDFUNCPTR           WU_pfuncWakeup;                               /*  �ǹ̶������������Ͷ�ʱ��  */
    PVOID                 WU_pvWakeupArg;
} LW_CLASS_WAKEUP;
typedef LW_CLASS_WAKEUP        *PLW_CLASS_WAKEUP;

/*********************************************************************************************************
  ��ʱ�����ƿ�
*********************************************************************************************************/
#if ((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

struct sigevent;
typedef struct {
    LW_LIST_MONO               TIMER_monoResrcList;                     /*  ��ʱ����Դ��                */
    LW_CLASS_WAKEUP_NODE       TIMER_wunTimer;                          /*  �ȴ���������                */
#define TIMER_ulCounter        TIMER_wunTimer.WUN_ulCounter
    
    UINT8                      TIMER_ucType;                            /*  ��ʱ������                  */
    ULONG                      TIMER_ulCounterSave;                     /*  ��ʱ������ֵ����ֵ          */
    ULONG                      TIMER_ulOption;                          /*  ��ʱ������ѡ��              */
    UINT8                      TIMER_ucStatus;                          /*  ��ʱ��״̬                  */
    PTIMER_CALLBACK_ROUTINE    TIMER_cbRoutine;                         /*  ִ�к���                    */
    PVOID                      TIMER_pvArg;                             /*  ��ʱ������                  */
    UINT16                     TIMER_usIndex;                           /*  �����е�����                */
    
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    LW_OBJECT_HANDLE           TIMER_ulTimer;
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
    
    LW_OBJECT_HANDLE           TIMER_ulThreadId;                        /*  �߳� ID                     */
    struct sigevent            TIMER_sigevent;                          /*  ��ʱ���ź��������          */
                                                                        /*  SIGEV_THREAD ����ʹ�� POSIX */
    UINT64                     TIMER_u64Overrun;                        /*  timer_getoverrun            */
    clockid_t                  TIMER_clockid;                           /*  ���� POSIX ��ʱ����Ч       */
    
#if LW_CFG_TIMERFD_EN > 0
    PVOID                      TIMER_pvTimerfd;                         /*  timerfd �ṹ                */
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */
    
    CHAR                       TIMER_cTmrName[LW_CFG_OBJECT_NAME_SIZE]; /*  ��ʱ����                    */
    LW_SPINLOCK_DEFINE        (TIMER_slLock);                           /*  ������                      */
} LW_CLASS_TIMER;
typedef LW_CLASS_TIMER        *PLW_CLASS_TIMER;

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  �¼������ƿ�
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

typedef struct {                                                        /*  �¼���                      */
    LW_LIST_MONO         EVENTSET_monoResrcList;                        /*  ������Դ��                  */
    UINT8                EVENTSET_ucType;                               /*  ����                        */
    PLW_LIST_LINE        EVENTSET_plineWaitList;                        /*  ָ���һ���ȴ��߳�          */
    ULONG                EVENTSET_ulEventSets;                          /*  32 bit �¼�λ               */
    ULONG                EVENTSET_ulOption;                             /*  �¼���ѡ��                  */
    UINT16               EVENTSET_usIndex;                              /*  �����е�����                */
    CHAR                 EVENTSET_cEventSetName[LW_CFG_OBJECT_NAME_SIZE];
                                                                        /*  �¼���־����                */
} LW_CLASS_EVENTSET;
typedef LW_CLASS_EVENTSET *PLW_CLASS_EVENTSET;

typedef struct {                                                        /*  �¼����ڵ�                  */
    LW_LIST_LINE         EVENTSETNODE_lineManage;                       /*  �¼���־������            */
    
    PVOID                EVENTSETNODE_ptcbMe;                           /*  ָ��ȴ������TCB           */
    PVOID                EVENTSETNODE_pesEventSet;                      /*  ָ���־��                  */
    ULONG                EVENTSETNODE_ulEventSets;                      /*  ��־�鿪ʼ�ȴ�              */
    UINT8                EVENTSETNODE_ucWaitType;                       /*  �ȴ�����                    */
} LW_CLASS_EVENTSETNODE;
typedef LW_CLASS_EVENTSETNODE   *PLW_CLASS_EVENTSETNODE;
typedef PLW_CLASS_EVENTSETNODE   PLW_EVENTSETNODE;

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  �ȴ����п��ƿ�
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
/*********************************************************************************************************
  ����ʹ�ù�ϣ��ɢ�к���ʹ�����ȼ���Ϊɢ�еĲ���.
*********************************************************************************************************/

typedef union { 
    PLW_LIST_RING         WL_pringFifo;                                 /*  ���������ȳ��ĵȴ�����      */
    PLW_LIST_RING         WL_pringPrio[__EVENT_Q_SIZE];                 /*  �������ȼ��ĵȴ�����        */
} LW_UNION_WAITLIST;
typedef LW_UNION_WAITLIST *PLW_UNION_WAITLIST;

typedef struct {
    LW_UNION_WAITLIST     WQ_wlQ;                                       /*  �ȴ�����                    */
    UINT16                WQ_usNum;                                     /*  �ȴ������е��̸߳���        */
} LW_CLASS_WAITQUEUE;
typedef LW_CLASS_WAITQUEUE *PLW_CLASS_WAITQUEUE;

/*********************************************************************************************************
  �¼����ƿ� (�����ڼ������������ź����������ź���������Ϣ���С�PART�ڴ��)
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO          EVENT_monoResrcList;                          /*  ������Դ��                  */
    UINT8                 EVENT_ucType;                                 /*  �¼�����                    */
    PVOID                 EVENT_pvTcbOwn;                               /*  ռ����Դ��TCBָ��           */
                                                                        /*  �����ڼ�������������      */
    ULONG                 EVENT_ulCounter;                              /*  ������ֵ                    */
    ULONG                 EVENT_ulMaxCounter;                           /*  �����ֵ                  */
    
    INT                   EVENT_iStatus;
#define EVENT_RW_STATUS_R 0
#define EVENT_RW_STATUS_W 1
    
    ULONG                 EVENT_ulOption;                               /*  �¼�ѡ��                    */
    UINT8                 EVENT_ucCeilingPriority;                      /*  �컨�����ȼ�                */
    PVOID                 EVENT_pvPtr;                                  /*  ����;ָ��                  */
                                                                        /*  �����������ź�����Ϣ��      */
                                                                        /*  �ӿ��ƿ������Ϣ���У�      */
                                                                        /*  �ڴ��                      */
    LW_CLASS_WAITQUEUE    EVENT_wqWaitQ[2];                             /*  ˫�ȴ�����                  */
#define EVENT_SEM_Q       0
    
#define EVENT_RW_Q_R      0
#define EVENT_RW_Q_W      1

#define EVENT_MSG_Q_R     0
#define EVENT_MSG_Q_S     1
    
    UINT16                EVENT_usIndex;                                /*  �������е��±�              */
    CHAR                  EVENT_cEventName[LW_CFG_OBJECT_NAME_SIZE];    /*  �¼���                      */
} LW_CLASS_EVENT;
typedef LW_CLASS_EVENT   *PLW_CLASS_EVENT;

/*********************************************************************************************************
  ��Ϣ���нڵ� (��Ȼ����)
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO          MSGNODE_monoManage;                           /*  ��Ϣ��������                */
    size_t                MSGNODE_stMsgLen;                             /*  ��Ϣ����                    */
} LW_CLASS_MSGNODE;
typedef LW_CLASS_MSGNODE *PLW_CLASS_MSGNODE;

/*********************************************************************************************************
  ��Ϣ���ж���
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO          MSGQUEUE_monoResrcList;                       /*  ������Դ��                  */
    LW_LIST_MONO_HEADER   MSGQUEUE_pmonoFree;                           /*  ������Ϣ�ڵ���              */
    
#define EVENT_MSG_Q_PRIO        8
#define EVENT_MSG_Q_PRIO_HIGH   0
#define EVENT_MSG_Q_PRIO_LOW    7
    LW_LIST_MONO_HEADER   MSGQUEUE_pmonoHeader[EVENT_MSG_Q_PRIO];       /*  ��Ϣͷ                      */
    LW_LIST_MONO_HEADER   MSGQUEUE_pmonoTail[EVENT_MSG_Q_PRIO];         /*  ��Ϣβ                      */
    UINT32                MSGQUEUE_uiMap;                               /*  ���ȼ�λͼ��                */
    
    PVOID                 MSGQUEUE_pvBuffer;                            /*  ������                      */
    size_t                MSGQUEUE_stMaxBytes;                          /*  ÿ����Ϣ��󳤶�            */
} LW_CLASS_MSGQUEUE;
typedef LW_CLASS_MSGQUEUE   *PLW_CLASS_MSGQUEUE;

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  Э�����ݽṹ
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0

typedef struct {
    ARCH_REG_CTX          COROUTINE_archRegCtx;                         /*  �Ĵ���������                */
    PLW_STACK             COROUTINE_pstkStackTop;                       /*  �߳�����ջջ��              */
                                                                        /*  ������ CRCB ��ջ��          */
    PLW_STACK             COROUTINE_pstkStackBottom;                    /*  �߳�����ջջ��              */
                                                                        /*  ������ CRCB ��ջ��          */
    size_t                COROUTINE_stStackSize;                        /*  �̶߳�ջ��С(��λ����)      */
                                                                        /*  ���� CRCB ���ڵ����ж�ջ    */
    PLW_STACK             COROUTINE_pstkStackLowAddr;                   /*  �ܶ�ջ��͵�ַ              */
    
    LW_LIST_RING          COROUTINE_ringRoutine;                        /*  Э���е�Э���б�            */
    PVOID                 COROUTINE_pvArg;                              /*  Э�����в���                */
    
    LW_OBJECT_HANDLE      COROUTINE_ulThread;                           /*  �����߳�                    */
    ULONG                 COROUTINE_ulFlags;
#define LW_COROUTINE_FLAG_DELETE    0x1                                 /*  ��Ҫɾ��                    */
#define LW_COROUTINE_FLAG_DYNSTK    0x2                                 /*  ��Ҫ�ͷŶ�ջ                */
} LW_CLASS_COROUTINE;
typedef LW_CLASS_COROUTINE     *PLW_CLASS_COROUTINE;

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  ���±����ݽṹ
*********************************************************************************************************/
#if  (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)

typedef struct {
    ULONG                NOTEPAD_ulNotePad[LW_CFG_MAX_NOTEPADS];
} LW_CLASS_NOTEPAD;
typedef LW_CLASS_NOTEPAD    *PLW_CLASS_NOTEPAD;

#endif                                                                  /*  LW_CFG_THREAD_NOTE_PAD_EN   */
                                                                        /*  (LW_CFG_MAX_NOTEPADS > 0)   */
/*********************************************************************************************************
  ˽�л��������ƿ�
*********************************************************************************************************/

typedef struct {
     LW_LIST_LINE         PRIVATEVAR_lineVarList;                       /*  ˫���߱�                    */
     LW_LIST_MONO         PRIVATEVAR_monoResrcList;                     /*  ������Դ��                  */
                                                                        /*  Ϊ�˿��ټ��� �� LW_LIST_LINE*/
                                                                        /*  ���� 0 ƫ������             */
     ULONG               *PRIVATEVAR_pulAddress;                        /*  4/8 BYTE ˽�л�������ַ     */
     ULONG                PRIVATEVAR_ulValueSave;                       /*  ����ı�����ֵ              */
} LW_CLASS_THREADVAR;
typedef LW_CLASS_THREADVAR *PLW_CLASS_THREADVAR;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  �߳����Կ�
*********************************************************************************************************/

typedef struct {
     PLW_STACK            THREADATTR_pstkLowAddr;                       /*  ȫ����ջ�����ڴ���ʼ��ַ    */
     size_t               THREADATTR_stGuardSize;                       /*  ��ջ��������С              */
     size_t               THREADATTR_stStackByteSize;                   /*  ȫ����ջ����С(�ֽ�)        */
     UINT8                THREADATTR_ucPriority;                        /*  �߳����ȼ�                  */
     ULONG                THREADATTR_ulOption;                          /*  ����ѡ��                    */
     PVOID                THREADATTR_pvArg;                             /*  �̲߳���                    */
     PVOID                THREADATTR_pvExt;                             /*  ��չ���ݶ�ָ��              */
} LW_CLASS_THREADATTR;
typedef LW_CLASS_THREADATTR     *PLW_CLASS_THREADATTR;

/*********************************************************************************************************
  �߳���չ���ƿ�
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
typedef struct {
    LW_LIST_MONO          CUR_monoNext;                                 /*  ��������һ���ڵ�            */
    VOIDFUNCPTR           CUR_pfuncClean;                               /*  �������                    */
    PVOID                 CUR_pvArg;                                    /*  ��������                    */
} __LW_CLEANUP_ROUTINE;
typedef __LW_CLEANUP_ROUTINE    *__PLW_CLEANUP_ROUTINE;

typedef struct {
    BOOL                 *TEX_pbOnce;                                   /*  ����ִ�е� once ����        */
    LW_OBJECT_HANDLE      TEX_ulMutex;                                  /*  ������                      */
    PLW_LIST_MONO         TEX_pmonoCurHeader;                           /*  cleanup node header         */
} __LW_THREAD_EXT;
typedef __LW_THREAD_EXT  *__PLW_THREAD_EXT;
#endif                                                                  /*  __SYLIXOS_KERNEL            */

typedef struct {
    LW_OBJECT_HANDLE      TCD_ulSignal;                                 /*  �ȴ��ź������              */
    LW_OBJECT_HANDLE      TCD_ulMutex;                                  /*  �����ź���                  */
    ULONG                 TCD_ulCounter;                                /*  ���ü�����                  */
} LW_THREAD_COND;
typedef LW_THREAD_COND   *PLW_THREAD_COND;

/*********************************************************************************************************
  ����������������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#if LW_CFG_CPU_FPU_EN > 0
typedef struct {
    ARCH_FPU_CTX          FPUCTX_fpuctxContext;                         /*  ��ϵ�ṹ��� FPU ������     */
    ULONG                 FPUCTX_ulReserve[2];                          /*  ���Ա���                    */
} LW_FPU_CONTEXT;
typedef LW_FPU_CONTEXT   *PLW_FPU_CONTEXT;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
typedef struct {
    ARCH_DSP_CTX          DSPCTX_dspctxContext;                         /*  ��ϵ�ṹ��� DSP ������     */
    ULONG                 DSPCTX_ulReserve[2];                          /*  ���Ա���                    */
} LW_DSP_CONTEXT;
typedef LW_DSP_CONTEXT   *PLW_DSP_CONTEXT;
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

/*********************************************************************************************************
  �߳� shell ��Ϣ
*********************************************************************************************************/

#if LW_CFG_SHELL_EN > 0
typedef struct {
    ULONG                 SHC_ulShellMagic;                             /*  shell ����ʶ���            */
    ULONG                 SHC_ulShellStdFile;                           /*  shell stdfile               */
    ULONG                 SHC_ulShellError;                             /*  shell ϵͳ����              */
    ULONG                 SHC_ulShellOption;                            /*  shell ��������              */
    addr_t                SHC_ulShellHistoryCtx;                        /*  shell Input History         */
    LW_OBJECT_HANDLE      SHC_ulShellMain;                              /*  shell ���߳�                */
    addr_t                SHC_ulGetOptCtx;                              /*  getopt() ȫ�ֱ����л���ַ   */
    FUNCPTR               SHC_pfuncShellCallback;                       /*  shell �����ص�              */
    PVOID                 SHC_pvCallbackArg;                            /*  shell �ص�����              */
} LW_SHELL_CONTEXT;
typedef LW_SHELL_CONTEXT *PLW_SHELL_CONTEXT;
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

/*********************************************************************************************************
  �߳̿��ƿ�
  
  ע��: ����������������ָ���Ƿ���Ч, �� BSP FPU ���ʵ�־���, 
        �û���ֹʹ�� LW_OPTION_THREAD_USED_FP / LW_OPTION_THREAD_STK_MAIN ѡ��.
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)                    /*  select() �ļ�               */
#include "../SylixOS/system/select/select.h"
#endif

typedef struct __lw_tcb {
    ARCH_REG_CTX          TCB_archRegCtx;                               /*  �Ĵ���������                */
    PVOID                 TCB_pvStackFP;                                /*  ����������������ָ��        */
    PVOID                 TCB_pvStackDSP;                               /*  DSP ������ָ��              */
    PVOID                 TCB_pvStackExt;                               /*  ��չ��ջ��                  */
    
    LW_LIST_MONO          TCB_monoResrcList;                            /*  ������Դ��                  */
    UINT16                TCB_usIndex;                                  /*  �������е��±�              */
    
    PLW_STACK             TCB_pstkStackTop;                             /*  �̶߳�ջջ��(��ʼ��)        */
    PLW_STACK             TCB_pstkStackBottom;                          /*  �̶߳�ջջ��(������)        */
    size_t                TCB_stStackSize;                              /*  �̶߳�ջ��С(��λ: ��)      */
    
    PLW_STACK             TCB_pstkStackLowAddr;                         /*  �ܶ�ջ��͵�ַ              */
    PLW_STACK             TCB_pstkStackGuard;                           /*  ��ջ�����                  */

#if LW_CFG_MODULELOADER_EN > 0
    INT                   TCB_iStkLocation;                             /*  ��ջλ��                    */
#define LW_TCB_STK_NONE   0
#define LW_TCB_STK_HEAP   1
#define LW_TCB_STK_VMM    2
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_CPU_FPU_EN > 0
    LW_FPU_CONTEXT        TCB_fpuctxContext;                            /*  FPU ������                  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    LW_DSP_CONTEXT        TCB_dspctxContext;                            /*  DSP ������                  */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

    INT                   TCB_iSchedRet;                                /*  ���������ص�ֵ, signal      */
    ULONG                 TCB_ulOption;                                 /*  �߳�ѡ��                    */
    LW_OBJECT_ID          TCB_ulId;                                     /*  �߳�Id                      */
    ULONG                 TCB_ulLastError;                              /*  �߳����һ�δ���            */
    
    PVOID                 TCB_pvArg;                                    /*  �߳���ڲ���                */
    
    BOOL                  TCB_bDetachFlag;                              /*  �߳� DETACH ��־λ          */
    PVOID                *TCB_ppvJoinRetValSave;                        /*  JOIN �̷߳���ֵ�����ַ     */
    PLW_LIST_LINE         TCB_plineJoinHeader;                          /*  �����̵߳ȴ��Լ�������ͷ    */
    LW_LIST_LINE          TCB_lineJoin;                                 /*  �̺߳ϲ��߱�                */
    struct __lw_tcb      *TCB_ptcbJoin;                                 /*  �ϲ���Ŀ���߳�              */
    ULONG                 TCB_ulJoinType;                               /*  �ϲ�����                    */
    
    LW_LIST_LINE          TCB_lineManage;                               /*  �ں˹������߱�              */
    LW_LIST_RING          TCB_ringReady;                                /*  ��ͬ���ȼ�������            */
    
    LW_CLASS_WAKEUP_NODE  TCB_wunDelay;                                 /*  �ȴ��ڵ�                    */
#define TCB_ulDelay       TCB_wunDelay.WUN_ulCounter
    
#if (LW_CFG_THREAD_POOL_EN > 0) && ( LW_CFG_MAX_THREAD_POOLS > 0)       /*  �̳߳ر�                    */
    LW_LIST_RING          TCB_ringThreadPool;                           /*  �̳߳ر�                    */
#endif
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    INT                   TCB_iPendQ;
    LW_LIST_RING          TCB_ringEvent;                                /*  �¼��ȴ����б�              */
    PLW_CLASS_EVENT       TCB_peventPtr;                                /*  �ȴ��¼�ָ��                */
    PLW_LIST_RING        *TCB_ppringPriorityQueue;                      /*  �� PRIORITY ����λ��        */
                                                                        /*  ����ɾ���߳�ʱ��Ҫ��ȴ�����*/
                                                                        /*  ��ʱ��Ҫȷ������λ��        */
#endif

#if LW_CFG_COROUTINE_EN > 0
    LW_CLASS_COROUTINE    TCB_crcbOrigent;                              /*  Э����Դ��                  */
    PLW_LIST_RING         TCB_pringCoroutineHeader;                     /*  Э���׵�ַ                  */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

                                                                        /*  ��������ز���              */
    LW_SPINLOCK_DEFINE   (TCB_slLock);                                  /*  ������                      */
    
#if LW_CFG_SMP_EN > 0
    BOOL                  TCB_bCPULock;                                 /*  �Ƿ��������� CPU ID         */
    ULONG                 TCB_ulCPULock;                                /*  �������е� CPU ID           */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    volatile ULONG        TCB_ulCPUId;                                  /*  �����������, ��ʾ���� CPU  */
    volatile BOOL         TCB_bIsCand;                                  /*  �Ƿ��ں�ѡ���б���          */

#if LW_CFG_SMP_EN > 0
    struct __lw_tcb      *TCB_ptcbWaitStatus;                           /*  �ȴ�״̬�޸ĵ�Ŀ���߳�      */
    PLW_LIST_LINE         TCB_plineStatusReqHeader;                     /*  �ȴ��޸ı��߳�״̬          */
    LW_LIST_LINE          TCB_lineStatusPend;                           /*  �ȴ�Ŀ���߳�״̬�޸�        */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    UINT                  TCB_uiStatusChangeReq;                        /*  ״̬ת������(�����ڷ������)*/
    ULONG                 TCB_ulStopNesting;                            /*  ֹͣǶ�ײ���                */

#define LW_TCB_REQ_SUSPEND          1                                   /*  ��������                    */
#define LW_TCB_REQ_STOP             2                                   /*  ����ֹͣ                    */
#define LW_TCB_REQ_WDEATH           3                                   /*  ����ɾ��                    */

    UINT8                 TCB_ucSchedPolicy;                            /*  ���Ȳ���                    */
    UINT8                 TCB_ucSchedActivate;                          /*  ��ͬ���ȼ����߳����ȼ�      */
    UINT8                 TCB_ucSchedActivateMode;                      /*  �߳̾�����ʽ                */
    
    UINT16                TCB_usSchedSlice;                             /*  �߳�ʱ��Ƭ����ֵ            */
    UINT16                TCB_usSchedCounter;                           /*  �̵߳�ǰʣ��ʱ��Ƭ          */
    
    UINT8                 TCB_ucPriority;                               /*  �߳����ȼ�                  */
    UINT16                TCB_usStatus;                                 /*  �̵߳�ǰ״̬                */
    
    UINT8                 TCB_ucWaitTimeout;                            /*  �ȴ���ʱ                    */
    UINT8                 TCB_ucIsEventDelete;                          /*  �¼��Ƿ�ɾ��              */
    
    ULONG                 TCB_ulSuspendNesting;                         /*  �̹߳���Ƕ�ײ���            */
    INT                   TCB_iDeleteProcStatus;                        /*  �Ƿ����ڱ�ɾ����������      */
#define LW_TCB_DELETE_PROC_NONE     0                                   /*  û�н���ɾ������            */
#define LW_TCB_DELETE_PROC_DEL      1                                   /*  ���ڱ�ɾ��                  */
#define LW_TCB_DELETE_PROC_RESTART  2                                   /*  ���ڱ�����                  */
    
    PVOID                 TCB_pvMsgBoxMessage;                          /*  ���䴫������Ϣ              */
    PVOID                 TCB_pvRetValue;                               /*  �߳��ݴ�ķ���ֵ            */

#if LW_CFG_MSGQUEUE_EN > 0
    PVOID                 TCB_pvMsgQueueMessage;                        /*  �����û���Ϣ�����ַ        */
    size_t                TCB_stMaxByteSize;                            /*  ��������С                  */
    size_t               *TCB_pstMsgByteSize;                           /*  ��Ϣ����ָ��                */
    ULONG                 TCB_ulRecvOption;                             /*  ��Ϣ���н���ѡ��            */
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
#if (LW_CFG_THREAD_DEL_EN > 0) || (LW_CFG_THREAD_RESTART_EN > 0) || (LW_CFG_SIGNAL_EN > 0)
    PLW_EVENTSETNODE      TCB_pesnPtr;                                  /*  ָ����̵߳ȴ�              */
#endif
    ULONG                 TCB_ulEventSets;                              /*  �û��ȴ��� EVENTSET         */
#endif

#if LW_CFG_THREAD_CANCEL_EN > 0
    BOOL                  TCB_bCancelRequest;                           /*  ȡ������                    */
    INT                   TCB_iCancelState;                             /*  ȡ��״̬                    */
    INT                   TCB_iCancelType;                              /*  ȡ������                    */
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
    
#if LW_CFG_THREAD_RESTART_EN > 0
    BOOL                  TCB_bRestartReq;                              /*  �� safe ģʽ�� restart ���� */
    PTHREAD_START_ROUTINE TCB_pthreadStartAddress;                      /*  �߳���������ʱ��������ַ    */
#endif

    ULONG                 TCB_ulCPUTicks;                               /*  ���̴߳����� CPU ticks      */
    ULONG                 TCB_ulCPUKernelTicks;                         /*  ���̴߳����� CPU kernel tick*/

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    ULONG                 TCB_ulCPUUsageTicks;                          /*  ���߳� CPU �����ʼ�ʱ��     */
    ULONG                 TCB_ulCPUUsageKernelTicks;                    /*  ����ʱ����, �ں�ռ�õ�ʱ��  */
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0                                     /*  ϵͳ�߳̿��Ź�              */
    LW_CLASS_WAKEUP_NODE  TCB_wunWatchDog;                              /*  ���Ź��ȴ��ڵ�              */
#define TCB_bWatchDogInQ  TCB_wunWatchDog.WUN_bInQ
#define TCB_ulWatchDog    TCB_wunWatchDog.WUN_ulCounter
#endif
    
    volatile ULONG        TCB_ulThreadLockCounter;                      /*  �߳�����������              */
    volatile ULONG        TCB_ulThreadSafeCounter;                      /*  �̰߳�ȫģʽ��־            */
    
#if LW_CFG_THREAD_DEL_EN > 0
    struct __lw_tcb      *TCB_ptcbDeleteMe;                             /*  Ҫ�����߳�ɾ���� TCB        */
    struct __lw_tcb      *TCB_ptcbDeleteWait;                           /*  �ȴ��Է�ɾ���� TCB          */
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

    UINT8                 TCB_ucStackAutoAllocFlag;                     /*  ��ջ�Ƿ���ϵͳ�ڶ��п���    */

#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)
    LW_CLASS_NOTEPAD      TCB_notepadThreadNotePad;                     /*  �̼߳��±�                  */
#endif

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    PLW_LIST_LINE         TCB_plinePrivateVars;                         /*  ȫ�ֱ���˽�л��߱�          */
#endif

    CHAR                  TCB_cThreadName[LW_CFG_OBJECT_NAME_SIZE];     /*  �߳���                      */

/*********************************************************************************************************
  POSIX EXT
*********************************************************************************************************/

#if LW_CFG_THREAD_EXT_EN > 0
    __LW_THREAD_EXT       TCB_texExt;                                   /*  �߳���չ���ݲ���            */
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

/*********************************************************************************************************
  SYSTEM (ÿ���߳�ӵ���Լ���˽�б�׼�ļ�������, �����ȼ�����ȫ�ֱ�׼�ļ�������)
  Ĭ�������̶߳���ӵ��˽�е� io ����, ֻ�е����� ioPrivateEvn() ���ӵ���Լ������� io ����.
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0)
    PVOID                 TCB_pvIoEnv;                                  /*  �߳�˽�� io ����            */
    INT                   TCB_iTaskStd[3];                              /*  ��׼ in out err �ļ�������  */
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
    LW_SEL_CONTEXT       *TCB_pselctxContext;                           /*  select ������               */
#endif

/*********************************************************************************************************
  POSIX ��չ��
*********************************************************************************************************/

    PVOID                 TCB_pvPosixContext;                           /*  pthread ���                */

/*********************************************************************************************************
  vprocess ��չ��
*********************************************************************************************************/

    atomic_t              TCB_atomicKernelSpace;                        /*  �Ƿ����ں˿ռ�              */
    PVOID                 TCB_pvVProcessContext;                        /*  ����������                  */
#if LW_CFG_MODULELOADER_EN > 0
    LW_LIST_LINE          TCB_lineProcess;
#endif                                                                  /*  ���̱�                      */
    
/*********************************************************************************************************
  ȱҳ�ж���Ϣ
*********************************************************************************************************/

    INT64                 TCB_i64PageFailCounter;                       /*  ȱҳ�жϴ���                */

/*********************************************************************************************************
  TCB �ۺ�����չ��
*********************************************************************************************************/

#if LW_CFG_SHELL_EN > 0
    LW_SHELL_CONTEXT      TCB_shc;                                      /*  shell ��Ϣ                  */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

/*********************************************************************************************************
  ���� SAFE ������
*********************************************************************************************************/

#if LW_CFG_NET_EN > 0 && LW_CFG_NET_SAFE > 0
    LW_OBJECT_HANDLE      TCB_ulNetSem;                                 /*  ȫ˫���ź���                */
#endif

/*********************************************************************************************************
  GDB ��չ��
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
#define LW_GDB_ADDR_INVAL   ((addr_t)PX_ERROR)                          /*  ��Ч��ַ                    */
    addr_t                TCB_ulStepAddr;                               /*  ������ַ��-1 ��ʾ�ǵ���ģʽ */
    ULONG                 TCB_ulStepInst;                               /*  ������ַָ���            */
    BOOL                  TCB_bStepClear;                               /*  �����ϵ��Ƿ����          */
    addr_t                TCB_ulAbortPointAddr;                         /*  ��ֹ���ַ                  */
    ULONG                 TCB_ulAbortPointInst;                         /*  ��ֹ���ַָ���          */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  TCB Ȩ�޹�����չ
*********************************************************************************************************/
    
    uid_t                 TCB_uid;                                      /*  �û�                        */
    gid_t                 TCB_gid;
    
    uid_t                 TCB_euid;                                     /*  ִ��                        */
    gid_t                 TCB_egid;
    
    uid_t                 TCB_suid;                                     /*  ����                        */
    gid_t                 TCB_sgid;
    
    gid_t                 TCB_suppgid[16];                              /*  ������ (16 MAX)             */
    UINT                  TCB_iNumSuppGid;
    
/*********************************************************************************************************
  MIPS FPU ģ��
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_MIPS
    ULONG                 TCB_ulBdEmuBranchPC;                          /*  ��֧�� PC                   */
    ULONG                 TCB_ulBdEmuContPC;                            /*  �ָ��� PC                   */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

/*********************************************************************************************************
  TCB ������չ
*********************************************************************************************************/

    PVOID                 TCB_pvReserveContext;
} LW_CLASS_TCB;
typedef LW_CLASS_TCB     *PLW_CLASS_TCB;

/*********************************************************************************************************
  �������߳� wait join ��Ϣ
*********************************************************************************************************/

typedef struct __lw_tcb_waitjoin {
    PLW_CLASS_TCB         TWJ_ptcb;
    PVOID                 TWJ_pvReturn;                                 /*  �̷߳���ֵ                  */
} LW_CLASS_WAITJOIN;
typedef LW_CLASS_WAITJOIN   *PLW_CLASS_WAITJOIN;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  �߳���Ϣ
*********************************************************************************************************/

typedef struct __lw_tcb_desc {
    PLW_STACK             TCBD_pstkStackNow;                            /*  �̵߳�ǰ��ջָ��            */
    PVOID                 TCBD_pvStackFP;                               /*  ����������������ָ��        */
    PVOID                 TCBD_pvStackExt;                              /*  ��չ��ջ��                  */
    
    PLW_STACK             TCBD_pstkStackTop;                            /*  �̶߳�ջջ��(��ʼ��)        */
    PLW_STACK             TCBD_pstkStackBottom;                         /*  �̶߳�ջջ��(������)        */
    size_t                TCBD_stStackSize;                             /*  �̶߳�ջ��С(��λ����)      */
    PLW_STACK             TCBD_pstkStackLowAddr;                        /*  �ܶ�ջ��͵�ַ              */
    PLW_STACK             TCBD_pstkStackGuard;                          /*  ������ָ��                  */
    
    INT                   TCBD_iSchedRet;                               /*  ���������ص�ֵ, signal      */
    ULONG                 TCBD_ulOption;                                /*  �߳�ѡ��                    */
    LW_OBJECT_ID          TCBD_ulId;                                    /*  �߳�Id                      */
    ULONG                 TCBD_ulLastError;                             /*  �߳����һ�δ���            */
    
    BOOL                  TCBD_bDetachFlag;                             /*  �߳� DETACH ��־λ          */
    ULONG                 TCBD_ulJoinType;                              /*  �ϲ�����                    */
    
    ULONG                 TCBD_ulWakeupLeft;                            /*  �ȴ�ʱ��                    */
    
    ULONG                 TCBD_ulCPUId;                                 /*  ����ʹ�õ� CPU ��           */
                                                                        /*  �����������, ��ʾ���� CPU  */
    BOOL                  TCBD_bIsCand;                                 /*  �Ƿ��ں�ѡ���б���          */
    
    UINT8                 TCBD_ucSchedPolicy;                           /*  ���Ȳ���                    */
    UINT8                 TCBD_ucSchedActivate;                         /*  ��ͬ���ȼ����߳����ȼ�      */
    UINT8                 TCBD_ucSchedActivateMode;                     /*  �߳̾�����ʽ                */
    
    UINT16                TCBD_usSchedSlice;                            /*  �߳�ʱ��Ƭ����ֵ            */
    UINT16                TCBD_usSchedCounter;                          /*  �̵߳�ǰʣ��ʱ��Ƭ          */
    
    UINT8                 TCBD_ucPriority;                              /*  �߳����ȼ�                  */
    UINT16                TCBD_usStatus;                                /*  �̵߳�ǰ״̬                */
    
    UINT8                 TCBD_ucWaitTimeout;                           /*  �ȴ���ʱ                    */
    
    ULONG                 TCBD_ulSuspendNesting;                        /*  �̹߳���Ƕ�ײ���            */
    INT                   TCBD_iDeleteProcStatus;                       /*  �Ƿ����ڱ�ɾ����������      */
    
    BOOL                  TCBD_bCancelRequest;                          /*  ȡ������                    */
    INT                   TCBD_iCancelState;                            /*  ȡ��״̬                    */
    INT                   TCBD_iCancelType;                             /*  ȡ������                    */
    
    PTHREAD_START_ROUTINE TCBD_pthreadStartAddress;                     /*  �߳���������ʱ��������ַ    */
    
    ULONG                 TCBD_ulCPUTicks;                              /*  ���̴߳����� CPU ticks      */
    ULONG                 TCBD_ulCPUKernelTicks;                        /*  ���̴߳����� CPU kernel tick*/

    ULONG                 TCBD_ulCPUUsageTicks;                         /*  ���߳� CPU �����ʼ�ʱ��     */
    ULONG                 TCBD_ulCPUUsageKernelTicks;                   /*  ����ʱ����, �ں�ռ�õ�ʱ��  */
    
    ULONG                 TCBD_ulWatchDogLeft;                          /*  ���Ź�ʣ��ʱ��              */
    
    ULONG                 TCBD_ulThreadLockCounter;                     /*  �߳�����������              */
    ULONG                 TCBD_ulThreadSafeCounter;                     /*  �̰߳�ȫģʽ��־            */
    
    UINT8                 TCBD_ucStackAutoAllocFlag;                    /*  ��ջ�Ƿ���ϵͳ�ڶ��п���    */
    CHAR                  TCBD_cThreadName[LW_CFG_OBJECT_NAME_SIZE];    /*  �߳���                      */
    
    LONG                  TCBD_lPid;                                    /*  PID                         */
    INT64                 TCBD_i64PageFailCounter;                      /*  ȱҳ�жϴ���                */
    
    uid_t                 TCBD_uid;                                     /*  �û�                        */
    gid_t                 TCBD_gid;
    
    uid_t                 TCBD_euid;                                    /*  ִ��                        */
    gid_t                 TCBD_egid;
    
    uid_t                 TCBD_suid;                                    /*  ����                        */
    gid_t                 TCBD_sgid;
    
    gid_t                 TCBD_suppgid[16];                             /*  ������ (16 MAX)             */
    UINT                  TCBD_iNumSuppGid;
} LW_CLASS_TCB_DESC;
typedef LW_CLASS_TCB_DESC   *PLW_CLASS_TCB_DESC;

/*********************************************************************************************************
  λͼ��
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    volatile UINT32       BMAP_uiMap;                                   /*  ��λͼ����                  */
    volatile UINT32       BMAP_uiSubMap[(LW_PRIO_LOWEST >> 5) + 1];     /*  ��λͼ����                  */
} LW_CLASS_BMAP;
typedef LW_CLASS_BMAP    *PLW_CLASS_BMAP;

/*********************************************************************************************************
  ���ȼ����ƿ�
*********************************************************************************************************/

typedef struct {
    LW_LIST_RING_HEADER   PCB_pringReadyHeader;                         /*  �����������̻߳���          */
    UINT8                 PCB_ucPriority;                               /*  ���ȼ�                      */
} LW_CLASS_PCB;
typedef LW_CLASS_PCB     *PLW_CLASS_PCB;

/*********************************************************************************************************
  ������
*********************************************************************************************************/

typedef struct {
    LW_CLASS_BMAP         PCBM_bmap;
    LW_CLASS_PCB          PCBM_pcb[LW_PRIO_LOWEST + 1];
} LW_CLASS_PCBBMAP;
typedef LW_CLASS_PCBBMAP *PLW_CLASS_PCBBMAP;

/*********************************************************************************************************
  RATE MONO SCHEDLER
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO         RMS_monoResrcList;                             /*  ������Դ��                  */
    
    UINT8                RMS_ucType;
    UINT8                RMS_ucStatus;                                  /*  ״̬                        */
    ULONG                RMS_ulTickNext;                                /*  ����ʱ��                    */
    ULONG                RMS_ulTickSave;                                /*  ϵͳʱ�䱣��                */
    PLW_CLASS_TCB        RMS_ptcbOwner;                                 /*  ������                      */
    
    UINT16               RMS_usIndex;                                   /*  �±�                        */

    CHAR                 RMS_cRmsName[LW_CFG_OBJECT_NAME_SIZE];         /*  ����                        */
} LW_CLASS_RMS;
typedef LW_CLASS_RMS    *PLW_CLASS_RMS;

/*********************************************************************************************************
  PARTITION ���ƿ�
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO         PARTITION_monoResrcList;                       /*  ������Դ��                  */
    
    UINT8                PARTITION_ucType;                              /*  ���ͱ�־                    */
    PLW_LIST_MONO        PARTITION_pmonoFreeBlockList;                  /*  �����ڴ���                */
    size_t               PARTITION_stBlockByteSize;                     /*  ÿһ��Ĵ�С                */
                                                                        /*  ������� sizeof(PVOID)      */
    ULONG                PARTITION_ulBlockCounter;                      /*  ������                      */
    ULONG                PARTITION_ulFreeBlockCounter;                  /*  ���п�����                  */
    
    UINT16               PARTITION_usIndex;                             /*  ����������                  */
    
    CHAR                 PARTITION_cPatitionName[LW_CFG_OBJECT_NAME_SIZE];
                                                                        /*  ����                        */
    LW_SPINLOCK_DEFINE  (PARTITION_slLock);                             /*  ������                      */
} LW_CLASS_PARTITION;
typedef LW_CLASS_PARTITION   *PLW_CLASS_PARTITION;

/*********************************************************************************************************
  �����Ͷ��� (��������ʹ�� ring �ṹ)
*********************************************************************************************************/

typedef struct {                                                        /*  �Ѳ���                      */
    LW_LIST_MONO         HEAP_monoResrcList;                            /*  ������Դ��                  */
    PLW_LIST_RING        HEAP_pringFreeSegment;                         /*  ���жα�                    */

#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    LW_OBJECT_HANDLE     HEAP_ulLock;                                   /*  ʹ�� semaphore ��Ϊ��       */
#endif                                                                  /*  (LW_CFG_SEMB_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
    PVOID                HEAP_pvStartAddress;                           /*  ���ڴ���ʼ��ַ              */
    size_t               HEAP_stTotalByteSize;                          /*  ���ڴ��ܴ�С                */
    
    ULONG                HEAP_ulSegmentCounter;                         /*  �ֶ�����                    */
    size_t               HEAP_stUsedByteSize;                           /*  ʹ�õ��ֽ�����              */
    size_t               HEAP_stFreeByteSize;                           /*  ���е��ֽ�����              */
    size_t               HEAP_stMaxUsedByteSize;                        /*  ���ʹ�õ��ֽ�����          */
    
    UINT16               HEAP_usIndex;                                  /*  �ѻ�������                  */
    
    CHAR                 HEAP_cHeapName[LW_CFG_OBJECT_NAME_SIZE];       /*  ����                        */
    LW_SPINLOCK_DEFINE  (HEAP_slLock);                                  /*  ������                      */
} LW_CLASS_HEAP;
typedef LW_CLASS_HEAP   *PLW_CLASS_HEAP;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  ���ڷֶ�����, ע��: �����Ƿ�ʹ�ñ�־�Ϳ��ж��и���free list, ����Ϊ��ʹ�����ڴ��ͷ�ʱ׼ȷ�ж�, 
                      �ж��Ƿ���еı�־�����ڷֶ���Ϣ�����һ��Ԫ��.
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE         SEGMENT_lineManage;                            /*  �����ھ�ָ��                */
    LW_LIST_RING         SEGMENT_ringFreeList;                          /*  ��һ�����зֶ�����          */
    size_t               SEGMENT_stByteSize;                            /*  �ֶδ�С                    */
    size_t               SEGMENT_stMagic;                               /*  �ֶα�ʶ                    */
} LW_CLASS_SEGMENT;
typedef LW_CLASS_SEGMENT    *PLW_CLASS_SEGMENT;

/*********************************************************************************************************
  ��ѡ���б�ṹ
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    volatile PLW_CLASS_TCB  CAND_ptcbCand;                              /*  ��ѡ�����߳�                */
    volatile BOOL           CAND_bNeedRotate;                           /*  ���ܲ��������ȼ�����        */
} LW_CLASS_CAND;
typedef LW_CLASS_CAND      *PLW_CLASS_CAND;

/*********************************************************************************************************
  �ں����ṹ
*********************************************************************************************************/

typedef struct {
    LW_SPINLOCK_CA_DEFINE  (KERN_slcaLock);
#define KERN_slLock         KERN_slcaLock.SLCA_sl

    PVOID                   KERN_pvCpuOwner;
    LW_OBJECT_HANDLE        KERN_ulKernelOwner;
    CPCHAR                  KERN_pcKernelEnterFunc;
} LW_CLASS_KERNLOCK LW_CACHE_LINE_ALIGN;
typedef LW_CLASS_KERNLOCK  *PLW_CLASS_KERNLOCK;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __K_CLASS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
