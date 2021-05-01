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
** 文   件   名: arm64Elf.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: 实现 ARM64 体系结构的 ELF 文件重定位.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef LW_CFG_CPU_ARCH_ARM64                                            /*  ARM64 体系结构              */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  ARM64 relocs
*********************************************************************************************************/
#define R_AARCH64_NONE                         0                        /*  No relocation.              */
/*********************************************************************************************************
  ILP32 AArch64 relocs
*********************************************************************************************************/
#define R_AARCH64_P32_ABS32                    1                        /*  Direct 32 bit.              */
#define R_AARCH64_P32_COPY                     180                      /*  Copy symbol at runtime.     */
#define R_AARCH64_P32_GLOB_DAT                 181                      /*  Create GOT entry.           */
#define R_AARCH64_P32_JUMP_SLOT                182                      /*  Create PLT entry.           */
#define R_AARCH64_P32_RELATIVE                 183                      /*  Adjust by program base.     */
#define R_AARCH64_P32_TLS_DTPMOD               184                      /*  Module number, 32 bit.      */
#define R_AARCH64_P32_TLS_DTPREL               185                      /*  Module-rela offset, 32 bit. */
#define R_AARCH64_P32_TLS_TPREL                186                      /*  TP-relative offset, 32 bit. */
#define R_AARCH64_P32_TLSDESC                  187                      /*  TLS Descriptor.             */
#define R_AARCH64_P32_IRELATIVE                188                      /*  STT_GNU_IFUNC relocation.   */
/*********************************************************************************************************
  LP64 AArch64 relocs
*********************************************************************************************************/
#define R_AARCH64_ABS64                        257                      /*  Direct 64 bit.              */
#define R_AARCH64_ABS32                        258                      /*  Direct 32 bit.              */
#define R_AARCH64_ABS16                        259                      /*  Direct 16-bit.              */
#define R_AARCH64_PREL64                       260                      /*  PC-relative 64-bit.         */
#define R_AARCH64_PREL32                       261                      /*  PC-relative 32-bit.         */
#define R_AARCH64_PREL16                       262                      /*  PC-relative 16-bit.         */
#define R_AARCH64_MOVW_UABS_G0                 263                      /*  Dir MOVZ imm from bits 15:0 */
#define R_AARCH64_MOVW_UABS_G0_NC              264                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_UABS_G1                 265                      /*  Dir MOVZ imm from bits 31:16*/
#define R_AARCH64_MOVW_UABS_G1_NC              266                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_UABS_G2                 267                      /*  Dir MOVZ imm from bits 47:32*/
#define R_AARCH64_MOVW_UABS_G2_NC              268                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_UABS_G3                 269                      /*  Dir MOV{K,Z} imm from 63:48.*/
#define R_AARCH64_MOVW_SABS_G0                 270                      /*  Dir MOV{N,Z} imm from 15:0. */
#define R_AARCH64_MOVW_SABS_G1                 271                      /*  Dir MOV{N,Z} imm from 31:16 */
#define R_AARCH64_MOVW_SABS_G2                 272                      /*  Dir MOV{N,Z} imm from 47:32.*/
#define R_AARCH64_LD_PREL_LO19                 273                      /*  PC-rel LD imm from b  20:2  */
#define R_AARCH64_ADR_PREL_LO21                274                      /*  PC-rel ADR imm from b 20:0. */
#define R_AARCH64_ADR_PREL_PG_HI21             275                      /*  Page-rel ADRP imm from 32:12*/
#define R_AARCH64_ADR_PREL_PG_HI21_NC          276                      /*  Likewise; no overflow check.*/
#define R_AARCH64_ADD_ABS_LO12_NC              277                      /*  Dir ADD imm from bits 11:0. */
#define R_AARCH64_LDST8_ABS_LO12_NC            278                      /*  Likewise for LD/ST;no check.*/
#define R_AARCH64_TSTBR14                      279                      /*  PC-rel TBZ/TBNZ im from 15:2*/
#define R_AARCH64_CONDBR19                     280                      /*  PC-rel cond br imm from 20:2*/
#define R_AARCH64_JUMP26                       282                      /*  PC-rel B imm from bits 27:2.*/
#define R_AARCH64_CALL26                       283                      /*  Likewise for CALL.          */
#define R_AARCH64_LDST16_ABS_LO12_NC           284                      /*  Dir ADD imm from bits 11:1. */
#define R_AARCH64_LDST32_ABS_LO12_NC           285                      /*  Likewise for bits 11:2.     */
#define R_AARCH64_LDST64_ABS_LO12_NC           286                      /*  Likewise for bits 11:3.     */
#define R_AARCH64_MOVW_PREL_G0                 287                      /*  PC-rel MOV{N,Z} im from 15:0*/
#define R_AARCH64_MOVW_PREL_G0_NC              288                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_PREL_G1                 289                      /*  PC-rel MOV{N,Z} im fro 31:16*/
#define R_AARCH64_MOVW_PREL_G1_NC              290                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_PREL_G2                 291                      /*  PC-rel MOV{N,Z} im fro 47:32*/
#define R_AARCH64_MOVW_PREL_G2_NC              292                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_PREL_G3                 293                      /*  PC-rel MOV{N,Z} im fro 63:48*/
#define R_AARCH64_LDST128_ABS_LO12_NC          299                      /*  Dir. ADD imm. from bits 11:4*/
#define R_AARCH64_MOVW_GOTOFF_G0               300                      /*  GOT-rel off MOV{N,Z} im 15:0*/
#define R_AARCH64_MOVW_GOTOFF_G0_NC            301                      /*  Likewise for MOVK; no check */
#define R_AARCH64_MOVW_GOTOFF_G1               302                      /*  GOT-rel o. MOV{N,Z} im 31:16*/
#define R_AARCH64_MOVW_GOTOFF_G1_NC            303                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_GOTOFF_G2               304                      /*  GOT-rel o. MOV{N,Z} im 47:32*/
#define R_AARCH64_MOVW_GOTOFF_G2_NC            305                      /*  Likewise for MOVK; no check.*/
#define R_AARCH64_MOVW_GOTOFF_G3               306                      /*  GOT-rel o MOV{N,Z} im 63:48.*/
#define R_AARCH64_GOTREL64                     307                      /*  GOT-relative 64-bit.        */
#define R_AARCH64_GOTREL32                     308                      /*  GOT-relative 32-bit.        */
#define R_AARCH64_GOT_LD_PREL19                309                      /*  PC-rel GOT off load im 20:2.*/
#define R_AARCH64_LD64_GOTOFF_LO15             310                      /*  GOT-rel off LD/ST imm. 14:3.*/
#define R_AARCH64_ADR_GOT_PAGE                 311                      /*  Ppage-rel GOT off ADRP 32:12*/
#define R_AARCH64_LD64_GOT_LO12_NC             312                      /*  Dir GOT off LD/ST imm. 11:3.*/
#define R_AARCH64_LD64_GOTPAGE_LO15            313                      /*  GOT-p-rel GOT off LD/ST 14:3*/
#define R_AARCH64_TLSGD_ADR_PREL21             512                      /*  PC-relative ADR imm. 20:0.  */
#define R_AARCH64_TLSGD_ADR_PAGE21             513                      /*  page-rel. ADRP imm. 32:12.  */
#define R_AARCH64_TLSGD_ADD_LO12_NC            514                      /*  direct ADD imm. from 11:0.  */
#define R_AARCH64_TLSGD_MOVW_G1                515                      /*  GOT-rel. MOV{N,Z} 31:16.    */
#define R_AARCH64_TLSGD_MOVW_G0_NC             516                      /*  GOT-rel. MOVK imm. 15:0.    */
#define R_AARCH64_TLSLD_ADR_PREL21             517                      /*  Like 512;local dynamic model*/
#define R_AARCH64_TLSLD_ADR_PAGE21             518                      /*  Like 513;local dynamic model*/
#define R_AARCH64_TLSLD_ADD_LO12_NC            519                      /*  Like 514;local dynamic model*/
#define R_AARCH64_TLSLD_MOVW_G1                520                      /*  Like 515;local dynamic model*/
#define R_AARCH64_TLSLD_MOVW_G0_NC             521                      /*  Like 516;local dynamic model*/
#define R_AARCH64_TLSLD_LD_PREL19              522                      /*  TLS PC-rel. load imm. 20:2. */
#define R_AARCH64_TLSLD_MOVW_DTPREL_G2         523                      /*  TLS DTP-rel. MOV{N,Z} 47:32.*/
#define R_AARCH64_TLSLD_MOVW_DTPREL_G1         524                      /*  TLS DTP-rel. MOV{N,Z} 31:16.*/
#define R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC      525                      /*  Likewise; MOVK; no check.   */
#define R_AARCH64_TLSLD_MOVW_DTPREL_G0         526                      /*  TLS DTP-rel. MOV{N,Z} 15:0. */
#define R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC      527                      /*  Likewise; MOVK; no check.   */
#define R_AARCH64_TLSLD_ADD_DTPREL_HI12        528                      /*  DTP-rel. ADD imm. from 23:12*/
#define R_AARCH64_TLSLD_ADD_DTPREL_LO12        529                      /*  DTP-rel. ADD imm. from 11:0.*/
#define R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC     530                      /*  Likewise; no ovfl. check.   */
#define R_AARCH64_TLSLD_LDST8_DTPREL_LO12      531                      /*  DTP-rel. LD/ST imm. 11:0.   */
#define R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC   532                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLD_LDST16_DTPREL_LO12     533                      /*  DTP-rel. LD/ST imm. 11:1.   */
#define R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC  534                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLD_LDST32_DTPREL_LO12     535                      /*  DTP-rel. LD/ST imm. 11:2.   */
#define R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC  536                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLD_LDST64_DTPREL_LO12     537                      /*  DTP-rel. LD/ST imm. 11:3.   */
#define R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC  538                      /*  Likewise; no check.         */
#define R_AARCH64_TLSIE_MOVW_GOTTPREL_G1       539                      /*  GOT-rel. MOV{N,Z} 31:16.    */
#define R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC    540                      /*  GOT-rel. MOVK 15:0.         */
#define R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21    541                      /*  Page-rel. ADRP 32:12.       */
#define R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC  542                      /*  Direct LD off. 11:3.        */
#define R_AARCH64_TLSIE_LD_GOTTPREL_PREL19     543                      /*  PC-rel. load imm. 20:2.     */
#define R_AARCH64_TLSLE_MOVW_TPREL_G2          544                      /*  TLS TP-rel. MOV{N,Z} 47:32. */
#define R_AARCH64_TLSLE_MOVW_TPREL_G1          545                      /*  TLS TP-rel. MOV{N,Z} 31:16. */
#define R_AARCH64_TLSLE_MOVW_TPREL_G1_NC       546                      /*  Likewise; MOVK; no check.   */
#define R_AARCH64_TLSLE_MOVW_TPREL_G0          547                      /*  TLS TP-rel. MOV{N,Z} 15:0.  */
#define R_AARCH64_TLSLE_MOVW_TPREL_G0_NC       548                      /*  Likewise; MOVK; no check.   */
#define R_AARCH64_TLSLE_ADD_TPREL_HI12         549                      /*  TP-rel. ADD imm. 23:12.     */
#define R_AARCH64_TLSLE_ADD_TPREL_LO12         550                      /*  TP-rel. ADD imm. 11:0.      */
#define R_AARCH64_TLSLE_ADD_TPREL_LO12_NC      551                      /*  Likewise; no ovfl. check.   */
#define R_AARCH64_TLSLE_LDST8_TPREL_LO12       552                      /*  TP-rel. LD/ST off. 11:0.    */
#define R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC    553                      /*  Likewise; no ovfl. check.   */
#define R_AARCH64_TLSLE_LDST16_TPREL_LO12      554                      /*  TP-rel. LD/ST off. 11:1.    */
#define R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC   555                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLE_LDST32_TPREL_LO12      556                      /*  TP-rel. LD/ST off. 11:2.    */
#define R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC   557                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLE_LDST64_TPREL_LO12      558                      /*  TP-rel. LD/ST off. 11:3.    */
#define R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC   559                      /*  Likewise; no check.         */
#define R_AARCH64_TLSDESC_LD_PREL19            560                      /*  PC-rel. load immediate 20:2.*/
#define R_AARCH64_TLSDESC_ADR_PREL21           561                      /*  PC-rel. ADR immediate 20:0. */
#define R_AARCH64_TLSDESC_ADR_PAGE21           562                      /*  Page-rel. ADRP imm. 32:12.  */
#define R_AARCH64_TLSDESC_LD64_LO12            563                      /*  Direct LD off. from 11:3.   */
#define R_AARCH64_TLSDESC_ADD_LO12             564                      /*  Direct ADD imm. from 11:0.  */
#define R_AARCH64_TLSDESC_OFF_G1               565                      /*  GOT-rel. MOV{N,Z} imm. 31:16*/
#define R_AARCH64_TLSDESC_OFF_G0_NC            566                      /*  GOT-rel MOVK imm 15:0; no ck*/
#define R_AARCH64_TLSDESC_LDR                  567                      /*  Relax LDR.                  */
#define R_AARCH64_TLSDESC_ADD                  568                      /*  Relax ADD.                  */
#define R_AARCH64_TLSDESC_CALL                 569                      /*  Relax BLR.                  */
#define R_AARCH64_TLSLE_LDST128_TPREL_LO12     570                      /*  TP-rel. LD/ST off. 11:4.    */
#define R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC  571                      /*  Likewise; no check.         */
#define R_AARCH64_TLSLD_LDST128_DTPREL_LO12    572                      /*  DTP-rel. LD/ST imm. 11:4.   */
#define R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC 573                      /*  Likewise; no check.         */
#define R_AARCH64_COPY                         1024                     /*  Copy symbol at runtime.     */
#define R_AARCH64_GLOB_DAT                     1025                     /*  Create GOT entry.           */
#define R_AARCH64_JUMP_SLOT                    1026                     /*  Create PLT entry.           */
#define R_AARCH64_RELATIVE                     1027                     /*  Adjust by program base.     */
#define R_AARCH64_TLS_DTPMOD                   1028                     /*  Module number, 64 bit.      */
#define R_AARCH64_TLS_DTPREL                   1029                     /*  Module-rela offset, 64 bit. */
#define R_AARCH64_TLS_TPREL                    1030                     /*  TP-relative offset, 64 bit. */
#define R_AARCH64_TLSDESC                      1031                     /*  TLS Descriptor.             */
#define R_AARCH64_IRELATIVE                    1032                     /*  STT_GNU_IFUNC relocation.   */
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define JMP_TABLE_ITEMLEN       20                                      /*  跳转表条目长度              */
#define ADR_IMM_HILOSPLIT       2
#define ADR_IMM_SIZE            (2 * 1024 * 1024)
#define ADR_IMM_LOMASK          ((1 << ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_HIMASK          ((ADR_IMM_SIZE >> ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_LOSHIFT         29
#define ADR_IMM_HISHIFT         5
/*********************************************************************************************************
  极值定义
*********************************************************************************************************/
#define BIT(n)                  ((UINT64)1U << (n))
#define U16_MAX                 __ARCH_USHRT_MAX
#define S16_MAX                 __ARCH_SHRT_MAX
#define S16_MIN                 __ARCH_SHRT_MIN
#define S32_MAX                 __ARCH_INT_MAX
#define S32_MIN                 __ARCH_INT_MIN
#define U32_MAX                 __ARCH_UINT_MAX
/*********************************************************************************************************
  这里指令码以机器字长度的整数表示，不需要处理大小端问题，因为整数的高字节也是指令的高字节
*********************************************************************************************************/
#define BR_X16_INSTRUCTION      0xd61f0200                              /*  跳转指令                    */
/*********************************************************************************************************
  跳转表项类型定义
*********************************************************************************************************/
typedef struct {
    UINT32   uiMov0;                                                    /*  movn x16, #0x....           */
    UINT32   uiMov1;                                                    /*  movk x16, #0x...., lsl #16  */
    UINT32   uiMov2;                                                    /*  movk x16, #0x...., lsl #32  */
    UINT32   uiMov3;                                                    /*  movk x16, #0x...., lsl #48  */
    UINT32   uiBr;                                                      /*  br   x16                    */
} LONG_JMP_ITEM;
/*********************************************************************************************************
  类型定义
*********************************************************************************************************/
typedef enum {
    AARCH64_INSN_IMM_ADR,
    AARCH64_INSN_IMM_26,
    AARCH64_INSN_IMM_19,
    AARCH64_INSN_IMM_16,
    AARCH64_INSN_IMM_14,
    AARCH64_INSN_IMM_12,
    AARCH64_INSN_IMM_9,
    AARCH64_INSN_IMM_7,
    AARCH64_INSN_IMM_6,
    AARCH64_INSN_IMM_S,
    AARCH64_INSN_IMM_R,
    AARCH64_INSN_IMM_MAX
} __ARM64_INSN_IMM;

