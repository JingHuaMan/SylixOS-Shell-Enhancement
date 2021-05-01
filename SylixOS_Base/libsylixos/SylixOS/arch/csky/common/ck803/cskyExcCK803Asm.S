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
;** 文   件   名: cskyExcCK803Asm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 11 月 12 日
;**
;** 描        述: C-SKY CK803 体系架构异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

#if defined(__SYLIXOS_CSKY_ARCH_CK803__)

#include "cskyContextCK803Asm.h"

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    IMPORT_LABEL(bspIntHandle)

    IMPORT_LABEL(archResetHandle)
    IMPORT_LABEL(archUnalignedExceptHandle)
    IMPORT_LABEL(archAccessExceptHandle)
    IMPORT_LABEL(archDivideZeroExceptHandle)
    IMPORT_LABEL(archIllegalInstExceptHandle)
    IMPORT_LABEL(archPrivilegeExceptHandle)
    IMPORT_LABEL(archTrapExceptHandle)
    IMPORT_LABEL(archBreakPointExceptHandle)
    IMPORT_LABEL(archFatalErrExceptHandle)
    IMPORT_LABEL(archIdlyExceptHandle)
    IMPORT_LABEL(archAutoIntHandle)
    IMPORT_LABEL(archReservedExceptHandle)
    IMPORT_LABEL(archTrap0ExceptHandle)
    IMPORT_LABEL(archTrap1ExceptHandle)
    IMPORT_LABEL(archTrap2ExceptHandle)
    IMPORT_LABEL(archTrap3ExceptHandle)
    IMPORT_LABEL(archTspendIntHandle)
    IMPORT_LABEL(archFpuExceptHandle)
    IMPORT_LABEL(archExcOldStack)
    IMPORT_LABEL(archExcIrqStack)
    
    EXPORT_LABEL(archUnalignedExceptEntry)
    EXPORT_LABEL(archAccessExceptEntry)
    EXPORT_LABEL(archDivideZeroExceptEntry)
    EXPORT_LABEL(archIllegalInstExceptEntry)
    EXPORT_LABEL(archPrivilegeExceptEntry)
    EXPORT_LABEL(archTrapExceptEntry)
    EXPORT_LABEL(archBreakPointExceptEntry)
    EXPORT_LABEL(archFatalErrExceptEntry)
    EXPORT_LABEL(archIdlyExceptEntry)
    EXPORT_LABEL(archAutoIntEntry)
    EXPORT_LABEL(archReservedExceptEntry)
    EXPORT_LABEL(archTrap0ExceptEntry)
    EXPORT_LABEL(archTrap1ExceptEntry)
    EXPORT_LABEL(archTrap2ExceptEntry)
    EXPORT_LABEL(archTrap3ExceptEntry)
    EXPORT_LABEL(archTspendIntEntry)
    EXPORT_LABEL(archFpuExceptEntry)

    EXPORT_LABEL(vector)

;/*********************************************************************************************************
;  这里的异常向量表 vector 声明为弱符号, BSP 可以重新实现异常向量表
;*********************************************************************************************************/

    WEAK(vector)

;/*********************************************************************************************************
;  异常/中断入口宏
;*********************************************************************************************************/

    SECTION(.text)

MACRO_DEF(IRQ_ENTRY  irq  handle)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将寄存器保存到临时上下文保存区
    ; */
    PUSH        R4 , R5                                                 ;/*  先将 R4, R5 入栈            */
    LRW         R4 , archExcOldStack                                    ;/*  获取存储异常前 SP 的位置    */
    ADDI        R5 , SP , 2 * ARCH_REG_SIZE                             ;/*  获取异常前 SP 指针          */
    ST.W        R5 , (R4)                                               ;/*  将异常前 SP 存入对应位置    */
    LD.W        R4 , (SP)                                               ;/*  不能使用 POP 指令           */
    LD.W        R5 , (SP, ARCH_REG_SIZE)                                ;/*  POP 指令会同时改变 PC       */

    LRW         SP , archExcIrqStack                                    ;/*  获取异常临时栈 SP 的位置    */
    LD.W        SP , (SP)                                               ;/*  得到异常栈指针              */
    SUBI        SP , ARCH_REG_CTX_SIZE                                  ;/*  在临时堆栈开辟上下文保存区  */

    STM         R0-R15 , (SP)
    ST.W        R28 , (SP , XGREG28)

    MFCR        R0 , EPC                                                ;/*  EPC 代替 PC 保存            */
    STW         R0 , (SP , XPC)

    MFCR        R0 , EPSR                                               ;/*  保存 PSR 寄存器             */
    ST.W        R0 , (SP , XPSR)

    LRW         R0 , archExcOldStack                                    ;/*  读出异常前 SP               */
    LD.W        R0 , (R0)
    ST.W        R0 , (SP , XGREG(14))                                   ;/*  保存异常前 SP               */

    ;/*
    ; * API_InterEnter(寄存器上下文)
    ; */
    MOV         A0 , SP
    JSRI        API_InterEnter

    CMPNEI      A0 , 1
    BT          3f

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    JSRI        API_InterStackBaseGet
    MOV         SP , A0

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    JSRI        API_ThreadTcbInter
    MOV         A1 , A0

