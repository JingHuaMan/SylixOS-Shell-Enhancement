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
** ��   ��   ��: sparcUnaligned.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 10 �� 13 ��
**
** ��        ��: SPARC ��ϵ���ܷǶ����쳣����.
*********************************************************************************************************/
/*
 * unaligned.c: Unaligned load/store trap handling with special
 *              cases for the kernel to do them more quickly.
 *
 * Copyright (C) 1996 David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1996 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 */

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "linux/compat.h"
#include "arch/sparc/param/sparcParam.h"

enum direction {
    load,    /* ld, ldd, ldh, ldsh */
    store,   /* st, std, sth, stsh */
    both,    /* Swap, ldstub, etc. */
    fpload,
    fpstore,
    invalid,
};

/* sparcUnalignedAsm.S */
extern int do_int_load(unsigned long *dest_reg, int size, unsigned long *saddr, int is_signed);
extern int __do_int_store(unsigned long *dst_addr, int size, unsigned long *src_val);

static inline enum direction decode_direction(unsigned int insn)
{
    unsigned long tmp = (insn >> 21) & 1;

    if(!tmp)
        return load;
    else {
        if(((insn>>19)&0x3f) == 15)
            return both;
        else
            return store;
    }
}

/* 8 = double-word, 4 = word, 2 = half-word */
static inline int decode_access_size(unsigned int insn)
{
    insn = (insn >> 19) & 3;

    if(!insn)
        return 4;
    else if(insn == 3)
        return 8;
    else if(insn == 2)
        return 2;
    else {
        return -1; /* just to keep gcc happy. */
    }
}

/* 0x400000 = signed, 0 = unsigned */
static inline int decode_signedness(unsigned int insn)
{
    return (insn & 0x400000);
}

static inline int sign_extend_imm13(int imm)
{
    return imm << 19 >> 19;
}

static inline unsigned long fetch_reg(unsigned int reg, ARCH_REG_CTX *regs)
{
    if (reg == 0) {
        return 0;

    } else if (reg < 8) {
        return regs->REG_uiGlobal[reg];

    } else if (reg < 16) {
        return regs->REG_uiOutput[reg - 8];

    } else if (reg < 24) {
        return regs->REG_uiLocal[reg - 16];

    } else {
        return regs->REG_uiInput[reg - 24];
    }
}

static inline unsigned long *fetch_reg_addr(unsigned int reg, ARCH_REG_CTX *regs)
{
    if (reg < 8) {
        return (unsigned long *)&regs->REG_uiGlobal[reg];

    } else if (reg < 16) {
        return (unsigned long *)&regs->REG_uiOutput[reg - 8];

    } else if (reg < 24) {
        return (unsigned long *)&regs->REG_uiLocal[reg - 16];

    } else {
        return (unsigned long *)&regs->REG_uiInput[reg - 24];
    }
}

static unsigned long compute_effective_address(ARCH_REG_CTX *regs,
                           unsigned int insn)
{
    unsigned int rs1 = (insn >> 14) & 0x1f;
    unsigned int rs2 = insn & 0x1f;

    if(insn & 0x2000) {
        return (fetch_reg(rs1, regs) + sign_extend_imm13(insn));
    } else {
        return (fetch_reg(rs1, regs) + fetch_reg(rs2, regs));
    }
}

static int do_int_store(int reg_num, int size, unsigned long *dst_addr,
            ARCH_REG_CTX *regs)
{
    unsigned long zero[2] = { 0, 0 };
    unsigned long *src_val;

    if (reg_num)
        src_val = fetch_reg_addr(reg_num, regs);
    else {
        src_val = &zero[0];
        if (size == 8)
            zero[1] = fetch_reg(1, regs);
    }
    return __do_int_store(dst_addr, size, src_val);
}

static inline void advance(ARCH_REG_CTX *regs)
{
    regs->REG_uiPc   = regs->REG_uiNPc;
    regs->REG_uiNPc += 4;
}

static inline int floating_point_load_or_store_p(unsigned int insn)
{
    return (insn >> 24) & 1;
}
/*********************************************************************************************************
** ��������: sparcUnalignedHandle
** ��������: SPARC �Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
**           pulAbortAddr      ��ֹ��ַ
**           pabtInfo          ��ֹ��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  sparcUnalignedHandle (ARCH_REG_CTX  *pregctx, addr_t  *pulAbortAddr, PLW_VMM_ABORT  pabtInfo)
{
    SPARC_PARAM    *param = archKernelParamGet();
    UINT            insn;
    enum direction  dir;
    INT             err, size;
    addr_t          addr;

    *pulAbortAddr = (addr_t)-1;

    if ((pregctx->REG_uiPc | pregctx->REG_uiNPc) & 3) {
        *pulAbortAddr = pregctx->REG_uiPc;
        goto  sigbus;
    }

    insn = *(UINT *)pregctx->REG_uiPc;

    if (((insn >> 30) & 3) != 3) {
        goto  sigbus;
    }

    dir  = decode_direction(insn);
    size = decode_access_size(insn);
    if (size < 0) {
        printk("Impossible unaligned trap. insn=%08x\n", insn);
        printk("Byte sized unaligned access?!?!\n");
        goto  sigbus;
    }

    if (floating_point_load_or_store_p(insn)) {
        printk("User FPU load/store unaligned unsupported.\n");
        goto  sigill;
    }

    addr          = compute_effective_address(pregctx, insn);
    *pulAbortAddr = addr;

    /*
     * unsupport unalign access
     */
    if (param->SPARC_bUnalign == LW_FALSE) {
        goto  sigbus;
    }

    switch (dir) {
    case load:
        err = do_int_load(fetch_reg_addr(((insn >> 25) & 0x1f), pregctx),
                          size,
                          (unsigned long *)addr,
                          decode_signedness(insn));
        if (err) {
            goto  sigbus;
        }
        break;

    case store:
        err = do_int_store(((insn >> 25) & 0x1f),
                           size,
                           (unsigned long *)addr,
                           pregctx);
        if (err) {
            goto  sigbus;
        }
        break;

    case both:
        /*
         * the value of SWAP instruction across word boundaries.
         */
        printk("Unaligned SWAP unsupported.\n");
        goto  sigill;
        break;

    default:
        printk("Impossible user unaligned trap.\n");
        goto  sigill;
        break;
    }

    advance(pregctx);
    pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;
    return;

sigbus:
    pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    pabtInfo->VMABT_uiMethod = BUS_ADRALN;
    return;

sigill:
    pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
