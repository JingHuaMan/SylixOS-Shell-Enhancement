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
** ��   ��   ��: armElf.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ʵ�� ARM ��ϵ�ṹ�� ELF �ļ��ض�λ.
**
** BUG:
2014.12.08  ���thumb2�ض�λ֧��.
2015.01.06  ��Ŀ��Ϊthumb��BLת��ΪBLX.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_ARM                                            /*  ARM ��ϵ�ṹ                */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  ARM Relocation types
*********************************************************************************************************/
#define R_ARM_NONE              0
#define R_ARM_PC24              1
#define R_ARM_ABS32             2
#define R_ARM_REL32             3                                       /* PC relative 32 bit           */
#define R_ARM_PC13              4
#define R_ARM_ABS16             5                                       /* Direct 16 bit                */
#define R_ARM_ABS12             6                                       /* Direct 12 bit                */
#define R_ARM_THM_ABS5          7
#define R_ARM_ABS8              8                                       /* Direct 8 bit                 */
#define R_ARM_SBREL32           9
#define R_ARM_THM_PC22          10
#define R_ARM_THM_PC8           11
#define R_ARM_AMP_VCALL9        12
#define R_ARM_SWI24             13
#define R_ARM_THM_SWI8          14
#define R_ARM_XPC25             15
#define R_ARM_THM_XPC22         16
#define R_ARM_COPY              20                                      /* Copy symbol at runtime       */
#define R_ARM_GLOB_DAT          21                                      /* Create GOT entry             */
#define R_ARM_JUMP_SLOT         22                                      /* Create PLT entry             */
#define R_ARM_RELATIVE          23                                      /* Adjust by program base       */
#define R_ARM_GOTOFF            24                                      /* 32 bit offset to GOT         */
#define R_ARM_GOTPC             25                                      /* 32 bit PC relative offset to */
                                                                        /* GOT                          */
#define R_ARM_GOT32             26                                      /* 32 bit GOT entry             */
#define R_ARM_PLT32             27                                      /* 32 bit PLT address           */
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_THM_JUMP24        30
#define R_ARM_TARGET1           38
#define R_ARM_THM_MOVW_ABS_NC   47
#define R_ARM_THM_MOVT_ABS      48

#define R_ARM_V4BX              40
#define R_ARM_PREL31            42
#define R_ARM_MOVW_ABS_NC       43
#define R_ARM_MOVT_ABS          44

#define R_ARM_GNU_VTENTRY       100
#define R_ARM_GNU_VTINHERIT     101
#define R_ARM_THM_PC11          102                                     /* thumb unconditional branch   */
#define R_ARM_THM_PC9           103                                     /* thumb conditional branch     */
#define R_ARM_RXPC25            249
#define R_ARM_RSBREL32          250
#define R_ARM_THM_RPC22         251
#define R_ARM_RREL32            252
#define R_ARM_RABS22            253
#define R_ARM_RPC24             254
#define R_ARM_RBASE             255

