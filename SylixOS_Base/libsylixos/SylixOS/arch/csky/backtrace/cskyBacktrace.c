/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: cskyBacktrace.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系架构堆栈回溯 (来源于 linux).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "stdlib.h"
#include "arch/csky/inc/backtrace.h"
#include "cskyBacktrace.h"
/*********************************************************************************************************
** 函数名称: getEndStack
** 功能描述: 获得堆栈结束地址
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}
/*********************************************************************************************************
** 函数名称: csky_get_insn
** 功能描述: 获取指令
** 输　入  : addr         地址
**           insn         指令
** 输　出  : 指令长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int csky_get_insn (unsigned long  addr, unsigned int  *insn)
{
    if ((*(unsigned short *)addr >> 14) == 0x3) {
        *insn = *(unsigned short *)(addr + 2) | (*(unsigned short *)addr) << 16;
        return  (4);

    } else {
        *insn = *(unsigned short *)addr;
        return  (2);
    }
}
/*********************************************************************************************************
** 函数名称: csky_analyze_prologue
** 功能描述: 分析堆栈
** 输　入  : start_pc        起始地址
**           limit_pc        结束地址
**           r15_offset      r15 偏移
**           r8_offset       r8 偏移
**           subi_len        sp 移动大小
**           subi2_len       sp 第二次移动大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  csky_analyze_prologue (unsigned long   start_pc,    unsigned long   limit_pc,
                                    unsigned long  *r15_offset,  unsigned long  *r8_offset,
                                    unsigned long  *subi_len,    unsigned long  *subi2_len)
{
    unsigned long   addr;
    unsigned int    insn, rn;
    int             framesize;
    int             stacksize;
    int             flag = 0;

#define CSKY_NUM_GREGS_v2               32
#define CSKY_NUM_GREGS_v2_SAVED_GREGS   (CSKY_NUM_GREGS_v2 + 4)
    int             register_offsets[CSKY_NUM_GREGS_v2_SAVED_GREGS];    /*  32 general regs + 4         */
    int             insn_len;

    int             mfcr_regnum;
    int             stw_regnum;

    /*
     * When build programe with -fno-omit-frame-pointer, there must be
     * mov r8, r0
     * in prologue, here, we check such insn, once hit, we set IS_FP_SAVED.
     */

    /*
     * REGISTER_OFFSETS will contain offsets, from the top of the frame
     * (NOT the frame pointer), for the various saved registers or -1
     * if the register is not saved.
     */
    for (rn = 0; rn < CSKY_NUM_GREGS_v2_SAVED_GREGS; rn++) {
        register_offsets[rn] = -1;
    }

    /*
     * Analyze the prologue. Things we determine from analyzing the
     * prologue include:
     * the size of the frame
     * where saved registers are located (and which are saved)
     * FP used?
     */

    stacksize  = 0;
    *subi2_len = 0;
    insn_len   = 2;                                                     /*  instruction is 16bit        */
    for (addr = start_pc; addr < limit_pc; addr += insn_len) {

        insn_len = csky_get_insn(addr, &insn);                          /*  Get next insn               */

        if(insn_len == 4) {                                             /*  if 32bit                    */
            if (V2_32_IS_SUBI0(insn)) {                                 /*  subi32 sp,sp oimm12         */
                int offset = V2_32_SUBI_IMM(insn);                      /*  got oimm12                  */

                stacksize += offset;
                if (flag) {
                    *subi2_len += offset;
                }
                continue;

            } else if (V2_32_IS_STMx0(insn)) {                          /*  stm32 ry-rz,(sp)            */
                /*
                 * Spill register(s)
                 */
                int start_register;
                int reg_count;
                int offset;

                /*
                 * BIG WARNING! The CKCore ABI does not restrict functions
                 * to taking only one stack allocation. Therefore, when
                 * we save a register, we record the offset of where it was
                 * saved relative to the current stacksize. This will
                 * then give an offset from the SP upon entry to our
                 * function. Remember, stacksize is NOT constant until
                 * we're done scanning the prologue.
                 */
                start_register = V2_32_STM_VAL_REGNUM(insn);            /*  ry                          */
                reg_count      = V2_32_STM_SIZE(insn);

                for (rn = start_register, offset = 0;
                     rn <= start_register + reg_count;
                     rn++, offset += 4) {
                    register_offsets[rn] = stacksize - offset;
                }

                continue;

            } else if (V2_32_IS_STWx0(insn)) {                          /*  stw ry,(sp,disp)            */
                /*
                 * Spill register: see note for IS_STM above.
                 */
                int disp;

                rn   = V2_32_ST_VAL_REGNUM(insn);
                disp = V2_32_ST_OFFSET(insn);
                register_offsets[rn] = stacksize - disp;
                continue;

            } else if (V2_32_IS_MOV_FP_SP(insn)) {
                /*
                 * Do not forget to skip this insn.
                 */
                continue;

            } else if (V2_32_IS_MFCR_EPSR(insn)) {
                unsigned int insn2;

                addr       += 4;
                mfcr_regnum = insn & 0x1f;
                insn_len    = csky_get_insn(addr, &insn2);
                if (insn_len == 2) {
                    stw_regnum = (insn2 >> 5) & 0x7;
                    if (V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2;                     /*  CSKY_EPSR_REGNUM            */
                        offset = V2_16_STWx0_OFFSET(insn2);
                        register_offsets[rn] = stacksize - offset;
                        continue;
                    }
                    break;

                } else {                                                /*  insn_len == 4               */
                    stw_regnum = (insn2 >> 21) & 0x1f;
                    if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2;                     /*  CSKY_EPSR_REGNUM            */
                        offset = V2_32_ST_OFFSET(insn2);
                        register_offsets[rn] = framesize - offset;
                        continue;
                    }
                    break;
                }

            } else if (V2_32_IS_MFCR_FPSR(insn)) {
                unsigned int insn2;

                addr       += 4;
                mfcr_regnum = insn & 0x1f;
                insn_len    = csky_get_insn(addr, &insn2);
                if (insn_len == 2) {
                    stw_regnum = (insn2 >> 5) & 0x7;
                    if (V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 1;                 /*  CSKY_FPSR_REGNUM            */
                        offset = V2_16_STWx0_OFFSET(insn2);
                        register_offsets[rn] = stacksize - offset;
                        continue;
                    }
                    break;

                } else {                                                /*  insn_len == 4               */
                    stw_regnum = (insn2 >> 21) & 0x1f;

                    if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 1;                 /*  CSKY_FPSR_REGNUM            */
                        offset = V2_32_ST_OFFSET(insn2);
                        register_offsets[rn] = framesize - offset;
                        continue;
                    }
                    break;
                }

            } else if (V2_32_IS_MFCR_EPC(insn)) {
                unsigned int insn2;

                addr       += 4;
                mfcr_regnum = insn & 0x1f;
                insn_len    = csky_get_insn(addr, &insn2);
                if (insn_len == 2) {
                    stw_regnum = (insn2 >> 5) & 0x7;
                    if (V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 2;                 /*  CSKY_EPC_REGNUM             */
                        offset = V2_16_STWx0_OFFSET(insn2);
                        register_offsets[rn] = stacksize - offset;
                        continue;
                    }
                    break;

                } else {                                                /*  insn_len == 4               */
                    stw_regnum = (insn2 >> 21) & 0x1f;
                    if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 2;                 /*  CSKY_EPC_REGNUM             */
                        offset = V2_32_ST_OFFSET(insn2);
                        register_offsets[rn] = framesize - offset;
                        continue;
                    }
                    break;
                }

            } else if (V2_32_IS_MFCR_FPC(insn)) {
                unsigned int insn2;

                addr       += 4;
                mfcr_regnum = insn & 0x1f;
                insn_len    = csky_get_insn(addr, &insn2);
                if (insn_len == 2) {
                    stw_regnum = (insn2 >> 5) & 0x7;
                    if (V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 3;                 /*  CSKY_FPC_REGNUM             */
                        offset = V2_16_STWx0_OFFSET(insn2);
                        register_offsets[rn] = stacksize - offset;
                        continue;
                    }
                    break;

                } else {                                                /*  insn_len == 4               */
                    stw_regnum = (insn2 >> 21) & 0x1f;
                    if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum)) {
                        int offset;

                        rn     = CSKY_NUM_GREGS_v2 + 3;                 /*  CSKY_FPC_REGNUM             */
                        offset = V2_32_ST_OFFSET(insn2);
                        register_offsets[rn] = framesize - offset;
                        continue;
                    }
                    break;
                }

            }  else if (V2_32_IS_PUSH(insn)) {                          /*  push for 32_bit             */
                int offset = 0;

                if (V2_32_IS_PUSH_R29(insn)) {
                    stacksize += 4;
                    if (flag) {
                        *subi2_len += 4;
                    }
                    register_offsets[29] = stacksize;
                    offset += 4;
                }

                if (V2_32_PUSH_LIST2(insn)) {
                    int num = V2_32_PUSH_LIST2(insn);
                    int tmp = 0;

                    stacksize += num * 4;
                    if (flag) {
                        *subi2_len += num * 4;;
                    }
                    offset += num * 4;

                    for (rn = 16; rn <= 16 + num - 1; rn++) {
                        register_offsets[rn] = stacksize - tmp;
                        tmp += 4;
                    }
                }

                if (V2_32_IS_PUSH_R15(insn)) {
                    stacksize += 4;
                    if (flag) {
                        *subi2_len += 4;;
                    }
                    register_offsets[15] = stacksize;
                    offset += 4;
                }

                if (V2_32_PUSH_LIST1(insn)) {
                    int num = V2_32_PUSH_LIST1(insn);
                    int tmp = 0;

                    stacksize += num * 4;
                    if (flag) {
                        *subi2_len += num * 4;;
                    }
                    offset += num * 4;

                    for (rn = 4; rn <= 4 + num - 1; rn++) {
                        register_offsets[rn] = stacksize - tmp;
                        tmp += 4;
                    }
                }

                framesize = stacksize;
                continue;

            }  else if (V2_32_IS_LRW4(insn)   || V2_32_IS_MOVI4(insn) ||
                        V2_32_IS_MOVIH4(insn) || V2_32_IS_BMASKI4(insn)) {
                int          adjust = 0;
                int          offset = 0;
                unsigned int insn2;

                if (V2_32_IS_LRW4(insn)) {
                    int literal_addr = (addr + ((insn & 0xffff) << 2)) & 0xfffffffc;
                    csky_get_insn(literal_addr, (unsigned int *)&adjust);

                } else if (V2_32_IS_MOVI4(insn)) {
                    adjust = (insn  & 0xffff);

                } else if (V2_32_IS_MOVIH4(insn)) {
                    adjust = (insn & 0xffff) << 16;

                } else {                                                /*  V2_32_IS_BMASKI4(insn)      */
                    adjust = (1 << (((insn & 0x3e00000) >> 21) + 1)) - 1;
                }

                /*
                 * May have zero or more insns which modify r4
                 */
                offset = 4;
                insn_len = csky_get_insn(addr + offset, &insn2);
                while (V2_IS_R4_ADJUSTER(insn2)) {
                    if (V2_32_IS_ADDI4(insn2)) {
                        int imm = (insn2 & 0xfff) + 1;
                        adjust += imm;

                    } else if (V2_32_IS_SUBI4(insn2)) {
                        int imm = (insn2 & 0xfff) + 1;
                        adjust -= imm;

                    } else if (V2_32_IS_NOR4(insn2)) {
                        adjust = ~adjust;

                    } else if (V2_32_IS_ROTLI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);
                        int temp = adjust >> (32 - imm);

                        adjust <<= imm;
                        adjust |= temp;

                    } else if (V2_32_IS_LISI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust <<= imm;

                    } else if (V2_32_IS_BSETI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust |= (1 << imm);

                    } else if (V2_32_IS_BCLRI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust &= ~(1 << imm);

                    } else if (V2_32_IS_IXH4(insn2)) {
                        adjust *= 3;

                    } else if (V2_32_IS_IXW4(insn2)) {
                        adjust *= 5;

                    } else if (V2_16_IS_ADDI4(insn2)) {
                        int imm = (insn2 & 0xff) + 1;

                        adjust += imm;

                    } else if (V2_16_IS_SUBI4(insn2)) {
                        int imm = (insn2 & 0xff) + 1;

                        adjust -= imm;

                    } else if (V2_16_IS_NOR4(insn2)) {
                        adjust = ~adjust;

                    } else if (V2_16_IS_BSETI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust |= (1 << imm);

                    } else if (V2_16_IS_BCLRI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust &= ~(1 << imm);

                    } else if (V2_16_IS_LSLI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust <<= imm;
                    }

                    offset  += insn_len;
                    insn_len = csky_get_insn(addr + offset, &insn2);
                }

                /*
                 * If the next insn adjusts the stack pointer, we keep everything;
                 * if not, we scrap it and we've found the end of the prologue.
                 */
                if (V2_IS_SUBU4(insn2)) {
                    addr      += offset;
                    stacksize += adjust;
                    if (flag) {
                        *subi2_len += adjust;
                    }
                    continue;
                }

                /*
                 * None of these instructions are prologue, so don't touch anything.
                 */
                break;

            } else {
                if (!flag) {
                    continue;
                }
            }

        } else {
            if (V2_16_IS_SUBI0(insn)) {                                 /*  subi.sp sp,disp             */
                int offset = V2_16_SUBI_IMM(insn);

                stacksize += offset;                                    /*  capacity of creating space  */
                if (flag) {                                             /*  in stack                    */
                    *subi2_len += offset;
                }
                continue;

            } else if (V2_16_IS_STWx0(insn)) {                          /*  stw.16 rz,(sp,disp)         */
                /*
                 * Spill register: see note for IS_STM above.
                 */
                int disp;

                rn   = V2_16_ST_VAL_REGNUM(insn);
                disp = V2_16_ST_OFFSET(insn);
                register_offsets[rn] = stacksize - disp;
                continue;

            } else if (V2_16_IS_MOV_FP_SP(insn)) {
                flag = 1;
                continue;

            } else if (V2_16_IS_PUSH(insn)) {                           /*  push for 16_bit             */
                int offset = 0;

                if (V2_16_IS_PUSH_R15(insn)) {
                    stacksize += 4;
                    if (flag) {
                        *subi2_len += 4;
                    }
                    register_offsets[15] = stacksize;
                    offset += 4;
                }

                if (V2_16_PUSH_LIST1(insn)) {
                    int num = V2_16_PUSH_LIST1(insn);
                    int tmp = 0;

                    stacksize += num * 4;
                    if (flag) {
                        *subi2_len += num * 4;
                    }
                    offset += num * 4;
                    for (rn = 4; rn <= 4 + num - 1; rn++) {
                        register_offsets[rn] = stacksize - tmp;
                        tmp += 4;
                    }
                }

                framesize = stacksize;
                continue;

            } else if (V2_16_IS_LRW4(insn) || V2_16_IS_MOVI4(insn)) {
                int          adjust = 0;
                int          offset = 0;
                unsigned int insn2;

                if (V2_16_IS_LRW4(insn)) {
                    int offset = ((insn & 0x300) >> 3) | (insn & 0x1f);
                    int literal_addr = (addr + ( offset << 2)) & 0xfffffffc;

                    adjust = *(unsigned long*)literal_addr;

                } else {                                                /*  V2_16_IS_MOVI4(insn)        */
                    adjust = (insn  & 0xff);
                }

                /*
                 * May have zero or more insns which modify r4
                 */
                offset   = 2;
                insn_len = csky_get_insn(addr + offset, &insn2);

                while (V2_IS_R4_ADJUSTER(insn2)) {
                    if (V2_32_IS_ADDI4(insn2)) {
                        int imm = (insn2 & 0xfff) + 1;

                        adjust += imm;

                    } else if (V2_32_IS_SUBI4(insn2)) {
                        int imm = (insn2 & 0xfff) + 1;

                        adjust -= imm;

                    }  else if (V2_32_IS_NOR4(insn2)) {
                        adjust = ~adjust;

                    } else if (V2_32_IS_ROTLI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);
                        int temp = adjust >> (32 - imm);

                        adjust <<= imm;
                        adjust |= temp;

                    } else if (V2_32_IS_LISI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust <<= imm;

                    } else if (V2_32_IS_BSETI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust |= (1 << imm);

                    } else if (V2_32_IS_BCLRI4(insn2)) {
                        int imm = ((insn2 >> 21) & 0x1f);

                        adjust &= ~(1 << imm);

                    } else if (V2_32_IS_IXH4(insn2)) {
                        adjust *= 3;

                    } else if (V2_32_IS_IXW4(insn2)) {
                        adjust *= 5;

                    } else if (V2_16_IS_ADDI4(insn2)) {
                        int imm = (insn2 & 0xff) + 1;

                        adjust += imm;

                    } else if (V2_16_IS_SUBI4(insn2)) {
                        int imm = (insn2 & 0xff) + 1;

                        adjust -= imm;

                    } else if (V2_16_IS_NOR4(insn2)) {
                        adjust = ~adjust;

                    } else if (V2_16_IS_BSETI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust |= (1 << imm);

                    } else if (V2_16_IS_BCLRI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust &= ~(1 << imm);

                    } else if (V2_16_IS_LSLI4(insn2)) {
                        int imm = (insn2 & 0x1f);

                        adjust <<= imm;
                    }

                    offset  += insn_len;
                    insn_len = csky_get_insn(addr + offset, &insn2);
                }

                /*
                 * If the next insn adjusts the stack pointer, we keep everything;
                 * if not, we scrap it and we've found the end of the prologue.
                 */
                if (V2_IS_SUBU4(insn2)) {
                    addr      += offset;
                    stacksize += adjust;
                    if (flag) {
                        *subi2_len += adjust;
                    }
                    continue;
                }

                /*
                 * None of these instructions are prologue, so don't touch anything.
                 */
                break;

            } else {
                if (!flag) {
                    continue;
                }
            }
        }

        /*
         * This is not a prologue insn, so stop here.
         */
        break;
    }

    *subi_len   = stacksize;
    *r15_offset = register_offsets[15];
    *r8_offset  = register_offsets[8];
}
/*********************************************************************************************************
** 函数名称: backtrace
** 功能描述: 获得当前任务调用栈
** 输　入  : array     获取数组
**           size      数组大小
** 输　出  : 获取的数目
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int  backtrace (void **array, int size)
{
    CSKY_INSTRUCTION  *addr;
    CSKY_INSTRUCTION   inst;
    unsigned long     *ra;
    unsigned long     *sp;
    unsigned long     *fp = 0;
    unsigned long     *end_stack;
    unsigned long     *low_stack;
    unsigned int       cnt;
    unsigned long      start_pc, limit_pc, temp_pc;
    unsigned long      r15_offset, r8_offset, subi_len, subi2_len = 0;

    if (!array || (size < 0)) {
        return  (-1);
    }

    __asm__ __volatile__("MOV  %0, R15\n"                               /*  获得 RA                     */
                         "MOV  %1, SP\n"                                /*  获得 SP                     */
                         :"=r"(ra), "=r"(low_stack));

    cnt = 0;
    ra  = (unsigned long *)backtrace;
    sp  = low_stack;

    end_stack = getEndStack();                                          /*  获得堆栈结束地址            */
    start_pc  = (unsigned long)ra;

    for (; cnt < size; ) {                                              /*  backtrace                   */
__find_start:
        start_pc = 0;
        limit_pc = 0;

        for (addr = (CSKY_INSTRUCTION *)ra; ; addr--) {                 /*  查找函数开始地址            */
            inst = *addr;
            if ((V2_16_IS_RTS(inst) && V2_16_IS_ADDI14(*(addr - 1)))    ||
                (V2_16_IS_POP(inst) && V2_16_IS_MOV_SP_FP(*(addr - 1))) ||
                (V2_32_IS_POP((*(addr - 1) << 16 | inst)) && V2_16_IS_MOV_SP_FP(*(addr - 2)))) {
                temp_pc = (unsigned long)(addr - 2);
                addr++;
                addr = (CSKY_INSTRUCTION *)ROUND_UP(addr, 4);

                for ( ; ; addr++) {
                    inst = *addr;
                    if ((V2_16_IS_SUBI0(inst) && V2_16_IS_STWx0(*(addr + 1)))                  ||
                        (V2_16_IS_PUSH(inst)  && V2_16_IS_MOV_FP_SP(*(addr + 1)))              ||
                        (V2_32_IS_PUSH((inst << 16 | *(addr + 1)))                             &&
                        (V2_16_IS_MOV_FP_SP(*(addr + 2)) || V2_16_IS_MOV_FP_SP(*(addr + 6))))  ||
                        (V2_16_IS_SUBI0(inst) && V2_32_IS_STWx0((*(addr + 1) << 16 | *(addr + 2))))) {
                        if ((unsigned long)addr > (unsigned long)ra) {
                            if (!start_pc) {
                                ra = (unsigned long *)temp_pc;
                                goto  __find_start;
                            }
                            break;
                        }
                        start_pc = (unsigned long)addr;
                    }
                }
                break;
            }
        }

        if ((unsigned long)ra < start_pc) {
            ra = (unsigned long *)temp_pc;
            goto  __find_start;
        }

        for (addr = (CSKY_INSTRUCTION *)start_pc; ; addr++) {           /*  查找函数结束结束地址        */
            inst = *addr;

            if (IS_T32(inst)) {
                if (V2_32_IS_POP((inst << 16 | *(addr + 1)))) {
                    break;
                }
                addr++;

            } else if (V2_16_IS_RTS(inst) || V2_16_IS_POP(inst))  {
                break;
            }

            if ((V2_16_IS_SUBI0(inst) && V2_16_IS_STWx0(*(addr + 1)))                          ||
                (V2_16_IS_PUSH(inst)  && V2_16_IS_MOV_FP_SP(*(addr + 1)))                      ||
                (V2_32_IS_PUSH((inst << 16 | *(addr + 1))) && V2_16_IS_MOV_FP_SP(*(addr + 2))) ||
                (V2_16_IS_SUBI0(inst) && V2_32_IS_STWx0((*(addr + 1) << 16 | *(addr + 2))))) {
                if ((unsigned long)addr > start_pc) {
                    addr--;
                    break;
                }
            }
        }
        limit_pc = (unsigned long)addr;

        csky_analyze_prologue(start_pc, limit_pc, &r15_offset, &r8_offset, &subi_len, &subi2_len);
        if ((r15_offset != -1) && (subi_len != 0)) {
            if (fp) {
               fp = (unsigned long *)((unsigned long)fp - subi2_len);
               if (fp != sp) {                                          /*  计算的 SP 与保存的 FP 不相同*/
                   sp = (unsigned long *)((unsigned long)fp + subi2_len);
               }
            }

            if ((sp >= end_stack) || (sp < low_stack)) {                /*  SP 不合法                   */
                break;
            }

            ra = (unsigned long *)(*(unsigned long *)((unsigned long)sp + subi_len - r15_offset));
            fp = (unsigned long *)(*(unsigned long *)((unsigned long)sp + subi_len - r8_offset));
            sp = (unsigned long *)((unsigned long)sp + subi_len);

            if (ra == 0) {                                              /*  最后一层无返回函数          */
                break;
            }

            array[cnt++] = ra;

        } else {
            break;
        }
    }

    return  (cnt);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
