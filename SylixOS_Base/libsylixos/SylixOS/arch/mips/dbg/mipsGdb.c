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
** ��   ��   ��: mipsGdb.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ� GDB ���Խӿ�.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../mips_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cMipsCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.mips.cpu\">"
          "<reg name=\"zero\" bitsize=\"32\" type=\"uint32\" regnum=\"0\"/>"
          "<reg name=\"at\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"v0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"v1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t4\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t5\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t6\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t7\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s4\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s5\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s6\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s7\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t8\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t9\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"k0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"k1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"gp\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"sp\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s8\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"ra\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"sr\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"lo\" bitsize=\"32\" type=\"uint32\" regnum=\"33\"/>"
          "<reg name=\"hi\" bitsize=\"32\" type=\"uint32\" regnum=\"34\"/>"
          "<reg name=\"bad\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"cause\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"pc\" bitsize=\"32\" type=\"uint32\" regnum=\"37\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cTargetSystem[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  �Ĵ����� GDB_REG_SET �ṹ�е�����
*********************************************************************************************************/
#define GDB_REG_INDEX_GREG(n)   (n)                                     /*  32 ��ͨ��Ŀ�ļĴ���         */
#define GDB_REG_INDEX_SR        32                                      /*  CP0 Э������״̬�Ĵ���      */
#define GDB_REG_INDEX_LO        33                                      /*  ������λ�Ĵ���              */
#define GDB_REG_INDEX_HI        34                                      /*  ������λ�Ĵ���              */
#define GDB_REG_INDEX_BAD       35                                      /*  �����ַ�Ĵ���              */
#define GDB_REG_INDEX_CAUSE     36                                      /*  �����жϻ����쳣�鿴�ļĴ���*/
#define GDB_REG_INDEX_PC        37                                      /*  ����������Ĵ���            */
#define GDB_REG_INDEX_FP(n)     (38 + (n))                              /*  32 ���������ݼĴ���         */
#define GDB_REG_INDEX_FCSR      (38 + 32 + 0)                           /*  �������״̬�Ĵ���          */
#define GDB_REG_INDEX_FIR       (38 + 32 + 1)                           /*  ����ʵ�ּĴ���              */

#if (LW_CFG_CPU_DSP_EN > 0) && defined(_MIPS_ARCH_HR2)
#define GDB_REG_INDEX_HR2_V(n, sn)  \
                                (38 + 34 + (n) * 4 + (sn))              /*  32 ���������ݼĴ���         */