2:
    PSRSET      EE

    ;/*
    ; * handle(vector, 寄存器上下文)
    ; */
    MOVI        A0 , \irq

    JSRI        \handle

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数
    ; */
    JSRI        API_InterExit

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV         A1 , SP
    RESTORE_REGS

3:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LRW         SP , archExcOldStack
    LD.W        SP , (SP)                                               ;/*  获取异常前 SP 的位置        */
    SUBI        SP , ARCH_REG_CTX_SIZE
    MOV         A1 , SP
    BR          2b
    MACRO_END()

;/*********************************************************************************************************
;  未对齐访问异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archUnalignedExceptEntry)
    IRQ_ENTRY   1   archUnalignedExceptHandle
    FUNC_END(archUnalignedExceptEntry)

;/*********************************************************************************************************
;  访问错误异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archAccessExceptEntry)
    IRQ_ENTRY   2   archAccessExceptHandle
    FUNC_END(archAccessExceptEntry)

;/*********************************************************************************************************
;  除零异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archDivideZeroExceptEntry)
    IRQ_ENTRY   3   archDivideZeroExceptHandle
    FUNC_END(archDivideZeroExceptEntry)

;/*********************************************************************************************************
;  非法指令异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archIllegalInstExceptEntry)
    IRQ_ENTRY   4   archIllegalInstExceptHandle
    FUNC_END(archIllegalInstExceptEntry)

;/*********************************************************************************************************
;  特权违反异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archPrivilegeExceptEntry)
    IRQ_ENTRY   5   archPrivilegeExceptHandle
    FUNC_END(archPrivilegeExceptEntry)

;/*********************************************************************************************************
;  跟踪异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archTrapExceptEntry)
    IRQ_ENTRY   6   archTrapExceptHandle
    FUNC_END(archTrapExceptEntry)

;/*********************************************************************************************************
;  断点异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archBreakPointExceptEntry)
    IRQ_ENTRY   7   archBreakPointExceptHandle
    FUNC_END(archBreakPointExceptEntry)

;/*********************************************************************************************************
;  不可恢复错误异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archFatalErrExceptEntry)
    IRQ_ENTRY   8   archFatalErrExceptHandle
    FUNC_END(archFatalErrExceptEntry)

;/*********************************************************************************************************
;  Idly 异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archIdlyExceptEntry)
    IRQ_ENTRY   9   archIdlyExceptHandle
    FUNC_END(archIdlyExceptEntry)

;/*********************************************************************************************************
;  普通中断入口函数
;*********************************************************************************************************/

FUNC_DEF(archAutoIntEntry)
    IRQ_ENTRY   10  archAutoIntHandle
    FUNC_END(archAutoIntEntry)

;/*********************************************************************************************************
;  Reserved 异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archReservedExceptEntry)
    IRQ_ENTRY   12  archReservedExceptHandle
    FUNC_END(archReservedExceptEntry)

;/*********************************************************************************************************
;  陷阱指令异常 0 入口函数
;*********************************************************************************************************/

FUNC_DEF(archTrap0ExceptEntry)
    IRQ_ENTRY   16  archTrap0ExceptHandle
    FUNC_END(archTrap0ExceptEntry)

;/*********************************************************************************************************
;  陷阱指令异常 1 入口函数
;*********************************************************************************************************/

FUNC_DEF(archTrap1ExceptEntry)
    IRQ_ENTRY   17  archTrap1ExceptHandle
    FUNC_END(archTrap1ExceptEntry)

;/*********************************************************************************************************
;  陷阱指令异常 2 入口函数
;*********************************************************************************************************/

FUNC_DEF(archTrap2ExceptEntry)
    IRQ_ENTRY   18  archTrap2ExceptHandle
    FUNC_END(archTrap2ExceptEntry)

;/*********************************************************************************************************
;  陷阱指令异常 3 入口函数
;*********************************************************************************************************/

