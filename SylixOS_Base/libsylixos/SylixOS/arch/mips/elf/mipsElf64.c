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
** ��   ��   ��: mipsElf64.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: ʵ�� MIPS64 ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if defined(LW_CFG_CPU_ARCH_MIPS)                                       /*  MIPS ��ϵ�ṹ               */
#if LW_CFG_CPU_WORD_LENGHT == 64
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
#include "../SylixOS/loader/include/loader_symbol.h"
#include "./mipsElfCommon.h"
/*********************************************************************************************************
** ��������: archElfRelocateRel
** ��������: �ض�λ REL ���͵��ض�λ��
** ��  ��  : module       ģ�� 
**           prel         REL ����
**           psym         ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRelocateRel (PVOID        pmodule,
                         Elf_Rel     *prel,
                         Elf_Sym     *psym,
                         Elf_Addr     addrSymVal,
                         PCHAR        pcTargetSec,
                         PCHAR        pcBuffer,
                         size_t       stBuffLen)
{
    Elf_Addr  *paddrWhere;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_MIPS_R_TYPE(prel)) {

    case R_MIPS_REL32:
        mipsElfREL32RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                                paddrWhere,
                                ELF_MIPS_R_SYM(prel));
        break;

    case R_MIPS_NONE:
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_MIPS_R_TYPE(prel));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsElfRelocateRela
** ��������: �ض�λ RELA ���͵��ض�λ��
** ��  ��  : module       ģ�� 
**           prela        RELA ����
**           psym         ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRelocateRela (PVOID       pmodule,
                          Elf_Rela   *prela,
                          Elf_Sym    *psym,
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    UINT32  *paddrWhere;

    paddrWhere = (UINT32 *)((size_t)pcTargetSec + prela->r_offset);     /*  �����ض�λĿ���ַ          */

    addrSymVal += prela->r_addend;

    switch (ELF_MIPS_R_TYPE(prela)) {

    case R_MIPS_NONE:
        break;

    case R_MIPS_32:
        (*(UINT32 *)paddrWhere) += (UINT32)addrSymVal;
        break;

    case R_MIPS_64:
        (*(UINT64 *)paddrWhere) += (UINT64)addrSymVal;
        break;

    case R_MIPS_HIGHER:
        *paddrWhere = (*paddrWhere & 0xffff0000) |
                      ((((INT64)addrSymVal + 0x80008000LL) >> 32) & 0xffff);
        break;

    case R_MIPS_HIGHEST:
        *paddrWhere = (*paddrWhere & 0xffff0000) |
                      ((((INT64)addrSymVal + 0x800080008000LL) >> 48) & 0xffff);
        break;

    case R_MIPS_REL32:
        mipsElfREL32RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                                (Elf_Addr *)paddrWhere,
                                ELF_MIPS_R_SYM(prela));
        break;

    case R_MIPS_26:
        if (addrSymVal & 0x03) {
            return  (PX_ERROR);
        }
        if ((addrSymVal & 0xf0000000) != (((Elf_Addr)paddrWhere + 4) & 0xf0000000)) {
            return  (PX_ERROR);
        }
        *paddrWhere = (*paddrWhere & ~0x03ffffff) |
                      ((*paddrWhere + (addrSymVal >> 2)) & 0x03ffffff);
        break;

    case R_MIPS_HI16:
        *paddrWhere = (*paddrWhere & 0xffff0000) |
                      ((((INT64)addrSymVal + 0x8000LL) >> 16) & 0xffff);
        break;

    case R_MIPS_LO16:
        *paddrWhere = (*paddrWhere & 0xffff0000) | (addrSymVal & 0xffff);
        break;

    case R_MIPS_JUMP_SLOT:
        *paddrWhere = (Elf_Addr)addrSymVal;
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_MIPS_R_TYPE(prela));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS64      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