#define GDB_REG_NR              (38 + 34 + 129)                         /*  �Ĵ�������                  */
#else
#define GDB_REG_NR              (38 + 34)                               /*  �Ĵ�������                  */
#endif                                                                  /*  DSP_EN & _MIPS_ARCH_HR2     */
/*********************************************************************************************************
  ����״̬�Ĵ�������Ϊ��fpcsr �Ĵ�����λΪ 1 ��ʶ��������
*********************************************************************************************************/
#define FP_COND                 0x800000
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
    return  (cTargetSystem);
}
/*********************************************************************************************************
** ��������: archGdbCoreXml
** ��������: ��� Xfer:features:read:mips-core.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (cMipsCore);
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
INT  archGdbRegsGet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX    regctx;
    ARCH_REG_T      regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->regArr[GDB_REG_INDEX_GREG( 0)].GDBRA_ulValue = 0;          /*  ZERO                        */
    pregset->regArr[GDB_REG_INDEX_GREG( 1)].GDBRA_ulValue = regctx.REG_ulReg[ 1];
    pregset->regArr[GDB_REG_INDEX_GREG( 2)].GDBRA_ulValue = regctx.REG_ulReg[ 2];
    pregset->regArr[GDB_REG_INDEX_GREG( 3)].GDBRA_ulValue = regctx.REG_ulReg[ 3];
    pregset->regArr[GDB_REG_INDEX_GREG( 4)].GDBRA_ulValue = regctx.REG_ulReg[ 4];
    pregset->regArr[GDB_REG_INDEX_GREG( 5)].GDBRA_ulValue = regctx.REG_ulReg[ 5];
    pregset->regArr[GDB_REG_INDEX_GREG( 6)].GDBRA_ulValue = regctx.REG_ulReg[ 6];
    pregset->regArr[GDB_REG_INDEX_GREG( 7)].GDBRA_ulValue = regctx.REG_ulReg[ 7];
    pregset->regArr[GDB_REG_INDEX_GREG( 8)].GDBRA_ulValue = regctx.REG_ulReg[ 8];
    pregset->regArr[GDB_REG_INDEX_GREG( 9)].GDBRA_ulValue = regctx.REG_ulReg[ 9];
    pregset->regArr[GDB_REG_INDEX_GREG(10)].GDBRA_ulValue = regctx.REG_ulReg[10];
    pregset->regArr[GDB_REG_INDEX_GREG(11)].GDBRA_ulValue = regctx.REG_ulReg[11];
    pregset->regArr[GDB_REG_INDEX_GREG(12)].GDBRA_ulValue = regctx.REG_ulReg[12];
    pregset->regArr[GDB_REG_INDEX_GREG(13)].GDBRA_ulValue = regctx.REG_ulReg[13];
    pregset->regArr[GDB_REG_INDEX_GREG(14)].GDBRA_ulValue = regctx.REG_ulReg[14];
    pregset->regArr[GDB_REG_INDEX_GREG(15)].GDBRA_ulValue = regctx.REG_ulReg[15];
    pregset->regArr[GDB_REG_INDEX_GREG(16)].GDBRA_ulValue = regctx.REG_ulReg[16];
    pregset->regArr[GDB_REG_INDEX_GREG(17)].GDBRA_ulValue = regctx.REG_ulReg[17];
    pregset->regArr[GDB_REG_INDEX_GREG(18)].GDBRA_ulValue = regctx.REG_ulReg[18];
    pregset->regArr[GDB_REG_INDEX_GREG(19)].GDBRA_ulValue = regctx.REG_ulReg[19];
    pregset->regArr[GDB_REG_INDEX_GREG(20)].GDBRA_ulValue = regctx.REG_ulReg[20];
    pregset->regArr[GDB_REG_INDEX_GREG(21)].GDBRA_ulValue = regctx.REG_ulReg[21];
    pregset->regArr[GDB_REG_INDEX_GREG(22)].GDBRA_ulValue = regctx.REG_ulReg[22];
    pregset->regArr[GDB_REG_INDEX_GREG(23)].GDBRA_ulValue = regctx.REG_ulReg[23];
    pregset->regArr[GDB_REG_INDEX_GREG(24)].GDBRA_ulValue = regctx.REG_ulReg[24];
    pregset->regArr[GDB_REG_INDEX_GREG(25)].GDBRA_ulValue = regctx.REG_ulReg[25];
    pregset->regArr[GDB_REG_INDEX_GREG(26)].GDBRA_ulValue = regctx.REG_ulReg[26];
    pregset->regArr[GDB_REG_INDEX_GREG(27)].GDBRA_ulValue = regctx.REG_ulReg[27];
    pregset->regArr[GDB_REG_INDEX_GREG(28)].GDBRA_ulValue = regctx.REG_ulReg[28];
    pregset->regArr[GDB_REG_INDEX_GREG(29)].GDBRA_ulValue = regSp;      /*  SP                          */
    pregset->regArr[GDB_REG_INDEX_GREG(30)].GDBRA_ulValue = regctx.REG_ulReg[30];
    pregset->regArr[GDB_REG_INDEX_GREG(31)].GDBRA_ulValue = regctx.REG_ulReg[31];
    pregset->regArr[GDB_REG_INDEX_SR].GDBRA_ulValue       = regctx.REG_ulCP0Status;
    pregset->regArr[GDB_REG_INDEX_LO].GDBRA_ulValue       = regctx.REG_ulCP0DataLo;
    pregset->regArr[GDB_REG_INDEX_HI].GDBRA_ulValue       = regctx.REG_ulCP0DataHi;
    pregset->regArr[GDB_REG_INDEX_BAD].GDBRA_ulValue      = regctx.REG_ulCP0BadVAddr;
    pregset->regArr[GDB_REG_INDEX_CAUSE].GDBRA_ulValue    = regctx.REG_ulCP0Cause;
    pregset->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue       = regctx.REG_ulCP0Epc;

    /*
     * ��� Cause �Ĵ��� BD λ��Ϊ 1����˵�������жϵ�Ϊ��֧��ʱ��ָ�PC �Ĵ���ֵ�����
     */
    if (regctx.REG_ulCP0Cause & CAUSEF_BD) {
        pregset->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue += sizeof(MIPS_INSTRUCTION);
    }

