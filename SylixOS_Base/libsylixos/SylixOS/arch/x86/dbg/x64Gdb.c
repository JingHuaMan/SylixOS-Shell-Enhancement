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
** ��   ��   ��: x64Gdb.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 07 ��
**
** ��        ��: x86-64 ��ϵ���� GDB ���Խӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "arch/arch_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cX86CoreXml[] = \
    "l<?xml version=\"1.0\"?>"
    "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
        "Copying and distribution of this file, with or without modification,"
        "are permitted in any medium without royalty provided the copyright"
        "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
    "<feature name=\"org.gnu.gdb.i386.core\">"
        "<flags id=\"i386_eflags\" size=\"4\">"
        "<field name=\"CF\" start=\"0\" end=\"0\"/>"
        "<field name=\"\" start=\"1\" end=\"1\"/>"
        "<field name=\"PF\" start=\"2\" end=\"2\"/>"
        "<field name=\"AF\" start=\"4\" end=\"4\"/>"
        "<field name=\"ZF\" start=\"6\" end=\"6\"/>"
        "<field name=\"SF\" start=\"7\" end=\"7\"/>"
        "<field name=\"TF\" start=\"8\" end=\"8\"/>"
        "<field name=\"IF\" start=\"9\" end=\"9\"/>"
        "<field name=\"DF\" start=\"10\" end=\"10\"/>"
        "<field name=\"OF\" start=\"11\" end=\"11\"/>"
        "<field name=\"NT\" start=\"14\" end=\"14\"/>"
        "<field name=\"RF\" start=\"16\" end=\"16\"/>"
        "<field name=\"VM\" start=\"17\" end=\"17\"/>"
        "<field name=\"AC\" start=\"18\" end=\"18\"/>"
        "<field name=\"VIF\" start=\"19\" end=\"19\"/>"
        "<field name=\"VIP\" start=\"20\" end=\"20\"/>"
        "<field name=\"ID\" start=\"21\" end=\"21\"/>"
      "</flags>"
      "<reg name=\"rax\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rbx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rcx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rdx\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rsi\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rdi\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rbp\" bitsize=\"64\" type=\"data_ptr\"/>"
      "<reg name=\"rsp\" bitsize=\"64\" type=\"data_ptr\"/>"
      "<reg name=\"r8\"  bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r9\"  bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r10\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r11\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r12\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r13\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r14\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"r15\" bitsize=\"64\" type=\"int64\"/>"
      "<reg name=\"rip\" bitsize=\"64\" type=\"code_ptr\"/>"
      "<reg name=\"eflags\" bitsize=\"32\" type=\"i386_eflags\"/>"
      "<reg name=\"cs\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ss\" bitsize=\"32\" type=\"int32\"/>"
    "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cX86TargetXml[] = \
    "l<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
        "<architecture>i386:x86-64</architecture>"
        "<xi:include href=\"arch-core.xml\"/>"
    "</target>";
