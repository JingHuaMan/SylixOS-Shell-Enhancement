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
** ��   ��   ��: ppcCache460.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2019 �� 08 �� 21 ��
**
** ��        ��: PowerPC 460 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID     ppc460DCacheDisable(VOID);
extern VOID     ppc460DCacheEnable(VOID);
extern VOID     ppc460ICacheDisable(VOID);
extern VOID     ppc460ICacheEnable(VOID);

extern VOID     ppc460DCacheInvalidateAll(VOID);
extern VOID     ppc460ICacheInvalidateAll(VOID);

extern VOID     ppc460DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc460ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc460BranchPredictionDisable(VOID);
extern VOID     ppc460BranchPredictionEnable(VOID);
extern VOID     ppc460BranchPredictorInvalidate(VOID);

extern VOID     ppc460TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                 UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: bspCacheSetSize
** ��������: ��� CACHE Set ����Ŀ
** �䡡��  : NONE
** �䡡��  : CACHE Set ����Ŀ
** ȫ�ֱ���:
** ����ģ��:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspCacheSetSize (VOID)
{
    return  (8);                                                        /*  8 / 16                      */
}
/*********************************************************************************************************
** ��������: ppc460CacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   ppc460CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_460) == 0) {
        pICache->CACHE_uiLineSize  = 32;
        pICache->CACHE_uiWayNr     = 64;
        pICache->CACHE_uiSetNr     = bspCacheSetSize();
        pICache->CACHE_uiSize      = pICache->CACHE_uiSetNr * pICache->CACHE_uiWayNr * \
                                     pICache->CACHE_uiLineSize;
        pICache->CACHE_uiWaySize   = pICache->CACHE_uiSetNr * pICache->CACHE_uiLineSize;

        pDCache->CACHE_uiLineSize  = 32;
        pDCache->CACHE_uiWayNr     = 64;
        pDCache->CACHE_uiSetNr     = bspCacheSetSize();
        pDCache->CACHE_uiSize      = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiWayNr * \
                                     pDCache->CACHE_uiLineSize;
        pDCache->CACHE_uiWaySize   = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  460 CACHE ����
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc460CacheDriver = {
    "460",
    ppc460CacheProbe,

    ppc460DCacheDisable,
    ppc460DCacheEnable,
    ppc460ICacheDisable,
    ppc460ICacheEnable,

    LW_NULL,
    LW_NULL,
    ppc460ICacheInvalidateAll,

    ppc460DCacheClear,
    ppc460DCacheFlush,
    ppc460DCacheInvalidate,
    ppc460ICacheInvalidate,

    ppc460BranchPredictionDisable,
    ppc460BranchPredictionEnable,
    ppc460BranchPredictorInvalidate,

    ppc460TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
