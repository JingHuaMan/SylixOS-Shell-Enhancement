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
** ��   ��   ��: _MsgQueueInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��Ϣ���г�ʼ����

** BUG:
2013.11.14  ʹ�ö�����Դ�������ṹ���������Դ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _MsgQueueInit
** ��������: ��Ϣ���г�ʼ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _MsgQueueInit (VOID)
{
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

#if  LW_CFG_MAX_MSGQUEUES == 1

    _K_resrcMsgQueue.RESRC_pmonoFreeHeader = &_K_msgqueueBuffer[0].MSGQUEUE_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(_K_resrcMsgQueue.RESRC_pmonoFreeHeader);
    
    _K_resrcMsgQueue.RESRC_pmonoFreeTail = _K_resrcMsgQueue.RESRC_pmonoFreeHeader;
#else

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_MSGQUEUE     pmsgqueueTemp1;
    REGISTER PLW_CLASS_MSGQUEUE     pmsgqueueTemp2;

    _K_resrcMsgQueue.RESRC_pmonoFreeHeader = &_K_msgqueueBuffer[0].MSGQUEUE_monoResrcList;

    pmsgqueueTemp1 = &_K_msgqueueBuffer[0];
    pmsgqueueTemp2 = &_K_msgqueueBuffer[1];
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_MSGQUEUES) - 1); ulI++) {
        pmonoTemp1 = &pmsgqueueTemp1->MSGQUEUE_monoResrcList;
        pmonoTemp2 = &pmsgqueueTemp2->MSGQUEUE_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pmsgqueueTemp1++;
        pmsgqueueTemp2++;
    }
    
    pmonoTemp1 = &pmsgqueueTemp1->MSGQUEUE_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcMsgQueue.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                  /*  LW_CFG_MAX_MSGQUEUES == 1   */

    _K_resrcMsgQueue.RESRC_uiUsed    = 0;
    _K_resrcMsgQueue.RESRC_uiMaxUsed = 0;

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
