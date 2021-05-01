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
** ��   ��   ��: _Sched.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: ����ϵͳ�ں˵�������

** BUG
2008.01.04  �޸Ĵ����ʽ��ע��.
2008.04.06  ֧���߳������ķ���ֵ�Ĺ���.
2009.04.29  ���� SMP ֧��.
2011.02.22  ���� _SchedInt() �жϵ���(û�н���ǰ�ֳ�ѹջ), �������еĵ��ȶ�����������.
2012.09.07  �Ż�����.
2012.09.23  ���� IDLE ENTER �� IDLE EXIT �ص�����.
2013.07.17  ������ֻ���𱾺˵� cpu �ṹ��ֵ, ������ cpu ֻ���ú˼��ж�֪ͨ����.
2013.07.19  �ϲ� _SchedInt �� _SchedCoreInt ͨ�� Cur �� High ͬʱ�ж��Ƿ���Ҫ���ͺ˼��ж�.
2013.07.21  ������Ӧ�������ж��������Ƿ���Ҫ����, �����Ҫ, ���ͺ˼��ж�, Ȼ���ٴ����˵���.
2013.07.29  �����ѡ���б�������ȼ�����, �����ȴ������.
2013.08.28  �����ں��¼����.
2013.12.02  _SchedGetCandidate() ������ռ��������Ϊ 1.
2014.01.05  �������� BUG ���ٷ��ڴ˴�.
2014.01.07  ���� _SchedCrSwp, ͳһ������Э����ֲ�ĸ�ʽ.
2014.07.21  ���� CPU ֹͣ����.
2015.11.13  ���� spinlock bug trace.
2016.05.14  SMP ����֧����������˼��ж����ٵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  scheduler bug trace
*********************************************************************************************************/
#ifdef  __LW_SCHEDULER_BUG_TRACE_EN
#define __LW_SCHEDULER_BUG_TRACE(ptcb)  \
        _BugHandle((ptcb == LW_NULL), LW_TRUE, \
                   "scheduler candidate serious error, ptcb == NULL.\r\n"); \
        _BugFormat((!__LW_THREAD_IS_READY(ptcb)), LW_TRUE,  \
                   "scheduler candidate serious error, "    \
                   "ptcb %p, name \"%s\", status 0x%x.\r\n",    \
                   ptcb, ptcb->TCB_cThreadName, ptcb->TCB_usStatus);
#else
#define __LW_SCHEDULER_BUG_TRACE(ptcb)
#endif                                                                  /*  __LW_SCHEDULER_BUG_TRACE_EN */
/*********************************************************************************************************
  spinlock bug trace
*********************************************************************************************************/
#ifdef  __LW_SPINLOCK_BUG_TRACE_EN
#define __LW_SPINLOCK_BUG_TRACE(ptcb)   \
        _BugHandle((LW_CPU_SPIN_NESTING_GET(ptcb) > 0ul), LW_FALSE, \
                   "pend on spin lock nesting status!\r\n");
