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
** ��   ��   ��: x86Segment.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 05 ��
**
** ��        ��: x86 ��ϵ���� Segment.
*********************************************************************************************************/

#ifndef __ARCH_X86SEGMENT_H
#define __ARCH_X86SEGMENT_H

/*********************************************************************************************************
  Global and local (GDT/LDT) segment descriptor definition and
  structure. These segments map virtual addresses (ie
  data/instruction addresses, relative to these segment descriptors)
  to linear addresses (ie addresses in the paged-memory space).

  @see Intel x86 doc, vol 3 chapter 3.
*********************************************************************************************************/
/*********************************************************************************************************
  Global segment selectors (GDT) for x86.

  @see X86Gdt.h
*********************************************************************************************************/

#define X86_SEG_NULL            0                       /*  NULL segment, unused by the procesor        */
#define X86_SEG_KCODE           1                       /*  Kernel code segment                         */
#define X86_SEG_KDATA           2                       /*  Kernel data segment                         */
#define X86_SEG_UCODE           3                       /*  User code segment                           */
#define X86_SEG_UDATA           4                       /*  User data segment                           */
#define X86_SEG_KERNEL_TSS      5                       /*  Kernel TSS for CPL3 -> CPL0 privilege change*/
#define X86_SEG_MAX             6

/*********************************************************************************************************
  Helper macro that builds a segment register's value
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

#define X86_BUILD_SEGMENT_REG_VALUE(desc_privilege, in_ldt, seg_index) \
    ((((desc_privilege) & 0x3)  << 0) \
   | (((in_ldt) ? 1 : 0)        << 2) \
   | ((seg_index)               << 3))

#else

/*********************************************************************************************************
  Assembler-compliant version.

  Caution: In assembler code, "in_ldt" MUST be either 1 or 0, nothing else.
*********************************************************************************************************/

#define X86_BUILD_SEGMENT_REG_VALUE(desc_privilege, in_ldt, seg_index) \
    ((((desc_privilege) & 0x3) << 0) \
   | ((in_ldt & 1)             << 2) \
   | ((seg_index)              << 3))

#endif                                                                  /*  ASSEMBLY                    */

/*********************************************************************************************************
  �μĴ���ֵ
*********************************************************************************************************/

#define X86_CS_KERNEL       X86_BUILD_SEGMENT_REG_VALUE(0, 0, X86_SEG_KCODE)
#define X86_DS_KERNEL       X86_BUILD_SEGMENT_REG_VALUE(0, 0, X86_SEG_KDATA)
#define X86_CS_USER         X86_BUILD_SEGMENT_REG_VALUE(3, 0, X86_SEG_UCODE)
#define X86_DS_USER         X86_BUILD_SEGMENT_REG_VALUE(3, 0, X86_SEG_UDATA)
#define X86_KERNEL_TSS      X86_BUILD_SEGMENT_REG_VALUE(0, 0, X86_SEG_KERNEL_TSS)

#endif                                                                  /*  __ARCH_X86SEGMENT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
