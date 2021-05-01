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
** ��   ��   ��: ppcCache.c
**
** ��   ��   ��: Yang.HaiFeng (���)
**
** �ļ���������: 2016 �� 01 �� 18 ��
**
** ��        ��: PowerPC ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache.h"
/*********************************************************************************************************
  L2 CACHE ֧��
*********************************************************************************************************/
#if LW_CFG_PPC_CACHE_L2 > 0
#include "../l2/ppcL2.h"
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define PPC_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT              _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
static PPC_CACHE        _G_ICache, _G_DCache;                           /*  ICACHE �� DCACHE ��Ϣ       */
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
UINT8                  *_G_pucPpcCacheReadBuffer = (UINT8 *)0x10000;
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
extern PPC_L1C_DRIVER   _G_ppc460CacheDriver;
extern PPC_L1C_DRIVER   _G_ppc603CacheDriver;
extern PPC_L1C_DRIVER   _G_ppc604CacheDriver;
extern PPC_L1C_DRIVER   _G_ppc745xCacheDriver;
extern PPC_L1C_DRIVER   _G_ppc83xxCacheDriver;
extern PPC_L1C_DRIVER   _G_ppcEC603CacheDriver;
extern PPC_L1C_DRIVER   _G_ppcE200CacheDriver;
extern PPC_L1C_DRIVER   _G_ppcE500CacheDriver;

static PPC_L1C_DRIVER  *_G_ppcCacheDrivers[] = {
    &_G_ppc460CacheDriver,
    &_G_ppc603CacheDriver,
    &_G_ppc604CacheDriver,
    &_G_ppc745xCacheDriver,
    &_G_ppc83xxCacheDriver,
    &_G_ppcEC603CacheDriver,
    &_G_ppcE200CacheDriver,
    &_G_ppcE500CacheDriver,
    LW_NULL,
};

static PPC_L1C_DRIVER  *_G_pcachedriver = LW_NULL;
/*********************************************************************************************************
  ���� CACHE ������
*********************************************************************************************************/
#define ppcDCacheDisable                 _G_pcachedriver->L1CD_pfuncDCacheDisable
#define ppcDCacheEnable                  _G_pcachedriver->L1CD_pfuncDCacheEnable

#define ppcICacheDisable                 _G_pcachedriver->L1CD_pfuncICacheDisable
#define ppcICacheEnable                  _G_pcachedriver->L1CD_pfuncICacheEnable

/*
 * ������������ All ����ʱ, �������Ƿ���Ч
 */
#define ppcDCacheClearAll                _G_pcachedriver->L1CD_pfuncDCacheClearAll
#define ppcDCacheFlushAll                _G_pcachedriver->L1CD_pfuncDCacheFlushAll
#define ppcICacheInvalidateAll           _G_pcachedriver->L1CD_pfuncICacheInvalidateAll

#define ppcDCacheClear                   _G_pcachedriver->L1CD_pfuncDCacheClear
#define ppcDCacheFlush                   _G_pcachedriver->L1CD_pfuncDCacheFlush
#define ppcDCacheInvalidate              _G_pcachedriver->L1CD_pfuncDCacheInvalidate
#define ppcICacheInvalidate              _G_pcachedriver->L1CD_pfuncICacheInvalidate

#define ppcBranchPredictionDisable       _G_pcachedriver->L1CD_pfuncBranchPredictionDisable
#define ppcBranchPredictionEnable        _G_pcachedriver->L1CD_pfuncBranchPredictionEnable
#define ppcBranchPredictorInvalidate     _G_pcachedriver->L1CD_pfuncBranchPredictorInvalidate

