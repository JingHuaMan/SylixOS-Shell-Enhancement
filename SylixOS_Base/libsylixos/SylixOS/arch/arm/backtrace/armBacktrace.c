/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: armBacktrace.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ���ܶ�ջ���� (��Դ�� glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "armBacktrace.h"
/*********************************************************************************************************
  This implementation assumes a stack layout that matches the defaults
  used by gcc's `__builtin_frame_address' and `__builtin_return_address'
  (FP is the frame pointer register):

        +-----------------+     +-----------------+
  FP -> | previous FP     |---->| previous FP     |---->...
        |                 |     |                 |
        | return address  |     | return address  |
        +-----------------+     +-----------------+
*********************************************************************************************************/
/*********************************************************************************************************
  Get some notion of the current stack.  Need not be exactly the top
  of the stack, just something somewhere in the current frame.
*********************************************************************************************************/
#ifndef CURRENT_STACK_FRAME
#define CURRENT_STACK_FRAME         ({ char __csf; &__csf; })
#endif
/*********************************************************************************************************
  By default we assume that the stack grows downward.
*********************************************************************************************************/
#ifndef INNER_THAN
#define INNER_THAN                  <
#endif
/*********************************************************************************************************
  By default assume the `next' pointer in struct layout points to the
  next struct layout.
*********************************************************************************************************/
#ifndef ADVANCE_STACK_FRAME
#define ADVANCE_STACK_FRAME(next)   BOUNDED_1((struct layout *) (next))
#endif
/*********************************************************************************************************
  By default, the frame pointer is just what we get from gcc.
*********************************************************************************************************/
#ifndef FIRST_FRAME_POINTER
#define FIRST_FRAME_POINTER         __builtin_frame_address(0)
#endif
/*********************************************************************************************************
** ��������: getEndStack
** ��������: ��ö�ջ������ַ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}
/*********************************************************************************************************
** ��������: backtrace
** ��������: ��õ�ǰ�������ջ
** �䡡��  : array     ��ȡ����
**           size      �����С
** �䡡��  : ��ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  backtrace (void **array, int size)
{
    struct layout *current;
    void *__unbounded top_frame;
    void *__unbounded top_stack;
    void *__unbounded end_stack;
    int cnt = 0;

    top_frame = FIRST_FRAME_POINTER;
    top_stack = CURRENT_STACK_FRAME;
    end_stack = getEndStack();

    /* 
     * We skip the call to this function, it makes no sense to record it.
     */
    current = BOUNDED_1((struct layout *)top_frame);
    current = ADJUST_FRAME_POINTER(current);
    
    while (cnt < size) {
        if ((void *) current INNER_THAN top_stack || !((void *) current INNER_THAN end_stack)) {
            /* 
             * This means the address is out of range.  Note that for the
             * toplevel we see a frame pointer with value NULL which clearly is
             * out of range.
             */
            break;
        }

        array[cnt++] = current->return_address;

        current = ADVANCE_STACK_FRAME(current->next);
        current = ADJUST_FRAME_POINTER(current);
    }

    return cnt;
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
