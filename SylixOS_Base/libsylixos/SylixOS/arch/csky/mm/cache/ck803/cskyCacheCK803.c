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
** ��   ��   ��: cskyCacheCK803.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 11 �� 12 ��
**
** ��        ��: C-SKY CK803 ��ϵ�ܹ� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY ��ϵ�ܹ�
*********************************************************************************************************/
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../../mpu/cskyMpu.h"
/*********************************************************************************************************
  L2 CACHE ֧��
*********************************************************************************************************/
#if LW_CFG_CSKY_CACHE_L2 > 0
#include "../l2/cskyL2.h"
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
#define L1_CACHE_I_EN                      0x01
#define L1_CACHE_D_EN                      0x02
#define L1_CACHE_EN                        (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS                       0x00

static INT  iCacheStatus = L1_CACHE_DIS;
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
typedef struct {
    UINT32      CACHE_uiSize;                                           /*  CACHE ��С                  */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE �д�С                */
    UINT32      CACHE_uiWaySize;                                        /*  ·��С                      */
} CSKY_CACHE;

static CSKY_CACHE _G_ICacheInfo = {
    4 * LW_CFG_KB_SIZE,                                                 /*  4KB ����·������ ICACHE     */
    16,
    1 * LW_CFG_KB_SIZE
};

static CSKY_CACHE _G_DCacheInfo = {
    4 * LW_CFG_KB_SIZE,                                                 /*  4KB ����·������ DCACHE     */
    16,
    1 * LW_CFG_KB_SIZE
};

#define CSKY_ICACHE_SIZE                   _G_ICacheInfo.CACHE_uiSize
#define CSKY_DCACHE_SIZE                   _G_DCacheInfo.CACHE_uiSize

#define CSKY_ICACHE_LINE_SIZE              _G_ICacheInfo.CACHE_uiLineSize
#define CSKY_DCACHE_LINE_SIZE              _G_DCacheInfo.CACHE_uiLineSize

#define CSKY_ICACHE_WAY_SIZE               _G_ICacheInfo.CACHE_uiWaySize
#define CSKY_DCACHE_WAY_SIZE               _G_DCacheInfo.CACHE_uiWaySize

#define CSKY_L1_CACHE_LOOP_OP_MAX_SIZE     (CSKY_DCACHE_SIZE >> 1)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  On chip cache structure.
*********************************************************************************************************/
typedef struct
{
    volatile UINT32  CER;                   /*  Cache enable register                                   */
    volatile UINT32  CIR;                   /*  Cache invalid register                                  */
    volatile UINT32  CRCR[4U];              /*  Cache Configuration register                            */
    volatile UINT32  RSERVED0[1015U];
    volatile UINT32  CPFCR;                 /*  Cache performance analisis control register             */
    volatile UINT32  CPFATR;                /*  Cache access times register                             */
    volatile UINT32  CPFMTR;                /*  Cache missing times register                            */
} CACHE_Type;
/*********************************************************************************************************
  Memory mapping of CK803 Hardware
*********************************************************************************************************/
#define TCIP_BASE                           (0xe000e000ul)              /*  Titly Coupled IP Base Addr  */
#define CACHE_BASE                          (TCIP_BASE + 0x1000ul)      /*  CACHE Base Address          */

#define CACHE                               ((CACHE_Type *)CACHE_BASE)  /*  cache configuration struct  */
/*********************************************************************************************************
  CACHE Register Definitions
*********************************************************************************************************/
#define CACHE_CER_EN_Pos                    0u                          /*  CACHE CER: EN Position      */
#define CACHE_CER_EN_Msk                    (0x1ul << CACHE_CER_EN_Pos)

#define CACHE_CER_CFIG_Pos                  1u                          /*  CACHE CER: CFIG Position    */
#define CACHE_CER_CFIG_Msk                  (0x1ul << CACHE_CER_CFIG_Pos)

#define CACHE_CER_WB_Pos                    2u                          /*  CACHE CER: WB Position      */
#define CACHE_CER_WB_Msk                    (0x1ul << CACHE_CER_WB_Pos)

#define CACHE_CER_WCFIG_Pos                 3u                          /*  CACHE CER: WCFIG Position   */
#define CACHE_CER_WCFIG_Msk                 (0x1ul << CACHE_CER_WCFIG_Pos)

#define CACHE_CER_DCW_Pos                   4u                          /*  CACHE CER: DCW Position     */
#define CACHE_CER_DCW_Msk                   (0x1ul << CACHE_CER_DCW_Pos)

