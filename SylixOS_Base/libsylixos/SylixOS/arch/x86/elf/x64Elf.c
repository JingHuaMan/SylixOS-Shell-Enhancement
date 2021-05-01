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
** ��   ��   ��: x64Elf.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 05 ��
**
** ��        ��: ʵ�� x86-64 ��ϵ�ṹ�� ELF �ļ��ض�λ.
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
#define R_X86_64_NONE       0                           /*  No reloc                                    */
#define R_X86_64_64         1                           /*  Direct 64 bit                               */
#define R_X86_64_PC32       2                           /*  PC relative 32 bit signed                   */
#define R_X86_64_GOT32      3                           /*  32 bit GOT entry                            */
#define R_X86_64_PLT32      4                           /*  32 bit PLT address                          */
#define R_X86_64_COPY       5                           /*  Copy symbol at runtime                      */
#define R_X86_64_GLOB_DAT   6                           /*  Create GOT entry                            */
#define R_X86_64_JUMP_SLOT  7                           /*  Create PLT entry                            */
#define R_X86_64_RELATIVE   8                           /*  Adjust by program base                      */
#define R_X86_64_GOTPCREL   9                           /*  32 bit signed pc relative offset to GOT     */
#define R_X86_64_32         10                          /*  Direct 32 bit zero extended                 */
#define R_X86_64_32S        11                          /*  Direct 32 bit sign extended                 */
#define R_X86_64_16         12                          /*  Direct 16 bit zero extended                 */
#define R_X86_64_PC16       13                          /*  16 bit sign extended pc relative            */
#define R_X86_64_8          14                          /*  Direct 8 bit sign extended                  */
#define R_X86_64_PC8        15                          /*  8 bit sign extended pc relative             */
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
    Elf_Addr  *paddrWhere;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  �����ض�λĿ���ַ          */

    addrSymVal = addrSymVal + prela->r_addend;                          /*  ���ж��ü��� ADD            */

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_X86_64_NONE:
        break;

    case R_X86_64_JUMP_SLOT:
    {
#ifdef  __SYLIXOS_DEBUG
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
#endif
        *paddrWhere = addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_JUMP_SLOT: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_GLOB_DAT:
    {
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_GLOB_DAT: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_RELATIVE:
    {
#ifdef  __SYLIXOS_DEBUG
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
#endif
        *paddrWhere = addrSymVal + (Elf_Addr)((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvBaseAddr;
        LD_DEBUG_MSG(("R_X86_64_RELATIVE: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_64:
    {
        UINT64  valueRaw;
        valueRaw = *(UINT64 *)paddrWhere;
        *(UINT64 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_64: where %lx raw %llx val %lx -> %llx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT64 *)paddrWhere));
    }
    break;

    case R_X86_64_32:
    {
        UINT32  valueRaw;
        valueRaw = *(UINT32 *)paddrWhere;
        *(UINT32 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_32: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT32 *)paddrWhere));
    }
    break;

    case R_X86_64_32S:
    {
        INT32  valueRaw;
        valueRaw  = *(INT32 *)paddrWhere;
        *(INT32 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_32S: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(INT32 *)paddrWhere));
    }
    break;

    case R_X86_64_PC32:
    {
        UINT32  valueRaw;
        valueRaw = *(UINT32 *)paddrWhere;
        *(UINT32 *)paddrWhere = valueRaw + addrSymVal - (Elf_Addr)paddrWhere;
        LD_DEBUG_MSG(("R_X86_64_PC32: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT32 *)paddrWhere));
    }
    break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
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
    _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
    return  (PX_ERROR);
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
