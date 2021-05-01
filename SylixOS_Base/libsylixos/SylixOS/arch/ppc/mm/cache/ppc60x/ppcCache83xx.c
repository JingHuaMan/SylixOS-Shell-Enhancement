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
** ��   ��   ��: ppcCache83xx.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC MPC83XX ��ϵ���� CACHE ����.
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
extern VOID     ppc83xxDCacheDisable(VOID);
extern VOID     ppc83xxDCacheEnable(VOID);
extern VOID     ppc83xxICacheDisable(VOID);
extern VOID     ppc83xxICacheEnable(VOID);

extern VOID     ppc83xxDCacheClearAll(VOID);
extern VOID     ppc83xxDCacheFlushAll(VOID);
extern VOID     ppc83xxICacheInvalidateAll(VOID);

extern VOID     ppc83xxDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc83xxBranchPredictionDisable(VOID);
extern VOID     ppc83xxBranchPredictionEnable(VOID);
extern VOID     ppc83xxBranchPredictorInvalidate(VOID);

extern VOID     ppc83xxTextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: ppc83xxCacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   ppc83xxCacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0) {
        pICache->CACHE_uiLineSize  = 32;
        pICache->CACHE_uiWayNr     = 8;
        pICache->CACHE_uiSetNr     = 128;
        pICache->CACHE_uiSize      = pICache->CACHE_uiSetNr * pICache->CACHE_uiWayNr * \
                                     pICache->CACHE_uiLineSize;
        pICache->CACHE_uiWaySize   = pICache->CACHE_uiSetNr * pICache->CACHE_uiLineSize;

        pDCache->CACHE_uiLineSize  = 32;
        pDCache->CACHE_uiWayNr     = 8;
        pDCache->CACHE_uiSetNr     = 128;
        pDCache->CACHE_uiSize      = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiWayNr * \
                                     pDCache->CACHE_uiLineSize;
        pDCache->CACHE_uiWaySize   = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  MPC83XX CACHE ����
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc83xxCacheDriver = {
    "83XX",
    ppc83xxCacheProbe,

    ppc83xxDCacheDisable,
    ppc83xxDCacheEnable,
    ppc83xxICacheDisable,
    ppc83xxICacheEnable,

    ppc83xxDCacheClearAll,
    ppc83xxDCacheFlushAll,
    ppc83xxICacheInvalidateAll,

    ppc83xxDCacheClear,
    ppc83xxDCacheFlush,
    ppc83xxDCacheInvalidate,
    ppc83xxICacheInvalidate,

    ppc83xxBranchPredictionDisable,
    ppc83xxBranchPredictionEnable,
    ppc83xxBranchPredictorInvalidate,

    ppc83xxTextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
