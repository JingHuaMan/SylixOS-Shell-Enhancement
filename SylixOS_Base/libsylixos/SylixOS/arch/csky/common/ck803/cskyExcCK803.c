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
** 文   件   名: cskyExcCK803.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 11 月 12 日
**
** 描        述: C-SKY CK803 体系架构异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/csky/common/unaligned/cskyUnaligned.h"
#if LW_CFG_CPU_FPU_EN > 0
#include "arch/csky/fpu/fpu/cskyVfp.h"
#endif
#include "arch/csky/param/cskyParam.h"
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  C-SKY 体系架构
*********************************************************************************************************/
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif
/*********************************************************************************************************
** 函数名称: archIntHandle
** 功能描述: bspIntHandle 需要调用此函数处理中断 (关闭中断情况被调用)
** 输　入  : ulVector         中断向量
**           bPreemptive      中断是否可抢占
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  向量号不正确                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  允许中断                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  调用中断服务程序            */

    KN_INT_DISABLE();                                                   /*  禁能中断                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  允许 vector 中断            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
** 函数名称: archAutoIntHandle
** 功能描述: 普通中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archAutoIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(ulVector);
}
/*********************************************************************************************************
** 函数名称: archTrapExceptHandle
** 功能描述: 跟踪异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrapExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrap0ExceptHandle
** 功能描述: 陷阱指令异常 0
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap0ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap1ExceptHandle
** 功能描述: 陷阱指令异常 1
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap1ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap2ExceptHandle
** 功能描述: 陷阱指令异常 2
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap2ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap3ExceptHandle
** 功能描述: 陷阱指令异常 3
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap3ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archReservedExceptHandle
** 功能描述: Reserved 中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archReservedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTspendIntHandle
** 功能描述: TSPEND 中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTspendIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(ulVector);
}
/*********************************************************************************************************
** 函数名称: archFpuExceptHandle
** 功能描述: 浮点异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_CPU_FPU_EN > 0
    UINT32         uiFESR;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;                       /*  终止类型默认为浮点异常      */

#if LW_CFG_CPU_FPU_EN > 0
    uiFESR = cskyVfpGetFESR();
    if (uiFESR & FESR_ILLE) {                                           /*  非法指令                    */
        abtInfo.VMABT_uiMethod = ILL_ILLOPC;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;

    } else if (uiFESR & FESR_IDC) {                                     /*  非规格数输入                */
        abtInfo.VMABT_uiMethod = ILL_ILLOPN;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;

    } else if (uiFESR & FESR_FEC) {
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;

        if (uiFESR & FESR_IOC) {
            abtInfo.VMABT_uiMethod = FPE_FLTINV;                        /*  非法操作异常                */

        } else if (uiFESR & FESR_DZC) {
            abtInfo.VMABT_uiMethod = FPE_FLTDIV;                        /*  除零异常                    */

        }  else if (uiFESR & FESR_UFC) {
            abtInfo.VMABT_uiMethod = FPE_FLTUND;                        /*  下溢异常                    */

        } else if (uiFESR & FESR_OFC) {
            abtInfo.VMABT_uiMethod = FPE_FLTOVF;                        /*  上溢异常                    */

        } else if (uiFESR & FESR_IXC) {
            abtInfo.VMABT_uiMethod = FPE_FLTRES;                        /*  不精确异常                  */

        } else {
            abtInfo.VMABT_uiMethod = 0;
        }

    } else {
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archIdlyExceptHandle
** 功能描述: Idly 异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archIdlyExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archFatalErrExceptHandle
** 功能描述: 不可恢复错误异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFatalErrExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archBreakPointExceptHandle
** 功能描述: 断点异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archBreakPointExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(pregctx->REG_ulPc, LW_NULL);             /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(pregctx->REG_ulPc, uiBpType) == ERROR_NONE) {
            return;                                                     /*  进入调试接口断点处理        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archPrivilegeExceptHandle
** 功能描述: 特权违反异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archPrivilegeExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archIllegalInstExceptHandle
** 功能描述: 非法指令异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archIllegalInstExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDivideZeroExceptHandle
** 功能描述: 除以零异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDivideZeroExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archAccessExceptHandle
** 功能描述: 访问错误异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archAccessExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;

    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archUnalignedHandle
** 功能描述: 非对齐内存访问异常处理
** 输　入  : pabtctx    abort 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archUnalignedHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    cskyUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                        &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** 函数名称: archUnalignedExceptHandle
** 功能描述: 未对齐访问异常处理
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archUnalignedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    CSKY_PARAM    *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    if (param->CP_bUnalign) {
        API_VmmAbortIsrEx(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur, archUnalignedHandle);

    } else {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
    }
}

#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
/*********************************************************************************************************
  END
*********************************************************************************************************/
