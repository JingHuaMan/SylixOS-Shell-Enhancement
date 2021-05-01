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
** ��   ��   ��: treeOp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 17 ��
**
** ��        ��: ����ϵͳ���������塣
*********************************************************************************************************/

#ifndef __TREEOP_H
#define __TREEOP_H

/*********************************************************************************************************
  �ж������Ƿ�Ϊ��
*********************************************************************************************************/

#define _TREE_RB_IS_EMPTY(ptrbrRoot)                 \
        (ptrbrRoot->TRBR_ptrbnNode == LW_NULL)

/*********************************************************************************************************
  �������ĸ��ָ��ṹ
*********************************************************************************************************/

#define _TREE_ENTRY(ptr, type, member)              \
        _LIST_CONTAINER_OF(ptr, type, member)

/*********************************************************************************************************
  ��һ��
*********************************************************************************************************/

static LW_INLINE PLW_TREE_RB_NODE   _tree_rb_get_left (PLW_TREE_RB_NODE  ptrbn)
{
    return  (ptrbn->TRBN_ptrbnLeft);
}
static LW_INLINE PLW_TREE_RB_NODE   _tree_rb_get_right (PLW_TREE_RB_NODE  ptrbn)
{
    return  (ptrbn->TRBN_ptrbnRight);
}

static LW_INLINE PLW_TREE_RB_NODE  *_tree_rb_get_left_addr (PLW_TREE_RB_NODE  ptrbn)
{
    return  (&(ptrbn->TRBN_ptrbnLeft));
}
static LW_INLINE PLW_TREE_RB_NODE  *_tree_rb_get_right_addr (PLW_TREE_RB_NODE  ptrbn)
{
    return  (&(ptrbn->TRBN_ptrbnRight));
}

/*********************************************************************************************************
  ������ز���
*********************************************************************************************************/

static LW_INLINE VOID _tree_rb_link_node (PLW_TREE_RB_NODE  ptrbn, 
                                          PLW_TREE_RB_NODE  ptrbnParent, 
                                          PLW_TREE_RB_NODE *pptrbnLink)
{
    ptrbn->TRBN_ptrbnParent = ptrbnParent;
	ptrbn->TRBN_iColor      = LW_TREE_RB_RED;
	ptrbn->TRBN_ptrbnLeft   = ptrbn->TRBN_ptrbnRight = LW_NULL;

	*pptrbnLink = ptrbn;
}

#endif                                                                  /*  __TREEOP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
