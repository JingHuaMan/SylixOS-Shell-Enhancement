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
** ��   ��   ��: _ThreadInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�̳߳�ʼ�������⡣

** BUG
2007.05.08  ������̱߳�׼���롢����ʹ����ļ��������ĳ�ʼ������ʼ��Ϊȫ�����롢����ʹ����ļ�������
2007.11.07  CreateHook ֻ�ڻص����м��� option ����.
2007.11.13  ���� TCB_ptcbJoin ��ʼ��.
2007.12.22  ���� TCB_bIsInDeleteProc �ĳ�ʼ��.
2007.12.22  �� pulNote ��Ϊ ULONG ����.
2008.01.20  �Ľ����̵߳��������Ļ���, ������Ϊ�߳�˽�б���. ���������߳��������ĳ�ʼ��.
2008.03.30  ʹ���µľ���������.
2008.04.14  Ϊ��ʵ���ź�ϵͳ�������������ֵ�ĳ�ʼ��.
2008.06.01  �����Ƕ�ͯ��, Ԥף���봨������ʧȥ��԰�ĺ���, ��ͯ�ڿ���.
            ���ﲻ��ʹ�� _signalInit() ֱ�Ӵ���, ��ʹ�� hook �����, ���� ThreadRestart() �ڴ����ź�ʱ
            �����г�ʼ������, ��ӵĽ����һ�����ص��ڴ�й¶����.
2008.06.06  ���� thread_cancel() �������.
2008.06.19  �������ϵͳ��Э�̵�֧��.
2009.02.03  ���� TCB ��չģ��ĳ�ʼ��.
2009.04.09  �޸Ļص�����.
2009.04.28  ���� TCB_bRunning ��ʼ��.
2009.10.11  �����˻ص���������˳��.
2009.11.21  ��ʼ��ʹ��ȫ�� io ����.
2009.12.14  ������߳��ں������ʵĲ����ʼ��.
2009.12.30  ����� TCB_pvPosixContext �ĳ�ʼ��.
2010.08.03  �޸�һЩע��.
2011.08.17  �� libc �߳������ĵĳ�ʼ��ͳһ���� lib_nlreent_init ����.
2012.03.30  ����� uid euid gid egid �ĳ�ʼ��.
2012.08.24  ����Խ�����Ϣ�ļ̳�.
2012.09.11  ���� OSFpuInitCtx() ��ʼ�� TCB �� FPU ����������.
2012.12.06  LW_OPTION_OBJECT_GLOBAL �̲߳������κν���.
2013.05.07  TCB �ӽ��������ջ����.
2013.08.28  _TCBBuild() ���ٴ����������.
            ����������ĺ�������.
