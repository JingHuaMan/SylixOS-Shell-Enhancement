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
** ��   ��   ��: elf_loader.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ʵ�� elf ����

** BUG:
2010.08.05  ���� status ��ϸ��Ϣ.
2011.02.20  �Ľ����ű����, ����ֲ���ȫ�ֵ�����. (����)
2011.03.01  �� st_other != STV_DEFAULT ʱ, �����˷��ŵ�����ű�.
2011.03.23  ֧�� .so �� C++ ģ��.
2011.05.20  loader ������ API_CacheTextUpdate() ����. (����)
2011.12.09  �� API_CacheTextUpdate() ���������. (����)
2012.03.23  ����ģ�������ϵͳ�����Լ��. (����)
2012.04.21  elfSymbolExport() ����� __sylixos_version ����, ��ǿ�Ƶ���. (����)
2012.09.29  ʹ elf stat ������ȡ����Ϣ�����꾡���׶�. (����)
2012.10.16  __moduleSymGetValue() �����������ŵĶ�λ, �����Ƚ�ͳһ. (����)
2012.12.10  ����Ƿ���ƥ���ҵ�����ڵ�ַ, �����¼����. (����)
2013.03.27  װ���������ֵܿ�ʱ, Ӧ��ʹ�õ���ʽ. (����)
2013.05.22  elfSymbolExport ����������, һ��Ϊ�ܷ�����, һ��Ϊ��ǰ�����ķ�����. ����������ڴ�Ч��.
            ģ���ڷ��ű�ʹ�� hash ��, ����ʹ�ñ�ƽ����. (����)
            ����ʹ�û��� IO �����ñ�׼ IO �ӿ�.
