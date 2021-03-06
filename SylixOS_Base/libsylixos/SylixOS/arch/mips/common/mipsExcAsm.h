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
** 文   件   名: mipsExcAsm.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系架构异常处理.
*********************************************************************************************************/

#ifndef __ARCH_MIPSEXCASM_H
#define __ARCH_MIPSEXCASM_H

    IMPORT_LABEL(archInterruptEntry)
    IMPORT_LABEL(archCacheErrorEntry)
    IMPORT_LABEL(archExceptionEntry)
    IMPORT_LABEL(mipsMmuTlbRefillEntry)

;/*********************************************************************************************************
;  CACHE 错误异常入口跳转
;*********************************************************************************************************/

MACRO_DEF(MIPS_CACHE_ERROR_HANDLE)
    .set    push
    .set    noat

    J       archCacheErrorEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  通用异常入口跳转
;*********************************************************************************************************/

MACRO_DEF(MIPS_EXCEPTION_HANDLE)
    .set    push
    .set    noat

    J       archExceptionEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  中断入口跳转
;*********************************************************************************************************/

MACRO_DEF(MIPS_INTERRUPT_HANDLE)
    .set    push
    .set    noat

    J       archInterruptEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  TLB 重填入口跳转
;*********************************************************************************************************/

MACRO_DEF(MIPS_TLB_REFILL_HANDLE)
    .set    push
    .set    noat

#if LW_CFG_CPU_WORD_LENGHT == 64
    DMFC0   K0 , CP0_CONTEXT                                            ;/* CP0_XCONTEXT =  CP0_CONTEXT  */
    MIPS_EHB
    DMTC0   K0 , CP0_XCONTEXT
    MIPS_EHB
#endif

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  XTLB 重填入口跳转
;*********************************************************************************************************/

#if LW_CFG_CPU_WORD_LENGHT == 64
MACRO_DEF(MIPS_XTLB_REFILL_HANDLE)
    .set    push
    .set    noat

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()
#endif

;/*********************************************************************************************************
;  TLB 重填入口跳转(老 BSP 兼容接口, 新 BSP 建议使用 MIPS_TLB_REFILL_HANDLE 宏)
;*********************************************************************************************************/

MACRO_DEF(MIPS32_TLB_REFILL_HANDLE)
    .set    push
    .set    noat

#if LW_CFG_CPU_WORD_LENGHT == 64
    DMFC0   K0 , CP0_CONTEXT                                            ;/* CP0_XCONTEXT =  CP0_CONTEXT  */
    MIPS_EHB
    DMTC0   K0 , CP0_XCONTEXT
    MIPS_EHB
#endif

    J       mipsMmuTlbRefillEntry
    NOP

    .set    pop
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