2014.05.13  ����Խ������߳������֧��.
2014.07.26  ����� GDB �����ĳ�ʼ��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  ����ջʾ��ͼ (������ FP ��ջ�� �� ��չջ��)
*********************************************************************************************************/
/*********************************************************************************************************
1:  �Ӹߵ�ַ��͵�ַ��������

*         - ���ڴ��ַ -
*    __________________________ 
*     |//////////////////////|  <--- pstkStackTop ----------|
*     |//////////////////////|                              |
*     |//// THREAD STACK ////|                              |
*     |//////////////////////|       ��ջ��                 | 
*     |//////////////////////|                              |
*     |//////////////////////|                              |
*    _|//////////////////////|_ <--- pstkStackLowAddr ------|
*         - ���ڴ��ַ -            (pstkStackButtom)

2:  �ӵ͵�ַ��ߵ�ַ��������

*         - ���ڴ��ַ -
*    __________________________
*     |//////////////////////|  <--- pstkStackButtom -------|
*     |//////////////////////|                              |
*     |//////////////////////|                              |
*     |//// THREAD STACK ////|       ��ջ��                 |
*     |//////////////////////|								|
*     |//////////////////////|                              |
*    _|//////////////////////|_ <--- pstkStackTop ----------|
*         - ���ڴ��ַ -           (pstkStackLowAddr) 

*********************************************************************************************************/
/*********************************************************************************************************
** ��������: _TCBBuild
** ��������: ����һ��TCB
** �䡡��  : ucPriority                  ���ȼ�
**           pstkStackTop                ��ʵ������ջ����ջ��
**           pstkStackButtom             ��ʵ������ջ����ջ��
**           pstkStackGuard              ��ջ�����
**           pvStackExt                  ��չ��ջ��ջ��
**           pstkStackLowAddr            ���� TCB ��ȫ��ջ������ڴ��ַ
**           ulStackSize                 ���� TCB ��ȫ��ջ����С(��λ����)
**           pulId                       ������ TCB ID��
**           pulOption                   �����߳�ѡ��
**           pThread                     �߳���ʼ��ַ
**           ptcb                        ��Ҫ��ʼ���� TCB
**           pvArg                       �߳���������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TCBBuild (UINT8                    ucPriority,
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
                 PVOID                    pvArg)
{
    REGISTER PLW_CLASS_TCB        ptcbCur;
    
#if LW_CFG_COROUTINE_EN > 0
    REGISTER PLW_CLASS_COROUTINE  pcrcb;
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

#if LW_CFG_THREAD_NOTE_PAD_EN > 0
    REGISTER UINT8            ucI;
    REGISTER ULONG           *pulNote;
#endif                                                                  /*  LW_CFG_THREAD_NOTE_PAD_EN   */

    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ ptcb                   */

    ptcb->TCB_pstkStackTop    = pstkStackTop;                           /*  ��ʵ��ջ�� ͷ               */
    ptcb->TCB_pstkStackBottom = pstkStackButtom;                        /*  ��ʵ��ջ�� ��               */
    ptcb->TCB_pstkStackGuard  = pstkStackGuard;                         /*  ��ջ������                  */
    
    ptcb->TCB_pvStackExt   = pvStackExt;
    ptcb->TCB_stStackSize  = stStackSize;                               /*  ��CPU��Ϊ��λ�Ķ�ջ����С   */

    ptcb->TCB_pstkStackLowAddr = pstkStackLowAddr;                      /*  ���� TCB ���ڵĶ�ջ����ַ   */

    ptcb->TCB_iSchedRet     = ERROR_NONE;                               /*  ��ʼ������������ֵ          */
    ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                       /*  û�г�ʱ                    */
    
    ptcb->TCB_ulOption = ulOption;                                      /*  ѡ��                        */

#if defined(LW_CFG_CPU_ARCH_CSKY) && (LW_CFG_CPU_FPU_EN > 0)
    /*
     * C-SKY CPU �� FPU һֱ��, ����û�� FPU ������λ�ɹ��ж�, �����ǰ�߳�ʹ��Ӳ������,
     * �����������̶߳���Ϊʹ��Ӳ������
     */
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        ptcb->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    }
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */

    ptcb->TCB_ulId          = ulId;                                     /*  Id                          */
    ptcb->TCB_ulLastError   = ERROR_NONE;                               /*  ���һ������                */
    ptcb->TCB_pvArg         = pvArg;                                    /*  ����                        */
    
    __WAKEUP_NODE_INIT(&ptcb->TCB_wunDelay);
    
    ptcb->TCB_ucPriority          = ucPriority;                         /*  ���ȼ�                      */
    ptcb->TCB_ucSchedPolicy       = LW_OPTION_SCHED_RR;                 /*  ��ʼ��ͬ���ȼ����Ȳ���      */
    ptcb->TCB_usSchedSlice        = LW_SCHED_SLICE;                     /*  ʱ��Ƭ��ʼ��                */
    ptcb->TCB_usSchedCounter      = LW_SCHED_SLICE;
    ptcb->TCB_ucSchedActivateMode = LW_OPTION_RESPOND_AUTO;             /*  ��ͬ���ȼ���Ӧ�̶�          */
                                                                        /*  ��ʼ��Ϊ�ں��Զ�ʶ��        */
    ptcb->TCB_ucSchedActivate     = LW_SCHED_ACT_OTHER;                 /*  ��ʼ��Ϊ���жϼ��ʽ      */
    ptcb->TCB_iDeleteProcStatus   = LW_TCB_DELETE_PROC_NONE;            /*  û�б�ɾ��                  */
    
#if	LW_CFG_THREAD_RESTART_EN > 0
    ptcb->TCB_bRestartReq         = LW_FALSE;                           /*  û����������                */
    ptcb->TCB_pthreadStartAddress = pThread;                            /*  ����������ʼ����ָ��        */
#endif

    ptcb->TCB_ulCPUTicks       = 0ul;
    ptcb->TCB_ulCPUKernelTicks = 0ul;

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    ptcb->TCB_ulCPUUsageTicks       = 0ul;                              /*  CPU�����ʼ���               */
    ptcb->TCB_ulCPUUsageKernelTicks = 0ul;
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0                                     /*  ���Ź�������                */
    __WAKEUP_NODE_INIT(&ptcb->TCB_wunWatchDog);
#endif

    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineManage);
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringReady);

