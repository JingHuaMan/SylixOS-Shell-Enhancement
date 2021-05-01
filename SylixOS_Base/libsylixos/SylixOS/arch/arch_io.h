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
** ��   ��   ��: arch_io.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: SylixOS ��ϵ���� I/O ���ʽӿ�.
*********************************************************************************************************/

#ifndef __ARCH_IO_H
#define __ARCH_IO_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_io.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "./arm64/arm64_io.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_io.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_io.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_io.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_io.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_io.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "./riscv/riscv_io.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "./csky/csky_io.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_IO_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
