/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armCacheV7M.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 08 日
**
** 描        述: ARMv7 Cortex-Mx 体系构架 CACHE 驱动.
**
** CHANGELOG:
2018.01.02 Hou.JinYu (侯进宇)    实现 Cortex-M7 体系架构 CACHE 驱动.
2018.01.08 Jiao.JinXing (焦进星) 加入 SCB CACHE 操作.
2018.07.11 Han.Hui (韩辉) 加入 Cache line size 探测.
2018.10.30 Jiao.JinXing (焦进星) 修正 TextUpdate 问题.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
#include "../../mpu/v7m/armMpuV7M.h"
#endif
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_ARM_CACHE_L2 > 0
#include "../l2/armL2.h"
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT      iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  CACHE 参数
*********************************************************************************************************/
static UINT32                           uiArmV7MICacheLineSize;
static UINT32                           uiArmV7MDCacheLineSize;
#define ARMv7M_CACHE_LOOP_OP_MAX_SIZE   (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  Structure type to access the System Control Block (SCB).
*********************************************************************************************************/
typedef struct {
    UINT32  CPUID;                          /*  CPUID Base Register                                     */
    UINT32  ICSR;                           /*  Interrupt Control and State Register                    */
    UINT32  VTOR;                           /*  Vector Table Offset Register                            */
    UINT32  AIRCR;                          /*  Application Interrupt and Reset Control Register        */
    UINT32  SCR;                            /*  System Control Register                                 */
    UINT32  CCR;                            /*  Configuration Control Register                          */
    UINT8   SHPR[12U];                      /*  System Handlers Priority Registers (4-7, 8-11, 12-15)   */
    UINT32  SHCSR;                          /*  System Handler Control and State Register               */
    UINT32  CFSR;                           /*  Configurable Fault Status Register                      */
    UINT32  HFSR;                           /*  HardFault Status Register                               */
    UINT32  DFSR;                           /*  Debug Fault Status Register                             */
    UINT32  MMFAR;                          /*  MemManage Fault Address Register                        */
    UINT32  BFAR;                           /*  BusFault Address Register                               */
    UINT32  AFSR;                           /*  Auxiliary Fault Status Register                         */
    UINT32  ID_PFR[2U];                     /*  Processor Feature Register                              */
    UINT32  ID_DFR;                         /*  Debug Feature Register                                  */
    UINT32  ID_AFR;                         /*  Auxiliary Feature Register                              */
    UINT32  ID_MFR[4U];                     /*  Memory Model Feature Register                           */
    UINT32  ID_ISAR[5U];                    /*  Instruction Set Attributes Register                     */
    UINT32  RESERVED0[1U];
    UINT32  CLIDR;                          /*  Cache Level ID register                                 */
    UINT32  CTR;                            /*  Cache Type register                                     */
    UINT32  CCSIDR;                         /*  Cache Size ID Register                                  */
    UINT32  CSSELR;                         /*  Cache Size Selection Register                           */
    UINT32  CPACR;                          /*  Coprocessor Access Control Register                     */
    UINT32  RESERVED3[93U];
    UINT32  STIR;                           /*  Software Triggered Interrupt Register                   */
    UINT32  RESERVED4[15U];
    UINT32  MVFR0;                          /*  Media and VFP Feature Register 0                        */
    UINT32  MVFR1;                          /*  Media and VFP Feature Register 1                        */
    UINT32  MVFR2;                          /*  Media and VFP Feature Register 2                        */
    UINT32  RESERVED5[1U];
    UINT32  ICIALLU;                        /*  I-Cache Invalidate All to PoU                           */
    UINT32  RESERVED6[1U];
    UINT32  ICIMVAU;                        /*  I-Cache Invalidate by MVA to PoU                        */
    UINT32  DCIMVAC;                        /*  D-Cache Invalidate by MVA to PoC                        */
    UINT32  DCISW;                          /*  D-Cache Invalidate by Set-way                           */
    UINT32  DCCMVAU;                        /*  D-Cache Clean by MVA to PoU                             */
    UINT32  DCCMVAC;                        /*  D-Cache Clean by MVA to PoC                             */
    UINT32  DCCSW;                          /*  D-Cache Clean by Set-way                                */
    UINT32  DCCIMVAC;                       /*  D-Cache Clean and Invalidate by MVA to PoC              */
    UINT32  DCCISW;                         /*  D-Cache Clean and Invalidate by Set-way                 */
    UINT32  RESERVED7[6U];
    UINT32  ITCMCR;                         /*  Instruction Tightly-Coupled Memory Control Register     */
    UINT32  DTCMCR;                         /*  Data Tightly-Coupled Memory Control Registers           */
    UINT32  AHBPCR;                         /*  AHBP Control Register                                   */
    UINT32  CACR;                           /*  L1 Cache Control Register                               */
    UINT32  AHBSCR;                         /*  AHB Slave Control Register                              */
    UINT32  RESERVED8[1U];
    UINT32  ABFSR;                          /*  Auxiliary Bus Fault Status Register                     */
} SCB_Type;
/*********************************************************************************************************
  Memory mapping of Core Hardware
*********************************************************************************************************/
#define SCS_BASE                            (0xe000e000UL)              /*  System Control Space Address*/
#define SCB_BASE                            (SCS_BASE + 0x0d00UL)       /*  System Control Block Address*/