#else
#define __LW_SPINLOCK_BUG_TRACE(ptcb)
#endif                                                                  /*  __LW_SPINLOCK_BUG_TRACE_EN  */
/*********************************************************************************************************
  ����˽�б����л�
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
#define __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh)     _ThreadVarSwitch(ptcbCur, ptcbHigh)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_VAR(ptcbCur)                 _ThreadVarSave(ptcbCur)
#endif
#else
#define __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_VAR(ptcbCur)
#endif
#endif
/*********************************************************************************************************
  ���� FPU �������л�
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#define __LW_TASK_SWITCH_FPU(bIntSwitch)            _ThreadFpuSwitch(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_FPU(ptcbCur, bIntSwitch)     _ThreadFpuSave(ptcbCur, bIntSwitch)
#endif
#else
#define __LW_TASK_SWITCH_FPU(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_FPU(ptcbCur, bIntSwitch)
#endif
#endif
/*********************************************************************************************************
  ���� DSP �������л�
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0
#define __LW_TASK_SWITCH_DSP(bIntSwitch)            _ThreadDspSwitch(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_DSP(ptcbCur, bIntSwitch)     _ThreadDspSave(ptcbCur, bIntSwitch)
#endif
#else
#define __LW_TASK_SWITCH_DSP(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_DSP(ptcbCur, bIntSwitch)
#endif
#endif
/*********************************************************************************************************
  ����״̬Ǩ���ж�
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define __LW_STATUS_CHANGE_EN(ptcb, pcpu)           (!(pcpu)->CPU_ulInterNesting && \
                                                     (__THREAD_LOCK_GET(ptcb) <= 1ul))
/*********************************************************************************************************
  scheduler ֪ͨ����
*********************************************************************************************************/
static VOIDFUNCPTR                                  _K_pfuncSchedSmpNotify;
#define __LW_SMP_NOTIFY(ulCPUIdCur)                 _K_pfuncSchedSmpNotify(ulCPUIdCur)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: _SchedSmpNotify
** ��������: ֪ͨ��Ҫ���ȵ� CPU
** �䡡��  : ulCPUIdCur ��ǰ CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����һ���˲����˵���������, ����û���ڴ��� sched �˼��ж�, 
             ���ͺ˼��ж�.
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  _SchedSmpNotify (ULONG  ulCPUIdCur)
{
    INT             i;
    PLW_CLASS_CPU   pcpu;
    PLW_CLASS_TCB   ptcb;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUIdCur) {                      /*  ���� CPU ����Ƿ���Ҫ����   */
        pcpu = LW_CPU_GET(i);
        if (LW_CAND_ROT(pcpu) &&                                        /*  ��Ҫ������                */
            ((LW_CPU_GET_IPI_PEND(i) & LW_IPI_SCHED_MSK) == 0) &&       /*  û�к˼��жϱ�־            */
            !LW_ACCESS_ONCE(ULONG, pcpu->CPU_ulInterNesting)) {         /*  �����ж���                  */
            ptcb = LW_CAND_TCB(pcpu);
            if (LW_CPU_LOCK_QUICK_GET(pcpu) || !__THREAD_LOCK_GET(ptcb)) {
                _SmpSendIpi(i, LW_IPI_SCHED, 0, LW_TRUE);               /*  �����˼��ж�                */
            }
        }
    }
}
/*********************************************************************************************************
** ��������: _SchedSmpSmtNotify
** ��������: ֪ͨ��Ҫ���ȵ� CPU (smt balance sched)
** �䡡��  : ulCPUIdCur ��ǰ CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����һ���˲����˵���������, ����û���ڴ��� sched �˼��ж�, 
             ������ѡ������ CPU ���е� CPU ���ͺ˼��ж�.
*********************************************************************************************************/
#if LW_CFG_CPU_ARCH_SMT > 0

static VOID  _SchedSmpSmtNotify (ULONG  ulCPUIdCur)
{
    INT                i;
    PLW_CLASS_CPU      pcpu;
    PLW_CLASS_PHYCPU   pphycpu;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUIdCur) {                      /*  ���� CPU ����Ƿ���Ҫ����   */
        pcpu = LW_CPU_GET(i);
        if (LW_CAND_ROT(pcpu) &&
            ((LW_CPU_GET_IPI_PEND(i) & LW_IPI_SCHED_MSK) == 0)) {
            pphycpu = LW_PHYCPU_GET(pcpu->CPU_ulPhyId);
            if (pphycpu->PHYCPU_uiNonIdle == 0) {                       /*  ������ CPU ���� idle ����   */
                _SmpSendIpi(i, LW_IPI_SCHED, 0, LW_TRUE);               /*  �����˼��ж�                */
                return;
            }
        }
    }
    
    _SchedSmpNotify(ulCPUIdCur);                                        /*  ������ͨ SMP ����֪ͨ       */
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
** ��������: _SchedCpuDown
** ��������: CPU ֹͣ����
** �䡡��  : pcpuCur       ��ǰ CPU ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

