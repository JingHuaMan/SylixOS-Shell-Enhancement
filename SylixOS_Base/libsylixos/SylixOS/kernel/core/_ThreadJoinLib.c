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
** ��   ��   ��: _ThreadJoinLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: �̺߳ϲ��ͽ���CORE������

** BUG
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.13  ���� TCB_ptcbJoin ��Ϣ��¼���ж�.
2008.03.30  ʹ���µľ���������.
2010.08.03  ���ⲿ��ʹ�õĺ�����Ϊ static ����, 
            ����ĺ����������ں�����ģʽ�µ��õ�, ����ֻ��ر��жϾͿɱ��� SMP ��ռ.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2014.12.03  ����ʹ�� suspend ����, ת��ʹ�� LW_THREAD_STATUS_JOIN.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadJoinWait
** ��������: �̺߳ϲ��������Լ� (�ڽ����ں˲����жϺ󱻵���)
** �䡡��  : ptcbCur   ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _ThreadJoinWait (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_CLASS_PCB  ppcbMe;
             
    ppcbMe = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcbMe);                             /*  �Ӿ�������ɾ��              */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_JOIN;                     /*  ����Ϊ join ״̬            */
}
/*********************************************************************************************************
** ��������: _ThreadJoinWakeup
** ��������: ���������߳� (�ڽ����ں˺󱻵���)
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _ThreadJoinWakeup (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_PCB  ppcb;
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);
    if (__LW_THREAD_IS_READY(ptcb)) {
       ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;              /*  �жϼ��ʽ                */
       ppcb = _GetPcb(ptcb);
       __ADD_TO_READY_RING(ptcb, ppcb);                                 /*  ���������                  */
    }
}
/*********************************************************************************************************
** ��������: _ThreadJoin
** ��������: ����ǰ�̺߳ϲ��������߳� (�ڽ����ں˺󱻵���)
** �䡡��  : ptcbDes          Ҫ�ϲ����߳�
**           ptwj             Wait Join ����
**           ppvRetValSave    Ŀ���߳̽���ʱ�ķ���ֵ��ŵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadJoin (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  *ppvRetValSave)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_JOIN, 
                      ptcbCur->TCB_ulId, ptcbDes->TCB_ulId, LW_NULL);
    
    if (ptcbDes) {
        ptcbCur->TCB_ppvJoinRetValSave = ppvRetValSave;                 /*  �����ŷ���ֵ�ĵ�ַ        */
                                                                        /*  ����ȴ�����                */
        _List_Line_Add_Ahead(&ptcbCur->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);

        ptcbCur->TCB_ptcbJoin = ptcbDes;                                /*  ��¼Ŀ���߳�                */

        iregInterLevel = KN_INT_DISABLE();
        _ThreadJoinWait(ptcbCur);                                       /*  �����Լ��ȴ��Է�����        */
        KN_INT_ENABLE(iregInterLevel);

    } else if (ptwj) {                                                  /*  �ȴ����� ID ��Դ            */
        if (ppvRetValSave) {
            *ppvRetValSave = ptwj->TWJ_pvReturn;
        }
        _Free_Tcb_Object(ptwj->TWJ_ptcb);                               /*  �ͷ� ID                     */
        ptwj->TWJ_ptcb = LW_NULL;
    }
}
/*********************************************************************************************************
** ��������: _ThreadDisjoin
** ��������: ָ���߳̽���ϲ����߳� (�ڽ����ں˺󱻵���)
** �䡡��  : ptcbDes          �ϲ���Ŀ���߳�
**           ptcbDisjoin      ��Ҫ����ϲ����߳�
**           bWakeup          �Ƿ��� (FALSE ʱ������������)
**           pvArg            ���Ѳ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadDisjoin (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_TCB  ptcbDisjoin, BOOL  bWakeup, PVOID  pvArg)
{
    INTREG  iregInterLevel;
    
    if (ptcbDisjoin->TCB_ppvJoinRetValSave) {                           /*  �ȴ�����ֵ                  */
        *ptcbDisjoin->TCB_ppvJoinRetValSave = pvArg;                    /*  ���淵��ֵ                  */
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DETACH, 
                      ptcbDisjoin->TCB_ulId, pvArg, LW_NULL);
                      
    _List_Line_Del(&ptcbDisjoin->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);
    
    ptcbDisjoin->TCB_ptcbJoin = LW_NULL;                                /*  �����¼�ĵȴ��߳� tcb      */
    
    iregInterLevel = KN_INT_DISABLE();
    if (bWakeup) {
        _ThreadJoinWakeup(ptcbDisjoin);                                 /*  �ͷźϲ����߳�              */
    } else {
        ptcbDisjoin->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);          /*  ���޸�״̬����              */
    }
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: _ThreadDetach
** ��������: ָ���߳̽���ϲ������������̲߳������������̺߳ϲ��Լ� (�ڽ����ں˺󱻵���)
** �䡡��  : ptcbDes          �ϲ���Ŀ���߳�
**           ptwj             Wait Join ����
**           pvRetVal         ����ֵ
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _ThreadDetach (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal)
{
             INT            iCnt = 0;
    REGISTER PLW_CLASS_TCB  ptcbJoin;

    if (ptcbDes) {
        while (ptcbDes->TCB_plineJoinHeader) {
            iCnt++;
            ptcbJoin = _LIST_ENTRY(ptcbDes->TCB_plineJoinHeader, LW_CLASS_TCB, TCB_lineJoin);
            _ThreadDisjoin(ptcbDes, ptcbJoin, LW_TRUE, pvRetVal);
        }
        ptcbDes->TCB_bDetachFlag = LW_TRUE;                             /*  �Ͻ��ϲ��Լ�                */

    } else if (ptwj) {
        _Free_Tcb_Object(ptwj->TWJ_ptcb);                               /*  �ͷ� ID                     */
        ptwj->TWJ_ptcb = LW_NULL;
    }

    return  (iCnt);
}
/*********************************************************************************************************
** ��������: _ThreadWjAdd
** ��������: ��ָ���̷߳��� Wait Join ���� (�ڽ����ں˺󱻵���)
** �䡡��  : ptcbDes          ָ���߳�
**           ptwj             Wait Join ����
**           pvRetVal         ����ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _ThreadWjAdd (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal)
{
    ptwj->TWJ_ptcb     = ptcbDes;
    ptwj->TWJ_pvReturn = pvRetVal;
}
/*********************************************************************************************************
** ��������: _ThreadWjClear
** ��������: ��� Wait Join �� (�ڽ����ں˺󱻵���)
** �䡡��  : pvVProc   ���̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _ThreadWjClear (PVOID  pvVProc)
{
    INT  i;

    for (i = LW_NCPUS; i < LW_CFG_MAX_THREADS; i++) {
        if (_K_twjTable[i].TWJ_ptcb) {
            if (_K_twjTable[i].TWJ_ptcb->TCB_pvVProcessContext == pvVProc) {
                _Free_Tcb_Object(_K_twjTable[i].TWJ_ptcb);              /*  �ͷ� ID                     */
                _K_twjTable[i].TWJ_ptcb = LW_NULL;
            }
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
