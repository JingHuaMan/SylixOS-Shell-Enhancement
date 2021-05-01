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
;** 文   件   名: ppcMmu460Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2019 年 08 月 14 日
;**
;** 描        述: PowerPC 460 体系构架 MMU 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/kernel/vmm_cfg.h>

#if LW_CFG_VMM_EN > 0

#include "arch/ppc/arch_e500.h"
#include "arch/ppc/arch_mmu.h"
#include "./ppcMmu460Reg.h"

    FILE_BEGIN()

    IMPORT_LABEL(archE500DataStorageExceptionEntry)
    IMPORT_LABEL(archE500InstructionStorageExceptionEntry)
    IMPORT_LABEL(ppc460MmuTblIndex)

    EXPORT_LABEL(ppc460MmuInvalidateTLBEA)
    EXPORT_LABEL(arch460DataTLBErrorEntry)
    EXPORT_LABEL(arch460InstructionTLBErrorEntry)

;/*********************************************************************************************************
;  无效指定地址和 PID 的 TLB
;*********************************************************************************************************/

FUNC_DEF(ppc460MmuInvalidateTLBEA)
    MFSPR       R5 , PPC460_MMUCR

    /*
     * We write 16 bits of STID since 47x supports that much, we
     * will never be passed out of bounds values on 440 (hopefully)
     */
    RLWIMI      R5 , R4 , 0 , 16 , 31

    /* We have to run the search with interrupts disabled, otherwise
     * an interrupt which causes a TLB miss can clobber the MMUCR
     * between the mtspr and the tlbsx.
     *
     * Critical and Machine Check interrupts take care of saving
     * and restoring MMUCR, so only normal interrupts have to be
     * taken care of.
     */
    MTSPR       PPC460_MMUCR , R5
    TLBSX.      R6 , 0 , R3
    BNE         __noFound

    CMPWI       CR7 , R6 , PPC460_TLB_BASE
    BLT         CR7 , __noFound

    /* On 440 There are only 64 TLB entries, so r3 < 64, which means bit
     * 22, is clear.  Since 22 is the V bit in the TLB_PAGEID, loading this
     * value will invalidate the TLB entry.
     */
    SYNC
    TLBWE       R6 , R6 , 0
    ISYNC
LINE_LABEL(__noFound)
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  异常处理程序都放在 .vector 节区
;*********************************************************************************************************/

    SECTION(.vector)

;/*********************************************************************************************************
;  TLB 错误异常入口宏
;*********************************************************************************************************/

MACRO_DEF(PPC460_TLB_ERROR_ENTRY, addr_reg, exc_entry)
    MTSPR       SPRG4_W , R3                ;/*  用 SPRG4, 5, 6, 7 备份 R3, R4, R5, CR                   */
    MTSPR       SPRG5_W , R4
    MTSPR       SPRG6_W , R5
    MFCR        R3
    MTSPR       SPRG7_W , R3

    MFSPR       R5 , \addr_reg              ;/*  R5 = ulMissAddr;                                        */
    ISYNC
    MFSPR       R4 , SPRG3                  ;/*  R4 = PGDTable;                                          */

    RLWINM      R3 , R5 , 10 , 22 , 31      ;/*  R3 = ulMissAddr >> LW_CFG_VMM_PGD_SHIFT = PGD_NR;       */

    RLWINM      R3 , R3 , 2 , 0 , 29        ;/*  R3 = R3 << 2 = PGD_NR * sizeof(LW_PGD_TRANSENTRY);      */

    ADD         R3 , R4 , R3                ;/*  R3 = p_pgdentry = PGDTable + PGD_NR * sizeof(PGD_ENTRY);*/

    LWZ         R3 , 0(R3)                  ;/*  R3 = pgdentry = *p_pgdentry;                            */

    CMPWI       CR7 , R3 , 0                ;/*  if (!pgdentry)                                          */
    BEQ         CR7 , 2f                    ;/*  then goto 2;                                            */

    RLWINM      R4 , R5 , 10 , 0 , 21       ;/*  R4 = ulMissAddr << (32 - LW_CFG_VMM_PGD_SHIFT);         */

#if   LW_CFG_VMM_PAGE_SIZE == (4 * LW_CFG_KB_SIZE)
    RLWINM      R4 , R4 , 10 , 22 , 31      ;/*  R4 = R4 >> (32 - PGD_SHIFT + PAGE_SHIFT) = PAGE_NR;     */
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
    RLWINM      R4 , R4 , 8 , 24 , 31       ;/*  R4 = R4 >> (32 - PGD_SHIFT + PAGE_SHIFT) = PAGE_NR;     */
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
    RLWINM      R4 , R4 , 6 , 26 , 31       ;/*  R4 = R4 >> (32 - PGD_SHIFT + PAGE_SHIFT) = PAGE_NR;     */
