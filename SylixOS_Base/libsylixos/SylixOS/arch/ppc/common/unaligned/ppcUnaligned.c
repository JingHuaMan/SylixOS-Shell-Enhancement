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
** 文   件   名: ppcUnaligned.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 08 月 27 日
**
** 描        述: PowerPC 体系构架非对齐处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>
#include "porting.h"
#include "sstep.h"
#include "disassemble.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
int fix_alignment(ARCH_REG_CTX *regs, enum instruction_type  *inst_type);
/*********************************************************************************************************
** 函数名称: ppcUnalignedHandle
** 功能描述: PowerPC 非对齐处理
** 输　入  : pregctx           寄存器上下文
**           pabtInfo          终止信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcUnalignedHandle (ARCH_REG_CTX  *pregctx, PLW_VMM_ABORT  pabtInfo)
{
    INT                    iFixed;
    enum instruction_type  type;

    if (pregctx->REG_uiDar == pregctx->REG_uiPc) {
        goto  sigbus;
    }

    iFixed = fix_alignment(pregctx, &type);
    if (iFixed == 1) {
        pregctx->REG_uiPc += 4;                                         /*  Skip over emulated inst     */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;
        return;

    } else if (iFixed == -EFAULT) {                                     /*  Operand address was bad     */
        switch (type) {

        case STORE:
        case STORE_MULTI:
        case STORE_FP:
        case STORE_VMX:
        case STORE_VSX:
        case STCX:
            pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_MAP;
            pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            break;

        case LOAD:
        case LOAD_MULTI:
        case LOAD_FP:
        case LOAD_VMX:
        case LOAD_VSX:
        case LARX:
        default:
            pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_MAP;
            pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
            break;
        }
        return;
    }

sigbus:
    pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    pabtInfo->VMABT_uiMethod = BUS_ADRALN;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
