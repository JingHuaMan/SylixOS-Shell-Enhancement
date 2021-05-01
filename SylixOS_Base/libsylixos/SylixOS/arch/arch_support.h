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
** ��   ��   ��: arch_support.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: SylixOS ��ϵ����֧��.
*********************************************************************************************************/

#ifndef __ARCH_SUPPORT_H
#define __ARCH_SUPPORT_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_support.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "./arm64/arm64_support.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_support.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_support.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_support.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_support.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_support.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "./riscv/riscv_support.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "./csky/csky_support.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_SUPPORT_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