#else
#error LW_CFG_VMM_PAGE_SIZE no define!
#endif

    RLWINM      R4 , R4 , 2  , 0 , 29       ;/*  R4 = PAGE_NR * sizeof(LW_PTE_TRANSENTRY);               */

    ADD         R3 , R4 , R3                ;/*  R3 = p_pteentry + PAGE_NR * sizeof(LW_PTE_TRANSENTRY);  */

    LWZ         R3 , 0(R3)                  ;/*  R3 = pteentry = *p_pteentry;                            */

    RLWINM      R4 , R3 , 0 , 20 , 20       ;/*  R4 = pteentry.WORD0_bValid;                             */

    CMPWI       CR7 , R4 , 0                ;/*  if (!pteentry.WORD0_bValid)                             */
    BEQ         CR7 , 2f                    ;/*  then goto 2;                                            */

    LIS         R4 , ppc460MmuTblIndex@ha
    LWZ         R4 , ppc460MmuTblIndex@l(R4);/*  R4 = ppc460MmuTblIndex;                                 */

    RLWINM      R5 , R5 , 0 , 0 , 19        ;/*  R5  = ulMissAddr & TLB_EPN_MASK(0xfffff000);            */
#if   LW_CFG_VMM_PAGE_SIZE == (4 * LW_CFG_KB_SIZE)
    ORI         R5 , R5 , (0x200 | 0x10)    ;/*  R5 |= TLB_VALID | TLB_4K (TS=0);                        */
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
    ORI         R5 , R5 , (0x200 | 0x20)    ;/*  R5 |= TLB_VALID | TLB_16K (TS=0);                       */
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
    ORI         R5 , R5 , (0x200 | 0x30)    ;/*  R5 |= TLB_VALID | TLB_64K (TS=0);                       */
#endif
    TLBWE       R5 , R4 , 0                 ;/*  TLB WORD0                                               */
    ISYNC

    RLWINM      R5 , R3 , 0 , 0 , 19        ;/*  R5 = pteentry & TLB_RPN_MASK(0xfffff000);               */
    TLBWE       R5 , R4 , 1                 ;/*  TLB WORD1                                               */
    ISYNC

    MR          R5 , R3                     ;/*  R5 = pteentry;                                          */
    RLWINM      R5 , R5 , 2 , 0  , 29       ;/*  R5 = pteentry << 2;                                     */
    RLWINM      R5 , R5 , 0 , 20 , 23       ;/*  R5 = R5 & TLB_WIMG_MASK;                                */
    CLRLWI      R3 , R3 , 26                ;/*  R3 = pteentry & TLB_PERM_MASK;                          */
    OR          R5 , R5 , R3                ;/*  R5 = TLB_WIMG | TLB_PERM;                               */
    TLBWE       R5 , R4 , 2                 ;/*  TLB WORD2                                               */
    ISYNC

    ADDI        R4  , R4 , 1                ;/*  R4++;                                                   */
    CMPWI       CR7 , R4 , (PPC460_TLB_SIZE - 1)  ;/*  if (R4 < PPC460_TLB_SIZE)                         */
    BLE         CR7 , 1f                    ;/*  then goto 1;                                            */
    LI          R4  , PPC460_TLB_BASE       ;/*  R4 = PPC460_TLB_BASE;                                   */
LINE_LABEL(1)
    LIS         R5 , ppc460MmuTblIndex@ha
    STW         R4 , ppc460MmuTblIndex@l(R5);/*  ppc460MmuTblIndex = R4;                                 */

    MFSPR       R3 , SPRG7_R                ;/*  从 SPRG4, 5, 6 恢复 R3, R4, R5                          */
    MTCR        R3
    MFSPR       R5 , SPRG6_R
    MFSPR       R4 , SPRG5_R
    MFSPR       R3 , SPRG4_R

    RFI                                     ;/*  从 SRR0 返回, 同时 MSR = SRR1                           */

LINE_LABEL(2)
    MFSPR       R3 , SPRG7_R                ;/*  从 SPRG4, 5, 6, 7 恢复 R3, R4, R5, CR                   */
    MTCR        R3
    MFSPR       R5 , SPRG6_R
    MFSPR       R4 , SPRG5_R
    MFSPR       R3 , SPRG4_R

    B           \exc_entry                  ;/*  PTE 无效, 跳转到异常处理                                */
    MACRO_END()

;/*********************************************************************************************************
;  数据 TLB 错误异常入口(Vector IVOR13)
;*********************************************************************************************************/

EXCE_DEF(arch460DataTLBErrorEntry)
    PPC460_TLB_ERROR_ENTRY  DEAR , archE500DataStorageExceptionEntry
    EXCE_END()

;/*********************************************************************************************************
;  指令 TLB 错误异常入口(Vector IVOR14)
;*********************************************************************************************************/

EXCE_DEF(arch460InstructionTLBErrorEntry)
    PPC460_TLB_ERROR_ENTRY  SRR0 , archE500InstructionStorageExceptionEntry
    EXCE_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
