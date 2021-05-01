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
** ��   ��   ��: c6xElf.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2017 �� 07 �� 24 ��
**
** ��        ��: ʵ�� c6x ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
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
#define R_C6000_NONE            0
#define R_C6000_ABS32           1
#define R_C6000_ABS16           2
#define R_C6000_ABS8            3
#define R_C6000_PCR_S21         4
#define R_C6000_PCR_S12         5
#define R_C6000_PCR_S10         6
#define R_C6000_PCR_S7          7
#define R_C6000_ABS_S16         8
#define R_C6000_ABS_L16         9
#define R_C6000_ABS_H16         10
#define R_C6000_SBR_U15_B       11
#define R_C6000_SBR_U15_H       12
#define R_C6000_SBR_U15_W       13
#define R_C6000_SBR_S16         14
#define R_C6000_SBR_L16_B       15
#define R_C6000_SBR_L16_H       16
#define R_C6000_SBR_L16_W       17
#define R_C6000_SBR_H16_B       18
#define R_C6000_SBR_H16_H       19
#define R_C6000_SBR_H16_W       20
#define R_C6000_SBR_GOT_U15_W   21
#define R_C6000_SBR_GOT_L16_W   22
#define R_C6000_SBR_GOT_H16_W   23
#define R_C6000_DSBT_INDEX      24
#define R_C6000_PREL31          25
#define R_C6000_COPY            26
#define R_C6000_JUMP_SLOT       27
#define R_C6000_SBR_GOT32       28
#define R_C6000_PCR_H16         29
#define R_C6000_PCR_L16         30
#define R_C6000_ALIGN           253
#define R_C6000_FPHEAD          254
#define R_C6000_NOCMP           255
/*********************************************************************************************************
  ����ָ�����Ի����ֳ��ȵ�������ʾ������Ҫ�����С�����⣬��Ϊ�����ĸ��ֽ�Ҳ��ָ��ĸ��ֽ�
*********************************************************************************************************/
#define C6X_INSTRUCTION_MVKL    0x0051d1aa                              /*  MVKL ָ��                   */
#define C6X_INSTRUCTION_MVKH    0X0051d1ea                              /*  MVKH ָ��                   */
#define C6X_INSTRUCTION_NOP     0x0080a362                              /*  NOP  ָ��                   */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
  ��ת�������Ͷ���
