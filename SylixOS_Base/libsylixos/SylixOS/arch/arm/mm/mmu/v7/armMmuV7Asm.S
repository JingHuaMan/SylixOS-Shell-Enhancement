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
;** 文   件   名: armMmuV7Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv7 体系架构 MMU 函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>
#include <config/cpu/cpu_cfg_arm.h>

#if LW_CFG_VMM_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armMmuV7GetTTBCR)
    EXPORT_LABEL(armMmuV7SetTTBCR)

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
    EXPORT_LABEL(armMmuV7SetTTBase)
    EXPORT_LABEL(armMmuV7SetTTBase1)
    EXPORT_LABEL(armMmuV7GetTTBase)
    EXPORT_LABEL(armMmuV7GetTTBase1)
    EXPORT_LABEL(armMmuV7GetMAIR0)
    EXPORT_LABEL(armMmuV7GetMAIR1)
    EXPORT_LABEL(armMmuV7SetMAIR0)
    EXPORT_LABEL(armMmuV7SetMAIR1)
#endif
    
;/*********************************************************************************************************
;  This API get TTBCR register
;*********************************************************************************************************/

FUNC_DEF(armMmuV7GetTTBCR)
    MRC     p15, 0, R0, c2, c0, 2
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  This API set TTBCR register
;*********************************************************************************************************/

FUNC_DEF(armMmuV7SetTTBCR)
    MCR     p15, 0, R0, c2, c0, 2
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  设置页表基地址 TTBR0 为进程相关页表  TTBR1 为系统全局页表
;*********************************************************************************************************/
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0

FUNC_DEF(armMmuV7SetTTBase)
    ARM_DSB()
    ARM_ISB()
    MCRR    p15, 0, R0, R1, c2                                          ;/* Set TT Base                  */

    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

FUNC_DEF(armMmuV7SetTTBase1)
    ARM_DSB()
    ARM_ISB()
    MCRR    p15, 1, R0, R1, c2                                          ;/* Set TT Base                  */

    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

FUNC_DEF(armMmuV7GetTTBase)
    MRRC    p15, 0, R2, R3, c2
    STR     R2 , [R0]
    STR     R3 , [R1]
    BX      LR
    FUNC_END()

FUNC_DEF(armMmuV7GetTTBase1)
    MRRC    p15, 1, R2, R3, c2
    STR     R2 , [R0]
    STR     R3 , [R1]
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  获得域属性
;*********************************************************************************************************/

FUNC_DEF(armMmuV7GetMAIR0)
    MRC     p15, 0, R0, c10, c2, 0
    BX      LR
    FUNC_END()

FUNC_DEF(armMmuV7GetMAIR1)
    MRC     p15, 0, R0, c10, c2, 1
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  设置域属性
;*********************************************************************************************************/

#define MAIR(attr, mt)      ((attr) << ((mt) * 8))
#define MT_DEVICE_nGnRnE    0
#define MT_DEVICE_nGnRE     1
#define MT_DEVICE_nGRE      2
#define MT_DEVICE_GRE       3
#define MT_NORMAL_NC        0
#define MT_NORMAL           1
#define MT_NORMAL_WT        2

FUNC_DEF(armMmuV7SetMAIR0)
    LDR     R0 , =MAIR(0x00, MT_DEVICE_nGnRnE) | \
                  MAIR(0x04, MT_DEVICE_nGnRE)  | \
                  MAIR(0x08, MT_DEVICE_nGRE)   | \
                  MAIR(0x0c, MT_DEVICE_GRE)
    MCR     p15, 0, R0, c10, c2, 0
    BX      LR
    FUNC_END()

FUNC_DEF(armMmuV7SetMAIR1)
    LDR     R0 , =MAIR(0x44, MT_NORMAL_NC)     | \
                  MAIR(0xff, MT_NORMAL)        | \
                  MAIR(0xbb, MT_NORMAL_WT)
    MCR     p15, 0, R0, c10, c2, 1
    BX      LR
    FUNC_END()

#endif

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
