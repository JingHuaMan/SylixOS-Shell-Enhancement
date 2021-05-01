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
** ��   ��   ��: c6xGdb.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2017 �� 09 �� 06 ��
**
** ��        ��: C6X ��ϵ���� GDB ���Խӿ�.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../c6x_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cC6xCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2017 ACOINFO co.,ltd."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.tic6x.core\">"
		"<reg name=\"A0\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A1\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A2\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A3\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A4\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A5\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A6\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A7\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A8\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A9\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A10\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A11\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A12\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A13\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A14\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A15\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B0\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B1\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B2\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B3\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B4\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B5\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B6\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B7\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B8\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B9\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B10\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B11\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B12\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B13\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B14\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B15\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"None\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"PC\" bitsize=\"32\" type=\"code_ptr\"/>"
		"<reg name=\"IRP\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"IFR\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"NPR\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A16\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A17\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A18\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A19\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A20\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A21\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A22\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A23\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A24\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A25\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A26\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A27\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A28\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A29\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A30\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"A31\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B16\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B17\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B18\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B19\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B20\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B21\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B22\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B23\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B24\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B25\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B26\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B27\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B28\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B29\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B30\" bitsize=\"32\" type=\"uint32\"/>"
		"<reg name=\"B31\" bitsize=\"32\" type=\"uint32\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cTargetSystem[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
		"<architecture>tic6x</architecture>"
		"<osabi>GNU/Linux</osabi>"
		"<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC ���ջ�Ĵ����� GDB_REG_SET �ṹ�е�����
*********************************************************************************************************/
#define C6X_REG_INDEX_PC        33
#define C6X_REG_INDEX_B15       31
/*********************************************************************************************************
  ָ�������غ궨��
*********************************************************************************************************/
#define TIC6X_OPCODE_SIZE       4                                       /* ָ����                     */
#define TIC6X_FETCH_PACKET_SIZE 32
#define INST_S_BIT(INST)        ((INST >>  1) & 1)
#define INST_X_BIT(INST)        ((INST >> 12) & 1)
/*********************************************************************************************************
  CPSR �Ĵ����� GDB_REG_SET �ṹ�е�����
*********************************************************************************************************/
#define C6X_REGS_NUMS           69
/*********************************************************************************************************
  �Ĵ���ӳ���
*********************************************************************************************************/
#if LW_CFG_CPU_ENDIAN == 1
static INT iRegMap[] = {
    /*
     * A0 - A15
     */
    77, 76, 73, 72, 15, 14, 69, 68,
    65, 65, 11, 10,  7,  6,  3,  2,

    /*
     * B0 - B15
     */
    80, 79, 75, 74, 71, 70, 67, 66,
    63, 62, 13, 12,  9,  8,  5,  4,

    /*
     * CSR PC IRP IFR NPR
     */
    16, 19, 19, -1, -1,

    /*
     * A16 - A31
     */
    61, 60, 57, 56, 53, 52, 49, 48,
    45, 44, 41, 40, 37, 36, 33, 32,

    /*
     * B16 -B31
     */
    59, 58, 55, 54, 51, 50, 47, 46,
    43, 42, 39, 38, 35, 34, 31, 30,
};

#elif LW_CFG_CPU_ENDIAN == 0
static INT iRegMap[] = {
    /*
     * A0 - A15
     */
    76, 77, 72, 73, 14, 15, 68, 69,
    64, 65, 10, 11,  6,  7,  2,  3,

    /*
     * B0 - B15
     */
    79, 80, 74, 75, 70, 71, 66, 67,
    62, 63, 12, 13,  8,  9,  4,  5,

    /*
     * CSR PC IRP IFR NPR
     */
    16, 19, 19, -1, -1,

    /*
     * A16 - A31
     */
    60, 61, 56, 57, 52, 53, 48, 49,
    44, 45, 40, 41, 36, 37, 32, 33,

    /*
     * B16 -B31
     */
    58, 59, 54, 55, 50, 51, 46, 47,
    42, 43, 38, 39, 34, 35, 30, 31,
};