#define CACHE_CER_WA_Pos                    5u                          /*  CACHE CER: WA Position      */
#define CACHE_CER_WA_Msk                    (0x1ul << CACHE_CER_WA_Pos)

#define CACHE_CIR_INV_ALL_Pos               0u                          /*  CACHE CIR: INV_ALL Position */
#define CACHE_CIR_INV_ALL_Msk               (0x1ul << CACHE_CIR_INV_ALL_Pos)

#define CACHE_CIR_INV_ONE_Pos               1u                          /*  CACHE CIR: INV_ONE Position */
#define CACHE_CIR_INV_ONE_Msk               (0x1ul << CACHE_CIR_INV_ONE_Pos)

#define CACHE_CIR_CLR_ALL_Pos               2u                          /*  CACHE CIR: CLR_ALL Position */
#define CACHE_CIR_CLR_ALL_Msk               (0x1ul << CACHE_CIR_CLR_ALL_Pos)

#define CACHE_CIR_CLR_ONE_Pos               3u                          /*  CACHE CIR: CLR_ONE Position */
#define CACHE_CIR_CLR_ONE_Msk               (0x1ul << CACHE_CIR_CLR_ONE_Pos)

#define CACHE_CIR_INV_ADDR_Pos              4u                          /*  CACHE CIR: INV_ADDR Position*/
#define CACHE_CIR_INV_ADDR_Msk              (0xffffffful << CACHE_CIR_INV_ADDR_Pos)

#define CACHE_CRCR_EN_Pos                   0u                          /*  CACHE CRCR: EN Position     */
#define CACHE_CRCR_EN_Msk                   (0x1ul << CACHE_CRCR_EN_Pos)

#define CACHE_CRCR_SIZE_Pos                 1u                          /*  CACHE CRCR: Size Position   */
#define CACHE_CRCR_SIZE_Msk                 (0x1ful << CACHE_CRCR_SIZE_Pos)

#define CACHE_CRCR_BASE_ADDR_Pos            10u                         /*  CACHE CRCR: base addr Pos   */
#define CACHE_CRCR_BASE_ADDR_Msk            (0x3ffffful << CACHE_CRCR_BASE_ADDR_Pos)

#define CACHE_CPFCR_PFEN_Pos                0u                          /*  CACHE CPFCR: PFEN Position  */
#define CACHE_CPFCR_PFEN_Msk                (0x1ul << CACHE_CPFCR_PFEN_Pos)

