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
;** 文   件   名: armCacheV8Asm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 02 月 26 日
;**
;** 描        述: ARMv8 体系架构 CACHE 通用函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>
#include <arch/arm64/arch_regs.h>

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN() 

    EXPORT_LABEL(arm64CacheCCSIDR)  
    EXPORT_LABEL(arm64CacheSetCSSELR)
    EXPORT_LABEL(arm64ICacheEnable)
    EXPORT_LABEL(arm64DCacheEnable)
    EXPORT_LABEL(arm64ICacheDisable)
    EXPORT_LABEL(arm64DCacheDisable)
    EXPORT_LABEL(arm64DCacheFlush)
    EXPORT_LABEL(arm64DCacheFlushAll)
    EXPORT_LABEL(arm64DCacheFlushPoU)
    EXPORT_LABEL(arm64DCacheClear)
    EXPORT_LABEL(arm64DCacheClearAll)
    EXPORT_LABEL(arm64DCacheInvalidate)
    EXPORT_LABEL(arm64DCacheInvalidateAll)
    EXPORT_LABEL(arm64ICacheInvalidate)
    EXPORT_LABEL(arm64ICacheInvalidateAll)
    EXPORT_LABEL(arm64ICacheInvalidateAllInner)

;/*********************************************************************************************************
;  ARM64 获得 CACHE 相关寄存器
;*********************************************************************************************************/

FUNC_DEF(arm64CacheCCSIDR)
    MRS     X0,  CCSIDR_EL1
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 设置 CACHE 相关寄存器
;*********************************************************************************************************/

FUNC_DEF(arm64CacheSetCSSELR)
    MSR     CSSELR_EL1, X0
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(arm64ICacheEnable) 
    MOV     X13 , LR
    BL      arm64ICacheInvalidateAll
    MRS     X0  , SCTLR_EL1
    ORR     X0  , X0,  #(1 << 12)
    MSR     SCTLR_EL1, X0
    MOV     LR  , X13
    RET
    FUNC_END()

FUNC_DEF(arm64DCacheEnable)
    MRS     X0 , SCTLR_EL1
    ORR     X0 , X0, #(1 << 2)
    MSR     SCTLR_EL1 , X0
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(arm64ICacheDisable)
    MRS     X0 , SCTLR_EL1
    BIC     X0 , X0 , #(1 << 12)
    MSR     SCTLR_EL1 , X0
    RET
    FUNC_END()

FUNC_DEF(arm64DCacheDisable)
    MRS     X0  , SCTLR_EL1
    AND     X0  , X0 , #(1 << 2)
    CMP     X0  , #0
    MOV     X13 , LR                                                    ;/*  记录返回地址                */
    BEQ     dcache_not_en
    BL      arm64DCacheClearAll

    ARM_DSB()
    ARM_ISB()

    MRS     X0 ,  SCTLR_EL1
    BIC     X0 ,  X0 , #(1 << 2)
    MSR     SCTLR_EL1 , X0
    MOV     LR , X13
    RET

LINE_LABEL(dcache_not_en)            
    BL      arm64DCacheInvalidateAll  
    MOV     LR , X13
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARMv8 操作 DCACHE
;*********************************************************************************************************/

MACRO_DEF(armDCacheV8Op  Op  Kstart  Kend   LineSize  Tmp) 
    SUB     \Tmp ,    \LineSize , #1                                    ;/*  Tmp 存储 Cache Line 掩码    */
    BIC     \Kstart , \Kstart ,   \Tmp                                  ;/*  取对齐地址                  */
1:
    DC      \Op ,     \Kstart                                           ;/*  进行 DCache 设置操作        */
    ADD     \Kstart , \Kstart ,   \LineSize                             ;/*  每次移动 Cache Line 大小    */
    CMP     \Kstart , \Kend                                             ;/*  是否到达 End                */
    B.LO    1b
    ARM_DSB()
    RET
    MACRO_END()

