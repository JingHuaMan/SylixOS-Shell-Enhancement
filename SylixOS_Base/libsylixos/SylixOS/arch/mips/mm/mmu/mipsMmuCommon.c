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
** ��   ��   ��: mipsMmuCommon.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 10 �� 12 ��
**
** ��        ��: MIPS ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "mipsMmuCommon.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/inc/addrspace.h"
#include "mips32/mips32Mmu.h"
#include "mips64/mips64Mmu.h"
/*********************************************************************************************************
  UNIQUE ENTRYHI(�� 512KB ����, ����������, TLB ��Ŀ��� 256 ��, �������絽 CKSEG1, CKSEG0 �ռ��� 512MB)
*********************************************************************************************************/
#define MIPS_UNIQUE_ENTRYHI(idx)        (CKSEG0 + ((idx) << (18 + 1)))
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
BOOL         _G_bMmuHasXI             = LW_FALSE;                       /*  �Ƿ��� XI λ                */
UINT32       _G_uiMmuTlbSize          = 0;                              /*  TLB �����С                */
UINT32       _G_uiMmuEntryLoUnCache   = CONF_CM_UNCACHED;               /*  �Ǹ��ٻ���                  */
UINT32       _G_uiMmuEntryLoUnCacheWb = CONF_CM_UNCACHED;               /*  �Ǹ��ٻ������(д����)      */
UINT32       _G_uiMmuEntryLoCache     = CONF_CM_CACHABLE_NONCOHERENT;   /*  һ���Ը��ٻ���              */
/*********************************************************************************************************
** ��������: mipsMmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsMmuEnable (VOID)
{
}
/*********************************************************************************************************
** ��������: mipsMmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsMmuDisable (VOID)
{
}
/*********************************************************************************************************
** ��������: mipsMmuInvalidateMicroTLB
** ��������: ��Ч Micro TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsMmuInvalidateMicroTLB (VOID)
{
    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
    case CPU_LOONGSON2K:
        mipsCp0DiagWrite(LOONGSON_DIAG_ITLB);                           /*  ��Ч ITLB                   */
        break;

    case CPU_LOONGSON3:
        mipsCp0DiagWrite((LOONGSON_DIAG_DTLB) |                         /*  GS464E ���� DTLB, GS464 û��*/
                         (LOONGSON_DIAG_ITLB));                         /*  ��Ч ITLB DTLB              */
        break;

    default:
        /*
         * CETC-HR2 �;����� CPU, ITLB �� DTLB �����͸��
         */
        break;
    }
}
/*********************************************************************************************************
** ��������: mipsMmuInvalidateTLB
** ��������: ��Ч TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�����Ч Micro TLB
*********************************************************************************************************/
static VOID  mipsMmuInvalidateTLB (VOID)
{
    ULONG   ulEntryHiBak = mipsCp0EntryHiRead();
    INT     i;

    for (i = 0; i < MIPS_MMU_TLB_SIZE; i++) {
        mipsCp0IndexWrite(i);
        mipsCp0EntryLo0Write(0);
        mipsCp0EntryLo1Write(0);
        mipsCp0EntryHiWrite(MIPS_UNIQUE_ENTRYHI(i));
        MIPS_MMU_TLB_WRITE_INDEX();
    }

    mipsCp0EntryHiWrite(ulEntryHiBak);

    mipsMmuInvalidateMicroTLB();                                        /*  ��Ч Micro TLB              */
}
/*********************************************************************************************************
** ��������: mipsMmuInvalidateTLBMVA
** ��������: ��Чָ�� MVA �� TLB
** �䡡��  : ulAddr            ָ�� MVA
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�������Ч Micro TLB, �ⲿ������ɺ������Ч Micro TLB
*********************************************************************************************************/
VOID  mipsMmuInvalidateTLBMVA (addr_t  ulAddr)
{
    ULONG   ulEntryHiBak = mipsCp0EntryHiRead();
    ULONG   ulEntryHi    = ulAddr & (LW_CFG_VMM_PAGE_MASK << 1);
    INT32   iIndex;
    INT     iReTry;

    for (iReTry = 0; iReTry < 2; iReTry++) {                            /*  �����������һ���� TLB ��Ŀ */
        mipsCp0EntryHiWrite(ulEntryHi);
        MIPS_MMU_TLB_PROBE();
        iIndex = mipsCp0IndexRead();
        if (iIndex >= 0) {
            mipsCp0IndexWrite(iIndex);
            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);
            mipsCp0EntryHiWrite(MIPS_UNIQUE_ENTRYHI(iIndex));
            MIPS_MMU_TLB_WRITE_INDEX();

        } else {
            break;
        }
    }

    mipsCp0EntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** ��������: mipsMmuDumpTLB
