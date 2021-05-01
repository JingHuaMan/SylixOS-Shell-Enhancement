/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: cskyContext.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 11 日
**
** 描        述: C-SKY 体系架构上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY 体系架构
*********************************************************************************************************/
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
** 函数名称: archTaskCtxCreate
** 功能描述: 创建任务上下文
** 输　入  : pregctx        寄存器上下文
**           pfuncTask      任务入口
**           pvArg          入口参数
**           ptcb           任务控制块
**           pstkTop        初始化堆栈起点
**           ulOpt          任务创建选项
** 输　出  : 初始化堆栈结束点
** 全局变量:
** 调用模块:
** 注  意  : 堆栈从高地址向低地址增长.
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  保证出栈后 SP 8 字节对齐    */

    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));

    /*
     * 初始化寄存器上下文
     */
    for (i = 0; i < ARCH_GREG_NR; i++) {
        pregctx->REG_ulReg[i] = i;
    }

    pregctx->REG_ulReg[REG_A0] = (ARCH_REG_T)pvArg;
    pregctx->REG_ulReg[REG_RA] = (ARCH_REG_T)0x0;
    pregctx->REG_ulReg[REG_SP] = (ARCH_REG_T)pfpctx;

    ulPsr  = archGetPSR();                                              /*  获得当前的 PSR 寄存器       */
    ulPsr |= bspIntInitEnableStatus() | M_PSR_IE | M_PSR_EE;            /*  使能中断和异常              */
    pregctx->REG_ulPsr = (ARCH_REG_T)ulPsr;
    pregctx->REG_ulPc  = (ARCH_REG_T)pfuncTask;
    pregctx->REG_ulLo  = (ARCH_REG_T)0x0;
    pregctx->REG_ulHi  = (ARCH_REG_T)0x0;
    pregctx->REG_ulMeh = (ARCH_REG_T)0x0;

    return  ((PLW_STACK)pfpctx);
}
/*********************************************************************************************************
** 函数名称: archTaskCtxSetFp
** 功能描述: 设置任务上下文栈帧 (用于 backtrace 回溯, 详情请见 backtrace 相关文件)
** 输　入  : pstkDest      目的 stack frame
**           pregctxDest   目的寄存器上下文
**           pregctxSrc    源寄存器上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskCtxSetFp (PLW_STACK            pstkDest,
                        ARCH_REG_CTX        *pregctxDest,
                        const ARCH_REG_CTX  *pregctxSrc)
{    
    pregctxDest->REG_ulReg[REG_FP] = (ARCH_REG_T)pregctxSrc->REG_ulReg[REG_SP];
    pregctxDest->REG_ulReg[REG_RA] = (ARCH_REG_T)pregctxSrc->REG_ulPc;
}
/*********************************************************************************************************
** 函数名称: archTaskRegsGet
** 功能描述: 获取寄存器上下文
** 输　入  : pregctx        寄存器上下文
**           pregSp         SP 指针
** 输　出  : 寄存器上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *archTaskRegsGet (ARCH_REG_CTX  *pregctx, ARCH_REG_T *pregSp)
{
    *pregSp = pregctx->REG_ulReg[REG_SP];
    
    return  (pregctx);
}
/*********************************************************************************************************
** 函数名称: archTaskRegsSet
** 功能描述: 设置寄存器上下文
** 输　入  : pregctxDest    目的寄存器上下文
**           pregctxSrc     源寄存器上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskRegsSet (ARCH_REG_CTX  *pregctxDest, const ARCH_REG_CTX  *pregctxSrc)
{   
#ifdef __CSKYABIV2__
    pregctxDest->REG_ulReg[0]  = pregctxSrc->REG_ulReg[0];              /*  SP 不设置, 保持原值         */
#endif
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
#ifndef __CSKYABIV2__
    pregctxDest->REG_ulReg[14] = pregctxSrc->REG_ulReg[14];             /*  SP 不设置, 保持原值         */
