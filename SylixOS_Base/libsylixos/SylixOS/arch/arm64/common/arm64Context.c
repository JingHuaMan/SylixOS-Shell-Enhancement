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
** 文   件   名: arm64Context.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 22 日
**
** 描        述: ARM64 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
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
    ARCH_REG_T    ulPstate;
    INT           i;
    
    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, ARCH_STK_ALIGN_SIZE);      /*  堆栈指针向下 16 字节对齐    */
    
    pfpctx  = (ARCH_FP_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    
    pfpctx->FP_ulFp = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_ulLr = (ARCH_REG_T)LW_NULL;
    
    ulPstate  = arm64GetNZCV() |                                        /*  获得当前 NZCV 寄存器        */
                arm64GetDAIF();                                         /*  获得当前 DAIF 寄存器        */
    ulPstate &= ~M_PSTATE_I;                                            /*  使能 IRQ                    */
    pregctx->REG_ulPstate = ulPstate;
    
    /*
     * 初始化寄存器上下文
     */
    for (i = 0; i < ARCH_GREG_NR; i++) {
        pregctx->REG_ulReg[i] = i;
    }
    
    pregctx->REG_ulSmallCtx = 1;                                        /*  小上下文                    */
    pregctx->REG_ulReg[0]   = (ARCH_REG_T)pvArg;
    pregctx->REG_ulLr       = (ARCH_REG_T)pfuncTask;
    pregctx->REG_ulPc       = (ARCH_REG_T)pfuncTask;
    pregctx->REG_ulSp       = (ARCH_REG_T)pfpctx;

#if LW_CFG_ARM64_FAST_TCB_CUR > 0
    pregctx->REG_ulReg[18]  = (ARCH_REG_T)ptcb;                         /*  FAST_TCB_CUR                */
#endif

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
VOID  archTaskCtxSetFp (PLW_STACK               pstkDest,
                        ARCH_REG_CTX           *pregctxDest,
                        const ARCH_REG_CTX     *pregctxSrc)
{
    ARCH_FP_CTX  *pfpctx = (ARCH_FP_CTX *)pstkDest;

    pfpctx->FP_ulFp = pregctxSrc->REG_ulFp;
    pfpctx->FP_ulLr = pregctxSrc->REG_ulPc;

    pregctxDest->REG_ulFp = (ARCH_REG_T)&pfpctx->FP_ulFp;
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
    *pregSp = pregctx->REG_ulSp;

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
    archTaskCtxCopy(pregctxDest, pregctxSrc);
}
/*********************************************************************************************************
** 函数名称: archTaskCtxPstate
** 功能描述: 获得 PSTATE 字符串
** 输　入  : regPstate     PSTATE 状态
             pcPstate      字符串缓冲
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archTaskCtxPstate (ARCH_REG_T  regPstate, PCHAR  pcPstate)
{
    if (regPstate & M_PSTATE_N) {
        pcPstate[0] = 'N';
    } else {
        pcPstate[0] = 'n';
    }
    
    if (regPstate & M_PSTATE_Z) {
        pcPstate[1] = 'Z';
    } else {
        pcPstate[1] = 'z';
    }
    
    if (regPstate & M_PSTATE_C) {
        pcPstate[2] = 'C';
    } else {
        pcPstate[2] = 'c';
    }
    
    if (regPstate & M_PSTATE_V) {
        pcPstate[3] = 'V';
    } else {
        pcPstate[3] = 'v';
    }
    
    if (regPstate & M_PSTATE_D) {
        pcPstate[4] = 'D';
    } else {
        pcPstate[4] = 'd';
    }
    
    if (regPstate & M_PSTATE_A) {
        pcPstate[5] = 'A';
    } else {
        pcPstate[5] = 'a';
    }
    
    if (regPstate & M_PSTATE_I) {
        pcPstate[6] = 'I';
    } else {
        pcPstate[6] = 'i';
    }
    
    if (regPstate & M_PSTATE_F) {
        pcPstate[7] = 'F';
    } else {
        pcPstate[7] = 'f';
    }
    
    pcPstate[8] = 0;
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
    CHAR  acPstate[32 + 1] = "\0";

#define LX_FMT      "0x%016lx  "

    if (iFd >= 0) {
        archTaskCtxPstate(pregctx->REG_ulPstate, acPstate);

        fdprintf(iFd, "PSTATE   = %s\n",       acPstate);
        fdprintf(iFd, "PC       = "LX_FMT"\n", pregctx->REG_ulPc);
        fdprintf(iFd, "SP       = "LX_FMT"\n", pregctx->REG_ulSp);
        fdprintf(iFd, "LR(X30)  = "LX_FMT"\n", pregctx->REG_ulLr);
        fdprintf(iFd, "X0  = "LX_FMT,          pregctx->REG_ulReg[0]);
        fdprintf(iFd, "X1  = "LX_FMT"\n",      pregctx->REG_ulReg[1]);
        fdprintf(iFd, "X2  = "LX_FMT,          pregctx->REG_ulReg[2]);
        fdprintf(iFd, "X3  = "LX_FMT"\n",      pregctx->REG_ulReg[3]);
        fdprintf(iFd, "X4  = "LX_FMT,          pregctx->REG_ulReg[4]);
        fdprintf(iFd, "X5  = "LX_FMT"\n",      pregctx->REG_ulReg[5]);
        fdprintf(iFd, "X6  = "LX_FMT,          pregctx->REG_ulReg[6]);
        fdprintf(iFd, "X7  = "LX_FMT"\n",      pregctx->REG_ulReg[7]);
        fdprintf(iFd, "X8  = "LX_FMT,          pregctx->REG_ulReg[8]);
        fdprintf(iFd, "X9  = "LX_FMT"\n",      pregctx->REG_ulReg[9]);
        fdprintf(iFd, "X10 = "LX_FMT,          pregctx->REG_ulReg[10]);
        fdprintf(iFd, "X11 = "LX_FMT"\n",      pregctx->REG_ulReg[11]);
        fdprintf(iFd, "X12 = "LX_FMT,          pregctx->REG_ulReg[12]);
        fdprintf(iFd, "X13 = "LX_FMT"\n",      pregctx->REG_ulReg[13]);
        fdprintf(iFd, "X14 = "LX_FMT,          pregctx->REG_ulReg[14]);
        fdprintf(iFd, "X15 = "LX_FMT"\n",      pregctx->REG_ulReg[15]);
        fdprintf(iFd, "X16 = "LX_FMT,          pregctx->REG_ulReg[16]);
        fdprintf(iFd, "X17 = "LX_FMT"\n",      pregctx->REG_ulReg[17]);
        fdprintf(iFd, "X18 = "LX_FMT,          pregctx->REG_ulReg[18]);
        fdprintf(iFd, "X19 = "LX_FMT"\n",      pregctx->REG_ulReg[19]);
        fdprintf(iFd, "X20 = "LX_FMT,          pregctx->REG_ulReg[20]);
        fdprintf(iFd, "X21 = "LX_FMT"\n",      pregctx->REG_ulReg[21]);
        fdprintf(iFd, "X22 = "LX_FMT,          pregctx->REG_ulReg[22]);
        fdprintf(iFd, "X23 = "LX_FMT"\n",      pregctx->REG_ulReg[23]);
        fdprintf(iFd, "X24 = "LX_FMT,          pregctx->REG_ulReg[24]);
        fdprintf(iFd, "X25 = "LX_FMT"\n",      pregctx->REG_ulReg[25]);
        fdprintf(iFd, "X26 = "LX_FMT,          pregctx->REG_ulReg[26]);
        fdprintf(iFd, "X27 = "LX_FMT"\n",      pregctx->REG_ulReg[27]);
        fdprintf(iFd, "X28 = "LX_FMT,          pregctx->REG_ulReg[28]);
        fdprintf(iFd, "X29 = "LX_FMT"\n",      pregctx->REG_ulReg[29]);

    } else {
        archTaskCtxPrint(LW_NULL, 0, pregctx);
    }
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
    CHAR  acPstate[32 + 1] = "\0";

    archTaskCtxPstate(pregctx->REG_ulPstate, acPstate);

    if (pvBuffer && stSize) {
        size_t  stOft = 0;
        
        stOft = bnprintf(pvBuffer, stSize, stOft, "PSTATE   = %s\n",       acPstate);
        stOft = bnprintf(pvBuffer, stSize, stOft, "PC       = "LX_FMT"\n", pregctx->REG_ulPc);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SP       = "LX_FMT"\n", pregctx->REG_ulSp);
        stOft = bnprintf(pvBuffer, stSize, stOft, "LR(X30)  = "LX_FMT"\n", pregctx->REG_ulLr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X0  = "LX_FMT,          pregctx->REG_ulReg[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X1  = "LX_FMT"\n",      pregctx->REG_ulReg[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X2  = "LX_FMT,          pregctx->REG_ulReg[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X3  = "LX_FMT"\n",      pregctx->REG_ulReg[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X4  = "LX_FMT,          pregctx->REG_ulReg[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X5  = "LX_FMT"\n",      pregctx->REG_ulReg[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X6  = "LX_FMT,          pregctx->REG_ulReg[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X7  = "LX_FMT"\n",      pregctx->REG_ulReg[7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X8  = "LX_FMT,          pregctx->REG_ulReg[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X9  = "LX_FMT"\n",      pregctx->REG_ulReg[9]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X10 = "LX_FMT,          pregctx->REG_ulReg[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X11 = "LX_FMT"\n",      pregctx->REG_ulReg[11]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X12 = "LX_FMT,          pregctx->REG_ulReg[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X13 = "LX_FMT"\n",      pregctx->REG_ulReg[13]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X14 = "LX_FMT,          pregctx->REG_ulReg[14]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X15 = "LX_FMT"\n",      pregctx->REG_ulReg[15]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X16 = "LX_FMT,          pregctx->REG_ulReg[16]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X17 = "LX_FMT"\n",      pregctx->REG_ulReg[17]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X18 = "LX_FMT,          pregctx->REG_ulReg[18]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X19 = "LX_FMT"\n",      pregctx->REG_ulReg[19]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X20 = "LX_FMT,          pregctx->REG_ulReg[20]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X21 = "LX_FMT"\n",      pregctx->REG_ulReg[21]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X22 = "LX_FMT,          pregctx->REG_ulReg[22]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X23 = "LX_FMT"\n",      pregctx->REG_ulReg[23]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X24 = "LX_FMT,          pregctx->REG_ulReg[24]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X25 = "LX_FMT"\n",      pregctx->REG_ulReg[25]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X26 = "LX_FMT,          pregctx->REG_ulReg[26]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X27 = "LX_FMT"\n",      pregctx->REG_ulReg[27]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X28 = "LX_FMT,          pregctx->REG_ulReg[28]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "X29 = "LX_FMT"\n",      pregctx->REG_ulReg[29]);
    
    } else {
        _PrintFormat("PSTATE   = %s\r\n",       acPstate);
        _PrintFormat("PC       = "LX_FMT"\r\n", pregctx->REG_ulPc);
        _PrintFormat("SP       = "LX_FMT"\r\n", pregctx->REG_ulSp);
        _PrintFormat("LR(X30)  = "LX_FMT"\r\n", pregctx->REG_ulLr);
        _PrintFormat("X0  = "LX_FMT,            pregctx->REG_ulReg[0]);
        _PrintFormat("X1  = "LX_FMT"\r\n",      pregctx->REG_ulReg[1]);
        _PrintFormat("X2  = "LX_FMT,            pregctx->REG_ulReg[2]);
        _PrintFormat("X3  = "LX_FMT"\r\n",      pregctx->REG_ulReg[3]);
        _PrintFormat("X4  = "LX_FMT,            pregctx->REG_ulReg[4]);
        _PrintFormat("X5  = "LX_FMT"\r\n",      pregctx->REG_ulReg[5]);
        _PrintFormat("X6  = "LX_FMT,            pregctx->REG_ulReg[6]);
        _PrintFormat("X7  = "LX_FMT"\r\n",      pregctx->REG_ulReg[7]);
        _PrintFormat("X8  = "LX_FMT,            pregctx->REG_ulReg[8]);
        _PrintFormat("X9  = "LX_FMT"\r\n",      pregctx->REG_ulReg[9]);
        _PrintFormat("X10 = "LX_FMT,            pregctx->REG_ulReg[10]);
        _PrintFormat("X11 = "LX_FMT"\r\n",      pregctx->REG_ulReg[11]);
        _PrintFormat("X12 = "LX_FMT,            pregctx->REG_ulReg[12]);
        _PrintFormat("X13 = "LX_FMT"\r\n",      pregctx->REG_ulReg[13]);
        _PrintFormat("X14 = "LX_FMT,            pregctx->REG_ulReg[14]);
        _PrintFormat("X15 = "LX_FMT"\r\n",      pregctx->REG_ulReg[15]);
        _PrintFormat("X16 = "LX_FMT,            pregctx->REG_ulReg[16]);
        _PrintFormat("X17 = "LX_FMT"\r\n",      pregctx->REG_ulReg[17]);
        _PrintFormat("X18 = "LX_FMT,            pregctx->REG_ulReg[18]);
        _PrintFormat("X19 = "LX_FMT"\r\n",      pregctx->REG_ulReg[19]);
        _PrintFormat("X20 = "LX_FMT,            pregctx->REG_ulReg[20]);
        _PrintFormat("X21 = "LX_FMT"\r\n",      pregctx->REG_ulReg[21]);
        _PrintFormat("X22 = "LX_FMT,            pregctx->REG_ulReg[22]);
        _PrintFormat("X23 = "LX_FMT"\r\n",      pregctx->REG_ulReg[23]);
        _PrintFormat("X24 = "LX_FMT,            pregctx->REG_ulReg[24]);
        _PrintFormat("X25 = "LX_FMT"\r\n",      pregctx->REG_ulReg[25]);
        _PrintFormat("X26 = "LX_FMT,            pregctx->REG_ulReg[26]);
        _PrintFormat("X27 = "LX_FMT"\r\n",      pregctx->REG_ulReg[27]);
        _PrintFormat("X28 = "LX_FMT,            pregctx->REG_ulReg[28]);
        _PrintFormat("X29 = "LX_FMT"\r\n",      pregctx->REG_ulReg[29]);
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
    return  ((PLW_STACK)pregctx->REG_ulSp);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
