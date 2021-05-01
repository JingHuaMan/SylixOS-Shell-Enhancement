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
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V64 �ڴ�������.
*********************************************************************************************************/

#ifndef __RISCV_ARCH_MMU64_H
#define __RISCV_ARCH_MMU64_H

/*********************************************************************************************************
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#if LW_CFG_RISCV_MMU_SV39 > 0
#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  sv39 ��Ҫ 3 ��ҳ��֧��      */
#else
#define LW_CFG_VMM_PAGE_4L_EN                 1                         /*  sv48 ��Ҫ 4 ��ҳ��֧��      */
#endif

/*********************************************************************************************************
  �����ڴ�ҳ���������
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#if LW_CFG_RISCV_MMU_SV39 > 0
#define LW_CFG_VMM_PMD_SHIFT                  21
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (0x1fful << LW_CFG_VMM_PMD_SHIFT)

#define LW_CFG_VMM_PGD_SHIFT                  30
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (0x1fful << LW_CFG_VMM_PGD_SHIFT)
#else
#define LW_CFG_VMM_PTS_SHIFT                  21
#define LW_CFG_VMM_PTS_SIZE                   (1ul << LW_CFG_VMM_PTS_SHIFT)
#define LW_CFG_VMM_PTS_MASK                   (0x1fful << LW_CFG_VMM_PTS_SHIFT)

#define LW_CFG_VMM_PMD_SHIFT                  30
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (0x1fful << LW_CFG_VMM_PMD_SHIFT)

#define LW_CFG_VMM_PGD_SHIFT                  39
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (0x1fful << LW_CFG_VMM_PGD_SHIFT)
#endif

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

typedef UINT64  LW_PGD_TRANSENTRY;                                      /*  PGD ҳĿ¼����              */
typedef UINT64  LW_PMD_TRANSENTRY;                                      /*  PMD ҳĿ¼����              */
#if LW_CFG_RISCV_MMU_SV39 == 0
typedef UINT64  LW_PTS_TRANSENTRY;                                      /*  PTS ҳĿ¼����              */
#endif                                                                  /*  LW_CFG_RISCV_MMU_SV39 == 0  */
typedef UINT64  LW_PTE_TRANSENTRY;                                      /*  ҳ����Ŀ����                */

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __RISCV_ARCH_MMU64_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