#if	LW_CFG_THREAD_POOL_EN > 0
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringThreadPool);                  /*  ����̳߳�ָ��              */
#endif

#if LW_CFG_EVENT_EN > 0
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringEvent);
    ptcb->TCB_peventPtr           = LW_NULL;                            /*  �������ȴ��¼�            */
    ptcb->TCB_ucIsEventDelete     = LW_EVENT_EXIST;                     /*  �¼�����                    */
    ptcb->TCB_ppringPriorityQueue = LW_NULL;                            /*  ��ʼ���޵ȴ�����            */
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
#if (LW_CFG_THREAD_DEL_EN > 0) || (LW_CFG_SIGNAL_EN > 0)
    ptcb->TCB_pesnPtr = LW_NULL;                                        /*  ָ����̵߳ȴ���            */
#endif                                                                  /*  EVENTSETNODE ��             */
#endif
    
    ptcb->TCB_ulThreadLockCounter = 0;                                  /*  �߳�������                  */
    
    if (ulOption & LW_OPTION_THREAD_SAFE) {
        ptcb->TCB_ulThreadSafeCounter = 1ul;                            /*  �����ʼ��Ϊ��ȫ״̬        */
    } else {
        ptcb->TCB_ulThreadSafeCounter = 0ul;                            /*  �����ʼ��Ϊ����ȫ״̬      */
    }

#if LW_CFG_THREAD_DEL_EN > 0
    ptcb->TCB_ptcbDeleteMe   = LW_NULL;                                 /*  û������ɾ�����������      */
    ptcb->TCB_ptcbDeleteWait = LW_NULL;
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    ptcb->TCB_plinePrivateVars  = LW_NULL;                              /*  ��ʼ��û��˽�б���          */
#endif
    
#if LW_CFG_COROUTINE_EN > 0
    pcrcb = &ptcb->TCB_crcbOrigent;
    pcrcb->COROUTINE_pstkStackTop     = pstkStackTop;
    pcrcb->COROUTINE_pstkStackBottom  = pstkStackButtom;
    pcrcb->COROUTINE_stStackSize      = stStackSize;
    pcrcb->COROUTINE_pstkStackLowAddr = pstkStackLowAddr;
    pcrcb->COROUTINE_ulThread         = ulId;
    pcrcb->COROUTINE_ulFlags          = 0ul;                            /*  ����Ҫ�����ͷ�              */

    ptcb->TCB_pringCoroutineHeader = LW_NULL;
    _List_Ring_Add_Ahead(&pcrcb->COROUTINE_ringRoutine,
                         &ptcb->TCB_pringCoroutineHeader);              /*  ����Э�̱�                  */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
    
#if LW_CFG_THREAD_CANCEL_EN > 0
    ptcb->TCB_bCancelRequest = LW_FALSE;                                /*  û������ CANCEL           */
    ptcb->TCB_iCancelState   = LW_THREAD_CANCEL_ENABLE;                 /*  PTHREAD_CANCEL_ENABLE       */
    ptcb->TCB_iCancelType    = LW_THREAD_CANCEL_DEFERRED;               /*  PTHREAD_CANCEL_DEFERRED     */
#endif
    
    if (ulOption & LW_OPTION_THREAD_DETACHED) {                         /*  ��ʹ�úϲ��㷨              */
        ptcb->TCB_bDetachFlag = LW_TRUE;
    } else {
        ptcb->TCB_bDetachFlag = LW_FALSE;                               /*  ʹ�úϲ�                    */
    }
    ptcb->TCB_ppvJoinRetValSave = LW_NULL;                              /*  ����ֵ��ַ                  */
    ptcb->TCB_plineJoinHeader   = LW_NULL;                              /*  û���̺߳ϲ��Լ�            */
    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineJoin);                        /*  û�кϲ������߳�            */
    ptcb->TCB_ptcbJoin = LW_NULL;                                       /*  û�кϲ��������߳�          */

    LW_SPIN_INIT(&ptcb->TCB_slLock);                                    /*  ��ʼ��������                */
    
#if LW_CFG_SMP_EN > 0
    if (ulOption & LW_OPTION_THREAD_NO_AFFINITY) {                      /*  �Ƿ�̳� CPU �׺Ͷȹ�ϵ     */
        ptcb->TCB_bCPULock  = LW_FALSE;
        ptcb->TCB_ulCPULock = 0ul;
    } else {
        ptcb->TCB_bCPULock  = ptcbCur->TCB_bCPULock;
        ptcb->TCB_ulCPULock = ptcbCur->TCB_ulCPULock;                   /*  �̳� CPU �׺Ͷȹ�ϵ         */
    }
    ptcb->TCB_ulCPUId = ptcb->TCB_ulCPULock;
