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
;** 文   件   名: ppcMmu460RegAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2019 年 08 月 14 日
;**
;** 描        述: PowerPC 460 体系构架 MMU 寄存器函数库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

#if LW_CFG_VMM_EN > 0

#include "arch/ppc/arch_e500.h"
#include "./ppcMmu460Reg.h"

    FILE_BEGIN()

    EXPORT_LABEL(ppc460MmuGetMMUCR)
    EXPORT_LABEL(ppc460MmuSetMMUCR)
    EXPORT_LABEL(ppc460MmuGetPID)
    EXPORT_LABEL(ppc460MmuSetPID)

;/*********************************************************************************************************
;  SPR GET SET 函数
;*********************************************************************************************************/

MACRO_DEF(SET_SPR, spr)
    MTSPR       \spr , R3
    ISYNC
    BLR
    MACRO_END()

MACRO_DEF(GET_SPR, spr)
    MFSPR       R3 , \spr
    ISYNC
    BLR
    MACRO_END()

FUNC_DEF(ppc460MmuGetMMUCR)
    GET_SPR     PPC460_MMUCR
    FUNC_END()

FUNC_DEF(ppc460MmuSetMMUCR)
    SYNC
    SET_SPR     PPC460_MMUCR
    FUNC_END()

FUNC_DEF(ppc460MmuGetPID)
    GET_SPR     PID
    FUNC_END()

FUNC_DEF(ppc460MmuSetPID)
    SYNC
    SET_SPR     PID
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