2017.02.26  ������ż���֧��.
2017.08.17  ���� c6x DSP ��֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
#include "elf_type.h"
#include "elf_loader.h"
#include "elf_arch.h"
/*********************************************************************************************************
  _G_pcInitSecArr: ��ʼ������������
  _G_pcFiniSecArr: ��������������
  Ŀǰ����ʵ�ֲ������������õ�ѭ��
*********************************************************************************************************/
static const PCHAR              _G_pcInitSecArr[] = {".preinit_array", ".init_array", ".init_array.00100"};
static const PCHAR              _G_pcFiniSecArr[] = {".fini_array", ".fini_array.00100"};
#define __LW_CTORS_SECTION      ".ctors"                                /*  GCC Ĭ�Ϲ���������������    */
#define __LW_DTORS_SECTION      ".dtors"
/*********************************************************************************************************
  module������ṩ�Ļص��������������������
*********************************************************************************************************/
extern LW_LD_EXEC_MODULE *moduleLoadSub(LW_LD_EXEC_MODULE *pmodule, CPCHAR pchLibName, BOOL bCreate);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static CPCHAR  __elfGetMachineStr(Elf_Half  ehMachine);
/*********************************************************************************************************
  �Ƿ� entry ����������ű�
*********************************************************************************************************/
#define __LW_ENTRY_SYMBOL       1
/*********************************************************************************************************
** ��������: elfSymHashSize
** ��������: ���ݷ�������ȷ�� hash ���С.
** �䡡��  : ulSymMax      ��������
** �䡡��  : hash ��С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfSymHashSize (ULONG  ulSymMax)
{
    if (ulSymMax < 10) {
        return  (1);
    } else if (ulSymMax < 100) {
        return  (13);
    } else if (ulSymMax < 1000) {
        return  (97);
    } else if (ulSymMax < 5000) {
        return  (191);
    } else {
        return  (397);
    }
}
/*********************************************************************************************************
** ��������: elfCheck
** ��������: ���elf�ļ�ͷ��Ч��.
** �䡡��  : pehdr         �ļ�ͷ
**           bLoad         �Ƿ�Ϊװ�ز���
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfCheck (Elf_Ehdr *pehdr, BOOL bLoad)
{
    if ((pehdr->e_ident[EI_MAG0] != ELFMAG0) ||                         /*  ���ELFħ��                 */
        (pehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (pehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (pehdr->e_ident[EI_MAG3] != ELFMAG3)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown file format!\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        return  (PX_ERROR);
    }
    
    if (ELF_CLASS != pehdr->e_ident[EI_CLASS]) {                        /*  ���ELF CPU�ֳ��Ƿ�ƥ��     */
        if (bLoad) {
            fprintf(stderr, "[ld]Architecture error: this CPU is %d-bits but ELF file is %d-bits!\n",
                    (ELF_CLASS == ELFCLASS32) ? 32 : 64,
                    (pehdr->e_ident[EI_CLASS] == ELFCLASS32) ? 32 : 64);
        }
        _ErrorHandle(ERROR_LOADER_ARCH);
        return  (PX_ERROR);
    }

    if (ELF_ARCH != pehdr->e_machine) {                                 /*  ���ELF��ϵ�ṹ�Ƿ�ƥ��     */
        if (bLoad) {
            fprintf(stderr, "[ld]Architecture error: this CPU is \"%s\" but ELF file CPU is \"%s\"!\n",
                    __elfGetMachineStr(ELF_ARCH), __elfGetMachineStr(pehdr->e_machine));
        }
        _ErrorHandle(ERROR_LOADER_ARCH);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfRelaRelocate
** ��������: ������������ض�λ����.
** �䡡��  : pmodule       ģ��ָ��
**           psymTable     ���ű�
**           prela         �ض�λ��
**           pcTargetSect  Ŀ�����
**           ulRelocCount  �ض�λ����
**           pcStrTab      �ַ�����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfRelaRelocate (LW_LD_EXEC_MODULE *pmodule,
                            Elf_Sym           *psymTable,
                            Elf_Rela          *prela,
                            PCHAR              pcTargetSect,
                            ULONG              ulRelocCount,
                            PCHAR              pcStrTab)
{
    Elf_Sym   *psym;
    PCHAR      pcJmpTable;
    PCHAR      pcSymName;

    size_t     stJmpTableItem;
    Elf_Addr   addrSymVal;
    ULONG      i;
    BOOL       bNoSymbol = LW_FALSE;

    LD_DEBUG_MSG(("relocateSectionRela()\r\n"));

    pcJmpTable =                                                        /*  ��ת����ʼ��ַ              */
        (PCHAR)pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_ulAddr;
    stJmpTableItem =                                                    /*  ��ת���С                  */
        pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_stLen;

    for (i = 0; i < ulRelocCount; i++) {
#if defined(LW_CFG_CPU_ARCH_MIPS64)
        psym = &psymTable[ELF_MIPS_R_SYM(prela)];
#else
        psym = &psymTable[ELF_R_SYM(prela->r_info)];
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS64      */
        if (SHN_UNDEF == psym->st_shndx) {                              /*  �ⲿ��������ҷ��ű�        */
            pcSymName = pcStrTab + psym->st_name;
            if (lib_strlen(pcSymName) == 0) {                           /*  ������ţ�����ϵ�ṹ���    */
                addrSymVal = 0;
            
            } else {
                if (__moduleSymGetValue(pmodule,
                                        (STB_WEAK == ELF_ST_BIND(psym->st_info)),
                                        pcSymName,
                                        &addrSymVal,
                                        LW_LD_SYM_ANY) < 0) {           /*  ��ѯ��Ӧ���ŵĵ�ַ          */
                    bNoSymbol = LW_TRUE;
                    prela++;
                    continue;
                }
            }
            
        } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {         /*  ģ���ڲ�����                */
            addrSymVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                       + psym->st_value;
        }

        LD_DEBUG_MSG(("relocate %s :", pcSymName));

        if (archElfRelocateRela(pmodule,
                                prela,
                                psym,
                                addrSymVal,
                                pcTargetSect,
                                pcJmpTable,
                                stJmpTableItem) < 0) {                  /*  ����ϵ�ṹ��ص��ض�λ      */
            _ErrorHandle(ERROR_LOADER_RELOCATE);
            return  (PX_ERROR);
        }

        prela++;
    }
    
    if (bNoSymbol) {
        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
        return  (PX_ERROR);

    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: elfRelRelocate
** ��������: �������������ض�λ����.
** �䡡��  : pmodule       ģ��ָ��
**           psymTable     ���ű�
**           prel          �ض�λ��
**           pcTargetSect  Ŀ�����
**           ulRelocCount  �ض�λ����
**           pcStrTab      �ַ�����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfRelRelocate (LW_LD_EXEC_MODULE *pmodule,
                           Elf_Sym           *psymTable,
                           Elf_Rel           *prel,
                           PCHAR              pcTargetSect,
                           ULONG              ulRelocCount,
                           PCHAR              pcStrTab)
{
    Elf_Sym   *psym;
    PCHAR      pcJmpTable;
    PCHAR      pcSymName;

    size_t     stJmpTableItem;
    Elf_Addr   symVal;
    ULONG      i;
    BOOL       bNoSymbol = LW_FALSE;

    LD_DEBUG_MSG(("relocateSectionRel()\r\n"));

    pcJmpTable =                                                        /*  ��ת����ʼ��ַ              */
            (PCHAR)pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_ulAddr;
    stJmpTableItem =                                                    /*  ��ת���С                  */
            pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_stLen;

    for (i = 0; i < ulRelocCount; i++) {
#if defined(LW_CFG_CPU_ARCH_MIPS64)
        psym = &psymTable[ELF_MIPS_R_SYM(prel)];
#else
        psym = &psymTable[ELF_R_SYM(prel->r_info)];
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS64      */
        pcSymName = pcStrTab + psym->st_name;
        if (SHN_UNDEF == psym->st_shndx) {                              /*  �ⲿ��������ҷ��ű�        */
            if (lib_strlen(pcSymName) == 0) {                           /* ������ţ�����ϵ�ṹ���     */
                symVal = 0;
            
            } else {
                if (__moduleSymGetValue(pmodule,
                                        (STB_WEAK == ELF_ST_BIND(psym->st_info)),
                                        pcSymName,
                                        &symVal,
                                        LW_LD_SYM_ANY) < 0) {           /*  ��ѯ��Ӧ���ŵĵ�ַ          */
                    bNoSymbol = LW_TRUE;
                    prel++;
                    continue;
                }
            }
            
        } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {         /*  ģ���ڲ�����                */
            symVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                   + psym->st_value;
        }

        LD_DEBUG_MSG(("relocate %s :", pcSymName));
        
        if (archElfRelocateRel(pmodule,
                               prel,
                               psym,
                               symVal,
                               pcTargetSect,
                               pcJmpTable,
                               stJmpTableItem) < 0) {                   /*  ����ϵ�ṹ��ص��ض�λ      */
            _ErrorHandle(ERROR_LOADER_RELOCATE);
            return  (PX_ERROR);
        }

        prel++;
    }

    if (bNoSymbol) {
        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
        return  (PX_ERROR);

    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: elfSectionsRelocate
** ��������: ���������ض�λ��.
** �䡡��  : pmodule       ģ��ָ��
**           pshdrArr      ����ͷ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfSectionsRelocate (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    PCHAR      pcTargetSect;
    PCHAR      pcStrTab;

    ULONG      ulRelocCount;
    ULONG      ulStrShIndex;
    INT        iError = PX_ERROR;
    ULONG      i;

    LD_DEBUG_MSG(("relocateSections()\r\n"));
    
    pshdr = pshdrArr;
    for (i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if ((pshdr->sh_type != SHT_REL) && (pshdr->sh_type != SHT_RELA)) {
            continue;                                                   /* ֻ�����ض�λ��               */
        }

        if (pshdr->sh_entsize == 0) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        /*
         *  �ض�λĿ���
         */
        pcTargetSect = (PCHAR)pmodule->EMOD_psegmentArry[pshdr->sh_info].ESEG_ulAddr;
        if (pcTargetSect == 0) {
            continue;                                                   /*  ���ǿɼ��ؽ���              */
        }

        /*
         *  �ض�λ��ʹ�õķ��ű�
         */
        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[pshdr->sh_link].ESEG_ulAddr;
        if (psymTable == 0) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        /*
         *  �ض�λ���ű�ʹ�õ��ַ�����
         */
        ulStrShIndex = (INT)pshdrArr[pshdr->sh_link].sh_link;
        pcStrTab     = (PCHAR)pmodule->EMOD_psegmentArry[ulStrShIndex].ESEG_ulAddr;
        if (LW_NULL == pcStrTab) {
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        ulRelocCount = (INT)(pshdr->sh_size / pshdr->sh_entsize);       /*  �����ض�λ������Ŀ          */
        switch (pshdr->sh_type) {

        case SHT_REL:                                                   /*  �����������ض�λ��          */
            iError = elfRelRelocate(pmodule,
                                    psymTable,
                                    (Elf_Rel *)(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr),
                                    pcTargetSect,
                                    ulRelocCount,
                                    pcStrTab);
            break;

        case SHT_RELA:                                                  /*  ���������ض�λ��            */
            iError = elfRelaRelocate(pmodule,
                                     psymTable,
                                     (Elf_Rela *)(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr),
                                     pcTargetSect,
                                     ulRelocCount,
                                     pcStrTab);
            break;

        default:
            _ErrorHandle(ERROR_LOADER_FORMAT);
            iError = PX_ERROR;
            break;
        }

        if (iError < 0) {
            return  (iError);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfSymbolExport
** ��������: ����ģ���ʼ�����ź���ڷ��ţ�����ȫ�ַ���. (�������������Ϊ������ڴ�Ч��)
** �䡡��  : pmodule       ģ��ָ��
**           pcShName      ������
**           pcSymName     ������
**           psym          ����ָ��
**           addrSymVal    ����ֵ
**           ulAllSymCnt   �ܷ�����
**           ulCurSymNum   ��ǰΪ�ڼ���
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : __sylixos_version ���Ų����Ƿ��е�����������, �����뵼������.
*********************************************************************************************************/
static INT elfSymbolExport (LW_LD_EXEC_MODULE *pmodule,
                            PCHAR              pcShName,
                            PCHAR              pcSymName,
                            Elf_Sym           *psym,
                            Elf_Addr           addrSymVal,
                            ULONG              ulAllSymCnt,
                            ULONG              ulCurSymNum)
{
    INT  iFlag;

    if ((LW_NULL != pmodule->EMOD_pcInit) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcInit))) {           /*  ��ʼ������                  */
        pmodule->EMOD_pfuncInit = (FUNCPTR)addrSymVal;
        LD_DEBUG_MSG(("init: %lx\r\n", addrSymVal));
        return  (ERROR_NONE);
    }

    if ((LW_NULL != pmodule->EMOD_pcExit) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcExit))) {           /*  ���ٺ���                    */
        pmodule->EMOD_pfuncExit = (FUNCPTR)addrSymVal;
        LD_DEBUG_MSG(("exit: %lx\r\n", addrSymVal));
        return  (ERROR_NONE);
    }

    if ((LW_NULL != pmodule->EMOD_pcEntry) &&
        (!pmodule->EMOD_bIsSymbolEntry) &&
        (0 == lib_strcmp(pcSymName, pmodule->EMOD_pcEntry))) {          /*  ��ں���                    */
        pmodule->EMOD_pfuncEntry = (FUNCPTR)addrSymVal;
        pmodule->EMOD_bIsSymbolEntry = LW_TRUE;                         /*  ͨ������ƥ���ҵ����        */
        LD_DEBUG_MSG(("entry: %lx\r\n", addrSymVal));
        
#if __LW_ENTRY_SYMBOL == 0
        return  (ERROR_NONE);
#endif                                                                  /*  __LW_ENTRY_SYMBOL == 0      */
    }

    if (LW_FALSE == pmodule->EMOD_bExportSym) {
        return  (ERROR_NONE);
    }

#if !defined(LW_CFG_CPU_ARCH_C6X)
    if ((pcShName != LW_NULL) &&
        (pmodule->EMOD_pcSymSection != LW_NULL) &&
        (0 != lib_strcmp(pcShName, pmodule->EMOD_pcSymSection)) &&
        (0 != lib_strcmp(pcSymName, "__sylixos_version"))) {            /* ֻ����ָ���ڵķ���           */
        return  (ERROR_NONE);
    }
#else
    if ((pcShName != LW_NULL) &&
        (pmodule->EMOD_pcSymSection != LW_NULL) &&
        (pcShName != lib_strstr(pcShName, pmodule->EMOD_pcSymSection)) &&
        (0 != lib_strcmp(pcSymName, "__sylixos_version"))) {            /* ֻ����ָ���ڵķ���           */
        return  (ERROR_NONE);
    }
#endif

    if (STB_WEAK == ELF_ST_BIND(psym->st_info)) {
        iFlag = LW_SYMBOL_FLAG_WEAK;                                    /*  ������                      */
    } else {
        iFlag = 0;
    }

    switch (ELF_ST_TYPE(psym->st_info)) {

    case STT_FUNC:                                                      /*  ��������                    */
        iFlag |= LW_LD_SYM_FUNCTION;
        if (__moduleExportSymbol(pmodule, pcSymName, addrSymVal,
                                 iFlag, ulAllSymCnt, ulCurSymNum) < 0) {
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);

    case STT_OBJECT:                                                    /*  ��������                    */
#if defined(LW_CFG_CPU_ARCH_C6X)
    case STT_COMMON:
#endif
        iFlag |= LW_LD_SYM_DATA;
        if (__moduleExportSymbol(pmodule, pcSymName, addrSymVal,
                                 iFlag, ulAllSymCnt, ulCurSymNum) < 0) {
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);

    default:
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: elfSymbolsExport
** ��������: ������ض�λ�ļ������з��ű�.
** �䡡��  : pmodule       ģ��ָ��
**           pshdrArr      ����ͷ����
**           uiShStrNdx    ������������ַ��������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfSymbolsExport (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr, UINT uiShStrNdx)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    PCHAR      pcStrTab;
    Elf_Sym   *psym;
    PCHAR      pcSymName;
    PCHAR      pcShName;

    Elf_Addr   addrSymVal;
    ULONG      ulSymCount;
    ULONG      ulStrShIndex;
    ULONG      i, j;

    LD_DEBUG_MSG(("exportSymbols()\r\n"));
    
    pshdr = pshdrArr;
    for (i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_type != SHT_SYMTAB) {                             /*  ֻ������ű����            */
            continue;
        }

        /*
         *  ��ȡ���ű�ָ��
         */
        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (psymTable == 0) {
            continue;
        }

        /*
         *  ��ȡ����ű��Ӧ���ַ�����ָ��
         */
        ulStrShIndex = pshdr->sh_link;
        LD_DEBUG_MSG(("sybol table: %lx, string table: %lx\r\n", i, ulStrShIndex));
        pcStrTab = (PCHAR)pmodule->EMOD_psegmentArry[ulStrShIndex].ESEG_ulAddr;
        if (pcStrTab == 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "string table is NULL!\r\n");
            _ErrorHandle(ERROR_LOADER_FORMAT);
            return  (PX_ERROR);
        }

        ulSymCount = (ULONG)(pshdr->sh_size / sizeof(Elf_Sym));
        for (j = 0; j < ulSymCount; j++) {                              /*  �������ű�                  */
            psym = &psymTable[j];
            pcSymName = pcStrTab + psym->st_name;
            if ((psym->st_shndx == SHN_UNDEF)                ||
#if !defined(LW_CFG_CPU_ARCH_C6X)
                (psym->st_other != STV_DEFAULT)              ||
#endif
                (ELF_ST_BIND(psym->st_info) == STB_LOCAL)    ||
                (psym->st_shndx >= pmodule->EMOD_ulSegCount) ||
                (0 == lib_strlen(pcSymName))) {
                continue;
            }

            addrSymVal = (Elf_Addr)pmodule->EMOD_psegmentArry[psym->st_shndx].ESEG_ulAddr
                       + psym->st_value;                                /*  ��ȡ���ŵ�λ��              */
            LD_DEBUG_MSG(("symbol: %s val: %lx\r\n", pcSymName, addrSymVal));

            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdrArr[psym->st_shndx].sh_name;                /* ��ȡ�������ڽ�����           */

            if (elfSymbolExport(pmodule, pcShName, pcSymName, 
                                psym, addrSymVal, ulSymCount, j) < 0) {
                _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
                return  (PX_ERROR);
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfModuleMemoryInit
** ��������: ��ʼ�����ض�λ�ļ�ģ���ڴ�.
** �䡡��  : pmodule       ģ��ָ��
**           pshdrArr      ����ͷ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfModuleMemoryInit (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdrArr)
{
    Elf_Shdr  *pshdr;
    Elf_Sym   *psymTable;
    Elf_Sym   *psym;

    size_t     stTotalSize;
    ULONG      ulSecAddr;
    ULONG      ulAlign;
    ULONG      ulMaxAlign = 0;
    ULONG      ulSymCount;
    ULONG      ulCommonSize  = 0;                                       /*  �������С                  */
    ULONG      ulExportCount = 0;                                       /*  ����������                  */
    ULONG      ulJmpItemCnt  = 0;
    ULONG      i, j;

    LD_DEBUG_MSG(("initModuleMemory()\r\n"));

    lib_bzero(&pshdrArr[pmodule->EMOD_ulSegCount], sizeof(Elf_Shdr) * 2);

    /*
     *  ������ת��BSS�͵������ű��С
     */
    pshdr = pshdrArr;
    for (i = 0; i < pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_type != SHT_SYMTAB) {                             /*  ֻ�账����ű�              */
            continue;
        }

        psymTable = (Elf_Sym *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (psymTable == 0) {
            continue;
        }
        
        ulSymCount = (INT)(pshdr->sh_size / sizeof(Elf_Sym));
        for (j = 0; j < ulSymCount; j++) {
            psym = &psymTable[j];

            if (SHN_UNDEF == psym->st_shndx) {                          /*  ����ģ����ķ���            */
                ulJmpItemCnt++;

            } else if (SHN_COMMON == psym->st_shndx) {                  /*  ������                      */
                ulAlign = psym->st_value < sizeof(ULONG) ? sizeof(ULONG) : psym->st_value;
                ulCommonSize += ulAlign - 1;                            /*  st_value�б����˶���ֵ      */
                ulCommonSize = (ulCommonSize/ulAlign) * ulAlign;
                psym->st_shndx = (Elf32_Half)pmodule->EMOD_ulSegCount;
                psym->st_value = ulCommonSize;
                ulCommonSize += psym->st_size;
                if (ulAlign > ulMaxAlign) {
                    ulMaxAlign = ulAlign;
                }

            } else if (psym->st_shndx < pmodule->EMOD_ulSegCount) {
                if (SHT_NOBITS == pshdrArr[psym->st_shndx].sh_type &&   /*  ����BSS��С                 */
                    (psym->st_value + psym->st_size) >
                    pshdrArr[psym->st_shndx].sh_size) {
                    pshdrArr[psym->st_shndx].sh_size = psym->st_value
                                                     + psym->st_size;
                }

                if (((STT_FUNC  == ELF_ST_TYPE(psym->st_info)) ||       /*  ���㵼�����ű��С          */
                     (STT_OBJECT == ELF_ST_TYPE(psym->st_info))) &&
                    (STB_GLOBAL == ELF_ST_BIND(psym->st_info))) {
                    ulExportCount++;
                }
            }
        }
    }

    /*
     *  Ϊ�������ű�����ڴ�
     */
    if (pmodule->EMOD_bExportSym && ulExportCount > 0) {
        ULONG   ulHashBytes;
        pmodule->EMOD_ulSymHashSize = (ULONG)elfSymHashSize(ulExportCount);
        ulHashBytes = sizeof(LW_LIST_LINE_HEADER) * pmodule->EMOD_ulSymHashSize;
        pmodule->EMOD_psymbolHash = (LW_LIST_LINE_HEADER *)LW_LD_SAFEMALLOC(ulHashBytes);
        if (LW_NULL == pmodule->EMOD_psymbolHash) {
            pmodule->EMOD_ulSymHashSize = 0ul;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        lib_bzero(pmodule->EMOD_psymbolHash, ulHashBytes);
        LD_DEBUG_MSG(("symbol table addr: %lx\r\n", (ULONG)pmodule->EMOD_psymbolHash));
    }

    /*
     *  ����Common�ڣ�����Common����.
     */
    if (ulCommonSize > 0) {
        pshdrArr[pmodule->EMOD_ulSegCount].sh_size      = ulCommonSize;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_addralign = ulMaxAlign;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_flags     = SHF_ALLOC;
        pshdrArr[pmodule->EMOD_ulSegCount].sh_type      = SHT_NOBITS;

        pmodule->EMOD_ulSegCount++;
    }

    /*
     *  ����һ�������ڱ�����ת��
     */
#ifndef LW_CFG_CPU_ARCH_RISCV
    pshdrArr[pmodule->EMOD_ulSegCount].sh_size  = ulJmpItemCnt
                                                * archElfRGetJmpBuffItemLen(pmodule);
#else
    pshdrArr[pmodule->EMOD_ulSegCount].sh_size  = LW_CFG_RISCV_GOT_SIZE + LW_CFG_RISCV_HI20_SIZE;
#endif                                                                  /*  LW_CFG_CPU_ARCH_RISCV       */
    pshdrArr[pmodule->EMOD_ulSegCount].sh_flags = SHF_ALLOC;
    pshdrArr[pmodule->EMOD_ulSegCount].sh_type  = SHT_NOBITS;

    /*
     *  ����ģ����Ҫ������ڴ��С
     */
    stTotalSize = 0;
    ulMaxAlign  = 0;
    pshdr = pshdrArr;
    for (i = 0; i <= pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_flags & SHF_ALLOC) {
            ulAlign = pshdr->sh_addralign > sizeof(ULONG) ? pshdr->sh_addralign : sizeof(ULONG);
            stTotalSize += ulAlign - 1;
            stTotalSize  = (stTotalSize / ulAlign) * ulAlign;
            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr = stTotalSize;
            pmodule->EMOD_psegmentArry[i].ESEG_stLen = pshdr->sh_size;
            stTotalSize += pshdr->sh_size;
            if (ulAlign > ulMaxAlign) {
                ulMaxAlign = ulAlign;
            }
        }
    }
    stTotalSize += ulMaxAlign;

    pmodule->EMOD_pvBaseAddr = LW_LD_VMSAFEMALLOC(stTotalSize);         /*  �����ڴ�                    */
    if (LW_NULL == pmodule->EMOD_pvBaseAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "alloc vm-memory error!\r\n");
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pmodule->EMOD_pvBaseAddr, stTotalSize);

    pmodule->EMOD_stLen = stTotalSize;
    LD_DEBUG_MSG(("base addr: %lx\r\n", (addr_t)pmodule->EMOD_pvBaseAddr));

    /*
     *  ����ÿ�������ڴ��е�λ��
     */
    pshdr      = pshdrArr;
    ulSecAddr  = (ULONG)pmodule->EMOD_pvBaseAddr;
    ulSecAddr += ulMaxAlign - 1;
    ulSecAddr  = (ulSecAddr / ulMaxAlign) * ulMaxAlign;
    for (i = 0; i <= pmodule->EMOD_ulSegCount; i++, pshdr++) {
        if (pshdr->sh_flags & SHF_ALLOC) {
            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr += ulSecAddr;
        }
    }

#ifdef LW_CFG_CPU_ARCH_RISCV
    pmodule->EMOD_ulRiscvGotNr    = 0;
    pmodule->EMOD_ulRiscvHi20Nr   = 0;
    pmodule->EMOD_ulRiscvGotBase  = pmodule->EMOD_psegmentArry[pmodule->EMOD_ulSegCount].ESEG_ulAddr;
    pmodule->EMOD_ulRiscvHi20Base = pmodule->EMOD_ulRiscvGotBase + LW_CFG_RISCV_GOT_SIZE;
#endif                                                                  /*  LW_CFG_CPU_ARCH_RISCV       */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfBuildInitTbl
** ��������: ������ʼ������������������
** �䡡��  : pmodule       ģ��ָ��
**           pshdrArr      ����ͷ����
**           uiShStrNdx    ������������ַ��������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfBuildInitTbl (LW_LD_EXEC_MODULE *pmodule, Elf_Shdr *pshdr, UINT uiShStrNdx)
{
    INT       i, j, k;
    UINT      uiInitTblSize = 0;
    UINT      uiFiniTblSize = 0;
    PCHAR     pcShName      = LW_NULL;
    Elf_Addr *paddr         = LW_NULL;

    /*
     *  ����init��������init���е������init����������
     */
    for (j = 0; j < (sizeof(_G_pcInitSecArr) / sizeof(_G_pcInitSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /*  ����init���С              */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /*  ��ȡ�������ڽ�����          */
            if (0 == lib_strcmp(pcShName, _G_pcInitSecArr[j])) {        /*  ƥ��.init_array��           */
                uiInitTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        if (pcShName == lib_strstr(pcShName, __LW_CTORS_SECTION)) {     /*  ƥ��.ctor��                 */
            uiInitTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
        }
    }

    if (uiInitTblSize == 0) {                                           /*  û���κκ���                */
        goto    __finibuild;
    }

    pmodule->EMOD_ppfuncInitArray = 
        (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiInitTblSize * sizeof(VOIDFUNCPTR));
    if (!pmodule->EMOD_ppfuncInitArray) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __error;
    }

    pmodule->EMOD_ulInitArrCnt = uiInitTblSize;
    uiInitTblSize = 0;

    for (j = 0; j < (sizeof(_G_pcInitSecArr) / sizeof(_G_pcInitSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /* ����init������               */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /* ��ȡ�������ڽ�����           */
            paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;

            if (0 != lib_strcmp(pcShName, _G_pcInitSecArr[j])) {
                continue;
            }
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncInitArray[uiInitTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (pcShName == lib_strstr(pcShName, __LW_CTORS_SECTION)) {     /*  ƥ��.ctor��                 */
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncInitArray[uiInitTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

__finibuild:
    /*
     *  ����Fini��������Fini���е������Fini����������
     */
    for (j = 0; j < (sizeof(_G_pcFiniSecArr) / sizeof(_G_pcFiniSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {                /*  ����Fini���С              */
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /*  ��ȡ�������ڽ�����          */

            if (0 == lib_strcmp(pcShName, _G_pcFiniSecArr[j])) {        /*  ƥ��.fini_array��           */
                uiFiniTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;
        if (pcShName == lib_strstr(pcShName, __LW_DTORS_SECTION)) {     /*  ƥ��.dtor��                 */
            uiFiniTblSize += (pshdr[i].sh_size / sizeof(Elf_Addr));
        }
    }

    if (uiFiniTblSize == 0) {                                           /*  û����������                */
        goto    __out;
    }

    pmodule->EMOD_ppfuncFiniArray = 
        (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiFiniTblSize * sizeof(VOIDFUNCPTR));
    if (!pmodule->EMOD_ppfuncFiniArray) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __error;
    }

    pmodule->EMOD_ulFiniArrCnt = uiFiniTblSize;
    uiFiniTblSize = 0;
    for (j = 0; j < (sizeof(_G_pcFiniSecArr) / sizeof(_G_pcFiniSecArr[0])); j++) {
        for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
            pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                     + pshdr[i].sh_name;                                /*  ��ȡ�������ڽ�����          */
            paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;

            if (0 != lib_strcmp(pcShName, _G_pcFiniSecArr[j])) {
                continue;
            }
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncFiniArray[uiFiniTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

    for (i = 0; i < pmodule->EMOD_ulSegCount; i++) {
        pcShName = (PCHAR)pmodule->EMOD_psegmentArry[uiShStrNdx].ESEG_ulAddr
                 + pshdr[i].sh_name;                                    /*  ��ȡ�������ڽ�����          */
        paddr = (Elf_Addr *)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr;
        if (pcShName == lib_strstr(pcShName, __LW_DTORS_SECTION)) {     /*  ƥ��.dtor��                 */
            for (k = 0; k < (pshdr[i].sh_size / sizeof(Elf_Addr)); k++) {
                pmodule->EMOD_ppfuncFiniArray[uiFiniTblSize++] = (VOIDFUNCPTR)paddr[k];
            }
        }
    }

__out:
    return  (ERROR_NONE);

__error:
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: elfLoadReloc
** ��������: ���ؿ��ض�λelf�ļ�.
** �䡡��  : pmodule       ģ��ָ��
**           pehdr         �ļ�ͷ
**           iFd           �ļ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfLoadReloc (LW_LD_EXEC_MODULE *pmodule, Elf_Ehdr *pehdr, INT  iFd)
{
    Elf_Shdr    *pshdr;
    PCHAR        pcBuf = LW_NULL;

    size_t       stShdrSize;
    ULONG        i;
    INT          iError = PX_ERROR;

    if (LW_NULL != pmodule->EMOD_psegmentArry ||
        LW_NULL != pmodule->EMOD_psymbolHash  ||
        LW_NULL != pmodule->EMOD_pvBaseAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmodule->EMOD_ulModType = LW_LD_MOD_TYPE_KO;

    /*
     *  ��ȡ����ͷ����
     */
    stShdrSize = pehdr->e_shentsize * pehdr->e_shnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC((stShdrSize +
                                    pehdr->e_shentsize * 2));           /*  ���ܴ��ڹ�����ͳ���ת��    */
    if (LW_NULL == pcBuf) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    if (lseek(iFd, pehdr->e_shoff, SEEK_SET) < 0) {
        goto    __out0;
    }

    if (read(iFd, pcBuf, stShdrSize) < stShdrSize) {
        goto    __out0;
    }

    pmodule->EMOD_psegmentArry = (LW_LD_EXEC_SEGMENT *)LW_LD_SAFEMALLOC(
                                 (sizeof(LW_LD_EXEC_SEGMENT) * (pehdr->e_shnum + 2)));
    if (!pmodule->EMOD_psegmentArry) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }
    
    pmodule->EMOD_ulSegCount = pehdr->e_shnum;
    lib_bzero(pmodule->EMOD_psegmentArry,
              sizeof(LW_LD_EXEC_SEGMENT) * (pehdr->e_shnum + 2));

    /*
     *  ������ű� �ض�λ�ڣ��ַ�����
     */
    pshdr = (Elf_Shdr *)pcBuf;

    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if ((pshdr->sh_type == SHT_SYMTAB) ||
            (pshdr->sh_type == SHT_STRTAB) ||
            (pshdr->sh_type == SHT_RELA)   ||
            (pshdr->sh_type == SHT_REL)) {

            if (pshdr->sh_size <= 0) {
                continue;
            }
            LD_DEBUG_MSG(("load section %x, %x, %x\r\n",
                    pshdr->sh_type,
                    pshdr->sh_offset,
                    pshdr->sh_size));

            pmodule->EMOD_psegmentArry[i].ESEG_ulAddr =
                     (addr_t)LW_LD_VMSAFEMALLOC((size_t)pshdr->sh_size);
            pmodule->EMOD_psegmentArry[i].ESEG_stLen = pshdr->sh_size;
            if (0 == pmodule->EMOD_psegmentArry[i].ESEG_ulAddr) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
                _ErrorHandle(ENOMEM);
                goto    __out1;
            }

            if (lseek(iFd, pshdr->sh_offset, SEEK_SET) < 0) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
                goto    __out1;
            }

            if (read(iFd, (PVOID)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr,
                     pshdr->sh_size) < pshdr->sh_size) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "read file error!\r\n");
                goto    __out1;
            }
        }
    }

    if (elfModuleMemoryInit(pmodule, (Elf_Shdr *)pcBuf) < 0) {
        goto    __out1;
    }

    /*
     *  ��ȡ�������ݶΡ������
     */
    pshdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if ((pshdr->sh_flags & SHF_ALLOC)  &&
            (pshdr->sh_type != SHT_NOBITS) &&
            (pshdr->sh_size > 0)) {

            if (lseek(iFd, pshdr->sh_offset, SEEK_SET) < 0) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
                goto    __out2;
            }

            if (read(iFd, (PVOID)pmodule->EMOD_psegmentArry[i].ESEG_ulAddr,
                     pshdr->sh_size) < pshdr->sh_size) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "read file error!\r\n");
                goto    __out2;
            }
            
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  C6X ʹ��SEGMENTʵ��backtrace*/
            pmodule->EMOD_psegmentArry[i].ESEG_bCanExec = (pshdr->sh_flags & SHF_EXECINSTR) ?
                                                          LW_TRUE : LW_FALSE;
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
        }
    }

    if (elfSectionsRelocate(pmodule, (Elf_Shdr *)pcBuf) < 0) {          /*  �ض�λ                      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("relocation failed\r\n"));
        _ErrorHandle(ERROR_LOADER_RELOCATE);
        goto    __out2;
    }

    if (elfSymbolsExport(pmodule, (Elf_Shdr *)pcBuf, pehdr->e_shstrndx)) {
                                                                        /*  �������ű����ҳ�ʼ������  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("export symbol failed\r\n"));
        _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
        goto    __out2;
    }

    if (elfBuildInitTbl(pmodule, (Elf_Shdr *)pcBuf, pehdr->e_shstrndx)) {/* ������ʼ��������            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, ("build init & fini table failed\r\n"));
        _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
    }

    iError = ERROR_NONE;
    goto    __out1;

__out2:
    LW_LD_VMSAFEFREE(pmodule->EMOD_pvBaseAddr);
    __moduleDeleteAllSymbol(pmodule);
    LW_LD_SAFEFREE(pmodule->EMOD_psymbolHash);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);

__out1:
    pshdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if (!(pshdr->sh_flags & SHF_ALLOC)) {
            if (pmodule->EMOD_psegmentArry[i].ESEG_ulAddr) {
                LW_LD_VMSAFEFREE(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr);
            }
        }
    }
    
#if (LW_CFG_HWDBG_GDBMOD_EN == 0) && \
    (LW_CFG_TRUSTED_COMPUTING_EN == 0) && \
     !defined(LW_CFG_CPU_ARCH_C6X)                                      /*  Ӳ������������ż�����Ҫ    */
    LW_LD_SAFEFREE(pmodule->EMOD_psegmentArry);
    pmodule->EMOD_ulSegCount = 0;
#endif                                                                  /*  !LW_CFG_HWDBG_GDBMOD_EN     */
                                                                        /*  !LW_CFG_TRUSTED_COMPUTING_EN*/
__out0:
    LW_LD_SAFEFREE(pcBuf);
    return  (iError);
}
/*********************************************************************************************************
** ��������: dynPhdrParse
** ��������: ����dynamic��
** �䡡��  : pmodule       ģ��ָ��
**           pdyndir       dynamic���ݽṹ
**           pphdr         �α�
**           iPhdrCnt      ����Ŀ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT dynPhdrParse (LW_LD_EXEC_MODULE *pmodule,
                         ELF_DYN_DIR       *pdyndir,
                         Elf_Phdr          *pphdr,
                         INT                iPhdrCnt)
{
    Elf_Addr     addrMin     = pdyndir->addrMin;
    Elf_Dyn     *pdyn        = LW_NULL;                                 /*  dynamic��ָ��               */
    ULONG        ulItemCount = 0;                                       /*  dynamic����Ŀ��             */

    INT          i, j;

    /*
     *  ����dynamic��
     */
    for (i = 0; i < iPhdrCnt; i++, pphdr++) {
        if (PT_DYNAMIC == pphdr->p_type) {                              /*  �ҵ�dynamic��               */
            pdyn = (Elf_Dyn *)LW_LD_V2PADDR(addrMin,
                                            pmodule->EMOD_pvBaseAddr,
                                            pphdr->p_vaddr);

#if defined(LW_CFG_CPU_ARCH_CSKY)
            pmodule->EMOD_pvCSkyDynamicAddr = (PVOID)pdyn;
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */

            ulItemCount = pphdr->p_filesz / sizeof(Elf_Dyn);            /*  ��̬�ṹ����                */

            for (j = 0; j < ulItemCount; j++, pdyn++) {
                switch (pdyn->d_tag) {

                case DT_NEEDED:
                    if (pdyndir->ulNeededCnt >= __LW_MAX_NEEDED_LIB) {
                        _DebugHandle(__ERRORMESSAGE_LEVEL, "too many needed librarys.\r\n");
                        _ErrorHandle(ERROR_LOADER_FORMAT);
                        return  (PX_ERROR);
                    }
                    pdyndir->wdNeededArr[pdyndir->ulNeededCnt++] = pdyn->d_un.d_val;
                    break;

                case DT_SYMTAB:                                         /*  ���ű�                      */
                    pdyndir->psymTable   = (Elf_Sym *)LW_LD_V2PADDR(addrMin,
                                                           pmodule->EMOD_pvBaseAddr,
                                                           pdyn->d_un.d_ptr);
                    break;

                case DT_STRTAB:                                         /*  �ַ�����                    */
                    pdyndir->pcStrTable  = (PCHAR)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_REL:                                            /*  REL�ض�λ��                 */
                    pdyndir->prelTable   = (Elf_Rel *)LW_LD_V2PADDR(addrMin,
                                                           pmodule->EMOD_pvBaseAddr,
                                                           pdyn->d_un.d_ptr);
                    break;

                case DT_RELSZ:                                          /*  REL�ض�λ���С             */
                    pdyndir->ulRelSize   = pdyn->d_un.d_val;
                    break;

                case DT_RELA:                                           /*  RELA�ض�λ��                */
                    pdyndir->prelaTable  = (Elf_Rela *)LW_LD_V2PADDR(addrMin,
                                                            pmodule->EMOD_pvBaseAddr,
                                                            pdyn->d_un.d_ptr);
                    break;

                case DT_RELASZ:                                         /*  RELA�ض�λ���С            */
                    pdyndir->ulRelaSize  = pdyn->d_un.d_val;
                    break;

                case DT_HASH:                                           /*  HASH��                      */
                    pdyndir->phash       = (Elf_Hash *)LW_LD_V2PADDR(addrMin,
                                                            pmodule->EMOD_pvBaseAddr,
                                                            pdyn->d_un.d_ptr);
                    break;

                case DT_PLTREL:                                         /*  PLT�ض�λ������             */
                    pdyndir->ulPltRel    = pdyn->d_un.d_val;
                    break;

                case DT_JMPREL:                                         /*  PLT�ض�λ��                 */
                    pdyndir->pvJmpRTable = (PCHAR)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_PLTRELSZ:                                       /*  PLT�ض�λ���С             */
                    pdyndir->ulJmpRSize  = pdyn->d_un.d_val;
                    break;

                case DT_INIT_ARRAY:
                    pdyndir->paddrInitArray = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_INIT_ARRAYSZ:
                    pdyndir->ulInitArrSize  = pdyn->d_un.d_val;
                    break;

                case DT_FINI_ARRAY:
                    pdyndir->paddrFiniArray = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                       pmodule->EMOD_pvBaseAddr,
                                                       pdyn->d_un.d_ptr);
                    break;

                case DT_FINI_ARRAYSZ:
                    pdyndir->ulFiniArrSize = pdyn->d_un.d_val;
                    break;

                case DT_INIT:
                    pdyndir->addrInit = (Elf_Addr)LW_LD_V2PADDR(addrMin,
                                                        pmodule->EMOD_pvBaseAddr,
                                                        pdyn->d_un.d_val);
                    break;

                case DT_FINI:
                    pdyndir->addrFini = (Elf_Addr)LW_LD_V2PADDR(addrMin,
                                                        pmodule->EMOD_pvBaseAddr,
                                                        pdyn->d_un.d_val);
                    break;
                    
                case DT_PLTGOT:
                    pdyndir->ulPltGotAddr  = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                         pmodule->EMOD_pvBaseAddr,
                                                         pdyn->d_un.d_ptr);
                    break;
                    
#ifdef  LW_CFG_CPU_ARCH_MIPS
                case DT_MIPS_GOTSYM:
                    pdyndir->ulMIPSGotSymIdx      = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_LOCAL_GOTNO:
                    pdyndir->ulMIPSLocalGotNumIdx = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_SYMTABNO:
                    pdyndir->ulMIPSSymNumIdx      = pdyn->d_un.d_val;
                    break;
                
                case DT_MIPS_PLTGOT:
                    pdyndir->ulMIPSPltGotIdx      = pdyn->d_un.d_val;
                    break;
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

#if defined(LW_CFG_CPU_ARCH_C6X)
                case DT_C6000_DSBT_BASE:
                    pmodule->EMOD_pulDsbtTable = (Elf_Addr *)LW_LD_V2PADDR(addrMin,
                                                 pmodule->EMOD_pvBaseAddr,
                                                 pdyn->d_un.d_ptr);
                    break;

                case DT_C6000_DSBT_SIZE:
                    pmodule->EMOD_ulDsbtSize   = pdyn->d_un.d_val;
                    break;

                case DT_C6000_DSBT_INDEX:
                    pmodule->EMOD_ulDsbtIndex  = pdyn->d_un.d_val;
                    break;
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
                }
            }
            break;
        }
    }

    if (i == iPhdrCnt) {                                                /*  �Ҳ���.dynamic��            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not find dynamic header.\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        return  (PX_ERROR);
    }

    pdyndir->ulRelCount  = pdyndir->ulRelSize / sizeof(Elf_Rel);        /*  REL�ض�λ������             */
    pdyndir->ulRelaCount = pdyndir->ulRelaSize / sizeof(Elf_Rela);      /*  RELA�ض�λ������            */
    pdyndir->ulSymCount  = pdyndir->phash->nchain;                      /*  ���ű�����                  */

    /*
     *  TODO: Ŀǰ��ΪJMPREL����REL��RELA��֮�����û���ҵ�REL���RELA����ʹ��JMPREL��
     */
#if !defined(LW_CFG_CPU_ARCH_PPC) && !defined(LW_CFG_CPU_ARCH_C6X) && \
    !defined(LW_CFG_CPU_ARCH_SPARC) && !defined(LW_CFG_CPU_ARCH_RISCV)
    if (pdyndir->pvJmpRTable) {
        if (DT_REL == pdyndir->ulPltRel) {
            if (!pdyndir->prelTable) {
                pdyndir->prelTable = (Elf_Rel*)(pdyndir->pvJmpRTable);
            }
            pdyndir->ulRelCount += pdyndir->ulJmpRSize/sizeof(Elf_Rel);

        } else if (DT_RELA == pdyndir->ulPltRel) {
            if (!pdyndir->prelaTable) {
                pdyndir->prelaTable = (Elf_Rela*)pdyndir->pvJmpRTable;
            }
            pdyndir->ulRelaCount += pdyndir->ulJmpRSize/sizeof(Elf_Rela);
        }
    }
#endif

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfPhdrRead
** ��������: ��ȡ������elf�ļ�
** �䡡��  : pmodule       ģ��ָ��
**           pdyndir       dynamic���ݽṹ
**           pehdr         elf�ļ�ͷ
**           iFd           elf�ļ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfPhdrRead (LW_LD_EXEC_MODULE *pmodule,
                        ELF_DYN_DIR       *pdyndir,
                        Elf_Ehdr          *pehdr,
                        INT                iFd)
{
    INT             iError = PX_ERROR;

    PCHAR           pcBuf  = LW_NULL;
    Elf_Phdr       *pphdr;

    size_t          stShdrSize;
    ULONG           i;
    Elf_Word        dwAlign  = 0;
    Elf_Word        dwMapOff = 0;
    Elf_Addr        addrMin  = (Elf_Addr)~0;
    Elf_Addr        addrMax  = 0x0;
    
    BOOL            bCanShare;
    BOOL            bCanExec;
    struct stat64   stat64Buf;

#ifdef  LW_CFG_CPU_ARCH_CSKY
    PCHAR           pcShdrBuf = LW_NULL;
    PCHAR           pcShName  = LW_NULL;
    Elf_Shdr       *pshdr;
    INT             iReadLen;
    BOOL            bHasExceptTbl;
#endif

    /*
     *  ��ȡ��ͷ����
     */
    stShdrSize = pehdr->e_phentsize * pehdr->e_phnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC((stShdrSize + pehdr->e_shentsize));

    if (LW_NULL == pcBuf) {
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    if (lseek(iFd, pehdr->e_phoff, SEEK_SET) < 0) {
        goto    __out0;
    }

    if (read(iFd, pcBuf, stShdrSize) < stShdrSize) {
        goto    __out0;
    }
    
    if (fstat64(iFd, &stat64Buf)) {
        goto    __out0;
    }

    pphdr = (Elf_Phdr *)pcBuf;
    for (i = 0; i < pehdr->e_phnum; i++, pphdr++) {
        if (PT_LOAD != pphdr->p_type) {                                 /*  ֻ����ɼ��ض�              */
            continue;
        }

#if LW_CFG_VMM_EN > 0
        if (pphdr->p_align < LW_CFG_VMM_PAGE_SIZE) {                    /*  ���������������ͬһҳ��    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "alignment < page size!\r\n");
            _ErrorHandle(ERROR_LOADER_FORMAT);
            goto    __out0;
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

        if (pphdr->p_vaddr < addrMin) {
            addrMin = pphdr->p_vaddr;
        }

        if ((pphdr->p_vaddr + pphdr->p_memsz) > addrMax) {
            addrMax = pphdr->p_vaddr + pphdr->p_memsz;
        }

        if (pphdr->p_align > dwAlign) {
            dwAlign = pphdr->p_align;
        }
    }

    if (addrMin >= addrMax) {                                           /*  û�пɼ��صĶ�              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no segment can be loaded.\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        goto    __out0;
    }

    /*
     *  TODO: Ŀǰ��δ������ת���������ڷ����Ľ������̬���ӵĿ�ִ���ļ����Դ���ת��(PLT)
     */
    pmodule->EMOD_stLen      = (size_t)(addrMax - addrMin);
    pmodule->EMOD_pvBaseAddr = LW_LD_VMSAFEMALLOC_AREA_ALIGN(pmodule->EMOD_stLen,
                                                             (size_t)dwAlign);
    if (!pmodule->EMOD_pvBaseAddr) {                                    /*  Ϊ��ִ���ļ������ڴ�ռ�    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }

    /*
     *  �ֶ�ӳ��ɼ��ض�
     */
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0)  || defined(LW_CFG_CPU_ARCH_C6X)  /*  ��̬������Ҫȷ���ڴ�ṹ    */
    pmodule->EMOD_ulSegCount   = pehdr->e_phnum;
    pmodule->EMOD_psegmentArry = (LW_LD_EXEC_SEGMENT *)LW_LD_SAFEMALLOC(
                                 (sizeof(LW_LD_EXEC_SEGMENT) * pmodule->EMOD_ulSegCount));
    if (!pmodule->EMOD_psegmentArry) {
        pmodule->EMOD_ulSegCount = 0ul;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
        _ErrorHandle(ENOMEM);
        goto    __out0;
    }
    lib_bzero(pmodule->EMOD_psegmentArry,
              sizeof(LW_LD_EXEC_SEGMENT) * pmodule->EMOD_ulSegCount);
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
     
#ifdef  LW_CFG_CPU_ARCH_CSKY
    /*
     *  ��ȡ����ͷ����
     */
    stShdrSize = pehdr->e_shentsize * pehdr->e_shnum;

    pcShdrBuf  = (PCHAR)LW_LD_SAFEMALLOC(stShdrSize);
    if (LW_NULL == pcShdrBuf) {
        _ErrorHandle(ENOMEM);
        goto    __out2;
    }

    if (lseek(iFd, pehdr->e_shoff, SEEK_SET) < 0) {
        goto    __out2;
    }

    if (read(iFd, pcShdrBuf, stShdrSize) < stShdrSize) {
        goto    __out2;
    }

    pshdr = (Elf_Shdr *)pcShdrBuf;

    pcShName = (PCHAR)LW_LD_SAFEMALLOC(pshdr[pehdr->e_shstrndx].sh_size);
    if (pcShName == LW_NULL) {
        goto    __out2;
    }

    if (lseek(iFd, pshdr[pehdr->e_shstrndx].sh_offset, SEEK_SET) < 0) {
        goto    __out2;
    }

    iReadLen = read(iFd, pcShName, pshdr[pehdr->e_shstrndx].sh_size);
    if (iReadLen < pshdr[pehdr->e_shstrndx].sh_size) {
        goto    __out2;
    }

    bHasExceptTbl = LW_FALSE;

    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if (lib_strcmp(pcShName + pshdr->sh_name, ".gcc_except_table") == 0) {
            bHasExceptTbl = LW_TRUE;
        }
    }
#endif

    pphdr = (Elf_Phdr *)pcBuf;
    for (i = 0; i < pehdr->e_phnum; i++, pphdr++) {
        
#ifdef LW_CFG_CPU_ARCH_ARM
        if (pphdr->p_type == PT_ARM_EXIDX) {
            pmodule->EMOD_pvARMExidx = (PVOID)LW_LD_V2PADDR(addrMin,
                                       pmodule->EMOD_pvBaseAddr, pphdr->p_vaddr);
            pmodule->EMOD_stARMExidxCount = pphdr->p_filesz / 8;
        }
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */
        if ((PT_LOAD != pphdr->p_type) || (pphdr->p_filesz == 0)) {
            continue;
        }

        dwMapOff  = pphdr->p_vaddr - addrMin;
#ifdef  LW_CFG_CPU_ARCH_CSKY
        bCanShare = (bHasExceptTbl || (PF_W & pphdr->p_flags))
                    ? LW_FALSE : LW_TRUE;                               /*  �Ƿ�ɹ���                  */
#else
        bCanShare = PF_W & pphdr->p_flags ? LW_FALSE : LW_TRUE;         /*  �Ƿ�ɹ���                  */
#endif
        bCanExec  = PF_X & pphdr->p_flags ? LW_TRUE  : LW_FALSE;        /*  �Ƿ��ִ��                  */

#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
        if (!pmodule->EMOD_stCodeLen && bCanExec) {                     /*  ��¼�����λ����Ϣ          */
            pmodule->EMOD_ulCodeOft = dwMapOff;
            pmodule->EMOD_stCodeLen = pphdr->p_memsz;
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

        if (LW_LD_VMSAFEMAP_AREA(pmodule->EMOD_pvBaseAddr, dwMapOff, iFd, &stat64Buf,
                                 pphdr->p_offset, pphdr->p_filesz, 
                                 bCanShare, bCanExec) != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "mmap error!\r\n");
            goto    __out1;
        }
        
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  ��̬������Ҫȷ���ڴ�ṹ    */
        pmodule->EMOD_psegmentArry[i].ESEG_ulAddr   = (addr_t)pmodule->EMOD_pvBaseAddr
                                                    + dwMapOff;
        pmodule->EMOD_psegmentArry[i].ESEG_stLen    = pphdr->p_filesz;
        pmodule->EMOD_psegmentArry[i].ESEG_bCanExec = bCanExec;
#endif                                                                  /* LW_CFG_TRUSTED_COMPUTING_EN  */

        if (pphdr->p_memsz > pphdr->p_filesz) {                         /* �����ڴ�ռ�����ļ����ݳ��� */
            dwMapOff += pphdr->p_filesz;
            
            if (LW_LD_VMSAFEMAP_AREA(pmodule->EMOD_pvBaseAddr, dwMapOff, PX_ERROR, &stat64Buf,
                                     0, (pphdr->p_memsz - pphdr->p_filesz), 
                                     bCanShare, bCanExec)  != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "mmap error!\r\n");
                goto    __out1;
            }
        }
    }
    
    LW_LD_VMSAFE_SHARE(pmodule->EMOD_pvBaseAddr, pmodule->EMOD_stLen, 
                       stat64Buf.st_dev, stat64Buf.st_ino);             /*  ���ù�����                */
    
    /*
     *   ����dynamic��
     */
    pdyndir->addrMin = addrMin;
    pdyndir->addrMax = addrMax;
    if (dynPhdrParse(pmodule, pdyndir, (Elf_Phdr *)pcBuf, pehdr->e_phnum)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "seek file error!\r\n");
        _ErrorHandle(ERROR_LOADER_FORMAT);
        goto    __out1;
    }

#ifdef  LW_CFG_CPU_ARCH_CSKY
    /*
     *  ���� RELA �ض�λ���������ض�λ��
     */
    pshdr = (Elf_Shdr *)pcShdrBuf;

    for (i = 0; i < pehdr->e_shnum; i++, pshdr++) {
        if ((pshdr->sh_type == SHT_RELA)   ||
            (pshdr->sh_type == SHT_REL)) {

            if (pshdr->sh_size <= 0) {
                continue;
            }

            pdyndir->ulRelaSize   = pshdr->sh_size;
            pdyndir->ulRelaCount += pshdr->sh_size / sizeof(Elf_Rela);   /*  RELA�ض�λ������            */
            pdyndir->prelaTable   = (Elf_Rela *)LW_LD_V2PADDR(addrMin,
                                                    pmodule->EMOD_pvBaseAddr,
                                                    pshdr->sh_offset);
            break;
        }
    }

__out2:
    LW_LD_SAFEFREE(pcShName);
    LW_LD_SAFEFREE(pcShdrBuf);
#endif

    iError = ERROR_NONE;
    goto    __out0;

__out1:
    LW_LD_VMSAFEFREE_AREA(pmodule->EMOD_pvBaseAddr);

__out0:
    LW_LD_SAFEFREE(pcBuf);

    return  (iError);
}
/*********************************************************************************************************
** ��������: elfPhdrRelocate
** ��������: �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           pdyndir       dynamic���ݽṹ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfPhdrRelocate (LW_LD_EXEC_MODULE *pmodule, ELF_DYN_DIR  *pdyndir)
{
    Elf_Addr     addrMin   = pdyndir->addrMin;
    Elf_Rel     *prel      = LW_NULL;
    Elf_Rela    *prela     = LW_NULL;
    PCHAR        pcSymName = LW_NULL;
    Elf_Sym     *psym      = LW_NULL;
    Elf_Addr     addrSymVal;

    PCHAR        pcBase    = LW_NULL;
    INT          i;
    BOOL         bNoSymbol = LW_FALSE;

    pcBase = (PCHAR)LW_LD_V2PADDR(addrMin,
                                  pmodule->EMOD_pvBaseAddr,
                                  0);                                   /*  elf��0��ַ�ļ��ص�ַ        */

    if (archElfGotInit(pmodule) < 0) {
        return  (PX_ERROR);
    }

    /*
     *  �ض�λ
     */
    if (pdyndir->prelTable) {                                           /*  REL�ض�λ�ṹ               */
        for (i = 0; i < pdyndir->ulRelCount; i++) {
            prel = &pdyndir->prelTable[i];
#if defined(LW_CFG_CPU_ARCH_MIPS64)
            psym = &pdyndir->psymTable[ELF_MIPS_R_SYM(prel)];
#else
            psym = &pdyndir->psymTable[ELF_R_SYM(prel->r_info)];
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS64      */
            pcSymName = pdyndir->pcStrTable + psym->st_name;

            addrSymVal = 0;
            if (SHN_UNDEF == psym->st_shndx) {                          /*  �ⲿ��������ҷ��ű�        */
                if (lib_strlen(pcSymName) == 0) {                       /*  ������ţ�����ϵ�ṹ���    */
                    addrSymVal = 0;
                
                } else {
                    if (__moduleSymGetValue(pmodule,
                                            (STB_WEAK == ELF_ST_BIND(psym->st_info)),
                                            pcSymName,
                                            &addrSymVal,
                                            LW_LD_SYM_ANY) < 0) {       /*  ��ѯ��Ӧ���ŵĵ�ַ          */
                        bNoSymbol = LW_TRUE;
                        continue;
                    }
                }
                
            } else {                                                    /*  ģ���ڲ�����                */
                addrSymVal = LW_LD_V2PADDR(addrMin,
                                           pmodule->EMOD_pvBaseAddr,
                                           psym->st_value);
            }

            if (archElfRelocateRel(pmodule,                             /*  �ض�λ����                  */
                                   prel,
                                   psym,
                                   addrSymVal,
                                   pcBase,
                                   LW_NULL, 0) < 0) {
                _ErrorHandle(ERROR_LOADER_RELOCATE);
                return  (PX_ERROR);
            }
        }
    
    } else if (pdyndir->prelaTable) {                                   /*  RELA�ض�λ�ṹ              */
        for (i = 0; i < pdyndir->ulRelaCount; i++) {
            prela = &pdyndir->prelaTable[i];
#if defined(LW_CFG_CPU_ARCH_MIPS64)
            psym = &pdyndir->psymTable[ELF_MIPS_R_SYM(prela)];
#else
            psym = &pdyndir->psymTable[ELF_R_SYM(prela->r_info)];
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS64      */
            pcSymName = pdyndir->pcStrTable + psym->st_name;

            addrSymVal = 0;
            if (SHN_UNDEF == psym->st_shndx) {                          /*  �ⲿ��������ҷ��ű�        */
                if (lib_strlen(pcSymName) == 0) {                       /*  ������ţ�����ϵ�ṹ���    */
                    addrSymVal = 0;
                
                } else {
                    if (__moduleSymGetValue(pmodule,
                                            (STB_WEAK == ELF_ST_BIND(psym->st_info)),
                                            pcSymName,
                                            &addrSymVal,
                                            LW_LD_SYM_ANY) < 0) {       /*  ��ѯ��Ӧ���ŵĵ�ַ          */
                        bNoSymbol = LW_TRUE;
                        continue;
                    }
                }
                
            } else {                                                    /*  ģ���ڲ�����                */
                addrSymVal = LW_LD_V2PADDR(addrMin,
                                           pmodule->EMOD_pvBaseAddr,
                                           psym->st_value);
            }

            if (archElfRelocateRela(pmodule,                            /*  �ض�λ����                  */
                                    prela,
                                    psym,
                                    addrSymVal,
                                    pcBase,
                                    LW_NULL, 0) < 0) {
                _ErrorHandle(ERROR_LOADER_RELOCATE);
                return  (PX_ERROR);
            }
        }
    }

    if (bNoSymbol) {
        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
        return  (PX_ERROR);

    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: elfPhdrSymExport
** ��������: ��������
** �䡡��  : pmodule       ģ��ָ��
**           pdyndir       dynamic���ݽṹ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfPhdrSymExport (LW_LD_EXEC_MODULE *pmodule, ELF_DYN_DIR  *pdyndir)
{
    Elf_Addr     addrMin   = pdyndir->addrMin;
    PCHAR        pcSymName = LW_NULL;
    Elf_Sym     *psym      = LW_NULL;

    Elf_Addr     addrSymVal;
    INT          i;

    if (pmodule->EMOD_bExportSym && pdyndir->ulSymCount) {              /*  Ϊ�������ű�����ڴ�        */
        ULONG   ulHashBytes;
        pmodule->EMOD_ulSymHashSize = (ULONG)elfSymHashSize(pdyndir->ulSymCount);
        ulHashBytes = sizeof(LW_LIST_LINE_HEADER) * pmodule->EMOD_ulSymHashSize;
        pmodule->EMOD_psymbolHash = (LW_LIST_LINE_HEADER *)LW_LD_SAFEMALLOC(ulHashBytes);
        if (LW_NULL == pmodule->EMOD_psymbolHash) {
            pmodule->EMOD_ulSymHashSize = 0ul;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        lib_bzero(pmodule->EMOD_psymbolHash, ulHashBytes);
        LD_DEBUG_MSG(("symbol table addr: %lx\r\n", (addr_t)pmodule->EMOD_psymbolHash));
    }

    for (i = 0; i < pdyndir->ulSymCount; i++) {                         /*  ������ű�                  */
        psym = &pdyndir->psymTable[i];
        pcSymName = pdyndir->pcStrTable + psym->st_name;
        if ((psym->st_shndx == SHN_UNDEF) ||
            (ELF_ST_BIND(psym->st_info) == STB_LOCAL) ||
            (0 == lib_strlen(pcSymName))) {
            continue;
        }

        addrSymVal = LW_LD_V2PADDR(addrMin,
                                   pmodule->EMOD_pvBaseAddr,
                                   psym->st_value);                     /*  ��ȡ���ŵ�λ��              */
        LD_DEBUG_MSG(("symbol: %s val: %lx\r\n", pcSymName, addrSymVal));

        if (elfSymbolExport(pmodule, LW_NULL, pcSymName, 
                            psym, addrSymVal, pdyndir->ulSymCount, i) < 0) {
                                                                        /*  �������ű���ʼ����ں���  */
            _ErrorHandle(ERROR_LOADER_EXPORT_SYM);
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfPhdrBuildInitTable
** ��������: ���� dymanic ��ʼ����
** �䡡��  : pmodule       ģ��ָ��
**           pdyndir       dynamic���ݽṹ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfPhdrBuildInitTable (LW_LD_EXEC_MODULE *pmodule,
                                  ELF_DYN_DIR       *pdyndir)
{
    INT       i;
    UINT      uiInitTblSize = (UINT)(pdyndir->ulInitArrSize / sizeof(Elf_Addr));
    UINT      uiFiniTblSize = (UINT)(pdyndir->ulFiniArrSize / sizeof(Elf_Addr));

    if ((uiInitTblSize > 0) && (LW_NULL != pdyndir->paddrInitArray)) {
        pmodule->EMOD_ppfuncInitArray = 
            (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiInitTblSize * sizeof(VOIDFUNCPTR));
        if (!pmodule->EMOD_ppfuncInitArray) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pmodule->EMOD_ulInitArrCnt = uiInitTblSize;

        for (i = 0; i < uiInitTblSize; i++) {
            pmodule->EMOD_ppfuncInitArray[i] = (VOIDFUNCPTR)pdyndir->paddrInitArray[i];
        }
    }

    if (pdyndir->addrInit != 0) {
        pmodule->EMOD_pfuncInit = (FUNCPTR)pdyndir->addrInit;
    }

    if ((uiFiniTblSize > 0) && (LW_NULL != pdyndir->paddrFiniArray)) {
        pmodule->EMOD_ppfuncFiniArray = 
            (VOIDFUNCPTR *)LW_LD_SAFEMALLOC(uiFiniTblSize * sizeof(VOIDFUNCPTR));
        if (!pmodule->EMOD_ppfuncFiniArray) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory!\r\n");
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pmodule->EMOD_ulFiniArrCnt = uiFiniTblSize;

        for (i = 0; i < uiFiniTblSize; i++) {
            pmodule->EMOD_ppfuncFiniArray[i] = (VOIDFUNCPTR)pdyndir->paddrFiniArray[i];
        }
    }

    if (pdyndir->addrFini != 0) {
        pmodule->EMOD_pfuncExit = (FUNCPTR)pdyndir->addrFini;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: elfLoadExec
** ��������: ���ؿ�ִ��elf�ļ�.
** �䡡��  : pmodule       ģ��ָ��
**           pehdr         �ļ�ͷ
**           iFd           �ļ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT elfLoadExec (LW_LD_EXEC_MODULE *pmodule, Elf_Ehdr *pehdr, INT iFd)
{
    ELF_DYN_DIR        *pdyndir     = LW_NULL;
    INT                 i;
    PCHAR               pchLibName  = LW_NULL;
    LW_LD_EXEC_MODULE **ppmodUseArr = LW_NULL;

    if (LW_NULL != pmodule->EMOD_psegmentArry ||
        LW_NULL != pmodule->EMOD_psymbolHash  ||
        LW_NULL != pmodule->EMOD_pvBaseAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmodule->EMOD_ulModType = LW_LD_MOD_TYPE_SO;

    pdyndir = (ELF_DYN_DIR *)LW_LD_SAFEMALLOC(sizeof(ELF_DYN_DIR));
    if (!pdyndir) {
        goto __out;
    }

    lib_bzero(pdyndir, sizeof(ELF_DYN_DIR));
    if (elfPhdrRead(pmodule, pdyndir, pehdr, iFd)) {                    /*  ��ȡ�ͽ���elf�еĸ�����     */
        goto    __out;
    }

    /*
     *  TODO: ELFͷ�л���г�����ڵ�ַ����loader�ӿ���Ҳ��������ں������ƣ������߲�һ��ʱ�ᵼ�»��ҡ�
     */
    if (pehdr->e_entry) {                                               /*  �������                    */
        pmodule->EMOD_pfuncEntry = (FUNCPTR)LW_LD_V2PADDR(pdyndir->addrMin,
                                                 pmodule->EMOD_pvBaseAddr,
                                                 pehdr->e_entry);
    }

    if (elfPhdrSymExport(pmodule, pdyndir)) {                           /*  �������ű�                  */
        goto    __out;
    }

    /*
     *  �����������Ŀ⣬Ϊ���ѭ���������⣬���������ű������һ��ǰ�档
     */
    if (pdyndir->ulNeededCnt > 0) {
        pmodule->EMOD_ulUsedCnt = pdyndir->ulNeededCnt;
        pmodule->EMOD_pvUsedArr = LW_LD_SAFEMALLOC(pmodule->EMOD_ulUsedCnt *
                                                   sizeof (LW_LD_EXEC_MODULE *));
        if (!pmodule->EMOD_pvUsedArr) {
            goto    __out;
        }

        lib_bzero(pmodule->EMOD_pvUsedArr,
                  pmodule->EMOD_ulUsedCnt * sizeof (LW_LD_EXEC_MODULE *));
    }

    /*
     *  ������ķ�ʽ�������������Ŀ�.
     */
    ppmodUseArr = pmodule->EMOD_pvUsedArr;
    for (i = 0; i < pdyndir->ulNeededCnt; i++) {
        if (LW_NULL == pdyndir->pcStrTable) {
            break;
        }
        pchLibName = pdyndir->pcStrTable + pdyndir->wdNeededArr[pdyndir->ulNeededCnt - i - 1];
#if !defined(LW_CFG_CPU_ARCH_C6X)
        ppmodUseArr[i] = moduleLoadSub(pmodule, pchLibName, LW_TRUE);
#else
        ppmodUseArr[i] = moduleLoadSub(pmodule, _PathLastNamePtr(pchLibName), LW_TRUE);
#endif
        if (ppmodUseArr[i] == LW_NULL) {
            goto    __out;
        }
        ppmodUseArr[i]->EMOD_ulRefs++;
    }

    pmodule->EMOD_pvFormatInfo = (PVOID)pdyndir;                        /*  ���е�����װ�سɹ�!         */

    return  (ERROR_NONE);

__out:
    LW_LD_VMSAFEFREE_AREA(pmodule->EMOD_pvBaseAddr);
    __moduleDeleteAllSymbol(pmodule);
    LW_LD_SAFEFREE(pmodule->EMOD_psymbolHash);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);
    LW_LD_SAFEFREE(pmodule->EMOD_pvUsedArr);
    LW_LD_SAFEFREE(pdyndir);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __elfLoad
** ��������: ����elf�ļ�.
** �䡡��  : pmodule       ģ��ָ��
**           pcPath        �ļ�·��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __elfLoad (LW_LD_EXEC_MODULE *pmodule, CPCHAR pcPath)
{
    INT           iFd;
    struct stat   statBuf;
    Elf_Ehdr      ehdr;
    INT           iError = PX_ERROR;
    PCHAR         pcModVersion = LW_NULL;

    iFd = open(pcPath, O_RDONLY);
    if (iFd < 0) {                                                      /*  ���ļ�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "open file error!\r\n");
        return  (PX_ERROR);
    }
    
    if (fstat(iFd, &statBuf) < ERROR_NONE) {                            /*  ����ļ���Ϣ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not get file stat!\r\n");
        goto    __out;
    }
    
    if (_IosCheckPermissions(O_RDONLY, LW_TRUE, statBuf.st_mode,        /*  ����ļ�ִ��Ȩ��            */
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(ERROR_LOADER_EACCES);
        goto    __out;
    }

    if (read(iFd, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {                /*  ��ȡELFͷ                   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "read elf header error!\r\n");
        goto    __out;
    }

    if (elfCheck(&ehdr, LW_TRUE) < 0) {                                 /*  ����ļ�ͷ��Ч��            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "elf format error!\r\n");
        goto    __out;
    }

    switch (ehdr.e_type) {

    case ET_REL:
        iError = elfLoadReloc(pmodule, &ehdr, iFd);                     /*  ���ؿ��ض�λ�ļ�            */
        break;

    case ET_EXEC:
    case ET_DYN:
        iError = elfLoadExec(pmodule, &ehdr, iFd);                      /*  ���ؿ�ִ���ļ�              */
        break;

    default:
        _ErrorHandle(ERROR_LOADER_FORMAT);
    }

    if (iError == ERROR_NONE) {
        if (ERROR_NONE != __moduleFindSym(pmodule,
                                          LW_LD_VER_SYM,
                                          (ULONG*)&pcModVersion,
                                          LW_NULL,
                                          LW_LD_SYM_DATA)) {            /*  ��ȡģ��汾                */
            pcModVersion = LW_LD_DEF_VER;
        }

        if (pcModVersion == LW_NULL) {
            pcModVersion = LW_LD_DEF_VER;                               /*  ʹ��Ĭ�ϰ汾                */
        }

        if (ERROR_NONE != LW_LD_VERIFY_VER(pmodule->EMOD_pcModulePath, 
                                           pcModVersion, 
                                           pmodule->EMOD_ulModType)) {
            _ErrorHandle(ERROR_LOADER_VERSION);                         /*  �汾�����Դ���              */
            iError = PX_ERROR;
        }
    }

__out:
    close(iFd);
    
    if (iError == ERROR_NONE) {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_LOAD,
                          pmodule, pmodule->EMOD_pvproc->VP_pid, pcPath);
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __elfListLoad
** ��������: ����ģ������ָ�룬���ڴ�������ϵ��ģ�����
** �䡡��  : pmodule       ģ������ָ��
**           pcPath        �ļ�·��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __elfListLoad (LW_LD_EXEC_MODULE *pmodule, CPCHAR pcPath)
{
    INT                iError    = PX_ERROR;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LIST_RING      *pringTemp = LW_NULL;

    /*
     *  ����װ��������Ҫװ�ص�ģ��
     */
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_ulStatus == LW_LD_STATUS_UNLOAD) {           /*  ģ�黹û�б�װ��            */
            if (__elfLoad(pmodTemp, pmodTemp->EMOD_pcModulePath)) {
                if (errno == ERROR_LOADER_EACCES) {
                    fprintf(stderr, "[ld]%s insufficient permissions!\n",
                            pmodTemp->EMOD_pcModulePath);               /*  �ӱ�׼�������ӡ��Ȩ����Ϣ  */
                } else {
                    fprintf(stderr, "[ld]Load file \"%s\" error %s!\n",
                            pmodTemp->EMOD_pcModulePath, lib_strerror(errno));
                    _ErrorHandle(ERROR_LOADER_NO_MODULE);
                }
                goto    __out;
            }
            pmodTemp->EMOD_ulStatus = LW_LD_STATUS_LOADED;
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    /*
     *  Ȼ���ض�λ������Ҫ�ض�λ��ģ��
     */
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_pvFormatInfo) {                              /*  ģ�黹û������ض�λ        */
            if (elfPhdrRelocate(pmodTemp, (ELF_DYN_DIR *)pmodTemp->EMOD_pvFormatInfo)) {
                                                                        /*  �ض�λ                      */
                goto    __out;
            }

            if (elfPhdrBuildInitTable(pmodTemp, (ELF_DYN_DIR *)pmodTemp->EMOD_pvFormatInfo)) {
                goto    __out;
            }

#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
            if (pmodTemp->EMOD_stCodeLen) {                             /*  ���������                  */
                LW_LD_VMSAFE_PROTECT(pmodTemp->EMOD_pvBaseAddr,
                                     pmodTemp->EMOD_ulCodeOft,
                                     pmodTemp->EMOD_stCodeLen);
            }
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */
        }
#if LW_CFG_CACHE_EN > 0
        API_CacheTextUpdate(pmodTemp->EMOD_pvBaseAddr, pmodTemp->EMOD_stLen);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    iError = ERROR_NONE;

__out:
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        LW_LD_SAFEFREE(pmodTemp->EMOD_pvFormatInfo);
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);

    return  (iError);
}
/*********************************************************************************************************
** ��������: __elfListUnload
** ��������: ж�� elf �ļ�, ���������� elf.
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
** ע  ��  : ��ǰ���ǽ�ж��ģ�����Ϣ��¼�� monitor.
*********************************************************************************************************/
VOID  __elfListUnload (LW_LD_EXEC_MODULE *pmodule)
{
#if LW_CFG_MONITOR_EN > 0
    LW_LIST_RING      *pringTemp;
    LW_LD_EXEC_MODULE *pmodTemp;
    LW_LD_VPROC       *pvproc;
    
    pvproc    = pmodule->EMOD_pvproc;
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_ulRefs == 0) {
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_UNLOAD,
                              pmodTemp, pvproc->VP_pid, LW_NULL);
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
}
/*********************************************************************************************************
** ��������: __elfGetMachineStr
** ��������: ͨ�� e_machine ��ȡ���������ͷ���.
** �䡡��  : ehMachine     �������
** �䡡��  : ���������ַ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR  __elfGetMachineStr (Elf_Half  ehMachine)
{
    switch (ehMachine) {

    case EM_NONE:
        return  ("none");

    case EM_M32:
        return  ("AT&T WE 32100");

    case EM_SPARC:
        return  ("Sun SPARC");

    case EM_386:
        return  ("Intel 80386");

    case EM_68K:
        return  ("Motorola 68000");

    case EM_88K:
        return  ("Motorola 88000");

    case EM_486:
        return  ("Intel 80486");

    case EM_860:
        return  ("Intel i860");

    case EM_MIPS:
        return  ("MIPS family");

    case EM_PARISC:
        return  ("HPPA");
        
    case EM_SPARC32PLUS:
        return  ("Sun's SPARCv8 plus");

    case EM_PPC:
        return  ("PowerPC family");

    case EM_PPC64:
        return  ("PowerPC64 family");

    case EM_S390:
        return  ("IBM S/390");
        
    case EM_SPU:
        return  ("Cell BE SPU");

    case EM_CSKY:
        return  ("C-SKY");

    case EM_ARM:
        return  ("ARM family");

    case EM_SH:
        return  ("Hitachi SH family");
        
    case EM_SPARCV9:
        return  ("SPARC v9 64-bit");
    
    case EM_H8_300:
        return  ("Renesas H8/300,300H,H8S");
    
    case EM_IA_64:
        return  ("HP/Intel IA-64");
    
    case EM_X86_64:
        return  ("AMD x86-64");
    
    case EM_CRIS:
        return  ("Axis 32-bit processor");
    
    case EM_V850:
        return  ("NEC v850");
    
    case EM_M32R:
        return  ("Renesas M32R");
    
    case EM_MN10300:
        return  ("Panasonic/MEI MN10300, AM33");
    
    case EM_BLACKFIN:
        return  ("ADI Blackfin Processor");
    
    case EM_ALTERA_NIOS2:
        return  ("ALTERA NIOS2");

    case EM_TI_C6000:
        return  ("TI C6x DSPs");

    case EM_AARCH64:
        return  ("ARM AArch64");
    
    case EM_RISCV:
        return  ("RISC-V");
        
    case EM_FRV:
        return  ("Fujitsu FR-V");
    
    case EM_AVR32:
        return  ("Atmel AVR32");
    
    case EM_ALPHA:
        return  ("Alpha");
    
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** ��������: __elfGetETypeStr
** ��������: ͨ�� e_type ��ȡ elf ����.
** �䡡��  : ehEType      e_type
** �䡡��  : elf ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR  __elfGetETypeStr (Elf_Half  ehEType)
{
    switch (ehEType) {
    
    case ET_NONE:
        return  ("ET_NONE");
        
    case ET_REL:
        return  ("ET_REL");
        
    case ET_EXEC:
        return  ("ET_EXEC");
    
    case ET_DYN:
        return  ("ET_DYN");
    
    case ET_CORE:
        return  ("ET_CORE");
    
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** ��������: __elfGetShTypeStr
** ��������: ͨ�� sh_type ��ȡ�ֶ�����.
** �䡡��  : ewShType      �ֶ�����
** �䡡��  : �ֶ������ַ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR  __elfGetShTypeStr (Elf_Word  ewShType)
{
    switch (ewShType) {

    case SHT_NULL:
        return  ("NULL");

    case SHT_PROGBITS:
        return  ("PROGBITS");

    case SHT_SYMTAB:
        return  ("SYMTAB");

    case SHT_STRTAB:
        return  ("STRTAB");

    case SHT_RELA:
        return  ("RELA");

    case SHT_HASH:
        return  ("HASH");

    case SHT_DYNAMIC:
        return  ("DYNAMIC");

    case SHT_NOTE:
        return  ("NOTE");

    case SHT_NOBITS:
        return  ("NOBITS");

    case SHT_REL:
        return  ("REL");

    case SHT_SHLIB:
        return  ("SHLIB");

    case SHT_DYNSYM:
        return  ("DYNSYM");

    case SHT_NUM:
        return  ("NUM");

    case SHT_LOPROC:
        return  ("LOPROC");

    case SHT_HIPROC:
        return  ("HIPROC");

    case SHT_LOUSER:
        return  ("LOUSER");

    case SHT_HIUSER:
        return  ("HIUSER");

    default:
        return  ("NONE");
    }
}
/*********************************************************************************************************
** ��������: __elfGetShFlagStr
** ��������: ͨ�� sh_flag ��ȡ�ֶ�flag.
** �䡡��  : ewShFlag      ewShFlag
**           pcBuffer      �������
**           stSize        �����С
** �䡡��  : �ֶ�flag
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR  __elfGetShFlagStr (Elf_Word  ewShFlag, PCHAR pcBuffer, size_t stSize)
{
    pcBuffer[0] = PX_EOS;

    if (ewShFlag & SHF_ALLOC) {
        lib_strlcat(pcBuffer, "[ALLOC]", stSize);
    }
    
    if (ewShFlag & SHF_EXECINSTR) {
        lib_strlcat(pcBuffer, "[EXEC]", stSize);
    }
    
    if (ewShFlag & SHF_WRITE) {
        lib_strlcat(pcBuffer, "[WRITE]", stSize);
    }
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** ��������: __elfStatus
** ��������: �鿴elf�ļ���Ϣ.
** �䡡��  : pcPath     �ļ�·��
**           iFd        ��Ϣ����ļ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __elfStatus (CPCHAR pcPath, INT iFd)
{
    INT           iFdElf;
    Elf_Shdr     *shdr;
    PCHAR         pcBuf = LW_NULL;

    Elf_Ehdr      ehdr;

    size_t        stHdSize;
    ULONG         i;

    iFdElf = open(pcPath, O_RDONLY);
    if (iFdElf < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "open file error!\r\n");
        return  (PX_ERROR);
    }

    if (read(iFdElf, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {             /*  ��ȡELFͷ                   */
        close(iFdElf);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "read elf header error!\r\n");
        return  (PX_ERROR);
    }

    if (elfCheck(&ehdr, LW_FALSE) < 0) {                                /*  ����ļ�ͷ��Ч��            */
        if (API_GetLastError() != ERROR_LOADER_ARCH) {                  /*  �ļ���ʽ����                */
            close(iFdElf);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "elf format error!\r\n");
            return  (PX_ERROR);
        }
    }

    fdprintf(iFd, "File Type: ELF\r\n");                                /*  ��ӡ�ļ���Ϣ                */
    fdprintf(iFd, "Machine:   %s\r\n", __elfGetMachineStr(ehdr.e_machine));
    fdprintf(iFd, "Type:      %s\r\n", __elfGetETypeStr(ehdr.e_type));
    fdprintf(iFd, "Entry:     %x\r\n", ehdr.e_entry);

    /*
     *  ��ȡ����ͷ����
     */
    stHdSize = ehdr.e_shentsize * ehdr.e_shnum;
    pcBuf = (PCHAR)LW_LD_SAFEMALLOC(stHdSize);
    if (pcBuf == LW_NULL) {
        close(iFdElf);
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    if (lseek(iFdElf, ehdr.e_shoff, SEEK_SET) < 0) {
        close(iFdElf);
        LW_LD_SAFEFREE(pcBuf);
        return  (PX_ERROR);
    }
    if (read(iFdElf, pcBuf, stHdSize) < stHdSize) {
        close(iFdElf);
        LW_LD_SAFEFREE(pcBuf);
        return  (PX_ERROR);
    }

    /*
     *  ��ӡ����ͷ������Ϣ
     */
    fdprintf(iFd, "Section Headers:\r\n");
    fdprintf(iFd, "%-10s%-10s%-10s%-10s%-10s\r\n",
             "TYPE",
             "ADDRESS",
             "OFFSET",
             "SIZE",
             "FLAGS");

    shdr = (Elf_Shdr *)pcBuf;
    for (i = 0; i < ehdr.e_shnum; i++, shdr++) {
        CHAR    cBuffer[128];
        fdprintf(iFd, "%-8s  %08x  %8x  %8x  %s\r\n",
                 __elfGetShTypeStr(shdr->sh_type),
                 shdr->sh_addr,
                 shdr->sh_offset,
                 shdr->sh_size,
                 __elfGetShFlagStr(shdr->sh_flags, cBuffer, sizeof(cBuffer)));
    }

    close(iFdElf);
    LW_LD_SAFEFREE(pcBuf);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
