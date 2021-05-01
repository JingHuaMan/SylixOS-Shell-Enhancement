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
** ��   ��   ��: lib_lib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ������
*********************************************************************************************************/

#ifndef __LIB_LIB_H
#define __LIB_LIB_H

#ifndef __EXCLIB

#define __LW_RETU_FUNC_DECLARE(ret, name, declare) \
        ret name declare;

#include "./libc/assert/lib_assert.h"
#include "./libc/string/lib_string.h"
#include "./libc/stdlib/lib_stdlib.h"
#include "./libc/ctype/lib_ctype.h"
#include "./libc/time/lib_time.h"
#include "./libc/float/lib_float.h"

#if defined(__SYLIXOS_STDARG) || !defined(__SYLIXOS_KERNEL)
#include "./libc/stdarg/lib_stdarg.h"
#endif

#if defined(__SYLIXOS_STDIO) || !defined(__SYLIXOS_KERNEL)
#include "./libc/stdio/lib_stdio.h"
#endif                                                                  /*  defined(__SYLIXOS_STDIO)... */

#if defined(__SYLIXOS_LIMITS) || !defined(__SYLIXOS_KERNEL)
#include "./libc/limits/lib_limits.h"
#endif

/*********************************************************************************************************
  ����ͷ�ļ�Ĭ�ϲ����� (SylixOS ϵͳ������Ҫ)
*********************************************************************************************************/

#if defined(__SYLIXOS_STDINT)
#include "./libc/stdint/lib_stdint.h"
#endif

#if defined(__SYLIXOS_INTTYPES)
#include "./libc/inttypes/lib_inttypes.h"
#endif

#if defined(__SYLIXOS_PANIC)
#include "./libc/error/lib_error.h"
#endif

/*********************************************************************************************************
  newlib reent ���ݲ�
*********************************************************************************************************/

#if defined(__SYLIXOS_STDIO) || !defined(__SYLIXOS_KERNEL)
#include "./nl_compatible/nl_reent.h"
#endif                                                                  /*  defined(__SYLIXOS_STDIO)... */

#endif                                                                  /*  __EXCLIB                    */
#endif                                                                  /*  __LIB_LIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
