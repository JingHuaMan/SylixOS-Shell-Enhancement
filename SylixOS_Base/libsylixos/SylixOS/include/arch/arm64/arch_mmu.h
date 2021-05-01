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
** ��   ��   ��: arch_mmu.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 22 ��
**
** ��        ��: ARM64 �ڴ�������.
*********************************************************************************************************/

#ifndef __ARM64_ARCH_MMU_H
#define __ARM64_ARCH_MMU_H

/*********************************************************************************************************
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#if LW_CFG_ARM64_PAGE_SHIFT == 12
#define LW_CFG_VMM_PAGE_4L_EN                 1                         /*  ��Ҫ 4 ��ҳ��֧��           */
#elif LW_CFG_ARM64_PAGE_SHIFT == 16
#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  ����Ҫ 4 ��ҳ��֧��         */
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 64K)!
#endif

/*********************************************************************************************************
  �����ڴ�ҳ���������
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 LW_CFG_ARM64_PAGE_SHIFT   /*  2^n                         */
#define LW_CFG_VMM_PAGE_SIZE                  (__CONST64(1) << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PTE_SHIFT                  LW_CFG_VMM_PAGE_SHIFT
#define LW_CFG_VMM_PTE_SIZE                   LW_CFG_VMM_PAGE_SIZE
#define LW_CFG_VMM_PTE_BLKSIZE                LW_CFG_VMM_PAGE_SIZE

/*********************************************************************************************************
 * 4K ҳ��С�������ļ�ҳ��
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
 * 64K ҳ��С����������ҳ��
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
  �ڴ��������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   16                        /*  ���������                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  ���������                  */

/*********************************************************************************************************
  MMU ת����Ŀ����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT64  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */
typedef UINT64  LW_PTS_TRANSENTRY;                                      /*  PTS ҳĿ¼����              */
typedef UINT64  LW_PTE_TRANSENTRY;                                      /*  ҳ����Ŀ����                */

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARM64_ARCH_MMU_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
