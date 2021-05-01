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
** ��   ��   ��: mipsCacheLs3x.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 11 �� 02 ��
**
** ��        ��: Loongson-3x ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  mipsCacheR4kEnableHw(VOID);
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT  _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
** ��������: ls3xCacheEnableHw
** ��������: ʹ�� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ls3xCacheEnableHw (VOID)
{
    mipsCacheR4kEnableHw();
}
/*********************************************************************************************************
** ��������: ls3xBranchPredictionDisable
** ��������: ���ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionDisable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 0;                                                   /*  �� 1 ʱ���� RAS             */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3xBranchPredictionEnable
** ��������: ʹ�ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionEnable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag &= ~(1 << 0);
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3xBranchPredictorInvalidate
** ��������: ��Ч��֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictorInvalidate (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 1;                                                   /*  д�� 1 ��� BRBTB �� BTAC   */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3aR1CacheFlushAll
** ��������: Loongson-3A R1 ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3aR1CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  Flush L1 ICACHE             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  Flush L1 DCACHE             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3aR2CacheFlushAll
** ��������: Loongson-3A R2 ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3aR2CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  Flush L1 ICACHE             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  Flush L1 DCACHE             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "2: cache 2, 0(%[addr])           \n"                           /*  Flush L1 VCACHE             */
        "   cache 2, 1(%[addr])           \n"
        "   cache 2, 2(%[addr])           \n"
        "   cache 2, 3(%[addr])           \n"
        "   cache 2, 4(%[addr])           \n"
        "   cache 2, 5(%[addr])           \n"
        "   cache 2, 6(%[addr])           \n"
        "   cache 2, 7(%[addr])           \n"
        "   cache 2, 8(%[addr])           \n"
        "   cache 2, 9(%[addr])           \n"
        "   cache 2, 10(%[addr])          \n"
        "   cache 2, 11(%[addr])          \n"
        "   cache 2, 12(%[addr])          \n"
        "   cache 2, 13(%[addr])          \n"
        "   cache 2, 14(%[addr])          \n"
        "   cache 2, 15(%[addr])          \n"
        "   addiu %[vsets], %[vsets], -1  \n"
        "   bnez  %[vsets], 2b            \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr),
          [vsets] "r" (_G_VCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3bCacheFlushAll
** ��������: Loongson-3B ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3bCacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  Flush L1 ICACHE             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  Flush L1 DCACHE             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushAll
** ��������: ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ls3xCacheFlushAll (VOID)
{
    mipsCpuProbe(MIPS_MACHINE_LS3X);                                    /*  MIPS CPU ̽��               */
    mipsCacheProbe(MIPS_MACHINE_LS3X);                                  /*  CACHE ̽��                  */

    switch (_G_uiMipsPridRev) {

    case PRID_REV_LOONGSON3A_R2:
    case PRID_REV_LOONGSON3A_R3_0:
    case PRID_REV_LOONGSON3A_R3_1:
        ls3aR2CacheFlushAll();
        break;

    case PRID_REV_LOONGSON3B_R1:
    case PRID_REV_LOONGSON3B_R2:
        ls3bCacheFlushAll();
        break;

    case PRID_REV_LOONGSON3A_R1:                                        /*  use default case            */
    case PRID_REV_LOONGSON2K_R1:
    case PRID_REV_LOONGSON2K_R2:
    default:
        ls3aR1CacheFlushAll();
        break;
    }
}
/*********************************************************************************************************
** ��������: ls3xCacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_D_EN;
        }
    }

    if (_G_iCacheStatus == L1_CACHE_EN) {
        ls3xCacheEnableHw();
        ls3xBranchPredictionEnable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_D_EN;
        }
    }

    if (_G_iCacheStatus == L1_CACHE_DIS) {
        /*
         * ���ܹر� CACHE
         */
        ls3xBranchPredictionDisable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushPageNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheInvalidateNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheInvalidatePageNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                         PVOID          pvAdrs,
                                         PVOID          pvPdrs,
                                         size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheClearNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheClearPageNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls3xCacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls3xCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheLs3xInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheLs3xInit (LW_CACHE_OP  *pcacheop,
                         CACHE_MODE    uiInstruction,
                         CACHE_MODE    uiData,
                         CPCHAR        pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    mipsCacheInfoShow();                                                /*  ��ӡ CACHE ��Ϣ             */
    /*
     * ���ܹر� CACHE
     */
    ls3xBranchPredictorInvalidate();                                    /*  ��Ч��֧Ԥ��                */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  ���� TEXT_UPDATE_MP ѡ��    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;                      /*  VIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWaySize;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWaySize;

    pcacheop->CACHEOP_pfuncEnable  = ls3xCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ls3xCacheDisable;
    /*
     * Loongson-3x ʵ���˸��� CACHE ��Ӳ��һ���ԣ����� CACHE �������� NONE ����
     */
    pcacheop->CACHEOP_pfuncFlush          = ls3xCacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = ls3xCacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = ls3xCacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = ls3xCacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = ls3xCacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = ls3xCacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = ls3xCacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = ls3xCacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = ls3xCacheLockNone;          /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock         = ls3xCacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mipsCacheLs3xReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsCacheLs3xReset (CPCHAR  pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    /*
     * ���ܹر� CACHE
     */
    ls3xBranchPredictorInvalidate();                                    /*  ��Ч��֧Ԥ��                */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
