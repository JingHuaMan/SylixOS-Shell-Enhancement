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
** ��   ��   ��: mipsElfCommon.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: ʵ�� MIPS ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/

#ifndef __ARCH_MIPSELFCOMMON_H
#define __ARCH_MIPSELFCOMMON_H

/*********************************************************************************************************
  MIPS relocs
*********************************************************************************************************/
#define R_MIPS_NONE             0                                       /*  No reloc                    */
#define R_MIPS_16               1                                       /*  Direct 16 bit               */
#define R_MIPS_32               2                                       /*  Direct 32 bit               */
#define R_MIPS_REL32            3                                       /*  PC relative 32 bit          */
#define R_MIPS_26               4                                       /*  Direct 26 bit shifted       */
#define R_MIPS_HI16             5                                       /*  High 16 bit                 */
#define R_MIPS_LO16             6                                       /*  Low 16 bit                  */
#define R_MIPS_GPREL16          7                                       /*  GP relative 16 bit          */
#define R_MIPS_LITERAL          8                                       /*  16 bit literal entry        */
#define R_MIPS_GOT16            9                                       /*  16 bit GOT entry            */
#define R_MIPS_PC16             10                                      /*  PC relative 16 bit          */
#define R_MIPS_CALL16           11                                      /*  16 bit GOT entry for fun    */
#define R_MIPS_GPREL32          12                                      /*  GP relative 32 bit          */
#define R_MIPS_UNUSED1          13
#define R_MIPS_UNUSED2          14
#define R_MIPS_UNUSED3          15
#define R_MIPS_SHIFT5           16
#define R_MIPS_SHIFT6           17
#define R_MIPS_64               18
#define R_MIPS_GOT_DISP         19
#define R_MIPS_GOT_PAGE         20
#define R_MIPS_GOT_OFST         21
#define R_MIPS_GOTHI16          22
#define R_MIPS_GOTLO16          23
#define R_MIPS_SUB              24
#define R_MIPS_INSERT_A         25
#define R_MIPS_INSERT_B         26
#define R_MIPS_DELETE           27
#define R_MIPS_HIGHER           28
#define R_MIPS_HIGHEST          29
#define R_MIPS_CALLHI16         30
#define R_MIPS_CALLLO16         31
#define R_MIPS_LOVENDOR         100
#define R_MIPS_JUMP_SLOT        127
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define JMP_TABLE_ITEMLEN       8                                       /*  ��ת����Ŀ����              */
#define LOW16_VALUE(val)        ((val) & 0x0000ffff)                    /*  Get LOW 16 Bit              */
#define SIGN_LOW16_VALUE(val)   (((LOW16_VALUE(val)^0x8000) - 0x8000))  /*  Get Sign LOW 16 Bit         */

INT  mipsElfREL32RelocateRel(LW_LD_EXEC_MODULE  *pmodule,
                             Elf_Addr           *pRelocAdrs,
                             Elf_Addr            symIndex);

#endif                                                                  /*  __ARCH_MIPSELFCOMMON_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