#if LW_CFG_CPU_WORD_LENGHT == 64
#if LW_CFG_CPU_FPU_EN > 0
    if (regctx.REG_ulCP0Status & ST0_CU1) {
        ARCH_FPU_CTX    fpuctx;

        API_DtraceGetFpuRegs(pvDtrace, ulThread, &fpuctx);

        pregset->regArr[GDB_REG_INDEX_FP( 0)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 0].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 1)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 1].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 2)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 2].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 3)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 3].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 4)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 4].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 5)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 5].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 6)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 6].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 7)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 7].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 8)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 8].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP( 9)].GDBRA_ulValue = fpuctx.FPUCTX_reg[ 9].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(10)].GDBRA_ulValue = fpuctx.FPUCTX_reg[10].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(11)].GDBRA_ulValue = fpuctx.FPUCTX_reg[11].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(12)].GDBRA_ulValue = fpuctx.FPUCTX_reg[12].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(13)].GDBRA_ulValue = fpuctx.FPUCTX_reg[13].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(14)].GDBRA_ulValue = fpuctx.FPUCTX_reg[14].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(15)].GDBRA_ulValue = fpuctx.FPUCTX_reg[15].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(16)].GDBRA_ulValue = fpuctx.FPUCTX_reg[16].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(17)].GDBRA_ulValue = fpuctx.FPUCTX_reg[17].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(18)].GDBRA_ulValue = fpuctx.FPUCTX_reg[18].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(19)].GDBRA_ulValue = fpuctx.FPUCTX_reg[19].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(20)].GDBRA_ulValue = fpuctx.FPUCTX_reg[20].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(21)].GDBRA_ulValue = fpuctx.FPUCTX_reg[21].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(22)].GDBRA_ulValue = fpuctx.FPUCTX_reg[22].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(23)].GDBRA_ulValue = fpuctx.FPUCTX_reg[23].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(24)].GDBRA_ulValue = fpuctx.FPUCTX_reg[24].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(25)].GDBRA_ulValue = fpuctx.FPUCTX_reg[25].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(26)].GDBRA_ulValue = fpuctx.FPUCTX_reg[26].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(27)].GDBRA_ulValue = fpuctx.FPUCTX_reg[27].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(28)].GDBRA_ulValue = fpuctx.FPUCTX_reg[28].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(29)].GDBRA_ulValue = fpuctx.FPUCTX_reg[29].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(30)].GDBRA_ulValue = fpuctx.FPUCTX_reg[30].val64[0];
        pregset->regArr[GDB_REG_INDEX_FP(31)].GDBRA_ulValue = fpuctx.FPUCTX_reg[31].val64[0];
        pregset->regArr[GDB_REG_INDEX_FCSR].GDBRA_ulValue   = fpuctx.FPUCTX_uiFcsr;
        pregset->regArr[GDB_REG_INDEX_FIR].GDBRA_ulValue    = archFpuGetFIR();
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if (LW_CFG_CPU_DSP_EN > 0) && defined(_MIPS_ARCH_HR2)
    if (regctx.REG_ulCP0Status & ST0_CU2) {
        ARCH_DSP_CTX    dspctx;

        API_DtraceGetDspRegs(pvDtrace, ulThread, &dspctx);

        lib_memcpy(&pregset->regArr[GDB_REG_INDEX_HR2_V(0, 0)].GDBRA_ulValue,
                   &dspctx,
                   sizeof(ARCH_DSP_CTX));
    }
