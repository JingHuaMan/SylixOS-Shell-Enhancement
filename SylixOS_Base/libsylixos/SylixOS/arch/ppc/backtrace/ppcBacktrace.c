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
** ��   ��   ��: ppcBacktrace.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���ܶ�ջ���� (��Դ�� glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "ppcBacktrace.h"
/*********************************************************************************************************
   This is the stack layout we see with every stack frame.
   Note that every routine is required by the ABI to lay out the stack
   like this.

            +----------------+        +-----------------+
    %r1  -> | %r1 last frame | -----> | %r1 last frame  | ---> ... --> NULL
            |                |        |                 |
            | (unused)       |        | return address  |
            +----------------+        +-----------------+
*********************************************************************************************************/
/*********************************************************************************************************
  By default we assume that the stack grows downward.
*********************************************************************************************************/
#ifndef INNER_THAN
#define INNER_THAN                  <
#endif
/*********************************************************************************************************
  Get some notion of the current stack.  Need not be exactly the top
  of the stack, just something somewhere in the current frame.
*********************************************************************************************************/
#ifndef CURRENT_STACK_FRAME
#define CURRENT_STACK_FRAME         ({ char __csf; &__csf; })
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
int  backtrace (void  **array, int  size)
{
    struct layout    *current;
    void *__unbounded top_stack;
    void *__unbounded end_stack;
    int               count;

    /*
     * Force gcc to spill LR.
     */
    asm volatile ("" : "=l"(current));

    /*
     * Get the address on top-of-stack.
     */
    asm volatile ("lwz %0 , 0(1)" : "=r"(current));
    current = BOUNDED_1(current);

    top_stack = CURRENT_STACK_FRAME;
    end_stack = getEndStack();

    for (count = 0;
         current != NULL && count < size;
         current = BOUNDED_1(current->next), count++) {

        if ((void *) current INNER_THAN top_stack || !((void *) current INNER_THAN end_stack)) {
            /*
             * This means the address is out of range.  Note that for the
             * toplevel we see a frame pointer with value NULL which clearly is
             * out of range.
             */
            break;
        }

        array[count] = current->return_address;
    }

    /* It's possible the second-last stack frame can't return
       (that is, it's __libc_start_main), in which case
       the CRT startup code will have set its LR to 'NULL'.  */
    if (count > 0 && array[count - 1] == NULL) {
        count--;
    }

    return  (count);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
