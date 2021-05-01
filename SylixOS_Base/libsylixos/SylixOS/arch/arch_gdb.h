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
** ��   ��   ��: arch_gdb.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 22 ��
**
** ��        ��: SylixOS ��ϵ���� GDB ���Խӿ�.
*********************************************************************************************************/

#ifndef __ARCH_GDB_H
#define __ARCH_GDB_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "./arm64/arm64_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "./riscv/riscv_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "./csky/csky_gdb.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_GDB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
