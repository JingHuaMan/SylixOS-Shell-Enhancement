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
** ��   ��   ��: sparcGdb.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2017 �� 10 �� 16 ��
**
** ��        ��: SPARC ��ϵ���� GDB ���Խӿ�.
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
#include "arch/sparc/backtrace/sparcBacktrace.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cSparcCoreXml[] = \
    "<?xml version=\"1.0\"?>"
    "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
         "Copying and distribution of this file, with or without modification,"
         "are permitted in any medium without royalty provided the copyright"
         "notice and this notice are preserved.  -->"
    "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
    "<feature name=\"org.gnu.gdb.sparc.core\">"
    "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   _G_cSparcTargetXml[] = \
    "l<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
        "<xi:include href=\"arch-core.xml\"/>"
    "</target>";
/*********************************************************************************************************
  �Ĵ����������е�����
*********************************************************************************************************/
#define GDB_SPARC_PC_INDEX      68
#define GDB_SPARC_NPC_INDEX     69
#define GDB_SPARC_REG_NR        72
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
    return  (_G_cSparcTargetXml);
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
    return  (_G_cSparcCoreXml);
}
/*********************************************************************************************************
** ��������: __archBackTracePc
** ��������: ���ݲ����ص���ջ�е�һ�������ں˵�ַ�ռ��е�ջ֡���ص�ַ
** �䡡��  : ulFp           FP �Ĵ���ֵ
**           ulI7           I7 �Ĵ���ֵ
**           ulPc           PC �Ĵ���ֵ
** �䡡��  : ��һ�������ں˵�ַ�ռ��е�ջ֡���ص�ַ����� PC �����ں˵�ַ�ռ䣬�򷵻� PC
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __archBackTracePc (ULONG  ulFp, ULONG  ulI7, ULONG  ulPc)
{
    struct layout  *pCurrent;

    if (API_VmmVirtualIsInside(ulPc)) {
        return  (ulPc);
    }

    if (API_VmmVirtualIsInside(ulI7)) {
        return  (ulI7);
    }

    pCurrent = (struct layout *)(ulFp);
    if (pCurrent == LW_NULL) {
        return  (ulPc);
    }

    while (!API_VmmVirtualIsInside((addr_t)pCurrent->return_address)) {
        if (!pCurrent->next) {
            break;
        }

        pCurrent = (struct layout *)(pCurrent->next);
    }

    return  ((ULONG)pCurrent->return_address);
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
    INT           i;
    INT           iIndex = 0;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = GDB_SPARC_REG_NR;

    for (i = 0; i < 8; i++) {
        pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiGlobal[i];
    }

    for (i = 0; i < 8; i++) {
        pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiOutput[i];
    }

    for (i = 0; i < 8; i++) {
        pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiLocal[i];
    }

    for (i = 0; i < 8; i++) {
        pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiInput[i];
    }

    for (i = 0; i < 32; i++) {
        pregset->regArr[iIndex++].GDBRA_ulValue = 0;
    }

    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiY;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiPsr;
    iIndex++;
    iIndex++;

    /*
     * ��� PC ���ں˵�ַ�ռ䣬���ݲ�ȡ�õ���ջ�е�һ�������ں˵�ַ�ռ��е�ջ֡���ص�ַ��Ϊ PC��
     * ������ƭ gdb�������ڶ��̵߳���ʱ SPARC gdb ����ͼ���ں˿ռ��ڶϵ�
     */
    pregset->regArr[iIndex++].GDBRA_ulValue = __archBackTracePc(regctx.REG_uiFp,
                                                                regctx.REG_uiInput[7],
                                                                regctx.REG_uiPc);
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiNPc;

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
    INT           i;
    INT           iIndex = 0;

    lib_bzero(&regctx, sizeof(ARCH_REG_CTX));

    for (i = 0; i < 8; i++) {
        regctx.REG_uiGlobal[i] = pregset->regArr[iIndex++].GDBRA_ulValue;
    }

    for (i = 0; i < 8; i++) {
        regctx.REG_uiOutput[i] = pregset->regArr[iIndex++].GDBRA_ulValue;
    }

    for (i = 0; i < 8; i++) {
        regctx.REG_uiLocal[i] = pregset->regArr[iIndex++].GDBRA_ulValue;
    }

    for (i = 0; i < 8; i++) {
        regctx.REG_uiInput[i] = pregset->regArr[iIndex++].GDBRA_ulValue;
    }

    iIndex += 32;

    regctx.REG_uiY   = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiPsr = pregset->regArr[iIndex++].GDBRA_ulValue;
    iIndex++;
    iIndex++;

    regctx.REG_uiPc  = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiNPc = pregset->regArr[iIndex++].GDBRA_ulValue;

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

    regctx.REG_uiPc = ulPc;

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
ULONG  archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_SPARC_PC_INDEX].GDBRA_ulValue);
}
/*********************************************************************************************************
** ��������: archGdbGetNextPc
** ��������: ��ȡ��һ��ָ���ַ������֧Ԥ��
** �䡡��  : pRegs       �Ĵ�������
** �䡡��  : ��һ��ָ���ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_SPARC_NPC_INDEX].GDBRA_ulValue);
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