#define ppcTextUpdate                    _G_pcachedriver->L1CD_pfuncTextUpdate
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: ppcCacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_I_EN;
        }
        ppcICacheEnable();

        ppcBranchPredictionEnable();

    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_D_EN;
        }
        ppcDCacheEnable();
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (_G_iCacheStatus == L1_CACHE_EN)) {
        ppcL2Enable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_I_EN;
        }
        ppcICacheDisable();
        ppcBranchPredictionDisable();

    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_D_EN;
        }
        ppcDCacheDisable();
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (_G_iCacheStatus == L1_CACHE_DIS)) {
        ppcL2Disable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT  ppcCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,                        /*  ���ֻ�д                    */
                           _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2FlushAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,                        /*  ���ֻ�д                    */
                           _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT  ppcCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                ppcDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvAdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheInvalidatePage (LW_CACHE_TYPE    cachetype,
                                    PVOID            pvAdrs,
                                    PVOID            pvPdrs,
                                    size_t           stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                ppcDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvPdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT  ppcCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  ȫ����д����Ч              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2ClearAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  ȫ����д����Ч              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppcCacheUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppcCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ppcCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == sizeof(addr_t)) {
        stBytes = _G_DCache.CACHE_uiLineSize << 2;                      /*  XXX ?                       */
    }

    if (ppcTextUpdate) {
        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppcTextUpdate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
        return  (ERROR_NONE);
    }

    if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) &&
        ppcDCacheFlushAll && ppcICacheInvalidateAll) {
        ppcDCacheFlushAll();                                            /*  DCACHE ȫ����д             */
        ppcICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,
                       _G_DCache.CACHE_uiLineSize);                     /*  ���ֻ�д                    */

        PPC_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        ppcICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
INT  ppcCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д                    */
        }

    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheProbe
** ��������: CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcCacheProbe (CPCHAR       pcMachineName)
{
    static BOOL             bProbed = LW_FALSE;
    LW_MMU_PHYSICAL_DESC    phydescText;
    INT                     i;

    if (bProbed) {
        return  (ERROR_NONE);
    }

    i = 0;
    while (1) {
        _G_pcachedriver = _G_ppcCacheDrivers[i];

        if (_G_pcachedriver) {
            if (_G_pcachedriver->L1CD_pfuncProbe(pcMachineName, &_G_ICache, &_G_DCache) == ERROR_NONE) {
                break;
            }
        } else {
            return  (PX_ERROR);
        }

        i++;
    }

    /*
     * _G_pucPpcCacheReadBuffer ����Ϊ text �εĿ�ʼ��ַ
     * ��Ϊ text ������ӳ��Ϳ� CACHE ��
     * ������������� CACHE ʱ, ��һ��������һ�����״̬
     */
    API_VmmPhysicalKernelDesc(&phydescText, LW_NULL);

    _G_pucPpcCacheReadBuffer = (UINT8 *)ROUND_UP(phydescText.PHYD_ulVirMap, LW_CFG_VMM_PAGE_SIZE);

    bProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcCacheInit (LW_CACHE_OP *pcacheop,
                   CACHE_MODE   uiInstruction,
                   CACHE_MODE   uiData,
                   CPCHAR       pcMachineName)
{
    INT     iError;

    iError = ppcCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return  (iError);
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    ppcL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  ���� TEXT_UPDATE_MP ѡ��    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWaySize;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWaySize;

    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC I-Cache line size = %d byte Way size = %d byte.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC D-Cache line size = %d byte Way size = %d byte.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = ppcCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ppcCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = ppcCacheLock;                       /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = ppcCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = ppcCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = ppcCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = ppcCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = ppcCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = ppcCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = ppcCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = ppcCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = ppcCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
*********************************************************************************************************/
INT  ppcCacheReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ppcCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return  (iError);
    }

    if (ppcICacheInvalidateAll) {
        ppcICacheInvalidateAll();
    }
    ppcDCacheDisable();
    ppcICacheDisable();
    ppcBranchPredictorInvalidate();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcCacheStatus
** ��������: ��� CACHE ״̬
** �䡡��  : NONE
** �䡡��  : CACHE ״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcCacheStatus (VOID)
{
    return  (_G_iCacheStatus);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
