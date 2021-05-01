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
** ��   ��   ��: cskyElf.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: ʵ�� C-SKY ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef LW_CFG_CPU_ARCH_CSKY                                             /*  C-SKY ��ϵ�ṹ              */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  C-SKY �ض�λ����
*********************************************************************************************************/
#define R_CKCORE_NONE                  0
#define R_CKCORE_ADDR32                1
#define R_CKCORE_PCRELIMM8BY4          2
#define R_CKCORE_PCRELIMM11BY2         3
#define R_CKCORE_PCRELIMM4BY2          4
#define R_CKCORE_PCREL32               5
#define R_CKCORE_PCRELJSR_IMM11BY2     6
#define R_CKCORE_GNU_VTINHERIT         7
#define R_CKCORE_GNU_VTENTRY           8
#define R_CKCORE_RELATIVE              9
#define R_CKCORE_COPY                  10
#define R_CKCORE_GLOB_DAT              11
#define R_CKCORE_JUMP_SLOT             12
#define R_CKCORE_GOTOFF                13
#define R_CKCORE_GOTPC                 14
#define R_CKCORE_GOT32                 15
#define R_CKCORE_PLT32                 16
#define R_CKCORE_ADDRGOT               17
#define R_CKCORE_ADDRPLT               18
#define R_CKCORE_PCREL_IMM26BY2        19
#define R_CKCORE_PCREL_IMM16BY2        20
#define R_CKCORE_PCREL_IMM16BY4        21
#define R_CKCORE_PCREL_IMM10BY2        22
#define R_CKCORE_PCREL_IMM10BY4        23
#define R_CKCORE_ADDR_HI16             24
#define R_CKCORE_ADDR_LO16             25
#define R_CKCORE_GOTPC_HI16            26
#define R_CKCORE_GOTPC_LO16            27
#define R_CKCORE_GOTOFF_HI16           28
#define R_CKCORE_GOTOFF_LO16           29
#define R_CKCORE_GOT12                 30
#define R_CKCORE_GOT_HI16              31
#define R_CKCORE_GOT_LO16              32
#define R_CKCORE_PLT12                 33
#define R_CKCORE_PLT_HI16              34
#define R_CKCORE_PLT_LO16              35
#define R_CKCORE_ADDRGOT_HI16          36
#define R_CKCORE_ADDRGOT_LO16          37
#define R_CKCORE_ADDRPLT_HI16          38
#define R_CKCORE_ADDRPLT_LO16          39
#define R_CKCORE_PCREL_JSR_IMM26BY2    40
#define R_CKCORE_TOFFSET_LO16          41
#define R_CKCORE_DOFFSET_LO16          42
#define R_CKCORE_PCREL_IMM18BY2        43
#define R_CKCORE_DOFFSET_IMM18         44
#define R_CKCORE_DOFFSET_IMM18BY2      45
#define R_CKCORE_DOFFSET_IMM18BY4      46
#define R_CKCORE_GOTOFF_IMM18          47
#define R_CKCORE_GOT_IMM18BY4          48
#define R_CKCORE_PLT_IMM18BY4          49
#define R_CKCORE_PCREL_IMM7BY4         50
#define R_CKCORE_TLS_LE32              51
#define R_CKCORE_TLS_IE32              52
#define R_CKCORE_TLS_GD32              53
#define R_CKCORE_TLS_LDM32             54
#define R_CKCORE_TLS_LDO32             55
#define R_CKCORE_TLS_DTPMOD32          56
#define R_CKCORE_TLS_DTPOFF32          57
#define R_CKCORE_TLS_TPOFF32           58
/*********************************************************************************************************
  BSR/JSRI
*********************************************************************************************************/
#define IS_BSR32(hi16, lo16)           (((hi16) & 0xfc00) == 0xe000)
#define IS_JSRI32(hi16, lo16)          ((hi16) == 0xeae0)
#define CHANGE_JSRI_TO_LRW(addr)       *(UINT16 *)(addr) = (*(UINT16 *)(addr) & 0xff9f) | 0x001a; \
                                       *((UINT16 *)(addr) + 1) = *((UINT16 *)(addr) + 1) & 0xffff
