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
** ��   ��   ��: _ThreadIdInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�߳�ID��ʼ�������⡣

** BUG:
2013.05.07  �������� THREAD ID �� TCB, ����ֱ�ӳ�ʼ�� TCB ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadIdInit
** ��������: ��ʼ���߳�ID    һ����ʼ�� LW_CFG_MAX_THREADS ���ڵ�
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadIdInit (VOID)
{
    REGISTER ULONG                 ulI;
    
    REGISTER PLW_CLASS_TCB         ptcbTemp1;
    REGISTER PLW_CLASS_TCB         ptcbTemp2;
    
    REGISTER PLW_LIST_MONO         pmonoTemp1;
    REGISTER PLW_LIST_MONO         pmonoTemp2;
    
    _K_resrcTcb.RESRC_pmonoFreeHeader = &_K_tcbBuffer[0].TCB_monoResrcList;  
                                                                        /*  ������Դ��ͷ                */
    ptcbTemp1 = &_K_tcbBuffer[0];
    ptcbTemp2 = &_K_tcbBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_THREADS - 1); ulI++) {
        pmonoTemp1 = &ptcbTemp1->TCB_monoResrcList;
        pmonoTemp2 = &ptcbTemp2->TCB_monoResrcList;
        LW_SPIN_INIT(&ptcbTemp1->TCB_slLock);
        
        ptcbTemp1->TCB_usIndex = (UINT16)ulI;                           /*  ���û�����ڲ����          */
         
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        ptcbTemp1++;
        ptcbTemp2++;
    }
    
    pmonoTemp1 = &ptcbTemp1->TCB_monoResrcList;
    LW_SPIN_INIT(&ptcbTemp1->TCB_slLock);
    
    ptcbTemp1->TCB_usIndex = (UINT16)ulI;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                   /*  ����Ϊ���һ���ڵ�          */
    
    _K_resrcTcb.RESRC_pmonoFreeTail = pmonoTemp1;
    
    _K_resrcTcb.RESRC_uiUsed    = 0;
    _K_resrcTcb.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