#define SCB                                 ((SCB_Type *)SCB_BASE)      /*  SCB configuration struct    */
/*********************************************************************************************************
  SCB Configuration Control Register Definitions
*********************************************************************************************************/
#define SCB_CCR_BP_Pos                      18U                         /*  Branch prediction enable bit*/
#define SCB_CCR_BP_Msk                      (1UL << SCB_CCR_BP_Pos)

#define SCB_CCR_IC_Pos                      17U                         /*  Instruction cache enable bit*/
#define SCB_CCR_IC_Msk                      (1UL << SCB_CCR_IC_Pos)

#define SCB_CCR_DC_Pos                      16U                         /*  Cache enable bit            */
#define SCB_CCR_DC_Msk                      (1UL << SCB_CCR_DC_Pos)

#define SCB_CCR_STKALIGN_Pos                9U                          /*  STKALIGN                    */
#define SCB_CCR_STKALIGN_Msk                (1UL << SCB_CCR_STKALIGN_Pos)

#define SCB_CCR_BFHFNMIGN_Pos               8U                          /*  BFHFNMIGN                   */
#define SCB_CCR_BFHFNMIGN_Msk               (1UL << SCB_CCR_BFHFNMIGN_Pos)

#define SCB_CCR_DIV_0_TRP_Pos               4U                          /*  DIV_0_TRP                   */
#define SCB_CCR_DIV_0_TRP_Msk               (1UL << SCB_CCR_DIV_0_TRP_Pos)

#define SCB_CCR_UNALIGN_TRP_Pos             3U                          /*  UNALIGN_TRP                 */
#define SCB_CCR_UNALIGN_TRP_Msk             (1UL << SCB_CCR_UNALIGN_TRP_Pos)

#define SCB_CCR_USERSETMPEND_Pos            1U                          /*  USERSETMPEND                */
#define SCB_CCR_USERSETMPEND_Msk            (1UL << SCB_CCR_USERSETMPEND_Pos)

#define SCB_CCR_NONBASETHRDENA_Pos          0U                          /*  NONBASETHRDENA              */
#define SCB_CCR_NONBASETHRDENA_Msk          (1UL << SCB_CCR_NONBASETHRDENA_Pos)
/*********************************************************************************************************
  SCB Cache Level ID Register Definitions
*********************************************************************************************************/
#define SCB_CLIDR_LOUU_Pos                  27U                         /*  LoUU Position               */
#define SCB_CLIDR_LOUU_Msk                  (7UL << SCB_CLIDR_LOUU_Pos)

