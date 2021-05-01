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
;** 文   件   名: arm64VfpV4Asm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 07 月 03 日
;**
;** 描        述: ARM64 体系架构 VFPv4 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

#include <arch/arm64/arch_def.h>

    FILE_BEGIN()

    EXPORT_LABEL(arm64VfpV4HwInit)
    EXPORT_LABEL(arm64VfpV4Mvfr0EL1)
    EXPORT_LABEL(arm64VfpV4Enable)
    EXPORT_LABEL(arm64VfpV4Disable)
    EXPORT_LABEL(arm64VfpV4IsEnable)
    EXPORT_LABEL(arm64VfpV4Save)
    EXPORT_LABEL(arm64VfpV4Restore)
    
    WEAK(arm64VfpV4HwInit)

;/*********************************************************************************************************
;  浮点运算器基本操作函数
;*********************************************************************************************************/

FUNC_DEF(arm64VfpV4HwInit)
    MOV     X0 , #2 << 20
    MSR     CPACR_EL1 , X0
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4Mvfr0EL1)
    MRS     X0 , MVFR0_EL1
    RET    
    FUNC_END()

FUNC_DEF(arm64VfpV4Enable)
    MRS     X0 , CPACR_EL1
    ORR     X0 , X0 , #CPACR_EL1_FPEN_EN
    MSR     CPACR_EL1 , X0
    ISB
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4Disable)
    MOV     X0 , #2 << 20
    MSR     CPACR_EL1 , X0
    RET
    FUNC_END()
    
FUNC_DEF(arm64VfpV4IsEnable)
    MRS     X1 , CPACR_EL1
    MOV     X0 , X1 , LSR #20
    AND     X0 , X0 , #1
    RET
    FUNC_END()
  
FUNC_DEF(arm64VfpV4Save)
    MOV     X1  , #CPACR_EL1_FPEN_EN
    MSR     CPACR_EL1 , X1

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
    
FUNC_DEF(arm64VfpV4Restore)
    MOV     X1   , #CPACR_EL1_FPEN_EN
    MSR     CPACR_EL1  , X1

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