#define SET_JSR32_R26(addr)            *(UINT16 *)(addr) = 0xe8fa; \
                                       *((UINT16 *)(addr) + 1) = 0x0000;
/*********************************************************************************************************
** ��������: archElfRelocateRela
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
    Elf_Addr   *paddrWhere;
    Elf_Addr    addrValue;
    UINT32      uiInsnOpCode;
    UINT16     *pusOpCode16Addr;
    INT16      *addrTmp;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  �����ض�λĿ���ַ          */
    addrValue  = addrSymVal + prela->r_addend;

    pusOpCode16Addr = (UINT16 *)paddrWhere;

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_CKCORE_NONE:
        break;

    case R_CKCORE_ADDR32:
        *paddrWhere = addrValue;
        break;

    case R_CKCORE_GLOB_DAT:
    case R_CKCORE_JUMP_SLOT:
        *paddrWhere = addrSymVal;
        break;

    case R_CKCORE_RELATIVE:
        *paddrWhere = (size_t)pcTargetSec + prela->r_addend;
        break;

    case R_CKCORE_PCREL_IMM26BY2:
    {
        INT  iOffSet = ((INT)(addrValue - (INT)paddrWhere) >> 1);

        if ((iOffSet > 0x3ffffff) || (iOffSet < -0x3ffffff)) {
            _DebugFormat(__ERRORMESSAGE_LEVEL,
                         "The reloc R_CKCORE_PCREL_IMM26BY2 cannot reach the symbol.\r\n");
            return  (PX_ERROR);
        }

        uiInsnOpCode         = (*pusOpCode16Addr << 16) | (*(pusOpCode16Addr + 1));
        uiInsnOpCode         = (uiInsnOpCode & ~0x3ffffff) | (iOffSet & 0x3ffffff);
        *(pusOpCode16Addr++) = (UINT16)(uiInsnOpCode >> 16);
        *pusOpCode16Addr     = (UINT16)(uiInsnOpCode & 0xffff);
        break;
    }

    case R_CKCORE_PCREL32:
        *paddrWhere = addrValue - (Elf_Addr)paddrWhere;
        break;

    case R_CKCORE_PCRELJSR_IMM11BY2:
        break;

    case R_CKCORE_PCREL_JSR_IMM26BY2:
#if defined(__CSKYABIV2__) && defined(__CK810__)
        if (IS_BSR32(*pusOpCode16Addr, *(pusOpCode16Addr + 1))) {
            break;

        } else if (IS_JSRI32(*pusOpCode16Addr, *(pusOpCode16Addr + 1))) {
            CHANGE_JSRI_TO_LRW(paddrWhere);                             /*  jsri 0x... --> lrw r26,0x...*/
            SET_JSR32_R26(paddrWhere + 1);                              /*  lsli r0, r0 --> jsr r26     */
        }
#endif                                                                  /*  __CSKYABIV2__ && __CK810__  */
        break;

    case R_CKCORE_ADDR_HI16:
        addrTmp  = ((INT16 *)paddrWhere) + 1;
        *addrTmp = (INT16)(addrValue >> 16);
        break;

    case R_CKCORE_ADDR_LO16:
        addrTmp  = ((INT16 *)paddrWhere) + 1;
        *addrTmp = (INT16)(addrValue & 0xffff);
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
    INT16     *addrTmp;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_CKCORE_NONE:
    case R_CKCORE_PCRELJSR_IMM11BY2:
    case R_CKCORE_PCREL_JSR_IMM26BY2:
        break;

    case R_CKCORE_ADDR32:
        *paddrWhere += (Elf_Addr)addrSymVal;
        break;

    case R_CKCORE_PCREL32:
        *paddrWhere += (Elf_Addr)addrSymVal - (Elf_Addr)paddrWhere;
        break;

    case R_CKCORE_ADDR_HI16:
        addrTmp  = ((INT16 *)paddrWhere) + 1;
        *addrTmp = (INT16)(addrSymVal >> 16);
        break;

    case R_CKCORE_ADDR_LO16:
        addrTmp  = ((INT16 *)paddrWhere) + 1;
        *addrTmp = (INT16)(addrSymVal & 0xffff);
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
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
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
