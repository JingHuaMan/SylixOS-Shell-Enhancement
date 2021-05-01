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
;** 文   件   名: arm64VfpV4El2Asm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 10 月 27 日
;**
;** 描        述: ARM64 体系架构 VFPv4 支持 (在 EL2 中处理).
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

#include <arch/arm64/arch_def.h>

    FILE_BEGIN()

    EXPORT_LABEL(arm64VfpV4El2HwInit)
    EXPORT_LABEL(arm64VfpV4El2Mvfr0EL1)
    EXPORT_LABEL(arm64VfpV4El2Enable)
    EXPORT_LABEL(arm64VfpV4El2Disable)
    EXPORT_LABEL(arm64VfpV4El2IsEnable)
    EXPORT_LABEL(arm64VfpV4El2Save)
    EXPORT_LABEL(arm64VfpV4El2Restore)
    EXPORT_LABEL(arm64VfpV4El2FpuHandle)
    
    WEAK(arm64VfpV4El2HwInit)

;/*********************************************************************************************************
;  浮点运算器使能禁能操作
;  某些控制器在 EL1 操作 FPU 使能寄存器会出现问题，统一放在 EL2 阶段操作
;*********************************************************************************************************/
#define ARM64_FPU_ENABLE      0x0
#define ARM64_FPU_DISABLE     0x1

;/*********************************************************************************************************
;  浮点运算器基本操作函数
;*********************************************************************************************************/

FUNC_DEF(arm64VfpV4El2HwInit)
    STP     X29 , LR, [SP, #-16]!
    MOV     X29 , SP
    MOV     X0  , #ARM64_FPU_DISABLE
    HVC     #HVC_CMD_FPU
    LDP     X29 , LR, [SP], #16

    MRS     X0 , CPACR_EL1                                              ;/*  禁能 FPU 操作               */
    BIC     X0 , X0, #CPACR_EL1_FPEN_EN                                 ;/*  先清零                      */
    ORR     X0 , X0, #CPACR_EL1_FPEN_DIS                                ;/*  设置为禁能                  */
    MSR     CPACR_EL1 , X0
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4El2Mvfr0EL1)
    MRS     X0 , MVFR0_EL1
    RET    
    FUNC_END()

;/*********************************************************************************************************
;  EL2 中处理 FPU 函数，参数通过 X2 传递
;*********************************************************************************************************/

FUNC_DEF(arm64VfpV4El2FpuHandle)
    CMP     X2 , #ARM64_FPU_ENABLE                                      ;/*  判断是使能还是禁能 FPU      */
    B.NE    DisableFpu

LINE_LABEL(EnableFpu)
    MRS     X0 , CPACR_EL1                                              ;/*  使能 FPU 操作               */
    ORR     X0 , X0, #CPACR_EL1_FPEN_EN
    MSR     CPACR_EL1 , X0
    ISB
    RET

LINE_LABEL(DisableFpu)
    MRS     X0 , CPACR_EL1                                              ;/*  禁能 FPU 操作               */
    BIC     X0 , X0, #CPACR_EL1_FPEN_EN                                 ;/*  先清零                      */
    ORR     X0 , X0, #CPACR_EL1_FPEN_DIS                                ;/*  设置为禁能                  */
    MSR     CPACR_EL1 , X0
    ISB
    RET
    FUNC_END()

FUNC_DEF(arm64VfpV4El2Enable)
    STP     X29 , LR, [SP, #-16]!
    MOV     X29 , SP
    MOV     X0  , #ARM64_FPU_ENABLE
    HVC     #HVC_CMD_FPU
    LDP     X29 , LR, [SP], #16

    MRS     X0 , CPACR_EL1                                              ;/*  在 EL1 阶段也操作           */
    ORR     X0 , X0, #CPACR_EL1_FPEN_EN
    MSR     CPACR_EL1 , X0
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4El2Disable)
    STP     X29 , LR, [SP, #-16]!
    MOV     X29 , SP
    MOV     X0  , #ARM64_FPU_DISABLE
    HVC     #HVC_CMD_FPU
    LDP     X29 , LR, [SP], #16

    MRS     X0 , CPACR_EL1                                              ;/*  禁能 FPU 操作               */
    BIC     X0 , X0, #CPACR_EL1_FPEN_EN                                 ;/*  先清零                      */
    ORR     X0 , X0, #CPACR_EL1_FPEN_DIS                                ;/*  设置为禁能                  */
    MSR     CPACR_EL1 , X0

    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4El2IsEnable)
    MRS     X1 , CPACR_EL1
    MOV     X0 , X1 , LSR #20
    AND     X0 , X0 , #1
    RET
    FUNC_END()
  
