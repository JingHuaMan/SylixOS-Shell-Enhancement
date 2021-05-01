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
** ��   ��   ��: arch_inc.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ��ϵ���ͷ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_INC_H
#define __ARCH_INC_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "arm/arch_types.h"
#include "arm/arch_def.h"
#include "arm/arch_compiler.h"
#include "arm/arch_float.h"
#include "arm/arch_limits.h"
#include "arm/arch_regs.h"
#include "arm/arch_mmu.h"
#include "arm/arch_mpu.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "arm64/arch_types.h"
#include "arm64/arch_def.h"
#include "arm64/arch_compiler.h"
#include "arm64/arch_float.h"
#include "arm64/arch_limits.h"
#include "arm64/arch_regs.h"
#include "arm64/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "x86/arch_types.h"
#include "x86/arch_compiler.h"
#include "x86/arch_float.h"
#include "x86/arch_limits.h"
#include "x86/arch_regs.h"
#include "x86/arch_mmu.h"
#include "x86/arch_pc.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "mips/arch_types.h"
#include "mips/arch_def.h"
#include "mips/arch_compiler.h"
#include "mips/arch_float.h"
#include "mips/arch_dsp.h"
#include "mips/arch_limits.h"
#include "mips/arch_regs.h"
#include "mips/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "ppc/arch_types.h"
#include "ppc/arch_def.h"
#include "ppc/arch_compiler.h"
#include "ppc/arch_float.h"
#include "ppc/arch_dsp.h"
#include "ppc/arch_limits.h"
#include "ppc/arch_regs.h"
#include "ppc/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "c6x/arch_types.h"
#include "c6x/arch_compiler.h"
#include "c6x/arch_float.h"
#include "c6x/arch_limits.h"
#include "c6x/arch_regs.h"
#include "c6x/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "sparc/arch_types.h"
#include "sparc/arch_compiler.h"
#include "sparc/arch_def.h"
#include "sparc/arch_float.h"
#include "sparc/arch_limits.h"
#include "sparc/arch_regs.h"
#include "sparc/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "riscv/arch_types.h"
#include "riscv/arch_compiler.h"
#include "riscv/arch_def.h"
#include "riscv/arch_float.h"
#include "riscv/arch_limits.h"
#include "riscv/arch_regs.h"
#include "riscv/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "csky/arch_types.h"
#include "csky/arch_compiler.h"
#include "csky/arch_def.h"
#include "csky/arch_float.h"
#include "csky/arch_limits.h"
#include "csky/arch_regs.h"
#include "csky/arch_mmu.h"
#include "csky/arch_mpu.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_INC_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
