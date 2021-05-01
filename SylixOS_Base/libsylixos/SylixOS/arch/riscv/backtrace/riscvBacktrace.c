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
** ��   ��   ��: riscvBacktrace.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ���ܶ�ջ���� (��Դ�� glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "riscvBacktrace.h"
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
  By default, the frame pointer is just what we get from gcc.
*********************************************************************************************************/
#ifndef FIRST_FRAME_POINTER
#define FIRST_FRAME_POINTER         __builtin_frame_address(0)
#endif
/*********************************************************************************************************
** ��������: getEndStack
** ��������: ��ö�ջ������ַ
** �䡡��  : NONE
** �䡡��  : ��ջ������ַ
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

    asm volatile ("move %0, s0" : "=r"(top_frame));

    top_stack = CURRENT_STACK_FRAME;
    end_stack = getEndStack();

    /*
     * We skip the call to this function, it makes no sense to record it.
     */
    current = (struct layout *)((unsigned long)top_frame - 2 * sizeof(unsigned long));

    while (cnt < size) {
        if (((void *)current INNER_THAN top_stack) || (!((void *)current INNER_THAN end_stack))) {
            /*
             * This means the address is out of range.  Note that for the
             * toplevel we see a frame pointer with value NULL which clearly is
             * out of range.
             */
            break;
        }

        if (current->return_address == 0) {
            break;
        }

        array[cnt++] = current->return_address;

        current = (struct layout *)((unsigned long)(current->next) - 2 * sizeof(unsigned long));
    }

    return  (cnt);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
