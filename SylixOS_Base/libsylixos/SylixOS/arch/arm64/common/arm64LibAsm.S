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
;** 文   件   名: arm64LibAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 06 月 22 日
;**
;** 描        述: ARM64 体系构架内部库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <arch/arm64/arch_def.h>
#include <config/cpu/cpu_cfg.h>

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)
    
    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)
    
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)

    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)

    EXPORT_LABEL(archPageCopy)
    EXPORT_LABEL(archReboot)
    EXPORT_LABEL(archBogoMipsLoop)

    EXPORT_LABEL(arm64GetDAIF)
    EXPORT_LABEL(arm64GetNZCV)
    EXPORT_LABEL(arm64GetCurrentEL)
    EXPORT_LABEL(arm64GetSPSel)
    EXPORT_LABEL(arm64WaitForInterrupt)
    EXPORT_LABEL(arm64EL2SwitchEL1)
    EXPORT_LABEL(arm64EL3SwitchEL2)
    EXPORT_LABEL(arm64SwitchToEl1)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)
    WEAK(arm64EL2SwitchEL1)
    WEAK(arm64EL3SwitchEL2)
    WEAK(arm64SwitchToEl1)

;/*********************************************************************************************************
;  ARM64 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable) 
    MRS     X0 , DAIF
    MSR     DAIFSET , #2
    MRS     X2 , DAIF
    AND     X2 , X2 , #0x80
    CMP     X2 , #0x80
    BNE     archIntDisable
    RET
    FUNC_END()
    
FUNC_DEF(archIntEnable)
    MSR     DAIF , X0
    RET
    FUNC_END()
    
FUNC_DEF(archIntEnableForce)
    MSR     DAIFCLR , #2
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    MOV     W1 , #0
    SUB     W1 , W1 , W0
    AND     W0 , W1 , W0
    MOV     W1 , #32
    CLZ     W0 , W0
    SUB     W0 , W1 , W0
    RET
    FUNC_END()

FUNC_DEF(archFindMsb)
    MOV     W1 , #32
    CLZ     W0 , W0    
    SUB     W0 , W1 , W0
    RET
    FUNC_END()

;/*********************************************************************************************************
;  页面拷贝
;*********************************************************************************************************/
#define PAGE_SZ         4096

