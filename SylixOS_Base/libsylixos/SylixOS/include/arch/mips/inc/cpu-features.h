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
** ��   ��   ��: cpu-features.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 28 ��
**
** ��        ��: MIPS CPU ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCPUFEATURES_H
#define __ARCH_MIPSCPUFEATURES_H

/*********************************************************************************************************
  MicroMIPS
*********************************************************************************************************/

#define cpu_has_mmips       LW_FALSE

/*********************************************************************************************************
  DSP
*********************************************************************************************************/

#if LW_CFG_CPU_DSP_EN > 0
#define cpu_has_dsp         LW_TRUE
#else
#define cpu_has_dsp         LW_FALSE
#endif

/*********************************************************************************************************
  MAS
*********************************************************************************************************/

#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
#define cpu_has_msa         LW_TRUE
#else
#define cpu_has_msa         LW_FALSE
#endif

/*********************************************************************************************************
  ISA
*********************************************************************************************************/

#if defined(_MIPS_ARCH_MIPS2)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_FALSE
# define cpu_has_mips_4     LW_FALSE
# define cpu_has_mips_3     LW_FALSE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS3)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_FALSE
# define cpu_has_mips_4     LW_FALSE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS4)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_FALSE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS5)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_TRUE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS64)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_TRUE
# define cpu_has_mips32r1   LW_TRUE
# define cpu_has_mips_5     LW_TRUE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS64R2) || defined(_MIPS_ARCH_HR2)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_TRUE
# define cpu_has_mips32r2   LW_TRUE
# define cpu_has_mips64r1   LW_TRUE
# define cpu_has_mips32r1   LW_TRUE
# define cpu_has_mips_5     LW_TRUE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS32R2)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_TRUE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_TRUE
# define cpu_has_mips_5     LW_TRUE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS32)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_FALSE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_TRUE
# define cpu_has_mips_5     LW_TRUE
# define cpu_has_mips_4     LW_TRUE
# define cpu_has_mips_3     LW_TRUE
# define cpu_has_mips_2     LW_TRUE
#endif

#if defined(_MIPS_ARCH_MIPS64R6)
# define cpu_has_mips64r6   LW_TRUE
# define cpu_has_mips32r6   LW_TRUE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_FALSE
# define cpu_has_mips_4     LW_FALSE
# define cpu_has_mips_3     LW_FALSE
# define cpu_has_mips_2     LW_FALSE
#endif

#if defined(_MIPS_ARCH_MIPS32R6)
# define cpu_has_mips64r6   LW_FALSE
# define cpu_has_mips32r6   LW_TRUE
# define cpu_has_mips64r2   LW_FALSE
# define cpu_has_mips32r2   LW_FALSE
# define cpu_has_mips64r1   LW_FALSE
# define cpu_has_mips32r1   LW_FALSE
# define cpu_has_mips_5     LW_FALSE
# define cpu_has_mips_4     LW_FALSE
# define cpu_has_mips_3     LW_FALSE
# define cpu_has_mips_2     LW_FALSE
#endif

/*********************************************************************************************************
  Shortcuts
*********************************************************************************************************/

#define cpu_has_mips_2_3_4_5    (cpu_has_mips_2 | cpu_has_mips_3_4_5)
#define cpu_has_mips_3_4_5      (cpu_has_mips_3 | cpu_has_mips_4_5)
#define cpu_has_mips_4_5        (cpu_has_mips_4 | cpu_has_mips_5)

#define cpu_has_mips_2_3_4_5_r  (cpu_has_mips_2 | cpu_has_mips_3_4_5_r)
#define cpu_has_mips_3_4_5_r    (cpu_has_mips_3 | cpu_has_mips_4_5_r)
#define cpu_has_mips_4_5_r      (cpu_has_mips_4 | cpu_has_mips_5_r)
#define cpu_has_mips_5_r        (cpu_has_mips_5 | cpu_has_mips_r)

#define cpu_has_mips_3_4_5_64_r2_r6                 \
                                (cpu_has_mips_3 | cpu_has_mips_4_5_64_r2_r6)
#define cpu_has_mips_4_5_64_r2_r6                   \
                                (cpu_has_mips_4_5 | cpu_has_mips64r1 |  \
                                 cpu_has_mips_r2  | cpu_has_mips_r6)

#define cpu_has_mips32          (cpu_has_mips32r1 | cpu_has_mips32r2 | cpu_has_mips32r6)
#define cpu_has_mips64          (cpu_has_mips64r1 | cpu_has_mips64r2 | cpu_has_mips64r6)
#define cpu_has_mips_r1         (cpu_has_mips32r1 | cpu_has_mips64r1)
#define cpu_has_mips_r2         (cpu_has_mips32r2 | cpu_has_mips64r2)
#define cpu_has_mips_r6         (cpu_has_mips32r6 | cpu_has_mips64r6)
#define cpu_has_mips_r          (cpu_has_mips32r1 | cpu_has_mips32r2 | \
                                 cpu_has_mips32r6 | cpu_has_mips64r1 | \
                                 cpu_has_mips64r2 | cpu_has_mips64r6)

/*********************************************************************************************************
  MIPSR2 and MIPSR6 have a lot of similarities
*********************************************************************************************************/

#define cpu_has_mips_r2_r6      (cpu_has_mips_r2 | cpu_has_mips_r6)

#endif                                                                  /*  __ARCH_MIPSCPUFEATURES_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
