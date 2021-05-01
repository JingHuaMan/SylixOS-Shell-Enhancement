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
;** 文   件   名: ppcMmuHashAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 01 月 14 日
;**
;** 描        述: PowerPC 体系构架 HASH 页表 MMU 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

#if LW_CFG_VMM_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(ppcHashMmuEnable)
    EXPORT_LABEL(ppcHashMmuDisable)

    EXPORT_LABEL(ppcHashMmuInvalidateTLBNr)
    EXPORT_LABEL(ppcHashMmuInvalidateTLBEA)

    EXPORT_LABEL(ppcHashMmuSetSDR1)
    EXPORT_LABEL(ppcHashMmuSetSR)

    EXPORT_LABEL(ppcHashMmuGetSRR1)
    EXPORT_LABEL(ppcHashMmuGetDSISR)

    EXPORT_LABEL(ppcHashMmuHashPageTblPteSet)

;/*********************************************************************************************************
;  使能 MMU
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuEnable)
    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_DR | ARCH_PPC_MSR_IR
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  禁能 MMU
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuDisable)
    MFMSR   R3
    ISYNC
    ARCH_PPC_MMU_MASK(R3 , R3)
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  无效所有 TLB
;  The tlbia instruction is not implemented on 750/E600/745x/E300/603e and when its opcode is encountered,
;  an illegal instruction program exception is generated. To invalidate all entries of both TLBs,
;  64(750/E600/745x)/32(E300/603e) tlbie instructions must be executed,
;  incrementing the value in EA14–EA19(750/E600/745x)/EA15–EA19(E300/603e) by one each time.
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuInvalidateTLBNr)
    ISYNC
    MTCTR   R3                                                          ;/*  CTR = R3                    */
    XOR     R4 , R4 , R4                                                ;/*  R4 = 0                      */

LINE_LABEL(tlbloop)
    ISYNC
    TLBIE   R4
    EIEIO
    TLBSYNC
    SYNC
    ADDI    R4 , R4 , 0x1000                                            ;/*  Increment bits 14-19        */
    BDNZ    tlbloop                                                     ;/*  Dec CTR, branch if CTR != 0 */
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  无效指定 EA 的 TLB
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuInvalidateTLBEA)
    SYNC
    TLBIE   R3
    EIEIO
    TLBSYNC
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  设置 PTE，并无效 TLB
;  VOID  ppcHashMmuHashPageTblPteSet(PTE    *pPte,
;                                    UINT32  uiWord0,
;                                    UINT32  uiWord1,
;                                    UINT32  uiEffectiveAddr);
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuHashPageTblPteSet)
    SYNC
    STW     R5, 4(R3)
    EIEIO
    STW     R4, 0(R3)
    SYNC
    TLBIE   R6
    EIEIO
    TLBSYNC
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  设置 SDR1 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuSetSDR1)
    SYNC
    MTSPR   SDR1 , R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  设置段寄存器
;  VOID  ppcHashMmuSetSR(UINT32  uiSRn, UINT32  uiValue);
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuSetSR)
    SYNC
    RLWINM  R3 , R3 , 28 , 0 , 3
    ISYNC
    MTSRIN  R4 , R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  获得 SRR1 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuGetSRR1)
    MFSPR   R3 , SRR1
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  获得 DSISR 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcHashMmuGetDSISR)
    MFSPR   R3 , DSISR
    ISYNC
    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