#define SCB_CLIDR_LOC_Pos                   24U                         /*  LoC Position                */
#define SCB_CLIDR_LOC_Msk                   (7UL << SCB_CLIDR_LOC_Pos)
/*********************************************************************************************************
  SCB Cache Type Register Definitions
*********************************************************************************************************/
#define SCB_CTR_FORMAT_Pos                  29U                         /*  Format Position             */
#define SCB_CTR_FORMAT_Msk                  (7UL << SCB_CTR_FORMAT_Pos)

#define SCB_CTR_CWG_Pos                     24U                         /*  CWG Position                */
#define SCB_CTR_CWG_Msk                     (0xFUL << SCB_CTR_CWG_Pos)

#define SCB_CTR_ERG_Pos                     20U                         /*  ERG Position                */
#define SCB_CTR_ERG_Msk                     (0xFUL << SCB_CTR_ERG_Pos)

#define SCB_CTR_DMINLINE_Pos                16U                         /*  DminLine Position           */
#define SCB_CTR_DMINLINE_Msk                (0xFUL << SCB_CTR_DMINLINE_Pos)

#define SCB_CTR_IMINLINE_Pos                0U                          /*  ImInLine Position           */
#define SCB_CTR_IMINLINE_Msk                (0xFUL << SCB_CTR_IMINLINE_Pos)
/*********************************************************************************************************
  SCB Cache Size ID Register Definitions
*********************************************************************************************************/
#define SCB_CCSIDR_WT_Pos                   31U                         /*  WT Position                 */
#define SCB_CCSIDR_WT_Msk                   (1UL << SCB_CCSIDR_WT_Pos)

#define SCB_CCSIDR_WB_Pos                   30U                         /*  WB Position                 */
#define SCB_CCSIDR_WB_Msk                   (1UL << SCB_CCSIDR_WB_Pos)

#define SCB_CCSIDR_RA_Pos                   29U                         /*  RA Position                 */
#define SCB_CCSIDR_RA_Msk                   (1UL << SCB_CCSIDR_RA_Pos)

#define SCB_CCSIDR_WA_Pos                   28U                         /*  WA Position                 */
#define SCB_CCSIDR_WA_Msk                   (1UL << SCB_CCSIDR_WA_Pos)

#define SCB_CCSIDR_NUMSETS_Pos              13U                         /*  NumSets Position            */
#define SCB_CCSIDR_NUMSETS_Msk              (0x7FFFUL << SCB_CCSIDR_NUMSETS_Pos)

#define SCB_CCSIDR_ASSOCIATIVITY_Pos        3U                          /*  Associativity Position      */
#define SCB_CCSIDR_ASSOCIATIVITY_Msk        (0x3FFUL << SCB_CCSIDR_ASSOCIATIVITY_Pos)

#define SCB_CCSIDR_LINESIZE_Pos             0U                          /*  LineSize Position           */
#define SCB_CCSIDR_LINESIZE_Msk             (7UL << SCB_CCSIDR_LINESIZE_Pos)
/*********************************************************************************************************
  SCB Cache Size Selection Register Definitions
*********************************************************************************************************/
#define SCB_CSSELR_LEVEL_Pos                1U                          /*  Level Position              */
#define SCB_CSSELR_LEVEL_Msk                (7UL << SCB_CSSELR_LEVEL_Pos)

#define SCB_CSSELR_IND_Pos                  0U                          /*  InD Position                */
#define SCB_CSSELR_IND_Msk                  (1UL << SCB_CSSELR_IND_Pos)
/*********************************************************************************************************
  SCB D-Cache Invalidate by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCISW_WAY_Pos                   30U                         /*  Way Position                */
#define SCB_DCISW_WAY_Msk                   (3UL << SCB_DCISW_WAY_Pos)

#define SCB_DCISW_SET_Pos                   5U                          /*  Set Position                */
#define SCB_DCISW_SET_Msk                   (0x1FFUL << SCB_DCISW_SET_Pos)
/*********************************************************************************************************
  SCB D-Cache Clean by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCCSW_WAY_Pos                   30U                         /*  Way Position                */
#define SCB_DCCSW_WAY_Msk                   (3UL << SCB_DCCSW_WAY_Pos)

