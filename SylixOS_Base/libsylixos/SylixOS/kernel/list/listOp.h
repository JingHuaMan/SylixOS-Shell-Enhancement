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
** ��   ��   ��: listOp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳʹ�õ���������ͼ��������塣

** BUG
2007.11.13  �����ȡ������һ���ڵ����������, ��ȫ��װ�������.
2009.04.09  ����˳����Դ��, Ԥ���������.
2012.12.07  �����˫������ڵ��Ƿ��������е��жϺ�.
2013.11.29  ָ��ת����ʹ�� size_t ����.
*********************************************************************************************************/

#ifndef __LISTOP_H
#define __LISTOP_H

#include "../SylixOS/kernel/list/listType.h"

/*********************************************************************************************************
  ����ṹ��ʼ��
*********************************************************************************************************/

#define	_LIST_RING_INIT(name)	 { LW_NULL, LW_NULL }
#define _LIST_LINE_INIT(name)    { LW_NULL, LW_NULL }
#define _LIST_MONO_INIT(name)    { LW_NULL }

/*********************************************************************************************************
  ����ṹ��ʼ��
*********************************************************************************************************/

#define	_LIST_RING_INIT_IN_CODE(name) do {                    \
            (name).RING_plistNext = LW_NULL;                  \
            (name).RING_plistPrev = LW_NULL;                  \
        } while (0)
#define _LIST_LINE_INIT_IN_CODE(name) do {                    \
            (name).LINE_plistNext = LW_NULL;                  \
            (name).LINE_plistPrev = LW_NULL;                  \
        } while (0)
#define _LIST_MONO_INIT_IN_CODE(name) do {                    \
            (name).MONO_plistNext = LW_NULL;                  \
        } while (0)
        
/*********************************************************************************************************
  ���б������������ṹ��ʼ��
*********************************************************************************************************/

#define _LIST_RING_HEAD(name)                                 \
        LW_LIST_RING name = _LIST_RING_INIT(name)
#define _LIST_LINE_HEAD(name)                                 \
        LW_LIST_LINE name = _LIST_LINE_INIT(name)
#define _LIST_MONO_HEAD(name)                                 \
        LW_LIST_MONO name = _LIST_MONO_INIT(name)
        
/*********************************************************************************************************
  ����ָ���ʼ��
*********************************************************************************************************/

#define _INIT_LIST_RING_HEAD(ptr) do {                        \
            (ptr)->RING_plistNext = LW_NULL;                  \
            (ptr)->RING_plistPrev = LW_NULL;                  \
        } while (0)
#define _INIT_LIST_LINE_HEAD(ptr) do {                        \
            (ptr)->LINE_plistNext = LW_NULL;                  \
            (ptr)->LINE_plistPrev = LW_NULL;                  \
        } while (0)
#define _INIT_LIST_MONO_HEAD(ptr) do {                        \
            (ptr)->MONO_plistNext = LW_NULL;                  \
        } while (0)
        
/*********************************************************************************************************
  ƫ��������
*********************************************************************************************************/

#define _LIST_OFFSETOF(type, member)                          \
        ((size_t)&((type *)0)->member)
        
/*********************************************************************************************************
  �õ�ptr�������ṹ
*********************************************************************************************************/

#define _LIST_CONTAINER_OF(ptr, type, member)                 \
        ((type *)((size_t)ptr - _LIST_OFFSETOF(type, member)))
        
/*********************************************************************************************************
  �������ĸ��ָ��ṹ
*********************************************************************************************************/

#define _LIST_ENTRY(ptr, type, member)                        \
        _LIST_CONTAINER_OF(ptr, type, member)
        
/*********************************************************************************************************
  �ж������Ƿ�Ϊ��
*********************************************************************************************************/

#define _LIST_RING_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
#define _LIST_LINE_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
#define _LIST_MONO_IS_EMPTY(ptr)                              \
        ((ptr) == LW_NULL)
        
/*********************************************************************************************************
  �жϽڵ��Ƿ���������
*********************************************************************************************************/

