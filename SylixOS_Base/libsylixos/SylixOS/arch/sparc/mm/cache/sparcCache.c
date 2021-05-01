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
** ��   ��   ��: sparcCache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 10 �� 10 ��
**
** ��        ��: SPARC ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  LEON CACHE FLUSH ��������ָ��
*********************************************************************************************************/
static VOID  (*leonFlushICacheAll)(VOID) = LW_NULL;
static VOID  (*leonFlushDCacheAll)(VOID) = LW_NULL;
/*********************************************************************************************************
  LEON2 ��ض���
*********************************************************************************************************/
#define LEON2_PREGS           0x80000000
#define LEON2_CCR             (LEON2_PREGS + 0x14)
#define LEON2_LCR             (LEON2_PREGS + 0x24)

#define LEON2_CCR_IEN         (3 << 0)                                  /*  ʹ�� ICACHE                 */
#define LEON2_CCR_DEN         (3 << 2)                                  /*  ʹ�� DCACHE                 */
#define LEON2_CCR_IFP         (1 << 15)                                 /*  ICACHE flush pend           */
#define LEON2_CCR_DFP         (1 << 14)                                 /*  DCACHE flush pend           */
#define LEON2_CCR_IBURST      (1 << 16)                                 /*  ʹ�� Instruction burst fetch*/
#define LEON2_CCR_DSNOOP      (1 << 23)                                 /*  ʹ�� DCACHE SNOOP           */
/*********************************************************************************************************
** ��������: leon2FlushICacheAll
** ��������: LEON2 FLUSH ���� ICACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon2FlushICacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ (" flush ");

    while ((uiCCR = read32(LEON2_CCR)) & LEON2_CCR_IFP) {
    }
}
/*********************************************************************************************************
** ��������: leon2FlushDCacheAll
** ��������: LEON2 FLUSH ���� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon2FlushDCacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ ("sta %%g0, [%%g0] %0\n\t" : :
                          "i"(ASI_LEON_DFLUSH) : "memory");

    while ((uiCCR = read32(LEON2_CCR)) & LEON2_CCR_DFP) {
    }
}
/*********************************************************************************************************
** ��������: leon2CacheEnable
** ��������: LEON2 ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  leon2CacheEnable (LW_CACHE_TYPE  cachetype)
{
    UINT32  uiCCR;

    if (cachetype == INSTRUCTION_CACHE) {
        leon2FlushICacheAll();

        uiCCR  = read32(LEON2_CCR);
        uiCCR |= LEON2_CCR_IEN;                                         /*  ʹ�� ICACHE                 */
        uiCCR |= LEON2_CCR_IBURST;                                      /*  ʹ�� Instruction burst fetch*/
        write32(uiCCR, LEON2_CCR);

    } else {
        leon2FlushDCacheAll();

        uiCCR  = read32(LEON2_CCR);
        uiCCR |= LEON2_CCR_DEN;                                         /*  ʹ�� DCACHE                 */
        uiCCR |= LEON2_CCR_DSNOOP;                                      /*  ʹ�� DCACHE SNOOP           */
        write32(uiCCR, LEON2_CCR);
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: leon2CacheDisable
** ��������: LEON2 ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : DCACHE Ϊд��͸ģʽ, ���û�д.
*********************************************************************************************************/
static INT  leon2CacheDisable (LW_CACHE_TYPE  cachetype)
{
    UINT32  uiCCR = read32(LEON2_CCR);

    if (cachetype == INSTRUCTION_CACHE) {
        uiCCR &= ~(LEON2_CCR_IEN);                                      /*  ���� ICACHE                 */

    } else {
        uiCCR &= ~(LEON2_CCR_DEN);                                      /*  ���� ICACHE                 */
    }

    write32(uiCCR, LEON2_CCR);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: leon2CacheProbe
** ��������: LEON2 ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon2CacheProbe (LW_CACHE_OP *pcacheop,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    pcacheop->CACHEOP_pfuncEnable  = leon2CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = leon2CacheDisable;

    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;                  /*  No SMP support, safe only!  */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = 8 * LW_CFG_KB_SIZE;              /*  32KB ����·������ ICACHE    */
    pcacheop->CACHEOP_iDCacheWaySize = 8 * LW_CFG_KB_SIZE;              /*  16KB ����·������ DCACHE    */

    leonFlushICacheAll = leon2FlushICacheAll;
    leonFlushDCacheAll = leon2FlushDCacheAll;
}
/*********************************************************************************************************
  LEON3 ��ض���
*********************************************************************************************************/
typedef struct {
    ULONG   LC_ulCCR;                               /*  0x00 - Cache Control Register                   */
    ULONG   LC_ulICCR;                              /*  0x08 - Instruction Cache Configuration Register */
    ULONG   LC_ulDCCR;                              /*  0x0c - Data Cache Configuration Register        */
} LEON3_CACHE_REGS;                                 /*  LEON3 CACHE ���üĴ���                          */

#define LEON3_CCR_IFP         (1 << 15)                                 /*  ICACHE flush pend           */
#define LEON3_CCR_DFP         (1 << 14)                                 /*  DCACHE flush pend           */
/*********************************************************************************************************
** ��������: leon3FlushICacheAll
** ��������: LEON3 FLUSH ���� ICACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon3FlushICacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ (" flush ");

    do {
        __asm__ __volatile__ ("lda [%%g0] 2, %0\n\t" : "=r"(uiCCR));
    } while (uiCCR & LEON3_CCR_IFP);
}
/*********************************************************************************************************
** ��������: leon3FlushDCacheAll
** ��������: LEON3 FLUSH ���� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon3FlushDCacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ ("sta %%g0, [%%g0] %0\n\t" : :
                          "i"(ASI_LEON_DFLUSH) : "memory");

    do {
        __asm__ __volatile__ ("lda [%%g0] 2, %0\n\t" : "=r"(uiCCR));
    } while (uiCCR & LEON3_CCR_DFP);
}
/*********************************************************************************************************
** ��������: leon3CacheEnable
** ��������: LEON3 ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  leon3CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leon3FlushICacheAll();

        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x010003, %%l2\n\t"                 /*  ʹ�� ICACHE                 */
                              "or   %%l2, %%l1, %%l2\n\t"               /*  ʹ�� Instruction burst fetch*/
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    } else {
        leon3FlushDCacheAll();

        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x80000c, %%l2\n\t"                 /*  ʹ�� DCACHE                 */
                              "or   %%l2, %%l1, %%l2\n\t"               /*  ʹ�� DCACHE SNOOP           */
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: leon3CacheDisable
** ��������: LEON3 ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : DCACHE Ϊд��͸ģʽ, ���û�д.
*********************************************************************************************************/
static INT  leon3CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x000003, %%l2\n\t"                 /*  ���� ICACHE                 */
                              "andn %%l2, %%l1, %%l2\n\t"
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    } else {
        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x00000c, %%l2\n\t"                 /*  ���� DCACHE                 */
                              "andn %%l2, %%l1, %%l2\n\t"
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: leon3GetCacheRegs
** ��������: LEON3 ������� CACHE ���üĴ���
** �䡡��  : pRegs         CACHE ���üĴ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon3GetCacheRegs (LEON3_CACHE_REGS  *pRegs)
{
    ULONG  ulCCR, ulICCR, ulDCCR;

    if (!pRegs) {
        return;
    }

    /*
     * Get Cache regs from "Cache ASI" address 0x0, 0x8 and 0xC
     */
    __asm__ __volatile__ ("lda [%%g0] %3, %0\n\t"
                          "mov 0x08, %%g1\n\t"
                          "lda [%%g1] %3, %1\n\t"
                          "mov 0x0c, %%g1\n\t"
                          "lda [%%g1] %3, %2\n\t"
                          : "=r"(ulCCR), "=r"(ulICCR), "=r"(ulDCCR)     /*  output                      */
                          : "i"(ASI_LEON_CACHEREGS)                     /*  input                       */
                          : "g1"                                        /*  clobber list                */
                          );

    pRegs->LC_ulCCR  = ulCCR;
    pRegs->LC_ulICCR = ulICCR;
    pRegs->LC_ulDCCR = ulDCCR;
}
/*********************************************************************************************************
** ��������: leon3CacheProbe
** ��������: LEON3 ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  leon3CacheProbe (LW_CACHE_OP *pcacheop,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    LEON3_CACHE_REGS  cregs;
    UINT              uiSetSize, uiSetNr;
    CHAR             *pcSetStr[4] = { "direct mapped",
                                      "2-way associative",
                                      "3-way associative",
                                      "4-way associative" };

    leon3GetCacheRegs(&cregs);

    uiSetNr = (cregs.LC_ulDCCR & LEON3_XCCR_SETS_MASK) >> 24;

    /*
     * (ssize=>realsize) 0=>1k, 1=>2k, 2=>4k, 3=>8k ...
     */
    uiSetSize = 1 << ((cregs.LC_ulDCCR & LEON3_XCCR_SSIZE_MASK) >> 20);

    _DebugFormat(__LOGMESSAGE_LEVEL, "CACHE: %s cache, set size %dk\r\n",
                 uiSetNr > 3 ? "unknown" : pcSetStr[uiSetNr], uiSetSize);

    if ((uiSetSize <= (LW_CFG_VMM_PAGE_SIZE / 1024)) && (uiSetNr == 0)) {
        /*
         * Set Size <= Page size  ==> Invalidate on every context switch not needed.
         */
        _DebugFormat(__LOGMESSAGE_LEVEL, "CACHE: not invalidate on every context switch\r\n");
    }

    pcacheop->CACHEOP_pfuncEnable  = leon3CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = leon3CacheDisable;

    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = uiSetSize * 1024;
    pcacheop->CACHEOP_iDCacheWaySize = uiSetSize * 1024;

    leonFlushICacheAll = leon3FlushICacheAll;
    leonFlushDCacheAll = leon3FlushDCacheAll;
}
/*********************************************************************************************************
** ��������: sparcCacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : DCACHE Ϊд��͸ģʽ.
*********************************************************************************************************/
LW_WEAK INT  sparcCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : DCACHE Ϊд��͸ģʽ.
*********************************************************************************************************/
static INT  sparcCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sparcCacheUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  sparcCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sparcCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
             DCACHE Ϊд��͸ģʽ, ֻ��Ҫ��Ч ICACHE.
*********************************************************************************************************/
static INT  sparcCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    leonFlushICacheAll();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sparcCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
             DCACHE Ϊд��͸ģʽ.
*********************************************************************************************************/
static INT  sparcCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV4Init
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  sparcCacheInit (LW_CACHE_OP *pcacheop,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    if ((lib_strcmp(pcMachineName, SPARC_MACHINE_LEON3) == 0) ||
        (lib_strcmp(pcMachineName, SPARC_MACHINE_LEON4) == 0)) {
        leon3CacheProbe(pcacheop, uiInstruction, uiData, pcMachineName);

    } else {
        leon2CacheProbe(pcacheop, uiInstruction, uiData, pcMachineName);
    }

    pcacheop->CACHEOP_pfuncLock   = sparcCacheLock;                     /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = sparcCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = sparcCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = sparcCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = sparcCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = sparcCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = sparcCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = sparcCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = sparcCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = sparcCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheV4Reset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  sparcCacheReset (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, SPARC_MACHINE_LEON3) == 0) ||
        (lib_strcmp(pcMachineName, SPARC_MACHINE_LEON4) == 0)) {
        leon3CacheDisable(DATA_CACHE);
        leon3CacheDisable(INSTRUCTION_CACHE);

    } else {
        leon2CacheDisable(DATA_CACHE);
        leon2CacheDisable(INSTRUCTION_CACHE);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