;/*********************************************************************************************************
;  ARMv8 回写 部分 DCACHE      X0=void *start; X1=void *end; X2=cache line size
;*********************************************************************************************************/
FUNC_DEF(arm64DCacheFlushPoU)
    armDCacheV8Op   CVAU  X0  X1  X2  X3
    FUNC_END()

FUNC_DEF(arm64DCacheFlush)
    armDCacheV8Op   CVAC  X0  X1  X2  X3
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 回写并无效部分 DCACHE X0=void *start; X1=void *end; X2=cache line size
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheClear)
    armDCacheV8Op   CIVAC  X0  X1  X2  X3
    FUNC_END()
    
;/*********************************************************************************************************
;  ARM64 无效 部分 DCACHE
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheInvalidate)
    armDCacheV8Op   IVAC  X0  X1  X2  X3
    FUNC_END()

;/*********************************************************************************************************
;  ARMv8 DCACHE All 相关操作
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheLevel)
    LSL     X12 , X0 , #1                                               ;/*  CSSELR_EL1 3-1 为 Level 选择*/
    MSR     CSSELR_EL1 , X12                                            ;/*  选择 Cache Level            */
    ARM_ISB()                                                           ;/*  同步 CCSIDR_EL1             */
    MRS     X6 , CCSIDR_EL1                                             ;/*  读取 CCSIDR_EL1             */
    AND     X2 , X6 , #7                                                ;/*  CCSIDR_EL1 2-0 为 LineSize-4*/
    ADD     X2 , X2 , #4                                                ;/*  X2 记录 LineSize            */
    MOV     X3 , #0x3ff                                                 ;/*  CCSIDR_EL1 12-3  为 ways - 1*/
    AND     X3 , X3 , X6 , LSR #3                                       ;/*  X3 记录最大的 ways          */
    CLZ     W5 , W3                                                     ;/*  记录 ways 的 bit 数         */
    MOV     X4 , #0x7fff                                                ;/*  CCSIDR_EL1 27-13 为 sets - 1*/
    AND     X4 , X4 , X6 , LSR #13                                      ;/*  X4 记录最大的 sets          */
                                                                        ;/*  X12 <- cache level << 1     */
                                                                        ;/*  X2  <- line size            */
                                                                        ;/*  X3  <- number of ways - 1   */
                                                                        ;/*  X4  <- number of sets - 1   */
                                                                        ;/*  X5  <- bit position of #ways*/
                                                                        ;/*  X1 = 0 clean only,          */
                                                                        ;/*  X1 = 1 clean and invalidate */
                                                                        ;/*  X1 = 2 invalidate only      */

LINE_LABEL(loop_set)                                                    ;/*  依次处理每个 set            */
    MOV     X6 , X3                                                     ;/*  X6 <- working copy of #ways */
LINE_LABEL(loop_way)                                                    ;/*  依次处理每个 way            */
    LSL     X7 , X6 , X5                                                ;/*  CISW 3 - 1 为 Level         */
    ORR     X9 , X12, X7                                                ;/*  CISW 31- 4 为 SetWay        */
    LSL     X7 , X4 , X2
    ORR     X9 , X9 , X7                                                ;/*  详见 SetWay 说明            */
    TBZ     W1 , #0 , 1f                                                ;/*  判断 X1                     */
    CMP     X1 , #2
    BNE     2f                                                          ;/*  如果不是 "只是无效" 则跳转  */
    DC      ISW, X9                                                     ;/*  只是 无效  DCache           */
    B       3f
1:
    DC      CSW  ,  X9                                                  ;/*  回写 DCache                 */
2:
    DC      CISW , X9                                                   ;/*  回写并无效 DCache           */
3:
    SUBS    X6 , X6 , #1                                                ;/*  递减 way                    */
    B.GE    loop_way
    SUBS    X4 , X4 , #1                                                ;/*  递减 set                    */
    B.GE    loop_set

    RET
    FUNC_END()

