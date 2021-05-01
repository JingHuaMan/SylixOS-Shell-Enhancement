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
** ��   ��   ��: x86Elf.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: ʵ�� x86 ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_X86                                            /*  x86 ��ϵ�ṹ                */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  �ض�λ���Ͷ���
*********************************************************************************************************/
#define R_386_NONE          0                               /*  No reloc                                */
#define R_386_32            1                               /*  Direct 32 bit                           */
#define R_386_PC32          2                               /*  PC relative 32 bit                      */
#define R_386_GOT32         3                               /*  32 bit GOT entry                        */
#define R_386_PLT32         4                               /*  32 bit PLT address                      */
#define R_386_COPY          5                               /*  Copy symbol at runtime                  */
#define R_386_GLOB_DAT      6                               /*  Create GOT entry                        */
#define R_386_JMP_SLOT      7                               /*  Create PLT entry                        */
#define R_386_RELATIVE      8                               /*  Adjust by program base                  */
#define R_386_GOTOFF        9                               /*  32 bit offset to GOT                    */
#define R_386_GOTPC         10                              /*  32 bit signed pc relative offset to GOT */
/*********************************************************************************************************
** ��������: archElfGotInit
** ��������: GOT �ض�λ
** �䡡��  : pmodule       ģ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfGotInit (PVOID  pmodule)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfRelocateRela
** ��������: �ض�λ RELA ���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           prela        RELA ����
**           psym         ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR CODE
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
    _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: archElfRelocateRel
** ��������: �ض�λ REL ���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           prel         REL ����
**           psym         ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR CODE
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
    Elf_Addr   valueRaw;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_386_JMP_SLOT:
        valueRaw    = *paddrWhere;
        *paddrWhere = addrSymVal;
        LD_DEBUG_MSG(("R_386_JMP_SLOT: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_RELATIVE:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal + (Elf_Addr)((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvBaseAddr;
        LD_DEBUG_MSG(("R_386_RELATIVE: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_GLOB_DAT:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_386_GLOB_DAT: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_32:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_386_32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_PC32:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal - (Elf_Addr)paddrWhere;
        LD_DEBUG_MSG(("R_386_PC32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_NONE:
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfRGetJmpBuffItemLen
** ��������: ������ת�����
** ��  ��  : pmodule       ģ��
** ��  ��  : ��ת�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRGetJmpBuffItemLen (PVOID  pmodule)
{
    return  (0);                                                        /*  ����Ҫ��ת����              */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