#endif
    pregctxDest->REG_ulReg[15] = pregctxSrc->REG_ulReg[15];
    pregctxDest->REG_ulReg[16] = pregctxSrc->REG_ulReg[16];
    pregctxDest->REG_ulReg[17] = pregctxSrc->REG_ulReg[17];
    pregctxDest->REG_ulReg[18] = pregctxSrc->REG_ulReg[18];
    pregctxDest->REG_ulReg[19] = pregctxSrc->REG_ulReg[19];
    pregctxDest->REG_ulReg[20] = pregctxSrc->REG_ulReg[20];
    pregctxDest->REG_ulReg[21] = pregctxSrc->REG_ulReg[21];
    pregctxDest->REG_ulReg[22] = pregctxSrc->REG_ulReg[22];
    pregctxDest->REG_ulReg[23] = pregctxSrc->REG_ulReg[23];
    pregctxDest->REG_ulReg[24] = pregctxSrc->REG_ulReg[24];
    pregctxDest->REG_ulReg[25] = pregctxSrc->REG_ulReg[25];
    pregctxDest->REG_ulReg[26] = pregctxSrc->REG_ulReg[26];
    pregctxDest->REG_ulReg[27] = pregctxSrc->REG_ulReg[27];
    pregctxDest->REG_ulReg[28] = pregctxSrc->REG_ulReg[28];
    pregctxDest->REG_ulReg[29] = pregctxSrc->REG_ulReg[29];
    pregctxDest->REG_ulReg[30] = pregctxSrc->REG_ulReg[30];   
    pregctxDest->REG_ulReg[31] = pregctxSrc->REG_ulReg[31];
    
    pregctxDest->REG_ulLo  = pregctxSrc->REG_ulLo;                      /*  除数低位寄存器              */
    pregctxDest->REG_ulHi  = pregctxSrc->REG_ulHi;                      /*  除数高位寄存器              */
    pregctxDest->REG_ulPsr = pregctxSrc->REG_ulPsr;                     /*  PSR 寄存器                  */
    pregctxDest->REG_ulPc  = pregctxSrc->REG_ulPc;                      /*  程序计数器寄存器            */
    pregctxDest->REG_ulMeh = pregctxSrc->REG_ulMeh;                     /*  错误地址寄存器              */
}
/*********************************************************************************************************
** 函数名称: archTaskCtxShow
** 功能描述: 打印任务上下文
** 输　入  : iFd        文件描述符
             pregctx    寄存器上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_T  ulPsr = pregctx->REG_ulPsr;

#define LX_FMT      "0x%08x"

    if (iFd >= 0) {
        fdprintf(iFd, "\n");

        fdprintf(iFd, "PC      = "LX_FMT"\n", pregctx->REG_ulPc);
        fdprintf(iFd, "LO      = "LX_FMT"\n", pregctx->REG_ulLo);
        fdprintf(iFd, "HI      = "LX_FMT"\n", pregctx->REG_ulHi);
        fdprintf(iFd, "MEH     = "LX_FMT"\n", pregctx->REG_ulMeh);
        
#ifdef __CSKYABIV2__
        fdprintf(iFd, "R0(A0)  = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        fdprintf(iFd, "R1(A1)  = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        fdprintf(iFd, "R2(A2)  = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        fdprintf(iFd, "R3(A3)  = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        fdprintf(iFd, "R4(S0)  = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        fdprintf(iFd, "R5(S1)  = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        fdprintf(iFd, "R6(S2)  = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        fdprintf(iFd, "R7(S3)  = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        fdprintf(iFd, "R8(FP)  = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        fdprintf(iFd, "R9(S5)  = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        fdprintf(iFd, "R10(S6) = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        fdprintf(iFd, "R11(S7) = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        fdprintf(iFd, "R12(S8) = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        fdprintf(iFd, "R13(S9) = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        fdprintf(iFd, "R14(SP) = "LX_FMT"\n", pregctx->REG_ulReg[14]);
#else
        fdprintf(iFd, "R0(SP)  = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        fdprintf(iFd, "R1(S9)  = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        fdprintf(iFd, "R2(A0)  = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        fdprintf(iFd, "R3(A1)  = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        fdprintf(iFd, "R4(A2)  = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        fdprintf(iFd, "R5(A3)  = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        fdprintf(iFd, "R6(S0)  = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        fdprintf(iFd, "R7(S1)  = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        fdprintf(iFd, "R8(S2)  = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        fdprintf(iFd, "R9(S3)  = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        fdprintf(iFd, "R10(S4) = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        fdprintf(iFd, "R11(S5) = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        fdprintf(iFd, "R12(S6) = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        fdprintf(iFd, "R13(S7) = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        fdprintf(iFd, "R14(S8) = "LX_FMT"\n", pregctx->REG_ulReg[14]);
#endif                                                                  /*  __CSKYABIV2__               */

        fdprintf(iFd, "R15     = "LX_FMT"\n", pregctx->REG_ulReg[15]);
        fdprintf(iFd, "R16     = "LX_FMT"\n", pregctx->REG_ulReg[16]);
        fdprintf(iFd, "R17     = "LX_FMT"\n", pregctx->REG_ulReg[17]);
        fdprintf(iFd, "R18     = "LX_FMT"\n", pregctx->REG_ulReg[18]);
        fdprintf(iFd, "R19     = "LX_FMT"\n", pregctx->REG_ulReg[19]);
        fdprintf(iFd, "R20     = "LX_FMT"\n", pregctx->REG_ulReg[20]);
        fdprintf(iFd, "R21     = "LX_FMT"\n", pregctx->REG_ulReg[21]);
        fdprintf(iFd, "R22     = "LX_FMT"\n", pregctx->REG_ulReg[22]);
        fdprintf(iFd, "R23     = "LX_FMT"\n", pregctx->REG_ulReg[23]);
        fdprintf(iFd, "R24     = "LX_FMT"\n", pregctx->REG_ulReg[24]);
        fdprintf(iFd, "R25     = "LX_FMT"\n", pregctx->REG_ulReg[25]);
        fdprintf(iFd, "R26     = "LX_FMT"\n", pregctx->REG_ulReg[26]);
        fdprintf(iFd, "R27     = "LX_FMT"\n", pregctx->REG_ulReg[27]);
        fdprintf(iFd, "R28     = "LX_FMT"\n", pregctx->REG_ulReg[28]);
        fdprintf(iFd, "R29     = "LX_FMT"\n", pregctx->REG_ulReg[29]);
        fdprintf(iFd, "R30     = "LX_FMT"\n", pregctx->REG_ulReg[30]);
        fdprintf(iFd, "R31     = "LX_FMT"\n", pregctx->REG_ulReg[31]);
        
        fdprintf(iFd, "PSR Status Register:\n");
        fdprintf(iFd, "S   = %d  ", (ulPsr & M_PSR_S)  >> S_PSR_S);
        fdprintf(iFd, "TE  = %d\n", (ulPsr & M_PSR_TE) >> S_PSR_TE);
        fdprintf(iFd, "MM  = %d  ", (ulPsr & M_PSR_MM) >> S_PSR_MM);
        fdprintf(iFd, "EE  = %d\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        fdprintf(iFd, "IC  = %d  ", (ulPsr & M_PSR_IC) >> S_PSR_IC);
        fdprintf(iFd, "IE  = %d\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        fdprintf(iFd, "FE  = %d  ", (ulPsr & M_PSR_FE) >> S_PSR_FE);
        fdprintf(iFd, "AF  = %d\n", (ulPsr & M_PSR_AF) >> S_PSR_AF);
        fdprintf(iFd, "C   = %d\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

    } else {
        archTaskCtxPrint(LW_NULL, 0, pregctx);
    }

#undef LX_FMT
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: archTaskCtxPrint
** 功能描述: 直接打印任务上下文
** 输　入  : pvBuffer   内存缓冲区 (NULL, 表示直接打印)
**           stSize     缓冲大小
**           pregctx    寄存器上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PVOID  pvBuffer, size_t  stSize, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_T  ulPsr = pregctx->REG_ulPsr;

    if (pvBuffer && stSize) {

#define LX_FMT      "0x%08x"

        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "PC      = "LX_FMT"\n", pregctx->REG_ulPc);
        stOft = bnprintf(pvBuffer, stSize, stOft, "LO      = "LX_FMT"\n", pregctx->REG_ulLo);
        stOft = bnprintf(pvBuffer, stSize, stOft, "HI      = "LX_FMT"\n", pregctx->REG_ulHi);
        stOft = bnprintf(pvBuffer, stSize, stOft, "MEH     = "LX_FMT"\n", pregctx->REG_ulMeh);
    
#ifdef __CSKYABIV2__
        stOft = bnprintf(pvBuffer, stSize, stOft, "R0(A0)  = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R1(A1)  = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R2(A2)  = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R3(A3)  = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R4(S0)  = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R5(S1)  = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R6(S2)  = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R7(S3)  = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R8(FP)  = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R9(S5)  = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R10(S6) = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R11(S7) = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R12(S8) = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R13(S9) = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R14(SP) = "LX_FMT"\n", pregctx->REG_ulReg[14]);
#else
        stOft = bnprintf(pvBuffer, stSize, stOft, "R0(SP)  = "LX_FMT"\n", pregctx->REG_ulReg[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R1(S9)  = "LX_FMT"\n", pregctx->REG_ulReg[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R2(A0)  = "LX_FMT"\n", pregctx->REG_ulReg[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R3(A1)  = "LX_FMT"\n", pregctx->REG_ulReg[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R4(A2)  = "LX_FMT"\n", pregctx->REG_ulReg[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R5(A3)  = "LX_FMT"\n", pregctx->REG_ulReg[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R6(S0)  = "LX_FMT"\n", pregctx->REG_ulReg[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R7(S1)  = "LX_FMT"\n", pregctx->REG_ulReg[7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R8(S2)  = "LX_FMT"\n", pregctx->REG_ulReg[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R9(S3)  = "LX_FMT"\n", pregctx->REG_ulReg[9]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R10(S4) = "LX_FMT"\n", pregctx->REG_ulReg[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R11(S5) = "LX_FMT"\n", pregctx->REG_ulReg[11]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R12(S6) = "LX_FMT"\n", pregctx->REG_ulReg[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R13(S7) = "LX_FMT"\n", pregctx->REG_ulReg[13]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R14(S8) = "LX_FMT"\n", pregctx->REG_ulReg[14]);
#endif                                                                  /*  __CSKYABIV2__               */

        stOft = bnprintf(pvBuffer, stSize, stOft, "R15     = "LX_FMT"\n", pregctx->REG_ulReg[15]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R16     = "LX_FMT"\n", pregctx->REG_ulReg[16]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R17     = "LX_FMT"\n", pregctx->REG_ulReg[17]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R18     = "LX_FMT"\n", pregctx->REG_ulReg[18]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R19     = "LX_FMT"\n", pregctx->REG_ulReg[19]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R20     = "LX_FMT"\n", pregctx->REG_ulReg[20]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R21     = "LX_FMT"\n", pregctx->REG_ulReg[21]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R22     = "LX_FMT"\n", pregctx->REG_ulReg[22]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R23     = "LX_FMT"\n", pregctx->REG_ulReg[23]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R24     = "LX_FMT"\n", pregctx->REG_ulReg[24]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R25     = "LX_FMT"\n", pregctx->REG_ulReg[25]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R26     = "LX_FMT"\n", pregctx->REG_ulReg[26]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R27     = "LX_FMT"\n", pregctx->REG_ulReg[27]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R28     = "LX_FMT"\n", pregctx->REG_ulReg[28]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R29     = "LX_FMT"\n", pregctx->REG_ulReg[29]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R30     = "LX_FMT"\n", pregctx->REG_ulReg[30]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R31     = "LX_FMT"\n", pregctx->REG_ulReg[31]);
        
        stOft = bnprintf(pvBuffer, stSize, stOft, "PSR Status Register:\n");
        stOft = bnprintf(pvBuffer, stSize, stOft, "S   = %d  ", (ulPsr & M_PSR_S)  >> S_PSR_S);
        stOft = bnprintf(pvBuffer, stSize, stOft, "TE  = %d\n", (ulPsr & M_PSR_TE) >> S_PSR_TE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "MM  = %d  ", (ulPsr & M_PSR_MM) >> S_PSR_MM);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EE  = %d\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IC  = %d  ", (ulPsr & M_PSR_IC) >> S_PSR_IC);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IE  = %d\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "FE  = %d  ", (ulPsr & M_PSR_FE) >> S_PSR_FE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "AF  = %d\n", (ulPsr & M_PSR_AF) >> S_PSR_AF);
        stOft = bnprintf(pvBuffer, stSize, stOft, "C   = %d\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

#undef LX_FMT
    } else {
#define LX_FMT      "0x%08x"

        _PrintFormat("\r\n");

        _PrintFormat("PC      = "LX_FMT"\r\n", pregctx->REG_ulPc);
        _PrintFormat("LO      = "LX_FMT"\r\n", pregctx->REG_ulLo);
        _PrintFormat("HI      = "LX_FMT"\r\n", pregctx->REG_ulHi);
        _PrintFormat("MEH     = "LX_FMT"\r\n", pregctx->REG_ulMeh);
        
#ifdef __CSKYABIV2__
        _PrintFormat("R0(A0)  = "LX_FMT"\r\n", pregctx->REG_ulReg[0]);
        _PrintFormat("R1(A1)  = "LX_FMT"\r\n", pregctx->REG_ulReg[1]);
        _PrintFormat("R2(A2)  = "LX_FMT"\r\n", pregctx->REG_ulReg[2]);
        _PrintFormat("R3(A3)  = "LX_FMT"\r\n", pregctx->REG_ulReg[3]);
        _PrintFormat("R4(S0)  = "LX_FMT"\r\n", pregctx->REG_ulReg[4]);
        _PrintFormat("R5(S1)  = "LX_FMT"\r\n", pregctx->REG_ulReg[5]);
        _PrintFormat("R6(S2)  = "LX_FMT"\r\n", pregctx->REG_ulReg[6]);
        _PrintFormat("R7(S3)  = "LX_FMT"\r\n", pregctx->REG_ulReg[7]);
        _PrintFormat("R8(FP)  = "LX_FMT"\r\n", pregctx->REG_ulReg[8]);
        _PrintFormat("R9(S5)  = "LX_FMT"\r\n", pregctx->REG_ulReg[9]);
        _PrintFormat("R10(S6) = "LX_FMT"\r\n", pregctx->REG_ulReg[10]);
        _PrintFormat("R11(S7) = "LX_FMT"\r\n", pregctx->REG_ulReg[11]);
        _PrintFormat("R12(S8) = "LX_FMT"\r\n", pregctx->REG_ulReg[12]);
        _PrintFormat("R13(S9) = "LX_FMT"\r\n", pregctx->REG_ulReg[13]);
        _PrintFormat("R14(SP) = "LX_FMT"\r\n", pregctx->REG_ulReg[14]);
#else
        _PrintFormat("R0(SP)  = "LX_FMT"\r\n", pregctx->REG_ulReg[0]);
        _PrintFormat("R1(S9)  = "LX_FMT"\r\n", pregctx->REG_ulReg[1]);
        _PrintFormat("R2(A0)  = "LX_FMT"\r\n", pregctx->REG_ulReg[2]);
        _PrintFormat("R3(A1)  = "LX_FMT"\r\n", pregctx->REG_ulReg[3]);
        _PrintFormat("R4(A2)  = "LX_FMT"\r\n", pregctx->REG_ulReg[4]);
        _PrintFormat("R5(A3)  = "LX_FMT"\r\n", pregctx->REG_ulReg[5]);
        _PrintFormat("R6(S0)  = "LX_FMT"\r\n", pregctx->REG_ulReg[6]);
        _PrintFormat("R7(S1)  = "LX_FMT"\r\n", pregctx->REG_ulReg[7]);
        _PrintFormat("R8(S2)  = "LX_FMT"\r\n", pregctx->REG_ulReg[8]);
        _PrintFormat("R9(S3)  = "LX_FMT"\r\n", pregctx->REG_ulReg[9]);
        _PrintFormat("R10(S4) = "LX_FMT"\r\n", pregctx->REG_ulReg[10]);
        _PrintFormat("R11(S5) = "LX_FMT"\r\n", pregctx->REG_ulReg[11]);
        _PrintFormat("R12(S6) = "LX_FMT"\r\n", pregctx->REG_ulReg[12]);
        _PrintFormat("R13(S7) = "LX_FMT"\r\n", pregctx->REG_ulReg[13]);
        _PrintFormat("R14(S8) = "LX_FMT"\r\n", pregctx->REG_ulReg[14]);
#endif                                                                  /*  __CSKYABIV2__               */
        
        _PrintFormat("R15     = "LX_FMT"\r\n", pregctx->REG_ulReg[15]);
        _PrintFormat("R16     = "LX_FMT"\r\n", pregctx->REG_ulReg[16]);
        _PrintFormat("R17     = "LX_FMT"\r\n", pregctx->REG_ulReg[17]);
        _PrintFormat("R18     = "LX_FMT"\r\n", pregctx->REG_ulReg[18]);
        _PrintFormat("R19     = "LX_FMT"\r\n", pregctx->REG_ulReg[19]);
        _PrintFormat("R20     = "LX_FMT"\r\n", pregctx->REG_ulReg[20]);
        _PrintFormat("R21     = "LX_FMT"\r\n", pregctx->REG_ulReg[21]);
        _PrintFormat("R22     = "LX_FMT"\r\n", pregctx->REG_ulReg[22]);
        _PrintFormat("R23     = "LX_FMT"\r\n", pregctx->REG_ulReg[23]);
        _PrintFormat("R24     = "LX_FMT"\r\n", pregctx->REG_ulReg[24]);
        _PrintFormat("R25     = "LX_FMT"\r\n", pregctx->REG_ulReg[25]);
        _PrintFormat("R26     = "LX_FMT"\r\n", pregctx->REG_ulReg[26]);
        _PrintFormat("R27     = "LX_FMT"\r\n", pregctx->REG_ulReg[27]);
        _PrintFormat("R28     = "LX_FMT"\r\n", pregctx->REG_ulReg[28]);
        _PrintFormat("R29     = "LX_FMT"\r\n", pregctx->REG_ulReg[29]);
        _PrintFormat("R30     = "LX_FMT"\r\n", pregctx->REG_ulReg[30]);
        _PrintFormat("R31     = "LX_FMT"\r\n", pregctx->REG_ulReg[31]);
        
        _PrintFormat("PSR Status Register:\r\n");
        _PrintFormat("S   = %d  ",   (ulPsr & M_PSR_S)  >> S_PSR_S);
        _PrintFormat("TE  = %d\r\n", (ulPsr & M_PSR_TE) >> S_PSR_TE);
        _PrintFormat("MM  = %d  ",   (ulPsr & M_PSR_MM) >> S_PSR_MM);
        _PrintFormat("EE  = %d\r\n", (ulPsr & M_PSR_EE) >> S_PSR_EE);
        _PrintFormat("IC  = %d  ",   (ulPsr & M_PSR_IC) >> S_PSR_IC);
        _PrintFormat("IE  = %d\r\n", (ulPsr & M_PSR_IE) >> S_PSR_IE);
        _PrintFormat("FE  = %d  ",   (ulPsr & M_PSR_FE) >> S_PSR_FE);
        _PrintFormat("AF  = %d\r\n", (ulPsr & M_PSR_AF) >> S_PSR_AF);
        _PrintFormat("C   = %d\r\n", (ulPsr & M_PSR_C)  >> S_PSR_C);

#undef LX_FMT
    }
}
/*********************************************************************************************************
** 函数名称: archIntCtxSaveReg
** 功能描述: 中断保存寄存器
** 输　入  : pcpu      CPU 结构
**           reg0      寄存器 0
**           reg1      寄存器 1
**           reg2      寄存器 2
**           reg3      寄存器 3
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: archCtxStackEnd
** 功能描述: 根据寄存器上下文获得栈结束地址
** 输　入  : pregctx    寄存器上下文
** 输　出  : 栈结束地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_STACK  archCtxStackEnd (const ARCH_REG_CTX  *pregctx)
{
    return  ((PLW_STACK)pregctx->REG_ulReg[REG_SP]);
}

#endif                                                                  /*  !__SYLIXOS_CSKY_ARCH_CK803__*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
