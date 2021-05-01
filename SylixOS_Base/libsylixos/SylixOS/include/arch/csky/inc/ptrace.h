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
** 文   件   名: ptrace.h
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2018 年 06 月 05 日
**
** 描        述: ptrace 头文件.
*********************************************************************************************************/
/*
 * Common low level (register) ptrace helpers
 *
 * Copyright 2004-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __ASM_GENERIC_PTRACE_H
#define __ASM_GENERIC_PTRACE_H

#ifndef __ASSEMBLY__

/* Helpers for working with the instruction pointer */
#ifndef GET_IP
#define GET_IP(regs)      ((regs)->REG_ulPc)
#endif
#ifndef SET_IP
#define SET_IP(regs, val) (GET_IP(regs) = (val))
#endif

static inline unsigned long instruction_pointer(ARCH_REG_CTX  *pregctx)
{
    return GET_IP(pregctx);
}

static inline void instruction_pointer_set(ARCH_REG_CTX  *pregctx,
                                           unsigned long val)
{
    SET_IP(pregctx, val);
}

#ifndef profile_pc
#define profile_pc(regs) instruction_pointer(regs)
#endif

#endif                                                                  /* __ASSEMBLY__                 */

#endif                                                                  /* __ASM_GENERIC_PTRACE_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
