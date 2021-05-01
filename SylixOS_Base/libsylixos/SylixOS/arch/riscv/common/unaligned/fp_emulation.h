/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: fp_emulation.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 08 月 29 日
**
** 描        述: RISC-V 体系构架 FPU 模拟相关定义.
*********************************************************************************************************/

#ifndef __ARCH_RISCV_FP_EMU_H
#define __ARCH_RISCV_FP_EMU_H

#include "emulation.h"

#define GET_PRECISION(insn) (((insn) >> 25) & 3)
#define GET_RM(insn) (((insn) >> 12) & 7)
#define PRECISION_S 0
#define PRECISION_D 1

#define GET_F32_REG(insn, pos, regs) ({ \
  register int32_t value asm("a0") = SHIFT_RIGHT(insn, (pos)-3) & 0xf8; \
  uintptr_t tmp; \
  asm ("1: auipc %0, %%pcrel_hi(get_f32_reg); add %0, %0, %1; jalr t0, %0, %%pcrel_lo(1b)" : "=&r"(tmp), "+&r"(value) :: "t0"); \
  value; })

#define SET_F32_REG(insn, pos, regs, val) ({ \
  register uint32_t value asm("a0") = (val); \
  uintptr_t offset = SHIFT_RIGHT(insn, (pos)-3) & 0xf8; \
  uintptr_t tmp; \
  asm volatile ("1: auipc %0, %%pcrel_hi(put_f32_reg); add %0, %0, %2; jalr t0, %0, %%pcrel_lo(1b)" : "=&r"(tmp) : "r"(value), "r"(offset) : "t0"); })

#define GET_F64_REG(insn, pos, regs) ({ \
  register uintptr_t value asm("a0") = SHIFT_RIGHT(insn, (pos)-3) & 0xf8; \
  uintptr_t tmp; \
  asm ("1: auipc %0, %%pcrel_hi(get_f64_reg); add %0, %0, %1; jalr t0, %0, %%pcrel_lo(1b)" : "=&r"(tmp), "+&r"(value) :: "t0"); \
  sizeof(uintptr_t) == 4 ? *(int64_t*)value : (int64_t)value; })

#define SET_F64_REG(insn, pos, regs, val) ({ \
  uint64_t __val = (val); \
  register uintptr_t value asm("a0") = sizeof(uintptr_t) == 4 ? (uintptr_t)&__val : (uintptr_t)__val; \
  uintptr_t offset = SHIFT_RIGHT(insn, (pos)-3) & 0xf8; \
  uintptr_t tmp; \
  asm volatile ("1: auipc %0, %%pcrel_hi(put_f64_reg); add %0, %0, %2; jalr t0, %0, %%pcrel_lo(1b)" : "=&r"(tmp) : "r"(value), "r"(offset) : "t0"); })

#define SET_FS_DIRTY() ((void) 0)

#define GET_F32_RS1(insn, regs) (GET_F32_REG(insn, 15, regs))
#define GET_F32_RS2(insn, regs) (GET_F32_REG(insn, 20, regs))
#define GET_F32_RS3(insn, regs) (GET_F32_REG(insn, 27, regs))
#define GET_F64_RS1(insn, regs) (GET_F64_REG(insn, 15, regs))
#define GET_F64_RS2(insn, regs) (GET_F64_REG(insn, 20, regs))
#define GET_F64_RS3(insn, regs) (GET_F64_REG(insn, 27, regs))
#define SET_F32_RD(insn, regs, val) (SET_F32_REG(insn, 7, regs, val), SET_FS_DIRTY())
#define SET_F64_RD(insn, regs, val) (SET_F64_REG(insn, 7, regs, val), SET_FS_DIRTY())

#define GET_F32_RS2C(insn, regs) (GET_F32_REG(insn, 2, regs))
#define GET_F32_RS2S(insn, regs) (GET_F32_REG(RVC_RS2S(insn), 0, regs))
#define GET_F64_RS2C(insn, regs) (GET_F64_REG(insn, 2, regs))
#define GET_F64_RS2S(insn, regs) (GET_F64_REG(RVC_RS2S(insn), 0, regs))

#endif                                                                  /*  __ARCH_RISCV_FP_EMU_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