FUNC_DEF(archTrap3ExceptEntry)
    IRQ_ENTRY   19  archTrap3ExceptHandle
    FUNC_END(archTrap3ExceptEntry)

;/*********************************************************************************************************
;  TSPEND 中断入口函数
;*********************************************************************************************************/

FUNC_DEF(archTspendIntEntry)
    IRQ_ENTRY   22  archTspendIntHandle
    FUNC_END(archTspendIntEntry)

;/*********************************************************************************************************
;  浮点运算异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archFpuExceptEntry)
    IRQ_ENTRY   30  archFpuExceptHandle
    FUNC_END(archFpuExceptEntry)

;/*********************************************************************************************************
;  32 ~ 255 号中断入口函数
;*********************************************************************************************************/

    .irp        Id,                                   \
               32,  33,  34,  35,  36,  37,  38,  39, \
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49, \
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59, \
     60,  61,  62,  63,  64,  65,  66,  67,  68,  69, \
     70,  71,  72,  73,  74,  75,  76,  77,  78,  79, \
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89, \
     90,  91,  92,  93,  94,  95,  96,  97,  98,  99, \
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, \
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, \
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139, \
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, \
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, \
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, \
    170, 171, 172, 173, 174, 175, 176, 177, 178, 179, \
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, \
    190, 191, 192, 193, 194, 195, 196, 197, 198, 199, \
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, \
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, \
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, \
    230, 231, 232, 233, 234, 235, 236, 237, 238, 239, \
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255
    EXPORT_LABEL(archIntEntry\Id)
FUNC_DEF(archIntEntry\Id)
    IRQ_ENTRY   \Id  bspIntHandle
    FUNC_END(archIntEntry\Id)
    .endr

;/*********************************************************************************************************
;  异常向量表
;*********************************************************************************************************/

    .align  10
FUNC_DEF(vector)
    .long   archResetEntry                                              ;/*  0  - Reset                  */
    .long   archUnalignedExceptEntry                                    ;/*  1  - Unaligned              */
    .long   archAccessExceptEntry                                       ;/*  2  - Access                 */
    .long   archDivideZeroExceptEntry                                   ;/*  3  - Divide By Zero         */
    .long   archIllegalInstExceptEntry                                  ;/*  4  - Illegal Instruction    */
    .long   archPrivilegeExceptEntry                                    ;/*  5  - Privilege              */
    .long   archTrapExceptEntry                                         ;/*  6  - Trap                   */
    .long   archBreakPointExceptEntry                                   ;/*  7  - BreakPoint             */
    .long   archFatalErrExceptEntry                                     ;/*  8  - Fatal Err              */
    .long   archIdlyExceptEntry                                         ;/*  9  - Idly                   */
    .long   archAutoIntEntry                                            ;/*  10 - Auto                   */
    .long   archReservedExceptEntry                                     ;/*  11 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  12 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  13 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  14 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  15 - Reserved               */
    .long   archTrap0ExceptEntry                                        ;/*  16 - Trap0                  */
    .long   archTrap1ExceptEntry                                        ;/*  17 - Trap1                  */
    .long   archTrap2ExceptEntry                                        ;/*  18 - Trap2                  */
    .long   archTrap3ExceptEntry                                        ;/*  19 - Trap3                  */
    .long   archReservedExceptEntry                                     ;/*  20 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  21 - Reserved               */
    .long   archTspendIntEntry                                          ;/*  22 - TSPEND                 */
    .long   archReservedExceptEntry                                     ;/*  23 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  24 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  25 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  26 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  27 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  28 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  29 - Reserved               */
    .long   archFpuExceptEntry                                          ;/*  30 - FPU                    */
    .long   archReservedExceptEntry                                     ;/*  31 - Reserved               */

    .irp    Id,                                       \
               32,  33,  34,  35,  36,  37,  38,  39, \
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49, \
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59, \
     60,  61,  62,  63,  64,  65,  66,  67,  68,  69, \
     70,  71,  72,  73,  74,  75,  76,  77,  78,  79, \
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89, \
     90,  91,  92,  93,  94,  95,  96,  97,  98,  99, \
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, \
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, \
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139, \
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, \
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, \
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, \
    170, 171, 172, 173, 174, 175, 176, 177, 178, 179, \
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, \
    190, 191, 192, 193, 194, 195, 196, 197, 198, 199, \
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, \
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, \
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, \
    230, 231, 232, 233, 234, 235, 236, 237, 238, 239, \
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255
    .long   archIntEntry\Id
    .endr
    FUNC_END(vector)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
