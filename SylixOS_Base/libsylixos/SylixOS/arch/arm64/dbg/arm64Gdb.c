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
** 文   件   名: arm64Gdb.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: ARM64 体系构架 GDB 调试接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../arm64_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
static const CHAR   _G_cArm64CoreXml[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2018 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.aarch64.core\">"
          "<reg name=\"x0\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x1\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x2\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x3\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x4\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x5\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x6\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x7\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x8\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x9\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x10\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x11\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x12\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x13\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x14\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x15\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x16\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x17\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x18\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x19\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x20\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x21\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x22\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x23\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x24\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x25\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x26\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x27\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x28\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x29\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"x30\" bitsize=\"64\" type=\"uint64\"/>"
          "<reg name=\"sp\" bitsize=\"64\" type=\"data_ptr\"/>"
          "<reg name=\"pc\" bitsize=\"64\" type=\"code_ptr\"/>"
          "<reg name=\"pstate\" bitsize=\"32\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml 回应包
*********************************************************************************************************/
static const CHAR   _G_cArm64TargetXml[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC 寄存器在 GDB_REG_SET 结构中的索引
*********************************************************************************************************/
#define ARM64_REG_INDEX_FP         29
#define ARM64_REG_INDEX_SP         31
#define ARM64_REG_INDEX_PC         32
#define ARM64_REG_INDEX_PSTATE     33
#define ARM64_REG_INDEX_FP_HI(n)   (34 + ((n) << 1))
#define ARM64_REG_INDEX_FP_LO(n)   (34 + ((n) << 1) + 1)
#define ARM64_REG_INDEX_FPSR       98
#define ARM64_REG_INDEX_FPCR       99
/*********************************************************************************************************
  指令掩码
*********************************************************************************************************/
#define INSN_B_BL_MASK          0x7c000000
#define INSN_B_BL_VAL           0x14000000
#define INSN_BCOND_MASK         0xff000010
#define INSN_BCOND_VAL          0x54000000
#define INSN_CBZ_MASK           0x7f000000
#define INSN_CBZ_VAL            0x34000000
#define INSN_CBNZ_MASK          0x7f000000
#define INSN_CBNZ_VAL           0x35000000
#define INSN_TBZ_MASK           0x7f000000
#define INSN_TBZ_VAL            0x36000000
#define INSN_TBNZ_MASK          0x7f000000
#define INSN_TBNZ_VAL           0x37000000
#define INSN_LDR_LDRSW_MASK     0x3f000000
#define INSN_LDR_LDRSW_VAL      0x18000000
#define INSN_RET_MASK           0xfffffc1f
#define INSN_RET_VAL            0xd65f0000
#define INSN_BR_MASK            0xffff0000
#define INSN_BR_VAL             0xd61f0000
/*********************************************************************************************************
  条件执行条件
*********************************************************************************************************/
typedef struct {
    UINT8  UNC_ucCond;                                                  /* Cond 值                      */
    UINT8  UNC_ucNZCVMask;                                              /* NZCV 掩码                    */
    UINT8  UNC_ucNZCVVal;                                               /* NZCV 标志值                  */
} ARM64_COND;
/*********************************************************************************************************
  条件执行指令表
*********************************************************************************************************/
static ARM64_COND  arm64condTable [] = {
    {0x00, 0x04, 0x04},                                                 /* EQ   : 0b0000，Z==1          */
    {0x01, 0x04, 0x00},                                                 /* NE   : 0b0001，Z==0          */
    {0x02, 0x02, 0x02},                                                 /* HS/CS: 0b0010，C==1          */
    {0x03, 0x02, 0x00},                                                 /* LO/CC: 0b0011，C==0          */
    {0x04, 0x08, 0x08},                                                 /* MI   : 0b0100，N==1          */
    {0x05, 0x08, 0x00},                                                 /* PL   : 0b0101，N==0          */
    {0x06, 0x01, 0x01},                                                 /* VS   : 0b0110，V==1          */
    {0x07, 0x01, 0x00},                                                 /* VC   : 0b0111，V==0          */
    {0x08, 0x06, 0x02},                                                 /* HI   : 0b1000，C==1 && Z==0  */
    {0x09, 0x04, 0x00},                                                 /* LS   : 0b1001，C==0 && Z==0  */
    {0x09, 0x04, 0x04},                                                 /* LS   : 0b1001，C==0 && Z==1  */
    {0x09, 0x04, 0x06},                                                 /* LS   : 0b1001，C==1 && Z==1  */
    {0x0a, 0x09, 0x09},                                                 /* GE   : 0b1010, N==1 && V==1  */
    {0x0a, 0x09, 0x00},                                                 /* GE   : 0b1010, N==0 && V==0  */
    {0x0b, 0x09, 0x08},                                                 /* LT   : 0b1011，N==0 && V==1  */
    {0x0b, 0x09, 0x01},                                                 /* LT   : 0b1011，N==1 && V==0  */
    {0x0c, 0x0d, 0x09},                                                 /* GT   : 0b1100，Z==0&&N==V==1 */
    {0x0c, 0x0d, 0x00},                                                 /* GT   : 0b1100，Z==0&&N==V==1 */
    {0x0d, 0x0d, 0x0c},                                                 /* LE   : 0b1101，Z==1 || N!=V  */
    {0x0d, 0x0d, 0x08},                                                 /* LE   : 0b1101，Z==1 || N!=V  */
    {0x0d, 0x0d, 0x05},                                                 /* LE   : 0b1101，Z==1 || N!=V  */
    {0x0d, 0x0d, 0x01},                                                 /* LE   : 0b1101，Z==1 || N!=V  */
    {0x0e, 0x00, 0x00},                                                 /* AL   : 0b1110，ANY           */
    {0x0f, 0x00, 0x00},                                                 /* NV   : 0b1111，ANY           */
};
/*********************************************************************************************************
** 函数名称: archGdbTargetXml
** 功能描述: 获得 Xfer:features:read:target.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbTargetXml (VOID)
{
    return  (_G_cArm64TargetXml);
}
/*********************************************************************************************************
** 函数名称: archGdbCoreXml
** 功能描述: 获得 Xfer:features:read:arm-core.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (_G_cArm64CoreXml);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsGet
** 功能描述: 获取寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
** 输　出  : pregset        gdb 寄存器结构
**           返回值         成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;
    INT           i;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt    = GDB_MAX_REG_CNT;
    pregset->GDBR_iPstateInx = ARM64_REG_INDEX_PSTATE;
    pregset->GDBR_iFpsrInx   = ARM64_REG_INDEX_FPSR;
    pregset->GDBR_iFpcrInx   = ARM64_REG_INDEX_FPCR;

    for (i = 0; i < ARCH_GREG_NR; i++) {
        pregset->regArr[i].GDBRA_ulValue = regctx.REG_ulReg[i];
    }

    pregset->regArr[ARM64_REG_INDEX_SP].GDBRA_ulValue     = regSp;
    pregset->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue     = regctx.REG_ulPc;
    pregset->regArr[ARM64_REG_INDEX_PSTATE].GDBRA_ulValue = regctx.REG_ulPstate;

#if LW_CFG_CPU_FPU_EN > 0
    {
        ARCH_FPU_CTX    fpuctx;

        API_DtraceGetFpuRegs(pvDtrace, ulThread, &fpuctx);

        for (i = 0; i < FPU_REG_NR; i++) {
            pregset->regArr[ARM64_REG_INDEX_FP_HI(i)].GDBRA_ulValue = fpuctx.FPUCTX_uiDreg[i].u.val64[0];
            pregset->regArr[ARM64_REG_INDEX_FP_LO(i)].GDBRA_ulValue = fpuctx.FPUCTX_uiDreg[i].u.val64[1];
        }
        pregset->regArr[ARM64_REG_INDEX_FPSR].GDBRA_ulValue = fpuctx.FPUCTX_uiFpsr;
        pregset->regArr[ARM64_REG_INDEX_FPCR].GDBRA_ulValue = fpuctx.FPUCTX_uiFpcr;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsSet
** 功能描述: 设置寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           pregset        gdb 寄存器结构
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    INT           i;

    for (i = 0; i < ARCH_GREG_NR; i++) {
        regctx.REG_ulReg[i] = pregset->regArr[i].GDBRA_ulValue;
    }

    regctx.REG_ulSp     = pregset->regArr[ARM64_REG_INDEX_SP].GDBRA_ulValue;
    regctx.REG_ulPc     = pregset->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue;
    regctx.REG_ulPstate = pregset->regArr[ARM64_REG_INDEX_PSTATE].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegSetPc
** 功能描述: 设置 pc 寄存器值
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           ulPC           pc 寄存器值
** 输　出  : 成功-- ERROR_NONE，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_ulPc = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegGetPc
** 功能描述: 获取 pc 寄存器值
** 输　入  : pRegs       寄存器数组
** 输　出  : PC寄存器值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
** 函数名称: __arm64DecodeMaskedMatch
** 功能描述: 指令类型判断
** 输　入  : uiInsn       指令值
**           uiMask       指令掩码
**           uiPattern    指令判断值
** 输　出  : LW_TRUE 匹配， LW_FALSE 不匹配
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __arm64DecodeMaskedMatch (UINT32  uiInsn, UINT32  uiMask, UINT32  uiPattern)
{
    return  (uiInsn & uiMask) == uiPattern;
}
/*********************************************************************************************************
** 函数名称: __arm64ExtractSignedBitfield
** 功能描述: 指令类型判断
** 输　入  : uiInsn       指令值
**           uiWidth      指令宽度
**           uiOffset     偏移值
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT32  __arm64ExtractSignedBitfield (UINT32  uiInsn, UINT32  uiWidth, UINT32  uiOffset)
{
    UINT32  uiShiftL = sizeof(INT) * 8 - (uiOffset + uiWidth);
    UINT32  uiShiftR = sizeof(INT) * 8 - uiWidth;

    return ((INT)uiInsn << uiShiftL) >> uiShiftR;
}
/*********************************************************************************************************
** 函数名称: archGdbGetNextPc
** 功能描述: 获取下一条指令地址，含分支预测
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           pRegs          寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pRegs)
{
    ULONG    ulPc;
    ULONG    ulNextPc;
    UINT32   uiInstr;
    INT32    iOffset;
    UINT32   uiReg;
    UINT32   uiCond;
    INT      i;

    ulPc     = pRegs->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue;         /*  当前 PC                     */
    ulNextPc = ulPc + 4;                                                /*  默认为当前 PC + 4           */
    uiInstr  = *(UINT *)ulPc;                                           /*  取当前指令内容              */

    if (__arm64DecodeMaskedMatch(uiInstr,
                                 INSN_B_BL_MASK,
                                 INSN_B_BL_VAL)) {                      /*  B、BL 指令                  */
        iOffset  = __arm64ExtractSignedBitfield(uiInstr, 26, 0) << 2;
        ulNextPc = ulPc + iOffset;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_BCOND_MASK,
                                        INSN_BCOND_VAL)) {              /*  BCOND 指令                  */
         uiCond = uiInstr & 0xf;

         for (i = 0;
              i < (sizeof(arm64condTable) / sizeof(arm64condTable[0]));
              i++) {
              if ((arm64condTable[i].UNC_ucCond == uiCond) &&
                  (((pRegs->regArr[ARM64_REG_INDEX_PSTATE].GDBRA_ulValue >> 28) &
                    arm64condTable[i].UNC_ucNZCVMask) ==
                    arm64condTable[i].UNC_ucNZCVVal)) {
                  iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
                  ulNextPc = ulPc + iOffset;
                  break;
              }
         }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_CBZ_MASK,
                                        INSN_CBZ_VAL)) {                /*  CBZ 指令                    */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond == 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    }  else if (__arm64DecodeMaskedMatch(uiInstr,
                                         INSN_CBNZ_MASK,
                                         INSN_CBNZ_VAL)) {              /*  CBNZ 指令                   */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond != 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_TBZ_MASK,
                                        INSN_TBZ_VAL)) {                /*  TBZ 指令                    */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond == 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 14, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_TBNZ_MASK,
                                        INSN_TBNZ_VAL)) {               /*  TBNZ 指令                   */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond != 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 14, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_LDR_LDRSW_MASK,
                                        INSN_LDR_LDRSW_VAL)) {          /*  LDR、LDRSW 指令             */
        iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
        ulNextPc = ulPc + iOffset;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_RET_MASK,
                                        INSN_RET_VAL)) {                /*  RET 指令                    */
        uiReg    = (uiInstr & 0x3e0) >> 5;
        ulNextPc = pRegs->regArr[uiReg].GDBRA_ulValue;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_BR_MASK,
                                        INSN_BR_VAL)) {                 /*  BR 指令                     */
        uiReg    = (uiInstr & 0x3e0) >> 5;                              /*  获取寄存器                  */
        ulNextPc = pRegs->regArr[uiReg].GDBRA_ulValue;
    }

    return  (ulNextPc);
}
/*********************************************************************************************************
** 函数名称: archGdbGetStepSkip
** 功能描述: 是否忽略此单步断点
** 输　入  : pvDtrace       dtrace 句柄
**           ulThread       被调试线程
**           ulAddr         断点地址
** 输　出  : 是否忽略
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  archGdbGetStepSkip (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, addr_t ulAddr)
{
    if ((*(UINT32 *)ulAddr         == 0x910003fd) ||
        (*(((UINT32 *)ulAddr) + 1) == 0x910003fd) ||
        (*(((UINT32 *)ulAddr) + 2) == 0x910003fd)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
