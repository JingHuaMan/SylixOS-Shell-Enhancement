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
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ARM �ڴ�������.
**
** BUG:
2015.04.01 �� VMM ҳ��ߴ���ص������ƶ�������.
*********************************************************************************************************/

#ifndef __ARM_ARCH_MMU_H
#define __ARM_ARCH_MMU_H

/*********************************************************************************************************
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  �Ƿ���Ҫ 4 ��ҳ��֧��       */

/*********************************************************************************************************
  �����ڴ�ҳ���������
*********************************************************************************************************/

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PTE_SHIFT                  LW_CFG_VMM_PAGE_SHIFT
#define LW_CFG_VMM_PTE_SIZE                   LW_CFG_VMM_PAGE_SIZE
#define LW_CFG_VMM_PTE_MASK                   (0x1fful << LW_CFG_VMM_PTE_SHIFT)

#define LW_CFG_VMM_PMD_SHIFT                  21                        /*  2^21 = 2MB                  */
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (0x1fful << LW_CFG_VMM_PMD_SHIFT)

#define LW_CFG_VMM_PGD_SHIFT                  30                        /*  2^30 = 1GB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (0x3ul << LW_CFG_VMM_PGD_SHIFT)

#else
#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PMD_SHIFT                  20                        /*  NO PMD same as PGD          */
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (~(LW_CFG_VMM_PMD_SIZE - 1))

#define LW_CFG_VMM_PGD_SHIFT                  20                        /*  2^20 = 1MB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (~(LW_CFG_VMM_PGD_SIZE - 1))
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT  */

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

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
typedef UINT64  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT64  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */
typedef UINT64  LW_PTE_TRANSENTRY;                                      /*  ҳ����Ŀ����                */

#else                                                                   /*  LW_CFG_ARM_LPAE > 0         */
typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */
typedef UINT32  LW_PTE_TRANSENTRY;                                      /*  ҳ����Ŀ����                */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT  */

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARM_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