*********************************************************************************************************/
typedef struct {
    ULONG          ulInstMVKL;                                          /*  MVKL ָ��                   */
    ULONG          ulInstMVKH;                                          /*  MVKH ָ��                   */
    ULONG          ulInstNOP;                                           /*  NOP  ָ��                   */
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
    ULONG          ulJmpItemCnt = stBuffLen / sizeof(LONG_JMP_ITEM);
    ULONG          i;

    for (i = 0; i < ulJmpItemCnt; i++) {
        if (pJmpTable[i].ulInstNOP != C6X_INSTRUCTION_NOP) {            /*  �����±���                  */
            pJmpTable[i].ulInstMVKL = (C6X_INSTRUCTION_MVKL & ~0x007fff80) | ((addrSymVal & 0xffff) << 7);
            pJmpTable[i].ulInstMVKH = (C6X_INSTRUCTION_MVKH & ~0x007fff80) | ((addrSymVal >> 9) & 0x007fff80);
            pJmpTable[i].ulInstNOP  = C6X_INSTRUCTION_NOP;
            LD_DEBUG_MSG(("long jump item: %lx %lx\r\n",
                          (ULONG)&pJmpTable[i],
                          pJmpTable[i].addrJmpDest));
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
** ��������: �� DSP �У���������Ҫ���ڳ�ʼ�� DSBT ��
**           DSP ʹ�� DSBT ʵ�ֽ��̼����ι���������ÿ����������һ�����ݶεĿ�������������ϵͳ����ģ��
**           �� DSBT index �������ظ���
** ��  ��  : pmodule       ģ��
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfGotInit (PVOID  pmodule)
{
    LW_LD_EXEC_MODULE   *pmod  = (LW_LD_EXEC_MODULE *)pmodule;

    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;

    BOOL                bStart;
    ULONG               i;

    if (pmod->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {
        pmod->EMOD_ulDsbtIndex = 0;
        return  (ERROR_NONE);
    }

    for (i = 0; i < pmod->EMOD_ulDsbtSize; i++) {
        if (__TI_STATIC_BASE[i] == 0) {
            __TI_STATIC_BASE[i] = (ULONG)pmod->EMOD_pulDsbtTable;
            pmod->EMOD_ulDsbtIndex = i;
            break;
        }
    }

    if (i >= pmod->EMOD_ulDsbtSize) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "DSBT index overflow\r\n");
        return  (PX_ERROR);
    }

    lib_memcpy(pmod->EMOD_pulDsbtTable,
               __TI_STATIC_BASE,
               (pmod->EMOD_ulDsbtSize * sizeof(ULONG)));

    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            if (pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {
                continue;
            }

            pmodTemp->EMOD_pulDsbtTable[pmod->EMOD_ulDsbtIndex] = (ULONG)pmod->EMOD_pulDsbtTable;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfDSBTRemove
** ��������: ɾ��DSBT����
** ��  ��  : pmodule       ģ��
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfDSBTRemove (PVOID  pmodule)
{
    LW_LD_EXEC_MODULE   *pmod = (LW_LD_EXEC_MODULE *)pmodule;

    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    BOOL                bStart;

    if (pmod->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {
        return  (ERROR_NONE);
    }

    if (pmod->EMOD_ulDsbtIndex == 0) {
        return  (ERROR_NONE);
    }

    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            if (pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {
                continue;
            }

            pmodTemp->EMOD_pulDsbtTable[pmod->EMOD_ulDsbtIndex] = 0;
        }
    }

    __TI_STATIC_BASE[pmod->EMOD_ulDsbtIndex] = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfRelocateRela
** ��������: �ض�λ RELA ���͵��ض�λ��
** ��  ��  : pmodule      ģ��
**           prela        RELA ����
**           psym         ���ű���
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
    Elf_Addr   addrNewVal = 0;
    Elf_Addr   addrOldVal = addrSymVal;

    LW_LD_EXEC_MODULE *pmod  = (LW_LD_EXEC_MODULE *)pmodule;

    paddrWhere  = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);  /*  �����ض�λĿ���ַ          */
    addrSymVal += prela->r_addend;

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_C6000_NONE:
        break;

    case R_C6000_ABS32:
    case R_C6000_JUMP_SLOT:
        *paddrWhere = (Elf_Addr)addrSymVal;
        LD_DEBUG_MSG(("R_C6000_ABS32/R_C6000_JUMP_SLOT: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_DSBT_INDEX:
        *paddrWhere = ((*paddrWhere) & ~0x007fff00) | ((pmod->EMOD_ulDsbtIndex & 0x7fff) << 8);
        LD_DEBUG_MSG(("R_C6000_DSBT_INDEX: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_ABS_L16:
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | ((addrSymVal & 0xffff) << 7);
        LD_DEBUG_MSG(("R_C6000_ABS_L16: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_ABS_H16:
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | ((addrSymVal >> 9) & 0x007fff80);
        LD_DEBUG_MSG(("R_C6000_ABS_H16: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_L16:
        addrNewVal = addrOldVal - (((ULONG)paddrWhere - prela->r_addend) & 0xFFFFFFE0);
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | ((addrNewVal & 0xffff) << 7);
        LD_DEBUG_MSG(("R_C6000_PCR_L16: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_H16:
        addrNewVal = addrOldVal - (((ULONG)paddrWhere - prela->r_addend) & 0xFFFFFFE0);
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | ((addrNewVal >> 9) & 0x007fff80);
        LD_DEBUG_MSG(("R_C6000_PCR_H16: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_SBR_L16_W:
        addrNewVal = addrSymVal - (Elf_Addr)__TI_STATIC_BASE;
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | (((addrNewVal >> 2) & 0xffff) << 7);
        LD_DEBUG_MSG(("R_C6000_SBR_L16_W: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_SBR_H16_W:
        addrNewVal = addrSymVal - (Elf_Addr)__TI_STATIC_BASE;
        *paddrWhere = ((*paddrWhere) & ~0x007fff80) | ((addrNewVal >> 11) & 0x007fff80);
        LD_DEBUG_MSG(("R_C6000_SBR_H16_W: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_S21:
        addrNewVal = addrSymVal - (((ULONG)paddrWhere) & ~31);
        *paddrWhere = ((*paddrWhere) & ~0x0fffff80) | (((addrNewVal >> 2) & 0x1fffff) << 7);
        LD_DEBUG_MSG(("R_C6000_PCR_S21: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_COPY:
        if (addrSymVal) {
            lib_memcpy((char *)paddrWhere, (char *)addrSymVal, psym->st_size);
        }
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfFixupPcr
** ��������: ָ�����
** ��  ��  : pui32Ip      ָ���ַ
**           addrDest     Ŀ���ַ
**           ui32MaskBits ����
**           iShift       λ����
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE INT  archElfFixupPcr (Elf32_Addr  *pui32Ip,
                                       Elf32_Addr   addrDest,
                                       UINT32       ui32MaskBits,
                                       INT          iShift)
{
    UINT32      uiOpcode;
    LONG        lEp    = (LONG)pui32Ip & ~31;
    LONG        lDelta = ((LONG)addrDest - lEp) >> 2;
    LONG        lMask  = (1 << ui32MaskBits) - 1;

    if ((lDelta >> (ui32MaskBits - 1)) == 0 ||
        (lDelta >> (ui32MaskBits - 1)) == -1) {
        uiOpcode  = *pui32Ip;
        uiOpcode &= ~(lMask << iShift);
        uiOpcode |= ((lDelta & lMask) << iShift);
        *pui32Ip  = uiOpcode;

        return  (ERROR_NONE);
    }

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

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_C6000_DSBT_INDEX:
        *paddrWhere = ((*paddrWhere) & ~0x007fff00) | ((0 & 0x7fff) << 8);
        LD_DEBUG_MSG(("R_C6000_DSBT_INDEX: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_ABS32:
        *paddrWhere = (Elf_Addr)addrSymVal;
        LD_DEBUG_MSG(("R_C6000_ABS32: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_ABS16:
        *(Elf32_Half *)paddrWhere = (Elf32_Half)addrSymVal;
        LD_DEBUG_MSG(("R_C6000_ABS16: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_ABS8:
        *(UCHAR *)paddrWhere = (UCHAR)addrSymVal;
        LD_DEBUG_MSG(("R_C6000_ABS8: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_S21:
        if (archElfFixupPcr(paddrWhere, addrSymVal, 21, 7) != ERROR_NONE) {
            addrSymVal = jmpItemFind(addrSymVal, pcBuffer, stBuffLen);
            if (archElfFixupPcr(paddrWhere, addrSymVal, 21, 7) != ERROR_NONE) {
                return  (PX_ERROR);
            }
        }
        LD_DEBUG_MSG(("R_C6000_PCR_S21: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_S12:
        if (archElfFixupPcr(paddrWhere, addrSymVal, 12, 16) != ERROR_NONE) {
            addrSymVal = jmpItemFind(addrSymVal, pcBuffer, stBuffLen);
            if (archElfFixupPcr(paddrWhere, addrSymVal, 12, 16) != ERROR_NONE) {
                LD_DEBUG_MSG(("R_C6000_PCR_S10 failed: %lx -> %lx\r\n",
                             (ULONG)paddrWhere, *paddrWhere));
                return  (PX_ERROR);
            }
        }
        LD_DEBUG_MSG(("R_C6000_PCR_S12: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
        break;

    case R_C6000_PCR_S10:
        if (archElfFixupPcr(paddrWhere, addrSymVal, 10, 13) != ERROR_NONE) {
            addrSymVal = jmpItemFind(addrSymVal, pcBuffer, stBuffLen);
            if (archElfFixupPcr(paddrWhere, addrSymVal, 10, 13) != ERROR_NONE) {
                LD_DEBUG_MSG(("R_C6000_PCR_S10 failed: %lx -> %lx\r\n",
                             (ULONG)paddrWhere, *paddrWhere));
                return  (PX_ERROR);
            }
        }
        LD_DEBUG_MSG(("R_C6000_PCR_S10: %lx -> %lx\r\n",
                     (ULONG)paddrWhere, *paddrWhere));
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
    return  (sizeof(LONG_JMP_ITEM));
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
