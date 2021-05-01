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
;** 文   件   名: cskyExcAsm.S
;**
;** 创   建   人: Hui.Kai (惠凯)
;**
;** 文件创建日期: 2018 年 05 月 11 日
;**
;** 描        述: C-SKY 体系架构异常处理.
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
    IMPORT_LABEL(archFastAutoIntHandle)
    IMPORT_LABEL(archReservedExceptHandle)
    IMPORT_LABEL(archTlbFatalExceptHandle)
    IMPORT_LABEL(archTlbModExceptHandle)
    IMPORT_LABEL(archTrap0ExceptHandle)
    IMPORT_LABEL(archTrap1ExceptHandle)
    IMPORT_LABEL(archTrap2ExceptHandle)
    IMPORT_LABEL(archTrap3ExceptHandle)
    IMPORT_LABEL(archTlbLoadExceptHandle)
    IMPORT_LABEL(archTlbStoreExceptHandle)
    
#if LW_CFG_VMM_EN > 0
    IMPORT_LABEL(cskyMmuTlbRefillExceptEntry)
#else
    IMPORT_LABEL(archTlbRefillExceptHandle)
    EXPORT_LABEL(archTlbRefillExceptEntry)
#endif

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
    EXPORT_LABEL(archFastAutoIntEntry)
    EXPORT_LABEL(archReservedExceptEntry)
    EXPORT_LABEL(archTlbFatalExceptEntry)
    EXPORT_LABEL(archTlbModExceptEntry)
    EXPORT_LABEL(archTrap0ExceptEntry)
    EXPORT_LABEL(archTrap1ExceptEntry)
    EXPORT_LABEL(archTrap2ExceptEntry)
    EXPORT_LABEL(archTrap3ExceptEntry)
    EXPORT_LABEL(archTlbLoadExceptEntry)
    EXPORT_LABEL(archTlbStoreExceptEntry)

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
    MTCR        SP , SS1                                                ;/*  SS1 暂存异常前 SP           */
    MFCR        SP , SS0                                                ;/*  SP = 异常临时栈             */

    SUBI        SP , ARCH_REG_CTX_SIZE                                  ;/*  在临时堆栈开辟上下文保存区  */

    STM         R0-R31 , (SP)

    MFCR        R18 , EPC                                               ;/*  EPC 代替 PC 保存            */
    ST.W        R18 , (SP , XPC)

    MFCR        R18 , EPSR                                              ;/*  EPSR 代替 PSR 保存          */
    ST.W        R18 , (SP , XPSR)

    MFLO        R18                                                     ;/*  保存 LO 寄存器              */
    ST.W        R18 , (SP , XLO)

    MFHI        R18                                                     ;/*  保存 HI 寄存器              */
    ST.W        R18 , (SP , XHI)

    MFCR        R18 , CR<4, 15>                                         ;/*  保存 MEH 寄存器             */
    ST.W        R18 , (SP , XMEH)

    MFCR        R18 , SS1                                               ;/*  读出异常前 SP               */
#ifdef __CSKYABIV2__
    ST.W        R18 , (SP , XGREG(14))                                  ;/*  保存异常前 SP               */
#else
    ST.W        R18 , (SP , XGREG(0))
#endif

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
    PSRSET      EE                                                      ;/*  使能异常                    */

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
    RESTORE_KERN_REGS

3:
    ;/*
    ; * 不是第一次进入中断
    ; */
    MFCR        SP , SS1                                                ;/*  异常前 SP                   */
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
;  快速中断入口函数
;*********************************************************************************************************/

FUNC_DEF(archFastAutoIntEntry)
    JSRI            archFastAutoIntHandle
    RFI
    FUNC_END(archFastAutoIntEntry)

;/*********************************************************************************************************
;  Reserved 异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archReservedExceptEntry)
    IRQ_ENTRY   12  archReservedExceptHandle
    FUNC_END(archReservedExceptEntry)

;/*********************************************************************************************************
;  TLB 不可恢复异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archTlbFatalExceptEntry)
    IRQ_ENTRY   13  archTlbFatalExceptHandle
    FUNC_END(archTlbFatalExceptEntry)

;/*********************************************************************************************************
;  TLB 重填异常入口函数
;*********************************************************************************************************/

#if LW_CFG_VMM_EN == 0
FUNC_DEF(archTlbRefillExceptEntry)
    IRQ_ENTRY   14  archTlbRefillExceptHandle
    FUNC_END(archTlbRefillExceptEntry)
#endif

;/*********************************************************************************************************
;  TLB 修改异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archTlbModExceptEntry)
    IRQ_ENTRY   15  archTlbModExceptHandle
    FUNC_END(archTlbModExceptEntry)

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
;  TLB 读无效异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archTlbLoadExceptEntry)
    IRQ_ENTRY   20  archTlbLoadExceptHandle
    FUNC_END(archTlbLoadExceptEntry)

;/*********************************************************************************************************
;  TLB 写无效异常入口函数
;*********************************************************************************************************/

FUNC_DEF(archTlbStoreExceptEntry)
    IRQ_ENTRY   21  archTlbStoreExceptHandle
    FUNC_END(archTlbStoreExceptEntry)

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
    .long   archFastAutoIntEntry + 0x1                                  ;/*  11 - Fast Auto              */
    .long   archReservedExceptEntry                                     ;/*  12 - Reserved               */
    .long   archTlbFatalExceptEntry                                     ;/*  13 - TLB Fatal              */
#if LW_CFG_VMM_EN > 0
    .long   cskyMmuTlbRefillExceptEntry + 0x1                           ;/*  14 - TLB Refill             */
#else
    .long   archTlbRefillExceptEntry                                    ;/*  14 - TLB Refill             */
#endif
    .long   archTlbModExceptEntry                                       ;/*  15 - TLB Modified Fault     */
    .long   archTrap0ExceptEntry                                        ;/*  16 - Trap0                  */
    .long   archTrap1ExceptEntry                                        ;/*  17 - Trap1                  */
    .long   archTrap2ExceptEntry                                        ;/*  18 - Trap2                  */
    .long   archTrap3ExceptEntry                                        ;/*  19 - Trap3                  */
    .long   archTlbLoadExceptEntry                                      ;/*  20 - TLB Load               */
    .long   archTlbStoreExceptEntry                                     ;/*  21 - TLB Store              */
    .long   archReservedExceptEntry                                     ;/*  22 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  23 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  24 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  25 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  26 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  27 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  28 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  29 - Reserved               */
    .long   archReservedExceptEntry                                     ;/*  30 - Reserved               */
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
