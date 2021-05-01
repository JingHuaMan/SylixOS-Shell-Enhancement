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
** ��   ��   ��: _EventQueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 19 ��
**
** ��        ��: ����ϵͳ�¼����й�������

** BUG
2008.11.30  �����ļ�, �޸�ע��.
2009.04.28  �����ļ�, �޸�ע��.
2014.01.14  �޸ĳ�������.
2016.07.29  ���� _AddTCBToEventPriority() �������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _AddTCBToEventFifo
** ��������: ��һ���̼߳��� FIFO �¼��ȴ�����
** �䡡��  : ptcb           ������ƿ�
**           pevent         �¼����ƿ�
**           ppringList     �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

VOID  _AddTCBToEventFifo (PLW_CLASS_TCB    ptcb, 
                          PLW_CLASS_EVENT  pevent, 
                          PLW_LIST_RING   *ppringList)
{
    _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);             /*  ���뵽 FIFO ����ͷ          */
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum++;                 /*  �ȴ��¼��ĸ���++            */
}
/*********************************************************************************************************
** ��������: _DelTCBFromEventFifo
** ��������: �� FIFO �¼��ȴ�������ɾ��һ���߳�
** �䡡��  : ptcb           ������ƿ�
**           pevent         �¼����ƿ�
**           ppringList     �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _DelTCBFromEventFifo (PLW_CLASS_TCB      ptcb, 
                            PLW_CLASS_EVENT    pevent, 
                            PLW_LIST_RING     *ppringList)
{
    _List_Ring_Del(&ptcb->TCB_ringEvent, ppringList);
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum--;                 /*  �ȴ��¼��ĸ���--            */
}
/*********************************************************************************************************
** ��������: _AddTCBToEventPriority
** ��������: ��һ���̼߳��� PRIORITY �¼��ȴ�����
** �䡡��  : ptcb           ������ƿ�
**           pevent         �¼����ƿ�
**           ppringList     �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _AddTCBToEventPriority (PLW_CLASS_TCB    ptcb, 
                              PLW_CLASS_EVENT  pevent, 
                              PLW_LIST_RING   *ppringList)
{
             PLW_LIST_RING    pringList;
    REGISTER PLW_LIST_RING    pringListHeader;
    REGISTER PLW_CLASS_TCB    ptcbTemp;
    REGISTER UINT8            ucPriority;                               /*  ���ȼ��ݴ����              */
    
    ucPriority      = ptcb->TCB_ucPriority;                             /*  ���ȼ�                      */
    pringListHeader = *ppringList;
    
    if (pringListHeader == LW_NULL) {                                   /*  û�еȴ�����                */
        _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);         /*  ֱ�����                    */
        
    } else {
        ptcbTemp = _LIST_ENTRY(pringListHeader, LW_CLASS_TCB, TCB_ringEvent);
        if (LW_PRIO_IS_HIGH(ucPriority, ptcbTemp->TCB_ucPriority)) {
            _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);     /*  ���뵽����ͷ                */
        
        } else {
            pringList = _list_ring_get_next(pringListHeader);
            while (pringList != pringListHeader) {
                ptcbTemp = _LIST_ENTRY(pringList, LW_CLASS_TCB, TCB_ringEvent);
                if (LW_PRIO_IS_HIGH(ucPriority, ptcbTemp->TCB_ucPriority)) {
                    break;
                }
                pringList = _list_ring_get_next(pringList);
            }
            _List_Ring_Add_Last(&ptcb->TCB_ringEvent, &pringList);
        }
    }
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum++;                 /*  �ȴ��¼��ĸ���++            */
}
/*********************************************************************************************************
** ��������: _DelTCBFromEventPriority
** ��������: ��FIFO�¼��ȴ�������ɾ��һ���߳�
** �䡡��  : ptcb           ������ƿ�
**           pevent         �¼����ƿ�
**           ppringList     �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _DelTCBFromEventPriority (PLW_CLASS_TCB      ptcb, 
                                PLW_CLASS_EVENT    pevent, 
                                PLW_LIST_RING     *ppringList)
{    
    _List_Ring_Del(&ptcb->TCB_ringEvent, ppringList);
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum--;                 /*  �ȴ��¼��ĸ���--            */
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
