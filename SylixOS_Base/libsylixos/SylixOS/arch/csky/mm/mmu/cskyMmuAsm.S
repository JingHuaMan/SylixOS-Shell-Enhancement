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
;** 文   件   名: cskyMmuAsm.S
;**
;** 创   建   人: Hui.Kai (惠凯)
;**
;** 文件创建日期: 2018 年 05 月 14 日
;**
;** 描        述: C-SKY 体系构架 MMU TLB 重填处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>
#include <config/kernel/memory_cfg.h>

#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)

#if LW_CFG_VMM_EN > 0
#include "arch/csky/arch_mmu.h"
#include "arch/csky/inc/cskyregs.h"

;/*********************************************************************************************************
;  PTE BASE 相关定义
;*********************************************************************************************************/

#define CSKY_PBA_SHIFT          12

    EXPORT_LABEL(cskyMmuContextSet)
    EXPORT_LABEL(cskyMmuPageMaskSet)
    EXPORT_LABEL(cskyMmuTlbRefillExceptEntry)
    EXPORT_LABEL(cskyMmuTlbLdStExceptHandle)
    EXPORT_LABEL(cskyMmuEnableHw)
    EXPORT_LABEL(cskyMmuDisableHw)

    FILE_BEGIN()
   
;/*********************************************************************************************************
;  设置 MMU 上下文
;*********************************************************************************************************/

FUNC_DEF(cskyMmuContextSet)
#if LW_CFG_CSKY_HARD_TLB_REFILL > 0
    ORI         A0 , A0 , 1
#endif
    MTCR        A0 , CR<29, 15>
    RTS
    FUNC_END(cskyMmuContextSet)
    
;/*********************************************************************************************************
;  设置 PageMask
;*********************************************************************************************************/

FUNC_DEF(cskyMmuPageMaskSet)
    MTCR        A0 , CR<6, 15>
    RTS
    FUNC_END(cskyMmuPageMaskSet)

;/*********************************************************************************************************
;  使能 MMU
;*********************************************************************************************************/

FUNC_DEF(cskyMmuEnableHw)
    MFCR        A0 , CR18
    BSETI       A0 , 0
    BCLRI       A0 , 1
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyMmuEnableHw)

;/*********************************************************************************************************
;  禁能 MMU
;*********************************************************************************************************/

FUNC_DEF(cskyMmuDisableHw)
    MFCR        A0 , CR18
    BCLRI       A0 , 0
    BCLRI       A0 , 1
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyMmuDisableHw)

;/*********************************************************************************************************
;  TLB 重填
;*********************************************************************************************************/

FUNC_DEF(cskyMmuTlbRefillExceptEntry)
    MTCR        A0 , SS2
    MTCR        A1 , SS3
    MTCR        A2 , SS4

#if LW_CFG_CSKY_HARD_TLB_REFILL == 0
    ;/*
    ; * A0 = Bad VPN
    ; * A2 = MPGD
    ; */
    MFCR        A0 , CR<4, 15>
    MFCR        A2 , CR<29, 15>

    LSRI        A2 , CSKY_PBA_SHIFT
    LSLI        A2 , CSKY_PBA_SHIFT

    ;/*
    ; * 获得 PGD 条目
    ; */
    MOV         A1 , A0
    LSRI        A1 , LW_CFG_VMM_PGD_SHIFT
    LSLI        A1 , LONGLOG
    ADDU        A1 , A2
    BSETI       A1 , 31
    LD.W        A2 , (A1, 0)
    CMPNEI      A2 , 0
    BF          __error_handle

    ;/*
    ; * 获得一对 PTE 条目
    ; */
    ZEXT        A1 , A0 , LW_CFG_VMM_PGD_SHIFT - 1 , LW_CFG_VMM_PAGE_SHIFT + 1
    LSLI        A1 , LONGLOG + 1
    ADDU        A1 , A2
    BSETI       A1 , 31
    LD.W        A0 , (A1 , 0)
    LD.W        A1 , (A1 , 4)

    ;/*
    ; * TLB 重填
    ; */
    MTCR        A0 , CR<2, 15>
    MTCR        A1 , CR<3, 15>

    BGENI       A1 , 28
    MTCR        A1 , CR<8, 15>

    MFCR        A1 , EPSR
    BCLRI       A1 , 13
    MTCR        A1 , EPSR

    MFCR        A0 , SS2
    MFCR        A1 , SS3
    MFCR        A2 , SS4

    RTE

