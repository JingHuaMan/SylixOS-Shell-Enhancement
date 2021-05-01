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
** ��   ��   ��: _MsgQueueLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 07 ��
**
** ��        ��: ��Ϣ�����ڲ������⡣

** BUG:
2009.06.19  �޸�ע��.
2018.07.16  ʹ��˫���ȼ�����ģʽ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _MsgQueueClear
** ��������: ��Ϣ�������
** �䡡��  : pmsgqueue     ��Ϣ���п��ƿ�
**           ulMsgTotal    ����Ϣ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

VOID  _MsgQueueClear (PLW_CLASS_MSGQUEUE    pmsgqueue,
                      ULONG                 ulMsgTotal)
{
    ULONG               i;
    size_t              stMaxByteAlign;
    PLW_CLASS_MSGNODE   pmsgnode;

    pmsgqueue->MSGQUEUE_pmonoFree = LW_NULL;
    pmsgqueue->MSGQUEUE_uiMap     = 0;
    
    for (i = 0; i < EVENT_MSG_Q_PRIO; i++) {
        pmsgqueue->MSGQUEUE_pmonoHeader[i] = LW_NULL;
        pmsgqueue->MSGQUEUE_pmonoTail[i]   = LW_NULL;
    }
    
    stMaxByteAlign = ROUND_UP(pmsgqueue->MSGQUEUE_stMaxBytes, sizeof(size_t));
    pmsgnode       = (PLW_CLASS_MSGNODE)pmsgqueue->MSGQUEUE_pvBuffer;
    
    for (i = 0; i < ulMsgTotal; i++) {
        _list_mono_free(&pmsgqueue->MSGQUEUE_pmonoFree, &pmsgnode->MSGNODE_monoManage);
        pmsgnode = (PLW_CLASS_MSGNODE)((UINT8 *)pmsgnode + sizeof(LW_CLASS_MSGNODE) + stMaxByteAlign);
    }
}
/*********************************************************************************************************
** ��������: _MsgQueueGet
** ��������: ����Ϣ�����л��һ����Ϣ
** �䡡��  : pmsgqueue     ��Ϣ���п��ƿ�
**         : pvMsgBuffer   ���ջ�����
**         : stMaxByteSize ��������С
**         : pstMsgLen     �����Ϣ�ĳ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _MsgQueueGet (PLW_CLASS_MSGQUEUE    pmsgqueue,
                    PVOID                 pvMsgBuffer,
                    size_t                stMaxByteSize,
                    size_t               *pstMsgLen)
{
    INT                 iQ;
    PLW_CLASS_MSGNODE   pmsgnode;
    
    iQ = archFindLsb(pmsgqueue->MSGQUEUE_uiMap) - 1;                    /*  �������ȼ�                  */
    
    _BugHandle(!pmsgqueue->MSGQUEUE_pmonoHeader[iQ], LW_TRUE, "buffer is empty!\r\n");
    
    pmsgnode = (PLW_CLASS_MSGNODE)_list_mono_allocate_seq(&pmsgqueue->MSGQUEUE_pmonoHeader[iQ],
                                                          &pmsgqueue->MSGQUEUE_pmonoTail[iQ]);
    
    if (pmsgqueue->MSGQUEUE_pmonoHeader[iQ] == LW_NULL) {
        pmsgqueue->MSGQUEUE_uiMap &= ~(1 << iQ);                        /*  ������ȼ�λͼ              */
    }
    
    *pstMsgLen = (stMaxByteSize < pmsgnode->MSGNODE_stMsgLen) ? 
                 (stMaxByteSize) : (pmsgnode->MSGNODE_stMsgLen);        /*  ȷ��������Ϣ����            */
                 
    lib_memcpy(pvMsgBuffer, (PVOID)(pmsgnode + 1), *pstMsgLen);         /*  ������Ϣ                    */
    
    _list_mono_free(&pmsgqueue->MSGQUEUE_pmonoFree, &pmsgnode->MSGNODE_monoManage);
}
/*********************************************************************************************************
** ��������: _MsgQueuePut
** ��������: ����Ϣ������д��һ����Ϣ
** �䡡��  : pmsgqueue     ��Ϣ���п��ƿ� 
**         : pvMsgBuffer   ��Ϣ������
**         : stMsgLen      ��Ϣ����
**         : uiPrio        ��Ϣ���ȼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _MsgQueuePut (PLW_CLASS_MSGQUEUE    pmsgqueue,
                    PVOID                 pvMsgBuffer,
                    size_t                stMsgLen, 
                    UINT                  uiPrio)
{
    PLW_CLASS_MSGNODE  pmsgnode;
    
    pmsgnode = (PLW_CLASS_MSGNODE)_list_mono_allocate(&pmsgqueue->MSGQUEUE_pmonoFree);
    
    pmsgnode->MSGNODE_stMsgLen = stMsgLen;
    
    lib_memcpy((PVOID)(pmsgnode + 1), pvMsgBuffer, stMsgLen);           /*  ������Ϣ                    */
    
    if (pmsgqueue->MSGQUEUE_pmonoHeader[uiPrio] == LW_NULL) {
        pmsgqueue->MSGQUEUE_uiMap |= (1 << uiPrio);                     /*  �������ȼ�λͼ              */
    }
    
    _list_mono_free_seq(&pmsgqueue->MSGQUEUE_pmonoHeader[uiPrio], 
                        &pmsgqueue->MSGQUEUE_pmonoTail[uiPrio], 
                        &pmsgnode->MSGNODE_monoManage);
}
/*********************************************************************************************************
** ��������: _MsgQueueMsgLen
** ��������: ����Ϣ�����л��һ����Ϣ�ĳ���
** �䡡��  : pmsgqueue     ��Ϣ���п��ƿ�
**           pstMsgLen     ��Ϣ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _MsgQueueMsgLen (PLW_CLASS_MSGQUEUE  pmsgqueue, size_t  *pstMsgLen)
{
    INT                 iQ;
    PLW_CLASS_MSGNODE   pmsgnode;
    
    iQ = archFindLsb(pmsgqueue->MSGQUEUE_uiMap) - 1;                    /*  �������ȼ�                  */
    
    _BugHandle(!pmsgqueue->MSGQUEUE_pmonoHeader[iQ], LW_TRUE, "buffer is empty!\r\n");

    pmsgnode = (PLW_CLASS_MSGNODE)pmsgqueue->MSGQUEUE_pmonoHeader[iQ];
    
    *pstMsgLen = pmsgnode->MSGNODE_stMsgLen;
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
