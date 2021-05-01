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
** ��   ��   ��: cskyCache.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY ��ϵ�ܹ�
*********************************************************************************************************/
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cskyCache.h"
#include "arch/csky/inc/cskyregs.h"
#include "arch/csky/arch_mmu.h"
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

static INT      iCacheStatus = L1_CACHE_DIS;
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static UINT32   uiCacheCfg = 0;
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  cskyICacheInvalidateAll(VOID);
extern VOID  cskyICacheEnableHw(VOID);
extern VOID  cskyICacheDisableHw(VOID);

extern VOID  cskyDCacheInvalidateAll(VOID);
extern VOID  cskyDCacheClearAll(VOID);
extern VOID  cskyDCacheFlushAll(VOID);
extern VOID  cskyDCacheDisableHw(VOID);
extern VOID  cskyDCacheEnableHw(UINT32  uiCacheCfg);

extern VOID  cskyBranchPredictorInvalidate(VOID);
extern VOID  cskyBranchPredictionEnable(VOID);
extern VOID  cskyBranchPredictionDisable(VOID);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
typedef struct {
    UINT32      CACHE_uiSize;                                           /*  CACHE ��С                  */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE �д�С                */
    UINT32      CACHE_uiWaySize;                                        /*  ·��С                      */
} CSKY_CACHE;

static CSKY_CACHE _G_ICacheInfo = {
    8 * LW_CFG_KB_SIZE,                                                 /*  8KB ����·������ ICACHE     */
    32,
    2 * LW_CFG_KB_SIZE
};

static CSKY_CACHE _G_DCacheInfo = {
    8 * LW_CFG_KB_SIZE,                                                 /*  8KB ����·������ DCACHE     */
    32,
    2 * LW_CFG_KB_SIZE
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
** ��������: cskyCacheEnable
** ��������: C-SKY ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyICacheEnableHw();                                           /*  ʹ�� ICACHE                 */

        cskyBranchPredictionEnable();

    } else {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyDCacheEnableHw(uiCacheCfg);                                 /*  ʹ�� DCACHE                 */
    }

#if LW_CFG_CSKY_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        cskyL2Enable();
    }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheDisable
** ��������: C-SKY ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : DCACHE Ϊд��͸ģʽ, ���û�д.
*********************************************************************************************************/
static INT  cskyCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyICacheDisableHw();                                          /*  ���� ICACHE                 */

        cskyBranchPredictionDisable();

    } else {
#if LW_CFG_CSKY_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        cskyDCacheDisableHw();                                          /*  ���� DCACHE                 */
    }

#if LW_CFG_CSKY_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        cskyL2Disable();
    }
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheOp
** ��������: CACHE ����
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
**           uiVal          ��������
**           pCacheOp       ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyCacheOp (PVOID        pvStart,
                          PVOID        pvEnd,
                          size_t       uiStep,
                          UINT32       uiVal,
                          VOIDFUNCPTR  pCacheOp)
{    
    UINT32   uiCr22Value      = (UINT32)pvStart;
    UINT32   uiCr17Value      = M_CFR_OMS | uiVal;
    UINT32   uiCr17ReadValue  = 0;
    UINT32   uiTmp            = 0;
    UINT32   i;
   
    SET_CIR(uiCr22Value);                                               /*  д����ʼ�����ַ            */
    GET_CFR(uiCr17Value, uiCr17ReadValue, uiTmp);
    if (unlikely((uiCr17ReadValue & M_CFR_LICF) != 0)) {                /*  �����˲����쳣              */
#if LW_CFG_CSKY_HARD_TLB_REFILL > 0
        pCacheOp();
        return;
#else
        LDW_ADDR(pvStart, uiTmp);
#endif
    }

    uiCr22Value = (UINT32)pvEnd - 1;                                    /*  д����������ַ            */
    SET_CIR(uiCr22Value);
    GET_CFR(uiCr17Value, uiCr17ReadValue, uiTmp);
    if (unlikely((uiCr17ReadValue & M_CFR_LICF) != 0)) {                /*  �����˲����쳣              */
#if LW_CFG_CSKY_HARD_TLB_REFILL > 0
        pCacheOp();
        return;
#else
        LDW_ADDR(pvEnd - 1, uiTmp);
#endif
    }
    
    for (i = (UINT32)pvStart; i < (UINT32)pvEnd; i += uiStep) {
       SET_CIR(i);
       SET_CFR(uiCr17Value);
    }
}
/*********************************************************************************************************
** ��������: cskyDCacheFlush
** ��������: DCACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyDCacheFlush (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_CLR,
                cskyDCacheFlushAll);
}
/*********************************************************************************************************
** ��������: cskyDCacheClear
** ��������: DCACHE �����ݻ�д����Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyDCacheClear (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_CLR | M_CFR_INV,
                cskyDCacheClearAll);
}
/*********************************************************************************************************
** ��������: cskyDCacheInvalidate
** ��������: DCACHE ����������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyDCacheInvalidate (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{    
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_D | M_CFR_INV,
                cskyDCacheInvalidateAll);
}
/*********************************************************************************************************
** ��������: cskyICacheInvalidate
** ��������: ICACHE ����������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  cskyICacheInvalidate (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    cskyCacheOp(pvStart,
                pvEnd,
                uiStep,
                B_CFR_CACHE_I | M_CFR_INV,
                cskyICacheInvalidateAll);
}
/*********************************************************************************************************
** ��������: cskyCacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
INT  cskyCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2FlushAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д                    */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT  cskyCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                cskyDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2Invalidate(pvAdrs, stBytes);                          /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulStart, (PVOID)ulStart, CSKY_DCACHE_LINE_SIZE);
                ulStart += CSKY_DCACHE_LINE_SIZE;
            }

            if (ulEnd & ((addr_t)CSKY_DCACHE_LINE_SIZE - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)CSKY_DCACHE_LINE_SIZE - 1);
                cskyDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                cskyDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);
            }

