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
** ��   ��   ��: lib_stdarg.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 13 ��
**
** ��        ��: stdarg.h.

** BUG:
2009.06.30  ʹ�� GCC ǿ���Ƽ�ʹ�ñ������ڽ� stdarg.h
2010.08.26  ��ʹ�� GCC ����ʱ������ֱ��ʹ�ñ������ڽ�������
*********************************************************************************************************/

#ifndef __LIB_STDARG_H
#define __LIB_STDARG_H

/*********************************************************************************************************
  ע��:
        ͨ�������, ��ȷ�����������Ĳ������ݶ�ʹ�ö�ջ, RISC �� CISC ��ͬ. ȷ����һ��������λ����ͨ�����
        һ��ȷ���Ĳ����ڲ�����ջ�е�λ��.
        ����: printf(fmt, ...); �������ͨ�� fmt �����ڶ�ջ�е�λ��, Ȼ��ͨ��ĳ���㷨ƫ������ȷ���������
        ������λ��. ����, ǰ��������: fmt �����ǼĴ�������, Ҳ����˵ fmt ����ʹ�ö�ջ����, ����㷨������
        Ч. ��Ȼ, �ھ��������������, �Դ�������������ζ����ö�ջ, ���Ƶķ��������Դ���.
        
        ���� GCC �� -O3 �Ż��ȼ�ʱʹ���� -finline-function �Ż�ѡ��. ��ѡ���ʹ��Ƶ�ʽϸߵĺ�������������
        ����, �ܶ��������˼Ĵ���ֱ�Ӵ���, ���ڿɱ������������, ��Ϊ��׼����(�����е� fmt)����ǼĴ���
        ����, ����Ĳ��������������Եĺ��. 2009.06.30 GCC -O3 ���� ioctl ʱ, �����˴�������. ioctl �ĵ�
        ��������Ϊ�Ĵ�������, ���µ���������ֱ�Ӵ���. 
        
        ����ǿ���Ƽ�ʹ�� GCC ���� SylixOS ʱ, ʹ�� GGC �ڽ��� stdarg.h ͷ�ļ�.
*********************************************************************************************************/

/*********************************************************************************************************
  �䳤������ջ���� (__EXCLIB_STDARG ��ʾ�����ⲿ stdarg.h)
*********************************************************************************************************/
#if defined(__EXCLIB_STDARG)
#if defined(__TMS320C6X__)
#include <arch/c6x/arch_stdarg.h>
#else
#include <stdarg.h>                                                     /*  ʹ���ⲿ stdarg ��          */
#endif                                                                  /*  __TMS320C6X__               */
/*********************************************************************************************************
  GCC ����ֱ��ʹ�ñ������ڽ�����, ���� -O3 (-finline-function) �Ż�ʱ���ܳ��ִ���
*********************************************************************************************************/
#elif defined(__GNUC__)
#ifndef va_start
#define va_start(v,l)        __builtin_va_start(v,l)
#endif                                                                  /*  va_start                    */

#ifndef va_end
#define va_end(v)            __builtin_va_end(v)
#endif                                                                  /*  va_end                      */

#ifndef va_arg
#define va_arg(v,l)          __builtin_va_arg(v,l)
#endif                                                                  /*  va_arg                      */

#ifndef _VA_LIST_DEFINED
#ifndef _VA_LIST
#ifndef _VA_LIST_T_H
#ifndef __va_list__

typedef __builtin_va_list    va_list;

#endif                                                                  /*  __va_list__                 */
#endif                                                                  /*  _VA_LIST_T_H                */
#endif                                                                  /*  _VA_LIST                    */
#endif                                                                  /*  _VA_LIST_DEFINED            */

#ifndef _VA_LIST
#define _VA_LIST
#endif                                                                  /*  _VA_LIST                    */
#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
#endif                                                                  /*  _VA_LIST_DEFINED            */
#ifndef _VA_LIST_T_H
#define _VA_LIST_T_H
#endif                                                                  /*  _VA_LIST_T_H                */
#ifndef __va_list__
#define __va_list__
#endif                                                                  /*  __va_list__                 */

#if !defined(__STRICT_ANSI__) || \
    (defined(__STDC_VERSION__) && (__STDC_VERSION__ + 0 >= 199900L)) || \
    defined(__GXX_EXPERIMENTAL_CXX0X__)
#ifndef va_copy
#define va_copy(d,s)	     __builtin_va_copy(d,s)
#endif                                                                  /*  va_copy                     */
#endif                                                                  /*  __STRICT_ANSI__ ...         */

#ifndef __va_copy
#define __va_copy(d,s)	     __builtin_va_copy(d,s)
#endif                                                                  /*  __va_copy                   */
/*********************************************************************************************************
  �����������쳣ʱ, ��ѡ���ⲿ stdarg ��
*********************************************************************************************************/
#else
#include "../SylixOS/config/cpu/cpu_cfg.h"
#include "../SylixOS/include/arch/arch_inc.h"

typedef char  *va_list;

#define __STACK_SIZEOF(n)    ((sizeof(n) + sizeof(LW_STACK) - 1) & ~(sizeof(LW_STACK) - 1))

#if LW_CFG_ARG_STACK_GROWTH == 0                                        /*  ����������ַ����            */
#define va_start(ap, v)      (ap = (va_list)&v + __STACK_SIZEOF(v))
#define va_arg(ap, t)        (*(t *)((ap += __STACK_SIZEOF(t)) - __STACK_SIZEOF(t)))
#define va_end(ap)           (ap = (va_list)0)
#else                                                                   /*  ����������ַ�ݼ�            */
#define va_start(ap, v)      (ap = (va_list)&v - __STACK_SIZEOF(v))
#define va_arg(ap, t)        (*(t *)((ap -= __STACK_SIZEOF(t)) + __STACK_SIZEOF(t)))
#define va_end(ap)           (ap = (va_list)0)
#endif                                                                  /*  LW_CFG_ARG_STACK_GROWTH     */

#endif                                                                  /*  __EXLIB_STDARG || __GNUC__  */

#endif                                                                  /*  __LIB_STDARG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
