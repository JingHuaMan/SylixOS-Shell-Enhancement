/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ppcMmu460FTlb.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2019 �� 08 �� 14 ��
**
** ��        ��: PowerPC 460 ��ϵ���� MMU �̶� TLB ������.
**
** ע        ��: ���������κβ���ϵͳ����, �����ڲ���ϵͳ��ʼ��ǰ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmu460Reg.h"
#include "alloca.h"
/*********************************************************************************************************
  TLB ������
*********************************************************************************************************/
typedef struct {
    ULONG       FTLB_ulFlag;                                            /*  ӳ���־                    */
    TLB_WORD0   FTLB_uiWord0;                                           /*  tlb word0                   */
    TLB_WORD1   FTLB_uiWord1;                                           /*  tlb word1                   */
    TLB_WORD2   FTLB_uiWord2;                                           /*  tlb word2                   */
} TLB_WORDS;
/*********************************************************************************************************
** ��������: arch460MmuFTLBGlobalMap
** ��������: 460 MMU �̶� TLB ��Ŀȫ��ӳ��
** �䡡��  : pcMachineName          ʹ�õĻ�������
**           pdesc                  ӳ���ϵ����
**           pfuncPreRemoveTempMap  �Ƴ���ʱӳ��ǰ�Ļص�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �����ڹ��ж�����µ���
*********************************************************************************************************/
INT  arch460MmuFTLBGlobalMap (CPCHAR                pcMachineName,
                              PPPC_FTLB_MAP_DESC    pdesc,
                              VOID                (*pfuncPreRemoveTempMap)(VOID))
{
    PPC_FTLB_MAP_DESC       desc;
    TLB_WORD0               uiWord0;
    TLB_WORD1               uiWord1;
    TLB_WORD2               uiWord2;
    UINT                    i;
    size_t                  stRemainSize;
    TLB_WORDS              *tlbWords;

    if (!pdesc) {
        return  (PX_ERROR);
    }

    ppc460MmuSetPID(0);                                                 /*  ���� PID:0                  */
    ppc460MmuSetMMUCR(0);                                               /*  ���� MMUCR(STID:0, STS:0)   */

    /*
     * ��һ��: ���������ڴ���Ϣ����
     */
    tlbWords = (TLB_WORDS *)alloca(sizeof(TLB_WORDS)*PPC460_FTLB_SIZE); /*  ��ջ�����                  */
    lib_bzero(tlbWords, sizeof(TLB_WORDS) * PPC460_FTLB_SIZE);

    desc         = *pdesc;                                              /*  �ӵ�һ����ʼ����            */
    stRemainSize = desc.FTLBD_stSize;

    for (i = 0; (i < PPC460_FTLB_SIZE) && stRemainSize;) {
        if (!(desc.FTLBD_ulFlag & PPC_FTLB_FLAG_VALID)) {               /*  ��Ч��ӳ���ϵ              */
            pdesc++;
            desc         = *pdesc;
            stRemainSize = desc.FTLBD_stSize;
            continue;
        }

        /*
         * WORD0
         */
        uiWord0.WORD0_uiValue = 0;
        uiWord0.WORD0_bValid  = LW_TRUE;                                /*   TLB ��Ч                   */
        uiWord0.WORD0_bTS     = 0;                                      /*   ��ַ�ռ� 0                 */
        uiWord0.WORD0_uiEPN   = desc.FTLBD_ulVirMap >> MMU_RPN_SHIFT;

        if ((desc.FTLBD_stSize >= 1 * LW_CFG_GB_SIZE) &&
           !(desc.FTLBD_ui64PhyAddr & (1 * LW_CFG_GB_SIZE - 1))) {
            desc.FTLBD_stSize    = 1 * LW_CFG_GB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_1G;

        } else if ((desc.FTLBD_stSize >= 256 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (256 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 256 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_256M;

        } else if ((desc.FTLBD_stSize >= 16 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (16 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 16 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_16M;

        } else if ((desc.FTLBD_stSize >= 1 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (1 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 1 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_1M;

        } else if ((desc.FTLBD_stSize >= 256 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (256 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 256 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_256K;

        } else if ((desc.FTLBD_stSize >= 64 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (64 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 64 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_64K;

        } else if ((desc.FTLBD_stSize >= 16 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (16 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 16 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_16K;

        } else if ((desc.FTLBD_stSize >= 4 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (4 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 4 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_4K;

        } else {
            _BugFormat(LW_TRUE, LW_TRUE, "map size 0x%x is NOT 4K align!\r\n", pdesc->FTLBD_stSize);
        }

        /*
         * WORD1
         */
        uiWord1.WORD1_uiValue = 0;
        uiWord1.WORD1_uiRPN   = (desc.TLB1D_ui64PhyAddr >> MMU_RPN_SHIFT) & 0xfffff;
        uiWord1.WORD1_ucERPN  = desc.FTLBD_ui64PhyAddr >> 32;

        /*
         * WORD2
         */
        uiWord2.WORD2_uiValue       = 0;
        uiWord2.WORD2_bLittleEndian = LW_FALSE;                         /*  ���                        */
        uiWord2.WORD2_bMemCoh       = LW_FALSE;                         /*  û�ж��, ����Ҫһ����      */

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_CACHEABLE) {              /*  ��д CACHE                  */
            uiWord2.WORD2_bUnCache = LW_FALSE;
            uiWord2.WORD2_bWT      = LW_FALSE;
            uiWord2.WORD2_bGuarded = LW_FALSE;

        } else if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_WRITETHROUGH) {    /*  д��͸ CACHE                */
            uiWord2.WORD2_bUnCache = LW_FALSE;
            uiWord2.WORD2_bWT      = LW_TRUE;
            uiWord2.WORD2_bGuarded = LW_FALSE;

        } else {                                                        /*  ������ CACHE                */
            uiWord2.WORD2_bUnCache = LW_TRUE;
            uiWord2.WORD2_bWT      = LW_TRUE;
            uiWord2.WORD2_bGuarded = LW_TRUE;
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_ACCESS) {
            uiWord2.WORD2_bSuperRead = LW_TRUE;                         /*  ��Ȩ̬�ɶ�                  */
            uiWord2.WORD2_bUserRead  = LW_FALSE;                        /*  �û�̬���ɶ�                */
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_WRITABLE) {
            uiWord2.WORD2_bSuperWrite = LW_TRUE;                        /*  ��Ȩ̬��д                  */
            uiWord2.WORD2_bUserWrite  = LW_FALSE;                       /*  �û�̬����д                */
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_EXECABLE) {
            uiWord2.WORD2_bSuperExec = LW_TRUE;                         /*  ��Ȩ̬��ִ��                */
            uiWord2.WORD2_bUserExec  = LW_FALSE;                        /*  �û�̬����ִ��              */
        }

        /*
         * �����������¼�� tlbWords ����
         */
        tlbWords[i].FTLB_ulFlag  = desc.FTLBD_ulFlag;
        tlbWords[i].FTLB_uiWord0 = uiWord0;
        tlbWords[i].FTLB_uiWord1 = uiWord1;
        tlbWords[i].FTLB_uiWord2 = uiWord2;

        i++;

        stRemainSize = stRemainSize - desc.FTLBD_stSize;
        if (stRemainSize > 0) {                                         /*  ��δ"ӳ��"�Ĳ���            */
            desc.FTLBD_ui64PhyAddr += desc.FTLBD_stSize;                /*  ����"ӳ��"ʣ��Ĳ���        */
            desc.FTLBD_ulVirMap    += desc.FTLBD_stSize;
            desc.FTLBD_stSize       = stRemainSize;

        } else {
            pdesc++;                                                    /*  "ӳ��"��һ��                */
            desc         = *pdesc;
            stRemainSize = desc.FTLBD_stSize;
        }
    }

    _BugHandle(i > PPC460_FTLB_SIZE, LW_TRUE, "to many map desc!\r\n");

    /*
     * �ڶ���: �� tlbWords �����¼��ֵ������������ӳ��
     */
    for (i = 0; i < PPC460_FTLB_SIZE; i++) {
        __asm__ __volatile__(
            "isync\n"
            "tlbwe  %2, %3, 2\n"
            "tlbwe  %1, %3, 1\n"
            "tlbwe  %0, %3, 0\n"
            "isync\n"
            :
            : "r" (tlbWords[i].FTLB_uiWord0.WORD0_uiValue),
              "r" (tlbWords[i].FTLB_uiWord1.WORD1_uiValue),
              "r" (tlbWords[i].FTLB_uiWord2.WORD2_uiValue),
              "r" (i));
    }

    if (pfuncPreRemoveTempMap) {
        pfuncPreRemoveTempMap();
    }

    /*
     * ������: ɾ����ʱӳ��
     */
    for (i = 0; i < PPC460_FTLB_SIZE; i++) {
        if (tlbWords[i].FTLB_ulFlag & PPC_FTLB_FLAG_TEMP) {
            __asm__ __volatile__(
                "sync\n"
                "tlbwe  %0, %0, 0\n"
                "isync\n"
                :
                : "r" (i));
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
