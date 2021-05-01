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
** ��   ��   ��: riscvElf.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: ʵ�� RISC-V ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef LW_CFG_CPU_ARCH_RISCV                                            /*  RISC-V ��ϵ�ṹ             */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
#include <linux/compat.h>
/*********************************************************************************************************
  RISC-V relocation types
*********************************************************************************************************/
/*********************************************************************************************************
  Relocation types used by the dynamic linker
*********************************************************************************************************/
#define R_RISCV_NONE            0
#define R_RISCV_32              1
#define R_RISCV_64              2
#define R_RISCV_RELATIVE        3
#define R_RISCV_COPY            4
#define R_RISCV_JUMP_SLOT       5
#define R_RISCV_TLS_DTPMOD32    6
#define R_RISCV_TLS_DTPMOD64    7
#define R_RISCV_TLS_DTPREL32    8
#define R_RISCV_TLS_DTPREL64    9
#define R_RISCV_TLS_TPREL32     10
#define R_RISCV_TLS_TPREL64     11
/*********************************************************************************************************
  Relocation types not used by the dynamic linker
*********************************************************************************************************/
#define R_RISCV_BRANCH          16
#define R_RISCV_JAL             17
#define R_RISCV_CALL            18
#define R_RISCV_CALL_PLT        19
#define R_RISCV_GOT_HI20        20
#define R_RISCV_TLS_GOT_HI20    21
#define R_RISCV_TLS_GD_HI20     22
#define R_RISCV_PCREL_HI20      23
#define R_RISCV_PCREL_LO12_I    24
#define R_RISCV_PCREL_LO12_S    25
#define R_RISCV_HI20            26
#define R_RISCV_LO12_I          27
#define R_RISCV_LO12_S          28
#define R_RISCV_TPREL_HI20      29
#define R_RISCV_TPREL_LO12_I    30
#define R_RISCV_TPREL_LO12_S    31
#define R_RISCV_TPREL_ADD       32
#define R_RISCV_ADD8            33
#define R_RISCV_ADD16           34
#define R_RISCV_ADD32           35
#define R_RISCV_ADD64           36
#define R_RISCV_SUB8            37
#define R_RISCV_SUB16           38
#define R_RISCV_SUB32           39
#define R_RISCV_SUB64           40
#define R_RISCV_GNU_VTINHERIT   41
#define R_RISCV_GNU_VTENTRY     42
#define R_RISCV_ALIGN           43
#define R_RISCV_RVC_BRANCH      44
#define R_RISCV_RVC_JUMP        45
#define R_RISCV_LUI             46
#define R_RISCV_GPREL_I         47
#define R_RISCV_GPREL_S         48
#define R_RISCV_TPREL_I         49
#define R_RISCV_TPREL_S         50
#define R_RISCV_RELAX           51
/*********************************************************************************************************
  �ض�λ��������ָ������
*********************************************************************************************************/
typedef INT  (*RISCV_RELA_HANDLE_FUNC) (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal);
/*********************************************************************************************************
  GOT ��Ŀ
*********************************************************************************************************/
typedef struct {
    Elf_Addr                GOTE_symVal;
} RISCV_GOT_ENTRY, *PRISCV_GOT_ENTRY;
/*********************************************************************************************************
  RISCV_HI20_RELOC_INFO �ṩһ�ַ����� HI20 �ض�λ����Ϣ���ݸ� LO12 �ض�λ��
*********************************************************************************************************/
typedef struct {
    Elf_Rela                HI20_rela;
    Elf_Addr                HI20_symValue;
    PCHAR                   HI20_pcTargetSec;
} RISCV_HI20_RELOC_INFO, *PRISCV_HI20_RELOC_INFO;
/*********************************************************************************************************
** ��������: riscvElfModuleEmitGotEntry
** ��������: ���� GOT ��Ŀ
** �䡡��  : pmodule       ģ��ָ��
**           symVal        ����ֵ
** �䡡��  : GOT ��Ŀ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  riscvElfModuleEmitGotEntry (LW_LD_EXEC_MODULE  *pmodule, Elf_Addr  symVal)
{
    PRISCV_GOT_ENTRY  pGotArray = (PRISCV_GOT_ENTRY)pmodule->EMOD_ulRiscvGotBase;
    ULONG             i;

    for (i = 0; i < pmodule->EMOD_ulRiscvGotNr; i++) {
        if (pGotArray[i].GOTE_symVal == symVal) {
            return  ((UINT64)(&pGotArray[i]));
        }
    }

    if (pmodule->EMOD_ulRiscvGotNr >= (LW_CFG_RISCV_GOT_SIZE / sizeof(RISCV_GOT_ENTRY))) {
        return  (0);
    }

    pGotArray[i].GOTE_symVal = symVal;
    pmodule->EMOD_ulRiscvGotNr++;

    return  ((UINT64)(&pGotArray[i]));
}
/*********************************************************************************************************
** ��������: riscvElfModuleEmitPltEntry
** ��������: ���� PLT ��Ŀ
** �䡡��  : pmodule       ģ��ָ��
**           symVal        ����ֵ
** �䡡��  : PLT ��Ŀ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  riscvElfModuleEmitPltEntry (LW_LD_EXEC_MODULE  *pmodule, Elf_Addr  symVal)
{
    _DebugFormat(__ERRORMESSAGE_LEVEL, "%s: failed to emit plt entry!\r\n", pmodule->EMOD_pcModulePath);
    return  (0);
}
/*********************************************************************************************************
** ��������: riscvElf32Rela
** ��������: R_RISCV_32 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElf32Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(UINT32 *)puiLocation = symVal;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElf64Rela
** ��������: R_RISCV_64 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElf64Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(UINT64 *)puiLocation = symVal;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfBranchRela
** ��������: R_RISCV_BRANCH �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfBranchRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset  = (VOID *)symVal - (VOID *)puiLocation;
    UINT32  imm12   = (offset & 0x1000) << (31 - 12);
    UINT32  imm11   = (offset & 0x800)  >> (11 -  7);
    UINT32  imm10_5 = (offset & 0x7e0)  << (30 - 10);
    UINT32  imm4_1  = (offset & 0x1e)   << (11 -  4);

    *puiLocation = (*puiLocation & 0x1fff07f) | imm12 | imm11 | imm10_5 | imm4_1;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfJalRela
** ��������: R_RISCV_JAL �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfJalRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset   = (VOID *)symVal - (VOID *)puiLocation;
    UINT32  imm20    = (offset & 0x100000) << (31 - 20);
    UINT32  imm19_12 = (offset & 0xff000);
    UINT32  imm11    = (offset & 0x800)    << (20 - 11);
    UINT32  imm10_1  = (offset & 0x7fe)    << (30 - 10);

    *puiLocation = (*puiLocation & 0xfff) | imm20 | imm19_12 | imm11 | imm10_1;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfRvcBranchRela
** ��������: R_RISCV_RVC_BRANCH �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfRvcBranchRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset = (VOID *)symVal - (VOID *)puiLocation;
    UINT16  imm8   = (offset & 0x100) << (12 -  8);
    UINT16  imm7_6 = (offset & 0xc0)  >> ( 6 -  5);
    UINT16  imm5   = (offset & 0x20)  >> ( 5 -  2);
    UINT16  imm4_3 = (offset & 0x18)  << (12 -  5);
    UINT16  imm2_1 = (offset & 0x6)   << (12 - 10);

    *(UINT16 *)puiLocation = (*(UINT16 *)puiLocation & 0xe383) | imm8 | imm7_6 | imm5 | imm4_3 | imm2_1;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfRvcJumpRela
** ��������: R_RISCV_RVC_JUMP �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfRvcJumpRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr symVal)
{
    SINT64  offset = (VOID *)symVal - (VOID *)puiLocation;
    UINT16  imm11  = (offset & 0x800) << (12 - 11);
    UINT16  imm10  = (offset & 0x400) >> (10 -  8);
    UINT16  imm9_8 = (offset & 0x300) << (12 - 11);
    UINT16  imm7   = (offset & 0x80)  >> ( 7 -  6);
    UINT16  imm6   = (offset & 0x40)  << (12 - 11);
    UINT16  imm5   = (offset & 0x20)  >> ( 5 -  2);
    UINT16  imm4   = (offset & 0x10)  << (12 -  5);
    UINT16  imm3_1 = (offset & 0xe)   << (12 - 10);

    *(UINT16 *)puiLocation = (*(UINT16 *)puiLocation & 0xe003) | \
                             imm11 | imm10 | imm9_8 | imm7 | imm6 | imm5 | imm4 | imm3_1;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfPcrelHi20Rela
** ��������: R_RISCV_PCREL_HI20 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfPcrelHi20Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset = (VOID *)symVal - (VOID *)puiLocation;
    SINT32  hi20;

    if (offset != (SINT32)offset) {
        _DebugFormat(__ERRORMESSAGE_LEVEL,
          "%s: target %016llx can not be addressed by the 32-bit offset from PC = %p\r\n",
          pmodule->EMOD_pcModulePath, symVal, puiLocation);
        return  (-EINVAL);
    }

    hi20 = (offset + 0x800) & 0xfffff000;
    *puiLocation = (*puiLocation & 0xfff) | hi20;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfPcrelLo12IRela
** ��������: R_RISCV_PCREL_LO12_I �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfPcrelLo12IRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    /*
     * v is the lo12 value to fill. It is calculated before calling this
     * handler.
     */
    *puiLocation = (*puiLocation & 0xfffff) | ((symVal & 0xfff) << 20);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfPcrelLo12SRela
