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
** ��   ��   ��: endian_cfg.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 13 ��
**
** ��        ��: ������ (�������) ��С�˶���.
*********************************************************************************************************/

#ifndef __ENDIAN_CFG_H
#define __ENDIAN_CFG_H

/*********************************************************************************************************
  �����С�˶��� 
*********************************************************************************************************/

/*********************************************************************************************************
  GCC EABI ���� ARM VFP ���˵��
  
  GCC preprocessor macros for floating point
  
  When porting code to "armel", the following preprocessor macros are interesting:

  __VFP_FP__   means that the floating point format in use is that of the ARM VFP unit, which is 
               native-endian IEEE-754.
               
  __MAVERICK__ means that the floating point format is that of the Cirrus Logic MaverickCrunch, 
               which is also IEEE-754 and is always little-endian.
               
  __SOFTFP__   means that instead of floating point instructions, library calls are being generated for 
               floating point math operations so that the code will run on a processor without an FPU.
               
  __VFP_FP__ and __MAVERICK__ are mutually exclusive. If neither is set, that means the floating point 
                              format in use is the old mixed-endian 45670123 format of the FPA unit.

  Note that __VFP_FP__ does not mean that VFP code generation has been selected. It only speaks of the 
  
  floating point data format in use and is normally set when soft-float has been selected. The correct test 
  
  for VFP code generation, for example around asm fragments containing VFP instructions, is

  #if (defined(__VFP_FP__) && !defined(__SOFTFP__))
  
  Paradoxically, the -mfloat-abi=softfp does not set the __SOFTFP___ macro, since it selects real 
  
  floating point instructions using the soft-float ABI at function-call interfaces.

  __ARM_PCS_VFP is set instead (or as well?) when compiling for the armhf VFP hardfloat port.

  By default in Debian armel, __VFP_FP__ && __SOFTFP__ are selected.

  Struct packing and alignment

  With the new ABI, default structure packing changes, as do some default data sizes and alignment 
  
  (which also have a knock-on effect on structure packing). In particular the minimum size and alignment 
  
  of a structure was 4 bytes. Under the EABI there is no minimum and the alignment is determined by the 
  
  types of the components it contains. This will break programs that know too much about the way 
  
  structures are packed and can break code that writes binary files by dumping and reading structures.
  
  
  -mfloat-abi=soft    ʹ���������ʱ���佫���������(softfloat lib)��֧�ֶԸ��������.
  
  -mfloat-abi=softfp and  -mfloat-abi=hard ��������������������Ӳ����ָ����ڲ����������͵�Ӳ����ָ�
  
  ��Ҫ��-mfpu=xxx������ָ�������������ͬ�ĵط��ǣ�-mfloat-abi=softfp���ɵĴ�����ü���������ýӿ�
  
  (��ʹ�� -mfloat-abi=soft ʱ�ĵ��ýӿ�)
  
  ����ϵͳ�ں�, ��������, BSP, �ں�ģ����Բ��� -mfloat-abi=soft ����.
  
  ������� VFP Ӧ�ó����ʹ�� -mfloat-abi=softfp ������, ��Ȼ, ����ָ���ͨ�� -mfpu=? ��ѡ���, ����
  
  -mfpu=vfp

*********************************************************************************************************/

#if defined(__CC_ARM) || defined(LW_CFG_CPU_ARCH_ARM)                   /*  armcc & arm-sylixos-eabi-gcc*/
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  armcc default native-endian */

#elif defined(__GNUC__) && defined(__arm__)

#if !defined(__VFP_FP__) || \
    (defined(__VFP_FP__) && defined(__MAVERICK__))
#define LW_CFG_DOUBLE_MIX_ENDIAN            1                           /*  arm-sylixos-eabi-gcc        */
                                                                        /*  old mixed-endian            */
#else
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  native-endian               */
#endif                                                                  /*  __VFP_FP__ & __MAVERICK__   */

#else
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  default native-endian       */
#endif                                                                  /*  __CC_ARM                    */
                                                                        
#endif                                                                  /*  __ENDIAN_CFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