#define SCB_DCCSW_SET_Pos                   5U                          /*  Set Position                */
#define SCB_DCCSW_SET_Msk                   (0x1FFUL << SCB_DCCSW_SET_Pos)
/*********************************************************************************************************
  SCB D-Cache Clean and Invalidate by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCCISW_WAY_Pos                  30U                         /*  Way Position                */
#define SCB_DCCISW_WAY_Msk                  (3UL << SCB_DCCISW_WAY_Pos)

#define SCB_DCCISW_SET_Pos                  5U                          /*  Set Position                */
#define SCB_DCCISW_SET_Msk                  (0x1FFUL << SCB_DCCISW_SET_Pos)
/*********************************************************************************************************
  Cache Size ID Register Macros
*********************************************************************************************************/
#define CCSIDR_WAYS(x)                      \
    (((x) & SCB_CCSIDR_ASSOCIATIVITY_Msk) >> SCB_CCSIDR_ASSOCIATIVITY_Pos)
#define CCSIDR_SETS(x)                      \
    (((x) & SCB_CCSIDR_NUMSETS_Msk) >> SCB_CCSIDR_NUMSETS_Pos)
#define CCSIDR_LINESIZE(x)                  \
    (((x) & SCB_CCSIDR_LINESIZE_Msk) >> SCB_CCSIDR_LINESIZE_Pos)
