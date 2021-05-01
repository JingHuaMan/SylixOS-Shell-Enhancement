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
** 文   件   名: arch_mmu.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 22 日
**
** 描        述: ARM64 内存管理相关.
*********************************************************************************************************/

#ifndef __ARM64_ARCH_MMU_H
#define __ARM64_ARCH_MMU_H

/*********************************************************************************************************
  L4 微内核虚拟机 MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  是否使用 L4 虚拟机 MMU      */

/*********************************************************************************************************
  是否需要内核超过 3 级页表支持
*********************************************************************************************************/

#if LW_CFG_ARM64_PAGE_SHIFT == 12
#define LW_CFG_VMM_PAGE_4L_EN                 1                         /*  需要 4 级页表支持           */
#elif LW_CFG_ARM64_PAGE_SHIFT == 16
#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  不需要 4 级页表支持         */
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 64K)!
#endif

/*********************************************************************************************************
  虚拟内存页表相关配置
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 LW_CFG_ARM64_PAGE_SHIFT   /*  2^n                         */
#define LW_CFG_VMM_PAGE_SIZE                  (__CONST64(1) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PTE_SHIFT                  LW_CFG_VMM_PAGE_SHIFT
#define LW_CFG_VMM_PTE_SIZE                   LW_CFG_VMM_PAGE_SIZE
#define LW_CFG_VMM_PTE_BLKSIZE                LW_CFG_VMM_PAGE_SIZE

/*********************************************************************************************************
 * 4K 页大小，采用四级页表
 *
 * +------------+------------+------------+------------+------------+
 * |47        39|38        30|29        21|20        12|11         0|
 * +----------------------------------------------------------------+
 * |    PGD     |    PMD     |    PTS     |    PTE     |   OFFSET   |
 * +------------+------------+------------+------------+------------+
*********************************************************************************************************/
#if LW_CFG_ARM64_PAGE_SHIFT == 12

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x1ff) << LW_CFG_VMM_PAGE_SHIFT)

#define LW_CFG_VMM_PTS_SHIFT                  21
#define LW_CFG_VMM_PTS_SIZE                   (__CONST64(1) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (__CONST64(0x1ff) << LW_CFG_VMM_PTS_SHIFT)

#define LW_CFG_VMM_PMD_SHIFT                  30
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0x1ff) << LW_CFG_VMM_PMD_SHIFT)

#define LW_CFG_VMM_PGD_SHIFT                  39
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0x1ff) << LW_CFG_VMM_PGD_SHIFT)

/*********************************************************************************************************
 * 64K 页大小，采用三级页表
 *
 * +------------+------------+------------+-------------------------+
 * |47        42|41        29|28        16|15                      0|
 * +----------------------------------------------------------------+
 * |    PGD     |    PMD     |    PTE     |          OFFSET         |
 * +------------+------------+------------+-------------------------+
*********************************************************************************************************/
#elif LW_CFG_ARM64_PAGE_SHIFT == 16

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PAGE_SHIFT)

#define LW_CFG_VMM_PMD_SHIFT                  29
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PMD_SHIFT)

#define LW_CFG_VMM_PGD_SHIFT                  42
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0x3f) << LW_CFG_VMM_PGD_SHIFT)

#endif                                                                  /*  LW_CFG_ARM64_PAGE_SHIFT==16 */
/*********************************************************************************************************
  内存分组数量
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   16                        /*  物理分区数                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  虚拟分区数                  */

/*********************************************************************************************************
  MMU 转换条目类型
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64  LW_PGD_TRANSENTRY;                                      /*  页目录类型                  */
typedef UINT64  LW_PMD_TRANSENTRY;                                      /*  中间页目录类型              */
typedef UINT64  LW_PTS_TRANSENTRY;                                      /*  PTS 页目录类型              */
typedef UINT64  LW_PTE_TRANSENTRY;                                      /*  页表条目类型                */

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARM64_ARCH_MMU_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
