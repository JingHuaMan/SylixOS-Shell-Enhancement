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
** ��   ��   ��: _EventHighLevel.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 19 ��
**
** ��        ��: ����ϵͳ�¼����и߼�������.

** BUG
2007.04.08  ɾ���� _EventMakeThreadReadyFIFO_LowLevel() �� _EventMakeThreadReadyPRIO_LowLevel() �� 
            ppcb�������������û����.
2008.03.30  ʹ���µľ���������.
2008.03.30  �����µ� wake up ���ƵĴ���.
2008.05.18  ���� _EventMakeThreadWait????() ���ڹ��ж��н��е�, ���Խ�ʱ����ڱ�־�Ĵ����������, ������Ϊ
            ��ȫ.
2008.11.30  �����ļ�, �޸�ע��.
2009.04.28  _EventMakeThreadReady_HighLevel() �޸Ŀ����ж�ʱ��.
2010.08.03  ��Ӳ���ע��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.09.02  �ȴ����͸�Ϊ 16 λ.
2014.01.14  �޸ĳ�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventWaitFifo
** ��������: ��һ���̼߳��� FIFO �¼��ȴ�����,ͬʱ������ر�־λ (�����ں��ҹ��ж�״̬�±�����)
** �䡡��  : pevent        �¼�
**           ppringList    �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

