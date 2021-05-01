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
** 文   件   名: riscvUnaligned.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 11 月 16 日
**
** 描        述: RISC-V 体系构架非对齐处理.
*********************************************************************************************************/

#ifndef __ARCH_RISCVUNALIGNED_H
#define __ARCH_RISCVUNALIGNED_H

#if LW_CFG_RISCV_M_LEVEL > 0

extern VOID  riscvLoadUnalignedHandle(ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo);
extern VOID  riscvStoreUnalignedHandle(ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo);

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */

#endif                                                                  /*  __ARCH_RISCVUNALIGNED_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