#define _LIST_RING_IS_NOTLNK(ptr)                             \
        (((ptr)->RING_plistNext == LW_NULL) || ((ptr)->RING_plistPrev == LW_NULL))
#define _LIST_LINE_IS_NOTLNK(ptr)                             \
        (((ptr)->LINE_plistNext == LW_NULL) && ((ptr)->LINE_plistPrev == LW_NULL))

/*********************************************************************************************************
  ��Դ���ʼ������
*********************************************************************************************************/

#define _LIST_MONO_LINK(ptr, ptrnext)                         \
        ((ptr)->MONO_plistNext = (ptrnext))

/*********************************************************************************************************
  ��һ��
*********************************************************************************************************/

static LW_INLINE VOID _list_ring_next (PLW_LIST_RING  *phead)
{
    *phead = (*phead)->RING_plistNext;
}
static LW_INLINE VOID _list_line_next (PLW_LIST_LINE  *phead)
{
    *phead = (*phead)->LINE_plistNext;
}
static LW_INLINE VOID _list_mono_next (PLW_LIST_MONO  *phead)
{
    *phead = (*phead)->MONO_plistNext;
}

/*********************************************************************************************************
  ��ȡ��һ��
*********************************************************************************************************/

static LW_INLINE PLW_LIST_RING    _list_ring_get_next (PLW_LIST_RING  pring)
{
    return  (pring->RING_plistNext);
}
static LW_INLINE PLW_LIST_LINE    _list_line_get_next (PLW_LIST_LINE  pline)
{
    return  (pline->LINE_plistNext);
}
static LW_INLINE PLW_LIST_MONO    _list_mono_get_next (PLW_LIST_MONO  pmono)
{
    return  (pmono->MONO_plistNext);
}

/*********************************************************************************************************
  ��ȡ��һ��
*********************************************************************************************************/

static LW_INLINE PLW_LIST_RING    _list_ring_get_prev (PLW_LIST_RING  pring)
{
    return  (pring->RING_plistPrev);
}
static LW_INLINE PLW_LIST_LINE    _list_line_get_prev (PLW_LIST_LINE  pline)
{
    return  (pline->LINE_plistPrev);
}

/*********************************************************************************************************
  ��Դ����ط�������ղ���
*********************************************************************************************************/

static LW_INLINE PLW_LIST_MONO _list_mono_allocate (PLW_LIST_MONO  *phead)
{
    REGISTER  PLW_LIST_MONO  pallo;
    
    pallo  = *phead;
    *phead = (*phead)->MONO_plistNext;
    
    return  (pallo);
}
static LW_INLINE VOID _list_mono_free (PLW_LIST_MONO  *phead, PLW_LIST_MONO  pfree)
{
    pfree->MONO_plistNext = *phead;
    *phead = pfree;
}

/*********************************************************************************************************
  ��Դ����ط�������ղ��� (˳����Դ��, Ԥ���������)
*********************************************************************************************************/

static LW_INLINE PLW_LIST_MONO _list_mono_allocate_seq (PLW_LIST_MONO  *phead, PLW_LIST_MONO  *ptail)
{
    REGISTER  PLW_LIST_MONO  pallo;
    
    pallo = *phead;
    if (*ptail == *phead) {
        *ptail = (*phead)->MONO_plistNext;
    }
    *phead = (*phead)->MONO_plistNext;
    
    return  (pallo);
}
static LW_INLINE VOID _list_mono_free_seq (PLW_LIST_MONO  *phead, PLW_LIST_MONO  *ptail, PLW_LIST_MONO  pfree)
{
    pfree->MONO_plistNext = LW_NULL;
    if (*ptail) {                                                       /*  �����п��нڵ�ʱ            */
        (*ptail)->MONO_plistNext = pfree;
    } else {
        *phead = pfree;                                                 /*  û�п��нڵ�                */
    }
    *ptail = pfree;
}

#endif                                                                  /*  __LISTOP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
