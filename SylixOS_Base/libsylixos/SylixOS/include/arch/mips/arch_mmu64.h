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
** ��   ��   ��: arch_mmu64.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS64 �ڴ�������.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_MMU64_H
#define __MIPS_ARCH_MMU64_H

/*********************************************************************************************************
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 1                         /*  �Ƿ���Ҫ 4 ��ҳ��֧��       */

/*********************************************************************************************************
  �����ڴ�ҳ���������
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 LW_CFG_MIPS_PAGE_SHIFT    /*  2^n                         */
#define LW_CFG_VMM_PAGE_SIZE                  (__CONST64(1) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PTE_SHIFT                  LW_CFG_VMM_PAGE_SHIFT
#define LW_CFG_VMM_PTE_SIZE                   LW_CFG_VMM_PAGE_SIZE

/*********************************************************************************************************
 * +------------+------------+------------+------------+------------+
 * |6          4|4          3|3          2|2          1|1          0|
 * |3    15b   9|8    14b   5|4   14b    1|0    9b    2|1    12b   0|
 * +----------------------------------------------------------------+
 * |   PGD      |    PMD     |    PTS     |    PTE     |   OFFSET   |
 * +------------+------------+------------+------------+------------+
*********************************************************************************************************/
#if LW_CFG_MIPS_PAGE_SHIFT == 12

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x1ff) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PTE_BLKSIZE                (4 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PTS_SHIFT                  21
#define LW_CFG_VMM_PTS_SIZE                   (__CONST64(1) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (__CONST64(0x3fff) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_BLKSIZE                (128 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PMD_SHIFT                  35
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0x3fff) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_BLKSIZE                (128 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PGD_SHIFT                  50
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0x7fff) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_BLKSIZE                (256 * LW_CFG_KB_SIZE)

/*********************************************************************************************************
 * +------------+------------+------------+------------+------------+
 * |6          5|5          3|3          2|2          1|1          0|
 * |3    13b   1|0    13b   8|7   13b    5|4    11b   4|3    14b   0|
 * +----------------------------------------------------------------+
 * |   PGD      |    PMD     |    PTS     |    PTE     |   OFFSET   |
 * +------------+------------+------------+------------+------------+
*********************************************************************************************************/
#elif LW_CFG_MIPS_PAGE_SHIFT == 14

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x7ff) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PTE_BLKSIZE                (16 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PTS_SHIFT                  25
#define LW_CFG_VMM_PTS_SIZE                   (__CONST64(1) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_BLKSIZE                (64 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PMD_SHIFT                  38
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_BLKSIZE                (64 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PGD_SHIFT                  51
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_BLKSIZE                (64 * LW_CFG_KB_SIZE)

/*********************************************************************************************************
 * +------------+------------+------------+------------+------------+
 * |6          5|5          4|3          2|2          1|1          0|
 * |3    12b   2|1    12b   0|9   11b    9|8    13b   6|5    16b   0|
 * +----------------------------------------------------------------+
 * |   PGD      |    PMD     |    PTS     |    PTE     |   OFFSET   |
 * +------------+------------+------------+------------+------------+
*********************************************************************************************************/
#elif LW_CFG_MIPS_PAGE_SHIFT == 16

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x1fff) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PTE_BLKSIZE                (64 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PTS_SHIFT                  29
#define LW_CFG_VMM_PTS_SIZE                   (__CONST64(1) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (__CONST64(0x7ff) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_BLKSIZE                (16 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PMD_SHIFT                  40
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0xfff) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_BLKSIZE                (32 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PGD_SHIFT                  52
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0xfff) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_BLKSIZE                (32 * LW_CFG_KB_SIZE)

/*********************************************************************************************************
 * +------------+------------+------------+------------+------------+
 * |6          5|5          4|3          2|2          1|1          0|
 * |3    12b   2|1    12b   0|9   11b    9|8    11b   8|7    18b   0|
 * +----------------------------------------------------------------+
 * |   PGD      |    PMD     |    PTS     |    PTE     |   OFFSET   |
 * +------------+------------+------------+------------+------------+
*********************************************************************************************************/
#elif LW_CFG_MIPS_PAGE_SHIFT == 18

#define LW_CFG_VMM_PTE_MASK                   (__CONST64(0x7ff) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PTE_BLKSIZE                (16 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PTS_SHIFT                  29
#define LW_CFG_VMM_PTS_SIZE                   (__CONST64(1) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (__CONST64(0x7ff) << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_BLKSIZE                (16 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PMD_SHIFT                  40
#define LW_CFG_VMM_PMD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (__CONST64(0xfff) << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_BLKSIZE                (32 * LW_CFG_KB_SIZE)

#define LW_CFG_VMM_PGD_SHIFT                  52
#define LW_CFG_VMM_PGD_SIZE                   (__CONST64(1) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (__CONST64(0xfff) << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_BLKSIZE                (32 * LW_CFG_KB_SIZE)

#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K, 256K)!
#endif

/*********************************************************************************************************
  �ڴ��������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   8                         /*  ���������                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  ���������                  */

/*********************************************************************************************************
  MMU ת����Ŀ����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT64  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */
typedef UINT64  LW_PTS_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */
typedef UINT64  LW_PTE_TRANSENTRY;                                      /*  ҳ����Ŀ����                */

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __MIPS_ARCH_MMU64_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
