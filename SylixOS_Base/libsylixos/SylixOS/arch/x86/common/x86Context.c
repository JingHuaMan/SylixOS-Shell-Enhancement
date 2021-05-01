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
** ��   ��   ��: x86Context.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  ��֤��ջ�� SP 8 �ֽڶ���    */

    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    pfpctx->FP_uiArg     = (ARCH_REG_T)pvArg;
    pfpctx->FP_uiRetAddr = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiEAX = 0xeaeaeaea;                                    /*  4 �����ݼĴ���              */
    pregctx->REG_uiEBX = 0xebebebeb;
    pregctx->REG_uiECX = 0xecececec;
    pregctx->REG_uiEDX = 0xedededed;

    pregctx->REG_uiESI = 0xe0e0e0e0;                                    /*  2 ����ַ��ָ��Ĵ���        */
    pregctx->REG_uiEDI = 0xe1e1e1e1;

    pregctx->REG_uiEBP = (ARCH_REG_T)pfpctx;                            /*  ջָ֡��Ĵ���              */
    pregctx->REG_uiESP = (ARCH_REG_T)pfpctx;                            /*  ��ջָ��Ĵ���              */

    pregctx->REG_uiError  = 0x00000000;                                 /*  ERROR CODE                  */
    pregctx->REG_uiEIP    = (ARCH_REG_T)pfuncTask;                      /*  ָ��ָ��Ĵ���(EIP)         */

    pregctx->REG_uiCS     = X86_CS_KERNEL;                              /*  ����μĴ���(CS)            */
    pregctx->REG_uiSS     = X86_DS_KERNEL;                              /*  ����μĴ���(SS)            */
    pregctx->REG_uiEFLAGS = X86_EFLAGS_IF;                              /*  ��־�Ĵ��������ж�ʹ��λ    */

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

    pregctxDest->REG_uiEBP = (ARCH_REG_T)pregctxSrc->REG_uiEBP;
    pfpctx->FP_uiRetAddr   = (ARCH_REG_T)pregctxSrc->REG_uiEIP;
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
    *pregSp = pregctx->REG_uiESP;

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
** ע  ��  : ���޸ĶμĴ���, EBP, ESP
*********************************************************************************************************/
VOID  archTaskRegsSet (ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc)
{
    pregctxDest->REG_uiEAX = pregctxSrc->REG_uiEAX;
    pregctxDest->REG_uiEBX = pregctxSrc->REG_uiEBX;
    pregctxDest->REG_uiECX = pregctxSrc->REG_uiECX;
    pregctxDest->REG_uiEDX = pregctxSrc->REG_uiEDX;

    pregctxDest->REG_uiESI = pregctxSrc->REG_uiESI;
    pregctxDest->REG_uiEDI = pregctxSrc->REG_uiEDI;

    pregctxDest->REG_uiEIP    = pregctxSrc->REG_uiEIP;
    pregctxDest->REG_uiEFLAGS = pregctxSrc->REG_uiEFLAGS;
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
        fdprintf(iFd, "EFLAGS = 0x%08x\n", pregctx->REG_uiEFLAGS);

        fdprintf(iFd, "EIP = 0x%08x\n", pregctx->REG_uiEIP);

        fdprintf(iFd, "CS  = 0x%08x  ", pregctx->REG_uiCS);
        fdprintf(iFd, "SS  = 0x%08x\n", pregctx->REG_uiSS);

        fdprintf(iFd, "EAX = 0x%08x  ", pregctx->REG_uiEAX);
        fdprintf(iFd, "EBX = 0x%08x\n", pregctx->REG_uiEBX);
        fdprintf(iFd, "ECX = 0x%08x  ", pregctx->REG_uiECX);
        fdprintf(iFd, "EDX = 0x%08x\n", pregctx->REG_uiEDX);

        fdprintf(iFd, "ESI = 0x%08x  ", pregctx->REG_uiESI);
        fdprintf(iFd, "EDI = 0x%08x\n", pregctx->REG_uiEDI);

        fdprintf(iFd, "EBP = 0x%08x  ", pregctx->REG_uiEBP);
        fdprintf(iFd, "ESP = 0x%08x\n", pregctx->REG_uiESP);

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

        stOft = bnprintf(pvBuffer, stSize, stOft, "EFLAGS = 0x%08x\n", pregctx->REG_uiEFLAGS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "EIP = 0x%08x\n", pregctx->REG_uiEIP);

        stOft = bnprintf(pvBuffer, stSize, stOft, "CS  = 0x%08x  ", pregctx->REG_uiCS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SS  = 0x%08x\n", pregctx->REG_uiSS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "EAX = 0x%08x  ", pregctx->REG_uiEAX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EBX = 0x%08x\n", pregctx->REG_uiEBX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ECX = 0x%08x  ", pregctx->REG_uiECX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EDX = 0x%08x\n", pregctx->REG_uiEDX);

        stOft = bnprintf(pvBuffer, stSize, stOft, "ESI = 0x%08x  ", pregctx->REG_uiESI);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EDI = 0x%08x\n", pregctx->REG_uiEDI);

        stOft = bnprintf(pvBuffer, stSize, stOft, "EBP = 0x%08x  ", pregctx->REG_uiEBP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ESP = 0x%08x\n", pregctx->REG_uiESP);

    } else {
        _PrintFormat("\r\n");

        _PrintFormat("EFLAGS = 0x%08x\r\n", pregctx->REG_uiEFLAGS);

        _PrintFormat("EIP = 0x%08x\r\n", pregctx->REG_uiEIP);

        _PrintFormat("CS  = 0x%08x  ",   pregctx->REG_uiCS);
        _PrintFormat("SS  = 0x%08x\r\n", pregctx->REG_uiSS);

        _PrintFormat("EAX = 0x%08x  ",   pregctx->REG_uiEAX);
        _PrintFormat("EBX = 0x%08x\r\n", pregctx->REG_uiEBX);
        _PrintFormat("ECX = 0x%08x  ",   pregctx->REG_uiECX);
        _PrintFormat("EDX = 0x%08x\r\n", pregctx->REG_uiEDX);

        _PrintFormat("ESI = 0x%08x  ",   pregctx->REG_uiESI);
        _PrintFormat("EDI = 0x%08x\r\n", pregctx->REG_uiEDI);

        _PrintFormat("EBP = 0x%08x  ",   pregctx->REG_uiEBP);
        _PrintFormat("ESP = 0x%08x\r\n", pregctx->REG_uiESP);
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
    ARCH_REG_T     iD0, iD1, iD2;

#define COPY_CTX(to, from, cnt)                                             \
    __asm__ __volatile__ ("cld\n\t"                                         \
                          "rep ; movsl"                                     \
                          : "=&c" (iD0), "=&D" (iD1), "=&S" (iD2)           \
                          : "0" (cnt), "1" ((LONG)to), "2" ((LONG)from)     \
                          : "memory")

    if (pcpu->CPU_ulInterNesting == 1) {
        pregctx = (ARCH_REG_CTX *)reg0;
        if (pregctx->REG_uiCS == X86_CS_USER) {
            COPY_CTX(&pcpu->CPU_ptcbTCBCur->TCB_archRegCtx, pregctx, ARCH_REG_CTX_WORD_SIZE);

        } else {
            COPY_CTX(&pcpu->CPU_ptcbTCBCur->TCB_archRegCtx, pregctx, ARCH_REG_CTX_WORD_SIZE - 2);
            pregctx = &pcpu->CPU_ptcbTCBCur->TCB_archRegCtx;
            pregctx->REG_uiSS  = X86_DS_KERNEL;
            pregctx->REG_uiESP = reg0 + sizeof(ARCH_REG_CTX) - 2 * sizeof(ARCH_REG_T);
        }
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
    return  ((PLW_STACK)pregctx->REG_uiESP);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