FUNC_DEF(arm64DCacheAll)
    MOV     X1  , X0
    ARM_DSB()
    MRS     X10 , CLIDR_EL1                                             ;/*  读取 Cache Level ID         */
    LSR     X11 , X10 , #24                                             ;/*  CLIDR_EL1 24位为 一致性级别 */
    AND     X11 , X11 , #0x7                                            ;/*  获取一致性级别（LOC）       */
    CBZ     X11 , finished                                              ;/*  如果 LOC 为 0，返回         */
    MOV     X15 , LR                                                    ;/*  记录返回地址                */
    MOV     X0  , #0                                                    ;/*  开始 flush level 0 的 Cache */
                                                                        ;/*  X0  <- cache level          */
                                                                        ;/*  X10 <- clidr_el1            */
                                                                        ;/*  X11 <- LOC                  */
                                                                        ;/*  x15 <- return address       */
LINE_LABEL(loop_level)
    LSL     X12 , X0  , #1                                              ;/*  X12 <- Cache level << 1     */
    ADD     X12 , X12 , X0                                              ;/*  X12 <- 3 倍的 Cache level   */
    LSR     X12 , X10 , X12
    AND     X12 , X12 , #7                                              ;/*  X12 记录 Cache Type         */
    CMP     X12 , #2                                                    ;/*  比较 Cache 类型是否为 DCache*/
    B.LT    skip                                                        ;/*  如果不是 DCache 则跳过      */
    BL      arm64DCacheLevel                                            ;/*  X1 = 0 clean only,          */
                                                                        ;/*  X1 = 1 clean and invalidate */
                                                                        ;/*  X1 = 2 invalidate only      */
LINE_LABEL(skip)
    ADD     X0  , X0 , #1                                               ;/*  Cache Level 递增            */
    CMP     X11 , X0                                                    ;/*  比较 Cache Level 到达最大   */
    B.GT    loop_level
    MOV     X0  , #0
    MSR     CSSELR_EL1 , X0                                             ;/*  重置 CSSELR_EL1             */
    ARM_DSB()
    ARM_ISB()
    MOV     LR  , X15
      
LINE_LABEL(finished)
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARMv8 回写 全部 DCACHE
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheFlushAll)
    MOV     X14 , LR                                                    ;/*  记录返回地址                */
    MOV     X0  , #0
    BL      arm64DCacheAll
    MOV     LR  , X14
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARMv8 回写并无效 全部 DCACHE
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheClearAll)
    MOV     X14 , LR                                                    ;/*  记录返回地址                */
    MOV     X0  , #1
    BL      arm64DCacheAll
    MOV     LR  , X14
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 无效 全部 DCACHE
;*********************************************************************************************************/

FUNC_DEF(arm64DCacheInvalidateAll)
    MOV     X14 , LR                                                    ;/*  记录返回地址                */
    MOV     X0  , #1
    BL      arm64DCacheAll
    MOV     LR  , X14
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 无效整个 ICACHE
;*********************************************************************************************************/

FUNC_DEF(arm64ICacheInvalidateAll)
    IC      IALLU
    ARM_ISB()
    RET
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv8 无效整个 ICACHE 和 Inner Shareable
;*********************************************************************************************************/

FUNC_DEF(arm64ICacheInvalidateAllInner)
    IC      IALLUIS
    ARM_ISB()
    RET
    FUNC_END()

;/*********************************************************************************************************
;  ARM64 无效 ICACHE
;*********************************************************************************************************/

FUNC_DEF(arm64ICacheInvalidate)
    SUB     X3  , X2 , #1                                               ;/*  X3 存储 Cache Line 掩码     */
    BIC     X0  , X0 , X3                                               ;/*  取对齐地址                  */
1:
    IC      IVAU, X0                                                    ;/*  进行 ICache 设置操作        */
    ADD     X0  , X0 , X2                                               ;/*  每次移动 Cache Line 大小    */
    CMP     X0  , X1                                                    ;/*  是否到达 End                */
    B.LO    1b
    DSB     ISH
    ARM_ISB()
    MOV     X0  , #0
    RET
    FUNC_END()

    FILE_END()
    
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
