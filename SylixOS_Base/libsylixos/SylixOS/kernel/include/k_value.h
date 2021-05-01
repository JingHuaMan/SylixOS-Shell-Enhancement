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
** ��   ��   ��: k_value.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 12 ��
**
** ��        ��: ����ϵͳ���������ֵ�����塣

** BUG
2007.11.04  �� LW_TRUE ����Ϊ (BOOL)(-1)
2009.04.04  �� LW_TRUE ����Ϊ 1, �Ա������ļ�����.
2009.07.11  ������뵽ϵͳʱ�Ӹ�����ת��.
2010.05.05  �� LW_TRUE ����ʹ�� (BOOL) ǿ��ת��.
*********************************************************************************************************/

#ifndef __K_VALUE_H
#define __K_VALUE_H

/*********************************************************************************************************
  BOOL
*********************************************************************************************************/

#define LW_FALSE                        (0)                             /*  0x0                         */
#define LW_TRUE                         (1)                             /*  0x1                         */

#ifndef TRUE
#define TRUE                            LW_TRUE
#endif                                                                  /*  TRUE                        */

#ifndef FALSE
#define FALSE                           LW_FALSE
#endif                                                                  /*  FALSE                       */

/*********************************************************************************************************
  NULL
*********************************************************************************************************/

#ifdef __cplusplus
#define LW_NULL                         0                               /*  C++ �µ� NULL               */
#ifndef NULL
#define NULL                            0
#endif                                                                  /*  NULL                        */
#else
#define LW_NULL                         ((PVOID)0)                      /*  C   �µ� NULL               */
#ifndef NULL
#define NULL                            ((PVOID)0)
#endif                                                                  /*  NULL                        */
#endif

/*********************************************************************************************************
  ROUND_UP
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#ifndef ROUND_UP
#define ROUND_UP(x, align)              (size_t)(((size_t)(x) +  (align - 1)) & ~(align - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)            (size_t)( (size_t)(x) & ~(align - 1))
#endif

#ifndef ALIGNED
#define ALIGNED(x, align)               (((size_t)(x) & (align - 1)) == 0)
#endif

/*********************************************************************************************************
  Physical address ROUND_UP
*********************************************************************************************************/

#ifndef PHY_ROUND_UP
#define PHY_ROUND_UP(x, align)          (phys_addr_t)(((phys_addr_t)(x) +  (align - 1)) & ~(align - 1))
#endif

#ifndef PHY_ROUND_DOWN
#define PHY_ROUND_DOWN(x, align)        (phys_addr_t)( (phys_addr_t)(x) & ~(align - 1))
#endif

#ifndef PHY_ALIGNED
#define PHY_ALIGNED(x, align)           (((phys_addr_t)(x) & (align - 1)) == 0)
#endif

/*********************************************************************************************************
  DIV_ROUND_UP
*********************************************************************************************************/

#ifndef DIV_ROUND
#define DIV_ROUND(n, d)                 (((n) + ((d) / 2)) / (d))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)              (((n) + (d) - 1) / (d))
#endif

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  local or global function
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_SYSPERF_EN > 0
#define LW_STATIC
#else
#define LW_STATIC                       static
#endif                                                                  /*  LW_CFG_SYSPERF_EN > 0       */
#endif

/*********************************************************************************************************
  ID_SELF
*********************************************************************************************************/

#ifndef __SYLIXOS_KERNEL
#define LW_ID_SELF                      API_ThreadIdSelf()
#else
#define LW_ID_SELF                      Lw_Thread_Self()
#endif
#define LW_HANDLE_SELF                  LW_ID_SELF

/*********************************************************************************************************
  KERNEL TIMING
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#define LW_TICK_HZ                      _K_timingKernel.TIMING_ulTickHz
#define LW_NSEC_PER_TICK                _K_timingKernel.TIMING_ulNsecPerTick
#define LW_HTIMER_HZ                    _K_timingKernel.TIMING_ulHTimerHz
#define LW_ITIMER_RATE                  _K_timingKernel.TIMING_ulITimerRate
#define LW_HOTPLUG_SEC                  _K_timingKernel.TIMING_ulHotplugSec
#define LW_REBOOT_TO_SEC                _K_timingKernel.TIMING_ulRebootToSec
#define LW_SCHED_SLICE                  _K_timingKernel.TIMING_usSlice
#endif

/*********************************************************************************************************
  micro-second
*********************************************************************************************************/
#define LW_MSECOND_TO_TICK_1(ulMs)      \
        (((ulMs * CLOCKS_PER_SEC) / 1000) ? ((ulMs * CLOCKS_PER_SEC) / 1000) : (1))
         
#define LW_MSECOND_TO_TICK_0(ulMs)      \
        ((ulMs * CLOCKS_PER_SEC) / 1000)

/*********************************************************************************************************
  CACHE LINE ALIGN
*********************************************************************************************************/

#ifdef __GNUC__
#if LW_CFG_SMP_EN > 0 && LW_CFG_CPU_ARCH_CACHE_LINE > 0
#define LW_CACHE_LINE_ALIGN         __attribute__((aligned(LW_CFG_CPU_ARCH_CACHE_LINE)))
#else
#define LW_CACHE_LINE_ALIGN
#endif                                                                  /*  LW_CFG_CPU_ARCH_CACHE_LINE  */
#else
#define LW_CACHE_LINE_ALIGN
#endif

/*********************************************************************************************************
  access once
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#define LW_ACCESS_ONCE(type, x)     (*(volatile type *)&(x))
#endif

#endif                                                                  /*  __K_VALUE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
