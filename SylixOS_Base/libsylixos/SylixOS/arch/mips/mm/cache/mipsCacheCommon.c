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
** ��   ��   ��: mipsCacheCommon.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 01 ��
**
** ��        ��: MIPS ��ϵ���� CACHE ����.
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
  CACHE ��Ϣ
*********************************************************************************************************/
MIPS_CACHE  _G_ICache, _G_DCache;                                       /*  ICACHE �� DCACHE ��Ϣ       */
MIPS_CACHE  _G_VCache, _G_SCache;                                       /*  VCACHE �� SCACHE ��Ϣ       */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
BOOL        _G_bHaveHitWritebackS = LW_FALSE;                           /*  �Ƿ��� HitWritebackS ����   */
BOOL        _G_bHaveHitWritebackD = LW_FALSE;                           /*  �Ƿ��� HitWritebackD ����   */
BOOL        _G_bHaveFillI         = LW_FALSE;                           /*  �Ƿ��� FillI ����           */
BOOL        _G_bHaveTagHi         = LW_FALSE;                           /*  �Ƿ��� TagHi �Ĵ���         */
BOOL        _G_bHaveECC           = LW_FALSE;                           /*  �Ƿ��� ECC �Ĵ���           */
UINT32      _G_uiEccValue         = 0;                                  /*  ECC �Ĵ�����ֵ              */
/*********************************************************************************************************
** ��������: mipsPCacheProbe
** ��������: MIPS PCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsPCacheProbe (VOID)
{
    UINT32  uiConfig = mipsCp0ConfigRead();
    UINT32  uiPRId   = mipsCp0PRIdRead();
    UINT32  uiConfig1;
    UINT32  uiLineSize;

    _G_ICache.CACHE_bPresent = LW_FALSE;
    _G_DCache.CACHE_bPresent = LW_FALSE;

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
        _G_ICache.CACHE_uiSize = 1 << (12 + ((uiConfig & CONF_IC) >> 9));
        _G_ICache.CACHE_uiLineSize = 16 << ((uiConfig & CONF_IB) >> 5);
        if (uiPRId & 0x3) {
            _G_ICache.CACHE_uiWayNr = 4;
        } else {
            _G_ICache.CACHE_uiWayNr = 2;
        }
        _G_ICache.CACHE_uiWayBit = 0;
        _G_ICache.CACHE_bPresent = LW_TRUE;

        _G_DCache.CACHE_uiSize = 1 << (12 + ((uiConfig & CONF_DC) >> 6));
        _G_DCache.CACHE_uiLineSize = 16 << ((uiConfig & CONF_DB) >> 4);
        if (uiPRId & 0x3) {
            _G_DCache.CACHE_uiWayNr = 4;
        } else {
            _G_DCache.CACHE_uiWayNr = 2;
        }
        _G_DCache.CACHE_uiWayBit = 0;
        _G_DCache.CACHE_bPresent = LW_TRUE;
        break;

    case CPU_LOONGSON3:
    case CPU_LOONGSON2K:
    case CPU_CETC_HR2:
        uiConfig1 = mipsCp0Config1Read();
        uiLineSize = (uiConfig1 >> 19) & 7;
        if (uiLineSize) {
            _G_ICache.CACHE_uiLineSize = 2 << uiLineSize;
        } else {
            _G_ICache.CACHE_uiLineSize = 0;
        }
        _G_ICache.CACHE_uiSetNr = 64 << ((uiConfig1 >> 22) & 7);
        _G_ICache.CACHE_uiWayNr = 1 + ((uiConfig1 >> 16) & 7);
        _G_ICache.CACHE_uiSize  = _G_ICache.CACHE_uiSetNr *
                                  _G_ICache.CACHE_uiWayNr *
                                  _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayBit = 0;
        _G_ICache.CACHE_bPresent = LW_TRUE;

        uiLineSize = (uiConfig1 >> 10) & 7;
        if (uiLineSize) {
            _G_DCache.CACHE_uiLineSize = 2 << uiLineSize;
        } else {
            _G_DCache.CACHE_uiLineSize = 0;
        }
        _G_DCache.CACHE_uiSetNr = 64 << ((uiConfig1 >> 13) & 7);
        _G_DCache.CACHE_uiWayNr = 1 + ((uiConfig1 >> 7) & 7);
        _G_DCache.CACHE_uiSize  = _G_DCache.CACHE_uiSetNr *
                                  _G_DCache.CACHE_uiWayNr *
                                  _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayBit = 0;
        _G_DCache.CACHE_bPresent = LW_TRUE;
        break;

    default:
        if (!(uiConfig & MIPS_CONF_M)) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "Don't know how to probe P-caches on this cpu.\r\n");
            return;
        }

        /*
         * So we seem to be a MIPS32 or MIPS64 CPU
         * So let's probe the I-cache ...
         */
        uiConfig1 = mipsCp0Config1Read();

        uiLineSize = (uiConfig1 >> 19) & 7;

        /*
         * IL == 7 is reserved
         */
        if (uiLineSize == 7) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "Invalid icache line size\r\n");
            return;
        }

        _G_ICache.CACHE_uiLineSize = uiLineSize ? 2 << uiLineSize : 0;
        _G_ICache.CACHE_uiSetNr    = 32 << (((uiConfig1 >> 22) + 1) & 7);
        _G_ICache.CACHE_uiWayNr    = 1 + ((uiConfig1 >> 16) & 7);
        _G_ICache.CACHE_uiSize     = _G_ICache.CACHE_uiSetNr *
                                     _G_ICache.CACHE_uiWayNr *
                                     _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayBit   = lib_ffs(_G_ICache.CACHE_uiSize / _G_ICache.CACHE_uiWayNr) - 1;
        _G_ICache.CACHE_bPresent   = LW_TRUE;

        /*
         * Now probe the MIPS32 / MIPS64 data cache.
         */
        uiLineSize = (uiConfig1 >> 10) & 7;

        /*
         * DL == 7 is reserved
         */
        if (uiLineSize == 7) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "Invalid dcache line size\r\n");
            return;
        }

        _G_DCache.CACHE_uiLineSize = uiLineSize ? 2 << uiLineSize : 0;
        _G_DCache.CACHE_uiSetNr    = 32 << (((uiConfig1 >> 13) + 1) & 7);
        _G_DCache.CACHE_uiWayNr    = 1 + ((uiConfig1 >> 7) & 7);
        _G_DCache.CACHE_uiSize     = _G_DCache.CACHE_uiSetNr *
                                     _G_DCache.CACHE_uiWayNr *
                                     _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayBit   = lib_ffs(_G_DCache.CACHE_uiSize / _G_DCache.CACHE_uiWayNr) - 1;
        _G_DCache.CACHE_bPresent   = LW_TRUE;
        break;
    }

    /*
     * Compute a couple of other cache variables
     */
    _G_ICache.CACHE_uiWaySize = _G_ICache.CACHE_uiSize / _G_ICache.CACHE_uiWayNr;
    _G_DCache.CACHE_uiWaySize = _G_DCache.CACHE_uiSize / _G_DCache.CACHE_uiWayNr;

    _G_ICache.CACHE_uiSetNr = _G_ICache.CACHE_uiLineSize ?
                              _G_ICache.CACHE_uiSize / (_G_ICache.CACHE_uiLineSize * _G_ICache.CACHE_uiWayNr) : 0;
    _G_DCache.CACHE_uiSetNr = _G_DCache.CACHE_uiLineSize ?
                              _G_DCache.CACHE_uiSize / (_G_DCache.CACHE_uiLineSize * _G_DCache.CACHE_uiWayNr) : 0;

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
        /*
         * LOONGSON2 has 4 way icache, but when using indexed cache op,
         * one op will act on all 4 ways
         */
        _G_ICache.CACHE_uiWayNr = 1;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mipsVCacheProbe
