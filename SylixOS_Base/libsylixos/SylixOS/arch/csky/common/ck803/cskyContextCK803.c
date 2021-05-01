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
** ��   ��   ��: cskyContextCK803.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 11 �� 12 ��
**
** ��        ��: C-SKY CK803 ��ϵ�ܹ������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY ��ϵ�ܹ�
*********************************************************************************************************/
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
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
    ARCH_REG_T    ulPsr;
    INT           i;

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  ��֤��ջ�� SP 8 �ֽڶ���    */

    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    /*
     * ��ʼ���Ĵ���������
     */
    for (i = 0; i < ARCH_GREG_NR; i++) {
        pregctx->REG_ulReg[i] = i;
    }

    ulPsr  = archGetPSR();                                              /*  ��õ�ǰ�� PSR �Ĵ���       */
    ulPsr |= bspIntInitEnableStatus() | M_PSR_IE | M_PSR_EE;            /*  ʹ���жϺ��쳣              */

    pregctx->REG_ulReg[REG_A0] = (ARCH_REG_T)pvArg;
    pregctx->REG_ulReg[REG_RA] = (ARCH_REG_T)0x0;
    pregctx->REG_ulReg[REG_SP] = (ARCH_REG_T)pfpctx;

    pregctx->REG_ulPsr = (ARCH_REG_T)ulPsr;
    pregctx->REG_ulPc  = (ARCH_REG_T)pfuncTask;

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
    pregctxDest->REG_ulReg[REG_FP] = (ARCH_REG_T)pregctxSrc->REG_ulReg[REG_SP];
    pregctxDest->REG_ulReg[REG_RA] = (ARCH_REG_T)pregctxSrc->REG_ulPc;
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
ARCH_REG_CTX  *archTaskRegsGet (ARCH_REG_CTX  *pregctx, ARCH_REG_T  *pregSp)
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
    pregctxDest->REG_ulReg[0]  = pregctxSrc->REG_ulReg[0];              /*  SP ������, ����ԭֵ         */
    pregctxDest->REG_ulReg[1]  = pregctxSrc->REG_ulReg[1];
    pregctxDest->REG_ulReg[2]  = pregctxSrc->REG_ulReg[2];
    pregctxDest->REG_ulReg[3]  = pregctxSrc->REG_ulReg[3];
    pregctxDest->REG_ulReg[4]  = pregctxSrc->REG_ulReg[4];
    pregctxDest->REG_ulReg[5]  = pregctxSrc->REG_ulReg[5];
    pregctxDest->REG_ulReg[6]  = pregctxSrc->REG_ulReg[6];
    pregctxDest->REG_ulReg[7]  = pregctxSrc->REG_ulReg[7];
    pregctxDest->REG_ulReg[8]  = pregctxSrc->REG_ulReg[8];
    pregctxDest->REG_ulReg[9]  = pregctxSrc->REG_ulReg[9];
    pregctxDest->REG_ulReg[10] = pregctxSrc->REG_ulReg[10];
    pregctxDest->REG_ulReg[11] = pregctxSrc->REG_ulReg[11];
    pregctxDest->REG_ulReg[12] = pregctxSrc->REG_ulReg[12];
    pregctxDest->REG_ulReg[13] = pregctxSrc->REG_ulReg[13];
    pregctxDest->REG_ulReg[14] = pregctxSrc->REG_ulReg[14];
    pregctxDest->REG_ulReg[15] = pregctxSrc->REG_ulReg[15];
    pregctxDest->REG_ulReg[16] = pregctxSrc->REG_ulReg[16];             /*  R28 ͨ��Ŀ�ļĴ���          */

    pregctxDest->REG_ulPsr = pregctxSrc->REG_ulPsr;                     /*  SPR �Ĵ���                  */
    pregctxDest->REG_ulPc  = pregctxSrc->REG_ulPc;                      /*  ����������Ĵ���            */
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
    ARCH_REG_T  ulPsr = pregctx->REG_ulPsr;

    if (iFd >= 0) {
#define LX_FMT      "0x%08x"

        fdprintf(iFd, "\n");

        fdprintf(iFd, "PC       = "LX_FMT"\n", pregctx->REG_ulPc);

        fdprintf(iFd, "R0(A0)   = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        fdprintf(iFd, "R1(A1)   = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        fdprintf(iFd, "R2(A2)   = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        fdprintf(iFd, "R3(A3)   = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        fdprintf(iFd, "R4       = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        fdprintf(iFd, "R5       = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        fdprintf(iFd, "R6       = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        fdprintf(iFd, "R7       = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        fdprintf(iFd, "R8       = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        fdprintf(iFd, "R9       = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        fdprintf(iFd, "R10      = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        fdprintf(iFd, "R11      = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        fdprintf(iFd, "R12      = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        fdprintf(iFd, "R13      = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        fdprintf(iFd, "R14(SP)  = "LX_FMT"\n", pregctx->REG_ulReg[14]);
        fdprintf(iFd, "R15      = "LX_FMT"\n", pregctx->REG_ulReg[15]);
        fdprintf(iFd, "R28      = "LX_FMT"\n", pregctx->REG_ulReg[16]);

        fdprintf(iFd, "PSR Status Register:\n");
        fdprintf(iFd, "S   = %d  ", (ulPsr & M_PSR_S)  >> S_PSR_S);
        fdprintf(iFd, "MM  = %d  ", (ulPsr & M_PSR_MM) >> S_PSR_MM);
        fdprintf(iFd, "EE  = %d\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        fdprintf(iFd, "IC  = %d  ", (ulPsr & M_PSR_IC) >> S_PSR_IC);
        fdprintf(iFd, "IE  = %d\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        fdprintf(iFd, "C   = %d\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

#undef LX_FMT
    } else {
        archTaskCtxPrint(LW_NULL, 0, pregctx);
    }
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
    ARCH_REG_T  ulPsr = pregctx->REG_ulPsr;

    if (pvBuffer && stSize) {
#define LX_FMT      "0x%08x"

        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "PC       = "LX_FMT"\n", pregctx->REG_ulPc);
    
        stOft = bnprintf(pvBuffer, stSize, stOft, "R0(A0)   = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R1(A1)   = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R2(A2)   = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R3(A3)   = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R4       = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R5       = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R6       = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R7       = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R8       = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R9       = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R10      = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R11      = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R12      = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R13      = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R14(SP)  = "LX_FMT"\n", pregctx->REG_ulReg[14]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R15      = "LX_FMT"\n", pregctx->REG_ulReg[15]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R28      = "LX_FMT"\n", pregctx->REG_ulReg[16]);
        
        stOft = bnprintf(pvBuffer, stSize, stOft, "PSR Status Register:\n");
        stOft = bnprintf(pvBuffer, stSize, stOft, "S   = %d  ", (ulPsr & M_PSR_S)  >> S_PSR_S);
        stOft = bnprintf(pvBuffer, stSize, stOft, "MM  = %d  ", (ulPsr & M_PSR_MM) >> S_PSR_MM);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EE  = %d\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IC  = %d  ", (ulPsr & M_PSR_IC) >> S_PSR_IC);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IE  = %d\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "C   = %d\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

#undef LX_FMT
    } else {
#define LX_FMT      "0x%08x"

        _PrintFormat("\r\n");

        _PrintFormat("PC       = "LX_FMT"\r\n", pregctx->REG_ulPc);

        _PrintFormat("R0(A0)   = "LX_FMT"\r\n", pregctx->REG_ulReg[0]);
        _PrintFormat("R1(A1)   = "LX_FMT"\r\n", pregctx->REG_ulReg[1]);
        _PrintFormat("R2(A2)   = "LX_FMT"\r\n", pregctx->REG_ulReg[2]);
        _PrintFormat("R3(A3)   = "LX_FMT"\r\n", pregctx->REG_ulReg[3]);
        _PrintFormat("R4       = "LX_FMT"\r\n", pregctx->REG_ulReg[4]);
        _PrintFormat("R5       = "LX_FMT"\r\n", pregctx->REG_ulReg[5]);
        _PrintFormat("R6       = "LX_FMT"\r\n", pregctx->REG_ulReg[6]);
        _PrintFormat("R7       = "LX_FMT"\r\n", pregctx->REG_ulReg[7]);
        _PrintFormat("R8       = "LX_FMT"\r\n", pregctx->REG_ulReg[8]);
        _PrintFormat("R9       = "LX_FMT"\r\n", pregctx->REG_ulReg[9]);
        _PrintFormat("R10      = "LX_FMT"\r\n", pregctx->REG_ulReg[10]);
        _PrintFormat("R11      = "LX_FMT"\r\n", pregctx->REG_ulReg[11]);
        _PrintFormat("R12      = "LX_FMT"\r\n", pregctx->REG_ulReg[12]);
        _PrintFormat("R13      = "LX_FMT"\r\n", pregctx->REG_ulReg[13]);
        _PrintFormat("R14(SP)  = "LX_FMT"\r\n", pregctx->REG_ulReg[14]);
        _PrintFormat("R15      = "LX_FMT"\r\n", pregctx->REG_ulReg[15]);
        _PrintFormat("R28      = "LX_FMT"\r\n", pregctx->REG_ulReg[16]);

        _PrintFormat("PSR Status Register:\r\n");
        _PrintFormat("S   = %d  ",   (ulPsr & M_PSR_S)  >> S_PSR_S);
        _PrintFormat("MM  = %d  ",   (ulPsr & M_PSR_MM) >> S_PSR_MM);
        _PrintFormat("EE  = %d\r\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        _PrintFormat("IC  = %d  ",   (ulPsr & M_PSR_IC) >> S_PSR_IC);
        _PrintFormat("IE  = %d\r\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        _PrintFormat("C   = %d\r\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

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
    ARCH_REG_CTX  *pregctx;

    if (pcpu->CPU_ulInterNesting == 1) {
        pregctx = &pcpu->CPU_ptcbTCBCur->TCB_archRegCtx;

    } else {
        pregctx = (ARCH_REG_CTX *)(((ARCH_REG_CTX *)reg0)->REG_ulReg[REG_SP] - ARCH_REG_CTX_SIZE);
    }

    archTaskCtxCopy(pregctx, (ARCH_REG_CTX *)reg0);
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

#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
/*********************************************************************************************************
  END
*********************************************************************************************************/
