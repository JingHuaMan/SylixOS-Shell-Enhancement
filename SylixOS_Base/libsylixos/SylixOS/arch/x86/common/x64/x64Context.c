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
** ��   ��   ��: x64Context.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 06 ��
**
** ��        ��: x86-64 ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  ��֤��ջ�� SP 16 �ֽڶ���   */
    pstkTop--;                                                          /*  GCC PUSH ��תΪ 16 �ֽڶ��� */
    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    pfpctx->FP_ulSavedRBP = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_ulRetAddr  = (ARCH_REG_T)LW_NULL;

    pregctx->REG_ulRAX = 0xeaeaeaeaeaeaeaea;
    pregctx->REG_ulRBX = 0xebebebebebebebeb;
    pregctx->REG_ulRCX = 0xecececececececec;
    pregctx->REG_ulRDX = 0xedededededededed;

    pregctx->REG_ulRSI = 0xe0e0e0e0e0e0e0e0;
    pregctx->REG_ulRDI = (ARCH_REG_T)pvArg;                             /*  ����                        */

    pregctx->REG_ulR8  = 0x0808080808080808;
    pregctx->REG_ulR9  = 0x0909090909090909;
    pregctx->REG_ulR10 = 0x1010101010101010;
    pregctx->REG_ulR11 = 0x1111111111111111;
    pregctx->REG_ulR12 = 0x1212121212121212;
    pregctx->REG_ulR13 = 0x1313131313131313;
    pregctx->REG_ulR14 = 0x1414141414141414;
    pregctx->REG_ulR15 = 0x1515151515151515;

    pregctx->REG_ulRBP = (ARCH_REG_T)pfpctx;                            /*  RBP ָ��Ĵ���              */
    pregctx->REG_ulRSP = (ARCH_REG_T)pfpctx;                            /*  RSP ָ��Ĵ���              */

    pregctx->REG_ulRIP = 0x0000000000000000;
    pregctx->REG_ulRIP = (ARCH_REG_T)pfuncTask;

    pregctx->REG_ulCS = X86_CS_KERNEL;
    pregctx->REG_ulSS = X86_DS_KERNEL;

    pregctx->REG_ulRFLAGS = X86_EFLAGS_IF;                              /*  �����ж�ʹ��λ              */

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
VOID  archTaskCtxSetFp (PLW_STACK               pstkDest,
                        ARCH_REG_CTX           *pregctxDest,
                        const ARCH_REG_CTX     *pregctxSrc)
{
    ARCH_FP_CTX  *pfpctx = (ARCH_FP_CTX *)pstkDest;

    pregctxDest->REG_ulRBP = (ARCH_REG_T)pregctxSrc->REG_ulRBP;
    pfpctx->FP_ulRetAddr   = (ARCH_REG_T)pregctxSrc->REG_ulRIP;
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
    *pregSp = pregctx->REG_ulRSP;

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
** ע  ��  : ���޸ĶμĴ���, RBP, RSP
*********************************************************************************************************/
VOID  archTaskRegsSet (ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc)
{
    pregctxDest->REG_ulRAX = pregctxSrc->REG_ulRAX;
    pregctxDest->REG_ulRBX = pregctxSrc->REG_ulRBX;
    pregctxDest->REG_ulRCX = pregctxSrc->REG_ulRCX;
    pregctxDest->REG_ulRDX = pregctxSrc->REG_ulRDX;

    pregctxDest->REG_ulRSI = pregctxSrc->REG_ulRSI;
    pregctxDest->REG_ulRDI = pregctxSrc->REG_ulRDI;

    pregctxDest->REG_ulR8  = pregctxSrc->REG_ulR8;
    pregctxDest->REG_ulR9  = pregctxSrc->REG_ulR9;
    pregctxDest->REG_ulR10 = pregctxSrc->REG_ulR10;
    pregctxDest->REG_ulR11 = pregctxSrc->REG_ulR11;
    pregctxDest->REG_ulR12 = pregctxSrc->REG_ulR12;
    pregctxDest->REG_ulR13 = pregctxSrc->REG_ulR13;
    pregctxDest->REG_ulR14 = pregctxSrc->REG_ulR14;
    pregctxDest->REG_ulR15 = pregctxSrc->REG_ulR15;

    pregctxDest->REG_ulRIP = pregctxSrc->REG_ulRIP;
    pregctxDest->REG_ulRFLAGS = pregctxSrc->REG_ulRFLAGS;
}
/*********************************************************************************************************
** ��������: archTaskCtxShow
** ��������: ��ӡ����������
** �䡡��  : iFd        �ļ�������
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, const ARCH_REG_CTX  *pregctx)
{
    if (iFd >= 0) {
        fdprintf(iFd, "\n");
        fdprintf(iFd, "RFLAGS = 0x%016lx\n", pregctx->REG_ulRFLAGS);

        fdprintf(iFd, "RIP = 0x%016lx\n", pregctx->REG_ulRIP);

        fdprintf(iFd, "SS  = 0x%016lx  ", pregctx->REG_ulSS);
        fdprintf(iFd, "CS  = 0x%016lx\n", pregctx->REG_ulCS);

        fdprintf(iFd, "RAX = 0x%016lx  ", pregctx->REG_ulRAX);
        fdprintf(iFd, "RBX = 0x%016lx\n", pregctx->REG_ulRBX);
        fdprintf(iFd, "RCX = 0x%016lx  ", pregctx->REG_ulRCX);
        fdprintf(iFd, "RDX = 0x%016lx\n", pregctx->REG_ulRDX);

        fdprintf(iFd, "RSI = 0x%016lx  ", pregctx->REG_ulRSI);
        fdprintf(iFd, "RDI = 0x%016lx\n", pregctx->REG_ulRDI);

        fdprintf(iFd, "R8  = 0x%016lx  ", pregctx->REG_ulR8);
        fdprintf(iFd, "R9  = 0x%016lx\n", pregctx->REG_ulR9);
        fdprintf(iFd, "R10 = 0x%016lx  ", pregctx->REG_ulR10);
        fdprintf(iFd, "R11 = 0x%016lx\n", pregctx->REG_ulR11);
        fdprintf(iFd, "R12 = 0x%016lx  ", pregctx->REG_ulR12);
        fdprintf(iFd, "R13 = 0x%016lx\n", pregctx->REG_ulR13);
        fdprintf(iFd, "R14 = 0x%016lx  ", pregctx->REG_ulR14);
        fdprintf(iFd, "R15 = 0x%016lx\n", pregctx->REG_ulR15);

        fdprintf(iFd, "RSP = 0x%016lx  ", pregctx->REG_ulRSP);
        fdprintf(iFd, "RBP = 0x%016lx\n", pregctx->REG_ulRBP);

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
    if (pvBuffer && stSize) {
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "RFLAGS = 0x%016qx\n", pregctx->REG_ulRFLAGS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RIP = 0x%016qx\n", pregctx->REG_ulRIP);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SS  = 0x%016qx  ", pregctx->REG_ulSS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CS  = 0x%016qx\n", pregctx->REG_ulCS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RAX = 0x%016qx  ", pregctx->REG_ulRAX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RBX = 0x%016qx\n", pregctx->REG_ulRBX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RCX = 0x%016qx  ", pregctx->REG_ulRCX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RDX = 0x%016qx\n", pregctx->REG_ulRDX);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RSI = 0x%016qx  ", pregctx->REG_ulRSI);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RDI = 0x%016qx\n", pregctx->REG_ulRDI);

        stOft = bnprintf(pvBuffer, stSize, stOft, "R8  = 0x%016qx  ", pregctx->REG_ulR8);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R9  = 0x%016qx\n", pregctx->REG_ulR9);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R10 = 0x%016qx  ", pregctx->REG_ulR10);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R11 = 0x%016qx\n", pregctx->REG_ulR11);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R12 = 0x%016qx  ", pregctx->REG_ulR12);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R13 = 0x%016qx\n", pregctx->REG_ulR13);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R14 = 0x%016qx  ", pregctx->REG_ulR14);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R15 = 0x%016qx\n", pregctx->REG_ulR15);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RSP = 0x%016qx  ", pregctx->REG_ulRSP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RBP = 0x%016qx\n", pregctx->REG_ulRBP);

    } else {
        _PrintFormat("\r\n");
        _PrintFormat("RFLAGS = 0x%016qx\r\n", pregctx->REG_ulRFLAGS);

        _PrintFormat("RIP = 0x%016qx\r\n",   pregctx->REG_ulRIP);

        _PrintFormat("SS  = 0x%016qx  ",   pregctx->REG_ulSS);
        _PrintFormat("CS  = 0x%016qx\r\n", pregctx->REG_ulCS);

        _PrintFormat("RAX = 0x%016qx  ",   pregctx->REG_ulRAX);
        _PrintFormat("RBX = 0x%016qx\r\n", pregctx->REG_ulRBX);
        _PrintFormat("RCX = 0x%016qx  ",   pregctx->REG_ulRCX);
        _PrintFormat("RDX = 0x%016qx\r\n", pregctx->REG_ulRDX);

        _PrintFormat("RSI = 0x%016qx  ",   pregctx->REG_ulRSI);
        _PrintFormat("RDI = 0x%016qx\r\n", pregctx->REG_ulRDI);

        _PrintFormat("R8  = 0x%016qx  ",   pregctx->REG_ulR8);
        _PrintFormat("R9  = 0x%016qx\r\n", pregctx->REG_ulR9);
        _PrintFormat("R10 = 0x%016qx  ",   pregctx->REG_ulR10);
        _PrintFormat("R11 = 0x%016qx\r\n", pregctx->REG_ulR11);
        _PrintFormat("R12 = 0x%016qx  ",   pregctx->REG_ulR12);
        _PrintFormat("R13 = 0x%016qx\r\n", pregctx->REG_ulR13);
        _PrintFormat("R14 = 0x%016qx  ",   pregctx->REG_ulR14);
        _PrintFormat("R15 = 0x%016qx\r\n", pregctx->REG_ulR15);

        _PrintFormat("RSP = 0x%016qx  ",   pregctx->REG_ulRSP);
        _PrintFormat("RBP = 0x%016qx\r\n", pregctx->REG_ulRBP);
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
    ARCH_REG_T  iD0, iD1, iD2;

#define COPY_CTX(to, from, cnt)                                             \
    __asm__ __volatile__ ("cld\n\t"                                         \
                          "rep ; movsq"                                     \
                          : "=&c" (iD0), "=&D" (iD1), "=&S" (iD2)           \
                          : "0" (cnt), "1" ((LONG)to), "2" ((LONG)from)     \
                          : "memory")

    if (pcpu->CPU_ulInterNesting == 1) {
        COPY_CTX(&pcpu->CPU_ptcbTCBCur->TCB_archRegCtx, reg0, ARCH_REG_CTX_WORD_SIZE);
    }
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
    return  ((PLW_STACK)pregctx->REG_ulRSP);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
