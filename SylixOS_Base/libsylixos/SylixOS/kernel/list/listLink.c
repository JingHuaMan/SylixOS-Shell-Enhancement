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
** ��   ��   ��: listLink.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 17 ��
**
** ��        ��: ����ϵͳ�������Ӳ������塣

** BUG
2008.11.30  �����ļ�, �޸�ע��.
2010.08.16  ���а���������ʯ�������ѵ�ͬ��!
            ����������������Ҳ��빦��.
2011.06.12  ���� _List_Line_Del() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _List_Ring_Add_Ahead
** ��������: �� RING ͷ�в���һ���ڵ� (Header ��ָ������ڵ�, �ȴ���Ĳ���)
** �䡡��  : pringNew      �µĽڵ�
**           ppringHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Ahead (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  �����ڻ��нڵ�              */
        pringNew->RING_plistNext = pringHeader;
        pringNew->RING_plistPrev = pringHeader->RING_plistPrev;
        pringHeader->RING_plistPrev->RING_plistNext = pringNew;
        pringHeader->RING_plistPrev = pringNew;
        
    } else {                                                            /*  ������û�нڵ�              */
        pringNew->RING_plistPrev = pringNew;                            /*  ֻ���½ڵ�                  */
        pringNew->RING_plistNext = pringNew;                            /*  ���ұ���                    */
    }
    
    *ppringHeader = pringNew;                                           /*  ����ͷָ���½ڵ�            */
}
/*********************************************************************************************************
** ��������: _List_Ring_Add_Front
** ��������: �� RING ͷ�в���һ���ڵ� (Header �нڵ�ʱ�����仯��������Ĳ���)
** �䡡��  : pringNew      �µĽڵ�
**           ppringHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Front (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  �����ڻ��нڵ�              */
        pringNew->RING_plistPrev = pringHeader;
        pringNew->RING_plistNext = pringHeader->RING_plistNext;
        pringHeader->RING_plistNext->RING_plistPrev = pringNew;
        pringHeader->RING_plistNext = pringNew;
    
    } else {                                                            /*  ������û�нڵ�              */
        pringNew->RING_plistPrev = pringNew;                            /*  ֻ���½ڵ�                  */
        pringNew->RING_plistNext = pringNew;                            /*  ���ұ���                    */
        *ppringHeader = pringNew;                                       /*  ��������ͷ                  */
    }
}
/*********************************************************************************************************
** ��������: _List_Ring_Add_Last
** ��������: �Ӻ����� RING β�в���һ���ڵ�
** �䡡��  : pringNew      �µĽڵ�
**           ppringHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Last (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  û�и�������                */
        pringHeader->RING_plistPrev->RING_plistNext = pringNew;
        pringNew->RING_plistPrev = pringHeader->RING_plistPrev;
        pringNew->RING_plistNext = pringHeader;
        pringHeader->RING_plistPrev = pringNew;
        
    } else {
        pringNew->RING_plistPrev = pringNew;
        pringNew->RING_plistNext = pringNew;
        *ppringHeader = pringNew;                                       /*  ��������ͷ                  */
    }
}
/*********************************************************************************************************
** ��������: _List_Ring_Del
** ��������: ɾ��һ���ڵ�
** �䡡��  : pringDel      ��Ҫɾ���Ľڵ�
**           ppringHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Ring_Del (PLW_LIST_RING  pringDel, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringDel->RING_plistNext == pringDel) {                         /*  ������ֻ��һ���ڵ�          */
        *ppringHeader = LW_NULL;
        _INIT_LIST_RING_HEAD(pringDel);
        return;
    
    } else if (pringDel == pringHeader) {
        _list_ring_next(ppringHeader);
    }
    
    pringHeader = pringDel->RING_plistPrev;                             /*  pringHeader ������ʱ����    */
    pringHeader->RING_plistNext = pringDel->RING_plistNext;
    pringDel->RING_plistNext->RING_plistPrev = pringHeader;
    
    _INIT_LIST_RING_HEAD(pringDel);                                     /*  prev = next = NULL          */
}
/*********************************************************************************************************
** ��������: _List_Line_Add_Ahead
** ��������: ��ǰ���� Line ͷ�в���һ���ڵ� (Header ��ָ������ڵ�)
** �䡡��  : plingNew      �µĽڵ�
**           pplingHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Line_Add_Ahead (PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader)
{
    REGISTER LW_LIST_LINE_HEADER    plineHeader;
    
    plineHeader = *pplineHeader;
    
    plineNew->LINE_plistNext = plineHeader;
    plineNew->LINE_plistPrev = LW_NULL;
    
    if (plineHeader) {    
        plineHeader->LINE_plistPrev = plineNew;
    }
    
    *pplineHeader = plineNew;                                           /*  ָ�����±���                */
}
/*********************************************************************************************************
** ��������: _List_Line_Add_Tail
** ��������: ��ǰ���� Line ͷ�в���һ���ڵ� (Header ����)
** �䡡��  : plingNew      �µĽڵ�
**           pplingHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Line_Add_Tail (PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader)
{
    REGISTER LW_LIST_LINE_HEADER    plineHeader;
    
    plineHeader = *pplineHeader;
    
    if (plineHeader) {
        if (plineHeader->LINE_plistNext) {
            plineNew->LINE_plistNext = plineHeader->LINE_plistNext;
            plineNew->LINE_plistPrev = plineHeader;
            plineHeader->LINE_plistNext->LINE_plistPrev = plineNew;
            plineHeader->LINE_plistNext = plineNew;
        
        } else {
            plineHeader->LINE_plistNext = plineNew;
            plineNew->LINE_plistPrev = plineHeader;
            plineNew->LINE_plistNext = LW_NULL;
        }
    
    } else {
        plineNew->LINE_plistPrev = LW_NULL;
        plineNew->LINE_plistNext = LW_NULL;
        
        *pplineHeader = plineNew;
    }
}
/*********************************************************************************************************
** ��������: _List_Line_Add_Left
** ��������: ���µĽڵ����ָ���ڵ�����.
** �䡡��  : plineNew      �µĽڵ�
**           plineRight    �Ҳ�ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Line_Add_Left (PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineRight)
{
    REGISTER PLW_LIST_LINE      plineLeft = plineRight->LINE_plistPrev;
    
    plineNew->LINE_plistNext = plineRight;
    plineNew->LINE_plistPrev = plineLeft;
    
    if (plineLeft) {
        plineLeft->LINE_plistNext = plineNew;
    }
    
    plineRight->LINE_plistPrev = plineNew;
}
/*********************************************************************************************************
** ��������: _List_Line_Add_Right
** ��������: ���µĽڵ����ָ���ڵ���Ҳ�.
** �䡡��  : plineNew      �µĽڵ�
**           plineLeft     ���ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Line_Add_Right (PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineLeft)
{
    REGISTER PLW_LIST_LINE      plineRight = plineLeft->LINE_plistNext;
    
    plineNew->LINE_plistNext = plineRight;
    plineNew->LINE_plistPrev = plineLeft;
    
    if (plineRight) {
        plineRight->LINE_plistPrev = plineNew;
    }
    
    plineLeft->LINE_plistNext = plineNew;
}
/*********************************************************************************************************
** ��������: _List_Line_Del
** ��������: ɾ��һ���ڵ�
** �䡡��  : plingDel      ��Ҫɾ���Ľڵ�
**           pplingHeader  ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _List_Line_Del (PLW_LIST_LINE  plineDel, LW_LIST_LINE_HEADER  *pplineHeader)
{
    
    if (plineDel->LINE_plistPrev == LW_NULL) {                          /*  ��ͷ                        */
        *pplineHeader = plineDel->LINE_plistNext;
    } else {
        plineDel->LINE_plistPrev->LINE_plistNext = plineDel->LINE_plistNext;
    }
    
    if (plineDel->LINE_plistNext) {
        plineDel->LINE_plistNext->LINE_plistPrev = plineDel->LINE_plistPrev;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
