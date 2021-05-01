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
** ��   ��   ��: config.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 28 ��
**
** ��        ��: MIPS ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCONFIG_H
#define __ARCH_MIPSCONFIG_H

#if LW_CFG_CPU_WORD_LENGHT == 32
#define CONFIG_32BIT                    32                              /*  ʹ�� 32 λ                  */
#undef  CONFIG_64BIT
#else
#define CONFIG_64BIT                    64                              /*  ʹ�� 64 λ                  */
#undef  CONFIG_32BIT
#endif

#define CONFIG_DEBUG_FPU_EMU                                            /*  FPU ģ��ʹ�ܵ���            */

#define MIPS_CONFIG_EVA                 0                               /*  �ݲ�֧����ǿ�����ַ(EVA)   */
#undef  CONFIG_EVA                                                      /*  �ݲ�֧����ǿ�����ַ(EVA)   */

#if LW_CFG_CPU_WORD_LENGHT == 32
#define CONFIG_MIPS32_O32               1                               /*  O32 ABI                     */
#define CONFIG_MIPS_O32_FP64_SUPPORT    0
#else
#define CONFIG_MIPS32_O32               0                               /*  N64 ABI                     */
#define CONFIG_MIPS_O32_FP64_SUPPORT    0
#endif

#undef  CONFIG_SYS_SUPPORTS_MIPS16                                      /*  �ݲ�֧�� MIPS16             */
#undef  CONFIG_SYS_SUPPORTS_MICROMIPS                                   /*  �ݲ�֧�� MICROMIPS          */
#undef  CONFIG_CPU_MICROMIPS                                            /*  �ݲ�֧�� MICROMIPS          */
#undef  CONFIG_CPU_CAVIUM_OCTEON                                        /*  �ݲ�֧�� CAVIUM OCTEON      */

#if defined(_MIPS_ARCH_MIPS64R2) || \
    defined(_MIPS_ARCH_MIPS64R3) || \
    defined(_MIPS_ARCH_MIPS64R5) || \
    defined(_MIPS_ARCH_MIPS64R6) || \
    defined(_MIPS_ARCH_MIPS32R2) || \
    defined(_MIPS_ARCH_MIPS32R3) || \
    defined(_MIPS_ARCH_MIPS32R5) || \
    defined(_MIPS_ARCH_MIPS32R6) || \
    defined(_MIPS_ARCH_HR2)
#define NO_R6EMU                        0                               /*  ֧�� MIPSR2-R6 ģ��         */
#else
#define NO_R6EMU                        1                               /*  ��֧�� MIPSR2-R6 ģ��       */
#endif

#if defined(_MIPS_ARCH_MIPS64R6) || \
    defined(_MIPS_ARCH_MIPS32R6)
#define CONFIG_CPU_MIPSR6               1                               /*  ֧�� MIPSR6                 */
#else
#undef  CONFIG_CPU_MIPSR6                                               /*  ��֧�� MIPSR6               */
#endif

#endif                                                                  /*  __ARCH_MIPSCONFIG_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
