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
** 文   件   名: riscvUnaligned.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 08 月 27 日
**
** 描        述: RISC-V 体系构架非对齐处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if LW_CFG_RISCV_M_LEVEL > 0
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern int  misaligned_load_trap(ARCH_REG_CTX  *pregctx);
extern int  misaligned_store_trap(ARCH_REG_CTX  *pregctx);
/*********************************************************************************************************
** 函数名称: riscvLoadUnalignedHandle
** 功能描述: RISC-V load 指令非对齐处理
** 输　入  : pregctx           寄存器上下文
**           pabtInfo          异常信息
** 输　出  : 终止信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  riscvLoadUnalignedHandle (ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo)
{
    if (misaligned_load_trap(pregctx) == ERROR_NONE) {
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;

    } else {
        pabtInfo->VMABT_uiMethod = BUS_ADRALN;
        pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    }
}
/*********************************************************************************************************
** 函数名称: riscvStoreUnalignedHandle
** 功能描述: RISC-V store 指令非对齐处理
** 输　入  : pregctx           寄存器上下文
**           pabtInfo          异常信息
** 输　出  : 终止信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  riscvStoreUnalignedHandle (ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo)
{
    if (misaligned_store_trap(pregctx) == ERROR_NONE) {
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;

    } else {
        pabtInfo->VMABT_uiMethod = BUS_ADRALN;
        pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    }
}

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
