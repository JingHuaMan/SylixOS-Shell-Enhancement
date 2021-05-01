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
** ��   ��   ��: _EventSetReady.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: ����ȴ��¼������е��߳�

** BUG
2008.03.29  �����µ� wake up ����.
2008.03.30  ʹ���µľ���������.
2009.07.03  bIsSched Ӧ�ñ���ʼ��Ϊ LW_FALSE.
2012.07.04  �ϲ��ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventSetDeleteReady
** ��������: �����¼���ɾ��������ȴ��¼������е��߳� (�����ں˲����жϺ󱻵���)
** �䡡��  : pesn      �¼�����ƿ�
** �䡡��  : �Ƿ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

BOOL  _EventSetDeleteReady (PLW_CLASS_EVENTSETNODE    pesn)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    REGISTER PLW_CLASS_PCB    ppcb;
    REGISTER BOOL             bIsSched = LW_FALSE;
    
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                        /*  �¼���ɾ����                */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  ������ wake up ����         */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
    }
    
    ptcb->TCB_ulDelay      = 0ul;
    ptcb->TCB_ulEventSets  = 0ul;
    ptcb->TCB_usStatus    &= (~LW_THREAD_STATUS_EVENTSET);
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  �Ƿ����                    */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
        bIsSched = LW_TRUE;
    }
    
    _EventSetUnQueue(pesn);
    
    return  (bIsSched);
}
/*********************************************************************************************************
** ��������: _EventSetThreadReady
** ��������: ����ȴ��¼������е��߳� (�����ں˲����жϺ󱻵���)
** �䡡��  : pesn              �¼�����ƿ�
**           ulEventsReady     �µ��¼���־
** �䡡��  : �Ƿ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _EventSetThreadReady (PLW_CLASS_EVENTSETNODE    pesn,
                            ULONG                     ulEventsReady)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    REGISTER PLW_CLASS_PCB    ppcb;
    REGISTER BOOL             bIsSched = LW_FALSE;
    
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  ������ wake up ����         */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
    }
    
    ptcb->TCB_ulDelay      = 0ul;
    ptcb->TCB_ulEventSets  = ulEventsReady;
    ptcb->TCB_usStatus    &= (~LW_THREAD_STATUS_EVENTSET);
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  �Ƿ����                    */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
        bIsSched = LW_TRUE;
    }
    
    _EventSetUnQueue(pesn);
    
    return  (bIsSched);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
