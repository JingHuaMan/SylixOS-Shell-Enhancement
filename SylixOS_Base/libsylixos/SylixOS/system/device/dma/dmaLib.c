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
** ��   ��   ��: dmaLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 06 ��
**
** ��        ��: ͨ�� DMA �豸�����.

** BUG:
2009.04.15  �޸Ĵ�����.
2009.09.15  �������������ͬ������.
2009.12.11  ���� DMA Ӳ������ṹ, ֧��ϵͳͬʱ���ڶ����칹 DMA ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __DMA_WAITNODE       _G_dmanBuffer[LW_CFG_MAX_DMA_LISTNODES];    /*  �ȴ��ڵ㻺��                */
static PLW_LIST_RING        _G_pringDmanFreeHeader = LW_NULL;           /*  ���л������ڵ�              */
/*********************************************************************************************************
** ��������: _dmaInit
** ��������: ��ʼ�� DMA
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    _dmaInit (VOID)
{
    REGISTER INT    i;
    
    for (i = 0; i < LW_CFG_MAX_DMA_LISTNODES; i++) {                    /*  �����нڵ㴮������          */
        _List_Ring_Add_Ahead(&_G_dmanBuffer[i].DMAN_ringManage, 
                             &_G_pringDmanFreeHeader);
    }
}
/*********************************************************************************************************
** ��������: _dmaWaitnodeAlloc
** ��������: �ӻ�����������һ�� DMA �ȴ��ڵ�
** �䡡��  : NONE
** �䡡��  : ��������Ľڵ�, ��������û�п��нӵ�ʱ, ���� LW_NULL;
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
__PDMA_WAITNODE     _dmaWaitnodeAlloc (VOID)
{
    REGISTER PLW_LIST_RING      pringNewNode;
    REGISTER __PDMA_WAITNODE    pdmanNewNode;
    
    if (_G_pringDmanFreeHeader) {                                       /*  �����ڿ��еĽڵ�            */
        pringNewNode = _G_pringDmanFreeHeader;
        _List_Ring_Del(_G_pringDmanFreeHeader,
                      &_G_pringDmanFreeHeader);                         /*  ɾ����ͷ�Ľڵ�              */
        pdmanNewNode = _LIST_ENTRY(pringNewNode, __DMA_WAITNODE, DMAN_ringManage);
        return  (pdmanNewNode);                                         /*  �����½ڵ�                  */
    
    } else {
        return  (LW_NULL);                                              /*  û���½ڵ���                */
    }
}
/*********************************************************************************************************
** ��������: _dmaWaitnodeFree
** ��������: �򻺳������ͷ�һ�� DMA �ȴ��ڵ�
** �䡡��  : pdmanNode     Ҫ�ͷŵĽڵ�.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    _dmaWaitnodeFree (__PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Add_Ahead(&pdmanNode->DMAN_ringManage, 
                         &_G_pringDmanFreeHeader);                      /*  ������ж���                */
}
/*********************************************************************************************************
** ��������: _dmaInsertToWaitList
** ��������: ��ָ�� DMA ͨ���ĵȴ����в���һ���ڵ� (�嵽���һ���ڵ�)
** �䡡��  : pdmanNode     Ҫ����Ľڵ�.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    _dmaInsertToWaitList (__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Add_Last(&pdmanNode->DMAN_ringManage,
                        &pdmacChannel->DMAC_pringHead);                 /*  �������                    */
    pdmacChannel->DMAC_iNodeCounter++;                                  /*  ���� ++                     */
}
/*********************************************************************************************************
** ��������: _dmaDeleteFromWaitList
** ��������: ��ָ�� DMA ͨ���ĵȴ�����ɾ��һ���ڵ�
** �䡡��  : pdmanNode     Ҫɾ���Ľڵ�.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    _dmaDeleteFromWaitList (__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Del(&pdmanNode->DMAN_ringManage,
                   &pdmacChannel->DMAC_pringHead);                      /*  �Ӷ�����ɾ��                */
    pdmacChannel->DMAC_iNodeCounter--;                                  /*  ���� --                     */
}
/*********************************************************************************************************
** ��������: _dmaGetFirstInWaitList
** ��������: ��ָ�� DMA ͨ���Ļ�õȴ������е�һ���ڵ�, ���ǲ�ɾ��.
** �䡡��  : pdmanNode     Ҫɾ���Ľڵ�.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
__PDMA_WAITNODE     _dmaGetFirstInWaitList (__PDMA_CHANNEL    pdmacChannel)
{
    REGISTER __PDMA_WAITNODE   pdmanNode;
    REGISTER PLW_LIST_RING     pringNode;
    
    pringNode = pdmacChannel->DMAC_pringHead;
    if (pringNode) {
        pdmanNode = _LIST_ENTRY(pringNode, __DMA_WAITNODE, DMAN_ringManage);
        return  (pdmanNode);
    
    } else {
        return  (LW_NULL);
    }
}

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