/*********************************************************************************************************
  �Ĵ����������е�����
*********************************************************************************************************/
#define GDB_X64_RAX_INDEX       0
#define GDB_X64_RBX_INDEX       1
#define GDB_X64_RCX_INDEX       2
#define GDB_X64_RDX_INDEX       3
#define GDB_X64_RSI_INDEX       4
#define GDB_X64_RDI_INDEX       5
#define GDB_X64_RBP_INDEX       6
#define GDB_X64_RSP_INDEX       7
#define GDB_X64_R8_INDEX        8
#define GDB_X64_R9_INDEX        9
#define GDB_X64_R10_INDEX       10
#define GDB_X64_R11_INDEX       11
#define GDB_X64_R12_INDEX       12
#define GDB_X64_R13_INDEX       13
#define GDB_X64_R14_INDEX       14
#define GDB_X64_R15_INDEX       15
#define GDB_X64_RIP_INDEX       16
#define GDB_X64_EFLAGS_INDEX    17
#define GDB_X64_CS_INDEX        18
#define GDB_X64_SS_INDEX        19
#define GDB_X64_DS_INDEX        20
#define GDB_X64_ES_INDEX        21
#define GDB_X64_FS_INDEX        22
#define GDB_X64_GS_INDEX        23
#define GDB_X64_REG_NR          24
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
    return  (_G_cX86TargetXml);
}
/*********************************************************************************************************
** ��������: archGdbCoreXml
** ��������: ��� Xfer:features:read:arch-core.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (_G_cX86CoreXml);
}
/*********************************************************************************************************
** ��������: archGdbRegsGet
** ��������: ��ȡ�Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace �ڵ�
**           ulThread       �߳̾��
**           pregset        gdb �Ĵ����ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = GDB_X64_REG_NR;

    pregset->regArr[GDB_X64_RAX_INDEX].GDBRA_ulValue    = regctx.REG_ulRAX;
    pregset->regArr[GDB_X64_RBX_INDEX].GDBRA_ulValue    = regctx.REG_ulRBX;
    pregset->regArr[GDB_X64_RCX_INDEX].GDBRA_ulValue    = regctx.REG_ulRCX;
    pregset->regArr[GDB_X64_RDX_INDEX].GDBRA_ulValue    = regctx.REG_ulRDX;

    pregset->regArr[GDB_X64_RSI_INDEX].GDBRA_ulValue    = regctx.REG_ulRSI;
    pregset->regArr[GDB_X64_RDI_INDEX].GDBRA_ulValue    = regctx.REG_ulRDI;

    pregset->regArr[GDB_X64_RBP_INDEX].GDBRA_ulValue    = regctx.REG_ulRBP;
    pregset->regArr[GDB_X64_RSP_INDEX].GDBRA_ulValue    = regSp;

    pregset->regArr[GDB_X64_R8_INDEX].GDBRA_ulValue     = regctx.REG_ulR8;
    pregset->regArr[GDB_X64_R9_INDEX].GDBRA_ulValue     = regctx.REG_ulR9;
    pregset->regArr[GDB_X64_R10_INDEX].GDBRA_ulValue    = regctx.REG_ulR10;
    pregset->regArr[GDB_X64_R11_INDEX].GDBRA_ulValue    = regctx.REG_ulR11;
    pregset->regArr[GDB_X64_R12_INDEX].GDBRA_ulValue    = regctx.REG_ulR12;
    pregset->regArr[GDB_X64_R13_INDEX].GDBRA_ulValue    = regctx.REG_ulR13;
    pregset->regArr[GDB_X64_R14_INDEX].GDBRA_ulValue    = regctx.REG_ulR14;
    pregset->regArr[GDB_X64_R15_INDEX].GDBRA_ulValue    = regctx.REG_ulR15;

    pregset->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue    = regctx.REG_ulRIP;

    pregset->regArr[GDB_X64_EFLAGS_INDEX].GDBRA_ulValue = regctx.REG_ulRFLAGS;

    pregset->regArr[GDB_X64_CS_INDEX].GDBRA_ulValue     = regctx.REG_ulCS;
    pregset->regArr[GDB_X64_SS_INDEX].GDBRA_ulValue     = regctx.REG_ulSS;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegsSet
** ��������: ���üĴ���ֵ
** �䡡��  : pvDtrace       dtrace �ڵ�
**           ulThread       �߳̾��
**           pregset        gdb �Ĵ����ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;

    lib_bzero(&regctx, sizeof(ARCH_REG_CTX));

    regctx.REG_ulRAX    = pregset->regArr[GDB_X64_RAX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRBX    = pregset->regArr[GDB_X64_RBX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRCX    = pregset->regArr[GDB_X64_RCX_INDEX].GDBRA_ulValue;
    regctx.REG_ulRDX    = pregset->regArr[GDB_X64_RDX_INDEX].GDBRA_ulValue;

    regctx.REG_ulRSI    = pregset->regArr[GDB_X64_RSI_INDEX].GDBRA_ulValue;
    regctx.REG_ulRDI    = pregset->regArr[GDB_X64_RDI_INDEX].GDBRA_ulValue;

    regctx.REG_ulRBP    = pregset->regArr[GDB_X64_RBP_INDEX].GDBRA_ulValue;

    regctx.REG_ulR8     = pregset->regArr[GDB_X64_R8_INDEX].GDBRA_ulValue;
    regctx.REG_ulR9     = pregset->regArr[GDB_X64_R9_INDEX].GDBRA_ulValue;
    regctx.REG_ulR10    = pregset->regArr[GDB_X64_R10_INDEX].GDBRA_ulValue;
    regctx.REG_ulR11    = pregset->regArr[GDB_X64_R11_INDEX].GDBRA_ulValue;
    regctx.REG_ulR12    = pregset->regArr[GDB_X64_R12_INDEX].GDBRA_ulValue;
    regctx.REG_ulR13    = pregset->regArr[GDB_X64_R13_INDEX].GDBRA_ulValue;
    regctx.REG_ulR14    = pregset->regArr[GDB_X64_R14_INDEX].GDBRA_ulValue;
    regctx.REG_ulR15    = pregset->regArr[GDB_X64_R15_INDEX].GDBRA_ulValue;

    regctx.REG_ulRIP    = pregset->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue;

    regctx.REG_ulRFLAGS = pregset->regArr[GDB_X64_EFLAGS_INDEX].GDBRA_ulValue;

    regctx.REG_ulCS     = pregset->regArr[GDB_X64_CS_INDEX].GDBRA_ulValue;
    regctx.REG_ulSS     = pregset->regArr[GDB_X64_SS_INDEX].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegSetPc
** ��������: ���� PC �Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace �ڵ�
**           ulThread       �߳̾��
**           ulPC           PC �Ĵ���ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_ulRIP = ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegGetPc
** ��������: ��ȡ PC �Ĵ���ֵ
** �䡡��  : pRegs       �Ĵ�������
** �䡡��  : PC �Ĵ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_X64_RIP_INDEX].GDBRA_ulValue);
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
