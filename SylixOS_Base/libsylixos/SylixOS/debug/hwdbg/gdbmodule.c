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
** ��   ��   ��: gdbmodule.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 06 �� 15 ��
**
** ��        ��: Ӳ�� GDB ���������ɶ�Ӧģ����Ӧ�ó�����ű�����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_HWDBG_GDBMOD_EN > 0)
#include "loader/elf/elf_type.h"
#include "loader/include/loader_lib.h"
/*********************************************************************************************************
** ��������: gdbmElfCheck
** ��������: ���elf�ļ�ͷ��Ч��.
** �䡡��  : pehdr         �ļ�ͷ
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  gdbmElfCheck (Elf_Ehdr *pehdr)
{
    if ((pehdr->e_ident[EI_MAG0] != ELFMAG0) ||                         /*  ���ELFħ��                 */
        (pehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (pehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (pehdr->e_ident[EI_MAG3] != ELFMAG3)) {
        fprintf(stderr, "unknown file format!\r\n");
        return  (PX_ERROR);
    }
    
    if (ELF_ARCH != pehdr->e_machine) {
        fprintf(stderr, "ELF file machine not fixed with current machine!\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbmAddSym
** ��������: gdbaddsym ����
** �䡡��  : argc         ��������
**           argv         ������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbmAddSym (INT argc, CHAR **argv)
{
    INT                 i;
    Elf_Shdr           *pshdr;
    Elf_Ehdr            ehdr;
    FILE               *fp;
    PCHAR               pcBuf;
    PCHAR               pcShName;
    PCHAR               pcFileName;
    LW_LD_EXEC_MODULE  *pmodule = LW_NULL;
    size_t              stHdSize;
    addr_t              ulTextAddr = 0;
    
    if (argc < 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    if (sscanf(argv[1], "%p", &pmodule) != 1) {
        fprintf(stderr, "module handle error.\n");
        return  (PX_ERROR);
    }
    
    if (pmodule->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC) {
        fprintf(stderr, "module handle error.\n");
        return  (PX_ERROR);
    }
    
    fp = fopen(pmodule->EMOD_pcModulePath, "r");
    if (LW_NULL == fp) {
        fprintf(stderr, "module can not open: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    if (fread(&ehdr, 1, sizeof(ehdr), fp) < sizeof(ehdr)) {             /*  ��ȡELFͷ                   */
        fclose(fp);
        fprintf(stderr, "read ELF header error.\n");
        return  (PX_ERROR);
    }

    if (gdbmElfCheck(&ehdr) < 0) {                                      /*  ����ļ�ͷ��Ч��            */
        fclose(fp);
        fprintf(stderr, "ELF format error.\n");
        return  (PX_ERROR);
    }
    
    /*
     *  ��ȡ����ͷ����
     */
    stHdSize = ehdr.e_shentsize * ehdr.e_shnum;
    pcBuf    = (PCHAR)lib_malloc(stHdSize);
    if (pcBuf == LW_NULL) {
        fclose(fp);
        return  (PX_ERROR);
    }

    if (fseek(fp, ehdr.e_shoff, SEEK_SET) < 0) {
        fclose(fp);
        lib_free(pcBuf);
        return  (PX_ERROR);
    }
    
    if (fread(pcBuf, 1, stHdSize, fp) < stHdSize) {
        fclose(fp);
        lib_free(pcBuf);
        return  (PX_ERROR);
    }

    pshdr = (Elf_Shdr *)pcBuf;

    /*
     *  ��ȡ����������Ϣ
     */
    pcShName = (PCHAR)lib_malloc(pshdr[ehdr.e_shstrndx].sh_size);
    if (pcShName == LW_NULL) {
        fclose(fp);
        lib_free(pcBuf);
        return  (PX_ERROR);
    }

    if (fseek(fp, pshdr[ehdr.e_shstrndx].sh_offset, SEEK_SET) < 0) {
        fclose(fp);
        lib_free(pcBuf);
        lib_free(pcShName);
        return  (PX_ERROR);
    }
    
    if (fread(pcShName, 1, pshdr[ehdr.e_shstrndx].sh_size, fp) < pshdr[ehdr.e_shstrndx].sh_size) {
        fclose(fp);
        lib_free(pcBuf);
        lib_free(pcShName);
        return  (PX_ERROR);
    }

    pcFileName = lib_rindex(pmodule->EMOD_pcModulePath, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    
    } else {
        pcFileName = pmodule->EMOD_pcModulePath;
    }

    printf("GDB Command:\n");
    for (i = 0; i < ehdr.e_shnum; i++, pshdr++) {
        if (lib_strcmp(pcShName + pshdr->sh_name, ".text") == 0) {
            ulTextAddr = (addr_t)pmodule->EMOD_pvBaseAddr + pshdr->sh_addr;
        }
    }

    printf("#attach module %s\n", pcFileName);
    printf("add-symbol-file <YOUR PPROJECT OUTPUT DIR>/%s 0x%lx ", pcFileName, ulTextAddr);
    for (i = 0, pshdr = (Elf_Shdr *)pcBuf; i < ehdr.e_shnum; i++, pshdr++) {
        if ((pshdr->sh_flags & SHF_ALLOC)  &&
            (pshdr->sh_type != SHT_NOBITS) &&
            (pshdr->sh_size > 0)) {
            if (ehdr.e_type == ET_REL) {
                printf("-s %s 0x%lx ", 
                           pcShName + pshdr->sh_name,
                           pmodule->EMOD_psegmentArry[i].ESEG_ulAddr);
                 
            } else if ((ehdr.e_type == ET_EXEC) || (ehdr.e_type == ET_DYN)) {
                printf("-s %s 0x%lx ", 
                           pcShName + pshdr->sh_name,
                           pshdr->sh_addr + (addr_t)pmodule->EMOD_pvBaseAddr);
            }
        }
    }
    printf("\n");

    fclose(fp);
    lib_free(pcShName);
    lib_free(pcBuf);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GdbModuleInit
** ��������: ע�� GDB module ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_GdbModuleInit (VOID)
{
    API_TShellKeywordAdd("gdbaddsym", gdbmAddSym);
    API_TShellFormatAdd("gdbaddsym", " [module handle]");
    API_TShellHelpAdd("gdbaddsym",   "generate GDB add symbol command to debug "
                                     "application and module with JTAG port\n");
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                                                                        /*  LW_CFG_HWDBG_GDBMOD_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