VOID  _EventWaitFifo (PLW_CLASS_EVENT  pevent, PLW_LIST_RING  *ppringList)
{
    REGISTER PLW_CLASS_PCB    ppcb;
             PLW_CLASS_TCB    ptcbCur;
             
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��ǰ������ƿ�              */
    
    ptcbCur->TCB_peventPtr = pevent;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ���������ɾ��            */
    _AddTCBToEventFifo(ptcbCur, pevent, ppringList);                    /*  ����ȴ�����                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
    }
    
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;                      /*  �¼�����                    */
}
/*********************************************************************************************************
** ��������: _EventWaitPriority
** ��������: ��һ���̼߳������ȼ��¼��ȴ�����,ͬʱ������ر�־λ (�����ں��ҹ��ж�״̬�±�����)
** �䡡��  : pevent        �¼�
**           ppringList    �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _EventWaitPriority (PLW_CLASS_EVENT  pevent, PLW_LIST_RING  *ppringList)
{
    REGISTER PLW_CLASS_PCB    ppcb;
             PLW_CLASS_TCB    ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��ǰ������ƿ�              */
    
    ptcbCur->TCB_peventPtr = pevent;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ���������ɾ��            */
    _AddTCBToEventPriority(ptcbCur, pevent, ppringList);                /*  ����ȴ�����                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
    }
    
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;                      /*  �¼�����                    */
}
/*********************************************************************************************************
** ��������: _EventReadyFifoLowLevel
** ��������: ��һ���̴߳� FIFO �¼��ȴ������н���������, ͬʱ������ر�־λ
** �䡡��  : pevent            �¼�
**           pvMsgBoxMessage   ��չ��Ϣ
**           ppringList        �ȴ�����
** �䡡��  : �����������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_TCB  _EventReadyFifoLowLevel (PLW_CLASS_EVENT  pevent, 
                                        PVOID            pvMsgBoxMessage, 
                                        PLW_LIST_RING   *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    
    ptcb = _EventQGetTcbFifo(pevent, ppringList);                       /*  ������Ҫ������߳�          */
    
    _DelTCBFromEventFifo(ptcb, pevent, ppringList);
    
    ptcb->TCB_pvMsgBoxMessage = pvMsgBoxMessage;                        /*  ���ݶ������ź�������Ϣ    */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �˳��ȴ�����                */
        ptcb->TCB_ulDelay = 0ul;
    }
    
    return  (ptcb);
}
/*********************************************************************************************************
** ��������: _EventReadyPriorityLowLevel
** ��������: ��һ���̴߳����ȼ��¼��ȴ������н���������, ͬʱ������ر�־λ
** �䡡��  : pevent            �¼�
**           pvMsgBoxMessage   ��չ��Ϣ
**           ppringList        �ȴ�����
** �䡡��  : �����������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_TCB  _EventReadyPriorityLowLevel (PLW_CLASS_EVENT  pevent, 
                                            PVOID            pvMsgBoxMessage, 
                                            PLW_LIST_RING   *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    
    ptcb = _EventQGetTcbPriority(pevent, ppringList);                   /*  ������Ҫ������߳�          */
    
    _DelTCBFromEventPriority(ptcb, pevent, ppringList);
    
    ptcb->TCB_pvMsgBoxMessage = pvMsgBoxMessage;                        /*  ���ݶ������ź�������Ϣ    */

    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �˳��ȴ�����                */
        ptcb->TCB_ulDelay = 0ul;
    }

    return  (ptcb);
}
/*********************************************************************************************************
** ��������: _EventReadyHighLevel
** ��������: ��һ���̴߳� FIFO �¼��ȴ������н���������, ͬʱ������ر�־λ, ��Ҫʱ���������.
**           �˺������ں�����״̬������.
** �䡡��  : ptcb          ������ƿ�
**           usWaitType    �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                      �˺����ڿ��ж�ʱ������.
*********************************************************************************************************/
VOID  _EventReadyHighLevel (PLW_CLASS_TCB    ptcb, UINT16   usWaitType)
{
             INTREG           iregInterLevel;
    REGISTER PLW_CLASS_PCB    ppcb;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb->TCB_peventPtr = LW_NULL;
    
    if (ptcb->TCB_ucWaitTimeout) {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  �����ʱλ                  */
    
    } else {                                                            /*  �����Ӧ�ȴ�λ              */
        ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & (~usWaitType));
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  �Ƿ����                    */
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;         /*  �жϼ��ʽ                */
            ppcb = _GetPcb(ptcb);
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  ���뵽������ȼ�������      */
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                      */
}
/*********************************************************************************************************
** ��������: _EventPrioTryBoost
** ��������: �����ź�������ӵ���������ȼ�.
**           �˺������ں�����״̬������.
** �䡡��  : pevent        �¼�
**           ptcbCur       ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _EventPrioTryBoost (PLW_CLASS_EVENT  pevent, PLW_CLASS_TCB   ptcbCur)
{
    PLW_CLASS_TCB    ptcbOwner = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;
    
    if (ptcbOwner->TCB_iDeleteProcStatus) {                             /*  �����ѱ�ɾ�������ڱ�ɾ��    */
        return;
    }
    
    if (LW_PRIO_IS_HIGH(ptcbCur->TCB_ucPriority, 
                        ptcbOwner->TCB_ucPriority)) {                   /*  ��Ҫ�ı����ȼ�              */
        if (pevent->EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY) {      /*  ���ȼ��̳�                  */
            _SchedSetPrio(ptcbOwner, ptcbCur->TCB_ucPriority);
        
        } else if (LW_PRIO_IS_HIGH(pevent->EVENT_ucCeilingPriority,
                                   ptcbOwner->TCB_ucPriority)) {        /*  ���ȼ��컨��                */
            _SchedSetPrio(ptcbOwner, pevent->EVENT_ucCeilingPriority);
        }
    }
}
/*********************************************************************************************************
** ��������: _EventPrioTryResume
** ��������: �����ź������͵�ǰ�������ȼ�.
**           �˺������ں�����״̬������.
** �䡡��  : pevent        �¼�
**           ptcbCur       ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _EventPrioTryResume (PLW_CLASS_EVENT  pevent, PLW_CLASS_TCB   ptcbCur)
{
    UINT8   ucPriorityOld = (UINT8)pevent->EVENT_ulMaxCounter;
    
    if (!LW_PRIO_IS_EQU(ptcbCur->TCB_ucPriority, ucPriorityOld)) {      /*  ���������ȼ��任            */
        _SchedSetPrio(ptcbCur, ucPriorityOld);                          /*  �������ȼ�                  */
    }
}
/*********************************************************************************************************
** ��������: _EventUnQueue
** ��������: ��һ���̴߳��¼��ȴ������н���
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : �¼����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��������ȼ��ȴ�����, ���ڵȴ������п��������ȼ��ĸı�, ���Բ��ܿ���ǰ���ȼ����ж϶��е�λ��
*********************************************************************************************************/
PLW_CLASS_EVENT  _EventUnQueue (PLW_CLASS_TCB    ptcb)
{
    REGISTER PLW_CLASS_EVENT    pevent;
    REGISTER PLW_LIST_RING     *ppringList;
    
    pevent = ptcb->TCB_peventPtr;
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  ���ȼ�����                  */
        ppringList = ptcb->TCB_ppringPriorityQueue;                     /*  ȷ���ȴ�����λ��            */
        _DelTCBFromEventPriority(ptcb, pevent, ppringList);             /*  �Ӷ������Ƴ�                */
        
    } else {                                                            /*  FIFO ����                   */
        _EVENT_FIFO_Q_PTR(ptcb->TCB_iPendQ, ppringList);
        _DelTCBFromEventFifo(ptcb, pevent, ppringList);                 /*  �Ӷ������Ƴ�                */
    }
    
    ptcb->TCB_peventPtr = LW_NULL;                                      /*  ����¼�                    */
    
    return  (pevent);
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
