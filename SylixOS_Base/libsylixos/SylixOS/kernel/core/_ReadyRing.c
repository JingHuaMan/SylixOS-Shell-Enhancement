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
** ��   ��   ��: _ReadyRing.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 17 ��
**
** ��        ��: ����ϵͳ��������������

** BUG
2007.12.22 ����ע��.
2009.04.28 ���ھ�����δ����ʱ���������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _AddTCBToReadyRing
** ��������: ��һ��������������
** �䡡��  : ptcb        ��Ҫ���������
**           ppcb        ���ȼ����ƿ�
**           bIsHeader   �Ƿ����ͷ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _AddTCBToReadyRing (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb, BOOL  bIsHeader)
{
    if (bIsHeader) {
        _List_Ring_Add_Ahead(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        return;
    
    } else if (ptcb->TCB_ucSchedPolicy == LW_OPTION_SCHED_FIFO) {
        _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        return;
    }
    
    switch (ptcb->TCB_ucSchedActivateMode) {
    
    case LW_OPTION_RESPOND_IMMIEDIA:
        _List_Ring_Add_Front(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        break;
        
    case LW_OPTION_RESPOND_STANDARD:
        _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        break;
        
    default:
        if (ptcb->TCB_ucSchedActivate == LW_SCHED_ACT_OTHER) {
            _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        
        } else {
            _List_Ring_Add_Front(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        }
        break;
    }
}
/*********************************************************************************************************
** ��������: _DelTCBFromReadyRing
** ��������: ��һ������Ӿ������н��
** �䡡��  : ptcb        ��Ҫɾ��������
**           ppcb        ���ȼ����ƿ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _DelTCBFromReadyRing (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    _List_Ring_Del(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
