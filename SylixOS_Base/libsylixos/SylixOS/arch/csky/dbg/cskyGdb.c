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
** ��   ��   ��: cskyGdb.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ�ܹ� GDB ���Խӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../csky_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cCskyCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2018 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.csky.abiv2\">"
          "<reg name=\"r0\" bitsize=\"32\" regnum=\"0\"/>"
          "<reg name=\"r1\" bitsize=\"32\"/>"
          "<reg name=\"r2\" bitsize=\"32\"/>"
          "<reg name=\"r3\" bitsize=\"32\"/>"
          "<reg name=\"r4\" bitsize=\"32\"/>"
          "<reg name=\"r5\" bitsize=\"32\"/>"
          "<reg name=\"r6\" bitsize=\"32\"/>"
          "<reg name=\"r7\" bitsize=\"32\"/>"
          "<reg name=\"r8\" bitsize=\"32\"/>"
          "<reg name=\"r9\" bitsize=\"32\"/>"
          "<reg name=\"r10\" bitsize=\"32\"/>"
          "<reg name=\"r11\" bitsize=\"32\"/>"
          "<reg name=\"r12\" bitsize=\"32\"/>"
          "<reg name=\"r13\" bitsize=\"32\"/>"
          "<reg name=\"r14\" bitsize=\"32\"/>"
          "<reg name=\"r15\" bitsize=\"32\"/>"
          "<reg name=\"r16\" bitsize=\"32\"/>"
          "<reg name=\"r17\" bitsize=\"32\"/>"
          "<reg name=\"r18\" bitsize=\"32\"/>"
          "<reg name=\"r19\" bitsize=\"32\"/>"
          "<reg name=\"r20\" bitsize=\"32\"/>"
          "<reg name=\"r21\" bitsize=\"32\"/>"
          "<reg name=\"r22\" bitsize=\"32\"/>"
          "<reg name=\"r23\" bitsize=\"32\"/>"
          "<reg name=\"r24\" bitsize=\"32\"/>"
          "<reg name=\"r25\" bitsize=\"32\"/>"
          "<reg name=\"r26\" bitsize=\"32\"/>"
          "<reg name=\"r27\" bitsize=\"32\"/>"
          "<reg name=\"r28\" bitsize=\"32\"/>"
          "<reg name=\"r29\" bitsize=\"32\"/>"
          "<reg name=\"r30\" bitsize=\"32\"/>"
          "<reg name=\"r31\" bitsize=\"32\"/>"

          "<reg name=\"hi\" bitsize=\"32\" regnum=\"36\"/>"
          "<reg name=\"lo\" bitsize=\"32\"/>"

          "<reg name=\"fr0\"  bitsize=\"64\" regnum=\"40\"/> "
          "<reg name=\"fr1\"  bitsize=\"64\"/>"
          "<reg name=\"fr2\"  bitsize=\"64\"/>"
          "<reg name=\"fr3\"  bitsize=\"64\"/>"
          "<reg name=\"fr4\"  bitsize=\"64\"/>"
          "<reg name=\"fr5\"  bitsize=\"64\"/>"
          "<reg name=\"fr6\"  bitsize=\"64\"/>"
          "<reg name=\"fr7\"  bitsize=\"64\"/>"
          "<reg name=\"fr8\"  bitsize=\"64\"/>"
          "<reg name=\"fr9\"  bitsize=\"64\"/>"
          "<reg name=\"fr10\" bitsize=\"64\"/>"
          "<reg name=\"fr11\" bitsize=\"64\"/>"
          "<reg name=\"fr12\" bitsize=\"64\"/>"
          "<reg name=\"fr13\" bitsize=\"64\"/>"
          "<reg name=\"fr14\" bitsize=\"64\"/>"
          "<reg name=\"fr15\" bitsize=\"64\"/>"

          "<reg name=\"vr0\"  bitsize=\"128\" regnum=\"56\"/>"
          "<reg name=\"vr1\"  bitsize=\"128\"/>"
          "<reg name=\"vr2\"  bitsize=\"128\"/>"
          "<reg name=\"vr3\"  bitsize=\"128\"/>"
          "<reg name=\"vr4\"  bitsize=\"128\"/>"
          "<reg name=\"vr5\"  bitsize=\"128\"/>"
          "<reg name=\"vr6\"  bitsize=\"128\"/>"
          "<reg name=\"vr7\"  bitsize=\"128\"/>"
          "<reg name=\"vr8\"  bitsize=\"128\"/>"
          "<reg name=\"vr9\"  bitsize=\"128\"/>"
          "<reg name=\"vr10\" bitsize=\"128\"/>"
          "<reg name=\"vr11\" bitsize=\"128\"/>"
          "<reg name=\"vr12\" bitsize=\"128\"/>"
          "<reg name=\"vr13\" bitsize=\"128\"/>"
          "<reg name=\"vr14\" bitsize=\"128\"/>"
          "<reg name=\"vr15\" bitsize=\"128\"/>"

          "<reg name=\"pc\" bitsize=\"32\" regnum=\"72\"/>"
          "<reg name=\"psr\" bitsize=\"32\" regnum=\"89\"/>"
          "<reg name=\"fcr\" bitsize=\"32\" regnum=\"121\"/>"
          "<reg name=\"fid\" bitsize=\"32\"/>"
          "<reg name=\"fesr\" bitsize=\"32\"/>"
          "<reg name=\"usrsp\" bitsize=\"32\" regnum=\"127\"/>"
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
#define GDB_REG_INDEX_PSR       89                                      /*  ������״̬�Ĵ���            */
#define GDB_REG_INDEX_LO        37                                      /*  ������λ�Ĵ���              */
#define GDB_REG_INDEX_HI        36                                      /*  ������λ�Ĵ���              */
#define GDB_REG_INDEX_PC        72                                      /*  ����������Ĵ���            */
#define GDB_REG_INDEX_FP(n)     (40 + (n))                              /*  32 ���������ݼĴ���         */
#define GDB_REG_INDEX_FPCR      (121)                                   /*  ������ƼĴ���              */
#define GDB_REG_INDEX_FPESR     (123)                                   /*  �쳣�Ĵ���                  */
#define GDB_REG_NR              (128)                                   /*  �Ĵ�������                  */
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
    return  (cCskyCore);
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
    ARCH_FPU_CTX  fpuctx;
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);
    API_DtraceGetFpuRegs(pvDtrace, ulThread, &fpuctx);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->regArr[GDB_REG_INDEX_GREG( 0)].GDBRA_ulValue = regctx.REG_ulReg[ 0];
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
    pregset->regArr[GDB_REG_INDEX_GREG(29)].GDBRA_ulValue = regctx.REG_ulReg[29];
    pregset->regArr[GDB_REG_INDEX_GREG(30)].GDBRA_ulValue = regctx.REG_ulReg[30];
    pregset->regArr[GDB_REG_INDEX_GREG(31)].GDBRA_ulValue = regctx.REG_ulReg[31];
    pregset->regArr[GDB_REG_INDEX_PSR].GDBRA_ulValue      = regctx.REG_ulPsr;
    pregset->regArr[GDB_REG_INDEX_LO].GDBRA_ulValue       = regctx.REG_ulLo;
    pregset->regArr[GDB_REG_INDEX_HI].GDBRA_ulValue       = regctx.REG_ulHi;
    pregset->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue       = regctx.REG_ulPc;

    pregset->regArr[GDB_REG_INDEX_FP( 0)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 0].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP( 1)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 0].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP( 2)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 1].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP( 3)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 1].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP( 4)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 2].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP( 5)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 2].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP( 6)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 3].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP( 7)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 3].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP( 8)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 4].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP( 9)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 4].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(10)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 5].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(11)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 5].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(12)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 6].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(13)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 6].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(14)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 7].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(15)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 7].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(16)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 8].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(17)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 8].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(18)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 9].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(19)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[ 9].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(20)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[10].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(21)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[10].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(22)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[11].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(23)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[11].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(24)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[12].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(25)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[12].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(26)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[13].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(27)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[13].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(28)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[14].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(29)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[14].val32[1];
    pregset->regArr[GDB_REG_INDEX_FP(30)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[15].val32[0];
    pregset->regArr[GDB_REG_INDEX_FP(31)].GDBRA_ulValue  = fpuctx.FPUCTX_uiDreg[15].val32[1];
    pregset->regArr[GDB_REG_INDEX_FPCR].GDBRA_ulValue    = fpuctx.FPUCTX_uiFpcr;
    pregset->regArr[GDB_REG_INDEX_FPESR].GDBRA_ulValue   = fpuctx.FPUCTX_uiFpesr;

    API_DtraceSetFpuRegs(pvDtrace, ulThread, &fpuctx);

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
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{   
    ARCH_REG_CTX  regctx;
    ARCH_FPU_CTX  fpuctx;

    regctx.REG_ulReg[ 0] = pregset->regArr[GDB_REG_INDEX_GREG( 0)].GDBRA_ulValue;
    regctx.REG_ulReg[ 1] = pregset->regArr[GDB_REG_INDEX_GREG( 1)].GDBRA_ulValue;
    regctx.REG_ulReg[ 2] = pregset->regArr[GDB_REG_INDEX_GREG( 2)].GDBRA_ulValue;
    regctx.REG_ulReg[ 3] = pregset->regArr[GDB_REG_INDEX_GREG( 3)].GDBRA_ulValue;
    regctx.REG_ulReg[ 4] = pregset->regArr[GDB_REG_INDEX_GREG( 4)].GDBRA_ulValue;
    regctx.REG_ulReg[ 5] = pregset->regArr[GDB_REG_INDEX_GREG( 5)].GDBRA_ulValue;
    regctx.REG_ulReg[ 6] = pregset->regArr[GDB_REG_INDEX_GREG( 6)].GDBRA_ulValue;
    regctx.REG_ulReg[ 7] = pregset->regArr[GDB_REG_INDEX_GREG( 7)].GDBRA_ulValue;
    regctx.REG_ulReg[ 8] = pregset->regArr[GDB_REG_INDEX_GREG( 8)].GDBRA_ulValue;
    regctx.REG_ulReg[ 9] = pregset->regArr[GDB_REG_INDEX_GREG( 9)].GDBRA_ulValue;
    regctx.REG_ulReg[10] = pregset->regArr[GDB_REG_INDEX_GREG(10)].GDBRA_ulValue;
    regctx.REG_ulReg[11] = pregset->regArr[GDB_REG_INDEX_GREG(11)].GDBRA_ulValue;
    regctx.REG_ulReg[12] = pregset->regArr[GDB_REG_INDEX_GREG(12)].GDBRA_ulValue;
    regctx.REG_ulReg[13] = pregset->regArr[GDB_REG_INDEX_GREG(13)].GDBRA_ulValue;
    regctx.REG_ulReg[14] = pregset->regArr[GDB_REG_INDEX_GREG(14)].GDBRA_ulValue;
    regctx.REG_ulReg[15] = pregset->regArr[GDB_REG_INDEX_GREG(15)].GDBRA_ulValue;
    regctx.REG_ulReg[16] = pregset->regArr[GDB_REG_INDEX_GREG(16)].GDBRA_ulValue;
    regctx.REG_ulReg[17] = pregset->regArr[GDB_REG_INDEX_GREG(17)].GDBRA_ulValue;
    regctx.REG_ulReg[18] = pregset->regArr[GDB_REG_INDEX_GREG(18)].GDBRA_ulValue;
    regctx.REG_ulReg[19] = pregset->regArr[GDB_REG_INDEX_GREG(19)].GDBRA_ulValue;
    regctx.REG_ulReg[20] = pregset->regArr[GDB_REG_INDEX_GREG(20)].GDBRA_ulValue;
    regctx.REG_ulReg[21] = pregset->regArr[GDB_REG_INDEX_GREG(21)].GDBRA_ulValue;
    regctx.REG_ulReg[22] = pregset->regArr[GDB_REG_INDEX_GREG(22)].GDBRA_ulValue;
    regctx.REG_ulReg[23] = pregset->regArr[GDB_REG_INDEX_GREG(23)].GDBRA_ulValue;
    regctx.REG_ulReg[24] = pregset->regArr[GDB_REG_INDEX_GREG(24)].GDBRA_ulValue;
    regctx.REG_ulReg[25] = pregset->regArr[GDB_REG_INDEX_GREG(25)].GDBRA_ulValue;
    regctx.REG_ulReg[26] = pregset->regArr[GDB_REG_INDEX_GREG(26)].GDBRA_ulValue;
    regctx.REG_ulReg[27] = pregset->regArr[GDB_REG_INDEX_GREG(27)].GDBRA_ulValue;
    regctx.REG_ulReg[28] = pregset->regArr[GDB_REG_INDEX_GREG(28)].GDBRA_ulValue;
    regctx.REG_ulReg[29] = pregset->regArr[GDB_REG_INDEX_GREG(29)].GDBRA_ulValue;
    regctx.REG_ulReg[30] = pregset->regArr[GDB_REG_INDEX_GREG(30)].GDBRA_ulValue;
    regctx.REG_ulReg[31] = pregset->regArr[GDB_REG_INDEX_GREG(31)].GDBRA_ulValue;
    regctx.REG_ulPsr     = pregset->regArr[GDB_REG_INDEX_PSR].GDBRA_ulValue;
    regctx.REG_ulLo      = pregset->regArr[GDB_REG_INDEX_LO].GDBRA_ulValue;
    regctx.REG_ulHi      = pregset->regArr[GDB_REG_INDEX_HI].GDBRA_ulValue;
    regctx.REG_ulPc      = pregset->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue;
  
    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    fpuctx.FPUCTX_uiDreg[ 0].val32[0] = pregset->regArr[GDB_REG_INDEX_FP( 0)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 0].val32[1] = pregset->regArr[GDB_REG_INDEX_FP( 1)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 1].val32[0] = pregset->regArr[GDB_REG_INDEX_FP( 2)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 1].val32[1] = pregset->regArr[GDB_REG_INDEX_FP( 3)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 2].val32[0] = pregset->regArr[GDB_REG_INDEX_FP( 4)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 2].val32[1] = pregset->regArr[GDB_REG_INDEX_FP( 5)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 3].val32[0] = pregset->regArr[GDB_REG_INDEX_FP( 6)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 3].val32[1] = pregset->regArr[GDB_REG_INDEX_FP( 7)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 4].val32[0] = pregset->regArr[GDB_REG_INDEX_FP( 8)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 4].val32[1] = pregset->regArr[GDB_REG_INDEX_FP( 9)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 5].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(10)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 5].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(11)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 6].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(12)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 6].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(13)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 7].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(14)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 7].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(15)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 8].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(16)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 8].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(17)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 9].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(18)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[ 9].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(19)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[10].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(20)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[10].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(21)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[11].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(22)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[11].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(23)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[12].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(24)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[12].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(25)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[13].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(26)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[13].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(27)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[14].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(28)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[14].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(29)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[15].val32[0] = pregset->regArr[GDB_REG_INDEX_FP(30)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiDreg[15].val32[1] = pregset->regArr[GDB_REG_INDEX_FP(31)].GDBRA_ulValue;
    fpuctx.FPUCTX_uiFpcr              = pregset->regArr[GDB_REG_INDEX_FPCR].GDBRA_ulValue;
    fpuctx.FPUCTX_uiFpesr             = pregset->regArr[GDB_REG_INDEX_FPESR].GDBRA_ulValue;

    API_DtraceSetFpuRegs(pvDtrace, ulThread, &fpuctx);

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
ULONG  archGdbGetNextPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pRegs)
{
    ARCH_REG_T  uiPc;
    ULONG       ulNextPc;
    UINT32      uiIns;
    INT         iInsnLen;
    INT         iRsVal;
    INT         iOffset;
    INT         iC;

    uiPc = (ARCH_REG_T)pRegs->regArr[GDB_REG_INDEX_PC].GDBRA_ulValue;   /*  ��ǰ PC ָ��                */
    iC   = pRegs->regArr[GDB_REG_INDEX_PSR].GDBRA_ulValue & 0x1;

    if (IS_T32(*(UINT16 *)uiPc)) {
        uiIns    = ((*(UINT16 *)uiPc) << 16) | *(UINT16 *)(uiPc + 2);
        iInsnLen = 4;

    } else {
        uiIns    = *(UINT16 *)uiPc;
        iInsnLen = 2;
    }

    ulNextPc = uiPc + iInsnLen;

    if (iInsnLen == 4) {
        iRsVal  = pRegs->regArr[(uiIns >> 16) & 0x1f].GDBRA_ulValue;
        iOffset = (INT16)uiIns;

        if ((uiIns & 0xffe00000) == 0xe9000000) {                       /*  BEZ                         */
            if (iRsVal == 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffe00000) == 0xe9a00000) {                /*  BHSZ                        */
            if (iRsVal >= 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffe00000) == 0xe9400000) {                /*  BHZ                         */
            if (iRsVal > 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffe00000) == 0xe9600000) {                /*  BLSZ                        */
            if (iRsVal <= 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffe00000) == 0xe9800000) {                /*  BLZ                         */
            if (iRsVal < 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffe00000) == 0xe9200000) {                /*  BNEZ                        */
            if (iRsVal != 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }
        } else if ((uiIns & 0xffff0000) == 0xe8600000) {                /*  BT                          */
            if (iC == 1) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffff0000) == 0xe8400000) {                /*  BF                          */
            if (iC == 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 4;
            }

        } else if ((uiIns & 0xffff0000) == 0xe8000000) {                /*  BR                          */
            ulNextPc = uiPc + (iOffset << 1);

        } else if ((uiIns & 0xfc000000) == 0xe0000000) {                /*  BSR                         */
            iOffset = uiIns & 0x3ffffff;
            if (iOffset & (1 << 25)) {
                iOffset |= 0xfc000000;
            }
            ulNextPc = uiPc + (iOffset << 1);

        } else if ((uiIns & 0xffffffff) == 0xe8cf0000) {                /*  RTS                         */
            ulNextPc = pRegs->regArr[15].GDBRA_ulValue & 0xfffffffe;

        } else if (((uiIns & 0xffe0ffff) == 0xe8c00000) ||              /*  JMP                         */
                   ((uiIns & 0xffe0ffff) == 0xe8e00000)) {              /*  JSR                         */
            ulNextPc = iRsVal & 0xfffffffe;

        } else if (((uiIns & 0xffff0000) == 0xeac00000) ||              /*  JMPI                        */
                   ((uiIns & 0xffff0000) == 0xeae00000)) {              /*  JSRI                        */
            iOffset  = uiIns & 0xffff;
            ulNextPc = *(ULONG *)((uiPc + (iOffset << 2)) & 0xfffffffc);
        }

    } else {
        iOffset = uiIns & 0x3ff;

        if (iOffset & (1 << 9)) {
            iOffset |= 0xfffffc00;
        }

        if ((uiIns & 0xfc00) == 0x0800) {                               /*  BT                          */
            if (iC == 1) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 2;
            }

        } else if ((uiIns & 0xfc00) == 0x0c00) {                        /*  BF                          */
            if (iC == 0) {
                ulNextPc = uiPc + (iOffset << 1);
            } else {
                ulNextPc = uiPc + 2;
            }

        } else if (((uiIns & 0xfc00) == 0x0000) ||                      /*  BSR                         */
                   ((uiIns & 0xfc00) == 0x0400)) {                      /*  BR                          */
            ulNextPc = uiPc + (iOffset << 1);

        } else if ((uiIns & 0xffff) == 0x783c) {                        /*  RTS                         */
            ulNextPc = pRegs->regArr[15].GDBRA_ulValue & 0xfffffffe;

        } else if (((uiIns & 0xffc3) == 0x7800)||                       /*  JMP                         */
                   ((uiIns & 0xffc3) == 0x7bc1)) {                      /*  JSR                         */
            iRsVal   = pRegs->regArr[(uiIns >> 2) & 0xf].GDBRA_ulValue;
            ulNextPc = iRsVal & 0xfffffffe;
        }
    }

    if (API_VmmVirtualIsInside(ulNextPc)) {
        return  (ulNextPc);

    } else {
        return  (uiPc + iInsnLen);
    }
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