#endif                                                                  /*  DSP_EN & _MIPS_ARCH_HR2     */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/

    pregset->GDBR_iRegCnt = GDB_REG_NR;

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
INT  archGdbRegsSet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX  regctx;

    regctx.REG_ulReg[ 1]     = pregset->regArr[GDB_REG_INDEX_GREG( 1)].GDBRA_ulValue;
    regctx.REG_ulReg[ 2]     = pregset->regArr[GDB_REG_INDEX_GREG( 2)].GDBRA_ulValue;
    regctx.REG_ulReg[ 3]     = pregset->regArr[GDB_REG_INDEX_GREG( 3)].GDBRA_ulValue;
    regctx.REG_ulReg[ 4]     = pregset->regArr[GDB_REG_INDEX_GREG( 4)].GDBRA_ulValue;
    regctx.REG_ulReg[ 5]     = pregset->regArr[GDB_REG_INDEX_GREG( 5)].GDBRA_ulValue;
    regctx.REG_ulReg[ 6]     = pregset->regArr[GDB_REG_INDEX_GREG( 6)].GDBRA_ulValue;
    regctx.REG_ulReg[ 7]     = pregset->regArr[GDB_REG_INDEX_GREG( 7)].GDBRA_ulValue;
    regctx.REG_ulReg[ 8]     = pregset->regArr[GDB_REG_INDEX_GREG( 8)].GDBRA_ulValue;
    regctx.REG_ulReg[ 9]     = pregset->regArr[GDB_REG_INDEX_GREG( 9)].GDBRA_ulValue;
    regctx.REG_ulReg[10]     = pregset->regArr[GDB_REG_INDEX_GREG(10)].GDBRA_ulValue;
    regctx.REG_ulReg[11]     = pregset->regArr[GDB_REG_INDEX_GREG(11)].GDBRA_ulValue;
    regctx.REG_ulReg[12]     = pregset->regArr[GDB_REG_INDEX_GREG(12)].GDBRA_ulValue;
    regctx.REG_ulReg[13]     = pregset->regArr[GDB_REG_INDEX_GREG(13)].GDBRA_ulValue;
    regctx.REG_ulReg[14]     = pregset->regArr[GDB_REG_INDEX_GREG(14)].GDBRA_ulValue;
    regctx.REG_ulReg[15]     = pregset->regArr[GDB_REG_INDEX_GREG(15)].GDBRA_ulValue;
    regctx.REG_ulReg[16]     = pregset->regArr[GDB_REG_INDEX_GREG(16)].GDBRA_ulValue;
    regctx.REG_ulReg[17]     = pregset->regArr[GDB_REG_INDEX_GREG(17)].GDBRA_ulValue;
    regctx.REG_ulReg[18]     = pregset->regArr[GDB_REG_INDEX_GREG(18)].GDBRA_ulValue;
    regctx.REG_ulReg[19]     = pregset->regArr[GDB_REG_INDEX_GREG(19)].GDBRA_ulValue;
    regctx.REG_ulReg[20]     = pregset->regArr[GDB_REG_INDEX_GREG(20)].GDBRA_ulValue;
    regctx.REG_ulReg[21]     = pregset->regArr[GDB_REG_INDEX_GREG(21)].GDBRA_ulValue;
    regctx.REG_ulReg[22]     = pregset->regArr[GDB_REG_INDEX_GREG(22)].GDBRA_ulValue;
    regctx.REG_ulReg[23]     = pregset->regArr[GDB_REG_INDEX_GREG(23)].GDBRA_ulValue;
    regctx.REG_ulReg[24]     = pregset->regArr[GDB_REG_INDEX_GREG(24)].GDBRA_ulValue;
    regctx.REG_ulReg[25]     = pregset->regArr[GDB_REG_INDEX_GREG(25)].GDBRA_ulValue;
    regctx.REG_ulReg[26]     = pregset->regArr[GDB_REG_INDEX_GREG(26)].GDBRA_ulValue;
    regctx.REG_ulReg[27]     = pregset->regArr[GDB_REG_INDEX_GREG(27)].GDBRA_ulValue;
    regctx.REG_ulReg[28]     = pregset->regArr[GDB_REG_INDEX_GREG(28)].GDBRA_ulValue;
    regctx.REG_ulReg[29]     = pregset->regArr[GDB_REG_INDEX_GREG(29)].GDBRA_ulValue;
    regctx.REG_ulReg[30]     = pregset->regArr[GDB_REG_INDEX_GREG(30)].GDBRA_ulValue;
    regctx.REG_ulReg[31]     = pregset->regArr[GDB_REG_INDEX_GREG(31)].GDBRA_ulValue;
    regctx.REG_ulCP0Status   = pregset->regArr[GDB_REG_INDEX_SR].GDBRA_ulValue;
    regctx.REG_ulCP0DataLo   = pregset->regArr[GDB_REG_INDEX_LO].GDBRA_ulValue;
    regctx.REG_ulCP0DataHi   = pregset->regArr[GDB_REG_INDEX_HI].GDBRA_ulValue;
    regctx.REG_ulCP0BadVAddr = pregset->regArr[GDB_REG_INDEX_BAD].GDBRA_ulValue;
    regctx.REG_ulCP0Cause    = pregset->regArr[GDB_REG_INDEX_CAUSE].GDBRA_ulValue;
    regctx.REG_ulCP0Epc      = pregset->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