LINE_LABEL(__error_handle)
    ;/*
    ; * TLB 重填
    ; */
    MOVI        A0 , 0
    MTCR        A0 , CR<2, 15>
    MTCR        A0 , CR<3, 15>

    BGENI       A1 , 28
    MTCR        A1 , CR<8, 15>
#endif

    MFCR        A1 , EPSR
    BCLRI       A1 , 13
    MTCR        A1 , EPSR

    MFCR        A0 , SS2
    MFCR        A1 , SS3
    MFCR        A2 , SS4

    RTE
    FUNC_END(cskyMmuTlbRefillExceptEntry)

;/*********************************************************************************************************
;  TLB 读写无效
;*********************************************************************************************************/

FUNC_DEF(cskyMmuTlbLdStExceptHandle)
    MTCR        A3 , SS2
    MTCR        R6 , SS3
    MTCR        A2 , SS4

    MFCR        A3 , EPSR
    BCLRI       A3 , 13
    MTCR        A3 , EPSR

    ;/*
    ; * A2 = Bad VPN
    ; * R6 = MPGD
    ; */
    MFCR        A2 , CR<4, 15>
    MFCR        R6 , CR<29, 15>

    LSRI        R6 , CSKY_PBA_SHIFT
    LSLI        R6 , CSKY_PBA_SHIFT
    BSETI       R6 , 31

    ;/*
    ; * 获得 PGD 条目
    ; */
    MOV         A3 , A2
    LSRI        A3 , LW_CFG_VMM_PGD_SHIFT
    LSLI        A3 , LONGLOG
    ADDU        A3 , R6
    LD.W        R6 , (A3, 0)
    CMPNEI      R6 , 0
    BF          __tlbinv_handle

    ;/*
    ; * 获得 PTE 条目
    ; */
    ZEXT        A3 , A2 , LW_CFG_VMM_PGD_SHIFT - 1 , LW_CFG_VMM_PAGE_SHIFT
    LSLI        A3 , LONGLOG
    ADDU        R6 , A3
    BSETI       R6 , 31
    LD.W        A2 , (R6 , 0)

    ;/*
    ; *  TLB probe 命令，查找错误条目
    ; */
    BGENI       A3 , 31
    MTCR        A3 , CR<8, 15>
    MOVI        A3 , ENTRYLO_V
    AND         A2 , A3
    CMPNEI      A2 , 0
    BF          __tlbinv_handle
    LD.W        A2 , (R6 , 0)

    MOVI        A3 , ENTRYLO_V
    OR          A2 , A3
    ST.W        A2 , (R6 , 0)

    BCLRI       R6 , LONGLOG
    LD.W        A2 , (R6 , 0)
    LD.W        A3 , (R6 , 4)

    MTCR        A2 , CR<2, 15>
    MTCR        A3 , CR<3, 15>

    MFCR        A3 , CR<0, 15>
    BGENI       A2 , 29
    BTSTI       A3 , 31
    BF          1f
    BGENI       A2 , 25
    MTCR        A2 , CR<8, 15>
    BGENI       A2 , 28

LINE_LABEL(1)
    MTCR        A2 , CR<8, 15>

    MFCR        A3 , SS2
    MFCR        R6 , SS3
    MFCR        A2 , SS4
    MOVI        A0 , 0
    RTS

LINE_LABEL(__tlbinv_handle)
    MFCR        A3 , SS2
    MFCR        R6 , SS3
    MFCR        A2 , SS4
    MOVI        A0 , 2
    RTS
    FUNC_END(cskyMmuTlbLdStExceptHandle)

    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
