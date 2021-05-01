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
** ��   ��   ��: assembler.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 06 �� 14 ��
**
** ��        ��: ��ϵ�ṹ������.
*********************************************************************************************************/

#ifndef __ARCH_ASSEMBLER_H
#define __ARCH_ASSEMBLER_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "arm/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_ARM64)
#include "arm64/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "x86/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "mips/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "ppc/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "c6x/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "sparc/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_RISCV)
#include "riscv/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_CSKY)
#include "csky/asm/assembler.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_ASSEMBLER_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
