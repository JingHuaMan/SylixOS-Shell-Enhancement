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
** ��   ��   ��: ppcCache745x.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC MPC745X ��ϵ���� CACHE ����.
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
extern VOID     ppc745xDCacheDisable(VOID);
extern VOID     ppc745xDCacheEnable(VOID);
extern VOID     ppc745xICacheDisable(VOID);
extern VOID     ppc745xICacheEnable(VOID);

extern VOID     ppc745xDCacheClearAll(VOID);
extern VOID     ppc745xDCacheFlushAll(VOID);
extern VOID     ppc745xICacheInvalidateAll(VOID);

extern VOID     ppc745xDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc745xBranchPredictionDisable(VOID);
extern VOID     ppc745xBranchPredictionEnable(VOID);
extern VOID     ppc745xBranchPredictorInvalidate(VOID);

extern VOID     ppc745xTextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: ppc745xCacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   ppc745xCacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    /*
     * TODO ��ʱ��֧��
     */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  MPC745X CACHE ����
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc745xCacheDriver = {
    "745X",
    ppc745xCacheProbe,

    ppc745xDCacheDisable,
    ppc745xDCacheEnable,
    ppc745xICacheDisable,
    ppc745xICacheEnable,

    ppc745xDCacheClearAll,
    ppc745xDCacheFlushAll,
    ppc745xICacheInvalidateAll,

    ppc745xDCacheClear,
    ppc745xDCacheFlush,
    ppc745xDCacheInvalidate,
    ppc745xICacheInvalidate,

    ppc745xBranchPredictionDisable,
    ppc745xBranchPredictionEnable,
    ppc745xBranchPredictorInvalidate,

    ppc745xTextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
