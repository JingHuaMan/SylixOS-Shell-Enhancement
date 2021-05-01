;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: arm64ContextAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 06 月 23 日
;**
;** 描        述: ARM64 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/cpu/cpu_cfg.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

#include "arm64ContextAsm.h"

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif
    
    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
    EXPORT_LABEL(archTaskCtxCopy)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LDR     X18, [X0]                                                   ;/*  获取当前 TCB 的 REG_CTX 地址*/

#if LW_CFG_ARM64_HW_STEP_EN > 0
    MOV     X0,        #0
    MSR     OSLAR_EL1, X0                                               ;/*  解锁 OS Lock                */
    MOV     X0 ,       MDSCR_EL1_KDE_EN | MDSCR_EL1_SS_EN
    MSR     MDSCR_EL1, X0                                               ;/*  使能 Software Step 功能     */
#endif

LINE_LABEL(archTaskCtxLoad)
    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    LDR     X18, [X0]                                                   ;/*  获取当前 TCB 的 REG_CTX 地址*/
    
    SAVE_SMALL_REG_CTX                                                  ;/*  保存小寄存器上下文          */

    MOV     X19 , X0                                                    ;/*  X19 暂存 X0                 */

#if LW_CFG_SMP_EN > 0    
    BL      _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    MOV     SP , X0                                                     ;/*  设置 SP                     */
    MOV     X0 , X19                                                    ;/*  恢复 X0                     */
#endif

    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */

    LDR     X18, [X19]                                                  ;/*  获取当前 TCB 的 REG_CTX 地址*/
    LDR     X9 , [X18, #CTX_TYPE_OFFSET]                                ;/*  获得上下文类型              */
    CMP     X9 , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */

LINE_LABEL(_RestoreSmallCtx)
    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    ADD     X1  , X0 , #16
    LDR     X18 , [X1]                                                  ;/*  获取当前 CCB 的 REG_CTX 地址*/

    SAVE_SMALL_REG_CTX                                                  ;/*  保存小寄存器上下文          */

    MOV     X19 , X0                                                    ;/*  X19 暂存 X0                 */
    BL      _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    MOV     X0  , X19

    ADD     X1  , X0 , #16
    LDR     X18 , [X1]                                                  ;/*  获取当前 CCB 的 REG_CTX 地址*/

    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LDR     X18 , [X0]                                                  ;/*  获取当前 TCB 的 REG_CTX 地址*/
    LDR     X9  , [X18, #CTX_TYPE_OFFSET]                               ;/*  获得上下文类型              */
    CMP     X9  , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  信号上下文返回
;  参数为 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV     X18 , X0
    LDR     X9  , [X18, #CTX_TYPE_OFFSET]                               ;/*  获得上下文类型              */
    CMP     X9  , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  拷贝任务上下文
;  参数 X0 为目的 ARCH_REG_CTX 指针, X1 为源 ARCH_REG_CTX 指针
;  不拷贝 X18 寄存器
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxCopy)
    LDR     X9 ,       [X1 , #CTX_TYPE_OFFSET]
    STR     X9 ,       [X1 , #CTX_TYPE_OFFSET]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(0)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(0)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(2)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(2)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(4)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(4)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(6)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(6)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(8)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(8)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(10)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(10)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(12)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(12)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(14)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(14)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(16)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(16)]

    LDR     X9 , [X1 , #XGREG_OFFSET(19)]
    STR     X9 , [X0 , #XGREG_OFFSET(19)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(20)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(20)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(22)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(22)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(24)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(24)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(26)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(26)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(28)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(28)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(30)]                         ;/*  LR、PC 寄存器               */
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(30)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(32)]                         ;/*  SP、PSTATE 寄存器           */
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(32)]
    
    RET
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