** ��������: Dump TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsMmuDumpTLB (VOID)
{
    ULONG   ulEntryHiBak = mipsCp0EntryHiRead();
    ULONG   ulEntryLo0;
    ULONG   ulEntryLo1;
    ULONG   ulEntryHi;
    ULONG   ulPageMask;
    INT     i;

#if LW_CFG_CPU_WORD_LENGHT == 32
#define LX_FMT      "0x%08x"
#else
#define LX_FMT      "0x%016lx"
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    for (i = 0; i < MIPS_MMU_TLB_SIZE; i++) {
        mipsCp0IndexWrite(i);
        MIPS_MMU_TLB_READ();
        ulEntryLo0 = mipsCp0EntryLo0Read();
        ulEntryLo1 = mipsCp0EntryLo1Read();
        ulEntryHi  = mipsCp0EntryHiRead();
        ulPageMask = mipsCp0PageMaskRead();

        _PrintFormat("TLB[%02d]: EntryLo0="LX_FMT", EntryLo1="LX_FMT", "
                     "EntryHi="LX_FMT", PageMask="LX_FMT"\r\n",
                     i, ulEntryLo0, ulEntryLo1, ulEntryHi, ulPageMask);
    }

    mipsCp0EntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** ��������: mipsMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mipsMmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);                                      /*  ��λ CACHE                  */

    mipsCp0PageMaskWrite(MIPS_MMU_PAGE_MASK);                           /*  PAGE MASK                   */
    mipsCp0EntryHiWrite(0);                                             /*  ASID = 0                    */
    mipsCp0WiredWrite(0);                                               /*  ȫ����������滻            */
    mipsMmuInvalidateTLB();                                             /*  ��Ч TLB                    */

    if (_G_bMmuHasXI) {                                                 /*  ��ִ����ֹλ                */
        /*
         * д PageGrain ǰ��Ҫ��Ч TLB
         */
        mipsCp0PageGrainWrite(
#if LW_CFG_CPU_WORD_LENGHT == 64
                              PG_ELPA |                                 /*  ֧�� 48 λ�����ַ          */
#endif
                              PG_XIE  |                                 /*  ʹ�� EntryLo ִ����ֹλ     */
                              PG_IEC);                                  /*  ִ����ֹ������ TLBXI ����   */

        /*
         * ĿǰΪ��ͨ����, ��û��ʹ�ܸ�������(GS264, GS464E, ���, ����)���еĶ���ֹ RI λ,
         * ����ʹ�� ENTRYLO_V λ���ж�ӳ���Ƿ���Ч
         */
    }

    if ((_G_uiMipsCpuType == CPU_LOONGSON3) ||                          /*  Loongson-3x/2G/2H           */
        (_G_uiMipsCpuType == CPU_LOONGSON2K)) {                         /*  Loongson-2K                 */
        UINT32  uiGSConfig = mipsCp0GSConfigRead();
        uiGSConfig &= ~(1 <<  3);                                       /*  Store ����Ҳ����Ӳ���Զ�Ԥȡ*/
        uiGSConfig &= ~(1 << 15);                                       /*  ʹ�� VCache                 */
        uiGSConfig |=  (1 <<  8) |                                      /*  Store �����Զ�д�ϲ�        */
                       (1 <<  1) |                                      /*  ȡָ��������Ӳ���Զ�Ԥȡ    */
                       (1 <<  0) |                                      /*  ���ݷô��������Ӳ���Զ�Ԥȡ*/
                       MIPS_CONF6_FTLBDIS;                              /*  ֻ�� VTLB, ���� FTLB        */
        KN_SYNC();
        mipsCp0GSConfigWrite(uiGSConfig);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsMmuTlbLoadStoreExcHandle
** ��������: MMU TLB ����/�洢�쳣����
** �䡡��  : ulAbortAddr       ��ֹ��ַ
** �䡡��  : ��ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  mipsMmuTlbLoadStoreExcHandle (addr_t  ulAbortAddr, BOOL  bStore)
{
    ULONG  ulAbortType  = LW_VMM_ABORT_TYPE_MAP;
    ULONG  ulEntryHiBak = mipsCp0EntryHiRead();
    ULONG  ulEntryLo;
    INT32  iIndex;
    BOOL   bIsEntryLo1;

    mipsCp0EntryHiWrite(ulAbortAddr & (LW_CFG_VMM_PAGE_MASK << 1));
    MIPS_MMU_TLB_PROBE();
    iIndex = mipsCp0IndexRead();
    if (iIndex >= 0) {
        MIPS_MMU_TLB_READ();
        bIsEntryLo1 = !!(ulAbortAddr & (1 << LW_CFG_VMM_PAGE_SHIFT));
        if (bIsEntryLo1) {
            ulEntryLo = mipsCp0EntryLo1Read();

        } else {
            ulEntryLo = mipsCp0EntryLo0Read();
        }

        if (ulEntryLo & ENTRYLO_V) {
            /*
             * TLB ����ӳ����Ŀ, ����ӳ����Ч, �����ϲ�Ӧ�ó����������,
             * ���� Loongson-1B ��������ż�������(QEMU ��������),
             * �������� Loongson-1B �������� BUG, ������Ӧ�����������
             */
            ulAbortType = 0;

        } else {
            /*
             * TLB ��Ч�쳣(��ȷ���)
             */
            ulAbortType = LW_VMM_ABORT_TYPE_MAP;
        }
    } else {
        /*
         * ��Ҫ TLB ����, ����Ƶ� TLB ������Ʋ�������ͨ���쳣���������
         */
        ULONG  ulNesting = LW_CPU_GET_CUR_NESTING();
        if (ulNesting > 1) {
            /*
             * ��� TLB �����쳣�������쳣��, ֱ����ֹ
             */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "TLB refill error.\r\n");
            ulAbortType = LW_VMM_ABORT_TYPE_FATAL_ERROR;

        } else {
            /*
             * ��� TLB �����쳣���Ƿ����쳣��, ����֮, Ҳ������ TLB ����
             * QEMU ���� Qt ʱ�����, Loongson-1B ��������������
             */
            ulAbortType = 0;
        }
    }

    mipsCp0EntryHiWrite(ulEntryHiBak);

    return  (ulAbortType);
}
/*********************************************************************************************************
** ��������: mipsMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    REGISTER ULONG   i;

    if (ulPageNum > (MIPS_MMU_TLB_SIZE >> 1)) {
        mipsMmuInvalidateTLB();                                         /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            mipsMmuInvalidateTLBMVA(ulPageAddr);                        /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }

        mipsMmuInvalidateMicroTLB();                                    /*  ��Ч Micro TLB              */
    }
}
/*********************************************************************************************************
** ��������: mipsMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    UINT32  uiConfig;
    UINT32  uiMT;

    uiConfig = mipsCp0ConfigRead();                                     /*  �� Config0                  */
    uiMT = uiConfig & MIPS_CONF_MT;
    if ((uiMT != MIPS_CONF_MT_TLB) && (uiMT != MIPS_CONF_MT_FTLB)) {    /*  Config0 MT �� != 1��û�� MMU*/
        _DebugFormat(__PRINTMESSAGE_LEVEL,
                     "Warning: Config register MMU type is not standard: %d!\r\n", uiMT);
    }

    if (uiConfig & MIPS_CONF_M) {                                       /*  �� Config1                  */
        uiConfig        = mipsCp0Config1Read();                         /*  �� Config1                  */
        _G_uiMmuTlbSize = ((uiConfig & MIPS_CONF1_TLBS) >> 25) + 1;     /*  ��� MMUSize ��             */

    } else {
        _G_uiMmuTlbSize = 64;                                           /*  �������                    */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s MMU TLB size = %d.\r\n", pcMachineName, MIPS_MMU_TLB_SIZE);

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON1:
    case CPU_LOONGSON2:
    case CPU_LOONGSON3:
    case CPU_LOONGSON2K:
        _G_bMmuHasXI             = LW_TRUE;                             /*  ��ִ����ֹλ                */
        _G_uiMmuEntryLoUnCache   = 0x2;                                 /*  �Ǹ��ٻ���                  */
        _G_uiMmuEntryLoUnCacheWb = 0x7;                                 /*  �Ǹ��ٻ������              */
        _G_uiMmuEntryLoCache     = 0x3;                                 /*  һ���Ը��ٻ���              */
        break;

    case CPU_JZRISC:
        _G_bMmuHasXI             = LW_TRUE;                             /*  ��ִ����ֹλ                */
        _G_uiMmuEntryLoUnCache   = 0x2;                                 /*  �Ǹ��ٻ���                  */
        _G_uiMmuEntryLoUnCacheWb = 0x7;                                 /*  �Ǹ��ٻ������              */
        /*
         * 4: Cacheable, coherent, write-back, write-allocate, read misses request Exclusive
         * 5: Cacheable, coherent, write-back, write-allocate, read misses request Shared
         */
        _G_uiMmuEntryLoCache     = 0x5;                                 /*  һ���Ը��ٻ���              */
        break;

    case CPU_CETC_HR2:                                                  /*  CETC-HR2                    */
        _G_bMmuHasXI             = LW_TRUE;                             /*  ��ִ����ֹλ                */
        _G_uiMmuEntryLoUnCache   = 0x2;                                 /*  �Ǹ��ٻ���                  */
        _G_uiMmuEntryLoUnCacheWb = 0x2;                                 /*  �Ǹ��ٻ���                  */
        _G_uiMmuEntryLoCache     = 0x6;                                 /*  һ���Ը��ٻ���              */
        _G_uiMmuTlbSize          = 256;                                 /*  256 �� TLB ��Ŀ             */
        break;

    default:
        break;
    }

#if LW_CFG_SMP_EN > 0
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;
#else
    pmmuop->MMUOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    pmmuop->MMUOP_pfuncGlobalInit    = mipsMmuGlobalInit;
    pmmuop->MMUOP_pfuncInvalidateTLB = mipsMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = mipsMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = mipsMmuDisable;

#if LW_CFG_CPU_WORD_LENGHT == 32
    mips32MmuInit(pmmuop, pcMachineName);
#else
    mips64MmuInit(pmmuop, pcMachineName);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
