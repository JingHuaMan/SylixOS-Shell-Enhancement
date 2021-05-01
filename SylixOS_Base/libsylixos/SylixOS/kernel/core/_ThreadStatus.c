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
** ��   ��   ��: _ThreadStat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 02 ��
**
** ��        ��: �߳�״̬�޸�.

** BUG:
2013.12.11  ���� _ThreadUnwaitStatus() ɾ�����������߳�ʱ, ������ڵȴ������̸߳ı�״̬, ����Ҫ�˳�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadWaitStatus
** ��������: �ȴ�Ŀ���߳�״̬������� (������������ں˲����ж�״̬������)
** �䡡��  : ptcbCur       ��ǰ�߳̿��ƿ�
**           ptcbDest      Ŀ���߳̿��ƿ�
**           uiStatusReq   ����ı��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  _ThreadWaitStatus (PLW_CLASS_TCB  ptcbCur, PLW_CLASS_TCB  ptcbDest, UINT  uiStatusReq)
{
    PLW_CLASS_PCB  ppcb;

    if (__LW_THREAD_IS_READY(ptcbCur)) {
        ppcb = _GetPcb(ptcbCur);
        __DEL_FROM_READY_RING(ptcbCur, ppcb);                           /*  �Ӿ�������ɾ��              */
    }
    
    ptcbCur->TCB_ptcbWaitStatus     = ptcbDest;
    ptcbCur->TCB_uiStatusChangeReq  = uiStatusReq;                      /*  �����޸ĵ�״̬              */
    ptcbCur->TCB_usStatus          |= LW_THREAD_STATUS_WSTAT;
    
    _List_Line_Add_Ahead(&ptcbCur->TCB_lineStatusPend,
                         &ptcbDest->TCB_plineStatusReqHeader);
    KN_SMP_MB();                                                        /*  ȷ�������Ѹ���              */
}
/*********************************************************************************************************
** ��������: _ThreadUnwaitStatus
** ��������: �ӵȴ�Ŀ���߳�״̬�����������˳�, ��������������ȴ��ҵ�״̬�ı�.(�����ں˲����ж�״̬������)
** �䡡��  : ptcbCur       ��ǰ�߳̿��ƿ�
**           ptcbDest      Ŀ���߳̿��ƿ�
**           uiStatusReq   ����ı��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ֻ���˳�����, ��������������.
*********************************************************************************************************/
VOID  _ThreadUnwaitStatus (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_TCB  ptcbDest = ptcb->TCB_ptcbWaitStatus;
    PLW_CLASS_TCB  ptcbWait;
    PLW_CLASS_PCB  ppcb;

    if (ptcbDest) {
        _List_Line_Del(&ptcb->TCB_lineStatusPend, 
                       &ptcbDest->TCB_plineStatusReqHeader);
                       
        ptcb->TCB_ptcbWaitStatus    = LW_NULL;
        ptcb->TCB_uiStatusChangeReq = 0;
    }
    
    while (ptcb->TCB_plineStatusReqHeader) {                            /*  �������߳�����              */
        ptcbWait = _LIST_ENTRY(ptcb->TCB_plineStatusReqHeader, 
                               LW_CLASS_TCB, 
                               TCB_lineStatusPend);
        ptcbWait->TCB_ptcbWaitStatus = LW_NULL;
        
        _List_Line_Del(&ptcbWait->TCB_lineStatusPend, 
                       &ptcb->TCB_plineStatusReqHeader);
        
        if (!__LW_THREAD_IS_READY(ptcbWait)) {                          /*  ���û�о���, ȡ�� WSTAT    */
            ptcbWait->TCB_usStatus &= ~LW_THREAD_STATUS_WSTAT;
            if (__LW_THREAD_IS_READY(ptcbWait)) {
                ptcbWait->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  �жϼ��ʽ                */
                ppcb = _GetPcb(ptcbWait);
                __ADD_TO_READY_RING(ptcbWait, ppcb);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: _ThreadStatusChangeCur
** ��������: �ı䵱ǰ�߳�״̬ (�ں�״̬�ҹر��ж�״̬������)
** �䡡��  : ptcbCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID  _ThreadStatusChangeCur (PLW_CLASS_CPU    pcpuCur)
{
    PLW_CLASS_TCB  ptcbCur;
    PLW_CLASS_TCB  ptcb;
    PLW_CLASS_PCB  ppcb;

    ptcbCur = pcpuCur->CPU_ptcbTCBCur;                                  /*  ��ǰ�߳�                    */

    if (ptcbCur->TCB_plineStatusReqHeader) {                            /*  ��Ҫ����                    */
        if (__LW_THREAD_IS_READY(ptcbCur)) {
            ppcb = _GetPcb(ptcbCur);
            __DEL_FROM_READY_RING(ptcbCur, ppcb);                       /*  �Ӿ�������ɾ��              */
        }
        
        do {
            ptcb = _LIST_ENTRY(ptcbCur->TCB_plineStatusReqHeader, LW_CLASS_TCB, TCB_lineStatusPend);
            
            switch (ptcb->TCB_uiStatusChangeReq) {
            
            case LW_TCB_REQ_SUSPEND:
                ptcbCur->TCB_ulSuspendNesting++;
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
                break;
                
            case LW_TCB_REQ_STOP:
                ptcbCur->TCB_ulStopNesting++;
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_STOP;
                break;
                
            case LW_TCB_REQ_WDEATH:
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_WDEATH;
                break;
            }
            
            ptcb->TCB_ptcbWaitStatus    = LW_NULL;
            ptcb->TCB_uiStatusChangeReq = 0;                            /*  �����޸ĵ�״̬�ɹ�          */
            
            _List_Line_Del(&ptcb->TCB_lineStatusPend, 
                           &ptcbCur->TCB_plineStatusReqHeader);
            
            if (!__LW_THREAD_IS_READY(ptcb)) {                          /*  ���û�о���, ȡ�� WSTAT    */
                ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_WSTAT;
                if (__LW_THREAD_IS_READY(ptcb)) {
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  �жϼ��ʽ                */
                    ppcb = _GetPcb(ptcb);
                    __ADD_TO_READY_RING(ptcb, ppcb);
                }
            }
        } while (ptcbCur->TCB_plineStatusReqHeader);
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: _ThreadStatusChange
** ��������: �ı��߳�״̬ (�����ں�״̬������)
** �䡡��  : ptcb          �߳̿��ƿ�
**           uiStatusReq   ״̬ (LW_TCB_REQ_SUSPEND / LW_TCB_REQ_WDEATH / LW_TCB_REQ_STOP)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����Ѿ�Ԥ�õ�������, ���һ���˳��ں�ʱ, �������ȴ�Ŀ���߳�״̬�ı����.
*********************************************************************************************************/
ULONG  _ThreadStatusChange (PLW_CLASS_TCB  ptcb, UINT  uiStatusReq)
{
    INTREG         iregInterLevel;
    PLW_CLASS_PCB  ppcb;
    PLW_CLASS_TCB  ptcbCur;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    
#if LW_CFG_SMP_EN > 0
    if (ptcb == ptcbCur) {                                              /*  �ı��Լ���״̬              */
        goto    __change1;
    
    } else if (!__LW_THREAD_IS_READY(ptcb)) {                           /*  ����û�о���                */
        goto    __change2;
    
    } else {                                                            /*  Ŀ�������ھ���״̬        */
        if (LW_CPU_GET_CUR_NESTING()) {
            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */
            return  (ERROR_KERNEL_IN_ISR);                              /*  �ж�״̬���޷���ɴ˹���    */
        }
        
        if (!__LW_THREAD_IS_RUNNING(ptcb)) {                            /*  Ŀ������������ִ��        */
            ppcb = _GetPcb(ptcb);
            __DEL_FROM_READY_RING(ptcb, ppcb);                          /*  �Ӿ�������ɾ��              */
            goto    __change2;
        
        } else {                                                        /*  Ŀ����������ִ��            */
            _ThreadWaitStatus(ptcbCur, ptcb, uiStatusReq);              /*  ���õȴ��Է����״̬        */
            _SmpUpdateIpi(LW_CPU_GET(ptcb->TCB_ulCPUId));               /*  ���ͺ˼��ж�֪ͨ�ı�״̬    */
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return  (ERROR_NONE);                                           /*  �˳��ں�ʱ��ʼ�ȴ�          */
    }
    
__change1:
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        ppcb = _GetPcb(ptcb);
        __DEL_FROM_READY_RING(ptcb, ppcb);                              /*  �Ӿ�������ɾ��              */
    }
    
#if LW_CFG_SMP_EN > 0
__change2:
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    ptcbCur->TCB_uiStatusChangeReq = 0;                                 /*  ״̬�޸Ĳ���ִ�гɹ�        */
    
    switch (uiStatusReq) {
    
    case LW_TCB_REQ_SUSPEND:
        ptcb->TCB_ulSuspendNesting++;
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
        break;
        
    case LW_TCB_REQ_STOP:
        ptcb->TCB_ulStopNesting++;
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_STOP;
        break;
        
    case LW_TCB_REQ_WDEATH:
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_WDEATH;
        break;
    }
        
    KN_INT_ENABLE(iregInterLevel);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
