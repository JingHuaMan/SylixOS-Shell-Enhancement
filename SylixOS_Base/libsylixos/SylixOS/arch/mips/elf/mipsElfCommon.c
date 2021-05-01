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
** ��   ��   ��: mipsElfCommon.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: ʵ�� MIPS ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if defined(LW_CFG_CPU_ARCH_MIPS)                                       /*  MIPS ��ϵ�ṹ               */
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
** ��������: mipsElfREL32RelocateRel
** ��������: �ض�λ R_MIPS_REL32 ���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           pRelocAdrs   �ض�λ��ַ
**           SymIndex     �ض�λ��������ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsElfREL32RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                              Elf_Addr           *pRelocAdrs,
                              Elf_Addr            symIndex)
{
    ELF_DYN_DIR  *pdyndir = (ELF_DYN_DIR *)(pmodule->EMOD_pvFormatInfo);

    if (symIndex) {
        if (symIndex < pdyndir->ulMIPSGotSymIdx) {
            *pRelocAdrs += (Elf_Addr)pmodule->EMOD_pvBaseAddr +
                            pdyndir->psymTable[symIndex].st_value;
        } else {
            *pRelocAdrs += pdyndir->ulPltGotAddr[symIndex + pdyndir->ulMIPSLocalGotNumIdx -
                                                 pdyndir->ulMIPSGotSymIdx];
        }

    } else {
        *pRelocAdrs += (Elf_Addr)pmodule->EMOD_pvBaseAddr;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfGotInit
** ��������: MIPS GOT �ض�λ
** �䡡��  : pmodule       ģ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfGotInit (PVOID  pmodule)
{
    Elf_Addr           *pMipsGotEntry;
    Elf_Sym            *pMipsSym;
    CHAR               *pchStrTab;
    Elf_Addr            addrSymVal;
    ULONG               ulTemp = 0;
    INT                 iIndex = 2;
    LW_LD_EXEC_MODULE  *pprivmodule = (LW_LD_EXEC_MODULE *)pmodule;
    ELF_DYN_DIR        *pprivdyndir = (ELF_DYN_DIR *)pprivmodule->EMOD_pvFormatInfo;

    pMipsGotEntry = pprivdyndir->ulPltGotAddr + iIndex;
    for (; iIndex < pprivdyndir->ulMIPSLocalGotNumIdx; iIndex++, pMipsGotEntry++) {
        *pMipsGotEntry += (Elf_Addr)pprivmodule->EMOD_pvBaseAddr;
    }

    pMipsGotEntry = pprivdyndir->ulPltGotAddr + pprivdyndir->ulMIPSLocalGotNumIdx;
    pMipsSym      = (Elf_Sym *)pprivdyndir->psymTable + pprivdyndir->ulMIPSGotSymIdx;
    pchStrTab     = (CHAR *)pprivdyndir->pcStrTable;
    ulTemp        = pprivdyndir->ulMIPSSymNumIdx - pprivdyndir->ulMIPSGotSymIdx;

    while (ulTemp--) {
        if ((pMipsSym->st_shndx == SHN_UNDEF) || (pMipsSym->st_shndx == SHN_COMMON)) {
            BOOL  bWeak = (STB_WEAK == ELF_ST_BIND(pMipsSym->st_info));

            if (__moduleSymGetValue(pprivmodule,
                                    bWeak,
                                    pchStrTab + pMipsSym->st_name,
                                    &addrSymVal,
                                    LW_LD_SYM_ANY) < 0) {
                return  (PX_ERROR);
            }
            *pMipsGotEntry = addrSymVal;

        } else if (ELF_ST_TYPE(pMipsSym->st_info) == STT_SECTION) {
            if (pMipsSym->st_other == 0) {
                *pMipsGotEntry += (Elf_Addr)pprivmodule->EMOD_pvBaseAddr;
            }

        } else {
            addrSymVal     = LW_LD_V2PADDR(pprivdyndir->addrMin,
                                           pprivmodule->EMOD_pvBaseAddr,
                                           pMipsSym->st_value);
            *pMipsGotEntry = addrSymVal;
        }

        pMipsGotEntry++;
        pMipsSym++;
    }

    return (ERROR_NONE);
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
    return  (JMP_TABLE_ITEMLEN);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
