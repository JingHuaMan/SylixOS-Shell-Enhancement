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
** ��   ��   ��: archprob.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 06 �� 08 ��
**
** ��        ��: ƽ̨����̽��.
*********************************************************************************************************/

#ifndef __ARCHPROB_H
#define __ARCHPROB_H

/*********************************************************************************************************
  mips architecture detect
*********************************************************************************************************/

#define __SYLIXOS_MIPS_ARCH_MIPS32      32
#define __SYLIXOS_MIPS_ARCH_MIPS32R2    33
#define __SYLIXOS_MIPS_ARCH_MIPS64      64
#define __SYLIXOS_MIPS_ARCH_MIPS64R2    65

#ifdef __GNUC__
#  if defined(_MIPS_ARCH_MIPS32)
#    define __SYLIXOS_MIPS_ARCH__   __SYLIXOS_MIPS_ARCH_MIPS32

#  elif defined(_MIPS_ARCH_MIPS32R2)
#    define __SYLIXOS_MIPS_ARCH__   __SYLIXOS_MIPS_ARCH_MIPS32R2

#  elif defined(_MIPS_ARCH_MIPS64)
#    define __SYLIXOS_MIPS_ARCH__   __SYLIXOS_MIPS_ARCH_MIPS64

#  elif defined(_MIPS_ARCH_MIPS64R2) || defined(_MIPS_ARCH_HR2)
#    define __SYLIXOS_MIPS_ARCH__   __SYLIXOS_MIPS_ARCH_MIPS64R2

#  endif                                                                /*  user define only            */

#else
#  define __SYLIXOS_MIPS_ARCH__     __SYLIXOS_MIPS_ARCH_MIPS32          /*  default MIPS32              */
#endif

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
