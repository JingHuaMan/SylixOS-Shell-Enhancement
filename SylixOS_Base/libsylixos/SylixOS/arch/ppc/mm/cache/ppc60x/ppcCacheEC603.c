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
** ��   ��   ��: ppcCacheEC603.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC EC603 ��ϵ���� CACHE ����.
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
extern VOID     ppcEC603DCacheDisable(VOID);
extern VOID     ppcEC603DCacheEnable(VOID);
extern VOID     ppcEC603ICacheDisable(VOID);
extern VOID     ppcEC603ICacheEnable(VOID);

extern VOID     ppcEC603DCacheClearAll(VOID);
extern VOID     ppcEC603DCacheFlushAll(VOID);
extern VOID     ppcEC603ICacheInvalidateAll(VOID);

extern VOID     ppcEC603DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppcEC603BranchPredictionDisable(VOID);
extern VOID     ppcEC603BranchPredictionEnable(VOID);
extern VOID     ppcEC603BranchPredictorInvalidate(VOID);

extern VOID     ppcEC603TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                   UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: ppcEC603CacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   ppcEC603CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    /*
     * TODO ��ʱ��֧��
     */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  EC603 CACHE ����
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppcEC603CacheDriver = {
    "EC603",
    ppcEC603CacheProbe,

    ppcEC603DCacheDisable,
    ppcEC603DCacheEnable,
    ppcEC603ICacheDisable,
    ppcEC603ICacheEnable,

    ppcEC603DCacheClearAll,
    ppcEC603DCacheFlushAll,
    ppcEC603ICacheInvalidateAll,

    ppcEC603DCacheClear,
    ppcEC603DCacheFlush,
    ppcEC603DCacheInvalidate,
    ppcEC603ICacheInvalidate,

    ppcEC603BranchPredictionDisable,
    ppcEC603BranchPredictionEnable,
    ppcEC603BranchPredictorInvalidate,

    ppcEC603TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