typedef enum {
    RELOC_OP_NONE,
    RELOC_OP_ABS,
    RELOC_OP_PREL,
    RELOC_OP_PAGE,
} __ARM64_RELOC_OP;
/*********************************************************************************************************
** 函数名称: __arm64RelocOp
** 功能描述: 重定位操作
** 输　入  : relocOp   操作符类型
**           pvPlace   重定位目标地址
**           ui64Val   (S + A)
** 输　出  : 重定位后值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  __arm64RelocOp (__ARM64_RELOC_OP  relocOp, PVOID  pvPlace, UINT64  ui64Val)
{
    switch (relocOp) {

    case RELOC_OP_ABS:
        return  (ui64Val);

    case RELOC_OP_PREL:
        return  (ui64Val - (UINT64)pvPlace);

    case RELOC_OP_PAGE:
        return  ((ui64Val & ~0xfff) - ((UINT64)pvPlace & ~0xfff));

    case RELOC_OP_NONE:
        return  (0);

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "do_reloc: unknown relocation operation %d\n", relocOp);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __arm64ImmShiftMaskGet
** 功能描述: 立即数掩码获取
** 输　入  : immtype       立即数类型
**           puiMask       掩码
**           piShift       位移
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __arm64ImmShiftMaskGet (__ARM64_INSN_IMM  immtype, UINT32  *puiMask, INT  *piShift)
{
    UINT32  uiMask;
    INT     iShift;

    switch (immtype) {

    case AARCH64_INSN_IMM_26:
        uiMask = BIT(26) - 1;
        iShift = 0;
        break;

    case AARCH64_INSN_IMM_19:
        uiMask = BIT(19) - 1;
        iShift = 5;
        break;

    case AARCH64_INSN_IMM_16:
        uiMask = BIT(16) - 1;
        iShift = 5;
        break;

    case AARCH64_INSN_IMM_14:
        uiMask = BIT(14) - 1;
        iShift = 5;
        break;

    case AARCH64_INSN_IMM_12:
        uiMask = BIT(12) - 1;
        iShift = 10;
        break;

    case AARCH64_INSN_IMM_9:
        uiMask = BIT(9) - 1;
        iShift = 12;
        break;

    case AARCH64_INSN_IMM_7:
        uiMask = BIT(7) - 1;
        iShift = 15;
        break;

    case AARCH64_INSN_IMM_6:
    case AARCH64_INSN_IMM_S:
        uiMask = BIT(6) - 1;
        iShift = 10;
        break;

    case AARCH64_INSN_IMM_R:
        uiMask = BIT(6) - 1;
        iShift = 16;
        break;

    default:
        return  (-EINVAL);
    }

    *puiMask = uiMask;
    *piShift = iShift;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __arm64InsnEncodeImmediate
** 功能描述: 立即数寻址指令拼装
** 输　入  : immType       立即数类型
**           uiInsn        指令值
**           ui64Imm       立即数
** 输　出  : 拼装后的指令
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __arm64InsnEncodeImmediate (__ARM64_INSN_IMM  immType, UINT32  uiInsn, UINT64  ui64Imm)
{
    UINT32  uiImmlo;
    UINT32  uiImmhi;
    UINT32  uiMask;
    INT     iShift;

    switch (immType) {

    case  AARCH64_INSN_IMM_ADR:
        iShift    = 0;
        uiImmlo   = (ui64Imm & ADR_IMM_LOMASK) << ADR_IMM_LOSHIFT;
        ui64Imm >>= ADR_IMM_HILOSPLIT;
        uiImmhi   = (ui64Imm & ADR_IMM_HIMASK) << ADR_IMM_HISHIFT;
        ui64Imm   = uiImmlo | uiImmhi;
        uiMask    = ((ADR_IMM_LOMASK << ADR_IMM_LOSHIFT) |
                     (ADR_IMM_HIMASK << ADR_IMM_HISHIFT));
        break;

    default:
        if (__arm64ImmShiftMaskGet(immType, &uiMask, &iShift) < 0) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "aarch64_insn_encode_immediate: "
                         "unknown immediate encoding %d\n", immType);
            return  (PX_ERROR);
        }
        break;
    }

    uiInsn &= ~(uiMask << iShift);                                      /* Update the immediate field. */
    uiInsn |= (ui64Imm & uiMask) << iShift;

    return  (uiInsn);
}
/*********************************************************************************************************
** 函数名称: __arm64RelocInsnImm
** 功能描述: 立即数寻址指令重定位
** 输　入  : op            操作符类型
**           pvPlace       目标重定位地址
**           ui64Val       (S + A)
**           iLsb          需要移位操作的位数
**           iLen          指令长度
**           immType       立即数类型
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __arm64RelocInsnImm (__ARM64_RELOC_OP  op,
                                 PVOID             pvPlace,
                                 UINT64            ui64Val,
                                 INT               iLsb,
                                 INT               iLen,
                                 __ARM64_INSN_IMM  immType)
{
    UINT64  u64Imm;
    UINT64  u64ImmMask;
    INT64   i64Val;
    UINT32  uiInsn  = *(UINT32 *)pvPlace;

    i64Val     = __arm64RelocOp(op, pvPlace, ui64Val);                 /*  Calculate the reloca value. */
    i64Val   >>= iLsb;
    u64ImmMask = (BIT(iLsb + iLen) - 1) >> iLsb;                       /*  Extract the value bits and  */
                                                                       /*  shift them to bit 0.        */
    u64Imm     = i64Val & u64ImmMask;
    uiInsn     = __arm64InsnEncodeImmediate(immType, uiInsn, u64Imm);  /*  Update instruct imm field.  */

    *(UINT32 *)pvPlace = uiInsn;

    /*
     * Extract the upper value bits (including the sign bit) and
     * shift them to bit 0.
     */
    i64Val = (INT64)(i64Val & ~(u64ImmMask >> 1)) >> (iLen - 1);

    /*
     * Overflow has occurred if the upper bits are not all equal to
     * the sign bit of the value.
     */
    if ((UINT64)(i64Val + 1) >= 2) {
        return  (-ERANGE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __arm64RelocData
** 功能描述: 进行重定位操作
** 输　入  : op            操作符类型
**           pvPlace       目标重定位地址
**           ui64Val       (S + A)
**           iLen          指令长度
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __arm64RelocData (__ARM64_RELOC_OP  op, PVOID  pvPlace, UINT64  ui64Val, INT  iLen)
{
    INT64  i64Val = __arm64RelocOp(op, pvPlace, ui64Val);

    switch (iLen) {

    case 16:
        *(INT16 *)pvPlace = i64Val;
        if ((i64Val < S16_MIN) || (i64Val > U16_MAX)) {
            return  (-ERANGE);
        }
        break;

    case 32:
        *(INT32 *)pvPlace = i64Val;
        if ((i64Val < S32_MIN) || (i64Val > U32_MAX)) {
            return  (-ERANGE);
        }
        break;

    case 64:
        *(INT64 *)pvPlace = i64Val;
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Invalid length (%d) for data relocation\n", iLen);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __arm64ModulePltEntryEmit
** 功能描述: 查找跳转表项，如果没有，则新建一跳转表项
** 输　入  : pcBuffer      跳转表起始地址
**           stBuffLen     跳转表长度
**           prela         RELA 表项
**           psym          符号
**           addrSymVal    符号地址
** 输　出  : 返回跳转表项跳转指令的地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  __arm64ModulePltEntryEmit (PCHAR       pcBuffer,
                                          size_t      stBuffLen,
                                          Elf_Rela   *prela,
                                          Elf_Sym    *psym,
                                          Elf_Addr    addrSymVal)
{
    LONG_JMP_ITEM  *pJmpTable    = (LONG_JMP_ITEM *)pcBuffer;
    UINT64          ui64Val      = addrSymVal + prela->r_addend;
    ULONG           ulJmpItemCnt = stBuffLen / JMP_TABLE_ITEMLEN;
    ULONG           i;

    /*
     * We only emit PLT entries against undefined (SHN_UNDEF) symbols,
     * which are listed in the ELF symtab section, but without a type
     * or a size.
     * So, similar to how the module loader uses the Elf64_Sym::st_value
     * field to store the resolved addresses of undefined symbols, let's
     * borrow the Elf64_Sym::st_size field (whose value is never used by
     * the module loader, even for symbols that are defined) to record
     * the address of a symbol's associated PLT entry as we emit it for a
     * zero addend relocation (which is the only kind we have to deal with
     * in practice). This allows us to find duplicates without having to
     * go through the table every time.
     */
    if ((prela->r_addend == 0) && (psym->st_size != 0)) {
        return  (psym->st_size);
    }

    for (i = 0; i < ulJmpItemCnt; i++) {
        if (pJmpTable[i].uiBr != BR_X16_INSTRUCTION) {
            break;
        }
    }

    /*
     * MOVK/MOVN/MOVZ opcode:
     * +--------+------------+--------+-----------+-------------+---------+
     * | sf[31] | opc[30:29] | 100101 | hw[22:21] | imm16[20:5] | Rd[4:0] |
     * +--------+------------+--------+-----------+-------------+---------+
     *
     * Rd     := 0x10 (x16)
     * hw     := 0b00 (no shift), 0b01 (lsl #16), 0b10 (lsl #32), 0b11 (lsl #48)
     * opc    := 0b11 (MOVK), 0b00 (MOVN), 0b10 (MOVZ)
     * sf     := 1 (64-bit variant)
     */
    pJmpTable[i] = (LONG_JMP_ITEM){(0x92800010 | (((~ui64Val      ) & 0xffff)) << 5),
                                   (0xf2a00010 | ((( ui64Val >> 16) & 0xffff)) << 5),
                                   (0xf2c00010 | ((( ui64Val >> 32) & 0xffff)) << 5),
                                   (0xf2e00010 | ((( ui64Val >> 48) & 0xffff)) << 5),
                                   (0xd61f0200)};

    if (prela->r_addend == 0) {
        psym->st_size = (UINT64)&pJmpTable[i];
    }

    return  ((UINT64)&pJmpTable[i]);
}
/*********************************************************************************************************
** 函数名称: archElfGotInit
** 功能描述: GOT 重定位
** 输　入  : pmodule       模块
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfGotInit (PVOID  pmodule)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archElfRelocateRela
** 功能描述: 重定位 RELA 类型的重定位项
** 输  入  : pmodule      模块
**           prela        RELA 表项
**           psym         符号
**           addrSymVal   重定位符号的值
**           pcTargetSec  重定位目标节区
**           pcBuffer     跳转表起始地址
**           stBuffLen    跳转表长度
** 输  出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRelocateRela (PVOID       pmodule,
                          Elf_Rela   *prela,
                          Elf_Sym    *psym,
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    Elf_Addr  *paddrWhere;   
    UINT64     ui64Val;
    INT        iOverflow;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  计算重定位目标地址          */
    ui64Val    = addrSymVal + prela->r_addend;

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_AARCH64_NONE:                                                /*  Null relocations.           */
        break;

    /*
     *  Data relocations.
     */
    case R_AARCH64_ABS64:
    case R_AARCH64_GLOB_DAT:                                            /*  REL_GOT                     */
    case R_AARCH64_JUMP_SLOT:                                           /*  REL_PLT                     */
        __arm64RelocData(RELOC_OP_ABS,  paddrWhere, ui64Val, 64);
        break;

    case R_AARCH64_RELATIVE:
        *paddrWhere += (Elf_Addr)pcTargetSec;
        break;

    case R_AARCH64_ABS32:
        __arm64RelocData(RELOC_OP_ABS,  paddrWhere, ui64Val, 32);
        break;

    case R_AARCH64_ABS16:
        __arm64RelocData(RELOC_OP_ABS,  paddrWhere, ui64Val, 16);
        break;

    case R_AARCH64_PREL64:
        __arm64RelocData(RELOC_OP_PREL, paddrWhere, ui64Val, 64);
        break;

    case R_AARCH64_PREL32:
        __arm64RelocData(RELOC_OP_PREL, paddrWhere, ui64Val, 32);
        break;

    case R_AARCH64_PREL16:
        __arm64RelocData(RELOC_OP_PREL, paddrWhere, ui64Val, 16);
        break;

    /*
     * Immediate instruction relocations.
     */
    case R_AARCH64_LD_PREL_LO19:
        __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 2, 19, AARCH64_INSN_IMM_19);
        break;

    case R_AARCH64_ADR_PREL_LO21:
        __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 0, 21, AARCH64_INSN_IMM_ADR);
        break;

    case R_AARCH64_ADR_PREL_PG_HI21_NC:
    case R_AARCH64_ADR_PREL_PG_HI21:
        __arm64RelocInsnImm(RELOC_OP_PAGE, paddrWhere, ui64Val, 12, 21, AARCH64_INSN_IMM_ADR);
        break;

    case R_AARCH64_ADD_ABS_LO12_NC:
    case R_AARCH64_LDST8_ABS_LO12_NC:
        __arm64RelocInsnImm(RELOC_OP_ABS,  paddrWhere, ui64Val, 0, 12, AARCH64_INSN_IMM_12);
        break;

    case R_AARCH64_LDST16_ABS_LO12_NC:
        __arm64RelocInsnImm(RELOC_OP_ABS, paddrWhere, ui64Val, 1, 11, AARCH64_INSN_IMM_12);
        break;

    case R_AARCH64_LDST32_ABS_LO12_NC:
        __arm64RelocInsnImm(RELOC_OP_ABS, paddrWhere, ui64Val, 2, 10, AARCH64_INSN_IMM_12);
        break;

    case R_AARCH64_LDST64_ABS_LO12_NC:
        __arm64RelocInsnImm(RELOC_OP_ABS, paddrWhere, ui64Val, 3, 9, AARCH64_INSN_IMM_12);
        break;

    case R_AARCH64_LDST128_ABS_LO12_NC:
        __arm64RelocInsnImm(RELOC_OP_ABS, paddrWhere, ui64Val, 4, 8, AARCH64_INSN_IMM_12);
        break;

    case R_AARCH64_TSTBR14:
        __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 2, 14, AARCH64_INSN_IMM_14);
        break;

    case R_AARCH64_CONDBR19:
        __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 2, 19, AARCH64_INSN_IMM_19);
        break;

    case R_AARCH64_JUMP26:
    case R_AARCH64_CALL26:
        iOverflow = __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 2, 26, AARCH64_INSN_IMM_26);
        if (iOverflow == -ERANGE) {
            ui64Val = __arm64ModulePltEntryEmit(pcBuffer,stBuffLen, prela, psym, addrSymVal);

            __arm64RelocInsnImm(RELOC_OP_PREL, paddrWhere, ui64Val, 2, 26, AARCH64_INSN_IMM_26);
        }
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archElfRelocateRel
** 功能描述: 重定位 REL 类型的重定位项
** 输  入  : pmodule      模块
**           prel         REL 表项
**           psym         符号
**           addrSymVal   重定位符号的值
**           pcTargetSec  重定位目目标节区
**           pcBuffer     跳转表起始地址
**           stBuffLen    跳转表长度
** 输  出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRelocateRel (PVOID        pmodule,
                         Elf_Rel     *prel,
                         Elf_Sym     *psym,
                         Elf_Addr     addrSymVal,
                         PCHAR        pcTargetSec,
                         PCHAR        pcBuffer,
                         size_t       stBuffLen)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: archElfRGetJmpBuffItemLen
** 功能描述: 返回跳转表项长度
** 输  入  : pmodule       模块
** 输  出  : 跳转表项长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRGetJmpBuffItemLen (PVOID  pmodule)
{
    return  (JMP_TABLE_ITEMLEN);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM64       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
