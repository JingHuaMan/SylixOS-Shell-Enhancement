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
** ��   ��   ��: compiler.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 18 ��
**
** ��        ��: ��������ض���.
*********************************************************************************************************/

#ifndef __SYS_COMPILER_H
#define __SYS_COMPILER_H

#include "cdefs.h"

/*********************************************************************************************************
  GCC keywords & attribute
*********************************************************************************************************/

#undef __pure
#undef __noinline
#undef __noreturn
#undef __malloc
#undef __must_check
#undef __deprecated
#undef __used
#undef __unused
#undef __packed
#undef __align
#undef __align_max
#undef __section
#undef likely
#undef unlikely

#if defined(__lint__) || defined(lint)
# define __pure
# define __noinline
# define __noreturn
# define __malloc
# define __must_check
# define __deprecated
# define __used
# define __unused
# define __packed
# define __align(x)
# define __align_max
# define __section(S)

#else /* !__lint__ */
# define __pure          __attribute__ ((pure))
# define __noinline      __attribute__ ((noinline))
# define __noreturn      __attribute__ ((noreturn))
# define __malloc        __attribute__ ((malloc))
# define __must_check    __attribute__ ((warn_unused_result))
# define __deprecated    __attribute__ ((deprecated))
# define __used          __attribute__ ((used))
# define __unused        __attribute__ ((unused))
# define __packed        __attribute__ ((packed))
# define __align(x)      __attribute__ ((aligned (x)))
# define __align_max     __attribute__ ((aligned))

/* bsd system use __attribute__ ((__section__(S))) not #S, so compiler bsd source MUST defined BSD */
# ifdef BSD
#  define __section(S)   __attribute__ ((__section__(S)))
# else
#  define __section(S)   __attribute__ ((__section__(#S)))
# endif
#endif /* !__lint__ */

#if !defined(likely) && (defined(__SYLIXOS_KERNEL) || defined(__NEED_LIKELY__))
# ifdef __GNUC__
#  define likely(x)      __builtin_expect(!!(x), 1)
#  define unlikely(x)    __builtin_expect(!!(x), 0)
# else
#  define likely(x)      (x)
#  define unlikely(x)    (x)
# endif
#endif 

#endif                                                                  /*  __SYS_COMPILER_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
