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
** ��   ��   ��: branch.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 25 ��
**
** ��        ��: MIPS ��֧Ԥ��.
*********************************************************************************************************/
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 2001 by Ralf Baechle
 */
#ifndef __ASM_BRANCH_H
#define __ASM_BRANCH_H

/*
 * microMIPS bitfields
 */
#define MM_POOL32A_MINOR_MASK   0x3f
#define MM_POOL32A_MINOR_SHIFT  0x6
#define MM_MIPS32_COND_FC       0x30

static inline int delay_slot (ARCH_REG_CTX *regs)
{
    return  (regs->REG_ulCP0Cause & CAUSEF_BD);
}

static inline void clear_delay_slot (ARCH_REG_CTX *regs)
{
    regs->REG_ulCP0Cause &= ~CAUSEF_BD;
}

static inline void set_delay_slot (ARCH_REG_CTX *regs)
{
    regs->REG_ulCP0Cause |= CAUSEF_BD;
}

static inline unsigned long exception_epc (ARCH_REG_CTX *regs)
{
    if (likely(!delay_slot(regs))) {
        return 	(regs->REG_ulCP0Epc);
    }

    return  (regs->REG_ulCP0Epc + 4);
}

#define BRANCH_LIKELY_TAKEN 0x0001

extern int __compute_return_epc(ARCH_REG_CTX *regs);

static inline int compute_return_epc (ARCH_REG_CTX *regs)
{
    if (!delay_slot(regs)) {
        regs->REG_ulCP0Epc += 4;
        return  (0);
    }

    return  (__compute_return_epc(regs));
}

#endif                                                                  /*  __ASM_BRANCH_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
