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
** ��   ��   ��: _EventSetUnQueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: ���̴߳ӵȴ��¼��������н��

** BUG:
2009.12.12 �޸��ļ���ʽ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventSetUnQueue
** ��������: ���̴߳ӵȴ��¼��������н��
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

VOID  _EventSetUnQueue (PLW_CLASS_EVENTSETNODE   pesn)
{
#if LW_CFG_THREAD_DEL_EN > 0
    REGISTER PLW_CLASS_TCB         ptcb;
#endif
    
    REGISTER PLW_CLASS_EVENTSET    pes;
    
    pes = (PLW_CLASS_EVENTSET)pesn->EVENTSETNODE_pesEventSet;
    
    _List_Line_Del(&pesn->EVENTSETNODE_lineManage, &pes->EVENTSET_plineWaitList);
    
#if LW_CFG_THREAD_DEL_EN > 0
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    ptcb->TCB_pesnPtr = LW_NULL;
#endif
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
