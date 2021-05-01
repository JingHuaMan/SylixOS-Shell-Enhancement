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
** ��   ��   ��: arch_limits.h
**
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC limits ���.
*********************************************************************************************************/

#ifndef __SPARC_ARCH_LIMITS_H
#define __SPARC_ARCH_LIMITS_H

#ifndef __ARCH_CHAR_BIT
#define __ARCH_CHAR_BIT            8
#endif

#ifndef __ARCH_CHAR_MAX
#define __ARCH_CHAR_MAX            127
#endif

#ifndef __ARCH_CHAR_MIN
#define __ARCH_CHAR_MIN            (-127-1)
#endif

#ifndef __ARCH_SHRT_MAX
#define __ARCH_SHRT_MAX            32767
#endif

#ifndef __ARCH_SHRT_MIN
#define __ARCH_SHRT_MIN            (-32767-1)
#endif

#ifndef __ARCH_INT_MAX
#define __ARCH_INT_MAX             2147483647
#endif

#ifndef __ARCH_INT_MIN
#define __ARCH_INT_MIN             (-2147483647-1)
#endif

#ifndef __ARCH_LONG_MAX
#define __ARCH_LONG_MAX            2147483647
#endif

#ifndef __ARCH_LONG_MIN
#define __ARCH_LONG_MIN            (-2147483647-1)
#endif

#ifndef __ARCH_SCHAR_MAX
#define __ARCH_SCHAR_MAX           127
#endif

#ifndef __ARCH_SCHAR_MIN
#define __ARCH_SCHAR_MIN           (-127-1)
#endif

#ifndef __ARCH_UCHAR_MAX
#define __ARCH_UCHAR_MAX           255
#endif

#ifndef __ARCH_USHRT_MAX
#define __ARCH_USHRT_MAX           65535
#endif

#ifndef __ARCH_UINT_MAX
#ifdef  __STDC__
#define __ARCH_UINT_MAX            4294967295u
#else
#define __ARCH_UINT_MAX            4294967295
#endif
#endif

#ifndef __ARCH_ULONG_MAX
#ifdef  __STDC__
#define __ARCH_ULONG_MAX           4294967295ul
#else
#define __ARCH_ULONG_MAX           4294967295
#endif
#endif

#endif                                                                  /*  __SPARC_ARCH_LIMITS_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