#define CACHE_CPFCR_PFRST_Pos               1U                          /*  CACHE CPFCR: PFRST Position */
#define CACHE_CPFCR_PFRST_Msk               (0x1ul << CACHE_CPFCR_PFRST_Pos)
/*********************************************************************************************************
   Mask and shift operation.
*********************************************************************************************************/
#define _VAL2FLD(field, value)              ((value << field ## _Pos) & field ## _Msk)
#define _FLD2VAL(field, value)              ((value & field ## _Msk) >> field ## _Pos)
/*********************************************************************************************************
** ��������: cskyICacheCK803Enable
** ��������: ʹ�� ICACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyICacheCK803Enable (VOID)
{
    CACHE->CIR  = CACHE_CIR_INV_ALL_Msk;                                 /*  invalidate all Cache       */
    CACHE->CER |= (UINT32)(CACHE_CER_EN_Msk | CACHE_CER_CFIG_Msk);       /*  enable all Cache           */
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803Enable
** ��������: ʹ�� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803Enable (VOID)
{
    CACHE->CIR = CACHE_CIR_INV_ALL_Msk;                                 /*  invalidate all Cache        */
    CACHE->CER = (UINT32)(CACHE_CER_EN_Msk | CACHE_CER_WB_Msk | CACHE_CER_DCW_Msk);
                                                                        /*  enable all Cache            */
}
/*********************************************************************************************************
** ��������: cskyICacheCK803Disable
** ��������: ���� ICACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyICacheCK803Disable (VOID)
{
    CACHE->CER &= ~(UINT32)(CACHE_CER_EN_Msk | CACHE_CER_CFIG_Msk);     /*  disable all Cache           */
    CACHE->CIR  = CACHE_CIR_INV_ALL_Msk;                                /*  invalidate all Cache        */
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803Disable
** ��������: ���� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803Disable (VOID)
{
    CACHE->CER &= ~(UINT32)CACHE_CER_EN_Msk;                            /*  disable all Cache           */
    CACHE->CIR  = CACHE_CIR_INV_ALL_Msk;                                /*  invalidate all Cache        */
}
/*********************************************************************************************************
** ��������: cskyICacheCK803InvalidateAll
** ��������: ��Ч���� ICACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyICacheCK803InvalidateAll (VOID)
{
    CACHE->CIR = CACHE_CIR_INV_ALL_Msk;                                 /*  invalidate all Cache        */
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803FlushAll
** ��������: ��д���� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803FlushAll (VOID)
{
    CACHE->CIR = _VAL2FLD(CACHE_CIR_CLR_ALL, 1);                        /*  clean all Cache             */
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803ClearAll
** ��������: ��д����Ч���� DCACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803ClearAll (VOID)
{
    CACHE->CIR = _VAL2FLD(CACHE_CIR_INV_ALL, 1) | _VAL2FLD(CACHE_CIR_CLR_ALL, 1);
                                                                        /*  clean and inv all Cache     */
}
/*********************************************************************************************************
** ��������: cskyICacheCK803Invalidate
** ��������: ��Чָ������� ICACHE
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyICacheCK803Invalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    UINT32  uiOpAddr   = (UINT32)pvStart & CACHE_CIR_INV_ADDR_Msk;
    UINT32  uiLineSize = 16;
    INT32   iOpSize    = (UINT32)pvEnd - (UINT32)pvStart;

    uiOpAddr |= _VAL2FLD(CACHE_CIR_INV_ONE, 1);

    while (iOpSize >= 128) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;

        iOpSize   -= 128;
    }

    while (iOpSize > 0) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        iOpSize   -= uiLineSize;
    }
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803Invalidate
** ��������: ��Чָ������� DCACHE
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803Invalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    UINT32  uiOpAddr   = (UINT32)pvStart & CACHE_CIR_INV_ADDR_Msk;
    UINT32  uiLineSize = 16;
    INT32   iOpSize    = (UINT32)pvEnd - (UINT32)pvStart;

    if (iOpSize == 0) {
        iOpSize = uiLineSize;
    }

    uiOpAddr |= _VAL2FLD(CACHE_CIR_INV_ONE, 1);

    while (iOpSize >= 128) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;

        iOpSize   -= 128;
    }

    while (iOpSize > 0) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        iOpSize   -= uiLineSize;
    }
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803Flush
** ��������: ��дָ������� DCACHE
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803Flush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    UINT32  uiOpAddr   = (UINT32)pvStart & CACHE_CIR_INV_ADDR_Msk;
    UINT32  uiLineSize = 16;
    INT32   iOpSize    = (UINT32)pvEnd - (UINT32)pvStart;

    if (iOpSize == 0) {
        iOpSize = uiLineSize;
    }

    uiOpAddr |= _VAL2FLD(CACHE_CIR_CLR_ONE, 1);

    while (iOpSize >= 128) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;

        iOpSize   -= 128;
    }

    while (iOpSize > 0) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        iOpSize   -= uiLineSize;
    }
}
/*********************************************************************************************************
** ��������: cskyDCacheCK803Clear
** ��������: ��д����Чָ������� DCACHE
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyDCacheCK803Clear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    UINT32  uiOpAddr   = (UINT32)pvStart & CACHE_CIR_INV_ADDR_Msk;
    UINT32  uiLineSize = 16;
    INT32   iOpSize    = (UINT32)pvEnd - (UINT32)pvStart;

    if (iOpSize == 0) {
        iOpSize = uiLineSize;
    }

    uiOpAddr |= _VAL2FLD(CACHE_CIR_INV_ONE, 1) | _VAL2FLD(CACHE_CIR_CLR_ONE, 1);

    while (iOpSize >= 128) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;

        iOpSize   -= 128;
    }

    while (iOpSize > 0) {
        CACHE->CIR = uiOpAddr;
        uiOpAddr  += uiLineSize;
        iOpSize   -= uiLineSize;
    }
}
/*********************************************************************************************************
** ��������: cskyBranchPredictorCK803Invalidate
** ��������: ��Ч��֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyBranchPredictorCK803Invalidate (VOID)
{
}
/*********************************************************************************************************
** ��������: cskyBranchPredictionCK803Disable
** ��������: ���ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyBranchPredictionCK803Disable (VOID)
{
}
/*********************************************************************************************************
** ��������: cskyBranchPredictionCK803Enable
** ��������: ʹ�ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyBranchPredictionCK803Enable (VOID)
{
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Enable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        cskyICacheCK803Enable();
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyBranchPredictionCK803Enable();

    } else {
        cskyDCacheCK803Enable();
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

#if LW_CFG_CSKY_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        cskyL2CK803Enable();
    }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Disable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        cskyICacheCK803Disable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        cskyBranchPredictionCK803Disable();

    } else {
        cskyDCacheCK803Disable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }

#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        cskyL2CK803Disable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803FlushAll();                                  /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Flush(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2CK803FlushAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803FlushAll();                                  /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Flush(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2CK803Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheCK803InvalidateAll();                             /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheCK803Invalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheCK803Clear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheCK803Clear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                cskyDCacheCK803Invalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2CK803Invalidate(pvAdrs, stBytes);                     /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheCK803InvalidateAll();                             /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheCK803Invalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheCK803Clear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheCK803Clear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                cskyDCacheCK803Invalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2CK803Invalidate(pvPdrs, stBytes);                     /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheCK803InvalidateAll();                             /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheCK803Invalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803ClearAll();                                  /*  ȫ����д����Ч              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Clear(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2CK803ClearAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheCK803InvalidateAll();                             /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheCK803Invalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803ClearAll();                                  /*  ȫ����д����Ч              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Clear(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2CK803Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Lock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803Unlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheCK803Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  cskyCacheCK803TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
        cskyDCacheCK803ClearAll();                                      /*  DCACHE ȫ����д             */
        cskyICacheCK803InvalidateAll();                                 /*  ICACHE ȫ����Ч             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
        cskyDCacheCK803Flush(pvAdrs, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);

        CSKY_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
        cskyICacheCK803Invalidate(pvAdrsBak, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  cskyCacheCK803DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803FlushAll();                                  /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Flush(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д                    */
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheCK803ClearAll();                                  /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheCK803Clear(pvAdrs, (PVOID)ulEnd,
                                 CSKY_DCACHE_LINE_SIZE);                /*  ���ֻ�д                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheCK803RangeSet
** ��������: CACHE ��������
** �䡡��  : uiIndex  �洢 CACHE ���õ� CRCR �±�
**           ulBase   �������ַ
**           uiSize   �����С
**           uiEnable �Ƿ�ʹ�� CACHE ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyCacheCK803RangeSet (UINT32  uiIndex, ULONG  ulBase, UINT32  uiSize, UINT32  uiEnable)
{
    CACHE->CRCR[uiIndex] = ((ulBase & CACHE_CRCR_BASE_ADDR_Msk) |
                            (_VAL2FLD(CACHE_CRCR_SIZE, uiSize)) |
                            (_VAL2FLD(CACHE_CRCR_EN, uiEnable)));
}
/*********************************************************************************************************
** ��������: cskyCacheInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyCacheInit (LW_CACHE_OP *pcacheop,
                             CACHE_MODE   uiInstruction,
                             CACHE_MODE   uiData,
                             CPCHAR       pcMachineName)
{
    if (lib_strcmp(pcMachineName, CSKY_MACHINE_803) != 0) {
        return;
    }

    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    pcacheop->CACHEOP_iICacheLine = CSKY_ICACHE_LINE_SIZE;
    pcacheop->CACHEOP_iDCacheLine = CSKY_DCACHE_LINE_SIZE;

    pcacheop->CACHEOP_iICacheWaySize = CSKY_ICACHE_WAY_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize = CSKY_ICACHE_WAY_SIZE;

    pcacheop->CACHEOP_pfuncEnable  = cskyCacheCK803Enable;
    pcacheop->CACHEOP_pfuncDisable = cskyCacheCK803Disable;

    pcacheop->CACHEOP_pfuncLock   = cskyCacheCK803Lock;                 /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = cskyCacheCK803Unlock;

    pcacheop->CACHEOP_pfuncFlush          = cskyCacheCK803Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = cskyCacheCK803FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = cskyCacheCK803Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = cskyCacheCK803InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = cskyCacheCK803Clear;
    pcacheop->CACHEOP_pfuncClearPage      = cskyCacheCK803ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = cskyCacheCK803TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = cskyCacheCK803DataUpdate;

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = cskyMpuDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = cskyMpuDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = cskyMpuDmaFree;
#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
}
/*********************************************************************************************************
** ��������: cskyCacheReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  cskyCacheReset (CPCHAR  pcMachineName)
{
    if (lib_strcmp(pcMachineName, CSKY_MACHINE_803) != 0) {
        return;
    }

    cskyICacheCK803InvalidateAll();
    cskyDCacheCK803Disable();
    cskyICacheCK803Disable();
    cskyBranchPredictorCK803Invalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
/*********************************************************************************************************
  END
*********************************************************************************************************/
