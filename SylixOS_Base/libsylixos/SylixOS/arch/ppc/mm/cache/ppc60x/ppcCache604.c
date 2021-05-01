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
** ��   ��   ��: ppcCache604.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC 604 ��ϵ���� CACHE ����.
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
extern VOID     ppc604DCacheDisable(VOID);
extern VOID     ppc604DCacheEnable(VOID);
extern VOID     ppc604ICacheDisable(VOID);
extern VOID     ppc604ICacheEnable(VOID);

extern VOID     ppc604DCacheClearAll(VOID);
extern VOID     ppc604DCacheFlushAll(VOID);
extern VOID     ppc604ICacheInvalidateAll(VOID);

extern VOID     ppc604DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc604BranchPredictionDisable(VOID);
extern VOID     ppc604BranchPredictionEnable(VOID);
extern VOID     ppc604BranchPredictorInvalidate(VOID);

extern VOID     ppc604TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                 UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: ppc604CacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   ppc604CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
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
  604 CACHE ����
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc604CacheDriver = {
    "604",
    ppc604CacheProbe,

    ppc604DCacheDisable,
    ppc604DCacheEnable,
    ppc604ICacheDisable,
    ppc604ICacheEnable,

    ppc604DCacheClearAll,
    ppc604DCacheFlushAll,
    ppc604ICacheInvalidateAll,

    ppc604DCacheClear,
    ppc604DCacheFlush,
    ppc604DCacheInvalidate,
    ppc604ICacheInvalidate,

    ppc604BranchPredictionDisable,
    ppc604BranchPredictionEnable,
    ppc604BranchPredictorInvalidate,

    ppc604TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
