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
** 文   件   名: misaligned_ldst.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 08 月 29 日
**
** 描        述: RISC-V 体系构架非对齐处理.
*********************************************************************************************************/
#ifdef SYLIXOS
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "bits.h"
#endif

#include "emulation.h"
#include "fp_emulation.h"
#include "unprivileged_memory.h"
#ifndef SYLIXOS
#include "mtrap.h"
#include "config.h"
#include "pk.h"
#endif

#if LW_CFG_RISCV_M_LEVEL > 0

union byte_array {
  uint8_t bytes[8];
  uintptr_t intx;
  uint64_t int64;
};

static inline int insn_len(long insn)
{
  return (insn & 0x3) < 0x3 ? 2 : 4;
}

int misaligned_load_trap(ARCH_REG_CTX  *pregctx)
{
  union byte_array val;
  uintptr_t mstatus;
  insn_t insn = get_insn(pregctx->REG_ulEpc, &mstatus);
  uintptr_t npc = pregctx->REG_ulEpc + insn_len(insn);
  uintptr_t addr = pregctx->REG_ulTrapVal;
  uintptr_t *regs = (uintptr_t *)pregctx->REG_ulReg;

  int shift = 0, fp = 0, len;
  if ((insn & MASK_LW) == MATCH_LW)
    len = 4, shift = 8*(sizeof(uintptr_t) - len);
#if __riscv_xlen == 64
  else if ((insn & MASK_LD) == MATCH_LD)
    len = 8, shift = 8*(sizeof(uintptr_t) - len);
  else if ((insn & MASK_LWU) == MATCH_LWU)
    len = 4;
#endif
#ifdef PK_ENABLE_FP_EMULATION
  else if ((insn & MASK_FLD) == MATCH_FLD)
    fp = 1, len = 8;
  else if ((insn & MASK_FLW) == MATCH_FLW)
    fp = 1, len = 4;
#endif
  else if ((insn & MASK_LH) == MATCH_LH)
    len = 2, shift = 8*(sizeof(uintptr_t) - len);
  else if ((insn & MASK_LHU) == MATCH_LHU)
    len = 2;
#ifdef __riscv_compressed
# if __riscv_xlen >= 64
  else if ((insn & MASK_C_LD) == MATCH_C_LD)
    len = 8, shift = 8*(sizeof(uintptr_t) - len), insn = RVC_RS2S(insn) << SH_RD;
  else if ((insn & MASK_C_LDSP) == MATCH_C_LDSP && ((insn >> SH_RD) & 0x1f))
    len = 8, shift = 8*(sizeof(uintptr_t) - len);
# endif
  else if ((insn & MASK_C_LW) == MATCH_C_LW)
    len = 4, shift = 8*(sizeof(uintptr_t) - len), insn = RVC_RS2S(insn) << SH_RD;
  else if ((insn & MASK_C_LWSP) == MATCH_C_LWSP && ((insn >> SH_RD) & 0x1f))
    len = 4, shift = 8*(sizeof(uintptr_t) - len);
# ifdef PK_ENABLE_FP_EMULATION
  else if ((insn & MASK_C_FLD) == MATCH_C_FLD)
    fp = 1, len = 8, insn = RVC_RS2S(insn) << SH_RD;
  else if ((insn & MASK_C_FLDSP) == MATCH_C_FLDSP)
    fp = 1, len = 8;
#  if __riscv_xlen == 32
  else if ((insn & MASK_C_FLW) == MATCH_C_FLW)
    fp = 1, len = 4, insn = RVC_RS2S(insn) << SH_RD;
  else if ((insn & MASK_C_FLWSP) == MATCH_C_FLWSP)
    fp = 1, len = 4;
#  endif
# endif
#endif
  else
    return (PX_ERROR);

  val.int64 = 0;
  for (intptr_t i = 0; i < len; i++)
    val.bytes[i] = load_uint8_t((void *)(addr + i), pregctx->REG_ulEpc);

  if (!fp)
    SET_RD(insn, regs, (intptr_t)val.intx << shift >> shift);
  else if (len == 8)
    SET_F64_RD(insn, regs, val.int64);
  else
    SET_F32_RD(insn, regs, val.intx);

  pregctx->REG_ulEpc = npc;

  return (ERROR_NONE);
}

int misaligned_store_trap(ARCH_REG_CTX  *pregctx)
{
  union byte_array val;
  uintptr_t mstatus;
  insn_t insn = get_insn(pregctx->REG_ulEpc, &mstatus);
  uintptr_t npc = pregctx->REG_ulEpc + insn_len(insn);
  int len;
  uintptr_t *regs = (uintptr_t *)pregctx->REG_ulReg;

  val.intx = GET_RS2(insn, regs);
  if ((insn & MASK_SW) == MATCH_SW)
    len = 4;
#if __riscv_xlen == 64
  else if ((insn & MASK_SD) == MATCH_SD)
    len = 8;
#endif
#ifdef PK_ENABLE_FP_EMULATION
  else if ((insn & MASK_FSD) == MATCH_FSD)
    len = 8, val.int64 = GET_F64_RS2(insn, regs);
  else if ((insn & MASK_FSW) == MATCH_FSW)
    len = 4, val.intx = GET_F32_RS2(insn, regs);
#endif
  else if ((insn & MASK_SH) == MATCH_SH)
    len = 2;
#ifdef __riscv_compressed
# if __riscv_xlen >= 64
  else if ((insn & MASK_C_SD) == MATCH_C_SD)
    len = 8, val.intx = GET_RS2S(insn, regs);
  else if ((insn & MASK_C_SDSP) == MATCH_C_SDSP && ((insn >> SH_RD) & 0x1f))
    len = 8, val.intx = GET_RS2C(insn, regs);
# endif
  else if ((insn & MASK_C_SW) == MATCH_C_SW)
    len = 4, val.intx = GET_RS2S(insn, regs);
  else if ((insn & MASK_C_SWSP) == MATCH_C_SWSP && ((insn >> SH_RD) & 0x1f))
    len = 4, val.intx = GET_RS2C(insn, regs);
# ifdef PK_ENABLE_FP_EMULATION
  else if ((insn & MASK_C_FSD) == MATCH_C_FSD)
    len = 8, val.int64 = GET_F64_RS2S(insn, regs);
  else if ((insn & MASK_C_FSDSP) == MATCH_C_FSDSP)
    len = 8, val.int64 = GET_F64_RS2C(insn, regs);
#  if __riscv_xlen == 32
  else if ((insn & MASK_C_FSW) == MATCH_C_FSW)
    len = 4, val.intx = GET_F32_RS2S(insn, regs);
  else if ((insn & MASK_C_FSWSP) == MATCH_C_FSWSP)
    len = 4, val.intx = GET_F32_RS2C(insn, regs);
#  endif
# endif
#endif
  else
    return (PX_ERROR);

  uintptr_t addr = pregctx->REG_ulTrapVal;
  for (int i = 0; i < len; i++)
    store_uint8_t((void *)(addr + i), val.bytes[i], pregctx->REG_ulEpc);

  pregctx->REG_ulEpc = npc;

  return (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