FUNC_DEF(archPageCopy)
    STR     X19 , [SP, #-16]!

    PRFM    PLDL1STRM , [X1 , #128]
    PRFM    PLDL1STRM , [X1 , #256]
    
    LDP     X2  ,  X3 , [X1]
    LDP     X4  ,  X5 , [X1 , #16]
    LDP     X6  ,  X7 , [X1 , #32]
    LDP     X8  ,  X9 , [X1 , #48]
    LDP     X10 , X11 , [X1 , #64]
    LDP     X12 , X13 , [X1 , #80]
    LDP     X14 , X15 , [X1 , #96]
    LDP     X16 , X17 , [X1 , #112]

    MOV     X19 , #(PAGE_SZ - 128)
    ADD     X1  , X1  , #128
1:
    SUBS    X19 , X19 , #128

    PRFM    PLDL1STRM , [X1 , #384]

    STNP    X2  ,  X3 , [X0]
    LDP     X2  ,  X3 , [X1]
    STNP    X4  ,  X5 , [X0 , #16]
    LDP     X4  ,  X5 , [X1 , #16]
    STNP    X6  ,  X7 , [X0 , #32]
    LDP     X6  ,  X7 , [X1 , #32]
    STNP    X8  ,  X9 , [X0 , #48]
    LDP     X8  ,  X9 , [X1 , #48]
    STNP    X10 , X11 , [X0 , #64]
    LDP     X10 , X11 , [X1 , #64]
    STNP    X12 , X13 , [X0 , #80]
    LDP     X12 , X13 , [X1 , #80]
    STNP    X14 , X15 , [X0 , #96]
    LDP     X14 , X15 , [X1 , #96]
    STNP    X16 , X17 , [X0 , #112]
    LDP     X16 , X17 , [X1 , #112]

    ADD     X0  , X0  , #128
    ADD     X1  , X1  , #128

    B.GT    1b

    STNP    X2  ,  X3 , [x0]
    STNP    X4  ,  X5 , [X0 , #16]
    STNP    X6  ,  X7 , [X0 , #32]
    STNP    X8  ,  X9 , [X0 , #48]
    STNP    X10 , X11 , [X0 , #64]
    STNP    X12 , X13 , [X0 , #80]
    STNP    X14 , X15 , [X0 , #96]
    STNP    X16 , X17 , [X0 , #112]

    LDR     X19 , [SP], #16
    RET
    FUNC_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    SUB     SP  , SP  , #4 << 3                                         ;/*  确保 16 字节对齐            */
    STP     X0  , X1  , [SP]
    STR     LR  ,       [SP , #2 << 3]
    BL      __sigsetjmpSetup
    LDR     LR  ,       [SP , #2 << 3]
    LDP     X0  , X1  , [SP]
    ADD     SP  , SP  , #4 << 3

    STP     X19 , X20 , [X0 , #0  << 3]
    STP     X21 , X22 , [X0 , #2  << 3]
    STP     X23 , X24 , [X0 , #4  << 3]
    STP     X25 , X26 , [X0 , #6  << 3]
    STP     X27 , X28 , [X0 , #8  << 3]
    STP     X29 , X30 , [X0 , #10 << 3]

    STP     D8  , D9  , [X0 , #14 << 3]
    STP     D10 , D11 , [X0 , #16 << 3]
    STP     D12 , D13 , [X0 , #18 << 3]
    STP     D14 , D15 , [X0 , #20 << 3]

    MOV     X2  , SP
    STR     X2  ,       [X0 , #13 << 3]

    MOV     W0,  #0                                                     ;/*  Return zero.                */

    RET
    FUNC_END()

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    SUB     SP  , SP  , #4 << 3                                         ;/*  确保 16 字节对齐            */
    STP     X0  , X1  , [SP]
    STR     LR  ,       [SP , #2 << 3]
    BL      __setjmpSetup
    LDR     LR  ,       [SP , #2 << 3]
    LDP     X0  , X1  , [SP]
    ADD     SP  , SP  , #4 << 3

    STP     X19 , X20 , [X0 , #0  << 3]
    STP     X21 , X22 , [X0 , #2  << 3]
    STP     X23 , X24 , [X0 , #4  << 3]
    STP     X25 , X26 , [X0 , #6  << 3]
    STP     X27 , X28 , [X0 , #8  << 3]
    STP     X29 , X30 , [X0 , #10 << 3]

    STP     D8  , D9  , [X0 , #14 << 3]
    STP     D10 , D11 , [X0 , #16 << 3]
    STP     D12 , D13 , [X0 , #18 << 3]
    STP     D14 , D15 , [X0 , #20 << 3]

    MOV     X2  , SP
    STR     X2  ,       [X0 , #13 << 3]

    MOV     W0,  #0                                                     ;/*  Return zero.                */

    RET
    FUNC_END()
    
;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    SUB     SP  , SP  , #4 << 3                                         ;/*  确保 16 字节对齐            */
    STP     X0  , X1  , [SP]
    STR     LR  ,       [SP , #2 << 3]
    BL      __siglongjmpSetup
    LDR     LR  ,       [SP , #2 << 3]
    LDP     X0  , X1  , [SP]
    ADD     SP  , SP  , #4 << 3

    LDP     X19 , X20 , [X0 , #0  << 3]
    LDP     X21 , X22 , [X0 , #2  << 3]
    LDP     X23 , X24 , [X0 , #4  << 3]
    LDP     X25 , X26 , [X0 , #6  << 3]
    LDP     X27 , X28 , [X0 , #8  << 3]
    LDP     X29 , X30 , [X0 , #10 << 3]
    LDP     D8  , D9  , [X0 , #14 << 3]
    LDP     D10 , D11 , [X0 , #16 << 3]
    LDP     D12 , D13 , [X0 , #18 << 3]
    LDP     D14 , D15 , [X0 , #20 << 3]

    LDR     X5  , [X0 , #13 << 3]
    MOV     SP  , X5

    CMP     X1  , #0
    MOV     X0  , #1
    CSEL    X0  , X1  , X0 , NE
    BR      X30
    FUNC_END()

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    SUB     SP  , SP  , #4 << 3                                         ;/*  确保 16 字节对齐            */
    STP     X0  , X1  , [SP]
    STR     LR  ,       [SP , #2 << 3]
    BL      __longjmpSetup
    LDR     LR  ,       [SP , #2 << 3]
    LDP     X0  , X1  , [SP]
    ADD     SP  , SP  , #4 << 3

    LDP     X19 , X20 , [X0 , #0  << 3]
    LDP     X21 , X22 , [X0 , #2  << 3]
    LDP     X23 , X24 , [X0 , #4  << 3]
    LDP     X25 , X26 , [X0 , #6  << 3]
    LDP     X27 , X28 , [X0 , #8  << 3]
    LDP     X29 , X30 , [X0 , #10 << 3]
    LDP     D8  , D9  , [X0 , #14 << 3]
    LDP     D10 , D11 , [X0 , #16 << 3]
    LDP     D12 , D13 , [X0 , #18 << 3]
    LDP     D14 , D15 , [X0 , #20 << 3]

    LDR     X5  , [X0 , #13 << 3]
    MOV     SP  , X5

    CMP     X1  , #0
    MOV     X0  , #1
    CSEL    X0  , X1  , X0 , NE
    BR      X30
    FUNC_END()
    
;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    B       bspReboot                                                   ;/* 不再返回, 由 BSP 进行复位操作*/
    FUNC_END()
    
;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
1:
    MOV     X1 , #1
    MOV     X2 , #2
    MOV     X3 , #3
    MOV     X1 , #1
    MOV     X2 , #2
    MOV     X3 , #3
    SUBS    X0 , X0 , #1
    BNE     1b
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 获得 PSTATE
;*********************************************************************************************************/

FUNC_DEF(arm64GetDAIF)
    MRS     X0 , DAIF
    RET
    FUNC_END()

FUNC_DEF(arm64GetNZCV)
    MRS     X0 , NZCV
    RET
    FUNC_END()

FUNC_DEF(arm64GetCurrentEL)
    MRS     X0 , CurrentEL
    RET
    FUNC_END()

FUNC_DEF(arm64GetSPSel)
    MRS     X0 , SPSel
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 进入节能状态，等待中断
;*********************************************************************************************************/

FUNC_DEF(arm64WaitForInterrupt)
    ARM_DSB()
    ARM_ISB()
    WFI
    RET
    FUNC_END()

;/*********************************************************************************************************
;  EL 状态切换
;*********************************************************************************************************/

MACRO_DEF(SWITCH_EL, xreg, el3_label, el2_label, el1_label)
    MRS     \xreg ,  CurrentEL
    CMP     \xreg ,  0xc
    B.EQ    \el3_label
    CMP     \xreg ,  0x8
    B.EQ    \el2_label
    CMP     \xreg ,  0x4
    B.EQ    \el1_label
    MACRO_END()

;/*********************************************************************************************************
;  切换到 EL1 (仅允许在操作系统启动前调用! 因为这里没有使用堆栈, 使用了 x18 寄存器)
;*********************************************************************************************************/

FUNC_DEF(arm64SwitchToEl1)
    MOV         X18, LR
    SWITCH_EL   X6 , el3, el2, el1

LINE_LABEL(el3)
    BL      arm64El3SwitchEl2
    BL      arm64EL2SwitchEL1
    MOV     LR , X18
    RET

LINE_LABEL(el2)
    BL      arm64EL2SwitchEL1
    MOV     LR , X18
    RET

LINE_LABEL(el1)
    MOV     LR , X18
    RET

;/*********************************************************************************************************
;  EL3 切换到 EL2 
;*********************************************************************************************************/

FUNC_DEF(arm64El3SwitchEl2)
    MOV     X0 ,       #0x5b1                                            ;/*  Non-secure EL0/1 |HVC|64bit*/
    MSR     SCR_EL3 ,  X0
    MSR     CPTR_EL3,  XZR                                               ;/*  禁能协处理器陷入 EL3       */

    ;/*
    ; * 从 EL3 跳转至 EL2 AARCH64
    ; */
    MOV     X0 ,       SP                                                ;/*  Ret EL2_SP2 mode from EL3  */
    MSR     SP_EL2 ,   X0                                                ;/*  Migrate SP                 */
    MOV     X0 ,       #0x3c9
    MSR     SPSR_EL3 , X0                                                ;/*  EL2_SP2 | D | A | I | F    */
    MSR     ELR_EL3  , LR

    ERET
    FUNC_END()

;/*********************************************************************************************************
;  EL2 切换到 EL1
;*********************************************************************************************************/

FUNC_DEF(arm64EL2SwitchEL1)
    MRS     X0 , CNTHCTL_EL2                                             ;/*  初始化通用定时器           */
    ORR     X0 , X0 , #(CNTHCTL_EL2_EL1PCEN_EN | \
                        CNTHCTL_EL2_EL1PCTEN_EN)                         ;/*  使能 EL1 对 定时器的访问   */
    MSR     CNTHCTL_EL2 , X0
    MSR     CNTVOFF_EL2 , XZR

    MRS     X0 , MIDR_EL1                                                ;/*  初始化 MPID/MPIDR          */
    MSR     VPIDR_EL2  , X0
    MRS     X0 , MPIDR_EL1
    MSR     VMPIDR_EL2 , X0

    MOV     X0 , #CPTR_EL2_RES1                                          ;/*  禁能协处理器的陷入         */
    MSR     CPTR_EL2 , X0                                                ;/*  禁能协处理器陷入 EL2       */
    MSR     HSTR_EL2 , XZR

    MOV     X0 , #CPACR_EL1_FPEN_EN                                      ;/*  Enable FP/SIMD at EL1      */
    MSR     CPACR_EL1 , X0

    MRS     X0,  FPEXC32_EL2
    ORR     X0,  X0, #(1 << 30)
    MSR     FPEXC32_EL2, X0

    MSR     DAIFSET ,  #2                                                ;/*  关中断                    */
    ADRP    X0 ,       vector
    ADD     X0 ,       X0 , #:lo12:vector
    MSR     VBAR_EL2 , X0                                                ;/*  设置 EL2 的向量表地址     */
    MSR     VBAR_EL1 , X0                                                ;/*  设置 EL1 的向量表地址     */

    ;/*
    ; * SCTLR_EL1 初始化
    ; */
    LDR     X0 , =(SCTLR_EL1_RES1        | SCTLR_EL1_UCI_DIS    | \
                   SCTLR_EL1_EE_LE       | SCTLR_EL1_WXN_DIS    | \
                   SCTLR_EL1_NTWE_DIS    | SCTLR_EL1_NTWI_DIS   | \
                   SCTLR_EL1_UCT_DIS     | SCTLR_EL1_DZE_DIS    | \
                   SCTLR_EL1_ICACHE_DIS  | SCTLR_EL1_UMA_DIS    | \
                   SCTLR_EL1_SED_EN      | SCTLR_EL1_ITD_EN     | \
                   SCTLR_EL1_CP15BEN_DIS | SCTLR_EL1_SA0_DIS    | \
                   SCTLR_EL1_SA_DIS      | SCTLR_EL1_DCACHE_DIS | \
                   SCTLR_EL1_ALIGN_DIS   | SCTLR_EL1_MMU_DIS)
    MSR     SCTLR_EL1 , X0
    LDR     X0 , =(HCR_EL2_RW_AARCH64 | HCR_EL2_HCD_DIS)                 ;/*  初始化 HCR_EL2            */
    MSR     HCR_EL2 , X0

    ;/*  
    ; * 从 EL2 跳转至 EL1 AARCH64 
    ; */
    LDR     X0 , =(SPSR_D_BIT | SPSR_A_BIT      | \
                   SPSR_F_BIT | SPSR_MODE64_BIT | SPSR_MODE_EL1h)
    MSR     SPSR_EL2 , X0
    MSR     ELR_EL2  , X30
    ERET
    FUNC_END()

    FILE_END()
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
