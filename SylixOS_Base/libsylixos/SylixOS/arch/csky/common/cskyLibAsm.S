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
;** 文   件   名: cskyLibAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 05 月 11 日
;**
;** 描        述: C-SKY 体系架构内部库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/cpu/cpu_cfg.h>
#include "arch/csky/arch_regs.h"

    FILE_BEGIN()

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)

    EXPORT_LABEL(archGetPSR)
    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)
    EXPORT_LABEL(archReboot)
    EXPORT_LABEL(archBogoMipsLoop)
    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  C-SKY 获取处理器状态寄存器
;*********************************************************************************************************/

FUNC_DEF(archGetPSR)
    MFCR        A0 , PSR                                                ;/*  结果存放在 A0 中            */
    RTS
    FUNC_END(archGetPSR)

;/*********************************************************************************************************
;  C-SKY 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    MFCR        A0 , PSR
    PSRCLR      IE
    RTS
    FUNC_END(archIntDisable)

;/*********************************************************************************************************
;  C-SKY 打开总中断
;*********************************************************************************************************/

FUNC_DEF(archIntEnable)
    ANDI        A0 , A0 , M_PSR_IE
    BEZ         A0 , 1f
    PSRSET      IE
1:
    RTS
    FUNC_END(archIntEnable)

;/*********************************************************************************************************
;  C-SKY 强制打开总中断
;*********************************************************************************************************/

FUNC_DEF(archIntEnableForce)
    PSRSET      IE
    RTS
    FUNC_END(archIntEnableForce)

;/*********************************************************************************************************
;  C-SKY 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)                                                   ;/*  查找 32 位中最低的 1 位     */
    MOVI        A1 , 0
    SUBU        A1 , A0
    AND         A0 , A1
    FF1         A1 , A0                                                 ;/*  结果存放在 A1 中            */
    MOVI        A2 , 32
    SUBU        A0 , A2 , A1
    RTS
    FUNC_END(archFindLsb)

FUNC_DEF(archFindMsb)                                                   ;/*  查找 32 位中最高的 1 位     */
    FF1         A1 , A0                                                 ;/*  结果存放在 A1 中            */
    MOVI        A2 , 32
    SUBU        A0 , A2 , A1
    RTS
    FUNC_END(archFindMsb)

;/*********************************************************************************************************
;  调用设置函数宏
;*********************************************************************************************************/

MACRO_DEF(CALL_SETUP, setup)
    SUBI        SP , (4 * ARCH_REG_SIZE)
    ST.W        RA , (SP, 0 * ARCH_REG_SIZE)
    ST.W        A0 , (SP, 1 * ARCH_REG_SIZE)
    ST.W        A1 , (SP, 2 * ARCH_REG_SIZE)

    JSRI        \setup

    LD.W        RA , (SP, 0 * ARCH_REG_SIZE)
    LD.W        A0 , (SP, 1 * ARCH_REG_SIZE)
    LD.W        A1 , (SP, 2 * ARCH_REG_SIZE)
    ADDI        SP , (4 * ARCH_REG_SIZE)
    MACRO_END()

;/*********************************************************************************************************
;  保存寄存器宏
;  注意: 目前仅适合 ABIv2
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)
    ST.W        R4  , (A0 , XGREG(4))
    ST.W        R5  , (A0 , XGREG(5))
    ST.W        R6  , (A0 , XGREG(6))
    ST.W        R7  , (A0 , XGREG(7))
    ST.W        R8  , (A0 , XGREG(8))
    ST.W        R9  , (A0 , XGREG(9))
    ST.W        R10 , (A0 , XGREG(10))
    ST.W        R11 , (A0 , XGREG(11))
    ST.W        R14 , (A0 , XGREG(14))
    ST.W        R15 , (A0 , XGREG(15))
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
    ST.W        R16 , (A0 , XGREG(16))
    ST.W        R17 , (A0 , XGREG(17))
    ST.W        R26 , (A0 , XGREG(26))
    ST.W        R27 , (A0 , XGREG(27))
    ST.W        R28 , (A0 , XGREG(28))
    ST.W        R29 , (A0 , XGREG(29))
    ST.W        R30 , (A0 , XGREG(30))
    ST.W        R31 , (A0 , XGREG(31))
#else
    ST.W        R28 , (A0 , XGREG28)
#endif
    MACRO_END()

;/*********************************************************************************************************
;  恢复寄存器宏
;  注意: 目前仅适合 ABIv2
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)
    LD.W        R4  , (A0 , XGREG(4))
    LD.W        R5  , (A0 , XGREG(5))
    LD.W        R6  , (A0 , XGREG(6))
    LD.W        R7  , (A0 , XGREG(7))
    LD.W        R8  , (A0 , XGREG(8))
    LD.W        R9  , (A0 , XGREG(9))
    LD.W        R10 , (A0 , XGREG(10))
    LD.W        R11 , (A0 , XGREG(11))
    LD.W        R14 , (A0 , XGREG(14))
    LD.W        R15 , (A0 , XGREG(15))
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
    LD.W        R16 , (A0 , XGREG(16))
    LD.W        R17 , (A0 , XGREG(17))
    LD.W        R26 , (A0 , XGREG(26))
    LD.W        R27 , (A0 , XGREG(27))
    LD.W        R28 , (A0 , XGREG(28))
    LD.W        R29 , (A0 , XGREG(29))
    LD.W        R30 , (A0 , XGREG(30))
    LD.W        R31 , (A0 , XGREG(31))
#else
    LD.W        R28 , (A0 , XGREG28)
#endif
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    CALL_SETUP  __sigsetjmpSetup

    SAVE_NON_VOLATILE_REG

    MOVI        A0 , 0
    RTS
    FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    CALL_SETUP  __setjmpSetup

    SAVE_NON_VOLATILE_REG

    MOVI        A0 , 0
    RTS
    FUNC_END(setjmp)

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    CALL_SETUP  __siglongjmpSetup

    RESTORE_NON_VOLATILE_REG

    MOV         A0 , A1
    CMPNEI      A0 , 0
    BT          siglongjmpnotzero
    MOVI        A0 , 1
LINE_LABEL(siglongjmpnotzero)
    RTS
    FUNC_END(siglongjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    CALL_SETUP  __longjmpSetup

    RESTORE_NON_VOLATILE_REG

    MOV         A0 , A1
    CMPNEI      A0 , 0
    BT          longjmpnotzero
    MOVI        A0 , 1
LINE_LABEL(longjmpnotzero)
    RTS
    FUNC_END(longjmp)

;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    JMPI          bspReboot                                             ;/*  由BSP进行复位操作, 不再返回 */
    FUNC_END(archReboot)

;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
    MOVI        A1 , 1
    MOVI        A2 , 2
    MOVI        A3 , 3
    MOVI        A1 , 1
    MOVI        A2 , 2
    MOVI        A3 , 3
    SUBI        A0 , 1
    BNEZ        A0 , archBogoMipsLoop
    RTS
    FUNC_END(archBogoMipsLoop)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