#else
    ptcb->TCB_ulCPUId = 0ul;                                            /*  Ĭ��ʹ�� 0 �� CPU           */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcb->TCB_bIsCand = LW_FALSE;                                       /*  û�м������б�              */
    
#if LW_CFG_SMP_EN > 0
    ptcb->TCB_ptcbWaitStatus       = LW_NULL;
    ptcb->TCB_plineStatusReqHeader = LW_NULL;
    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineStatusPend);
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcb->TCB_uiStatusChangeReq = 0;
    ptcb->TCB_ulStopNesting     = 0ul;
    
#if (LW_CFG_DEVICE_EN > 0)                                              /*  �豸����                    */
    ptcb->TCB_pvIoEnv = LW_NULL;                                        /*  Ĭ��ʹ��ȫ�� io ����        */
    
    ptcb->TCB_iTaskStd[0] = 0;                                          /*  �� GLOBAL IN OUT ERR ��ͬ   */
    ptcb->TCB_iTaskStd[1] = 1;
    ptcb->TCB_iTaskStd[2] = 2;
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    lib_nlreent_init(ulId);                                             /*  init libc reent             */
#endif

#if LW_CFG_GDB_EN > 0
    ptcb->TCB_ulStepAddr       = LW_GDB_ADDR_INVAL;
    ptcb->TCB_ulStepInst       = 0ul;
    ptcb->TCB_bStepClear       = LW_TRUE;
    ptcb->TCB_ulAbortPointAddr = LW_GDB_ADDR_INVAL;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

#if LW_CFG_THREAD_NOTE_PAD_EN > 0                                       /*  ������±�                  */
    pulNote = &ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[0];
    for (ucI = 0; ucI < LW_CFG_MAX_NOTEPADS; ucI++) {
        *pulNote++ = 0ul;
    }
#endif

    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  ��ʼ��Ϊ�û��ռ�            */
    ptcb->TCB_pvPosixContext = LW_NULL;

    ptcb->TCB_usStatus = LW_THREAD_STATUS_RDY;                          /*  ����Ϊ����״̬              */

#if LW_CFG_THREAD_SUSPEND_EN > 0                                        /*  ����                        */
    if (ulOption & LW_OPTION_THREAD_SUSPEND) {
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
        ptcb->TCB_ulSuspendNesting = 1;                                 /*  ����                        */
    } else {
        ptcb->TCB_ulSuspendNesting = 0;
    }
#else
    ptcb->TCB_ulSuspendNesting = 0;
#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
    
    ptcb->TCB_usStatus |= LW_THREAD_STATUS_INIT;                        /*  ��ʼ��                      */

    if (LW_SYS_STATUS_IS_RUNNING()) {
        INT i;
        
        ptcb->TCB_uid  = ptcbCur->TCB_uid;
        ptcb->TCB_gid  = ptcbCur->TCB_gid;
        
        ptcb->TCB_euid = ptcbCur->TCB_euid;
        ptcb->TCB_egid = ptcbCur->TCB_egid;
        
        ptcb->TCB_suid = ptcbCur->TCB_suid;
        ptcb->TCB_sgid = ptcbCur->TCB_sgid;
    
        for (i = 0; i < ptcbCur->TCB_iNumSuppGid; i++) {                /*  ���渽������Ϣ              */
            ptcb->TCB_suppgid[i] = ptcbCur->TCB_suppgid[i];
        }
        ptcb->TCB_iNumSuppGid = ptcbCur->TCB_iNumSuppGid;
    
    } else {
        ptcb->TCB_uid  = 0;                                             /*  root Ȩ��                   */
        ptcb->TCB_gid  = 0;
        
        ptcb->TCB_euid = 0;
        ptcb->TCB_egid = 0;
        
        ptcb->TCB_suid = 0;
        ptcb->TCB_sgid = 0;
        
        ptcb->TCB_iNumSuppGid = 0;                                      /*  û�и�������Ϣ              */
    }

#if LW_CFG_CPU_FPU_EN > 0
    ptcb->TCB_pvStackFP = &ptcb->TCB_fpuctxContext;
    __ARCH_FPU_CTX_INIT(ptcb->TCB_pvStackFP);                           /*  ��ʼ����ǰ���� FPU ������   */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    ptcb->TCB_pvStackDSP = &ptcb->TCB_dspctxContext;
    __ARCH_DSP_CTX_INIT(ptcb->TCB_pvStackDSP);                          /*  ��ʼ����ǰ���� DSP ������   */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