static LW_INLINE VOID  _SchedCpuDown (PLW_CLASS_CPU  pcpuCur, BOOL  bIsIntSwitch)
{
    REGISTER PLW_CLASS_TCB  ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    REGISTER ULONG          ulCPUId = pcpuCur->CPU_ulCPUId;

    _CpuInactive(pcpuCur);                                              /*  ֹͣ CPU                    */
    
    __LW_TASK_SAVE_VAR(ptcbCur);
    __LW_TASK_SAVE_FPU(ptcbCur, bIsIntSwitch);
    __LW_TASK_SAVE_DSP(ptcbCur, bIsIntSwitch);
    
    __LW_SMP_NOTIFY(ulCPUId);                                           /*  �������� CPU ����           */
    
    pcpuCur->CPU_ulStatus &= ~LW_CPU_STATUS_RUNNING;
    
    LW_CPU_CLR_IPI_PEND2(pcpuCur, LW_IPI_DOWN_MSK);                     /*  ��� CPU �رձ�־           */
    
    LW_SPIN_KERN_UNLOCK_SCHED(ptcbCur);                                 /*  �����ں� spinlock           */

#if LW_CFG_CACHE_EN > 0
    if (!LW_KERN_REBOOT_CACHE_EN_GET()) {
        API_CacheDisable(DATA_CACHE);                                   /*  ���� CACHE                  */
        API_CacheDisable(INSTRUCTION_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

#if LW_CFG_VMM_EN > 0
    if (!LW_KERN_REBOOT_VMM_EN_GET()) {
        API_VmmMmuDisable();                                            /*  �ر� MMU                    */
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    LW_SPINLOCK_NOTIFY();
    bspCpuDown(ulCPUId);                                                /*  BSP ֹͣ CPU                */
    
    _BugHandle(LW_TRUE, LW_TRUE, "CPU Down error!\r\n");                /*  �������е�����              */
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: _SchedSwap
** ��������: arch �����л��������ȱ��浱ǰ�߳�������, Ȼ����ô˺���, Ȼ���ٻָ���Ҫִ�������������.
** �䡡��  : pcpuCur   ��ǰCPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID _SchedSwp (PLW_CLASS_CPU pcpuCur)
{
    REGISTER PLW_CLASS_TCB      ptcbCur      = pcpuCur->CPU_ptcbTCBCur;
    REGISTER PLW_CLASS_TCB      ptcbHigh     = pcpuCur->CPU_ptcbTCBHigh;
    REGISTER LW_OBJECT_HANDLE   ulCurId      = ptcbCur->TCB_ulId;
    REGISTER LW_OBJECT_HANDLE   ulHighId     = ptcbHigh->TCB_ulId;
             BOOL               bIsIntSwitch = pcpuCur->CPU_bIsIntSwitch;

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_SMP_CPU_DOWN_EN > 0)
    if (LW_CPU_GET_IPI_PEND(pcpuCur->CPU_ulCPUId) & LW_IPI_DOWN_MSK) {  /*  ��ǰ CPU ��Ҫֹͣ           */
        _SchedCpuDown(pcpuCur, bIsIntSwitch);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
    __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh);                            /*  �߳�˽�л������л�          */
    __LW_TASK_SWITCH_FPU(bIsIntSwitch);
    __LW_TASK_SWITCH_DSP(bIsIntSwitch);
    
    bspTaskSwapHook(ulCurId, ulHighId);                                 /*  ���� hook ����              */
    __LW_THREAD_SWAP_HOOK(ulCurId, ulHighId);
    
    if (_ObjectGetIndex(ulHighId) < LW_NCPUS) {                         /*  CPU �������ģʽ            */
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
        LW_PHYCPU_GET(pcpuCur->CPU_ulPhyId)->PHYCPU_uiNonIdle--;
#endif
        __LW_CPU_IDLE_ENTER_HOOK(ulCurId);
        
    } else if (_ObjectGetIndex(ulCurId) < LW_NCPUS) {                   /*  CPU �˳�����ģʽ            */
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
        LW_PHYCPU_GET(pcpuCur->CPU_ulPhyId)->PHYCPU_uiNonIdle++;
#endif
        __LW_CPU_IDLE_EXIT_HOOK(ulHighId);
    }
    
#if LW_CFG_SMP_EN > 0
    __LW_SMP_NOTIFY(pcpuCur->CPU_ulCPUId);                              /*  SMP ����֪ͨ                */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcpuCur->CPU_ptcbTCBCur = ptcbHigh;                                 /*  �л�����                    */

#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
    LW_SPIN_KERN_UNLOCK_SCHED(ptcbCur);                                 /*  �����ں� spinlock           */
#endif
    
    if (bIsIntSwitch) {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SCHED, MONITOR_EVENT_SCHED_INT, 
                          ulCurId, ulHighId, LW_NULL);
    } else {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SCHED, MONITOR_EVENT_SCHED_TASK,
                          ulCurId, ulHighId, LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: _CoroutineDeleteAll
** ��������: �ͷ�ָ���߳����е�Э�̿ռ�.
** �䡡��  : ptcb                     �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID _SchedCrSwp (PLW_CLASS_CPU pcpuCur)
{
#if LW_CFG_COROUTINE_EN > 0
    pcpuCur->CPU_pcrcbCur = pcpuCur->CPU_pcrcbNext;                     /*  �л�Э��                    */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
}
/*********************************************************************************************************
** ��������: _SchedSafeStack
** ��������: arch �����л�������ð�ȫ��ջλ��.
** �䡡��  : pcpuCur   ��ǰCPU
** �䡡��  : ��ջλ��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

PVOID _SchedSafeStack (PLW_CLASS_CPU pcpuCur)
{
    return  (pcpuCur->CPU_pstkInterBase);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: Schedule
** ��������: �ں��˳�ʱ, ����ô˵��Ⱥ��� (�����ں�״̬�����жϱ�����)
** �䡡��  : NONE
** �䡡��  : �߳������ķ���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _Schedule (VOID)
{
    ULONG            ulCPUId;
    PLW_CLASS_CPU    pcpuCur;
    PLW_CLASS_TCB    ptcbCur;
    PLW_CLASS_TCB    ptcbCand;
    INT              iRetVal = ERROR_NONE;
    
    ulCPUId = LW_CPU_GET_CUR_ID();                                      /*  ��ǰ CPUID                  */
    pcpuCur = LW_CPU_GET(ulCPUId);                                      /*  ��ǰ CPU ���ƿ�             */
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    
#if LW_CFG_SMP_EN > 0
    if (LW_UNLIKELY(ptcbCur->TCB_plineStatusReqHeader)) {               /*  ����ǰ����ı�״̬        */
        if (__LW_STATUS_CHANGE_EN(ptcbCur, pcpuCur)) {                  /*  �Ƿ���Խ���״̬�л�        */
            _ThreadStatusChangeCur(pcpuCur);                            /*  ����Ƿ���Ҫ����״̬�л�    */
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcbCand = _SchedGetCand(pcpuCur, 1ul);                             /*  �����Ҫ���е��߳�          */
    if (ptcbCand != ptcbCur) {                                          /*  ����뵱ǰ���еĲ�ͬ, �л�  */
        __LW_SCHEDULER_BUG_TRACE(ptcbCand);                             /*  ������ BUG ���             */
        
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ϵͳ                    */
        __LW_SPINLOCK_BUG_TRACE(pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        pcpuCur->CPU_bIsIntSwitch = LW_FALSE;                           /*  ���жϵ���                  */
        pcpuCur->CPU_ptcbTCBHigh  = ptcbCand;
        
        /*
         *  TASK CTX SAVE();
         *  SWITCH to SAFE stack if in SMP system;
         *  _SchedSwp();
         *  TASK CTX LOAD();
         */
        archTaskCtxSwitch(pcpuCur);                                     /*  �߳��л�,���ͷ��ں�������   */
#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
        LW_SPIN_KERN_LOCK_IGNIRQ();                                     /*  �ں����������¼���          */
#endif                                                                  /*  LW_CFG_CORTEX_M_SVC_SWITCH  */
    }
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ϵͳ                    */
      else {
        __LW_SMP_NOTIFY(ulCPUId);                                       /*  SMP ����֪ͨ                */
        return  (iRetVal);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ����µĵ�ǰ TCB            */
    
    iRetVal = ptcbCur->TCB_iSchedRet;                                   /*  ��õ������źŵķ���ֵ      */
    ptcbCur->TCB_iSchedRet = ERROR_NONE;                                /*  ���                        */
    
    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: _ScheduleInt
** ��������: �ж��˳�ʱ, ����ô˵��Ⱥ��� (�����ں�״̬�����жϱ�����)
** �䡡��  : pcpuCur    ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ScheduleInt (PLW_CLASS_CPU  pcpuCur)
{
    PLW_CLASS_TCB    ptcbCur;
    PLW_CLASS_TCB    ptcbCand;
    
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    
#if LW_CFG_SMP_EN > 0
    if (__LW_STATUS_CHANGE_EN(ptcbCur, pcpuCur)) {                      /*  �Ƿ���Խ���״̬�л�        */
        if (LW_UNLIKELY(ptcbCur->TCB_plineStatusReqHeader)) {           /*  ����ǰ����ı�״̬        */
            _ThreadStatusChangeCur(pcpuCur);                            /*  ����Ƿ���Ҫ����״̬�л�    */
        }
        
#if LW_CFG_SMP_CPU_DOWN_EN > 0
        if (LW_CPU_GET_IPI_PEND(pcpuCur->CPU_ulCPUId) & 
            LW_IPI_DOWN_MSK) {                                          /*  ��ǰ CPU ��Ҫֹͣ           */
            _SchedCpuDown(pcpuCur, LW_TRUE);
        }
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcbCand = _SchedGetCand(pcpuCur, 1ul);                             /*  �����Ҫ���е��߳�          */
    if (ptcbCand != ptcbCur) {                                          /*  ����뵱ǰ���еĲ�ͬ, �л�  */
        __LW_SCHEDULER_BUG_TRACE(ptcbCand);                             /*  ������ BUG ���             */

#if LW_CFG_SMP_EN > 0                                                   /*  SMP ϵͳ                    */
        __LW_SPINLOCK_BUG_TRACE(pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        pcpuCur->CPU_bIsIntSwitch = LW_TRUE;                            /*  �жϵ���                    */
        pcpuCur->CPU_ptcbTCBHigh  = ptcbCand;
        
#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
        _SchedSwp(pcpuCur);                                             /*  ֱ�ӵ��� _SchedSwp()        */
#endif                                                                  /*  LW_CFG_CORTEX_M_SVC_SWITCH  */
        /*
         *  TASK CTX LOAD();
         */
        archIntCtxLoad(pcpuCur);                                        /*  �ж����������߳��л�        */
#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
        _BugHandle(LW_TRUE, LW_TRUE, "serious error!\r\n");
#endif                                                                  /*  LW_CFG_CORTEX_M_SVC_SWITCH  */
    }
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ϵͳ                    */
      else {
        __LW_SMP_NOTIFY(LW_CPU_GET_ID(pcpuCur));                        /*  SMP ����֪ͨ                */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}
/*********************************************************************************************************
** ��������: _ScheduleIntCheck
** ��������: �ж��˳�ʱ, �ɵ��ô˵��Ⱥ����ж��Ƿ���Ҫ���� (�����ں�״̬�����жϱ�����)
** �䡡��  : pcpuCur    ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)

BOOL  _ScheduleIntCheck (PLW_CLASS_CPU  pcpuCur)
{
    PLW_CLASS_TCB    ptcbCur;
    PLW_CLASS_TCB    ptcbCand;
    BOOL             bNeedSched;

    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    
#if LW_CFG_SMP_EN > 0
    if (__LW_STATUS_CHANGE_EN(ptcbCur, pcpuCur)) {                      /*  �Ƿ���Խ���״̬�л�        */
        if (LW_UNLIKELY(ptcbCur->TCB_plineStatusReqHeader)) {           /*  ����ǰ����ı�״̬        */
            _ThreadStatusChangeCur(pcpuCur);                            /*  ����Ƿ���Ҫ����״̬�л�    */
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcbCand = _SchedGetCand(pcpuCur, 1ul);                             /*  �����Ҫ���е��߳�          */
    if (ptcbCand != ptcbCur) {                                          /*  ����뵱ǰ���еĲ�ͬ, �л�  */
        bNeedSched = LW_TRUE;
    } else {
        bNeedSched = LW_FALSE;
    }
    
    return  (bNeedSched);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
** ��������: _ScheduleInit
** ��������: ��ʼ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ScheduleInit (VOID)
{
#if LW_CFG_SMP_EN > 0
#if LW_CFG_CPU_ARCH_SMT > 0
    if (LW_KERN_SMT_BSCHED_EN_GET()) {
        _K_pfuncSchedSmpNotify = _SchedSmpSmtNotify;
        
    } else 
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
    {
        _K_pfuncSchedSmpNotify = _SchedSmpNotify;
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: _SchedSetRet
** ��������: ���õ�ǰ�������������ֵ, �ڲ�����������ʱ, ����ȡ���ֵ (���ж�����±�����)
** �䡡��  : iSchedRet         ����������ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SchedSetRet (INT  iSchedSetRet)
{
    PLW_CLASS_TCB    ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);
    if (ptcbCur->TCB_iSchedRet == ERROR_NONE) {
        ptcbCur->TCB_iSchedRet =  iSchedSetRet;
    }
}
/*********************************************************************************************************
** ��������: _SchedSetPrio
** ��������: ����ָ���������ȼ� (�����ں�״̬�±�����)
** �䡡��  : ptcb           ����
**           ucPriority     ���ȼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ǵ�ǰ���ں�ѡ����, ֻҪ����Ŀ���������� CPU ��ѡ����Ʊ�Ǽ���, ��ǰ����������ʱ, ����
             �����������Ʊ�־, ���½���һ����ռ���Ȳ���, ����и��Ӻ��ʵ��������, ����ռĿ������.
*********************************************************************************************************/
VOID  _SchedSetPrio (PLW_CLASS_TCB  ptcb, UINT8  ucPriority)
{
    INTREG           iregInterLevel;
    PLW_CLASS_PCB    ppcbFrom;
    PLW_CLASS_PCB    ppcbTo;
    
    ppcbFrom = _GetPcb(ptcb);
    ppcbTo   = _GetPcbEx(ptcb, ucPriority);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_PRIO,
                      ptcb->TCB_ulId, ptcb->TCB_ucPriority, ucPriority, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  �߳̾���                    */
        if (ptcb->TCB_bIsCand) {                                        /*  �ں�ѡ����                  */
            LW_CAND_ROT(LW_CPU_GET(ptcb->TCB_ulCPUId)) =  LW_TRUE;      /*  �˳��ں�ʱ������ռ����      */
            ptcb->TCB_ucPriority = ucPriority;                          /*  ֱ�������µ����ȼ�          */
            
        } else {                                                        /*  ���ں�ѡ����                */
            __DEL_FROM_READY_RING(ptcb, ppcbFrom);                      /*  �Ӿ�������ɾ��              */
            
            ptcb->TCB_ucPriority = ucPriority;                          /*  �����µ����ȼ�              */
            __ADD_TO_READY_RING(ptcb, ppcbTo);                          /*  �����µľ�����              */
        }
    } else {
        ptcb->TCB_ucPriority = ucPriority;                              /*  ֱ�������µ����ȼ�          */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