#else
#error "TI Compiler does not specify endianness"
#endif
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
** ��������: ��� Xfer:features:read:arm-core.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (cC6xCore);
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
    ARCH_REG_CTX      regctx;
    ARCH_REG_T        regSp;
    INT               i = 0;
    ULONG            *pulRegVals;

    API_DtraceGetRegs(pvDtrace, ulThread, (ARCH_REG_CTX *)&regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = C6X_REGS_NUMS;
    pulRegVals = (ULONG *)&regctx;

    for (i = 0; i < pregset->GDBR_iRegCnt; i++) {
        if (iRegMap[i] != -1) {
            pregset->regArr[i].GDBRA_ulValue = pulRegVals[iRegMap[i]];
        }
    }

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
    ARCH_REG_CTX      regctx;
    INT               i = 0;
    ULONG            *pulRegVals;

    pregset->GDBR_iRegCnt = C6X_REGS_NUMS;
    pulRegVals = (ULONG *)&regctx;

    for (i = 0; i < pregset->GDBR_iRegCnt; i++) {
        if (iRegMap[i] != -1) {
            pulRegVals[iRegMap[i]] = pregset->regArr[i].GDBRA_ulValue;
        }
    }

    API_DtraceSetRegs(pvDtrace, ulThread, (const ARCH_REG_CTX *)&regctx);

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
    ARCH_REG_CTX      regctx;
    ARCH_REG_T        regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, (ARCH_REG_CTX *)&regctx, &regSp);

    regctx.REG_uiIrp = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, (const ARCH_REG_CTX *)&regctx);

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
ULONG  archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[C6X_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
  ���´���ʵ�ַ�֧Ԥ��
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: archTiC6xConditionTrue
** ��������: �ж�ָ��ִ�������Ƿ�����
** �䡡��  : pRegs          �Ĵ����б�
**           ulInst         ָ��
** �䡡��  : �������㷵��1�����򷵻�0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  archTiC6xConditionTrue (GDB_REG_SET  *pRegs, ULONG  ulInst)
{
    static const INT  iRegisterNumbers[8] = { -1, 16, 17, 18, 1, 2, 0, -1 };
                 INT  iRegisterNumber;
                 INT  iRegisterValue;

    iRegisterNumber = iRegisterNumbers[(ulInst >> 29) & 7];
    if (iRegisterNumber == -1) {
        return 1;
    }

    iRegisterValue = pRegs->regArr[iRegisterNumber].GDBRA_ulValue;
    if ((ulInst & 0x10000000) != 0) {
        return iRegisterValue == 0;
    }

    return  (iRegisterValue != 0);
}
/*********************************************************************************************************
** ��������: archTiC6xRegisterNumber
** ��������: ת���Ĵ�����Ϊgdb�еļĴ���ƫ��
** �䡡��  : iReg           �Ĵ�����
**           iSide          sideλ
**           iCrossPath     cλ
** �䡡��  : �Ĵ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  archTiC6xRegisterNumber (INT  iReg, INT  iSide, INT  iCrossPath)
{
    INT  iRet = (iReg & 15) | ((iCrossPath ^ iSide) << 4);
	
    if ((iReg & 16) != 0) {                                             /* A16 - A31, B16 - B31         */
        iRet += 37;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: archTiC6xExtractSignedField
** ��������: ��ȡָ���е�ָ��λ��
** �䡡��  : iValue         ָ��ֵ
**           iLowBit        ��ʼλ
**           pRegs          λ��
** �䡡��  : λ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  archTiC6xExtractSignedField (INT  iValue, INT  iLowBit, INT  iBits)
{
    INT  iMask = (1 << iBits) - 1;
    INT  iRet  = (iValue >> iLowBit) & iMask;
	
    if ((iRet & (1 << (iBits - 1))) != 0) {
        iRet -= iMask + 1;
    }

    return  (iRet);
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
    ULONG   ulNextPc;
    ULONG   uiInst;
    INT     iRegNum;

    do {
        uiInst = *((ULONG *)pRegs->regArr[C6X_REG_INDEX_PC].GDBRA_ulValue);

    	if (archTiC6xConditionTrue(pRegs, uiInst)) {
    	    if ((uiInst & 0x0000007c) == 0x00000010) {                  /* B with displacement          */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 21) << 2;
                break;
            }

    	    if ((uiInst & 0x0f83effc) == 0x00000360) {                  /* B with register              */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 18) & 0x1f,
                                                  INST_S_BIT (uiInst),
                                                  INST_X_BIT (uiInst));
                ulNextPc = pRegs->regArr[iRegNum].GDBRA_ulValue;
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00001020) {                  /* BDEC                         */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 23) & 0x1f,
                                                  INST_S_BIT (uiInst), 0);
                if (((INT)pRegs->regArr[iRegNum].GDBRA_ulValue) >= 0) {
                    ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                    ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 10) << 2;
                }
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00000120) {                  /* BNOP with displacement       */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 16, 12) << 2;
                break;
            }

            if ((uiInst & 0x0f830ffe) == 0x00800362) {                  /* BNOP with register           */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 18) & 0x1f,
                                                  1, INST_X_BIT (uiInst));
                ulNextPc = pRegs->regArr[iRegNum].GDBRA_ulValue;
                break;
            }

            if ((uiInst & 0x00001ffc) == 0x00000020) {                  /* BPOS                         */
                iRegNum = archTiC6xRegisterNumber((uiInst >> 23) & 0x1f,
                                                  INST_S_BIT (uiInst), 0);
                if (((INT)pRegs->regArr[iRegNum].GDBRA_ulValue) >= 0) {
                    ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                    ulNextPc += archTiC6xExtractSignedField(uiInst, 13, 10) << 2;
                }
                break;
            }

            if ((uiInst & 0xf000007c) == 0x10000010) {                  /* CALLP                        */
                ulNextPc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
                ulNextPc += archTiC6xExtractSignedField(uiInst, 7, 21) << 2;
                break;
            }
        }

        ulNextPc += TIC6X_OPCODE_SIZE;
    } while ((uiInst & 1));

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
    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
