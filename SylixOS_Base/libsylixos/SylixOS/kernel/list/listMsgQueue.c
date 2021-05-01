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
** ��   ��   ��: listMsgQueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Message Queue ��Դ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_MsgQueue_Object
** ��������: �ӿ��� MsgQueue �ؼ�����ȡ��һ������ MsgQueue
** �䡡��  : 
** �䡡��  : ��õ� MsgQueue ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

PLW_CLASS_MSGQUEUE  _Allocate_MsgQueue_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueueFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcMsgQueue.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree      = _list_mono_allocate_seq(&_K_resrcMsgQueue.RESRC_pmonoFreeHeader, 
                                             &_K_resrcMsgQueue.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    pmsgqueueFree  = _LIST_ENTRY(pmonoFree, LW_CLASS_MSGQUEUE, 
                                 MSGQUEUE_monoResrcList);               /*  �����Դ��������ַ          */
                                 
    _K_resrcMsgQueue.RESRC_uiUsed++;
    if (_K_resrcMsgQueue.RESRC_uiUsed > _K_resrcMsgQueue.RESRC_uiMaxUsed) {
        _K_resrcMsgQueue.RESRC_uiMaxUsed = _K_resrcMsgQueue.RESRC_uiUsed;
    }
    
    return  (pmsgqueueFree);
}
/*********************************************************************************************************
** ��������: _Free_MsgQueue_Object
** ��������: �� MsgQueue ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_MsgQueue_Object (PLW_CLASS_MSGQUEUE    pmsgqueueFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pmsgqueueFree->MSGQUEUE_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcMsgQueue.RESRC_pmonoFreeHeader, 
                        &_K_resrcMsgQueue.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcMsgQueue.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