#if LW_CFG_CPU_WORD_LENGHT == 64
#if LW_CFG_CPU_FPU_EN > 0
    if (regctx.REG_ulCP0Status & ST0_CU1) {
        ARCH_FPU_CTX    fpuctx;

        fpuctx.FPUCTX_reg[ 0].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 0)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 1].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 1)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 2].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 2)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 3].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 3)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 4].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 4)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 5].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 5)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 6].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 6)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 7].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 7)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 8].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 8)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[ 9].val64[0] = pregset->regArr[GDB_REG_INDEX_FP( 9)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[10].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(10)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[11].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(11)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[12].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(12)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[13].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(13)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[14].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(14)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[15].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(15)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[16].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(16)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[17].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(17)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[18].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(18)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[19].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(19)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[20].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(20)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[21].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(21)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[22].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(22)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[23].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(23)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[24].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(24)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[25].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(25)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[26].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(26)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[27].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(27)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[28].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(28)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[29].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(29)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[30].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(30)].GDBRA_ulValue;
        fpuctx.FPUCTX_reg[31].val64[0] = pregset->regArr[GDB_REG_INDEX_FP(31)].GDBRA_ulValue;
        fpuctx.FPUCTX_uiFcsr           = pregset->regArr[GDB_REG_INDEX_FCSR].GDBRA_ulValue;

        API_DtraceSetFpuRegs(pvDtrace, ulThread, &fpuctx);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if (LW_CFG_CPU_DSP_EN > 0) && defined(_MIPS_ARCH_HR2)
    if (regctx.REG_ulCP0Status & ST0_CU2) {
        ARCH_DSP_CTX    dspctx;

        lib_memcpy(&dspctx,
                   &pregset->regArr[GDB_REG_INDEX_HR2_V(0, 0)].GDBRA_ulValue,
                   sizeof(ARCH_DSP_CTX));

        API_DtraceSetDspRegs(pvDtrace, ulThread, &dspctx);
    }
