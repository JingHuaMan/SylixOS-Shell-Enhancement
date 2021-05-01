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
** ��   ��   ��: mipsElf32.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: ʵ�� MIPS32 ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if defined(LW_CFG_CPU_ARCH_MIPS)                                       /*  MIPS ��ϵ�ṹ               */
#if LW_CFG_CPU_WORD_LENGHT == 32
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
** ��������: mipsElfHI16RelocateRel
** ��������: �ض�λ R_MIPS_HI16���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           pRelocAdrs   �ض�λ��ַ
**           addrSymVal   �ض�λ���ŵ�ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mipsElfHI16RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                                    Elf_Addr           *pRelocAdrs,
                                    Elf_Addr            addrSymVal)
{
    PMIPS_HI16_RELOC_INFO  pHi16Info;

    pHi16Info = LW_LD_SAFEMALLOC(sizeof(MIPS_HI16_RELOC_INFO));
    if (!pHi16Info) {
        return  (PX_ERROR);
    }

    pHi16Info->HI16_pAddr    	= (Elf_Addr *)pRelocAdrs;
    pHi16Info->HI16_valAddr  	= addrSymVal;
    pHi16Info->HI16_pNext    	= pmodule->EMOD_pMIPSHi16List;        	/*  ����һ�� List �ڵ�          */
    pmodule->EMOD_pMIPSHi16List = pHi16Info;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsElfFreeHI16Relocatelist
** ��������: �ض�λ R_MIPS_LO16���͵��ض�λ��
** ��  ��  : pHi16Info   HI16_RELOC_INFO������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsElfFreeHI16Relocatelist (PMIPS_HI16_RELOC_INFO  pHi16Info)
{
    PMIPS_HI16_RELOC_INFO  pNext;

    while (pHi16Info != NULL) {
        pNext = pHi16Info->HI16_pNext;
        LW_LD_SAFEFREE(pHi16Info);
        pHi16Info = pNext;
    }
}
/*********************************************************************************************************
** ��������: mipsElfLO16RelocateRel
** ��������: �ض�λ R_MIPS_LO16���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           pRelocAdrs   �ض�λ��ַ
**           addrSymVal   �ض�λ���ŵ�ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mipsElfLO16RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                                    Elf_Addr           *pRelocAdrs,
                                    Elf_Addr            addrSymVal)
{
    PMIPS_HI16_RELOC_INFO  pHi16Info;
    Elf_Addr               addrVal, addrValLO;
    Elf_Addr               addrInsnLO = *pRelocAdrs;

    addrValLO = SIGN_LOW16_VALUE(addrInsnLO);

    if (pmodule->EMOD_pMIPSHi16List != NULL) {
        pHi16Info = pmodule->EMOD_pMIPSHi16List;
        while (pHi16Info != NULL) {
            PMIPS_HI16_RELOC_INFO  pNext;
            Elf_Addr               addrInsn;

            /*
             * ��� HI16 �� LO16 ���ض�λ offset �Ƿ�һ��
             */
            if (addrSymVal != pHi16Info->HI16_valAddr) {
                mipsElfFreeHI16Relocatelist(pHi16Info);
                pmodule->EMOD_pMIPSHi16List = NULL;
                return  (PX_ERROR);
            }

            /*
             * �����ܵĵ�ַ
             */
            addrInsn  = *pHi16Info->HI16_pAddr;
            addrVal   = (LOW16_VALUE(addrInsn) << 16) + addrValLO;
            addrVal  += addrSymVal;

            /*
             * ��� BIT15 �ķ���ֵ(sign extension)
             */
            addrVal   = ((addrVal >> 16) + ((addrVal & 0x8000) != 0)) & 0xffff;

            addrInsn  = (addrInsn & ~0xffff) | addrVal;
            *pHi16Info->HI16_pAddr = addrInsn;

            pNext     = pHi16Info->HI16_pNext;
            LW_LD_SAFEFREE(pHi16Info);
            pHi16Info = pNext;
        }

        pmodule->EMOD_pMIPSHi16List = NULL;
    }

    addrVal     = addrSymVal + addrValLO;
    addrInsnLO  = (addrInsnLO & ~0xffff) | LOW16_VALUE(addrVal);
    *pRelocAdrs = addrInsnLO;

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
** ע  ��  : R_MIPS_HI16 �� R_MIPS_LO16 Ϊ�ɶԳ���, ȷ���ڴ���й©.
**           TODO: ��� ELF �ļ���, ���ܷ����ڴ�й©.
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

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_MIPS_NONE:
        break;

    case R_MIPS_32:
        *paddrWhere += (Elf_Addr)addrSymVal;
        break;

    case R_MIPS_REL32:
        mipsElfREL32RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                                paddrWhere,
                                ELF_R_SYM(prel->r_info));
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
        mipsElfHI16RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                               paddrWhere,
                               addrSymVal);
        break;

    case R_MIPS_LO16:
        mipsElfLO16RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                               paddrWhere,
                               addrSymVal);
        break;

    case R_MIPS_JUMP_SLOT:
        *paddrWhere = (Elf_Addr)addrSymVal;
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
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
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
