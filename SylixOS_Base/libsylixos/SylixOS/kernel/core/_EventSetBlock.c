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
** ��   ��   ��: _EventSetBlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: ���̲߳���ȴ��¼�������

** BUG
2008.03.29  �����µ� wake up ����.
2008.03.30  ʹ���µľ���������.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventSetBlock
** ��������: ���̲߳���ȴ��¼������� (�����ں��ҹ��ж�״̬�±�����)
** �䡡��  : pes            �¼�����ƿ�
**           pesn           �¼���ȴ��ڵ�
**           ulEvents       �¼���
**           ucWaitType     �ȴ�����
**           ulWaitTime     �ȴ�ʱ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

VOID  _EventSetBlock (PLW_CLASS_EVENTSET        pes,
                      PLW_CLASS_EVENTSETNODE    pesn,
                      ULONG                     ulEvents,
                      UINT8                     ucWaitType,
                      ULONG                     ulWaitTime)
{
    REGISTER PLW_CLASS_PCB               ppcb;
             PLW_CLASS_TCB               ptcbCur;
             
    LW_TCB_GET_CUR(ptcbCur);                                            /*  ��ǰ������ƿ�              */
    
    ptcbCur->TCB_usStatus       |= LW_THREAD_STATUS_EVENTSET;
    ptcbCur->TCB_ucWaitTimeout   = LW_WAIT_TIME_CLEAR;
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;
    
    ptcbCur->TCB_ulDelay = ulWaitTime;
    if (ulWaitTime) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
    }
    
#if LW_CFG_THREAD_DEL_EN > 0
    ptcbCur->TCB_pesnPtr = pesn;
#endif
    
    pesn->EVENTSETNODE_pesEventSet = (PVOID)pes;
    pesn->EVENTSETNODE_ulEventSets = ulEvents;
    pesn->EVENTSETNODE_ucWaitType  = ucWaitType;
    pesn->EVENTSETNODE_ptcbMe      = (PVOID)ptcbCur;
    
    _List_Line_Add_Ahead(&pesn->EVENTSETNODE_lineManage, &pes->EVENTSET_plineWaitList);
    
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
