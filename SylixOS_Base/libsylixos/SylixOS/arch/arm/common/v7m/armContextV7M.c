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
** ��   ��   ��: armContextV7M.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  ��ջָ������ 8 �ֽڶ���     */

    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    pfpctx->FP_uiFp = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiLr = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiXpsr = 1 << 24;                                      /*  ���� Thumb ״̬λ           */
    pregctx->REG_uiR0   = (ARCH_REG_T)pvArg;
    pregctx->REG_uiR1   = 0x01010101;
    pregctx->REG_uiR2   = 0x02020202;
    pregctx->REG_uiR3   = 0x03030303;
    pregctx->REG_uiR4   = 0x04040404;
    pregctx->REG_uiR5   = 0x05050505;
    pregctx->REG_uiR6   = 0x06060606;
    pregctx->REG_uiR7   = 0x07070707;
    pregctx->REG_uiR8   = 0x08080808;
    pregctx->REG_uiR9   = 0x09090909;
    pregctx->REG_uiR10  = 0x10101010;
    pregctx->REG_uiFp   = pfpctx->FP_uiFp;
    pregctx->REG_uiIp   = 0x12121212;
    pregctx->REG_uiLr   = (ARCH_REG_T)pfuncTask;
    pregctx->REG_uiPc   = (ARCH_REG_T)pfuncTask;
    pregctx->REG_uiSp   = (ARCH_REG_T)pfpctx;

    pregctx->REG_uiExcRet  = 0xfffffffd;                                /*  ���߳�ģʽ����ʹ���̶߳�ջ  */
                                                                        /*  (SP=PSP)                    */
    pregctx->REG_uiBasePri = 0;

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

    /*
     *  �� ARCH_FP_CTX ������, ģ����һ��
     *  push {fp, lr}
     *  add  fp, sp, #4
     */
    pfpctx->FP_uiFp = pregctxSrc->REG_uiFp;
    pfpctx->FP_uiLr = pregctxSrc->REG_uiLr;

    pregctxDest->REG_uiFp = (ARCH_REG_T)&pfpctx->FP_uiLr;
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
    *pregSp = pregctx->REG_uiSp;

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
    *pregctxDest = *pregctxSrc;
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
        fdprintf(iFd, "XPSR    = 0x%08x\n", pregctx->REG_uiXpsr);
        fdprintf(iFd, "BASEPRI = 0x%08x\n", pregctx->REG_uiBasePri);
        fdprintf(iFd, "EXCRET  = 0x%08x\n", pregctx->REG_uiExcRet);

        fdprintf(iFd, "r0  = 0x%08x  ", pregctx->REG_uiR0);
        fdprintf(iFd, "r1  = 0x%08x\n", pregctx->REG_uiR1);
        fdprintf(iFd, "r2  = 0x%08x  ", pregctx->REG_uiR2);
        fdprintf(iFd, "r3  = 0x%08x\n", pregctx->REG_uiR3);
        fdprintf(iFd, "r4  = 0x%08x  ", pregctx->REG_uiR4);
        fdprintf(iFd, "r5  = 0x%08x\n", pregctx->REG_uiR5);
        fdprintf(iFd, "r6  = 0x%08x  ", pregctx->REG_uiR6);
        fdprintf(iFd, "r7  = 0x%08x\n", pregctx->REG_uiR7);
        fdprintf(iFd, "r8  = 0x%08x  ", pregctx->REG_uiR8);
        fdprintf(iFd, "r9  = 0x%08x\n", pregctx->REG_uiR9);
        fdprintf(iFd, "r10 = 0x%08x  ", pregctx->REG_uiR10);
        fdprintf(iFd, "fp  = 0x%08x\n", pregctx->REG_uiFp);
        fdprintf(iFd, "ip  = 0x%08x  ", pregctx->REG_uiIp);
        fdprintf(iFd, "sp  = 0x%08x\n", pregctx->REG_uiSp);
        fdprintf(iFd, "lr  = 0x%08x  ", pregctx->REG_uiLr);
        fdprintf(iFd, "pc  = 0x%08x\n", pregctx->REG_uiPc);

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

        stOft = bnprintf(pvBuffer, stSize, stOft, "XPSR    = 0x%08x\n", pregctx->REG_uiXpsr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "BASEPRI = 0x%08x\n", pregctx->REG_uiBasePri);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EXCRET  = 0x%08x\n", pregctx->REG_uiExcRet);

        stOft = bnprintf(pvBuffer, stSize, stOft, "r0  = 0x%08x  ", pregctx->REG_uiR0);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r1  = 0x%08x\n", pregctx->REG_uiR1);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r2  = 0x%08x  ", pregctx->REG_uiR2);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r3  = 0x%08x\n", pregctx->REG_uiR3);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r4  = 0x%08x  ", pregctx->REG_uiR4);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r5  = 0x%08x\n", pregctx->REG_uiR5);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r6  = 0x%08x  ", pregctx->REG_uiR6);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r7  = 0x%08x\n", pregctx->REG_uiR7);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r8  = 0x%08x  ", pregctx->REG_uiR8);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r9  = 0x%08x\n", pregctx->REG_uiR9);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r10 = 0x%08x  ", pregctx->REG_uiR10);
        stOft = bnprintf(pvBuffer, stSize, stOft, "fp  = 0x%08x\n", pregctx->REG_uiFp);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ip  = 0x%08x  ", pregctx->REG_uiIp);
        stOft = bnprintf(pvBuffer, stSize, stOft, "sp  = 0x%08x\n", pregctx->REG_uiSp);
        stOft = bnprintf(pvBuffer, stSize, stOft, "lr  = 0x%08x  ", pregctx->REG_uiLr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "pc  = 0x%08x\n", pregctx->REG_uiPc);

    } else {
        _PrintFormat("XPSR    = 0x%08x\r\n", pregctx->REG_uiXpsr);
        _PrintFormat("BASEPRI = 0x%08x\r\n", pregctx->REG_uiBasePri);
        _PrintFormat("EXCRET  = 0x%08x\r\n", pregctx->REG_uiExcRet);

        _PrintFormat("r0  = 0x%08x  ",   pregctx->REG_uiR0);
        _PrintFormat("r1  = 0x%08x\r\n", pregctx->REG_uiR1);
        _PrintFormat("r2  = 0x%08x  ",   pregctx->REG_uiR2);
        _PrintFormat("r3  = 0x%08x\r\n", pregctx->REG_uiR3);
        _PrintFormat("r4  = 0x%08x  ",   pregctx->REG_uiR4);
        _PrintFormat("r5  = 0x%08x\r\n", pregctx->REG_uiR5);
        _PrintFormat("r6  = 0x%08x  ",   pregctx->REG_uiR6);
        _PrintFormat("r7  = 0x%08x\r\n", pregctx->REG_uiR7);
        _PrintFormat("r8  = 0x%08x  ",   pregctx->REG_uiR8);
        _PrintFormat("r9  = 0x%08x\r\n", pregctx->REG_uiR9);
        _PrintFormat("r10 = 0x%08x  ",   pregctx->REG_uiR10);
        _PrintFormat("fp  = 0x%08x\r\n", pregctx->REG_uiFp);
        _PrintFormat("ip  = 0x%08x  ",   pregctx->REG_uiIp);
        _PrintFormat("sp  = 0x%08x\r\n", pregctx->REG_uiSp);
        _PrintFormat("lr  = 0x%08x  ",   pregctx->REG_uiLr);
        _PrintFormat("pc  = 0x%08x\r\n", pregctx->REG_uiPc);
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
#if LW_CFG_CORTEX_M_SVC_SWITCH > 0

VOID  archIntCtxSaveReg (PLW_CLASS_CPU  pcpu,
                         ARCH_REG_T     reg0,
                         ARCH_REG_T     reg1,
                         ARCH_REG_T     reg2,
                         ARCH_REG_T     reg3)
{
    ARCH_REG_CTX  *pregctx;

    if (pcpu->CPU_ulInterNesting == 1) {
        if (reg2 & (CORTEX_M_EXC_RETURN_MODE_MASK)) {
            pregctx = &pcpu->CPU_ptcbTCBCur->TCB_archRegCtx;

        } else {
            pregctx = (ARCH_REG_CTX *)(reg0 - sizeof(ARCH_REG_CTX));
        }
    } else {
        pregctx = (ARCH_REG_CTX *)(reg0 - sizeof(ARCH_REG_CTX));
    }

    pregctx->REG_uiSp      = reg0;
    pregctx->REG_uiBasePri = reg1;
    pregctx->REG_uiExcRet  = reg2;
}
/*********************************************************************************************************
** ��������: archPendSvSaveReg
** ��������: PendSV �жϱ���Ĵ���
** �䡡��  : reg0      �Ĵ��� 0
**           reg1      �Ĵ��� 1
**           reg2      �Ĵ��� 2
**           reg3      �Ĵ��� 3
** �䡡��  : �Ĵ���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#else                                                                   /*  LW_CFG_CORTEX_M_SVC_SWITCH  */

ARCH_REG_CTX  *archPendSvSaveReg (ARCH_REG_T     reg0,
                                  ARCH_REG_T     reg1,
                                  ARCH_REG_T     reg2,
                                  ARCH_REG_T     reg3)
{
    ARCH_REG_CTX   *pregctx;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    pregctx = &ptcbCur->TCB_archRegCtx;

    pregctx->REG_uiSp     = reg0;
    pregctx->REG_uiExcRet = reg1;

    return  (pregctx);
}

#endif                                                                  /*  !LW_CFG_CORTEX_M_SVC_SWITCH */
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
    return  ((PLW_STACK)pregctx->REG_uiSp);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
