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
** ��   ��   ��: cpu_cfg_mips.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 11 �� 20 ��
**
** ��        ��: MIPS CPU �����빦������.
*********************************************************************************************************/

#ifndef __CPU_CFG_MIPS_H
#define __CPU_CFG_MIPS_H

/*********************************************************************************************************
  CPU ��ϵ�ṹ
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_MIPS            1                               /*  CPU �ܹ�                    */

#if defined(_MIPS_ARCH_MIPS64R2) || defined(_MIPS_ARCH_HR2)
#define LW_CFG_CPU_ARCH_FAMILY          "MIPS64R2(R)"                   /*  MIPS64R2 family             */

#elif defined(_MIPS_ARCH_MIPS64)
#define LW_CFG_CPU_ARCH_FAMILY          "MIPS64(R)"                     /*  MIPS64 family               */

#elif defined(_MIPS_ARCH_MIPS32R2)
#define LW_CFG_CPU_ARCH_FAMILY          "MIPS32R2(R)"                   /*  MIPS32R2 family             */

#else
#define LW_CFG_CPU_ARCH_FAMILY          "MIPS32(R)"                     /*  MIPS32 family               */
#endif

/*********************************************************************************************************
  SMT ͬ�����̵߳����Ż�
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_SMT             0                               /*  ͬ�����߳��Ż�              */

/*********************************************************************************************************
  CACHE LINE ����
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_CACHE_LINE      64                              /*  cache ����ж�������        */

/*********************************************************************************************************
  CPU �ֳ������ʹ�С�˶���
*********************************************************************************************************/

#if defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LW_CFG_CPU_ENDIAN               0                               /*  0: С��                     */
#else
#define LW_CFG_CPU_ENDIAN               1                               /*  1: ���                     */
#endif                                                                  /*  __BYTE_ORDER__ == LITTLE    */
#else
#define LW_CFG_CPU_ENDIAN               0                               /*  0: С��  1: ���            */
#endif                                                                  /*  defined(__GNUC__)           */

#if defined(__mips64)
#define LW_CFG_CPU_WORD_LENGHT          64                              /*  CPU �ֳ�                    */
#define LW_CFG_CPU_PHYS_ADDR_64BIT      1                               /*  �����ַ 64bit ���         */
#define LW_CFG_CPU_ARCH_MIPS64          1                               /*  CPU �ܹ�                    */

#else
#define LW_CFG_CPU_WORD_LENGHT          32                              /*  CPU �ֳ�                    */
#define LW_CFG_CPU_PHYS_ADDR_64BIT      0                               /*  �����ַ 64bit ���         */
#endif

/*********************************************************************************************************
  MIPS MMU ����

  LW_CFG_MIPS_PAGE_SHIFT �����õ�ֵ������:
        12 :  4K Bytes per page.
        14 : 16K Bytes per page.
        16 : 64K Bytes per page.
        18 : 256K Bytes per page.
*********************************************************************************************************/

#define LW_CFG_MIPS_PAGE_SHIFT          14                              /*  MMU ҳ���С                */

/*********************************************************************************************************
  MIPS CACHE ����
*********************************************************************************************************/

#define LW_CFG_MIPS_CACHE_L2            1                               /*  �Ƿ�������� MIPS ���� CACHE*/
#define LW_CFG_MIPS_CACHE_L3            1                               /*  �Ƿ�������� MIPS ���� CACHE*/

/*********************************************************************************************************
  MIPS ָ������
*********************************************************************************************************/

#define LW_CFG_MIPS_HAS_CLZ_INSTR       1                               /*  �Ƿ�֧��ǰ���� CLZ ָ��     */
#define LW_CFG_MIPS_HAS_SYNC_INSTR      1                               /*  �Ƿ�֧�� SYNC ָ��          */
#define LW_CFG_MIPS_HAS_MSA_INSTR       0                               /*  �Ƿ�֧�� MSA(SIMD) ָ��     */
#if defined(_MIPS_ARCH_MIPS64R2) || defined(_MIPS_ARCH_HR2)
#define LW_CFG_MIPS_HAS_RDHWR_INSTR     1                               /*  MIPS64R2 ֧�� RDHWR ָ��    */
#else
#define LW_CFG_MIPS_HAS_RDHWR_INSTR     0                               /*  �Ƿ�֧�� RDHWR ָ��         */
#endif

