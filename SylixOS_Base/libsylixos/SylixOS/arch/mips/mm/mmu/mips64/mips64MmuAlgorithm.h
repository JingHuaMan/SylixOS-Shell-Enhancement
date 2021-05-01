/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mips64MmuAlgorithm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 12 �� 15 ��
**
** ��        ��: MIPS64 ��ϵ���� MMU TLB �����㷨.
*********************************************************************************************************/

#ifndef __ARCH_MIPS64MMUALGORITHM_H
#define __ARCH_MIPS64MMUALGORITHM_H

/*********************************************************************************************************
  PTE BASE ��ض���
*********************************************************************************************************/

#define MIPS64_PTE_BASE_OFFSET          33
#define MIPS64_PTE_BASE_SIZE            31

/*********************************************************************************************************
  BADVPN2 ��ض���
*********************************************************************************************************/

#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define MIPS64_BADVPN2_SHIFT            0
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MIPS64_BADVPN2_SHIFT            2
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MIPS64_BADVPN2_SHIFT            4
#elif LW_CFG_VMM_PAGE_SIZE == (256 * LW_CFG_KB_SIZE)
#define MIPS64_BADVPN2_SHIFT            6
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K, 256K)!
#endif

/*********************************************************************************************************
  BADVPN2 ��ض���
*********************************************************************************************************/

#define MIPS64_TLB_CTX_SIZE      64
#define MIPS64_TLB_CTX_LOCK      (0 * ARCH_REG_SIZE)
#define MIPS64_TLB_CTX_T0        (1 * ARCH_REG_SIZE)
#define MIPS64_TLB_CTX_T1        (2 * ARCH_REG_SIZE)
#define MIPS64_TLB_CTX_T2        (3 * ARCH_REG_SIZE)
#define MIPS64_TLB_CTX_PGD       (4 * ARCH_REG_SIZE)

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef struct {
    ULONG       CTX_ulSpinLock;
    ULONG       CTX_ulT0;
    ULONG       CTX_ulT1;
    ULONG       CTX_ulT2;
    ULONG       CTX_ulPGD;
} MIPS64_TLB_REFILL_CTX;                                                /*  !defined(__ASSEMBLY__)      */
#endif

#endif                                                                  /*  __ARCH_MIPS64MMUALGORITHM_H */
/*********************************************************************************************************
  END
*********************************************************************************************************/