FUNC_DEF(arm64VfpV4El2Save)
    STP     X29 , LR, [SP, #-16]!
    MOV     X29 , SP
    MOV     X5  , X0
    MOV     X0  , #ARM64_FPU_ENABLE
    HVC     #HVC_CMD_FPU
    MOV     X0  , X5
    LDP     X29 , LR, [SP], #16

    STP     Q0  , Q1  , [X0 , #16 *  0]
    STP     Q2  , Q3  , [X0 , #16 *  2]
    STP     Q4  , Q5  , [X0 , #16 *  4]
    STP     Q6  , Q7  , [X0 , #16 *  6]
    STP     Q8  , Q9  , [X0 , #16 *  8]
    STP     Q10 , Q11 , [X0 , #16 * 10]
    STP     Q12 , Q13 , [X0 , #16 * 12]
    STP     Q14 , Q15 , [X0 , #16 * 14]
    STP     Q16 , Q17 , [X0 , #16 * 16]
    STP     Q18 , Q19 , [X0 , #16 * 18]
    STP     Q20 , Q21 , [X0 , #16 * 20]
    STP     Q22 , Q23 , [X0 , #16 * 22]
    STP     Q24 , Q25 , [X0 , #16 * 24]
    STP     Q26 , Q27 , [X0 , #16 * 26]
    STP     Q28 , Q29 , [X0 , #16 * 28]
    STP     Q30 , Q31 , [X0 , #16 * 30]!
    MRS     X8  , FPSR
    STR     W8  , [X0 , #16 * 2]
    MRS     X8  , FPCR
    STR     W8  , [X0 , #16 * 2 + 4]
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4El2Restore)
    STP     X29 , LR, [SP, #-16]!
    MOV     X29 , SP
    MOV     X5  , X0
    MOV     X0  , #ARM64_FPU_ENABLE
    HVC     #HVC_CMD_FPU
    MOV     X0  , X5
    LDP     X29 , LR, [SP], #16

    LDP     Q0   ,  Q1 , [X0 , #16 *  0]
    LDP     Q2   ,  Q3 , [X0 , #16 *  2]
    LDP     Q4   ,  Q5 , [X0 , #16 *  4]
    LDP     Q6   ,  Q7 , [X0 , #16 *  6]
    LDP     Q8   ,  Q9 , [X0 , #16 *  8]
    LDP     Q10  , Q11 , [X0 , #16 * 10]
    LDP     Q12  , Q13 , [X0 , #16 * 12]
    LDP     Q14  , Q15 , [X0 , #16 * 14]
    LDP     Q16  , Q17 , [X0 , #16 * 16]
    LDP     Q18  , Q19 , [X0 , #16 * 18]
    LDP     Q20  , Q21 , [X0 , #16 * 20]
    LDP     Q22  , Q23 , [X0 , #16 * 22]
    LDP     Q24  , Q25 , [X0 , #16 * 24]
    LDP     Q26  , Q27 , [X0 , #16 * 26]
    LDP     Q28  , Q29 , [X0 , #16 * 28]
    LDP     Q30  , Q31 , [X0 , #16 * 30]!
    LDR     W8   , [X0 , #16 * 2]
    MSR     FPSR , X8
    LDR     W8   , [X0 , #16 * 2 + 4]
    MSR     FPCR , X8
    RET
    FUNC_END()
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
