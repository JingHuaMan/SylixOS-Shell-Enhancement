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
** 文   件   名: assembler.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 22 日
**
** 描        述: ARM64 汇编相关.
*********************************************************************************************************/

#ifndef __ASMARM64_ASSEMBLER_H
#define __ASMARM64_ASSEMBLER_H

/*********************************************************************************************************
  arm architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#define EXPORT_LABEL(label)       .global label
#define IMPORT_LABEL(label)       .extern label

#define FUNC_LABEL(func)          func:
#define LINE_LABEL(line)          line:

#define FUNC_DEF(func)               \
        .balign   8;                 \
        .type     func, %function;   \
func:

#define FUNC_END()                   \
        .ltorg
        
#define MACRO_DEF(mfunc...)          \
        .macro  mfunc
        
#define MACRO_END()                  \
        .endm

#define FILE_BEGIN()                 \
        .text;                       \
        .balign    8;

#define FILE_END()                   \
        .end
        
#define SECTION(sec)                 \
        .section   sec

#define WEAK(sym)                    \
        .weak      sym;

#define ARM_ISB()     isb sy
#define ARM_DSB()     dsb sy
#define ARM_DMB()     dmb sy

/*********************************************************************************************************
  ABI 相关定义
*********************************************************************************************************/

#define LR      X30

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __ASMARM64_ASSEMBLER_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
