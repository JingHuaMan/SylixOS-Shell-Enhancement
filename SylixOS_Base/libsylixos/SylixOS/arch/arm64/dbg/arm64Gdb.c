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
** ��   ��   ��: arm64Gdb.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 29 ��
**
** ��        ��: ARM64 ��ϵ���� GDB ���Խӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../arm64_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
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
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cArm64TargetXml[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC �Ĵ����� GDB_REG_SET �ṹ�е�����
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
  ָ������
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
  ����ִ������
*********************************************************************************************************/
typedef struct {
    UINT8  UNC_ucCond;                                                  /* Cond ֵ                      */
    UINT8  UNC_ucNZCVMask;                                              /* NZCV ����                    */
    UINT8  UNC_ucNZCVVal;                                               /* NZCV ��־ֵ                  */
} ARM64_COND;
/*********************************************************************************************************
  ����ִ��ָ���
*********************************************************************************************************/
static ARM64_COND  arm64condTable [] = {
    {0x00, 0x04, 0x04},                                                 /* EQ   : 0b0000��Z==1          */
    {0x01, 0x04, 0x00},                                                 /* NE   : 0b0001��Z==0          */
    {0x02, 0x02, 0x02},                                                 /* HS/CS: 0b0010��C==1          */
    {0x03, 0x02, 0x00},                                                 /* LO/CC: 0b0011��C==0          */
    {0x04, 0x08, 0x08},                                                 /* MI   : 0b0100��N==1          */
    {0x05, 0x08, 0x00},                                                 /* PL   : 0b0101��N==0          */
    {0x06, 0x01, 0x01},                                                 /* VS   : 0b0110��V==1          */
    {0x07, 0x01, 0x00},                                                 /* VC   : 0b0111��V==0          */
    {0x08, 0x06, 0x02},                                                 /* HI   : 0b1000��C==1 && Z==0  */
    {0x09, 0x04, 0x00},                                                 /* LS   : 0b1001��C==0 && Z==0  */
    {0x09, 0x04, 0x04},                                                 /* LS   : 0b1001��C==0 && Z==1  */
    {0x09, 0x04, 0x06},                                                 /* LS   : 0b1001��C==1 && Z==1  */
    {0x0a, 0x09, 0x09},                                                 /* GE   : 0b1010, N==1 && V==1  */
    {0x0a, 0x09, 0x00},                                                 /* GE   : 0b1010, N==0 && V==0  */
    {0x0b, 0x09, 0x08},                                                 /* LT   : 0b1011��N==0 && V==1  */
    {0x0b, 0x09, 0x01},                                                 /* LT   : 0b1011��N==1 && V==0  */
    {0x0c, 0x0d, 0x09},                                                 /* GT   : 0b1100��Z==0&&N==V==1 */
    {0x0c, 0x0d, 0x00},                                                 /* GT   : 0b1100��Z==0&&N==V==1 */
    {0x0d, 0x0d, 0x0c},                                                 /* LE   : 0b1101��Z==1 || N!=V  */
    {0x0d, 0x0d, 0x08},                                                 /* LE   : 0b1101��Z==1 || N!=V  */
    {0x0d, 0x0d, 0x05},                                                 /* LE   : 0b1101��Z==1 || N!=V  */
    {0x0d, 0x0d, 0x01},                                                 /* LE   : 0b1101��Z==1 || N!=V  */
    {0x0e, 0x00, 0x00},                                                 /* AL   : 0b1110��ANY           */
    {0x0f, 0x00, 0x00},                                                 /* NV   : 0b1111��ANY           */
};
/*********************************************************************************************************
** ��������: archGdbTargetXml
** ��������: ��� Xfer:features:read:target.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbTargetXml (VOID)
{
    return  (_G_cArm64TargetXml);
}
/*********************************************************************************************************
** ��������: archGdbCoreXml
** ��������: ��� Xfer:features:read:arm-core.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (_G_cArm64CoreXml);
}
/*********************************************************************************************************
** ��������: archGdbRegsGet
** ��������: ��ȡ�Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
** �䡡��  : pregset        gdb �Ĵ����ṹ
**           ����ֵ         �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: archGdbRegsSet
** ��������: ���üĴ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           pregset        gdb �Ĵ����ṹ
** �䡡��  : �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: archGdbRegSetPc
** ��������: ���� pc �Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           ulPC           pc �Ĵ���ֵ
** �䡡��  : �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: archGdbRegGetPc
** ��������: ��ȡ pc �Ĵ���ֵ
** �䡡��  : pRegs       �Ĵ�������
** �䡡��  : PC�Ĵ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
** ��������: __arm64DecodeMaskedMatch
** ��������: ָ�������ж�
** �䡡��  : uiInsn       ָ��ֵ
**           uiMask       ָ������
**           uiPattern    ָ���ж�ֵ
** �䡡��  : LW_TRUE ƥ�䣬 LW_FALSE ��ƥ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __arm64DecodeMaskedMatch (UINT32  uiInsn, UINT32  uiMask, UINT32  uiPattern)
{
    return  (uiInsn & uiMask) == uiPattern;
}
/*********************************************************************************************************
** ��������: __arm64ExtractSignedBitfield
** ��������: ָ�������ж�
** �䡡��  : uiInsn       ָ��ֵ
**           uiWidth      ָ����
**           uiOffset     ƫ��ֵ
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT32  __arm64ExtractSignedBitfield (UINT32  uiInsn, UINT32  uiWidth, UINT32  uiOffset)
{
    UINT32  uiShiftL = sizeof(INT) * 8 - (uiOffset + uiWidth);
    UINT32  uiShiftR = sizeof(INT) * 8 - uiWidth;

    return ((INT)uiInsn << uiShiftL) >> uiShiftR;
}
/*********************************************************************************************************
** ��������: archGdbGetNextPc
** ��������: ��ȡ��һ��ָ���ַ������֧Ԥ��
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           pRegs          �Ĵ�������
** �䡡��  : ��һ��ָ���ַ
** ȫ�ֱ���:
** ����ģ��:
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

    ulPc     = pRegs->regArr[ARM64_REG_INDEX_PC].GDBRA_ulValue;         /*  ��ǰ PC                     */
    ulNextPc = ulPc + 4;                                                /*  Ĭ��Ϊ��ǰ PC + 4           */
    uiInstr  = *(UINT *)ulPc;                                           /*  ȡ��ǰָ������              */

    if (__arm64DecodeMaskedMatch(uiInstr,
                                 INSN_B_BL_MASK,
                                 INSN_B_BL_VAL)) {                      /*  B��BL ָ��                  */
        iOffset  = __arm64ExtractSignedBitfield(uiInstr, 26, 0) << 2;
        ulNextPc = ulPc + iOffset;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_BCOND_MASK,
                                        INSN_BCOND_VAL)) {              /*  BCOND ָ��                  */
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
                                        INSN_CBZ_VAL)) {                /*  CBZ ָ��                    */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond == 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    }  else if (__arm64DecodeMaskedMatch(uiInstr,
                                         INSN_CBNZ_MASK,
                                         INSN_CBNZ_VAL)) {              /*  CBNZ ָ��                   */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond != 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_TBZ_MASK,
                                        INSN_TBZ_VAL)) {                /*  TBZ ָ��                    */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond == 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 14, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_TBNZ_MASK,
                                        INSN_TBNZ_VAL)) {               /*  TBNZ ָ��                   */
        uiCond = pRegs->regArr[uiInstr & 0x1f].GDBRA_ulValue;
        if (uiCond != 0) {
            iOffset  = __arm64ExtractSignedBitfield(uiInstr, 14, 5) << 2;
            ulNextPc = ulPc + iOffset;
        }

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_LDR_LDRSW_MASK,
                                        INSN_LDR_LDRSW_VAL)) {          /*  LDR��LDRSW ָ��             */
        iOffset  = __arm64ExtractSignedBitfield(uiInstr, 19, 5) << 2;
        ulNextPc = ulPc + iOffset;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_RET_MASK,
                                        INSN_RET_VAL)) {                /*  RET ָ��                    */
        uiReg    = (uiInstr & 0x3e0) >> 5;
        ulNextPc = pRegs->regArr[uiReg].GDBRA_ulValue;

    } else if (__arm64DecodeMaskedMatch(uiInstr,
                                        INSN_BR_MASK,
                                        INSN_BR_VAL)) {                 /*  BR ָ��                     */
        uiReg    = (uiInstr & 0x3e0) >> 5;                              /*  ��ȡ�Ĵ���                  */
        ulNextPc = pRegs->regArr[uiReg].GDBRA_ulValue;
    }

    return  (ulNextPc);
}
/*********************************************************************************************************
** ��������: archGdbGetStepSkip
** ��������: �Ƿ���Դ˵����ϵ�
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           ulAddr         �ϵ��ַ
** �䡡��  : �Ƿ����
** ȫ�ֱ���:
** ����ģ��:
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
