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
** ��   ��   ��: mipsUnaligned.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 25 ��
**
** ��        ��: MIPS �Ƕ��봦��.
*********************************************************************************************************/
/*
 * Handle unaligned accesses by emulation.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1998, 1999, 2002 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2014 Imagination Technologies Ltd.
 *
 * This file contains exception handler for address error exception with the
 * special capability to execute faulting instructions in software.  The
 * handler does not try to handle the case when the program counter points
 * to an address not aligned to a word boundary.
 *
 * Putting data to unaligned addresses is a bad practice even on Intel where
 * only the performance is affected.  Much worse is that such code is non-
 * portable.  Due to several programs that die on MIPS due to alignment
 * problems I decided to implement this handler anyway though I originally
 * didn't intend to do this at all for user code.
 *
 * For now I enable fixing of address errors by default to make life easier.
 * I however intend to disable this somewhen in the future when the alignment
 * problems with user programs have been fixed.	 For programmers this is the
 * right way to go.
 *
 * Fixing address errors is a per process option.  The option is inherited
 * across fork(2) and execve(2) calls.	If you really want to use the
 * option in your user programs - I discourage the use of the software
 * emulation strongly - use the following code in your userland stuff:
 *
 * #include <sys/sysmips.h>
 *
 * ...
 * sysmips(MIPS_FIXADE, x);
 * ...
 *
 * The argument x is 0 for disabling software emulation, enabled otherwise.
 *
 * Below a little program to play around with this feature.
 *
 * #include <stdio.h>
 * #include <sys/sysmips.h>
 *
 * struct foo {
 *	   unsigned char bar[8];
 * };
 *
 * main(int argc, char *argv[])
 * {
 *	   struct foo x = {0, 1, 2, 3, 4, 5, 6, 7};
 *	   unsigned int *p = (unsigned int *) (x.bar + 3);
 *	   int i;
 *
 *	   if (argc > 1)
 *		   sysmips(MIPS_FIXADE, atoi(argv[1]));
 *
 *	   printf("*p = %08lx\n", *p);
 *
 *	   *p = 0xdeadface;
 *
 *	   for(i = 0; i <= 7; i++)
 *	   printf("%02x ", x.bar[i]);
 *	   printf("\n");
 * }
 *
 * Coprocessor loads are not supported; I think this case is unimportant
 * in the practice.
 *
 * TODO: Handle ndc (attempted store to doubleword in uncached memory)
 *	 exception for the R6000.
 *	 A store crossing a page boundary might be executed only partially.
 *	 Undo the partial store in this case.
 */

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>
#include "arch/mips/inc/config.h"
#include "arch/mips/inc/porting.h"
#include "arch/mips/inc/asm-eva.h"
#include "arch/mips/inc/inst.h"
#include "arch/mips/inc/branch.h"
#if LW_CFG_CPU_FPU_EN > 0
#include "arch/mips/fpu/emu/mipsFpuEmu.h"
#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
#include "arch/mips/inc/msa.h"
#endif                                                                  /*  LW_CFG_MIPS_HAS_MSA_INSTR   */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#undef  PTR
#define PTR                 .word

#define IS_ENABLED(config)  MIPS_##config

#define cp0_epc             REG_ulCP0Epc
#define regs                REG_ulReg