/*********************************************************************************************************
** 函数名称: armICacheV7MEnable
** 功能描述: 使能 ICACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armICacheV7MEnable (VOID)
{
    armDsb();
    armIsb();
    SCB->ICIALLU = 0UL;                                                 /*  Invalidate I-Cache          */
    armDsb();
    armIsb();
    SCB->CCR |=  (UINT32)SCB_CCR_IC_Msk;                                /*  Enable I-Cache              */
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MEnable
** 功能描述: 使能 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MEnable (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCISW = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
                          ((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while (sets-- != 0U);
    armDsb();

    SCB->CCR |= (UINT32)SCB_CCR_DC_Msk;                                 /*  Enable D-Cache              */

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armICacheV7MDisable
** 功能描述: 禁能 ICACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armICacheV7MDisable (VOID)
{
    armDsb();
    armIsb();
    SCB->CCR &= ~(UINT32)SCB_CCR_IC_Msk;                                /*  Disable I-Cache             */
    armDsb();
    armIsb();
    SCB->ICIALLU = 0UL;                                                 /*  Invalidate I-Cache          */
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MDisable
** 功能描述: 禁能 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MDisable (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    SCB->CCR &= ~(UINT32)SCB_CCR_DC_Msk;                                /*  Disable D-Cache             */

    /*
     * Clean & invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCISW = (((sets << SCB_DCCISW_SET_Pos) & SCB_DCCISW_SET_Msk) |
                           ((ways << SCB_DCCISW_WAY_Pos) & SCB_DCCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while (sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armICacheV7MInvalidateAll
** 功能描述: 无效所有 ICACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armICacheV7MInvalidateAll (VOID)
{
    armDsb();
    armIsb();
    SCB->ICIALLU = 0UL;                                                 /*  I-Cache Invalid All to PoU  */
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MFlushAll
** 功能描述: 回写所有 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MFlushAll (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * D-Cache Clean by Set-way
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCSW = (((sets << SCB_DCCSW_SET_Pos) & SCB_DCCSW_SET_Msk) |
                          ((ways << SCB_DCCSW_WAY_Pos) & SCB_DCCSW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while (sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MFlushAllPoU
** 功能描述: 回写所有 DCACHE 到 PoU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MFlushAllPoU (VOID)
{
    armDCacheV7MFlushAll();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MClearAll
** 功能描述: 回写并无效所有 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MClearAll (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * D-Cache Clean and Invalidate by Set-way
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCISW = (((sets << SCB_DCCSW_SET_Pos) & SCB_DCCSW_SET_Msk) |
                           ((ways << SCB_DCCSW_WAY_Pos) & SCB_DCCSW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while (sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armICacheV7MInvalidate
** 功能描述: 无效指定区间的 ICACHE
** 输　入  : pvStart       起始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armICacheV7MInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    /*
     * I-Cache Invalidate by MVA to PoU
     */
    while (pvStart < pvEnd) {
        armDsb();

        SCB->ICIMVAU = (UINT32)pvStart;

        armDsb();
        armIsb();

        pvStart = (PVOID)((addr_t)pvStart + uiStep);
    }
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MInvalidate
** 功能描述: 无效指定区间的 DCACHE
** 输　入  : pvStart       起始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    armDsb();

    /*
     * D-Cache Invalidate by MVA to PoC
     */
    while (pvStart < pvEnd) {
        SCB->DCIMVAC = (UINT32)pvStart;
        pvStart = (PVOID)((addr_t)pvStart + uiStep);
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MFlush
** 功能描述: 回写指定区间的 DCACHE
** 输　入  : pvStart       起始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MFlush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    armDsb();

    /*
     * D-Cache Clean by MVA to PoC
     */
    while (pvStart < pvEnd) {
        SCB->DCCMVAC = (UINT32)pvStart;
        pvStart = (PVOID)((addr_t)pvStart + uiStep);
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armDCacheV7MClear
** 功能描述: 回写并无效指定区间的 DCACHE
** 输　入  : pvStart       起始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armDCacheV7MClear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    armDsb();

    /*
     * D-Cache Clean and Invalidate by MVA to PoC
     */
    while (pvStart < pvEnd) {
        SCB->DCCIMVAC = (UINT32)pvStart;
        pvStart = (PVOID)((addr_t)pvStart + uiStep);
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** 函数名称: armTextUpdateV7MPoU
** 功能描述: 指定区间进行 TextUpdate
** 输　入  : pvStart       起始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armTextUpdateV7MPoU (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    while (pvStart < pvEnd) {
        armDsb();

        SCB->DCCMVAU = (UINT32)pvStart;
        SCB->ICIMVAU = (UINT32)pvStart;

        armDsb();
        armIsb();

        pvStart = (PVOID)((addr_t)pvStart + uiStep);
    }
}
/*********************************************************************************************************
** 函数名称: armBranchPredictorV7MInvalidate
** 功能描述: 无效分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armBranchPredictorV7MInvalidate (VOID)
{
}
/*********************************************************************************************************
** 函数名称: armBranchPredictionV7MDisable
** 功能描述: 禁能分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armBranchPredictionV7MDisable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: armBranchPredictionV7MEnable
** 功能描述: 使能分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armBranchPredictionV7MEnable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: armCacheV7MEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheV7MEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionV7MEnable();

    } else {
        armDCacheV7MEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }

#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        armL2Enable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheV7MDisable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionV7MDisable();

    } else {
        armDCacheV7MDisable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }

#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        armL2Disable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv7M_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7MFlushAll();                                     /*  全部回写                    */

        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7MDCacheLineSize);
            armDCacheV7MFlush(pvAdrs, (PVOID)ulEnd,
                              uiArmV7MDCacheLineSize);                  /*  部分回写                    */
        }

#if LW_CFG_ARM_CACHE_L2 > 0
        armL2FlushAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7M_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheV7MInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7MICacheLineSize);
            armICacheV7MInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7MICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)uiArmV7MDCacheLineSize - 1)) {       /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV7MDCacheLineSize - 1);
                armDCacheV7MClear((PVOID)ulStart, (PVOID)ulStart, uiArmV7MDCacheLineSize);
                ulStart += uiArmV7MDCacheLineSize;
            }

            if (ulEnd & ((addr_t)uiArmV7MDCacheLineSize - 1)) {         /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV7MDCacheLineSize - 1);
                armDCacheV7MClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV7MDCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  仅无效对齐部分              */
                armDCacheV7MInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV7MDCacheLineSize);
            }

#if LW_CFG_ARM_CACHE_L2 > 0
            armL2Invalidate(pvAdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7M_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheV7MInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7MICacheLineSize);
            armICacheV7MInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7MICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv7M_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7MClearAll();                                     /*  全部回写并无效              */

        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7MDCacheLineSize);
            armDCacheV7MClear(pvAdrs, (PVOID)ulEnd,
                              uiArmV7MDCacheLineSize);                  /*  部分回写并无效              */
        }

#if LW_CFG_ARM_CACHE_L2 > 0
        armL2ClearAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  armCacheV7MTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv7M_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV7MFlushAllPoU();                                      /*  DCACHE 全部回写             */
        armICacheV7MInvalidateAll();                                    /*  ICACHE 全部无效             */

    } else {
        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7MDCacheLineSize);
        armTextUpdateV7MPoU(pvAdrs, (PVOID)ulEnd, uiArmV7MDCacheLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheV7MInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MInit (LW_CACHE_OP *pcacheop,
                               CACHE_MODE   uiInstruction,
                               CACHE_MODE   uiData,
                               CPCHAR       pcMachineName)
{
    UINT32  uiICCSIDR;
    UINT32  uiDCCSIDR;

#define ARMV7_CSSELR_IND_DATA_UNIFIED   0
#define ARMV7_CSSELR_IND_INSTRUCTION    1

#define ARMv7_CCSIDR_LINESIZE_MASK      0x7
#define ARMv7_CCSIDR_LINESIZE(x)        ((x) & ARMv7_CCSIDR_LINESIZE_MASK)
#define ARMv7_CACHE_LINESIZE(x)         (16 << ARMv7_CCSIDR_LINESIZE(x))

#define ARMv7_CCSIDR_NUMSET_MASK        0xfffe000
#define ARMv7_CCSIDR_NUMSET(x)          ((x) & ARMv7_CCSIDR_NUMSET_MASK)
#define ARMv7_CACHE_NUMSET(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 13) + 1)

#define ARMv7_CCSIDR_WAYNUM_MSK         0x1ff8
#define ARMv7_CCSIDR_WAYNUM(x)          ((x) & ARMv7_CCSIDR_WAYNUM_MSK)
#define ARMv7_CACHE_WAYNUM(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 3) + 1)

    if (lib_strcmp(pcMachineName, ARM_MACHINE_M7) != 0) {
        return;
    }

    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    SCB->CSSELR = ARMV7_CSSELR_IND_INSTRUCTION;
    uiICCSIDR   = SCB->CCSIDR;

    SCB->CSSELR = ARMV7_CSSELR_IND_DATA_UNIFIED;
    uiDCCSIDR   = SCB->CCSIDR;

    pcacheop->CACHEOP_iICacheLine = ARMv7_CACHE_LINESIZE(uiICCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv7_CACHE_LINESIZE(uiDCCSIDR);

    uiArmV7MICacheLineSize = (UINT32)pcacheop->CACHEOP_iICacheLine;
    uiArmV7MDCacheLineSize = (UINT32)pcacheop->CACHEOP_iDCacheLine;

    pcacheop->CACHEOP_iICacheWaySize = uiArmV7MICacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiICCSIDR);   /*  ICACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV7MDCacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiDCCSIDR);   /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7-M I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7-M D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = armCacheV7MEnable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7MDisable;

    pcacheop->CACHEOP_pfuncLock    = armCacheV7MLock;
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7MUnlock;

    pcacheop->CACHEOP_pfuncFlush          = armCacheV7MFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7MInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = LW_NULL;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7MClear;
    pcacheop->CACHEOP_pfuncClearPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7MTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = LW_NULL;

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7MDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7MDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7MDmaFree;
#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
}
/*********************************************************************************************************
** 函数名称: archCacheV7MReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MReset (CPCHAR  pcMachineName)
{
    if (lib_strcmp(pcMachineName, ARM_MACHINE_M7) != 0) {
        return;
    }

    armICacheV7MInvalidateAll();
    armDCacheV7MDisable();
    armICacheV7MDisable();
    armBranchPredictorV7MInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