** ��������: MIPS VCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsVCacheProbe (VOID)
{
    UINT32  uiConfig2;
    UINT32  uiLineSize;

    _G_VCache.CACHE_bPresent = LW_FALSE;

    if ((_G_uiMipsCpuType != CPU_LOONGSON3) &&                          /*  Loongson-3x/2G/2H           */
        (_G_uiMipsCpuType != CPU_LOONGSON2K)) {                         /*  Loongson-2K                 */
        return;
    }

    uiConfig2 = mipsCp0Config2Read();
    if ((uiLineSize = ((uiConfig2 >> 20) & 15))) {
        _G_VCache.CACHE_uiLineSize = 2 << uiLineSize;

    } else {
        _G_VCache.CACHE_uiLineSize = uiLineSize;
    }

    _G_VCache.CACHE_uiSetNr   = 64 << ((uiConfig2 >> 24) & 15);
    _G_VCache.CACHE_uiWayNr   = 1 + ((uiConfig2 >> 16) & 15);
    _G_VCache.CACHE_uiSize    = _G_VCache.CACHE_uiSetNr * _G_VCache.CACHE_uiWayNr * _G_VCache.CACHE_uiLineSize;
    _G_VCache.CACHE_uiWayBit  = 0;
    _G_VCache.CACHE_uiWaySize = _G_VCache.CACHE_uiSize / _G_VCache.CACHE_uiWayNr;
    _G_VCache.CACHE_bPresent  = LW_TRUE;
}
/*********************************************************************************************************
** ��������: loongson2SCacheInit
** ��������: loongson2(2E/2F) SCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  loongson2SCacheInit (VOID)
{
    _G_SCache.CACHE_uiSize     = 512 * 1024;
    _G_SCache.CACHE_uiLineSize = 32;
    _G_SCache.CACHE_uiWayNr    = 4;
    _G_SCache.CACHE_uiWayBit   = 0;
    _G_SCache.CACHE_uiWaySize  = _G_SCache.CACHE_uiSize / (_G_SCache.CACHE_uiWayNr);
    _G_SCache.CACHE_uiSetNr    = _G_SCache.CACHE_uiSize / (_G_SCache.CACHE_uiLineSize * _G_SCache.CACHE_uiWayNr);
    _G_SCache.CACHE_bPresent   = LW_TRUE;
}
/*********************************************************************************************************
** ��������: loongson3SCacheInit
** ��������: loongson3(2G/2H/3x) SCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  loongson3SCacheInit (VOID)
{
    UINT32  uiConfig2, uiLineSize;

    uiConfig2 = mipsCp0Config2Read();
    uiLineSize = (uiConfig2 >> 4) & 15;
    if (uiLineSize) {
        _G_SCache.CACHE_uiLineSize = 2 << uiLineSize;
    } else {
        _G_SCache.CACHE_uiLineSize = 0;
    }
    _G_SCache.CACHE_uiSetNr = 64 << ((uiConfig2 >> 8) & 15);
    _G_SCache.CACHE_uiWayNr = 1 + (uiConfig2 & 15);
    _G_SCache.CACHE_uiSize  = _G_SCache.CACHE_uiSetNr *
                              _G_SCache.CACHE_uiWayNr *
                              _G_SCache.CACHE_uiLineSize;

    if (_G_uiMipsCpuType != CPU_LOONGSON2K) {
        /*
         * Loongson-3x has 4 cores, 1MB scache for each. scaches are shared
         */
        _G_SCache.CACHE_uiSize *= 4;
    }

    _G_SCache.CACHE_uiWayBit  = 0;
    _G_SCache.CACHE_uiWaySize = _G_SCache.CACHE_uiSize / _G_SCache.CACHE_uiWayNr;

    if (_G_SCache.CACHE_uiSize) {
        _G_SCache.CACHE_bPresent = LW_TRUE;
    }
}
/*********************************************************************************************************
** ��������: mipsSCacheInit
** ��������: MIPS SCACHE init
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsSCacheInit (VOID)
{
    UINT32  uiConfig0, uiConfig1, uiConfig2;
    UINT32  uiTemp;

    uiConfig0 = mipsCp0ConfigRead();                                    /*  �� Config0                  */
    if (!(uiConfig0 & MIPS_CONF_M)) {                                   /*  �� Config1, �˳�            */
        return;
    }
    uiConfig1 = mipsCp0Config1Read();                                   /*  �� Config1                  */
    if (!(uiConfig1 & MIPS_CONF_M)) {                                   /*  �� Config2, �˳�            */
        return;
    }

    uiConfig2 = mipsCp0Config2Read();                                   /*  �� Config2                  */
    if (uiConfig2 & (1 << 12)) {                                        /*  Check the bypass bit (L2B)  */
        return;
    }

    uiTemp = (uiConfig2 >> 4) & 0x0f;
    if ((0 == uiTemp) || (uiTemp > 7)) {
        return;
    }
    _G_SCache.CACHE_uiLineSize = 2 << uiTemp;

    uiTemp = (uiConfig2 >> 8) & 0x0f;
    if (uiTemp > 7) {
        return;
    }
    _G_SCache.CACHE_uiSetNr = 64 << uiTemp;

    uiTemp = (uiConfig2 >> 0) & 0x0f;
    if (uiTemp > 7) {
        return;
    }
    _G_SCache.CACHE_uiWayNr = uiTemp + 1;

    _G_SCache.CACHE_uiWaySize = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;
    _G_SCache.CACHE_uiWayBit  = lib_ffs(_G_SCache.CACHE_uiWaySize) - 1;
    _G_SCache.CACHE_uiSize    = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                _G_SCache.CACHE_uiLineSize;
    _G_SCache.CACHE_bPresent  = LW_TRUE;
}
/*********************************************************************************************************
** ��������: mipsSCacheSetup
** ��������: MIPS SCACHE Setup
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsSCacheSetup (VOID)
{
    _G_SCache.CACHE_bPresent = LW_FALSE;

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
        loongson2SCacheInit();
        break;

    case CPU_LOONGSON3:
    case CPU_LOONGSON2K:
    case CPU_CETC_HR2:
        loongson3SCacheInit();
        break;

    default:
        mipsSCacheInit();
        break;
    }
}
/*********************************************************************************************************
** ��������: mipsCacheInfoShow
** ��������: CACHE ��Ϣ��ӡ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheInfoShow (VOID)
{
    INT  iLevel = 2;

    if (_G_ICache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 I-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_ICache.CACHE_uiSize / 1024,
                     _G_ICache.CACHE_uiLineSize,
                     _G_ICache.CACHE_uiWayNr,
                     _G_ICache.CACHE_uiSetNr);
    }

    if (_G_DCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 D-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_DCache.CACHE_uiSize / 1024,
                     _G_DCache.CACHE_uiLineSize,
                     _G_DCache.CACHE_uiWayNr,
                     _G_DCache.CACHE_uiSetNr);
    }

    if (_G_VCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L2 V-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_VCache.CACHE_uiSize / 1024,
                     _G_VCache.CACHE_uiLineSize,
                     _G_VCache.CACHE_uiWayNr,
                     _G_VCache.CACHE_uiSetNr);
        iLevel++;
    }

    if (_G_SCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L%d S-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     iLevel,
                     _G_SCache.CACHE_uiSize / 1024,
                     _G_SCache.CACHE_uiLineSize,
                     _G_SCache.CACHE_uiWayNr,
                     _G_SCache.CACHE_uiSetNr);
    }
}
/*********************************************************************************************************
** ��������: mipsCacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName     ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheProbe (CPCHAR  pcMachineName)
{
    static  BOOL  bIsProbed = LW_FALSE;

    if (bIsProbed) {
        return;
    }
    bIsProbed = LW_TRUE;

    mipsCpuProbe(pcMachineName);                                        /*  MIPS CPU ̽��               */

    switch (_G_uiMipsCpuType) {

    /*
     * ����
     * Loongson1B_processor_user_manual_V2.2 PAGE31
     * Loongson2F_UM_CN_V1.5 PAGE90
     * Loongson_3A1000_cpu_user_2 PAGE24 ���ĵ�
     * Loongson-1x/2x/3x ���� HitWritebackS HitWritebackD FillI ����
     */
    case CPU_LOONGSON1:                                                 /*  Loongson-1x                 */
        _G_bHaveTagHi = LW_FALSE;
        _G_bHaveECC   = LW_FALSE;
        break;

    case CPU_LOONGSON2:                                                 /*  Loongson-2E/2F              */
        _G_bHaveTagHi = LW_TRUE;
        _G_bHaveECC   = LW_FALSE;
        break;

    case CPU_LOONGSON3:                                                 /*  Loongson-2G/2H/3x           */
    case CPU_LOONGSON2K:                                                /*  Loongson-2K                 */
    case CPU_CETC_HR2:                                                  /*  CETC-HR2                    */
        _G_bHaveTagHi = LW_TRUE;
        _G_bHaveECC   = LW_TRUE;
        _G_uiEccValue = 0x22;                                           /*  ECC ��ֵ                    */
        break;

    case CPU_JZRISC:                                                    /*  ���� CPU                    */
        _G_bHaveTagHi = LW_FALSE;
        _G_bHaveECC   = LW_TRUE;
        _G_uiEccValue = 0;                                              /*  ���� L1C ��ָ�Ӱ�� L2C   */
        _G_bHaveFillI = LW_TRUE;
        _G_bHaveHitWritebackD = LW_TRUE;
        /*
         * TODO: _G_bHaveHitWritebackS ??
         */
        break;

    default:
        break;
    }

    mipsPCacheProbe();
    mipsVCacheProbe();
    mipsSCacheSetup();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