#if BYTE_ORDER == BIG_ENDIAN
#define     _LoadHW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (".set\tnoat\n"        \
			"1:\t"type##_lb("%0", "0(%2)")"\n"  \
			"2:\t"type##_lbu("$1", "1(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\t.set\tat\n\t"                  \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadW(addr, value, res, type)   \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "(%2)")"\n"   \
			"2:\t"type##_lwr("%0", "3(%2)")"\n\t"\
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has no lwl instruction */
#define     _LoadW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n"			    \
			".set\tnoat\n\t"		    \
			"1:"type##_lb("%0", "0(%2)")"\n\t"  \
			"2:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "3(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */

#define     _LoadHWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_lbu("%0", "0(%2)")"\n" \
			"2:\t"type##_lbu("$1", "1(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".set\tat\n\t"                      \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadWU(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "(%2)")"\n"  \
			"2:\t"type##_lwr("%0", "3(%2)")"\n\t"\
			"dsll\t%0, %0, 32\n\t"              \
			"dsrl\t%0, %0, 32\n\t"              \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tldl\t%0, (%2)\n"               \
			"2:\tldr\t%0, 7(%2)\n\t"            \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has not lwl and ldl instructions */
#define	    _LoadWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_lbu("%0", "0(%2)")"\n\t" \
			"2:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "3(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:lb\t%0, 0(%2)\n\t"    	    \
			"2:lbu\t $1, 1(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:lbu\t$1, 2(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:lbu\t$1, 3(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"5:lbu\t$1, 4(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"6:lbu\t$1, 5(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"7:lbu\t$1, 6(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"8:lbu\t$1, 7(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n\t"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */


#define     _StoreHW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_sb("%1", "1(%2)")"\n"  \
			"srl\t$1, %1, 0x8\n"                \
			"2:\t"type##_sb("$1", "0(%2)")"\n"  \
			".set\tat\n\t"                      \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=r" (res)                        \
			: "r" (value), "r" (addr), "i" (-EFAULT));\
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_swl("%1", "(%2)")"\n"  \
			"2:\t"type##_swr("%1", "3(%2)")"\n\t"\
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tsdl\t%1,(%2)\n"                \
			"2:\tsdr\t%1, 7(%2)\n\t"            \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#else
/* MIPSR6 has no swl and sdl instructions */
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_sb("%1", "3(%2)")"\n\t"  \
			"srl\t$1, %1, 0x8\n\t"		    \
			"2:"type##_sb("$1", "2(%2)")"\n\t"  \
			"srl\t$1, $1,  0x8\n\t"		    \
			"3:"type##_sb("$1", "1(%2)")"\n\t"  \
			"srl\t$1, $1, 0x8\n\t"		    \
			"4:"type##_sb("$1", "0(%2)")"\n\t"  \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:sb\t%1, 7(%2)\n\t"    	    \
			"dsrl\t$1, %1, 0x8\n\t"		    \
			"2:sb\t$1, 6(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"3:sb\t$1, 5(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"4:sb\t$1, 4(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"5:sb\t$1, 3(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"6:sb\t$1, 2(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"7:sb\t$1, 1(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"8:sb\t$1, 0(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */

#else /* __BIG_ENDIAN */

#define     _LoadHW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (".set\tnoat\n"        \
			"1:\t"type##_lb("%0", "1(%2)")"\n"  \
			"2:\t"type##_lbu("$1", "0(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\t.set\tat\n\t"                  \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadW(addr, value, res, type)   \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "3(%2)")"\n" \
			"2:\t"type##_lwr("%0", "(%2)")"\n\t"\
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has no lwl instruction */
#define     _LoadW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n"			    \
			".set\tnoat\n\t"		    \
			"1:"type##_lb("%0", "3(%2)")"\n\t"  \
			"2:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "0(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */


#define     _LoadHWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_lbu("%0", "1(%2)")"\n" \
			"2:\t"type##_lbu("$1", "0(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".set\tat\n\t"                      \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadWU(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "3(%2)")"\n" \
			"2:\t"type##_lwr("%0", "(%2)")"\n\t"\
			"dsll\t%0, %0, 32\n\t"              \
			"dsrl\t%0, %0, 32\n\t"              \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tldl\t%0, 7(%2)\n"              \
			"2:\tldr\t%0, (%2)\n\t"             \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has not lwl and ldl instructions */
#define	    _LoadWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_lbu("%0", "3(%2)")"\n\t" \
			"2:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "0(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:lb\t%0, 7(%2)\n\t"    	    \
			"2:lbu\t$1, 6(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:lbu\t$1, 5(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:lbu\t$1, 4(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"5:lbu\t$1, 3(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"6:lbu\t$1, 2(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"7:lbu\t$1, 1(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"8:lbu\t$1, 0(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n\t"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)
#endif /* CONFIG_CPU_MIPSR6 */

#define     _StoreHW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_sb("%1", "0(%2)")"\n"  \
			"srl\t$1,%1, 0x8\n"                 \
			"2:\t"type##_sb("$1", "1(%2)")"\n"  \
			".set\tat\n\t"                      \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=r" (res)                        \
			: "r" (value), "r" (addr), "i" (-EFAULT));\
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_swl("%1", "3(%2)")"\n" \
			"2:\t"type##_swr("%1", "(%2)")"\n\t"\
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tsdl\t%1, 7(%2)\n"              \
			"2:\tsdr\t%1, (%2)\n\t"             \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#else
/* MIPSR6 has no swl and sdl instructions */
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_sb("%1", "0(%2)")"\n\t"  \
			"srl\t$1, %1, 0x8\n\t"		    \
			"2:"type##_sb("$1", "1(%2)")"\n\t"  \
			"srl\t$1, $1,  0x8\n\t"		    \
			"3:"type##_sb("$1", "2(%2)")"\n\t"  \
			"srl\t$1, $1, 0x8\n\t"		    \
			"4:"type##_sb("$1", "3(%2)")"\n\t"  \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:sb\t%1, 0(%2)\n\t"    	    \
			"dsrl\t$1, %1, 0x8\n\t"		    \
			"2:sb\t$1, 1(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"3:sb\t$1, 2(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"4:sb\t$1, 3(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"5:sb\t$1, 4(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"6:sb\t$1, 5(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"7:sb\t$1, 6(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"8:sb\t$1, 7(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */
#endif

#define LoadHWU(addr, value, res)	_LoadHWU(addr, value, res, kernel)
#define LoadHWUE(addr, value, res)	_LoadHWU(addr, value, res, user)
#define LoadWU(addr, value, res)	_LoadWU(addr, value, res, kernel)
#define LoadWUE(addr, value, res)	_LoadWU(addr, value, res, user)
#define LoadHW(addr, value, res)	_LoadHW(addr, value, res, kernel)
#define LoadHWE(addr, value, res)	_LoadHW(addr, value, res, user)
#define LoadW(addr, value, res)		_LoadW(addr, value, res, kernel)
#define LoadWE(addr, value, res)	_LoadW(addr, value, res, user)
#define LoadDW(addr, value, res)	_LoadDW(addr, value, res)

#define StoreHW(addr, value, res)	_StoreHW(addr, value, res, kernel)
#define StoreHWE(addr, value, res)	_StoreHW(addr, value, res, user)
#define StoreW(addr, value, res)	_StoreW(addr, value, res, kernel)
#define StoreWE(addr, value, res)	_StoreW(addr, value, res, user)
#define StoreDW(addr, value, res)	_StoreDW(addr, value, res)

static void emulate_load_store_insn(ARCH_REG_CTX *regs,
	void __user *addr, unsigned int __user *pc, PLW_VMM_ABORT pabtInfo)
{
	union mips_instruction insn;
	unsigned long value;
	unsigned int res;
	unsigned long origpc;
	unsigned long orig31;
	void __user *fault_addr = NULL;
#ifdef	CONFIG_EVA
	mm_segment_t seg;
#endif
#if LW_CFG_CPU_FPU_EN > 0
    PLW_CLASS_TCB  ptcbCur;
    ARCH_FPU_CTX  *pFpuCtx;
#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
	union fpureg *fpr;
	enum msa_2b_fmt df;
	unsigned int wd;
#endif                                                                  /*  MIPS_HAS_MSA_INSTR > 0      */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

	origpc = (unsigned long)pc;
	orig31 = regs->regs[31];

	perf_sw_event(PERF_COUNT_SW_EMULATION_FAULTS, 1, regs, 0);

	/*
	 * This load never faults.
	 */
	res = __get_user(insn.word, pc);

	switch (insn.i_format.opcode) {
		/*
		 * These are instructions that a compiler doesn't generate.  We
		 * can assume therefore that the code is MIPS-aware and
		 * really buggy.  Emulating these instructions would break the
		 * semantics anyway.
		 */
	case ll_op:
	case lld_op:
	case sc_op:
	case scd_op:

		/*
		 * For these instructions the only way to create an address
		 * error is an attempted access to kernel/supervisor address
		 * space.
		 */
	case ldl_op:
	case ldr_op:
	case lwl_op:
	case lwr_op:
	case sdl_op:
	case sdr_op:
	case swl_op:
	case swr_op:
	case lb_op:
	case lbu_op:
	case sb_op:
		goto sigbus;

		/*
		 * The remaining opcodes are the ones that are really of
		 * interest.
		 */
	case spec3_op:
		if (insn.dsp_format.func == lx_op) {
			switch (insn.dsp_format.op) {
			case lwx_op:
				if (!access_ok(VERIFY_READ, addr, 4))
					goto sigbus;
				LoadW(addr, value, res);
				if (res)
					goto fault;
				compute_return_epc(regs);
				regs->regs[insn.dsp_format.rd] = value;
				break;
			case lhx_op:
				if (!access_ok(VERIFY_READ, addr, 2))
					goto sigbus;
				LoadHW(addr, value, res);
				if (res)
					goto fault;
				compute_return_epc(regs);
				regs->regs[insn.dsp_format.rd] = value;
				break;
			default:
				goto sigill;
			}
		}
#ifdef CONFIG_EVA
		else {
			/*
			 * we can land here only from kernel accessing user
			 * memory, so we need to "switch" the address limit to
			 * user space, so that address check can work properly.
			 */
			seg = get_fs();
			set_fs(USER_DS);
			switch (insn.spec3_format.func) {
			case lhe_op:
				if (!access_ok(VERIFY_READ, addr, 2)) {
					set_fs(seg);
					goto sigbus;
				}
				LoadHWE(addr, value, res);
				if (res) {
					set_fs(seg);
					goto fault;
				}
				compute_return_epc(regs);
				regs->regs[insn.spec3_format.rt] = value;
				break;
			case lwe_op:
				if (!access_ok(VERIFY_READ, addr, 4)) {
					set_fs(seg);
					goto sigbus;
				}
				LoadWE(addr, value, res);
				if (res) {
					set_fs(seg);
					goto fault;
				}
				compute_return_epc(regs);
				regs->regs[insn.spec3_format.rt] = value;
				break;
			case lhue_op:
				if (!access_ok(VERIFY_READ, addr, 2)) {
					set_fs(seg);
					goto sigbus;
				}
				LoadHWUE(addr, value, res);
				if (res) {
					set_fs(seg);
					goto fault;
				}
				compute_return_epc(regs);
				regs->regs[insn.spec3_format.rt] = value;
				break;
			case she_op:
				if (!access_ok(VERIFY_WRITE, addr, 2)) {
					set_fs(seg);
					goto sigbus;
				}
				compute_return_epc(regs);
				value = regs->regs[insn.spec3_format.rt];
				StoreHWE(addr, value, res);
				if (res) {
					set_fs(seg);
					goto fault;
				}
				break;
			case swe_op:
				if (!access_ok(VERIFY_WRITE, addr, 4)) {
					set_fs(seg);
					goto sigbus;
				}
				compute_return_epc(regs);
				value = regs->regs[insn.spec3_format.rt];
				StoreWE(addr, value, res);
				if (res) {
					set_fs(seg);
					goto fault;
				}
				break;
			default:
				set_fs(seg);
				goto sigill;
			}
			set_fs(seg);
		}
#endif
		break;
	case lh_op:
		if (!access_ok(VERIFY_READ, addr, 2))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (uaccess_kernel())
				LoadHW(addr, value, res);
			else
				LoadHWE(addr, value, res);
		} else {
			LoadHW(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->regs[insn.i_format.rt] = value;
		break;

	case lw_op:
		if (!access_ok(VERIFY_READ, addr, 4))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (uaccess_kernel())
				LoadW(addr, value, res);
			else
				LoadWE(addr, value, res);
		} else {
			LoadW(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->regs[insn.i_format.rt] = value;
		break;

	case lhu_op:
		if (!access_ok(VERIFY_READ, addr, 2))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (uaccess_kernel())
				LoadHWU(addr, value, res);
			else
				LoadHWUE(addr, value, res);
		} else {
			LoadHWU(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->regs[insn.i_format.rt] = value;
		break;

	case lwu_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_READ, addr, 4))
			goto sigbus;

		LoadWU(addr, value, res);
		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->regs[insn.i_format.rt] = value;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case ld_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_READ, addr, 8))
			goto sigbus;

		LoadDW(addr, value, res);
		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->regs[insn.i_format.rt] = value;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case sh_op:
		if (!access_ok(VERIFY_WRITE, addr, 2))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->regs[insn.i_format.rt];

		if (IS_ENABLED(CONFIG_EVA)) {
			if (uaccess_kernel())
				StoreHW(addr, value, res);
			else
				StoreHWE(addr, value, res);
		} else {
			StoreHW(addr, value, res);
		}

		if (res)
			goto fault;
		break;

	case sw_op:
		if (!access_ok(VERIFY_WRITE, addr, 4))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->regs[insn.i_format.rt];

		if (IS_ENABLED(CONFIG_EVA)) {
			if (uaccess_kernel())
				StoreW(addr, value, res);
			else
				StoreWE(addr, value, res);
		} else {
			StoreW(addr, value, res);
		}

		if (res)
			goto fault;
		break;

	case sd_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_WRITE, addr, 8))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->regs[insn.i_format.rt];
		StoreDW(addr, value, res);
		if (res)
			goto fault;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case lwc1_op:
	case ldc1_op:
	case swc1_op:
	case sdc1_op:
	case cop1x_op:
#if LW_CFG_CPU_FPU_EN > 0
	    LW_NONSCHED_MODE_PROC(
	        LW_TCB_GET_CUR(ptcbCur);
	        pFpuCtx = ptcbCur->TCB_pvStackFP;
	        __ARCH_FPU_ENABLE();
	        __ARCH_FPU_SAVE(pFpuCtx);                                   /*  ���浱ǰ FPU CTX            */
	    );

		res = fpu_emulator_cop1Handler(regs, pFpuCtx, 1, &fault_addr);
		if (res) {
	        LW_NONSCHED_MODE_PROC(
	            __ARCH_FPU_DISABLE();
	        );
	        goto sigill;
		}

        LW_NONSCHED_MODE_PROC(
            __ARCH_FPU_RESTORE(pFpuCtx);                                /*  �ָ���ǰ FPU CTX            */
            __ARCH_FPU_DISABLE();
        );
#else
        goto sigill;
        break;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    case msa_op:
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_MIPS_HAS_MSA_INSTR > 0)
        if (!cpu_has_msa)
            goto sigill;

        /*
         * TODO ����Ŀǰ����֧�ֶ�Э������ͬʱʹ��, msa �ݲ�֧��
         */

        df = insn.msa_mi10_format.df;
        wd = insn.msa_mi10_format.wd;

        switch (insn.msa_mi10_format.func) {
        case msa_ld_op:
            if (!access_ok(VERIFY_READ, addr, sizeof(*fpr)))
                goto sigbus;

            LW_NONSCHED_MODE_PROC(
                LW_TCB_GET_CUR(ptcbCur);
                pFpuCtx = ptcbCur->TCB_pvStackFP;
                __ARCH_FPU_ENABLE();
                __ARCH_FPU_SAVE(pFpuCtx);                               /*  ���浱ǰ FPU CTX            */
            );

            fpr = &pFpuCtx->FPUCTX_reg[wd];

            lib_memcpy(fpr, addr, sizeof(*fpr));
            write_msa_wr(wd, fpr, df);

            LW_NONSCHED_MODE_PROC(
                __ARCH_FPU_RESTORE(pFpuCtx);                            /*  �ָ���ǰ FPU CTX            */
                __ARCH_FPU_DISABLE();
            );
            break;

        case msa_st_op:
            if (!access_ok(VERIFY_WRITE, addr, sizeof(*fpr)))
                goto sigbus;

            LW_NONSCHED_MODE_PROC(
                LW_TCB_GET_CUR(ptcbCur);
                pFpuCtx = ptcbCur->TCB_pvStackFP;
                __ARCH_FPU_ENABLE();
                __ARCH_FPU_SAVE(pFpuCtx);                               /*  ���浱ǰ FPU CTX            */
            );

            fpr = &pFpuCtx->FPUCTX_reg[wd];

            read_msa_wr(wd, fpr, df);
            lib_memcpy(addr, fpr, sizeof(*fpr));

            LW_NONSCHED_MODE_PROC(
                __ARCH_FPU_RESTORE(pFpuCtx);                            /*  �ָ���ǰ FPU CTX            */
                __ARCH_FPU_DISABLE();
            );
            break;

        default:
            goto sigbus;
        }

        compute_return_epc(regs);
        break;
#else
        goto sigill;
        break;
#endif                                                                  /*  FPU_EN && HAS_MSA_INSTR     */

#ifndef CONFIG_CPU_MIPSR6
	/*
	 * COP2 is available to implementor for application specific use.
	 * It's up to applications to register a notifier chain and do
	 * whatever they have to do, including possible sending of signals.
	 *
	 * This instruction has been reallocated in Release 6
	 */
	case lwc2_op:
	case ldc2_op:
	case swc2_op:
	case sdc2_op:
        /*
         * TODO COP2 �ݲ�֧��ģ��
         */
        goto sigill;
		break;
#endif
	default:
		/*
		 * Pheeee...  We encountered an yet unknown instruction or
		 * cache coherence problem.  Die sucker, die ...
		 */
		goto sigill;
	}

#ifdef CONFIG_DEBUG_FS
	unaligned_instructions++;
#endif

    pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;
    return;

fault:
	/* roll back jump/branch */
	regs->cp0_epc  = origpc;
	regs->regs[31] = orig31;
    pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
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
** ��������: mipsUnalignedHandle
** ��������: MIPS �Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
**           pabtInfo          ��ֹ��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsUnalignedHandle (ARCH_REG_CTX  *pregctx, PLW_VMM_ABORT  pabtInfo)
{
    MIPS_INSTRUCTION  *pc;

    /*
     * Did we catch a fault trying to load an instruction?
     * Or are we running in MIPS16 mode?
     */
    if ((pregctx->REG_ulCP0BadVAddr == pregctx->REG_ulCP0Epc) || (pregctx->REG_ulCP0Epc & 0x1)) {
        goto  sigbus;
    }

    pc = (MIPS_INSTRUCTION *)exception_epc(pregctx);

    /*
     * Do branch emulation only if we didn't forward the exception.
     * This is all so but ugly ...
     */
    emulate_load_store_insn(pregctx, (VOID *)pregctx->REG_ulCP0BadVAddr, pc, pabtInfo);
    return;

sigbus:
    pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    pabtInfo->VMABT_uiMethod = BUS_ADRALN;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