#if LW_CFG_NET_EN > 0 && LW_CFG_NET_SAFE > 0
    ptcb->TCB_ulNetSem = LW_OBJECT_HANDLE_INVALID;
#endif                                                                  /*  LW_CFG_NET_SAFE > 0         */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    _K_ptcbTCBIdTable[_ObjectGetIndex(ulId)] = ptcb;                    /*  ����TCB���ƿ�               */
    _List_Line_Add_Ahead(&ptcb->TCB_lineManage, 
                         &_K_plineTCBHeader);                           /*  ���� TCB ��������           */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

#if LW_CFG_MODULELOADER_EN > 0                                          /*  ���Ϊ GLOBAL �����ڽ���  */
    if (ptcbCur && !(ulOption & LW_OPTION_OBJECT_GLOBAL)) {             /*  �̳н��̿��ƿ�              */
        ptcb->TCB_pvVProcessContext = ptcbCur->TCB_pvVProcessContext;
        vprocThreadAdd(ptcb->TCB_pvVProcessContext, ptcb);
    }
#endif

    bspTCBInitHook(ulId, ptcb);                                         /*  ���ù��Ӻ���                */
    __LW_THREAD_INIT_HOOK(ulId, ptcb);
    bspTaskCreateHook(ulId);                                            /*  ���ù��Ӻ���                */
    __LW_THREAD_CREATE_HOOK(ulId, ulOption);
}
/*********************************************************************************************************
** ��������: _TCBDestroy
** ��������: ����һ��TCB
** �䡡��  : ptcb              ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TCBDestroy (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (!(ptcb->TCB_ulOption & LW_OPTION_OBJECT_GLOBAL)) {
        vprocThreadDelete(ptcb->TCB_pvVProcessContext, ptcb);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: _TCBBuildExt
** ��������: ����һ��TCB��չģ��
** �䡡��  : ptcb              ������ƿ�
** �䡡��  : ERROR_NONE        ��ʾ��ȷ
**           ����ֵ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _TCBBuildExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
    REGISTER __PLW_THREAD_EXT  ptex = &ptcb->TCB_texExt;
    
    ptex->TEX_pbOnce         = LW_NULL;
    ptex->TEX_ulMutex        = 0ul;                                     /*  ��ʱ����Ҫ�ڲ�������        */
    ptex->TEX_pmonoCurHeader = LW_NULL;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _TCBDestroyExt
** ��������: ����һ��TCB��չģ��
** �䡡��  : ptcb              ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TCBDestroyExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
    REGISTER __PLW_THREAD_EXT   ptex = &ptcb->TCB_texExt;
    
    if (ptex->TEX_pbOnce) {
        *(ptex->TEX_pbOnce) = LW_FALSE;                                 /*  δ��ɵ� once ����          */
        ptex->TEX_pbOnce = LW_NULL;
    }
    if (ptex->TEX_ulMutex) {                                            /*  �Ƿ���Ҫɾ��������          */
        API_SemaphoreMDelete(&ptex->TEX_ulMutex);
    }
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
}
/*********************************************************************************************************
** ��������: _TCBCleanupPopExt
** ��������: ������ѹջ�������в��ͷ�
** �䡡��  : ptcb              ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TCBCleanupPopExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
             INTREG                 iregInterLevel;
    REGISTER __PLW_THREAD_EXT       ptex = &ptcb->TCB_texExt;
    REGISTER __PLW_CLEANUP_ROUTINE  pcur;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    while (ptex->TEX_pmonoCurHeader) {
        pcur = (__PLW_CLEANUP_ROUTINE)(ptex->TEX_pmonoCurHeader);       /*  ��һ��Ԫ��                  */
        _list_mono_next(&ptex->TEX_pmonoCurHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        
		LW_SOFUNC_PREPARE(pcur->CUR_pfuncClean);
        pcur->CUR_pfuncClean(pcur->CUR_pvArg);                          /*  ִ�����ٳ���                */
        __KHEAP_FREE(pcur);                                             /*  �ͷ�                        */
    
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
}
/*********************************************************************************************************
** ��������: _TCBTryRun
** ��������: �������õ� TCB �����������������
** �䡡��  : ptcb              ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _TCBTryRun (PLW_CLASS_TCB  ptcb)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */

    ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_INIT;                       /*  ȥ�� init ��־              */
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ����                        */
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���뵽������ȼ�������      */
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