** ��������: R_RISCV_PCREL_LO12_S �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfPcrelLo12SRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    /*
     * v is the lo12 value to fill. It is calculated before calling this
     * handler.
     */
    UINT32  imm11_5 = (symVal & 0xfe0) << (31 - 11);
    UINT32  imm4_0  = (symVal & 0x1f)  << (11 -  4);

    *puiLocation = (*puiLocation & 0x1fff07f) | imm11_5 | imm4_0;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfHi20Rela
** ��������: R_RISCV_HI20 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfHi20Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT32  hi20;

    hi20 = ((SINT32)symVal + 0x800) & 0xfffff000;
    *puiLocation = (*puiLocation & 0xfff) | hi20;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfLo12IRela
** ��������: R_RISCV_LO12_I �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfLo12IRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr symVal)
{
    /*
     * Skip medlow checking because of filtering by HI20 already
     */
    SINT32  hi20 = ((SINT32)symVal + 0x800) & 0xfffff000;
    SINT32  lo12 = ((SINT32)symVal - hi20);

    *puiLocation = (*puiLocation & 0xfffff) | ((lo12 & 0xfff) << 20);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfLo12SRela
** ��������: R_RISCV_LO12_S �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfLo12SRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    /*
     * Skip medlow checking because of filtering by HI20 already
     */
    SINT32  hi20    = ((SINT32)symVal + 0x800) & 0xfffff000;
    SINT32  lo12    = ((SINT32)symVal - hi20);
    UINT32  imm11_5 = (lo12 & 0xfe0) << (31 - 11);
    UINT32  imm4_0  = (lo12 &  0x1f) << (11 -  4);

    *puiLocation = (*puiLocation & 0x1fff07f) | imm11_5 | imm4_0;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfGotHi20Rela
** ��������: R_RISCV_GOT_HI20 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfGotHi20Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64 offset = (VOID *)symVal - (VOID *)puiLocation;
    SINT32 hi20;

    /*
     * Always emit the got entry
     */
    offset = riscvElfModuleEmitGotEntry(pmodule, symVal);
    if (offset == 0) {
        return  (PX_ERROR);
    }
    offset = (VOID *)offset - (VOID *)puiLocation;

    hi20 = (offset + 0x800) & 0xfffff000;
    *puiLocation = (*puiLocation & 0xfff) | hi20;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfCallPltRela
** ��������: R_RISCV_CALL_PLT �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfCallPltRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset = (VOID *)symVal - (VOID *)puiLocation;
    SINT32  fill_v = offset;
    UINT32  hi20, lo12;

    if (offset != fill_v) {
        /*
         * Only emit the plt entry if offset over 32-bit range
         */
        offset = riscvElfModuleEmitPltEntry(pmodule, symVal);
        if (offset == 0) {
            return  (PX_ERROR);
        }
        offset = (VOID *)offset - (VOID *)puiLocation;
    }

    hi20 = (offset + 0x800) & 0xfffff000;
    lo12 = (offset - hi20)  & 0xfff;
    *puiLocation       = (*puiLocation & 0xfff) | hi20;
    *(puiLocation + 1) = (*(puiLocation + 1) & 0xfffff) | (lo12 << 20);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfCallRela
** ��������: R_RISCV_CALL �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfCallRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    SINT64  offset = (VOID *)symVal - (VOID *)puiLocation;
    SINT32  fill_v = offset;
    UINT32  hi20, lo12;

    if (offset != fill_v) {
        _DebugFormat(__ERRORMESSAGE_LEVEL,
          "%s: target %016llx can not be addressed by the 32-bit offset from PC = %p\r\n",
          pmodule->EMOD_pcModulePath, symVal, puiLocation);
        return  (-EINVAL);
    }

    hi20 = (offset + 0x800) & 0xfffff000;
    lo12 = (offset - hi20) & 0xfff;
    *puiLocation = (*puiLocation & 0xfff) | hi20;
    *(puiLocation + 1) = (*(puiLocation + 1) & 0xfffff) | (lo12 << 20);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfRelaxRela
** ��������: R_RISCV_RELAX �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfRelaxRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfAlignRela
** ��������: R_RISCV_ALIGN �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfAlignRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfAdd32Rela
** ��������: R_RISCV_ADD32 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfAdd32Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(UINT32 *)puiLocation += symVal;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfSub32Rela
** ��������: R_RISCV_SUB32 �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfSub32Rela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(UINT32 *)puiLocation -= symVal;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfJumpSlotRela
** ��������: R_RISCV_JUMP_SLOT �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfJumpSlotRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(Elf_Addr *)puiLocation = symVal;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfRelativeRela
** ��������: R_RISCV_RELATIVE �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfRelativeRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    *(Elf_Addr *)puiLocation = symVal + (Elf_Addr)pmodule->EMOD_pvBaseAddr;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfNoneRela
** ��������: R_RISCV_NONE �ض�λ
** �䡡��  : pmodule       ģ��ָ��
**           puiLocation   �ض�λ��λ��
**           symVal        ����ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfNoneRela (LW_LD_EXEC_MODULE  *pmodule, UINT32  *puiLocation, Elf_Addr  symVal)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfAddHI20RelaSymVal
** ��������: ��� HI20 ���͵��ض�λ��ͷ���ֵ
** ��  ��  : pmodule      Module������
**           prela        �ض�λ��
**           symVal       �ض�λ���ŵ�ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfAddHI20RelaSymVal (LW_LD_EXEC_MODULE  *pmodule,
                                       Elf_Rela           *prela,
                                       PCHAR               pcTargetSec,
                                       Elf_Addr            symVal)
{
    PRISCV_HI20_RELOC_INFO  pHi20InfoArray = (PRISCV_HI20_RELOC_INFO)pmodule->EMOD_ulRiscvHi20Base;
    PRISCV_HI20_RELOC_INFO  pHi20Info;

    if (pmodule->EMOD_ulRiscvHi20Nr >= (LW_CFG_RISCV_HI20_SIZE / sizeof(RISCV_HI20_RELOC_INFO))) {
        return  (PX_ERROR);
    }

    pHi20Info = &pHi20InfoArray[pmodule->EMOD_ulRiscvHi20Nr];
    pHi20Info->HI20_rela        = *prela;
    pHi20Info->HI20_symValue    = symVal;
    pHi20Info->HI20_pcTargetSec = pcTargetSec;

    pmodule->EMOD_ulRiscvHi20Nr++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: riscvElfGetLo12RelaSymVal
** ��������: ��� LO12 ���͵��ض�λ�����ֵ
** ��  ��  : pmodule      ģ��
**           psym         Symbol ����
**           symVal       ����ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  riscvElfGetLo12RelaSymVal (LW_LD_EXEC_MODULE  *pmodule,
                                       Elf_Sym            *psym,
                                       PCHAR               pcTargetSec,
                                       Elf_Addr           *symVal)
{
    PRISCV_HI20_RELOC_INFO  pHi20Info = (PRISCV_HI20_RELOC_INFO)pmodule->EMOD_ulRiscvHi20Base;
    ULONG                   i;

    for (i = 0; i < pmodule->EMOD_ulRiscvHi20Nr; i++, pHi20Info++) {
        UINT64  ulHi20Loc = pHi20Info->HI20_rela.r_offset + (UINT64)pHi20Info->HI20_pcTargetSec;

        if (ulHi20Loc == (psym->st_value + (UINT64)pcTargetSec)) {
            SINT32  hi20;
            SINT32  lo12;
            UINT64  uiHi20SymVal = pHi20Info->HI20_symValue;

            /*
             * Calculate lo12
             */
            UINT64  offset = uiHi20SymVal - ulHi20Loc;
            if (ELF_R_TYPE(pHi20Info->HI20_rela.r_info) == R_RISCV_GOT_HI20) {
                offset = riscvElfModuleEmitGotEntry(pmodule, uiHi20SymVal);
                if (offset == 0) {
                    return  (PX_ERROR);
                }
                offset = offset - ulHi20Loc;
            }

            hi20 = (offset + 0x800) & 0xfffff000;
            lo12 = offset - hi20;

            *symVal = lo12;
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  �ض�λ��������
*********************************************************************************************************/
static RISCV_RELA_HANDLE_FUNC   _G_pfuncRiscvRelaHandleTbl[] = {
    [R_RISCV_32]            = riscvElf32Rela,
    [R_RISCV_64]            = riscvElf64Rela,
    [R_RISCV_BRANCH]        = riscvElfBranchRela,
    [R_RISCV_JAL]           = riscvElfJalRela,
    [R_RISCV_RVC_BRANCH]    = riscvElfRvcBranchRela,
    [R_RISCV_RVC_JUMP]      = riscvElfRvcJumpRela,
    [R_RISCV_PCREL_HI20]    = riscvElfPcrelHi20Rela,
    [R_RISCV_PCREL_LO12_I]  = riscvElfPcrelLo12IRela,
    [R_RISCV_PCREL_LO12_S]  = riscvElfPcrelLo12SRela,
    [R_RISCV_HI20]          = riscvElfHi20Rela,
    [R_RISCV_LO12_I]        = riscvElfLo12IRela,
    [R_RISCV_LO12_S]        = riscvElfLo12SRela,
    [R_RISCV_GOT_HI20]      = riscvElfGotHi20Rela,
    [R_RISCV_CALL_PLT]      = riscvElfCallPltRela,
    [R_RISCV_CALL]          = riscvElfCallRela,
    [R_RISCV_RELAX]         = riscvElfRelaxRela,
    [R_RISCV_ALIGN]         = riscvElfAlignRela,
    [R_RISCV_ADD32]         = riscvElfAdd32Rela,
    [R_RISCV_SUB32]         = riscvElfSub32Rela,
    [R_RISCV_JUMP_SLOT]     = riscvElfJumpSlotRela,
    [R_RISCV_RELATIVE]      = riscvElfRelativeRela,
    [R_RISCV_NONE]          = riscvElfNoneRela,
};
/*********************************************************************************************************
** ��������: archElfGotInit
** ��������: GOT �ض�λ
** �䡡��  : pmodule       ģ��ָ��
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
**           psym         ���ű���
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
    LW_LD_EXEC_MODULE      *pmod = (LW_LD_EXEC_MODULE *)pmodule;
    UINT32                 *paddrWhere;
    Elf_Addr                addrNewSymVal;
    RISCV_RELA_HANDLE_FUNC  pfuncHandle;
    UINT                    uiType;

    uiType = ELF_R_TYPE(prela->r_info);

    if (uiType < ARRAY_SIZE(_G_pfuncRiscvRelaHandleTbl)) {
        pfuncHandle = _G_pfuncRiscvRelaHandleTbl[uiType];
    } else {
        pfuncHandle = LW_NULL;
    }

    if (!pfuncHandle) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "%s: Unknown relocate type %d.\r\n",
                pmod->EMOD_pcModulePath, uiType);
        return  (PX_ERROR);
    }

    paddrWhere = (UINT32 *)((size_t)pcTargetSec + prela->r_offset);     /*  �����ض�λĿ���ַ          */

    if ((uiType == R_RISCV_PCREL_LO12_I) || (uiType == R_RISCV_PCREL_LO12_S)) {
       INT  iError  = riscvElfGetLo12RelaSymVal(pmodule, psym, pcTargetSec, &addrNewSymVal);
       if (iError) {
           return  (iError);
       }

    } else {
        addrNewSymVal = addrSymVal + prela->r_addend;

        if ((uiType == R_RISCV_PCREL_HI20) || (uiType == R_RISCV_GOT_HI20)) {
            riscvElfAddHI20RelaSymVal(pmodule, prela, pcTargetSec, addrNewSymVal);
        }
    }

    return (pfuncHandle(pmod, paddrWhere, addrNewSymVal));
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
    return  (0);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_RISCV       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