#if LW_CFG_CSKY_CACHE_L2 > 0
            cskyL2Invalidate(pvPdrs, stBytes);                          /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT  cskyCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  ȫ����д����Ч              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2ClearAll();
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyICacheInvalidateAll();                                  /*  ICACHE ȫ����Ч             */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
            cskyICacheInvalidate(pvAdrs, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  ȫ����д����Ч              */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_CSKY_CACHE_L2 > 0
        cskyL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_CSKY_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: cskyCacheUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: cskyCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  cskyCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
        cskyDCacheClearAll();                                           /*  DCACHE ȫ����д             */
        cskyICacheInvalidateAll();                                      /*  ICACHE ȫ����Ч             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
        cskyDCacheFlush(pvAdrs, (PVOID)ulEnd, CSKY_DCACHE_LINE_SIZE);

        CSKY_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, CSKY_ICACHE_LINE_SIZE);
        cskyICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, CSKY_ICACHE_LINE_SIZE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  cskyCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheFlushAll();                                       /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheFlush(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д                    */
        }

    } else {
        if (stBytes >= CSKY_L1_CACHE_LOOP_OP_MAX_SIZE) {
            cskyDCacheClearAll();                                       /*  ȫ����д                    */

        } else {
            CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, CSKY_DCACHE_LINE_SIZE);
            cskyDCacheClear(pvAdrs, (PVOID)ulEnd,
                            CSKY_DCACHE_LINE_SIZE);                     /*  ���ֻ�д                    */
        }
    }

    return  (ERROR_NONE);
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
VOID  cskyCacheInit (LW_CACHE_OP *pcacheop,
                     CACHE_MODE   uiInstruction,
                     CACHE_MODE   uiData,
                     CPCHAR       pcMachineName)
{
    if (uiData & CACHE_COPYBACK) {
        uiCacheCfg |= M_CACHE_CFG_WB;
    }

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
    if (LW_NCPUS > 1) {
        uiCacheCfg |= M_CACHE_CFG_WA;                                   /*  ���ʹ�� CACHE д����       */
    }
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    pcacheop->CACHEOP_iICacheLine = CSKY_ICACHE_LINE_SIZE;
    pcacheop->CACHEOP_iDCacheLine = CSKY_DCACHE_LINE_SIZE;

    pcacheop->CACHEOP_iICacheWaySize = CSKY_ICACHE_WAY_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize = CSKY_ICACHE_WAY_SIZE;

    pcacheop->CACHEOP_pfuncEnable  = cskyCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = cskyCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = cskyCacheLock;                      /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = cskyCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = cskyCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = cskyCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = cskyCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = cskyCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = cskyCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = cskyCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = cskyCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = cskyCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: cskyCacheReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  cskyCacheReset (CPCHAR  pcMachineName)
{
    cskyICacheInvalidateAll();
    cskyDCacheDisableHw();
    cskyICacheDisableHw();
    cskyBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_CSKY_ARCH_CK803__*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
