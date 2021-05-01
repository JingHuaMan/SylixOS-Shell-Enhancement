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
  arm architecture detect
*********************************************************************************************************/

#ifdef __GNUC__
#  if defined(__ARM_ARCH_2__)
#    define __SYLIXOS_ARM_ARCH__    2

#  elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
#    define __SYLIXOS_ARM_ARCH__    3

#  elif defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__)
#    define __SYLIXOS_ARM_ARCH__    4

#  elif defined(__ARM_ARCH_5__)  || defined(__ARM_ARCH_5E__) || \
        defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || \
        defined(__ARM_ARCH_5TEJ__)
#    define __SYLIXOS_ARM_ARCH__    5

#  elif defined(__ARM_ARCH_6__)   || defined(__ARM_ARCH_6J__) || \
        defined(__ARM_ARCH_6K__)  || defined(__ARM_ARCH_6Z__) || \
        defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__)
#    define __SYLIXOS_ARM_ARCH__    6

#  elif defined(__ARM_ARCH_7__)  || defined(__ARM_ARCH_7A__) || \
        defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || \
        defined(__ARM_ARCH_7EM__)
#    define __SYLIXOS_ARM_ARCH__    7

#  elif defined(__ARM_ARCH_8__) || defined(__ARM_ARCH_8A__)
#    define __SYLIXOS_ARM_ARCH__    8
#  endif                                                                /*  user define only            */

#else
#  define __SYLIXOS_ARM_ARCH__      4                                   /*  default ARMv4               */
#endif

/*********************************************************************************************************
  arm R & M architecture detect
*********************************************************************************************************/

#if defined(__ARM_ARCH_7R__)
#  define __SYLIXOS_ARM_ARCH_R__    7

#  undef LW_CFG_ARM_PL330
#  define LW_CFG_ARM_PL330          0

#  undef LW_CFG_ARM_CACHE_L2
#  define LW_CFG_ARM_CACHE_L2       0

#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#  define __SYLIXOS_ARM_ARCH_M__    7

/*********************************************************************************************************
  arm M architecture do not include the following component
*********************************************************************************************************/

#  undef LW_CFG_ARM_CP15
#  define LW_CFG_ARM_CP15           0

#  undef LW_CFG_ARM_PL330
#  define LW_CFG_ARM_PL330          0

#  undef LW_CFG_ARM_CACHE_L2
#  define LW_CFG_ARM_CACHE_L2       0

#  undef LW_CFG_VMM_EN
#  define LW_CFG_VMM_EN             0

#  undef LW_CFG_GDB_EN
#  define LW_CFG_GDB_EN             0

#  undef LW_CFG_MODULELOADER_EN
#  define LW_CFG_MODULELOADER_EN    0

#  undef LW_CFG_SMP_EN
#  define LW_CFG_SMP_EN             0

#  if LW_CFG_CORTEX_M_SVC_SWITCH == 0
#    undef LW_CFG_SIGNAL_EN
#    define LW_CFG_SIGNAL_EN        0

#    undef LW_CFG_COROUTINE_EN
#    define LW_CFG_COROUTINE_EN     0
#  endif

#endif

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

#if __SYLIXOS_ARM_ARCH__ >= 6
#  define LW_CFG_CPU_ATOMIC_EN      1
#  ifndef __SYLIXOS_ARM_ARCH_M__
#    define LW_CFG_CPU_ATOMIC64_EN  1                                   /*  Only for A, R               */
#  else
#    define LW_CFG_CPU_ATOMIC64_EN  0
#  endif

#else
#  define LW_CFG_CPU_ATOMIC_EN      0
#  define LW_CFG_CPU_ATOMIC64_EN    0
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