/*********************************************************************************************************
  ��� MIPS CP0 Hazard ָ������

  LW_CFG_MIPS_CP0_HAZARD_INSTR �����õ�ֵ������:
        0  : EHB.
        1  : SYNC.
        2  : 4 �� SSNOP.
*********************************************************************************************************/

#if defined(_MIPS_ARCH_HR2)
#define LW_CFG_MIPS_CP0_HAZARD_INSTR    1                               /*  ���2��ʹ�� SYNC ָ��       */
#else
#define LW_CFG_MIPS_CP0_HAZARD_INSTR    0                               /*  ʹ�� EHB ָ��               */
#endif

/*********************************************************************************************************
  On the Loongson-2G/2H/3A/3B there is a bug that ll / sc and lld / scd is very weak ordering.
  NOTICE: LW_CFG_MIPS_CPU_LOONGSON1 / LW_CFG_MIPS_CPU_LOONGSON2K / LW_CFG_MIPS_CPU_LOONGSON3 Only one can
          set to 1.
*********************************************************************************************************/

#define LW_CFG_MIPS_CPU_LOONGSON1       0                               /*  Loongson-1x                 */
#define LW_CFG_MIPS_CPU_LOONGSON2K      0                               /*  Loongson-2K                 */
#define LW_CFG_MIPS_CPU_LOONGSON3       1                               /*  Loongson-2G/2H/3A/3B        */

#if (LW_CFG_MIPS_CPU_LOONGSON3 > 0) || defined(_MIPS_ARCH_HR2)
#define LW_CFG_MIPS_LOONGSON_LLSC_WAR   1                               /*  ��о3�ţ����2�� LLSC ����  */
#else
#define LW_CFG_MIPS_LOONGSON_LLSC_WAR   0
#endif

#if (LW_CFG_MIPS_CPU_LOONGSON3 > 0) || (LW_CFG_MIPS_CPU_LOONGSON2K > 0) || defined(_MIPS_ARCH_HR2)
#define LW_CFG_MIPS_WEAK_REORDERING             1
#define LW_CFG_MIPS_WEAK_REORDERING_BEYOND_LLSC 1
#else
#define LW_CFG_MIPS_WEAK_REORDERING             0
#define LW_CFG_MIPS_WEAK_REORDERING_BEYOND_LLSC 0
#endif

/*********************************************************************************************************
  LL/SC nesting detect fail bug
*********************************************************************************************************/

#if (LW_CFG_MIPS_CPU_LOONGSON3 > 0) || (LW_CFG_MIPS_CPU_LOONGSON2K > 0)
#define LW_CFG_MIPS_NEST_LLSC_BUG       1
#else
#define LW_CFG_MIPS_NEST_LLSC_BUG       0
#endif

/*********************************************************************************************************
  �������㵥Ԫ
*********************************************************************************************************/

#define LW_CFG_CPU_FPU_EN               1                               /*  CPU �Ƿ�ӵ�� FPU            */

/*********************************************************************************************************
  DSP �����źŴ�����
*********************************************************************************************************/

#define LW_CFG_CPU_DSP_EN               1                               /*  CPU �Ƿ�ӵ�� DSP            */

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

#define LW_CFG_CPU_ATOMIC_EN            1

#if LW_CFG_CPU_WORD_LENGHT == 64
#define LW_CFG_CPU_ATOMIC64_EN          1
#else
#define LW_CFG_CPU_ATOMIC64_EN          0
#endif

#endif                                                                  /*  __CPU_CFG_MIPS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