#define R_ARM_NUM               256                                     /* Keep this the last entry.    */
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define JMP_TABLE_ITEMLEN       8                                       /*  ��ת����Ŀ����              */
/*********************************************************************************************************
  ����ָ�����Ի����ֳ��ȵ�������ʾ������Ҫ�����С�����⣬��Ϊ�����ĸ��ֽ�Ҳ��ָ��ĸ��ֽ�
*********************************************************************************************************/
#define LDR_INSTRUCTION         0xe51ff004                              /*  ��תָ��                    */
/*********************************************************************************************************
  ��ת�������Ͷ���
*********************************************************************************************************/
typedef struct long_jmp_item {
    ULONG          ulJmpInstruction;                                    /*  ��תָ��                    */
    Elf_Addr       addrJmpDest;                                         /*  Ŀ���ַ                    */
} LONG_JMP_ITEM;
/*********************************************************************************************************
** ��������: jmpItemFind
** ��������: ������ת������û�У����½�һ��ת����
** �䡡��  : addrSymVal    Ҫ���ҵķ��ŵ�ֵ
**           pcBuffer      ��ת����ʼ��ַ
**           stBuffLen     ��ת����
** �䡡��  : ������ת������תָ��ĵ�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static Elf_Addr jmpItemFind (Elf_Addr addrSymVal,
                             PCHAR    pcBuffer,
                             size_t   stBuffLen)
{
    LONG_JMP_ITEM *pJmpTable    = (LONG_JMP_ITEM *)pcBuffer;
    ULONG          ulJmpItemCnt = stBuffLen / 8;
    ULONG          i;

    for (i = 0; i < ulJmpItemCnt; i++) {
        if (pJmpTable[i].ulJmpInstruction != LDR_INSTRUCTION) {         /*  �����±���                  */
            pJmpTable[i].ulJmpInstruction = LDR_INSTRUCTION;
            pJmpTable[i].addrJmpDest = addrSymVal;
            LD_DEBUG_MSG(("long jump item: %lx %lx\r\n",
                          (ULONG)&pJmpTable[i],
                          pJmpTable[i].addrJmpDest));
        }

        if (pJmpTable[i].addrJmpDest == addrSymVal) {
            break;
        }
    }

    if (i >= ulJmpItemCnt) {
        return  (0);
    }

    return  (Elf_Addr)(&pJmpTable[i]);
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
** ��������: archElfRelocateRela
** ��������: �ض�λ RELA ���͵��ض�λ��
** ��  ��  : pmodule      ģ��
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
    INT32      addrTmp;
    Elf_Sword  swordAddend;
    Elf_Sword  swordTopBits;
    Elf_Addr   upper, lower, sign, j1, j2, H;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_ARM_NONE:
        break;

    case R_ARM_TARGET1:
    case R_ARM_ABS32:                                                   /*  ���Ե�ַ�ض�λ (A+S)        */
        *paddrWhere += (Elf_Addr)addrSymVal;
        LD_DEBUG_MSG(("R_ARM_ABS32: %lx -> %lx\r\n", (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_ARM_JUMP_SLOT:                                               /*  PLT                         */
    case R_ARM_GLOB_DAT:
        LD_DEBUG_MSG(("R_ARM_ABS32: %lx -> %lx\r\n", (ULONG)paddrWhere, addrSymVal));
        *paddrWhere = (Elf_Addr)addrSymVal;
        break;

    case R_ARM_RELATIVE:
        LD_DEBUG_MSG(("R_ARM_RELATIVE: %lx -> %lx\r\n", (ULONG)paddrWhere, addrSymVal));
        *paddrWhere += (Elf_Addr)pcTargetSec;
        break;

    case R_ARM_PC24:                                                    /*  ��Ե�ַ�ض�λ              */
    case R_ARM_PLT32:
    case R_ARM_CALL:
    case R_ARM_JUMP24:
        LD_DEBUG_MSG(("R_ARM_PC24: %lx -> %lx\r\n", (ULONG)paddrWhere, addrSymVal));
        swordAddend = *paddrWhere & 0x00ffffff;
        if (swordAddend & 0x00800000)
            swordAddend |= 0xff000000;
        addrTmp = addrSymVal - (Elf_Addr)paddrWhere + (swordAddend << 2);

        swordTopBits = addrTmp & 0xfe000000;
        if (swordTopBits != 0xfe000000 && swordTopBits != 0x00000000) {  /*  ������Ե�ַ��ת��Χ       */
            addrSymVal = jmpItemFind(addrSymVal, pcBuffer, stBuffLen);
            if (0 == addrSymVal) {
                return  (PX_ERROR);
            }
            addrTmp = addrSymVal - (Elf_Addr)paddrWhere + (swordAddend << 2);
        }

        if ((*paddrWhere & 0x0F000000) == 0x0B000000 &&
            (addrTmp & 0x3) != 0) {                                     /*  ��Ŀ��Ϊthumb��BLת��ΪBLX  */
            H = ((addrTmp >> 1) & 1) << 24;
            addrTmp >>= 2;
            *paddrWhere = (0xFA << 24) | H | (addrTmp & 0x00ffffff);
        } else {
            addrTmp >>= 2;
            *paddrWhere = (*paddrWhere & 0xff000000) | (addrTmp & 0x00ffffff);
        }
        break;

    case R_ARM_REL32:
        *paddrWhere += (Elf_Addr)addrSymVal - (Elf_Addr)paddrWhere;     /*  prel->r_offset              */
        break;

    case R_ARM_V4BX:
        *paddrWhere &= 0xf000000f;
        *paddrWhere |= 0x01a0f000;
        break;

    case R_ARM_PREL31:
        *paddrWhere = (*paddrWhere) + addrSymVal - (Elf_Addr)paddrWhere;
        *paddrWhere &= 0x7fffffff;
        break;

    case R_ARM_MOVW_ABS_NC:
    case R_ARM_MOVT_ABS:
        addrTmp = *paddrWhere;
        addrTmp = ((addrTmp & 0xf0000) >> 4) | (addrTmp & 0xfff);
        addrTmp = (addrTmp ^ 0x8000) - 0x8000;
        addrTmp += addrSymVal;
        if (ELF32_R_TYPE(prel->r_info) == R_ARM_MOVT_ABS) {
            addrTmp >>= 16;
        }

        *paddrWhere &= 0xfff0f000;
        *paddrWhere |= ((addrTmp & 0xf000) << 4) |
                       (addrTmp & 0x0fff);
        break;

    case R_ARM_THM_PC22:
    case R_ARM_THM_JUMP24:
        upper = *(UINT16 *)paddrWhere;
        lower = *(UINT16 *)(paddrWhere + 2);

        /*
         *  25 bit signed address range (Thumb-2 BL and B.W
         *  instructions):
         *   S:I1:I2:imm10:imm11:0
         *  where:
         *   S     = upper[10]   = offset[24]
         *   I1    = ~(J1 ^ S)   = offset[23]
         *   I2    = ~(J2 ^ S)   = offset[22]
         *   imm10 = upper[9:0]  = offset[21:12]
         *   imm11 = lower[10:0] = offset[11:1]
         *   J1    = lower[13]
         *   J2    = lower[11]
         */
        sign = (upper >> 10) & 1;
        j1 = (lower >> 13) & 1;
        j2 = (lower >> 11) & 1;
        addrTmp = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
        ((~(j2 ^ sign) & 1) << 22) |
        ((upper & 0x03ff) << 12) |
        ((lower & 0x07ff) << 1);
        if (addrTmp & 0x01000000) {
            addrTmp -= 0x02000000;
        }
        addrTmp += addrSymVal - (Elf_Addr)paddrWhere;

        /* 
         *  only Thumb addresses allowed (no interworking)
         */
        if (!(addrTmp & 1) ||
            addrTmp <= (INT32)0xff000000 ||
            addrTmp >= (INT32)0x01000000) {
            return (PX_ERROR);
        }

        sign = (addrTmp >> 24) & 1;
        j1 = sign ^ (~(addrTmp >> 23) & 1);
        j2 = sign ^ (~(addrTmp >> 22) & 1);
        upper = (UINT16)((upper & 0xf800) | (sign << 10) |
                    ((addrTmp >> 12) & 0x03ff));
        lower = (UINT16)((lower & 0xd000) |
                  (j1 << 13) | (j2 << 11) |
                  ((addrTmp >> 1) & 0x07ff));

        *(UINT16 *)paddrWhere = upper;
        *(UINT16 *)(paddrWhere + 2) = lower;
        break;

    case R_ARM_THM_MOVW_ABS_NC:
    case R_ARM_THM_MOVT_ABS:
        upper = *(UINT16 *)paddrWhere;
        lower = *(UINT16 *)(paddrWhere + 2);

        /*
         * MOVT/MOVW instructions encoding in Thumb-2:
         *
         * i    = upper[10]
         * imm4 = upper[3:0]
         * imm3 = lower[14:12]
         * imm8 = lower[7:0]
         *
         * imm16 = imm4:i:imm3:imm8
         */
        addrTmp = ((upper & 0x000f) << 12) |
            ((upper & 0x0400) << 1) |
            ((lower & 0x7000) >> 4) | (lower & 0x00ff);
        addrTmp = (addrTmp ^ 0x8000) - 0x8000;
        addrTmp += addrSymVal;

        if (ELF32_R_TYPE(prel->r_info) == R_ARM_THM_MOVT_ABS)
            addrTmp >>= 16;

        upper = (UINT16)((upper & 0xfbf0) |
                  ((addrTmp & 0xf000) >> 12) |
                  ((addrTmp & 0x0800) >> 1));
        lower = (UINT16)((lower & 0x8f00) |
                  ((addrTmp & 0x0700) << 4) |
                  (addrTmp & 0x00ff));
        *(UINT16 *)paddrWhere = upper;
        *(UINT16 *)(paddrWhere + 2) = lower;
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
    return  (JMP_TABLE_ITEMLEN);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
