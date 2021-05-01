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
** ��   ��   ��: arch_atomic.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 07 �� 27 ��
**
** ��        ��: SylixOS ��ϵ���� ATOMIC �ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_ATOMIC_H
#define __ARCH_ATOMIC_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "./arm64/arm64_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "./riscv/riscv_atomic.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "./csky/csky_atomic.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_ATOMIC_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
