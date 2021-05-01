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
;** 文   件   名: x86AtomicAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2018 年 07 月 30 日
;**
;** 描        述: x86 体系构架处理器原子量操作.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    EXPORT_LABEL(archAtomicAdd)
    EXPORT_LABEL(archAtomicSub)
    EXPORT_LABEL(archAtomicAnd)
    EXPORT_LABEL(archAtomicOr)
    EXPORT_LABEL(archAtomicXor)

#if LW_CFG_CPU_ATOMIC64_EN > 0
    EXPORT_LABEL(archAtomic64SetCx8)
    EXPORT_LABEL(archAtomic64GetCx8)
    EXPORT_LABEL(archAtomic64AddReturnCx8)
    EXPORT_LABEL(archAtomic64SubReturnCx8)
#endif

;/*********************************************************************************************************
;  原子量加
;*********************************************************************************************************/

FUNC_DEF(archAtomicAdd)
    MOVL        X86_SP_ARG1(%ESP) , %EAX                                ;/*  值                          */
    MOVL        X86_SP_ARG2(%ESP) , %EDX                                ;/*  地址                        */
    MOVL        %EAX , %ECX
    LOCK
    XADDL       %EAX , (%EDX)
    ADDL        %ECX , %EAX
    RET
    FUNC_END(archAtomicAdd)

;/*********************************************************************************************************
;  原子量减
;*********************************************************************************************************/

FUNC_DEF(archAtomicSub)
    MOVL        X86_SP_ARG1(%ESP) , %EAX                                ;/*  值                          */
    MOVL        X86_SP_ARG2(%ESP) , %EDX                                ;/*  地址                        */
    NEGL        %EAX
    MOVL        %EAX , %ECX
    LOCK
    XADDL       %EAX , (%EDX)
    ADDL        %ECX , %EAX
    RET
    FUNC_END(archAtomicSub)

;/*********************************************************************************************************
;  原子量与
;*********************************************************************************************************/

FUNC_DEF(archAtomicAnd)
    MOVL        X86_SP_ARG2(%ESP) , %EDX                                ;/*  地址                        */
    MOVL        (%EDX), %EAX                                            ;/*  旧值                        */
LINE_LABEL(1)
    MOVL        X86_SP_ARG1(%ESP) , %ECX                                ;/*  值                          */
    ANDL        %EAX , %ECX
    LOCK
    CMPXCHGL    %ECX, (%EDX)
    JNZ         1b
    MOVL        %ECX , %EAX
    RET
    FUNC_END(archAtomicAnd)

;/*********************************************************************************************************
;  原子量或
;*********************************************************************************************************/

FUNC_DEF(archAtomicOr)
    MOVL        X86_SP_ARG2(%ESP) , %EDX                                ;/*  地址                        */
    MOVL        (%EDX), %EAX                                            ;/*  旧值                        */
LINE_LABEL(1)
    MOVL        X86_SP_ARG1(%ESP) , %ECX                                ;/*  值                          */
    ORL         %EAX , %ECX
    LOCK
    CMPXCHGL    %ECX, (%EDX)
    JNZ         1b
    MOVL        %ECX , %EAX
    RET
    FUNC_END(archAtomicOr)

;/*********************************************************************************************************
;  原子量异或
;*********************************************************************************************************/

FUNC_DEF(archAtomicXor)
    MOVL        X86_SP_ARG2(%ESP) , %EDX                                ;/*  地址                        */
    MOVL        (%EDX), %EAX                                            ;/*  旧值                        */
LINE_LABEL(1)
    MOVL        X86_SP_ARG1(%ESP) , %ECX                                ;/*  值                          */
    XORL        %EAX , %ECX
    LOCK
    CMPXCHGL    %ECX, (%EDX)
    JNZ         1b
    MOVL        %ECX , %EAX
    RET
    FUNC_END(archAtomicXor)

;/*********************************************************************************************************
;  64 位原子量
;*********************************************************************************************************/

#if LW_CFG_CPU_ATOMIC64_EN > 0

MACRO_DEF(READ64  reg)
    MOVL        %EBX , %EAX
    MOVL        %ECX , %EDX
    ;/* we need LOCK since otherwise cmpxchg8b always does the write */
    LOCK
    CMPXCHG8B   (\reg)
    MACRO_END()

FUNC_DEF(archAtomic64GetCx8)
    READ64      %ECX
    RET
    FUNC_END(archAtomic64GetCx8)

FUNC_DEF(archAtomic64SetCx8)
LINE_LABEL(1)
    ;/* we don't need LOCK since aligned 64-bit writes are atomic on 586 and newer */
    CMPXCHG8B   (%ESI)
    JNE         1b
    RET
    FUNC_END(archAtomic64SetCx8)

MACRO_DEF(ADD_SUB_RET_CX8  func  ins  insc)
FUNC_DEF(archAtomic64\func)
    PUSHL       %EBP
    PUSHL       %EBX
    PUSHL       %ESI
    PUSHL       %EDI

    MOVL        %EAX , %ESI
    MOVL        %EDX , %EDI
    MOVL        %ECX , %EBP

    READ64      %ECX
LINE_LABEL(1)
    MOVL        %EAX , %EBX
    MOVL        %EDX , %ECX
    \ins\()L    %ESI , %EBX
    \insc\()L   %EDI , %ECX
    LOCK
    CMPXCHG8B   (%EBP)
    JNE         1b

    MOVL        %EBX , %EAX
    MOVL        %ECX , %EDX
    POPL        %EDI
    POPL        %ESI
    POPL        %EBX
    POPL        %EBP
    RET
    FUNC_END(archAtomic64\func)
    MACRO_END()

ADD_SUB_RET_CX8  AddReturnCx8  ADD  ADC
ADD_SUB_RET_CX8  SubReturnCx8  SUB  SBB

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
