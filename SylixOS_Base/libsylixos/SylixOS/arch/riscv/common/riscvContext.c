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
** ��   ��   ��: riscvContext.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
ARCH_REG_T  archGetGp(VOID);
/*********************************************************************************************************
** ��������: archTaskCtxCreate
** ��������: ��������������
** �䡡��  : pregctx        �Ĵ���������
**           pfuncTask      �������
**           pvArg          ��ڲ���
**           ptcb           ������ƿ�
**           pstkTop        ��ʼ����ջ���
**           ulOpt          ���񴴽�ѡ��
** �䡡��  : ��ʼ����ջ������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��ջ�Ӹߵ�ַ��͵�ַ����.
*********************************************************************************************************/
PLW_STACK  archTaskCtxCreate (ARCH_REG_CTX          *pregctx,
                              PTHREAD_START_ROUTINE  pfuncTask,
                              PVOID                  pvArg,
                              PLW_CLASS_TCB          ptcb,
                              PLW_STACK              pstkTop, 
                              ULONG                  ulOpt)
{
    ARCH_FP_CTX  *pfpctx;
    INT           i;

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  ��֤��ջ�� SP 8 �ֽڶ���    */

    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    pfpctx->FP_ulFp = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_ulLr = (ARCH_REG_T)LW_NULL;

    for (i = 0; i < 32; i++) {
        pregctx->REG_ulReg[i] = i;
    }

    pregctx->REG_ulSmallCtx    = 1;                                     /*  С������                    */
    pregctx->REG_ulStatus      = XSTATUS_XPP | XSTATUS_XPIE;
    if (ulOpt & LW_OPTION_THREAD_USED_FP) {
        pregctx->REG_ulStatus |= XSTATUS_FS;                            /*  ʹ�� FPU                    */
    } else {
        pregctx->REG_ulStatus &= ~XSTATUS_FS;                           /*  ���� FPU                    */
    }
    pregctx->REG_ulTrapVal     = 0;
    pregctx->REG_ulCause       = 0;
    pregctx->REG_ulEpc         = (ARCH_REG_T)pfuncTask;
    pregctx->REG_ulReg[REG_RA] = (ARCH_REG_T)pfuncTask;
    pregctx->REG_ulReg[REG_A0] = (ARCH_REG_T)pvArg;
    pregctx->REG_ulReg[REG_SP] = (ARCH_REG_T)pfpctx;
    pregctx->REG_ulReg[REG_FP] = pfpctx->FP_ulFp;
    pregctx->REG_ulReg[REG_GP] = archGetGp();
    pregctx->REG_ulReg[REG_TP] = 0;

    return  ((PLW_STACK)pfpctx);
}
/*********************************************************************************************************
** ��������: archTaskCtxSetFp
** ��������: ��������������ջ֡ (���� backtrace ����, ������� backtrace ����ļ�)
** �䡡��  : pstkDest      Ŀ�� stack frame
**           pregctxDest   Ŀ�ļĴ���������
**           pregctxSrc    Դ�Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archTaskCtxSetFp (PLW_STACK            pstkDest,
                        ARCH_REG_CTX        *pregctxDest,
                        const ARCH_REG_CTX  *pregctxSrc)
{
    ARCH_FP_CTX  *pfpctx = (ARCH_FP_CTX *)pstkDest;

    pfpctx->FP_ulFp = pregctxSrc->REG_ulReg[REG_FP];
    pfpctx->FP_ulLr = pregctxSrc->REG_ulEpc;

    pregctxDest->REG_ulReg[REG_FP] = (ARCH_REG_T)&pfpctx->FP_ulLr + sizeof(ARCH_REG_T);
}
/*********************************************************************************************************
** ��������: archTaskRegsGet
** ��������: ��ȡ�Ĵ���������
** �䡡��  : pregctx        �Ĵ���������
**           pregSp         SP ָ��
** �䡡��  : �Ĵ���������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ARCH_REG_CTX  *archTaskRegsGet (ARCH_REG_CTX  *pregctx, ARCH_REG_T *pregSp)
{
    *pregSp = pregctx->REG_ulReg[REG_SP];

    return  (pregctx);
}
/*********************************************************************************************************
** ��������: archTaskRegsSet
** ��������: ���üĴ���������
** �䡡��  : pregctxDest    Ŀ�ļĴ���������
**           pregctxSrc     Դ�Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archTaskRegsSet (ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc)
{
    archTaskCtxCopy(pregctxDest, pregctxSrc);
}
/*********************************************************************************************************
** ��������: archTaskCtxShow
** ��������: ��ӡ����������
** �䡡��  : iFd        �ļ�������
             pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, const ARCH_REG_CTX  *pregctx)
{
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LX_FMT      "0x%08x"
#else
#define LX_FMT      "0x%016lx"
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    if (iFd >= 0) {
        fdprintf(iFd, "\n");

        fdprintf(iFd, "STATUS    = "LX_FMT"  ", pregctx->REG_ulStatus);
        fdprintf(iFd, "EPC       = "LX_FMT"\n", pregctx->REG_ulEpc);

        fdprintf(iFd, "TVAL      = "LX_FMT"  ", pregctx->REG_ulTrapVal);
        fdprintf(iFd, "CAUSE     = "LX_FMT"\n", pregctx->REG_ulCause);

        fdprintf(iFd, "X00(ZERO) = "LX_FMT"  ", pregctx->REG_ulReg[0]);
        fdprintf(iFd, "X01(RA)   = "LX_FMT"\n", pregctx->REG_ulReg[1]);

        fdprintf(iFd, "X02(SP)   = "LX_FMT"  ", pregctx->REG_ulReg[2]);
        fdprintf(iFd, "X03(GP)   = "LX_FMT"\n", pregctx->REG_ulReg[3]);

        fdprintf(iFd, "X04(TP)   = "LX_FMT"  ", pregctx->REG_ulReg[4]);
        fdprintf(iFd, "X05(T0)   = "LX_FMT"\n", pregctx->REG_ulReg[5]);

        fdprintf(iFd, "X06(T1)   = "LX_FMT"  ", pregctx->REG_ulReg[6]);
        fdprintf(iFd, "X07(T2)   = "LX_FMT"\n", pregctx->REG_ulReg[7]);

        fdprintf(iFd, "X08(S0)   = "LX_FMT"  ", pregctx->REG_ulReg[8]);
        fdprintf(iFd, "X09(S1)   = "LX_FMT"\n", pregctx->REG_ulReg[9]);

        fdprintf(iFd, "X10(A0)   = "LX_FMT"  ", pregctx->REG_ulReg[10]);
        fdprintf(iFd, "X11(A1)   = "LX_FMT"\n", pregctx->REG_ulReg[11]);

        fdprintf(iFd, "X12(A2)   = "LX_FMT"  ", pregctx->REG_ulReg[12]);
        fdprintf(iFd, "X13(A3)   = "LX_FMT"\n", pregctx->REG_ulReg[13]);

        fdprintf(iFd, "X14(A4)   = "LX_FMT"  ", pregctx->REG_ulReg[14]);
        fdprintf(iFd, "X15(A5)   = "LX_FMT"\n", pregctx->REG_ulReg[15]);

        fdprintf(iFd, "X16(A6)   = "LX_FMT"  ", pregctx->REG_ulReg[16]);
        fdprintf(iFd, "X17(A7)   = "LX_FMT"\n", pregctx->REG_ulReg[17]);

        fdprintf(iFd, "X18(S2)   = "LX_FMT"  ", pregctx->REG_ulReg[18]);
        fdprintf(iFd, "X19(S3)   = "LX_FMT"\n", pregctx->REG_ulReg[19]);

        fdprintf(iFd, "X20(S4)   = "LX_FMT"  ", pregctx->REG_ulReg[20]);
        fdprintf(iFd, "X21(S5)   = "LX_FMT"\n", pregctx->REG_ulReg[21]);

        fdprintf(iFd, "X22(S6)   = "LX_FMT"  ", pregctx->REG_ulReg[22]);
        fdprintf(iFd, "X23(S7)   = "LX_FMT"\n", pregctx->REG_ulReg[23]);

        fdprintf(iFd, "X24(S8)   = "LX_FMT"  ", pregctx->REG_ulReg[24]);
        fdprintf(iFd, "X25(S9)   = "LX_FMT"\n", pregctx->REG_ulReg[25]);

        fdprintf(iFd, "X26(S10)  = "LX_FMT"  ", pregctx->REG_ulReg[26]);
        fdprintf(iFd, "X27(S11)  = "LX_FMT"\n", pregctx->REG_ulReg[27]);

        fdprintf(iFd, "X28(T3)   = "LX_FMT"  ", pregctx->REG_ulReg[28]);
        fdprintf(iFd, "X29(T4)   = "LX_FMT"\n", pregctx->REG_ulReg[29]);

        fdprintf(iFd, "X30(T5)   = "LX_FMT"  ", pregctx->REG_ulReg[30]);
        fdprintf(iFd, "X31(T6)   = "LX_FMT"\n", pregctx->REG_ulReg[31]);

    } else {
        archTaskCtxPrint(LW_NULL, 0, pregctx);
    }

#undef LX_FMT
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: archTaskCtxPrint
** ��������: ֱ�Ӵ�ӡ����������
** �䡡��  : pvBuffer   �ڴ滺���� (NULL, ��ʾֱ�Ӵ�ӡ)
**           stSize     �����С
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PVOID  pvBuffer, size_t  stSize, const ARCH_REG_CTX  *pregctx)
{
    if (pvBuffer && stSize) {
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LX_FMT      "0x%08x"
#else
#define LX_FMT      "0x%016lx"
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "STATUS    = "LX_FMT"  ", pregctx->REG_ulStatus);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EPC       = "LX_FMT"\n", pregctx->REG_ulEpc);

        stOft = bnprintf(pvBuffer, stSize, stOft, "TVAL      = "LX_FMT"  ", pregctx->REG_ulTrapVal);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CAUSE     = "LX_FMT"\n", pregctx->REG_ulCause);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X00(ZERO) = "LX_FMT"  ", pregctx->REG_ulReg[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X01(RA)   = "LX_FMT"\n", pregctx->REG_ulReg[1]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X02(SP)   = "LX_FMT"  ", pregctx->REG_ulReg[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X03(GP)   = "LX_FMT"\n", pregctx->REG_ulReg[3]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X04(TP)   = "LX_FMT"  ", pregctx->REG_ulReg[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X05(T0)   = "LX_FMT"\n", pregctx->REG_ulReg[5]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X06(T1)   = "LX_FMT"  ", pregctx->REG_ulReg[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X07(T2)   = "LX_FMT"\n", pregctx->REG_ulReg[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X08(S0)   = "LX_FMT"  ", pregctx->REG_ulReg[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X09(S1)   = "LX_FMT"\n", pregctx->REG_ulReg[9]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X10(A0)   = "LX_FMT"  ", pregctx->REG_ulReg[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X11(A1)   = "LX_FMT"\n", pregctx->REG_ulReg[11]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X12(A2)   = "LX_FMT"  ", pregctx->REG_ulReg[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X13(A3)   = "LX_FMT"\n", pregctx->REG_ulReg[13]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X14(A4)   = "LX_FMT"  ", pregctx->REG_ulReg[14]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X15(A5)   = "LX_FMT"\n", pregctx->REG_ulReg[15]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X16(A6)   = "LX_FMT"  ", pregctx->REG_ulReg[16]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X17(A7)   = "LX_FMT"\n", pregctx->REG_ulReg[17]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X18(S2)   = "LX_FMT"  ", pregctx->REG_ulReg[18]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X19(S3)   = "LX_FMT"\n", pregctx->REG_ulReg[19]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X20(S4)   = "LX_FMT"  ", pregctx->REG_ulReg[20]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X21(S5)   = "LX_FMT"\n", pregctx->REG_ulReg[21]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X22(S6)   = "LX_FMT"  ", pregctx->REG_ulReg[22]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X23(S7)   = "LX_FMT"\n", pregctx->REG_ulReg[23]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X24(S8)   = "LX_FMT"  ", pregctx->REG_ulReg[24]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X25(S9)   = "LX_FMT"\n", pregctx->REG_ulReg[25]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X26(S10)  = "LX_FMT"  ", pregctx->REG_ulReg[26]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X27(S11)  = "LX_FMT"\n", pregctx->REG_ulReg[27]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X28(T3)   = "LX_FMT"  ", pregctx->REG_ulReg[28]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X29(T4)   = "LX_FMT"\n", pregctx->REG_ulReg[29]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "X30(T5)   = "LX_FMT"  ", pregctx->REG_ulReg[30]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X31(T6)   = "LX_FMT"\n", pregctx->REG_ulReg[31]);

#undef LX_FMT
    } else {
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LX_FMT      "0x%08x"
#else
#define LX_FMT      "0x%016qx"
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

        _PrintFormat("\r\n");

        _PrintFormat("STATUS    = "LX_FMT"  ",   pregctx->REG_ulStatus);
        _PrintFormat("EPC       = "LX_FMT"\r\n", pregctx->REG_ulEpc);

        _PrintFormat("TVAL      = "LX_FMT"  ",   pregctx->REG_ulTrapVal);
        _PrintFormat("CAUSE     = "LX_FMT"\r\n", pregctx->REG_ulCause);

        _PrintFormat("X00(ZERO) = "LX_FMT"  ",   pregctx->REG_ulReg[0]);
        _PrintFormat("X01(RA)   = "LX_FMT"\r\n", pregctx->REG_ulReg[1]);

        _PrintFormat("X02(SP)   = "LX_FMT"  ",   pregctx->REG_ulReg[2]);
        _PrintFormat("X03(GP)   = "LX_FMT"\r\n", pregctx->REG_ulReg[3]);

        _PrintFormat("X04(TP)   = "LX_FMT"  ",   pregctx->REG_ulReg[4]);
        _PrintFormat("X05(T0)   = "LX_FMT"\r\n", pregctx->REG_ulReg[5]);

        _PrintFormat("X06(T1)   = "LX_FMT"  ",   pregctx->REG_ulReg[6]);
        _PrintFormat("X07(T2)   = "LX_FMT"\r\n", pregctx->REG_ulReg[7]);

        _PrintFormat("X08(S0)   = "LX_FMT"  ",   pregctx->REG_ulReg[8]);
        _PrintFormat("X09(S1)   = "LX_FMT"\r\n", pregctx->REG_ulReg[9]);

        _PrintFormat("X10(A0)   = "LX_FMT"  ",   pregctx->REG_ulReg[10]);
        _PrintFormat("X11(A1)   = "LX_FMT"\r\n", pregctx->REG_ulReg[11]);

        _PrintFormat("X12(A2)   = "LX_FMT"  ",   pregctx->REG_ulReg[12]);
        _PrintFormat("X13(A3)   = "LX_FMT"\r\n", pregctx->REG_ulReg[13]);

        _PrintFormat("X14(A4)   = "LX_FMT"  ",   pregctx->REG_ulReg[14]);
        _PrintFormat("X15(A5)   = "LX_FMT"\r\n", pregctx->REG_ulReg[15]);

        _PrintFormat("X16(A6)   = "LX_FMT"  ",   pregctx->REG_ulReg[16]);
        _PrintFormat("X17(A7)   = "LX_FMT"\r\n", pregctx->REG_ulReg[17]);

        _PrintFormat("X18(S2)   = "LX_FMT"  ",   pregctx->REG_ulReg[18]);
        _PrintFormat("X19(S3)   = "LX_FMT"\r\n", pregctx->REG_ulReg[19]);

        _PrintFormat("X20(S4)   = "LX_FMT"  ",   pregctx->REG_ulReg[20]);
        _PrintFormat("X21(S5)   = "LX_FMT"\r\n", pregctx->REG_ulReg[21]);

        _PrintFormat("X22(S6)   = "LX_FMT"  ",   pregctx->REG_ulReg[22]);
        _PrintFormat("X23(S7)   = "LX_FMT"\r\n", pregctx->REG_ulReg[23]);

        _PrintFormat("X24(S8)   = "LX_FMT"  ",   pregctx->REG_ulReg[24]);
        _PrintFormat("X25(S9)   = "LX_FMT"\r\n", pregctx->REG_ulReg[25]);

        _PrintFormat("X26(S10)  = "LX_FMT"  ",   pregctx->REG_ulReg[26]);
        _PrintFormat("X27(S11)  = "LX_FMT"\r\n", pregctx->REG_ulReg[27]);

        _PrintFormat("X28(T3)   = "LX_FMT"  ",   pregctx->REG_ulReg[28]);
        _PrintFormat("X29(T4)   = "LX_FMT"\r\n", pregctx->REG_ulReg[29]);

        _PrintFormat("X30(T5)   = "LX_FMT"  ",   pregctx->REG_ulReg[30]);
        _PrintFormat("X31(T6)   = "LX_FMT"\r\n", pregctx->REG_ulReg[31]);

#undef LX_FMT
    }
}
/*********************************************************************************************************
** ��������: archIntCtxSaveReg
** ��������: �жϱ���Ĵ���
** �䡡��  : pcpu      CPU �ṹ
**           reg0      �Ĵ��� 0
**           reg1      �Ĵ��� 1
**           reg2      �Ĵ��� 2
**           reg3      �Ĵ��� 3
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archIntCtxSaveReg (PLW_CLASS_CPU  pcpu,
                         ARCH_REG_T     reg0,
                         ARCH_REG_T     reg1,
                         ARCH_REG_T     reg2,
                         ARCH_REG_T     reg3)
{
}
/*********************************************************************************************************
** ��������: archCtxStackEnd
** ��������: ���ݼĴ��������Ļ��ջ������ַ
** �䡡��  : pregctx    �Ĵ���������
** �䡡��  : ջ������ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_STACK  archCtxStackEnd (const ARCH_REG_CTX  *pregctx)
{
    return  ((PLW_STACK)pregctx->REG_ulReg[REG_SP]);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
