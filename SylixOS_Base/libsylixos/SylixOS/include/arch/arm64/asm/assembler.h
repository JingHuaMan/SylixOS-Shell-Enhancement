/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: assembler.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 22 ��
**
** ��        ��: ARM64 ������.
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
  ABI ��ض���
*********************************************************************************************************/

#define LR      X30

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __ASMARM64_ASSEMBLER_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
