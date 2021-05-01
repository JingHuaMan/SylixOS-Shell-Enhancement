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
;** 文   件   名: cskyContextAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 05 月 11 日
;**
;** 描        述: C-SKY 体系架构上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)

#include "cskyContextAsm.h"

    FILE_BEGIN()

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
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    SUBI        SP , 1024                                               ;/*  在启动栈开辟异常临时栈      */
    ANDNI       SP , SP , 0x7                                           ;/*  SP 向下 8 字节对齐          */
    MTCR        SP , SS0                                                ;/*  设置异常临时栈(使用启动栈)  */

    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/

    RESTORE_REGS                                                        ;/*  恢复寄存器                  */
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  线程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/

    SAVE_KERN_REGS                                                      ;/*  保存内核态任务寄存器        */

    MOV         S0 , A0                                                 ;/*  S0 暂存 A0                  */

#if LW_CFG_SMP_EN > 0
    JSRI        _SchedSafeStack                                         ;/*  _SchedSafeStack();          */
    MOV         SP , A0
    MOV         A0 , S0                                                 ;/*  恢复 A0                     */
#endif

    JSRI        _SchedSwp                                               ;/*  _SchedSwp();                */
    
    LD.W        A1 , (S0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/
    
    RESTORE_REGS                                                        ;/*  恢复任务寄存器              */
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    LD.W        A1 , (A0 , (2 * PTRSIZE))                               ;/*  当前 CCB 的 REG_CTX 地址    */
    
    SAVE_KERN_REGS                                                      ;/*  保存内核态协程寄存器        */
    
    MOV         S0 , A0                                                 ;/*  S0 暂存 A0                  */

    JSRI        _SchedCrSwp                                             ;/*  _SchedCrSwp();              */
    
    LD.W        A1 , (S0 , (2 * PTRSIZE))                               ;/*  当前 CCB 的 REG_CTX 地址    */

    RESTORE_KERN_REGS                                                   ;/*  恢复内核态协程寄存器        */
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/

    RESTORE_REGS                                                        ;/*  恢复任务寄存器              */
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为 ARCH_REG_CTX 指针, 即 A0 为 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV         A1 , A0
    
    RESTORE_REGS                                                        ;/*  恢复任务寄存器              */
    FUNC_END(archSigCtxLoad)

;/*********************************************************************************************************
;  拷贝任务上下文
;  参数 A0 为目的 ARCH_REG_CTX 指针, A1 为源 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxCopy)
    LDM         R18-R25 , (A1)
    STM         R18-R25 , (A0)
    ADDI        A0 , (8 * ARCH_REG_SIZE)
    ADDI        A1 , (8 * ARCH_REG_SIZE)
    
    LDM         R18-R25 , (A1)
    STM         R18-R25 , (A0)
    ADDI        A0 , (8 * ARCH_REG_SIZE)
    ADDI        A1 , (8 * ARCH_REG_SIZE)
    
    LDM         R18-R25 , (A1)
    STM         R18-R25 , (A0)
    ADDI        A0 , (8 * ARCH_REG_SIZE)
    ADDI        A1 , (8 * ARCH_REG_SIZE)
    
    LDM         R18-R25 , (A1)
    STM         R18-R25 , (A0)
    ADDI        A0 , (8 * ARCH_REG_SIZE)
    ADDI        A1 , (8 * ARCH_REG_SIZE)
    
    LDM         R18-R23 , (A1)
    STM         R18-R23 , (A0)
    RTS
    FUNC_END(archTaskCtxCopy)
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
