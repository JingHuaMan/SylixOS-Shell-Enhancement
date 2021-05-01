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
** ��   ��   ��: x86Gdb.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ���� GDB ���Խӿ�.
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
    "<?xml version=\"1.0\"?>"
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
      "<reg name=\"eax\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ecx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"edx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ebx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"esp\" bitsize=\"32\" type=\"data_ptr\"/>"
      "<reg name=\"ebp\" bitsize=\"32\" type=\"data_ptr\"/>"
      "<reg name=\"esi\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"edi\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"eip\" bitsize=\"32\" type=\"code_ptr\"/>"
      "<reg name=\"eflags\" bitsize=\"32\" type=\"i386_eflags\"/>"
      "<reg name=\"cs\" bitsize=\"32\" type=\"int32\"/>"
    "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cX86TargetXml[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  �Ĵ����������е�����
*********************************************************************************************************/
#define GDB_X86_EAX_INDEX       0
#define GDB_X86_ECX_INDEX       1
#define GDB_X86_EDX_INDEX       2
#define GDB_X86_EBX_INDEX       3
#define GDB_X86_ESP_INDEX       4
#define GDB_X86_EBP_INDEX       5
#define GDB_X86_ESI_INDEX       6
#define GDB_X86_EDI_INDEX       7
#define GDB_X86_EIP_INDEX       8
#define GDB_X86_EFLAGS_INDEX    9
#define GDB_X86_CS_INDEX        10
#define GDB_X86_REG_NR          11
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

    pregset->GDBR_iRegCnt = GDB_X86_REG_NR;

    pregset->regArr[GDB_X86_EAX_INDEX].GDBRA_ulValue    = regctx.REG_uiEAX;
    pregset->regArr[GDB_X86_ECX_INDEX].GDBRA_ulValue    = regctx.REG_uiECX;
    pregset->regArr[GDB_X86_EDX_INDEX].GDBRA_ulValue    = regctx.REG_uiEDX;
    pregset->regArr[GDB_X86_EBX_INDEX].GDBRA_ulValue    = regctx.REG_uiEBX;
    pregset->regArr[GDB_X86_ESP_INDEX].GDBRA_ulValue    = regSp;
    pregset->regArr[GDB_X86_EBP_INDEX].GDBRA_ulValue    = regctx.REG_uiEBP;
    pregset->regArr[GDB_X86_ESI_INDEX].GDBRA_ulValue    = regctx.REG_uiESI;
    pregset->regArr[GDB_X86_EDI_INDEX].GDBRA_ulValue    = regctx.REG_uiEDI;
    pregset->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue    = regctx.REG_uiEIP;

    pregset->regArr[GDB_X86_EFLAGS_INDEX].GDBRA_ulValue = regctx.REG_uiEFLAGS;
    pregset->regArr[GDB_X86_CS_INDEX].GDBRA_ulValue     = regctx.REG_uiCS;

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

    regctx.REG_uiEAX    = pregset->regArr[GDB_X86_EAX_INDEX].GDBRA_ulValue;
    regctx.REG_uiECX    = pregset->regArr[GDB_X86_ECX_INDEX].GDBRA_ulValue;
    regctx.REG_uiEDX    = pregset->regArr[GDB_X86_EDX_INDEX].GDBRA_ulValue;
    regctx.REG_uiEBX    = pregset->regArr[GDB_X86_EBX_INDEX].GDBRA_ulValue;

    regctx.REG_uiEBP    = pregset->regArr[GDB_X86_EBP_INDEX].GDBRA_ulValue;
    regctx.REG_uiESI    = pregset->regArr[GDB_X86_ESI_INDEX].GDBRA_ulValue;
    regctx.REG_uiEDI    = pregset->regArr[GDB_X86_EDI_INDEX].GDBRA_ulValue;
    regctx.REG_uiEIP    = pregset->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue;

    regctx.REG_uiEFLAGS = pregset->regArr[GDB_X86_EFLAGS_INDEX].GDBRA_ulValue;
    regctx.REG_uiCS     = pregset->regArr[GDB_X86_CS_INDEX].GDBRA_ulValue;

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

    regctx.REG_uiEIP = ulPc;

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
    return  (pRegs->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue);
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
    /*
     * x86 ����ʱ������ push %ebp (0x55) �� mov %esp %ebp (0x89) ָ��,
     * ��Ϊ��ͣ��������ָ��ʱ���ܵ���gdb��ջ����.
     */
    if ((*(UINT8 *)ulAddr == 0x55) || (*(UINT8 *)ulAddr == 0x89)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
