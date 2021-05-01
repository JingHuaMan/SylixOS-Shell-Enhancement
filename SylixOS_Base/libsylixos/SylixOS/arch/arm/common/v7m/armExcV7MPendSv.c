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
** 文   件   名: armExcV7MPendSv.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 11 月 14 日
**
** 描        述: ARMv7M 体系构架异常处理(PendSV 方式任务切换).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__) && (LW_CFG_CORTEX_M_SVC_SWITCH == 0)
#include "armExcV7M.h"
/*********************************************************************************************************
** 函数名称: armv7mIntHandle
** 功能描述: 中断处理
** 输　入  : uiVector  中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mIntHandle (UINT32  uiVector)
{
    KN_INT_ENABLE_FORCE();                                              /*  打开中断, 允许嵌套          */

    API_InterVectorIsr((ULONG)uiVector);

    KN_INT_DISABLE();                                                   /*  禁能中断                    */
}
/*********************************************************************************************************
** 函数名称: armv7mNMIIntHandle
** 功能描述: NMI 处理
** 输　入  : uiVector  中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armv7mNMIIntHandle (UINT32  uiVector)
{
    armv7mIntHandle(uiVector);
}
/*********************************************************************************************************
** 函数名称: armv7mSysTickIntHandle
** 功能描述: SysTick 中断处理
** 输　入  : uiVector  中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mSysTickIntHandle (UINT32  uiVector)
{
    armv7mIntHandle(uiVector);
}
/*********************************************************************************************************
** 函数名称: armv7mSvcHandle
** 功能描述: SVC 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mSvcHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  关键性错误                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "SVC");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  直接重新启动操作系统        */
}
/*********************************************************************************************************
** 函数名称: armv7mDebugMonitorHandle
** 功能描述: Debug Monitor 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mDebugMonitorHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  关键性错误                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "DebugMonitor");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  直接重新启动操作系统        */
}
/*********************************************************************************************************
** 函数名称: armv7mReservedIntHandle
** 功能描述: Reserved 中断处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mReservedIntHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  关键性错误                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "ReservedInt");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  直接重新启动操作系统        */
}
/*********************************************************************************************************
** 函数名称: armv7mFaultCommonHandle
** 功能描述: Common routine for high-level exception handlers.
** 输　入  : ulRetAddr 返回地址
**           ptcbCur   当前任务控制块
**           in        异常号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armv7mFaultCommonHandle (addr_t          ulRetAddr,
                                      PLW_CLASS_TCB   ptcbCur,
                                      INTERRUPTS      in)
{
    UINT32  uiHStatus;
    UINT32  uiLStatus;
    addr_t  ulAbortAddr;

    uiHStatus = read32((addr_t)&SCB->HFSR);
    uiLStatus = read32((addr_t)&SCB->CFSR);

    if (uiLStatus & BFARVALID && (in == BUSFAULT ||
        (in == HARDFAULT && uiHStatus & FORCED))) {
        ulAbortAddr = read32((addr_t)&SCB->BFAR);

    } else {
        ulAbortAddr = ulRetAddr;
    }

    write32(uiHStatus, (addr_t)&SCB->HFSR);
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  关键性错误                  */
                 "FATAL ERROR: exception in thread %lx[%s].\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName);

    armv7mFaultPrintInfo(in, ulAbortAddr, uiHStatus, uiLStatus);

    _PrintFormat("rebooting...\r\n");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  直接重新启动操作系统        */
}
/*********************************************************************************************************
** 函数名称: armv7mHardFaultHandle
** 功能描述: Hard Fault 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mHardFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, HARDFAULT);
}
/*********************************************************************************************************
** 函数名称: armv7mBusFaultHandle
** 功能描述: Bus Fault 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mBusFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, BUSFAULT);
}
/*********************************************************************************************************
** 函数名称: armv7mUsageFaultHandle
** 功能描述: Usage Fault 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mUsageFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, USAGEFAULT);
}
/*********************************************************************************************************
** 函数名称: armv7mMemFaultHandle
** 功能描述: Mem Fault 处理
** 输　入  : uiVector  中断向量
**           ulRetAddr 返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mMemFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    UINT32          uiLStatus;
    addr_t          ulAbortAddr;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * Read the fault status register from the MPU hardware
     */
    uiLStatus = read32((addr_t)&SCB->CFSR);

    /*
     * Clean up the memory fault status register for a next exception
     */
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    if ((uiLStatus & 0xf0) == 0x80) {
        /*
         * Did we get a valid address in the memory fault address register?
         * If so, this is a data access failure (can't tell read or write).
         */
        ulAbortAddr = read32((addr_t)&SCB->MMFAR);

    } else if (uiLStatus & 0x1) {
        /*
         * If there is no valid address, did we get an instuction access
         * failure? If so, this is a code access failure.
         */
        ulAbortAddr = ulRetAddr;

    } else {
        /*
         * Some error bit set in the memory fault address register.
         * This must be MUNSTKERR due to a stacking error, which
         * implies that we have gone beyond the low stack boundary
         * (or somehow SP got incorrect).
         * There is no recovery from that since no registers
         * were saved on the stack on this exception, that is,
         * we have no PC saved to return to user mode.
         */
        ulAbortAddr = (addr_t)-1;
    }

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  关键性错误                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulAbortAddr, "MemFault");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  直接重新启动操作系统        */
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