#endif                                                                  /*  DSP_EN & _MIPS_ARCH_HR2     */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/

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
INT  archGdbRegSetPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, ULONG ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_ulCP0Epc = (ARCH_REG_T)ulPc;

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
ULONG archGdbRegGetPc (GDB_REG_SET *pRegs)
{
    return  (pRegs->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue);
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
    INT                 rsVal;
    INT                 rtVal;
    INT                 disp;
    UINT                fccN;
    MIPS_INSTRUCTION    machInstr;
    ARCH_REG_T          npc;
    ARCH_REG_T          pc;
    ARCH_REG_CTX        regctx;
    ARCH_REG_T          regSp;
#if LW_CFG_CPU_FPU_EN > 0
    ARCH_FPU_CTX        fpuctx;
    UINT32              uiFcsr;

#define __GET_FPU_CTX(FscrDefault)                              \
    if (regctx.REG_ulCP0Status & ST0_CU1) {                     \
        API_DtraceGetFpuRegs(pvDtrace, ulThread, &fpuctx);      \
        uiFcsr = fpuctx.FPUCTX_uiFcsr;                          \
    } else {                                                    \
        uiFcsr = FscrDefault;                                   \
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if (LW_CFG_CPU_DSP_EN > 0) && defined(_MIPS_ARCH_HR2)
    ARCH_DSP_CTX        dspctx;
    UINT32              uiVccr;

#define __GET_DSP_CTX(VccrDefault)                              \
    if (regctx.REG_ulCP0Status & ST0_CU2) {                     \
        API_DtraceGetDspRegs(pvDtrace, ulThread, &dspctx);      \
        uiVccr = dspctx.DSPCTX_hr2VectorCtx.HR2VECCTX_uiVccr;   \
    } else {                                                    \
        uiVccr = VccrDefault;                                   \
    }
#endif                                                                  /*  DSP_EN & _MIPS_ARCH_HR2     */

    pc = (ARCH_REG_T)pRegs->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue;     /*  ��ǰ PC ָ��                */
    /*
     * ��� PC Ϊ��֧��ʱ��ָ��������Ϊ branch ָ��
     */
    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);
    if (regctx.REG_ulCP0Cause & CAUSEF_BD) {
        pc -= sizeof(MIPS_INSTRUCTION);
    }
    machInstr = *(MIPS_INSTRUCTION *)pc;                                /*  ��ǰָ��                    */

    npc = pc + sizeof(MIPS_INSTRUCTION);                                /*  Ĭ��ֱ��ȡ��һ��ָ��        */

    /*
     * ��ǰָ��Ϊ��תָ��ʱ���������������������ת��������������һ��ָ��ΪĿ���ֵַ
     * ������һ��ָ��Ϊ PC + 8����Ϊ��תָ��ͽ������ķ�֧��ʱ��Ϊһ�����壬
     * ������Ϊһ��ָ���
     */
    rsVal = pRegs->regArr[(machInstr >> 21) & 0x1f].GDBRA_ulValue;
    rtVal = pRegs->regArr[(machInstr >> 16) & 0x1f].GDBRA_ulValue;
    disp  = ((INT)((machInstr & 0x0000ffff) << 16)) >> 14;
    fccN  = ((machInstr >> 18) & 0x07);
    if (fccN == 0) {
        fccN = 23;
    } else {
        fccN += 24;
    }

    if (((machInstr & 0xfc1f0000) == 0x04130000) ||                     /*  BGEZALL                     */
        ((machInstr & 0xfc1f0000) == 0x04030000)) {                     /*  BGEZL                       */
        if (rsVal >= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc1f0000) == 0x5c000000) {                /*  BGTZL                       */
        if (rsVal > 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc1f0000) == 0x58000000) {                /*  BLEZL                       */
        if (rsVal <= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if (((machInstr & 0xfc1f0000) == 0x04120000) ||              /*  BLTZALL                     */
               ((machInstr & 0xfc1f0000) == 0x04020000)) {              /*  BLTZL                       */
        if (rsVal < 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc000000) == 0x50000000) {                /*  BEQL                        */
        if (rsVal == rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc000000) == 0x54000000) {                /*  BNEL                        */
        if (rsVal != rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if (((machInstr & 0xfc000000) == 0x08000000) ||              /*  J                           */
               ((machInstr & 0xfc000000) == 0x0c000000)) {              /*  JAL                         */
        npc = ((machInstr & 0x03ffffff) << 2) | (pc & 0xf0000000);

    } else if (((machInstr & 0xfc1f07ff) == 0x00000009) ||              /*  JALR                        */
               ((machInstr & 0xfc1fffff) == 0x00000008)) {              /*  JR                          */
        npc = pRegs->regArr[(machInstr >> 21) & 0x1f].GDBRA_ulValue;

    } else if ((machInstr & 0xf3e30000) == 0x41000000) {                /*  BCzF                        */
        INT  copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        if (copId == 1) {                                               /*  0:bc0* 1:bc1* 2:bc2*        */
#if LW_CFG_CPU_FPU_EN > 0
            __GET_FPU_CTX((1 << fccN));
            if (!(uiFcsr & (1 << fccN))) {
                npc = disp + pc + 4;
            }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        }

    } else if ((machInstr & 0xf3e30000) == 0x41010000) {                /*  BCzT                        */
        INT  copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        if (copId == 1) {                                               /*  0:bc0* 1:bc1* 2:bc2*        */
#if LW_CFG_CPU_FPU_EN > 0
            __GET_FPU_CTX(~(1 << fccN));
            if (uiFcsr & (1 << fccN)) {
                npc = disp + pc + 4;
            }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        }

    } else if ((machInstr & 0xf3e30000) == 0x41020000) {                /*  BCzFL                       */
        INT  copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        if (copId == 1) {                                               /*  0:bc0* 1:bc1* 2:bc2*        */
#if LW_CFG_CPU_FPU_EN > 0
            __GET_FPU_CTX((1 << fccN));
            if (!(uiFcsr & (1 << fccN))) {
                npc = disp + pc + 4;
            }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        }

    } else if ((machInstr & 0xf3ff0000) == 0x41030000) {                /*  BCzTL                       */
        INT  copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        if (copId == 1) {                                               /*  0:bc0* 1:bc1* 2:bc2*        */
#if LW_CFG_CPU_FPU_EN > 0
            __GET_FPU_CTX(~(1 << fccN));
            if (uiFcsr & (1 << fccN)) {
                npc = disp + pc + 4;
            }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        }

    } else if ((machInstr & 0xfc000000) == 0x10000000) {                /*  BEQ                         */
        if (rsVal == rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if (((machInstr & 0xfc1f0000) == 0x04010000) ||              /*  BGEZ                        */
               ((machInstr & 0xfc1f0000) == 0x04110000)) {              /*  BGEZAL                      */
        if (rsVal >= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc1f0000) == 0x1c000000) {                /*  BGTZ                        */
        if (rsVal > 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc1f0000) == 0x18000000) {                /*  BLEZ                        */
        if (rsVal <= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if (((machInstr & 0xfc1f0000) == 0x04000000) ||              /*  BLTZ                        */
               ((machInstr & 0xfc1f0000) == 0x04100000)) {              /*  BLTZAL                      */
        if (rsVal < 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xfc000000) == 0x14000000) {                /*  BNE                         */
        if (rsVal != rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

#if (LW_CFG_CPU_DSP_EN > 0) && defined(_MIPS_ARCH_HR2)
    } else if ((machInstr & 0xffff0000) == 0xe8000000) {                /*  BC2F vcc Ϊ false ʱ��֧    */
        __GET_DSP_CTX(1);
        if (!uiVccr) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }

    } else if ((machInstr & 0xffff0000) == 0xe8010000) {                /*  BC2T vcc Ϊ true ʱ��֧     */
        __GET_DSP_CTX(0);
        if (uiVccr) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
#endif                                                                  /*  DSP_EN & _MIPS_ARCH_HR2     */
    } else {
                                                                        /*  ��ָͨ��                    */
    }
    return  (npc);
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
    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
